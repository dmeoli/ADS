/*--------------------------------------------------------------------------*/
/*--------------------------- File Heapsort.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Heapsort.h [see Heapsort.h].
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

template< RankStore Store , class T , class C >
void maxHeapRestore( Store && A , std::size_t i , std::size_t dim , C less )
{
 ADS_PRE( ( i < dim ) && ( dim <= A.size() ) );
 for( ; ; ) {
  std::size_t max = i;
  const std::size_t l = 2 * i + 1 , r = 2 * i + 2;
  if( ( l < dim ) && less( A[ max ] , A[ l ] ) )
   max = l;
  if( ( r < dim ) && less( A[ max ] , A[ r ] ) )
   max = r;
  if( max == i )
   return;
  std::swap( A[ i ] , A[ max ] );
  i = max;
  }
 }

template< RankStore Store , class T , class C >
void heapBuild( Store && A , C less )
{
 for( std::size_t i = A.size() / 2 ; i-- > 0 ; )
  maxHeapRestore( A , i , A.size() , less );
 ADS_AUDIT( detail::contract(
   [ & ]( void ) { return( isMaxHeap( A , A.size() , less ) ); } ) );
 }

template< RankStore Store , class T , class C >
void heapsort( Store && A , C less )
{
 const auto before = detail::auditCopy( A );
 heapBuild( A , less );
 for( std::size_t i = A.size() ; i-- > 1 ; ) {
  std::swap( A[ 0 ] , A[ i ] );
  maxHeapRestore( A , 0 , i , less );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< RankStore Store , class T , class C >
bool isMaxHeap( Store && A , std::size_t dim , C less )
{
 ADS_PRE( dim <= A.size() );
 for( std::size_t i = 1 ; i < dim ; ++i )
  if( less( A[ ( i - 1 ) / 2 ] , A[ i ] ) )
   return( false );
 return( true );
 }

// the vectors of the book, by a call of the sort on their storage

template< class T , class C >
ADS_SORT_FORWARD void maxHeapRestore( std::vector< T > & A , std::size_t i ,
				      std::size_t dim , C less )
{
 return( maxHeapRestore( std::move( A ) , i , dim , less ) );
 }

template< class T , class C >
ADS_SORT_FORWARD void heapBuild( std::vector< T > & A , C less )
{
 return( heapBuild( std::move( A ) , less ) );
 }

template< class T , class C >
ADS_SORT_FORWARD void heapsort( std::vector< T > & A , C less )
{
 return( heapsort( std::move( A ) , less ) );
 }

template< class T , class C >
ADS_SORT_FORWARD bool isMaxHeap( const std::vector< T > & A , std::size_t dim ,
				 C less )
{
 return( isMaxHeap( std::move( A ) , dim , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Heapsort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
