/*--------------------------------------------------------------------------*/
/*---------------------------- File XorList.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of XorList [see XorList.h], with their contracts.
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
XorList< T >::XorList( std::initializer_list< T > l )
{
 for( const T & x : l )
  push_back( x );
 }

template< class T >
XorList< T >::XorList( const XorList & o )
{
 for( const T & x : o )
  push_back( x );
 }

template< class T >
XorList< T >::XorList( XorList && o ) noexcept
    : f_head( std::exchange( o.f_head , nullptr ) ) ,
      f_tail( std::exchange( o.f_tail , nullptr ) ) ,
      f_n( std::exchange( o.f_n , 0 ) ) , f_pool( std::move( o.f_pool ) )
{ }

template< class T >
XorList< T > & XorList< T >::operator=( XorList o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class T >
XorList< T >::~XorList()
{
 if constexpr( ! std::is_trivially_destructible_v< T > )
  clear();
 }

template< class T >
void XorList< T >::swap( XorList & o ) noexcept
{
 std::swap( f_head , o.f_head );
 std::swap( f_tail , o.f_tail );
 std::swap( f_n , o.f_n );
 f_pool.swap( o.f_pool );
 }

template< class T >
const T & XorList< T >::front( void ) const
{
 ADS_PRE( f_n > 0 );
 return( f_head->value );
 }

template< class T >
const T & XorList< T >::back( void ) const
{
 ADS_PRE( f_n > 0 );
 return( f_tail->value );
 }

template< class T >
void XorList< T >::push_front( const T & x )
{
 Cell * c = f_pool.make( Cell{ word( f_head ) , x } );
 if( f_head )
  f_head->link ^= word( c );
 else
  f_tail = c;
 f_head = c;
 ++f_n;
 ADS_INVARIANT( walks() );
 }

template< class T >
void XorList< T >::push_back( const T & x )
{
 Cell * c = f_pool.make( Cell{ word( f_tail ) , x } );
 if( f_tail )
  f_tail->link ^= word( c );
 else
  f_head = c;
 f_tail = c;
 ++f_n;
 ADS_INVARIANT( walks() );
 }

template< class T >
void XorList< T >::pop_front( void )
{
 ADS_PRE( f_n > 0 );
 Cell * c = f_head;
 f_head = other( c , nullptr );  // its only neighbour
 if( f_head )
  f_head->link ^= word( c );
 else
  f_tail = nullptr;
 f_pool.free( c );
 --f_n;
 ADS_INVARIANT( walks() );
 }

template< class T >
void XorList< T >::pop_back( void )
{
 ADS_PRE( f_n > 0 );
 Cell * c = f_tail;
 f_tail = other( c , nullptr );
 if( f_tail )
  f_tail->link ^= word( c );
 else
  f_head = nullptr;
 f_pool.free( c );
 --f_n;
 ADS_INVARIANT( walks() );
 }

template< class T >
void XorList< T >::clear( void )
{
 while( f_n > 0 )
  pop_front();
 }

template< class T >
bool XorList< T >::operator==( const XorList & o ) const
{
 if( f_n != o.f_n )
  return( false );
 auto a = begin();
 for( auto b = o.begin() ; b != o.end() ; ++a , ++b )
  if( ! ( *a == *b ) )
   return( false );
 return( true );
 }

template< class T >
bool XorList< T >::walks( void ) const
{
 const Cell *p = nullptr , *c = f_head;
 std::size_t k = 0;
 for( ; c ; ++k ) {
  const Cell * n = other( c , p );
  p = c;
  c = n;
  }
 return( ( k == f_n ) && ( p == f_tail ) );
 }

template< std::unsigned_integral U >
constexpr void xorSwap( U & a , U & b )
{
 a ^= b;
 b ^= a;
 a ^= b;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File XorList.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
