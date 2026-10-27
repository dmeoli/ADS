/*--------------------------------------------------------------------------*/
/*------------------------ File MonotoneSearch.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of MonotoneSearch.h [see
 * MonotoneSearch.h], with their contracts.
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

template< std::integral I , class P >
I exponentialSearch( P && p , I lo )
{
 using U = std::make_unsigned_t< I >;
 constexpr I top = std::numeric_limits< I >::max();
 I a = lo , b = lo;                         // p false on [ lo , a )
 U step = 1;
 while( ! p( b ) ) {
  ADS_PRE( b != top );                      // p true somewhere
  a = b + 1;
  if( U( U( top ) - U( a ) ) < step - 1 )
   b = top;
  else
   b = I( U( a ) + ( step - 1 ) );
  if( step <= std::numeric_limits< U >::max() / 2 )
   step *= 2;
  }
 while( a < b ) {                           // p( b ), p false before a
  const I m = I( U( a ) + U( U( b ) - U( a ) ) / 2 );
  if( p( m ) )
   b = m;
  else
   a = m + 1;
  }
 ADS_POST( detail::contract( [ & ]( void ) {
  return( p( a ) && ( ( a == lo ) || ! p( I( a - 1 ) ) ) ); } ) );
 return( a );
 }

template< class G , class P >
std::size_t unboundedSearch( G && get , P && pred )
{
 return( exponentialSearch( [ & ]( std::size_t i ) {
  const auto x = get( i );
  return( ! x || pred( *x ) ); } , std::size_t( 0 ) ) );
 }

template< std::unsigned_integral U >
U integerSqrt( U n )
{
 U lo = 0;                                  // lo^2 <= n
 U hi = U( U( 1 ) << ( std::numeric_limits< U >::digits / 2 ) );
 while( hi - lo > 1 ) {                     // hi^2 > n
  const U m = lo + U( hi - lo ) / 2;
  if( m <= n / m )
   lo = m;
  else
   hi = m;
  }
 ADS_POST( ( ( lo == 0 ) || ( lo <= n / lo ) ) &&
           ( U( lo + 1 ) > n / U( lo + 1 ) ) );
 return( lo );
 }

template< std::unsigned_integral U >
bool isPerfectSquare( U n )
{
 const U r = integerSqrt( n );
 return( U( r * r ) == n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File MonotoneSearch.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
