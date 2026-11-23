/*--------------------------------------------------------------------------*/
/*--------------------------- File UnionFind.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DisjointSets [see UnionFind.h], with their contracts.
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

namespace ADS
{

template< Union L , Compression C , class P >
inline DisjointSets< L , C , P >::DisjointSets( int n )
{
 ADS_PRE( n >= 0 );
 f_p.assign( std::size_t( n ) , -1 );
 f_count = n;
 }

template< Union L , Compression C , class P >
template< class... O >
 requires( sizeof...( O ) > 0 )
DisjointSets< L , C , P >::DisjointSets( int n , O &... o ) : f_sub( o... )
{
 ADS_PRE( n >= 0 );
 f_p.assign( std::size_t( n ) , -1 );
 f_count = n;
 }

template< Union L , Compression C , class P >
inline void DisjointSets< L , C , P >::reset( int n )
{
 ADS_PRE( n >= 0 );
 f_p.assign( std::size_t( n ) , -1 );
 f_count = n;
 if constexpr( Subject::template listened< mod::Rebuilt > )
  if( f_sub.template live< mod::Rebuilt >() ) {
   f_sub.emit( mod::Rebuilt{} );
   f_sub.audit( *this );
   }
 }

template< Union L , Compression C , class P >
inline int DisjointSets< L , C , P >::find( int x )
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 int * p = f_p.data();
 if constexpr( C == Compression::HALVING ) {
  while( p[ x ] >= 0 ) {
   const int y = p[ x ];
   if( p[ y ] < 0 )
    return( y );
   p[ x ] = p[ y ];  // x under its grandparent
   x = p[ y ];
   }
  return( x );
  }
 else {
  int r = x;
  while( p[ r ] >= 0 )
   r = p[ r ];
  while( x != r ) {  // the path under the root
   const int y = p[ x ];
   p[ x ] = r;
   x = y;
   }
  return( r );
  }
 }

template< Union L , Compression C , class P >
inline bool DisjointSets< L , C , P >::merge( int x , int y )
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) && ( y >= 0 ) && ( y < n() ) );
 x = find( x );
 y = find( y );
 if( x == y )
  return( false );
 // the roots hold - size, or - 1 - rank: the more negative is the larger
 if( f_p[ x ] < f_p[ y ] )  // x larger: y under x
  std::swap( x , y );
 if constexpr( Subject::template listened< Linked > )
  if( f_sub.template live< Linked >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    linkCold( x , y );
   else
    linkTold( x , y );
   return( true );
   }
 if constexpr( L == Union::SIZE )
  f_p[ y ] += f_p[ x ];
 else if( f_p[ x ] == f_p[ y ] )
  --f_p[ y ];  // a tie: the rank of y grows
 f_p[ x ] = y;
 --f_count;
 ADS_INVARIANT( forest() );
 return( true );
 }

template< Union L , Compression C , class P >
void DisjointSets< L , C , P >::link( int x , int y )
{
 if constexpr( L == Union::SIZE )
  f_p[ y ] += f_p[ x ];
 else if( f_p[ x ] == f_p[ y ] )
  --f_p[ y ];  // a tie: the rank of y grows
 f_p[ x ] = y;
 --f_count;
 }

template< Union L , Compression C , class P >
void DisjointSets< L , C , P >::linkTold( int x , int y )
{
 const Linked e{ x , -1 , y };
 f_sub.prepare( e );
 const int px = f_p[ x ] , py = f_p[ y ];
 link( x , y );
 f_sub.emit( e , [ & , px , py ]() noexcept {
  f_p[ x ] = px;  // x a root again, y as it was
  f_p[ y ] = py;
  ++f_count;
  } );
 f_sub.audit( *this );
 ADS_INVARIANT( forest() );
 }

template< Union L , Compression C , class P >
inline bool DisjointSets< L , C , P >::same( int x , int y )
{
 return( find( x ) == find( y ) );
 }

template< Union L , Compression C , class P >
inline int DisjointSets< L , C , P >::size( int x )
 requires( L == Union::SIZE )
{
 return( -f_p[ find( x ) ] );
 }

template< Union L , Compression C , class P >
bool DisjointSets< L , C , P >::forest( void ) const
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
   if( L == Union::SIZE ? -f_p[ x ] != size[ x ]
			: height[ x ] > -1 - f_p[ x ] )
    return( false );  // a rank bounds the height
   }
 return( roots == f_count );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File UnionFind.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
