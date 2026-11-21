/*--------------------------------------------------------------------------*/
/*--------------------------- File MPSCInbox.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MPSCInbox [see MPSCInbox.h], with their contracts.
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

/// the owner of the running thread, which at the end of the thread frees
/// the blocks it holds and closes its stack given back
template< class T >
class MPSCInbox< T >::Home
{
 public:
 Home( void ) : f_owner( new Owner ) { }

 ~Home() {
  Owner * o = f_owner;
  // the cells never used count as taken; a block all given out is left
  // to its consumers
  if( o->cur && ( o->used < kCells ) ) {
   const std::size_t u = kCells - o->used;
   if( o->cur->taken.fetch_add( u , std::memory_order_acq_rel ) + u ==
       kCells )
    destroy( o->cur );
   }
  for( Block * b : { o->spare ,
		     o->given.exchange( closed() ,
					std::memory_order_acquire ) } )
   while( b ) {
    Block * n = b->spare;
    destroy( b );
    b = n;
    }
  if( o->refs.fetch_sub( 1 , std::memory_order_acq_rel ) == 1 )
   delete o;
  }

 Home( const Home & ) = delete;
 Home & operator=( const Home & ) = delete;

 [[nodiscard]] Owner & owner( void ) { return( *f_owner ); }

 private:
 Owner * f_owner;
 };

template< class T >
typename MPSCInbox< T >::Owner & MPSCInbox< T >::owner( void )
{
 static thread_local Home h;
 return( h.owner() );
 }

template< class T >
typename MPSCInbox< T >::Block * MPSCInbox< T >::fresh( Owner & o )
{
 Block * b = o.spare;
 if( ! b )
  b = o.given.exchange( nullptr , std::memory_order_acquire );
 if( b ) {
  o.spare = b->spare;
  b->taken.store( 0 , std::memory_order_relaxed );
  return( b );
  }
 o.refs.fetch_add( 1 , std::memory_order_relaxed );
 return( new Block( &o ) );
 }

template< class T >
void MPSCInbox< T >::destroy( Block * b )
{
 Owner * o = b->owner;
 delete b;
 if( o->refs.fetch_sub( 1 , std::memory_order_acq_rel ) == 1 )
  delete o;
 }

template< class T >
void MPSCInbox< T >::release( Block * b , std::size_t k )
{
 if( b->taken.fetch_add( k , std::memory_order_acq_rel ) + k != kCells )
  return;
 Owner * o = b->owner;  // all taken: back to the owner, if it runs
 Block * top = o->given.load( std::memory_order_relaxed );
 do {
  if( top == closed() ) {
   destroy( b );
   return;
   }
  b->spare = top;
  } while( ! o->given.compare_exchange_weak( top , b ,
					      std::memory_order_release ,
					      std::memory_order_relaxed ) );
 }

template< class T >
template< class G , class F >
void MPSCInbox< T >::drain( Cell * c , G && count , F && f )
{
 Cell * r = nullptr;  // the stack reversed, the order of sending
 std::size_t m = 0;
 while( c ) {
  Cell * n = c->next;
  c->next = r;
  r = c;
  c = n;
  ++m;
  }
 count( m );
 Block * run = nullptr;  // the block of the last cells, and their number
 std::size_t k = 0;
 while( r ) {
  Cell * n = r->next;
  if( r->home != run ) {
   if( run )
    release( run , k );
   run = r->home;
   k = 0;
   }
  T * v = r->value();
  f( *v );
  v->~T();
  ++k;
  r = n;
  }
 if( run )
  release( run , k );
 }

/*--------------------------------------------------------------------------*/

template< class T >
MPSCInbox< T >::~MPSCInbox()
{
 drain(
   f_top.load( std::memory_order_acquire ) , []( std::size_t ) { } ,
   []( T & ) { } );
 }

template< class T >
void MPSCInbox< T >::push( T v )
{
 Owner & o = owner();
 if( ( ! o.cur ) || ( o.used == kCells ) ) {
  o.cur = fresh( o );
  o.used = 0;
  }
 Cell * c = &o.cur->cells[ o.used ];
 ::new( c->raw ) T( std::move( v ) );
 ++o.used;
 // the top read into a variable of its own: once pushed, the cell is the
 // consumer's, which may take it and write its link at once
 Cell * top = f_top.load( std::memory_order_relaxed );
 do
  c->next = top;
 while( ! f_top.compare_exchange_weak( top , c , std::memory_order_release ,
				       std::memory_order_relaxed ) );
 if( ! top )  // the first of a batch: the consumer may sleep
  f_wait.notify();
 }

template< class T >
std::size_t MPSCInbox< T >::takeInto( std::vector< T > & out )
{
 Cell * c = f_top.exchange( nullptr , std::memory_order_acquire );
 const std::size_t before = out.size();
 drain(
   c , [ & ]( std::size_t m ) { out.reserve( before + m ); } ,
   [ & ]( T & v ) { out.push_back( std::move( v ) ); } );
 return( out.size() - before );
 }

template< class T >
std::vector< T > MPSCInbox< T >::take( void )
{
 std::vector< T > out;
 takeInto( out );
 return( out );
 }

template< class T >
bool MPSCInbox< T >::empty( void ) const
{
 return( f_top.load( std::memory_order_acquire ) == nullptr );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPSCInbox.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
