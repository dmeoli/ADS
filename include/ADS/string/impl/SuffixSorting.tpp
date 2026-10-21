/*--------------------------------------------------------------------------*/
/*------------------------- File SuffixSorting.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuffixSorting [see SuffixSorting.h], with their
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

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

#if defined( __AVX2__ )
 #include <immintrin.h>
#endif

#if defined( __linux__ )
 #include <sys/mman.h>
#endif

namespace ADS {

namespace detail {

/// a pointer whose array no other pointer of the function reaches
#if defined( __GNUC__ ) || defined( __clang__ ) || defined( _MSC_VER )
 #define ADS_SAIS_RESTRICT __restrict
#else
 #define ADS_SAIS_RESTRICT
#endif

/// how far ahead the scans of the induction prefetch the text; on the
/// large alphabets the buckets are prefetched half as far and the places
/// they point to a quarter
inline constexpr std::ptrdiff_t saisAhead = 64;

/// the alphabets of more than saisSmall symbols count their kinds in one
/// histogram, and have the seeds of their LMS places prefetched
inline constexpr std::ptrdiff_t saisSmall = 1024;

/// the alphabets of more than saisCached symbols, whose buckets do not fit
/// in the cache, have them prefetched in the scans of the induction
inline constexpr std::ptrdiff_t saisCached = 65536;

/// x if not negative, 0 otherwise, with no branch
template< class S >
inline S saisClamp( S x )
{
 return( x & ~( x >> ( 8 * sizeof( S ) - 1 ) ) );
 }

/// the 8 bytes of x in the reverse order
inline std::uint64_t saisSwap( std::uint64_t x )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 return( __builtin_bswap64( x ) );
#else
 x = ( ( x >> 8 ) & 0x00FF00FF00FF00FFull ) |
     ( ( x & 0x00FF00FF00FF00FFull ) << 8 );
 x = ( ( x >> 16 ) & 0x0000FFFF0000FFFFull ) |
     ( ( x & 0x0000FFFF0000FFFFull ) << 16 );
 return( ( x >> 32 ) | ( x << 32 ) );
#endif
 }

/// the 64 bits of x in the reverse order
inline std::uint64_t saisReverse( std::uint64_t x )
{
 x = ( ( x >> 1 ) & 0x5555555555555555ull ) |
     ( ( x & 0x5555555555555555ull ) << 1 );
 x = ( ( x >> 2 ) & 0x3333333333333333ull ) |
     ( ( x & 0x3333333333333333ull ) << 2 );
 x = ( ( x >> 4 ) & 0x0F0F0F0F0F0F0F0Full ) |
     ( ( x & 0x0F0F0F0F0F0F0F0Full ) << 4 );
 x = ( ( x >> 8 ) & 0x00FF00FF00FF00FFull ) |
     ( ( x & 0x00FF00FF00FF00FFull ) << 8 );
 x = ( ( x >> 16 ) & 0x0000FFFF0000FFFFull ) |
     ( ( x & 0x0000FFFF0000FFFFull ) << 16 );
 return( ( x >> 32 ) | ( x << 32 ) );
 }

/// bit j of *G: T[ j ] < T[ j + 1 ], of *P: T[ j ] == T[ j + 1 ], for
/// j in [ 0 , 64 )
template< typename C >
inline void saisCompare( const C * T , std::uint64_t * G , std::uint64_t * P )
{
#if defined( __AVX2__ )
 if constexpr( ( sizeof( C ) == 1 ) && std::is_unsigned_v< C > ) {
  const __m256i flip = _mm256_set1_epi8( char( 0x80 ) );
  std::uint64_t g[ 2 ] , p[ 2 ];
  for( int h = 0 ; h < 2 ; ++h ) {
   const __m256i a = _mm256_loadu_si256(
                      reinterpret_cast< const __m256i * >( T + 32 * h ) ) ,
                 b = _mm256_loadu_si256(
                      reinterpret_cast< const __m256i * >( T + 32 * h + 1 ) );
   const __m256i sa = _mm256_xor_si256( a , flip ) ,
                 sb = _mm256_xor_si256( b , flip );
   g[ h ] = std::uint32_t( _mm256_movemask_epi8(
                            _mm256_cmpgt_epi8( sb , sa ) ) );
   p[ h ] = std::uint32_t( _mm256_movemask_epi8(
                            _mm256_cmpeq_epi8( a , b ) ) );
   }
  *G = g[ 0 ] | ( g[ 1 ] << 32 );
  *P = p[ 0 ] | ( p[ 1 ] << 32 );
  return;
  }
 else if constexpr( ( sizeof( C ) == 4 ) && std::is_signed_v< C > ) {
  std::uint64_t g = 0 , p = 0;        // the names, never negative
  for( int h = 0 ; h < 8 ; ++h ) {
   const __m256i a = _mm256_loadu_si256(
                      reinterpret_cast< const __m256i * >( T + 8 * h ) ) ,
                 b = _mm256_loadu_si256(
                      reinterpret_cast< const __m256i * >( T + 8 * h + 1 ) );
   g |= std::uint64_t( unsigned( _mm256_movemask_ps( _mm256_castsi256_ps(
         _mm256_cmpgt_epi32( b , a ) ) ) ) ) << ( 8 * h );
   p |= std::uint64_t( unsigned( _mm256_movemask_ps( _mm256_castsi256_ps(
         _mm256_cmpeq_epi32( a , b ) ) ) ) ) << ( 8 * h );
   }
  *G = g;
  *P = p;
  return;
  }
#endif
 std::uint64_t g = 0 , p = 0;
 for( unsigned j = 0 ; j < 64 ; ++j ) {
  g |= std::uint64_t( T[ j ] < T[ j + 1 ] ) << j;
  p |= std::uint64_t( T[ j ] == T[ j + 1 ] ) << j;
  }
 *G = g;
 *P = p;
 }

/// idx[ j ] = 4 x[ j ] + 2 t_j + b_j for j in [ 0 , 64 ), with t_j and b_j
/// the bits j of t and b: the counters of the kinds of 64 places
inline void saisKinds( const unsigned char * x , std::uint64_t t ,
                       std::uint64_t b , std::uint16_t * idx )
{
#if defined( __AVX2__ )
 // each bit of a 32-bit mask to a byte of all ones or zeros
 const __m256i spread = _mm256_setr_epi8( 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 ,
                         1 , 1 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 2 , 2 , 2 , 2 ,
                         2 , 2 , 3 , 3 , 3 , 3 , 3 , 3 , 3 , 3 ) ,
               bit = _mm256_set1_epi64x(
                      std::int64_t( 0x8040201008040201ull ) );
 auto bytes = [ & ]( std::uint32_t v ) {
  const __m256i a = _mm256_and_si256( _mm256_shuffle_epi8(
                     _mm256_set1_epi32( int( v ) ) , spread ) , bit );
  return( _mm256_cmpeq_epi8( a , bit ) );
  };
 for( int h = 0 ; h < 2 ; ++h ) {
  const __m256i k = _mm256_or_si256(
                     _mm256_and_si256( bytes( std::uint32_t( t >> 32 * h ) ) ,
                                       _mm256_set1_epi8( 2 ) ) ,
                     _mm256_and_si256( bytes( std::uint32_t( b >> 32 * h ) ) ,
                                       _mm256_set1_epi8( 1 ) ) );
  const __m256i v = _mm256_loadu_si256(
                     reinterpret_cast< const __m256i * >( x + 32 * h ) );
  for( int g = 0 ; g < 2 ; ++g ) {
   const __m128i vg = g ? _mm256_extracti128_si256( v , 1 ) :
                          _mm256_castsi256_si128( v ) ,
                 kg = g ? _mm256_extracti128_si256( k , 1 ) :
                          _mm256_castsi256_si128( k );
   const __m256i r = _mm256_or_si256(
                      _mm256_slli_epi16( _mm256_cvtepu8_epi16( vg ) , 2 ) ,
                      _mm256_cvtepu8_epi16( kg ) );
   _mm256_storeu_si256( reinterpret_cast< __m256i * >( idx + 32 * h +
                                                       16 * g ) , r );
   }
  }
#else
 for( unsigned j = 0 ; j < 64 ; ++j )
  idx[ j ] = std::uint16_t( 4 * x[ j ] + 2 * ( ( t >> j ) & 1 ) +
                            ( ( b >> j ) & 1 ) );
#endif
 }

/// asks for transparent huge pages under the whole pages of 2 MB of the b
/// bytes at p, not touched yet: their faults then come 512 times fewer
inline void saisHuge( void * p , std::size_t b )
{
#if defined( __linux__ ) && defined( MADV_HUGEPAGE )
 constexpr std::uintptr_t page = std::uintptr_t( 1 ) << 21;
 const std::uintptr_t from = ( std::uintptr_t( p ) + page - 1 ) &
                             ~( page - 1 ) ,
                      to = ( std::uintptr_t( p ) + b ) & ~( page - 1 );
 if( to > from )
  madvise( reinterpret_cast< void * >( from ) , to - from , MADV_HUGEPAGE );
#else
 (void) p;
 (void) b;
#endif
 }

/// a vector of n values, its pages huge where the system allows it
template< class I >
std::vector< I > saisVector( std::size_t n )
{
 std::vector< I > V;
 V.reserve( n );
 saisHuge( V.data() , n * sizeof( I ) );
 V.assign( n , I( 0 ) );              // a fill of zeros: one memset
 return( V );
 }

/// prefetches the line of p for a read
inline void saisPrefetch( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p , 0 , 3 );
#else
 (void) p;
#endif
 }

/// prefetches the text before the entry a (T[ a - 1 ] and T[ a - 2 ]),
/// that of the entry 2 if a <= 0 (a select, no branch)
template< class C , class S >
inline void saisPrefetch2( const C * T , S a )
{
 const C * t = T + ( a > 0 ? a : S( 2 ) );
 saisPrefetch( t - 1 );
 saisPrefetch( t - 2 );
 }

/// prefetches the line of p for a write
inline void saisPrefetchW( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p , 1 , 3 );
#else
 (void) p;
#endif
 }

/// the first (heads) or the next past the last (tails) place of each of
/// the K buckets, from their sizes
template< class S >
void saisBuckets( const S * size , S * B , S K , bool tails )
{
 S sum = 0;
 for( S c = 0 ; c < K ; ++c ) {
  sum += size[ c ];
  B[ c ] = tails ? sum : sum - size[ c ];
  }
 }

/// the top bit of an S, set if b
template< class S >
inline S saisTop( bool b )
{
 using U = std::make_unsigned_t< S >;
 return( S( U( b ) << ( 8 * sizeof( S ) - 1 ) ) );
 }

/// the move of the entry h by the scan from left to right
template< class C , class S >
inline void saisMoveL( const C * ADS_SAIS_RESTRICT T ,
                       S * ADS_SAIS_RESTRICT SA , S * ADS_SAIS_RESTRICT B ,
                       S h )
{
 constexpr S top = std::numeric_limits< S >::min();
 const S q = SA[ h ];
 SA[ h ] = q ^ top;
 if( q > 0 ) {
  const S p = q - 1;
  const S c0 = S( T[ p ] );
  SA[ B[ c0 ]++ ] = p | saisTop< S >( S( T[ p - ( p > 0 ) ] ) < c0 );
  }
 }

/// the move of the entry h by the scan from right to left
template< class C , class S >
inline void saisMoveS( const C * ADS_SAIS_RESTRICT T ,
                       S * ADS_SAIS_RESTRICT SA , S * ADS_SAIS_RESTRICT B ,
                       S h )
{
 constexpr S low = std::numeric_limits< S >::max();
 const S q = SA[ h ];
 if( q > 0 ) {
  const S p = q - 1;
  const S c0 = S( T[ p ] );
  SA[ --B[ c0 ] ] = p | saisTop< S >( S( T[ p - ( p > 0 ) ] ) > c0 );
  }
 else
  SA[ h ] = q & low;
 }

/// the scan from left to right: the L-suffixes, each stored with its top
/// bit set if the suffix before it is of type S, and the top bit of every
/// entry scanned flipped. Two entries a step, the text before the entries
/// D ahead prefetched, and on the large alphabets also their buckets (D /
/// 2 ahead) and the places these point to (D / 4 ahead); the arrays are
/// declared not to overlap (restrict), so that the loads of the next
/// entries need not wait for the stores of the moves
template< bool small , class C , class S >
void saisInduceL( const C * ADS_SAIS_RESTRICT T , S * ADS_SAIS_RESTRICT SA ,
                  S n , const S * size , S * ADS_SAIS_RESTRICT B , S K )
{
 constexpr S D = S( saisAhead ) , low = std::numeric_limits< S >::max();
 saisBuckets( size , B , K , false );
 {
  const S j = n - 1;
  SA[ B[ T[ j ] ]++ ] = j | saisTop< S >( ( j > 0 ) &&
                                           ( T[ j - 1 ] < T[ j ] ) );
  }
 S i = 0;
 for( ; i + 2 * D + 1 < n ; i += 2 ) { // two at a time, no checks
  saisPrefetchW( SA + i + 2 * D );
  const S a = SA[ i + D ] , b = SA[ i + D + 1 ];
  saisPrefetch2( T , a );
  saisPrefetch2( T , b );
  if constexpr( ! small ) {
   const S e = SA[ i + D / 2 ] , f = SA[ i + D / 4 ];
   saisPrefetch( B + T[ saisClamp( ( e & low ) - 1 ) ] );
   saisPrefetchW( SA + B[ T[ saisClamp( ( f & low ) - 1 ) ] ] );
   }
  saisMoveL( T , SA , B , i );
  saisMoveL( T , SA , B , i + 1 );
  }
 for( ; i < n ; ++i ) saisMoveL( T , SA , B , i );
 }

/// the scan from right to left: the S-suffixes, each stored with its top
/// bit set if the suffix before it is of type L, and the entries with the
/// top bit set (or 0) restored. Two entries a step, prefetched as in
/// saisInduceL()
template< bool small , class C , class S >
void saisInduceS( const C * ADS_SAIS_RESTRICT T , S * ADS_SAIS_RESTRICT SA ,
                  S n , const S * size , S * ADS_SAIS_RESTRICT B , S K )
{
 constexpr S D = S( saisAhead ) , low = std::numeric_limits< S >::max();
 saisBuckets( size , B , K , true );
 S i = n - 1;
 for( ; i >= 2 * D + 1 ; i -= 2 ) {    // two at a time, no checks
  saisPrefetchW( SA + i - 2 * D );
  const S a = SA[ i - D ] , b = SA[ i - D - 1 ];
  saisPrefetch2( T , a );
  saisPrefetch2( T , b );
  if constexpr( ! small ) {
   const S e = SA[ i - D / 2 ] , f = SA[ i - D / 4 ];
   saisPrefetch( B + T[ saisClamp( ( e & low ) - 1 ) ] );
   saisPrefetchW( SA + ( B[ T[ saisClamp( ( f & low ) - 1 ) ] ] - 1 ) );
   }
  saisMoveS( T , SA , B , i );
  saisMoveS( T , SA , B , i - 1 );
  }
 for( ; i >= 0 ; --i ) saisMoveS( T , SA , B , i );
 }

/// the two scans of the induction
template< class C , class S >
void saisInduce( const C * T , S * SA , S n , const S * size , S * B ,
                 S K )
{
 if( K <= S( saisCached ) ) {
  saisInduceL< true >( T , SA , n , size , B , K );
  saisInduceS< true >( T , SA , n , size , B , K );
  }
 else {
  saisInduceL< false >( T , SA , n , size , B , K );
  saisInduceS< false >( T , SA , n , size , B , K );
  }
 }

/// the suffix array of R, m names in [ 0 , K ) of which few repeat, into
/// SA[ 0 .. m - 1 ], by the doubling of Larsson and Sadakane on the groups
/// of equal names alone: the suffixes are first sorted by their name (a
/// counting sort by the counts of the names), so that a name that occurs
/// once is already in place, and a group of equal ranks is sorted by the
/// rank of the suffix h places ahead, h = 1, 2, 4, ..., its rank the last
/// place of its group; false, with SA undefined, if more than limit
/// suffixes are tied, unless the groups of a repeated name are small (16
/// suffixes on average, so that a round costs little and few rounds
/// separate them), and false if more than limit are still tied after a
/// round, or after 16 rounds, which leaves the work to the recursion
template< class S >
bool saisDoubling( const S * R , S * SA , S m ,
                   const std::vector< S > & count , S limit )
{
 const std::size_t K = count.size();
 S tied = 0 , groups = 0;             // in the groups of a repeated name
 for( std::size_t c = 0 ; c < K ; ++c )
  if( count[ c ] > 1 ) {
   tied += count[ c ];
   ++groups;
   }
 if( tied > limit ) {
  if( tied > 16 * groups ) return( false );
  limit = m;
  }
 std::vector< S > V( std::size_t( m ) , 0 ) , start( K , 0 );
 for( std::size_t c = 0 , sum = 0 ; c < K ; ++c ) {
  start[ c ] = S( sum );
  sum += std::size_t( count[ c ] );
  }
 // the counting sort, the group of each name prefetched, then its place
 constexpr S D = S( saisAhead );
 for( S k = 0 ; k < m ; ++k ) {
  if( k + 2 * D < m ) saisPrefetch( start.data() + R[ k + 2 * D ] );
  if( k + D < m ) saisPrefetchW( SA + start[ std::size_t( R[ k + D ] ) ] );
  SA[ start[ std::size_t( R[ k ] ) ]++ ] = k;
  }
 // the rank of a suffix, the last place of its group: start[ c ] is now
 // one past it
 for( S k = 0 ; k < m ; ++k ) {
  if( k + D < m ) saisPrefetch( start.data() + R[ k + D ] );
  V[ std::size_t( k ) ] = start[ std::size_t( R[ k ] ) ] - 1;
  }
 // the groups of more than one suffix, as ( first place , size )
 std::vector< std::pair< S , S > > G , H;
 for( std::size_t c = 0 ; c < K ; ++c )
  if( count[ c ] > 1 ) G.emplace_back( start[ c ] - count[ c ] , count[ c ] );
 std::vector< std::pair< S , S > > key;    // ( rank ahead , suffix )
 for( S h = 1 ; ! G.empty() ; h *= 2 ) {
  tied = 0;
  for( const auto & [ f , g ] : G ) tied += g;
  if( ( tied > limit ) || ( h > ( S( 1 ) << 16 ) ) ) return( false );
  // the keys of all the groups first, then the new ranks
  key.resize( std::size_t( tied ) );
  auto * kz = key.data();
  for( const auto & [ f , g ] : G )
   for( S i = f ; i < f + g ; ++i ) {
    if( i + D < m ) saisPrefetch( V.data() + std::min( SA[ i + D ] + h ,
                                                        m - 1 ) );
    const S k = SA[ i ];
    *kz++ = { k + h < m ? V[ std::size_t( k + h ) ] : S( -1 ) , k };
    }
  H.clear();
  std::size_t at = 0;
  for( const auto & [ f , g ] : G ) {
   auto * a = key.data() + at;
   if( g <= 16 )                      // most groups: few suffixes
    for( S i = 1 ; i < g ; ++i ) {
     const auto x = a[ i ];
     S j = i;
     for( ; ( j > 0 ) && ( x < a[ j - 1 ] ) ; --j ) a[ j ] = a[ j - 1 ];
     a[ j ] = x;
     }
   else
    std::sort( a , a + g );
   for( S i = 0 ; i < g ; ) {
    S j = i + 1;
    while( ( j < g ) && ( a[ j ].first == a[ i ].first ) ) ++j;
    if( j - i > 1 ) H.emplace_back( f + i , j - i );
    i = j;
    }
   at += std::size_t( g );
   }
  at = 0;
  for( const auto & [ f , g ] : G ) {
   const auto * a = key.data() + at;
   for( S i = 0 ; i < g ; ) {
    S j = i + 1;
    while( ( j < g ) && ( a[ j ].first == a[ i ].first ) ) ++j;
    for( S q = i ; q < j ; ++q ) {
     if( at + std::size_t( q + D ) < key.size() )
      saisPrefetchW( V.data() + a[ q + D ].second );
     SA[ f + q ] = a[ q ].second;
     V[ std::size_t( a[ q ].second ) ] = f + j - 1;
     }
    i = j;
    }
   at += std::size_t( g );
   }
  G.swap( H );
  }
 return( true );
 }

/// the groups of SA[ 0 .. m ) by the bit vector ends (bit i on if SA[ i ]
/// ends its group), each f( start , size ) in order
template< class S , class F >
inline void saisForGroups( const std::vector< std::uint64_t > & ends , S m ,
                           F && f )
{
 S start = 0;
 for( std::size_t w = 0 ; w < ends.size() ; ++w )
  for( std::uint64_t v = ends[ w ] ; v ; v &= v - 1 ) {
   const S e = S( w * 64 + std::size_t( std::countr_zero( v ) ) ) + 1;
   if( e > m ) return;
   f( start , e - start );
   start = e;
   }
 }

/// the groups of equal names among the LMS places sorted by their
/// substrings, SA[ 0 .. m ), each sorted by the names of the LMS places that
/// follow, read in the slots SA[ m + q / 2 ] (from 1, 0 elsewhere): two LMS
/// suffixes of equal names compare as the strings of the names from the
/// next LMS place on, which the bit vector lms gives; a group of 2 by
/// walking both, a larger one by a sort with that comparison. The suffixes
/// of the other names are already in place, and no recursion is needed.
/// False, with the groups in some order, if more than m / 4 suffixes are
/// tied or the names walked exceed budget
/// the first LMS place after p by the bit vector lms, n if none
template< class S >
inline S saisNextLms( const std::vector< std::uint64_t > & lms , S n , S p )
{
 std::size_t w = std::size_t( p + 1 ) >> 6;
 std::uint64_t v = lms[ w ] & ( ~std::uint64_t( 0 ) << ( ( p + 1 ) & 63 ) );
 while( ! v ) {
  if( ++w == lms.size() ) return( n );
  v = lms[ w ];
  }
 return( S( w * 64 + std::size_t( std::countr_zero( v ) ) ) );
 }

/// the groups of equal names among the LMS places sorted by their
/// substrings, SA[ 0 .. m ) by the bit vector ends, refined by the text that
/// follows the substring of the group (of the same length l in a group):
/// two suffixes p and q of equal LMS-substrings compare as T[ p + l .. ]
/// and T[ q + l .. ], read 8 symbols a round as one key in the order of
/// the bytes, a suffix that ends inside the key before one that goes on
/// with the same symbols. In rest the ends of the groups still tied after
/// at most rounds rounds or once budget keys are read (1 for each place
/// that is sorted), true if none is; nothing is done if the groups have
/// more than 16 places on average, or if half of those in the first round
/// on the first groups stay tied
template< class C , class S >
bool saisTextTies( const C * T , S n , S * SA , S m ,
                   const std::vector< std::uint64_t > & ends ,
                   const std::vector< std::uint64_t > & lms ,
                   std::vector< std::uint64_t > & rest , std::size_t budget ,
                   unsigned rounds )
{
 static_assert( ( sizeof( C ) == 1 ) && std::is_unsigned_v< C > );
 constexpr S D = S( saisAhead );
 struct Key {
  std::uint64_t key;
  S left , p;                         // the symbols in the key (<= 8)
  bool operator<( const Key & o ) const {
   return( ( key < o.key ) || ( ( key == o.key ) && ( left < o.left ) ) );
   }
  };
 struct Group { S f , g , at; };      // SA[ f .. f + g ), offset at
 std::vector< Group > G , H;
 std::size_t tied = 0;
 saisForGroups( ends , m , [ & ]( S f , S g ) {
  if( g > 1 ) {
   G.push_back( { f , g , 0 } );
   tied += std::size_t( g );
   }
  } );
 // the length of the LMS-substring of each group, the words of lms that
 // give the next LMS places prefetched
 for( std::size_t z = 0 ; z < G.size() ; ++z ) {
  if( z + D < G.size() )
   saisPrefetch( lms.data() + std::size_t( SA[ G[ z + D ].f ] + 1 ) / 64 );
  const S p = SA[ G[ z ].f ];
  G[ z ].at = saisNextLms( lms , n , p ) + 1 - p;
  }
 std::vector< Key > key;
 std::vector< std::uint64_t > u , w;  // the radix sort of large groups
 std::size_t spent = 0;
 // a group sorted by its keys: by insertion if small, else by a radix
 // sort on the bits that are not the same in all its keys (gathered by
 // pext, in their order), if at most 32 and no suffix ends in the keys
 auto sort = [ & ]( Key * a , S g ) {
  if( g <= 16 ) {
   for( S i = 1 ; i < g ; ++i ) {
    const Key x = a[ i ];
    S j = i;
    for( ; ( j > 0 ) && ( x < a[ j - 1 ] ) ; --j ) a[ j ] = a[ j - 1 ];
    a[ j ] = x;
    }
   return;
   }
#if defined( __BMI2__ )
  std::uint64_t one = ~std::uint64_t( 0 ) , any = 0;
  S full = 8;
  for( S i = 0 ; i < g ; ++i ) {
   one &= a[ i ].key;
   any |= a[ i ].key;
   full = std::min( full , a[ i ].left );
   }
  const std::uint64_t mask = one ^ any;
  const int b = std::popcount( mask );
  if( ( full == 8 ) && ( b <= 32 ) ) {
   if( b == 0 ) return;               // all equal
   u.resize( std::size_t( g ) );
   w.resize( std::size_t( g ) );
   for( S i = 0 ; i < g ; ++i )
    u[ i ] = ( _pext_u64( a[ i ].key , mask ) << 32 ) |
             std::uint32_t( a[ i ].p );
   for( int shift = 32 ; shift < 32 + b ; shift += 8 ) {
    S count[ 257 ] = {};
    for( S i = 0 ; i < g ; ++i ) ++count[ ( ( u[ i ] >> shift ) & 255 ) + 1 ];
    for( int c = 0 ; c < 256 ; ++c ) count[ c + 1 ] += count[ c ];
    for( S i = 0 ; i < g ; ++i )
     w[ std::size_t( count[ ( u[ i ] >> shift ) & 255 ]++ ) ] = u[ i ];
    u.swap( w );
    }
   for( S i = 0 ; i < g ; ++i )       // the key only for its equalities
    a[ i ] = { u[ i ] >> 32 , 8 , S( std::uint32_t( u[ i ] ) ) };
   return;
   }
#endif
  std::sort( a , a + g );
  };
 // one round on the groups [ from , to ) of G, the parts still tied to H;
 // the number of places in them
 auto round = [ & ]( std::size_t from , std::size_t to ) {
  std::size_t total = 0 , left = 0;
  for( std::size_t z = from ; z < to ; ++z ) total += std::size_t( G[ z ].g );
  key.resize( total );
  // the places of the keys first (key holds them), the groups ahead
  // prefetched; then the keys, the text D keys ahead prefetched
  std::size_t at = 0;
  for( std::size_t z = from ; z < to ; ++z ) {
   if( z + 8 < to ) saisPrefetch( SA + G[ z + 8 ].f );
   const Group & q = G[ z ];
   for( S i = q.f ; i < q.f + q.g ; ++i , ++at )
    key[ at ] = { std::uint64_t( SA[ i ] + q.at ) , 8 , SA[ i ] };
   }
  for( at = 0 ; at < total ; ++at ) {
   if( at + D < total )
    saisPrefetch( T + std::min( S( key[ at + D ].key ) , n - 1 ) );
   const S a = S( key[ at ].key );
   std::uint64_t v = 0;
   S l = 8;
   if( a + 8 <= n ) {
    std::memcpy( &v , T + a , 8 );
    if constexpr( std::endian::native == std::endian::little )
     v = saisSwap( v );
    }
   else {
    l = std::max< S >( n - a , 0 );
    for( S j = 0 ; j < l ; ++j )
     v |= std::uint64_t( std::uint8_t( T[ a + j ] ) ) << ( 56 - 8 * j );
    }
   key[ at ].key = v;
   key[ at ].left = l;
   }
  // each group sorted, and the parts still tied to the next round
  at = 0;
  for( std::size_t z = from ; z < to ; ++z ) {
   const Group q = G[ z ];
   Key * a = key.data() + at;
   sort( a , q.g );
   for( S i = 0 ; i < q.g ; ) {
    S j = i + 1;
    while( ( j < q.g ) && ( a[ j ].key == a[ i ].key ) &&
           ( a[ j ].left == a[ i ].left ) )
     ++j;
    if( ( j - i > 1 ) && ( a[ i ].left == 8 ) ) {
     H.push_back( { q.f + i , j - i , q.at + 8 } );
     left += std::size_t( j - i );
     }
    for( S y = i ; y < j ; ++y ) SA[ q.f + y ] = a[ y ].p;
    i = j;
    }
   at += std::size_t( q.g );
   }
  spent += total;
  return( std::pair( total , left ) );
  };
 // the first round on the first groups alone, up to an eighth of the
 // places: if half of them stay tied, the text is not the way (long
 // repeats), and the names do the work
 // large groups (more than 16 places on average): the names do better
 if( ( tied > budget ) || ( tied > 16 * G.size() ) ) {
  rest = ends;
  return( false );
  }
 std::size_t z = 0;
 for( std::size_t probe = 0 ; ( z < G.size() ) && ( 8 * probe < tied ) ; )
  probe += std::size_t( G[ z++ ].g );
 H.clear();
 const auto [ total , left ] = round( 0 , z );
 if( 2 * left > total ) {
  H.insert( H.end() , G.begin() + std::ptrdiff_t( z ) , G.end() );
  G.swap( H );
  }
 else {
  round( z , G.size() );
  G.swap( H );
  for( unsigned r = 1 ; ( r < rounds ) && ! G.empty() ; ++r ) {
   std::size_t next = 0;
   for( const Group & q : G ) next += std::size_t( q.g );
   if( spent + next > budget ) break;
   H.clear();
   round( 0 , G.size() );
   G.swap( H );
   }
  }
 if( G.empty() ) return( true );
 // every place a group of its own, but those of the groups left
 rest.assign( ends.size() , ~std::uint64_t( 0 ) );
 for( const Group & q : G )
  for( S i = q.f ; i < q.f + q.g - 1 ; ++i )
   rest[ std::size_t( i ) >> 6 ] &= ~( std::uint64_t( 1 ) << ( i & 63 ) );
 return( false );
 }

template< class S >
bool saisResolveTies( S * SA , S n , S m ,
                      const std::vector< std::uint64_t > & ends ,
                      const std::vector< std::uint64_t > & lms ,
                      std::size_t budget )
{
 const S * name = SA + m;
 auto next = [ & ]( S p ) { return( saisNextLms( lms , n , p ) ); };
 // the suffixes tied, those of the names of more than one; too many and
 // the doubling or the recursion do better
 std::size_t tied = 0 , spent = 0;
 saisForGroups( ends , m , [ & ]( S , S g ) {
  if( g > 1 ) tied += std::size_t( g ); } );
 if( tied > std::size_t( m ) / 4 ) return( false );
 // a before b, of equal names: their next names, until they differ; past
 // the budget every pair is equal, which ends any sort at once
 auto less = [ & ]( S a , S b ) {
  while( spent <= budget ) {
   a = next( a );
   b = next( b );
   if( ( a == n ) || ( b == n ) ) return( a == n );   // an end first
   const S x = name[ a >> 1 ] , y = name[ b >> 1 ];
   if( x != y ) return( x < y );
   ++spent;
   }
  return( false );
  };
 saisForGroups( ends , m , [ & ]( S f , S g ) {
  if( spent > budget ) return;
  if( g == 2 ) {
   if( less( SA[ f + 1 ] , SA[ f ] ) ) std::swap( SA[ f ] , SA[ f + 1 ] );
   }
  else if( g > 2 )
   std::sort( SA + f , SA + f + g , less );
  } );
 return( spent <= budget );
 }

/// the values but 0 of a[ from .. to ), less one, moved to a[ from .. ) in
/// their order (a slot of no LMS place is 0, a name is one more than its
/// value)
template< class S >
void saisCompact( S * a , S from , S to )
{
 S i = from , j = from;
#if defined( __AVX512F__ )
 if constexpr( sizeof( S ) == 4 ) {   // 16 at a time: the place j + 16 is
  const __m512i one = _mm512_set1_epi32( 1 );       // past no slot unread
  for( ; i + 16 <= to ; i += 16 ) {
   const __m512i v = _mm512_loadu_si512( a + i );
   const __mmask16 k = _mm512_test_epi32_mask( v , v );
   _mm512_storeu_si512( a + j , _mm512_maskz_compress_epi32( k ,
                                 _mm512_sub_epi32( v , one ) ) );
   j += S( std::popcount( unsigned( k ) ) );
   }
  }
#endif
 for( ; i < to ; ++i ) {
  const S v = a[ i ];
  a[ j ] = v - 1;
  j += S( v != 0 );
  }
 }

/// a part of a bucket in the partial induction: where the next entry
/// goes, and the name (the count of the marks met) of the last one moved in
template< class S >
struct SaisPart {
 S to , last;
 };

/// the move of the entry SA[ i ] by a scan of the partial induction, from
/// left to right (forS false: the suffix p - 1 , of type L , to the part
/// of its symbol by the type of p - 2 , at its start) or from right to left
/// (forS true: of type S, at its end); the suffix 0 moves nothing
template< bool forS , class C , class S >
inline void saisPartialMove( const C * ADS_SAIS_RESTRICT T ,
                             S * ADS_SAIS_RESTRICT SA ,
                             SaisPart< S > * ADS_SAIS_RESTRICT P , S i ,
                             S & d )
{
 constexpr S top = std::numeric_limits< S >::min() ,
             low = std::numeric_limits< S >::max();
 S p = SA[ i ];
 d += S( p < 0 );
 p &= low;
 const S j = p - 1;
 if( j == 0 ) return;
 const S c = S( T[ j ] ) , b = S( T[ j - 1 ] );
 SaisPart< S > & q = P[ 2 * std::size_t( c ) +
                        std::size_t( forS ? b > c : b < c ) ];
 const S at = forS ? --q.to : q.to++;
 SA[ at ] = j | ( top & -S( q.last != d ) );
 q.last = d;
 }

/// a scan of the partial induction over SA[ from .. to ), two entries a
/// step, with the prefetches of the final scans; returns d
template< bool forS , bool large , class C , class S >
S saisPartialScan( const C * ADS_SAIS_RESTRICT T , S * ADS_SAIS_RESTRICT SA ,
                   SaisPart< S > * ADS_SAIS_RESTRICT P , S from , S to ,
                   S d )
{
 constexpr S D = S( saisAhead ) , low = std::numeric_limits< S >::max();
 // the part of an entry p, with no check (p > 1 , or 0 for an entry not
 // yet there)
 auto part = [ & ]( S p ) {
  const S j = p - S( p > 1 );
  const S c = S( T[ j ] ) , b = S( T[ j - S( j > 0 ) ] );
  return( 2 * std::size_t( c ) + std::size_t( forS ? b > c : b < c ) );
  };
 auto ahead = [ & ]( S x , S y , S e , S f ) {
  x &= low;
  y &= low;
  saisPrefetch( T + ( x > 1 ? x : S( 2 ) ) - 2 );
  saisPrefetch( T + ( y > 1 ? y : S( 2 ) ) - 2 );
  if constexpr( large ) {             // the part, and where it writes
   saisPrefetchW( P + part( e & low ) );
   saisPrefetchW( SA + ( P[ part( f & low ) ].to - S( forS ) ) );
   }
  };
 auto ahead2 = [ & ]( S e , S f ) {   // the second entry of a step
  if constexpr( large ) {
   saisPrefetchW( P + part( e & low ) );
   saisPrefetchW( SA + ( P[ part( f & low ) ].to - S( forS ) ) );
   }
  };
 if constexpr( ! forS ) {
  S i = from;
  for( ; i + 2 * D + 1 < to ; i += 2 ) {
   saisPrefetch( SA + i + 2 * D );
   ahead( SA[ i + D ] , SA[ i + D + 1 ] , SA[ i + D / 2 ] ,
          SA[ i + D / 4 ] );
   ahead2( SA[ i + D / 2 + 1 ] , SA[ i + D / 4 + 1 ] );
   saisPartialMove< false >( T , SA , P , i , d );
   saisPartialMove< false >( T , SA , P , i + 1 , d );
   }
  for( ; i < to ; ++i ) saisPartialMove< false >( T , SA , P , i , d );
  }
 else {
  S i = to - 1;
  for( ; i - 2 * D - 1 >= from ; i -= 2 ) {
   saisPrefetch( SA + i - 2 * D );
   ahead( SA[ i - D ] , SA[ i - D - 1 ] , SA[ i - D / 2 ] ,
          SA[ i - D / 4 ] );
   ahead2( SA[ i - D / 2 - 1 ] , SA[ i - D / 4 - 1 ] );
   saisPartialMove< true >( T , SA , P , i , d );
   saisPartialMove< true >( T , SA , P , i - 1 , d );
   }
  for( ; i >= from ; --i ) saisPartialMove< true >( T , SA , P , i , d );
  }
 return( d );
 }

/// the classification of T (n >= 2 symbols in [ 0 , K )): the types of
/// its places and their kinds. The suffixes i >= 1 fall into four kinds
/// by the types of i and i - 1, LL, LS, LMS (S after L) and SS, counted
/// per first symbol in size4; lms is the bit vector of the LMS places,
/// *first the type of the suffix 0 (1 for S), and the number of the LMS
/// places is returned
template< class C , class S >
S saisClassify( const C * T , S n , S K , std::vector< S > & size4 ,
                std::vector< std::uint64_t > & lms , S * first )
{
 constexpr S D = S( saisAhead );
 const std::size_t k = std::size_t( K );
 // the types, 64 places a word from the last word: with G the places i
 // where T[ i ] < T[ i + 1 ] and P those where they are equal, t( i ) =
 // G( i ) | ( P( i ) & t( i + 1 ) ), the carries of the sum G + ( G | P )
 // read from the higher places to the lower ones, so that a word costs one
 // addition and the dependence passes from word to word only (the place
 // n - 1 is of type L, as if T[ n ] were smaller than every symbol)
 const std::size_t words = std::size_t( n ) / 64 + 1;
 std::vector< std::uint64_t > type( words , 0 );
 std::uint64_t above = 0;             // the type of the place above
 for( std::size_t w = words ; w-- > 0 ; ) {
  std::uint64_t Ga = 0 , Pa = 0;      // bit j: the place 64 w + j
  const S base = S( 64 * w );
  if( base + 64 < n )                 // T[ base + 64 ] is there
   saisCompare( T + base , &Ga , &Pa );
  else
   for( S i = base ; i < n - 1 ; ++i ) {
    const S a = S( T[ i ] ) , b = S( T[ i + 1 ] );
    Ga |= std::uint64_t( a < b ) << ( i - base );
    Pa |= std::uint64_t( a == b ) << ( i - base );
    }
  // bit j of G and P: the place 64 w + 63 - j
  const std::uint64_t G = saisReverse( Ga ) , P = saisReverse( Pa );
  const std::uint64_t B2 = G | P , s1 = G + B2 , s2 = s1 + above;
  const std::uint64_t out = std::uint64_t( s1 < G ) | std::uint64_t( s2 < s1 );
  const std::uint64_t X = ( ( s2 ^ G ^ B2 ) >> 1 ) | ( out << 63 );
  type[ w ] = saisReverse( X );       // bit j: the place 64 w + j
  above = X >> 63;
  }
 *first = S( type[ 0 ] & 1 );
 // the kinds of the places i >= 1, 2 t( i ) + t( i - 1 ): 0 LL , 1 LS ,
 // 2 LMS , 3 SS; on the small alphabets in two histograms, the even and
 // the odd places, so that equal symbols in a row do not wait for each
 // other
 size4.assign( 4 * k , 0 );
 lms.assign( words , 0 );
 const bool smallK = K <= S( saisSmall ) ,
            hugeK = K > S( saisCached );
 std::vector< S > odd( smallK ? 4 * k : 0 , 0 );
 S m = 0;
 // the place 0 is counted too, as if the place before were of type L,
 // and taken back after
 for( std::size_t w = 0 ; w < words ; ++w ) {
  const std::uint64_t t = type[ w ] ,
                      before = ( t << 1 ) | ( w ? type[ w - 1 ] >> 63 : 0 );
  std::uint64_t l = t & ~before;      // LMS: S after L
  if( w == 0 ) l &= ~std::uint64_t( 1 );
  lms[ w ] = l;
  m += S( std::popcount( l ) );
  const S base = S( 64 * w );
  const C * ADS_SAIS_RESTRICT x = T + base;
  // the kind of each place, 2 bits a place: bit 2 j + 1 the type of
  // 64 w + j, bit 2 j that of the place before
  if constexpr( ( sizeof( C ) == 1 ) && std::is_unsigned_v< C > )
   if( base + 64 <= n ) {
    S * ADS_SAIS_RESTRICT even = size4.data();
    S * ADS_SAIS_RESTRICT o = odd.data();
    alignas( 32 ) std::uint16_t idx[ 64 ];
    saisKinds( x , t , before , idx );
    for( unsigned j = 0 ; j < 64 ; j += 2 ) {
     ++even[ idx[ j ] ];
     ++o[ idx[ j + 1 ] ];
     }
    continue;
    }
  if( smallK && ( base + 64 <= n ) ) {
   S * ADS_SAIS_RESTRICT even = size4.data();
   S * ADS_SAIS_RESTRICT o = odd.data();
   for( unsigned j = 0 ; j < 64 ; j += 2 ) {
    const std::size_t k0 = 2 * ( ( t >> j ) & 1 ) + ( ( before >> j ) & 1 ) ,
                      k1 = 2 * ( ( t >> ( j + 1 ) ) & 1 ) +
                           ( ( before >> ( j + 1 ) ) & 1 );
    ++even[ 4 * std::size_t( x[ j ] ) + k0 ];
    ++o[ 4 * std::size_t( x[ j + 1 ] ) + k1 ];
    }
   }
  else {
   const S end = std::min< S >( base + 64 , n );
   for( S i = base ; i < end ; ++i ) {
    const unsigned j = unsigned( i - base );
    const std::size_t kind = 2 * ( ( t >> j ) & 1 ) + ( ( before >> j ) & 1 );
    if( hugeK && ( i + D < n ) )
     saisPrefetchW( size4.data() + 4 * std::size_t( T[ i + D ] ) );
    ++size4[ 4 * std::size_t( T[ i ] ) + kind ];
    }
   }
  }
 --size4[ 4 * std::size_t( T[ 0 ] ) + 2 * std::size_t( type[ 0 ] & 1 ) ];
 for( std::size_t c = 0 ; c < odd.size() ; ++c ) size4[ c ] += odd[ c ];
 return( m );
 }

/// the first stage of SA-IS, the partial induction: the LMS-substrings of
/// T (n >= 2 symbols in [ 0 , K ), m >= 1 LMS places classified by
/// saisClassify()) sorted and named by one scan of the L-suffixes whose
/// predecessor is L and one of those whose predecessor is S. The area A,
/// an LL part and an LMS part per symbol, holds the entries that the scan
/// from left to right moves, and the area C, an LS part and an SS part
/// per symbol, those that the scan from right to left moves, so that no
/// scan visits an entry with nothing to do. An entry is marked (its top
/// bit) when it starts a new name: a moved suffix is marked unless the
/// last one moved into the same part came from a suffix of the same name,
/// the name being the number d of the marks met by the scan. On return
/// SA[ 0 .. m ) holds the LMS places sorted by their substrings, ends the
/// bit vector of the last LMS place of each name and *names the number of
/// the names
template< class C , class S >
void saisPartial( const C * T , S * SA , S n , S K ,
                  const std::vector< S > & size4 ,
                  const std::vector< std::uint64_t > & lms , S m ,
                  S * names , std::vector< std::uint64_t > & ends )
{
 constexpr S top = std::numeric_limits< S >::min() ,
             low = std::numeric_limits< S >::max();
 const std::size_t k = std::size_t( K );

 // the parts: A = LL , LMS per symbol; C = LS , SS per symbol; the LMS
 // places sorted at last in [ 0 , m ), per symbol
 // a part v = 2 c + t of the moves of a scan: where the next move goes,
 // and the name of the last one, side by side in one line of cache
 using Part = SaisPart< S >;
 std::vector< Part > L( 2 * k , Part{ 0 , 0 } ) , R( 2 * k , Part{ 0 , 0 } );
 std::vector< S > seed( k );
 S sum = 0;
 for( std::size_t c = 0 ; c < k ; ++c ) {
  L[ 2 * c ].to = sum;               // LL: the moves of the L-scan into A
  sum += size4[ 4 * c ];
  seed[ c ] = sum;
  sum += size4[ 4 * c + 2 ];
  }
 const S endA = sum;
 for( std::size_t c = 0 , o = 0 ; c < k ; ++c ) {
  L[ 2 * c + 1 ].to = sum;           // LS: the moves of the L-scan into C
  sum += size4[ 4 * c + 1 ] + size4[ 4 * c + 3 ];
  R[ 2 * c ].to = sum;               // SS: the moves of the S-scan
  o += std::size_t( size4[ 4 * c + 2 ] );
  R[ 2 * c + 1 ].to = S( o );        // the LMS places, from the end
  }
 const S endC = sum;                 // n - 1: the suffix 0 is in none
 const bool large = K > S( saisSmall ) , huge = K > S( saisCached );
 // the prefetches of the huge alphabets read the symbols of entries not
 // written yet, which must then be 0
 if( huge ) std::fill( SA , SA + n , S( 0 ) );

 // the seeds, the LMS places by their first symbol, the first one of each
 // symbol marked
 for( std::size_t w = 0 ; w < lms.size() ; ++w ) {
  if( large && ( w + 1 < lms.size() ) )
   for( std::uint64_t v = lms[ w + 1 ] ; v ; v &= v - 1 )
    saisPrefetchW( seed.data() + T[ ( w + 1 ) * 64 +
                                    std::size_t( std::countr_zero( v ) ) ] );
  for( std::uint64_t v = lms[ w ] ; v ; v &= v - 1 ) {
   const S p = S( w * 64 + std::size_t( std::countr_zero( v ) ) );
   SA[ seed[ std::size_t( T[ p ] ) ]++ ] = p;
   }
  }
 for( std::size_t c = 0 ; c < k ; ++c )
  if( size4[ 4 * c + 2 ] ) SA[ seed[ c ] - size4[ 4 * c + 2 ] ] |= top;

 // the scan of A from left to right: the suffix p - 1, of type L, to the
 // LL part of its symbol if p - 2 is of type L, to its LS part otherwise
 S d = 1;
 {
  const S j = n - 1;
  const std::size_t v = 2 * std::size_t( T[ j ] ) +
                        std::size_t( T[ j - 1 ] < T[ j ] );
  SA[ L[ v ].to++ ] = j | top;
  L[ v ].last = d;
  }
 // the parts and the places they point to prefetched only on the
 // alphabets whose parts do not fit in the cache
 d = huge ?
     saisPartialScan< false , true >( T , SA , L.data() , S( 0 ) , endA , d ) :
     saisPartialScan< false , false >( T , SA , L.data() , S( 0 ) , endA , d );

 // the marks of the LS parts moved one place left, so that a mark tells
 // that an entry differs from the one after it, as the scan from right to
 // left meets them; the last entry of a part is always marked
 for( std::size_t c = 0 ; c < k ; ++c ) {
  const S e = L[ 2 * c + 1 ].to , b = e - size4[ 4 * c + 1 ];
  for( S i = e - 1 , carry = top ; i >= b ; --i ) {
   const S x = SA[ i ];
   SA[ i ] = ( x & low ) | carry;
   carry = x & top;
   }
  }

 // the scan of C from right to left: the suffix p - 1, of type S, to the
 // SS part of its symbol if p - 2 is of type S, among the sorted LMS
 // places otherwise
 d = huge ?
     saisPartialScan< true , true >( T , SA , R.data() , endA , endC , d ) :
     saisPartialScan< true , false >( T , SA , R.data() , endA , endC , d );

 // the names: SA[ i ] marked if it differs from SA[ i + 1 ]; here only
 // their groups, the names themselves written when needed
 ends.assign( std::size_t( m ) / 64 + 1 , 0 );
 S name = 0;
 for( std::size_t w = 0 ; 64 * w < std::size_t( m ) ; ++w ) {
  const S from = S( 64 * w ) , to = std::min< S >( from + 64 , m );
  std::uint64_t word = 0;
#if defined( __AVX2__ )
  if constexpr( sizeof( S ) == 4 )
   if( to - from == 64 ) {            // the signs by 8 at a time
    const __m256i keep = _mm256_set1_epi32( int( low ) );
    for( int h = 0 ; h < 8 ; ++h ) {
     auto * a = reinterpret_cast< __m256i * >( SA + from + 8 * h );
     const __m256i v = _mm256_loadu_si256( a );
     word |= std::uint64_t( unsigned( _mm256_movemask_ps(
              _mm256_castsi256_ps( v ) ) ) ) << ( 8 * h );
     _mm256_storeu_si256( a , _mm256_and_si256( v , keep ) );
     }
    ends[ w ] = word;
    name += S( std::popcount( word ) );
    continue;
    }
#endif
  for( S i = from ; i < to ; ++i ) {
   const S x = SA[ i ];
   word |= std::uint64_t( x < 0 ) << ( i - from );
   SA[ i ] = x & low;
   }
  ends[ w ] = word;
  name += S( std::popcount( word ) );
  }
 {                                    // the last place ends its group
  const std::size_t i = std::size_t( m - 1 );
  name += S( ( ( ends[ i >> 6 ] >> ( i & 63 ) ) & 1 ) == 0 );
  ends[ i >> 6 ] |= std::uint64_t( 1 ) << ( i & 63 );
  }
 *names = name;
 }

/// the text in codes of b bits a symbol, most significant first: the
/// ranks of the symbols that occur when they fit in 6 bits (b < 8), the
/// bytes themselves otherwise (b = 8); the windows of 64 bits at any bit
/// read past its end as zeros
struct SaisPacked {
 const unsigned char * t;             // the bytes, 8 bits a symbol
 std::size_t len;                     // the bytes in t
 std::vector< unsigned char > own;    // the packed text, padded
 unsigned b;                          // the bits of a symbol

 /// the bits [ o , o + 64 ) of the text
 std::uint64_t window( std::uint64_t o ) const {
  const std::size_t at = std::size_t( o >> 3 );
  const unsigned s = unsigned( o & 7 );
  std::uint64_t hi = 0 , lo = 0;
  if( at + 9 <= len ) {
   std::memcpy( &hi , t + at , 8 );
   if constexpr( std::endian::native == std::endian::little )
    hi = saisSwap( hi );
   lo = t[ at + 8 ];
   }
  else
   for( std::size_t j = 0 ; j < 9 ; ++j ) {
    const std::uint64_t v = at + j < len ? t[ at + j ] : 0;
    if( j < 8 ) hi |= v << ( 56 - 8 * j );
    else lo = v;
    }
  return( ( hi << s ) | ( lo >> ( 8 - s ) ) );
  }
 };

/// the codes of the symbols that occur as told by occurs, their ranks if
/// they fit in 6 bits, the bytes themselves otherwise; the bits of a code
inline unsigned saisCodes( const bool * occurs , unsigned char * code )
{
 unsigned sigma = 0;
 for( unsigned c = 0 ; c < 256 ; ++c ) {
  code[ c ] = static_cast< unsigned char >( sigma );
  sigma += occurs[ c ];
  }
 const unsigned b = unsigned( std::bit_width( std::max( sigma , 2u ) - 1 ) );
 if( b <= 6 ) return( b );
 for( unsigned c = 0 ; c < 256 ; ++c )
  code[ c ] = static_cast< unsigned char >( c );
 return( 8 );
 }

/// the window of 64 bits of the codes of T (b bits each) at the place p,
/// one symbol at a time, or only the symbols that cover its first limit
/// bits, the others zero
inline std::uint64_t saisSlowWindow( const unsigned char * T , std::size_t n ,
                                     const unsigned char * code , unsigned b ,
                                     std::size_t p , unsigned limit = 64 )
{
 std::uint64_t w = 0;
 unsigned have = 0;
 for( ; ( have < limit ) && ( have + b <= 64 ) && ( p < n ) ;
      have += b , ++p )
  w = ( w << b ) | code[ T[ p ] ];
 if( ( have >= limit ) || ( have + b <= 64 ) )        // or the text ended
  return( w << ( 64 - have ) );
 if( have == 64 ) return( w );
 const unsigned r = 64 - have;        // a part of the next symbol
 const std::uint64_t c = p < n ? code[ T[ p ] ] : 0;
 return( ( w << r ) | ( c >> ( b - r ) ) );
 }

/// the packed text of T, its codes of b bits [ see saisCodes() ]
inline void saisPack( const unsigned char * T , std::size_t n ,
                      const unsigned char * code , unsigned b ,
                      SaisPacked & P )
{
 if( b == 8 ) {                       // the bytes themselves
  P.t = T;
  P.len = n;
  P.b = 8;
  return;
  }
 P.b = b;
 P.own.assign( ( n * b + 7 ) / 8 + 16 , 0 );
 unsigned char * out = P.own.data();
 std::uint64_t acc = 0;               // the low have bits pending
 unsigned have = 0;
 auto put = [ & ]( std::uint64_t v , unsigned w ) {    // w <= 56 bits
  if( have + w < 64 ) {
   acc = ( acc << w ) | v;
   have += w;
   return;
   }
  const unsigned r = 64 - have;       // have >= 8, so r <= 56
  std::uint64_t word = ( acc << r ) | ( v >> ( w - r ) );
  if constexpr( std::endian::native == std::endian::little )
   word = saisSwap( word );
  std::memcpy( out , &word , 8 );
  out += 8;
  acc = v & ( ( std::uint64_t( 1 ) << ( w - r ) ) - 1 );
  have = w - r;
  };
 std::size_t i = 0;
#if defined( __BMI2__ )
 const std::uint64_t mask = 0x0101010101010101ull *
                            ( ( std::uint64_t( 1 ) << b ) - 1 );
 for( ; i + 8 <= n ; i += 8 ) {
  std::uint64_t x = 0;
  for( unsigned j = 0 ; j < 8 ; ++j )
   x |= std::uint64_t( code[ T[ i + j ] ] ) << ( 56 - 8 * j );
  put( _pext_u64( x , mask ) , 8 * b );
  }
#endif
 for( ; i < n ; ++i ) put( code[ T[ i ] ] , b );
 if( have ) {
  std::uint64_t word = acc << ( 64 - have );
  if constexpr( std::endian::native == std::endian::little )
   word = saisSwap( word );
  std::memcpy( out , &word , 8 );
  }
 P.t = P.own.data();
 P.len = P.own.size();
 }

/// the items e[ 0 .. g ) sorted by the P digits of bits bits of their keys
/// (keyOf) at the shifts sh[ 0 ] < ... < sh[ P - 1 ], by a radix sort from
/// the lowest digit, the counts of all the digits taken in one pass, back
/// and forth between e and t; count holds P << bits counters
template< unsigned P , class E , class F , class S >
void saisLsd( E * ADS_SAIS_RESTRICT e , E * ADS_SAIS_RESTRICT t , S g ,
              F keyOf , const unsigned * sh , unsigned bits ,
              S * ADS_SAIS_RESTRICT count )
{
 const std::size_t width = std::size_t( 1 ) << bits , mask = width - 1;
 std::fill( count , count + P * width , S( 0 ) );
 for( S i = 0 ; i < g ; ++i ) {
  const auto x = keyOf( e[ i ] );
  for( unsigned d = 0 ; d < P ; ++d )
   ++count[ d * width + ( std::size_t( x >> sh[ d ] ) & mask ) ];
  }
 for( unsigned d = 0 ; d < P ; ++d ) {   // the starts, exclusive
  S sum = 0;
  for( std::size_t z = 0 ; z < width ; ++z ) {
   const S c = count[ d * width + z ];
   count[ d * width + z ] = sum;
   sum += c;
   }
  }
 E * from = e , * to = t;
 for( unsigned d = 0 ; d < P ; ++d ) {
  S * const c = count + d * width;
  const unsigned s = sh[ d ];
  for( S i = 0 ; i < g ; ++i ) {
   const E x = from[ i ];
   to[ c[ std::size_t( keyOf( x ) >> s ) & mask ]++ ] = x;
   }
  std::swap( from , to );
  }
 if( from != e ) std::copy( from , from + g , e );
 }

/// the LMS-suffixes of T (n bytes, m LMS places in the bit vector lms,
/// size4 the counts of saisClassify()) sorted directly, into SA[ 0 .. m ),
/// by the windows of 64 bits of the packed text at their places: by the
/// top 8 bits into 256 buckets, then each bucket by a radix sort of the
/// bits of their keys below (the top 8 * sizeof( S ) bits of the window,
/// kept in SA[ m .. 2 m )) that its size needs and an insertion sort, and
/// the suffixes of equal keys by their whole windows and those that follow,
/// a shorter suffix first when one ends. False, with SA undefined, if a
/// sample of the windows repeats (the text has long repeats, and the
/// induction does better) or the ties read more than m windows
template< class S >
bool saisDirect( const unsigned char * T , S * SA , S n , S m ,
                 const std::vector< S > & size4 ,
                 const std::vector< std::uint64_t > & lms )
{
 using U = std::make_unsigned_t< S >;
 constexpr unsigned kb = 8 * sizeof( S );   // the bits of a key
 constexpr S D = S( saisAhead );
 bool occurs[ 256 ];
 for( unsigned c = 0 ; c < 256 ; ++c )
  occurs[ c ] = ( size4[ 4 * c ] | size4[ 4 * c + 1 ] | size4[ 4 * c + 2 ] |
                  size4[ 4 * c + 3 ] ) != 0;
 occurs[ T[ 0 ] ] = true;
 unsigned char code[ 256 ];
 const std::uint64_t b = saisCodes( occurs , code );
 // a sample of about 1024 keys of 32 bits, spread over the text: on a
 // text with no long repeats two of them are equal with a probability of
 // about 2^-32 per pair, and a few equal pairs tell of many ties among
 // all the windows, that the induction sorts in less time
 {
  std::vector< std::uint64_t > w;
  const std::size_t words = lms.size() , step = ( words + 1023 ) / 1024;
  for( std::size_t x = 0 ; x < words ; x += step )
   if( lms[ x ] )
    w.push_back( saisSlowWindow( T , std::size_t( n ) , code , unsigned( b ) ,
                  64 * x + std::size_t( std::countr_zero( lms[ x ] ) ) ,
                  32 ) );
  std::sort( w.begin() , w.end() );
  std::size_t twins = 0;
  for( std::size_t i = 1 ; i < w.size() ; ++i ) twins += w[ i ] == w[ i - 1 ];
  if( 256 * twins > w.size() ) return( false );
  }
 SaisPacked P;
 saisPack( T , std::size_t( n ) , code , unsigned( b ) , P );
 auto window = [ & ]( S p , std::uint64_t off ) {
  return( P.window( b * std::uint64_t( p ) + off ) );
  };
 // an item: the top bits of the window of a place (all 64 if S has 64
 // bits, else the top 32) and the place; on 32 bits one word, the key in
 // its high half, so that the items compare as their keys
 struct Wide {
  U k;
  S p;
  bool operator<( const Wide & o ) const { return( k < o.k ); }
  };
 using E = std::conditional_t< kb == 32 , std::uint64_t , Wide >;
 auto make = [ & ]( std::uint64_t w , S p ) {
  if constexpr( kb == 32 )
   return( E( ( w & 0xFFFFFFFF00000000ull ) | std::uint32_t( p ) ) );
  else
   return( E{ U( w ) , p } );
  };
 auto keyOf = [ & ]( const E & e ) {
  if constexpr( kb == 32 ) return( U( e >> 32 ) );
  else return( e.k );
  };
 auto placeOf = [ & ]( const E & e ) {
  if constexpr( kb == 32 ) return( S( std::uint32_t( e ) ) );
  else return( e.p );
  };
 // the items in SA[ 0 .. 2 m ). A radix sort on the top N bits of the
 // keys, N six more than the logarithm of m, so that few keys are equal
 // on them: on few items (m < 2^16, all in the cache) by passes of at
 // most 11 bits on all of them, else first into the 256 buckets of the
 // top 8 bits (on the bytes themselves the first symbols, whose LMS
 // places are counted), which then fit in the cache, and each bucket by
 // passes on the rest
 if( reinterpret_cast< std::uintptr_t >( SA ) % alignof( E ) ) return( false );
 E * const A = reinterpret_cast< E * >( SA );
 const unsigned N = std::min( kb ,
                     unsigned( std::bit_width( std::size_t( m ) ) ) + 6 ) ,
                B1 = m < ( S( 1 ) << 16 ) ? 0 : 8;
 std::vector< S > start( ( std::size_t( 1 ) << B1 ) + 1 , 0 );
 auto forLms = [ & ]( auto && f ) {
  for( std::size_t x = 0 ; x < lms.size() ; ++x )
   for( std::uint64_t v = lms[ x ] ; v ; v &= v - 1 )
    f( S( 64 * x + std::size_t( std::countr_zero( v ) ) ) );
  };
 if( B1 == 0 ) {
  start[ 1 ] = m;
  S i = 0;
  forLms( [ & ]( S p ) { A[ i++ ] = make( window( p , 0 ) , p ); } );
  }
 else {
  if( b == 8 )
   for( unsigned c = 0 ; c < 256 ; ++c ) start[ c + 1 ] = size4[ 4 * c + 2 ];
  else
   forLms( [ & ]( S p ) { ++start[ ( window( p , 0 ) >> 56 ) + 1 ]; } );
  for( std::size_t c = 0 ; c + 1 < start.size() ; ++c )
   start[ c + 1 ] += start[ c ];
  std::array< S , 256 > at;
  std::copy( start.begin() , start.end() - 1 , at.begin() );
  forLms( [ & ]( S p ) {
   const std::uint64_t w = window( p , 0 );
   A[ at[ w >> 56 ]++ ] = make( w , p );
   } );
  }
 // each bucket by the next N - B1 bits of its keys, in passes of at most
 // 11 bits and no more counters than twice its items; then the runs still
 // equal on the N bits (few) by insertion if short, else by a comparison
 // sort, and the runs of equal keys kept
 std::vector< E > A2;
 std::vector< std::pair< S , S > > G;      // ( first place , size )
 std::vector< S > count;
 auto insertion = [ & ]( E * e , S g ) {
  for( S i = 1 ; i < g ; ++i ) {
   const E x = e[ i ];
   S j = i;
   for( ; ( j > 0 ) && ( x < e[ j - 1 ] ) ; --j ) e[ j ] = e[ j - 1 ];
   e[ j ] = x;
   }
  };
 const unsigned low = kb - N;         // the bits of a key not sorted
 for( std::size_t c = 0 ; c + 1 < start.size() ; ++c ) {
  const S f = start[ c ] , g = start[ c + 1 ] - f;
  if( g < 2 ) continue;
  E * const e = A + f;
  if( g <= 16 )
   insertion( e , g );
  else {
   const unsigned R = N - B1 ,
                  most = std::min( 11u , unsigned( std::bit_width(
                                                  std::size_t( g ) ) ) + 1 ) ,
                  passes = std::min( ( R + most - 1 ) / most , 3u ) ,
                  bits = ( R + passes - 1 ) / passes;
   if( A2.size() < std::size_t( g ) ) A2.resize( std::size_t( g ) );
   std::array< unsigned , 3 > sh{};
   for( unsigned d = 0 ; d < passes ; ++d )
    sh[ d ] = unsigned( std::max( int( kb - B1 ) -
                                  int( bits * ( passes - d ) ) ,
                                  int( low ) ) );
   count.resize( std::size_t( 3 ) << bits );
   auto lsd = [ & ]< unsigned P >() {
    saisLsd< P >( e , A2.data() , g , keyOf , sh.data() , bits ,
                  count.data() );
    };
   if( passes == 1 ) lsd.template operator()< 1 >();
   else if( passes == 2 ) lsd.template operator()< 2 >();
   else lsd.template operator()< 3 >();
   for( S i = 1 ; i < g ; ++i )       // the runs equal on the N bits
    if( ( keyOf( e[ i ] ) >> low ) == ( keyOf( e[ i - 1 ] ) >> low ) ) {
     const U r = keyOf( e[ i ] ) >> low;
     S j = i + 1;
     while( ( j < g ) && ( ( keyOf( e[ j ] ) >> low ) == r ) ) ++j;
     if( j - i < 16 ) insertion( e + i - 1 , j - i + 1 );
     else std::sort( e + i - 1 , e + j );
     i = j;
     }
   }
  for( S i = 1 ; i < g ; ++i )        // the runs of equal keys
   if( keyOf( e[ i ] ) == keyOf( e[ i - 1 ] ) ) {
    S j = i + 1;
    while( ( j < g ) && ( keyOf( e[ j ] ) == keyOf( e[ i ] ) ) ) ++j;
    G.emplace_back( f + i - 1 , j - i + 1 );
    i = j;
    }
  }
 // the places alone in SA[ 0 .. m ), forward: the place i is written
 // after the item i (at or past it) is read
 S * const Q = SA;
 for( S i = 0 ; i < m ; ++i ) Q[ i ] = placeOf( A[ i ] );
 // the ties, by the windows from the bit off on (the first kb bits are
 // equal): with r the bits of the text from that bit on, a suffix that
 // ends in the window (r < 64) goes before those that go on (the shorter
 // first), and those that go on with equal windows carry on to the next
 // window; at most m windows read
 struct Key {
  std::uint64_t w;
  std::int64_t left;
  S p;
  bool operator<( const Key & o ) const {
   return( ( w < o.w ) || ( ( w == o.w ) && ( left < o.left ) ) );
   }
  };
 std::vector< Key > key;
 std::vector< std::pair< S , S > > H;
 std::size_t spent = 0;
 const std::int64_t bits = std::int64_t( b ) * std::int64_t( n );
 for( std::uint64_t off = kb ; ! G.empty() ; off += 64 ) {
  H.clear();
  for( const auto & [ f , g ] : G ) {
   spent += std::size_t( g );
   if( spent > std::size_t( m ) ) return( false );
   key.resize( std::size_t( g ) );
   for( S i = 0 ; i < g ; ++i ) {
    const S p = Q[ f + i ];
    if( i + D < g ) saisPrefetch( P.t + std::min< std::uint64_t >(
                      ( b * std::uint64_t( Q[ f + i + D ] ) + off ) >> 3 ,
                      P.len ) );
    const std::int64_t r = bits - std::int64_t( b * std::uint64_t( p ) +
                                                off );
    key[ std::size_t( i ) ] = { window( p , off ) ,
                                std::min< std::int64_t >( r , 64 ) , p };
    }
   std::sort( key.begin() , key.end() );
   for( S i = 0 ; i < g ; ) {
    S j = i + 1;
    while( ( j < g ) && ( key[ j ].w == key[ i ].w ) &&
           ( key[ j ].left == key[ i ].left ) )
     ++j;
    if( ( j - i > 1 ) && ( key[ i ].left == 64 ) )
     H.emplace_back( f + i , j - i );
    i = j;
    }
   for( S i = 0 ; i < g ; ++i ) Q[ f + i ] = key[ std::size_t( i ) ].p;
   }
  G.swap( H );
  }
 return( true );
 }

/// SA-IS on T[ 0 .. n - 1 ], symbols in [ 0 , K ), into SA[ 0 .. n - 1 ]
template< class C , class S >
void sais( const C * T , S * SA , S n , S K )
{
 if( n <= 1 ) {
  if( n == 1 ) SA[ 0 ] = 0;
  return;
  }
 constexpr S D = S( saisAhead );
 std::vector< S > size4;
 std::vector< std::uint64_t > lms , ends;
 S first , names;
 const S m = saisClassify( T , n , K , size4 , lms , &first );
 // the numbers of the symbols, the suffix 0 in none of the kinds
 std::vector< S > size( std::size_t( K ) , 0 ) , B( std::size_t( K ) , 0 );
 for( std::size_t c = 0 ; c < std::size_t( K ) ; ++c )
  size[ c ] = size4[ 4 * c ] + size4[ 4 * c + 1 ] + size4[ 4 * c + 2 ] +
              size4[ 4 * c + 3 ];
 ++size[ std::size_t( T[ 0 ] ) ];

 // no LMS place: the suffixes of type S, if any, are a prefix of T; with
 // none (the suffix 0 is of type L) T never rises, each suffix is larger
 // than the next one and SA is n - 1 , ... , 0
 if( m == 0 ) {
  if( first == 0 )
   for( S r = 0 ; r < n ; ++r ) SA[ r ] = n - 1 - r;
  else {
   std::fill( SA , SA + n , S( 0 ) );
   saisInduce( T , SA , n , size.data() , B.data() , K );
   }
  return;
  }

 // the LMS-suffixes sorted by the string of the names: if the names are
 // not all distinct, each group of equal names by the names that follow,
 // within a budget of names compared; beyond it, by the doubling on the
 // groups of equal names, if few, or else by recursion, on the string of
 // the names, gathered in SA[ m .. 2 m ) in the order of the text

 // the names, in the slot m + p / 2 of the LMS place p, when the text
 // leaves some tied: by the groups left, finer than those of the
 // LMS-substrings when the text has split some (equal names still mean
 // equal LMS-substrings, and the names grow with the suffixes)
 auto write = [ & ]( const std::vector< std::uint64_t > & by ) {
  std::fill( SA + m , SA + n , S( 0 ) );
  S name = 1;
  saisForGroups( by , m , [ & ]( S f , S g ) {
   S i = f;
   for( const S e = std::min( f + g , m - D ) ; i < e ; ++i ) {
    saisPrefetchW( SA + m + ( SA[ i + D ] >> 1 ) );
    SA[ m + ( SA[ i ] >> 1 ) ] = name;
    }
   for( ; i < f + g ; ++i ) SA[ m + ( SA[ i ] >> 1 ) ] = name;
   ++name;
   } );
  };
 std::vector< std::uint64_t > rest;
 const std::vector< std::uint64_t > * tied = nullptr;
 bool sorted = false;
 if constexpr( ( sizeof( C ) == 1 ) && std::is_unsigned_v< C > )
  sorted = saisDirect( T , SA , n , m , size4 , lms );
 if( ! sorted )
  saisPartial( T , SA , n , K , size4 , lms , m , &names , ends );
 else
  names = m;
 if( names < m ) {
  tied = &ends;
  if constexpr( ( sizeof( C ) == 1 ) && std::is_unsigned_v< C > ) {
   if( saisTextTies( T , n , SA , m , ends , lms , rest ,
                     2 * std::size_t( m ) , 8 ) )
    tied = nullptr;
   else
    tied = &rest;
   }
  if( tied ) write( *tied );
  }
 if( tied && ! saisResolveTies( SA , n , m , *tied , lms ,
                                std::size_t( m ) ) ) {
  saisCompact( SA , m , n );          // the names, in R = SA[ m .. 2 m )
  S * R = SA + m;
  std::vector< S > count;              // of each name, in their order
  count.reserve( std::size_t( names ) );
  saisForGroups( *tied , m , [ & ]( S , S g ) { count.push_back( g ); } );
  if( ! saisDoubling( static_cast< const S * >( R ) , SA , m , count ,
                      m / 4 ) )
   sais( static_cast< const S * >( R ) , SA , m , S( count.size() ) );
  S j = 0;                            // the LMS places, in order
  for( std::size_t w = 0 ; w < lms.size() ; ++w )
   for( std::uint64_t v = lms[ w ] ; v ; v &= v - 1 )
    R[ j++ ] = S( w * 64 + std::size_t( std::countr_zero( v ) ) );
  for( S i = 0 ; i < m ; ++i ) {
   if( i + D < m ) saisPrefetch( R + SA[ i + D ] );
   SA[ i ] = R[ SA[ i ] ];
   }
  }

 // the LMS-suffixes at the ends of their buckets, and the induction
 // the sorted LMS places come grouped by their first symbol, so that
 // each group moves at once to the end of its bucket, from the last one
 saisBuckets( size.data() , B.data() , K , true );
 S j = n , at = m;                   // [ j , n ) placed, SA[ 0 .. at ) to go
 for( S c = K - 1 ; ( c >= 0 ) && ( at > 0 ) ; --c ) {
  const S l = size4[ 4 * std::size_t( c ) + 2 ] , e = B[ std::size_t( c ) ];
  if( l == 0 ) continue;
  std::fill( SA + e , SA + j , S( 0 ) );
  std::memmove( SA + e - l , SA + at - l , std::size_t( l ) * sizeof( S ) );
  j = e - l;
  at -= l;
  }
 std::fill( SA , SA + j , S( 0 ) );
 saisInduce( T , SA , n , size.data() , B.data() , K );
 }

/// SA is the suffix array of T (bytes or integers), O( n )
template< class C , class I >
bool isSuffixArrayOf( const C * T , std::size_t n , const I * SA )
{
 std::vector< std::size_t > R( n + 1 , 0 );
 for( std::size_t r = 0 ; r < n ; ++r ) {
  if( ( SA[ r ] >= n ) || R[ SA[ r ] ] ) return( false );
  R[ SA[ r ] ] = r + 1;              // 0 for the end
  }
 for( std::size_t r = 1 ; r < n ; ++r ) {
  const std::size_t a = SA[ r - 1 ] , b = SA[ r ];
  if( T[ a ] != T[ b ] ) {
   if( T[ a ] > T[ b ] ) return( false );
   }
  else if( R[ a + 1 ] >= R[ b + 1 ] ) return( false );
  }
 return( true );
 }

/// LCP[ r ] is the lcp of the suffixes SA[ r ] and SA[ r + 1 ] of T
template< class I >
bool isLcpArrayOf( std::string_view T , const std::vector< I > & SA ,
                   const std::vector< I > & LCP )
{
 const std::size_t n = T.size();
 if( LCP.size() != ( n < 2 ? 0 : n - 1 ) ) return( false );
 for( std::size_t r = 0 ; r + 1 < n ; ++r ) {
  const std::size_t a = SA[ r ] , b = SA[ r + 1 ] , h = LCP[ r ];
  if( ( a + h > n ) || ( b + h > n ) ||
      ( T.substr( a , h ) != T.substr( b , h ) ) ||
      ( ( a + h < n ) && ( b + h < n ) && ( T[ a + h ] == T[ b + h ] ) ) )
   return( false );
  }
 return( true );
 }

/// the permuted LCP of T and SA into P, by Phi, and if Y is not null each
/// value also in Y[ i ], a byte, 255 for 255 or more, until more than n /
/// 64 values are 255 or more (long repeats): whether Y was filled is
/// returned (the arrays are declared not to overlap, so that the stores
/// into P do not reload SA and the text)
template< bool bytes , class I >
bool plcp( const unsigned char * ADS_SAIS_RESTRICT t , std::size_t n ,
           const I * ADS_SAIS_RESTRICT SA , I * ADS_SAIS_RESTRICT P ,
           unsigned char * ADS_SAIS_RESTRICT Y = nullptr )
{
 if( n == 0 ) return( bytes );
 // Phi in P, n for the first suffix of SA, the last suffix of SA kept
 constexpr std::size_t D = saisAhead;
 I prev = I( n );
 std::size_t r = 0;
 for( ; r + 2 * D + 4 <= n ; r += 4 ) {         // 4 at a time, no checks
  saisPrefetch( SA + r + 2 * D );
  for( std::size_t q = 0 ; q < 4 ; ++q ) saisPrefetchW( P + SA[ r + D + q ] );
  for( std::size_t q = 0 ; q < 4 ; ++q ) {
   P[ SA[ r + q ] ] = prev;
   prev = SA[ r + q ];
   }
  }
 for( ; r < n ; ++r ) {
  P[ SA[ r ] ] = prev;
  prev = SA[ r ];
  }
 // the text at Phi prefetched ahead, the place of the comparison but for
 // h; the first suffix of SA has Phi = n, and h is then already 0, since
 // the suffix before it in the text shares at most one symbol with its
 // predecessor in SA. The symbols compared one by one: h grows by n + 1
 // at most in all, so that wider loads would save little and cost on the
 // short lcps of the texts with few repeats
 auto lcp = [ & ]( std::size_t i , std::size_t h ) {
  const std::size_t j = P[ i ] , far = n - std::max( i , j );
  while( ( h < far ) && ( t[ i + h ] == t[ j + h ] ) ) ++h;
  return( h );
  };
 std::size_t h = 0 , i = 0 , big = 0;
 auto put = [ & ]( std::size_t x ) {
  P[ x ] = I( h );
  if constexpr( bytes ) if( Y ) {
   Y[ x ] = static_cast< unsigned char >( std::min< std::size_t >( h , 255 ) );
   big += h >= 255;
   if( big > n / 64 ) Y = nullptr;
   }
  };
 for( ; i + 2 * D < n ; ++i ) {                 // no checks
  saisPrefetchW( P + i + 2 * D );
  saisPrefetch( reinterpret_cast< const void * >(
                 reinterpret_cast< std::uintptr_t >( t ) + P[ i + D ] + h ) );
  h = lcp( i , h );
  put( i );
  h -= ( h != 0 );
  }
 for( ; i < n ; ++i ) {
  if( i + D < n )
   saisPrefetch( t + std::min< std::size_t >( P[ i + D ] + h , n ) );
  h = lcp( i , h );
  put( i );
  h -= ( h != 0 );
  }
 return( Y != nullptr );
 }

/// LCP[ r ] = P[ SA[ r ] ] for r in [ 0 , n ), the arrays declared not to
/// overlap and P prefetched D entries ahead
template< class I >
void plcpToLcp( const I * ADS_SAIS_RESTRICT P ,
                const I * ADS_SAIS_RESTRICT SA , I * ADS_SAIS_RESTRICT LCP ,
                std::size_t n )
{
 constexpr std::size_t D = saisAhead;
 std::size_t r = 0;
 for( ; r + 2 * D + 4 <= n ; r += 4 ) {         // 4 at a time, no checks
  saisPrefetch( SA + r + 2 * D );
  saisPrefetchW( LCP + r + D );
  for( std::size_t q = 0 ; q < 4 ; ++q ) saisPrefetch( P + SA[ r + D + q ] );
  for( std::size_t q = 0 ; q < 4 ; ++q ) LCP[ r + q ] = P[ SA[ r + q ] ];
  }
 for( ; r < n ; ++r ) LCP[ r ] = P[ SA[ r ] ];
 }

/// LCP[ r ] = P[ SA[ r ] ] for r in [ 0 , n ) by the bytes Y of P [ see
/// plcp() ], P read only for the values of 255 or more: the random reads
/// fall in an array of n bytes, not of n integers, which stays in the
/// cache four times longer
template< class I >
void plcpToLcp( const unsigned char * ADS_SAIS_RESTRICT Y ,
                const I * ADS_SAIS_RESTRICT P ,
                const I * ADS_SAIS_RESTRICT SA , I * ADS_SAIS_RESTRICT LCP ,
                std::size_t n )
{
 constexpr std::size_t D = saisAhead;
 std::size_t r = 0;
 auto get = [ & ]( std::size_t x ) {
  const I y = Y[ x ];
  return( y == 255 ? P[ x ] : y );
  };
 for( ; r + 2 * D + 4 <= n ; r += 4 ) {         // 4 at a time, no checks
  saisPrefetch( SA + r + 2 * D );
  saisPrefetchW( LCP + r + D );
  for( std::size_t q = 0 ; q < 4 ; ++q ) saisPrefetch( Y + SA[ r + D + q ] );
  for( std::size_t q = 0 ; q < 4 ; ++q ) LCP[ r + q ] = get( SA[ r + q ] );
  }
 for( ; r < n ; ++r ) LCP[ r ] = get( SA[ r ] );
 }

/// the texts from which the LCP array reads the permuted LCP by its bytes
/// [ see plcpToLcp() ]: where the integers would not stay in the cache
inline constexpr std::size_t lcpBytesFrom = std::size_t( 1 ) << 18;

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::vector< I > suffixArray( std::string_view T )
{
 using S = std::make_signed_t< I >;
 ADS_PRE( T.size() < std::size_t( std::numeric_limits< S >::max() ) );
 auto SA = detail::saisVector< I >( T.size() );
 detail::sais( reinterpret_cast< const unsigned char * >( T.data() ) ,
               reinterpret_cast< S * >( SA.data() ) , S( T.size() ) ,
               S( 256 ) );
 ADS_AUDIT( detail::isSuffixArrayOf(
             reinterpret_cast< const unsigned char * >( T.data() ) ,
             T.size() , SA.data() ) );
 return( SA );
 }

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::vector< I > suffixArray( const std::vector< I > & T , std::size_t K )
{
 using S = std::make_signed_t< I >;
 ADS_PRE( T.size() < std::size_t( std::numeric_limits< S >::max() ) );
 ADS_PRE( K <= std::size_t( std::numeric_limits< S >::max() ) );
 ADS_PRE( std::all_of( T.begin() , T.end() , [ K ]( I c ) {
  return( c < K ); } ) );
 auto SA = detail::saisVector< I >( T.size() );
 detail::sais( reinterpret_cast< const S * >( T.data() ) ,
               reinterpret_cast< S * >( SA.data() ) , S( T.size() ) ,
               S( std::max< std::size_t >( K , 1 ) ) );
 ADS_AUDIT( detail::isSuffixArrayOf( T.data() , T.size() , SA.data() ) );
 return( SA );
 }

template< class I >
std::vector< I > plcpArray( std::string_view T , const std::vector< I > & SA )
{
 const std::size_t n = T.size();
 ADS_AUDIT( ( SA.size() == n ) && detail::isSuffixArrayOf(
             reinterpret_cast< const unsigned char * >( T.data() ) , n ,
             SA.data() ) );
 auto P = detail::saisVector< I >( n );
 detail::plcp< false >( reinterpret_cast< const unsigned char * >(
                         T.data() ) , n , SA.data() , P.data() );
 return( P );
 }

template< class I >
std::vector< I > lcpArrayPhi( std::string_view T ,
                              const std::vector< I > & SA )
{
 const std::size_t n = T.size();
 ADS_AUDIT( ( SA.size() == n ) && detail::isSuffixArrayOf(
             reinterpret_cast< const unsigned char * >( T.data() ) , n ,
             SA.data() ) );
 const auto P = std::make_unique_for_overwrite< I[] >( n );
 detail::saisHuge( P.get() , n * sizeof( I ) );
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 auto LCP = detail::saisVector< I >( n < 2 ? 0 : n - 1 );
 if( n >= detail::lcpBytesFrom ) {
  // the bytes, unless many values do not fit (long repeats)
  const auto Y = std::make_unique_for_overwrite< unsigned char[] >( n );
  detail::saisHuge( Y.get() , n );
  if( detail::plcp< true >( t , n , SA.data() , P.get() , Y.get() ) )
   detail::plcpToLcp( Y.get() , P.get() , SA.data() + 1 , LCP.data() ,
                      n - 1 );
  else
   detail::plcpToLcp( P.get() , SA.data() + 1 , LCP.data() , n - 1 );
  }
 else {
  detail::plcp< false >( t , n , SA.data() , P.get() );
  if( n >= 2 )
   detail::plcpToLcp( P.get() , SA.data() + 1 , LCP.data() , n - 1 );
  }
 ADS_AUDIT( detail::isLcpArrayOf( T , SA , LCP ) );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SuffixSorting.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
