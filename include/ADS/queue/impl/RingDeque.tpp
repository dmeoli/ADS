/*--------------------------------------------------------------------------*/
/*--------------------------- File RingDeque.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RingDeque [see RingDeque.h], with their contracts.
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
RingDeque< T >::RingDeque( std::initializer_list< T > l )
{
 reserve( l.size() );
 for( const T & v : l ) emplace_back( v );
 }

template< class T >
RingDeque< T >::RingDeque( const RingDeque & o )
{
 reserve( o.size() );
 for( const T & v : o ) emplace_back( v );
 }

template< class T >
RingDeque< T >::RingDeque( RingDeque && o ) noexcept
 : f_A( std::exchange( o.f_A , nullptr ) ) ,
   f_mask( std::exchange( o.f_mask , Count( -1 ) ) ) ,
   f_head( std::exchange( o.f_head , 0 ) ) ,
   f_tail( std::exchange( o.f_tail , 0 ) )
{
 }

template< class T >
RingDeque< T > & RingDeque< T >::operator=( RingDeque o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class T >
RingDeque< T >::~RingDeque()
{
 clear();
 if( f_A )
  std::allocator< T >().deallocate( f_A , capacity() );
 }

template< class T >
void RingDeque< T >::swap( RingDeque & o ) noexcept
{
 std::swap( f_A , o.f_A );
 std::swap( f_mask , o.f_mask );
 std::swap( f_head , o.f_head );
 std::swap( f_tail , o.f_tail );
 }

template< class T >
void RingDeque< T >::reserve( std::size_t n )
{
 while( capacity() < n ) grow();
 }

template< class T >
T * RingDeque< T >::relocate( T * A , std::size_t m , std::size_t h ,
                              std::size_t n )
{
 const std::size_t M = m ? 2 * m : 8;
 T * B = std::allocator< T >().allocate( M );
 if( n ) {
  // the two pieces [ h , m ) and [ 0 , h + n - m ) to the front of B
  const std::size_t first = std::min( n , m - h );
  if constexpr( std::is_trivially_copyable_v< T > ) {
   std::memcpy( static_cast< void * >( B ) , A + h , first * sizeof( T ) );
   std::memcpy( static_cast< void * >( B + first ) , A ,
                ( n - first ) * sizeof( T ) );
   }
  else
   for( std::size_t i = 0 ; i < n ; ++i ) {
    T & x = A[ ( h + i ) & ( m - 1 ) ];
    ::new( static_cast< void * >( B + i ) ) T( std::move_if_noexcept( x ) );
    x.~T();
    }
  }
 if( A )
  std::allocator< T >().deallocate( A , m );
 return( B );
 }

template< class T >
inline void RingDeque< T >::grow( void )
{
 const std::size_t m = capacity() , n = size();
 f_A = relocate( f_A , m , m ? f_head & f_mask : 0 , n );
 f_mask = ( m ? 2 * m : 8 ) - 1;
 f_head = 0;
 f_tail = n;
 ADS_INVARIANT( fits() );
 }

template< class T >
inline T & RingDeque< T >::growBack( T && v )
{
 grow();
 T * p = ::new( static_cast< void * >( f_A + f_tail ) ) T( std::move( v ) );
 ++f_tail;
 return( *p );
 }

template< class T >
inline T & RingDeque< T >::growFront( T && v )
{
 grow();
 f_tail += f_mask + 1;                      // the front at the last place,
 f_head = f_mask;                           // the counters apart by size()
 return( *::new( static_cast< void * >( f_A + f_mask ) ) T( std::move( v ) ) );
 }

template< class T >
ADS_RING_INLINE T & RingDeque< T >::front( void )
{
 ADS_PRE( ! empty() );
 return( f_A[ f_head & f_mask ] );
 }

template< class T >
ADS_RING_INLINE const T & RingDeque< T >::front( void ) const
{
 ADS_PRE( ! empty() );
 return( f_A[ f_head & f_mask ] );
 }

template< class T >
ADS_RING_INLINE T & RingDeque< T >::back( void )
{
 ADS_PRE( ! empty() );
 return( f_A[ ( f_tail - 1 ) & f_mask ] );
 }

template< class T >
ADS_RING_INLINE const T & RingDeque< T >::back( void ) const
{
 ADS_PRE( ! empty() );
 return( f_A[ ( f_tail - 1 ) & f_mask ] );
 }

template< class T >
T & RingDeque< T >::operator[]( std::size_t i )
{
 ADS_PRE( i < size() );
 return( f_A[ ( f_head + i ) & f_mask ] );
 }

template< class T >
const T & RingDeque< T >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( f_A[ ( f_head + i ) & f_mask ] );
 }

template< class T >
template< class ... Args >
ADS_RING_INLINE T & RingDeque< T >::emplace_back( Args && ... args )
{
 if( size() == capacity() ) [[unlikely]]    // the item, which may be one
  return( growBack( T( std::forward< Args >( args )... ) ) );   // of ours
 T * p = ::new( static_cast< void * >( f_A + ( f_tail & f_mask ) ) )
          T( std::forward< Args >( args )... );
 ++f_tail;
 ADS_INVARIANT( fits() );
 return( *p );
 }

template< class T >
template< class ... Args >
ADS_RING_INLINE T & RingDeque< T >::emplace_front( Args && ... args )
{
 if( size() == capacity() ) [[unlikely]]
  return( growFront( T( std::forward< Args >( args )... ) ) );
 const Count h = f_head - 1;
 T * p = ::new( static_cast< void * >( f_A + ( h & f_mask ) ) )
          T( std::forward< Args >( args )... );
 f_head = h;
 ADS_INVARIANT( fits() );
 return( *p );
 }

template< class T >
ADS_RING_INLINE void RingDeque< T >::pop_front( void )
{
 ADS_PRE( ! empty() );
 f_A[ f_head & f_mask ].~T();
 ++f_head;
 }

template< class T >
ADS_RING_INLINE void RingDeque< T >::pop_back( void )
{
 ADS_PRE( ! empty() );
 --f_tail;
 f_A[ f_tail & f_mask ].~T();
 }

template< class T >
void RingDeque< T >::clear( void )
{
 if constexpr( ! std::is_trivially_destructible_v< T > )
  for( Count i = f_head ; i != f_tail ; ++i )
   f_A[ i & f_mask ].~T();
 f_head = f_tail = 0;
 }

template< class T >
bool RingDeque< T >::operator==( const RingDeque & o ) const
{
 if( size() != o.size() )
  return( false );
 for( std::size_t i = 0 ; i < size() ; ++i )
  if( !( ( *this )[ i ] == o[ i ] ) )
   return( false );
 return( true );
 }

template< class T >
bool RingDeque< T >::fits( void ) const
{
 return( ( f_A == nullptr ) ? ( size() == 0 ) :
         ( std::has_single_bit( capacity() ) && ( size() <= capacity() ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RingDeque.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
