/*--------------------------------------------------------------------------*/
/*------------------------ File ContiguousDeque.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ContiguousDeque [see ContiguousDeque.h], with their
 * contracts.
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
ContiguousDeque< T >::ContiguousDeque( std::initializer_list< T > l )
{
 for( const T & x : l )
  push_back( x );
 }

template< class T >
ContiguousDeque< T >::ContiguousDeque( const ContiguousDeque & o )
{
 for( const T & x : o )
  push_back( x );
 }

template< class T >
ContiguousDeque< T >::ContiguousDeque( ContiguousDeque && o ) noexcept
    : f_A( std::exchange( o.f_A , nullptr ) ) ,
      f_c( std::exchange( o.f_c , 0 ) ) ,
      f_h( std::exchange( o.f_h , 0 ) ) , f_n( std::exchange( o.f_n , 0 ) )
{ }

template< class T >
ContiguousDeque< T > &
ContiguousDeque< T >::operator=( ContiguousDeque o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class T >
ContiguousDeque< T >::~ContiguousDeque()
{
 clear();
 if( f_A )
  std::allocator< T >().deallocate( f_A , f_c );
 }

template< class T >
void ContiguousDeque< T >::swap( ContiguousDeque & o ) noexcept
{
 std::swap( f_A , o.f_A );
 std::swap( f_c , o.f_c );
 std::swap( f_h , o.f_h );
 std::swap( f_n , o.f_n );
 }

template< class T >
void ContiguousDeque< T >::rebuild( void )
{
 const std::size_t c = 3 * f_n + 8 , h = ( c - f_n ) / 2;
 if constexpr( std::is_trivially_copyable_v< T > )
  if( f_A && ( c <= f_c ) && ( 9 * f_n + 24 >= f_c ) ) {  // in place
   const std::size_t g = ( f_c - f_n ) / 2;
   if( f_n )
    std::memmove( static_cast< void * >( f_A + g ) , f_A + f_h ,
		  f_n * sizeof( T ) );
   f_h = g;
   ADS_INVARIANT( fits() );
   return;
   }
 T * B = std::allocator< T >().allocate( c );
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  T & x = f_A[ f_h + i ];
  ::new( static_cast< void * >( B + h + i ) ) T( std::move_if_noexcept( x ) );
  x.~T();
  }
 if( f_A )
  std::allocator< T >().deallocate( f_A , f_c );
 f_A = B;
 f_c = c;
 f_h = h;
 ADS_INVARIANT( fits() );
 }

template< class T >
void ContiguousDeque< T >::push_front( const T & x )
{
 if( f_h == 0 ) {
  T y = x;  // x may be an item
  rebuild();
  ::new( static_cast< void * >( f_A + f_h - 1 ) ) T( std::move( y ) );
  }
 else
  ::new( static_cast< void * >( f_A + f_h - 1 ) ) T( x );
 --f_h;
 ++f_n;
 }

template< class T >
void ContiguousDeque< T >::push_back( const T & x )
{
 if( f_h + f_n == f_c ) {
  T y = x;
  rebuild();
  ::new( static_cast< void * >( f_A + f_h + f_n ) ) T( std::move( y ) );
  }
 else
  ::new( static_cast< void * >( f_A + f_h + f_n ) ) T( x );
 ++f_n;
 }

template< class T >
void ContiguousDeque< T >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_n );
 if( i == 0 ) {
  push_front( x );
  return;
  }
 if( i == f_n ) {
  push_back( x );
  return;
  }
 [[maybe_unused]] const std::size_t n = f_n;
 T y = x;
 if( i < f_n / 2 ) {  // the front i items back by one
  if( f_h == 0 )
   rebuild();
  T * a = f_A + f_h;
  ::new( static_cast< void * >( a - 1 ) ) T( std::move( a[ 0 ] ) );
  std::move( a + 1 , a + i , a );
  a[ i - 1 ] = std::move( y );
  --f_h;
  }
 else {  // the back n - i items on
  if( f_h + f_n == f_c )
   rebuild();
  T * a = f_A + f_h;
  ::new( static_cast< void * >( a + f_n ) ) T( std::move( a[ f_n - 1 ] ) );
  std::move_backward( a + i , a + f_n - 1 , a + f_n );
  a[ i ] = std::move( y );
  }
 ++f_n;
 ADS_POST( f_n == n + 1 );
 ADS_INVARIANT( fits() );
 }

template< class T >
void ContiguousDeque< T >::erase( std::size_t i )
{
 ADS_PRE( i < f_n );
 [[maybe_unused]] const std::size_t n = f_n;
 T * a = f_A + f_h;
 if( i < f_n / 2 ) {
  std::move_backward( a , a + i , a + i + 1 );
  a[ 0 ].~T();
  ++f_h;
  }
 else {
  std::move( a + i + 1 , a + f_n , a + i );
  a[ f_n - 1 ].~T();
  }
 --f_n;
 if( f_c > 9 * f_n + 24 )
  rebuild();
 ADS_POST( f_n == n - 1 );
 ADS_INVARIANT( fits() );
 }

template< class T >
void ContiguousDeque< T >::clear( void )
{
 if constexpr( ! std::is_trivially_destructible_v< T > )
  for( std::size_t i = 0 ; i < f_n ; ++i )
   f_A[ f_h + i ].~T();
 f_n = 0;
 f_h = f_c / 2;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ContiguousDeque.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
