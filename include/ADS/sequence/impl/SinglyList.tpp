/*--------------------------------------------------------------------------*/
/*-------------------------- File SinglyList.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SinglyList [see SinglyList.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------- CONSTRUCTING AND COPYING -----------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
SinglyList< T >::SinglyList( std::initializer_list< T > items )
{
 Pos p = nullptr;
 for( const auto & v : items ) {
  insert( p , v );
  p = next( p );
  }
 }

template< class T >
SinglyList< T >::SinglyList( const SinglyList & other )
{
 Pos p = nullptr;
 for( Cell * c = other.f_first ; c ; c = c->next ) {
  insert( p , c->value );
  p = next( p );
  }
 }

template< class T >
SinglyList< T >::SinglyList( SinglyList && other ) noexcept
    : f_first( std::exchange( other.f_first , nullptr ) ) ,
      f_last( std::exchange( other.f_last , nullptr ) ) ,
      f_size( std::exchange( other.f_size , 0 ) ) ,
      f_pool( std::move( other.f_pool ) )
{ }

template< class T >
SinglyList< T > & SinglyList< T >::operator=( SinglyList other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
SinglyList< T >::~SinglyList()
{
 if constexpr( ! std::is_trivially_destructible_v< T > )
  while( ! empty() )
   remove( head() );
 }

template< class T >
void SinglyList< T >::swap( SinglyList & other ) noexcept
{
 std::swap( f_first , other.f_first );
 std::swap( f_last , other.f_last );
 std::swap( f_size , other.f_size );
 f_pool.swap( other.f_pool );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SEQUENCE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool SinglyList< T >::empty( void ) const
{
 return( f_first == nullptr );
 }

template< class T >
bool SinglyList< T >::finished( Pos p ) const
{
 if( p == &f_before )
  return( true );
 return( p ? ( p->next == nullptr ) : ( f_first == nullptr ) );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::head( void ) const
{
 return( nullptr );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::tail( void ) const
{
 if( ! f_first )
  return( &f_before );
 Pos p = nullptr;  // the cell before the last one
 while( ! finished( next( p ) ) )
  p = next( p );
 return( p );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::next( Pos p ) const
{
 if( p == &f_before )
  return( nullptr );
 ADS_PRE( ! finished( p ) );
 return( p ? p->next : f_first );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::prev( Pos p ) const
{
 if( ! p )
  return( &f_before );
 if( p == f_first )
  return( nullptr );
 Pos q = f_first;  // a scan for the cell before p
 while( q->next != p )
  q = q->next;
 return( q );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::insert( Pos p , const T & v )
{
 ADS_PRE( p != &f_before );
 Cell * c;
 if( ! p )
  c = f_first = f_pool.make( v , f_first );
 else
  c = p->next = f_pool.make( v , p->next );
 if( ! c->next )
  f_last = c;
 ++f_size;
 ADS_POST( detail::equal( read( p ) , v ) );
 return( p );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::remove( Pos p )
{
 ADS_PRE( ! finished( p ) );
 Cell * t = cell( p );
 ( p ? p->next : f_first ) = t->next;
 if( t == f_last )
  f_last = p;  // nil if the list is empty
 f_pool.free( t );
 --f_size;
 return( p );
 }

template< class T >
const T & SinglyList< T >::read( Pos p ) const
{
 ADS_PRE( ! finished( p ) );
 return( cell( p )->value );
 }

template< class T >
void SinglyList< T >::write( Pos p , const T & v )
{
 ADS_PRE( ! finished( p ) );
 cell( p )->value = v;
 ADS_POST( detail::equal( read( p ) , v ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
template< std::input_iterator I , std::sentinel_for< I > S >
SinglyList< T >::SinglyList( I first , S last )
{
 Pos p = head();  // the place after the last
 for( ; first != last ; ++first )
  p = next( insert( p , *first ) );
 }

template< class T >
void SinglyList< T >::push_front( const T & v )
{
 insert( head() , v );
 }

template< class T >
void SinglyList< T >::push_back( const T & v )
{
 insert( f_last , v );  // pos_n, nil when empty
 }

template< class T >
T SinglyList< T >::pop_front( void )
{
 ADS_PRE( f_first != nullptr );
 T x = std::move( f_first->value );
 remove( head() );
 return( x );
 }

template< class T >
const T & SinglyList< T >::front( void ) const
{
 ADS_PRE( f_first != nullptr );
 return( f_first->value );
 }

template< class T >
const T & SinglyList< T >::back( void ) const
{
 ADS_PRE( f_last != nullptr );
 return( f_last->value );
 }

template< class T >
typename SinglyList< T >::Cell * SinglyList< T >::walk( std::size_t i ) const
{
 Cell * u = f_first;
 for( ; i > 0 ; --i )
  u = u->next;
 return( u );
 }

template< class T >
const T & SinglyList< T >::get( std::size_t i ) const
{
 ADS_PRE( i < f_size );
 return( walk( i )->value );
 }

template< class T >
T SinglyList< T >::set( std::size_t i , const T & x )
{
 ADS_PRE( i < f_size );
 Cell * u = walk( i );
 T y = std::move( u->value );
 u->value = x;
 return( y );
 }

template< class T >
void SinglyList< T >::add( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_size );
 [[maybe_unused]] const std::size_t n = f_size;
 insert( i == 0 ? head() : walk( i - 1 ) , x );
 ADS_POST( f_size == n + 1 );
 }

template< class T >
T SinglyList< T >::erase( std::size_t i )
{
 ADS_PRE( i < f_size );
 [[maybe_unused]] const std::size_t n = f_size;
 const Pos p = i == 0 ? head() : walk( i - 1 );
 T x = std::move( cell( p )->value );
 remove( p );
 ADS_POST( f_size == n - 1 );
 return( x );
 }

template< class T >
const T & SinglyList< T >::secondLast( void ) const
{
 ADS_PRE( ( f_first != nullptr ) && ( f_first->next != nullptr ) );
 const Cell * u = f_first;
 while( u->next->next )
  u = u->next;
 return( u->value );
 }

template< class T >
void SinglyList< T >::reverse( void )
{
 const auto old = detail::snapshot( *this );
 Cell *p = nullptr , *u = f_first;
 f_last = u;
 while( u ) {
  Cell * w = u->next;
  u->next = p;
  p = u;
  u = w;
  }
 f_first = p;
 ADS_AUDIT( ( [ & ] {
  SinglyList r;
  for( const T & x : *old )
   r.push_front( x );
  return( r == *this );
  }() ) );
 }

template< class T >
void SinglyList< T >::checkSize( void ) const
{
 std::size_t c = 0;
 for( const Cell * u = f_first ; u ; u = u->next )
  ++c;
 if( c != f_size )
  throw( std::logic_error( "SinglyList::checkSize: " + std::to_string( c ) +
			   " cells, " + std::to_string( f_size ) +
			   " counted" ) );
 }

template< class T >
bool SinglyList< T >::operator==( const SinglyList & o ) const
{
 const Cell *a = f_first , *b = o.f_first;
 for( ; a && b ; a = a->next , b = b->next )
  if( ! ( a->value == b->value ) )
   return( false );
 return( ! a && ! b );
 }

template< class T >
typename SinglyList< T >::Cell * SinglyList< T >::cell( Pos p ) const
{
 return( p ? p->next : f_first );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SinglyList.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
