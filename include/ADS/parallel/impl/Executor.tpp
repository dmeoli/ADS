/*--------------------------------------------------------------------------*/
/*--------------------------- File Executor.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The patterns of Executor.h: the fork and join, the loops, the reduction
 * and the prefix sums by decoupled look-back.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <thread>

namespace ADS {

namespace detail {

/// the first exception thrown by the calls of a pattern, and the flag that
/// stops the claim of new blocks
class FirstError {
 public:

 /// true once a call has thrown
 [[nodiscard]] bool failed( void ) const noexcept {
  return( f_failed.load( std::memory_order_relaxed ) );
  }

 /// keeps the current exception if it is the first
 void keep( void ) noexcept {
  std::lock_guard< std::mutex > g( f_lock );
  if( ! f_error )
   f_error = std::current_exception();
  f_failed.store( true , std::memory_order_relaxed );
  }

 /// throws the exception kept, if any
 void rethrow( void ) const {
  if( f_error )
   std::rethrow_exception( f_error );
  }

 private:

 std::atomic< bool > f_failed{ false };
 std::mutex f_lock;
 std::exception_ptr f_error;
 };

/// a pause of the processor in a loop that waits for another thread
inline void spinPause( unsigned & k ) noexcept
{
 if( ++k < 1024 ) {
#if defined( __x86_64__ ) || defined( __i386__ )
  __builtin_ia32_pause();
#elif defined( __aarch64__ )
  asm volatile( "yield" );
#endif
  }
 else
  std::this_thread::yield();
 }

/// the state of a tile of the scan, on a line of its own: 0 nothing, 1 its
/// aggregate, 2 its inclusive prefix
template< class T >
struct alignas( 64 ) ScanTile {
 std::atomic< std::uint8_t > flag{ 0 };
 std::optional< T > aggregate , prefix;
 };

/// the scan of the n items from in into out by decoupled look-back,
/// inclusive or not; the total
template< bool Inclusive , class E , class I , class O , class T , class Op >
T lookBackScan( E & ex , I in , std::size_t n , O out , T init , Op & op )
{
 const std::size_t p = ex.concurrency();
 // tiles of about 64 KB, at least 256 items
 const std::size_t tile = std::max< std::size_t >( 256 ,
                                         ( std::size_t( 1 ) << 16 ) /
                                         std::max< std::size_t >(
                                          sizeof( T ) , 1 ) );
 const std::size_t tiles = ( n + tile - 1 ) / tile;
 if( ( p == 1 ) || ( tiles <= 1 ) ) {      // one pass on one thread
  T acc = init;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   T x = in[ i ];
   if constexpr( Inclusive ) {
    acc = op( acc , x );
    out[ i ] = acc;
    }
   else {
    out[ i ] = acc;
    acc = op( acc , x );
    }
   }
  return( acc );
  }
 std::unique_ptr< ScanTile< T >[] > S( new ScanTile< T >[ tiles ] );
 std::atomic< std::size_t > next{ 0 };
 FirstError error;
 ex.bulk( std::min( p , tiles ) , [ & ]( std::size_t ) {
  for( ;; ) {
   const std::size_t c = next.fetch_add( 1 , std::memory_order_relaxed );
   if( c >= tiles )
    return;
   const std::size_t lo = c * tile , hi = std::min( n , lo + tile );
   try {
    if( error.failed() )
     throw( 0 );                            // only to publish the tile
    // the aggregate of the tile, published at once
    T a = in[ lo ];
    for( std::size_t i = lo + 1 ; i < hi ; ++i )
     a = op( a , T( in[ i ] ) );
    T excl = init;
    if( c > 0 ) {
     S[ c ].aggregate.emplace( a );
     S[ c ].flag.store( 1 , std::memory_order_release );
     // the look-back: aggregates on the left, until a prefix
     std::optional< T > acc;
     for( std::size_t j = c ; j-- > 0 ; ) {
      unsigned k = 0;
      std::uint8_t f;
      while( ( f = S[ j ].flag.load( std::memory_order_acquire ) ) == 0 )
       spinPause( k );
      if( f == 3 )                          // a tile that failed
       throw( 0 );
      const T & v = ( f == 2 ) ? *S[ j ].prefix : *S[ j ].aggregate;
      acc = acc ? op( v , *acc ) : v;
      if( f == 2 )
       break;
      }
     excl = *acc;
     }
    else
     excl = init;
    S[ c ].prefix.emplace( op( excl , a ) );
    S[ c ].flag.store( 2 , std::memory_order_release );
    // the prefixes of the tile, from its cache
    T run = excl;
    for( std::size_t i = lo ; i < hi ; ++i ) {
     T x = in[ i ];
     if constexpr( Inclusive ) {
      run = op( run , x );
      out[ i ] = run;
      }
     else {
      out[ i ] = run;
      run = op( run , x );
      }
     }
    }
   catch( ... ) {
    if( ! error.failed() )
     error.keep();
    S[ c ].flag.store( 3 , std::memory_order_release );
    }
   }
  } );
 error.rethrow();
 return( *S[ tiles - 1 ].prefix );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , class F , class G >
void parallelInvoke( E & ex , F && f , G && g )
{
 if constexpr( requires { ex.invoke( f , g ); } )
  ex.invoke( f , g );
 else {
  detail::FirstError error;
  ex.bulk( 2 , [ & ]( std::size_t i ) {
   try {
    if( i == 0 ) f(); else g();
    }
   catch( ... ) {
    error.keep();
    }
   } );
  error.rethrow();
  }
 }

/*--------------------------------------------------------------------------*/

inline std::pair< std::size_t , std::size_t > blocksOf( std::size_t first ,
                                                        std::size_t last ,
                                                        std::size_t grain )
{
 const std::size_t n = last > first ? last - first : 0;
 if( n == 0 )
  return( std::pair< std::size_t , std::size_t >( 0 , 0 ) );
 grain = std::max< std::size_t >( grain , 1 );
 std::size_t size = std::max( grain , ( n + kBlocks - 1 ) / kBlocks );
 return( std::pair< std::size_t , std::size_t >( ( n + size - 1 ) / size ,
                                                 size ) );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class F >
void parallelForBlocks( E & ex , std::size_t first , std::size_t last ,
                        F && f , std::size_t grain )
{
 const auto cut = blocksOf( first , last , grain );
 const std::size_t b = cut.first , size = cut.second;
 if( b == 0 )
  return;
 if( ( b == 1 ) || ( ex.concurrency() == 1 ) ) {
  for( std::size_t i = 0 ; i < b ; ++i )
   f( first + i * size , std::min( last , first + ( i + 1 ) * size ) );
  return;
  }
 detail::FirstError error;
 ex.bulk( b , [ & ]( std::size_t i ) {
  if( error.failed() )
   return;
  try {
   f( first + i * size , std::min( last , first + ( i + 1 ) * size ) );
   }
  catch( ... ) {
   error.keep();
   }
  } );
 error.rethrow();
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class F >
void parallelFor( E & ex , std::size_t first , std::size_t last , F && f ,
                  std::size_t grain )
{
 parallelForBlocks( ex , first , last ,
                    [ & ]( std::size_t lo , std::size_t hi ) {
                     for( std::size_t i = lo ; i < hi ; ++i )
                      f( i );
                     } , grain );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class T , class F , class Op >
T parallelReduce( E & ex , std::size_t first , std::size_t last , T id ,
                  F && f , Op op , std::size_t grain )
{
 const auto cut = blocksOf( first , last , grain );
 const std::size_t b = cut.first , size = cut.second;
 if( b == 0 )
  return( id );
 std::vector< std::optional< T > > part( b );
 parallelForBlocks( ex , 0 , b , [ & ]( std::size_t lo , std::size_t hi ) {
  for( std::size_t i = lo ; i < hi ; ++i )
   part[ i ].emplace( f( first + i * size ,
                         std::min( last , first + ( i + 1 ) * size ) ) );
  } , 1 );
 T r = std::move( id );
 for( auto & x : part )
  r = op( std::move( r ) , std::move( *x ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class T , class Op >
T parallelInclusiveScan( E & ex , I in , std::size_t n , O out , T init ,
                         Op op )
{
 return( detail::lookBackScan< true >( ex , in , n , out , std::move( init ) ,
                                       op ) );
 }

template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class T , class Op >
T parallelExclusiveScan( E & ex , I in , std::size_t n , O out , T init ,
                         Op op )
{
 return( detail::lookBackScan< false >( ex , in , n , out ,
                                        std::move( init ) , op ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Executor.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
