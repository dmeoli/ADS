/*--------------------------------------------------------------------------*/
/*----------------------- File MultikeyQuickSort.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of MultikeyQuickSort.h, with its contracts.
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

/// the character d of s, -1 at its end
inline int keyAt( const std::string & s , std::size_t d )
{
 return( d < s.size() ? int( static_cast< unsigned char >( s[ d ] ) ) : -1 );
 }

/// the sort of A[ lo .. hi ) by the characters from d on
inline void mkqs( std::vector< std::string > & A , std::size_t lo ,
                  std::size_t hi , std::size_t d )
{
 while( hi - lo > 1 ) {
  if( hi - lo <= 10 ) {                     // small: insertion from d
   for( std::size_t i = lo + 1 ; i < hi ; ++i )
    for( std::size_t j = i ; ( j > lo ) &&
         ( A[ j ].compare( d , std::string::npos , A[ j - 1 ] , d ,
                           std::string::npos ) < 0 ) ; --j )
     std::swap( A[ j ] , A[ j - 1 ] );
   return;
   }
  std::swap( A[ lo ] , A[ lo + ( hi - lo ) / 2 ] );
  const int v = keyAt( A[ lo ] , d );       // the pivot character
  std::size_t lt = lo , gt = hi , i = lo + 1;   // < [ lo , lt ) , = , > [ gt ,
  while( i < gt ) {
   const int c = keyAt( A[ i ] , d );
   if( c < v ) std::swap( A[ lt++ ] , A[ i++ ] );
   else if( c > v ) std::swap( A[ i ] , A[ --gt ] );
   else ++i;
   }
  mkqs( A , lo , lt , d );                  // smaller: the same character
  if( v >= 0 )
   mkqs( A , lt , gt , d + 1 );             // equal: the next character
  lo = gt;                                  // larger: the same, iterated
  }
 }

}  // end( namespace detail )

inline void multikeyQuickSort( std::vector< std::string > & A )
{
 const auto before = detail::auditCopy( A );
 detail::mkqs( A , 0 , A.size() , 0 );
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File MultikeyQuickSort.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
