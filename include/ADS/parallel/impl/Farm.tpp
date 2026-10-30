/*--------------------------------------------------------------------------*/
/*----------------------------- File Farm.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The farm with feedback [see Farm.h]: the master and the workers on the
 * run() of an executor, and the emitter and the workers of a farm of
 * FastFlow.
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

namespace detail {

/// the farm of w = 0: the master works out its tasks, in their order
template< class T , class S , class O , class W >
void farmAlone( S & start , O & onResult , W & work )
{
 std::deque< T > ready;
 auto emit = [ & ]( T t ) { ready.push_back( std::move( t ) ); };
 start( emit );
 while( ! ready.empty() ) {
  T t = std::move( ready.front() );
  ready.pop_front();
  if( ! onResult( work( std::move( t ) ) , emit ) )
   return;
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , class R , WorkerExecutor E , class S , class O ,
          class W >
void feedbackFarm( E & ex , std::size_t w , S && start , O && onResult ,
                   W && work )
{
 ADS_PRE( ( w == 0 ) || ( w + 1 <= ex.concurrency() ) );
 if( w == 0 ) {
  detail::farmAlone< T >( start , onResult , work );
  return;
  }
 // the tasks for the free workers, the results with a lane per worker
 std::size_t c = 64;
 while( c < 4 * w )
  c *= 2;
 MPMCQueue< std::optional< T > > tasks( c );
 MPSCQueue< std::optional< R > , 64 > results;
 std::vector< typename MPSCQueue< std::optional< R > , 64 >::Producer >
  lane( w );
 for( auto & l : lane )
  l = results.producer();
 std::atomic< bool > end{ false };
 detail::FirstError error;

 auto master = [ & ]( void ) {
  std::deque< T > ready;                   // emitted, not yet queued
  std::size_t m = 0;                       // emitted, not yet back
  auto emit = [ & ]( T t ) {
   ready.push_back( std::move( t ) );
   ++m;
   };
  try {
   start( emit );
   bool go = true;
   unsigned k = 0;
   while( go && ( m > 0 ) ) {
    bool did = false;
    while( ! ready.empty() ) {
     std::optional< T > t( std::move( ready.front() ) );
     if( ! tasks.push( std::move( t ) ) ) {
      ready.front() = std::move( *t );     // full: kept for later
      break;
      }
     ready.pop_front();
     did = true;
     }
    std::optional< R > r;
    while( go && results.tryPop( r ) ) {
     did = true;
     --m;
     if( r && ! error.failed() && ! onResult( std::move( *r ) , emit ) )
      go = false;
     r.reset();
     }
    if( error.failed() )
     go = false;
    if( did )
     k = 0;
    else
     detail::spinPause( k );
    }
   }
  catch( ... ) {
   error.keep();
   }
  end.store( true , std::memory_order_release );
  };

 auto worker = [ & ]( std::size_t i ) {
  unsigned k = 0;
  while( ! end.load( std::memory_order_acquire ) ) {
   std::optional< T > t;
   if( ! tasks.tryPop( t ) ) {
    detail::spinPause( k );
    continue;
    }
   k = 0;
   std::optional< R > r;
   if( ! error.failed() )
    try {
     r.emplace( work( std::move( *t ) ) );
     }
    catch( ... ) {
     error.keep();
     }
   lane[ i ].push( std::move( r ) );
   }
  };

 ex.run( w + 1 , [ & ]( std::size_t i ) {
  if( i == 0 )
   master();
  else
   worker( i - 1 );
  } );
 error.rethrow();
 }

/*--------------------------------------------------------------------------*/

#if defined( ADS_WITH_FASTFLOW )

namespace detail {

/// the emitter of the farm of FastFlow: the master
template< class T , class R , class S , class O >
struct FarmEmitter : ff::ff_node_t< std::optional< R > , T > {
 FarmEmitter( S & s , O & o , std::atomic< bool > & stop , FirstError & e )
  : start( s ) , onResult( o ) , stop( stop ) , error( e ) {}

 T * svc( std::optional< R > * r ) override {
  auto emit = [ this ]( T t ) {
   ++m;
   this->ff_send_out( new T( std::move( t ) ) );
   };
  try {
   if( r == nullptr )
    start( emit );
   else {
    std::unique_ptr< std::optional< R > > own( r );
    --m;
    if( ( ! stopped ) && own->has_value() &&
        ! onResult( std::move( **own ) , emit ) )
     stopped = true;
    }
   }
  catch( ... ) {
   error.keep();
   }
  if( error.failed() )
   stopped = true;
  if( stopped )
   stop.store( true , std::memory_order_relaxed );
  return( m == 0 ? this->EOS : this->GO_ON );
  }

 S & start;                                ///< the first tasks
 O & onResult;                             ///< the master
 std::atomic< bool > & stop;               ///< told to the workers
 FirstError & error;                       ///< the first exception
 std::size_t m = 0;                        ///< the tasks not yet back
 bool stopped = false;                     ///< the master has stopped
 };

/// a worker of the farm of FastFlow
template< class T , class R , class W >
struct FarmWorker : ff::ff_node_t< T , std::optional< R > > {
 FarmWorker( W & w , std::atomic< bool > & stop , FirstError & e )
  : work( w ) , stop( stop ) , error( e ) {}

 std::optional< R > * svc( T * t ) override {
  std::unique_ptr< T > own( t );
  auto * r = new std::optional< R >();
  if( ! stop.load( std::memory_order_relaxed ) )
   try {
    r->emplace( work( std::move( *own ) ) );
    }
   catch( ... ) {
    error.keep();
    }
  return( r );
  }

 W & work;                                 ///< the work of a task
 std::atomic< bool > & stop;               ///< set by the master
 FirstError & error;                       ///< the first exception
 };

}  // end( namespace detail )

template< class T , class R , class S , class O , class W >
void feedbackFarm( FastFlowExecutor & ex , std::size_t w , S && start ,
                   O && onResult , W && work )
{
 ADS_PRE( w <= ex.concurrency() );
 if( w == 0 ) {
  detail::farmAlone< T >( start , onResult , work );
  return;
  }
 std::atomic< bool > stop{ false };
 detail::FirstError error;
 using Emitter = detail::FarmEmitter< T , R , std::remove_reference_t< S > ,
                                      std::remove_reference_t< O > >;
 using Worker = detail::FarmWorker< T , R , std::remove_reference_t< W > >;
 Emitter emitter( start , onResult , stop , error );
 std::vector< std::unique_ptr< ff::ff_node > > workers;
 for( std::size_t i = 0 ; i < w ; ++i )
  workers.push_back( std::make_unique< Worker >( work , stop , error ) );
 ff::ff_Farm<> farm( std::move( workers ) , emitter );
 farm.remove_collector();
 farm.set_scheduling_ondemand();
 farm.wrap_around();
 if( farm.run_and_wait_end() < 0 )
  throw( std::runtime_error( "feedbackFarm: the farm of FastFlow failed" ) );
 error.rethrow();
 }

#endif  /* ADS_WITH_FASTFLOW */

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Farm.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
