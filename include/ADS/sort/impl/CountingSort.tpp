/*--------------------------------------------------------------------------*/
/*------------------------- File CountingSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of CountingSort.h [see CountingSort.h].
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

template< class T >
void countingSort( std::vector< T > & A , std::size_t k )
{
 const auto before = detail::auditCopy( A );
 std::vector< std::size_t > B( k , 0 );          // the occurrences
 for( const T & a : A ) {
  ADS_PRE( !( a < 0 ) && ( std::size_t( a ) < k ) );
  ++B[ a ];
  }
 std::size_t j = 0;
 for( std::size_t i = 0 ; i < k ; ++i )
  for( std::size_t c = B[ i ] ; c > 0 ; --c )
   A[ j++ ] = T( i );
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File CountingSort.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
