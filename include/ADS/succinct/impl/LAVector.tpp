/*--------------------------------------------------------------------------*/
/*---------------------------- File LAVector.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of LAVector.h, with their contracts.
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
#include <limits>

namespace ADS {

inline std::uint64_t LAVector::whole( double v )
{
 if( !( v > 0 ) )
  return( 0 );
 if( v >= 18446744073709551616.0 )
  return( ~std::uint64_t( 0 ) );
 return( std::uint64_t( v ) );
 }

inline int LAVector::widest( const std::vector< std::uint64_t > & x )
{
 if( x.empty() )
  return( 1 );
 const std::uint64_t g = std::max< std::uint64_t >( x.back() / x.size() ,
                                                    1 );
 return( std::min( 63 , std::max( lambda( g ) + 6 ,
                                  ( lambda( std::max< std::uint64_t >(
                                     x.back() , 1 ) ) + 2 ) / 2 ) ) );
 }

inline std::int64_t LAVector::error( int c )
{
 return( c <= 1 ? 0 : ( std::int64_t( 1 ) << ( c - 1 ) ) - 1 );
 }

inline void LAVector::append( const std::vector< std::uint64_t > & x ,
                              std::size_t f , std::size_t l , double a )
{
 const std::uint64_t y0 = x[ f ];
 // the distances from the line through the first item, small whatever
 // the items since the line is within the error of each of them; their
 // least one goes into the base
 std::int64_t lo = std::numeric_limits< std::int64_t >::max();
 std::int64_t hi = std::numeric_limits< std::int64_t >::min();
 for( std::size_t k = 0 ; k < l - f ; ++k ) {
  const std::int64_t d = std::int64_t( x[ f + k ] - y0 -
                                       whole( a * double( k ) ) );
  lo = std::min( lo , d );
  hi = std::max( hi , d );
  }
 const std::uint64_t r = std::uint64_t( hi ) - std::uint64_t( lo );
 const std::uint64_t w = r == 0 ? 0 : std::uint64_t( lambda( r ) + 1 );
 f_seg.push_back( Segment{ y0 + std::uint64_t( lo ) , a ,
                           ( f_C.size() << 7 ) | w } );
 f_first.push_back( f );
 if( w > 0 )
  for( std::size_t k = 0 ; k < l - f ; ++k ) {
   const std::int64_t d = std::int64_t( x[ f + k ] - y0 -
                                        whole( a * double( k ) ) );
   f_C.append( std::uint64_t( d ) - std::uint64_t( lo ) , int( w ) );
   }
 }

inline void LAVector::finish( const std::vector< std::uint64_t > & x )
{
 const std::size_t m = f_seg.size();
 f_first.push_back( f_n );
 f_p.pack( f_first );
 f_last = x.back();

 // the segment of every 2^t-th place, 2^t about n / m
 f_t = f_n / m <= 1 ? 0 : lambda( f_n / m );
 std::vector< std::uint64_t > T( ( ( f_n - 1 ) >> f_t ) + 1 );
 for( std::size_t b = 0 , j = 0 ; b < T.size() ; ++b ) {
  while( f_first[ j + 1 ] <= ( b << f_t ) ) ++j;
  T[ b ] = j;
  }
 f_J.pack( T );

 // the segments whose first item is in a bucket before h, about m buckets
 // of the items less x_0 by their high bits
 f_x0 = x[ 0 ];
 const std::uint64_t R = f_last - f_x0;
 f_l = 0;
 while( ( f_l < 64 ) && ( ( R >> f_l ) >= m ) ) ++f_l;
 const std::size_t H = std::size_t( f_l >= 64 ? 0 : R >> f_l ) + 1;
 T.assign( H + 1 , 0 );
 for( std::size_t j = 0 ; j < m ; ++j ) {
  const std::uint64_t d = x[ f_first[ j ] ] - f_x0;
  ++T[ std::size_t( f_l >= 64 ? 0 : d >> f_l ) + 1 ];
  }
 for( std::size_t h = 1 ; h <= H ; ++h ) T[ h ] += T[ h - 1 ];
 f_V.pack( T );
 f_first = std::vector< std::uint64_t >();
 ADS_AUDIT( decode() == x );
 }

inline LAVector::LAVector( const std::vector< std::uint64_t > & x , int c )
 : f_n( x.size() ) , f_c( c )
{
 ADS_PRE( ( c >= 0 ) && ( c <= 63 ) );
 ADS_PRE( x.size() < ( std::size_t( 1 ) << 32 ) );
 ADS_AUDIT( std::is_sorted( x.begin() , x.end() ) );
 if( f_n == 0 )
  return;
 f_C.reserve( f_n * std::size_t( std::max( c , 1 ) ) );
 detail::optimalPla( f_n ,
                     []( std::size_t i ) { return( std::uint64_t( i ) ); } ,
                     [ & ]( std::size_t i ) { return( x[ i ] ); } ,
                     error( c ) , [ & ]( const detail::PlaSegment & g ) {
                      append( x , g.first , g.first + g.count ,
                              double( g.slope ) ); } );
 finish( x );
 }

inline LAVector LAVector::tuned( const std::vector< std::uint64_t > & x )
{
 // the odd widths up to the widest useful one, then the neighbours of
 // the best
 const int top = widest( x );
 LAVector b( x , 1 );
 for( int c = 3 ; c <= top ; c += 2 ) {
  LAVector v( x , c );
  if( v.sizeInBits() < b.sizeInBits() )
   b = std::move( v );
  }
 for( const int c : { b.f_c - 1 , b.f_c + 1 } )
  if( ( c >= 2 ) && ( c <= 63 ) ) {
   LAVector v( x , c );
   if( v.sizeInBits() < b.sizeInBits() )
    b = std::move( v );
   }
 return( b );
 }

inline std::size_t LAVector::size( void ) const
{
 return( f_n );
 }

inline bool LAVector::empty( void ) const
{
 return( f_n == 0 );
 }

inline int LAVector::correctionBits( void ) const
{
 return( f_c );
 }

inline std::size_t LAVector::segments( void ) const
{
 return( f_seg.size() );
 }

inline std::size_t LAVector::segmentOf( std::size_t i ,
                                        std::size_t & p ) const
{
 std::size_t j = f_J[ i >> f_t ];
 auto [ a , b ] = f_p.pair( j );
 while( b <= i ) {
  ++j;
  a = b;
  b = f_p[ j + 1 ];
  }
 p = a;
 return( j );
 }

inline std::uint64_t LAVector::item( std::size_t j , std::size_t d ) const
{
 const Segment & s = f_seg[ j ];
 const std::size_t w = std::size_t( s.ow & 127 );
 return( s.base + whole( s.slope * double( d ) ) +
         f_C.get( std::size_t( s.ow >> 7 ) + d * w , int( w ) ) );
 }

inline std::uint64_t LAVector::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::size_t p;
 const std::size_t j = segmentOf( i , p );
 return( item( j , i - p ) );
 }

inline std::uint64_t LAVector::operator[]( std::size_t i ) const
{
 return( access( i ) );
 }

inline std::uint64_t LAVector::select( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_n ) );
 return( access( k - 1 ) );
 }

inline std::size_t LAVector::nextGEQ( std::uint64_t x ) const
{
 if( ( f_n == 0 ) || ( x <= f_x0 ) )
  return( 0 );
 if( x > f_last )
  return( f_n );
 // the last segment whose first item is below x: x is above x_0, so there
 // is one, and among those of the bucket of x
 const std::uint64_t d = x - f_x0;
 const std::size_t h = std::size_t( f_l >= 64 ? 0 : d >> f_l );
 std::size_t s0 = f_V[ h ] , s1 = f_V[ h + 1 ];   // the first item >= x
 while( s0 < s1 ) {
  const std::size_t mid = s0 + ( s1 - s0 ) / 2;
  if( item( mid , 0 ) < x ) s0 = mid + 1;
  else s1 = mid;
  }
 const std::size_t j = s0 - 1;
 const std::size_t p = f_p[ j ] , q = f_p[ j + 1 ];
 const Segment & s = f_seg[ j ];
 const std::size_t len = q - p;
 const std::size_t w = std::size_t( s.ow & 127 );
 // the line of the segment, not decreasing in e: f( e ) <= x_{p+e} <
 // f( e ) + 2^w with f( e ) = base + g( e ), g( e ) the integer part of
 // the slope times e; the answer is between the first e with f( e ) above
 // x - 2^w and the first with f( e ) >= x, found by the line alone. The
 // base may be below 0 (a negative least correction, kept modulo 2^64),
 // and x - base above 2^64, and so the targets of g are computed apart
 auto g = [ & ]( std::size_t e ) { return( whole( s.slope * double( e ) ) ); };
 // the first e with g( e ) >= u, len if none
 auto reach = [ & ]( std::uint64_t u ) -> std::size_t {
  if( u == 0 )
   return( 0 );
  std::size_t e = len - 1;                  // the line inverted, a guess
  if( s.slope > 0 ) {
   const double c = double( u ) / s.slope;
   e = c >= double( len - 1 ) ? len - 1 : std::size_t( c );
   }
  std::size_t a , b;                        // g( a - 1 ) < u <= g( b )
  if( g( e ) >= u ) {                       // gallop down from the guess
   b = e;
   for( std::size_t h = 1 ; ; h *= 2 ) {
    if( b < h ) { a = 0; break; }
    if( g( b - h ) < u ) { a = b - h + 1; break; }
    b -= h;
    }
   }
  else {                                    // or up
   a = e + 1;
   for( std::size_t h = 1 ; ; h *= 2 ) {
    if( a + h - 1 >= len ) { b = len; break; }
    if( g( a + h - 1 ) >= u ) { b = a + h - 1; break; }
    a += h;
    }
   }
  while( a < b ) {
   const std::size_t m = a + ( b - a ) / 2;
   if( g( m ) < u ) a = m + 1;
   else b = m;
   }
  return( a );
  };
 // x less the base, from the first item, which is above the base by its
 // correction c0 and below x: dx + c0, len if it is 2^64 or more
 const std::uint64_t M = ~std::uint64_t( 0 );
 const std::uint64_t c0 = f_C.get( std::size_t( s.ow >> 7 ) , int( w ) );
 const std::uint64_t dx = x - ( s.base + c0 );
 const std::uint64_t span = w >= 64 ? M : ( std::uint64_t( 1 ) << w ) - 1;
 const std::size_t up = dx > M - c0 ? len : reach( dx + c0 );
 std::size_t lo;                            // dx + c0 - span, if above 0
 if( span >= c0 )
  lo = dx > span - c0 ? reach( dx - ( span - c0 ) ) : 0;
 else
  lo = dx > M - ( c0 - span ) ? len : reach( dx + ( c0 - span ) );
 std::size_t a = lo , n = up - lo;
 // the first item >= x in [ p + a , p + a + n ], with no branch
 while( n > 0 ) {
  const std::size_t h = n / 2;
  const bool less = item( j , a + h ) < x;
  a = less ? a + h + 1 : a;
  n = less ? n - h - 1 : h;
  }
 a += p;
 ADS_POST( ( a == f_n ) || ( access( a ) >= x ) );
 ADS_POST( ( a == 0 ) || ( access( a - 1 ) < x ) );
 return( a );
 }

inline std::size_t LAVector::rank( std::uint64_t x ) const
{
 return( nextGEQ( x ) );
 }

inline bool LAVector::contains( std::uint64_t x ) const
{
 const std::size_t p = nextGEQ( x );
 return( ( p < f_n ) && ( access( p ) == x ) );
 }

inline std::size_t LAVector::correctionsInBits( void ) const
{
 return( f_C.size() );
 }

inline std::size_t LAVector::sizeInBits( void ) const
{
 return( f_C.size() + 8 * f_seg.size() * sizeof( Segment ) +
         f_p.bits + f_J.bits + f_V.bits );
 }

inline std::vector< std::uint64_t > LAVector::decode( void ) const
{
 std::vector< std::uint64_t > x;
 x.reserve( f_n );
 for( std::size_t j = 0 ; j < f_seg.size() ; ++j ) {
  const Segment & s = f_seg[ j ];
  const int w = int( s.ow & 127 );
  std::size_t o = std::size_t( s.ow >> 7 );
  for( std::size_t d = 0 ; d < std::size_t( f_p[ j + 1 ] - f_p[ j ] ) ;
       ++d , o += std::size_t( w ) )
   x.push_back( s.base + whole( s.slope * double( d ) ) + f_C.get( o , w ) );
  }
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File LAVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
