/*--------------------------------------------------------------------------*/
/*------------------------ File VectorQuickSort.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The quicksort of VectorQuickSort.h: the registers of each instruction
 * set as a policy of a few operations, and the partition, the network and
 * the recursion written once on them. The keys are read and written only
 * by memcpy() or by the loads and stores of the registers, so that they
 * may live in the memory of the floating point numbers they come from.
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

namespace detail::vqs {

/// a function inlined whatever its size, so that the registers of a
/// network stay registers
#if defined( __GNUC__ )
 #define ADS_VQS_INLINE [[gnu::always_inline]] inline
#else
 #define ADS_VQS_INLINE inline
#endif

/// the key at p
template< class K >
inline K get( const K * p )
{
 K k;
 std::memcpy( &k , p , sizeof( K ) );
 return( k );
 }

/// the key k to p
template< class K >
inline void put( K * p , K k )
{
 std::memcpy( p , &k , sizeof( K ) );
 }

/// the mask of the lanes of a register of N lanes that keep the minimum in
/// the step of distance J of the bitonic network on blocks of K2 lanes; a
/// lane i keeps the minimum if it is the lower of its pair in a block
/// sorted upwards, or the higher in one sorted downwards, the direction
/// of the block given by Down when K2 >= N
template< unsigned N , unsigned J , unsigned K2 , bool Down >
constexpr std::uint32_t minMask()
{
 std::uint32_t m = 0;
 for( unsigned i = 0 ; i < N ; ++i ) {
  const bool down = K2 < N ? ( i & K2 ) != 0 : Down;
  if( ( ( i & J ) == 0 ) != down )
   m |= std::uint32_t( 1 ) << i;
  }
 return( m );
 }

/// ------------------------------------------------------- portable lanes

/// a register as an array of 32 / sizeof( K ) lanes, for any machine
template< class Key >
struct Lanes {
 using K = Key;
 static constexpr unsigned N = 32 / sizeof( K );
 struct R { K v[ N ]; };

 static R load( const K * p ) {
  R r;
  std::memcpy( r.v , p , sizeof( r.v ) );
  return( r );
  }
 static void store( K * p , const R & r ) {
  std::memcpy( p , r.v , sizeof( r.v ) );
  }
 static R set1( K k ) {
  R r;
  for( unsigned i = 0 ; i < N ; ++i ) r.v[ i ] = k;
  return( r );
  }
 /// the c < N first lanes from p, the others the largest key
 static R loadPad( const K * p , std::size_t c ) {
  R r = set1( std::numeric_limits< K >::max() );
  std::memcpy( r.v , p , c * sizeof( K ) );
  return( r );
  }
 static void storeN( K * p , const R & r , std::size_t c ) {
  std::memcpy( p , r.v , c * sizeof( K ) );
  }
 static R min( const R & a , const R & b ) {
  R r;
  for( unsigned i = 0 ; i < N ; ++i )
   r.v[ i ] = b.v[ i ] < a.v[ i ] ? b.v[ i ] : a.v[ i ];
  return( r );
  }
 static R max( const R & a , const R & b ) {
  R r;
  for( unsigned i = 0 ; i < N ; ++i )
   r.v[ i ] = a.v[ i ] < b.v[ i ] ? b.v[ i ] : a.v[ i ];
  return( r );
  }
 static K hmin( const R & a ) {
  K k = a.v[ 0 ];
  for( unsigned i = 1 ; i < N ; ++i ) k = a.v[ i ] < k ? a.v[ i ] : k;
  return( k );
  }
 static K hmax( const R & a ) {
  K k = a.v[ 0 ];
  for( unsigned i = 1 ; i < N ; ++i ) k = k < a.v[ i ] ? a.v[ i ] : k;
  return( k );
  }
 /// lane i takes lane i ^ J
 template< unsigned J >
 static R swap( const R & a ) {
  R r;
  for( unsigned i = 0 ; i < N ; ++i ) r.v[ i ] = a.v[ i ^ J ];
  return( r );
  }
 /// the lanes of M from mn, the others from mx
 template< std::uint32_t M >
 static R exchange( const R & a , const R & b ) {
  return( pick< M >( min( a , b ) , max( a , b ) ) );
  }
 template< std::uint32_t M >
 static R pick( const R & mn , const R & mx ) {
  R r;
  for( unsigned i = 0 ; i < N ; ++i )
   r.v[ i ] = ( M >> i ) & 1 ? mn.v[ i ] : mx.v[ i ];
  return( r );
  }
 /// the keys of a below p at l, the others ending at r; their count
 static unsigned part( const R & a , K p , K * l , K * r ) {
  K t[ 2 * N ];
  unsigned nl = 0 , nr = 0;
  for( unsigned i = 0 ; i < N ; ++i ) {
   const bool lt = a.v[ i ] < p;
   t[ nl ] = a.v[ i ];
   t[ 2 * N - 1 - nr ] = a.v[ i ];
   nl += lt;
   nr += ! lt;
   }
  std::memcpy( l , t , nl * sizeof( K ) );
  std::memcpy( r - nr , t + 2 * N - nr , nr * sizeof( K ) );
  return( nl );
  }
 };

#if ADS_VECTOR_SORT && defined( __AVX512F__ )

/// ------------------------------------------------------------- AVX-512

/// a register of AVX-512 of 16 keys of 32 bits or 8 of 64, signed or not,
/// K the type of a key
template< class Key >
struct Avx512 {
 using K = Key;
 static constexpr bool W32 = sizeof( K ) == 4;
 static constexpr bool S = std::is_signed_v< K >;
 static constexpr unsigned N = 64 / sizeof( K );
 using R = __m512i;
 using Mask = std::conditional_t< W32 , __mmask16 , __mmask8 >;

 static R load( const K * p ) { return( _mm512_loadu_si512( p ) ); }
 static void store( K * p , R r ) { _mm512_storeu_si512( p , r ); }
 static R set1( K k ) {
  if constexpr( W32 ) return( _mm512_set1_epi32( int( k ) ) );
  else return( _mm512_set1_epi64( ( long long )( k ) ) );
  }
 static Mask first( std::size_t c ) { return( Mask( ( 1u << c ) - 1 ) ); }
 static R loadPad( const K * p , std::size_t c ) {
  const R m = set1( std::numeric_limits< K >::max() );
  if constexpr( W32 ) return( _mm512_mask_loadu_epi32( m , first( c ) , p ) );
  else return( _mm512_mask_loadu_epi64( m , first( c ) , p ) );
  }
 static void storeN( K * p , R r , std::size_t c ) {
  if constexpr( W32 ) _mm512_mask_storeu_epi32( p , first( c ) , r );
  else _mm512_mask_storeu_epi64( p , first( c ) , r );
  }
 static R min( R a , R b ) {
  if constexpr( W32 && S ) return( _mm512_min_epi32( a , b ) );
  else if constexpr( W32 ) return( _mm512_min_epu32( a , b ) );
  else if constexpr( S ) return( _mm512_min_epi64( a , b ) );
  else return( _mm512_min_epu64( a , b ) );
  }
 static R max( R a , R b ) {
  if constexpr( W32 && S ) return( _mm512_max_epi32( a , b ) );
  else if constexpr( W32 ) return( _mm512_max_epu32( a , b ) );
  else if constexpr( S ) return( _mm512_max_epi64( a , b ) );
  else return( _mm512_max_epu64( a , b ) );
  }
 /// the lanes of f in which a < b
 static Mask less( Mask f , R a , R b ) {
  if constexpr( W32 && S ) return( _mm512_mask_cmplt_epi32_mask( f , a , b ) );
  else if constexpr( W32 ) return( _mm512_mask_cmplt_epu32_mask( f , a , b ) );
  else if constexpr( S ) return( _mm512_mask_cmplt_epi64_mask( f , a , b ) );
  else return( _mm512_mask_cmplt_epu64_mask( f , a , b ) );
  }
 static K hmin( R a ) {
  if constexpr( W32 && S ) return( K( _mm512_reduce_min_epi32( a ) ) );
  else if constexpr( W32 ) return( K( _mm512_reduce_min_epu32( a ) ) );
  else if constexpr( S ) return( K( _mm512_reduce_min_epi64( a ) ) );
  else return( K( _mm512_reduce_min_epu64( a ) ) );
  }
 static K hmax( R a ) {
  if constexpr( W32 && S ) return( K( _mm512_reduce_max_epi32( a ) ) );
  else if constexpr( W32 ) return( K( _mm512_reduce_max_epu32( a ) ) );
  else if constexpr( S ) return( K( _mm512_reduce_max_epi64( a ) ) );
  else return( K( _mm512_reduce_max_epu64( a ) ) );
  }
 template< unsigned J >
 static R swap( R a ) {
  constexpr unsigned L = W32 ? J : 2 * J;   // in lanes of 32 bits
  if constexpr( L == 1 ) return( _mm512_shuffle_epi32( a , _MM_PERM_CDAB ) );
  else if constexpr( L == 2 )
   return( _mm512_shuffle_epi32( a , _MM_PERM_BADC ) );
  else if constexpr( L == 4 ) return( _mm512_shuffle_i32x4( a , a , 0xB1 ) );
  else return( _mm512_shuffle_i32x4( a , a , 0x4E ) );
  }
 template< std::uint32_t M >
 static R pick( R mn , R mx ) {
  if constexpr( W32 ) return( _mm512_mask_blend_epi32( Mask( M ) , mx , mn ) );
  else return( _mm512_mask_blend_epi64( Mask( M ) , mx , mn ) );
  }
 /// the lanes of M take the minimum of a and b, the others the maximum
 template< std::uint32_t M >
 static R exchange( R a , R b ) {
  const R x = max( a , b );
  if constexpr( W32 && S ) return( _mm512_mask_min_epi32( x , M , a , b ) );
  else if constexpr( W32 ) return( _mm512_mask_min_epu32( x , M , a , b ) );
  else if constexpr( S )
   return( _mm512_mask_min_epi64( x , Mask( M ) , a , b ) );
  else return( _mm512_mask_min_epu64( x , Mask( M ) , a , b ) );
  }
 /// the lanes m of x replaced by their minimum, or maximum, with those of a
 static R minIn( R x , Mask m , R a ) {
  if constexpr( W32 && S ) return( _mm512_mask_min_epi32( x , m , x , a ) );
  else if constexpr( W32 ) return( _mm512_mask_min_epu32( x , m , x , a ) );
  else if constexpr( S ) return( _mm512_mask_min_epi64( x , m , x , a ) );
  else return( _mm512_mask_min_epu64( x , m , x , a ) );
  }
 static R maxIn( R x , Mask m , R a ) {
  if constexpr( W32 && S ) return( _mm512_mask_max_epi32( x , m , x , a ) );
  else if constexpr( W32 ) return( _mm512_mask_max_epu32( x , m , x , a ) );
  else if constexpr( S ) return( _mm512_mask_max_epi64( x , m , x , a ) );
  else return( _mm512_mask_max_epu64( x , m , x , a ) );
  }
 /// the keys of a in the lanes m at l
 static void compress( K * l , Mask m , R a ) {
  if constexpr( W32 ) _mm512_mask_compressstoreu_epi32( l , m , a );
  else _mm512_mask_compressstoreu_epi64( l , m , a );
  }
 /// part() of the first c < N keys at q
 static unsigned partN( const K * q , std::size_t c , K p , K * l , K * r ) {
  const Mask f = first( c );
  R a;
  if constexpr( W32 ) a = _mm512_maskz_loadu_epi32( f , q );
  else a = _mm512_maskz_loadu_epi64( f , q );
  const Mask m = less( f , a , set1( p ) ) , g = Mask( f & ~m );
  const unsigned nl = unsigned( std::popcount( unsigned( m ) ) );
  const unsigned nr = unsigned( std::popcount( unsigned( g ) ) );
  compress( l , m , a );
  compress( r - nr , g , a );
  return( nl );
  }
 /// the keys of a below p at l and those above it ending at r, the others
 /// equal to p left out: the counts of the two
 static std::pair< unsigned , unsigned > part3( R a , K p , K * l , K * r ,
                                                R * ex ) {
  const R q = set1( p );
  const Mask m = less( Mask( -1 ) , a , q ) , g = less( Mask( -1 ) , q , a );
  ex[ 0 ] = minIn( ex[ 0 ] , m , a );
  ex[ 1 ] = maxIn( ex[ 1 ] , m , a );
  ex[ 2 ] = minIn( ex[ 2 ] , g , a );
  ex[ 3 ] = maxIn( ex[ 3 ] , g , a );
  const unsigned nl = unsigned( std::popcount( unsigned( m ) ) );
  const unsigned ng = unsigned( std::popcount( unsigned( g ) ) );
  compress( l , m , a );
  compress( r - ng , g , a );
  return( std::pair( nl , ng ) );
  }
 /// part3() of the first c < N keys at q
 static std::pair< unsigned , unsigned > partN3( const K * q , std::size_t c ,
                                                 K p , K * l , K * r ,
                                                 R * ex ) {
  const Mask f = first( c );
  R a;
  if constexpr( W32 ) a = _mm512_maskz_loadu_epi32( f , q );
  else a = _mm512_maskz_loadu_epi64( f , q );
  const R v = set1( p );
  const Mask m = less( f , a , v ) , g = less( f , v , a );
  ex[ 0 ] = minIn( ex[ 0 ] , m , a );
  ex[ 1 ] = maxIn( ex[ 1 ] , m , a );
  ex[ 2 ] = minIn( ex[ 2 ] , g , a );
  ex[ 3 ] = maxIn( ex[ 3 ] , g , a );
  const unsigned nl = unsigned( std::popcount( unsigned( m ) ) );
  const unsigned ng = unsigned( std::popcount( unsigned( g ) ) );
  compress( l , m , a );
  compress( r - ng , g , a );
  return( std::pair( nl , ng ) );
  }
 /// the keys of a below p at l, the others ending at r, compressed into
 /// the memory; their count
 static unsigned part( R a , K p , K * l , K * r ) {
  const Mask m = less( Mask( -1 ) , a , set1( p ) );
  const unsigned nl = unsigned( std::popcount( unsigned( m ) ) );
  compress( l , m , a );
  compress( r - ( N - nl ) , Mask( ~m ) , a );
  return( nl );
  }
 };

template< class K >
using Best = Avx512< K >;

/// the keys of T: its own bits if it is an integer, which AVX-512 compares
/// signed or not, and a signed integer of its order otherwise
template< class T >
using KeyOf = std::conditional_t< std::is_integral_v< T > ,
 std::conditional_t< std::is_signed_v< T > ,
  std::conditional_t< sizeof( T ) == 4 , std::int32_t , std::int64_t > ,
  std::conditional_t< sizeof( T ) == 4 , std::uint32_t , std::uint64_t > > ,
 std::conditional_t< sizeof( T ) == 4 , std::int32_t , std::int64_t > >;

#elif ADS_VECTOR_SORT

/// ---------------------------------------------------------------- AVX2

/// the permutations that put the lanes of a mask of W lanes first, in
/// their order, and the others after them, as the indices of the lanes of
/// 32 bits that they take (two per lane of 64 bits if W = 4)
template< unsigned W >
struct PartTable {
 std::array< std::uint64_t , 1u << W > idx{};

 constexpr PartTable() {
  for( unsigned m = 0 ; m < ( 1u << W ) ; ++m ) {
   std::uint64_t t = 0;
   unsigned k = 0;
   for( int pass = 0 ; pass < 2 ; ++pass )
    for( unsigned i = 0 ; i < W ; ++i )
     if( ( ( m >> i ) & 1 ) == unsigned( pass == 0 ) )
      for( unsigned h = 0 ; h < 8 / W ; ++h , ++k )
       t |= std::uint64_t( i * ( 8 / W ) + h ) << ( 8 * k );
   idx[ m ] = t;
   }
  }
 };

template< unsigned W >
inline constexpr PartTable< W > kPart{};

/// the permutation of the table for the mask m, as indices of 32 bits
template< unsigned W >
inline __m256i partIndex( unsigned m )
{
 return( _mm256_cvtepu8_epi32( _mm_cvtsi64_si128(
  std::int64_t( kPart< W >.idx[ m ] ) ) ) );
 }

/// a register of AVX2 of 8 keys of 32 bits
struct Avx2I32 {
 using K = std::int32_t;
 static constexpr unsigned N = 8;
 using R = __m256i;

 static R load( const K * p ) {
  return( _mm256_loadu_si256( reinterpret_cast< const __m256i * >( p ) ) );
  }
 static void store( K * p , R r ) {
  _mm256_storeu_si256( reinterpret_cast< __m256i * >( p ) , r );
  }
 static R set1( K k ) { return( _mm256_set1_epi32( k ) ); }
 static R first( std::size_t c ) {
  return( _mm256_cmpgt_epi32( set1( int( c ) ) ,
                              _mm256_setr_epi32( 0 , 1 , 2 , 3 , 4 , 5 ,
                                                 6 , 7 ) ) );
  }
 static R loadPad( const K * p , std::size_t c ) {
  const R m = first( c );
  return( _mm256_blendv_epi8( set1( INT32_MAX ) ,
                              _mm256_maskload_epi32( p , m ) , m ) );
  }
 static void storeN( K * p , R r , std::size_t c ) {
  _mm256_maskstore_epi32( p , first( c ) , r );
  }
 static R min( R a , R b ) { return( _mm256_min_epi32( a , b ) ); }
 static R max( R a , R b ) { return( _mm256_max_epi32( a , b ) ); }
 static K hmin( R a ) {
  a = min( a , swap< 4 >( a ) );
  a = min( a , swap< 2 >( a ) );
  return( _mm256_cvtsi256_si32( min( a , swap< 1 >( a ) ) ) );
  }
 static K hmax( R a ) {
  a = max( a , swap< 4 >( a ) );
  a = max( a , swap< 2 >( a ) );
  return( _mm256_cvtsi256_si32( max( a , swap< 1 >( a ) ) ) );
  }
 template< unsigned J >
 static R swap( R a ) {
  if constexpr( J == 1 ) return( _mm256_shuffle_epi32( a , 0xB1 ) );
  else if constexpr( J == 2 ) return( _mm256_shuffle_epi32( a , 0x4E ) );
  else return( _mm256_permute2x128_si256( a , a , 1 ) );
  }
 template< std::uint32_t M >
 static R pick( R mn , R mx ) {
  return( _mm256_blend_epi32( mx , mn , int( M ) ) );
  }
 template< std::uint32_t M >
 static R exchange( R a , R b ) {
  return( pick< M >( min( a , b ) , max( a , b ) ) );
  }
 static unsigned part( R a , K p , K * l , K * r ) {
  const unsigned m = unsigned( _mm256_movemask_ps(
   _mm256_castsi256_ps( _mm256_cmpgt_epi32( set1( p ) , a ) ) ) );
  const R b = _mm256_permutevar8x32_epi32( a , partIndex< 8 >( m ) );
  store( l , b );
  store( r - N , b );
  return( unsigned( std::popcount( m ) ) );
  }
 };

/// a register of AVX2 of 4 keys of 64 bits
struct Avx2I64 {
 using K = std::int64_t;
 static constexpr unsigned N = 4;
 using R = __m256i;

 static R load( const K * p ) {
  return( _mm256_loadu_si256( reinterpret_cast< const __m256i * >( p ) ) );
  }
 static void store( K * p , R r ) {
  _mm256_storeu_si256( reinterpret_cast< __m256i * >( p ) , r );
  }
 static R set1( K k ) { return( _mm256_set1_epi64x( k ) ); }
 static R first( std::size_t c ) {
  return( _mm256_cmpgt_epi64( set1( K( c ) ) ,
                              _mm256_setr_epi64x( 0 , 1 , 2 , 3 ) ) );
  }
 static R loadPad( const K * p , std::size_t c ) {
  const R m = first( c );
  return( _mm256_blendv_epi8(
   set1( INT64_MAX ) ,
   _mm256_maskload_epi64( reinterpret_cast< const long long * >( p ) , m ) ,
   m ) );
  }
 static void storeN( K * p , R r , std::size_t c ) {
  _mm256_maskstore_epi64( reinterpret_cast< long long * >( p ) , first( c ) ,
                          r );
  }
 static R min( R a , R b ) {
  return( _mm256_blendv_epi8( a , b , _mm256_cmpgt_epi64( a , b ) ) );
  }
 static R max( R a , R b ) {
  return( _mm256_blendv_epi8( b , a , _mm256_cmpgt_epi64( a , b ) ) );
  }
 static K hmin( R a ) {
  a = min( a , swap< 2 >( a ) );
  return( _mm256_extract_epi64( min( a , swap< 1 >( a ) ) , 0 ) );
  }
 static K hmax( R a ) {
  a = max( a , swap< 2 >( a ) );
  return( _mm256_extract_epi64( max( a , swap< 1 >( a ) ) , 0 ) );
  }
 template< unsigned J >
 static R swap( R a ) {
  if constexpr( J == 1 ) return( _mm256_shuffle_epi32( a , 0x4E ) );
  else return( _mm256_permute2x128_si256( a , a , 1 ) );
  }
 template< std::uint32_t M >
 static R pick( R mn , R mx ) {
  constexpr int m2 = int( ( M & 1 ? 3 : 0 ) | ( M & 2 ? 12 : 0 ) |
                          ( M & 4 ? 48 : 0 ) | ( M & 8 ? 192 : 0 ) );
  return( _mm256_blend_epi32( mx , mn , m2 ) );
  }
 template< std::uint32_t M >
 static R exchange( R a , R b ) {
  return( pick< M >( min( a , b ) , max( a , b ) ) );
  }
 static unsigned part( R a , K p , K * l , K * r ) {
  const unsigned m = unsigned( _mm256_movemask_pd(
   _mm256_castsi256_pd( _mm256_cmpgt_epi64( set1( p ) , a ) ) ) );
  const R b = _mm256_permutevar8x32_epi32( a , partIndex< 4 >( m ) );
  store( l , b );
  store( r - N , b );
  return( unsigned( std::popcount( m ) ) );
  }
 };

template< class K >
using Best = std::conditional_t< sizeof( K ) == 4 , Avx2I32 , Avx2I64 >;

/// the keys of T: a signed integer of its order, the only ones that AVX2
/// compares
template< class T >
using KeyOf = std::conditional_t< sizeof( T ) == 4 , std::int32_t ,
                                  std::int64_t >;

#else

template< class K >
using Best = Lanes< K >;

/// the keys of T: a signed integer of its order
template< class T >
using KeyOf = std::conditional_t< sizeof( T ) == 4 , std::int32_t ,
                                  std::int64_t >;

#endif

/// --------------------------------------------------------- the network

/// the steps of distance J, J / 2, ..., 1 of the stage on blocks of K2
/// lanes of the bitonic network on the M registers r
template< class V , unsigned M , unsigned K2 , unsigned J >
ADS_VQS_INLINE void steps( typename V::R * r )
{
 constexpr unsigned N = V::N;
 if constexpr( J >= N ) {                   // between registers
  for( unsigned a = 0 ; a < M ; ++a ) {
   const unsigned b = a ^ ( J / N );
   if( b > a ) {
    const auto mn = V::min( r[ a ] , r[ b ] );
    const auto mx = V::max( r[ a ] , r[ b ] );
    const bool down = ( ( a * N ) & K2 ) != 0;
    r[ a ] = down ? mx : mn;
    r[ b ] = down ? mn : mx;
    }
   }
  }
 else                                       // within a register
  for( unsigned a = 0 ; a < M ; ++a ) {
   const auto p = V::template swap< J >( r[ a ] );
   if( ( K2 >= N ) && ( ( ( a * N ) & K2 ) != 0 ) )
    r[ a ] = V::template exchange< minMask< N , J , K2 , true >() >( r[ a ] ,
                                                                    p );
   else
    r[ a ] = V::template exchange< minMask< N , J , K2 , false >() >( r[ a ] ,
                                                                     p );
   }
 if constexpr( J > 1 )
  steps< V , M , K2 , J / 2 >( r );
 }

/// the stages on blocks of K2, 2 K2, ..., M N lanes
template< class V , unsigned M , unsigned K2 = 2 >
ADS_VQS_INLINE void network( typename V::R * r )
{
 steps< V , M , K2 , K2 / 2 >( r );
 if constexpr( K2 < M * V::N )
  network< V , M , 2 * K2 >( r );
 }

/// ------------------------------------------------- the column network

/// the number of comparators of the odd-even merge sort of Batcher on M
/// inputs, or their pairs into P
template< unsigned M >
constexpr std::size_t oddEven( std::array< std::uint8_t , 2 > * P )
{
 std::size_t c = 0;
 for( unsigned p = 1 ; p < M ; p <<= 1 )
  for( unsigned k = p ; k >= 1 ; k >>= 1 )
   for( unsigned j = k % p ; j + k < M ; j += 2 * k )
    for( unsigned i = 0 ; ( i < k ) && ( i + j + k < M ) ; ++i )
     if( ( i + j ) / ( 2 * p ) == ( i + j + k ) / ( 2 * p ) ) {
      if( P )
       P[ c ] = { std::uint8_t( i + j ) , std::uint8_t( i + j + k ) };
      ++c;
      }
 return( c );
 }

template< unsigned M >
constexpr auto oddEvenPairs()
{
 std::array< std::array< std::uint8_t , 2 > , oddEven< M >( nullptr ) > P{};
 oddEven< M >( P.data() );
 return( P );
 }

/// the comparators I of the odd-even merge sort on the M registers r,
/// lane by lane: each column sorted
template< class V , unsigned M , std::size_t... I >
ADS_VQS_INLINE void columns( typename V::R * r , std::index_sequence< I... > )
{
 constexpr auto P = oddEvenPairs< M >();
 ( ( [ & ]( auto & x , auto & y ) {
  const auto mn = V::min( x , y );
  y = V::max( x , y );
  x = mn; }( r[ P[ I ][ 0 ] ] , r[ P[ I ][ 1 ] ] ) ) , ... );
 }

/// the lanes of a register of N lanes whose bit B is clear
template< unsigned N , unsigned B >
constexpr std::uint32_t clearMask()
{
 std::uint32_t m = 0;
 for( unsigned i = 0 ; i < N ; ++i )
  if( ( i & B ) == 0 ) m |= std::uint32_t( 1 ) << i;
 return( m );
 }

/// the lanes of a reversed within the blocks of B lanes
template< class V , unsigned B >
ADS_VQS_INLINE typename V::R reverse( typename V::R a )
{
 if constexpr( B > 1 ) {
  a = V::template swap< B / 2 >( a );
  return( reverse< V , B / 2 >( a ) );
  }
 else
  return( a );
 }

/// the steps of distance J, J / 2, ..., 1 of a merge on the M registers r
/// holding the keys by columns, the key of index g = i M + h in the lane i
/// of r[ h ]: all upwards, the lower index taking the minimum
template< class V , unsigned M , unsigned J >
ADS_VQS_INLINE void halves( typename V::R * r )
{
 if constexpr( J >= M ) {                   // within a register
  constexpr unsigned L = J / M;
  for( unsigned h = 0 ; h < M ; ++h )
   r[ h ] = V::template exchange< clearMask< V::N , L >() >(
    r[ h ] , V::template swap< L >( r[ h ] ) );
  }
 else                                       // between registers
  for( unsigned h = 0 ; h < M ; ++h )
   if( ( h ^ J ) > h ) {
    const auto mn = V::min( r[ h ] , r[ h ^ J ] );
    r[ h ^ J ] = V::max( r[ h ] , r[ h ^ J ] );
    r[ h ] = mn;
    }
 if constexpr( J > 1 )
  halves< V , M , J / 2 >( r );
 }

/// the merges of the bitonic network, in its variant with all comparators
/// upwards, on blocks of K2, 2 K2, ..., M N keys held by columns: the index
/// g compared first with \f$ g \oplus ( K_2 - 1 ) \f$, the mirror within
/// its block, i.e., the row M - 1 - h with its lanes reversed within the
/// blocks of K2 / M, and then with \f$ g \oplus J \f$ for J = K2 / 4, ...,
/// 1
template< class V , unsigned M , unsigned K2 >
ADS_VQS_INLINE void merges( typename V::R * r )
{
 constexpr unsigned B = K2 / M;
 constexpr std::uint32_t low = clearMask< V::N , B / 2 >();
 for( unsigned h = 0 ; h < M / 2 ; ++h ) {
  const auto y = reverse< V , B >( r[ M - 1 - h ] );
  const auto mn = V::min( r[ h ] , y ) , mx = V::max( r[ h ] , y );
  r[ h ] = V::template pick< low >( mn , mx );
  r[ M - 1 - h ] = reverse< V , B >( V::template pick< low >( mx , mn ) );
  }
 if constexpr( K2 >= 4 )
  halves< V , M , K2 / 4 >( r );
 if constexpr( K2 < M * V::N )
  merges< V , M , 2 * K2 >( r );
 }

/// the rows s apart of the blocks of N registers from r swapped by blocks of
/// s lanes, a step of their transposition
template< class V , unsigned M , unsigned S >
ADS_VQS_INLINE void transpose( typename V::R * r )
{
 constexpr std::uint32_t c = clearMask< V::N , S >();
 for( unsigned h = 0 ; h < M ; ++h )
  if( ( h & S ) == 0 ) {
   const auto x = r[ h ] , y = r[ h + S ];
   r[ h ] = V::template pick< c >( x , V::template swap< S >( y ) );
   r[ h + S ] = V::template pick< c >( V::template swap< S >( x ) , y );
   }
 if constexpr( S > 1 )
  transpose< V , M , S / 2 >( r );
 }

/// the network on M >= N registers by columns: the columns sorted by the
/// odd-even merge sort, the merges of the bitonic network, and each block
/// of N registers transposed, so that the register q of the sorted keys is
/// the row ( q mod ( M / N ) ) N + floor( q / ( M / N ) ); the order of the
/// registers to the memory is left to the caller
template< class V , unsigned M >
ADS_VQS_INLINE void columnNetwork( typename V::R * r )
{
 columns< V , M >( r , std::make_index_sequence<
                          oddEvenPairs< M >().size() >() );
 merges< V , M , 2 * M >( r );
 transpose< V , M , V::N / 2 >( r );
 }

/// a[ 0 .. n ) sorted in M registers, n <= M N
template< class V , unsigned M >
void smallSort( typename V::K * a , std::size_t n )
{
 typename V::R r[ M ];
 for( unsigned i = 0 ; i < M ; ++i ) {
  const std::size_t s = std::size_t( i ) * V::N;
  r[ i ] = s + V::N <= n ? V::load( a + s ) :
           s < n ? V::loadPad( a + s , n - s ) :
           V::set1( std::numeric_limits< typename V::K >::max() );
  }
 constexpr bool byColumns = ( M >= V::N ) && ( M >= 4 );
 if constexpr( byColumns )
  columnNetwork< V , M >( r );
 else
  network< V , M >( r );
 for( unsigned q = 0 ; q < M ; ++q ) {
  constexpr unsigned b = byColumns ? M / V::N : 1;
  const unsigned i = byColumns ? ( q % b ) * V::N + q / b : q;
  const std::size_t s = std::size_t( q ) * V::N;
  if( s + V::N <= n )
   V::store( a + s , r[ i ] );
  else if( s < n )
   V::storeN( a + s , r[ i ] , n - s );
  }
 }

/// the registers of the largest network, half of those of AVX-512, so that
/// the network keeps them all in registers with room for its temporaries
template< class V >
inline constexpr unsigned kRegs = 16;

/// a[ 0 .. n ) sorted by the smallest network that holds it, n <= kRegs N
template< class V >
void networkSort( typename V::K * a , std::size_t n )
{
 const std::size_t m = ( n + V::N - 1 ) / V::N;
 if( m <= 1 ) smallSort< V , 1 >( a , n );
 else if( m <= 2 ) smallSort< V , 2 >( a , n );
 else if( m <= 4 ) smallSort< V , 4 >( a , n );
 else if( m <= 8 ) smallSort< V , 8 >( a , n );
 else smallSort< V , kRegs< V > >( a , n );
 }

/// ------------------------------------------------------- the recursion

/// heapsort of a[ 0 .. n ), the last resort of a part too deep
template< class K >
void heapKeys( K * a , std::size_t n )
{
 auto sift = [ a ]( std::size_t i , std::size_t m ) {
  const K x = get( a + i );
  for( std::size_t c ; ( c = 2 * i + 1 ) < m ; i = c ) {
   if( ( c + 1 < m ) && ( get( a + c ) < get( a + c + 1 ) ) ) ++c;
   if( ! ( x < get( a + c ) ) ) break;
   put( a + i , get( a + c ) );
   }
  put( a + i , x );
  };
 for( std::size_t i = n / 2 ; i-- > 0 ; ) sift( i , n );
 for( std::size_t m = n ; m-- > 1 ; ) {
  const K x = get( a );
  put( a , get( a + m ) );
  put( a + m , x );
  sift( 0 , m );
  }
 }

/// the vectors that the partition reads from one end at a time
template< class V >
inline constexpr std::size_t kUnroll = 8;

/// the partition of a[ 0 .. n ), n >= 2 U N for U = kUnroll< V >, into
/// the keys below p and the others: their number, and the minimum and the
/// maximum of the part. U vectors are kept aside from each end, and U are
/// read at a time from the end with fewer free places, which then has at
/// least U N; the choice is a branch, mispredicted half of the times on
/// random keys but once for U vectors, while a choice by a mask would chain
/// each load to the stores before it
template< class V , bool Track = false , bool Three = false >
std::size_t partition( typename V::K * a , std::size_t n , typename V::K p ,
                       typename V::K * lo = nullptr ,
                       typename V::K * hi = nullptr ,
                       std::size_t * gt = nullptr , bool * single = nullptr )
{
 using K = typename V::K;
 using R = typename V::R;
 constexpr std::size_t N = V::N , U = kUnroll< V > , UN = U * N;
 R aside[ 2 * U ];
 for( std::size_t u = 0 ; u < U ; ++u ) {
  aside[ u ] = V::load( a + u * N );
  aside[ U + u ] = V::load( a + n - UN + u * N );
  }
 R mn = aside[ 0 ] , mx = aside[ 0 ];       // the extremes, if tracked
 if constexpr( Track )
  for( std::size_t u = 1 ; u < 2 * U ; ++u ) {
   mn = V::min( mn , aside[ u ] );
   mx = V::max( mx , aside[ u ] );
   }
 R ex[ 4 ];                                  // the extremes of the two
 if constexpr( Three ) {                    // sides, if three ways
  ex[ 0 ] = ex[ 2 ] = V::set1( std::numeric_limits< K >::max() );
  ex[ 1 ] = ex[ 3 ] = V::set1( std::numeric_limits< K >::lowest() );
  }
 std::size_t lw = 0 , rw = n , lr = UN , rr = n - UN;
 auto part = [ & ]( const R & v ) {
  if constexpr( Three ) {
   const auto [ nl , ng ] = V::part3( v , p , a + lw , a + rw , ex );
   lw += nl;
   rw -= ng;
   }
  else {
   const std::size_t nl = V::part( v , p , a + lw , a + rw );
   lw += nl;
   rw -= N - nl;
   }
  };
 while( rr - lr >= UN ) {
  K * at;
  if( lr - lw <= rw - rr ) {
   at = a + lr;
   lr += UN;
   }
  else {
   rr -= UN;
   at = a + rr;
   }
  R v[ U ];
  for( std::size_t u = 0 ; u < U ; ++u ) {
   v[ u ] = V::load( at + u * N );
   if constexpr( Track ) {
    mn = V::min( mn , v[ u ] );
    mx = V::max( mx , v[ u ] );
    }
   }
  for( std::size_t u = 0 ; u < U ; ++u ) part( v[ u ] );
  }
 while( rr - lr >= N ) {                    // the free places are at
  R v;                                      // least 2 U N >= 2 N
  if( lr - lw <= rw - rr ) {
   v = V::load( a + lr );
   lr += N;
   }
  else {
   rr -= N;
   v = V::load( a + rr );
   }
  if constexpr( Track ) {
   mn = V::min( mn , v );
   mx = V::max( mx , v );
   }
  part( v );
  }
 const std::size_t c = rr - lr;             // the last < N keys: the
 K l{} , h{};
 if constexpr( Track ) {
  l = V::hmin( mn );
  h = V::hmax( mx );
  }
 if constexpr( Track )                      // gap [ lw , rw ) is free
  for( std::size_t i = 0 ; i < c ; ++i ) {
   const K x = get( a + lr + i );
   l = x < l ? x : l;
   h = h < x ? x : h;
   }
 if constexpr( Three ) {
  if( c > 0 ) {
   const auto [ nl , ng ] = V::partN3( a + lr , c , p , a + lw , a + rw ,
                                       ex );
   lw += nl;
   rw -= ng;
   }
  }
 else if constexpr( requires { V::partN; } ) {
  if( c > 0 ) {
   const std::size_t nl = V::partN( a + lr , c , p , a + lw , a + rw );
   lw += nl;
   rw -= c - nl;
   }
  }
 else {
  K t[ N ];
  std::memcpy( t , a + lr , c * sizeof( K ) );
  for( std::size_t i = 0 ; i < c ; ++i ) {
   const K x = t[ i ];
   const bool lt = x < p;
   put( a + lw , x );
   put( a + rw - 1 , x );
   lw += lt;
   rw -= ! lt;
   }
  }
 for( std::size_t u = 0 ; u < 2 * U ; ++u ) // the gap shrinks to 2 N,
  part( aside[ u ] );                       // then N for the last
 if constexpr( Track ) {
  *lo = l;
  *hi = h;
  }
 if constexpr( Three ) {                    // the keys equal to p, which
  const R v = V::set1( p );                 // were not written
  std::size_t i = lw;
  for( ; i + N <= rw ; i += N ) V::store( a + i , v );
  V::storeN( a + i , v , rw - i );
  *gt = rw;
  single[ 0 ] = V::hmin( ex[ 0 ] ) == V::hmax( ex[ 1 ] );  // each side
  single[ 1 ] = V::hmin( ex[ 2 ] ) == V::hmax( ex[ 3 ] );  // one key
  }
 return( lw );
 }

/// a pivot of a[ 0 .. n ): the median of a sample of S = 8 or N keys, the
/// larger, one from each of S equal stretches, at the same random offset
/// in each, the generator s advanced
template< class V >
std::pair< typename V::K , bool > pivot( const typename V::K * a ,
                                         std::size_t n , std::uint64_t & s )
{
 using K = typename V::K;
 constexpr unsigned M = V::N >= 8 ? 1 : 8 / V::N;
 constexpr std::size_t S = M * V::N;
 K t[ S ];
 const std::size_t step = n / S;
 s ^= s << 13;
 s ^= s >> 7;
 s ^= s << 17;
 const K * b = a + std::size_t( ( ( s >> 32 ) * step ) >> 32 );
 for( std::size_t i = 0 ; i < S ; ++i , b += step )
  t[ i ] = get( b );
 typename V::R r[ M ];
 for( unsigned i = 0 ; i < M ; ++i ) r[ i ] = V::load( t + i * V::N );
 network< V , M >( r );
 for( unsigned i = 0 ; i < M ; ++i ) V::store( t + i * V::N , r[ i ] );
 return( std::pair( t[ S / 2 ] , ( t[ S / 2 - 1 ] == t[ S / 2 ] ) ||
                                 ( t[ S / 2 + 1 ] == t[ S / 2 ] ) ) );
 }

/// the quicksort of a[ 0 .. n ) with depth levels left
template< class V >
void quick( typename V::K * a , std::size_t n , int depth ,
            std::uint64_t & s )
{
 using K = typename V::K;
 while( n > kRegs< V > * V::N ) {
  if( depth-- == 0 ) {
   heapKeys( a , n );
   return;
   }
  const auto [ p , dup ] = pivot< V >( a , n , s );
  std::size_t m;
  if constexpr( requires { V::part3; } )
   if( dup ) {                              // the sample repeats p: the
    std::size_t g;                          // keys equal to p apart, in
    bool one[ 2 ];                          // one pass, and a side of one
    m = partition< V , false , true >( a , n , p , nullptr , nullptr ,
                                       &g , one );   // key done
    if( one[ 0 ] ) m = 0;
    if( one[ 1 ] ) g = n;
    if( m < n - g ) {
     quick< V >( a , m , depth , s );
     a += g;
     n -= g;
     }
    else {
     quick< V >( a + g , n - g , depth , s );
     n = m;
     }
    continue;
    }
  if( dup ) {                               // the sample repeats p: the
   K lo , hi;                               // extremes, and the keys
   m = partition< V , true >( a , n , p , &lo , &hi );   // equal to p
   if( lo == hi )                           // aside
    return;
   if( hi == p ) {                          // the right part all p
    n = m;
    continue;
    }
   if( n - m >= 2 * kUnroll< V > * V::N ) {
    const std::size_t e = partition< V >( a + m , n - m , K( p + 1 ) );
    if( m < n - m - e )
     quick< V >( a , m , depth , s );
    else {
     quick< V >( a + m + e , n - m - e , depth , s );
     n = m;
     continue;
     }
    a += m + e;
    n -= m + e;
    continue;
    }
   }
  else
   m = partition< V >( a , n , p );
  if( m == 0 ) {                            // p the minimum: the keys
   if( p == std::numeric_limits< K >::max() )   // equal to it are done
    return;
   m = partition< V >( a , n , K( p + 1 ) );
   a += m;
   n -= m;
   continue;
   }
  if( m < n - m ) {
   quick< V >( a , m , depth , s );
   a += m;
   n -= m;
   }
  else {
   quick< V >( a + m , n - m , depth , s );
   n = m;
   }
  }
 networkSort< V >( a , n );
 }

/// the key of each item of a[ 0 .. n ) as the signed integer K of the same
/// order, or back: its own inverse
template< class T , class K >
void toKeys( T * a , std::size_t n )
{
 constexpr int w = 8 * sizeof( K );
 using U = std::make_unsigned_t< K >;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  U b;
  std::memcpy( &b , a + i , sizeof( b ) );
  if constexpr( std::is_floating_point_v< T > )
   b ^= U( K( b ) >> ( w - 1 ) ) >> 1;
  else
   b ^= U( 1 ) << ( w - 1 );
  std::memcpy( a + i , &b , sizeof( b ) );
  }
 }

/// the sort of a[ 0 .. n ) with the registers of V
template< class V , class T >
void sortWith( T * a , std::size_t n )
{
 using K = typename V::K;
 constexpr bool convert = std::is_floating_point_v< T > ||
                          ( std::is_unsigned_v< T > !=
                            std::is_unsigned_v< K > );
 if( n <= 1 )
  return;
 if constexpr( convert )
  toKeys< T , K >( a , n );
 K * const k = reinterpret_cast< K * >( a );
 if( n <= kRegs< V > * V::N )
  networkSort< V >( k , n );
 else {
  std::uint64_t s = 0x9E3779B97F4A7C15ull ^ n;
  quick< V >( k , n , 2 * std::bit_width( n ) , s );
  }
 if constexpr( convert )
  toKeys< T , K >( a , n );
 }


}  // end( namespace detail::vqs )

template< VectorKey T >
void vectorQuickSort( T * a , std::size_t n )
{
 using K = detail::vqs::KeyOf< T >;
 detail::vqs::sortWith< detail::vqs::Best< K > >( a , n );
 }

template< VectorKey T >
void vectorQuickSort( std::vector< T > & A )
{
 const auto before = detail::auditCopy( A );
 vectorQuickSort( A.data() , A.size() );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T x , T y ) {
  if constexpr( std::is_floating_point_v< T > ) {
   using K = detail::vqs::KeyOf< T >;
   K a , b;                                 // the total order
   std::memcpy( &a , &x , sizeof( a ) );
   std::memcpy( &b , &y , sizeof( b ) );
   a ^= K( std::make_unsigned_t< K >( a >> ( 8 * sizeof( K ) - 1 ) ) >> 1 );
   b ^= K( std::make_unsigned_t< K >( b >> ( 8 * sizeof( K ) - 1 ) ) >> 1 );
   return( a < b );
   }
  else
   return( x < y ); } ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File VectorQuickSort.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
