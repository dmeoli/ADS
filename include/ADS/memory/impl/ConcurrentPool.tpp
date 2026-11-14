/*--------------------------------------------------------------------------*/
/*------------------------ File ConcurrentPool.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConcurrentPoolResource [see ConcurrentPool.h], with
 * their contracts.
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

template< std::size_t M >
ConcurrentPoolResource< M >::ConcurrentPoolResource( void )
 : f_id( s_ids.fetch_add( 1 , std::memory_order_relaxed ) + 1 )
{
 }

template< std::size_t M >
ConcurrentPoolResource< M >::~ConcurrentPoolResource()
{
 Heap * h = f_heaps.load( std::memory_order_acquire );
 while( h ) {
  Heap * p = h->link;
  delete h;
  h = p;
  }
 }

template< std::size_t M >
ConcurrentPoolResource< M > & ConcurrentPoolResource< M >::global( void )
{
 static ConcurrentPoolResource * r = new ConcurrentPoolResource();
 return( *r );
 }

template< std::size_t M >
inline typename ConcurrentPoolResource< M >::Heap *
ConcurrentPoolResource< M >::mine( void )
{
 if( s_cache.id == f_id ) [[likely]]
  return( s_cache.heap );
 return( find() );
 }

template< std::size_t M >
typename ConcurrentPoolResource< M >::Heap *
ConcurrentPoolResource< M >::find( void )
{
 const std::thread::id me = std::this_thread::get_id();
 Heap * top = f_heaps.load( std::memory_order_acquire );
 Heap * h = top;
 while( h && ( h->id != me ) )
  h = h->link;
 if( ! h ) {                              // a new heap, pushed on the list
  h = new Heap;
  h->id = me;
  h->link = top;
  while( ! f_heaps.compare_exchange_weak( h->link , h ,
                                          std::memory_order_release ,
                                          std::memory_order_acquire ) ) {}
  }
 s_cache.id = f_id;
 s_cache.heap = h;
 return( h );
 }

template< std::size_t M >
inline void * ConcurrentPoolResource< M >::take( std::size_t c )
{
 Heap & h = *mine();
 if( void * p = h.pages.local( c ) ) [[likely]]
  return( p );
 return( refill( h , c ) );
 }

template< std::size_t M >
void * ConcurrentPoolResource< M >::refill( Heap & h , std::size_t c )
{
 send( h.out[ c ] , c );                   // its own frees for others
 std::atomic< Free * > & r = h.returns[ c ].top;
 if( r.load( std::memory_order_relaxed ) ) { // the returns, all at once
  Free * f = r.exchange( nullptr , std::memory_order_acquire );
  while( f ) {
   Free * const n = f->next;
   h.pages.give( Pages::pageOf( f ) , f );
   f = n;
   }
  if( void * p = h.pages.local( c ) )
   return( p );
  }
 return( h.pages.more( c ) );
 }

template< std::size_t M >
inline void ConcurrentPoolResource< M >::give( void * p , std::size_t c )
{
 ADS_PRE( p != nullptr );
 Page * const g = Pages::pageOf( p );
 ADS_PRE( g->c == c );
 Heap * const o = static_cast< Heap * >( g->owner );
 if( o == mine() ) [[likely]] {             // a slot of the own heap
  o->pages.give( g , p );
  return;
  }
 Free * const f = static_cast< Free * >( p );  // into the batch of o
 Batch & b = mine()->out[ c ];
 if( b.to != o ) {
  send( b , c );
  b.to = o;
  }
 f->next = b.head;
 b.head = f;
 if( ! b.tail )
  b.tail = f;
 if( ++b.n == BATCH )
  send( b , c );
 }

template< std::size_t M >
void ConcurrentPoolResource< M >::send( Batch & b , std::size_t c )
{
 if( b.n == 0 )
  return;
 std::atomic< Free * > & r = b.to->returns[ c ].top;
 Free * t = r.load( std::memory_order_relaxed );
 do
  b.tail->next = t;
 while( ! r.compare_exchange_weak( t , b.head , std::memory_order_release ,
                                   std::memory_order_relaxed ) );
 b.head = b.tail = nullptr;
 b.n = 0;
 }

template< std::size_t M >
template< std::size_t b , std::size_t a >
inline void * ConcurrentPoolResource< M >::allocate( void )
{
 if constexpr( pooled( b , a ) )
  return( take( classOf( b , a ) ) );
 else
  return( detail::poolNew( b , a ) );
 }

template< std::size_t M >
template< std::size_t b , std::size_t a >
inline void ConcurrentPoolResource< M >::deallocate( void * p )
{
 if constexpr( pooled( b , a ) )
  give( p , classOf( b , a ) );
 else
  detail::poolDelete( p , b , a );
 }

template< std::size_t M >
void * ConcurrentPoolResource< M >::allocate( std::size_t b , std::size_t a )
{
 if( pooled( b , a ) )
  return( take( classOf( b , a ) ) );
 return( detail::poolNew( b , a ) );
 }

template< std::size_t M >
void ConcurrentPoolResource< M >::deallocate( void * p , std::size_t b ,
                                              std::size_t a )
{
 if( pooled( b , a ) )
  give( p , classOf( b , a ) );
 else
  detail::poolDelete( p , b , a );
 }

template< std::size_t M >
void ConcurrentPoolResource< M >::collect( void )
{
 Heap & h = *mine();
 for( std::size_t c = 0 ; c < CLASSES ; ++c ) {
  send( h.out[ c ] , c );
  Free * f = h.returns[ c ].top.exchange( nullptr ,
                                          std::memory_order_acquire );
  while( f ) {
   Free * const n = f->next;
   h.pages.give( Pages::pageOf( f ) , f );
   f = n;
   }
  }
 }

template< std::size_t M >
std::size_t ConcurrentPoolResource< M >::heaps( void ) const
{
 std::size_t k = 0;
 for( const Heap * h = f_heaps.load( std::memory_order_acquire ) ; h ;
      h = h->link )
  ++k;
 return( k );
 }

template< std::size_t M >
std::size_t ConcurrentPoolResource< M >::bytes( void ) const
{
 std::size_t k = 0;
 for( const Heap * h = f_heaps.load( std::memory_order_acquire ) ; h ;
      h = h->link )
  k += h->pages.bytes();
 return( k );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ConcurrentPool.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
