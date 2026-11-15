/*--------------------------------------------------------------------------*/
/*-------------------------- File RandomQueue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RandomQueue [see RandomQueue.h], with their contracts.
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

template< class T >
std::size_t RandomQueue< T >::uniform( std::size_t n )
{
 ADS_PRE( n > 0 );
 const std::uint64_t s = n;
 std::uint64_t h;
 std::uint64_t l = detail::wideProduct( f_g() , s , h );
 if( l < s ) {  // rarely: reject the biased words
  const std::uint64_t t = ( 0 - s ) % s;
  while( l < t )
   l = detail::wideProduct( f_g() , s , h );
  }
 return( std::size_t( h ) );
 }

template< class T >
T RandomQueue< T >::pop( void )
{
 ADS_PRE( ! f_A.empty() );
 [[maybe_unused]] const std::size_t n = f_A.size();
 const std::size_t j = uniform( f_A.size() );
 if( j + 1 != f_A.size() )
  std::swap( f_A[ j ] , f_A.back() );
 T x = std::move( f_A.back() );
 f_A.pop_back();
 ADS_POST( f_A.size() == n - 1 );
 return( x );
 }

template< class T >
const T & RandomQueue< T >::sample( void )
{
 ADS_PRE( ! f_A.empty() );
 return( f_A[ uniform( f_A.size() ) ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File RandomQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
