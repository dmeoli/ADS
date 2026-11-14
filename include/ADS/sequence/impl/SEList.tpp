/*--------------------------------------------------------------------------*/
/*---------------------------- File SEList.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SEList [see SEList.h], with their contracts.
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
SEList< T >::SEList( std::size_t b )
 : f_dummy{ &f_dummy , &f_dummy , 0 , 0 } , f_b( b ) ,
   f_cap( b + 1 )
{
 ADS_PRE( b >= 3 );
 }

template< class T >
SEList< T >::SEList( std::initializer_list< T > l ) : SEList()
{
 for( const T & x : l ) push_back( x );
 }

template< class T >
SEList< T >::SEList( const SEList & o ) : SEList( o.f_b )
{
 for( const T & x : o ) push_back( x );
 }

template< class T >
SEList< T >::SEList( SEList && o ) noexcept : SEList( o.f_b )
{
 swap( o );
 }

template< class T >
SEList< T > & SEList< T >::operator=( SEList o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class T >
SEList< T >::~SEList()
{
 clear();
 }

template< class T >
void SEList< T >::swap( SEList & o ) noexcept
{
 // the blocks relinked to the other dummy
 auto relinkTo = []( Node & d , Node * first , Node * last ) {
  if( first == nullptr ) { d.next = d.prev = &d; return; }
  d.next = first; d.prev = last;
  first->prev = &d; last->next = &d;
  };
 Node * f1 = f_n ? f_dummy.next : nullptr , * l1 = f_dummy.prev;
 Node * f2 = o.f_n ? o.f_dummy.next : nullptr , * l2 = o.f_dummy.prev;
 relinkTo( f_dummy , f2 , l2 );
 relinkTo( o.f_dummy , f1 , l1 );
 std::swap( f_spare , o.f_spare );
 std::swap( f_n , o.f_n );
 std::swap( f_blocks , o.f_blocks );
 std::swap( f_b , o.f_b );
 std::swap( f_cap , o.f_cap );
 }

template< class T >
typename SEList< T >::Node * SEList< T >::newBlock( Node * p )
{
 Node * u = std::exchange( f_spare , nullptr );
 if( ! u )
  u = static_cast< Node * >( ::operator new( OFF + f_cap *
                                             sizeof( T ) ,
                                             std::align_val_t( ALIGN ) ) );
 u->head = u->n = 0;
 u->prev = p->prev;
 u->next = p;
 p->prev->next = u;
 p->prev = u;
 ++f_blocks;
 return( u );
 }

template< class T >
void SEList< T >::dropBlock( Node * u )
{
 u->prev->next = u->next;
 u->next->prev = u->prev;
 --f_blocks;
 if( ! f_spare )
  f_spare = u;
 else
  ::operator delete( u , std::align_val_t( ALIGN ) );
 }

template< class T >
void SEList< T >::pushFront( Node * u , T && x )
{
 const std::size_t h = u->head == 0 ? f_cap - 1 : u->head - 1;
 ::new( static_cast< void * >( places( u ) + h ) ) T( std::move( x ) );
 u->head = h;
 ++u->n;
 }

template< class T >
void SEList< T >::pushBack( Node * u , T && x )
{
 ::new( static_cast< void * >( &at( u , u->n ) ) ) T( std::move( x ) );
 ++u->n;
 }

template< class T >
T SEList< T >::popFront( Node * u )
{
 T & r = at( u , 0 );
 T x = std::move( r );
 r.~T();
 u->head = wrap( u->head + 1 );
 --u->n;
 return( x );
 }

template< class T >
T SEList< T >::popBack( Node * u )
{
 T & r = at( u , u->n - 1 );
 T x = std::move( r );
 r.~T();
 --u->n;
 return( x );
 }

template< class T >
void SEList< T >::insert( Node * u , std::size_t j , T && x )
{
 const std::size_t n = u->n;
 if( j == 0 ) { pushFront( u , std::move( x ) ); return; }
 if( j == n ) { pushBack( u , std::move( x ) ); return; }
 if( j < n / 2 ) {                          // the front j items back by one
  pushFront( u , std::move( at( u , 0 ) ) );
  for( std::size_t k = 1 ; k < j ; ++k )
   at( u , k ) = std::move( at( u , k + 1 ) );
  }
 else {                                     // the back n - j items on by one
  pushBack( u , std::move( at( u , n - 1 ) ) );
  for( std::size_t k = n - 1 ; k > j ; --k )
   at( u , k ) = std::move( at( u , k - 1 ) );
  }
 at( u , j ) = std::move( x );
 }

template< class T >
void SEList< T >::erase( Node * u , std::size_t j )
{
 const std::size_t n = u->n;
 if( j < n / 2 ) {
  for( std::size_t k = j ; k > 0 ; --k )
   at( u , k ) = std::move( at( u , k - 1 ) );
  at( u , 0 ).~T();
  u->head = wrap( u->head + 1 );
  }
 else {
  for( std::size_t k = j ; k + 1 < n ; ++k )
   at( u , k ) = std::move( at( u , k + 1 ) );
  at( u , n - 1 ).~T();
  }
 --u->n;
 }

template< class T >
typename SEList< T >::Node *
SEList< T >::locate( std::size_t i , std::size_t & j ) const
{
 //! [locate]
 Node * u;
 if( i < f_n / 2 ) {                        // from the front
  for( u = f_dummy.next ; i >= u->n ; u = u->next )
   i -= u->n;
  j = i;
  }
 else {                                     // from the back
  std::size_t k = f_n;
  for( u = &f_dummy ; k > i ; ) {
   u = u->prev;
   k -= u->n;
   }
  j = i - k;
  }
 //! [locate]
 return( u );
 }

template< class T >
T & SEList< T >::operator[]( std::size_t i )
{
 ADS_PRE( i < f_n );
 std::size_t j;
 Node * u = locate( i , j );
 return( at( u , j ) );
 }

template< class T >
const T & SEList< T >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::size_t j;
 Node * u = locate( i , j );
 return( at( u , j ) );
 }

template< class T >
const T & SEList< T >::front( void ) const
{
 ADS_PRE( f_n > 0 );
 return( at( f_dummy.next , 0 ) );
 }

template< class T >
const T & SEList< T >::back( void ) const
{
 ADS_PRE( f_n > 0 );
 return( at( f_dummy.prev , f_dummy.prev->n - 1 ) );
 }

template< class T >
void SEList< T >::push_back( const T & x )
{
 T y = x;                                   // x may be an item
 //! [push_back]
 Node * u = f_dummy.prev;
 if( ( u == &f_dummy ) || ( u->n == f_b + 1 ) )
  u = newBlock( &f_dummy );
 pushBack( u , std::move( y ) );
 ++f_n;
 //! [push_back]
 ADS_INVARIANT( bounded() );
 }

template< class T >
void SEList< T >::push_front( const T & x )
{
 T y = x;
 Node * u = f_dummy.next;
 if( ( u == &f_dummy ) || ( u->n == f_b + 1 ) )
  u = newBlock( u );
 pushFront( u , std::move( y ) );
 ++f_n;
 ADS_INVARIANT( bounded() );
 }

template< class T >
void SEList< T >::pop_back( void )
{
 ADS_PRE( f_n > 0 );
 //! [pop_back]
 Node * u = f_dummy.prev;
 popBack( u );
 if( u->n == 0 )
  dropBlock( u );
 --f_n;
 //! [pop_back]
 ADS_INVARIANT( bounded() );
 }

template< class T >
void SEList< T >::pop_front( void )
{
 ADS_PRE( f_n > 0 );
 Node * u = f_dummy.next;
 popFront( u );
 if( u->n == 0 )
  dropBlock( u );
 --f_n;
 ADS_INVARIANT( bounded() );
 }

template< class T >
void SEList< T >::spread( Node * u )
{
 //! [spread]
 Node * w = u;
 for( std::size_t k = 0 ; k < f_b ; ++k )
  w = w->next;
 w = newBlock( w );                         // after the b blocks from u
 for( ; w != u ; w = w->prev )
  while( w->n < f_b )
   pushFront( w , popBack( w->prev ) );
 //! [spread]
 }

template< class T >
void SEList< T >::gather( Node * u )
{
 //! [gather]
 for( std::size_t k = 1 ; k < f_b ; ++k , u = u->next )
  while( ( u->n < f_b ) && ( u->next->n > 0 ) )
   pushBack( u , popFront( u->next ) );
 dropBlock( u );                            // the b-th block, empty
 //! [gather]
 }

template< class T >
void SEList< T >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_n );
 if( i == 0 ) { push_front( x ); return; }
 if( i == f_n ) { push_back( x ); return; }
 [[maybe_unused]] const std::size_t n = f_n;
 T y = x;
 std::size_t j;
 Node * u = locate( i , j );
 if( u->n == f_b + 1 ) {
  //! [insert]
  Node * v = u;                             // the first block with room
  std::size_t r = 0;
  while( ( r < f_b ) && ( v != &f_dummy ) && ( v->n == f_b + 1 ) ) {
   v = v->next;
   ++r;
   }
  if( r == f_b ) {                          // b blocks full: spread them
   spread( u );
   u = locate( i , j );
   }
  else {
   if( v == &f_dummy )
    v = newBlock( &f_dummy );
   for( ; v != u ; v = v->prev )            // one item on from each block
    pushFront( v , popBack( v->prev ) );
   }
  //! [insert]
  }
 insert( u , j , std::move( y ) );
 ++f_n;
 ADS_POST( f_n == n + 1 );
 ADS_INVARIANT( bounded() );
 }

template< class T >
void SEList< T >::erase( std::size_t i )
{
 ADS_PRE( i < f_n );
 [[maybe_unused]] const std::size_t n = f_n;
 std::size_t j;
 Node * u = locate( i , j );
 erase( u , j );
 --f_n;
 //! [erase]
 if( u->n == 0 )                            // an end block, emptied
  dropBlock( u );
 else if( middle( u ) && ( u->n < f_b - 1 ) ) {
  Node * v = u->next;                       // the first block that can
  std::size_t r = 0;                        // give an item
  while( ( r < f_b ) && middle( v ) && ( v->n == f_b - 1 ) ) {
   v = v->next;
   ++r;
   }
  if( r == f_b )                            // b blocks at b - 1: gather
   gather( u );
  else {
   for( Node * w = u ; w != v ; w = w->next )   // one item back from each
    pushBack( w , popFront( w->next ) );
   if( v->n == 0 )                          // the last block, emptied
    dropBlock( v );
   }
  }
 //! [erase]
 ADS_POST( f_n == n - 1 );
 ADS_INVARIANT( bounded() );
 }

template< class T >
void SEList< T >::clear( void )
{
 while( f_dummy.next != &f_dummy ) {
  Node * u = f_dummy.next;
  if constexpr( ! std::is_trivially_destructible_v< T > )
   for( std::size_t j = 0 ; j < u->n ; ++j )
    at( u , j ).~T();
  u->n = 0;
  dropBlock( u );
  }
 if( f_spare )
  ::operator delete( std::exchange( f_spare , nullptr ) ,
                     std::align_val_t( ALIGN ) );
 f_n = 0;
 }

template< class T >
bool SEList< T >::operator==( const SEList & o ) const
{
 if( f_n != o.f_n )
  return( false );
 auto a = begin();
 for( auto b = o.begin() ; b != o.end() ; ++a , ++b )
  if( !( *a == *b ) )
   return( false );
 return( true );
 }

template< class T >
bool SEList< T >::bounded( void ) const
{
 std::size_t s = 0 , k = 0;
 for( const Node * u = f_dummy.next ; u != &f_dummy ; u = u->next , ++k ) {
  if( ( u->n == 0 ) || ( u->n > f_b + 1 ) ||
      ( middle( u ) && ( u->n < f_b - 1 ) ) )
   return( false );
  s += u->n;
  }
 return( ( s == f_n ) && ( k == f_blocks ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File SEList.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
