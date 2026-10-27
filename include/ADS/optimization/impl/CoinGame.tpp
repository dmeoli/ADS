/*--------------------------------------------------------------------------*/
/*--------------------------- File CoinGame.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of CoinGame [see CoinGame.h], with their contracts.
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
T coinsInALine( const std::vector< T > & V )
{
 const std::size_t n = V.size();
 std::vector< T > D( n + 1 , T( 0 ) );          // D[ i ]: V[ i .. i + h )
 for( std::size_t h = 1 ; h <= n ; ++h )
  for( std::size_t i = 0 ; i + h <= n ; ++i )
   D[ i ] = std::max( V[ i ] - D[ i + 1 ] , V[ i + h - 1 ] - D[ i ] );
 T sum = T( 0 );
 for( const T & v : V )
  sum += v;
 const T first = ( sum + D[ 0 ] ) / T( 2 );
 ADS_POST( first + first == sum + D[ 0 ] );
 return( first );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File CoinGame.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
