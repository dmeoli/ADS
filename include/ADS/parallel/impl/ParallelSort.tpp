/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel sorts of ParallelSort.h: the survey of the order, the two
 * classifiers, the distribution into the buckets and their sorts.
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

#include <array>
#include <atomic>
#include <cstring>
#include <mutex>
#include <bit>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace ADS {

namespace detail {

/// the numbers whose natural order is less, which the classifier by the
/// highest bits of radixKey() handles
template< class T , class C >
inline constexpr bool naturalNumbers = []() {
 if constexpr( RadixKey< T > )
  return( std::is_same_v< C , std::less<> > ||
          std::is_same_v< C , std::less< T > > );
 else
  return( false );
 }();

/// the bits of the number whose key of radixKey() is k, the inverse of it
template< class T , class K >
auto fromRadixKey( K k )
{
 constexpr K sign = K( K( 1 ) << ( 8 * sizeof( K ) - 1 ) );
 using U = std::conditional_t< sizeof( T ) == 1 , std::uint8_t ,
           std::conditional_t< sizeof( T ) == 2 , std::uint16_t ,
           std::conditional_t< sizeof( T ) == 4 , std::uint32_t ,
                                                  std::uint64_t > > >;
 if constexpr( std::is_floating_point_v< T > )
  return( U( ( k & sign ) ? ( k ^ sign ) : ~k ) );
 else if constexpr( std::is_signed_v< T > )
  return( U( k ^ sign ) );
 else
  return( U( k ) );
 }

/// the key of x for the classifier of the numbers, -0 as +0
template< class T >
auto sortKey( T x )
{
 if constexpr( std::is_floating_point_v< T > )
  if( x == T( 0 ) )
   x = T( 0 );
 return( radixKey( x ) );
 }

/// whether the distribution writes the items a line of the cache at a
/// time: those copied as bytes, several to a line
template< class T >
inline constexpr bool lined = std::is_trivially_copyable_v< T > &&
                              ( sizeof( T ) <= 16 ) &&
                              ( 64 % sizeof( T ) == 0 );

/// whether the buckets are sorted in their places by vectorQuickSort():
/// numbers of 32 or 64 bits in their natural order with the vector
/// registers there, not the floating point numbers of the stable sort (it
/// would put -0 before +0, which std::less finds equal)
template< bool Stable , class T , class C >
inline constexpr bool inPlace = []() {
 if constexpr( VectorKey< T > && naturalNumbers< T , C > )
  return( kVectorSort && ( ! Stable || std::is_integral_v< T > ) );
 else
  return( false );
 }();

/// the sequential sort of the parts
template< bool Stable , class T , class C >
void sequentialSort( std::vector< T > & A , C & less )
{
 if constexpr( Stable )
  stableSort( A , less );
 else
  sort( A , less );
 }

/// the classifier by k - 1 splitters in an implicit binary tree, with
/// the buckets of the items equal to a splitter if eq
template< class T , class C >
class TreeClassifier {
 public:

 /// the splitters from a sample of A, k a power of 2 at least 2
 TreeClassifier( const std::vector< T > & A , std::size_t k , C & less )
  : f_less( less ) , f_k( k ) , f_levels( std::countr_zero( k ) ) {
  const std::size_t n = A.size() , s = std::min( n , 16 * k );
  std::vector< T > S;
  S.reserve( s );
  std::uint64_t r = 0x9E3779B97F4A7C15ull ^ n;
  for( std::size_t i = 0 ; i < s ; ++i ) {
   r ^= r << 13;
   r ^= r >> 7;
   r ^= r << 17;
   S.push_back( A[ r % n ] );
   }
  sort( S , f_less );
  std::vector< T > sp;                      // the splitters, distinct
  sp.reserve( k - 1 );
  for( std::size_t i = 1 ; i < k ; ++i ) {
   const T & x = S[ i * s / k ];
   if( sp.empty() || f_less( sp.back() , x ) )
    sp.push_back( x );
   else
    f_eq = true;
   }
  while( sp.size() < k - 1 )                // a perfect tree: the last
   sp.push_back( sp.back() );               // repeated, empty buckets
  f_sorted = sp;
  f_tree.resize( k );
  build( sp , 1 , 0 , k - 1 );
  }

 /// whether the buckets of equality are there
 [[nodiscard]] bool equality( void ) const { return( f_eq ); }

 /// the number of buckets
 [[nodiscard]] std::size_t buckets( void ) const {
  return( f_eq ? 2 * f_k - 1 : f_k );
  }

 /// whether the bucket b holds equal items only
 [[nodiscard]] bool equal( std::size_t b ) const {
  return( f_eq && ( b & 1 ) );
  }

 /// the buckets of the m items from a into id, 8 at a time so that their
 /// searches overlap
 template< class Id >
 void classify( const T * a , std::size_t m , Id * id ) const {
  std::size_t i = 0;
  for( ; i + 8 <= m ; i += 8 ) {
   std::size_t j[ 8 ];
   for( int u = 0 ; u < 8 ; ++u )
    j[ u ] = 1;
   for( int l = 0 ; l < f_levels ; ++l )
    for( int u = 0 ; u < 8 ; ++u )
     j[ u ] = 2 * j[ u ] + std::size_t( f_less( f_tree[ j[ u ] ] ,
                                                a[ i + u ] ) );
   for( int u = 0 ; u < 8 ; ++u )
    id[ i + u ] = Id( finish( j[ u ] - f_k , a[ i + u ] ) );
   }
  for( ; i < m ; ++i ) {
   std::size_t j = 1;
   for( int l = 0 ; l < f_levels ; ++l )
    j = 2 * j + std::size_t( f_less( f_tree[ j ] , a[ i ] ) );
   id[ i ] = Id( finish( j - f_k , a[ i ] ) );
   }
  }

 private:

 /// the bucket of x, b splitters less than it
 std::size_t finish( std::size_t b , const T & x ) const {
  if( ! f_eq )
   return( b );
  return( 2 * b + std::size_t( ( b + 1 < f_k ) &&
                               ! f_less( x , f_sorted[ b ] ) ) );
  }

 /// the node j of the tree on the splitters [ lo , hi )
 void build( const std::vector< T > & sp , std::size_t j , std::size_t lo ,
             std::size_t hi ) {
  if( lo >= hi )
   return;
  const std::size_t mid = lo + ( hi - lo ) / 2;
  f_tree[ j ] = sp[ mid ];
  build( sp , 2 * j , lo , mid );
  build( sp , 2 * j + 1 , mid + 1 , hi );
  }

 C & f_less;
 std::size_t f_k;
 int f_levels;
 bool f_eq = false;
 std::vector< T > f_tree;                  ///< the nodes 1 .. k - 1
 std::vector< T > f_sorted;                ///< the splitters in order
 };

/// the classifier of the numbers by the highest bits in which their keys
/// differ, between the least and the greatest key of a sample: the keys
/// below the least go to the first bucket and those above the greatest to
/// the last, so that the order of the buckets stays that of the keys
template< class T >
class RadixClassifier {
 public:

 using Key = decltype( radixKey( T() ) );

 /// the bounds from a sample of 1024 items of A
 explicit RadixClassifier( const std::vector< T > & A ) {
  const std::size_t n = A.size();
  std::uint64_t r = 0x9E3779B97F4A7C15ull ^ n;
  Key lo = sortKey( A[ 0 ] ) , hi = lo;
  for( int i = 0 ; i < 1024 ; ++i ) {
   r ^= r << 13;
   r ^= r >> 7;
   r ^= r << 17;
   const Key x = sortKey( A[ r % n ] );
   lo = std::min( lo , x );
   hi = std::max( hi , x );
   }
  const int w = std::bit_width( Key( hi - lo ) );
  f_lo = lo;
  f_shift = w > 8 ? w - 8 : 0;
  f_last = std::size_t( Key( hi - lo ) >> f_shift );
  // the share of the fullest bucket in the sample, and its distinct keys
  std::vector< int > h( f_last + 1 , 0 );
  std::vector< Key > S;
  S.reserve( 1024 );
  for( int i = 0 ; i < 1024 ; ++i ) {
   r ^= r << 13;
   r ^= r >> 7;
   r ^= r << 17;
   f_full = std::max( f_full , ++h[ ( *this )( A[ r % n ] ) ] );
   S.push_back( sortKey( A[ r % n ] ) );
   }
  std::sort( S.begin() , S.end() );
  f_distinct = std::size_t( std::unique( S.begin() , S.end() ) - S.begin() );
  }

 /// the distinct keys of the sample
 [[nodiscard]] std::size_t distinct( void ) const { return( f_distinct ); }

 /// whether the bits spread the keys of the sample: no bucket with more
 /// than 4 times its share of them, and 32 more (the floating point
 /// numbers of a few exponents, or integers of a power law, are not)
 [[nodiscard]] bool spread( void ) const {
  return( ( f_shift == 0 ) ||
          ( f_full <= int( 4 * 1024 / ( f_last + 1 ) ) + 32 ) );
  }

 [[nodiscard]] std::size_t buckets( void ) const { return( f_last + 1 ); }

 /// whether each key in the bounds of the sample has a bucket of its own
 [[nodiscard]] bool exact( void ) const { return( f_shift == 0 ); }

 /// whether x is in the bounds of the sample
 [[nodiscard]] bool inBounds( const T & x ) const {
  const Key k = sortKey( x );
  return( ( k >= f_lo ) && ( Key( k - f_lo ) <= Key( f_last ) ) );
  }

 /// with no shift, each bucket but the two at the ends, which take the
 /// keys out of the bounds, holds one key
 [[nodiscard]] bool equal( std::size_t b ) const {
  return( ( f_shift == 0 ) && ( b > 0 ) && ( b < f_last ) );
  }

 [[nodiscard]] std::size_t operator()( const T & x ) const {
  const Key k = sortKey( x );
  const std::size_t b = k < f_lo ? 0 : std::size_t( Key( k - f_lo ) >>
                                                    f_shift );
  return( std::min( b , f_last ) );
  }

 private:

 Key f_lo;
 int f_shift;
 std::size_t f_last;
 int f_full = 0;                           ///< the fullest of the sample
 std::size_t f_distinct = 0;               ///< the keys of the sample
 };

template< bool Stable , class E , class T , class C >
void parallelSort( E & ex , std::vector< T > & A , C & less , int depth );

/// the distribution of A into the buckets of the classifier and their
/// sorts; Id the type of the bucket of an item if they are kept (void if
/// the classifier is cheap enough to run twice)
template< bool Stable , class Id , class E , class T , class C , class K >
void distribute( E & ex , std::vector< T > & A , C & less , const K & cls ,
                 int depth )
{
 const std::size_t n = A.size() , p = ex.concurrency() ,
                   k = cls.buckets();
 const std::size_t nb = std::min( 4 * p , std::max( p , n / 16384 ) ) ,
                   bs = ( n + nb - 1 ) / nb;
 std::vector< std::size_t > cnt( nb * k , 0 );
 // the integers of few distinct keys, each with a bucket of its own: the
 // counts are a counting sort, and each bucket is filled with its key
 constexpr bool counting = std::is_integral_v< T > &&
                           std::is_same_v< K , RadixClassifier< T > >;
 const bool exact = [ & ]() {
  if constexpr( counting ) return( cls.exact() ); else return( false );
  }();
 std::atomic< bool > outside{ false };
 std::vector< T > key( counting && exact ? k : 0 );
 constexpr bool keep = ! std::is_void_v< Id >;
 using Ids = std::conditional_t< keep , Id , char >;
 std::unique_ptr< Ids[] > id( keep ? new Ids[ n ] : nullptr );
 // the counts of each block
 ex.bulk( nb , [ & ]( std::size_t j ) {
  const std::size_t lo = j * bs , hi = std::min( n , lo + bs );
  if( lo >= hi )
   return;
  // the counts on the stack of the thread, not on lines that the
  // neighbouring blocks share
  std::vector< std::size_t > c( k , 0 );
  if constexpr( keep ) {
   cls.classify( A.data() + lo , hi - lo , id.get() + lo );
   for( std::size_t i = lo ; i < hi ; ++i )
    ++c[ id[ i ] ];
   }
  else if constexpr( counting ) {
   if( exact ) {
    bool out = false;
    for( std::size_t i = lo ; i < hi ; ++i ) {
     out |= ! cls.inBounds( A[ i ] );
     const std::size_t b = cls( A[ i ] );
     if( c[ b ]++ == 0 )                    // the key of the bucket
      std::atomic_ref< T >( key[ b ] ).store( A[ i ] ,
                                             std::memory_order_relaxed );
     }
    if( out )
     outside.store( true , std::memory_order_relaxed );
    }
   else
    for( std::size_t i = lo ; i < hi ; ++i )
     ++c[ cls( A[ i ] ) ];
   }
  else
   for( std::size_t i = lo ; i < hi ; ++i )
    ++c[ cls( A[ i ] ) ];
  std::copy( c.begin() , c.end() , cnt.begin() + std::ptrdiff_t( j * k ) );
  } );
 // the places, in the order ( bucket , block )
 std::vector< std::size_t > start( k + 1 , 0 );
 {
  std::size_t s = 0;
  for( std::size_t b = 0 ; b < k ; ++b ) {
   start[ b ] = s;
   for( std::size_t j = 0 ; j < nb ; ++j ) {
    const std::size_t c = cnt[ j * k + b ];
    cnt[ j * k + b ] = s;
    s += c;
    }
   }
  start[ k ] = s;
  }
 if constexpr( counting )
  if( exact && ! outside.load() ) {
   // the buckets filled with their keys
   parallelForBlocks( ex , 0 , n , [ & ]( std::size_t lo , std::size_t hi ) {
    std::size_t b = std::size_t( std::upper_bound( start.begin() ,
                                                   start.end() , lo ) -
                                 start.begin() ) - 1;
    for( std::size_t i = lo ; i < hi ; ++b ) {
     const std::size_t e = std::min( hi , start[ b + 1 ] );
     std::fill( A.begin() + std::ptrdiff_t( i ) ,
                A.begin() + std::ptrdiff_t( e ) , key[ b ] );
     i = e;
     }
    } , 16384 );
   return;
   }
 ScratchBuffer< T > buf( n );
 T * const B = buf.data();
 ex.bulk( nb , [ & ]( std::size_t j ) {
  const std::size_t lo = j * bs , hi = std::min( n , lo + bs );
  std::vector< std::size_t > c( cnt.begin() + std::ptrdiff_t( j * k ) ,
                                cnt.begin() + std::ptrdiff_t( j * k + k ) );
  auto bucket = [ & ]( std::size_t i ) {
   if constexpr( keep )
    return( std::size_t( id[ i ] ) );
   else
    return( cls( A[ i ] ) );
   };
  if constexpr( lined< T > ) {
   // a line of the cache per bucket on the stack, written out whole
   constexpr std::size_t L = 64 / sizeof( T );
   std::vector< std::array< T , L > > line( k );
   std::vector< std::uint8_t > fill( k , 0 );
   for( std::size_t i = lo ; i < hi ; ++i ) {
    const std::size_t b = bucket( i );
    line[ b ][ fill[ b ] ] = A[ i ];
    if( ++fill[ b ] == L ) {
     std::memcpy( static_cast< void * >( B + c[ b ] ) , line[ b ].data() ,
                  64 );
     c[ b ] += L;
     fill[ b ] = 0;
     }
    }
   for( std::size_t b = 0 ; b < k ; ++b )
    std::memcpy( static_cast< void * >( B + c[ b ] ) , line[ b ].data() ,
                 fill[ b ] * sizeof( T ) );
   }
  else
   for( std::size_t i = lo ; i < hi ; ++i )
    std::construct_at( B + c[ bucket( i ) ]++ , std::move( A[ i ] ) );
  } );
 id.reset();
 // the buckets from the largest; those beyond n / p first, on all threads
 std::vector< std::size_t > order( k );
 for( std::size_t b = 0 ; b < k ; ++b )
  order[ b ] = b;
 std::sort( order.begin() , order.end() ,
            [ & ]( std::size_t a , std::size_t b ) {
             return( start[ a + 1 ] - start[ a ] >
                     start[ b + 1 ] - start[ b ] );
             } );
 std::size_t first = 0;
 for( ; first < k ; ++first ) {
  const std::size_t b = order[ first ] , s = start[ b ] ,
                    m = start[ b + 1 ] - s;
  if( ( m <= n / p ) || cls.equal( b ) )
   break;
  std::vector< T > v( m );
  parallelFor( ex , 0 , m , [ & ]( std::size_t i ) {
   v[ i ] = std::move( B[ s + i ] );
   std::destroy_at( B + s + i );
   } );
  parallelSort< Stable >( ex , v , less , depth + 1 );
  parallelFor( ex , 0 , m , [ & ]( std::size_t i ) {
   A[ s + i ] = std::move( v[ i ] );
   } );
  }
 std::atomic< std::size_t > next{ first };
 ex.bulk( std::min( p , k - first ) , [ & ]( std::size_t ) {
  std::vector< T > v;
  for( ;; ) {
   const std::size_t r = next.fetch_add( 1 , std::memory_order_relaxed );
   if( r >= k )
    return;
   const std::size_t b = order[ r ] , s = start[ b ] ,
                     m = start[ b + 1 ] - s;
   if( ( m <= 1 ) || cls.equal( b ) ) {
    for( std::size_t i = s ; i < s + m ; ++i ) {
     A[ i ] = std::move( B[ i ] );
     std::destroy_at( B + i );
     }
    continue;
    }
   if constexpr( inPlace< Stable , T , C > ) {
    // back to its place, sorted there by the quicksort in the vector
    // registers that the sequential sort would choose for it
    std::copy( B + s , B + s + m , A.begin() + std::ptrdiff_t( s ) );
    // unless in order, or in few runs, which the sequential sort merges
    const T * a = A.data() + s;
    std::size_t r = 1;
    for( std::size_t i = 1 ; ( i < m ) && ( r <= kRuns ) ; ++i )
     r += less( a[ i ] , a[ i - 1 ] );
    if( r == 1 )
     continue;
    if( r > kRuns ) {
     vectorQuickSort( A.data() + s , m );
     continue;
     }
    v.assign( a , a + m );
    sequentialSort< Stable >( v , less );
    std::copy( v.begin() , v.end() , A.begin() + std::ptrdiff_t( s ) );
    continue;
    }
   v.assign( std::make_move_iterator( B + s ) ,
             std::make_move_iterator( B + s + m ) );
   std::destroy( B + s , B + s + m );
   sequentialSort< Stable >( v , less );
   std::move( v.begin() , v.end() , A.begin() + std::ptrdiff_t( s ) );
   }
  } );
 }

/// the runs of A, if few: each of B blocks cut into its maximal runs,
/// non-decreasing or strictly decreasing, each block giving up beyond
/// 8 of them (on most inputs after a few items); if no block gave up,
/// the decreasing runs are reversed and all are merged by the parallel
/// multiwayMerge() through a buffer, stably, and true is returned
template< bool Stable , class E , class T , class C >
bool fewRuns( E & ex , std::vector< T > & A , C & less )
{
 const std::size_t n = A.size() ,
                   B = std::max< std::size_t >( 8 , ex.concurrency() );
 constexpr std::size_t cap = 8;
 // the starts of the runs of each block, and their directions
 std::vector< std::vector< std::pair< std::size_t , bool > > > R( B );
 std::atomic< bool > many{ false };
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) {
  const std::size_t lo = n * b / B , hi = n * ( b + 1 ) / B;
  auto & r = R[ b ];
  for( std::size_t i = lo ; i < hi ; ) {
   if( ( r.size() == cap ) || many.load( std::memory_order_relaxed ) ) {
    many.store( true , std::memory_order_relaxed );
    return;
    }
   const bool down = ( i + 1 < hi ) && less( A[ i + 1 ] , A[ i ] );
   r.emplace_back( i , down );
   std::size_t j = i + 1;
   if( down )
    while( ( j < hi ) && less( A[ j ] , A[ j - 1 ] ) ) ++j;
   else
    while( ( j < hi ) && ! less( A[ j ] , A[ j - 1 ] ) ) ++j;
   i = j;
   }
  } , 1 );
 if( many.load() )
  return( false );
 std::vector< std::pair< T * , T * > > runs;
 for( std::size_t b = 0 ; b < B ; ++b )
  for( std::size_t q = 0 ; q < R[ b ].size() ; ++q ) {
   const std::size_t s = R[ b ][ q ].first ,
                     e = q + 1 < R[ b ].size() ? R[ b ][ q + 1 ].first :
                                                 n * ( b + 1 ) / B;
   if( R[ b ][ q ].second )
    std::reverse( A.begin() + std::ptrdiff_t( s ) ,
                  A.begin() + std::ptrdiff_t( e ) );
   runs.emplace_back( A.data() + s , A.data() + e );
   }
 if constexpr( std::is_trivially_copyable_v< T > ) {
  ScratchBuffer< T > buf( n );
  multiwayMerge( ex , runs , buf.data() , less );
  parallelFor( ex , 0 , n , [ & ]( std::size_t i ) {
   A[ i ] = buf.data()[ i ]; } , 16384 );
  }
 else {
  std::vector< T > out( n );
  multiwayMerge( ex , runs , out.begin() , less );
  parallelFor( ex , 0 , n , [ & ]( std::size_t i ) {
   A[ i ] = std::move( out[ i ] ); } , 4096 );
  }
 return( true );
 }

/// the counting sort of the numbers of A if they take at most 32 distinct
/// values, those of a sample of 1024 of them: each item found among them
/// (by their keys of radixKey(), which tell apart all the values, -0 and
/// +0 included) by counting the keys less than its own, 32 comparisons
/// that the compiler puts in vectors, the counts of the blocks
/// summed, and A filled with the values in order; false, A untouched, if
/// an item has another value
template< class E , class T >
bool fewKeys( E & ex , std::vector< T > & A )
{
 using Key = decltype( radixKey( T() ) );
 const std::size_t n = A.size();
 constexpr std::size_t D = 32;
 std::vector< Key > key;
 std::uint64_t r = 0x9E3779B97F4A7C15ull ^ ( n + 1 );
 for( int i = 0 ; i < 1024 ; ++i ) {
  r ^= r << 13;
  r ^= r >> 7;
  r ^= r << 17;
  key.push_back( radixKey( A[ r % n ] ) );
  }
 std::sort( key.begin() , key.end() );
 key.erase( std::unique( key.begin() , key.end() ) , key.end() );
 const std::size_t d = key.size();
 if( d > D )
  return( false );
 std::array< Key , D > K;                  // the keys, the last repeated
 for( std::size_t i = 0 ; i < D ; ++i )
  K[ i ] = key[ std::min( i , d - 1 ) ];
 const std::size_t B = std::max< std::size_t >( 1 , std::min( n / 16384 ,
                                         4 * ex.concurrency() ) );
 std::vector< std::array< std::size_t , D > > cnt( B );
 std::atomic< bool > other{ false };
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) {
  // the counts, the keys and the bounds in locals of the thread, which no
  // store of a count may alias
  std::size_t c[ D ] = {};
  const std::array< Key , D > k = K;
  const T * a = A.data();
  const std::size_t i0 = n * b / B , i1 = n * ( b + 1 ) / B;
  bool out = false;
  for( std::size_t i = i0 ; i < i1 ; ++i ) {
   const Key x = radixKey( a[ i ] );
   unsigned j = 0;                          // the keys less than x, by
   for( std::size_t q = 0 ; q < D ; ++q )   // comparisons in vectors
    j += k[ q ] < x;
   j = std::min( j , unsigned( D - 1 ) );
   out |= k[ j ] != x;
   ++c[ j ];
   }
  std::copy( c , c + D , cnt[ b ].begin() );
  if( out )
   other.store( true , std::memory_order_relaxed );
  } , 1 );
 if( other.load() )
  return( false );
 // the start of each value, and A filled
 std::array< std::size_t , D + 1 > start{};
 for( std::size_t j = 0 ; j < D ; ++j ) {
  std::size_t t = 0;
  for( std::size_t b = 0 ; b < B ; ++b ) t += cnt[ b ][ j ];
  start[ j + 1 ] = start[ j ] + t;
  }
 std::array< T , D > value;
 for( std::size_t j = 0 ; j < D ; ++j )
  value[ j ] = std::bit_cast< T >( fromRadixKey< T >( K[ j ] ) );
 parallelForBlocks( ex , 0 , n , [ & ]( std::size_t lo , std::size_t hi ) {
  std::size_t j = std::size_t( std::upper_bound( start.begin() ,
                                                 start.end() , lo ) -
                               start.begin() ) - 1;
  for( std::size_t i = lo ; i < hi ; ++j ) {
   const std::size_t e = std::min( hi , start[ j + 1 ] );
   std::fill( A.begin() + std::ptrdiff_t( i ) ,
              A.begin() + std::ptrdiff_t( e ) , value[ j ] );
   i = e;
   }
  } , 16384 );
 return( true );
 }

/// the survey of the order of A: whether sorted, whether decreasing
struct Survey {
 bool up = true , down = true;
 };

template< bool Stable , class E , class T , class C >
void parallelSort( E & ex , std::vector< T > & A , C & less , int depth )
{
 const std::size_t n = A.size() , p = ex.concurrency();
 if( ( p < kParallelSortThreads ) || ( n < kParallelSort ) ||
     ( depth > 3 ) ) {
  sequentialSort< Stable >( A , less );
  return;
  }
 constexpr bool numbers = naturalNumbers< T , C >;
 using S = Survey;
 const S sv = parallelReduce( ex , 0 , n , S() ,
  [ & ]( std::size_t lo , std::size_t hi ) {
   S s;
   const std::size_t e = std::min( hi , n - 1 );
   // stretches of 32 pairs with no branch inside, which vectorize; the
   // order looked for only while it may still hold
   bool up = true , down = true;
   for( std::size_t i = lo ; ( i < e ) && ( up || down ) ; ) {
    const std::size_t f = std::min( e , i + 32 );
    bool u = true , d = true;
    if( up )
     for( std::size_t j = i ; j < f ; ++j )
      u &= ! less( A[ j + 1 ] , A[ j ] );
    if( down )
     for( std::size_t j = i ; j < f ; ++j )
      d &= Stable ? less( A[ j + 1 ] , A[ j ] )
                  : ! less( A[ j ] , A[ j + 1 ] );
    up = up && u;
    down = down && d;
    i = f;
    }
   s.up = up;
   s.down = down;
   return( s );
   } ,
  []( S a , const S & b ) {
   a.up = a.up && b.up;
   a.down = a.down && b.down;
   return( a );
   } , 16384 );
 if( sv.up )
  return;
 if( sv.down ) {
  parallelFor( ex , 0 , n / 2 , [ & ]( std::size_t i ) {
   std::swap( A[ i ] , A[ n - 1 - i ] );
   } , 16384 );
  return;
  }
 if( fewRuns< Stable >( ex , A , less ) )
  return;
 const std::size_t k = n >= 256 * 4096 ? 256 :
                       std::max( std::bit_ceil( 4 * p ) ,
                                 std::bit_floor( n / 4096 ) );
 if constexpr( numbers ) {
  const RadixClassifier< T > cls( A );
  if( cls.spread() && ( cls.exact() || ( cls.distinct() > 32 ) ) ) {
   distribute< Stable , void >( ex , A , less , cls , depth );
   return;
   }
  // few distinct keys that the bits do not separate: counted, if no item
  // has another one (not for the stable sort of floating point numbers,
  // whose -0 and +0 are equal and differ)
  if constexpr( std::is_integral_v< T > || ! Stable )
   if( ( cls.distinct() <= 32 ) && fewKeys( ex , A ) )
    return;
  }
 {
  TreeClassifier< T , C > cls( A , std::min( k , std::size_t( 256 ) ) ,
                               less );
  if( cls.buckets() <= 256 )
   distribute< Stable , std::uint8_t >( ex , A , less , cls , depth );
  else
   distribute< Stable , std::uint16_t >( ex , A , less , cls , depth );
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , class T , class C >
void sort( E & ex , std::vector< T > & A , C less )
{
 detail::parallelSort< false >( ex , A , less , 0 );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() , less ) );
 }

template< Executor E , class T , class C >
void stableSort( E & ex , std::vector< T > & A , C less )
{
 detail::parallelSort< true >( ex , A , less , 0 );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelSort.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
