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

/// all the coordinates of P in [ -10^9 , 10^9 ]
inline bool smallCoordinates( const std::vector< Point > & P )
{
 const long M = 1000000000L;
 return( std::all_of( P.begin() , P.end() , [ & ]( const Point & p ) {
  return( ( p.x >= -M ) && ( p.x <= M ) && ( p.y >= -M ) && ( p.y <= M ) );
  } ) );
 }

}  // end( namespace detail )

inline long long cross( const Point & a , const Point & b ,
                         const Point & c )
{
 using L = long long;                      // long has 32 bits on Windows
 return( ( L( b.x ) - a.x ) * ( L( c.y ) - a.y ) -
         ( L( b.y ) - a.y ) * ( L( c.x ) - a.x ) );
 }

inline bool sameSide( const Point & p1 , const Point & p2 , const Point & p ,
                      const Point & q )
{
 const long long a = cross( p1 , p2 , p ) , b = cross( p1 , p2 , q );
 return( ( a >= 0 && b >= 0 ) || ( a <= 0 && b <= 0 ) );
 }

inline double pseudoAngle( long dx , long dy )
{
 if( ( dx == 0 ) && ( dy == 0 ) )
  return( 0 );
 // in double: | LONG_MIN | and the sum of the two overflow a long
 const double t = double( dy ) / ( std::fabs( double( dx ) ) +
                                   std::fabs( double( dy ) ) );
 if( dx < 0 )
  return( 2 - t );
 if( dy < 0 )                             // 4 + t may round up to 4
  return( std::min( 4 + t , std::nextafter( 4.0 , 0.0 ) ) );
 ADS_POST( ( t >= 0 ) && ( t <= 1 ) );
 return( t );
 }

inline std::vector< Point > convexHullBrute( std::vector< Point > P )
{
 ADS_PRE( detail::smallCoordinates( P ) );
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
    const long long c = cross( P[ i ] , P[ j ] , P[ k ] );
    if( c < 0 ) edge = false;
    // a point on the segment, strictly inside, is not a vertex
    using L = long long;
    if( ( c == 0 ) && ( k != i ) && ( k != j ) &&
        ( ( L( P[ k ].x ) - P[ i ].x ) * ( L( P[ k ].x ) - P[ j ].x ) +
          ( L( P[ k ].y ) - P[ i ].y ) * ( L( P[ k ].y ) - P[ j ].y ) > 0 ) )
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
 ADS_PRE( detail::smallCoordinates( P ) );
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
  const long long dx = ( long long )( a.x ) - o.x ,
                  dy = ( long long )( a.y ) - o.y;
  return( dx * dx + dy * dy ); };
 std::sort( P.begin() + 1 , P.end() , [ & ]( const Point & a ,
                                            const Point & b ) {
  const long long c = cross( o , a , b );
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

inline std::vector< Point > convexHull( const std::vector< Point > & P )
{
 ADS_PRE( detail::smallCoordinates( P ) );
 using L = long long;
 using U = std::uint64_t;
 const std::size_t n = P.size();
 if( n == 0 )
  return( std::vector< Point >() );
 // the extreme points in the directions of -y , x - y , x , x + y , y ,
 // y - x , -x , -x - y: the octagon, counter clockwise
 Point E[ 8 ];
 for( auto & e : E ) e = P[ 0 ];
 for( const Point & p : P ) {
  if( p.y < E[ 0 ].y ) E[ 0 ] = p;
  if( L( p.x ) - p.y > L( E[ 1 ].x ) - E[ 1 ].y ) E[ 1 ] = p;
  if( p.x > E[ 2 ].x ) E[ 2 ] = p;
  if( L( p.x ) + p.y > L( E[ 3 ].x ) + E[ 3 ].y ) E[ 3 ] = p;
  if( p.y > E[ 4 ].y ) E[ 4 ] = p;
  if( L( p.y ) - p.x > L( E[ 5 ].y ) - E[ 5 ].x ) E[ 5 ] = p;
  if( p.x < E[ 6 ].x ) E[ 6 ] = p;
  if( L( p.x ) + p.y < L( E[ 7 ].x ) + E[ 7 ].y ) E[ 7 ] = p;
  }
 Point O[ 8 ];                              // the distinct vertices
 int m = 0;
 for( const Point & e : E )
  if( ( m == 0 ) || !( e == O[ m - 1 ] ) ) O[ m++ ] = e;
 while( ( m > 1 ) && ( O[ m - 1 ] == O[ 0 ] ) ) --m;
 // the keys ( x , y ) of the points not strictly inside the octagon
 auto key = []( const Point & p ) {
  return( ( U( L( p.x ) + ( L( 1 ) << 31 ) ) << 32 ) |
          U( L( p.y ) + ( L( 1 ) << 31 ) ) ); };
 std::vector< U > K;
 K.reserve( m >= 3 ? n / 8 + 16 : n );
 for( const Point & p : P ) {
  bool inside = m >= 3;
  for( int i = 0 ; inside && ( i < m ) ; ++i )
   inside = cross( O[ i ] , O[ i + 1 == m ? 0 : i + 1 ] , p ) > 0;
  if( ! inside )
   K.push_back( key( p ) );
  }
 sort( K );
 K.erase( std::unique( K.begin() , K.end() ) , K.end() );
 auto point = []( U k ) {
  return( Point{ long( L( k >> 32 ) - ( L( 1 ) << 31 ) ) ,
                 long( L( k & 0xFFFFFFFFULL ) - ( L( 1 ) << 31 ) ) } ); };
 std::vector< Point > H;
 if( K.size() < 3 ) {
  for( U k : K ) H.push_back( point( k ) );   // by x, as graham()
  return( H );
  }
 H.resize( 2 * K.size() );
 std::size_t k = 0;
 for( std::size_t i = 0 ; i < K.size() ; ++i ) {   // the lower chain
  const Point p = point( K[ i ] );
  while( ( k >= 2 ) && ( cross( H[ k - 2 ] , H[ k - 1 ] , p ) <= 0 ) ) --k;
  H[ k++ ] = p;
  }
 for( std::size_t i = K.size() - 1 , t = k + 1 ; i-- > 0 ; ) {   // upper
  const Point p = point( K[ i ] );
  while( ( k >= t ) && ( cross( H[ k - 2 ] , H[ k - 1 ] , p ) <= 0 ) ) --k;
  H[ k++ ] = p;
  }
 H.resize( k - 1 );                         // the first point came back
 // from the lowest point, the rightmost of the lowest ones, as graham()
 const auto low = std::min_element( H.begin() , H.end() ,
                                    []( const Point & a , const Point & b ) {
  return( a.y < b.y || ( a.y == b.y && a.x > b.x ) ); } );
 std::rotate( H.begin() , low , H.end() );
 ADS_AUDIT( detail::isHullOf( H , P ) );
 return( H );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ConvexHull.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
