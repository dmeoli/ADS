/*--------------------------------------------------------------------------*/
/*-------------------------- File LinkedQueue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LinkedQueue [see LinkedQueue.h], with their contracts.
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

template< class T >
LinkedQueue< T >::LinkedQueue( const LinkedQueue & other )
{
 for( Cell * c = other.f_front ; c ; c = c->next )
  enqueue( c->value );
 }

template< class T >
LinkedQueue< T >::LinkedQueue( LinkedQueue && other ) noexcept
 : f_front( std::exchange( other.f_front , nullptr ) ) ,
   f_back( std::exchange( other.f_back , nullptr ) ) ,
   f_size( std::exchange( other.f_size , 0 ) ) ,
   f_pool( std::move( other.f_pool ) )
{
 }

template< class T >
LinkedQueue< T > & LinkedQueue< T >::operator=( LinkedQueue other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
LinkedQueue< T >::~LinkedQueue()
{
 while( ! empty() )
  dequeue();
 }

template< class T >
void LinkedQueue< T >::swap( LinkedQueue & other ) noexcept
{
 std::swap( f_front , other.f_front );
 std::swap( f_back , other.f_back );
 std::swap( f_size , other.f_size );
 f_pool.swap( other.f_pool );
 }

template< class T >
bool LinkedQueue< T >::empty( void ) const
{
 return( f_front == nullptr );
 }

template< class T >
void LinkedQueue< T >::enqueue( const T & v )
{
 [[maybe_unused]] const bool was_empty = empty();
 Cell * c = f_pool.make( Cell{ v , nullptr } );
 ( f_back ? f_back->next : f_front ) = c;
 f_back = c;
 ++f_size;
 ADS_POST( ! empty() && ( ! was_empty || detail::equal( top() , v ) ) );
 }

template< class T >
T LinkedQueue< T >::dequeue( void )
{
 ADS_PRE( ! empty() );
 Cell * c = f_front;
 T v = std::move( c->value );
 f_front = c->next;
 if( ! f_front )
  f_back = nullptr;
 f_pool.free( c );
 --f_size;
 return( v );
 }

template< class T >
const T & LinkedQueue< T >::top( void ) const
{
 ADS_PRE( ! empty() );
 return( f_front->value );
 }

template< class T >
std::size_t LinkedQueue< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
bool LinkedQueue< T >::operator==( const LinkedQueue & o ) const
{
 const Cell * a = f_front , * b = o.f_front;
 for( ; a && b ; a = a->next , b = b->next )
  if( !( a->value == b->value ) )
   return( false );
 return( ! a && ! b );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkedQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
