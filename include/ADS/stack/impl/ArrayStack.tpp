/*--------------------------------------------------------------------------*/
/*-------------------------- File ArrayStack.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ArrayStack [see ArrayStack.h], with their contracts.
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
ArrayStack< T >::ArrayStack( std::size_t dim )
 : f_A( new T[ dim ] ) , f_size( 0 ) , f_capacity( dim )
{
 }

template< class T >
ArrayStack< T >::ArrayStack( const ArrayStack & other )
 : f_A( new T[ other.f_capacity ] ) , f_size( other.f_size ) ,
   f_capacity( other.f_capacity )
{
 for( std::size_t i = 0 ; i < f_size ; ++i )
  f_A[ i ] = other.f_A[ i ];
 }

template< class T >
ArrayStack< T >::ArrayStack( ArrayStack && other ) noexcept
 : f_A( std::move( other.f_A ) ) ,
   f_size( std::exchange( other.f_size , 0 ) ) ,
   f_capacity( std::exchange( other.f_capacity , 0 ) )
{
 }

template< class T >
ArrayStack< T > & ArrayStack< T >::operator=( ArrayStack other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
void ArrayStack< T >::swap( ArrayStack & other ) noexcept
{
 f_A.swap( other.f_A );
 std::swap( f_size , other.f_size );
 std::swap( f_capacity , other.f_capacity );
 }

template< class T >
bool ArrayStack< T >::empty( void ) const
{
 return( f_size == 0 );
 }

template< class T >
const T & ArrayStack< T >::top( void ) const
{
 ADS_PRE( f_size > 0 );
 return( f_A[ f_size - 1 ] );
 }

template< class T >
T ArrayStack< T >::pop( void )
{
 ADS_PRE( f_size > 0 );
 [[maybe_unused]] const std::size_t n = f_size;
 T v = std::move( f_A[ --f_size ] );
 ADS_POST( f_size == n - 1 );   // and v the old top, by construction
 return( v );
 }

template< class T >
void ArrayStack< T >::push( const T & v )
{
 ADS_PRE( f_size < f_capacity );
 [[maybe_unused]] const std::size_t n = f_size;
 f_A[ f_size++ ] = v;
 ADS_POST( ( f_size == n + 1 ) && ( f_size <= f_capacity ) &&
           detail::equal( top() , v ) );
 }

template< class T >
std::size_t ArrayStack< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
std::size_t ArrayStack< T >::capacity( void ) const
{
 return( f_capacity );
 }

template< class T >
bool ArrayStack< T >::operator==( const ArrayStack & o ) const
{
 if( f_size != o.f_size )
  return( false );
 for( std::size_t i = 0 ; i < f_size ; ++i )
  if( !( f_A[ i ] == o.f_A[ i ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ArrayStack.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
