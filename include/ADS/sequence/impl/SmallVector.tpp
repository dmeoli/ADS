/*--------------------------------------------------------------------------*/
/*-------------------------- File SmallVector.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SmallVector [see SmallVector.h], with their contracts.
 * The hot ones are inline, so that the compiler expands them in the loop
 * of the caller.
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

template< class T , std::size_t N >
SmallVector< T , N >::SmallVector( std::size_t n , const T & x )
 : SmallVector()
{
 resize( n , x );
 }

template< class T , std::size_t N >
SmallVector< T , N >::SmallVector( std::initializer_list< T > items )
 : SmallVector( items.begin() , items.end() )
{
 }

template< class T , std::size_t N >
template< std::input_iterator I , std::sentinel_for< I > S >
SmallVector< T , N >::SmallVector( I first , S last ) : SmallVector()
{
 // if a push throws, the vector made by the delegation is destroyed
 if constexpr( std::forward_iterator< I > )
  reserve( std::size_t( std::ranges::distance( first , last ) ) );
 for( ; first != last ; ++first )
  emplace_back( *first );
 }

template< class T , std::size_t N >
SmallVector< T , N >::SmallVector( const SmallVector & other )
 : SmallVector( other.begin() , other.end() )
{
 }

template< class T , std::size_t N >
void SmallVector< T , N >::take( SmallVector & o )
{
 if( ! o.small() ) {                        // its memory
  f_A = std::exchange( o.f_A , o.inner() );
  f_cap = std::exchange( o.f_cap , N );
  f_n = std::exchange( o.f_n , 0 );
  }
 else {                                     // its items, one by one
  std::uninitialized_move( o.f_A , o.f_A + o.f_n , f_A );
  f_n = o.f_n;
  o.clear();
  }
 }

template< class T , std::size_t N >
SmallVector< T , N >::SmallVector( SmallVector && other )
 noexcept( std::is_nothrow_move_constructible_v< T > ) : SmallVector()
{
 take( other );
 }

template< class T , std::size_t N >
SmallVector< T , N > &
SmallVector< T , N >::operator=( const SmallVector & other )
{
 if( this != &other ) {
  SmallVector t( other );
  *this = std::move( t );
  }
 return( *this );
 }

template< class T , std::size_t N >
SmallVector< T , N > & SmallVector< T , N >::operator=( SmallVector && other )
 noexcept( std::is_nothrow_move_constructible_v< T > )
{
 if( this != &other ) {
  this->~SmallVector();
  ::new( static_cast< void * >( this ) ) SmallVector();
  take( other );
  }
 return( *this );
 }

template< class T , std::size_t N >
SmallVector< T , N >::~SmallVector()
{
 std::destroy( f_A , f_A + f_n );
 if( ! small() )
  std::allocator< T >().deallocate( f_A , f_cap );
 }

template< class T , std::size_t N >
void SmallVector< T , N >::swap( SmallVector & other )
{
 SmallVector t( std::move( other ) );
 other = std::move( *this );
 *this = std::move( t );
 }

template< class T , std::size_t N >
void SmallVector< T , N >::grow( std::size_t c )
{
 ADS_PRE( c > f_cap );
 T * B = std::allocator< T >().allocate( c );
 try {
  if constexpr( std::is_nothrow_move_constructible_v< T > ||
                ! std::is_copy_constructible_v< T > )
   std::uninitialized_move( f_A , f_A + f_n , B );
  else
   std::uninitialized_copy( f_A , f_A + f_n , B );
  }
 catch( ... ) {
  std::allocator< T >().deallocate( B , c );
  throw;
  }
 std::destroy( f_A , f_A + f_n );
 if( ! small() )
  std::allocator< T >().deallocate( f_A , f_cap );
 f_A = B;
 f_cap = c;
 }

template< class T , std::size_t N >
template< class... Args >
inline T & SmallVector< T , N >::emplace_back( Args &&... args )
{
 if( f_n == f_cap ) {                       // the item first, since the
  T x( std::forward< Args >( args )... );   // arguments may live here
  grow( 2 * f_cap );
  ::new( static_cast< void * >( f_A + f_n ) ) T( std::move( x ) );
  }
 else
  ::new( static_cast< void * >( f_A + f_n ) )
   T( std::forward< Args >( args )... );
 ++f_n;
 ADS_INVARIANT( ( f_n <= f_cap ) && ( f_cap >= N ) );
 return( f_A[ f_n - 1 ] );
 }

template< class T , std::size_t N >
inline void SmallVector< T , N >::pop_back( void )
{
 ADS_PRE( f_n > 0 );
 std::destroy_at( f_A + --f_n );
 }

template< class T , std::size_t N >
void SmallVector< T , N >::reserve( std::size_t n )
{
 if( n > f_cap )
  grow( n );
 }

template< class T , std::size_t N >
void SmallVector< T , N >::resize( std::size_t n , const T & x )
{
 if( n < f_n ) {
  std::destroy( f_A + n , f_A + f_n );
  f_n = n;
  return;
  }
 if( n > f_cap ) {
  const T y = x;                            // x may be an item
  grow( std::max( n , 2 * f_cap ) );
  std::uninitialized_fill( f_A + f_n , f_A + n , y );
  }
 else
  std::uninitialized_fill( f_A + f_n , f_A + n , x );
 f_n = n;
 }

template< class T , std::size_t N >
void SmallVector< T , N >::clear( void )
{
 std::destroy( f_A , f_A + f_n );
 f_n = 0;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SmallVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
