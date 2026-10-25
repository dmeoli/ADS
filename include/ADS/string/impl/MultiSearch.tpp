/*--------------------------------------------------------------------------*/
/*--------------------------- File MultiSearch.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of MultiSearcher [see MultiSearch.h], with their
 * contracts.
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

#include "ADS/Portable.h"
#include "ADS/string/StringMatching.h"
#include "ADS/string/StringSearch.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <numeric>

#if defined( __SSE2__ )
 #include <immintrin.h>
#endif
#include <utility>

namespace ADS {

namespace detail {

/// the occurrences ( start , number ) of the patterns P in T by
/// naiveMatching(), sorted
inline std::vector< std::pair< std::size_t , std::size_t > > multiNaive(
 std::string_view T , const std::vector< std::string > & P )
{
 std::vector< std::pair< std::size_t , std::size_t > > N;
 for( std::size_t j = 0 ; j < P.size() ; ++j )
  for( const std::size_t i : naiveMatching( T , P[ j ] ) )
   N.emplace_back( i , j );
 std::sort( N.begin() , N.end() );
 return( N );
 }

}  // end( namespace detail )

inline MultiSearcher::MultiSearcher( const std::vector< std::string > & P )
 : f_P( P )
{
 ADS_PRE( std::none_of( P.begin() , P.end() ,
                        []( const std::string & p ) { return( p.empty() ); }
                        ) );
 if( P.empty() ) return;
 f_least = P[ 0 ].size();
 for( const auto & p : P ) {
  f_least = std::min( f_least , p.size() );
  f_longest = std::max( f_longest , p.size() );
  }
 f_ac = AhoCorasick( P );
 // the patterns sorted, into 8 buckets of consecutive ones
 std::vector< std::uint32_t > order( P.size() );
 std::iota( order.begin() , order.end() , 0u );
 std::sort( order.begin() , order.end() , [ & ]( std::uint32_t a ,
                                                  std::uint32_t b ) {
  return( P[ a ] < P[ b ] ); } );
 const std::size_t per = ( P.size() + 7 ) / 8;
 if( P.size() > kFilter ) {
  // the shift-or filter: the byte L - 1 - r of the mask of the super
  // character x (a byte and the low 3 bits of the next one) has the bit b
  // off if a pattern of the bucket b has x at its place r < L; the last
  // place of a pattern of length L takes any next byte
  f_L = std::min< std::size_t >( f_least , 8 );
  // the bytes past the window block nothing
  f_sh.assign( 2048 , f_L == 8 ? ~std::uint64_t( 0 ) :
                      ( std::uint64_t( 1 ) << ( 8 * f_L ) ) - 1 );
  for( std::size_t z = 0 ; z < order.size() ; ++z ) {
   const std::size_t b = z / per;
   const std::string & p = P[ order[ z ] ];
   for( std::size_t r = 0 ; r < f_L ; ++r ) {
    const unsigned c = static_cast< unsigned char >( p[ r ] );
    const std::uint64_t off = ~( std::uint64_t( 1 ) <<
                                 ( 8 * ( f_L - 1 - r ) + b ) );
    if( r + 1 < p.size() )
     f_sh[ c | ( ( static_cast< unsigned char >( p[ r + 1 ] ) & 7u ) << 8 ) ]
      &= off;
    else
     for( unsigned x = 0 ; x < 8 ; ++x ) f_sh[ c | ( x << 8 ) ] &= off;
    }
   }
  // with patterns of 8 bytes or more, the filter of stride 2: the masks
  // of the super characters of the even places alone, W = min( 8 , l / 2
  // ) of them a window, a pattern in 4 buckets twice, at its even offsets
  // (the bits 0 to 3) and at its odd ones (the bits 4 to 7)
  if( f_least >= 8 ) {
   f_W = std::min< std::size_t >( f_least / 2 , 8 );
   f_sh2.assign( 2048 , f_W == 8 ? ~std::uint64_t( 0 ) :
                       ( std::uint64_t( 1 ) << ( 8 * f_W ) ) - 1 );
   const std::size_t per2 = ( P.size() + 3 ) / 4;
   for( std::size_t z = 0 ; z < order.size() ; ++z ) {
    const std::string & p = P[ order[ z ] ];
    for( unsigned phase = 0 ; phase < 2 ; ++phase )
     for( std::size_t i = 0 ; i < f_W ; ++i ) {
      const std::size_t o = 2 * i + phase;
      const unsigned c = static_cast< unsigned char >( p[ o ] );
      const std::uint64_t off = ~( std::uint64_t( 1 ) <<
                                   ( 8 * ( f_W - 1 - i ) + 4 * phase +
                                     z / per2 ) );
      if( o + 1 < p.size() )
       f_sh2[ c | ( ( static_cast< unsigned char >( p[ o + 1 ] ) & 7u )
                    << 8 ) ] &= off;
      else
       for( unsigned x = 0 ; x < 8 ; ++x ) f_sh2[ c | ( x << 8 ) ] &= off;
      }
    }
   }
  // the stride of 2 kept only if a window passes with probability below
  // 2^-8, the super characters of the text drawn with their frequencies
  // in the patterns: \sum_bits \prod_t ( the share of the super
  // characters with that bit off in the byte t ), its 4 buckets holding
  // twice the patterns of the 8 of the stride of 1
  if( f_W ) {
   std::vector< double > f( 2048 , 0.0 );
   double total = 0;
   for( const auto & p : P )
    for( std::size_t o = 0 ; o + 1 < p.size() ; ++o ) {
     f[ static_cast< unsigned char >( p[ o ] ) |
        ( ( static_cast< unsigned char >( p[ o + 1 ] ) & 7u ) << 8 ) ] += 1;
     total += 1;
     }
   double pass = 0;
   for( unsigned bit = 0 ; bit < 8 ; ++bit ) {
    double here = 1;
    for( std::size_t t = 0 ; t < f_W ; ++t ) {
     double q = 0;
     for( std::size_t x = 0 ; x < 2048 ; ++x )
      if( ! ( ( f_sh2[ x ] >> ( 8 * t + bit ) ) & 1 ) ) q += f[ x ] / total;
     here *= q;
     }
    pass += here;
    }
   if( pass >= 1.0 / 256 ) {
    f_W = 0;
    std::vector< std::uint64_t >().swap( f_sh2 );
    }
   }
  // the table of the prefixes of L bytes, chained
  f_bits = std::max( 4 , int( std::bit_width( 2 * P.size() ) ) );
  f_first.assign( std::size_t( 1 ) << f_bits , ~std::uint32_t( 0 ) );
  f_next.resize( P.size() );
  f_key.resize( P.size() );
  for( std::size_t j = P.size() ; j-- > 0 ; ) {
   std::uint64_t k = 0;
   std::memcpy( &k , P[ j ].data() , f_L );
   f_key[ j ] = k;
   const std::size_t h = std::size_t( ( k * 0x9E3779B97F4A7C15ull ) >>
                                      ( 64 - f_bits ) );
   f_next[ j ] = f_first[ h ];
   f_first[ h ] = std::uint32_t( j );
   }
  return;
  }
 f_q = std::min< std::size_t >( f_least , 8 );
 for( std::size_t z = 0 ; z < order.size() ; ++z ) {
  const std::size_t b = z / per;
  const std::uint32_t j = order[ z ];
  f_bucket[ b ].push_back( j );
  for( std::size_t r = 0 ; r < f_q ; ++r ) {
   const auto c = static_cast< unsigned char >( P[ j ][ r ] );
   f_lo[ r ][ c & 15 ] |= std::uint8_t( 1u << b );
   f_hi[ r ][ c >> 4 ] |= std::uint8_t( 1u << b );
   }
  }
 // the bytes of the filter: as many as make a place of a text with the
 // bytes of the patterns pass for some bucket with probability below
 // 2^-10, by the frequencies of the bytes in the patterns, at most 8
 {
  std::array< double , 256 > f{};
  double total = 0;
  for( const auto & p : P )
   for( const char x : p ) {
    f[ static_cast< unsigned char >( x ) ] += 1;
    total += 1;
    }
  // a place passes for the bucket b with probability prod_r pass_b( r ),
  // and for some bucket with at most the sum over b
  std::array< double , 8 > pass;
  pass.fill( 1.0 );
  std::size_t q = 0;
  for( double any = 1 ; ( q < f_q ) && ( any >= 1.0 / 1024 ) ; ++q ) {
   any = 0;
   for( unsigned b = 0 ; b < 8 ; ++b ) {
    double here = 0;
    for( unsigned x = 0 ; x < 256 ; ++x )
     if( f_lo[ q ][ x & 15 ] & f_hi[ q ][ x >> 4 ] & ( 1u << b ) )
      here += f[ x ] / total;
    pass[ b ] *= here;
    any += pass[ b ];
    }
   }
  f_q = std::max< std::size_t >( q , 1 );
  }
 // the first 8 bytes of each pattern, and which of them it has
 f_head.resize( P.size() );
 f_mask.resize( P.size() );
 for( std::size_t j = 0 ; j < P.size() ; ++j ) {
  const std::size_t l = std::min< std::size_t >( P[ j ].size() , 8 );
  std::uint64_t h = 0;
  std::memcpy( &h , P[ j ].data() , l );
  f_head[ j ] = h;
  f_mask[ j ] = l == 8 ? ~std::uint64_t( 0 ) :
                         ( std::uint64_t( 1 ) << ( 8 * l ) ) - 1;
  }
 }

template< class F >
std::size_t MultiSearcher::check( const unsigned char * T , std::size_t n ,
                           std::size_t s , std::uint64_t mask ,
                           const unsigned char * C , F && report ) const
{
 std::size_t checks = 0;
 for( ; mask ; mask &= mask - 1 ) {
  const unsigned p = unsigned( std::countr_zero( mask ) );
  const std::size_t i = s + p;
  for( unsigned c = C[ p ] ; c ; c &= c - 1 )
   for( const std::uint32_t j : f_bucket[ std::countr_zero( c ) ] ) {
    ++checks;
    const std::string & P = f_P[ j ];
    const std::size_t l = P.size();
    if( i + l > n ) continue;
    if( i + 8 <= n ) {                // the first 8 bytes in one word
     std::uint64_t w;
     std::memcpy( &w , T + i , 8 );
     if( ( ( w ^ f_head[ j ] ) & f_mask[ j ] ) != 0 ) continue;
     if( ( l > 8 ) && ( std::memcmp( T + i + 8 , P.data() + 8 , l - 8 ) ) )
      continue;
     }
    else if( std::memcmp( T + i , P.data() , l ) ) continue;
    report( i , std::size_t( j ) );
    }
  }
 return( checks );
 }

template< class F >
void MultiSearcher::automaton( std::string_view T , std::size_t from ,
                               F && report ) const
{
 const std::size_t b = std::min( from > f_longest ? from - f_longest : 0 ,
                                 T.size() );
 f_ac.forEach( T.substr( b ) , [ & ]( std::size_t i , std::size_t j ) {
  if( i + b >= from ) report( i + b , j ); } );
 }

template< class F >
std::size_t MultiSearcher::verify( const unsigned char * T , std::size_t n ,
                                   std::size_t s , F && report ) const
{
 std::uint64_t k = 0;
 std::memcpy( &k , T + s , f_L );
 const std::size_t h = std::size_t( ( k * 0x9E3779B97F4A7C15ull ) >>
                                    ( 64 - f_bits ) );
 std::size_t checks = 0;
 for( std::uint32_t j = f_first[ h ] ; j != ~std::uint32_t( 0 ) ;
      j = f_next[ j ] ) {
  ++checks;
  if( f_key[ j ] != k ) continue;
  const std::string & p = f_P[ j ];
  if( ( s + p.size() <= n ) &&
      ( std::memcmp( T + s + f_L , p.data() + f_L , p.size() - f_L ) == 0 ) )
   report( s , std::size_t( j ) );
  }
 return( checks );
 }

template< class F >
std::size_t MultiSearcher::shiftOr( std::string_view Tv ,
                                    F && report ) const
{
 const std::size_t n = Tv.size() , L = f_L;
 const auto * T = reinterpret_cast< const unsigned char * >( Tv.data() );
 if( n < f_least ) return( npos );
 const std::uint64_t * sh = f_sh.data();
 // the state of the ends j .. j + 7 (byte x: the end j + x, a bit off for
 // a bucket that may end there) and of the next 8; the ends before L - 1
 // are no end
 std::uint64_t cur = L > 1 ? ( std::uint64_t( 1 ) << ( 8 * ( L - 1 ) ) ) - 1
                           : 0 ,
               nxt = 0;
 std::size_t work = 0 , j = 0;
 unsigned char pad[ 24 ] = {};
 // the ends j + x whose byte of cur has a bit off, checked from their start
 auto ends = [ & ]( std::size_t from ) {
  for( std::uint64_t z = cur ; z != ~std::uint64_t( 0 ) ; ) {
   const unsigned x = unsigned( std::countr_one( z ) ) / 8;
   z |= std::uint64_t( 0xff ) << ( 8 * x );
   const std::size_t e = from + x;
   if( e < n ) work += verify( T , n , e + 1 - L , report );
   }
  };
 // the super character of the place j + d is the 11 bits of the text
 // from the bit 8 d of the word read at j (in the order of the bytes):
 // two words at j and j + 4 give those of 8 places
 for( ; j < n ; j += 8 ) {
  std::uint64_t w[ 2 ];
  if( j + 12 <= n ) {
   std::memcpy( &w[ 0 ] , T + j , 8 );
   std::memcpy( &w[ 1 ] , T + j + 4 , 8 );
   }
  else {                              // the last bytes, with 0 after them
   std::memcpy( pad , T + j , n - j );
   std::memcpy( &w[ 0 ] , pad , 8 );
   std::memcpy( &w[ 1 ] , pad + 4 , 8 );
   }
  if constexpr( std::endian::native == std::endian::big ) {
   w[ 0 ] = detail::byteSwap( w[ 0 ] );
   w[ 1 ] = detail::byteSwap( w[ 1 ] );
   }
#if defined( __SSE2__ )
  // the 16 bytes of cur and nxt in one register, each mask moved by d
  // bytes with no carry to handle
  __m128i acc = _mm_set_epi64x( std::int64_t( nxt ) , std::int64_t( cur ) );
  [ & ]< int... d >( std::integer_sequence< int , d... > ) {
   ( ( acc = _mm_or_si128( acc , _mm_slli_si128( _mm_loadl_epi64(
      reinterpret_cast< const __m128i * >( sh + ( ( w[ d / 4 ] >>
                                       ( 8 * ( d % 4 ) ) ) & 0x7ff ) ) ) ,
      d ) ) ) , ... );
   }( std::make_integer_sequence< int , 8 >() );
  cur = std::uint64_t( _mm_cvtsi128_si64( acc ) );
  nxt = std::uint64_t( _mm_cvtsi128_si64( _mm_unpackhi_epi64( acc , acc ) ) );
#else
  for( unsigned d = 0 ; d < 8 ; ++d ) {
   const std::uint64_t v = sh[ ( w[ d / 4 ] >> ( 8 * ( d % 4 ) ) ) & 0x7ff ];
   cur |= v << ( 8 * d );
   nxt |= d ? v >> ( 64 - 8 * d ) : 0;
   }
#endif
  if( cur != ~std::uint64_t( 0 ) ) [[unlikely]] {
   ends( j );
   if( work > 2 * j + 4096 ) {        // too many checks: the automaton
    return( j + 8 >= L - 1 ? j + 9 - L : 0 );
    }
   }
  cur = nxt;
  nxt = 0;
  }
 return( npos );
 }

template< class F >
std::size_t MultiSearcher::shiftOr2( std::string_view Tv ,
                                     F && report ) const
{
 const std::size_t n = Tv.size() , W = f_W , back = 2 * ( W - 1 );
 const auto * T = reinterpret_cast< const unsigned char * >( Tv.data() );
 if( n < f_least ) return( npos );
 const std::uint64_t * sh = f_sh2.data();
 // the state of the even ends j , j + 2 , ... , j + 14 (byte x: the end
 // j + 2 x) and of the next 8; a bit off of the bits 0 to 3 for a pattern
 // that may start at the end less 2 ( W - 1 ), of the bits 4 to 7 at one
 // place before
 std::uint64_t cur = 0 , nxt = 0;
 std::size_t work = 0 , j = 0;
 unsigned char pad[ 24 ] = {};
 auto ends = [ & ]( std::size_t from ) {
  for( std::uint64_t z = cur ; z != ~std::uint64_t( 0 ) ; ) {
   const unsigned x = unsigned( std::countr_one( z ) ) / 8;
   const unsigned bits = unsigned( ~( z >> ( 8 * x ) ) ) & 0xff;
   z |= std::uint64_t( 0xff ) << ( 8 * x );
   const std::size_t e = from + 2 * x;
   for( unsigned phase = 0 ; phase < 2 ; ++phase )
    if( ( bits >> ( 4 * phase ) ) & 15 ) {
     if( e < back + phase ) continue;
     const std::size_t s = e - back - phase;
     if( s + f_least <= n ) work += verify( T , n , s , report );
     }
   }
  };
 for( ; j < n ; j += 16 ) {
  std::uint64_t w[ 2 ];
  if( j + 16 <= n ) {
   std::memcpy( &w[ 0 ] , T + j , 8 );
   std::memcpy( &w[ 1 ] , T + j + 8 , 8 );
   }
  else {                              // the last bytes, with 0 after them
   std::memcpy( pad , T + j , n - j );
   std::memcpy( &w[ 0 ] , pad , 8 );
   std::memcpy( &w[ 1 ] , pad + 8 , 8 );
   }
  if constexpr( std::endian::native == std::endian::big ) {
   w[ 0 ] = detail::byteSwap( w[ 0 ] );
   w[ 1 ] = detail::byteSwap( w[ 1 ] );
   }
#if defined( __SSE2__ )
  __m128i acc = _mm_set_epi64x( std::int64_t( nxt ) , std::int64_t( cur ) );
  [ & ]< int... d >( std::integer_sequence< int , d... > ) {
   ( ( acc = _mm_or_si128( acc , _mm_slli_si128( _mm_loadl_epi64(
      reinterpret_cast< const __m128i * >( sh + ( ( w[ d / 4 ] >>
                                       ( 16 * ( d % 4 ) ) ) & 0x7ff ) ) ) ,
      d ) ) ) , ... );
   }( std::make_integer_sequence< int , 8 >() );
  cur = std::uint64_t( _mm_cvtsi128_si64( acc ) );
  nxt = std::uint64_t( _mm_cvtsi128_si64( _mm_unpackhi_epi64( acc , acc ) ) );
#else
  for( unsigned d = 0 ; d < 8 ; ++d ) {
   const std::uint64_t v = sh[ ( w[ d / 4 ] >> ( 16 * ( d % 4 ) ) ) & 0x7ff ];
   cur |= v << ( 8 * d );
   nxt |= d ? v >> ( 64 - 8 * d ) : 0;
   }
#endif
  if( cur != ~std::uint64_t( 0 ) ) [[unlikely]] {
   ends( j );
   if( work > 2 * j + 4096 )          // too many checks: the automaton
    return( j + 15 >= back ? j + 15 - back : 0 );
   }
  cur = nxt;
  nxt = 0;
  }
 return( npos );
 }

template< class F >
std::size_t MultiSearcher::scan( std::string_view Tv , F && report ) const
{
 const std::size_t n = Tv.size() , q = f_q;
 const auto * T = reinterpret_cast< const unsigned char * >( Tv.data() );
 if( n < f_least ) return( npos );
 const std::size_t last = n - f_least;  // the places 0 .. last
 std::size_t s = 0 , work = 0;
 alignas( 64 ) unsigned char C[ 64 ];
 // the checks of patterns counted: past 2 a place, the filter lets too
 // much through (a small alphabet, or patterns everywhere), and the
 // automaton goes on from s, its occurrences that start before s left out
#if defined( __AVX512BW__ )
 // 64 places a block while the loads of q - 1 places ahead are in T, two
 // blocks a step, q known at compile time so that the tables stay in
 // registers; npos when the blocks are done, else the place for the
 // automaton
 auto blocks = [ & ]< std::size_t Q >() {
  __m512i lo[ Q ] , hi[ Q ];
  for( std::size_t r = 0 ; r < Q ; ++r ) {
   lo[ r ] = _mm512_broadcast_i32x4( _mm_loadu_si128(
              reinterpret_cast< const __m128i * >( f_lo[ r ].data() ) ) );
   hi[ r ] = _mm512_broadcast_i32x4( _mm_loadu_si128(
              reinterpret_cast< const __m128i * >( f_hi[ r ].data() ) ) );
   }
  const __m512i low = _mm512_set1_epi8( 0x0f );
  auto probe = [ & ]( std::size_t t ) {
   __m512i acc = _mm512_set1_epi8( char( 0xff ) );
   for( std::size_t r = 0 ; r < Q ; ++r ) {
    const __m512i v = _mm512_loadu_si512( T + t + r );
    acc = _mm512_and_si512( acc , _mm512_and_si512(
           _mm512_shuffle_epi8( lo[ r ] , _mm512_and_si512( v , low ) ) ,
           _mm512_shuffle_epi8( hi[ r ] , _mm512_and_si512(
            _mm512_srli_epi16( v , 4 ) , low ) ) ) );
    }
   return( acc );
   };
  // the candidates of the block at t: false if the automaton must go on
  auto take = [ & ]( std::size_t t , __m512i acc ) {
   std::uint64_t m = _mm512_test_epi8_mask( acc , acc );
   if( m == 0 ) return( true );
   if( last - t < 63 ) m &= ( std::uint64_t( 1 ) << ( last - t + 1 ) ) - 1;
   _mm512_store_si512( C , acc );
   work += check( T , n , t , m , C , report );
   return( work <= 2 * t + 4096 );
   };
  for( ; ( s + 64 <= last ) && ( s + 128 + Q - 1 <= n ) ; s += 128 ) {
   const __m512i a = probe( s ) , b = probe( s + 64 );
   if( _mm512_test_epi8_mask( _mm512_or_si512( a , b ) ,
                              _mm512_or_si512( a , b ) ) == 0 ) [[likely]]
    continue;
   if( ! take( s , a ) ) return( s + 64 );
   if( ! take( s + 64 , b ) ) return( s + 128 );
   }
  for( ; ( s <= last ) && ( s + 64 + Q - 1 <= n ) ; s += 64 )
   if( ! take( s , probe( s ) ) ) return( s + 64 );
  return( npos );
  };
 {
  std::size_t from = npos;
  switch( q ) {
   case 1: from = blocks.template operator()< 1 >(); break;
   case 2: from = blocks.template operator()< 2 >(); break;
   case 3: from = blocks.template operator()< 3 >(); break;
   case 4: from = blocks.template operator()< 4 >(); break;
   case 5: from = blocks.template operator()< 5 >(); break;
   case 6: from = blocks.template operator()< 6 >(); break;
   case 7: from = blocks.template operator()< 7 >(); break;
   default: from = blocks.template operator()< 8 >();
   }
  if( from != npos ) return( from );
  }
#endif
#if defined( __AVX2__ )
 // 32 places a block while the loads of q - 1 places ahead are in T
 __m256i lo[ 8 ] , hi[ 8 ];
 for( std::size_t r = 0 ; r < q ; ++r ) {
  lo[ r ] = _mm256_broadcastsi128_si256( _mm_loadu_si128(
             reinterpret_cast< const __m128i * >( f_lo[ r ].data() ) ) );
  hi[ r ] = _mm256_broadcastsi128_si256( _mm_loadu_si128(
             reinterpret_cast< const __m128i * >( f_hi[ r ].data() ) ) );
  }
 const __m256i low = _mm256_set1_epi8( 0x0f ) , zero = _mm256_setzero_si256();
 for( ; ( s <= last ) && ( s + 32 + q - 1 <= n ) ; s += 32 ) {
  __m256i acc = _mm256_set1_epi8( char( 0xff ) );
  for( std::size_t r = 0 ; r < q ; ++r ) {
   const __m256i v = _mm256_loadu_si256(
                      reinterpret_cast< const __m256i * >( T + s + r ) );
   const __m256i a = _mm256_shuffle_epi8( lo[ r ] ,
                                           _mm256_and_si256( v , low ) ) ,
                 b = _mm256_shuffle_epi8( hi[ r ] , _mm256_and_si256(
                      _mm256_srli_epi16( v , 4 ) , low ) );
   acc = _mm256_and_si256( acc , _mm256_and_si256( a , b ) );
   }
  std::uint64_t m = ~std::uint32_t( _mm256_movemask_epi8(
                                     _mm256_cmpeq_epi8( acc , zero ) ) );
  if( m == 0 ) [[likely]] continue;
  if( last - s < 31 ) m &= ( std::uint64_t( 1 ) << ( last - s + 1 ) ) - 1;
  _mm256_store_si256( reinterpret_cast< __m256i * >( C ) , acc );
  work += check( T , n , s , m , C , report );
  if( work > 2 * s + 4096 ) {
   return( s + 32 );
   }
  }
#endif
 // the rest place by place, by the same tables
 for( ; s <= last ; s += 32 ) {
  std::uint64_t m = 0;
  const std::size_t e = std::min< std::size_t >( last - s + 1 , 32 );
  for( std::size_t p = 0 ; p < e ; ++p ) {
   unsigned c = 0xff;
   for( std::size_t r = 0 ; r < q ; ++r ) {
    const unsigned char x = T[ s + p + r ];
    c &= f_lo[ r ][ x & 15 ] & f_hi[ r ][ x >> 4 ];
    }
   C[ p ] = static_cast< unsigned char >( c );
   if( c ) m |= std::uint64_t( 1 ) << p;
   }
  work += check( T , n , s , m , C , report );
  if( ( work > 2 * s + 4096 ) && ( s + 32 <= last ) ) {
   return( s + 32 );
   }
  }
 return( npos );
 }

inline std::vector< std::pair< std::size_t , std::size_t > >
 MultiSearcher::findAll( std::string_view T ) const
{
 std::vector< std::pair< std::size_t , std::size_t > > R;
 auto push = [ & ]( std::size_t i , std::size_t j ) {
  R.emplace_back( i , j ); };
 if( f_P.size() == 1 )                // one pattern: the single search
  for( const std::size_t i : StringSearcher( f_P[ 0 ] ).findAll( T ) )
   R.emplace_back( i , 0 );
 else if( ! f_P.empty() ) {
  const std::size_t from = f_q ? scan( T , push ) :
                           f_W ? shiftOr2( T , push ) : shiftOr( T , push );
  if( from != npos ) automaton( T , from , push );
  }
 // within a place the numbers come by bucket, and the automaton gives
 // them by end
 std::sort( R.begin() , R.end() );
 ADS_AUDIT( detail::multiNaive( T , f_P ) == R );
 return( R );
 }

inline std::size_t MultiSearcher::count( std::string_view T ) const
{
 if( f_P.size() == 1 ) return( StringSearcher( f_P[ 0 ] ).count( T ) );
 if( f_P.empty() ) return( 0 );
 std::size_t c = 0;
 auto one = [ & ]( std::size_t , std::size_t ) { ++c; };
 const std::size_t from = f_q ? scan( T , one ) :
                          f_W ? shiftOr2( T , one ) : shiftOr( T , one );
 if( from == npos ) return( c );
 // the automaton from from on: its count from b = from - longest, less
 // the occurrences that start before from, all within the window
 // [ b , from + longest )
 const std::size_t n = T.size() ,
                   b = std::min( from > f_longest ? from - f_longest : 0 , n );
 c += f_ac.count( T.substr( b ) );
 f_ac.forEach( T.substr( b , from - std::min( from , b ) + f_longest ) ,
               [ & ]( std::size_t i , std::size_t ) {
                c -= std::size_t( i + b < from ); } );
 return( c );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File MultiSearch.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
