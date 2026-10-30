/*--------------------------------------------------------------------------*/
/*--------------------------- File Assignment.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Assignment [see Assignment.h], with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>

namespace ADS {

template< class W >
requires std::is_arithmetic_v< W >
Assignment< W >::Assignment( int n , std::vector< W > C )
 : f_n( n ) , f_C( std::move( C ) )
{
 ADS_PRE( ( n >= 0 ) &&
          ( f_C.size() == std::size_t( f_n ) * std::size_t( f_n ) ) );
 f_u.assign( f_n , W() );
 f_v.assign( f_n , W() );
 f_col.assign( f_n , -1 );
 f_row.assign( f_n , -1 );
 f_dirtyR.assign( f_n , 0 );
 f_dirtyC.assign( f_n , 0 );
 f_dist.assign( f_n , W() );
 f_pred.assign( f_n , -1 );
 f_scanned.assign( f_n , 0 );
 }

template< class W >
requires std::is_arithmetic_v< W >
W Assignment< W >::cost( int i , int j ) const
{
 ADS_PRE( ( i >= 0 ) && ( i < f_n ) && ( j >= 0 ) && ( j < f_n ) );
 return( f_C[ std::size_t( i ) * std::size_t( f_n ) + std::size_t( j ) ] );
 }

template< class W >
requires std::is_arithmetic_v< W >
void Assignment< W >::setCost( int i , int j , W c )
{
 ADS_PRE( ( i >= 0 ) && ( i < f_n ) && ( j >= 0 ) && ( j < f_n ) );
 W & x = f_C[ std::size_t( i ) * std::size_t( f_n ) + std::size_t( j ) ];
 if( ( x == c ) || ! f_ready ) {
  x = c;
  return;
  }
 f_cells.emplace_back( i , j );
 x = c;
 f_pending = true;
 }

template< class W >
requires std::is_arithmetic_v< W >
void Assignment< W >::setRow( int i , const std::vector< W > & R )
{
 ADS_PRE( ( i >= 0 ) && ( i < f_n ) && ( int( R.size() ) == f_n ) );
 std::copy( R.begin() , R.end() ,
            f_C.begin() + std::ptrdiff_t( i ) * f_n );
 if( f_ready && ! f_dirtyR[ i ] ) {
  f_dirtyR[ i ] = 1;
  f_DR.push_back( i );
  f_pending = true;
  }
 }

template< class W >
requires std::is_arithmetic_v< W >
void Assignment< W >::setColumn( int j , const std::vector< W > & K )
{
 ADS_PRE( ( j >= 0 ) && ( j < f_n ) && ( int( K.size() ) == f_n ) );
 for( int i = 0 ; i < f_n ; ++i )
  f_C[ std::size_t( i ) * std::size_t( f_n ) + std::size_t( j ) ] = K[ i ];
 if( f_ready && ! f_dirtyC[ j ] ) {
  f_dirtyC[ j ] = 1;
  f_DC.push_back( j );
  f_pending = true;
  }
 }

template< class W >
requires std::is_arithmetic_v< W >
void Assignment< W >::freeRow( int i )
{
 if( f_col[ i ] >= 0 ) {
  f_row[ f_col[ i ] ] = -1;
  f_col[ i ] = -1;
  }
 }

/*--------------------------------------------------------------------------*/

template< class W >
requires std::is_arithmetic_v< W >
bool Assignment< W >::augment( int i0 )
{
 const W INF = infinity< W >();
 const int n = f_n;
 std::fill( f_dist.begin() , f_dist.end() , INF );
 std::fill( f_scanned.begin() , f_scanned.end() , 0 );
 f_order.clear();
 W mu = W();
 int i = i0 , sink = -1;
 while( sink < 0 ) {
  const W * c = f_C.data() + std::size_t( i ) * std::size_t( n );
  const W ui = f_u[ i ];
  int best = -1;
  W low = INF;
  for( int j = 0 ; j < n ; ++j ) {
   if( f_scanned[ j ] )
    continue;
   if( c[ j ] != INF ) {
    const W r = mu + ( c[ j ] - ui - f_v[ j ] );
    if( r < f_dist[ j ] ) {
     f_dist[ j ] = r;
     f_pred[ j ] = i;
     }
    }
   if( ( f_dist[ j ] < low ) ||
       ( ( f_dist[ j ] == low ) && ( low != INF ) && ( f_row[ j ] < 0 ) &&
         ( best >= 0 ) && ( f_row[ best ] >= 0 ) ) ) {
    low = f_dist[ j ];
    best = j;
    }
   }
  if( ( best < 0 ) || ( low == INF ) )
   return( false );                         // no free column reached
  mu = low;
  f_scanned[ best ] = 1;
  f_order.push_back( best );
  if( f_row[ best ] < 0 )
   sink = best;
  else
   i = f_row[ best ];
  }
 // the potentials: the scanned columns and the rows of the tree
 f_u[ i0 ] += mu;
 for( int j : f_order )
  if( j != sink ) {
   f_u[ f_row[ j ] ] += mu - f_dist[ j ];
   f_v[ j ] -= mu - f_dist[ j ];
   }
 // the pairs flipped along the path
 for( int j = sink ; ; ) {
  const int r = f_pred[ j ];
  f_row[ j ] = r;
  std::swap( f_col[ r ] , j );
  if( r == i0 )
   break;
  }
 ++f_aug;
 return( true );
 }

template< class W >
requires std::is_arithmetic_v< W >
bool Assignment< W >::optimal( void ) const
{
 for( int i = 0 ; i < f_n ; ++i ) {
  for( int j = 0 ; j < f_n ; ++j )
   if( finite( i , j ) && ( reduced( i , j ) < W() ) ) {
    if constexpr( std::is_floating_point_v< W > ) {
     const W c = f_C[ std::size_t( i ) * std::size_t( f_n ) +
                      std::size_t( j ) ];
     const W tol = W( 1e-9 ) * ( W( 1 ) + ( c < 0 ? -c : c ) );
     if( reduced( i , j ) < -tol )
      return( false );
     }
    else
     return( false );
    }
  const int j = f_col[ i ];
  if( ( j >= 0 ) && ( ( f_row[ j ] != i ) || ! finite( i , j ) ) )
   return( false );
  if constexpr( ! std::is_floating_point_v< W > )
   if( ( j >= 0 ) && ( reduced( i , j ) != W() ) )
    return( false );
  }
 return( true );
 }

template< class W >
requires std::is_arithmetic_v< W >
void Assignment< W >::solve( void )
{
 const W INF = infinity< W >();
 const int n = f_n;
 if( f_ready && ! f_pending )
  return;
 // the single costs: harmless unless on a pair, or below the duals
 for( const auto & [ i , j ] : f_cells ) {
  if( f_dirtyR[ i ] || f_dirtyC[ j ] )
   continue;
  const bool pair = f_col[ i ] == j;
  if( pair || ( finite( i , j ) && ( reduced( i , j ) < W() ) ) ) {
   f_dirtyR[ i ] = 1;
   f_DR.push_back( i );
   }
  }
 f_cells.clear();
 const bool scratch = ! f_ready || ( 2 * ( f_DR.size() + f_DC.size() ) >
                                     std::size_t( n ) );
 int outcome;
 if( scratch ) {
  std::fill( f_v.begin() , f_v.end() , W() );
  std::fill( f_col.begin() , f_col.end() , -1 );
  std::fill( f_row.begin() , f_row.end() , -1 );
  for( int i = 0 ; i < n ; ++i ) {
   W m = INF;
   for( int j = 0 ; j < n ; ++j )
    if( finite( i , j ) && ( cost( i , j ) < m ) )
     m = cost( i , j );
   f_u[ i ] = m == INF ? W() : m;
   }
  outcome = 0;
  }
 else {
  for( int i : f_DR ) {
   freeRow( i );
   W m = INF;
   for( int j = 0 ; j < n ; ++j )
    if( finite( i , j ) && ( cost( i , j ) - f_v[ j ] < m ) )
     m = cost( i , j ) - f_v[ j ];
   f_u[ i ] = m == INF ? W() : m;
   }
  for( int j : f_DC ) {
   if( f_row[ j ] >= 0 )
    freeRow( f_row[ j ] );
   W m = INF;
   for( int i = 0 ; i < n ; ++i )
    if( finite( i , j ) && ( cost( i , j ) - f_u[ i ] < m ) )
     m = cost( i , j ) - f_u[ i ];
   f_v[ j ] = m == INF ? W() : m;
   }
  outcome = 2;
  }
 for( int i : f_DR )
  f_dirtyR[ i ] = 0;
 for( int j : f_DC )
  f_dirtyC[ j ] = 0;
 f_DR.clear();
 f_DC.clear();
 for( int i = 0 ; i < n ; ++i )
  if( f_col[ i ] < 0 ) {
   if( outcome == 2 )
    outcome = 1;
   augment( i );
   }
 f_ready = true;
 f_pending = false;
 f_outcome = outcome;
 ++f_count[ outcome ];
 ADS_AUDIT( optimal() );
 }

/*--------------------------------------------------------------------------*/

template< class W >
requires std::is_arithmetic_v< W >
int Assignment< W >::column( int i )
{
 ADS_PRE( ( i >= 0 ) && ( i < f_n ) );
 solve();
 return( f_col[ i ] );
 }

template< class W >
requires std::is_arithmetic_v< W >
int Assignment< W >::row( int j )
{
 ADS_PRE( ( j >= 0 ) && ( j < f_n ) );
 solve();
 return( f_row[ j ] );
 }

template< class W >
requires std::is_arithmetic_v< W >
bool Assignment< W >::perfect( void )
{
 solve();
 for( int i = 0 ; i < f_n ; ++i )
  if( f_col[ i ] < 0 )
   return( false );
 return( true );
 }

template< class W >
requires std::is_arithmetic_v< W >
W Assignment< W >::total( void )
{
 solve();
 W s = W();
 for( int i = 0 ; i < f_n ; ++i )
  if( f_col[ i ] >= 0 )
   s += cost( i , f_col[ i ] );
 return( s );
 }

template< class W >
requires std::is_arithmetic_v< W >
W Assignment< W >::rowPotential( int i )
{
 ADS_PRE( ( i >= 0 ) && ( i < f_n ) );
 solve();
 return( f_u[ i ] );
 }

template< class W >
requires std::is_arithmetic_v< W >
W Assignment< W >::columnPotential( int j )
{
 ADS_PRE( ( j >= 0 ) && ( j < f_n ) );
 solve();
 return( f_v[ j ] );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Assignment.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
