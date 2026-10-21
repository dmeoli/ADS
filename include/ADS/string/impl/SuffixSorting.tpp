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
#include <bit>
#include <cstring>
#include <limits>
#include <memory>
#include <type_traits>

#if defined( __AVX2__ )
 #include <immintrin.h>
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
               bit = _mm256_set1_epi64x( std::int64_t( 0x8040201008040201ull ) );
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

/// the scan from left to right: the L-suffixes, each stored complemented
/// if the suffix before it is of type S, and every entry scanned
/// complemented. Two entries a step, the text before the entries D ahead
/// prefetched, and on the large alphabets also their buckets (D / 2
/// ahead) and the places these point to (D / 4 ahead); the arrays are
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

/// the scan from right to left: the S-suffixes, each stored complemented
/// if the suffix before it is of type L (or it is the suffix 0), and the
/// complemented entries restored. Two entries a step, prefetched as in
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
   const S x = name[ a / 2 ] , y = name[ b / 2 ];
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
  saisPrefetch( T + saisClamp( x - 2 ) );
  saisPrefetch( T + saisClamp( y - 2 ) );
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

/// the first stage of SA-IS, the partial induction: the types and the LMS
/// places of T (n >= 2 symbols in [ 0 , K )), and the LMS-substrings
/// sorted and named by one scan of the L-suffixes whose predecessor is L
/// and one of those whose predecessor is S. The suffixes i >= 1 fall into
/// four kinds by the types of i and i - 1, LL, LS, LMS (S after L) and SS,
/// counted per first symbol in size4; the area A, an LL part and an LMS
/// part per symbol, holds the entries that the scan from left to right
/// moves, and the area C, an LS part and an SS part per symbol, those that
/// the scan from right to left moves, so that no scan visits an entry with
/// nothing to do. An entry is marked (its top bit) when it starts a new
/// name: a moved suffix is marked unless the last one moved into the same
/// part came from a suffix of the same name, the name being the number d
/// of the marks met by the scan. On return SA[ 0 .. m ) holds the LMS
/// places sorted by their substrings, ends the bit vector of the last LMS
/// place of each name, *names the number of the names, lms the
/// bit vector of the LMS places and *first the type of the suffix 0 (1 for
/// S); the number of the LMS places is returned
template< class C , class S >
S saisPartial( const C * T , S * SA , S n , S K ,
               std::vector< S > & size4 ,
               std::vector< std::uint64_t > & lms , S * first ,
               S * names , std::vector< std::uint64_t > & ends )
{
 constexpr S top = std::numeric_limits< S >::min() ,
             low = std::numeric_limits< S >::max();
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
 if( m == 0 ) return( 0 );

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
 for( S i = 0 ; i < m ; ++i ) {
  const S x = SA[ i ] , b = S( x < 0 ) | S( i + 1 == m );
  SA[ i ] = x & low;
  ends[ std::size_t( i ) >> 6 ] |= std::uint64_t( b ) << ( i & 63 );
  name += b;
  }
 *names = name;
 return( m );
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
 const S m = saisPartial( T , SA , n , K , size4 , lms , &first , &names ,
                          ends );
 // the numbers of the symbols, the suffix 0 in none of the kinds
 std::vector< S > size( std::size_t( K ) , 0 ) , B( std::size_t( K ) , 0 );
 for( std::size_t c = 0 ; c < std::size_t( K ) ; ++c )
  size[ c ] = size4[ 4 * c ] + size4[ 4 * c + 1 ] + size4[ 4 * c + 2 ] +
              size4[ 4 * c + 3 ];
 ++size[ std::size_t( T[ 0 ] ) ];
 // the LMS places from the last to the first, by the bits set
 auto forLms = [ & ]( auto && f ) {
  for( std::size_t w = lms.size() ; w-- > 0 ; )
   for( std::uint64_t v = lms[ w ] ; v ; ) {
    const int h = 63 - std::countl_zero( v );
    v &= ~( std::uint64_t( 1 ) << h );
    f( S( w * 64 + std::size_t( h ) ) );
    }
  };

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
 // the names at the back of SA
 // the names, in the slot m + p / 2 of the LMS place p, when the text
 // leaves some tied
 auto write = [ & ]() {
  std::fill( SA + m , SA + n , S( 0 ) );
  S name = 1;
  saisForGroups( ends , m , [ & ]( S f , S g ) {
   for( S i = f ; i < f + g ; ++i ) {
    if( i + D < m ) saisPrefetchW( SA + m + SA[ i + D ] / 2 );
    SA[ m + SA[ i ] / 2 ] = name;
    }
   ++name;
   } );
  };
 std::vector< std::uint64_t > rest;
 const std::vector< std::uint64_t > * tied = nullptr;
 if( names < m ) {
  tied = &ends;
  if constexpr( ( sizeof( C ) == 1 ) && std::is_unsigned_v< C > ) {
   if( saisTextTies( T , n , SA , m , ends , lms , rest ,
                     2 * std::size_t( m ) , 8 ) )
    tied = nullptr;
   else
    tied = &rest;
   }
  if( tied ) write();
  }
 if( tied && ! saisResolveTies( SA , n , m , *tied , lms ,
                                std::size_t( m ) ) ) {
  for( S i = n - 1 , j = n - 1 ; i >= m ; --i ) {
   const S v = SA[ i ];
   SA[ j ] = v - 1;                   // a slot of no LMS place is 0
   j -= S( v != 0 );
   }
  S * R = SA + ( n - m );
  std::vector< S > count;              // of each name, in their order
  count.reserve( std::size_t( names ) );
  saisForGroups( ends , m , [ & ]( S , S g ) { count.push_back( g ); } );
  if( ! saisDoubling( static_cast< const S * >( R ) , SA , m , count ,
                      m / 4 ) )
   sais( static_cast< const S * >( R ) , SA , m , names );
  S j = m;                            // the LMS places, in order
  forLms( [ & ]( S i ) { R[ --j ] = i; } );
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

/// the permuted LCP of T and SA into P, by Phi
/// (the arrays are declared not to overlap, so that the stores into P do
/// not reload SA and the text)
template< class I >
void plcp( const unsigned char * ADS_SAIS_RESTRICT t , std::size_t n ,
           const I * ADS_SAIS_RESTRICT SA , I * ADS_SAIS_RESTRICT P )
{
 if( n == 0 ) return;
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
 // predecessor in SA. The first 4 symbols are compared one by one, since
 // the lcp is short on the texts with few repeats (and a load of 8 would
 // often reach a line not prefetched), then 8 at a time
 auto lcp = [ & ]( std::size_t i , std::size_t h ) {
  const std::size_t j = P[ i ] , far = n - std::max( i , j );
  const std::size_t near = std::min( far , h + 4 );
  while( ( h < near ) && ( t[ i + h ] == t[ j + h ] ) ) ++h;
  if( h == near )
   for( ; ; ) {
    if( h + 8 > far ) {
     while( ( h < far ) && ( t[ i + h ] == t[ j + h ] ) ) ++h;
     break;
     }
    std::uint64_t a , b;
    std::memcpy( &a , t + i + h , 8 );
    std::memcpy( &b , t + j + h , 8 );
    if( a != b ) {
     if constexpr( std::endian::native == std::endian::little )
      h += std::size_t( std::countr_zero( a ^ b ) ) / 8;
     else
      h += std::size_t( std::countl_zero( a ^ b ) ) / 8;
     break;
     }
    h += 8;
    }
  return( h );
  };
 std::size_t h = 0 , i = 0;
 for( ; i + 2 * D < n ; ++i ) {                 // no checks
  saisPrefetchW( P + i + 2 * D );
  saisPrefetch( t + std::min< std::size_t >( P[ i + D ] + h , n ) );
  h = lcp( i , h );
  P[ i ] = I( h );
  h -= ( h != 0 );
  }
 for( ; i < n ; ++i ) {
  if( i + D < n )
   saisPrefetch( t + std::min< std::size_t >( P[ i + D ] + h , n ) );
  h = lcp( i , h );
  P[ i ] = I( h );
  h -= ( h != 0 );
  }
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

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::vector< I > suffixArray( std::string_view T )
{
 using S = std::make_signed_t< I >;
 ADS_PRE( T.size() < std::size_t( std::numeric_limits< S >::max() ) );
 std::vector< I > SA( T.size() );
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
 std::vector< I > SA( T.size() );
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
 std::vector< I > P( n );
 detail::plcp( reinterpret_cast< const unsigned char * >( T.data() ) , n ,
               SA.data() , P.data() );
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
 detail::plcp( reinterpret_cast< const unsigned char * >( T.data() ) , n ,
               SA.data() , P.get() );
 std::vector< I > LCP( n < 2 ? 0 : n - 1 );
 if( n >= 2 )
  detail::plcpToLcp( P.get() , SA.data() + 1 , LCP.data() , n - 1 );
 ADS_AUDIT( detail::isLcpArrayOf( T , SA , LCP ) );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SuffixSorting.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
