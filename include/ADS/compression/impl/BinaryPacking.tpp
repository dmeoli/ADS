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

namespace ADS
{

namespace detail::bp
{

inline constexpr std::size_t kBlock = 128;  ///< the integers of a block
inline constexpr std::size_t kGroup = 16;   ///< the blocks of a group

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

/// the 128 integers of in, below 2^b, packed into 16 b bytes at out, a lane
/// at a time: the integer 4 q + l at the bit q b of the lane l, the word k
/// of the lane l at the place 4 k + l
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
    v |= std::uint64_t( word( 4 * ( bit / 32 + 1 ) + l ) )
	 << ( 32 - bit % 32 );
   }
  std::uint32_t x = std::uint32_t( v & mask );
  if( sorted )
   x = prev[ l ] = x + prev[ l ];
  out[ i ] = x;
  }
 }

/// pack() by the kernel of Highway where there is one [see Simd.h]
inline void packBlock( const std::uint32_t * in , std::uint8_t * out , int b )
{
#ifdef ADS_BP_SIMD
 if constexpr( std::endian::native == std::endian::little )
  return( simd::bpPack( in , out , b ) );
#endif
 pack( in , out , b );
 }

/// unpack() by the kernel of Highway where there is one [see Simd.h]
inline void unpackBlock( const std::uint8_t * in , std::uint32_t * out ,
			 int b ,
			 bool sorted , std::uint32_t * prev )
{
#ifdef ADS_BP_SIMD
 if constexpr( std::endian::native == std::endian::little )
  return( simd::bpUnpack( in , out , b , sorted , prev ) );
#endif
 unpack( in , out , b , sorted , prev );
 }

}  // end( namespace detail::bp )

inline std::vector< std::uint8_t >
binaryPack( std::span< const std::uint32_t > x , bool sorted )
{
 using namespace detail::bp;
 ADS_AUDIT( ! sorted || std::is_sorted( x.begin() , x.end() ) );
 const std::size_t n = x.size() , blocks = ( n + kBlock - 1 ) / kBlock;
 const std::size_t full = n / kBlock;
 const std::size_t groups = ( blocks + kGroup - 1 ) / kGroup;
 const std::uint32_t * const p = x.data();
 // the block k into s, the differences taken and the last one filled
 // with 0, and the OR of its integers
 alignas( 16 ) std::uint32_t s[ kBlock ];
 auto fill = [ & ]( std::size_t k ) {
  const std::size_t j0 = k * kBlock , m = std::min( kBlock , n - j0 );
  std::uint32_t r = 0;
  if( sorted && ( j0 >= 4 ) )
   for( std::size_t i = 0 ; i < m ; ++i )
    r |= s[ i ] = p[ j0 + i ] - p[ j0 + i - 4 ];
  else
   for( std::size_t i = 0 ; i < m ; ++i )
    r |= s[ i ] = sorted ? p[ j0 + i ] - ( j0 + i >= 4 ? p[ j0 + i - 4 ] : 0 )
			 : p[ j0 + i ];
  std::fill( s + m , s + kBlock , 0u );
  return( r );
  };
 // the head of the group of the block k at o, with the widths w
 auto head = [ & ]( std::size_t k , std::uint8_t * o ,
		    const std::uint8_t * w ) {
  const std::size_t g = std::min( kGroup , blocks - k );
  std::copy( w , w + g , o );
  std::fill( o + g , o + kGroup , std::uint8_t( 0 ) );
  };
 std::vector< std::uint8_t > out;
 if( ! sorted ) {
  // the widths first, the full blocks read from x, and then the output
  // of its exact size, the full blocks packed from x
  std::vector< std::uint8_t > width( blocks );
  std::size_t bytes = 0;
  for( std::size_t k = 0 ; k < blocks ; ++k ) {
   std::uint32_t r = 0;
   if( k < full )
    for( std::size_t i = 0 ; i < kBlock ; ++i )
     r |= p[ k * kBlock + i ];
   else
    r = fill( k );
   width[ k ] = std::uint8_t( std::bit_width( r ) );
   bytes += 16 * std::size_t( width[ k ] );
   }
  out.resize( bytes + kGroup * groups );
  std::uint8_t * o = out.data();
  for( std::size_t k = 0 ; k < blocks ; ++k ) {
   if( k % kGroup == 0 ) {
    head( k , o , width.data() + k );
    o += kGroup;
    }
   packBlock( k < full ? p + k * kBlock : s , o , width[ k ] );
   o += 16 * std::size_t( width[ k ] );
   }
  }
 else {
  // the differences of a block once, packed at once into the room of the
  // widest blocks, not cleared, of which the output keeps the bytes
  // written
  const auto room = std::make_unique_for_overwrite< std::uint8_t[] >(
    16 * 32 * blocks + kGroup * groups );
  std::uint8_t * o = room.get();
  std::uint8_t * w = o;
  for( std::size_t k = 0 ; k < blocks ; ++k ) {
   if( k % kGroup == 0 ) {
    w = o;
    std::fill( o , o + kGroup , std::uint8_t( 0 ) );
    o += kGroup;
    }
   const int b = int( std::bit_width( fill( k ) ) );
   w[ k % kGroup ] = std::uint8_t( b );
   packBlock( s , o , b );
   o += 16 * std::size_t( b );
   }
  out.assign( room.get() , o );
  }
 ADS_AUDIT( binaryUnpack( out , n , sorted ) ==
	    std::vector< std::uint32_t >( x.begin() , x.end() ) );
 return( out );
 }

inline std::vector< std::uint32_t >
binaryUnpack( std::span< const std::uint8_t > in , std::size_t n ,
	      bool sorted )
{
 using namespace detail::bp;
 const std::size_t blocks = ( n + kBlock - 1 ) / kBlock;
 std::vector< std::uint32_t > out( blocks * kBlock );
 std::uint32_t prev[ 4 ] = { 0 , 0 , 0 , 0 };
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
  unpackBlock( in.data() + at , out.data() + k * kBlock , b , sorted , prev );
  at += 16 * std::size_t( b );
  }
 out.resize( n );
 return( out );
 }

inline std::vector< std::uint8_t >
patchedPack( std::span< const std::uint32_t > x , bool sorted )
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
    v[ i ] = i >= m0  ? 0
	     : sorted ? x[ j ] - ( j >= 4 ? x[ j - 4 ] : 0 )
		      : x[ j ];
    }
  for( std::size_t i = 0 ; i < kBlock ; ++i )
   ++cnt[ std::bit_width( v[ i ] ) ];
  int beta = 32;
  while( ( beta > 0 ) && ( cnt[ beta ] == 0 ) )
   --beta;
  int b = beta;
  std::size_t best = kBlock * std::size_t( beta ) , c = 0 , cb = 0;
  for( int t = beta - 1 ; t >= 0 ; --t ) {
   c += cnt[ t + 1 ];  // the integers of 2^t or more
   const std::size_t cost =
     kBlock * std::size_t( t ) + c * ( 8 + std::size_t( beta - t ) ) + 8;
   if( cost < best ) {
    best = cost;
    b = t;
    cb = c;
    }
   }
  *o++ = std::uint8_t( b );
  *o++ = std::uint8_t( cb );
  if( cb > 0 )
   *o++ = std::uint8_t( beta );
  const std::uint32_t m = b >= 32 ? ~0u : ( 1u << b ) - 1;
  if( cb > 0 )  // the low bits of each
   for( std::size_t i = 0 ; i < kBlock ; ++i )
    low[ i ] = v[ i ] & m;
  packBlock( cb > 0 ? low : v , o , b );
  o += 16 * std::size_t( b );
  if( cb == 0 )
   continue;
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
  if( fill > 0 )
   *o++ = std::uint8_t( acc );
  }
 std::fill( o , o + 8 , std::uint8_t( 0 ) );
 out.resize( std::size_t( o - out.data() ) + 8 );
 ADS_AUDIT( patchedUnpack( out , n , sorted ) ==
	    std::vector< std::uint32_t >( x.begin() , x.end() ) );
 return( out );
 }

inline std::vector< std::uint32_t >
patchedUnpack( std::span< const std::uint8_t > in , std::size_t n ,
	       bool sorted )
{
 using namespace detail::bp;
 const std::size_t blocks = ( n + kBlock - 1 ) / kBlock;
 std::vector< std::uint32_t > out( blocks * kBlock );
 ADS_PRE( in.size() >= 8 );
 const std::size_t end = in.size() - 8;  // the 8 bytes of 0 after
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
  const std::size_t hb =
    ( std::size_t( c ) * std::size_t( beta - b ) + 7 ) / 8;
  ADS_PRE( ( b <= beta ) && ( beta <= 32 ) && ( c <= int( kBlock ) ) &&
	   ( at + 16 * std::size_t( b ) + std::size_t( c ) + hb <= end ) );
  std::uint32_t * o = out.data() + k * kBlock;
  unpackBlock( in.data() + at , o , b , false , nullptr );
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
   o[ pos[ e ] ] |=
     std::uint32_t( ( ( littleEndian64( h ) >> ( bit % 8 ) ) & mask ) << b );
   }
  at += std::size_t( c ) + hb;
  }
 // the sums at a distance of 4 in a pass of their own, once the
 // exceptions are set: 4 integers an addition
 if( sorted && ( n > 4 ) ) {
  std::size_t i = 4;
  for( ; i < out.size() ; ++i )
   out[ i ] += out[ i - 4 ];
  }
 out.resize( n );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BinaryPacking.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
