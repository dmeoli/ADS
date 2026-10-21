/*--------------------------------------------------------------------------*/
/*--------------------------- File UnionFind.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DisjointSets [see UnionFind.h], with their contracts.
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

template< Union L , Compression C >
inline DisjointSets< L , C >::DisjointSets( int n )
{
 reset( n );
 }

template< Union L , Compression C >
inline void DisjointSets< L , C >::reset( int n )
{
 ADS_PRE( n >= 0 );
 f_p.assign( std::size_t( n < 0 ? 0 : n ) , -1 );
 f_count = n < 0 ? 0 : n;
 }

template< Union L , Compression C >
inline int DisjointSets< L , C >::find( int x )
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 int * p = f_p.data();
 if constexpr( C == Compression::HALVING ) {
  while( p[ x ] >= 0 ) {
   const int y = p[ x ];
   if( p[ y ] < 0 )
    return( y );
   p[ x ] = p[ y ];                       // x under its grandparent
   x = p[ y ];
   }
  return( x );
  }
 else {
  int r = x;
  while( p[ r ] >= 0 )
   r = p[ r ];
  while( x != r ) {                       // the path under the root
   const int y = p[ x ];
   p[ x ] = r;
   x = y;
   }
  return( r );
  }
 }

template< Union L , Compression C >
inline bool DisjointSets< L , C >::merge( int x , int y )
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) && ( y >= 0 ) && ( y < n() ) );
 x = find( x );
 y = find( y );
 if( x == y )
  return( false );
 // the roots hold - size, or - 1 - rank: the more negative is the larger
 if( f_p[ x ] < f_p[ y ] )                // x larger: y under x
  std::swap( x , y );
 if constexpr( L == Union::SIZE )
  f_p[ y ] += f_p[ x ];
 else if( f_p[ x ] == f_p[ y ] )
  --f_p[ y ];                             // a tie: the rank of y grows
 f_p[ x ] = y;
 --f_count;
 ADS_INVARIANT( forest() );
 return( true );
 }

template< Union L , Compression C >
inline bool DisjointSets< L , C >::same( int x , int y )
{
 return( find( x ) == find( y ) );
 }

template< Union L , Compression C >
inline int DisjointSets< L , C >::size( int x ) requires( L == Union::SIZE )
{
 return( -f_p[ find( x ) ] );
 }

template< Union L , Compression C >
bool DisjointSets< L , C >::forest( void ) const
{
 const int n = this->n();
 std::vector< int > size( n , 0 ) , height( n , 0 );
 int roots = 0;
 for( int x = 0 ; x < n ; ++x ) {
  int r = x , steps = 0;
  for( ; f_p[ r ] >= 0 ; ++steps ) {
   if( ( f_p[ r ] >= n ) || ( steps > n ) )
    return( false );
   r = f_p[ r ];
   }
  ++size[ r ];
  height[ r ] = std::max( height[ r ] , steps );
  }
 for( int x = 0 ; x < n ; ++x )
  if( f_p[ x ] < 0 ) {
   ++roots;
   if( L == Union::SIZE ? -f_p[ x ] != size[ x ] :
                          height[ x ] > -1 - f_p[ x ] )
    return( false );                      // a rank bounds the height
   }
 return( roots == f_count );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File UnionFind.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
