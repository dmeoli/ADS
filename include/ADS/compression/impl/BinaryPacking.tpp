/*--------------------------------------------------------------------------*/
/*------------------------- File BinaryPacking.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BinaryPacking.h, with their contracts.
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

namespace ADS {

namespace detail::bp {

constexpr std::size_t kBlock = 128;         ///< the integers of a block
constexpr std::size_t kGroup = 16;          ///< the blocks of a group

/// x with its bytes turned around on a big-endian machine
constexpr std::uint64_t littleEndian64( std::uint64_t x )
{
 if constexpr( std::endian::native == std::endian::big )
  return( ( ( x & 0xFFULL ) << 56 ) | ( ( x & 0xFF00ULL ) << 40 ) |
          ( ( x & 0xFF0000ULL ) << 24 ) | ( ( x & 0xFF000000ULL ) << 8 ) |
          ( ( x >> 8 ) & 0xFF000000ULL ) | ( ( x >> 24 ) & 0xFF0000ULL ) |
          ( ( x >> 40 ) & 0xFF00ULL ) | ( x >> 56 ) );
 return( x );
 }

#ifdef ADS_BP_SSE

/// the 128 integers of in, below 2^B, packed into 16 B bytes at out
template< int B >
inline void pack( const std::uint32_t * in , std::uint8_t * out )
{
 if constexpr( B == 0 ) return;
 else {
  auto * o = reinterpret_cast< __m128i * >( out );
  __m128i acc = _mm_setzero_si128();
  [ & ]< std::size_t... I >( std::index_sequence< I... > ) {
   ( [ & ]() {
     constexpr int s = int( I ) * B % 32;
     __m128i v = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                   in + 4 * I ) );
     if constexpr( s == 0 ) acc = v;
     else acc = _mm_or_si128( acc , _mm_slli_epi32( v , s ) );
     if constexpr( s + B >= 32 ) {         // the word is full
      _mm_storeu_si128( o++ , acc );
      if constexpr( s + B > 32 ) acc = _mm_srli_epi32( v , 32 - s );
      }
     }() , ... );
   }( std::make_index_sequence< 32 >() );
  }
 }

/// the last 4 integers before a block
struct Lanes {
 __m128i v;
 };

/// the 128 integers of B bits at in unpacked to out, each one plus the
/// one 4 places before it if Sorted (p the last 4 before the block)
template< int B , bool Sorted >
inline void unpack( const std::uint8_t * in , std::uint32_t * out ,
                    Lanes & p )
{
 __m128i prev = p.v;
 const auto * w = reinterpret_cast< const __m128i * >( in );
 const __m128i mask = _mm_set1_epi32( B >= 32 ? -1 : int( ( 1u << B ) - 1 ) );
 [ & ]< std::size_t... I >( std::index_sequence< I... > ) {
  ( [ & ]() {
    constexpr int q = int( I ) * B / 32 , s = int( I ) * B % 32;
    __m128i v;
    if constexpr( B == 0 ) v = _mm_setzero_si128();
    else {
     v = _mm_srli_epi32( _mm_loadu_si128( w + q ) , s );
     if constexpr( s + B > 32 )
      v = _mm_or_si128( v , _mm_slli_epi32( _mm_loadu_si128( w + q + 1 ) ,
                                            32 - s ) );
     if constexpr( B < 32 ) v = _mm_and_si128( v , mask );
     }
    if constexpr( Sorted ) v = prev = _mm_add_epi32( v , prev );
    _mm_storeu_si128( reinterpret_cast< __m128i * >( out + 4 * I ) , v );
    }() , ... );
  }( std::make_index_sequence< 32 >() );
 p.v = prev;
 }

using Packer = void ( * )( const std::uint32_t * , std::uint8_t * );
using Unpacker = void ( * )( const std::uint8_t * , std::uint32_t * ,
                             Lanes & );

/// the packers and the unpackers of the widths 0 to 32
inline const std::array< Packer , 33 > & packers( void )
{
 static const auto T = []< std::size_t... B >( std::index_sequence< B... > ) {
  return( std::array< Packer , 33 >{ &pack< int( B ) >... } );
  }( std::make_index_sequence< 33 >() );
 return( T );
 }

template< bool Sorted >
inline const std::array< Unpacker , 33 > & unpackers( void )
{
 static const auto T = []< std::size_t... B >( std::index_sequence< B... > ) {
  return( std::array< Unpacker , 33 >{ &unpack< int( B ) , Sorted >... } );
  }( std::make_index_sequence< 33 >() );
 return( T );
 }

#else

/// the same layout a lane at a time: the integer 4 q + l at the bit q b of
/// the lane l, the word k of the lane l at the place 4 k + l
inline void pack( const std::uint32_t * in , std::uint8_t * out , int b )
{
 std::uint32_t w[ 4 * 32 ] = {};
 for( std::size_t i = 0 ; i < kBlock ; ++i ) {
  const std::size_t l = i % 4 , q = i / 4 , bit = q * std::size_t( b );
  const std::uint64_t v = std::uint64_t( in[ i ] ) << ( bit % 32 );
  w[ 4 * ( bit / 32 ) + l ] |= std::uint32_t( v );
  if( bit % 32 + std::size_t( b ) > 32 )
   w[ 4 * ( bit / 32 + 1 ) + l ] |= std::uint32_t( v >> 32 );
  }
 for( std::size_t k = 0 ; k < 4 * std::size_t( b ) ; ++k )
  for( int c = 0 ; c < 4 ; ++c )
   out[ 4 * k + std::size_t( c ) ] = std::uint8_t( w[ k ] >> ( 8 * c ) );
 }

inline void unpack( const std::uint8_t * in , std::uint32_t * out , int b ,
                    bool sorted , std::uint32_t * prev )
{
 auto word = [ & ]( std::size_t k ) {
  std::uint32_t v = 0;
  for( int c = 0 ; c < 4 ; ++c )
   v |= std::uint32_t( in[ 4 * k + std::size_t( c ) ] ) << ( 8 * c );
  return( v );
  };
 const std::uint64_t mask = ( std::uint64_t( 1 ) << b ) - 1;
 for( std::size_t i = 0 ; i < kBlock ; ++i ) {
  const std::size_t l = i % 4 , q = i / 4 , bit = q * std::size_t( b );
  std::uint64_t v = 0;
  if( b > 0 ) {
   v = std::uint64_t( word( 4 * ( bit / 32 ) + l ) ) >> ( bit % 32 );
   if( bit % 32 + std::size_t( b ) > 32 )
    v |= std::uint64_t( word( 4 * ( bit / 32 + 1 ) + l ) ) <<
         ( 32 - bit % 32 );
   }
  std::uint32_t x = std::uint32_t( v & mask );
  if( sorted ) x = prev[ l ] = x + prev[ l ];
  out[ i ] = x;
  }
 }

#endif

}  // end( namespace detail::bp )

inline std::vector< std::uint8_t > binaryPack(
                         std::span< const std::uint32_t > x , bool sorted )
{
 using namespace detail::bp;
 ADS_AUDIT( ! sorted || std::is_sorted( x.begin() , x.end() ) );
 const std::size_t n = x.size() , blocks = ( n + kBlock - 1 ) / kBlock;
 // the blocks, the differences taken and the last one filled; their
 // widths first, then the bytes
 std::vector< std::uint32_t > v( blocks * kBlock , 0 );
 if( ! sorted ) std::copy( x.begin() , x.end() , v.begin() );
 else
  for( std::size_t i = 0 ; i < n ; ++i )
   v[ i ] = x[ i ] - ( i >= 4 ? x[ i - 4 ] : 0 );
 std::vector< std::uint8_t > width( blocks );
 std::size_t bytes = 0;
 for( std::size_t k = 0 ; k < blocks ; ++k ) {
  std::uint32_t o = 0;
  for( std::size_t i = 0 ; i < kBlock ; ++i ) o |= v[ k * kBlock + i ];
  width[ k ] = std::uint8_t( std::bit_width( o ) );
  bytes += 16 * std::size_t( width[ k ] );
  }
 std::vector< std::uint8_t > out( bytes + kGroup * ( ( blocks + kGroup - 1 ) /
                                                      kGroup ) , 0 );
 std::uint8_t * o = out.data();
 for( std::size_t k = 0 ; k < blocks ; ++k ) {
  if( k % kGroup == 0 ) {
   const std::size_t g = std::min( kGroup , blocks - k );
   std::copy( width.begin() + std::ptrdiff_t( k ) ,
              width.begin() + std::ptrdiff_t( k + g ) , o );
   o += kGroup;
   }
#ifdef ADS_BP_SSE
  packers()[ width[ k ] ]( v.data() + k * kBlock , o );
#else
  pack( v.data() + k * kBlock , o , width[ k ] );
#endif
  o += 16 * std::size_t( width[ k ] );
  }
 ADS_AUDIT( binaryUnpack( out , n , sorted ) ==
            std::vector< std::uint32_t >( x.begin() , x.end() ) );
 return( out );
 }

inline std::vector< std::uint32_t > binaryUnpack(
                         std::span< const std::uint8_t > in , std::size_t n ,
                         bool sorted )
{
 using namespace detail::bp;
 const std::size_t blocks = ( n + kBlock - 1 ) / kBlock;
 std::vector< std::uint32_t > out( blocks * kBlock );
#ifdef ADS_BP_SSE
 Lanes prev{ _mm_setzero_si128() };
 const auto & U = sorted ? unpackers< true >() : unpackers< false >();
#else
 std::uint32_t prev[ 4 ] = { 0 , 0 , 0 , 0 };
#endif
 std::size_t at = 0;
 const std::uint8_t * width = nullptr;
 for( std::size_t k = 0 ; k < blocks ; ++k ) {
  if( k % kGroup == 0 ) {
   ADS_PRE( at + kGroup <= in.size() );
   width = in.data() + at;
   at += kGroup;
   }
  const int b = width[ k % kGroup ];
  ADS_PRE( ( b <= 32 ) && ( at + 16 * std::size_t( b ) <= in.size() ) );
#ifdef ADS_BP_SSE
  U[ std::size_t( b ) ]( in.data() + at , out.data() + k * kBlock , prev );
#else
  unpack( in.data() + at , out.data() + k * kBlock , b , sorted , prev );
#endif
  at += 16 * std::size_t( b );
  }
 out.resize( n );
 return( out );
 }

inline std::vector< std::uint8_t > patchedPack(
                         std::span< const std::uint32_t > x , bool sorted )
{
 using namespace detail::bp;
 ADS_AUDIT( ! sorted || std::is_sorted( x.begin() , x.end() ) );
 const std::size_t n = x.size() , blocks = ( n + kBlock - 1 ) / kBlock;
 // at most 3 + 512 + 128 + 512 bytes a block, and 8 bytes of 0 at the end
 std::vector< std::uint8_t > out( blocks * 1155 + 8 );
 std::uint8_t * o = out.data();
 alignas( 16 ) std::uint32_t v[ kBlock ] , low[ kBlock ];
 for( std::size_t k = 0 ; k < blocks ; ++k ) {
  // the block, its counts of the widths and the width of least cost
  std::size_t cnt[ 33 ] = {};
  const std::size_t j0 = k * kBlock , m0 = std::min( kBlock , n - j0 );
  if( ( m0 == kBlock ) && ( ! sorted || ( j0 >= 4 ) ) ) {
   for( std::size_t i = 0 ; i < kBlock ; ++i )
    v[ i ] = sorted ? x[ j0 + i ] - x[ j0 + i - 4 ] : x[ j0 + i ];
   }
  else
   for( std::size_t i = 0 ; i < kBlock ; ++i ) {
    const std::size_t j = j0 + i;
    v[ i ] = i >= m0 ? 0 : sorted ? x[ j ] - ( j >= 4 ? x[ j - 4 ] : 0 )
                                  : x[ j ];
    }
  for( std::size_t i = 0 ; i < kBlock ; ++i )
   ++cnt[ std::bit_width( v[ i ] ) ];
  int beta = 32;
  while( ( beta > 0 ) && ( cnt[ beta ] == 0 ) ) --beta;
  int b = beta;
  std::size_t best = kBlock * std::size_t( beta ) , c = 0 , cb = 0;
  for( int t = beta - 1 ; t >= 0 ; --t ) {
   c += cnt[ t + 1 ];                       // the integers of 2^t or more
   const std::size_t cost = kBlock * std::size_t( t ) +
                            c * ( 8 + std::size_t( beta - t ) ) + 8;
   if( cost < best ) {
    best = cost;
    b = t;
    cb = c;
    }
   }
  *o++ = std::uint8_t( b );
  *o++ = std::uint8_t( cb );
  if( cb > 0 ) *o++ = std::uint8_t( beta );
  const std::uint32_t m = b >= 32 ? ~0u : ( 1u << b ) - 1;
  if( cb > 0 )                              // the low bits of each
   for( std::size_t i = 0 ; i < kBlock ; ++i ) low[ i ] = v[ i ] & m;
#ifdef ADS_BP_SSE
  packers()[ std::size_t( b ) ]( cb > 0 ? low : v , o );
#else
  pack( cb > 0 ? low : v , o , b );
#endif
  o += 16 * std::size_t( b );
  if( cb == 0 ) continue;
  // the places of the exceptions, then their high bits packed
  std::uint8_t * pos = o;
  o += cb;
  std::uint64_t acc = 0;
  int fill = 0;
  for( std::size_t i = 0 ; i < kBlock ; ++i )
   if( v[ i ] > m ) {
    *pos++ = std::uint8_t( i );
    acc |= std::uint64_t( v[ i ] >> b ) << fill;
    for( fill += beta - b ; fill >= 8 ; fill -= 8 , acc >>= 8 )
     *o++ = std::uint8_t( acc );
    }
  if( fill > 0 ) *o++ = std::uint8_t( acc );
  }
 std::fill( o , o + 8 , std::uint8_t( 0 ) );
 out.resize( std::size_t( o - out.data() ) + 8 );
 ADS_AUDIT( patchedUnpack( out , n , sorted ) ==
            std::vector< std::uint32_t >( x.begin() , x.end() ) );
 return( out );
 }

inline std::vector< std::uint32_t > patchedUnpack(
                         std::span< const std::uint8_t > in , std::size_t n ,
                         bool sorted )
{
 using namespace detail::bp;
 const std::size_t blocks = ( n + kBlock - 1 ) / kBlock;
 std::vector< std::uint32_t > out( blocks * kBlock );
 ADS_PRE( in.size() >= 8 );
 const std::size_t end = in.size() - 8;      // the 8 bytes of 0 after
#ifdef ADS_BP_SSE
 const auto & U = unpackers< false >();
#endif
 std::size_t at = 0;
 for( std::size_t k = 0 ; k < blocks ; ++k ) {
  ADS_PRE( at + 2 <= end );
  const int b = in[ at ] , c = in[ at + 1 ];
  at += 2;
  int beta = b;
  if( c > 0 ) {
   ADS_PRE( at < end );
   beta = in[ at++ ];
   }
  const std::size_t hb = ( std::size_t( c ) * std::size_t( beta - b ) + 7 ) /
                         8;
  ADS_PRE( ( b <= beta ) && ( beta <= 32 ) && ( c <= int( kBlock ) ) &&
           ( at + 16 * std::size_t( b ) + std::size_t( c ) + hb <= end ) );
  std::uint32_t * o = out.data() + k * kBlock;
#ifdef ADS_BP_SSE
  Lanes none{ _mm_setzero_si128() };
  U[ std::size_t( b ) ]( in.data() + at , o , none );
#else
  unpack( in.data() + at , o , b , false , nullptr );
#endif
  at += 16 * std::size_t( b );
  // the high bits of each exception from a load of 8 bytes, which the 8
  // bytes of 0 at the end keep within the code
  const std::uint8_t * pos = in.data() + at;
  const std::uint8_t * high = pos + c;
  const int w = beta - b;
  const std::uint64_t mask = ( std::uint64_t( 1 ) << w ) - 1;
  for( int e = 0 ; e < c ; ++e ) {
   const std::size_t bit = std::size_t( e ) * std::size_t( w );
   std::uint64_t h;
   std::memcpy( &h , high + bit / 8 , 8 );
   o[ pos[ e ] ] |= std::uint32_t( ( ( littleEndian64( h ) >> ( bit % 8 ) ) &
                                     mask ) << b );
   }
  at += std::size_t( c ) + hb;
  }
 // the sums at a distance of 4 in a pass of their own, once the
 // exceptions are set: 4 integers an addition
 if( sorted && ( n > 4 ) ) {
  std::size_t i = 4;
#ifdef ADS_BP_SSE
  __m128i prev = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                   out.data() ) );
  for( ; i + 4 <= out.size() ; i += 4 ) {
   auto * a = reinterpret_cast< __m128i * >( out.data() + i );
   prev = _mm_add_epi32( _mm_loadu_si128( a ) , prev );
   _mm_storeu_si128( a , prev );
   }
#endif
  for( ; i < out.size() ; ++i ) out[ i ] += out[ i - 4 ];
  }
 out.resize( n );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BinaryPacking.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
