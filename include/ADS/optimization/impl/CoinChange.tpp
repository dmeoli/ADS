/*--------------------------------------------------------------------------*/
/*-------------------------- File CoinChange.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CoinChange [see CoinChange.h], with their contracts.
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

inline std::optional< std::vector< long > > greedyChange(
                                       std::vector< long > coins , long c )
{
 ADS_PRE( c >= 0 );
 ADS_PRE( std::all_of( coins.begin() , coins.end() ,
                       []( long d ) { return( d > 0 ); } ) );
 const long c0 = c;
 std::sort( coins.rbegin() , coins.rend() );
 std::vector< long > out;
 for( long d : coins )
  for( ; ( d > 0 ) && ( c >= d ) ; c -= d )
   out.push_back( d );
 if( c != 0 )
  return( std::nullopt );
 ADS_POST( std::accumulate( out.begin() , out.end() , 0L ) == c0 );
 return( out );
 }

inline std::optional< long > optimalChange( const std::vector< long > & coins ,
                                            long c )
{
 ADS_PRE( c >= 0 );
 ADS_PRE( std::all_of( coins.begin() , coins.end() ,
                       []( long d ) { return( d > 0 ); } ) );
 const long INF = std::numeric_limits< long >::max();
 std::vector< long > N( c + 1 , INF );
 N[ 0 ] = 0;
 for( long x = 1 ; x <= c ; ++x )
  for( long d : coins )
   if( ( d > 0 ) && ( d <= x ) && ( N[ x - d ] != INF ) )
    N[ x ] = std::min( N[ x ] , N[ x - d ] + 1 );
 if( N[ c ] == INF )
  return( std::nullopt );
 ADS_POST( ( N[ c ] >= 0 ) && ( N[ c ] <= c ) );
 return( N[ c ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File CoinChange.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
