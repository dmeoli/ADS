/*--------------------------------------------------------------------------*/
/*-------------------------- File ArrayQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ArrayQueue [see ArrayQueue.h], with their contracts.
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
ArrayQueue< T >::ArrayQueue( std::size_t dim )
    : f_A( new T[ dim ] ) , f_size( 0 ) , f_head( 0 ) , f_capacity( dim )
{ }

template< class T >
ArrayQueue< T >::ArrayQueue( const ArrayQueue & other )
    : f_A( new T[ other.f_capacity ] ) , f_size( other.f_size ) , f_head( 0 ) ,
      f_capacity( other.f_capacity )
{
 for( std::size_t i = 0 , p = other.f_head ; i < f_size ; ++i ) {
  f_A[ i ] = other.f_A[ p ];
  if( ++p == f_capacity )
   p = 0;
  }
 }

template< class T >
ArrayQueue< T >::ArrayQueue( ArrayQueue && other ) noexcept
    : f_A( std::move( other.f_A ) ) ,
      f_size( std::exchange( other.f_size , 0 ) ) ,
      f_head( std::exchange( other.f_head , 0 ) ) ,
      f_capacity( std::exchange( other.f_capacity , 0 ) )
{ }

template< class T >
ArrayQueue< T > & ArrayQueue< T >::operator=( ArrayQueue other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
void ArrayQueue< T >::swap( ArrayQueue & other ) noexcept
{
 f_A.swap( other.f_A );
 std::swap( f_size , other.f_size );
 std::swap( f_head , other.f_head );
 std::swap( f_capacity , other.f_capacity );
 }

template< class T >
bool ArrayQueue< T >::empty( void ) const
{
 return( f_size == 0 );
 }

template< class T >
const T & ArrayQueue< T >::top( void ) const
{
 ADS_PRE( f_size > 0 );
 return( f_A[ f_head ] );
 }

template< class T >
T ArrayQueue< T >::dequeue( void )
{
 ADS_PRE( f_size > 0 );
 [[maybe_unused]] const std::size_t n = f_size;
 T t = std::move( f_A[ f_head ] );
 if( ++f_head == f_capacity )  // the place after, with no
  f_head = 0;                  // division
 --f_size;
 ADS_POST( f_size == n - 1 );
 ADS_INVARIANT( fits() );
 return( t );
 }

template< class T >
void ArrayQueue< T >::enqueue( const T & v )
{
 ADS_PRE( f_size < f_capacity );
 [[maybe_unused]] const std::size_t n = f_size;
 std::size_t t = f_head + f_size;  // the place of the back
 if( t >= f_capacity )
  t -= f_capacity;
 f_A[ t ] = v;
 ++f_size;
 ADS_POST( f_size == n + 1 );
 ADS_INVARIANT( fits() );
 }

template< class T >
std::size_t ArrayQueue< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
std::size_t ArrayQueue< T >::capacity( void ) const
{
 return( f_capacity );
 }

template< class T >
bool ArrayQueue< T >::contains( const T & v ) const
{
 for( std::size_t i = 0 , p = f_head ; i < f_size ; ++i ) {
  if( f_A[ p ] == v )
   return( true );
  if( ++p == f_capacity )
   p = 0;
  }
 return( false );
 }

template< class T >
bool ArrayQueue< T >::fits( void ) const
{
 return( ( f_size <= f_capacity ) &&
	  ( ( f_capacity == 0 ) || ( f_head < f_capacity ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ArrayQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
