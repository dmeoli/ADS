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
 std::size_t moves = 0;
 for( std::size_t i = lo + 1 ; i < hi ; ++i ) {
  if( ! less( A[ i ] , A[ i - 1 ] ) )
   continue;
  T x = std::move( A[ i ] );
  std::size_t j = i;
  do {
   A[ j ] = std::move( A[ j - 1 ] );
   --j;
   } while( ( j > lo ) && less( x , A[ j - 1 ] ) );
  A[ j ] = std::move( x );
  moves += i - j;
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
 T pivot = std::move( A[ lo ] );
 std::size_t i = lo , j = hi;
 do ++i; while( ( i < j ) && less( A[ i ] , pivot ) );
 do --j; while( ( j > i ) && ! less( A[ j ] , pivot ) );
 const bool already = i >= j;
 while( i < j ) {                           // A[ i ] >= pivot > A[ j ]
  std::swap( A[ i ] , A[ j ] );
  do ++i; while( less( A[ i ] , pivot ) );
  do --j; while( ! less( A[ j ] , pivot ) );
  }
 const std::size_t p = i - 1;
 if( p != lo )
  A[ lo ] = std::move( A[ p ] );
 A[ p ] = std::move( pivot );
 return( std::pair( p , already ) );
 }

/// the partition of A[ lo .. hi ) around the pivot A[ lo ], the equal
/// keys on the left; its place
template< class T , class C >
std::size_t partitionLeft( std::vector< T > & A , std::size_t lo ,
                           std::size_t hi , C & less )
{
 T pivot = std::move( A[ lo ] );
 std::size_t i = lo , j = hi;
 do --j; while( ( j > i ) && less( pivot , A[ j ] ) );
 do ++i; while( ( i < j ) && ! less( pivot , A[ i ] ) );
 while( i < j ) {                           // A[ i ] > pivot >= A[ j ]
  std::swap( A[ i ] , A[ j ] );
  do --j; while( less( pivot , A[ j ] ) );
  do ++i; while( ! less( pivot , A[ i ] ) );
  }
 if( j != lo )
  A[ lo ] = std::move( A[ j ] );
 A[ j ] = std::move( pivot );
 return( j );
 }

template< class T , class C >
void pdq( std::vector< T > & A , std::size_t lo , std::size_t hi ,
          int bad , bool leftmost , C & less )
{
 for( ; ; ) {
  const std::size_t n = hi - lo;
  if( n < 24 ) {
   insertionSort( A , lo , hi , less );
   return;
   }
  const std::size_t s = n / 2;              // the pivot to A[ lo ]
  if( n > 128 ) {                           // the ninther of Tukey
   sort3( A , lo , lo + s , hi - 1 , less );
   sort3( A , lo + 1 , lo + s - 1 , hi - 2 , less );
   sort3( A , lo + 2 , lo + s + 1 , hi - 3 , less );
   sort3( A , lo + s - 1 , lo + s , lo + s + 1 , less );
   std::swap( A[ lo ] , A[ lo + s ] );
   }
  else
   sort3( A , lo + s , lo , hi - 1 , less );
  if( ! leftmost && ! less( A[ lo - 1 ] , A[ lo ] ) ) {
   lo = partitionLeft( A , lo , hi , less ) + 1;   // a run of equals
   continue;
   }
  const auto [ p , already ] = partitionRight( A , lo , hi , less );
  const std::size_t l = p - lo , r = hi - p - 1;
  if( ( l < n / 8 ) || ( r < n / 8 ) ) {    // too unbalanced
   if( --bad == 0 ) {
    heapRange( A , lo , hi , less );
    return;
    }
   if( l >= 24 ) {                          // break the pattern
    std::swap( A[ lo ] , A[ lo + l / 4 ] );
    std::swap( A[ p - 1 ] , A[ p - l / 4 ] );
    }
   if( r >= 24 ) {
    std::swap( A[ p + 1 ] , A[ p + 1 + r / 4 ] );
    std::swap( A[ hi - 1 ] , A[ hi - r / 4 ] );
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
