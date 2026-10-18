/*--------------------------------------------------------------------------*/
/*----------------------------- File Arena.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Arena [see Arena.h], with their contracts.
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

template< class T , class A >
Arena< T , A >::Arena( std::size_t n , const A & alloc )
 : f_n( n ) , f_alloc( alloc )
{
 ADS_PRE( n >= 1 );
 }

template< class T , class A >
Arena< T , A >::~Arena()
{
 for( std::size_t b = 0 ; b < f_blocks.size() ; ++b ) {
  const std::size_t k = b + 1 < f_blocks.size() ? f_n : f_used;
  std::destroy_n( f_blocks[ b ] , k );
  std::allocator_traits< A >::deallocate( f_alloc , f_blocks[ b ] , f_n );
  }
 }

template< class T , class A >
template< class... Args >
T * Arena< T , A >::make( Args &&... args )
{
 if( f_blocks.empty() || ( f_used == f_n ) ) {
  f_blocks.push_back( std::allocator_traits< A >::allocate( f_alloc ,
                                                            f_n ) );
  f_used = 0;
  }
 T * p = ::new( static_cast< void * >( f_blocks.back() + f_used ) )
          T( std::forward< Args >( args )... );
 ++f_used;
 ++f_size;
 ADS_INVARIANT( f_size == ( f_blocks.size() - 1 ) * f_n + f_used );
 return( p );
 }

template< class T , class A >
std::size_t Arena< T , A >::size( void ) const
{
 return( f_size );
 }

template< class T , class A >
std::size_t Arena< T , A >::bytes( void ) const
{
 return( f_blocks.size() * f_n * sizeof( T ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Arena.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
