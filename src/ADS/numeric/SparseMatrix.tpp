/*--------------------------------------------------------------------------*/
/*------------------------- File SparseMatrix.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SparseMatrix [see SparseMatrix.h], with their
 * contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

// the head of the row i is the node i, that of the column j the node m + j;
// a head has row = m (or col = n) so that it closes the sorted lists

inline SparseMatrix::SparseMatrix( std::size_t m , std::size_t n )
 : f_m( m ) , f_n( n )
{
 for( std::size_t i = 0 ; i < m ; ++i )
  f_N.push_back( Node{ i , n , 0 , long( i ) , -1 } );
 for( std::size_t j = 0 ; j < n ; ++j )
  f_N.push_back( Node{ m , j , 0 , -1 , long( m + j ) } );
 }

inline std::size_t SparseMatrix::rows( void ) const { return( f_m ); }
inline std::size_t SparseMatrix::cols( void ) const { return( f_n ); }
inline std::size_t SparseMatrix::nonzeros( void ) const { return( f_nnz ); }

inline std::pair< long , long > SparseMatrix::findInRow( std::size_t i ,
                                                         std::size_t j )
 const
{
 long p = long( i );                        // the head of the row
 while( f_N[ f_N[ p ].right ].col < j )
  p = f_N[ p ].right;
 const long q = f_N[ p ].right;
 return( std::pair( p , f_N[ q ].col == j ? q : -1L ) );
 }

inline long SparseMatrix::beforeInCol( std::size_t i , std::size_t j ) const
{
 long p = long( f_m + j );
 while( f_N[ f_N[ p ].down ].row < i )
  p = f_N[ p ].down;
 return( p );
 }

inline double SparseMatrix::get( std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i < f_m ) && ( j < f_n ) );
 const long q = findInRow( i , j ).second;
 return( q < 0 ? 0 : f_N[ q ].val );
 }

inline void SparseMatrix::set( std::size_t i , std::size_t j , double v )
{
 ADS_PRE( ( i < f_m ) && ( j < f_n ) );
 const auto [ p , q ] = findInRow( i , j );
 if( q >= 0 ) {
  if( v != 0 ) {
   f_N[ q ].val = v;
   return;
   }
  f_N[ p ].right = f_N[ q ].right;          // unlinked from both lists
  const long u = beforeInCol( i , j );
  f_N[ u ].down = f_N[ q ].down;
  f_freeNodes.push_back( q );
  --f_nnz;
  }
 else if( v != 0 ) {
  long q2;
  if( f_freeNodes.empty() ) {
   q2 = long( f_N.size() );
   f_N.push_back( Node{} );
   }
  else {
   q2 = f_freeNodes.back();
   f_freeNodes.pop_back();
   }
  const long u = beforeInCol( i , j );
  f_N[ q2 ] = Node{ i , j , v , f_N[ p ].right , f_N[ u ].down };
  f_N[ p ].right = q2;
  f_N[ u ].down = q2;
  ++f_nnz;
  }
 ADS_AUDIT( linked() );
 }

inline std::vector< std::pair< std::size_t , double > > SparseMatrix::row(
                                                        std::size_t i ) const
{
 ADS_PRE( i < f_m );
 std::vector< std::pair< std::size_t , double > > out;
 for( long q = f_N[ i ].right ; q != long( i ) ; q = f_N[ q ].right )
  out.emplace_back( f_N[ q ].col , f_N[ q ].val );
 return( out );
 }

inline std::vector< std::pair< std::size_t , double > > SparseMatrix::col(
                                                        std::size_t j ) const
{
 ADS_PRE( j < f_n );
 std::vector< std::pair< std::size_t , double > > out;
 for( long q = f_N[ f_m + j ].down ; q != long( f_m + j ) ;
      q = f_N[ q ].down )
  out.emplace_back( f_N[ q ].row , f_N[ q ].val );
 return( out );
 }

inline void SparseMatrix::pivot( std::size_t r , std::size_t c , double eps )
{
 ADS_PRE( ( r < f_m ) && ( c < f_n ) && ( get( r , c ) != 0 ) );
 const double a = get( r , c );
 const auto R = row( r ) , C = col( c );    // the pivot row and column
 for( const auto & [ i , ci ] : C ) {       // d - b c / a
  if( i == r ) continue;
  for( const auto & [ j , bj ] : R ) {
   if( j == c ) continue;
   double d = get( i , j ) - bj * ci / a;
   if( std::abs( d ) <= eps ) d = 0;
   set( i , j , d );
   }
  }
 for( const auto & [ j , bj ] : R )         // b / a
  if( j != c ) set( r , j , bj / a );
 for( const auto & [ i , ci ] : C )         // - c / a
  if( i != r ) set( i , c , - ci / a );
 set( r , c , 1 / a );
 }

inline bool SparseMatrix::linked( void ) const
{
 std::size_t n = 0;
 for( std::size_t i = 0 ; i < f_m ; ++i ) {
  std::size_t last = 0;
  bool first = true;
  for( long q = f_N[ i ].right ; q != long( i ) ; q = f_N[ q ].right ) {
   if( ( f_N[ q ].row != i ) || ( ! first && ( f_N[ q ].col <= last ) ) ||
       ( ++n > f_nnz ) )
    return( false );
   last = f_N[ q ].col;
   first = false;
   }
  }
 std::size_t k = 0;
 for( std::size_t j = 0 ; j < f_n ; ++j )
  for( long q = f_N[ f_m + j ].down ; q != long( f_m + j ) ;
       q = f_N[ q ].down )
   if( ( f_N[ q ].col != j ) || ( ++k > f_nnz ) )
    return( false );
 return( ( n == f_nnz ) && ( k == f_nnz ) );
 }

/*--------------------------------------------------------------------------*/

inline std::ranges::subrange< SparseMatrix::const_iterator >
SparseMatrix::rowEntries( std::size_t i ) const
{
 ADS_PRE( i < f_m );
 const const_iterator first( &f_N , f_N[ i ].right , true ) ,
                      last( &f_N , long( i ) , true );
 return( std::ranges::subrange( first , last ) );
 }

inline std::ranges::subrange< SparseMatrix::const_iterator >
SparseMatrix::colEntries( std::size_t j ) const
{
 ADS_PRE( j < f_n );
 const long h = long( f_m + j );
 const const_iterator first( &f_N , f_N[ h ].down , false ) ,
                      last( &f_N , h , false );
 return( std::ranges::subrange( first , last ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SparseMatrix.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
