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
