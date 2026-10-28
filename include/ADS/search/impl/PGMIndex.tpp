/*--------------------------------------------------------------------------*/
/*---------------------------- File PGMIndex.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PGMIndex [see PGMIndex.h], with their contracts.
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
#include <cmath>
#include <cstring>

#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_PGM_PREFETCH( p ) __builtin_prefetch( p )
#else
 #define ADS_PGM_PREFETCH( p ) ( ( void ) 0 )
#endif

namespace ADS {

namespace detail {

/// a point of the plane, its ordinate of 128 bits and its abscissa of 64,
/// so that the products of the hulls, of the differences of the ordinates
/// by those of the abscissae (never negative, the abscissae increasing),
/// are exact
struct PlaPoint {
 std::uint64_t x;
 Int128 y;
 };

/// the cross product of b - a and c - a, positive on a left turn
inline Int128 plaCross( const PlaPoint & a , const PlaPoint & b ,
                        const PlaPoint & c )
{
 return( Int128( b.x - a.x ) * ( c.y - a.y ) -
         ( b.y - a.y ) * Int128( c.x - a.x ) );
 }

/// whether the slope of a -> b is smaller than that of c -> d, b and d on
/// the right of a and c
inline bool plaLess( const PlaPoint & a , const PlaPoint & b ,
                     const PlaPoint & c , const PlaPoint & d )
{
 return( ( b.y - a.y ) * Int128( d.x - c.x ) <
         ( d.y - c.y ) * Int128( b.x - a.x ) );
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

/// a segment of the optimal approximation of `optimalPla()`: its points,
/// the least and the largest slope of the lines within the error, and the
/// line kept, by its slope and its value at the first abscissa less the
/// first ordinate
struct PlaSegment {
 std::size_t first;        ///< the first point
 std::size_t count;        ///< the number of points
 long double lo;           ///< the least slope, not negative
 long double hi;           ///< the largest slope, at least lo
 long double slope;        ///< the slope of the line kept, in [ lo , hi ]
 long double icpt;         ///< the line at x( first ), less y( first )
 };

/// the optimal approximation with error e of a stream of points, x
/// increasing: the algorithm of O'Rourke as engineered by Xie et al. [see
/// PGMIndex.h], each point updating the two convex hulls and the two
/// extreme lines in amortized O( 1 ), and refused when it leaves no line;
/// the abscissae are integers of 64 bits and the ordinates of 128, so that
/// the products are exact [see `PlaPoint`], and the line is computed in
/// long double relative to the first point
class PlaStream {

 public:

 /// a stream with the error e and no point
 explicit PlaStream( std::int64_t e ) : f_e( e ) { }

 /// adds the point ( x , y ), true if a line within the error of all the
 /// points added is left, false (and the point not added) if not
 bool add( std::uint64_t x , Int128 y ) {
  using P = PlaPoint;
  const P pu{ x , y + f_e };
  const P pd{ x , y - f_e };
  if( f_n == 0 ) {
   U.assign( 1 , pu );
   D.assign( 1 , pd );
   u0 = d0 = 0;
   o = P{ x , y };
   f_n = 1;
   return( true );
   }
  if( f_n == 1 ) {
   r0 = U[ 0 ];
   r1 = D[ 0 ];
   r2 = pd;
   r3 = pu;
   U.push_back( pu );
   D.push_back( pd );
   f_n = 2;
   return( true );
   }
  if( plaLess( r2 , pu , r0 , r2 ) || plaLess( r1 , r3 , r3 , pd ) )
   return( false );                         // no line is left
  // a point up that does not lower the largest slope is above every line
  // left, and stays so as the lines only decrease: it never bounds a
  // line, and it is not added to the hull (and so for a point down); the
  // points at the start of a hull are dropped by moving its start
  if( plaLess( r1 , pu , r1 , r3 ) ) {      // the largest slope down
   std::size_t j = d0;
   while( ( j + 1 < D.size() ) &&
          ! plaLess( D[ j ] , pu , D[ j + 1 ] , pu ) ) ++j;
   r1 = D[ j ];
   r3 = pu;
   d0 = j;
   while( ( U.size() >= u0 + 2 ) &&          // the lower hull of the ups
          ( plaCross( U[ U.size() - 2 ] , U.back() , pu ) <= 0 ) )
    U.pop_back();
   U.push_back( pu );
   }
  if( plaLess( r0 , r2 , r0 , pd ) ) {      // the least slope up
   std::size_t j = u0;
   while( ( j + 1 < U.size() ) &&
          ! plaLess( U[ j + 1 ] , pd , U[ j ] , pd ) ) ++j;
   r0 = U[ j ];
   r2 = pd;
   u0 = j;
   while( ( D.size() >= d0 + 2 ) &&          // the upper hull of the downs
          ( plaCross( D[ D.size() - 2 ] , D.back() , pd ) >= 0 ) )
    D.pop_back();
   D.push_back( pd );
   }
  ++f_n;
  return( true );
  }

 /// forgets the points, keeping the memory of the hulls
 void restart( void ) { f_n = 0; }

 /// the number of points added since the last restart
 [[nodiscard]] std::size_t count( void ) const { return( f_n ); }

 /// the segment of the points added, the first one being the point first
 /// of the input
 [[nodiscard]] PlaSegment segment( std::size_t first ) const {
  PlaSegment g{ first , f_n , 0 , HUGE_VALL , 0 , 0 };
  if( f_n <= 1 )
   return( g );
  // the extreme lines, relative to the first point
  const long double a0 = (long double)( r0.x - o.x );
  const long double b0 = (long double)( r0.y - o.y );
  const long double a1 = (long double)( r1.x - o.x );
  const long double b1 = (long double)( r1.y - o.y );
  const long double s1 = (long double)( r2.y - r0.y ) /
                         (long double)( r2.x - r0.x );   // the least
  const long double s2 = (long double)( r3.y - r1.y ) /
                         (long double)( r3.x - r1.x );   // the largest
  long double s , c;                        // the slope, at the first x
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
  g.lo = std::max( s1 , (long double) 0 );
  g.hi = std::max( s2 , g.lo );             // s is in [ lo , hi ]
  g.slope = s;
  g.icpt = c;
  return( g );
  }

 private:

 std::int64_t f_e;                          ///< the error
 std::size_t f_n = 0;                       ///< the points
 std::vector< PlaPoint > U , D;             ///< the hulls, up and down
 std::size_t u0 = 0 , d0 = 0;               ///< where the hulls begin
 PlaPoint r0{} , r1{} , r2{} , r3{};        ///< the two extreme lines
 PlaPoint o{};                              ///< the first point

 };

/// the segments of the optimal approximation with error e of the points
/// ( x( i ) , y( i ) ), i < n, x increasing, passed to out( segment ) from
/// the left, by a `PlaStream` restarted at each point that it refuses
template< class X , class Y , class F >
void optimalPla( std::size_t n , X && x , Y && y , std::int64_t e ,
                 F && out )
{
 PlaStream S( e );
 std::size_t first = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint64_t xi = x( i );
  const Int128 yi( y( i ) );
  if( ! S.add( xi , yi ) ) {
   out( S.segment( first ) );
   S.restart();
   ( void ) S.add( xi , yi );
   first = i;
   }
  }
 if( S.count() > 0 )
  out( S.segment( first ) );
 }

/// the first place of an item of a[ 0 .. n ) not smaller than v, from the
/// prediction p of a model within e places of the answer when the items
/// are distinct: the window [ p - e - 2 , p + e + 3 ), whose lines of cache
/// are all prefetched at once so that their misses overlap, is searched
/// with no branch, and on repeated items, whose places a line cannot all
/// approximate, the search goes on past the window by galloping
template< class T >
inline std::size_t pgmLastMile( const T * a , std::size_t n , std::size_t p ,
                                std::size_t e , const T & v )
{
 std::size_t lo = p > e + 2 ? p - e - 2 : 0;
 std::size_t hi = std::min( n , p + e + 3 );
 for( std::size_t o = lo ; o < hi ; o += 64 / sizeof( T ) )
  ADS_PGM_PREFETCH( a + o );
 std::size_t r = windowLowerBound( a , lo , hi , v );
 if( ( r == hi ) && ( hi < n ) ) {          // gallop right
  std::size_t d = 1;
  while( ( hi + d < n ) && ( a[ hi + d ] < v ) ) d *= 2;
  r = windowLowerBound( a , hi , std::min( n , hi + d + 1 ) , v );
  }
 else if( ( r == lo ) && ( lo > 0 ) && !( a[ lo - 1 ] < v ) ) {
  std::size_t d = 1;                        // gallop left
  while( ( d < lo ) && !( a[ lo - d ] < v ) ) d *= 2;
  r = windowLowerBound( a , d < lo ? lo - d : 0 , lo , v );
  }
 return( r );
 }

/// the key k in an order of integers of 64 bits: the signed by flipping
/// the top bit, the floating by their bits
template< class K >
inline std::uint64_t pgmOrd( const K & k )
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

}  // end( namespace detail )

#define ADS_PGM_T template< class K , std::size_t E , std::size_t R > \
 requires std::is_arithmetic_v< K >
#define ADS_PGM PGMIndex< K , E , R >

ADS_PGM_T
inline std::uint64_t ADS_PGM::ord( const K & k )
{
 return( detail::pgmOrd( k ) );
 }

ADS_PGM_T
auto ADS_PGM::approximate( const std::vector< std::uint64_t > & x ,
                           const std::vector< std::uint32_t > & y ,
                           std::int64_t e , std::size_t total ) -> Level
{
 Level L;
 detail::optimalPla( x.size() ,
                     [ & ]( std::size_t i ) { return( x[ i ] ); } ,
                     [ & ]( std::size_t i ) { return( y[ i ] ); } , e ,
                     [ & ]( const detail::PlaSegment & g ) {
                      L.key.push_back( x[ g.first ] );
                      L.start.push_back( y[ g.first ] );
                      L.line.push_back( Line{ double( (long double)
                       y[ g.first ] + g.icpt ) , double( g.slope ) } );
                      } );
 L.start.push_back( std::uint32_t( total ) );
 return( L );
 }

ADS_PGM_T
ADS_PGM::PGMIndex( const std::vector< K > & A ) : f_A( A.begin() , A.end() )
{
 ADS_PRE( f_A.size() < ( std::size_t( 1 ) << 32 ) );
 ADS_AUDIT( std::is_sorted( f_A.begin() , f_A.end() ) );
 if( f_A.empty() )
  return;
 // the last level, by a stream of the distinct keys and their first
 // places, read off the items with no copy of them
 {
  Level L;
  detail::PlaStream S{ std::int64_t( E ) };
  std::uint64_t k0 = 0 , prev = 0;
  std::uint32_t p0 = 0;
  auto close = [ & ]( void ) {
   const detail::PlaSegment g = S.segment( 0 );
   L.key.push_back( k0 );
   L.start.push_back( p0 );
   L.line.push_back( Line{ double( (long double) p0 + g.icpt ) ,
                           double( g.slope ) } );
   };
  for( std::size_t i = 0 ; i < f_A.size() ; ++i ) {
   const std::uint64_t k = ord( f_A[ i ] );
   if( ( i > 0 ) && ( k == prev ) )
    continue;                               // the first place only
   prev = k;
   const std::uint64_t xi = k;
   const detail::Int128 yi{ std::uint32_t( i ) };
   if( ! S.add( xi , yi ) ) {
    close();
    S.restart();
    ( void ) S.add( xi , yi );
    k0 = k;
    p0 = std::uint32_t( i );
    }
   else if( S.count() == 1 ) {
    k0 = k;
    p0 = std::uint32_t( i );
    }
   }
  close();
  L.start.push_back( std::uint32_t( f_A.size() ) );
  f_L.push_back( std::move( L ) );
  }
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
 const std::size_t r = detail::pgmLastMile( f_A.data() , n ,
                                            predict( f_L[ 0 ] , s , x ) , E ,
                                            v );
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
