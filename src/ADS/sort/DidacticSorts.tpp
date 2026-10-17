/*--------------------------------------------------------------------------*/
/*------------------------- File DidacticSorts.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of DidacticSorts.h, with their contracts.
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

template< class T , class C >
void stooge( std::vector< T > & A , std::size_t i , std::size_t j , C & less )
{
 if( less( A[ j ] , A[ i ] ) )
  std::swap( A[ i ] , A[ j ] );
 if( j - i + 1 >= 3 ) {
  const std::size_t t = ( j - i + 1 ) / 3;
  stooge( A , i , j - t , less );           // the first two thirds
  stooge( A , i + t , j , less );           // the last two thirds
  stooge( A , i , j - t , less );           // the first two thirds again
  }
 }

template< class T , class C >
void slow( std::vector< T > & A , std::size_t i , std::size_t j , C & less )
{
 if( i >= j )
  return;
 const std::size_t m = i + ( j - i ) / 2;
 slow( A , i , m , less );
 slow( A , m + 1 , j , less );
 if( less( A[ j ] , A[ m ] ) )              // the maximum to the end
  std::swap( A[ m ] , A[ j ] );
 slow( A , i , j - 1 , less );
 }

}  // end( namespace detail )

template< class T , class C >
void stoogeSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 if( A.size() > 1 )
  detail::stooge( A , 0 , A.size() - 1 , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void slowSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 if( A.size() > 1 )
  detail::slow( A , 0 , A.size() - 1 , less );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class R , class C >
long bogoSort( std::vector< T > & A , R & g , C less )
{
 ADS_PRE( A.size() <= 10 );
 const auto before = detail::auditCopy( A );
 long shuffles = 0;
 while( ! std::is_sorted( A.begin() , A.end() , less ) ) {
  std::shuffle( A.begin() , A.end() , g );
  ++shuffles;
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 return( shuffles );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File DidacticSorts.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
