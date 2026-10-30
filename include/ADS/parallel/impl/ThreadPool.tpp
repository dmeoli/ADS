/*--------------------------------------------------------------------------*/
/*-------------------------- File ThreadPool.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ThreadPool [see ThreadPool.h]: the workers, the jobs of
 * bulk() and the tasks of invoke().
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

/// the slot of the caller for the time of a call
class ThreadPool::Enter {
 public:

 explicit Enter( ThreadPool & p ) : f_pool( p ) {
  if( tl_self.pool == &p ) {                // a thread of the pool
   f_slot = tl_self.slot;
   return;
   }
  p.f_master.lock();                        // the p-th, one at a time
  f_saved = tl_self;
  f_outside = true;
  tl_self = Self{ &p , 0 };
  f_slot = 0;
  }

 ~Enter() {
  if( f_outside ) {
   tl_self = f_saved;
   f_pool.f_master.unlock();
   }
  }

 Enter( const Enter & ) = delete;
 Enter & operator=( const Enter & ) = delete;

 [[nodiscard]] std::size_t slot( void ) const { return( f_slot ); }

 private:

 ThreadPool & f_pool;
 Self f_saved{ nullptr , 0 };
 std::size_t f_slot = 0;
 bool f_outside = false;
 };

/*--------------------------------------------------------------------------*/

/// a job of bulk(): the calls not claimed, the tokens out, the first error
template< class F >
struct ThreadPool::BulkJob : Task {
 F * f;
 std::size_t n;
 alignas( 64 ) std::atomic< std::size_t > next{ 0 };
 alignas( 64 ) std::atomic< std::size_t > out{ 0 };
 std::atomic< bool > failed{ false };
 std::exception_ptr error;
 std::mutex lock;

 /// claims calls until none is left
 void work( void ) noexcept {
  for( ;; ) {
   const std::size_t i = next.fetch_add( 1 , std::memory_order_relaxed );
   if( i >= n )
    return;
   try {
    ( *f )( i );
    }
   catch( ... ) {
    std::lock_guard< std::mutex > g( lock );
    if( ! error )
     error = std::current_exception();
    failed.store( true , std::memory_order_relaxed );
    next.store( n , std::memory_order_relaxed );
    }
   }
  }

 /// a token run by a thread: its share of the calls, then the token back,
 /// the last access to the job
 static void entry( Task * t ) noexcept {
  auto * j = static_cast< BulkJob * >( t );
  j->work();
  j->out.fetch_sub( 1 , std::memory_order_release );
  }
 };

/// the task of invoke(): g, and whether it has returned
template< class G >
struct ThreadPool::InvokeTask : Task {
 G * g;
 std::atomic< bool > done{ false };
 std::exception_ptr error;

 static void entry( Task * t ) noexcept {
  auto * k = static_cast< InvokeTask * >( t );
  try {
   ( *k->g )();
   }
  catch( ... ) {
   k->error = std::current_exception();
   }
  k->done.store( true , std::memory_order_release );
  }
 };

/*--------------------------------------------------------------------------*/

inline ThreadPool::ThreadPool( std::size_t p , std::int64_t spin )
 : f_spin( spin )
{
 if( p == 0 )
  p = std::max< std::size_t >( std::thread::hardware_concurrency() , 1 );
 ADS_PRE( p >= 1 );
 for( std::size_t s = 0 ; s < p ; ++s ) {
  f_slot.push_back( std::make_unique< Slot >() );
  f_slot.back()->seed = 0x9E3779B97F4A7C15ull * ( s + 1 );
  }
 for( std::size_t s = 1 ; s < p ; ++s )
  f_thread.emplace_back( [ this , s ] { loop( s ); } );
 }

inline ThreadPool::~ThreadPool()
{
 f_stop.store( true , std::memory_order_release );
 f_event.notify();
 for( auto & t : f_thread )
  t.join();
 }

/*--------------------------------------------------------------------------*/

inline ThreadPool::Task * ThreadPool::find( std::size_t s )
{
 Slot & me = *f_slot[ s ];
 if( ! me.deque.empty() )
  if( auto t = me.deque.take() )
   return( *t );
 const std::size_t p = f_slot.size();
 if( p == 1 )
  return( nullptr );
 // a random victim, then the others in turn
 me.seed ^= me.seed << 13;
 me.seed ^= me.seed >> 7;
 me.seed ^= me.seed << 17;
 std::size_t v = std::size_t( me.seed % p );
 for( std::size_t k = 0 ; k < p ; ++k , v = ( v + 1 == p ? 0 : v + 1 ) ) {
  if( v == s )
   continue;
  Task * t;
  if( f_slot[ v ]->deque.steal( t ) )
   return( t );
  }
 return( nullptr );
 }

inline bool ThreadPool::help( std::size_t s )
{
 const std::size_t p = f_slot.size();
 Slot & me = *f_slot[ s ];
 me.seed ^= me.seed << 13;
 me.seed ^= me.seed >> 7;
 me.seed ^= me.seed << 17;
 std::size_t v = std::size_t( me.seed % p );
 for( std::size_t k = 0 ; k < p ; ++k , v = ( v + 1 == p ? 0 : v + 1 ) ) {
  if( v == s )
   continue;
  Task * t;
  if( f_slot[ v ]->deque.steal( t ) ) {
   t->run( t );
   return( true );
   }
  }
 return( false );
 }

inline bool ThreadPool::claim( void )
{
 std::size_t i;
 void ( * call )( void * , std::size_t );
 void * ctx;
 {
  std::lock_guard< std::mutex > g( f_runLock );
  if( ( ! f_runOpen.load( std::memory_order_relaxed ) ) ||
      ( f_runNext >= f_runN ) )
   return( false );
  i = f_runNext++;
  if( f_runNext == f_runN )
   f_runOpen.store( false , std::memory_order_relaxed );
  call = f_runCall;
  ctx = f_runCtx;
  }
 call( ctx , i );
 f_runDone.fetch_add( 1 , std::memory_order_release );
 return( true );
 }

inline void ThreadPool::loop( std::size_t s )
{
 tl_self = Self{ this , s };
 for( ;; ) {
  Task * t = nullptr;
  bool open = false;
  f_event.await( [ & ] {
   // a call of a run first: only an idle worker takes one
   if( ( open = f_runOpen.load( std::memory_order_acquire ) ) )
    return( true );
   t = find( s );
   return( ( t != nullptr ) || f_stop.load( std::memory_order_acquire ) );
   } , f_spin );
  if( open ) {
   claim();
   continue;
   }
  if( ! t )
   return;
  t->run( t );
  }
 }

/*--------------------------------------------------------------------------*/

template< class F >
void ThreadPool::bulk( std::size_t n , F && f )
{
 if( n == 0 )
  return;
 const std::size_t p = f_slot.size();
 if( ( p == 1 ) || ( n == 1 ) ) {
  for( std::size_t i = 0 ; i < n ; ++i )
   f( i );
  return;
  }
 Enter e( *this );
 auto & d = f_slot[ e.slot() ]->deque;
 using Job = BulkJob< std::remove_reference_t< F > >;
 Job j;
 j.run = &Job::entry;
 j.f = &f;
 j.n = n;
 const std::size_t h = std::min( p - 1 , n - 1 );
 j.out.store( h , std::memory_order_relaxed );
 for( std::size_t k = 0 ; k < h ; ++k )
  d.push( &j );
 f_event.notify();
 j.work();
 // the tokens nobody stole, from the bottom of the deque; below them only
 // the tasks of the calls that enclose this one
 std::size_t back = 0;
 while( back < h ) {
  auto t = d.take();
  if( ! t )
   break;
  if( *t != &j ) {
   d.push( *t );
   break;
   }
  ++back;
  }
 if( back > 0 )
  j.out.fetch_sub( back , std::memory_order_relaxed );
 unsigned k = 0;
 while( j.out.load( std::memory_order_acquire ) != 0 )
  if( ! help( e.slot() ) )
   detail::spinPause( k );
 if( j.error )
  std::rethrow_exception( j.error );
 }

/*--------------------------------------------------------------------------*/

template< class F , class G >
void ThreadPool::invoke( F && f , G && g )
{
 if( f_slot.size() == 1 ) {
  f();
  g();
  return;
  }
 Enter e( *this );
 auto & d = f_slot[ e.slot() ]->deque;
 using Inv = InvokeTask< std::remove_reference_t< G > >;
 Inv k;
 k.run = &Inv::entry;
 k.g = &g;
 d.push( &k );
 f_event.notify();
 std::exception_ptr ef;
 try {
  f();
  }
 catch( ... ) {
  ef = std::current_exception();
  }
 Task * t = nullptr;
 if( const auto o = d.take() )
  t = *o;
 if( t == &k )
  Inv::entry( &k );                         // not stolen: here
 else {
  if( t )                                   // a task of an enclosing call
   d.push( t );
  unsigned w = 0;
  while( ! k.done.load( std::memory_order_acquire ) )
   if( ! help( e.slot() ) )
    detail::spinPause( w );
  }
 if( ef )
  std::rethrow_exception( ef );
 if( k.error )
  std::rethrow_exception( k.error );
 }

/*--------------------------------------------------------------------------*/

template< class F >
void ThreadPool::run( std::size_t p , F && f )
{
 ADS_PRE( p <= f_slot.size() );
 detail::RunCaller caller;
 if( p <= 1 ) {
  if( p == 1 )
   f( 0 );
  return;
  }
 std::lock_guard< std::mutex > one( f_runOne );
 Enter e( *this );
 detail::FirstError error;
 auto call = [ & ]( std::size_t i ) noexcept {
  try {
   f( i );
   }
  catch( ... ) {
   error.keep();
   }
  };
 using C = decltype( call );
 {
  std::lock_guard< std::mutex > g( f_runLock );
  f_runCall = []( void * c , std::size_t i ) {
   ( *static_cast< C * >( c ) )( i );
   };
  f_runCtx = &call;
  f_runNext = 1;
  f_runN = p;
  f_runDone.store( 0 , std::memory_order_relaxed );
  f_runOpen.store( true , std::memory_order_release );
  }
 f_event.notify();
 call( 0 );
 unsigned k = 0;
 while( f_runDone.load( std::memory_order_acquire ) != p - 1 )
  if( ! help( e.slot() ) )
   detail::spinPause( k );
 error.rethrow();
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ThreadPool.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
