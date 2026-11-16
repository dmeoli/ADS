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
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail::vqs
{

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
struct Lanes
{
 using K = Key;
 static constexpr unsigned N = 32 / sizeof( K );
 struct R
 {
  K v[ N ];
  };

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
  for( unsigned i = 0 ; i < N ; ++i )
   r.v[ i ] = k;
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
  for( unsigned i = 1 ; i < N ; ++i )
   k = a.v[ i ] < k ? a.v[ i ] : k;
  return( k );
  }
 static K hmax( const R & a ) {
  K k = a.v[ 0 ];
  for( unsigned i = 1 ; i < N ; ++i )
   k = k < a.v[ i ] ? a.v[ i ] : k;
  return( k );
  }
 /// lane i takes lane i ^ J
 template< unsigned J >
 static R swap( const R & a ) {
  R r;
  for( unsigned i = 0 ; i < N ; ++i )
   r.v[ i ] = a.v[ i ^ J ];
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

template< class K >
using Best = Lanes< K >;

/// the keys of T: a signed integer of its order
template< class T >
using KeyOf =
  std::conditional_t< sizeof( T ) == 4 , std::int32_t , std::int64_t >;


/// --------------------------------------------------------- the network

/// the steps of distance J, J / 2, ..., 1 of the stage on blocks of K2
/// lanes of the bitonic network on the M registers r
template< class V , unsigned M , unsigned K2 , unsigned J >
ADS_VQS_INLINE void steps( typename V::R * r )
{
 constexpr unsigned N = V::N;
 if constexpr( J >= N ) {  // between registers
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
 else  // within a register
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
    x = mn;
    }( r[ P[ I ][ 0 ] ] , r[ P[ I ][ 1 ] ] ) ) ,
   ... );
 }

/// the lanes of a register of N lanes whose bit B is clear
template< unsigned N , unsigned B >
constexpr std::uint32_t clearMask()
{
 std::uint32_t m = 0;
 for( unsigned i = 0 ; i < N ; ++i )
  if( ( i & B ) == 0 )
   m |= std::uint32_t( 1 ) << i;
 return( m );
 }

/// the lanes of a reversed within the blocks of B lanes
template< class V , unsigned B >
ADS_VQS_INLINE typename V::R reverse( typename V::R a )
{
 if constexpr( ( B > 2 ) && requires { V::template reverse< B >( a ); } )
  return( V::template reverse< B >( a ) );
 else if constexpr( B > 1 ) {
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
 if constexpr( J >= M ) {  // within a register
  constexpr unsigned L = J / M;
  for( unsigned h = 0 ; h < M ; ++h )
   r[ h ] = V::template exchange< clearMask< V::N , L >() >(
     r[ h ] , V::template swap< L >( r[ h ] ) );
  }
 else  // between registers
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
 columns< V , M >( r ,
		  std::make_index_sequence< oddEvenPairs< M >().size() >() );
 merges< V , M , 2 * M >( r );
 transpose< V , M , ( M < V::N ? M : V::N ) / 2 >( r );
 }

/// true if V stores the lanes of a mask, which the column network on
/// fewer registers than lanes needs
template< class V >
concept LaneStores = requires( typename V::K * p , typename V::R r )
{
 V::storeLanes( p , r , typename V::Mask() );
 };

/// a[ 0 .. n ) sorted in M registers, n <= M N
template< class V , unsigned M >
void smallSort( typename V::K * a , std::size_t n )
{
 typename V::R r[ M ];
 for( unsigned i = 0 ; i < M ; ++i ) {
  const std::size_t s = std::size_t( i ) * V::N;
  r[ i ] = s + V::N <= n ? V::load( a + s )
	   : s < n ? V::loadPad( a + s , n - s )
		   : V::set1( std::numeric_limits< typename V::K >::max() );
  }
 constexpr bool narrow =
   ( M < V::N ) && ( 2 * M >= V::N ) && ( M >= 4 ) && LaneStores< V >;
 constexpr bool byColumns = ( ( M >= V::N ) && ( M >= 4 ) ) || narrow;
 if constexpr( byColumns )
  columnNetwork< V , M >( r );
 else
  network< V , M >( r );
 if constexpr( narrow ) {                      // the chunk c of r[ h ], its
  using Mask = typename V::Mask;               // lanes [ c M , c M + M ),
  for( unsigned h = 0 ; h < M ; ++h )            // holds M keys from
   for( unsigned c = 0 ; c < V::N / M ; ++c ) {  // ( c M + h ) M
    const std::size_t s = std::size_t( c * M + h ) * M;
    const std::size_t k = s >= n ? 0 : std::min< std::size_t >( n - s , M );
    const Mask m = Mask( ( ( 1u << k ) - 1 ) << ( c * M ) );
    V::storeLanes( a + s - c * M , r[ h ] , m );
    }
  return;
  }
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
 if( m <= 1 )
  smallSort< V , 1 >( a , n );
 else if( m <= 2 )
  smallSort< V , 2 >( a , n );
 else if( m <= 4 )
  smallSort< V , 4 >( a , n );
 else if( m <= 8 )
  smallSort< V , 8 >( a , n );
 else
  smallSort< V , kRegs< V > >( a , n );
 }

/// the merge of the sorted a[ 0 .. na ) and b[ 0 .. nb ) into
/// o[ 0 .. na + nb ), register by register as in mergeLeaf(): forward,
/// the smallest keys first, if not Down, and backward, the largest first
/// from the end, if Down. The writes never overtake the reads of b, so
/// that b may lie at the end of o (at its start if Down), the run that
/// stays in place, while a is a copy aside. The keys left when the run to
/// read has less than a register, with the register of the keys carried,
/// are merged one by one
template< class V , bool Down >
void mergeRuns( const typename V::K * a , std::size_t na ,
		const typename V::K * b , std::size_t nb , typename V::K * o )
{
 using K = typename V::K;
 using R = typename V::R;
 constexpr std::size_t N = V::N;
 // the keys as seen from the end that the merge starts from
 auto at = []( const K * p , std::size_t n , std::size_t i ) {
  return( get( Down ? p + n - 1 - i : p + i ) );
  };
 auto before = []( K x , K y ) { return( Down ? y < x : x < y ); };
 auto put1 = [ & ]( std::size_t i , K x ) {
  put( Down ? o + na + nb - 1 - i : o + i , x );
  };
 std::size_t ia = 0 , ib = 0 , io = 0;  // taken from the start side
 K carry[ N ];
 std::size_t nc = 0;
 if( ( na >= N ) && ( nb >= N ) ) {
  R x = V::load( Down ? a + na - N : a ) ,
  y = V::load( Down ? b + nb - N : b );
  ia = ib = N;
  for( ; ; ) {
   y = reverse< V , N >( y );
   R lo = V::min( x , y ) , hi = V::max( x , y );
   halves< V , 1 , N / 2 >( &lo );
   halves< V , 1 , N / 2 >( &hi );
   if constexpr( Down ) {
    V::store( o + na + nb - io - N , hi );
    x = lo;
    }
   else {
    V::store( o + io , lo );
    x = hi;
    }
   io += N;
   const bool fromA =
     ( ia < na ) &&
     ( ( ib >= nb ) || ! before( at( b , nb , ib ) , at( a , na , ia ) ) );
   if( ( fromA ? na - ia : nb - ib ) < N )
    break;
   y = fromA ? V::load( Down ? a + na - ia - N : a + ia )
	     : V::load( Down ? b + nb - ib - N : b + ib );
   ia += fromA ? N : 0;
   ib += fromA ? 0 : N;
   }
  K t[ N ];          // the carried keys, in the
  V::store( t , x );  // order of the merge
  for( std::size_t i = 0 ; i < N ; ++i )
   carry[ i ] = t[ Down ? N - 1 - i : i ];
  nc = N;
  }
 std::size_t ic = 0;  // three ways, one by one, until
 while( ( ic < nc ) || ( ( ia < na ) && ( ib < nb ) ) ) {  // two are out
  const K * best = nullptr;
  K v{};
  if( ic < nc ) {
   v = carry[ ic ];
   best = carry;
   }
  if( ( ia < na ) && ( ! best || before( at( a , na , ia ) , v ) ) ) {
   v = at( a , na , ia );
   best = a;
   }
  if( ( ib < nb ) && ( ! best || before( at( b , nb , ib ) , v ) ) ) {
   v = at( b , nb , ib );
   best = b;
   }
  put1( io++ , v );
  ic += best == carry;
  ia += best == a;
  ib += best == b;
  }
 for( ; ia < na ; ++ia )
  put1( io++ , at( a , na , ia ) );
 if( ( ib < nb ) &&
     ( Down ? ( o + na + nb - io != b + nb - ib ) : ( o + io != b + ib ) ) )
  for( ; ib < nb ; ++ib )
   put1( io++ , at( b , nb , ib ) );
 }

/// ------------------------------------------------------- the recursion

/// heapsort of a[ 0 .. n ), the last resort of a part too deep
template< class K >
void heapKeys( K * a , std::size_t n )
{
 auto sift = [ a ]( std::size_t i , std::size_t m ) {
  const K x = get( a + i );
  for( std::size_t c ; ( c = 2 * i + 1 ) < m ; i = c ) {
   if( ( c + 1 < m ) && ( get( a + c ) < get( a + c + 1 ) ) )
    ++c;
   if( ! ( x < get( a + c ) ) )
    break;
   put( a + i , get( a + c ) );
   }
  put( a + i , x );
  };
 for( std::size_t i = n / 2 ; i-- > 0 ; )
  sift( i , n );
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
template< class V , bool Track = false , bool Three = false ,
	  std::size_t U = kUnroll< V > >
std::size_t partitionU( typename V::K * a , std::size_t n , typename V::K p ,
			typename V::K * lo = nullptr ,
			typename V::K * hi = nullptr ,
			std::size_t * gt = nullptr , bool * single = nullptr )
{
 using K = typename V::K;
 using R = typename V::R;
 constexpr std::size_t N = V::N , UN = U * N;
 R aside[ 2 * U ];
 for( std::size_t u = 0 ; u < U ; ++u ) {
  aside[ u ] = V::load( a + u * N );
  aside[ U + u ] = V::load( a + n - UN + u * N );
  }
 R mn = aside[ 0 ] , mx = aside[ 0 ];  // the extremes, if tracked
 if constexpr( Track )
  for( std::size_t u = 1 ; u < 2 * U ; ++u ) {
   mn = V::min( mn , aside[ u ] );
   mx = V::max( mx , aside[ u ] );
   }
 R ex[ 4 ];               // the extremes of the two
 if constexpr( Three ) {  // sides, if three ways
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
  const std::ptrdiff_t ahead =
    at == a + rr ? -std::ptrdiff_t( 8 * UN ) : std::ptrdiff_t( 8 * UN );
  for( std::size_t u = 0 ; u < U ; ++u ) {
   v[ u ] = V::load( at + u * N );
   ADS::detail::prefetchLine( at + u * N + ahead );
   if constexpr( Track ) {
    mn = V::min( mn , v[ u ] );
    mx = V::max( mx , v[ u ] );
    }
   }
  for( std::size_t u = 0 ; u < U ; ++u )
   part( v[ u ] );
  }
 while( rr - lr >= N ) {  // the free places are at
  R v;                    // least 2 U N >= 2 N
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
 const std::size_t c = rr - lr;  // the last < N keys: the
 K l{} , h{};
 if constexpr( Track ) {
  l = V::hmin( mn );
  h = V::hmax( mx );
  }
 if constexpr( Track )  // gap [ lw , rw ) is free
  for( std::size_t i = 0 ; i < c ; ++i ) {
   const K x = get( a + lr + i );
   l = x < l ? x : l;
   h = h < x ? x : h;
   }
 if constexpr( Three ) {
  if( c > 0 ) {
   const auto [ nl , ng ] = V::partN3( a + lr , c , p , a + lw , a + rw , ex );
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
 for( std::size_t u = 0 ; u < 2 * U ; ++u )  // the gap shrinks to 2 N,
  part( aside[ u ] );                      // then N for the last
 if constexpr( Track ) {
  *lo = l;
  *hi = h;
  }
 if constexpr( Three ) {     // the keys equal to p, which
  const R v = V::set1( p );  // were not written
  std::size_t i = lw;
  for( ; i + N <= rw ; i += N )
   V::store( a + i , v );
  V::storeN( a + i , v , rw - i );
  *gt = rw;
  single[ 0 ] = V::hmin( ex[ 0 ] ) == V::hmax( ex[ 1 ] );  // each side
  single[ 1 ] = V::hmin( ex[ 2 ] ) == V::hmax( ex[ 3 ] );  // one key
  }
 return( lw );
 }

/// partitionU() with the most vectors kept aside that a[ 0 .. n ) allows,
/// n >= 2 N
template< class V , bool Track = false , bool Three = false >
std::size_t partition( typename V::K * a , std::size_t n , typename V::K p ,
		       typename V::K * lo = nullptr ,
		       typename V::K * hi = nullptr ,
		       std::size_t * gt = nullptr , bool * single = nullptr )
{
 constexpr std::size_t N = V::N , U = kUnroll< V >;
 if( n >= 2 * U * N )
  return( partitionU< V , Track , Three , U >( a , n , p , lo , hi , gt ,
					       single ) );
 if( n >= 4 * N )
  return( partitionU< V , Track , Three , 2 >( a , n , p , lo , hi , gt ,
					       single ) );
 return( partitionU< V , Track , Three , 1 >( a , n , p , lo , hi , gt ,
					      single ) );
 }

/// the registers of the sample of the pivot of a part of n keys: 8 keys
/// or N, the larger, and four times as many from 1024 N keys
template< class V >
inline constexpr unsigned kSample = V::N >= 8 ? 1 : 8 / V::N;

/// a pivot of a[ 0 .. n ): the median of a sample of S = M N keys, one
/// from each of S equal stretches, at the same random offset in each, the
/// generator s advanced; and whether the sample repeats it
template< class V , unsigned M >
std::pair< typename V::K , bool > pivot( const typename V::K * a ,
					 std::size_t n ,
					std::uint64_t & s )
{
 using K = typename V::K;
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
 for( unsigned i = 0 ; i < M ; ++i )
  r[ i ] = V::load( t + i * V::N );
 network< V , M >( r );
 for( unsigned i = 0 ; i < M ; ++i )
  V::store( t + i * V::N , r[ i ] );
 return( std::pair( t[ S / 2 ] , ( t[ S / 2 - 1 ] == t[ S / 2 ] ) ||
				   ( t[ S / 2 + 1 ] == t[ S / 2 ] ) ) );
 }

/// the quicksort of a[ 0 .. n ) with depth levels left
template< class V >
void quick( typename V::K * a , std::size_t n , int depth , std::uint64_t & s )
{
 using K = typename V::K;
 while( n > kRegs< V > * V::N ) {
  if( depth-- == 0 ) {
   heapKeys( a , n );
   return;
   }
  const auto [ p , dup ] = n < 1024 * V::N
			    ? pivot< V , kSample< V > >( a , n , s )
			    : pivot< V , 4 * kSample< V > >( a , n , s );
  std::size_t m;
  if constexpr( requires { V::part3; } )
   if( dup ) {      // the sample repeats p: the
    std::size_t g;  // keys equal to p apart, in
    bool one[ 2 ];  // one pass, and a side of one
    m = partition< V , false , true >( a , n , p , nullptr , nullptr , &g ,
				     one );  // key done
    if( one[ 0 ] )
     m = 0;
    if( one[ 1 ] )
     g = n;
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
  if( dup ) {                                      // the sample repeats p: the
   K lo , hi;                                       // extremes, and the keys
   m = partition< V , true >( a , n , p , &lo , &hi );  // equal to p
   if( lo == hi )                                  // aside
    return;
   if( hi == p ) {  // the right part all p
    n = m;
    continue;
    }
   if( n - m >= 2 * V::N ) {
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
  if( m == 0 ) {                               // p the minimum: the keys
   if( p == std::numeric_limits< K >::max() )  // equal to it are done
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

/// the range of keys of 64 bits from which the sample of a counting sort
/// is too wide, 0 since there is none
inline constexpr std::uint64_t kCountRange = 0;

/// k[ 0 .. n ) reversed
template< class K >
void reverseKeys( K * k , std::size_t n )
{
 for( std::size_t i = 0 , j = n ; i + 1 < j ; ++i ) {
  --j;
  const K x = get( k + i );
  put( k + i , get( k + j ) );
  put( k + j , x );
  }
 }

/// the sort of a[ 0 .. n ) with the registers of V. The floating point
/// numbers are sorted as the signed integers of their bits, which keep the
/// order of the positive ones and reverse that of the negative ones, all
/// first: their block is then reversed, a pass over it alone
template< class V , class T >
void sortWith( T * a , std::size_t n )
{
 using K = typename V::K;
 constexpr bool convert =
   ( ! std::is_floating_point_v< T > ) &&
   ( std::is_unsigned_v< T > != std::is_unsigned_v< K > );
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
 if constexpr( std::is_floating_point_v< T > ) {
  std::size_t lo = 0 , hi = n;  // the first key not negative
  while( lo < hi ) {
   const std::size_t mid = lo + ( hi - lo ) / 2;
   if( get( k + mid ) < K( 0 ) )
    lo = mid + 1;
   else
    hi = mid;
   }
  reverseKeys( k , lo );
  }
 }

#if ADS_VECTOR_SORT

/// the sort of a[ 0 .. n ) by the quicksort of Highway, on the vectors of
/// the processor found when the program runs: the integers as they are,
/// the floating point numbers as the signed integers of their bits, whose
/// block of negative numbers is then reversed, as in sortWith()
template< class T >
void sortHighway( T * a , std::size_t n )
{
 using K = KeyOf< T >;
 if( n <= 1 )
  return;
 if constexpr( std::is_floating_point_v< T > ) {
  K * const k = reinterpret_cast< K * >( a );
  hwy::VQSort( k , n , hwy::SortAscending() );
  std::size_t lo = 0 , hi = n;  // the first key not negative
  while( lo < hi ) {
   const std::size_t mid = lo + ( hi - lo ) / 2;
   if( get( k + mid ) < K( 0 ) )
    lo = mid + 1;
   else
    hi = mid;
   }
  reverseKeys( k , lo );
  }
 else {
  using I =
    std::conditional_t< std::is_signed_v< T > , K ,
			std::make_unsigned_t< K > >;
  hwy::VQSort( reinterpret_cast< I * >( a ) , n , hwy::SortAscending() );
  }
 }

#endif

}  // end( namespace detail::vqs )

template< VectorKey T >
void vectorQuickSort( T * a , std::size_t n )
{
#if ADS_VECTOR_SORT
 detail::vqs::sortHighway( a , n );
#else
 using K = detail::vqs::KeyOf< T >;
 detail::vqs::sortWith< detail::vqs::Best< K > >( a , n );
#endif
 }

template< SortStore Store , VectorKey T >
void vectorQuickSort( Store && A )
{
 const auto before = detail::auditCopy( A );
 vectorQuickSort( A.data() , A.size() );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T x , T y ) {
  if constexpr( std::is_floating_point_v< T > ) {
   using K = detail::vqs::KeyOf< T >;
   K a , b;  // the total order
   std::memcpy( &a , &x , sizeof( a ) );
   std::memcpy( &b , &y , sizeof( b ) );
   a ^= K( std::make_unsigned_t< K >( a >> ( 8 * sizeof( K ) - 1 ) ) >> 1 );
   b ^= K( std::make_unsigned_t< K >( b >> ( 8 * sizeof( K ) - 1 ) ) >> 1 );
   return( a < b );
   }
  else
   return( x < y );
  } ) );
 }

// the vectors of the book, by a call of the sort on their storage

template< VectorKey T >
ADS_SORT_FORWARD void vectorQuickSort( std::vector< T > & A )
{
 return( vectorQuickSort( std::move( A ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File VectorQuickSort.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
