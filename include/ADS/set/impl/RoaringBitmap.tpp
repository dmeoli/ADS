/*--------------------------------------------------------------------------*/
/*------------------------- File RoaringBitmap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RoaringBitmap [see RoaringBitmap.h], with their
 * contracts.
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

#include <algorithm>
#include <bit>
#include <cstring>

#if defined( __AVX2__ ) || defined( __SSE4_2__ )
 #include <immintrin.h>
#endif

#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_ROAR_KERNEL __attribute__(( noinline ))
#else
 #define ADS_ROAR_KERNEL
#endif

namespace ADS {

namespace detail {

/// the bits of op( a[ k ] , b[ k ] ) for k < n, written to c unless it is
/// nullptr, and their count: with AVX2 4 words at a time, the bits of the
/// half bytes looked up by a shuffle of bytes [see BoolSet.h]
template< class Op >
inline std::uint32_t roaringWords( const std::uint64_t * a ,
                                   const std::uint64_t * b ,
                                   std::uint64_t * c , std::size_t n ,
                                   Op op )
{
 std::size_t k = 0;
 std::uint64_t cnt = 0;
#if defined( __AVX2__ )
 const __m256i table = _mm256_setr_epi8( 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                         1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 ,
                                         0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                         1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 );
 const __m256i low = _mm256_set1_epi8( 0x0F );
 __m256i acc = _mm256_setzero_si256();
 for( ; k + 4 <= n ; k += 4 ) {
  const __m256i v = op(
   _mm256_loadu_si256( reinterpret_cast< const __m256i * >( a + k ) ) ,
   _mm256_loadu_si256( reinterpret_cast< const __m256i * >( b + k ) ) );
  if( c )
   _mm256_storeu_si256( reinterpret_cast< __m256i * >( c + k ) , v );
  const __m256i t = _mm256_add_epi8(
   _mm256_shuffle_epi8( table , _mm256_and_si256( v , low ) ) ,
   _mm256_shuffle_epi8( table , _mm256_and_si256(
                                 _mm256_srli_epi16( v , 4 ) , low ) ) );
  acc = _mm256_add_epi64( acc , _mm256_sad_epu8( t ,
                                                 _mm256_setzero_si256() ) );
  }
 alignas( 32 ) std::uint64_t s[ 4 ];
 _mm256_store_si256( reinterpret_cast< __m256i * >( s ) , acc );
 cnt = s[ 0 ] + s[ 1 ] + s[ 2 ] + s[ 3 ];
#endif
 for( ; k < n ; ++k ) {
  const std::uint64_t v = op( a[ k ] , b[ k ] );
  if( c )
   c[ k ] = v;
  cnt += std::uint64_t( std::popcount( v ) );
  }
 return( std::uint32_t( cnt ) );
 }

#if defined( __SSE4_2__ )

/// the shuffles of bytes that pack to the front the items of 16 bits of a
/// vector of 8 chosen by a mask
struct Pack16 {
 alignas( 16 ) std::uint8_t p[ 256 ][ 16 ];
 constexpr Pack16( void ) : p{} {
  for( int m = 0 ; m < 256 ; ++m ) {
   int c = 0;
   for( int b = 0 ; b < 8 ; ++b )
    if( m & ( 1 << b ) ) {
     p[ m ][ 2 * c ] = std::uint8_t( 2 * b );
     p[ m ][ 2 * c + 1 ] = std::uint8_t( 2 * b + 1 );
     ++c;
     }
   for( int k = 2 * c ; k < 16 ; ++k ) p[ m ][ k ] = 0x80;
   }
  }
 };

inline constexpr Pack16 pack16{};

/// the bits of the items of va found in vb, 8 items of 16 bits each, none
/// 0: a comparison of strings of implicit length (0 would end a string),
/// cheaper than that of explicit length
inline unsigned found8( __m128i va , __m128i vb )
{
 return( unsigned( _mm_extract_epi32( _mm_cmpistrm( vb , va ,
          _SIDD_UWORD_OPS | _SIDD_CMP_EQUAL_ANY | _SIDD_BIT_MASK ) , 0 ) ) );
 }

#endif

/// the items common to the sorted a and b of 16 bits, written to out (which
/// has room for 8 more), and their number: with SSE4.2, blocks of 8 items
/// compared all against all by one comparison of strings (the method of
/// Schlegel, Willhalm and Lehner), the items of a found packed by a shuffle
/// and stored, and the block that ends first moving on; the rest by a
/// merge with no branch
ADS_ROAR_KERNEL inline std::size_t intersect16( const std::uint16_t * a ,
                                                std::size_t na ,
                                                const std::uint16_t * b ,
                                                std::size_t nb ,
                                                std::uint16_t * out )
{
 std::size_t i = 0 , j = 0 , k = 0;
 if( ( na > 0 ) && ( nb > 0 ) && ( ( a[ 0 ] == 0 ) || ( b[ 0 ] == 0 ) ) ) {
  if( ( a[ 0 ] == 0 ) && ( b[ 0 ] == 0 ) )  // the item 0 apart, which
   out[ k++ ] = 0;                          // would end a string
  i = a[ 0 ] == 0 ? 1 : 0;
  j = b[ 0 ] == 0 ? 1 : 0;
  }
#if defined( __SSE4_2__ )
 if( ( i + 8 <= na ) && ( j + 8 <= nb ) ) {
  __m128i va = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                 a + i ) );
  __m128i vb = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                 b + j ) );
  for( ; ; ) {                              // a block moves on, or both
   const unsigned r = found8( va , vb );
   const __m128i sh = _mm_load_si128( reinterpret_cast< const __m128i * >(
                                       pack16.p[ r ] ) );
   _mm_storeu_si128( reinterpret_cast< __m128i * >( out + k ) ,
                     _mm_shuffle_epi8( va , sh ) );
   k += std::size_t( std::popcount( r ) );
   const std::uint16_t x = a[ i + 7 ] , y = b[ j + 7 ];
   if( x <= y ) {
    i += 8;
    if( i + 8 > na ) break;
    va = _mm_loadu_si128( reinterpret_cast< const __m128i * >( a + i ) );
    }
   if( y <= x ) {
    j += 8;
    if( j + 8 > nb ) break;
    vb = _mm_loadu_si128( reinterpret_cast< const __m128i * >( b + j ) );
    }
   }
  }
#endif
 while( ( i < na ) && ( j < nb ) ) {
  const std::uint16_t p = a[ i ] , q = b[ j ];
  out[ k ] = p;
  k += p == q;
  i += p <= q;
  j += q <= p;
  }
 return( k );
 }

/// the items of the sorted a of 16 bits not in the sorted b, written to out
/// (which has room for 8 more), and their number: with SSE4.2 the blocks of
/// 8 are compared all against all as in intersect16(), the items of the
/// block of a found in some block of b gathered in a mask, and the others
/// packed and stored when the block of a moves on
ADS_ROAR_KERNEL inline std::size_t difference16( const std::uint16_t * a ,
                                                 std::size_t na ,
                                                 const std::uint16_t * b ,
                                                 std::size_t nb ,
                                                 std::uint16_t * out )
{
 std::size_t i = 0 , j = 0 , k = 0;
 if( ( na > 0 ) && ( nb > 0 ) && ( ( a[ 0 ] == 0 ) || ( b[ 0 ] == 0 ) ) ) {
  if( ( a[ 0 ] == 0 ) && ( b[ 0 ] != 0 ) )  // the item 0 apart
   out[ k++ ] = 0;
  i = a[ 0 ] == 0 ? 1 : 0;
  j = b[ 0 ] == 0 ? 1 : 0;
  }
 unsigned mask = 0;                         // the found of the block of a
#if defined( __SSE4_2__ )
 if( ( i + 8 <= na ) && ( j + 8 <= nb ) ) {
  __m128i va = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                 a + i ) );
  __m128i vb = _mm_loadu_si128( reinterpret_cast< const __m128i * >(
                                 b + j ) );
  for( ; ; ) {
   mask |= found8( va , vb );
   const std::uint16_t x = a[ i + 7 ] , y = b[ j + 7 ];
   if( x <= y ) {                           // the block of a is done
    const unsigned keep = ~mask & 0xFF;
    const __m128i sh = _mm_load_si128( reinterpret_cast< const __m128i * >(
                                        pack16.p[ keep ] ) );
    _mm_storeu_si128( reinterpret_cast< __m128i * >( out + k ) ,
                      _mm_shuffle_epi8( va , sh ) );
    k += std::size_t( std::popcount( keep ) );
    mask = 0;
    i += 8;
    if( i + 8 > na ) break;
    va = _mm_loadu_si128( reinterpret_cast< const __m128i * >( a + i ) );
    }
   if( y <= x ) {
    j += 8;
    if( j + 8 > nb ) break;
    vb = _mm_loadu_si128( reinterpret_cast< const __m128i * >( b + j ) );
    }
   }
  }
#endif
 const std::size_t i0 = i;                  // the rest, one at a time
 for( std::size_t t = i ; t < na ; ++t ) {
  if( ( t - i0 < 8 ) && ( ( mask >> ( t - i0 ) ) & 1 ) )
   continue;                                // found already
  const std::uint16_t x = a[ t ];
  while( ( j < nb ) && ( b[ j ] < x ) ) ++j;
  if( ( j == nb ) || ( b[ j ] != x ) )
   out[ k++ ] = x;
  }
 return( k );
 }

#if defined( __SSE4_2__ )

/// the two sorted vectors lo and hi of 8 items of 16 bits merged, the 8
/// smallest in lo and the 8 largest in hi, both sorted: the minima and the
/// maxima of the two, the minima rotated by one place 7 times (the network
/// of Inoue and Taura)
inline void merge8( __m128i & lo , __m128i & hi )
{
 __m128i mn = _mm_min_epu16( lo , hi ) , mx = _mm_max_epu16( lo , hi );
 for( int t = 0 ; t < 7 ; ++t ) {
  mn = _mm_alignr_epi8( mn , mn , 2 );
  const __m128i m = _mm_min_epu16( mn , mx );
  mx = _mm_max_epu16( mn , mx );
  mn = m;
  }
 lo = _mm_alignr_epi8( mn , mn , 2 );
 hi = mx;
 }

#endif

/// the items of the sorted a or b of 16 bits, each once, written to out
/// (which has room for na + nb + 8), and their number: with SSE4.2 merged
/// by blocks of 8 [see merge8()], the next block taken from the list with
/// the smaller head, the items equal to the one before them dropped by a
/// comparison with the block shifted by one place
ADS_ROAR_KERNEL inline std::size_t union16( const std::uint16_t * a ,
                                            std::size_t na ,
                                            const std::uint16_t * b ,
                                            std::size_t nb ,
                                            std::uint16_t * out )
{
 std::size_t i = 0 , j = 0 , k = 0;
 int last = -1;                             // the last item written
#if defined( __SSE4_2__ )
 if( ( na >= 8 ) && ( nb >= 8 ) ) {
  __m128i lo = _mm_loadu_si128( reinterpret_cast< const __m128i * >( a ) );
  __m128i hi = _mm_loadu_si128( reinterpret_cast< const __m128i * >( b ) );
  i = j = 8;
  __m128i prev = _mm_set1_epi16( -1 );      // its last lane, the last item
  bool first = true;
  for( ; ; ) {
   merge8( lo , hi );
   const __m128i sh = _mm_alignr_epi8( lo , prev , 14 );   // one place
   unsigned dup = unsigned( _mm_movemask_epi8( _mm_packs_epi16(
                   _mm_cmpeq_epi16( lo , sh ) , _mm_setzero_si128() ) ) );
   if( first ) {
    dup &= ~1u;
    first = false;
    }
   const unsigned keep = ~dup & 0xFF;
   const __m128i pk = _mm_load_si128( reinterpret_cast< const __m128i * >(
                                       pack16.p[ keep ] ) );
   _mm_storeu_si128( reinterpret_cast< __m128i * >( out + k ) ,
                     _mm_shuffle_epi8( lo , pk ) );
   k += std::size_t( std::popcount( keep ) );
   prev = lo;
   // the next block, from the list with the smaller head, if a whole
   // block; a smaller head in a tail ends the blocks
   const bool fa = i + 8 <= na , fb = j + 8 <= nb;
   const int ha = i < na ? a[ i ] : 1 << 16 , hb = j < nb ? b[ j ] : 1 << 16;
   if( fa && ( ha <= hb ) ) {
    lo = _mm_loadu_si128( reinterpret_cast< const __m128i * >( a + i ) );
    i += 8;
    }
   else if( fb && ( hb < ha ) ) {
    lo = _mm_loadu_si128( reinterpret_cast< const __m128i * >( b + j ) );
    j += 8;
    }
   else
    break;
   }
  last = std::uint16_t( _mm_extract_epi16( prev , 7 ) );
  alignas( 16 ) std::uint16_t h[ 8 ];       // hi and the two tails
  _mm_store_si128( reinterpret_cast< __m128i * >( h ) , hi );
  std::size_t p = 0;
  while( ( p < 8 ) || ( i < na ) || ( j < nb ) ) {
   int v = 1 << 17;
   int src = 0;
   if( ( p < 8 ) && ( h[ p ] < v ) ) { v = h[ p ]; src = 0; }
   if( ( i < na ) && ( a[ i ] < v ) ) { v = a[ i ]; src = 1; }
   if( ( j < nb ) && ( b[ j ] < v ) ) { v = b[ j ]; src = 2; }
   if( src == 0 ) ++p; else if( src == 1 ) ++i; else ++j;
   if( v != last ) {
    out[ k++ ] = std::uint16_t( v );
    last = v;
    }
   }
  return( k );
  }
#endif
 while( ( i < na ) || ( j < nb ) ) {        // one at a time
  int v;
  if( ( j == nb ) || ( ( i < na ) && ( a[ i ] <= b[ j ] ) ) ) v = a[ i++ ];
  else v = b[ j++ ];
  if( v != last ) {
   out[ k++ ] = std::uint16_t( v );
   last = v;
   }
  }
 return( k );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONTAINERS ------------------------------*/
/*--------------------------------------------------------------------------*/

inline auto RoaringBitmap::copy( const Block & b ) -> Block
{
 Block c;
 c.key = b.key;
 c.card = b.card;
 if( b.bits() ) {
  c.w.reset( new std::uint64_t[ WORDS ] );
  std::memcpy( c.w.get() , b.w.get() , WORDS * sizeof( std::uint64_t ) );
  }
 else
  c.a = b.a;
 return( c );
 }

inline void RoaringBitmap::toBitmap( Block & b )
{
 std::unique_ptr< std::uint64_t[] > w( new std::uint64_t[ WORDS ]() );
 for( const std::uint16_t x : b.a )
  w[ x >> 6 ] |= std::uint64_t( 1 ) << ( x & 63 );
 b.w = std::move( w );
 decltype( b.a )().swap( b.a );
 }

inline void RoaringBitmap::toArray( Block & b )
{
 b.a.resize( b.card );
 std::size_t k = 0;
 for( std::size_t i = 0 ; i < WORDS ; ++i )
  for( std::uint64_t v = b.w[ i ] ; v ; v &= v - 1 )
   b.a[ k++ ] = std::uint16_t( i * 64 + std::size_t( std::countr_zero( v ) ) );
 b.w.reset();
 }

inline auto RoaringBitmap::fromWords( std::uint16_t key ,
                                      std::unique_ptr< std::uint64_t[] > w ,
                                      std::uint32_t card ) -> Block
{
 Block b;
 b.key = key;
 b.card = card;
 b.w = std::move( w );
 if( card <= ARRAY )
  toArray( b );
 return( b );
 }

template< RoaringBitmap::Op O >
inline auto RoaringBitmap::combine( const Block & x , const Block & y )
 -> Block
{
 Block r;
 r.key = x.key;
 constexpr auto And = []( auto a , auto b ) { return( a & b ); };
 constexpr auto Or = []( auto a , auto b ) { return( a | b ); };
 constexpr auto AndNot = []( auto a , auto b ) { return( a & ~b ); };
 constexpr auto Xor = []( auto a , auto b ) { return( a ^ b ); };
 auto test = []( const std::uint64_t * w , std::uint16_t v ) {
  return( ( w[ v >> 6 ] >> ( v & 63 ) ) & 1 );
  };
 if( x.bits() && y.bits() ) {               // word by word
  const std::uint64_t * a = x.w.get() , * b = y.w.get();
  std::uint32_t c;
  if constexpr( ( O == Op::AND ) || ( O == Op::ANDNOT ) ) {
   const auto f = [ & ]( auto p , auto q ) {
    if constexpr( O == Op::AND ) return( And( p , q ) );
    else return( AndNot( p , q ) ); };
   std::unique_ptr< std::uint64_t[] > w( new std::uint64_t[ WORDS ] );
   c = detail::roaringWords( a , b , w.get() , WORDS , f );
   return( fromWords( r.key , std::move( w ) , c ) );
   }
  else {
   const auto f = [ & ]( auto p , auto q ) {
    if constexpr( O == Op::OR ) return( Or( p , q ) );
    else return( Xor( p , q ) ); };
   std::unique_ptr< std::uint64_t[] > w( new std::uint64_t[ WORDS ] );
   c = detail::roaringWords( a , b , w.get() , WORDS , f );
   return( fromWords( r.key , std::move( w ) , c ) );
   }
  }
 if( x.bits() || y.bits() ) {               // a bitmap and an array
  const Block & m = x.bits() ? x : y;       // the bitmap
  const Block & s = x.bits() ? y : x;       // the array
  if constexpr( O == Op::AND ) {
   r.a.resize( s.a.size() );
   std::size_t k = 0;
   for( const std::uint16_t v : s.a ) {
    r.a[ k ] = v;
    k += test( m.w.get() , v );
    }
   r.a.resize( k );
   r.card = std::uint32_t( k );
   return( r );
   }
  else if constexpr( O == Op::ANDNOT ) {
   if( ! x.bits() ) {                       // array - bitmap
    r.a.resize( x.a.size() );
    std::size_t k = 0;
    for( const std::uint16_t v : x.a ) {
     r.a[ k ] = v;
     k += 1 - test( y.w.get() , v );
     }
    r.a.resize( k );
    r.card = std::uint32_t( k );
    return( r );
    }
   r = copy( x );                           // bitmap - array
   for( const std::uint16_t v : y.a ) {
    const std::uint64_t bit = std::uint64_t( 1 ) << ( v & 63 );
    r.card -= std::uint32_t( ( r.w[ v >> 6 ] & bit ) != 0 );
    r.w[ v >> 6 ] &= ~bit;
    }
   if( r.card <= ARRAY )
    toArray( r );
   return( r );
   }
  else {
   r = copy( m );
   for( const std::uint16_t v : s.a ) {
    const std::uint64_t bit = std::uint64_t( 1 ) << ( v & 63 );
    const bool had = ( r.w[ v >> 6 ] & bit ) != 0;
    if constexpr( O == Op::OR ) {
     r.card += std::uint32_t( ! had );
     r.w[ v >> 6 ] |= bit;
     }
    else {
     r.card = had ? r.card - 1 : r.card + 1;
     r.w[ v >> 6 ] ^= bit;
     }
    }
   if constexpr( O == Op::XOR )
    if( r.card <= ARRAY )
     toArray( r );
   return( r );
   }
  }
 // two arrays, merged with no branch
 const std::uint16_t * a = x.a.data() , * b = y.a.data();
 const std::size_t na = x.a.size() , nb = y.a.size();
 auto & out = r.a;                          // room for 8 more
 out.resize( ( O == Op::AND ? std::min( na , nb ) : O == Op::ANDNOT ? na :
               na + nb ) + 8 );
 std::size_t k = 0;
 if constexpr( O == Op::AND ) {
  if( std::min( na , nb ) * 64 < std::max( na , nb ) ) {   // galloping
   const std::uint16_t * s = na < nb ? a : b , * l = na < nb ? b : a;
   const std::size_t ns = std::min( na , nb ) , nl = std::max( na , nb );
   for( std::size_t q = 0 , lo = 0 ; ( q < ns ) && ( lo < nl ) ; ++q ) {
    const std::uint16_t v = s[ q ];
    std::size_t d = 1;
    while( ( lo + d < nl ) && ( l[ lo + d ] < v ) ) d *= 2;
    lo = std::size_t( std::lower_bound( l + lo , l + std::min( nl , lo + d +
                                                               1 ) , v ) -
                      l );
    if( ( lo < nl ) && ( l[ lo ] == v ) )
     out[ k++ ] = v;
    }
   }
  else
   k = detail::intersect16( a , na , b , nb , out.data() );
  }
 else if constexpr( O == Op::OR )
  k = detail::union16( a , na , b , nb , out.data() );
 else if constexpr( O == Op::ANDNOT )
  k = detail::difference16( a , na , b , nb , out.data() );
 else if( na + nb <= 512 ) {               // short: one at a time
  std::size_t i = 0 , j = 0;
  while( ( i < na ) && ( j < nb ) ) {
   const std::uint16_t p = a[ i ] , q = b[ j ];
   out[ k ] = p < q ? p : q;
   k += p != q;
   i += p <= q;
   j += q <= p;
   }
  for( ; i < na ; ++i ) out[ k++ ] = a[ i ];
  for( ; j < nb ; ++j ) out[ k++ ] = b[ j ];
  }
 else {                                     // the two differences merged
  std::vector< std::uint16_t , detail::UninitAllocator< std::uint16_t > >
   d1( na + 8 ) , d2( nb + 8 );
  const std::size_t n1 = detail::difference16( a , na , b , nb ,
                                               d1.data() );
  const std::size_t n2 = detail::difference16( b , nb , a , na ,
                                               d2.data() );
  k = detail::union16( d1.data() , n1 , d2.data() , n2 , out.data() );
  }
 out.resize( k );
 r.card = std::uint32_t( k );
 if( r.card > ARRAY )
  toBitmap( r );
 return( r );
 }

template< RoaringBitmap::Op O >
inline RoaringBitmap RoaringBitmap::combine( const RoaringBitmap & A ,
                                             const RoaringBitmap & B )
{
 RoaringBitmap C;
 std::size_t i = 0 , j = 0;
 const std::size_t na = A.f_B.size() , nb = B.f_B.size();
 C.f_B.reserve( O == Op::AND ? std::min( na , nb ) :
                O == Op::ANDNOT ? na : na + nb );
 while( ( i < na ) && ( j < nb ) ) {
  const Block & x = A.f_B[ i ] , & y = B.f_B[ j ];
  if( x.key < y.key ) {
   if constexpr( O != Op::AND )
    C.f_B.push_back( copy( x ) );
   ++i;
   }
  else if( y.key < x.key ) {
   if constexpr( ( O == Op::OR ) || ( O == Op::XOR ) )
    C.f_B.push_back( copy( y ) );
   ++j;
   }
  else {
   Block z = combine< O >( x , y );
   if( z.card )
    C.f_B.push_back( std::move( z ) );
   ++i;
   ++j;
   }
  }
 if constexpr( O != Op::AND )
  for( ; i < na ; ++i ) C.f_B.push_back( copy( A.f_B[ i ] ) );
 if constexpr( ( O == Op::OR ) || ( O == Op::XOR ) )
  for( ; j < nb ; ++j ) C.f_B.push_back( copy( B.f_B[ j ] ) );
 for( const Block & b : C.f_B ) C.f_size += b.card;
 C.rekey();
 ADS_INVARIANT( ( C.f_size > 4096 ) || C.valid() );
 return( C );
 }

inline RoaringBitmap setUnion( const RoaringBitmap & A ,
                               const RoaringBitmap & B )
{
 return( RoaringBitmap::combine< RoaringBitmap::Op::OR >( A , B ) );
 }

inline RoaringBitmap setIntersection( const RoaringBitmap & A ,
                                      const RoaringBitmap & B )
{
 return( RoaringBitmap::combine< RoaringBitmap::Op::AND >( A , B ) );
 }

inline RoaringBitmap setDifference( const RoaringBitmap & A ,
                                    const RoaringBitmap & B )
{
 return( RoaringBitmap::combine< RoaringBitmap::Op::ANDNOT >( A , B ) );
 }

inline RoaringBitmap setSymmetricDifference( const RoaringBitmap & A ,
                                             const RoaringBitmap & B )
{
 return( RoaringBitmap::combine< RoaringBitmap::Op::XOR >( A , B ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE ITEMS ---------------------------------*/
/*--------------------------------------------------------------------------*/

inline RoaringBitmap::RoaringBitmap( const RoaringBitmap & o )
 : f_K( o.f_K ) , f_size( o.f_size )
{
 f_B.reserve( o.f_B.size() );
 for( const Block & b : o.f_B ) f_B.push_back( copy( b ) );
 }

inline RoaringBitmap & RoaringBitmap::operator=( RoaringBitmap o ) noexcept
{
 swap( o );
 return( *this );
 }

inline RoaringBitmap::RoaringBitmap( const std::uint32_t * a , std::size_t n )
{
 ADS_AUDIT( std::adjacent_find( a , a + n , std::greater_equal<>() ) ==
            a + n );
 for( std::size_t i = 0 ; i < n ; ) {       // a container per chunk
  const std::uint16_t key = std::uint16_t( a[ i ] >> 16 );
  std::size_t j = i;
  while( ( j < n ) && ( ( a[ j ] >> 16 ) == key ) ) ++j;
  Block b;
  b.key = key;
  b.card = std::uint32_t( j - i );
  if( b.card > ARRAY ) {
   b.w.reset( new std::uint64_t[ WORDS ]() );
   for( std::size_t t = i ; t < j ; ++t )
    b.w[ ( a[ t ] & 0xFFFF ) >> 6 ] |= std::uint64_t( 1 ) << ( a[ t ] & 63 );
   }
  else {
   b.a.resize( b.card );
   for( std::size_t t = i ; t < j ; ++t )
    b.a[ t - i ] = std::uint16_t( a[ t ] );
   }
  f_B.push_back( std::move( b ) );
  i = j;
  }
 f_size = n;
 rekey();
 ADS_INVARIANT( ( f_size > 4096 ) || valid() );
 }

inline void RoaringBitmap::rekey( void )
{
 f_K.resize( f_B.size() );
 for( std::size_t i = 0 ; i < f_B.size() ; ++i ) f_K[ i ] = f_B[ i ].key;
 }

inline std::size_t RoaringBitmap::find( std::uint16_t key ) const
{
 std::size_t n = f_K.size();                // the first chunk not smaller,
 if( n == 0 )                               // with no branch
  return( 0 );
 const std::uint16_t * base = f_K.data();
 while( n > 1 ) {
  const std::size_t h = n / 2;
  base = base[ h ] < key ? base + h : base;
  n -= h;
  }
 return( std::size_t( base - f_K.data() ) + ( *base < key ? 1 : 0 ) );
 }

inline bool RoaringBitmap::contains( std::uint32_t x ) const
{
 const std::uint16_t key = std::uint16_t( x >> 16 ) , v = std::uint16_t( x );
 const std::size_t i = find( key );
 if( ( i == f_B.size() ) || ( f_B[ i ].key != key ) )
  return( false );
 const Block & b = f_B[ i ];
 if( b.bits() )
  return( ( b.w[ v >> 6 ] >> ( v & 63 ) ) & 1 );
 const std::uint16_t * base = b.a.data();   // a search with no branch
 for( std::size_t n = b.a.size() ; n > 1 ; ) {
  const std::size_t h = n / 2;
  base = base[ h ] <= v ? base + h : base;
  n -= h;
  }
 return( *base == v );
 }

inline bool RoaringBitmap::insert( std::uint32_t x )
{
 const std::uint16_t key = std::uint16_t( x >> 16 ) , v = std::uint16_t( x );
 const std::size_t i = find( key );
 if( ( i == f_B.size() ) || ( f_B[ i ].key != key ) ) {
  Block b;
  b.key = key;
  b.card = 1;
  b.a.push_back( v );
  f_B.insert( f_B.begin() + long( i ) , std::move( b ) );
  f_K.insert( f_K.begin() + long( i ) , key );
  ++f_size;
  ADS_POST( contains( x ) );
  return( true );
  }
 Block & b = f_B[ i ];
 bool added;
 if( b.bits() ) {
  const std::uint64_t bit = std::uint64_t( 1 ) << ( v & 63 );
  added = ! ( b.w[ v >> 6 ] & bit );
  b.w[ v >> 6 ] |= bit;
  }
 else {
  const auto it = std::lower_bound( b.a.begin() , b.a.end() , v );
  added = ( it == b.a.end() ) || ( *it != v );
  if( added )
   b.a.insert( it , v );
  }
 if( added ) {
  ++b.card;
  ++f_size;
  if( ! b.bits() && ( b.card > ARRAY ) )
   toBitmap( b );
  }
 ADS_POST( contains( x ) );
 ADS_INVARIANT( ( f_size > 4096 ) || valid() );
 return( added );
 }

inline bool RoaringBitmap::remove( std::uint32_t x )
{
 const std::uint16_t key = std::uint16_t( x >> 16 ) , v = std::uint16_t( x );
 const std::size_t i = find( key );
 if( ( i == f_B.size() ) || ( f_B[ i ].key != key ) )
  return( false );
 Block & b = f_B[ i ];
 bool gone;
 if( b.bits() ) {
  const std::uint64_t bit = std::uint64_t( 1 ) << ( v & 63 );
  gone = ( b.w[ v >> 6 ] & bit ) != 0;
  b.w[ v >> 6 ] &= ~bit;
  }
 else {
  const auto it = std::lower_bound( b.a.begin() , b.a.end() , v );
  gone = ( it != b.a.end() ) && ( *it == v );
  if( gone )
   b.a.erase( it );
  }
 if( gone ) {
  --b.card;
  --f_size;
  if( b.card == 0 ) {
   f_B.erase( f_B.begin() + long( i ) );
   f_K.erase( f_K.begin() + long( i ) );
   }
  else if( b.bits() && ( b.card <= ARRAY ) )
   toArray( b );
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( ( f_size > 4096 ) || valid() );
 return( gone );
 }

inline std::size_t RoaringBitmap::size( void ) const
{
 return( f_size );
 }

inline bool RoaringBitmap::empty( void ) const
{
 return( f_size == 0 );
 }

inline void RoaringBitmap::clear( void )
{
 f_B.clear();
 f_K.clear();
 f_size = 0;
 }

inline void RoaringBitmap::swap( RoaringBitmap & o ) noexcept
{
 f_B.swap( o.f_B );
 f_K.swap( o.f_K );
 std::swap( f_size , o.f_size );
 }

inline std::size_t RoaringBitmap::containers( void ) const
{
 return( f_B.size() );
 }

inline std::size_t RoaringBitmap::bitmaps( void ) const
{
 return( std::size_t( std::count_if( f_B.begin() , f_B.end() ,
                                     []( const Block & b ) {
                                      return( b.bits() ); } ) ) );
 }

inline std::size_t RoaringBitmap::bytes( void ) const
{
 std::size_t s = f_B.size() * sizeof( Block );
 for( const Block & b : f_B )
  s += b.bits() ? WORDS * sizeof( std::uint64_t ) :
                  b.a.size() * sizeof( std::uint16_t );
 return( s );
 }

template< class F >
void RoaringBitmap::forEach( F && f ) const
{
 for( const Block & b : f_B ) {
  const std::uint32_t hi = std::uint32_t( b.key ) << 16;
  if( b.bits() ) {
   for( std::size_t i = 0 ; i < WORDS ; ++i )
    for( std::uint64_t v = b.w[ i ] ; v ; v &= v - 1 )
     f( hi | std::uint32_t( i * 64 + std::size_t(
                             std::countr_zero( v ) ) ) );
   }
  else
   for( const std::uint16_t v : b.a )
    f( hi | v );
  }
 }

inline std::vector< std::uint32_t > RoaringBitmap::items( void ) const
{
 std::vector< std::uint32_t > V;
 V.reserve( f_size );
 forEach( [ & ]( std::uint32_t x ) { V.push_back( x ); } );
 return( V );
 }

inline bool RoaringBitmap::valid( void ) const
{
 std::size_t n = 0;
 for( std::size_t i = 0 ; i < f_B.size() ; ++i ) {
  const Block & b = f_B[ i ];
  if( ( b.card == 0 ) || ( ( i > 0 ) && ( f_B[ i - 1 ].key >= b.key ) ) ||
      ( f_K.size() != f_B.size() ) || ( f_K[ i ] != b.key ) )
   return( false );
  if( b.bits() ) {
   std::uint32_t c = 0;
   for( std::size_t k = 0 ; k < WORDS ; ++k )
    c += std::uint32_t( std::popcount( b.w[ k ] ) );
   if( ( c != b.card ) || ( c <= ARRAY ) || ! b.a.empty() )
    return( false );
   }
  else if( ( b.a.size() != b.card ) || ( b.card > ARRAY ) ||
           ( std::adjacent_find( b.a.begin() , b.a.end() ,
                                 std::greater_equal<>() ) != b.a.end() ) )
   return( false );
  n += b.card;
  }
 return( n == f_size );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE ITERATORS -----------------------------*/
/*--------------------------------------------------------------------------*/

inline std::size_t RoaringBitmap::rank( std::uint32_t x ) const
{
 const std::uint16_t key = std::uint16_t( x >> 16 ) , v = std::uint16_t( x );
 const std::size_t i = find( key );
 std::size_t r = 0;
 for( std::size_t b = 0 ; b < i ; ++b ) r += f_B[ b ].card;
 if( ( i == f_B.size() ) || ( f_B[ i ].key != key ) )
  return( r );
 const Block & b = f_B[ i ];
 if( b.bits() ) {                           // the bits below v
  for( std::size_t k = 0 ; k < std::size_t( v >> 6 ) ; ++k )
   r += std::size_t( std::popcount( b.w[ k ] ) );
  r += std::size_t( std::popcount( b.w[ v >> 6 ] &
                                   ( ( std::uint64_t( 1 ) << ( v & 63 ) ) -
                                     1 ) ) );
  }
 else
  r += std::size_t( std::lower_bound( b.a.begin() , b.a.end() , v ) -
                    b.a.begin() );
 ADS_POST( r <= f_size );
 return( r );
 }

inline auto RoaringBitmap::lowerBound( std::uint32_t x ) const
 -> const_iterator
{
 const std::uint16_t key = std::uint16_t( x >> 16 ) , v = std::uint16_t( x );
 const_iterator it;
 it.f_s = this;
 it.f_b = find( key );
 it.f_i = 0;
 if( ( it.f_b < f_B.size() ) && ( f_B[ it.f_b ].key == key ) ) {
  const Block & b = f_B[ it.f_b ];
  if( b.bits() ) {                          // the word of v, from v on
   const std::uint64_t w = b.w[ v >> 6 ] & ( ~std::uint64_t( 0 ) <<
                                             ( v & 63 ) );
   if( w ) {
    it.f_i = v >> 6;
    it.f_word = w;
    it.f_base = ( std::uint32_t( key ) << 16 ) | std::uint32_t( it.f_i * 64 );
    it.f_x = it.f_base + std::uint32_t( std::countr_zero( w ) );
    return( it );
    }
   it.f_i = std::size_t( v >> 6 ) + 1;
   }
  else
   it.f_i = std::size_t( std::lower_bound( b.a.begin() , b.a.end() , v ) -
                         b.a.begin() );
  }
 it.settle();
 return( it );
 }

inline auto RoaringBitmap::select( std::size_t i ) const -> const_iterator
{
 if( i >= f_size )
  return( end() );
 std::size_t b = 0;
 while( i >= f_B[ b ].card ) i -= f_B[ b++ ].card;   // the container
 const Block & c = f_B[ b ];
 const std::uint32_t hi = std::uint32_t( c.key ) << 16;
 if( ! c.bits() )
  return( lowerBound( hi | c.a[ i ] ) );
 std::size_t k = 0;                         // the word, then the bit
 while( i >= std::size_t( std::popcount( c.w[ k ] ) ) )
  i -= std::size_t( std::popcount( c.w[ k++ ] ) );
 std::uint64_t w = c.w[ k ];
 for( ; i > 0 ; --i ) w &= w - 1;
 return( lowerBound( hi | std::uint32_t( k * 64 + std::size_t(
                                          std::countr_zero( w ) ) ) ) );
 }

inline RoaringBitmap::const_iterator::const_iterator( const RoaringBitmap * s ,
                                                      std::size_t b )
 : f_s( s ) , f_b( b )
{
 settle();
 }

inline void RoaringBitmap::const_iterator::settle( void )
{
 // f_i is the place in an array, or the word of a bitmap, to look at
 for( ; f_b < f_s->f_B.size() ; ++f_b , f_i = 0 ) {
  const Block & b = f_s->f_B[ f_b ];
  const std::uint32_t hi = std::uint32_t( b.key ) << 16;
  if( ! b.bits() ) {
   f_word = 0;
   if( f_i < b.a.size() ) {
    f_x = hi | b.a[ f_i ];
    return;
    }
   continue;
   }
  for( ; f_i < WORDS ; ++f_i )
   if( b.w[ f_i ] ) {
    f_word = b.w[ f_i ];
    f_base = hi | std::uint32_t( f_i * 64 );
    f_x = f_base + std::uint32_t( std::countr_zero( f_word ) );
    return;
    }
  }
 f_i = 0;
 f_word = 0;
 f_x = 0;
 }

#undef ADS_ROAR_KERNEL

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RoaringBitmap.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
