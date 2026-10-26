/*--------------------------------------------------------------------------*/
/*-------------------------- File SampleSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The samplesort of SampleSort.h.
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

namespace detail::ss {

/// the largest number of buckets, as its logarithm
inline constexpr int kLogBuckets = 8;

/// the parts up to which a bucket is sorted at once, by insertion
inline constexpr std::size_t kBase = 64;

/// the size of the buckets of the last level that the number of buckets
/// aims at
inline constexpr std::size_t kLeaf = 16;

/// the items of a block, about 2 KiB of them
template< class T >
inline constexpr std::size_t kBlock = sizeof( T ) >= 2048 ? 1 :
                                      2048 / sizeof( T );

/// the items that go down the tree together
inline constexpr std::size_t kUnroll = 4;

/// the insertion sort of a[ 0 .. n ), n >= 1, a bucket of the last level:
/// an item smaller than a[ 0 ] goes to the front at once, and any other
/// stops on a smaller item before the front, with no test of the front
template< class T , class C >
void insertion( T * a , std::size_t n , C & less )
{
 for( std::size_t i = 1 ; i < n ; ++i ) {
  T x = std::move( a[ i ] );
  if( less( x , a[ 0 ] ) ) {
   std::move_backward( a , a + i , a + i + 1 );
   a[ 0 ] = std::move( x );
   }
  else {
   std::size_t j = i;
   for( ; less( x , a[ j - 1 ] ) ; --j ) a[ j ] = std::move( a[ j - 1 ] );
   a[ j ] = std::move( x );
   }
  }
 }

template< class T , class C >
class Sorter {
 public:
  Sorter( std::vector< T > & A , C & less ) : A( A ) , less( less ) {}

  /// A[ lo .. hi ) sorted
  void sort( std::size_t lo , std::size_t hi );

 private:
  static constexpr std::size_t B = kBlock< T >;

  std::vector< T > & A;
  C & less;
  std::unique_ptr< T[] > buf;               // the buffers of the buckets,
  std::size_t bufSize = 0;                  // not initialized for bytes,
  std::unique_ptr< T[] > room;              // two blocks and the overflow
  T tree[ 1 << kLogBuckets ];               // t[ 1 .. k ) of the splitters
  T low[ 1 << kLogBuckets ];                // s_i of the bucket i > 0
  std::uint64_t seed = 0x9E3779B97F4A7C15ull;
  int logk = 0;
  bool equal = false;

  std::uint64_t random() {
   seed ^= seed << 13;
   seed ^= seed >> 7;
   seed ^= seed << 17;
   return( seed );
   }

  /// the bucket of x
  std::size_t bucket( const T & x ) const {
   std::size_t j = 1;
   for( int l = 0 ; l < logk ; ++l )
    j = 2 * j + ! less( x , tree[ j ] );
   const std::size_t i = j - ( std::size_t( 1 ) << logk );
   return( equal ? 2 * i + ! ( ( i != 0 ) & ! less( low[ i ] , x ) ) : i );
   }

  /// the splitters from a sample of A[ lo .. hi ): false if the sample
  /// holds one key alone
  bool splitters( std::size_t lo , std::size_t hi );

  /// t[ j ] and below from the sorted s[ a .. b )
  void build( const T * s , std::size_t j , std::size_t a , std::size_t b );
 };

template< class T , class C >
void Sorter< T , C >::build( const T * s , std::size_t j , std::size_t a ,
                             std::size_t b )
{
 const std::size_t m = a + ( b - a ) / 2;
 tree[ j ] = s[ m ];
 if( 2 * j < ( std::size_t( 1 ) << logk ) ) {
  build( s , 2 * j , a , m );
  build( s , 2 * j + 1 , m + 1 , b );
  }
 }

template< class T , class C >
bool Sorter< T , C >::splitters( std::size_t lo , std::size_t hi )
{
 const std::size_t n = hi - lo;
 const int levels = int( std::bit_width( n / kLeaf ) ) - 1;  // log2
 logk = levels <= kLogBuckets ? std::max( 1 , levels ) :
        levels <= 2 * kLogBuckets ? ( levels + 1 ) / 2 : kLogBuckets;
 std::size_t k = std::size_t( 1 ) << logk;
 const std::size_t alpha = std::max( std::size_t( 1 ) ,
                                     std::size_t( std::bit_width( n ) / 5 ) );
 const std::size_t s = alpha * k - 1;
 T * const a = A.data() + lo;
 for( std::size_t i = 0 ; i < s ; ++i )     // the sample at the front,
  std::swap( a[ i ] , a[ i + std::size_t(   // a place drawn by a product
   ( ( random() >> 32 ) * ( n - i ) ) >> 32 ) ] );
 pdq( A , lo , lo + s , int( std::bit_width( s ) ) , true , less );
 std::size_t m = 0;                         // the splitters, once each,
 for( std::size_t i = alpha - 1 ; i < s ; i += alpha )   // to low[ 1 .. ]
  if( ( m == 0 ) || less( low[ m ] , a[ i ] ) )
   low[ ++m ] = a[ i ];
 equal = m < k - 1;
 if( equal ) {
  if( ! less( a[ 0 ] , a[ s - 1 ] ) )       // the sample all equal
   return( false );
  logk = int( std::bit_width( m ) );         // k > m
  k = std::size_t( 1 ) << logk;
  }
 for( ; m < k - 1 ; ++m )
  low[ m + 1 ] = low[ m ];
 build( low + 1 , 1 , 0 , k - 1 );
 low[ 0 ] = low[ 1 ];
 return( true );
 }

template< class T , class C >
void Sorter< T , C >::sort( std::size_t lo , std::size_t hi )
{
 const std::size_t n = hi - lo;
 if( n <= kBase ) {
  if( n > 1 )
   insertion( A.data() + lo , n , less );
  return;
  }
 if( ! splitters( lo , hi ) ) {             // a sample of one key
  pdq( A , lo , hi , int( std::bit_width( n ) ) , true , less );
  return;
  }
 const std::size_t k = std::size_t( 1 ) << logk;
 const std::size_t nb = equal ? 2 * k : k;  // the buckets
 if( bufSize < nb * B ) {
  buf = std::make_unique_for_overwrite< T[] >( bufSize = nb * B );
  if( ! room )
   room = std::make_unique_for_overwrite< T[] >( 3 * B );
  }
 T * const a = A.data() + lo , * const bf = buf.get();

 // the classification: full buffers back to a[ 0 .. w ) as blocks
 std::size_t cnt[ 2 << kLogBuckets ] , full[ 2 << kLogBuckets ];
 std::fill( cnt , cnt + nb , 0 );
 std::fill( full , full + nb , 0 );
 std::size_t w = 0;
 auto put = [ & ]( std::size_t b , T & x ) {
  if( cnt[ b ] == B ) {
   std::move( bf + b * B , bf + ( b + 1 ) * B , a + w );
   w += B;
   ++full[ b ];
   cnt[ b ] = 0;
   }
  bf[ b * B + cnt[ b ]++ ] = std::move( x );
  };
 std::size_t i = 0;
 for( ; i + kUnroll <= n ; i += kUnroll ) {
  std::size_t j[ kUnroll ];
  for( std::size_t u = 0 ; u < kUnroll ; ++u ) j[ u ] = 1;
  for( int l = 0 ; l < logk ; ++l )
   for( std::size_t u = 0 ; u < kUnroll ; ++u )
    j[ u ] = 2 * j[ u ] + ! less( a[ i + u ] , tree[ j[ u ] ] );
  if( equal )
   for( std::size_t u = 0 ; u < kUnroll ; ++u ) {
    const std::size_t h = j[ u ] - k;
    j[ u ] = 2 * h + ! ( ( h != 0 ) & ! less( low[ h ] , a[ i + u ] ) );
    }
  else
   for( std::size_t u = 0 ; u < kUnroll ; ++u ) j[ u ] -= k;
  for( std::size_t u = 0 ; u < kUnroll ; ++u ) put( j[ u ] , a[ i + u ] );
  }
 for( ; i < n ; ++i ) put( bucket( a[ i ] ) , a[ i ] );

 // the starts of the buckets, and the areas of their blocks
 std::size_t st[ ( 2 << kLogBuckets ) + 1 ];
 std::size_t wp[ 2 << kLogBuckets ] , rp[ 2 << kLogBuckets ];
 st[ 0 ] = 0;
 for( std::size_t b = 0 ; b < nb ; ++b )
  st[ b + 1 ] = st[ b ] + full[ b ] * B + cnt[ b ];
 const std::size_t W = w / B;               // the blocks written
 for( std::size_t b = 0 ; b < nb ; ++b ) {
  wp[ b ] = ( st[ b ] + B - 1 ) / B;
  rp[ b ] = std::max( wp[ b ] , std::min( ( st[ b + 1 ] + B - 1 ) / B , W ) );
  }

 // the permutation of the blocks, through two blocks of room; a block
 // whose place goes past n goes to the overflow
 T * cur = room.get() , * oth = room.get() + B;
 T * const over = room.get() + 2 * B;
 std::size_t overSlot = std::size_t( -1 );
 for( std::size_t b = 0 ; b < nb ; ++b )
  while( rp[ b ] > wp[ b ] ) {
   --rp[ b ];
   std::move( a + rp[ b ] * B , a + ( rp[ b ] + 1 ) * B , cur );
   for( ; ; ) {
    const std::size_t t = bucket( cur[ 0 ] );
    const std::size_t d = wp[ t ]++;
    if( d < rp[ t ] ) {                     // a block not yet placed
     std::move( a + d * B , a + ( d + 1 ) * B , oth );
     std::move( cur , cur + B , a + d * B );
     std::swap( cur , oth );
     }
    else {
     if( ( d + 1 ) * B > n ) {
      std::move( cur , cur + B , over );
      overSlot = d;
      }
     else
      std::move( cur , cur + B , a + d * B );
     break;
     }
    }
   }
 if( overSlot != std::size_t( -1 ) )        // the part within the vector
  std::move( over , over + ( n - overSlot * B ) , a + overSlot * B );

 // the cleanup: the buffer and the items past its end of each bucket into
 // the places of its range that its blocks leave
 for( std::size_t b = 0 ; b < nb ; ++b ) {
  const std::size_t s = st[ b ] , e = st[ b + 1 ];
  const std::size_t bb = ( s + B - 1 ) / B * B;
  const std::size_t be = full[ b ] ? bb + full[ b ] * B : e;   // the blocks
  std::size_t f = s;                        // the next free place
  const std::size_t f1 = std::min( bb , e );
  const std::size_t f2 = full[ b ] ? be : f1;
  auto place = [ & ]( T & x ) {
   if( f == f1 ) f = f2;
   a[ f++ ] = std::move( x );
   };
  for( std::size_t h = 0 ; h < cnt[ b ] ; ++h ) place( bf[ b * B + h ] );
  for( std::size_t p = e ; p < be ; ++p )
   place( p < n ? a[ p ] : over[ p - overSlot * B ] );
  if( ( e - s > 1 ) && ( e - s <= kBase ) &&  // a small bucket at once,
      ! ( equal && ! ( b & 1 ) ) )            // while in the cache
   insertion( a + s , e - s , less );
  }

 // the buckets, but those of the items equal to a splitter
 const bool eq = equal;
 for( std::size_t b = 0 ; b < nb ; ++b ) {
  const std::size_t m = st[ b + 1 ] - st[ b ];
  if( ( m <= kBase ) || ( eq && ! ( b & 1 ) ) )
   continue;
  if( m > n / 2 )                           // an unlucky sample
   pdq( A , lo + st[ b ] , lo + st[ b + 1 ] , int( std::bit_width( m ) ) ,
        true , less );
  else
   sort( lo + st[ b ] , lo + st[ b + 1 ] );
  }
 }

}  // end( namespace detail::ss )

template< class T , class C >
void sampleSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 detail::ss::Sorter< T , C > S( A , less );
 S.sort( 0 , A.size() );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SampleSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
