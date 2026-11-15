/*--------------------------------------------------------------------------*/
/*--------------------- File EngineeredQuickSorts.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of EngineeredQuickSorts.h, with their contracts.
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

namespace detail
{

/// A[ a ] <= A[ b ] <= A[ c ]
template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C >
void sort3( Store && A , std::size_t a , std::size_t b , std::size_t c ,
	    C & less )
{
 if( less( A[ b ] , A[ a ] ) )
  std::swap( A[ a ] , A[ b ] );
 if( less( A[ c ] , A[ b ] ) ) {
  std::swap( A[ b ] , A[ c ] );
  if( less( A[ b ] , A[ a ] ) )
   std::swap( A[ a ] , A[ b ] );
  }
 }

/// ---------------------------------------------------------------- dual

template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C >
void dual( Store && A , std::size_t lo , std::size_t hi , C & less )
{
 while( hi - lo > 27 ) {                  // A[ lo .. hi )
  const std::size_t e = ( hi - lo ) / 6;  // five spaced items, sorted
  const std::size_t c3 = lo + ( hi - lo ) / 2 , c2 = c3 - e , c1 = c2 - e ,
		    c4 = c3 + e , c5 = c4 + e;
  sort3( A , c1 , c2 , c3 , less );
  sort3( A , c3 , c4 , c5 , less );
  sort3( A , c1 , c2 , c3 , less );
  sort3( A , c2 , c4 , c5 , less );
  if( less( A[ c4 ] , A[ c2 ] ) )
   std::swap( A[ c2 ] , A[ c4 ] );
  std::swap( A[ lo ] , A[ c2 ] );  // the pivots p <= q
  std::swap( A[ hi - 1 ] , A[ c4 ] );
  std::size_t l = lo + 1 , g = hi - 2 , k = l;
  while( k <= g ) {
   if( less( A[ k ] , A[ lo ] ) )  // < p
    std::swap( A[ k ] , A[ l++ ] );
   else if( ! less( A[ k ] , A[ hi - 1 ] ) ) {  // >= q
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
  std::swap( A[ lo ] , A[ l ] );  // the pivots in their places
  std::swap( A[ hi - 1 ] , A[ g ] );
  std::pair< std::size_t , std::size_t > P[ 3 ] = { { lo , l } ,
						   { l + 1 , g } ,
						   { g + 1 , hi } };
  if( ! less( A[ l ] , A[ g ] ) )  // p = q: the middle is done
   P[ 1 ] = { l + 1 , l + 1 };
  std::sort( P , P + 3 , []( const auto & a , const auto & b ) {
   return( a.second - a.first < b.second - b.first );
   } );
  dual( A , P[ 0 ].first , P[ 0 ].second , less );  // the two smaller
  dual( A , P[ 1 ].first , P[ 1 ].second , less );
  lo = P[ 2 ].first;  // the largest, iterated
  hi = P[ 2 ].second;
  }
 insertionSort( A , lo , hi , less );
 }

/// ---------------------------------------------------------------- 3-way

/// the three-way partition of A[ lo .. hi ) around the pivot P = A[ lo ]
/// (Algorithm 5.4 of Ferragina's book): c scans rightward, with
/// A[ lo .. l ) < P, A[ l .. r ) = P and A[ r .. c ) > P; an item equal to
/// P is swapped with A[ r ], a smaller one with A[ l ] and then with
/// A[ r ]; the bounds [ l , r ) of the items equal to P
template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C >
std::pair< std::size_t , std::size_t >
partitionThreeWay( Store && A , std::size_t lo , std::size_t hi , C & less )
{
 const T P = A[ lo ];
 std::size_t l = lo , r = lo + 1;
 for( std::size_t c = r ; c < hi ; ++c )
  if( less( A[ c ] , P ) ) {  // smaller: to the left part
   std::swap( A[ c ] , A[ l ] );
   std::swap( A[ c ] , A[ r ] );
   ++r;
   ++l;
   }
  else if( ! less( P , A[ c ] ) ) {  // equal: to the middle part
   std::swap( A[ c ] , A[ r ] );
   ++r;
   }
 return( std::pair( l , r ) );
 }

/// the quicksort with bounded recursive depth of the book (Algorithm 5.6)
/// on the partition above, the pivot drawn at random by a splitmix64 of
/// fixed seed, so that the sort is the same on every run and platform
template< SortStore Store ,
	  class T = std::ranges::range_value_t< std::remove_cvref_t< Store > > ,
	  class C >
void threeWay( Store && A , std::size_t lo , std::size_t hi , C & less ,
	       std::uint64_t & seed )
{
 while( hi - lo > 16 ) {  // A[ lo .. hi )
  std::uint64_t z = ( seed += 0x9E3779B97F4A7C15ULL );
  z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
  z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
  z ^= z >> 31;
  std::swap( A[ lo ] , A[ lo + std::size_t( z % ( hi - lo ) ) ] );
  const auto [ l , r ] = partitionThreeWay( A , lo , hi , less );
  if( l - lo <= hi - r ) {            // the smaller part recursively,
   threeWay( A , lo , l , less , seed );  // the larger by the loop
   lo = r;
   }
  else {
   threeWay( A , r , hi , less , seed );
   hi = l;
   }
  }
 insertionSort( A , lo , hi , less );
 }

}  // end( namespace detail )

template< SortStore Store , class T , class C >
void dualPivotQuickSort( Store && A , C less )
{
 const auto before = detail::auditCopy( A );
 detail::dual( A , 0 , A.size() , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< SortStore Store , class T , class C >
void threeWayQuickSort( Store && A , C less )
{
 const auto before = detail::auditCopy( A );
 std::uint64_t seed = 0;
 detail::threeWay( A , 0 , A.size() , less , seed );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

// the vectors of the book, by a call of the sort on their storage

template< class T , class C >
ADS_SORT_FORWARD void dualPivotQuickSort( std::vector< T > & A , C less )
{
 return( dualPivotQuickSort( std::move( A ) , less ) );
 }

template< class T , class C >
ADS_SORT_FORWARD void threeWayQuickSort( std::vector< T > & A , C less )
{
 return( threeWayQuickSort( std::move( A ) , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File EngineeredQuickSorts.tpp --------------------*/
/*--------------------------------------------------------------------------*/
