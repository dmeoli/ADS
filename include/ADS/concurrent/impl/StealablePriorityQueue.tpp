/*--------------------------------------------------------------------------*/
/*-------------------- File StealablePriorityQueue.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StealablePriorityQueue and of StealingFrontier [see
 * StealablePriorityQueue.h], with their contracts. The orders of memory:
 * the flag hungry and the hint canGive are hints, read and written with no
 * order; the offer is published with release and claimed, by the owner or
 * by a thief, by an exchange with acquire and release, so that the thread
 * that gets the heap sees the pairs written into it, and an owner that
 * finds the offer gone has seen the exchange of the thief, and so its entry
 * into the counter of the frontier.
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

/*--------------------------------------------------------------------------*/
/*---------------------- StealablePriorityQueue ----------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class T , class Less , class Bd , class BdLess >
StealablePriorityQueue< K , T , Less , Bd , BdLess >::
 StealablePriorityQueue( Less less ,
                                                             BdLess bl )
 : f_h{ Heap( ByKey{ less } , bl ) , Heap( ByKey{ less } , bl ) } ,
   f_less{ std::move( less ) } , f_bl( std::move( bl ) )
{
 }

template< class K , class T , class Less , class Bd , class BdLess >
StealablePriorityQueue< K , T , Less , Bd , BdLess >::~StealablePriorityQueue()
{
 delete f_offer.exchange( nullptr , std::memory_order_acq_rel );
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_INLINE int
StealablePriorityQueue< K , T , Less , Bd , BdLess >::best( void ) const
{
 if( f_n[ 1 ] == 0 )
  return( 0 );
 if( f_n[ 0 ] == 0 )
  return( 1 );
 return( f_less( f_h[ 1 ].top() , f_h[ 0 ].top() ) ? 1 : 0 );
 }

template< class K , class T , class Less , class Bd , class BdLess >
inline void
StealablePriorityQueue< K , T , Less , Bd , BdLess >::recount( void )
{
 f_n[ 0 ] = f_h[ 0 ].size();
 f_n[ 1 ] = f_h[ 1 ].size();
 hint();
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_INLINE void
StealablePriorityQueue< K , T , Less , Bd , BdLess >::hint( void )
{
 const bool can = f_n[ 0 ] + f_n[ 1 ] >= 2;
 if( can != f_can ) {
  f_can = can;
  f_canGive.store( can , std::memory_order_relaxed );
  }
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_INLINE void
StealablePriorityQueue< K , T , Less , Bd , BdLess >::place( Entry && e )
{
 int j;
 if( f_n[ 0 ] != f_n[ 1 ] )
  j = f_n[ 0 ] < f_n[ 1 ] ? 0 : 1;
 else
  j = f_turn ^= 1;
 f_h[ j ].push( e );
 if( ++f_n[ j ] + f_n[ 1 - j ] == 2 )      // from one pair to two
  hint();
 serve();
 ADS_INVARIANT( hinted() );
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_INLINE void
StealablePriorityQueue< K , T , Less , Bd , BdLess >::serve( void )
{
 if( f_hungry.load( std::memory_order_relaxed ) )
  answer();
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_NOINLINE void
StealablePriorityQueue< K , T , Less , Bd , BdLess >::answer( void )
{
 if( f_offered ) {
  if( f_offer.load( std::memory_order_acquire ) != nullptr )
   return;                                  // one offer at a time
  f_offered = false;                        // claimed by a thief
  f_offerKey.reset();
  f_offerBound.reset();
  }
 f_hungry.store( false , std::memory_order_relaxed );
 const std::size_t m = f_n[ 0 ] + f_n[ 1 ];
 if( m < 2 )
  return;
 int j = 1 - best();
 const std::size_t s = f_n[ j ];
 if( ( 5 * s < m ) || ( 5 * s > 4 * m ) ) {
  deal();
  j = 1 - best();
  }
 auto g = std::make_unique< Heap >( std::move( f_h[ j ] ) );
 f_h[ j ].clear();
 f_offerKey.emplace( g->top().key );
 if constexpr( bounded )
  f_offerBound.emplace( g->minBound() );
 f_offerSize = g->size();
 f_offered = true;
 f_offer.store( g.release() , std::memory_order_release );
 recount();
 }

template< class K , class T , class Less , class Bd , class BdLess >
void StealablePriorityQueue< K , T , Less , Bd , BdLess >::deal( void )
{
 std::vector< Entry > A[ 2 ];
 const std::size_t m = f_h[ 0 ].size() + f_h[ 1 ].size();
 A[ 0 ].reserve( m / 2 + 1 );
 A[ 1 ].reserve( m / 2 + 1 );
 std::size_t t = 0;
 for( const Heap & h : f_h )
  for( const Entry & e : h )
   A[ ( t++ ) & 1 ].push_back( e );
 Heap H[ 2 ] = { Heap( f_less , f_bl ) , Heap( f_less , f_bl ) };
 H[ 0 ].assign( A[ 0 ].begin() , A[ 0 ].end() );
 H[ 1 ].assign( A[ 1 ].begin() , A[ 1 ].end() );
 f_h[ 0 ] = std::move( H[ 0 ] );            // no copy from here on
 f_h[ 1 ] = std::move( H[ 1 ] );
 recount();
 }

template< class K , class T , class Less , class Bd , class BdLess >
void StealablePriorityQueue< K , T , Less , Bd , BdLess >::merge( Heap & g )
{
 const int j = f_h[ 0 ].size() <= f_h[ 1 ].size() ? 0 : 1;
 std::vector< Entry > A;
 A.reserve( g.size() + f_h[ j ].size() );
 A.insert( A.end() , g.begin() , g.end() );
 A.insert( A.end() , f_h[ j ].begin() , f_h[ j ].end() );
 Heap H( f_less , f_bl );
 H.assign( A.begin() , A.end() );
 f_h[ j ] = std::move( H );
 recount();
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_NOINLINE void
StealablePriorityQueue< K , T , Less , Bd , BdLess >::settle( void )
{
 if( ! f_offered )
  return;
 if( ( f_n[ 0 ] + f_n[ 1 ] > 0 ) &&
     ! f_less.less( *f_offerKey , f_h[ best() ].top().key ) )
  return;                                   // the least pair is local
 Heap * g = f_offer.exchange( nullptr , std::memory_order_acq_rel );
 f_offered = false;
 f_offerKey.reset();
 std::optional< Bound > ob = std::move( f_offerBound );
 f_offerBound.reset();
 if( g == nullptr )                         // claimed by a thief
  return;
 std::unique_ptr< Heap > own( g );
 try {
  merge( *own );
  }
 catch( ... ) {                             // the offer stays as it was
  f_offerKey.emplace( own->top().key );
  f_offerBound = std::move( ob );
  f_offered = true;
  f_offer.store( own.release() , std::memory_order_release );
  throw;
  }
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_INLINE auto
StealablePriorityQueue< K , T , Less , Bd , BdLess >::pop( void )
 -> std::optional< Entry >
{
 if( f_offered || f_hungry.load( std::memory_order_relaxed ) ) {
  serve();
  settle();
  }
 if( f_n[ 0 ] + f_n[ 1 ] == 0 )
  return( std::nullopt );
 const int j = best();
 std::optional< Entry > e( f_h[ j ].pop() );
 if( --f_n[ j ] + f_n[ 1 - j ] == 1 )      // from two pairs to one
  hint();
 ADS_INVARIANT( hinted() );
 return( e );
 }

template< class K , class T , class Less , class Bd , class BdLess >
auto
StealablePriorityQueue< K , T , Less , Bd , BdLess >::
 top( void ) -> const Entry &
{
 settle();
 ADS_PRE( f_n[ 0 ] + f_n[ 1 ] > 0 );
 return( f_h[ best() ].top() );
 }

template< class K , class T , class Less , class Bd , class BdLess >
std::size_t StealablePriorityQueue< K , T , Less , Bd , BdLess >::steal(
                                                 StealablePriorityQueue & v )
{
 ADS_PRE( &v != this );
 ADS_PRE( empty() );
 Heap * g = v.f_offer.load( std::memory_order_relaxed );
 if( g == nullptr ) {
  if( ! v.f_canGive.load( std::memory_order_relaxed ) )
   return( 0 );
  if( ! v.f_hungry.load( std::memory_order_relaxed ) )
   v.f_hungry.store( true , std::memory_order_relaxed );
  Backoff b;
  while( ( g == nullptr ) && b.spinning() ) {
   b.pause();
   g = v.f_offer.load( std::memory_order_relaxed );
   }
  if( g == nullptr )
   return( 0 );
  }
 g = v.f_offer.exchange( nullptr , std::memory_order_acq_rel );
 if( g == nullptr )                         // another thread was first
  return( 0 );
 const std::size_t s = g->size();
 f_h[ 0 ] = std::move( *g );                // no copy, and the pairs are
 delete g;                                  // safe from here on
 f_h[ 1 ].clear();
 recount();
 deal();
 ADS_INVARIANT( hinted() );
 return( s );
 }

template< class K , class T , class Less , class Bd , class BdLess >
auto StealablePriorityQueue< K , T , Less , Bd , BdLess >::minBound( void )
 const -> std::optional< Bound > requires bounded
{
 std::optional< Bound > b;
 for( int j = 0 ; j < 2 ; ++j )
  if( f_n[ j ] > 0 ) {
   const Bound c = f_h[ j ].minBound();
   if( ! b || f_bl( c , *b ) )
    b.emplace( c );
   }
 if( f_offered && ( f_offer.load( std::memory_order_acquire ) != nullptr ) &&
     ( ! b || f_bl( *f_offerBound , *b ) ) )
  b = f_offerBound;
 return( b );
 }

template< class K , class T , class Less , class Bd , class BdLess >
std::size_t
StealablePriorityQueue< K , T , Less , Bd , BdLess >::size( void ) const
{
 std::size_t m = f_n[ 0 ] + f_n[ 1 ];
 if( f_offered && ( f_offer.load( std::memory_order_acquire ) != nullptr ) )
  m += f_offerSize;
 return( m );
 }

template< class K , class T , class Less , class Bd , class BdLess >
bool
StealablePriorityQueue< K , T , Less , Bd , BdLess >::offering( void ) const
{
 return( f_offered &&
         ( f_offer.load( std::memory_order_acquire ) != nullptr ) );
 }

template< class K , class T , class Less , class Bd , class BdLess >
inline bool
StealablePriorityQueue< K , T , Less , Bd , BdLess >::hinted( void ) const
{
 return( ( f_n[ 0 ] == f_h[ 0 ].size() ) && ( f_n[ 1 ] == f_h[ 1 ].size() ) &&
         ( f_can == ( f_n[ 0 ] + f_n[ 1 ] >= 2 ) ) );
 }

template< class K , class T , class Less , class Bd , class BdLess >
bool
StealablePriorityQueue< K , T , Less , Bd , BdLess >::consistent( void ) const
{
 for( const Heap & h : f_h ) {
  if( ! h.consistent() )
   return( false );
  if constexpr( ! bounded ) {               // the order, for the plain heap
   const std::size_t n = h.size();
   const Entry * A = h.begin();
   for( std::size_t i = 1 ; i < n ; ++i )
    if( f_less( A[ i ] , A[ ( i - 1 ) / 4 ] ) )
     return( false );
   }
  }
 return( hinted() );
 }

template< class K , class T , class Less , class Bd , class BdLess >
template< class F >
std::size_t
StealablePriorityQueue< K , T , Less , Bd , BdLess >::drain( F && f )
{
 if( f_offered ) {
  Heap * g = f_offer.exchange( nullptr , std::memory_order_acq_rel );
  f_offered = false;
  f_offerKey.reset();
  f_offerBound.reset();
  if( g != nullptr ) {
   std::unique_ptr< Heap > own( g );
   merge( *own );
   }
  }
 std::size_t n = 0;
 while( f_n[ 0 ] + f_n[ 1 ] > 0 ) {
  const int j = best();
  Entry e = f_h[ j ].pop();
  --f_n[ j ];
  f( std::move( e ) );
  ++n;
  }
 hint();
 return( n );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- StealingFrontier -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class T , class Less , class Bd , class BdLess >
StealingFrontier< K , T , Less , Bd , BdLess >::StealingFrontier(
                     std::size_t p , std::uint64_t seed , Less less ,
                     BdLess bl )
 : f_p( p ) , f_w( nullptr ) , f_active( std::int64_t( p ) )
{
 ADS_PRE( p >= 1 );
 void * m = ::operator new( p * sizeof( Worker ) ,
                            std::align_val_t( alignof( Worker ) ) );
 Worker * w = static_cast< Worker * >( m );
 std::size_t i = 0;
 try {
  for( ; i < p ; ++i )
   new( w + i ) Worker( less , bl ,
                        seed + 0x9E3779B97F4A7C15ull * ( i + 1 ) );
  }
 catch( ... ) {
  while( i > 0 )
   w[ --i ].~Worker();
  ::operator delete( m , std::align_val_t( alignof( Worker ) ) );
  throw;
  }
 f_w = w;
 }

template< class K , class T , class Less , class Bd , class BdLess >
StealingFrontier< K , T , Less , Bd , BdLess >::~StealingFrontier()
{
 for( std::size_t i = f_p ; i > 0 ; )
  f_w[ --i ].~Worker();
 ::operator delete( f_w , std::align_val_t( alignof( Worker ) ) );
 }

template< class K , class T , class Less , class Bd , class BdLess >
std::uint64_t
StealingFrontier< K , T , Less , Bd , BdLess >::next( Worker & w )
{
 std::uint64_t z = ( w.x += 0x9E3779B97F4A7C15ull );
 z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
 z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBull;
 return( z ^ ( z >> 31 ) );
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_INLINE auto
StealingFrontier< K , T , Less , Bd , BdLess >::pop( std::size_t i )
 -> std::optional< Entry >
{
 ADS_PRE( i < workers() );
 if( stopped() )
  return( std::nullopt );
 std::optional< Entry > e = f_w[ i ].q.pop();
 if( e ) [[likely]]
  return( e );
 return( idle( i ) );
 }

template< class K , class T , class Less , class Bd , class BdLess >
ADS_SPQ_NOINLINE auto
StealingFrontier< K , T , Less , Bd , BdLess >::idle( std::size_t i )
 -> std::optional< Entry >
{
 Worker & w = f_w[ i ];
 f_active.fetch_sub( 1 , std::memory_order_acq_rel );
 const std::size_t p = workers();
 Backoff b;
 for( ; ; ) {
  if( stopped() || ( f_active.load( std::memory_order_acquire ) == 0 ) )
   return( std::nullopt );
  if( p > 1 ) {
   std::size_t v = std::size_t( next( w ) % ( p - 1 ) );
   v += ( v >= i );                         // any worker but i
   Queue & q = f_w[ v ].q;
   if( q.stealable() ) {
    f_active.fetch_add( 1 , std::memory_order_acq_rel );
    std::size_t s = 0;
    try {
     s = w.q.steal( q );
     }
    catch( ... ) {                          // the pairs stolen, if any,
     if( w.q.empty() )                      // are in the queue of i
      f_active.fetch_sub( 1 , std::memory_order_acq_rel );
     throw;
     }
    if( s > 0 ) {
     w.steals.fetch_add( 1 , std::memory_order_relaxed );
     return( w.q.pop() );
     }
    f_active.fetch_sub( 1 , std::memory_order_acq_rel );
    }
   }
  b.pause();
  }
 }

template< class K , class T , class Less , class Bd , class BdLess >
std::size_t
StealingFrontier< K , T , Less , Bd , BdLess >::steals( void ) const
{
 std::size_t s = 0;
 for( std::size_t i = 0 ; i < f_p ; ++i )
  s += f_w[ i ].steals.load( std::memory_order_relaxed );
 return( s );
 }

template< class K , class T , class Less , class Bd , class BdLess >
template< class E , class F >
void StealingFrontier< K , T , Less , Bd , BdLess >::run( E & ex , F && f )
{
 ex.run( workers() , [ this , &f ]( std::size_t i ) {
  try {
   f( i );
   }
  catch( ... ) {
   stop();
   throw;
   }
  } );
 }

template< class K , class T , class Less , class Bd , class BdLess >
template< class F >
std::size_t StealingFrontier< K , T , Less , Bd , BdLess >::leftovers( F && f )
{
 std::size_t n = 0;
 for( std::size_t i = 0 ; i < f_p ; ++i )
  n += f_w[ i ].q.drain( f );
 return( n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------ End File StealablePriorityQueue.tpp -------------------*/
/*--------------------------------------------------------------------------*/
