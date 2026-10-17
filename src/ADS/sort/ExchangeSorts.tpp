/*--------------------------------------------------------------------------*/
/*------------------------- File ExchangeSorts.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of ExchangeSorts.h, with their contracts.
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

template< class T , class C >
void bubbleSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 for( std::size_t end = A.size() ; end > 1 ; ) {
  std::size_t last = 0;                     // the last swap of the pass
  for( std::size_t i = 1 ; i < end ; ++i )
   if( less( A[ i ] , A[ i - 1 ] ) ) {
    std::swap( A[ i ] , A[ i - 1 ] );
    last = i;
    }
  end = last;                               // A[ last .. ) in place
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void cocktailShakerSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 std::size_t lo = 0 , hi = A.size();        // A[ lo .. hi ) to sort
 while( hi - lo > 1 ) {
  std::size_t last = lo;
  for( std::size_t i = lo + 1 ; i < hi ; ++i )   // to the right
   if( less( A[ i ] , A[ i - 1 ] ) ) {
    std::swap( A[ i ] , A[ i - 1 ] );
    last = i;
    }
  hi = last;
  if( hi - lo <= 1 )
   break;
  last = hi;
  for( std::size_t i = hi - 1 ; i > lo ; --i )   // to the left
   if( less( A[ i ] , A[ i - 1 ] ) ) {
    std::swap( A[ i ] , A[ i - 1 ] );
    last = i;
    }
  lo = last;
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void oddEvenSort( std::vector< T > & A , C less , std::size_t * rounds )
{
 const auto before = detail::auditCopy( A );
 std::size_t r = 0;
 for( bool sorted = false ; ! sorted ; ++r ) {
  sorted = true;
  for( std::size_t start : { 0 , 1 } )      // the even, then the odd pairs
   for( std::size_t i = start ; i + 1 < A.size() ; i += 2 )
    if( less( A[ i + 1 ] , A[ i ] ) ) {
     std::swap( A[ i ] , A[ i + 1 ] );
     sorted = false;
     }
  }
 if( rounds ) *rounds = r;
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void combSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 std::size_t gap = A.size();
 for( bool swapped = true ; ( gap > 1 ) || swapped ; ) {
  gap = gap * 10 / 13;                      // shrink by 1.3
  if( gap < 1 )
   gap = 1;
  swapped = false;
  for( std::size_t i = 0 ; i + gap < A.size() ; ++i )
   if( less( A[ i + gap ] , A[ i ] ) ) {
    std::swap( A[ i ] , A[ i + gap ] );
    swapped = true;
    }
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void gnomeSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 for( std::size_t i = 1 ; i < A.size() ; )
  if( ( i == 0 ) || ! less( A[ i ] , A[ i - 1 ] ) )
   ++i;                                     // in order: forward
  else {
   std::swap( A[ i ] , A[ i - 1 ] );        // out of order: back
   --i;
   }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ExchangeSorts.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
