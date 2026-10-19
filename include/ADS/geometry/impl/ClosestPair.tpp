/*--------------------------------------------------------------------------*/
/*-------------------------- File ClosestPair.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ClosestPair.h [see ClosestPair.h], with their contracts.
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

/// the closest pair so far and the square of its distance
struct ClosestSoFar {
 std::int64_t d = std::numeric_limits< std::int64_t >::max();
 std::size_t i = 0 , j = 0;

 void offer( const std::vector< Point > & P , std::size_t a ,
             std::size_t b ) {
  if( const std::int64_t e = squaredDistance( P[ a ] , P[ b ] ) ; e < d ) {
   d = e;
   i = std::min( a , b );
   j = std::max( a , b );
   }
  }
 };

/// the coordinates of the points within the bound of the contracts
inline bool closestPairInput( const std::vector< Point > & P )
{
 const long M = 1000000000L;
 return( std::all_of( P.begin() , P.end() , [ & ]( const Point & p ) {
  return( ( p.x >= -M ) && ( p.x <= M ) && ( p.y >= -M ) && ( p.y <= M ) );
  } ) );
 }

/// the closest pair among P[ I[ lo .. hi ) ], sorted by x, offered to b;
/// the range ends sorted by y, B a buffer
inline void closestRec( const std::vector< Point > & P ,
                        std::vector< std::size_t > & I ,
                        std::vector< std::size_t > & B , std::size_t lo ,
                        std::size_t hi , ClosestSoFar & b )
{
 const auto byY = [ & ]( std::size_t u , std::size_t v ) {
  return( P[ u ].y < P[ v ].y ); };
 const auto it = I.begin();
 if( hi - lo <= 3 ) {                       // all the pairs
  for( std::size_t u = lo ; u < hi ; ++u )
   for( std::size_t v = u + 1 ; v < hi ; ++v )
    b.offer( P , I[ u ] , I[ v ] );
  std::sort( it + std::ptrdiff_t( lo ) , it + std::ptrdiff_t( hi ) , byY );
  return;
  }
 const std::size_t mid = lo + ( hi - lo ) / 2;
 const std::int64_t xm = P[ I[ mid ] ].x;   // the split
 closestRec( P , I , B , lo , mid , b );
 closestRec( P , I , B , mid , hi , b );
 std::merge( it + std::ptrdiff_t( lo ) , it + std::ptrdiff_t( mid ) ,
             it + std::ptrdiff_t( mid ) , it + std::ptrdiff_t( hi ) ,
             B.begin() + std::ptrdiff_t( lo ) , byY );
 std::copy( B.begin() + std::ptrdiff_t( lo ) ,
            B.begin() + std::ptrdiff_t( hi ) , it + std::ptrdiff_t( lo ) );
 std::size_t s = lo;                        // the strip in B[ lo .. s )
 for( std::size_t u = lo ; u < hi ; ++u ) {
  const std::int64_t dx = P[ I[ u ] ].x - xm;
  if( dx * dx < b.d )
   B[ s++ ] = I[ u ];
  }
 for( std::size_t u = lo ; u < s ; ++u )
  for( std::size_t v = u + 1 ; v < s ; ++v ) {
   const std::int64_t dy = std::int64_t( P[ B[ v ] ].y ) - P[ B[ u ] ].y;
   if( dy * dy >= b.d )
    break;                                  // the next ones farther
   b.offer( P , B[ u ] , B[ v ] );
   }
 }

}  // end( namespace detail )

inline std::int64_t squaredDistance( const Point & p , const Point & q )
{
 const std::int64_t dx = std::int64_t( p.x ) - q.x ,
                    dy = std::int64_t( p.y ) - q.y;
 return( dx * dx + dy * dy );
 }

inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPair( const std::vector< Point > & P )
{
 ADS_PRE( ( P.size() >= 2 ) && detail::closestPairInput( P ) );
 std::vector< std::size_t > I( P.size() ) , B( P.size() );
 std::iota( I.begin() , I.end() , std::size_t( 0 ) );
 std::sort( I.begin() , I.end() , [ & ]( std::size_t u , std::size_t v ) {
  return( P[ u ].x < P[ v ].x ); } );
 detail::ClosestSoFar b;
 detail::closestRec( P , I , B , 0 , I.size() , b );
 ADS_POST( ( b.i < b.j ) && ( squaredDistance( P[ b.i ] , P[ b.j ] ) ==
                              b.d ) );
 return( std::pair( std::pair( b.i , b.j ) , b.d ) );
 }

namespace detail {

/// the sweep of closestPairSweep() along x, or along y if byY, on the keys
/// K: the least square distance and the keys of a pair at it, or nothing
/// if it gives up after budget comparisons
inline std::optional< std::tuple< std::int64_t , std::uint64_t ,
                                  std::uint64_t > >
sweep( const std::vector< Point > & P , std::vector< std::uint64_t > & K ,
       bool byY , std::size_t budget )
{
 using U = std::uint64_t;
 using I = std::int64_t;
 const std::size_t n = P.size();
 const I B = I( 1 ) << 31;                  // the coordinates made unsigned
 for( std::size_t u = 0 ; u < n ; ++u ) {
  const U x = U( P[ u ].x + B ) , y = U( P[ u ].y + B );
  K[ u ] = byY ? ( y << 32 ) | x : ( x << 32 ) | y;
  }
 sort( K );
 auto X = [ & ]( std::size_t u ) { return( I( K[ u ] >> 32 ) ); };
 auto Y = [ & ]( std::size_t u ) { return( I( K[ u ] & 0xFFFFFFFFULL ) ); };
 I best = std::numeric_limits< I >::max();
 std::size_t a = 0 , b = 1;
 for( std::size_t u = 1 , column = 0 ; ( u < n ) && ( best > 0 ) ; ++u ) {
  if( X( u ) != X( u - 1 ) ) column = u;    // the first of its x
  const I xu = X( u ) , yu = Y( u );
  for( std::size_t v = u ; v-- > 0 ; ) {
   const I dx = xu - X( v );
   if( dx * dx >= best )
    break;                                  // the next ones farther in x
   const I dy = yu - Y( v );
   if( dy * dy < best ) {
    if( dx * dx + dy * dy < best ) {
     best = dx * dx + dy * dy;
     a = v;
     b = u;
     }
    }
   else if( dx == 0 )                       // farther in y in the column:
    v = column;                             // its start
   if( --budget == 0 )
    return( std::nullopt );
   }
  }
 return( std::tuple( best , K[ a ] , K[ b ] ) );
 }

}  // end( namespace detail )

inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPairSweep( const std::vector< Point > & P )
{
 ADS_PRE( ( P.size() >= 2 ) && detail::closestPairInput( P ) );
 using U = std::uint64_t;
 const std::size_t n = P.size();
 std::vector< U > K( n );
 auto r = detail::sweep( P , K , false , 16 * n );
 const bool byY = ! r;                      // a long scan along x: along y
 if( byY ) r = detail::sweep( P , K , true , 16 * n );
 if( ! r )                                  // and along y: the book
  return( closestPair( P ) );
 const auto [ best , ka , kb ] = *r;
 // the places of the two keys in P, two different ones if they are equal
 const std::int64_t B = std::int64_t( 1 ) << 31;
 std::size_t i = n , j = n;
 for( std::size_t u = 0 ; u < n ; ++u ) {
  const U x = U( P[ u ].x + B ) , y = U( P[ u ].y + B ) ,
          k = byY ? ( y << 32 ) | x : ( x << 32 ) | y;
  if( ( i == n ) && ( k == ka ) ) i = u;
  else if( ( j == n ) && ( k == kb ) ) j = u;
  }
 if( i > j ) std::swap( i , j );
 ADS_POST( ( i < j ) && ( j < n ) && ( squaredDistance( P[ i ] , P[ j ] ) ==
                                       best ) );
 return( std::pair( std::pair( i , j ) , best ) );
 }

inline std::pair< std::pair< std::size_t , std::size_t > , std::int64_t >
closestPairBrute( const std::vector< Point > & P )
{
 ADS_PRE( ( P.size() >= 2 ) && detail::closestPairInput( P ) );
 detail::ClosestSoFar b;
 for( std::size_t u = 0 ; u < P.size() ; ++u )
  for( std::size_t v = u + 1 ; v < P.size() ; ++v )
   b.offer( P , u , v );
 return( std::pair( std::pair( b.i , b.j ) , b.d ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ClosestPair.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
