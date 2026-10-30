/*--------------------------------------------------------------------------*/
/*--------------------------- File RadixSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of RadixSort.h, with their contracts.
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

template< RadixKey T >
auto radixKey( T x )
{
 using U = std::conditional_t< sizeof( T ) == 1 , std::uint8_t ,
           std::conditional_t< sizeof( T ) == 2 , std::uint16_t ,
           std::conditional_t< sizeof( T ) <= 4 , std::uint32_t ,
                                                  std::uint64_t > > >;
 constexpr U sign = U( U( 1 ) << ( 8 * sizeof( T ) - 1 ) );
 if constexpr( std::is_floating_point_v< T > ) {
  const U u = std::bit_cast< U >( x );
  return( U( ( u & sign ) ? ~u : ( u | sign ) ) );
  }
 else if constexpr( std::is_signed_v< T > )
  return( U( U( x ) ^ sign ) );
 else
  return( U( x ) );
 }

namespace detail {

/// the counts of the bytes D[ 0 .. m ) of the unsigned key( x ) of the
/// items of s[ 0 .. n ) into H, in one sweep; the bytes 0 .. W - 1, all
/// those of H, by constant shifts
template< class T , class K , class N , std::size_t W >
void counts( const T * s , std::size_t n , K & key , const std::size_t * D ,
             std::size_t m , std::array< std::array< N , 256 > , W > & H )
{
 for( std::size_t t = 0 ; ( t < m ) && ( t < W ) ; ++t )
  H[ t ].fill( 0 );
 if( ( m == W ) && ( D[ W - 1 ] == W - 1 ) )
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const auto k = key( s[ i ] );
   for( std::size_t d = 0 ; d < W ; ++d )
    ++H[ d ][ std::size_t( ( k >> ( 8 * d ) ) & 0xFF ) ];
   }
 else
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const auto k = key( s[ i ] );
   for( std::size_t t = 0 ; t < m ; ++t )
    ++H[ t ][ std::size_t( ( k >> ( 8 * D[ t ] ) ) & 0xFF ) ];
   }
 }

/// one pass of the LSD sort, stable, from s[ 0 .. n ) to d by the byte of
/// the unsigned key( x ) at the shift sh, whose counts are H; the starts
/// of the buckets go to S, if given
template< class T , class K , class N >
void scatter( T * s , T * d , std::size_t n , K & key , int sh ,
              const N * H , std::size_t * S = nullptr )
{
 std::array< N , 256 > P;                   // the next place in each
 N sum = 0;
 for( std::size_t c = 0 ; c < 256 ; ++c ) {
  if( S )
   S[ c ] = sum;
  P[ c ] = sum;
  sum = N( sum + H[ c ] );
  }
 auto byte = [ & ]( const T & x ) {
  return( std::size_t( ( key( x ) >> sh ) & 0xFF ) ); };
 for( std::size_t i = 0 ; i < n ; ++i )
  d[ P[ byte( s[ i ] ) ]++ ] = std::move( s[ i ] );
 }

/// the LSD sort of s[ 0 .. n ) by the bytes D[ 0 .. m ) of the unsigned
/// key( x ), with r as the room of the passes, which go back and forth
/// between the two, and the one of the two where the items end sorted:
/// one sweep counts all the bytes, those in which the keys do not differ
/// make no pass, and at most 32 items are sorted by insertion, in s
template< class N , class T , class K >
T * lsdPasses( T * s , T * r , std::size_t n , K & key ,
               const std::size_t * D , std::size_t m )
{
 using U = std::decay_t< decltype( key( *s ) ) >;
 if( n <= 32 ) {
  for( std::size_t i = 1 ; i < n ; ++i ) {
   T x = std::move( s[ i ] );
   const U k = key( x );
   std::size_t j = i;
   for( ; ( j > 0 ) && ( k < key( s[ j - 1 ] ) ) ; --j )
    s[ j ] = std::move( s[ j - 1 ] );
   s[ j ] = std::move( x );
   }
  return( s );
  }
 std::array< std::array< N , 256 > , sizeof( U ) > H;
 counts( s , n , key , D , m , H );
 const U k0 = key( s[ 0 ] );
 for( std::size_t t = 0 ; t < m ; ++t ) {
  const int sh = int( 8 * D[ t ] );
  const std::size_t c0 = std::size_t( ( k0 >> sh ) & 0xFF );
  if( H[ t ][ c0 ] == n )
   continue;                                // all the same byte
  scatter( s , r , n , key , sh , H[ t ].data() );
  std::swap( s , r );
  }
 return( s );
 }

/// the radix sort of s[ 0 .. n ) by the bytes D[ 0 .. m ) of the unsigned
/// key( x ), with r as the room, and the one of the two where the items
/// end sorted: a part larger than the cache, with keys that differ in 3
/// bytes or more, goes first by its top byte into r, and each of its
/// parts is sorted in the same way and moved back to s, if it ends in r;
/// the others go to lsdPasses(), with the narrowest counters that hold n
template< class T , class K >
T * lsdSort( T * s , T * r , std::size_t n , K & key ,
             const std::size_t * D , std::size_t m )
{
 if( ( m < 3 ) || ( n * sizeof( T ) < ( std::size_t( 1 ) << 20 ) ) ) {
  if( n < ( std::size_t( 1 ) << 31 ) )
   return( lsdPasses< std::uint32_t >( s , r , n , key , D , m ) );
  return( lsdPasses< std::size_t >( s , r , n , key , D , m ) );
  }
 std::array< std::array< std::size_t , 256 > , 1 > H;
 counts( s , n , key , D + m - 1 , 1 , H );
 const int sh = int( 8 * D[ m - 1 ] );
 if( H[ 0 ][ std::size_t( ( key( s[ 0 ] ) >> sh ) & 0xFF ) ] == n )
  return( lsdSort( s , r , n , key , D , m - 1 ) );   // no split there
 std::array< std::size_t , 257 > S;
 scatter( s , r , n , key , sh , H[ 0 ].data() , S.data() );
 S[ 256 ] = n;
 for( std::size_t c = 0 ; c < 256 ; ++c ) {
  const std::size_t lo = S[ c ] , k = S[ c + 1 ] - lo;
  if( k == 0 )
   continue;
  T * const e = lsdSort( r + lo , s + lo , k , key , D , m - 1 );
  if( e != s + lo )
   std::move( e , e + k , s + lo );
  }
 return( s );
 }

/// the LSD sort of A by the unsigned key( x ), through a buffer: a first
/// sweep finds the bytes in which some keys differ, and only those are
/// counted and make a pass; if Exact, i.e., the key tells the item, and
/// the keys differ in one byte alone, a counting sort writes the items
/// back from one of each value, with no buffer and no pass that moves
/// them; the rest goes to lsdSort()
template< bool Exact , class T , class K >
void lsd( std::vector< T > & A , K & key )
{
 using U = std::decay_t< decltype( key( A[ 0 ] ) ) >;
 constexpr std::size_t W = sizeof( U );
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 U any = 0 , all = U( ~U( 0 ) );            // the bits set in some key
 for( const auto & x : A ) {                // and those set in all
  const U k = key( x );
  any |= k;
  all &= k;
  }
 std::array< std::size_t , W > D;          // the bytes that differ
 std::size_t m = 0;
 for( std::size_t d = 0 ; d < W ; ++d )
  if( ( ( any ^ all ) >> ( 8 * d ) ) & 0xFF )
   D[ m++ ] = d;
 if( m == 0 )
  return;                                   // all the keys are equal
 if constexpr( Exact )
  if( m == 1 ) {
   const int s = int( 8 * D[ 0 ] );
   std::array< std::size_t , 256 > h{};
   std::array< T , 256 > R{};               // an item of each byte
   for( const auto & x : A ) {
    const std::size_t c = std::size_t( ( key( x ) >> s ) & 0xFF );
    ++h[ c ];
    R[ c ] = x;
    }
   T * p = A.data();
   for( std::size_t c = 0 ; c < 256 ; ++c )
    p = std::fill_n( p , h[ c ] , R[ c ] );
   return;
   }
 T * const a = A.data();
 auto sortWith = [ & ]( T * const b ) {
  T * const e = lsdSort( a , b , n , key , D.data() , m );
  if( e != a )
   std::move( e , e + n , a );
  };
 if constexpr( std::is_trivially_default_constructible_v< T > )
  sortWith( std::make_unique_for_overwrite< T[] >( n ).get() );
 else {
  std::vector< T > B( n );
  sortWith( B.data() );
  }
 }

/// the sort of A[ lo .. lo + n ) by radixKey() with no further radix pass:
/// vectorQuickSort(), whose order on the floating point numbers is that of
/// IEEE 754 extended to a total order, the order of radixKey(), if the
/// registers are there; else pdqSort(), with the natural order on the
/// integers and by radixKey() on the floating point numbers
template< RadixKey T >
void flagGroup( std::vector< T > & A , std::size_t lo , std::size_t n )
{
 if constexpr( kVectorSort && VectorKey< T > )
  vectorQuickSort( A.data() + lo , n );
 else if constexpr( std::is_integral_v< T > ) {
  std::less<> less;
  pdq( A , lo , lo + n , int( std::bit_width( n ) ) , true , less );
  }
 else {
  auto less = []( T a , T b ) { return( radixKey( a ) < radixKey( b ) ); };
  pdq( A , lo , lo + n , int( std::bit_width( n ) ) , true , less );
  }
 }

/// the MSD sort in place of A[ lo .. lo + n ) by the bytes D[ 0 .. m ) of
/// radixKey(), the most significant first: the counts of the byte, and a
/// single bucket goes on with the next byte; otherwise each item of the
/// unfinished part of a bucket is swapped to the next free place of its
/// own, the buckets in turn until all are full, so that the loads of a
/// round do not wait for each other as along a cycle; then the buckets of
/// at least kFlagSmall items go on with the next byte, the others to
/// flagGroup()
template< RadixKey T >
void flag( std::vector< T > & A , std::size_t lo , std::size_t n ,
           const int * D , int m )
{
 T * const a = A.data() + lo;
 for( ; ; ) {
  const int sh = 8 * D[ 0 ];
  auto byte = [ sh ]( T x ) {
   return( std::size_t( ( radixKey( x ) >> sh ) & 0xFF ) ); };
  std::array< std::size_t , 256 > count{} , next , end;
  for( std::size_t i = 0 ; i < n ; ++i )
   ++count[ byte( a[ i ] ) ];
  std::array< unsigned char , 256 > full , todo;   // the non empty ones
  int k = 0;
  std::size_t sum = 0;
  for( std::size_t b = 0 ; b < 256 ; ++b ) {
   next[ b ] = sum;
   sum += count[ b ];
   end[ b ] = sum;
   if( count[ b ] )
    full[ k++ ] = static_cast< unsigned char >( b );
   }
  if( k == 1 ) {                            // one bucket: the next byte
   if( m == 1 )
    return;
   ++D;
   --m;
   continue;
   }
  std::copy( full.begin() , full.begin() + k , todo.begin() );
  for( int left = k ; left > 1 ; ) {        // the rounds
   int w = 0;
   for( int t = 0 ; t < left ; ++t ) {
    const std::size_t b = todo[ t ];
    T * x = a + next[ b ];
    for( std::size_t c = end[ b ] - next[ b ] ; c > 0 ; --c , ++x )
     std::swap( *x , a[ next[ byte( *x ) ]++ ] );
    if( next[ b ] != end[ b ] )
     todo[ w++ ] = static_cast< unsigned char >( b );
    }
   left = w;
   }
  if( m == 1 )
   return;
  for( int t = 0 ; t < k ; ++t ) {
   const std::size_t c = count[ full[ t ] ] , start = end[ full[ t ] ] - c;
   if( c >= kFlagSmall )
    flag( A , lo + start , c , D + 1 , m - 1 );
   else if( c > 1 )
    flagGroup( A , lo + start , c );
   }
  return;
  }
 }

/// the character d of s, 0 at its end, then the characters plus 1
inline std::uint16_t charAt( const std::string & s , std::size_t d )
{
 return( std::uint16_t( d < s.size() ? static_cast< unsigned char >( s[ d ] )
                                       + 1 : 0 ) );
 }

/// the length of the prefix that the strings of s[ 0 .. n ), n >= 2,
/// share from their character d, which they all have, found with one
/// comparison per character: a string that keeps the whole prefix costs
/// one call of memcmp(), and only one that cuts it is compared character
/// by character; equal tells whether the strings are all equal
inline std::size_t sharedPrefix( const std::string * s , std::size_t n ,
                                 std::size_t d , bool & equal )
{
 const char * f = s[ 0 ].data() + d;
 const std::size_t n0 = s[ 0 ].size();
 std::size_t l = n0 - d;
 bool same = true;                          // all as long as s[ 0 ]
 std::size_t i = 1;
 for( ; ( i < n ) && ( l > 0 ) ; ++i ) {
  const char * x = s[ i ].data() + d;
  const std::size_t m = std::min( l , s[ i ].size() - d );
  if( ( m < l ) || ( std::memcmp( x , f , m ) != 0 ) )
   l = std::size_t( std::mismatch( x , x + m , f ).first - x );
  same = same && ( s[ i ].size() == n0 );
  }
 equal = ( i == n ) && same && ( d + l == n0 );
 return( l );
 }

/// the MSD sort of the strings of s[ 0 .. n ) by their characters from d
/// on, through t, with the character d of each kept in k, so that it is
/// read once per pass; the strings share their first d characters. The
/// strings go back and forth between s and t, one move per pass, and end
/// in the array sorted, s if home and t otherwise; the prefix that all of
/// them share from d is skipped [see sharedPrefix()], so that equal keys
/// and long shared prefixes make no pass of their own
inline void msd( std::string * s , std::string * t , std::uint16_t * k ,
                 std::size_t n , std::size_t d , bool home )
{
 if( n <= 16 ) {                            // the rest of the strings
  for( std::size_t i = 1 ; i < n ; ++i )
   for( std::size_t j = i ; ( j > 0 ) &&
        ( s[ j ].compare( d , std::string::npos , s[ j - 1 ] , d ,
                          std::string::npos ) < 0 ) ; --j )
    std::swap( s[ j ] , s[ j - 1 ] );
  if( ! home )
   std::move( s , s + n , t );
  return;
  }
 bool equal;
 d += sharedPrefix( s , n , d , equal );
 std::array< std::size_t , 258 > c{};
 if( ! equal )
  for( std::size_t i = 0 ; i < n ; ++i ) {
   k[ i ] = charAt( s[ i ] , d );
   ++c[ k[ i ] + 1 ];
   }
 if( equal || ( c[ 1 ] == n ) ) {           // all equal
  if( ! home )
   std::move( s , s + n , t );
  return;
  }
 for( std::size_t r = 0 ; r + 1 < c.size() ; ++r ) c[ r + 1 ] += c[ r ];
 for( std::size_t i = 0 ; i < n ; ++i )
  t[ c[ k[ i ] ]++ ] = std::move( s[ i ] );
 for( std::size_t r = 0 ; r < 257 ; ++r ) {   // bucket r: [ b , e )
  const std::size_t b = r == 0 ? 0 : c[ r - 1 ] , e = c[ r ];
  if( ( r == 0 ) || ( e - b < 2 ) ) {       // sorted: back home
   if( home )
    std::move( t + b , t + e , s + b );
   }
  else
   msd( t + b , s + b , k + b , e - b , d + 1 , ! home );
  }
 }

/// msd() of the strings of s[ 0 .. n ) from their first character, with
/// no buffer at the first pass: the strings are permuted into their
/// buckets in place, following the cycles of the permutation, and a
/// buffer as large as the largest bucket is made only when a bucket is not
/// all of equal strings
inline void msdInPlace( std::string * s , std::uint16_t * k ,
                        std::size_t n )
{
 if( n <= 16 ) {
  msd( s , nullptr , k , n , 0 , true );
  return;
  }
 bool equal;
 const std::size_t d = sharedPrefix( s , n , 0 , equal );
 if( equal )
  return;
 std::array< std::size_t , 258 > c{};
 for( std::size_t i = 0 ; i < n ; ++i ) {
  k[ i ] = charAt( s[ i ] , d );
  ++c[ k[ i ] + 1 ];
  }
 for( std::size_t r = 0 ; r + 1 < c.size() ; ++r ) c[ r + 1 ] += c[ r ];
 std::array< std::size_t , 257 > next;      // bucket r: [ c[ r ] ,
 std::copy_n( c.begin() , 257 , next.begin() );   // c[ r + 1 ] )
 std::size_t most = 0;
 for( std::size_t r = 0 ; r < 257 ; ++r ) {
  most = std::max( most , c[ r + 1 ] - c[ r ] );
  while( next[ r ] < c[ r + 1 ] ) {         // the cycles into the buckets
   std::size_t x = k[ next[ r ] ];
   while( x != r ) {
    const std::size_t p = next[ x ]++;
    std::swap( s[ next[ r ] ] , s[ p ] );
    std::swap( k[ next[ r ] ] , k[ p ] );
    x = k[ next[ r ] ];
    }
   ++next[ r ];
   }
  }
 std::vector< std::string > T;
 for( std::size_t r = 1 ; r < 257 ; ++r ) {
  const std::size_t b = c[ r ] , m = c[ r + 1 ] - b;
  if( m < 2 )
   continue;
  if( m > 16 ) {
   sharedPrefix( s + b , m , d + 1 , equal );
   if( equal )
    continue;
   if( T.empty() )
    T.resize( most );
   }
  msd( s + b , T.data() , k + b , m , d + 1 , true );
  }
 }

/// the buckets of bucketSort() that a group of its first pass gathers
inline constexpr std::size_t kBucketGroup = 1024;

/// the items of s[ 0 .. k ), whose buckets are bucket( x ) - base in
/// [ 0 , L ), counted by their bucket in C[ 0 .. L ], scattered to A[ lo ..
/// lo + k ) in the order of their buckets and sorted there by insertion,
/// each within its bucket; a bucket of more than 32 items first by pdqSort(),
/// so that no bucket costs more than \f$ O( k \log k ) \f$
template< class N , class T , class K >
void bucketGroup( std::vector< T > & A , std::size_t lo , const T * s ,
                  std::size_t k , std::size_t base , std::size_t L , N * C ,
                  K & bucket )
{
 std::fill_n( C , L + 1 , N( 0 ) );
 for( std::size_t i = 0 ; i < k ; ++i )
  ++C[ bucket( s[ i ] ) - base + 1 ];
 bool big = false;
 for( std::size_t j = 1 ; j <= L ; ++j ) {
  big |= C[ j ] > 32;
  C[ j ] = N( C[ j ] + C[ j - 1 ] );
  }
 T * const d = A.data() + lo;
 for( std::size_t i = 0 ; i < k ; ++i ) {   // C[ j ] ends as the end of j
  const T x = s[ i ];
  d[ C[ bucket( x ) - base ]++ ] = x;
  }
 if( big ) {
  std::less<> less;
  for( std::size_t j = 0 , b = 0 ; j < L ; b = C[ j++ ] )
   if( C[ j ] - b > 32 )
    pdq( A , lo + b , lo + C[ j ] , int( std::bit_width( C[ j ] - b ) ) ,
         true , less );
  }
 for( std::size_t i = 1 ; i < k ; ++i ) {   // no item leaves its bucket
  const T x = d[ i ];
  std::size_t j = i;
  for( ; ( j > 0 ) && ( x < d[ j - 1 ] ) ; --j )
   d[ j ] = d[ j - 1 ];
  d[ j ] = x;
  }
 }

/// the n buckets of bucketSort() on A, with B as the room of n items: a
/// vector that fits in the cache (1 MiB) is copied to B and sorted back as
/// one group; a larger one first goes into B by groups of kBucketGroup
/// consecutive buckets, so that the passes over the buckets of a group stay
/// in the cache, and each group is sorted back to A
template< class N , class T >
void buckets( std::vector< T > & A , T * B )
{
 const std::size_t n = A.size();
 const T scale = T( n );
 auto bucket = [ n , scale ]( T x ) {      // x n , rounding kept below n
  const std::size_t b = std::size_t( x * scale );
  return( b < n ? b : n - 1 ); };
 const T * const a = A.data();
 if( n * sizeof( T ) <= ( std::size_t( 1 ) << 20 ) ) {
  std::copy( a , a + n , B );
  const auto C = std::make_unique_for_overwrite< N[] >( n + 1 );
  bucketGroup( A , 0 , B , n , 0 , n , C.get() , bucket );
  return;
  }
 const std::size_t G = ( n - 1 ) / kBucketGroup + 1;
 const auto S = std::make_unique< N[] >( G + 1 );
 for( std::size_t i = 0 ; i < n ; ++i )
  ++S[ bucket( a[ i ] ) / kBucketGroup + 1 ];
 for( std::size_t g = 1 ; g <= G ; ++g )
  S[ g ] = N( S[ g ] + S[ g - 1 ] );
 {
  const auto P = std::make_unique_for_overwrite< N[] >( G );
  std::copy( S.get() , S.get() + G , P.get() );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const T x = a[ i ];
   B[ P[ bucket( x ) / kBucketGroup ]++ ] = x;
   }
  }
 std::array< N , kBucketGroup + 1 > C;
 for( std::size_t g = 0 ; g < G ; ++g ) {
  const std::size_t lo = S[ g ] , k = S[ g + 1 ] - lo;
  if( k > 0 )
   bucketGroup( A , lo , B + lo , k , g * kBucketGroup ,
                std::min( kBucketGroup , n - g * kBucketGroup ) , C.data() ,
                bucket );
  }
 }

}  // end( namespace detail )

template< RadixKey T >
void radixSort( std::vector< T > & A )
{
 const auto before = detail::auditCopy( A );
 auto key = []( T x ) { return( radixKey( x ) ); };
 detail::lsd< true >( A , key );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T a , T b ) {
  return( radixKey( a ) < radixKey( b ) ); } ) );
 }

template< class T , class K >
void radixSortBy( std::vector< T > & A , K key )
{
 using U = std::decay_t< decltype( key( A[ 0 ] ) ) >;
 static_assert( std::is_unsigned_v< U > , "the key must be unsigned" );
 detail::lsd< false >( A , key );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  for( std::size_t i = 1 ; i < A.size() ; ++i )
   if( key( A[ i ] ) < key( A[ i - 1 ] ) ) return( false );
  return( true );
  } ) );
 }

template< RadixKey T >
void americanFlagSort( std::vector< T > & A )
{
 using U = decltype( radixKey( T() ) );
 constexpr int W = int( sizeof( U ) );
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 U any = 0 , all = U( ~U( 0 ) );            // the bits set in some key, in
 std::size_t up = 0 , down = 0;             // all, and the steps up and
 if( n > 0 ) {                              // down from one key to the next
  U prev = radixKey( A[ 0 ] );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const U k = radixKey( A[ i ] );
   any |= k;
   all &= k;
   up += k > prev;
   down += k < prev;
   prev = k;
   }
  }
 std::array< int , W > D;                   // the bytes that differ, the
 int m = 0;                                 // most significant first
 for( int d = W - 1 ; d >= 0 ; --d )
  if( ( ( any ^ all ) >> ( 8 * d ) ) & 0xFF )
   D[ m++ ] = d;
 if( ( n < 2 ) || ( down == 0 ) )
  ;                                         // sorted already
 else if( up == 0 )
  std::reverse( A.begin() , A.end() );      // in reverse order
 else if( m == 1 ) {                        // one byte: the key tells the
  const int s = 8 * D[ 0 ];                 // number, counted and written
  std::array< std::size_t , 256 > h{};      // back from one of each value
  std::array< T , 256 > R{};
  for( const T x : A ) {
   const std::size_t c = std::size_t( ( radixKey( x ) >> s ) & 0xFF );
   ++h[ c ];
   R[ c ] = x;
   }
  T * p = A.data();
  for( std::size_t c = 0 ; c < 256 ; ++c )
   p = std::fill_n( p , h[ c ] , R[ c ] );
  }
 else if( n < kFlagSmall )
  detail::flagGroup( A , 0 , n );
 else
  detail::flag( A , 0 , n , D.data() , m );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T a , T b ) {
  return( radixKey( a ) < radixKey( b ) ); } ) );
 }

inline void msdRadixSort( std::vector< std::string > & A )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 if( n > 1 ) {
  const auto K = std::make_unique_for_overwrite< std::uint16_t[] >( n );
  detail::msdInPlace( A.data() , K.get() , n );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

template< std::floating_point T >
void bucketSort( std::vector< T > & A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() , []( T x ) {
  return( ( x >= 0 ) && ( x < 1 ) ); } ) );
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 if( n > 1 ) {
  const auto B = std::make_unique_for_overwrite< T[] >( n );
  if( n < ( std::size_t( 1 ) << 32 ) )
   detail::buckets< std::uint32_t >( A , B.get() );
  else
   detail::buckets< std::size_t >( A , B.get() );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RadixSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
