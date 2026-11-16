/*--------------------------------------------------------------------------*/
/*----------------------------- File Hash.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The kernel of the hash of the long strings on Highway [see FastHash.h]:
 * the 8 lanes of the stripes of 64 bytes, a vector of \f$ L \f$ lanes at a
 * time, each adding its word to a sum of its own and the product of the
 * two halves of its word XOR a word of the salts, one product of 32 bits
 * per lane; the sums are added across the pairs of lanes ( 2 i , 2 i + 1 )
 * and the lanes scrambled every 16 stripes, word by word. The words are
 * those of the scalar code of FastHash.tpp, so that the hash does not
 * depend on the vectors of the machine.
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

#if defined( __ADS_simd_Hash ) == defined( HWY_TARGET_TOGGLE )
 #ifdef __ADS_simd_Hash
  #undef __ADS_simd_Hash
 #else
  #define __ADS_simd_Hash
 #endif

 #include <hwy/highway.h>

 #include <cstddef>
 #include <cstdint>

HWY_BEFORE_NAMESPACE();

namespace ADS::detail::simd::HWY_NAMESPACE
{

namespace hn = hwy::HWY_NAMESPACE;

/// the 8 lanes a of the stripes of 64 bytes of s[ 0 .. n ), n > 64, with
/// the seed and the salts k of the hash of FastHash.tpp, into out
inline void hashStripes( const unsigned char * HWY_RESTRICT s , std::size_t n ,
			 std::uint64_t seed ,
			 const std::uint64_t * HWY_RESTRICT k ,
			 std::uint64_t * HWY_RESTRICT out )
{
 const hn::CappedTag< std::uint64_t , 8 > d;
 const hn::Repartition< std::uint32_t , decltype( d ) > d32;
 const hn::Repartition< std::uint8_t , decltype( d ) > d8;
 const std::size_t L = hn::Lanes( d );
 alignas( 64 ) std::uint64_t a[ 8 ] , w[ 8 ];  // the lanes, their words
 for( int i = 0 ; i < 8 ; ++i ) {
  a[ i ] = k[ i ] ^ seed;
  w[ i ] = 0;
  }
 auto stripe = [ & ]( const unsigned char * p , std::size_t t ) HWY_ATTR {
  for( std::size_t i = 0 ; i < 8 ; i += L ) {
   const auto v = hn::BitCast( d , hn::LoadU( d8 , p + 8 * i ) );
   const auto x = hn::Xor( v , hn::LoadU( d , k + i + t ) );
   hn::Store( hn::Add( hn::Load( d , w + i ) , v ) , d , w + i );
   hn::Store(
     hn::Add( hn::Load( d , a + i ) ,
	      hn::MulEven( hn::BitCast( d32 , x ) ,
			   hn::BitCast( d32 , hn::ShiftRight< 32 >( x ) ) ) ) ,
     d , a + i );
   }
  };
 auto flush = [ & ]() {
  for( int i = 0 ; i < 8 ; ++i ) {
   a[ i ] += w[ i ^ 1 ];
   w[ i ^ 1 ] = 0;
   }
  };
 const std::size_t m = ( n - 1 ) / 64;  // the stripes, the last apart
 std::size_t j = 0;
 for( ; j + 16 <= m ; j += 16 ) {  // blocks of 1 KB
  for( std::size_t t = 0 ; t < 16 ; ++t ) {
   ADS::detail::prefetchLine( s + 64 * ( j + t ) + 384 , ADS::Locality( 3 ) );
   stripe( s + 64 * ( j + t ) , t );
   }
  flush();
  for( int i = 0 ; i < 8 ; ++i )
   a[ i ] = ( a[ i ] ^ ( a[ i ] >> 47 ) ^ k[ 16 + i ] ) * 0x9E3779B1ULL;
  }
 for( std::size_t t = 0 ; j + t < m ; ++t )
  stripe( s + 64 * ( j + t ) , t );
 stripe( s + n - 64 , 16 );  // the last 64 bytes
 flush();
 for( int i = 0 ; i < 8 ; ++i )
  out[ i ] = a[ i ];
 }

}  // end( namespace ADS::detail::simd::HWY_NAMESPACE )

HWY_AFTER_NAMESPACE();

#endif

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Hash.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
