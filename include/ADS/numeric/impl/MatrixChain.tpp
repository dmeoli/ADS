/*--------------------------------------------------------------------------*/
/*-------------------------- File MatrixChain.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of MatrixChain.h [see MatrixChain.h].
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

inline std::pair< long , std::vector< std::vector< std::size_t > > >
matrixChain( const std::vector< long > & c )
{
 ADS_PRE( ! c.empty() );
 ADS_AUDIT( std::all_of( c.begin() , c.end() ,
                       []( long x ) { return( x > 0 ); } ) );
 const std::size_t n = c.size() - 1;
 std::vector< std::vector< long > > DP( n , std::vector< long >( n , 0 ) );
 std::vector< std::vector< std::size_t > > S( n ,
                                             std::vector< std::size_t >( n ) );
 for( std::size_t h = 2 ; h <= n ; ++h )
  for( std::size_t i = 0 ; i + h <= n ; ++i ) {
   const std::size_t j = i + h - 1;
   DP[ i ][ j ] = std::numeric_limits< long >::max();
   for( std::size_t k = i ; k < j ; ++k ) {
    const long t = DP[ i ][ k ] + DP[ k + 1 ][ j ] +
                   c[ i ] * c[ k + 1 ] * c[ j + 1 ];
    if( t < DP[ i ][ j ] ) {
     DP[ i ][ j ] = t;
     S[ i ][ j ] = k;
     }
    }
   }
 return( std::pair( n ? DP[ 0 ][ n - 1 ] : 0 , S ) );
 }

inline std::string parenthesization(
                         const std::vector< std::vector< std::size_t > > & S ,
                         std::size_t i , std::size_t j )
{
 ADS_PRE( ( i <= j ) && ( j < S.size() ) );
 if( i == j )
  return( "A" + std::to_string( i ) );
 return( "(" + parenthesization( S , i , S[ i ][ j ] ) + "·" +
         parenthesization( S , S[ i ][ j ] + 1 , j ) + ")" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File MatrixChain.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
