/*--------------------------------------------------------------------------*/
/*------------------------- File Combinatorics.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Combinatorics.h [see Combinatorics.h].
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

inline std::uint64_t binomial( int n , int k , long * calls )
{
 ADS_PRE( ( k >= 0 ) && ( k <= n ) && ( n <= 67 ) );
 if( calls ) ++*calls;
 if( ( k == 0 ) || ( k == n ) )
  return( 1 );
 return( binomial( n - 1 , k - 1 , calls ) + binomial( n - 1 , k , calls ) );
 }

inline std::vector< std::vector< std::uint64_t > > tartaglia( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 67 ) );
 std::vector< std::vector< std::uint64_t > > C( n + 1 );
 for( int i = 0 ; i <= n ; ++i ) {
  C[ i ].assign( i + 1 , 1 );
  for( int j = 1 ; j < i ; ++j )
   C[ i ][ j ] = C[ i - 1 ][ j - 1 ] + C[ i - 1 ][ j ];
  }
 return( C );
 }

inline std::uint64_t fibonacciTable( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 92 ) );
 std::vector< std::uint64_t > F( n < 1 ? 2 : n + 1 );
 F[ 0 ] = F[ 1 ] = 1;
 for( int i = 2 ; i <= n ; ++i )
  F[ i ] = F[ i - 1 ] + F[ i - 2 ];
 return( F[ n ] );
 }

inline std::uint64_t fibonacci( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 92 ) );
 std::uint64_t F0 = 1 , F1 = 1 , F2 = 1;
 for( int i = 2 ; i <= n ; ++i ) {
  F0 = F1;
  F1 = F2;
  F2 = F0 + F1;
  }
 return( F2 );
 }

inline std::uint64_t FibonacciCache::value( int n ) const
{
 ADS_PRE( ( n >= 0 ) && ( n <= 92 ) );
 while( int( f_F.size() ) <= n )
  f_F.push_back( f_F[ f_F.size() - 1 ] + f_F[ f_F.size() - 2 ] );
 ADS_POST( int( f_F.size() ) > n );
 return( f_F[ n ] );
 }

inline int FibonacciCache::size( void ) const
{
 return( int( f_F.size() ) );
 }

inline std::uint64_t factorial( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 20 ) );
 std::uint64_t f = 1;
 for( int i = 2 ; i <= n ; ++i )
  f *= std::uint64_t( i );
 return( f );
 }

inline std::uint64_t factorialRec( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n <= 20 ) );
 return( n <= 1 ? 1 : std::uint64_t( n ) * factorialRec( n - 1 ) );
 }

inline std::uint64_t binomialByFactorials( int n , int k )
{
 ADS_PRE( ( k >= 0 ) && ( k <= n ) && ( n <= 20 ) );
 return( factorial( n ) / ( factorial( k ) * factorial( n - k ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File Combinatorics.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
