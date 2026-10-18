/*--------------------------------------------------------------------------*/
/*------------------------- File StockTrading.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of StockTrading [see StockTrading.h], with their
 * contracts.
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

/// free[ l ] after all the days: the best balance of exactly l closed
/// transactions, nothing if not reachable, for l = 0 .. k
template< class T >
std::vector< std::optional< T > > closedTransactions(
                            const std::vector< T > & prices , std::size_t k )
{
 std::vector< std::optional< T > > free( k + 1 ) , hold( k + 1 );
 free[ 0 ] = T( 0 );
 for( const T & x : prices )
  for( std::size_t l = k ; l >= 1 ; --l ) {
   if( hold[ l ] && ( ( ! free[ l ] ) || ( *free[ l ] < *hold[ l ] + x ) ) )
    free[ l ] = *hold[ l ] + x;
   if( free[ l - 1 ] &&
       ( ( ! hold[ l ] ) || ( *hold[ l ] < *free[ l - 1 ] - x ) ) )
    hold[ l ] = *free[ l - 1 ] - x;
   }
 return( free );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T >
T maxProfit( const std::vector< T > & prices , std::size_t k )
{
 const std::size_t n = prices.size();
 T best = T( 0 );
 if( k >= n / 2 ) {                             // as many as wanted
  for( std::size_t i = 1 ; i < n ; ++i )
   if( prices[ i - 1 ] < prices[ i ] )
    best += prices[ i ] - prices[ i - 1 ];
  }
 else
  for( const auto & f : detail::closedTransactions( prices , k ) )
   if( f && ( best < *f ) )
    best = *f;
 ADS_POST( ! ( best < T( 0 ) ) );
 return( best );
 }

/*--------------------------------------------------------------------------*/

template< class T >
std::optional< T > maxProfitExactly( const std::vector< T > & prices ,
                                     std::size_t k )
{
 if( prices.size() / 2 < k )
  return( std::nullopt );
 return( detail::closedTransactions( prices , k )[ k ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File StockTrading.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
