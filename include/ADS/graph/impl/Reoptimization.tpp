/*--------------------------------------------------------------------------*/
/*------------------------ File Reoptimization.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The observers of Reoptimization.h: the tests of the certificates, the
 * repairs and the computations from scratch.
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

#include "ADS/mfset/UnionFind.h"

#include <algorithm>
#include <functional>
#include <queue>

namespace ADS {

namespace detail {

/// the change of the edge of an event: its ends, the weight before and
/// after (nothing if absent); false for an event on a node
template< class W >
bool edgeChange( const GraphEvent< W > & e , int & u , int & v ,
                 std::optional< W > & before , std::optional< W > & after )
{
 before.reset();
 after.reset();
 if( const auto * x = std::get_if< EdgeInserted< W > >( & e ) ) {
  u = x->u; v = x->v; after = x->w;
  return( true );
  }
 if( const auto * x = std::get_if< EdgeDeleted< W > >( & e ) ) {
  u = x->u; v = x->v; before = x->w;
  return( true );
  }
 if( const auto * x = std::get_if< WeightChanged< W > >( & e ) ) {
  u = x->u; v = x->v; before = x->from; after = x->to;
  return( true );
  }
 return( false );
 }

/// the number of edges of g
template< class G >
long long edgesOf( const G & g )
{
 if constexpr( requires { g.m(); } )
  return( g.m() );
 else {
  long long m = 0;
  for( int u : g.V() )
   g.forEachAdj( u , [ & ]( int ) { ++m; } );
  return( g.directed() ? m : m / 2 );
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS ShortestPathsObserver -----------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
ShortestPathsObserver< G >::ShortestPathsObserver( const G & g , int r ,
                                                   Frontier how )
 : f_g( g ) , f_r( r ) , f_how( how )
{
 ADS_PRE( g.hasNode( r ) );
 }

template< Graph G >
void ShortestPathsObserver< G >::scratch( Frontier how )
{
 f_R = shortestPaths( f_g , f_r , how );
 f_ready = true;
 f_inOk = false;                          // the events not applied to it
 buildTree();
 }

template< Graph G >
void ShortestPathsObserver< G >::buildTree( void )
{
 const int n = int( f_R.T.size() );
 f_child.assign( n , NIL );
 f_next.assign( n , NIL );
 f_prev.assign( n , NIL );
 f_pops.assign( n , 0 );
 f_inA.assign( n , 0 );
 for( int v = 0 ; v < n ; ++v )
  if( f_R.T[ v ] != NIL )
   link( v , f_R.T[ v ] );
 }

template< Graph G >
void ShortestPathsObserver< G >::grow( void )
{
 const std::size_t n = std::size_t( f_g.bound() );
 if( f_R.d.size() >= n )
  return;
 f_R.d.resize( n , infinity< Weight >() );
 f_R.T.resize( n , NIL );
 f_child.resize( n , NIL );
 f_next.resize( n , NIL );
 f_prev.resize( n , NIL );
 f_pops.resize( n , 0 );
 f_inA.resize( n , 0 );
 if( f_inOk )
  f_in.resize( n );
 }

template< Graph G >
void ShortestPathsObserver< G >::link( int x , int p )
{
 f_prev[ x ] = NIL;
 f_next[ x ] = f_child[ p ];
 if( f_child[ p ] != NIL )
  f_prev[ f_child[ p ] ] = x;
 f_child[ p ] = x;
 }

template< Graph G >
void ShortestPathsObserver< G >::unlink( int x )
{
 const int p = f_R.T[ x ];
 if( p == NIL )
  return;
 if( f_prev[ x ] != NIL )
  f_next[ f_prev[ x ] ] = f_next[ x ];
 else
  f_child[ p ] = f_next[ x ];
 if( f_next[ x ] != NIL )
  f_prev[ f_next[ x ] ] = f_prev[ x ];
 f_prev[ x ] = f_next[ x ] = NIL;
 }

template< Graph G >
void ShortestPathsObserver< G >::buildIn( void )
{
 f_in.assign( std::size_t( f_g.bound() ) , {} );
 for( int u : f_g.V() )
  f_g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
   f_in[ v ].emplace_back( u , w ); } );
 f_inOk = true;
 }

template< Graph G >
void ShortestPathsObserver< G >::applyIn( std::span< const Event > P )
{
 for( const auto & e : P ) {
  int u , v;
  std::optional< Weight > w0 , w1;
  if( ! detail::edgeChange( e , u , v , w0 , w1 ) )
   continue;
  auto & L = f_in[ v ];
  auto it = std::find_if( L.begin() , L.end() , [ & ]( const auto & x ) {
   return( x.first == u ); } );
  if( ! w1 ) {                            // deleted
   if( it != L.end() ) {
    *it = L.back();
    L.pop_back();
    }
   }
  else if( it != L.end() )
   it->second = *w1;
  else
   L.emplace_back( u , *w1 );
  }
 }

template< Graph G >
template< class F >
void ShortestPathsObserver< G >::forEachIn( int x , F && f ) const
{
 if( f_g.directed() ) {
  for( const auto & [ u , w ] : f_in[ x ] )
   f( u , w );
  }
 else if( f_g.hasNode( x ) )              // the edges are symmetric
  f_g.forEachEdge( x , [ & ]( int u , const Weight & w ) { f( u , w ); } );
 }

template< Graph G >
void ShortestPathsObserver< G >::setParent( int v , int u )
{
 unlink( v );
 f_R.T[ v ] = u;
 if( u != NIL )
  link( v , u );
 }

template< Graph G >
bool ShortestPathsObserver< G >::repair( std::vector< int > & seeds )
{
 using D = Weight;
 const D INF = infinity< D >();
 const int n = f_g.bound();
 using Item = std::pair< D , int >;
 std::priority_queue< Item , std::vector< Item > , std::greater< Item > > H;
 for( int u : seeds )
  if( f_R.d[ u ] != INF )
   H.push( { f_R.d[ u ] , u } );
 std::vector< int > & popped = seeds;     // the nodes whose count to reset
 popped.clear();
 bool ok = true;
 while( ! H.empty() ) {
  const auto [ du , u ] = H.top();
  H.pop();
  if( du != f_R.d[ u ] )
   continue;                              // a stale entry
  if( f_pops[ u ]++ == 0 )
   popped.push_back( u );
  if( f_pops[ u ] > n ) {
   ok = false;
   break;
   }
  f_g.forEachEdge( u , [ & ]( int v , const D & w ) {
   if( f_R.d[ u ] + w < f_R.d[ v ] ) {
    f_R.d[ v ] = f_R.d[ u ] + w;
    setParent( v , u );
    H.push( { f_R.d[ v ] , v } );
    } } );
  }
 for( int u : popped )
  f_pops[ u ] = 0;
 return( ok );
 }

template< Graph G >
void ShortestPathsObserver< G >::solve( void )
{
 ADS_PRE( f_g.hasNode( f_r ) );
 if( ! f_ready ) {
  this->f_q.clear();
  scratch( f_how );
  this->done( Reopt::SCRATCH );
  return;
  }
 if( this->f_q.empty() )
  return;                                 // nothing to do
 const auto P = this->take();
 if( this->tooMany( P.size() , detail::edgesOf( f_g ) ) ||
     f_R.negativeCycle ) {
  scratch( f_R.negativeCycle ? Frontier::QUEUE : f_how );
  this->done( Reopt::SCRATCH );
  return;
  }
 grow();
 if( f_inOk )
  applyIn( P );

 // the tests on the tree before P: the heads of the arcs of T raised or
 // deleted root the subtrees whose distances are lost; the tails of the
 // arcs that got lighter are the seeds of the decreases
 const Weight INF = infinity< Weight >();
 std::vector< int > roots;
 struct Arc {
  int a , b;
  Weight w;
  };
 std::vector< Arc > lighter;
 auto arc = [ & ]( int a , int b , const std::optional< Weight > & w0 ,
                   const std::optional< Weight > & w1 ) {
  if( ( f_R.d[ b ] != INF ) && ( f_R.T[ b ] == a ) &&
      ( ! w1 || ( *w0 < *w1 ) ) )
   roots.push_back( b );                  // d may grow under b
  else if( w1 && ( ! w0 || ( *w1 < *w0 ) ) )
   lighter.push_back( { a , b , *w1 } );
  };
 for( const auto & e : P ) {
  int u , v;
  std::optional< Weight > w0 , w1;
  if( detail::edgeChange( e , u , v , w0 , w1 ) ) {
   arc( u , v , w0 , w1 );
   if( ! f_g.directed() )
    arc( v , u , w0 , w1 );
   }
  }

 // the subtrees under the roots, taken out of T
 std::vector< int > A;
 for( int b : roots ) {
  if( f_inA[ b ] )
   continue;
  unlink( b );
  f_inA[ b ] = 1;
  const std::size_t first = A.size();
  A.push_back( b );
  for( std::size_t i = first ; i < A.size() ; ++i )
   for( int c = f_child[ A[ i ] ] ; c != NIL ; c = f_next[ c ] )
    if( ! f_inA[ c ] ) {
     f_inA[ c ] = 1;
     A.push_back( c );
     }
  }
 if( 2 * A.size() > std::size_t( f_g.n() ) ) {
  for( int x : A )                        // most of the tree is lost
   f_inA[ x ] = 0;
  scratch( f_how );
  this->done( Reopt::SCRATCH );
  return;
  }
 for( int x : A ) {
  f_inA[ x ] = 0;
  f_R.d[ x ] = INF;
  f_R.T[ x ] = NIL;
  f_child[ x ] = f_next[ x ] = f_prev[ x ] = NIL;
  }

 // each node of A from its best entering arc out of A, then the seeds
 if( ! A.empty() && f_g.directed() && ! f_inOk )
  buildIn();
 std::vector< int > seeds;
 for( int x : A ) {
  int best = NIL;
  forEachIn( x , [ & ]( int u , const Weight & w ) {
   if( ( f_R.d[ u ] != INF ) && ( f_R.d[ u ] + w < f_R.d[ x ] ) ) {
    f_R.d[ x ] = f_R.d[ u ] + w;
    best = u;
    } } );
  if( best != NIL ) {
   setParent( x , best );
   seeds.push_back( x );
   }
  }
 for( const auto & c : lighter )
  if( ( f_R.d[ c.a ] != INF ) && ( f_R.d[ c.a ] + c.w < f_R.d[ c.b ] ) )
   seeds.push_back( c.a );
 for( const auto & e : P )                // the nodes gone, unreached
  if( const auto * x = std::get_if< NodeDeleted >( & e ) ) {
   setParent( x->u , NIL );
   f_R.d[ x->u ] = INF;
   }

 if( A.empty() && seeds.empty() )
  this->done( Reopt::CERTIFIED );
 else {
  f_R.order.clear();
  if( repair( seeds ) )
   this->done( Reopt::WARM );
  else {                                  // a negative cycle
   scratch( Frontier::QUEUE );
   this->done( Reopt::SCRATCH );
   }
  }
 ADS_AUDIT( f_R.negativeCycle || isShortestPathTree( f_g , f_r , f_R ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SpanningForestObserver ----------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
SpanningForestObserver< G >::SpanningForestObserver( const G & g )
 : f_g( g )
{
 ADS_PRE( ! g.directed() );
 }

template< Graph G >
void SpanningForestObserver< G >::setForest(
                         const std::vector< WeightedEdge< Weight > > & F )
{
 for( const std::uint64_t k : f_F ) {
  const auto it = f_E.find( k );
  if( it != f_E.end() )
   it->second.tree = false;
  }
 f_F.clear();
 for( const auto & e : F ) {
  f_F.push_back( key( e.u , e.v ) );
  f_E.at( f_F.back() ).tree = true;
  }
 f_shapeOk = f_liftOk = f_replOk = false;
 }

template< Graph G >
void SpanningForestObserver< G >::scratch( bool fromGraph )
{
 std::vector< WeightedEdge< Weight > > A;
 if( fromGraph ) {
  f_n = f_g.bound();
  f_E.clear();
  for( int u : f_g.V() )
   f_g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
    if( u < v ) {
     f_E[ key( u , v ) ] = { w , false };
     A.push_back( { u , v , w } );
     } } );
  }
 else {
  A.reserve( f_E.size() );
  for( const auto & [ k , r ] : f_E ) {
   const auto [ u , v ] = ends( k );
   A.push_back( { u , v , r.w } );
   }
  }
 setForest( kruskal( f_n , std::move( A ) ) );
 f_ready = true;
 }

template< Graph G >
void SpanningForestObserver< G >::buildLift( void )
{
 const int n = f_n;
 std::vector< std::vector< std::pair< int , Weight > > > adj( n );
 for( const std::uint64_t k : f_F ) {
  const auto [ u , v ] = ends( k );
  const Weight & w = f_E.at( k ).w;
  adj[ u ].emplace_back( v , w );
  adj[ v ].emplace_back( u , w );
  }
 f_log = 1;
 while( ( 1 << f_log ) < std::max( n , 1 ) )
  ++f_log;
 f_depth.assign( n , 0 );
 f_tree.assign( n , -1 );
 f_up.assign( f_log + 1 , std::vector< int >( n , -1 ) );
 f_mx.assign( f_log + 1 , std::vector< Weight >( n , Weight() ) );
 f_tin.assign( n , 0 );
 f_size.assign( n , 1 );
 std::vector< int > stack , order;
 order.reserve( n );
 for( int r = 0 ; r < n ; ++r ) {
  if( f_tree[ r ] >= 0 )
   continue;
  f_tree[ r ] = r;
  f_up[ 0 ][ r ] = r;
  stack.assign( 1 , r );
  while( ! stack.empty() ) {              // a visit in depth, iterative:
   const int u = stack.back();            // the preorder, each subtree
   stack.pop_back();                      // a run of it
   f_tin[ u ] = int( order.size() );
   order.push_back( u );
   for( const auto & [ v , w ] : adj[ u ] )
    if( f_tree[ v ] < 0 ) {
     f_tree[ v ] = r;
     f_depth[ v ] = f_depth[ u ] + 1;
     f_up[ 0 ][ v ] = u;
     f_mx[ 0 ][ v ] = w;
     stack.push_back( v );
     }
   }
  }
 for( std::size_t i = order.size() ; i-- > 0 ; )
  if( f_up[ 0 ][ order[ i ] ] != order[ i ] )
   f_size[ f_up[ 0 ][ order[ i ] ] ] += f_size[ order[ i ] ];
 for( int j = 1 ; j <= f_log ; ++j )
  for( int v = 0 ; v < n ; ++v ) {
   const int a = f_up[ j - 1 ][ v ];
   f_up[ j ][ v ] = f_up[ j - 1 ][ a ];
   f_mx[ j ][ v ] = f_depth[ v ] >= ( 1 << ( j - 1 ) ) ?
                    std::max( f_mx[ j - 1 ][ v ] , f_mx[ j - 1 ][ a ] ) :
                    f_mx[ j - 1 ][ v ];
   }
 f_shapeOk = f_liftOk = true;
 }

template< Graph G >
int SpanningForestObserver< G >::lca( int u , int v ) const
{
 if( f_depth[ u ] < f_depth[ v ] )
  std::swap( u , v );
 for( int j = f_log ; j >= 0 ; --j )
  if( f_depth[ u ] - ( 1 << j ) >= f_depth[ v ] )
   u = f_up[ j ][ u ];
 if( u == v )
  return( u );
 for( int j = f_log ; j >= 0 ; --j )
  if( f_up[ j ][ u ] != f_up[ j ][ v ] ) {
   u = f_up[ j ][ u ];
   v = f_up[ j ][ v ];
   }
 return( f_up[ 0 ][ u ] );
 }

template< Graph G >
std::optional< std::pair< typename G::Weight , std::uint64_t > >
SpanningForestObserver< G >::crossMin( int x ) const
{
 const int n = int( f_tree.size() );
 auto under = [ & ]( int a ) {            // whether a is in the subtree
  return( ( a < n ) && ( f_tree[ a ] == f_tree[ x ] ) &&
          ( f_tin[ a ] >= f_tin[ x ] ) &&
          ( f_tin[ a ] < f_tin[ x ] + f_size[ x ] ) );
  };
 std::optional< std::pair< Weight , std::uint64_t > > best;
 for( const auto & [ k , r ] : f_E )
  if( ! r.tree ) {
   const auto [ a , b ] = ends( k );
   if( ( under( a ) != under( b ) ) && ( ! best || ( r.w < best->first ) ) )
    best = std::pair< Weight , std::uint64_t >( r.w , k );
   }
 return( best );
 }

template< Graph G >
std::optional< typename G::Weight > SpanningForestObserver< G >::maxPath(
                                                     int u , int v ) const
{
 const int n = int( f_tree.size() );
 if( ( u >= n ) || ( v >= n ) || ( f_tree[ u ] != f_tree[ v ] ) )
  return( std::nullopt );
 std::optional< Weight > m;
 auto climb = [ & ]( int x , int d ) {    // d steps up from x
  for( int j = f_log ; j >= 0 ; --j )
   if( d >= ( 1 << j ) ) {
    m = m ? std::max( *m , f_mx[ j ][ x ] ) : f_mx[ j ][ x ];
    x = f_up[ j ][ x ];
    d -= 1 << j;
    }
  };
 const int a = lca( u , v );
 if( f_liftOk ) {
  climb( u , f_depth[ u ] - f_depth[ a ] );
  climb( v , f_depth[ v ] - f_depth[ a ] );
  }
 else                                     // the jumps of the maxima stale:
  for( int x : { u , v } )                // the path, edge by edge
   for( ; f_depth[ x ] > f_depth[ a ] ; x = f_up[ 0 ][ x ] )
    m = m ? std::max( *m , f_mx[ 0 ][ x ] ) : f_mx[ 0 ][ x ];
 return( m );
 }

template< Graph G >
void SpanningForestObserver< G >::buildRepl( void )
{
 if( ! f_shapeOk )
  buildLift();
 const int n = f_n;
 f_repl.assign( n , std::nullopt );
 f_replW.assign( n , Weight() );
 std::vector< std::pair< Weight , std::uint64_t > > out;
 for( const auto & [ k , r ] : f_E )
  if( ! r.tree )
   out.emplace_back( r.w , k );
 std::sort( out.begin() , out.end() );
 // jump[ x ]: the nearest of x and its ancestors whose edge to the parent
 // has no replacement yet (a root counts as one), found by halving
 std::vector< int > jump( n );
 for( int x = 0 ; x < n ; ++x )
  jump[ x ] = x;
 auto find = [ & ]( int x ) {
  while( jump[ x ] != x ) {
   jump[ x ] = jump[ jump[ x ] ];
   x = jump[ x ];
   }
  return( x );
  };
 for( const auto & [ w , k ] : out ) {
  const auto [ u , v ] = ends( k );
  if( f_tree[ u ] != f_tree[ v ] )
   continue;
  const int a = lca( u , v );
  for( int x : { u , v } )
   for( x = find( x ) ; f_depth[ x ] > f_depth[ a ] ; x = find( x ) ) {
    f_repl[ x ] = k;                      // the edge [ x , parent( x ) ]
    f_replW[ x ] = w;
    jump[ x ] = f_up[ 0 ][ x ];
    }
  }
 f_replOk = true;
 }

template< Graph G >
void SpanningForestObserver< G >::solve( void )
{
 if( ! f_ready ) {
  this->f_q.clear();
  scratch( true );
  this->done( Reopt::SCRATCH );
  return;
  }
 if( this->f_q.empty() )
  return;                                 // nothing to do
 const auto P = this->take();
 int outcome = this->tooMany( P.size() , detail::edgesOf( f_g ) ) ?
               Reopt::SCRATCH : Reopt::CERTIFIED;

 // the edges lowered or inserted out of F, those of F raised or deleted,
 // each with its new weight (nothing if deleted), and those of F whose
 // weight changes
 struct Change {
  std::uint64_t k;
  std::optional< Weight > w;
  };
 std::vector< Change > down , up , inF;
 for( const auto & e : P ) {
  int u , v;
  std::optional< Weight > w0 , w1;
  if( ( outcome == Reopt::SCRATCH ) ||
      ! detail::edgeChange( e , u , v , w0 , w1 ) )
   continue;
  const std::uint64_t k = key( u , v );
  const auto it = f_E.find( k );
  const bool tree = ( it != f_E.end() ) && it->second.tree;
  if( tree ) {
   if( ! w1 || ( *w0 < *w1 ) )
    up.push_back( { k , w1 } );          // lowering it is harmless
   if( w1 )
    inF.push_back( { k , w1 } );
   }
  else if( w1 && ( ! w0 || ( *w1 < *w0 ) ) )
   down.push_back( { k , w1 } );
  }

 // the tests, on the data of the certificate before P
 std::vector< std::uint64_t > failed;
 std::uint64_t swapIn = 0;
 if( outcome == Reopt::SCRATCH )
  ;
 else if( ! down.empty() && ! up.empty() )
  outcome = Reopt::SCRATCH;               // their conditions interact
 else if( ! down.empty() ) {
  if( ! f_shapeOk )
   buildLift();
  for( const auto & c : down ) {
   const auto [ u , v ] = ends( c.k );
   const auto m = maxPath( u , v );
   if( ! m || ( *c.w < *m ) )
    outcome = Reopt::WARM;
   }
  }
 else if( ! up.empty() ) {
  // the replacement of each edge raised: from the table, or, if it is
  // stale and the edges are few, by a scan of the edges across its cut
  const bool scan = ! f_replOk && ( up.size() <= 2 );
  if( scan && ! f_shapeOk )
   buildLift();
  else if( ! scan && ! f_replOk )
   buildRepl();
  std::optional< std::pair< Weight , std::uint64_t > > rho;
  for( const auto & c : up ) {
   const auto [ u , v ] = ends( c.k );
   const int x = f_up[ 0 ][ u ] == v ? u : v;   // the child
   std::optional< std::pair< Weight , std::uint64_t > > r;
   if( scan )
    r = crossMin( x );
   else if( f_repl[ x ] )
    r = std::pair< Weight , std::uint64_t >( f_replW[ x ] , *f_repl[ x ] );
   if( r && ( ! c.w || ( r->first < *c.w ) ) ) {
    failed.push_back( c.k );
    rho = r;
    }
   }
  if( ( failed.size() > 1 ) || ( ! failed.empty() && ( up.size() > 1 ) ) )
   outcome = Reopt::SCRATCH;              // the swaps would interact
  else if( failed.size() == 1 ) {
   // the swap with the replacement, if it is as the certificate saw it
   swapIn = rho->second;
   const auto it = f_E.find( swapIn );
   bool same = it != f_E.end();
   for( const auto & e : P ) {
    const auto [ a , b ] = endsOf( e );
    if( ( a >= 0 ) && ( key( a , b ) == swapIn ) )
     same = false;
    }
   if( same && ! ( it->second.w < rho->first ) &&
       ! ( rho->first < it->second.w ) )
    outcome = Reopt::WARM;
   else
    outcome = Reopt::SCRATCH;
   }
  }

 // the copy of the edges after P
 bool shape = false;                      // whether F loses an edge
 for( const auto & e : P ) {
  int u , v;
  std::optional< Weight > w0 , w1;
  if( ! detail::edgeChange( e , u , v , w0 , w1 ) )
   continue;
  const std::uint64_t k = key( u , v );
  if( ! w1 ) {
   const auto it = f_E.find( k );
   if( ( it != f_E.end() ) && it->second.tree ) {
    shape = true;
    f_F.erase( std::find( f_F.begin() , f_F.end() , k ) );
    }
   f_E.erase( k );
   }
  else {
   auto [ it , fresh ] = f_E.try_emplace( k , Rec{ *w1 , false } );
   if( ! fresh )
    it->second.w = *w1;
   }
  }
 const bool grown = f_g.bound() > f_n;
 f_n = std::max( f_n , f_g.bound() );

 if( outcome == Reopt::SCRATCH ) {
  scratch( false );                       // Kruskal on the copy
  this->done( Reopt::SCRATCH );
  }
 else if( outcome == Reopt::WARM ) {
  if( ! failed.empty() ) {                // the cut condition: a swap
   const auto it = f_E.find( failed[ 0 ] );
   if( it != f_E.end() ) {              // raised, not deleted
    it->second.tree = false;
    f_F.erase( std::find( f_F.begin() , f_F.end() , failed[ 0 ] ) );
    }
   f_E.at( swapIn ).tree = true;
   f_F.push_back( swapIn );
   f_shapeOk = f_liftOk = f_replOk = false;
   }
  else {                                  // Kruskal on F and D
   std::vector< WeightedEdge< Weight > > A;
   for( const std::uint64_t k : f_F ) {
    const auto [ u , v ] = ends( k );
    A.push_back( { u , v , f_E.at( k ).w } );
    }
   for( const auto & c : down ) {
    const auto [ u , v ] = ends( c.k );
    A.push_back( { u , v , f_E.at( c.k ).w } );
    }
   setForest( kruskal( f_n , std::move( A ) ) );
   }
  this->done( Reopt::WARM );
  }
 else {
  // the certificate held: which of its data are still sound
  if( shape || grown )
   f_shapeOk = f_liftOk = f_replOk = false;
  else if( f_shapeOk ) {
   for( const auto & c : inF ) {          // the edges to the parents
    const auto [ u , v ] = ends( c.k );
    f_mx[ 0 ][ f_up[ 0 ][ u ] == v ? u : v ] = *c.w;
    }
   if( ! up.empty() )
    f_liftOk = false;                     // the maxima of the jumps grow
   }
  if( ! down.empty() )
   f_replOk = false;                      // the replacements fall
  this->done( Reopt::CERTIFIED );
  }
 ADS_AUDIT( ( [ & ]( void ) {
  std::vector< WeightedEdge< Weight > > A , F;
  for( const auto & [ k , r ] : f_E ) {
   const auto [ u , v ] = ends( k );
   A.push_back( { u , v , r.w } );
   if( r.tree )
    F.push_back( { u , v , r.w } );
   }
  return( ( A.size() > 2000 ) || isMinimumSpanningForest( f_n , A , F ) );
  }() ) );
 }

template< Graph G >
std::vector< WeightedEdge< typename G::Weight > >
SpanningForestObserver< G >::forest( void )
{
 solve();
 std::vector< WeightedEdge< Weight > > F;
 F.reserve( f_F.size() );
 for( const std::uint64_t k : f_F ) {
  const auto [ u , v ] = ends( k );
  F.push_back( { u , v , f_E.at( k ).w } );
  }
 return( F );
 }

template< Graph G >
typename G::Weight SpanningForestObserver< G >::weight( void )
{
 Weight s = Weight();
 for( const auto & e : forest() )
  s = s + e.w;
 return( s );
 }

template< Graph G >
bool SpanningForestObserver< G >::inForest( int u , int v )
{
 solve();
 const auto it = f_E.find( key( u , v ) );
 return( ( it != f_E.end() ) && it->second.tree );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS MaxFlowObserver --------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
requires std::is_integral_v< typename G::Weight >
MaxFlowObserver< G >::MaxFlowObserver( const G & g , int s , int t ,
                                       Augment how )
 : f_g( g ) , f_s( s ) , f_t( t ) , f_how( how )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) && ( s != t ) );
 }

template< Graph G >
requires std::is_integral_v< typename G::Weight >
void MaxFlowObserver< G >::scratch( void )
{
 f_net = FlowNetwork( f_g.bound() );
 f_arc.clear();
 for( int u : f_g.V() )
  f_g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
   ADS_PRE( w >= 0 );
   if( f_g.directed() || ( u < v ) ) {
    f_arc[ key( u , v ) ] = f_net.addEdge( u , v , w );
    if( ! f_g.directed() )
     f_arc[ key( v , u ) ] = f_net.addEdge( v , u , w );
    } } );
 f_value = f_net.maxFlow( f_s , f_t , f_how );
 f_S = f_net.minCut( f_s );
 f_ready = true;
 }

template< Graph G >
requires std::is_integral_v< typename G::Weight >
long long MaxFlowObserver< G >::cutCapacity( void ) const
{
 long long c = 0;
 for( int u : f_g.V() )
  if( ( u < int( f_S.size() ) ) && f_S[ u ] )
   f_g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
    if( ( v >= int( f_S.size() ) ) || ! f_S[ v ] )
     c += w; } );
 return( c );
 }

template< Graph G >
requires std::is_integral_v< typename G::Weight >
void MaxFlowObserver< G >::solve( void )
{
 ADS_PRE( f_g.hasNode( f_s ) && f_g.hasNode( f_t ) );
 if( ! f_ready ) {
  this->f_q.clear();
  scratch();
  this->done( Reopt::SCRATCH );
  return;
  }
 if( this->f_q.empty() )
  return;                                 // nothing to do
 const auto P = this->take();
 if( this->tooMany( P.size() , detail::edgesOf( f_g ) ) ) {
  scratch();
  this->done( Reopt::SCRATCH );
  return;
  }
 while( f_net.n() < f_g.bound() ) {
  f_net.addNode();
  f_S.push_back( false );
  }

 // the arcs changed: ( u , v , new capacity ), and the tests; a capacity
 // below the flow of its arc leaves an excess and a deficit to route
 struct Arc {
  int u , v;
  long long c;
  };
 std::vector< Arc > A;
 bool warm = false;
 auto test = [ & ]( int a , int b , long long c0 , long long c1 ) {
  ADS_PRE( c1 >= 0 );
  const auto it = f_arc.find( key( a , b ) );
  const long long f = it == f_arc.end() ? 0 : f_net.flow( it->second );
  if( c1 < f )
   warm = true;                           // f no longer feasible
  else if( f_S[ a ] && ! f_S[ b ] && ( c1 > c0 ) )
   warm = true;                           // the cut grows
  A.push_back( { a , b , c1 } );
  };
 for( const auto & e : P ) {
  int u , v;
  std::optional< Weight > w0 , w1;
  if( ! detail::edgeChange( e , u , v , w0 , w1 ) )
   continue;
  const long long c0 = w0 ? *w0 : 0 , c1 = w1 ? *w1 : 0;
  test( u , v , c0 , c1 );
  if( ! f_g.directed() )
   test( v , u , c0 , c1 );
  }
 for( const auto & a : A ) {
  const auto it = f_arc.find( key( a.u , a.v ) );
  if( it == f_arc.end() )
   f_arc[ key( a.u , a.v ) ] = f_net.addEdge( a.u , a.v , a.c );
  else
   f_net.setCapacity( it->second , a.c );
  }
 if( warm ) {                             // from the flow there is
  f_value += f_net.maxFlow( f_s , f_t , f_how );
  f_S = f_net.minCut( f_s );
  this->done( Reopt::WARM );
  }
 else
  this->done( Reopt::CERTIFIED );
 ADS_AUDIT( cutCapacity() == f_value );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File Reoptimization.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
