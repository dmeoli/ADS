/*--------------------------------------------------------------------------*/
/*---------------------------- File PGMIndex.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PGMIndex [see PGMIndex.h], with their contracts.
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

#include <algorithm>
#include <cmath>
#include <cstring>

#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_PGM_PREFETCH( p ) __builtin_prefetch( p )
#else
 #define ADS_PGM_PREFETCH( p ) ( ( void ) 0 )
#endif

namespace ADS {

namespace detail {

/// a point of the plane with coordinates of 128 bits, so that the products
/// of the hulls are exact
struct PlaPoint {
 Int128 x;
 Int128 y;
 };

/// the cross product of b - a and c - a, positive on a left turn
inline Int128 plaCross( const PlaPoint & a , const PlaPoint & b ,
                        const PlaPoint & c )
{
 return( ( b.x - a.x ) * ( c.y - a.y ) - ( b.y - a.y ) * ( c.x - a.x ) );
 }

/// whether the slope of a -> b is smaller than that of c -> d, b and d on
/// the right of a and c
inline bool plaLess( const PlaPoint & a , const PlaPoint & b ,
                     const PlaPoint & c , const PlaPoint & d )
{
 return( ( b.y - a.y ) * ( d.x - c.x ) < ( d.y - c.y ) * ( b.x - a.x ) );
 }

/// the first place in [ lo , hi ) of an item not smaller than v, with no
/// branch
template< class T >
inline std::size_t windowLowerBound( const T * a , std::size_t lo ,
                                     std::size_t hi , const T & v )
{
 std::size_t n = hi - lo;
 if( n == 0 )
  return( lo );
 const T * base = a + lo;
 while( n > 1 ) {
  const std::size_t h = n / 2;
  base = base[ h ] < v ? base + h : base;
  n -= h;
  }
 return( std::size_t( base - a ) + ( *base < v ? 1 : 0 ) );
 }

}  // end( namespace detail )

#define ADS_PGM_T template< class K , std::size_t E , std::size_t R > \
 requires std::is_arithmetic_v< K >
#define ADS_PGM PGMIndex< K , E , R >

ADS_PGM_T
inline std::uint64_t ADS_PGM::ord( const K & k )
{
 if constexpr( std::is_floating_point_v< K > ) {
  const std::uint64_t b = std::bit_cast< std::uint64_t >( double( k ) );
  return( b >> 63 ? ~b : b | ( std::uint64_t( 1 ) << 63 ) );
  }
 else if constexpr( std::is_signed_v< K > )
  return( std::uint64_t( std::int64_t( k ) ) ^ ( std::uint64_t( 1 ) << 63 ) );
 else
  return( std::uint64_t( k ) );
 }

ADS_PGM_T
auto ADS_PGM::approximate( const std::vector< std::uint64_t > & x ,
                           const std::vector< std::uint32_t > & y ,
                           std::int64_t e , std::size_t total ) -> Level
{
 using P = detail::PlaPoint;
 Level L;
 std::vector< P > U , D;                    // the hulls, up and down
 P r0{} , r1{} , r2{} , r3{};               // the two extreme lines
 std::size_t first = 0 , count = 0;
 auto close = [ & ]( void ) {               // the segment from first
  L.key.push_back( x[ first ] );
  L.start.push_back( y[ first ] );
  if( count == 1 ) {
   L.line.push_back( Line{ double( y[ first ] ) , 0.0 } );
   return;
   }
  const long double x0 = (long double) x[ first ];
  const long double s1 = (long double)( r2.y - r0.y ) /
                         (long double)( r2.x - r0.x );   // the least
  const long double s2 = (long double)( r3.y - r1.y ) /
                         (long double)( r3.x - r1.x );   // the largest
  const long double a0 = (long double) r0.x - x0 , b0 = (long double) r0.y;
  const long double a1 = (long double) r1.x - x0 , b1 = (long double) r1.y;
  long double s , c;                        // the slope, at x0
  if( s1 >= s2 ) {                          // parallel: between them
   s = std::max( s1 , (long double) 0 );
   c = ( ( b0 - s * a0 ) + ( b1 - s * a1 ) ) / 2;
   }
  else {                                    // through their crossing
   const long double xi = ( b1 - b0 + s1 * a0 - s2 * a1 ) / ( s1 - s2 );
   const long double yi = b0 + s1 * ( xi - a0 );
   s = std::max( ( s1 + s2 ) / 2 , std::max( s1 , (long double) 0 ) );
   c = yi - s * xi;
   }
  L.line.push_back( Line{ double( c ) , double( s ) } );
  };
 for( std::size_t i = 0 ; i < x.size() ; ++i ) {
  const P pu{ detail::Int128( x[ i ] ) , detail::Int128( y[ i ] ) + e };
  const P pd{ detail::Int128( x[ i ] ) , detail::Int128( y[ i ] ) - e };
  if( count >= 2 && ( detail::plaLess( r2 , pu , r0 , r2 ) ||
                      detail::plaLess( r1 , r3 , r3 , pd ) ) ) {
   close();                                 // no line is left
   count = 0;
   }
  if( count == 0 ) {
   U.assign( 1 , pu );
   D.assign( 1 , pd );
   first = i;
   count = 1;
   continue;
   }
  if( count == 1 ) {
   r0 = U[ 0 ];
   r1 = D[ 0 ];
   r2 = pd;
   r3 = pu;
   U.push_back( pu );
   D.push_back( pd );
   count = 2;
   continue;
   }
  if( detail::plaLess( r1 , pu , r1 , r3 ) ) {   // the largest slope down
   std::size_t j = 0;
   while( ( j + 1 < D.size() ) &&
          ! detail::plaLess( D[ j ] , pu , D[ j + 1 ] , pu ) ) ++j;
   r1 = D[ j ];
   r3 = pu;
   D.erase( D.begin() , D.begin() + long( j ) );
   }
  if( detail::plaLess( r0 , r2 , r0 , pd ) ) {   // the least slope up
   std::size_t j = 0;
   while( ( j + 1 < U.size() ) &&
          ! detail::plaLess( U[ j + 1 ] , pd , U[ j ] , pd ) ) ++j;
   r0 = U[ j ];
   r2 = pd;
   U.erase( U.begin() , U.begin() + long( j ) );
   }
  while( ( U.size() >= 2 ) &&                // the lower hull of the ups
         ( detail::plaCross( U[ U.size() - 2 ] , U.back() , pu ) <= 0 ) )
   U.pop_back();
  U.push_back( pu );
  while( ( D.size() >= 2 ) &&                // the upper hull of the downs
         ( detail::plaCross( D[ D.size() - 2 ] , D.back() , pd ) >= 0 ) )
   D.pop_back();
  D.push_back( pd );
  ++count;
  }
 if( count > 0 )
  close();
 L.start.push_back( std::uint32_t( total ) );
 return( L );
 }

ADS_PGM_T
ADS_PGM::PGMIndex( std::vector< K > A ) : f_A( A.begin() , A.end() )
{
 ADS_PRE( f_A.size() < ( std::size_t( 1 ) << 32 ) );
 ADS_AUDIT( std::is_sorted( f_A.begin() , f_A.end() ) );
 if( f_A.empty() )
  return;
 std::vector< std::uint64_t > x;            // the distinct keys
 std::vector< std::uint32_t > y;            // and their first places
 for( std::size_t i = 0 ; i < f_A.size() ; ++i ) {
  const std::uint64_t k = ord( f_A[ i ] );
  if( x.empty() || ( k != x.back() ) ) {
   x.push_back( k );
   y.push_back( std::uint32_t( i ) );
   }
  }
 f_L.push_back( approximate( x , y , std::int64_t( E ) , f_A.size() ) );
 while( f_L.back().key.size() > 1 ) {       // the levels above
  const std::vector< std::uint64_t > k = f_L.back().key;
  std::vector< std::uint32_t > p( k.size() );
  for( std::size_t i = 0 ; i < p.size() ; ++i ) p[ i ] = std::uint32_t( i );
  f_L.push_back( approximate( k , p , std::int64_t( R ) , k.size() ) );
  }
 }

ADS_PGM_T
inline std::size_t ADS_PGM::predict( const Level & l , std::size_t s ,
                                     std::uint64_t x )
{
 const Line & q = l.line[ s ];
 const double p = q.icpt + q.slope * double( x - l.key[ s ] );
 const double lo = double( l.start[ s ] ) , hi = double( l.start[ s + 1 ] );
 return( std::size_t( p < lo ? lo : p > hi ? hi : p ) );
 }

ADS_PGM_T
inline std::size_t ADS_PGM::lowerBound( const K & v ) const
{
 const std::size_t n = f_A.size();
 if( n == 0 )
  return( 0 );
 const std::uint64_t x = ord( v );
 if( x < f_L[ 0 ].key[ 0 ] )                // before all the keys
  return( 0 );
 std::size_t s = 0;
 for( std::size_t l = f_L.size() - 1 ; l > 0 ; --l ) {   // down the levels
  const std::vector< std::uint64_t > & k = f_L[ l - 1 ].key;
  const std::size_t p = predict( f_L[ l ] , s , x );
  const std::size_t lo = p > R + 2 ? p - R - 2 : 0;
  const std::size_t hi = std::min( k.size() , p + R + 3 );
  // the last first key not larger than x
  s = detail::windowLowerBound( k.data() , lo , hi , x + 1 ) - 1;
  if( x == ~std::uint64_t( 0 ) )            // x + 1 wraps
   s = std::size_t( std::upper_bound( k.begin() + long( lo ) ,
                                      k.begin() + long( hi ) , x ) -
                    k.begin() ) - 1;
  }
 const std::size_t p = predict( f_L[ 0 ] , s , x );
 std::size_t lo = p > E + 2 ? p - E - 2 : 0;
 std::size_t hi = std::min( n , p + E + 3 );
 // the lines of the window are all prefetched at once, so that their
 // misses overlap, and searched with no branch
 for( std::size_t o = lo ; o < hi ; o += 64 / sizeof( K ) )
  ADS_PGM_PREFETCH( f_A.data() + o );
 std::size_t r = detail::windowLowerBound( f_A.data() , lo , hi , v );
 if( ( r == hi ) && ( hi < n ) ) {          // repeated keys: gallop right
  std::size_t d = 1;
  while( ( hi + d < n ) && ( f_A[ hi + d ] < v ) ) d *= 2;
  r = detail::windowLowerBound( f_A.data() , hi , std::min( n , hi + d + 1 ) ,
                                v );
  }
 else if( ( r == lo ) && ( lo > 0 ) && !( f_A[ lo - 1 ] < v ) ) {
  std::size_t d = 1;                        // gallop left
  while( ( d < lo ) && !( f_A[ lo - d ] < v ) ) d *= 2;
  r = detail::windowLowerBound( f_A.data() , d < lo ? lo - d : 0 , lo , v );
  }
 ADS_POST( ( ( r == n ) || !( f_A[ r ] < v ) ) &&
           ( ( r == 0 ) || ( f_A[ r - 1 ] < v ) ) );
 return( r );
 }

ADS_PGM_T
inline bool ADS_PGM::contains( const K & v ) const
{
 const std::size_t r = lowerBound( v );
 return( ( r < f_A.size() ) && !( v < f_A[ r ] ) );
 }

ADS_PGM_T
const K & ADS_PGM::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_A.size() );
 return( f_A[ i ] );
 }

ADS_PGM_T
std::size_t ADS_PGM::size( void ) const
{
 return( f_A.size() );
 }

ADS_PGM_T
bool ADS_PGM::empty( void ) const
{
 return( f_A.empty() );
 }

ADS_PGM_T
std::size_t ADS_PGM::segments( void ) const
{
 return( f_L.empty() ? 0 : f_L[ 0 ].key.size() );
 }

ADS_PGM_T
int ADS_PGM::height( void ) const
{
 return( int( f_L.size() ) );
 }

ADS_PGM_T
std::size_t ADS_PGM::bytes( void ) const
{
 std::size_t b = 0;
 for( const Level & l : f_L )
  b += l.key.size() * sizeof( std::uint64_t ) +
       l.line.size() * sizeof( Line ) +
       l.start.size() * sizeof( std::uint32_t );
 return( b );
 }

#undef ADS_PGM
#undef ADS_PGM_PREFETCH
#undef ADS_PGM_T

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File PGMIndex.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
