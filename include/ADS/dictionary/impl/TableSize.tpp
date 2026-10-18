/*--------------------------------------------------------------------------*/
/*--------------------------- File TableSize.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of TableSize.h, with their contracts.
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

inline bool is_prime( std::size_t n )
{
 if( n < 2 )
  return( false );
 for( std::size_t d = 2 ; d * d <= n ; ++d )
  if( n % d == 0 )
   return( false );
 return( true );
 }

inline std::size_t next_prime( std::size_t n )
{
 while( ! is_prime( n ) )
  ++n;
 ADS_POST( is_prime( n ) );
 return( n );
 }

inline bool is_pow2( std::size_t n )
{
 return( n && ! ( n & ( n - 1 ) ) );
 }

inline std::size_t resized( std::size_t m , bool grow , std::size_t m0 )
{
 ADS_PRE( ( m0 > 0 ) && ( m0 <= m ) );
 std::size_t r;
 if( is_pow2( m ) )
  r = grow ? 2 * m : std::max( m / 2 , m0 );
 else
  r = grow ? next_prime( 2 * m ) : std::max( next_prime( m / 2 ) , m0 );
 ADS_POST( ( r >= m0 ) && ( ! is_pow2( m ) || ( r == m0 ) || is_pow2( r ) ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File TableSize.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
