/*--------------------------------------------------------------------------*/
/*----------------------------- File Arena.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Arena [see Arena.h], with their contracts.
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

template< class T , class A >
Arena< T , A >::Arena( std::size_t n , const A & alloc )
    : f_n( n ) , f_alloc( alloc )
{
 ADS_PRE( n >= 1 );
 }

template< class T , class A >
Arena< T , A >::~Arena()
{
 const std::size_t u = used();
 for( std::size_t b = 0 ; b < f_blocks.size() ; ++b ) {
  // the blocks after the one in use hold no object: clear() destroyed
  // them, and they may still be forbidden to the sanitizer
  ADS_UNPOISON( f_blocks[ b ] , f_n * sizeof( T ) );
  if( b <= f_cur )
   std::destroy_n( f_blocks[ b ] , b < f_cur ? f_n : u );
  std::allocator_traits< A >::deallocate( f_alloc , f_blocks[ b ] , f_n );
  }
 }

template< class T , class A >
void Arena< T , A >::next( void )
{
 if( f_cur + 1 < f_blocks.size() )  // a block kept by clear()
  ++f_cur;
 else {
  T * const b = std::allocator_traits< A >::allocate( f_alloc , f_n );
  try {
   f_blocks.push_back( b );
   }
  catch( ... ) {  // the block not kept, freed
   std::allocator_traits< A >::deallocate( f_alloc , b , f_n );
   throw;
   }
  f_cur = f_blocks.size() - 1;
  }
 f_place = f_blocks[ f_cur ];
 f_end = f_place + f_n;
 }

template< class T , class A >
template< class... Args >
T * Arena< T , A >::make( Args &&... args )
{
 if( f_place == f_end )
  next();
 ADS_UNPOISON( f_place , sizeof( T ) );
 T * p = ::new( static_cast< void * >( f_place ) )
   T( std::forward< Args >( args )... );
 ++f_place;
 ++f_size;
 ADS_INVARIANT( f_size == f_cur * f_n + used() );
 return( p );
 }

template< class T , class A >
void Arena< T , A >::clear( void ) noexcept
{
 if( f_blocks.empty() )
  return;
 if constexpr( ! std::is_trivially_destructible_v< T > ) {
  const std::size_t u = used();
  for( std::size_t b = 0 ; b <= f_cur ; ++b )
   std::destroy_n( f_blocks[ b ] , b < f_cur ? f_n : u );
  }
 if constexpr( ADS_ASAN )  // the places of the epoch
  for( std::size_t b = 0 ; b <= f_cur ; ++b )
   ADS_POISON( f_blocks[ b ] , f_n * sizeof( T ) );
 f_cur = 0;
 f_place = f_blocks.front();
 f_end = f_place + f_n;
 f_size = 0;
 ADS_INVARIANT( f_size == f_cur * f_n + used() );
 }

template< class T , class A >
bool Arena< T , A >::fresh( const T * p ) const
{
 const std::less< const T * > lt;
 for( std::size_t b = 0 ; ( b <= f_cur ) && ( b < f_blocks.size() ) ; ++b ) {
  const T * x = f_blocks[ b ];
  if( ! lt( p , x ) && lt( p , x + f_n ) )
   return( ( b < f_cur ) || lt( p , f_place ) );
  }
 return( false );
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
