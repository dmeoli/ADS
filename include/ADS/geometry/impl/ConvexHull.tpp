/*--------------------------------------------------------------------------*/
/*-------------------------- File ConvexHull.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConvexHull [see ConvexHull.h], with their contracts.
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

namespace detail {

/// H is convex, counter clockwise, made of points of P, and no point of P
/// is out of it
inline bool isHullOf( const std::vector< Point > & H ,
                      const std::vector< Point > & P )
{
 for( const Point & h : H )
  if( std::find( P.begin() , P.end() , h ) == P.end() )
   return( false );
 if( H.size() < 3 )
  return( true );
 for( std::size_t i = 0 ; i < H.size() ; ++i ) {
  const Point & a = H[ i ] , & b = H[ ( i + 1 ) % H.size() ];
  if( cross( a , b , H[ ( i + 2 ) % H.size() ] ) <= 0 )
   return( false );                          // a vertex not strictly convex
  for( const Point & p : P )
   if( cross( a , b , p ) < 0 )
    return( false );                         // a point out of the hull
  }
 return( true );
 }

}  // end( namespace detail )

inline long cross( const Point & a , const Point & b , const Point & c )
{
 return( ( b.x - a.x ) * ( c.y - a.y ) - ( b.y - a.y ) * ( c.x - a.x ) );
 }

inline bool sameSide( const Point & p1 , const Point & p2 , const Point & p ,
                      const Point & q )
{
 const long a = cross( p1 , p2 , p ) , b = cross( p1 , p2 , q );
 return( ( a >= 0 && b >= 0 ) || ( a <= 0 && b <= 0 ) );
 }

inline double pseudoAngle( long dx , long dy )
{
 if( ( dx == 0 ) && ( dy == 0 ) )
  return( 0 );
 const double t = double( dy ) / double( std::labs( dx ) + std::labs( dy ) );
 if( dx < 0 )
  return( 2 - t );
 if( dy < 0 )
  return( 4 + t );
 ADS_POST( ( t >= 0 ) && ( t <= 1 ) );
 return( t );
 }

inline std::vector< Point > convexHullBrute( std::vector< Point > P )
{
 [[maybe_unused]] const std::vector< Point > P0 = P;
 std::sort( P.begin() , P.end() , []( const Point & a , const Point & b ) {
  return( a.x < b.x || ( a.x == b.x && a.y < b.y ) ); } );
 P.erase( std::unique( P.begin() , P.end() ) , P.end() );
 const std::size_t n = P.size();
 if( n < 3 )
  return( P );
 std::vector< bool > vertex( n , false );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j ) {
   if( i == j ) continue;
   bool edge = true;                       // all the others to the left
   for( std::size_t k = 0 ; k < n && edge ; ++k ) {
    const long c = cross( P[ i ] , P[ j ] , P[ k ] );
    if( c < 0 ) edge = false;
    // a point on the segment, strictly inside, is not a vertex
    if( ( c == 0 ) && ( k != i ) && ( k != j ) &&
        ( ( P[ k ].x - P[ i ].x ) * ( P[ k ].x - P[ j ].x ) +
          ( P[ k ].y - P[ i ].y ) * ( P[ k ].y - P[ j ].y ) > 0 ) )
     edge = false;
    }
   if( edge ) vertex[ i ] = vertex[ j ] = true;
   }
 std::vector< Point > H;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( vertex[ i ] ) H.push_back( P[ i ] );
 // counter clockwise around the lowest one
 auto low = std::min_element( H.begin() , H.end() , []( auto & a , auto & b ) {
  return( a.y < b.y || ( a.y == b.y && a.x > b.x ) ); } );
 std::iter_swap( H.begin() , low );
 const Point o = H[ 0 ];
 std::sort( H.begin() + 1 , H.end() , [ & ]( const Point & a ,
                                            const Point & b ) {
  return( cross( o , a , b ) > 0 ); } );
 ADS_AUDIT( detail::isHullOf( H , P0 ) );
 return( H );
 }

inline std::vector< Point > graham( std::vector< Point > P )
{
 [[maybe_unused]] const std::vector< Point > P0 = P;
 std::sort( P.begin() , P.end() , []( const Point & a , const Point & b ) {
  return( a.x < b.x || ( a.x == b.x && a.y < b.y ) ); } );
 P.erase( std::unique( P.begin() , P.end() ) , P.end() );
 if( P.size() < 3 )
  return( P );
 const std::size_t min = std::min_element( P.begin() , P.end() ,
                          []( const Point & a , const Point & b ) {
  return( a.y < b.y || ( a.y == b.y && a.x > b.x ) ); } ) - P.begin();
 std::swap( P[ 0 ] , P[ min ] );
 const Point o = P[ 0 ];
 auto dist = [ & ]( const Point & a ) {
  return( ( a.x - o.x ) * ( a.x - o.x ) + ( a.y - o.y ) * ( a.y - o.y ) ); };
 std::sort( P.begin() + 1 , P.end() , [ & ]( const Point & a ,
                                            const Point & b ) {
  const long c = cross( o , a , b );
  return( c > 0 || ( c == 0 && dist( a ) < dist( b ) ) ); } );
 // of the collinear ones with o only the farthest
 std::vector< Point > Q = { o };
 for( std::size_t i = 1 ; i < P.size() ; ++i ) {
  if( ( i + 1 < P.size() ) && ( cross( o , P[ i ] , P[ i + 1 ] ) == 0 ) )
   continue;
  Q.push_back( P[ i ] );
  }
 std::vector< Point > S;                     // the stack of the hull
 for( const Point & p : Q ) {
  while( ( S.size() >= 2 ) &&
         ( cross( S[ S.size() - 2 ] , S.back() , p ) <= 0 ) )
   S.pop_back();                             // the backtrack
  S.push_back( p );
  }
 ADS_AUDIT( detail::isHullOf( S , P0 ) );
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ConvexHull.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
