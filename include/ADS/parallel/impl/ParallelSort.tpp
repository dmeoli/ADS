/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel sorts of ParallelSort.h: the survey of the order, the two
 * classifiers, the distribution into the buckets and their sorts.
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

#include <atomic>
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

/// the key of x for the classifier of the numbers, -0 as +0
template< class T >
auto sortKey( T x )
{
 if constexpr( std::is_floating_point_v< T > )
  if( x == T( 0 ) )
   x = T( 0 );
 return( radixKey( x ) );
 }

/// the sequential sort of the parts
template< bool Stable , class T , class C >
void sequentialSort( std::vector< T > & A , C & less )
{
 if constexpr( Stable )
  stableSort( A , less );
 else
  sort( A , less );
 }

/// room for n items, not constructed, from the allocator of huge pages,
/// which keeps a large block for the next sort of the thread with its
/// pages already faulted in
template< class T >
class RawBuffer {
 public:
 explicit RawBuffer( std::size_t n )
  : f_n( std::max< std::size_t >( n , 1 ) ) ,
    f_p( HugePageAllocator< T >().allocate( f_n ) ) {}
 ~RawBuffer() { HugePageAllocator< T >().deallocate( f_p , f_n ); }
 RawBuffer( const RawBuffer & ) = delete;
 RawBuffer & operator=( const RawBuffer & ) = delete;
 [[nodiscard]] T * data( void ) const { return( f_p ); }
 private:
 std::size_t f_n;
 T * f_p;
 };

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
  }

 [[nodiscard]] std::size_t buckets( void ) const { return( f_last + 1 ); }

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
 RawBuffer< T > buf( n );
 T * const B = buf.data();
 ex.bulk( nb , [ & ]( std::size_t j ) {
  const std::size_t lo = j * bs , hi = std::min( n , lo + bs );
  std::vector< std::size_t > c( cnt.begin() + std::ptrdiff_t( j * k ) ,
                                cnt.begin() + std::ptrdiff_t( j * k + k ) );
  for( std::size_t i = lo ; i < hi ; ++i ) {
   std::size_t b;
   if constexpr( keep )
    b = id[ i ];
   else
    b = cls( A[ i ] );
   std::construct_at( B + c[ b ]++ , std::move( A[ i ] ) );
   }
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
   v.assign( std::make_move_iterator( B + s ) ,
             std::make_move_iterator( B + s + m ) );
   std::destroy( B + s , B + s + m );
   sequentialSort< Stable >( v , less );
   std::move( v.begin() , v.end() , A.begin() + std::ptrdiff_t( s ) );
   }
  } );
 }

/// the survey of the order of A: whether sorted, whether decreasing
struct Survey {
 bool up = true , down = true;
 };

template< bool Stable , class E , class T , class C >
void parallelSort( E & ex , std::vector< T > & A , C & less , int depth )
{
 const std::size_t n = A.size() , p = ex.concurrency();
 if( ( p == 1 ) || ( n < kParallelSort ) || ( depth > 3 ) ) {
  sequentialSort< Stable >( A , less );
  return;
  }
 constexpr bool numbers = naturalNumbers< T , C >;
 using S = Survey;
 const S sv = parallelReduce( ex , 0 , n , S() ,
  [ & ]( std::size_t lo , std::size_t hi ) {
   S s;
   const std::size_t e = std::min( hi , n - 1 );
   for( std::size_t i = lo ; ( i < e ) && ( s.up || s.down ) ; ++i ) {
    if( less( A[ i + 1 ] , A[ i ] ) )
     s.up = false;
    if( Stable ? ! less( A[ i + 1 ] , A[ i ] ) : less( A[ i ] , A[ i + 1 ] ) )
     s.down = false;
    }
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
 const std::size_t k = n >= 256 * 4096 ? 256 :
                       std::max( std::bit_ceil( 4 * p ) ,
                                 std::bit_floor( n / 4096 ) );
 if constexpr( numbers ) {
  distribute< Stable , void >( ex , A , less ,
                               RadixClassifier< T >( A ) ,
                               depth );
  }
 else {
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
