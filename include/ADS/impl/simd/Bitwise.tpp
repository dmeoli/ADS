/*--------------------------------------------------------------------------*/
/*---------------------------- File Bitwise.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The kernels of the 1 bits of arrays of words [see FastBitwise.h], on
 * Highway: the carry-save adders of Harley and Seal on vectors of words,
 * so that sixteen vectors cost one count of the lanes and fifteen adders
 * of three logical operations each, and the counts of the lanes of
 * Highway (one instruction where the processor has it, as AVX-512
 * VPOPCNTDQ and NEON, a table of nibbles elsewhere).
 *
 * \see W. Muła, N. Kurz, D. Lemire, Faster population counts using AVX2
 *      instructions, The Computer Journal 61(1), 111-120, 2018,
 *      doi:10.1093/comjnl/bxx046
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

#if defined( __ADS_simd_Bitwise ) == defined( HWY_TARGET_TOGGLE )
 #ifdef __ADS_simd_Bitwise
  #undef __ADS_simd_Bitwise
 #else
  #define __ADS_simd_Bitwise
 #endif

 #include <hwy/highway.h>

 #include <bit>
 #include <cstddef>
 #include <cstdint>

HWY_BEFORE_NAMESPACE();

namespace ADS::detail::simd::HWY_NAMESPACE
{

namespace hn = hwy::HWY_NAMESPACE;

/// the carry-save adder of three vectors of bits: h the carries (the
/// majority), l the sums
template< class V >
HWY_INLINE void csa( V & h , V & l , V a , V b , V c )
{
 const V u = hn::Xor( a , b );
 h = hn::Or( hn::And( a , b ) , hn::And( u , c ) );
 l = hn::Xor( u , c );
 }

/// the 1 bits of the vectors load( 0 ) , ... , load( m - 1 ) of the tag
/// d, by the adders of Harley and Seal on blocks of sixteen (a lambda of
/// a kernel carries HWY_ATTR, the instructions of its set, for Clang)
template< class D , class L >
HWY_INLINE std::uint64_t harleySeal( D d , L && load , std::size_t m )
{
 using V = decltype( hn::Zero( d ) );
 V total = hn::Zero( d ) , ones = total , twos = total , fours = total ,
   eights = total , sixteens , twosA , twosB , foursA , foursB , eightsA ,
   eightsB;
 std::size_t i = 0;
 for( ; i + 16 <= m ; i += 16 ) {
  csa( twosA , ones , ones , load( i ) , load( i + 1 ) );
  csa( twosB , ones , ones , load( i + 2 ) , load( i + 3 ) );
  csa( foursA , twos , twos , twosA , twosB );
  csa( twosA , ones , ones , load( i + 4 ) , load( i + 5 ) );
  csa( twosB , ones , ones , load( i + 6 ) , load( i + 7 ) );
  csa( foursB , twos , twos , twosA , twosB );
  csa( eightsA , fours , fours , foursA , foursB );
  csa( twosA , ones , ones , load( i + 8 ) , load( i + 9 ) );
  csa( twosB , ones , ones , load( i + 10 ) , load( i + 11 ) );
  csa( foursA , twos , twos , twosA , twosB );
  csa( twosA , ones , ones , load( i + 12 ) , load( i + 13 ) );
  csa( twosB , ones , ones , load( i + 14 ) , load( i + 15 ) );
  csa( foursB , twos , twos , twosA , twosB );
  csa( eightsB , fours , fours , foursA , foursB );
  csa( sixteens , eights , eights , eightsA , eightsB );
  total = hn::Add( total , hn::PopulationCount( sixteens ) );
  }
 total = hn::ShiftLeft< 4 >( total );
 total = hn::Add( total ,
		  hn::ShiftLeft< 3 >( hn::PopulationCount( eights ) ) );
 total = hn::Add( total , hn::ShiftLeft< 2 >( hn::PopulationCount( fours ) ) );
 total = hn::Add( total , hn::ShiftLeft< 1 >( hn::PopulationCount( twos ) ) );
 total = hn::Add( total , hn::PopulationCount( ones ) );
 for( ; i < m ; ++i )
  total = hn::Add( total , hn::PopulationCount( load( i ) ) );
 return( hn::GetLane( hn::SumOfLanes( d , total ) ) );
 }

/// the 1 bits of w[ 0 .. n - 1 ]
inline std::uint64_t popcountWords( const std::uint64_t * HWY_RESTRICT w ,
				    std::size_t n )
{
 const hn::ScalableTag< std::uint64_t > d;
 const std::size_t L = hn::Lanes( d ) , m = n / L;
 std::uint64_t s = harleySeal(
   d ,
   [ & ]( std::size_t i ) HWY_ATTR { return( hn::LoadU( d , w + L * i ) ); } ,
   m );
 for( std::size_t i = m * L ; i < n ; ++i )
  s += std::uint64_t( std::popcount( w[ i ] ) );
 return( s );
 }

/// the 1 bits of a[ i ] & b[ i ], 0 <= i < n
inline std::uint64_t popcountAndWords( const std::uint64_t * HWY_RESTRICT a ,
				       const std::uint64_t * HWY_RESTRICT b ,
				       std::size_t n )
{
 const hn::ScalableTag< std::uint64_t > d;
 const std::size_t L = hn::Lanes( d ) , m = n / L;
 std::uint64_t s = harleySeal(
   d ,
   [ & ]( std::size_t i ) HWY_ATTR {
    return( hn::And( hn::LoadU( d , a + L * i ) ,
		     hn::LoadU( d , b + L * i ) ) );
    } ,
   m );
 for( std::size_t i = m * L ; i < n ; ++i )
  s += std::uint64_t( std::popcount( a[ i ] & b[ i ] ) );
 return( s );
 }

}  // end( namespace ADS::detail::simd::HWY_NAMESPACE )

HWY_AFTER_NAMESPACE();

#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Bitwise.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
