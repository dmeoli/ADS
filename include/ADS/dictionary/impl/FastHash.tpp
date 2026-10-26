/*--------------------------------------------------------------------------*/
/*--------------------------- File FastHash.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash functions of FastHash.h, with their contracts.
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

#if defined( _MSC_VER ) && !defined( __clang__ )
 #include <intrin.h>
#endif
#if defined( __AVX2__ ) || defined( __AVX512F__ )
 #include <immintrin.h>
#endif

/// a function never inlined, the rare path of a long string
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_HASH_NOINLINE __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_HASH_NOINLINE __declspec( noinline )
#else
 #define ADS_HASH_NOINLINE
#endif

namespace ADS {

namespace detail {

/// the constants of the hashes, the first words of the hexadecimal digits
/// of pi after the point
inline constexpr std::uint64_t kHashK[ 7 ] = {
 0x243F6A8885A308D3ULL , 0x13198A2E03707344ULL , 0xA4093822299F31D0ULL ,
 0x082EFA98EC4E6C89ULL , 0x452821E638D01377ULL , 0xBE5466CF34E90C6CULL ,
 0xC0AC29B7C97C50DDULL };

/// the word of 8 bytes from p, in the byte order of the machine
[[nodiscard]] inline std::uint64_t read64( const unsigned char * p )
{
 std::uint64_t w;
 std::memcpy( &w , p , 8 );
 return( w );
 }

/// the word of 4 bytes from p, in the byte order of the machine
[[nodiscard]] inline std::uint64_t read32( const unsigned char * p )
{
 std::uint32_t w;
 std::memcpy( &w , p , 4 );
 return( w );
 }

/// the words of the stripes of the long strings, the outputs of
/// SplitMix64 from the first word of kHashK
inline constexpr auto kStripeK = []( void ) {
 std::array< std::uint64_t , 32 > k{};
 std::uint64_t x = kHashK[ 0 ];
 for( auto & w : k ) {
  x += 0x9E3779B97F4A7C15ULL;
  std::uint64_t z = x;
  z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
  z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
  w = z ^ ( z >> 31 );
  }
 return( k );
 }();

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

constexpr Product128 product128( std::uint64_t a , std::uint64_t b )
{
#if defined( __SIZEOF_INT128__ )
 const unsigned __int128 p = static_cast< unsigned __int128 >( a ) * b;
 return( Product128{ std::uint64_t( p ) , std::uint64_t( p >> 64 ) } );
#else
 if( ! std::is_constant_evaluated() ) {
 #if defined( _MSC_VER ) && defined( _M_X64 ) && !defined( __clang__ )
  std::uint64_t hi;
  const std::uint64_t lo = _umul128( a , b , &hi );
  return( Product128{ lo , hi } );
 #elif defined( _MSC_VER ) && defined( _M_ARM64 ) && !defined( __clang__ )
  return( Product128{ a * b , __umulh( a , b ) } );
 #endif
  }
 constexpr std::uint64_t LOW = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & LOW , a1 = a >> 32 , b0 = b & LOW ,
                     b1 = b >> 32;
 const std::uint64_t p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
                     p11 = a1 * b1;
 const std::uint64_t mid = ( p00 >> 32 ) + ( p01 & LOW ) + ( p10 & LOW );
 return( Product128{ ( mid << 32 ) | ( p00 & LOW ) ,
                     p11 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 ) } );
#endif
 }

constexpr std::uint64_t foldedProduct( std::uint64_t a , std::uint64_t b )
{
 const Product128 p = product128( a , b );
 return( p.lo ^ p.hi );
 }

constexpr std::uint64_t hashWord( std::uint64_t x , std::uint64_t seed )
{
 return( foldedProduct( x , seed ^ detail::kHashK[ 1 ] ) );
 }

namespace detail {

/// the 8 lanes of the stripes of 64 bytes: each adds the product of the
/// two halves of its word XOR a word of kStripeK, and its word to the
/// other lane of its pair ( 2 i , 2 i + 1 ), which a processor with
/// vectors of 4 or 8 words does by one product of 32 bits per lane; the
/// words go to a sum of their own, added across the pairs only before a
/// scrambling and at the end, so that a stripe moves no word across its
/// lanes. Every 16 stripes a lane is scrambled by a shift, an XOR and a
/// product. The vectors give the same words as the scalar operations, so
/// the hash does not depend on the instructions of the machine
struct Lanes {
#if defined( __AVX512F__ )
 __m512i a , d;              ///< the lanes and the sum of the words

 explicit Lanes( std::uint64_t seed )
  : a( _mm512_xor_si512( _mm512_loadu_si512( kStripeK.data() ) ,
                         _mm512_set1_epi64(
                          static_cast< long long >( seed ) ) ) ) ,
    d( _mm512_setzero_si512() ) {}

 void stripe( const unsigned char * p , int t ) {
  const __m512i w = _mm512_loadu_si512( p ) ,
                x = _mm512_xor_si512( w , _mm512_loadu_si512(
                                           kStripeK.data() + t ) );
  d = _mm512_add_epi64( d , w );
  a = _mm512_add_epi64( a , _mm512_mul_epu32( x , _mm512_shuffle_epi32(
                                               x , _MM_PERM_CDAB ) ) );
  }

 void flush( void ) {
  a = _mm512_add_epi64( a , _mm512_shuffle_epi32( d , _MM_PERM_BADC ) );
  d = _mm512_setzero_si512();
  }

 void scramble( void ) {
  flush();
  const __m512i c = _mm512_set1_epi64( 0x9E3779B1LL ) ,
                x = _mm512_xor_si512( _mm512_xor_si512( a ,
                                       _mm512_srli_epi64( a , 47 ) ) ,
                                      _mm512_loadu_si512(
                                       kStripeK.data() + 16 ) );
  a = _mm512_add_epi64( _mm512_mul_epu32( x , c ) , _mm512_slli_epi64(
       _mm512_mul_epu32( _mm512_srli_epi64( x , 32 ) , c ) , 32 ) );
  }

 void store( std::uint64_t * w ) {
  flush();
  _mm512_storeu_si512( w , a );
  }
#elif defined( __AVX2__ )
 __m256i a[ 2 ] , d[ 2 ];    ///< the lanes and the sums of the words

 static __m256i load( const void * p ) {
  return( _mm256_loadu_si256( static_cast< const __m256i * >( p ) ) );
  }

 explicit Lanes( std::uint64_t seed ) {
  const __m256i s = _mm256_set1_epi64x( static_cast< long long >( seed ) );
  for( int i = 0 ; i < 2 ; ++i ) {
   a[ i ] = _mm256_xor_si256( load( kStripeK.data() + 4 * i ) , s );
   d[ i ] = _mm256_setzero_si256();
   }
  }

 void stripe( const unsigned char * p , int t ) {
  for( int i = 0 ; i < 2 ; ++i ) {
   const __m256i w = load( p + 32 * i ) ,
                 x = _mm256_xor_si256( w , load( kStripeK.data() + t +
                                                 4 * i ) );
   d[ i ] = _mm256_add_epi64( d[ i ] , w );
   a[ i ] = _mm256_add_epi64( a[ i ] , _mm256_mul_epu32( x ,
                               _mm256_shuffle_epi32( x , 0xB1 ) ) );
   }
  }

 void flush( void ) {
  for( int i = 0 ; i < 2 ; ++i ) {
   a[ i ] = _mm256_add_epi64( a[ i ] , _mm256_shuffle_epi32( d[ i ] ,
                                                             0x4E ) );
   d[ i ] = _mm256_setzero_si256();
   }
  }

 void scramble( void ) {
  flush();
  const __m256i c = _mm256_set1_epi64x( 0x9E3779B1LL );
  for( int i = 0 ; i < 2 ; ++i ) {
   const __m256i x = _mm256_xor_si256( _mm256_xor_si256( a[ i ] ,
                      _mm256_srli_epi64( a[ i ] , 47 ) ) ,
                      load( kStripeK.data() + 16 + 4 * i ) );
   a[ i ] = _mm256_add_epi64( _mm256_mul_epu32( x , c ) ,
             _mm256_slli_epi64( _mm256_mul_epu32(
              _mm256_srli_epi64( x , 32 ) , c ) , 32 ) );
   }
  }

 void store( std::uint64_t * w ) {
  flush();
  for( int i = 0 ; i < 2 ; ++i )
   _mm256_storeu_si256( reinterpret_cast< __m256i * >( w + 4 * i ) ,
                        a[ i ] );
  }
#else
 std::uint64_t a[ 8 ] , d[ 8 ];   ///< the lanes and the sums of the words

 explicit Lanes( std::uint64_t seed ) {
  for( int i = 0 ; i < 8 ; ++i ) {
   a[ i ] = kStripeK[ i ] ^ seed;
   d[ i ] = 0;
   }
  }

 void stripe( const unsigned char * p , int t ) {
  for( int i = 0 ; i < 8 ; ++i ) {
   const std::uint64_t w = read64( p + 8 * i ) ,
                       x = w ^ kStripeK[ i + t ];
   d[ i ] += w;
   a[ i ] += ( x & 0xFFFFFFFFULL ) * ( x >> 32 );
   }
  }

 void flush( void ) {
  for( int i = 0 ; i < 8 ; ++i ) {
   a[ i ] += d[ i ^ 1 ];
   d[ i ^ 1 ] = 0;
   }
  }

 void scramble( void ) {
  flush();
  for( int i = 0 ; i < 8 ; ++i )
   a[ i ] = ( a[ i ] ^ ( a[ i ] >> 47 ) ^ kStripeK[ 16 + i ] ) *
            0x9E3779B1ULL;
  }

 void store( std::uint64_t * w ) {
  flush();
  for( int i = 0 ; i < 8 ; ++i )
   w[ i ] = a[ i ];
  }
#endif
 };

/// the hashes of the strings of more than 64 bytes, out of line
struct LongHash {

 /// the hash of more than 64 bytes
 ADS_HASH_NOINLINE static std::uint64_t hash( const unsigned char * s ,
                                              std::size_t n ,
                                              std::uint64_t seed ) {
  const unsigned char * e = s + n;       // the end
  if( n > 256 )
   return( stripes( s , n , seed ) );
  std::uint64_t g = seed ^ kHashK[ 0 ];
  if( n > 96 ) {                          // 6 states, 6 products at once
   std::uint64_t g1 = g , g2 = g , g3 = g , g4 = g , g5 = g;
   do {
    g = foldedProduct( read64( s ) ^ kHashK[ 1 ] , read64( s + 8 ) ^ g );
    g1 = foldedProduct( read64( s + 16 ) ^ kHashK[ 2 ] ,
                        read64( s + 24 ) ^ g1 );
    g2 = foldedProduct( read64( s + 32 ) ^ kHashK[ 3 ] ,
                        read64( s + 40 ) ^ g2 );
    g3 = foldedProduct( read64( s + 48 ) ^ kHashK[ 4 ] ,
                        read64( s + 56 ) ^ g3 );
    g4 = foldedProduct( read64( s + 64 ) ^ kHashK[ 5 ] ,
                        read64( s + 72 ) ^ g4 );
    g5 = foldedProduct( read64( s + 80 ) ^ kHashK[ 6 ] ,
                        read64( s + 88 ) ^ g5 );
    s += 96;
    } while( e - s > 96 );
   g ^= g1 ^ g2 ^ g3 ^ g4 ^ g5;
   }
  while( e - s > 16 ) {                  // the last 16 bytes are left
   g = foldedProduct( read64( s ) ^ kHashK[ 1 ] , read64( s + 8 ) ^ g );
   s += 16;
   }
  return( foldedProduct( read64( e - 16 ) ^ kHashK[ 1 ] ,
                         read64( e - 8 ) ^ n ^ g ) );
  }

 /// the hash of more than 256 bytes by stripes of 64 bytes
 ADS_HASH_NOINLINE static std::uint64_t stripes( const unsigned char * s ,
                                                 std::size_t n ,
                                                 std::uint64_t seed ) {
  Lanes L( seed );
  const std::size_t m = ( n - 1 ) / 64;  // the stripes, the last apart
  std::size_t j = 0;
  for( ; j + 16 <= m ; j += 16 ) {       // blocks of 1 KB
   for( int t = 0 ; t < 16 ; ++t ) {
#if defined( __GNUC__ ) || defined( __clang__ )
    __builtin_prefetch( s + 64 * ( j + std::size_t( t ) ) + 384 );
#endif
    L.stripe( s + 64 * ( j + std::size_t( t ) ) , t );
    }
   L.scramble();
   }
  for( int t = 0 ; j + std::size_t( t ) < m ; ++t )
   L.stripe( s + 64 * ( j + std::size_t( t ) ) , t );
  L.stripe( s + n - 64 , 16 );           // the last 64 bytes
  std::uint64_t a[ 8 ];
  L.store( a );
  std::uint64_t h = ( seed ^ kHashK[ 0 ] ) + n * kHashK[ 1 ];
  for( int i = 0 ; i < 8 ; i += 2 )
   h += foldedProduct( a[ i ] ^ kStripeK[ 24 + i ] ,
                       a[ i + 1 ] ^ kStripeK[ 25 + i ] );
  return( foldedProduct( h ^ kHashK[ 2 ] , ( h >> 32 ) ^ kHashK[ 3 ] ) );
  }

 };  // end( struct LongHash )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

inline std::uint64_t hashBytes( const void * p , std::size_t n ,
                                std::uint64_t seed )
{
 ADS_PRE( ( p != nullptr ) || ( n == 0 ) );
 using detail::kHashK;
 using detail::read32;
 using detail::read64;
 const auto * s = static_cast< const unsigned char * >( p );
 const std::uint64_t h = seed ^ kHashK[ 0 ];
 if( n <= 8 ) {
  // a product in each branch: one shared by the two is compiled by GCC as
  // a product of 128 bits, one more multiplication
  if( n >= 4 )
   return( foldedProduct( ( ( read32( s ) << 32 ) | read32( s + n - 4 ) ) ^
                          h , kHashK[ 1 ] ^ n ) );
  const std::uint64_t a = n == 0 ? 0 :
   ( std::uint64_t( s[ 0 ] ) << 48 ) |
   ( std::uint64_t( s[ n >> 1 ] ) << 24 ) | s[ n - 1 ];
  return( foldedProduct( a ^ h , kHashK[ 1 ] ^ n ) );
  }
 if( n <= 16 )
  return( foldedProduct( read64( s ) ^ h ,
                         read64( s + n - 8 ) ^ kHashK[ 1 ] ^ n ) );
 if( n <= 32 )                            // two products at once
  return( foldedProduct( read64( s ) ^ kHashK[ 2 ] , read64( s + 8 ) ^ h ) ^
          foldedProduct( read64( s + n - 16 ) ^ kHashK[ 3 ] ,
                         read64( s + n - 8 ) ^ h ^ n ) );
 if( n <= 64 ) {                          // four products at once
  const unsigned char * e = s + n;
  const std::uint64_t h0 = foldedProduct( read64( s ) ^ kHashK[ 2 ] ,
                                          read64( s + 8 ) ^ h ) ,
                      h1 = foldedProduct( read64( s + 16 ) ^ kHashK[ 3 ] ,
                                          read64( s + 24 ) ^ h ) ,
                      h2 = foldedProduct( read64( e - 32 ) ^ kHashK[ 4 ] ,
                                          read64( e - 24 ) ^ h ) ,
                      h3 = foldedProduct( read64( e - 16 ) ^ kHashK[ 5 ] ,
                                          read64( e - 8 ) ^ h ^ n );
  return( foldedProduct( h0 ^ h2 ^ kHashK[ 6 ] , h1 ^ h3 ^ kHashK[ 1 ] ) );
  }
 return( detail::LongHash::hash( s , n , seed ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File FastHash.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
