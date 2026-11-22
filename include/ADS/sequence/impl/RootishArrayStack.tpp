/*--------------------------------------------------------------------------*/
/*----------------------- File RootishArrayStack.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RootishArrayStack [see RootishArrayStack.h], with their
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

template< class T , class A >
RootishArrayStack< T , A >::RootishArrayStack( std::initializer_list< T > l )
{
 for( const T & x : l )
  emplace_back( x );
 }

template< class T , class A >
RootishArrayStack< T , A >::RootishArrayStack( const RootishArrayStack & o )
    : f_blocks( detail::emptyCopy( o.f_blocks ) )
{
 for( const T & x : o )
  emplace_back( x );
 }

template< class T , class A >
RootishArrayStack< T , A >::RootishArrayStack( const RootishArrayStack & o ,
					      const A & alloc )
    : f_blocks( alloc )
{
 for( const T & x : o )
  emplace_back( x );
 }

template< class T , class A >
RootishArrayStack< T ,
		   A >::RootishArrayStack( RootishArrayStack && o ) noexcept
    : f_blocks( std::move( o.f_blocks ) ) , f_n( std::exchange( o.f_n , 0 ) )
{
 o.f_blocks.clear();
 }

template< class T , class A >
RootishArrayStack< T , A >::RootishArrayStack( RootishArrayStack && o ,
					      const A & alloc )
    : f_blocks( alloc )
{
 if( f_blocks.get_allocator() == o.f_blocks.get_allocator() )
  swap( o );
 else {
  for( T & x : o )
   emplace_back( std::move( x ) );
  o.clear();
  }
 }

template< class T , class A >
RootishArrayStack< T , A > &
RootishArrayStack< T , A >::operator=( RootishArrayStack o ) noexcept
 requires detail::alwaysEqualAlloc< A >
{
 swap( o );
 return( *this );
 }

template< class T , class A >
RootishArrayStack< T , A >::~RootishArrayStack()
{
 clear();
 }

template< class T , class A >
void RootishArrayStack< T , A >::swap( RootishArrayStack & o ) noexcept(
  detail::swapNoexcept< A > )
{
 detail::swapCheck( f_blocks.get_allocator() , o.f_blocks.get_allocator() );
 f_blocks.swap( o.f_blocks );
 std::swap( f_n , o.f_n );
 }

template< class T , class A >
std::size_t RootishArrayStack< T , A >::block( std::size_t i )
{
 // (1), then the least b with ( b + 1 ) ( b + 2 ) / 2 > i by a correction
 auto b = std::size_t(
   std::ceil( ( -3.0 + std::sqrt( 9.0 + 8.0 * double( i ) ) ) / 2.0 ) );
 while( ( b > 0 ) && ( b * ( b + 1 ) / 2 > i ) )
  --b;
 while( ( b + 1 ) * ( b + 2 ) / 2 <= i )
  ++b;
 return( b );
 }

template< class T , class A >
T * RootishArrayStack< T , A >::place( std::size_t i ) const
{
 const std::size_t b = block( i );
 return( f_blocks[ b ] + ( i - b * ( b + 1 ) / 2 ) );
 }

template< class T , class A >
T & RootishArrayStack< T , A >::operator[]( std::size_t i )
{
 ADS_PRE( i < f_n );
 return( *place( i ) );
 }

template< class T , class A >
const T & RootishArrayStack< T , A >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 return( *place( i ) );
 }

template< class T , class A >
const T & RootishArrayStack< T , A >::front( void ) const
{
 ADS_PRE( f_n > 0 );
 return( *f_blocks[ 0 ] );
 }

template< class T , class A >
const T & RootishArrayStack< T , A >::back( void ) const
{
 ADS_PRE( f_n > 0 );
 return( *place( f_n - 1 ) );
 }

template< class T , class A >
void RootishArrayStack< T , A >::grow( void )
{
 f_blocks.reserve( f_blocks.size() + 1 );  // so that push_back cannot throw
 Alloc a( f_blocks.get_allocator() );
 f_blocks.push_back( Traits::allocate( a , f_blocks.size() + 1 ) );
 }

template< class T , class A >
void RootishArrayStack< T , A >::shrink( void )
{
 for( std::size_t r = f_blocks.size() ;
      ( r > 0 ) && ( ( r < 2 ? 0 : ( r - 2 ) * ( r - 1 ) / 2 ) >= f_n ) ;
      --r ) {
  Alloc a( f_blocks.get_allocator() );
  Traits::deallocate( a , f_blocks.back() , r );
  f_blocks.pop_back();
  }
 }

template< class T , class A >
template< class... Args >
T & RootishArrayStack< T , A >::emplace_back( Args &&... args )
{
 if( capacity() == f_n ) {
  T x( std::forward< Args >( args )... );  // which may be an item
  grow();
  T * p = ::new( static_cast< void * >( place( f_n ) ) ) T( std::move( x ) );
  ++f_n;
  return( *p );
  }
 T * p = ::new( static_cast< void * >( place( f_n ) ) )
   T( std::forward< Args >( args )... );
 ++f_n;
 ADS_INVARIANT( fits() );
 return( *p );
 }

template< class T , class A >
void RootishArrayStack< T , A >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_n );
 if( i == f_n ) {
  emplace_back( x );
  return;
  }
 [[maybe_unused]] const std::size_t n = f_n;
 T y = x;  // x may be an item
 emplace_back( std::move( ( *this )[ f_n - 1 ] ) );
 for( std::size_t k = f_n - 2 ; k > i ; --k )
  *place( k ) = std::move( *place( k - 1 ) );
 *place( i ) = std::move( y );
 ADS_POST( f_n == n + 1 );
 ADS_INVARIANT( fits() );
 }

template< class T , class A >
void RootishArrayStack< T , A >::erase( std::size_t i )
{
 ADS_PRE( i < f_n );
 [[maybe_unused]] const std::size_t n = f_n;
 for( std::size_t k = i ; k + 1 < f_n ; ++k )
  *place( k ) = std::move( *place( k + 1 ) );
 place( f_n - 1 )->~T();
 --f_n;
 shrink();
 ADS_POST( f_n == n - 1 );
 ADS_INVARIANT( fits() );
 }

template< class T , class A >
void RootishArrayStack< T , A >::clear( void )
{
 if constexpr( ! std::is_trivially_destructible_v< T > )
  for( std::size_t i = 0 ; i < f_n ; ++i )
   place( i )->~T();
 Alloc a( f_blocks.get_allocator() );
 for( std::size_t b = 0 ; b < f_blocks.size() ; ++b )
  Traits::deallocate( a , f_blocks[ b ] , b + 1 );
 f_blocks.clear();
 f_n = 0;
 }

template< class T , class A >
bool RootishArrayStack< T , A >::operator==(
  const RootishArrayStack & o ) const
{
 if( f_n != o.f_n )
  return( false );
 for( std::size_t i = 0 ; i < f_n ; ++i )
  if( ! ( *place( i ) == *o.place( i ) ) )
   return( false );
 return( true );
 }

template< class T , class A >
bool RootishArrayStack< T , A >::fits( void ) const
{
 const std::size_t r = f_blocks.size();
 if( f_n == 0 )
  return( r == 0 );
 return( ( f_n <= r * ( r + 1 ) / 2 ) &&
	  ( ( r < 2 ) || ( ( r - 2 ) * ( r - 1 ) / 2 < f_n ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RootishArrayStack.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
