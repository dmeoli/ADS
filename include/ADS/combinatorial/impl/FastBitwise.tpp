/*--------------------------------------------------------------------------*/
/*-------------------------- File FastBitwise.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of FastBitwise.h, with their contracts.
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

namespace ADS {

namespace detail {

/// the 1 bits of the words get( 0 ) , ... , get( n - 1 ), in four sums
template< class G >
inline std::uint64_t popcountScalar( G && get , std::size_t n )
{
 std::uint64_t s0 = 0 , s1 = 0 , s2 = 0 , s3 = 0;
 const std::size_t q = n / 4;
 for( std::size_t k = 0 ; k < q ; ++k ) {
  s0 += std::uint64_t( std::popcount( get( 4 * k ) ) );
  s1 += std::uint64_t( std::popcount( get( 4 * k + 1 ) ) );
  s2 += std::uint64_t( std::popcount( get( 4 * k + 2 ) ) );
  s3 += std::uint64_t( std::popcount( get( 4 * k + 3 ) ) );
  }
 for( std::size_t i = 4 * q ; i < n ; ++i )
  s0 += std::uint64_t( std::popcount( get( i ) ) );
 return( s0 + s1 + s2 + s3 );
 }

#if defined( __AVX512F__ ) && defined( __AVX512BW__ )

/// the 1 bits of each word of v, in its lane
inline __m512i popcount512( __m512i v )
{
#if defined( __AVX512VPOPCNTDQ__ )
 return( _mm512_popcnt_epi64( v ) );
#else
 const __m512i lut = _mm512_broadcast_i32x4( _mm_setr_epi8(
                      0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                      1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 ) );
 const __m512i low = _mm512_set1_epi8( 0x0F );
 const __m512i lo = _mm512_and_si512( v , low );
 const __m512i hi = _mm512_and_si512( _mm512_srli_epi16( v , 4 ) , low );
 const __m512i c = _mm512_add_epi8( _mm512_shuffle_epi8( lut , lo ) ,
                                    _mm512_shuffle_epi8( lut , hi ) );
 return( _mm512_sad_epu8( c , _mm512_setzero_si512() ) );
#endif
 }

/// the full adder of three vectors of bits: h the carries, l the sums
inline void csa512( __m512i & h , __m512i & l , __m512i a , __m512i b ,
                    __m512i c )
{
 l = _mm512_ternarylogic_epi64( a , b , c , 0x96 );   // a ^ b ^ c
 h = _mm512_ternarylogic_epi64( a , b , c , 0xE8 );   // the majority
 }

/// Harley-Seal on the vectors load( 0 ) , ... , load( n - 1 )
template< class L >
inline std::uint64_t harleySeal( L && load , std::size_t n )
{
 __m512i total = _mm512_setzero_si512() , ones = total , twos = total ,
         fours = total , eights = total , sixteens , twosA , twosB , foursA ,
         foursB , eightsA , eightsB;
 std::size_t i = 0;
 for( ; i + 16 <= n ; i += 16 ) {
  csa512( twosA , ones , ones , load( i ) , load( i + 1 ) );
  csa512( twosB , ones , ones , load( i + 2 ) , load( i + 3 ) );
  csa512( foursA , twos , twos , twosA , twosB );
  csa512( twosA , ones , ones , load( i + 4 ) , load( i + 5 ) );
  csa512( twosB , ones , ones , load( i + 6 ) , load( i + 7 ) );
  csa512( foursB , twos , twos , twosA , twosB );
  csa512( eightsA , fours , fours , foursA , foursB );
  csa512( twosA , ones , ones , load( i + 8 ) , load( i + 9 ) );
  csa512( twosB , ones , ones , load( i + 10 ) , load( i + 11 ) );
  csa512( foursA , twos , twos , twosA , twosB );
  csa512( twosA , ones , ones , load( i + 12 ) , load( i + 13 ) );
  csa512( twosB , ones , ones , load( i + 14 ) , load( i + 15 ) );
  csa512( foursB , twos , twos , twosA , twosB );
  csa512( eightsB , fours , fours , foursA , foursB );
  csa512( sixteens , eights , eights , eightsA , eightsB );
  total = _mm512_add_epi64( total , popcount512( sixteens ) );
  }
 total = _mm512_slli_epi64( total , 4 );
 total = _mm512_add_epi64( total ,
          _mm512_slli_epi64( popcount512( eights ) , 3 ) );
 total = _mm512_add_epi64( total ,
          _mm512_slli_epi64( popcount512( fours ) , 2 ) );
 total = _mm512_add_epi64( total ,
          _mm512_slli_epi64( popcount512( twos ) , 1 ) );
 total = _mm512_add_epi64( total , popcount512( ones ) );
 for( ; i < n ; ++i )
  total = _mm512_add_epi64( total , popcount512( load( i ) ) );
 return( std::uint64_t( _mm512_reduce_add_epi64( total ) ) );
 }

inline constexpr std::size_t vectorWords = 8;   ///< the words of a vector

#elif defined( __AVX2__ )

/// the 1 bits of each word of v, in its lane
inline __m256i popcount256( __m256i v )
{
 const __m256i lut = _mm256_setr_epi8( 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                       1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 ,
                                       0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                       1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 );
 const __m256i low = _mm256_set1_epi8( 0x0F );
 const __m256i lo = _mm256_and_si256( v , low );
 const __m256i hi = _mm256_and_si256( _mm256_srli_epi16( v , 4 ) , low );
 const __m256i c = _mm256_add_epi8( _mm256_shuffle_epi8( lut , lo ) ,
                                    _mm256_shuffle_epi8( lut , hi ) );
 return( _mm256_sad_epu8( c , _mm256_setzero_si256() ) );
 }

/// the full adder of three vectors of bits: h the carries, l the sums
inline void csa256( __m256i & h , __m256i & l , __m256i a , __m256i b ,
                    __m256i c )
{
 const __m256i u = _mm256_xor_si256( a , b );
 h = _mm256_or_si256( _mm256_and_si256( a , b ) , _mm256_and_si256( u , c ) );
 l = _mm256_xor_si256( u , c );
 }

/// Harley-Seal on the vectors load( 0 ) , ... , load( n - 1 )
template< class L >
inline std::uint64_t harleySeal( L && load , std::size_t n )
{
 __m256i total = _mm256_setzero_si256() , ones = total , twos = total ,
         fours = total , eights = total , sixteens , twosA , twosB , foursA ,
         foursB , eightsA , eightsB;
 std::size_t i = 0;
 for( ; i + 16 <= n ; i += 16 ) {
  csa256( twosA , ones , ones , load( i ) , load( i + 1 ) );
  csa256( twosB , ones , ones , load( i + 2 ) , load( i + 3 ) );
  csa256( foursA , twos , twos , twosA , twosB );
  csa256( twosA , ones , ones , load( i + 4 ) , load( i + 5 ) );
  csa256( twosB , ones , ones , load( i + 6 ) , load( i + 7 ) );
  csa256( foursB , twos , twos , twosA , twosB );
  csa256( eightsA , fours , fours , foursA , foursB );
  csa256( twosA , ones , ones , load( i + 8 ) , load( i + 9 ) );
  csa256( twosB , ones , ones , load( i + 10 ) , load( i + 11 ) );
  csa256( foursA , twos , twos , twosA , twosB );
  csa256( twosA , ones , ones , load( i + 12 ) , load( i + 13 ) );
  csa256( twosB , ones , ones , load( i + 14 ) , load( i + 15 ) );
  csa256( foursB , twos , twos , twosA , twosB );
  csa256( eightsB , fours , fours , foursA , foursB );
  csa256( sixteens , eights , eights , eightsA , eightsB );
  total = _mm256_add_epi64( total , popcount256( sixteens ) );
  }
 total = _mm256_slli_epi64( total , 4 );
 total = _mm256_add_epi64( total ,
          _mm256_slli_epi64( popcount256( eights ) , 3 ) );
 total = _mm256_add_epi64( total ,
          _mm256_slli_epi64( popcount256( fours ) , 2 ) );
 total = _mm256_add_epi64( total ,
          _mm256_slli_epi64( popcount256( twos ) , 1 ) );
 total = _mm256_add_epi64( total , popcount256( ones ) );
 for( ; i < n ; ++i )
  total = _mm256_add_epi64( total , popcount256( load( i ) ) );
 alignas( 32 ) std::uint64_t t[ 4 ];
 _mm256_store_si256( reinterpret_cast< __m256i * >( t ) , total );
 return( t[ 0 ] + t[ 1 ] + t[ 2 ] + t[ 3 ] );
 }

inline constexpr std::size_t vectorWords = 4;   ///< the words of a vector

#endif

}  // end( namespace detail )

inline std::uint64_t popcount( const Word * w , std::size_t n )
{
#if ( defined( __AVX512F__ ) && defined( __AVX512BW__ ) ) || \
    defined( __AVX2__ )
 constexpr std::size_t V = detail::vectorWords;
 if( n >= 16 * V ) {                        // a block of Harley-Seal
  const std::size_t nv = n / V;
#if defined( __AVX512F__ ) && defined( __AVX512BW__ )
  auto load = [ w ]( std::size_t i ) {
   return( _mm512_loadu_si512( w + V * i ) ); };
#else
  auto load = [ w ]( std::size_t i ) {
   return( _mm256_loadu_si256(
            reinterpret_cast< const __m256i * >( w + V * i ) ) ); };
#endif
  const std::uint64_t s = detail::harleySeal( load , nv );
  const Word * t = w + nv * V;
  return( s + detail::popcountScalar( [ t ]( std::size_t i ) {
   return( t[ i ] ); } , n - nv * V ) );
  }
#endif
 return( detail::popcountScalar( [ w ]( std::size_t i ) {
  return( w[ i ] ); } , n ) );
 }

inline std::uint64_t popcountAnd( const Word * a , const Word * b ,
                                  std::size_t n )
{
#if ( defined( __AVX512F__ ) && defined( __AVX512BW__ ) ) || \
    defined( __AVX2__ )
 constexpr std::size_t V = detail::vectorWords;
 if( n >= 16 * V ) {
  const std::size_t nv = n / V;
#if defined( __AVX512F__ ) && defined( __AVX512BW__ )
  auto load = [ a , b ]( std::size_t i ) {
   return( _mm512_and_si512( _mm512_loadu_si512( a + V * i ) ,
                             _mm512_loadu_si512( b + V * i ) ) ); };
#else
  auto load = [ a , b ]( std::size_t i ) {
   return( _mm256_and_si256(
    _mm256_loadu_si256( reinterpret_cast< const __m256i * >( a + V * i ) ) ,
    _mm256_loadu_si256( reinterpret_cast< const __m256i * >( b + V * i ) )
    ) ); };
#endif
  const std::uint64_t s = detail::harleySeal( load , nv );
  const std::size_t o = nv * V;
  return( s + detail::popcountScalar( [ a , b , o ]( std::size_t i ) {
   return( a[ o + i ] & b[ o + i ] ); } , n - o ) );
  }
#endif
 return( detail::popcountScalar( [ a , b ]( std::size_t i ) {
  return( a[ i ] & b[ i ] ); } , n ) );
 }

inline Word pdep( Word x , Word mask )
{
#if defined( __BMI2__ )
 return( Word( _pdep_u64( x , mask ) ) );
#else
 Word r = 0;
 for( Word b = 1 ; mask ; b += b ) {        // the 1 bits of mask in order
  if( x & b ) r |= mask & ( ~mask + 1 );
  mask &= mask - 1;
  }
 return( r );
#endif
 }

inline Word pext( Word x , Word mask )
{
#if defined( __BMI2__ )
 return( Word( _pext_u64( x , mask ) ) );
#else
 Word r = 0;
 for( Word b = 1 ; mask ; b += b ) {
  if( x & mask & ( ~mask + 1 ) ) r |= b;
  mask &= mask - 1;
  }
 return( r );
#endif
 }

inline int select( Word x , int r )
{
 ADS_PRE( ( r >= 0 ) && ( r < std::popcount( x ) ) );
#if defined( __BMI2__ )
 return( std::countr_zero( pdep( Word( 1 ) << r , x ) ) );
#else
 constexpr Word L8 = 0x0101010101010101ULL , H8 = 0x8080808080808080ULL;
 Word s = x - ( ( x >> 1 ) & 0x5555555555555555ULL );
 s = ( s & 0x3333333333333333ULL ) + ( ( s >> 2 ) & 0x3333333333333333ULL );
 s = ( ( s + ( s >> 4 ) ) & 0x0F0F0F0F0F0F0F0FULL ) * L8;   // prefix sums
 const Word le = ( ( Word( r ) * L8 | H8 ) - s ) & H8;   // bytes <= r
 const int k = std::popcount( le ) * 8;     // the place of the byte
 int rr = r - int( ( ( s << 8 ) >> k ) & 0xFF );
 Word b = ( x >> k ) & 0xFF;
 while( rr-- > 0 ) b &= b - 1;
 return( k + std::countr_zero( b ) );
#endif
 }

inline Word mortonEncode( std::uint32_t x , std::uint32_t y )
{
#if defined( __BMI2__ )
 return( pdep( x , 0x5555555555555555ULL ) |
         pdep( y , 0xAAAAAAAAAAAAAAAAULL ) );
#else
 return( interleave( x , y ) );
#endif
 }

inline std::pair< std::uint32_t , std::uint32_t > mortonDecode( Word z )
{
#if defined( __BMI2__ )
 return( std::pair( std::uint32_t( pext( z , 0x5555555555555555ULL ) ) ,
                    std::uint32_t( pext( z , 0xAAAAAAAAAAAAAAAAULL ) ) ) );
#else
 return( deinterleave( z ) );
#endif
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File FastBitwise.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
