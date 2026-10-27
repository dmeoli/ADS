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
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) {
  std::copy( L[ b ].begin() , L[ b ].end() ,
             head.begin() + std::ptrdiff_t( at[ b ] ) ); } , 1 );
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
 constexpr std::size_t kGrain = 1024;      // the nodes of a block
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
 auto d = detail::stepping( ex , g , s , [ & ]( const std::vector< int > & A ,
                                                const std::vector< D > & d ) {
  // the least active distance, and the last value of its bucket
  const D mu = parallelReduce( ex , 0 , A.size() , infinity< D >() ,
   [ & ]( std::size_t lo , std::size_t hi ) {
    D m = infinity< D >();
    for( std::size_t i = lo ; i < hi ; ++i )
     m = std::min( m , d[ std::size_t( A[ i ] ) ] );
    return( m ); } ,
   []( D a , D b ) { return( std::min( a , b ) ); } );
  D t;
  if constexpr( std::is_integral_v< D > ) {
   const D r = D( delta - 1 - mu % delta );
   t = mu > std::numeric_limits< D >::max() - r
       ? std::numeric_limits< D >::max() : D( mu + r );
   }
  else
   t = std::max( mu , D( std::floor( mu / delta ) * delta + delta ) );
  return( t ); } );
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
