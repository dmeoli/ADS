/*--------------------------------------------------------------------------*/
/*--------------------------- File CountLess.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The count of the items of a short array smaller than a key, the search in
 * a node of the B-trees in main memory [see BTreeMap.h and StaticSearch.h]:
 * on a sorted array of N items it is the place of the key, found with no
 * branch. countLess< N >( a , x ) and countLess< N , true >( a , x ), the
 * items not larger than x, read all the N items, which must be there (the
 * unused places of a node are padded with the largest value).
 * greaterBits< N >( a , x ) gives the items larger than x as the bits of a
 * word, N <= 64 [see WordRangeMinimum.h].
 *
 * On the integers of 32 and 64 bits and on the floating types they are a
 * few comparisons of vectors, each giving a mask of bits that a count of
 * the bits sums: of AVX-512 (16 items of 32 bits at once) if the target
 * has it, else of AVX2 (8 items), the unsigned integers being compared as
 * signed ones after flipping their top bit; elsewhere, and on the other
 * types, the loop of the comparisons, which the compiler may vectorize or
 * not (GCC 13 does not: it chains the carries of the comparisons, one item
 * per cycle and a dependence from each to the next).
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_CountLess
 #define __ADS_CountLess
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#if defined( __AVX2__ ) || defined( __AVX512F__ )
 #include <immintrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::detail {

/// the items of a[ 0 .. N ) smaller than x (not larger, if Up)
template< std::size_t N , bool Up = false , class T >
[[nodiscard]] inline std::size_t countLess( const T * a , const T & x )
{
 constexpr bool I32 = std::is_integral_v< T > && ( sizeof( T ) == 4 );
 constexpr bool I64 = std::is_integral_v< T > && ( sizeof( T ) == 8 );
 constexpr bool F32 = std::is_same_v< T , float >;
 constexpr bool F64 = std::is_same_v< T , double >;
 constexpr bool S = std::is_signed_v< T >;
#if defined( __AVX512F__ )
 constexpr std::size_t W = 64 / sizeof( T );    // the items of a vector
 if constexpr( ( I32 || I64 || F32 || F64 ) && ( N % W == 0 ) ) {
  std::size_t r = 0;
  for( std::size_t i = 0 ; i < N ; i += W ) {
   std::uint32_t m;
   constexpr int P = Up ? _MM_CMPINT_LE : _MM_CMPINT_LT;
   if constexpr( I32 && S )
    m = _mm512_cmp_epi32_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi32( std::int32_t( x ) ) , P );
   else if constexpr( I32 )
    m = _mm512_cmp_epu32_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi32( std::int32_t( x ) ) , P );
   else if constexpr( I64 && S )
    m = _mm512_cmp_epi64_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi64( std::int64_t( x ) ) , P );
   else if constexpr( I64 )
    m = _mm512_cmp_epu64_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi64( std::int64_t( x ) ) , P );
   else if constexpr( F32 )
    m = _mm512_cmp_ps_mask( _mm512_loadu_ps( a + i ) , _mm512_set1_ps( x ) ,
                            Up ? _CMP_LE_OQ : _CMP_LT_OQ );
   else
    m = _mm512_cmp_pd_mask( _mm512_loadu_pd( a + i ) , _mm512_set1_pd( x ) ,
                            Up ? _CMP_LE_OQ : _CMP_LT_OQ );
   r += std::size_t( std::popcount( m ) );
   }
  return( r );
  }
#elif defined( __AVX2__ )
 constexpr std::size_t W = 32 / sizeof( T );
 if constexpr( ( I32 || I64 || F32 || F64 ) && ( N % W == 0 ) ) {
  std::size_t r = 0;
  for( std::size_t i = 0 ; i < N ; i += W ) {
   int m;
   if constexpr( I32 || I64 ) {
    __m256i v = _mm256_loadu_si256( reinterpret_cast< const __m256i * >(
                                     a + i ) );
    __m256i k = I32 ? _mm256_set1_epi32( std::int32_t( x ) ) :
                      _mm256_set1_epi64x( std::int64_t( x ) );
    if constexpr( ! S ) {                   // unsigned as signed
     const __m256i b = I32 ? _mm256_set1_epi32( INT32_MIN ) :
                             _mm256_set1_epi64x( INT64_MIN );
     v = _mm256_xor_si256( v , b );
     k = _mm256_xor_si256( k , b );
     }
    // a < x is x > a, a <= x is not a > x
    const __m256i c = I32 ? ( Up ? _mm256_cmpgt_epi32( v , k ) :
                                   _mm256_cmpgt_epi32( k , v ) ) :
                            ( Up ? _mm256_cmpgt_epi64( v , k ) :
                                   _mm256_cmpgt_epi64( k , v ) );
    m = I32 ? _mm256_movemask_ps( _mm256_castsi256_ps( c ) ) :
              _mm256_movemask_pd( _mm256_castsi256_pd( c ) );
    if constexpr( Up )
     m ^= ( 1 << W ) - 1;
    }
   else if constexpr( F32 )
    m = _mm256_movemask_ps( _mm256_cmp_ps( _mm256_loadu_ps( a + i ) ,
                                           _mm256_set1_ps( x ) ,
                                           Up ? _CMP_LE_OQ : _CMP_LT_OQ ) );
   else
    m = _mm256_movemask_pd( _mm256_cmp_pd( _mm256_loadu_pd( a + i ) ,
                                           _mm256_set1_pd( x ) ,
                                           Up ? _CMP_LE_OQ : _CMP_LT_OQ ) );
   r += std::size_t( std::popcount( unsigned( m ) ) );
   }
  return( r );
  }
#endif
 ( void ) I32;
 ( void ) I64;
 ( void ) F32;
 ( void ) F64;
 ( void ) S;
 std::size_t r = 0;
 for( std::size_t i = 0 ; i < N ; ++i )
  if constexpr( Up )
   r += !( x < a[ i ] );
  else
   r += a[ i ] < x;
 return( r );
 }

/// the bits of the items of a[ 0 .. N ) larger than x, N <= 64: the bit q
/// is set if a[ q ] > x
template< std::size_t N , class T >
[[nodiscard]] inline std::uint64_t greaterBits( const T * a , const T & x )
{
 static_assert( N <= 64 , "the bits of a word" );
 constexpr bool I32 = std::is_integral_v< T > && ( sizeof( T ) == 4 );
 constexpr bool I64 = std::is_integral_v< T > && ( sizeof( T ) == 8 );
 constexpr bool F32 = std::is_same_v< T , float >;
 constexpr bool F64 = std::is_same_v< T , double >;
 constexpr bool S = std::is_signed_v< T >;
#if defined( __AVX512F__ )
 constexpr std::size_t W = 64 / sizeof( T );
 if constexpr( ( I32 || I64 || F32 || F64 ) && ( N % W == 0 ) ) {
  std::uint64_t r = 0;
  for( std::size_t i = 0 ; i < N ; i += W ) {
   std::uint64_t m;
   if constexpr( I32 && S )
    m = _mm512_cmp_epi32_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi32( std::int32_t( x ) ) ,
                               _MM_CMPINT_NLE );
   else if constexpr( I32 )
    m = _mm512_cmp_epu32_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi32( std::int32_t( x ) ) ,
                               _MM_CMPINT_NLE );
   else if constexpr( I64 && S )
    m = _mm512_cmp_epi64_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi64( std::int64_t( x ) ) ,
                               _MM_CMPINT_NLE );
   else if constexpr( I64 )
    m = _mm512_cmp_epu64_mask( _mm512_loadu_si512( a + i ) ,
                               _mm512_set1_epi64( std::int64_t( x ) ) ,
                               _MM_CMPINT_NLE );
   else if constexpr( F32 )
    m = _mm512_cmp_ps_mask( _mm512_loadu_ps( a + i ) , _mm512_set1_ps( x ) ,
                            _CMP_GT_OQ );
   else
    m = _mm512_cmp_pd_mask( _mm512_loadu_pd( a + i ) , _mm512_set1_pd( x ) ,
                            _CMP_GT_OQ );
   r |= m << i;
   }
  return( r );
  }
#elif defined( __AVX2__ )
 constexpr std::size_t W = 32 / sizeof( T );
 if constexpr( ( I32 || I64 || F32 || F64 ) && ( N % W == 0 ) ) {
  std::uint64_t r = 0;
  for( std::size_t i = 0 ; i < N ; i += W ) {
   int m;
   if constexpr( I32 || I64 ) {
    __m256i v = _mm256_loadu_si256( reinterpret_cast< const __m256i * >(
                                     a + i ) );
    __m256i k = I32 ? _mm256_set1_epi32( std::int32_t( x ) ) :
                      _mm256_set1_epi64x( std::int64_t( x ) );
    if constexpr( ! S ) {                   // unsigned as signed
     const __m256i b = I32 ? _mm256_set1_epi32( INT32_MIN ) :
                             _mm256_set1_epi64x( INT64_MIN );
     v = _mm256_xor_si256( v , b );
     k = _mm256_xor_si256( k , b );
     }
    const __m256i c = I32 ? _mm256_cmpgt_epi32( v , k ) :
                            _mm256_cmpgt_epi64( v , k );
    m = I32 ? _mm256_movemask_ps( _mm256_castsi256_ps( c ) ) :
              _mm256_movemask_pd( _mm256_castsi256_pd( c ) );
    }
   else if constexpr( F32 )
    m = _mm256_movemask_ps( _mm256_cmp_ps( _mm256_loadu_ps( a + i ) ,
                                           _mm256_set1_ps( x ) ,
                                           _CMP_GT_OQ ) );
   else
    m = _mm256_movemask_pd( _mm256_cmp_pd( _mm256_loadu_pd( a + i ) ,
                                           _mm256_set1_pd( x ) ,
                                           _CMP_GT_OQ ) );
   r |= std::uint64_t( unsigned( m ) ) << i;
   }
  return( r );
  }
#endif
 ( void ) I32;
 ( void ) I64;
 ( void ) F32;
 ( void ) F64;
 ( void ) S;
 std::uint64_t r = 0;
 for( std::size_t i = 0 ; i < N ; ++i )
  r |= std::uint64_t( x < a[ i ] ) << i;
 return( r );
 }

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/

#endif  /* CountLess.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CountLess.h --------------------------*/
/*--------------------------------------------------------------------------*/
