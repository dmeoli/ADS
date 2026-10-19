/*--------------------------------------------------------------------------*/
/*--------------------- File EngineeredQuickSorts.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of EngineeredQuickSorts.h, with their contracts.
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

/// the order is the natural one of T
template< class T , class C >
inline constexpr bool naturalOrder =
 std::is_same_v< C , std::less<> > || std::is_same_v< C , std::less< T > >;

/// heapsort of A[ lo .. hi )
template< class T , class C >
void heapRange( std::vector< T > & A , std::size_t lo , std::size_t hi ,
                C & less )
{
 std::make_heap( A.begin() + lo , A.begin() + hi , less );
 std::sort_heap( A.begin() + lo , A.begin() + hi , less );
 }

/// A[ a ] <= A[ b ] <= A[ c ]
template< class T , class C >
void sort3( std::vector< T > & A , std::size_t a , std::size_t b ,
            std::size_t c , C & less )
{
 if( less( A[ b ] , A[ a ] ) ) std::swap( A[ a ] , A[ b ] );
 if( less( A[ c ] , A[ b ] ) ) {
  std::swap( A[ b ] , A[ c ] );
  if( less( A[ b ] , A[ a ] ) ) std::swap( A[ a ] , A[ b ] );
  }
 }

/// ---------------------------------------------------------------- intro

template< class T , class C >
void intro( std::vector< T > & A , std::size_t lo , std::size_t hi ,
            int depth , C & less )
{
 while( hi - lo > 16 ) {
  if( depth-- == 0 ) {                      // too deep: heapsort
   heapRange( A , lo , hi , less );
   return;
   }
  const std::size_t mid = lo + ( hi - lo ) / 2;
  sort3( A , lo , mid , hi - 1 , less );    // A[ hi - 1 ] >= the median
  std::swap( A[ lo ] , A[ mid ] );          // the median in A[ lo ]
  std::size_t i = lo , j = hi;
  for( ; ; ) {                              // Hoare, with sentinels
   do ++i; while( less( A[ i ] , A[ lo ] ) );
   do --j; while( less( A[ lo ] , A[ j ] ) );
   if( i >= j )
    break;
   std::swap( A[ i ] , A[ j ] );
   }
  std::swap( A[ lo ] , A[ j ] );            // the pivot in its place j
  if( j - lo < hi - j - 1 ) {
   intro( A , lo , j , depth , less );
   lo = j + 1;
   }
  else {
   intro( A , j + 1 , hi , depth , less );
   hi = j;
   }
  }
 insertionSort( A , lo , hi , less );
 }

/// ---------------------------------------------------------------- pdq

/// insertion sort of A[ lo .. hi ) that gives up after 8 moves; true if
/// it sorted the part
template< class T , class C >
bool partialInsertion( std::vector< T > & A , std::size_t lo ,
                       std::size_t hi , C & less )
{
 T * const first = A.data() + lo , * const last = A.data() + hi;
 std::size_t moves = 0;
 for( T * i = first + 1 ; i < last ; ++i ) {
  if( ! less( *i , *( i - 1 ) ) )
   continue;
  T x = std::move( *i );
  T * j = i;
  do {
   *j = std::move( *( j - 1 ) );
   --j;
   } while( ( j > first ) && less( x , *( j - 1 ) ) );
  *j = std::move( x );
  moves += std::size_t( i - j );
  if( moves > 8 )
   return( false );
  }
 return( true );
 }

/// the partition of A[ lo .. hi ) around the pivot A[ lo ], the equal
/// keys on the right; its place, and whether nothing had to move
template< class T , class C >
std::pair< std::size_t , bool > partitionRight( std::vector< T > & A ,
                                                std::size_t lo ,
                                                std::size_t hi , C & less )
{
 T * const a = A.data();                   // pointers, not indices: no
 T pivot = std::move( a[ lo ] );            // scaling in the scans
 T * i = a + lo , * j = a + hi;            // the pivot is a median: an
 do ++i; while( less( *i , pivot ) );       // item >= it stops i, and one
 if( i - 1 == a + lo )                      // < it passed by i stops j
  do --j; while( ( j > i ) && ! less( *j , pivot ) );
 else
  do --j; while( ! less( *j , pivot ) );
 const bool already = i >= j;
 while( i < j ) {                           // *i >= pivot > *j
  std::swap( *i , *j );
  do ++i; while( less( *i , pivot ) );
  do --j; while( ! less( *j , pivot ) );
  }
 T * const p = i - 1;
 if( p != a + lo )
  a[ lo ] = std::move( *p );
 *p = std::move( pivot );
 return( std::pair( std::size_t( p - a ) , already ) );
 }

/// partitionRight() by blocks (BlockQuicksort): the items of a block of
/// 64 on the left that belong to the right, and those of a block on the
/// right that belong to the left, are first collected as offsets, with an
/// increment and no branch per comparison, and then swapped in pairs, so
/// that the comparisons cause no mispredicted branch; for the arithmetic
/// types with their natural order, whose comparison is a single instruction
template< class T , class C >
std::pair< std::size_t , bool > partitionRightBlocks( std::vector< T > & A ,
                                                      std::size_t lo ,
                                                      std::size_t hi ,
                                                      C & less )
{
 T * const a = A.data();
 constexpr std::ptrdiff_t B = 64;
 const T pivot = a[ lo ];
 T * pl = a + lo , * pr = a + hi;          // as in partitionRight()
 do ++pl; while( less( *pl , pivot ) );
 if( pl - 1 == a + lo )
  do --pr; while( ( pr > pl ) && ! less( *pr , pivot ) );
 else
  do --pr; while( ! less( *pr , pivot ) );
 std::ptrdiff_t l = pl - a , r = pr - a;
 const bool already = l >= r;
 if( ! already ) {                          // a[ l ] >= pivot > a[ r ]
  std::swap( a[ l++ ] , a[ r-- ] );         // [ l , r ] unknown
  unsigned char offL[ B ] , offR[ B ];
  std::ptrdiff_t numL = 0 , numR = 0 , startL = 0 , startR = 0;
  auto swaps = [ & ]( void ) {             // a cycle: 2 num + 1 moves
   const std::ptrdiff_t num = std::min( numL , numR );
   if( num > 0 ) {                          // through two pointers, each
    T * const bL = a + l , * const bR = a + r;   // place found once
    const unsigned char * const oL = offL + startL , * const oR =
     offR + startR;
    T * pL = bL + oL[ 0 ] , * pR = bR - oR[ 0 ];
    T t = std::move( *pL );
    *pL = std::move( *pR );
    for( std::ptrdiff_t k = 1 ; k < num ; ++k ) {
     pL = bL + oL[ k ];
     *pR = std::move( *pL );
     pR = bR - oR[ k ];
     *pL = std::move( *pR );
     }
    *pR = std::move( t );
    }
   numL -= num;
   numR -= num;
   startL += num;
   startR += num;
   };
  while( r - l + 1 > 2 * B ) {
   if( numL == 0 ) {                        // the misplaced on the left,
    startL = 0;                             // 8 at a time, which the
    const T * const x = a + l;              // compiler unrolls: no test
    for( std::ptrdiff_t i = 0 ; i < B ; i += 8 )   // per item
     for( std::ptrdiff_t u = i ; u < i + 8 ; ++u ) {
      offL[ numL ] = static_cast< unsigned char >( u );
      numL += ! less( x[ u ] , pivot );
      }
    }
   if( numR == 0 ) {                        // the misplaced on the right
    startR = 0;
    const T * const y = a + r;
    for( std::ptrdiff_t i = 0 ; i < B ; i += 8 )
     for( std::ptrdiff_t u = i ; u < i + 8 ; ++u ) {
      offR[ numR ] = static_cast< unsigned char >( u );
      numR += less( y[ -u ] , pivot );
      }
    }
   swaps();
   if( numL == 0 ) l += B;
   if( numR == 0 ) r -= B;
   }
  const std::ptrdiff_t unknown = r - l + 1; // at most 2 B: the last blocks
  std::ptrdiff_t ls , rs;
  if( ( numL == 0 ) && ( numR == 0 ) ) {
   ls = unknown / 2;
   rs = unknown - ls;
   }
  else if( numL == 0 ) {
   ls = unknown - B;
   rs = B;
   }
  else {
   ls = B;
   rs = unknown - B;
   }
  if( numL == 0 ) {
   startL = 0;
   for( std::ptrdiff_t i = 0 ; i < ls ; ++i ) {
    offL[ numL ] = static_cast< unsigned char >( i );
    numL += ! less( a[ l + i ] , pivot );
    }
   }
  if( numR == 0 ) {
   startR = 0;
   for( std::ptrdiff_t i = 0 ; i < rs ; ++i ) {
    offR[ numR ] = static_cast< unsigned char >( i );
    numR += less( a[ r - i ] , pivot );
    }
   }
  swaps();
  if( numL == 0 ) l += ls;
  if( numR == 0 ) r -= rs;
  if( numL > 0 ) {                          // the misplaced left to the
   while( numL > 0 ) {                      // end of the block, the last
    --numL;                                 // offset first
    std::swap( a[ l + offL[ startL + numL ] ] , a[ r-- ] );
    }
   l = r + 1;
   }
  else
   while( numR > 0 ) {                      // the misplaced right to the
    --numR;                                 // start of the block
    std::swap( a[ r - offR[ startR + numR ] ] , a[ l++ ] );
    }
  }
 const std::size_t p = std::size_t( l - 1 );   // a[ l ] is the first >= pivot
 a[ lo ] = std::move( a[ p ] );
 a[ p ] = pivot;
 return( std::pair( p , already ) );
 }

/// the partition of A[ lo .. hi ) around the pivot A[ lo ], the equal
/// keys on the left; its place
template< class T , class C >
std::size_t partitionLeft( std::vector< T > & A , std::size_t lo ,
                           std::size_t hi , C & less )
{
 T * const a = A.data();
 T pivot = std::move( a[ lo ] );
 T * i = a + lo , * j = a + hi;            // the pivot is a median: an
 do --j; while( less( pivot , *j ) );       // item <= it stops j, and one
 if( j + 1 == a + hi )                      // > it passed by j stops i
  do ++i; while( ( i < j ) && ! less( pivot , *i ) );
 else
  do ++i; while( ! less( pivot , *i ) );
 while( i < j ) {                           // *i > pivot >= *j
  std::swap( *i , *j );
  do --j; while( less( pivot , *j ) );
  do ++i; while( ! less( pivot , *i ) );
  }
 if( j != a + lo )
  a[ lo ] = std::move( *j );
 *j = std::move( pivot );
 return( std::size_t( j - a ) );
 }

template< class T , class C >
void pdq( std::vector< T > & A , std::size_t lo , std::size_t hi ,
          int bad , bool leftmost , C & less )
{
 T * const a = A.data();
 for( ; ; ) {
  const std::size_t n = hi - lo;
  if( n < 24 ) {
   if( leftmost )
    insertionSort( A , lo , hi , less );
   else                                     // A[ lo - 1 ] is a sentinel
    for( T * i = a + lo + 1 ; i < a + hi ; ++i ) {
     T x = std::move( *i );
     T * j = i;
     for( ; less( x , *( j - 1 ) ) ; --j )
      *j = std::move( *( j - 1 ) );
     *j = std::move( x );
     }
   return;
   }
  const std::size_t s = n / 2;              // the pivot to a[ lo ]
  if( n > 128 ) {                           // the ninther of Tukey
   sort3( A , lo , lo + s , hi - 1 , less );
   sort3( A , lo + 1 , lo + s - 1 , hi - 2 , less );
   sort3( A , lo + 2 , lo + s + 1 , hi - 3 , less );
   sort3( A , lo + s - 1 , lo + s , lo + s + 1 , less );
   std::swap( a[ lo ] , a[ lo + s ] );
   }
  else
   sort3( A , lo + s , lo , hi - 1 , less );
  if( ! leftmost && ! less( a[ lo - 1 ] , a[ lo ] ) ) {
   lo = partitionLeft( A , lo , hi , less ) + 1;   // a run of equals
   continue;
   }
  std::pair< std::size_t , bool > part;
  if constexpr( std::is_arithmetic_v< T > && naturalOrder< T , C > )
   part = partitionRightBlocks( A , lo , hi , less );
  else
   part = partitionRight( A , lo , hi , less );
  const auto [ p , already ] = part;
  const std::size_t l = p - lo , r = hi - p - 1;
  if( ( l < n / 8 ) || ( r < n / 8 ) ) {    // too unbalanced
   if( --bad == 0 ) {
    heapRange( A , lo , hi , less );
    return;
    }
   if( l >= 24 ) {                          // break the pattern
    std::swap( a[ lo ] , a[ lo + l / 4 ] );
    std::swap( a[ p - 1 ] , a[ p - l / 4 ] );
    if( l > 128 ) {                         // and the ninther's samples
     std::swap( a[ lo + 1 ] , a[ lo + l / 4 + 1 ] );
     std::swap( a[ lo + 2 ] , a[ lo + l / 4 + 2 ] );
     std::swap( a[ p - 2 ] , a[ p - l / 4 - 1 ] );
     std::swap( a[ p - 3 ] , a[ p - l / 4 - 2 ] );
     }
    }
   if( r >= 24 ) {
    std::swap( a[ p + 1 ] , a[ p + 1 + r / 4 ] );
    std::swap( a[ hi - 1 ] , a[ hi - r / 4 ] );
    if( r > 128 ) {
     std::swap( a[ p + 2 ] , a[ p + 2 + r / 4 ] );
     std::swap( a[ p + 3 ] , a[ p + 3 + r / 4 ] );
     std::swap( a[ hi - 2 ] , a[ hi - 1 - r / 4 ] );
     std::swap( a[ hi - 3 ] , a[ hi - 2 - r / 4 ] );
     }
    }
   }
  else if( already && partialInsertion( A , lo , p , less ) &&
           partialInsertion( A , p + 1 , hi , less ) )
   return;                                  // a run, already sorted
  pdq( A , lo , p , bad , leftmost , less );
  lo = p + 1;
  leftmost = false;
  }
 }

/// ---------------------------------------------------------------- dual

template< class T , class C >
void dual( std::vector< T > & A , std::size_t lo , std::size_t hi ,
           C & less )
{
 while( hi - lo > 27 ) {                    // A[ lo .. hi )
  const std::size_t e = ( hi - lo ) / 6;    // five spaced items, sorted
  const std::size_t c3 = lo + ( hi - lo ) / 2 , c2 = c3 - e , c1 = c2 - e ,
                    c4 = c3 + e , c5 = c4 + e;
  sort3( A , c1 , c2 , c3 , less );
  sort3( A , c3 , c4 , c5 , less );
  sort3( A , c1 , c2 , c3 , less );
  sort3( A , c2 , c4 , c5 , less );
  if( less( A[ c4 ] , A[ c2 ] ) ) std::swap( A[ c2 ] , A[ c4 ] );
  std::swap( A[ lo ] , A[ c2 ] );           // the pivots p <= q
  std::swap( A[ hi - 1 ] , A[ c4 ] );
  std::size_t l = lo + 1 , g = hi - 2 , k = l;
  while( k <= g ) {
   if( less( A[ k ] , A[ lo ] ) )           // < p
    std::swap( A[ k ] , A[ l++ ] );
   else if( ! less( A[ k ] , A[ hi - 1 ] ) ) {   // >= q
    while( less( A[ hi - 1 ] , A[ g ] ) && ( k < g ) )
     --g;
    std::swap( A[ k ] , A[ g-- ] );
    if( less( A[ k ] , A[ lo ] ) )
     std::swap( A[ k ] , A[ l++ ] );
    }
   ++k;
   }
  --l;
  ++g;
  std::swap( A[ lo ] , A[ l ] );            // the pivots in their places
  std::swap( A[ hi - 1 ] , A[ g ] );
  std::pair< std::size_t , std::size_t > P[ 3 ] = { { lo , l } ,
                                                  { l + 1 , g } ,
                                                  { g + 1 , hi } };
  if( ! less( A[ l ] , A[ g ] ) )           // p = q: the middle is done
   P[ 1 ] = { l + 1 , l + 1 };
  std::sort( P , P + 3 , []( const auto & a , const auto & b ) {
   return( a.second - a.first < b.second - b.first ); } );
  dual( A , P[ 0 ].first , P[ 0 ].second , less );   // the two smaller
  dual( A , P[ 1 ].first , P[ 1 ].second , less );
  lo = P[ 2 ].first;                        // the largest, iterated
  hi = P[ 2 ].second;
  }
 insertionSort( A , lo , hi , less );
 }

/// ---------------------------------------------------------------- 3-way

template< class T , class C >
void threeWay( std::vector< T > & A , std::size_t lo , std::size_t hi ,
               C & less )
{
 auto eq = [ & ]( const T & a , const T & b ) {
  return( ! less( a , b ) && ! less( b , a ) ); };
 while( hi - lo > 10 ) {                    // A[ lo .. hi )
  const std::size_t n = hi - lo , m = lo + n / 2;
  if( n > 40 ) {                            // the ninther
   const std::size_t e = n / 8;
   sort3( A , lo , lo + e , lo + 2 * e , less );
   sort3( A , m - e , m , m + e , less );
   sort3( A , hi - 1 - 2 * e , hi - 1 - e , hi - 1 , less );
   sort3( A , lo + e , m , hi - 1 - e , less );
   }
  else
   sort3( A , lo , m , hi - 1 , less );
  std::swap( A[ lo ] , A[ m ] );
  std::size_t i = lo , j = hi , p = lo , q = hi;   // = [ lo , p ] , ( q , hi )
  for( ; ; ) {
   while( less( A[ ++i ] , A[ lo ] ) )
    if( i == hi - 1 ) break;
   while( less( A[ lo ] , A[ --j ] ) )
    if( j == lo ) break;
   if( ( i == j ) && eq( A[ i ] , A[ lo ] ) )
    std::swap( A[ ++p ] , A[ i ] );
   if( i >= j )
    break;
   std::swap( A[ i ] , A[ j ] );
   if( eq( A[ i ] , A[ lo ] ) ) std::swap( A[ ++p ] , A[ i ] );
   if( eq( A[ j ] , A[ lo ] ) ) std::swap( A[ --q ] , A[ j ] );
   }
  i = j + 1;                                // the equals to the middle
  for( std::size_t k = lo ; k <= p ; ++k )
   std::swap( A[ k ] , A[ j-- ] );
  for( std::size_t k = hi - 1 ; k >= q ; --k )
   std::swap( A[ k ] , A[ i++ ] );
  const std::size_t lhi = j + 1 , rlo = i;  // A[ lo .. lhi ) , [ rlo , hi )
  if( lhi - lo < hi - rlo ) {
   threeWay( A , lo , lhi , less );
   lo = rlo;
   }
  else {
   threeWay( A , rlo , hi , less );
   hi = lhi;
   }
  }
 insertionSort( A , lo , hi , less );
 }

}  // end( namespace detail )

template< class T , class C >
void introSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 if( A.size() > 1 )
  detail::intro( A , 0 , A.size() , 2 * int( std::bit_width( A.size() ) - 1 ) ,
                 less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void pdqSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 if( A.size() > 1 )
  detail::pdq( A , 0 , A.size() , int( std::bit_width( A.size() ) ) , true ,
               less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void dualPivotQuickSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 detail::dual( A , 0 , A.size() , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void threeWayQuickSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 detail::threeWay( A , 0 , A.size() , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File EngineeredQuickSorts.tpp --------------------*/
/*--------------------------------------------------------------------------*/
