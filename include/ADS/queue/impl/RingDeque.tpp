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
void RingDeque< T >::shiftDown( std::size_t lo , std::size_t hi )
{
 const std::size_t m = f_mask + 1;
 for( std::size_t k = lo ; k < hi ; ) {    // D[ k ] = D[ k + 1 ], by runs
  const std::size_t d = ( f_head + k ) & f_mask , s = ( d + 1 ) & f_mask;
  std::size_t len = hi - k;                 // contiguous in source and
  if( len > m - s ) len = m - s;            // destination
  if( len > m - d ) len = m - d;
  std::move( f_A + s , f_A + s + len , f_A + d );
  k += len;
  }
 }

template< class T >
void RingDeque< T >::shiftUp( std::size_t lo , std::size_t hi )
{
 for( std::size_t k = hi ; k > lo ; ) {    // D[ k ] = D[ k - 1 ], by runs
  const std::size_t d = ( f_head + k ) & f_mask , s = ( d - 1 ) & f_mask;
  std::size_t len = k - lo;
  if( len > s + 1 ) len = s + 1;
  if( len > d + 1 ) len = d + 1;
  std::move_backward( f_A + s + 1 - len , f_A + s + 1 , f_A + d + 1 );
  k -= len;
  }
 }

template< class T >
void RingDeque< T >::insert( std::size_t i , const T & v )
{
 ADS_PRE( i <= size() );
 const std::size_t n = size();
 T x = v;                                   // v may be an item
 if( i == 0 ) {
  emplace_front( std::move( x ) );
  return;
  }
 if( i == n ) {
  emplace_back( std::move( x ) );
  return;
  }
 if( i < n / 2 ) {                          // the front i items back by one
  emplace_front( std::move( front() ) );
  shiftDown( 1 , i );
  }
 else {                                     // the back n - i items on by one
  emplace_back( std::move( back() ) );
  shiftUp( i , n - 1 );
  }
 ( *this )[ i ] = std::move( x );
 ADS_POST( size() == n + 1 );
 ADS_INVARIANT( fits() );
 }

template< class T >
void RingDeque< T >::erase( std::size_t i )
{
 ADS_PRE( i < size() );
 const std::size_t n = size();
 if( i < n / 2 ) {
  shiftUp( 0 , i );
  pop_front();
  }
 else {
  shiftDown( i , n - 1 );
  pop_back();
  }
 ADS_POST( size() == n - 1 );
 }

template< class T >
void RingDeque< T >::rotate( std::size_t r )
{
 const std::size_t n = size();
 if( n == 0 )
  return;
 r %= n;
 if( r == 0 )
  return;
 if( n == capacity() ) {                    // full: the ranks alone move
  f_head -= r;
  f_tail -= r;
  }
 else if( r <= n - r )
  for( std::size_t k = 0 ; k < r ; ++k ) {
   T x = std::move( back() );
   pop_back();
   emplace_front( std::move( x ) );         // never grows: one place free
   }
 else
  for( std::size_t k = 0 ; k < n - r ; ++k ) {
   T x = std::move( front() );
   pop_front();
   emplace_back( std::move( x ) );
   }
 ADS_INVARIANT( fits() );
 }

template< class T >
void RingDeque< T >::shrink_to_fit( void )
{
 const std::size_t n = size();
 if( n == 0 ) {
  if( f_A )
   std::allocator< T >().deallocate( f_A , capacity() );
  f_A = nullptr;
  f_mask = Count( -1 );
  f_head = f_tail = 0;
  return;
  }
 const std::size_t M = std::max( std::bit_ceil( n ) , std::size_t( 8 ) );
 if( M >= capacity() )
  return;
 T * B = std::allocator< T >().allocate( M );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  T & x = f_A[ ( f_head + i ) & f_mask ];
  ::new( static_cast< void * >( B + i ) ) T( std::move_if_noexcept( x ) );
  x.~T();
  }
 std::allocator< T >().deallocate( f_A , capacity() );
 f_A = B;
 f_mask = M - 1;
 f_head = 0;
 f_tail = n;
 ADS_INVARIANT( fits() );
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
