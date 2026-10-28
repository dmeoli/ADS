/*--------------------------------------------------------------------------*/
/*--------------------- File ParallelShortestPaths.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The stepping algorithms of ParallelShortestPaths.h.
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

#include <cmath>
#include <mutex>
#include <limits>
#include <type_traits>

namespace ADS {

namespace detail {

/// whether g is a StaticGraph, whose arcs are read by their places
template< class G >
inline constexpr bool isStaticGraph = false;

template< class W >
inline constexpr bool isStaticGraph< StaticGraph< W > > = true;

/// the arcs of u in g
template< class G >
std::size_t arcsOf( const G & g , int u )
{
 if constexpr( isStaticGraph< G > ) return( std::size_t( g.degree( u ) ) );
 else if constexpr( requires { g.neighbors( u ).size(); } )
  return( g.neighbors( u ).size() );
 else {
  std::size_t k = 0;
  g.forEachAdj( u , [ & ]( int ) { ++k; } );
  return( k );
  }
 }

/// the entries of a step from which the Delta-stepping decides whether to
/// keep the distance at which each node was last relaxed (0: from the
/// start), a hook of the tests
inline std::atomic< std::size_t > keepFrom{ 4096 };

/// x set to v if v is smaller, atomically; whether it was
template< class D >
bool writeMin( D & x , D v )
{
 std::atomic_ref< D > a( x );
 D c = a.load( std::memory_order_relaxed );
 while( v < c )
  if( a.compare_exchange_weak( c , v ) )
   return( true );
 return( false );
 }

/// the vectors that f( b ) gives for the blocks b = 0 , ... , B - 1,
/// concatenated in their order after the items of head
template< class E , class F >
std::vector< int > concatBlocks( E & ex , std::size_t B ,
                                 std::vector< int > head , F && f )
{
 std::vector< std::vector< int > > L( B );
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) { L[ b ] = f( b ); } , 1 );
 std::vector< std::size_t > at( B + 1 , head.size() );
 for( std::size_t b = 0 ; b < B ; ++b )
  at[ b + 1 ] = at[ b ] + L[ b ].size();
 head.resize( at[ B ] );
 // the copies in parallel only when they are many, as the small steps of
 // a graph of large diameter are
 auto copy = [ & ]( std::size_t b ) {
  std::copy( L[ b ].begin() , L[ b ].end() ,
             head.begin() + std::ptrdiff_t( at[ b ] ) ); };
 if( at[ B ] - at[ 0 ] < ( std::size_t( 1 ) << 16 ) )
  for( std::size_t b = 0 ; b < B ; ++b ) copy( b );
 else parallelFor( ex , 0 , B , copy , 1 );
 return( head );
 }

/// the steps from s, each extracting the active nodes v with d[ v ] at
/// most the threshold that theta( A , d ) gives for the active nodes A,
/// at least their least distance
template< Executor E , Graph G , class T >
std::vector< typename G::Weight > stepping( E & ex , const G & g , int s ,
                                            T && theta )
{
 using D = typename G::Weight;
 const int n = g.n();
 std::vector< D > d( std::size_t( n ) , infinity< D >() );
 std::vector< std::uint8_t > in( std::size_t( n ) , 0 );
 d[ std::size_t( s ) ] = D( 0 );
 in[ std::size_t( s ) ] = 1;
 std::vector< int > A{ s } , F , R;
 constexpr std::size_t kGrain = 8192;      // the nodes of a block
 constexpr std::size_t kArcs = 4096;       // the most arcs of a block
 constexpr std::size_t kFused = 1024;      // the nodes a block relaxes
 const std::size_t p = std::max< std::size_t >( ex.concurrency() , 1 );
 auto dist = [ & ]( int v ) {
  return( std::atomic_ref< D >( d[ std::size_t( v ) ] ).load(
                                              std::memory_order_relaxed ) ); };
 while( ! A.empty() ) {
  const D t = theta( A , d );
  // the extracted nodes, no longer active, and the others
  const auto cut = blocksOf( 0 , A.size() , kGrain );
  const std::size_t B = cut.first , size = cut.second;
  F = concatBlocks( ex , B , std::vector< int >() , [ & ]( std::size_t b ) {
   std::vector< int > L;
   const std::size_t i1 = std::min( A.size() , ( b + 1 ) * size );
   for( std::size_t i = b * size ; i < i1 ; ++i )
    if( d[ std::size_t( A[ i ] ) ] <= t ) {
     L.push_back( A[ i ] );
     in[ std::size_t( A[ i ] ) ] = 0;
     }
   return( L ); } );
  R.clear();
  if( F.size() < A.size() )
   R = concatBlocks( ex , B , std::vector< int >() , [ & ]( std::size_t b ) {
    std::vector< int > L;
    const std::size_t i1 = std::min( A.size() , ( b + 1 ) * size );
    for( std::size_t i = b * size ; i < i1 ; ++i )
     if( ! ( d[ std::size_t( A[ i ] ) ] <= t ) ) L.push_back( A[ i ] );
    return( L ); } );
  // the arcs of the extracted nodes, cut by the prefix sums of their
  // degrees into blocks of 256 to kArcs arcs, about 4 per thread
  std::vector< std::size_t > P( F.size() + 1 , 0 );
  parallelFor( ex , 0 , F.size() , [ & ]( std::size_t i ) {
   P[ i ] = arcsOf( g , F[ i ] ); } , kGrain );
  const std::size_t M = parallelExclusiveScan( ex , P.begin() , F.size() ,
                                               P.begin() , std::size_t( 0 ) );
  P[ F.size() ] = M;
  const auto acut = blocksOf( 0 , M , std::clamp( M / ( 4 * p ) ,
                                                  std::size_t( 256 ) ,
                                                  kArcs ) );
  const std::size_t AB = acut.first , asize = acut.second;
  A = concatBlocks( ex , AB , std::move( R ) , [ & ]( std::size_t b ) {
   std::vector< int > L , Q;
   std::size_t fused = 0;                   // the nodes taken from Q
   const std::size_t a0 = b * asize , a1 = std::min( M , a0 + asize );
   // the head u reached at the distance x: active if it improved and was
   // not active yet, relaxed by this block (Q) if x is within the
   // threshold and the block has taken fewer than kFused nodes there,
   // else in the next step (L); the atomics are sequentially consistent,
   // so that a node that this block takes from Q and reads after clearing
   // its flag is made active again by any later improvement
   auto relax = [ & ]( int u , D x ) {
    std::atomic_ref< std::uint8_t > f( in[ std::size_t( u ) ] );
    if( writeMin( d[ std::size_t( u ) ] , x ) && ( f.load() == 0 ) &&
        ( f.exchange( 1 ) == 0 ) )
     ( ( x <= t ) && ( fused + Q.size() < kFused ) ? Q : L ).push_back( u );
    };
   // the first node with an arc in the block
   std::size_t i = std::size_t( std::upper_bound( P.begin() , P.end() , a0 ) -
                                P.begin() ) - 1;
   for( ; ( i < F.size() ) && ( P[ i ] < a1 ) ; ++i ) {
    const int v = F[ i ];
    const D dv = dist( v );
    if constexpr( isStaticGraph< G > ) {
     const auto * arc = g.arcData() + g.offsets()[ v ];
     const std::size_t k0 = std::max( a0 , P[ i ] ) - P[ i ] ,
                       k1 = std::min( a1 , P[ i + 1 ] ) - P[ i ];
     for( std::size_t k = k0 ; k < k1 ; ++k )
      relax( arc[ k ].to , D( dv + arc[ k ].w ) );
     }
    else if( P[ i ] >= a0 )                 // the block of its first arc
     g.forEachEdge( v , [ & ]( int u , const D & w ) {
      relax( u , D( dv + w ) ); } );
    }
   // the nodes within the threshold that this block made active, relaxed
   // at once instead of in a step of their own (the fusion of the
   // buckets of Zhang et al.)
   while( ! Q.empty() ) {
    const int v = Q.back();
    Q.pop_back();
    ++fused;
    std::atomic_ref< std::uint8_t >( in[ std::size_t( v ) ] ).store( 0 );
    const D dv = std::atomic_ref< D >( d[ std::size_t( v ) ] ).load();
    g.forEachEdge( v , [ & ]( int u , const D & w ) {
     relax( u , D( dv + w ) ); } );
    }
   return( L ); } );
  }
 return( d );
 }

/// the barrier of the threads that run the calls of one `bulk()` of an
/// executor, which may join while the computation runs: a thread that
/// joins is counted at the next release, so that the barrier waits only
/// for threads already in it, and never for a call that the executor has
/// not started (it may run the calls one after the other on one thread)
class JoinBarrier {
 public:

 /// joins the computation: true for the first thread, which starts it at
 /// once, and for the others once they are counted, at a release; false
 /// if the computation is over
 bool join( void ) {
  std::uint32_t g;
  {
   std::lock_guard< std::mutex > lock( f_m );
   if( f_done ) return( false );
   if( ! f_started ) {
    f_started = true;
    f_count.store( 1 , std::memory_order_relaxed );
    return( true );
    }
   ++f_pending;
   g = f_gen.load( std::memory_order_relaxed );
   }
  unsigned k = 0;
  while( f_gen.load( std::memory_order_acquire ) == g ) spinPause( k );
  return( ! f_done );
  }

 /// the arrival of a thread: the last one counts the threads that joined,
 /// runs s() and releases the others
 template< class S >
 void arrive( S && s ) {
  const std::uint32_t g = f_gen.load( std::memory_order_acquire );
  if( f_arrived.fetch_add( 1 , std::memory_order_acq_rel ) + 1 ==
      f_count.load( std::memory_order_relaxed ) ) {
   std::lock_guard< std::mutex > lock( f_m );
   f_count.fetch_add( f_pending , std::memory_order_relaxed );
   f_pending = 0;
   s();
   f_arrived.store( 0 , std::memory_order_relaxed );
   f_gen.store( g + 1 , std::memory_order_release );
   return;
   }
  unsigned k = 0;
  while( f_gen.load( std::memory_order_acquire ) == g ) spinPause( k );
  }

 /// the computation ends at the next release (called in s())
 void finish( void ) { f_done = true; }

 /// whether the computation is over
 [[nodiscard]] bool done( void ) const { return( f_done ); }

 private:
 std::mutex f_m;                          ///< for the joins
 /// the releases, on a line of their own, where the threads wait
 alignas( 128 ) std::atomic< std::uint32_t > f_gen{ 0 };
 /// the arrivals and the threads in it, on another line
 alignas( 128 ) std::atomic< std::uint32_t > f_arrived{ 0 } ,
                                             f_count{ 0 };
 std::uint32_t f_pending = 0;             ///< those joining
 bool f_started = false ,                 ///< whether a thread started
      f_done = false;                     ///< whether it is over
 };

/// the Delta-stepping on thread-local buckets: each thread of the bulk
/// keeps the nodes it improves in its own buckets, a window of kWindow
/// buckets from base and an overflow past it; a step relaxes the shared
/// frontier of the bucket cur, the threads claiming 64 nodes at a time,
/// then each thread relaxes at once its own bucket cur while it holds
/// fewer than kFuse nodes, and votes the least bucket it holds; after the
/// barrier each thread copies its part of that bucket into the frontier,
/// at a place of its own, and a second barrier ends the step (Beamer and
/// Zhang in the GAP benchmark suite, with the bucketing of Julienne)
template< Executor E , Graph G >
std::vector< typename G::Weight > bucketStepping( E & ex , const G & g ,
                                                  int s ,
                                                  typename G::Weight delta )
{
 using D = typename G::Weight;
 constexpr std::size_t kWindow = 1024 , kFuse = 1000 , kChunk = 64;
 constexpr std::size_t kNone = std::numeric_limits< std::size_t >::max();
 const int n = g.n();
 std::vector< D > d( std::size_t( n ) , infinity< D >() );
 d[ std::size_t( s ) ] = D( 0 );
 auto bucket = [ delta ]( D x ) -> std::size_t {
  if constexpr( std::is_integral_v< D > )
   return( std::size_t( x / delta ) );
  else {
   const D b = std::floor( x / delta );
   return( b < D( kNone / 4 ) ? std::size_t( b ) : kNone / 4 );
   }
  };
 // the buckets of a thread
 struct Buckets {
  std::vector< std::vector< int > > win =
   std::vector< std::vector< int > >( kWindow );
  std::vector< int > over;               ///< the nodes past the window
  std::size_t overMin = kNone;           ///< a bound below their buckets
  };
 const std::size_t p = std::max< std::size_t >( ex.concurrency() , 1 );
 std::vector< Buckets * > slot( p , nullptr );
 std::vector< std::size_t > at( p + 1 , 0 );   // the places in the frontier
 std::vector< int > F{ s };                // the frontier of the bucket cur
 std::size_t tail = 1 , cur = 0 , base = 0;
 // the counters that the threads update, each on a line of its own,
 // away from the state that they read
 struct alignas( 128 ) Counter {
  std::atomic< std::size_t > v;           ///< the value
  };
 Counter claim{ 0 } , vote{ kNone } , nRelaxed{ 0 } , nAgain{ 0 };
 // whether the distance at which each node was last relaxed is kept, so
 // that an entry of a node met again at the same distance relaxes nothing:
 // from the step where the entries put back in the bucket of the step
 // outnumber a quarter of those relaxed (a bucket wider than the least
 // costs, where a node improves many times), and then for good
 const std::size_t from = keepFrom.load( std::memory_order_relaxed );
 bool keep = from == 0;
 std::vector< D > last;                    // made when needed
 if( keep ) last.assign( std::size_t( n ) , infinity< D >() );
 enum class Phase { RELAX , COPY , SPREAD };
 Phase phase = Phase::RELAX;
 JoinBarrier bar;
 auto dist = [ & ]( int v ) {
  return( std::atomic_ref< D >( d[ std::size_t( v ) ] ).load(
                                              std::memory_order_relaxed ) ); };
 // after the votes: the next bucket, the end, a move of the window, or
 // the places of the threads in the frontier
 auto next = [ & ]() {
  const std::size_t b = vote.v.exchange( kNone , std::memory_order_relaxed );
  // the distances kept from the next step on, if the entries put back in
  // the bucket of this one were more than a quarter of those relaxed (and
  // some thousands)
  const std::size_t r = nRelaxed.v.exchange( 0 , std::memory_order_relaxed ) ,
                    k = nAgain.v.exchange( 0 , std::memory_order_relaxed );
  if( ! keep && ( r + k >= from ) && ( 4 * k > r ) ) {
   keep = true;
   last.assign( std::size_t( n ) , infinity< D >() );
   }
  if( b == kNone ) {
   bar.finish();
   return;
   }
  if( b >= base + kWindow ) {
   base = b;
   phase = Phase::SPREAD;
   return;
   }
  cur = b;
  at[ 0 ] = 0;
  for( std::size_t k = 0 ; k < p ; ++k )
   at[ k + 1 ] = at[ k ] + ( slot[ k ] ? slot[ k ]->win[ b % kWindow ].size()
                                       : 0 );
  tail = at[ p ];
  if( F.size() < tail ) F.resize( tail );
  phase = Phase::COPY;
  };
 ex.bulk( p , [ & ]( std::size_t me ) {
  if( ! bar.join() ) return;
  Buckets B;
  slot[ me ] = &B;                         // read only at the barriers
  std::size_t relaxed = 0 , again = 0;     // the entries of a step
  auto push = [ & ]( int u , std::size_t b ) {
   if( b < base + kWindow ) B.win[ b % kWindow ].push_back( u );
   else {
    B.over.push_back( u );
    B.overMin = std::min( B.overMin , b );
    }
   };
  auto arc = [ & ]( int u , D x ) {
   if( writeMin( d[ std::size_t( u ) ] , x ) ) {
    const std::size_t b = std::max( bucket( x ) , cur );
    again += b == cur;
    push( u , b );
    }
   };
  auto relax = [ & ]( int v ) {
   const D dv = dist( v );
   if( keep ) {
    // relaxed at this distance already (by this thread or by another,
    // which relaxes the same arcs): nothing to do
    std::atomic_ref< D > lv( last[ std::size_t( v ) ] );
    if( lv.load( std::memory_order_relaxed ) == dv ) return;
    lv.store( dv , std::memory_order_relaxed );
    }
   ++relaxed;
   if constexpr( isStaticGraph< G > ) {
    const auto * a = g.arcData() + g.offsets()[ v ];
    const auto * e = g.arcData() + g.offsets()[ v + 1 ];
    for( ; a != e ; ++a ) arc( a->to , D( dv + a->w ) );
    }
   else g.forEachEdge( v , [ & ]( int u , const D & w ) {
    arc( u , D( dv + w ) ); } );
   };
  // whether a distance is below the bucket cur, by a product for the
  // integers (the least distance of cur, which fits since the bucket of a
  // distance does)
  auto below = [ & ]( D x ) {
   if constexpr( std::is_integral_v< D > )
    return( x < D( cur ) * delta );
   else return( bucket( x ) < cur );
   };
  // the least bucket from cur that this thread holds
  auto myVote = [ & ]() {
   std::size_t b = B.overMin;
   for( std::size_t i = std::max( cur , base ) ; i < base + kWindow ; ++i )
    if( ! B.win[ i % kWindow ].empty() ) {
     b = std::min( b , i );
     break;
     }
   std::size_t v = vote.v.load( std::memory_order_relaxed );
   while( ( b < v ) && ! vote.v.compare_exchange_weak(
                          v , b , std::memory_order_relaxed ) ) {}
   };
  std::vector< int > tmp;
  for( ;; ) {
   if( phase == Phase::RELAX ) {
    for( ;; ) {
     const std::size_t i0 = claim.v.fetch_add( kChunk ,
                                             std::memory_order_relaxed );
     if( i0 >= tail ) break;
     const std::size_t i1 = std::min( tail , i0 + kChunk );
     for( std::size_t i = i0 ; i < i1 ; ++i ) {
      const int v = F[ i ];
      if( ! below( dist( v ) ) ) relax( v );   // else relaxed earlier
      }
     }
    // the fusion: the own bucket cur while it is small
    auto & mine = B.win[ cur % kWindow ];
    while( ! mine.empty() && ( mine.size() < kFuse ) ) {
     tmp.swap( mine );
     mine.clear();
     for( const int v : tmp ) relax( v );
     tmp.clear();
     }
    myVote();
    nRelaxed.v.fetch_add( relaxed , std::memory_order_relaxed );
    nAgain.v.fetch_add( again , std::memory_order_relaxed );
    relaxed = again = 0;
    bar.arrive( next );
    }
   else if( phase == Phase::COPY ) {
    auto & mine = B.win[ cur % kWindow ];
    std::copy( mine.begin() , mine.end() ,
               F.begin() + std::ptrdiff_t( at[ me ] ) );
    mine.clear();
    bar.arrive( [ & ]() {
     claim.v.store( 0 , std::memory_order_relaxed );
     phase = Phase::RELAX; } );
    }
   else {
    // the overflow spread over the new window, the nodes whose distance
    // fell below it dropped (relaxed when it fell)
    std::vector< int > rest;
    B.overMin = kNone;
    for( const int v : B.over ) {
     const std::size_t b = bucket( dist( v ) );
     if( b < base ) continue;
     if( b < base + kWindow ) B.win[ b % kWindow ].push_back( v );
     else {
      rest.push_back( v );
      B.overMin = std::min( B.overMin , b );
      }
     }
    B.over.swap( rest );
    myVote();
    bar.arrive( next );
    }
   if( bar.done() ) break;
   }
  } );
 return( d );
 }

/// the costs of g are not negative, and d is a fixed point of the
/// relaxation with d[ s ] = 0
template< Graph G >
bool steppingFixedPoint( const G & g , int s ,
                         const std::vector< typename G::Weight > & d )
{
 using D = typename G::Weight;
 bool ok = d[ std::size_t( s ) ] == D( 0 );
 for( int v = 0 ; v < g.n() ; ++v )
  g.forEachEdge( v , [ & ]( int u , const D & w ) {
   ok = ok && ( w >= D( 0 ) );
   if( d[ std::size_t( v ) ] < infinity< D >() )
    ok = ok && ( d[ std::size_t( u ) ] <= D( d[ std::size_t( v ) ] + w ) );
   } );
 return( ok );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , Graph G >
std::vector< typename G::Weight > deltaStepping( E & ex , const G & g , int s ,
                                                 typename G::Weight delta )
{
 using D = typename G::Weight;
 ADS_PRE( g.hasNode( s ) );
 ADS_PRE( delta > D( 0 ) );
 auto d = detail::bucketStepping( ex , g , s , delta );
 ADS_AUDIT( detail::steppingFixedPoint( g , s , d ) );
 return( d );
 }

template< Executor E , Graph G >
std::vector< typename G::Weight > rhoStepping( E & ex , const G & g , int s ,
                                               std::size_t rho )
{
 using D = typename G::Weight;
 ADS_PRE( g.hasNode( s ) );
 ADS_PRE( rho >= 1 );
 auto d = detail::stepping( ex , g , s , [ & ]( const std::vector< int > & A ,
                                                const std::vector< D > & d ) {
  if( A.size() <= rho )
   return( infinity< D >() );
  // the quantile rho / |A| of the distances of 1024 nodes evenly spaced
  // in A
  constexpr std::size_t kSample = 1024;
  std::vector< D > x( kSample );
  for( std::size_t i = 0 ; i < kSample ; ++i )
   x[ i ] = d[ std::size_t( A[ i * A.size() / kSample ] ) ];
  const std::size_t k = std::min( kSample , std::max< std::size_t >( 1 ,
                         ( kSample * rho + A.size() - 1 ) / A.size() ) ) - 1;
  std::nth_element( x.begin() , x.begin() + std::ptrdiff_t( k ) , x.end() );
  return( x[ k ] ); } );
 ADS_AUDIT( detail::steppingFixedPoint( g , s , d ) );
 return( d );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File ParallelShortestPaths.tpp -------------------*/
/*--------------------------------------------------------------------------*/
