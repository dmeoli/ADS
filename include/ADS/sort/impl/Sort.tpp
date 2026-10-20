/*--------------------------------------------------------------------------*/
/*----------------------------- File Sort.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of Sort.h.
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

/// the end of the stretch of a[ 0 .. n ) from j where each item is in
/// order with the one before it, i.e., where less( a[ k ] , a[ k - 1 ] )
/// is Down for every k; for items copied as bytes, blocks of 32 items with
/// no branch inside, which the compiler vectorizes for numbers and which
/// keep more loads in flight for the others, and the last one item by
/// item
template< bool Down , class T , class C >
std::size_t runEnd( const T * a , std::size_t j , std::size_t n , C & less )
{
 if constexpr( std::is_trivially_copyable_v< T > )
  for( ; j + 32 <= n ; j += 32 ) {
   unsigned stop = 0;
   for( std::size_t k = j ; k < j + 32 ; ++k )
    stop |= unsigned( less( a[ k ] , a[ k - 1 ] ) != Down );
   if( stop )
    break;
   }
 while( ( j < n ) && ( less( a[ j ] , a[ j - 1 ] ) == Down ) ) ++j;
 return( j );
 }

/// the runs of A as powerSort() finds them, the maximal non-decreasing
/// or strictly decreasing stretches, counted up to cap + 1: one pass that
/// stops at the run cap + 1, so that an input in no order costs O( cap );
/// the end of each goes to E, room for cap + 1
template< class T , class C >
std::size_t runs( const std::vector< T > & A , C & less , std::size_t cap ,
                  std::size_t * E )
{
 const std::size_t n = A.size();
 const T * const a = A.data();
 std::size_t r = 0;
 for( std::size_t i = 0 ; ( i < n ) && ( r <= cap ) ; ++r ) {
  std::size_t j = i + 1;
  if( ( j < n ) && less( a[ j ] , a[ i ] ) )
   j = runEnd< true >( a , j + 1 , n , less );
  else
   j = runEnd< false >( a , j , n , less );
  E[ r ] = i = j;
  }
 return( r );
 }

/// powerSort() of A on the r runs that runs() found, ending at E[ 0 .. r ):
/// each strictly decreasing one reversed and the runs merged, with no
/// second search for them
template< class T , class C >
void powerSortRuns( std::vector< T > & A , C & less , const std::size_t * E ,
                    std::size_t r )
{
 const auto before = auditCopy( A );
 RunMerger< T , C > M( A , less );
 std::size_t t = 0;
 powerMerges( M , A.size() , [ & ]( std::size_t lo ) {
  const std::size_t e = E[ t++ ];
  if( ( e - lo > 1 ) && less( A[ lo + 1 ] , A[ lo ] ) )
   std::reverse( A.begin() + std::ptrdiff_t( lo ) ,
                 A.begin() + std::ptrdiff_t( e ) );
  return( std::pair( e , false ) ); } );
 ADS_POST( t == r );
 ADS_AUDIT( sortedPermutation( A , before , less ) );
 }

/// A sorted when it is one run: left as it is, or reversed when it is
/// strictly decreasing, which is stable since its keys are distinct
template< class T , class C >
void oneRun( std::vector< T > & A , C & less )
{
 if( ( A.size() > 1 ) && less( A[ 1 ] , A[ 0 ] ) )
  std::reverse( A.begin() , A.end() );
 }

/// the distinct keys, up to 64, of a sample of 64 equally spaced keys of A,
/// n >= 64, and the bytes in which they differ
template< RadixKey T >
std::pair< std::size_t , int > sampleKeys( const std::vector< T > & A )
{
 using U = decltype( radixKey( T() ) );
 std::array< U , 64 > S;
 const std::size_t step = A.size() / S.size();
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  S[ i ] = radixKey( A[ i * step ] );
 std::sort( S.begin() , S.end() );
 std::size_t k = 1;
 std::uint64_t diff = 0;                    // the bits that differ
 for( std::size_t i = 1 ; i < S.size() ; ++i ) {
  k += S[ i ] != S[ i - 1 ];
  diff |= std::uint64_t( S[ i ] ^ S[ 0 ] );
  }
 int bytes = 0;
 for( ; diff != 0 ; diff >>= 8 )
  bytes += ( diff & 0xFF ) != 0;
 return( std::pair( k , bytes ) );
 }

/// true if a sample of 64 equally spaced keys of A, n >= 64, holds at most
/// 16 distinct ones and they differ in at least 4 bytes: few keys, but
/// wide, which pdqSort() sorts in \f$ O( n \log k ) \f$ with fewer passes
/// than radixSort() needs bytes
template< RadixKey T >
bool fewWideKeys( const std::vector< T > & A )
{
 const auto [ k , bytes ] = sampleKeys( A );
 return( ( k <= 16 ) && ( bytes >= 4 ) );
 }

/// the runs up to which the numbers of T go to powerSort() when the vector
/// registers are there: fewer for the keys of 32 bits, which
/// vectorQuickSort() sorts faster than the merges of 16 runs
template< class T >
inline constexpr std::size_t kVectorRuns = sizeof( T ) == 4 ? 8 : kRuns;

/// true if A holds a -0
template< class T >
bool negativeZero( const std::vector< T > & A )
{
 using U = std::conditional_t< sizeof( T ) == 4 , std::uint32_t ,
                               std::uint64_t >;
 constexpr U z = U( 1 ) << ( 8 * sizeof( T ) - 1 );
 std::size_t c = 0;                         // a count, which vectorizes
 for( const T x : A ) {
  U b;
  std::memcpy( &b , &x , sizeof( b ) );
  c += b == z;
  }
 return( c > 0 );
 }

/// the stable sort of A, n >= 64, if a sample of 64 equally spaced items
/// holds at most 16 distinct keys by less and every item has one of them,
/// false otherwise, A unchanged: few keys, each item found among them by a
/// binary search with no branch, and moved by them as by a counting sort,
/// through a buffer of n / 2 items. The left half goes by its keys into
/// the buffer and the right one into the room that the left one left,
/// both stable; then, from the largest key down, the block of each key of
/// the right half moves to its place, which is never before it, and that
/// of the left half follows it from the buffer
template< class T , class C >
bool fewKeysSort( std::vector< T > & A , C & less )
{
 std::array< T , 64 > S;
 const std::size_t n = A.size() , step = n / S.size();
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  S[ i ] = A[ i * step ];
 std::sort( S.begin() , S.end() , less );
 std::size_t k = 1;                         // the distinct keys
 for( std::size_t i = 1 ; i < S.size() ; ++i )
  if( less( S[ k - 1 ] , S[ i ] ) ) {
   if( k == 16 )
    return( false );
   S[ k++ ] = S[ i ];
   }
 const auto I = std::make_unique_for_overwrite< std::uint8_t[] >( n );
 const std::size_t r = n / 2 , l = n - r;
 std::array< std::size_t , 17 > L{} , R{};  // the counts by key, of the
 T * const a = A.data();                    // left half and of the right
 for( std::size_t i = 0 ; i < n ; ++i ) {
  std::size_t b = 0;                        // the first key not before
  for( std::size_t len = k ; len > 1 ; ) {  // a[ i ]
   const std::size_t half = len / 2;
   b += less( S[ b + half ] , a[ i ] ) ? half : 0;
   len -= half;
   }
  b += less( S[ b ] , a[ i ] );
  if( ( b == k ) || less( a[ i ] , S[ b ] ) )
   return( false );                         // a key out of the sample
  I[ i ] = std::uint8_t( b );
  ++( i < l ? L : R )[ b + 1 ];
  }
 for( std::size_t b = 0 ; b < k ; ++b ) {   // the starts
  L[ b + 1 ] += L[ b ];
  R[ b + 1 ] += R[ b ];
  }
 const auto before = auditCopy( A );
 const auto B = std::make_unique_for_overwrite< T[] >( l );
 T * const buf = B.get();
 auto nl = L , nr = R;
 for( std::size_t i = 0 ; i < l ; ++i )
  buf[ nl[ I[ i ] ]++ ] = a[ i ];
 for( std::size_t i = l ; i < n ; ++i )
  a[ nr[ I[ i ] ]++ ] = a[ i ];
 for( std::size_t b = k ; b-- > 0 ; ) {
  const std::size_t t = L[ b ] + R[ b ];    // the place of the key b
  std::copy_backward( a + R[ b ] , a + R[ b + 1 ] ,
                      a + t + L[ b + 1 ] - L[ b ] + R[ b + 1 ] - R[ b ] );
  std::copy( buf + L[ b ] , buf + L[ b + 1 ] , a + t );
  }
 ADS_AUDIT( sortedPermutation( A , before , less ) );
 return( true );
 }

/// a if c and b otherwise, by a mask, so that the compiler makes no
/// branch of it
inline std::size_t select( bool c , std::size_t a , std::size_t b )
{
 return( b ^ ( ( a ^ b ) & ( std::size_t( 0 ) - c ) ) );
 }

/// the stable merge of the sorted halves s[ 0 .. l ) and s[ l .. n ), with
/// l = n / 2 and n >= 2, into d[ 0 .. n ), from both ends at once: the
/// front takes the n - l smallest items, the left one on a tie, and the
/// back the l largest, the right one on a tie; since the right half is not
/// shorter, neither end runs out of a half before its last step, so that
/// no step checks where a half ends, and the two chains of comparisons
/// overlap; each item is chosen with no branch, through its index
template< class T , class C >
void bothEndsMerge( const T * s , T * d , std::size_t n , C & less )
{
 const std::size_t l = n / 2;
 std::size_t i = 0 , j = l , ie = l - 1 , je = n - 1;
 T * f = d , * b = d + n;
 for( std::size_t t = 0 ; t < l ; ++t ) {
  const bool r = less( s[ j ] , s[ i ] );
  *f++ = s[ select( r , j , i ) ];
  j += r;
  i += ! r;
  const bool q = less( s[ je ] , s[ ie ] );
  *--b = s[ select( q , ie , je ) ];
  ie -= q;
  je -= ! q;
  }
 if( n - l > l )                            // the middle item: s[ i ] is
  *f = s[ less( s[ j ] , s[ i ] ) ? j : i ];   // s[ l ] if the left ended
 }

/// the stable merge sort of s[ 0 .. n ) into d[ 0 .. n ) if into, or in
/// place otherwise, with the other as the room of the merges: the halves
/// sorted into the other one and merged back, so that each level moves an
/// item once; two halves already in order, or in reverse, are moved
/// whole, and at most 16 items are sorted by insertion
template< class T , class C >
void mergeSortTo( T * s , T * d , std::size_t n , C & less , bool into )
{
 if( n <= 16 ) {
  T * const o = into ? d : s;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const T x = s[ i ];
   std::size_t k = i;
   for( ; ( k > 0 ) && less( x , o[ k - 1 ] ) ; --k ) o[ k ] = o[ k - 1 ];
   o[ k ] = x;
   }
  return;
  }
 const std::size_t l = n / 2;
 mergeSortTo( s , d , l , less , ! into );
 mergeSortTo( s + l , d + l , n - l , less , ! into );
 const T * const x = into ? s : d;          // the sorted halves
 T * const y = into ? d : s;
 if( ! less( x[ l ] , x[ l - 1 ] ) )
  std::copy( x , x + n , y );
 else if( less( x[ n - 1 ] , x[ 0 ] ) )
  std::copy( x , x + l , std::copy( x + l , x + n , y ) );
 else
  bothEndsMerge( x , y , n , less );
 }

/// the stable merge sort of A, through a buffer B of half its items, as
/// the spinsort of Boost: the left half, the larger, sorted into B, the
/// right one in place with the left one as its room, and the two merged
/// forward into A, which never overtakes the right half
template< class T , class C >
void pingPongSort( std::vector< T > & A , C & less )
{
 const auto before = auditCopy( A );
 const std::size_t n = A.size() , r = n / 2 , l = n - r;
 if( n > 1 ) {
  T * const a = A.data();
  const auto B = std::make_unique_for_overwrite< T[] >( l );
  T * const b = B.get();
  mergeSortTo( a , b , l , less , true );
  mergeSortTo( a + l , a , r , less , false );
  if( ! less( a[ l ] , b[ l - 1 ] ) )        // in order
   std::copy( b , b + l , a );
  else if( less( a[ n - 1 ] , b[ 0 ] ) )     // in reverse
   std::copy( b , b + l , std::copy( a + l , a + n , a ) );
  else {
   std::size_t i = 0 , j = l , k = 0;
   const auto pb = std::uintptr_t( b ) , pa = std::uintptr_t( a );
   while( ( i < l ) && ( j < n ) ) {
    const bool q = less( a[ j ] , b[ i ] );
    a[ k++ ] = *reinterpret_cast< const T * >(
     select( q , pa + j * sizeof( T ) , pb + i * sizeof( T ) ) );
    j += q;
    i += ! q;
    }
   std::copy( b + i , b + l , a + k );       // the right rest in place
   }
  }
 ADS_AUDIT( sortedPermutation( A , before , less ) );
 }

/// sort() of the numbers of 32 or 64 bits when the vector registers are
/// there, and stableSort() of them if stable: a vector of one run left as
/// it is or reversed, one of at most kVectorRuns< T > runs by the merges of
/// powerSort(), the others by vectorQuickSort(); for stableSort() of the
/// floating point numbers with a -0, whose order to +0 vectorQuickSort()
/// does not keep, powerSort() below kRadix items and the radix sort with
/// -0 and +0 as one key from there
template< class T , class C >
void vectorNumbers( std::vector< T > & A , C & less , bool stable )
{
 auto zeros = [ & ]() {
  return( stable && std::is_floating_point_v< T > && negativeZero( A ) ); };
 if( A.size() < kRadix ) {
  if( zeros() )
   powerSort( A , less );
  else
   vectorQuickSort( A );
  return;
  }
 std::array< std::size_t , kRuns + 1 > E;
 const std::size_t r = runs( A , less , kRuns , E.data() );
 if( r <= 1 )
  oneRun( A , less );
 else if( r <= kVectorRuns< T > )
  powerSortRuns( A , less , E.data() , r );
 else if( ! zeros() )
  vectorQuickSort( A );
 else if constexpr( std::is_floating_point_v< T > )   // -0 and +0 equal
  radixSortBy( A , []( T x ) {
   return( radixKey( x == T( 0 ) ? T( 0 ) : x ) ); } );
 }

}  // end( namespace detail )

template< class T , class C >
void sort( std::vector< T > & A , C less )
{
 if constexpr( detail::naturalOrder< T , C > && VectorKey< T > &&
               kVectorSort )
  detail::vectorNumbers( A , less , false );
 else if constexpr( detail::naturalOrder< T , C > && RadixKey< T > ) {
  std::array< std::size_t , kRuns + 1 > E;
  std::size_t r = 0;
  if( A.size() < kRadix )
   pdqSort( A , less );
  else if( ( r = detail::runs( A , less , kRuns , E.data() ) ) <= 1 )
   detail::oneRun( A , less );
  else if( r <= kRuns )
   detail::powerSortRuns( A , less , E.data() , r );
  else if( detail::fewWideKeys( A ) )
   pdqSort( A , less );
  else
   radixSort( A );
  }
 else if constexpr( detail::naturalOrder< T , C > &&
                    std::is_same_v< T , std::string > ) {
  std::array< std::size_t , kRuns + 1 > E;
  std::size_t r = 0;
  if( A.size() < kStrings )
   pdqSort( A , less );
  else if( ( r = detail::runs( A , less , kRuns , E.data() ) ) <= 1 )
   detail::oneRun( A , less );
  else if( r <= kRuns )
   detail::powerSortRuns( A , less , E.data() , r );
  else
   msdRadixSort( A );
  }
 else if constexpr( std::is_trivially_copyable_v< T > &&
                    std::is_trivially_default_constructible_v< T > ) {
  std::array< std::size_t , kRuns + 1 > E;
  std::size_t r = 0;
  if( A.size() < kRadix )
   pdqSort( A , less );
  else if( ( r = detail::runs( A , less , kRuns , E.data() ) ) <= 1 )
   detail::oneRun( A , less );
  else if( r <= kRuns )
   detail::powerSortRuns( A , less , E.data() , r );
  else
   sampleSort( A , less );                  // its buckets of equal keys
  }
 else if( A.size() < kRadix )
  pdqSort( A , less );
 else
  sampleSort( A , less );
 }

template< class T , class C >
void stableSort( std::vector< T > & A , C less )
{
 if constexpr( detail::naturalOrder< T , C > && VectorKey< T > &&
               kVectorSort )
  detail::vectorNumbers( A , less , true );
 else if constexpr( detail::naturalOrder< T , C > && RadixKey< T > ) {
  std::array< std::size_t , kRuns + 1 > E;
  const std::size_t r = A.size() < kRadix ? 0 : detail::runs( A , less ,
                                                               kRuns ,
                                                               E.data() );
  if( r == 0 )
   powerSort( A , less );
  else if( r == 1 )
   detail::oneRun( A , less );
  else if( r <= kRuns )
   detail::powerSortRuns( A , less , E.data() , r );
  else if constexpr( std::is_floating_point_v< T > )   // -0 and +0 equal
   radixSortBy( A , []( T x ) {
    return( radixKey( x == T( 0 ) ? T( 0 ) : x ) ); } );
  else
   radixSort( A );
  }
 else if constexpr( detail::naturalOrder< T , C > &&
                    std::is_same_v< T , std::string > ) {
  std::array< std::size_t , kRuns + 1 > E;
  const std::size_t r = A.size() < kStrings ? 0 : detail::runs( A , less ,
                                                                 kRuns ,
                                                                 E.data() );
  if( r == 0 )
   powerSort( A , less );
  else if( r == 1 )
   detail::oneRun( A , less );
  else if( r <= kRuns )
   detail::powerSortRuns( A , less , E.data() , r );
  else
   msdRadixSort( A );
  }
 else if constexpr( std::is_trivially_copyable_v< T > &&
                    std::is_trivially_default_constructible_v< T > ) {
  std::array< std::size_t , kRuns + 1 > E;
  const std::size_t r = detail::runs( A , less , kRuns , E.data() );
  if( r <= 1 )
   detail::oneRun( A , less );
  else if( r <= kRuns )
   detail::powerSortRuns( A , less , E.data() , r );
  else if( ( A.size() < kRadix ) || ! detail::fewKeysSort( A , less ) )
   detail::pingPongSort( A , less );
  }
 else
  powerSort( A , less );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sort.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
