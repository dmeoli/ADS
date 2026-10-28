/*--------------------------------------------------------------------------*/
/*----------------------- File CompressedPGMIndex.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CompressedPGMIndex [see CompressedPGMIndex.h], with
 * their contracts.
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

#include "ADS/combinatorial/Bitwise.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace ADS {

#define ADS_CPGM_T template< class K , std::size_t E > \
 requires std::is_arithmetic_v< K >
#define ADS_CPGM CompressedPGMIndex< K , E >

ADS_CPGM_T
ADS_CPGM::CompressedPGMIndex( std::vector< K > A )
 : f_A( A.begin() , A.end() )
{
 ADS_PRE( f_A.size() < ( std::size_t( 1 ) << 32 ) );
 ADS_AUDIT( std::is_sorted( f_A.begin() , f_A.end() ) );
 const std::size_t n = f_A.size();
 if( n == 0 )
  return;
 std::vector< std::uint64_t > x;            // the distinct keys
 std::vector< std::uint32_t > y;            // and their first places
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint64_t k = detail::pgmOrd( f_A[ i ] );
  if( x.empty() || ( k != x.back() ) ) {
   x.push_back( k );
   y.push_back( std::uint32_t( i ) );
   }
  }
 std::vector< detail::PlaSegment > G;
 detail::optimalPla( x.size() ,
                     [ & ]( std::size_t i ) { return( x[ i ] ); } ,
                     [ & ]( std::size_t i ) { return( y[ i ] ); } ,
                     std::int64_t( E ) ,
                     [ & ]( const detail::PlaSegment & g ) {
                      G.push_back( g ); } );
 f_m = G.size();

 // the stabbing of the intervals of slopes, sorted by their left end
 std::vector< std::size_t > by( f_m );
 std::iota( by.begin() , by.end() , std::size_t( 0 ) );
 std::stable_sort( by.begin() , by.end() ,
                   [ & ]( std::size_t a , std::size_t b ) {
                    return( G[ a ].lo < G[ b ].lo ); } );
 std::vector< std::uint32_t > sig( f_m );
 auto middle = []( long double a , long double b ) {
  return( float( std::isinf( b ) ? a : ( a + b ) / 2 ) ); };
 long double a = G[ by[ 0 ] ].lo , b = G[ by[ 0 ] ].hi;
 for( std::size_t k = 1 ; k < f_m ; ++k ) {
  const detail::PlaSegment & g = G[ by[ k ] ];
  if( g.lo > b ) {                          // a new group
   f_slope.push_back( middle( a , b ) );
   a = g.lo;
   b = g.hi;
   }
  else {
   a = std::max( a , g.lo );
   b = std::min( b , g.hi );
   }
  sig[ by[ k ] ] = std::uint32_t( f_slope.size() );
  }
 f_slope.push_back( middle( a , b ) );

 // with its slope, the intercept of a segment in the middle of those
 // within the error, as its distance from the first place; a segment that
 // the rounding of the line leaves out of the error takes its own slope
 std::vector< std::int64_t > delta( f_m );
 for( std::size_t s = 0 ; s < f_m ; ++s ) {
  const detail::PlaSegment & g = G[ s ];
  const std::size_t f = g.first , l = g.first + g.count;
  const std::uint32_t y0 = y[ f ];
  const std::uint32_t y1 = s + 1 < f_m ? y[ G[ s + 1 ].first ]
                                       : std::uint32_t( n );
  // the intercept of the slope sl, and whether its line, as the search
  // computes it, is within the error
  auto fit = [ & ]( float sl , std::int64_t & d ) {
   long double lo = -HUGE_VALL , hi = HUGE_VALL;
   for( std::size_t k = f ; k < l ; ++k ) {
    const long double v = (long double) y[ k ] -
                          (long double) sl * (long double)( x[ k ] - x[ f ] );
    lo = std::max( lo , v );
    hi = std::min( hi , v );
    }
   d = std::int64_t( std::llround( ( lo + hi ) / 2 ) ) - std::int64_t( y0 );
   for( std::size_t k = f ; k < l ; ++k ) {
    const double q = double( std::int64_t( y0 ) + d ) +
                     double( sl ) * double( x[ k ] - x[ f ] );
    const double p = q < double( y0 ) ? double( y0 ) :
                     q > double( y1 ) ? double( y1 ) : std::floor( q );
    if( std::fabs( p - double( y[ k ] ) ) > double( E ) + 1 )
     return( false );
    }
   return( true );
   };
  if( ! fit( f_slope[ sig[ s ] ] , delta[ s ] ) ) {   // its own slope
   sig[ s ] = std::uint32_t( f_slope.size() );
   f_slope.push_back( float( g.slope ) );
   ( void ) fit( f_slope.back() , delta[ s ] );
   }
  }

 // the slopes and the intercepts in fields of bits
 f_dmin = *std::min_element( delta.begin() , delta.end() );
 const std::uint64_t dr = std::uint64_t(
  *std::max_element( delta.begin() , delta.end() ) - f_dmin );
 f_wd = dr == 0 ? 0 : lambda( dr ) + 1;
 f_ws = f_slope.size() <= 1 ? 0 : lambda( f_slope.size() - 1 ) + 1;
 f_code.reserve( f_m * std::size_t( f_ws + f_wd ) );
 for( std::size_t s = 0 ; s < f_m ; ++s )
  if( f_ws + f_wd > 0 )
   f_code.append( std::uint64_t( sig[ s ] ) |
                  ( std::uint64_t( delta[ s ] - f_dmin ) << f_ws ) ,
                  f_ws + f_wd );

 // the first keys, by a PGM-index of their own, and the first places
 std::vector< std::uint64_t > ks , ps;
 for( std::size_t s = 0 ; s < f_m ; ++s ) {
  ks.push_back( x[ G[ s ].first ] );
  ps.push_back( y[ G[ s ].first ] );
  }
 ps.push_back( n );
 f_start = EliasFano( ps , n + 1 );
 f_key = PGMIndex< std::uint64_t , 4 , 4 >( ks );
 }

ADS_CPGM_T
inline std::size_t ADS_CPGM::lowerBound( const K & v ) const
{
 const std::size_t n = f_A.size();
 if( n == 0 )
  return( 0 );
 const std::uint64_t x = detail::pgmOrd( v );
 if( x < f_key[ 0 ] )                       // before all the keys
  return( 0 );
 // the last first key not larger than x, by the PGM-index of the keys
 std::size_t s = f_key.lowerBound( x );
 if( ( s == f_m ) || ( f_key[ s ] != x ) ) --s;
 const std::size_t p0 = std::size_t( f_start.access( s ) );
 const std::size_t p1 = std::size_t( f_start.access( s + 1 ) );
 const std::uint64_t c = f_code.get( s * std::size_t( f_ws + f_wd ) ,
                                     f_ws + f_wd );
 const std::size_t si = std::size_t( c & detail::lowMask( f_ws ) );
 const std::int64_t d = f_dmin + std::int64_t( f_ws >= 64 ? 0 : c >> f_ws );
 const double q = double( std::int64_t( p0 ) + d ) +
                  double( f_slope[ si ] ) * double( x - f_key[ s ] );
 const std::size_t p = q < double( p0 ) ? p0 :
                       q > double( p1 ) ? p1 : std::size_t( q );
 const std::size_t r = detail::pgmLastMile( f_A.data() , n , p , E , v );
 ADS_POST( ( ( r == n ) || !( f_A[ r ] < v ) ) &&
           ( ( r == 0 ) || ( f_A[ r - 1 ] < v ) ) );
 return( r );
 }

ADS_CPGM_T
inline bool ADS_CPGM::contains( const K & v ) const
{
 const std::size_t r = lowerBound( v );
 return( ( r < f_A.size() ) && !( v < f_A[ r ] ) );
 }

ADS_CPGM_T
const K & ADS_CPGM::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_A.size() );
 return( f_A[ i ] );
 }

ADS_CPGM_T
std::size_t ADS_CPGM::size( void ) const
{
 return( f_A.size() );
 }

ADS_CPGM_T
bool ADS_CPGM::empty( void ) const
{
 return( f_A.empty() );
 }

ADS_CPGM_T
std::size_t ADS_CPGM::segments( void ) const
{
 return( f_m );
 }

ADS_CPGM_T
std::size_t ADS_CPGM::slopes( void ) const
{
 return( f_slope.size() );
 }

ADS_CPGM_T
std::size_t ADS_CPGM::bytes( void ) const
{
 return( f_key.size() * sizeof( std::uint64_t ) + f_key.bytes() +
         ( f_start.sizeInBits() + f_code.size() + 7 ) /
         8 + f_slope.size() * sizeof( float ) );
 }

#undef ADS_CPGM
#undef ADS_CPGM_T

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File CompressedPGMIndex.tpp -------------------*/
/*--------------------------------------------------------------------------*/
