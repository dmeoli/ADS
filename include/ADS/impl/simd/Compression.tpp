/*--------------------------------------------------------------------------*/
/*-------------------------- File Compression.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The kernels of the compression on Highway: the encoder and the decoder
 * of Stream VByte [see StreamVByte.h], 4 integers of 32 bits a step in a
 * vector of 16 bytes. The encoder finds the lengths of the 4 integers by
 * three comparisons of their lanes, makes their control byte by a sum of
 * the lengths shifted to their places, and moves their bytes together by
 * the shuffle of bytes of the control byte; the decoder moves the bytes of
 * 4 integers to their places by the shuffle of their control byte, the
 * bytes 0x80 of the tables giving 0 [see `TableLookupBytesOr0()` of
 * Highway]; and the binary packing of blocks of 128 integers of
 * \f$ B \f$ bits in 4 lanes [see BinaryPacking.h], each lane shifted and
 * or-ed into a word of 32 bits until it is full, as in SIMD-BP128 of Lemire
 * and Boytsov. The bytes of an integer are those of the little-endian
 * order, which the callers require of the machine.
 *
 * \see D. Lemire, L. Boytsov, Decoding billions of integers per second
 *      through vectorization, Software: Practice and Experience 45(1),
 *      1-29, 2015, doi:10.1002/spe.2203
 * \see D. Lemire, N. Kurz, C. Rupp, Stream VByte: faster byte-oriented
 *      integer compression, Information Processing Letters 130, 1-6, 2018,
 *      doi:10.1016/j.ipl.2017.09.011
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

#if defined( __ADS_simd_Compression ) == defined( HWY_TARGET_TOGGLE )
 #ifdef __ADS_simd_Compression
  #undef __ADS_simd_Compression
 #else
  #define __ADS_simd_Compression
 #endif

 #include <hwy/highway.h>

 #include <array>
 #include <cstddef>
 #include <cstdint>
 #include <utility>

HWY_BEFORE_NAMESPACE();

namespace ADS::detail::simd::HWY_NAMESPACE
{

namespace hn = hwy::HWY_NAMESPACE;

/// the integers x[ 0 .. i ) of Stream VByte encoded 4 at a time, i the
/// largest multiple of 4 not above n: their control bytes in keys, their
/// bytes from *d, which moves past them (16 bytes of room after them), by
/// the shuffles pack and the lengths len of the 256 control bytes; i
inline std::size_t vbyteEncode( const std::uint32_t * HWY_RESTRICT x ,
				std::size_t n ,
				std::uint8_t * HWY_RESTRICT keys ,
				std::uint8_t ** d ,
				const std::uint8_t * HWY_RESTRICT pack ,
				const std::uint8_t * HWY_RESTRICT len )
{
 const hn::FixedTag< std::uint32_t , 4 > d32;
 const hn::FixedTag< std::uint8_t , 16 > d8;
 const auto one = hn::Set( d32 , 1u );
 const auto at = hn::ShiftLeft< 1 >( hn::Iota( d32 , 0 ) );  // 0 2 4 6
 std::uint8_t * p = *d;
 std::size_t i = 0;
 for( ; i + 4 <= n ; i += 4 ) {
  const auto v = hn::LoadU( d32 , x + i );
  // the length less 1 of each, its bytes past the first that are used
  auto l =
    hn::And( hn::VecFromMask( d32 , hn::Gt( v , hn::Set( d32 , 0xFFu ) ) ) ,
	     one );
  l = hn::Add(
    l ,
    hn::And( hn::VecFromMask( d32 , hn::Gt( v , hn::Set( d32 , 0xFFFFu ) ) ) ,
		one ) );
  l = hn::Add(
    l ,
    hn::And( hn::VecFromMask( d32 ,
			      hn::Gt( v , hn::Set( d32 , 0xFFFFFFu ) ) ) ,
		one ) );
  const unsigned c =
    unsigned( hn::GetLane( hn::SumOfLanes( d32 , hn::Shl( l , at ) ) ) );
  keys[ i / 4 ] = std::uint8_t( c );
  hn::StoreU( hn::TableLookupBytesOr0( hn::BitCast( d8 , v ) ,
				       hn::LoadU( d8 , pack + 16 * c ) ) ,
	      d8 , p );
  p += len[ c ];
  }
 *d = p;
 return( i );
 }

/// the integers x[ 0 .. i ) of Stream VByte decoded from their control
/// bytes keys and their bytes from *d, which moves past them, by the
/// shuffles shuffle and the lengths len of the 256 control bytes: 32 at a
/// time while 12 more follow, whose bytes leave room for the loads of 16
/// bytes, with no test on the data, then 4 at a time while 16 bytes are
/// left before end; i
inline std::size_t vbyteDecode( const std::uint8_t * HWY_RESTRICT keys ,
				const std::uint8_t ** d ,
				const std::uint8_t * end , std::size_t n ,
				std::uint32_t * HWY_RESTRICT x ,
				const std::uint8_t * HWY_RESTRICT shuffle ,
				const std::uint8_t * HWY_RESTRICT len )
{
 const hn::FixedTag< std::uint32_t , 4 > d32;
 const hn::FixedTag< std::uint8_t , 16 > d8;
 const std::uint8_t * p = *d;
 auto step = [ & ]( std::size_t j , unsigned c ) HWY_ATTR {
  hn::StoreU( hn::BitCast( d32 , hn::TableLookupBytesOr0(
				  hn::LoadU( d8 , p ) ,
				  hn::LoadU( d8 , shuffle + 16 * c ) ) ) ,
	      d32 , x + j );
  p += len[ c ];
  };
 std::size_t i = 0;
 for( ; i + 32 + 12 <= n ; i += 32 )
  for( unsigned g = 0 ; g < 8 ; ++g )
   step( i + 4 * g , keys[ i / 4 + g ] );
 for( ; ( i + 4 <= n ) && ( end - p >= 16 ) ; i += 4 )
  step( i , keys[ i / 4 ] );
 *d = p;
 return( i );
 }

/// the 128 integers of in, below 2^B, packed into 16 B bytes at out in the
/// layout of the binary packing of 4 lanes [see BinaryPacking.h]: the
/// integer 4 q + l at the bit q B of the lane l, the word k of the lane l
/// at the place 4 k + l, in little-endian order
template< int B >
inline void bpPackB( const std::uint32_t * HWY_RESTRICT in ,
		     std::uint8_t * HWY_RESTRICT out )
{
 if constexpr( B > 0 ) {
  const hn::FixedTag< std::uint32_t , 4 > d;
  auto * o = reinterpret_cast< std::uint32_t * >( out );
  auto acc = hn::Zero( d );
  [ & ]< std::size_t... I >( std::index_sequence< I... > ) HWY_ATTR {
   (
     [ & ]() HWY_ATTR {
      constexpr int s = int( I ) * B % 32;
      const auto v = hn::LoadU( d , in + 4 * I );
      if constexpr( s == 0 )
       acc = v;
      else
       acc = hn::Or( acc , hn::ShiftLeft< s >( v ) );
      if constexpr( s + B >= 32 ) {  // the word is full
       hn::StoreU( acc , d , o );
       o += 4;
       if constexpr( s + B > 32 )
	acc = hn::ShiftRight< 32 - s >( v );
       }
      }() ,
     ... );
   }( std::make_index_sequence< 32 >() );
  }
 }

/// the 128 integers of B bits at in unpacked to out, each one plus the one
/// 4 places before it if Sorted, prev the last 4 before the block
template< int B , bool Sorted >
inline void bpUnpackB( const std::uint8_t * HWY_RESTRICT in ,
		       std::uint32_t * HWY_RESTRICT out ,
		       std::uint32_t * prev )
{
 const hn::FixedTag< std::uint32_t , 4 > d;
 const auto * w = reinterpret_cast< const std::uint32_t * >( in );
 auto p = Sorted ? hn::LoadU( d , prev ) : hn::Zero( d );
 const auto mask =
   hn::Set( d , B >= 32 ? ~std::uint32_t( 0 ) : ( 1u << ( B & 31 ) ) - 1 );
 [ & ]< std::size_t... I >( std::index_sequence< I... > ) HWY_ATTR {
  (
    [ & ]() HWY_ATTR {
     constexpr int q = int( I ) * B / 32 , s = int( I ) * B % 32;
     auto v = hn::Zero( d );
     if constexpr( B > 0 ) {
      v = hn::ShiftRight< s >( hn::LoadU( d , w + 4 * q ) );
      if constexpr( s + B > 32 )
       v = hn::Or( v , hn::ShiftLeft< ( 32 - s ) & 31 >(
			hn::LoadU( d , w + 4 * q + 4 ) ) );
      if constexpr( B < 32 )
       v = hn::And( v , mask );
      }
     if constexpr( Sorted )
      v = p = hn::Add( v , p );
     hn::StoreU( v , d , out + 4 * I );
     }() ,
    ... );
  }( std::make_index_sequence< 32 >() );
 if constexpr( Sorted )
  hn::StoreU( p , d , prev );
 }

/// bpPackB() of the width b, 0 <= b <= 32
inline void bpPack( const std::uint32_t * in , std::uint8_t * out , int b )
{
 using F = void ( * )( const std::uint32_t * , std::uint8_t * );
 static constexpr auto T =
   []< std::size_t... B >( std::index_sequence< B... > ) {
    return( std::array< F , 33 >{ &bpPackB< int( B ) >... } );
    }( std::make_index_sequence< 33 >() );
 T[ std::size_t( b ) ]( in , out );
 }

/// bpUnpackB() of the width b, 0 <= b <= 32, Sorted if sorted (prev the
/// last 4 integers before the block, which become the last 4 of it)
inline void bpUnpack( const std::uint8_t * in , std::uint32_t * out , int b ,
		      bool sorted , std::uint32_t * prev )
{
 using F =
   void ( * )( const std::uint8_t * , std::uint32_t * , std::uint32_t * );
 static constexpr auto U =
   []< std::size_t... B >( std::index_sequence< B... > ) {
    return( std::array< F , 33 >{ &bpUnpackB< int( B ) , false >... } );
    }( std::make_index_sequence< 33 >() );
 static constexpr auto S =
   []< std::size_t... B >( std::index_sequence< B... > ) {
    return( std::array< F , 33 >{ &bpUnpackB< int( B ) , true >... } );
    }( std::make_index_sequence< 33 >() );
 ( sorted ? S : U )[ std::size_t( b ) ]( in , out , prev );
 }

}  // end( namespace ADS::detail::simd::HWY_NAMESPACE )

HWY_AFTER_NAMESPACE();

#endif

/*--------------------------------------------------------------------------*/
/*------------------------ End File Compression.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
