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
void SpanningForestObserver< G >::grow( int n )
{
 if( n <= f_n )
  return;
 f_inc.resize( n );
 f_adjF.resize( n );
 f_mark.resize( n , 0 );
 for( int v = f_n ; v < n ; ++v )
  f_vid.push_back( f_lct.addNode( Heavy() ) );
 f_n = n;
 }

template< Graph G >
void SpanningForestObserver< G >::addTree( std::uint64_t k )
{
 Rec & r = f_E.at( k );
 const auto [ u , v ] = ends( k );
 int x;
 if( f_free.empty() ) {
  x = f_lct.addNode( Heavy() );
  f_edgeOf.resize( std::size_t( f_lct.n() ) , 0 );
  }
 else {
  x = f_free.back();
  f_free.pop_back();
  }
 f_edgeOf[ x ] = k;
 f_off.erase( std::pair< Weight , std::uint64_t >( r.w , k ) );
 if constexpr( SUBTRACT )
  f_total = f_total + r.w;
 f_lct.setValue( x , Heavy{ true , r.w , x } );
 f_lct.link( x , f_vid[ u ] );
 f_lct.link( f_vid[ v ] , x );
 r.node = x;
 r.at = int( f_F.size() );
 f_F.push_back( k );
 f_adjF[ u ].push_back( v );
 f_adjF[ v ].push_back( u );
 }

template< Graph G >
void SpanningForestObserver< G >::removeTree( std::uint64_t k )
{
 Rec & r = f_E.at( k );
 const auto [ u , v ] = ends( k );
 f_lct.cut( r.node , f_vid[ u ] );
 f_lct.cut( r.node , f_vid[ v ] );
 f_lct.setValue( r.node , Heavy() );
 f_free.push_back( r.node );
 f_off.emplace( r.w , k );
 if constexpr( SUBTRACT )
  f_total = f_total - r.w;
 r.node = NIL;
 f_E.at( f_F.back() ).at = r.at;            // the last one into its place
 f_F[ r.at ] = f_F.back();
 f_F.pop_back();
 r.at = -1;
 for( auto [ a , b ] : { std::pair< int , int >( u , v ) ,
                         std::pair< int , int >( v , u ) } ) {
  auto & L = f_adjF[ a ];
  *std::find( L.begin() , L.end() , b ) = L.back();
  L.pop_back();
  }
 }

template< Graph G >
void SpanningForestObserver< G >::scratch( bool fromGraph )
{
 std::vector< WeightedEdge< Weight > > A;
 if( fromGraph ) {
  f_E.clear();
  f_inc.clear();
  f_adjF.clear();
  f_mark.clear();
  f_vid.clear();
  f_n = 0;
  grow( f_g.bound() );
  for( int u : f_g.V() )
   f_g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
    if( u < v ) {
     f_E[ key( u , v ) ] = Rec{ w , NIL , -1 };
     f_inc[ u ].push_back( v );
     f_inc[ v ].push_back( u );
     } } );
  }
 A.reserve( f_E.size() );
 for( auto & [ k , r ] : f_E ) {
  const auto [ u , v ] = ends( k );
  A.push_back( { u , v , r.w } );
  r.node = NIL;
  r.at = -1;
  }
 // the link-cut tree afresh: the nodes of the graph first, as in f_vid
 f_lct = LinkCutTree< Heavy , Heavier >( f_n , Heavy() );
 for( int v = 0 ; v < f_n ; ++v )
  f_vid[ v ] = v;
 f_edgeOf.assign( std::size_t( f_n ) , 0 );
 f_free.clear();
 f_F.clear();
 f_off.clear();
 f_total = Weight();
 for( const auto & [ k , r ] : f_E )
  f_off.emplace( r.w , k );
 for( auto & L : f_adjF )
  L.clear();
 for( const auto & e : kruskal( f_n , std::move( A ) ) )
  addTree( key( e.u , e.v ) );
 f_ready = true;
 }

template< Graph G >
bool SpanningForestObserver< G >::replacement( int u , int v ,
                                               std::uint64_t & best )
{
 if( ( ++f_stamp == 0 ) || ( 2 * f_stamp < 0 ) ) {   // the stamps wrapped
  std::fill( f_mark.begin() , f_mark.end() , 0 );
  f_stamp = 1;
  }
 const int su = 2 * f_stamp , sv = su + 1;
 // the visits of the two sides, one node of each in turn
 std::vector< int > Q[ 2 ] = { { u } , { v } };
 f_mark[ u ] = su;
 f_mark[ v ] = sv;
 std::size_t head[ 2 ] = { 0 , 0 };
 int side = -1 , turn = 0;
 // then the edges out of the side that ended: the node, the edge of it
 std::size_t xi = 0 , yi = 0;
 bool found = false;
 Weight bw = Weight();
 // the edges off F by increasing weight, the first one across the cut
 auto scan = f_off.begin();
 const int ru = f_lct.findRoot( f_vid[ u ] );
 for( ; ; ) {
  for( int units = 0 ; units < 8 ; ++units ) {
   if( side < 0 ) {                       // a step of the visits
    if( head[ turn ] == Q[ turn ].size() ) {
     side = turn;
     continue;
     }
    const int x = Q[ turn ][ head[ turn ]++ ];
    const int mk = turn ? sv : su;
    for( int y : f_adjF[ x ] )
     if( f_mark[ y ] != mk ) {
      f_mark[ y ] = mk;
      Q[ turn ].push_back( y );
      }
    units += int( f_adjF[ x ].size() );
    turn ^= 1;
    continue;
    }
   const int in = side ? sv : su;         // an edge out of the side
   if( xi == Q[ side ].size() )
    return( found );
   const int x = Q[ side ][ xi ];
   if( yi == f_inc[ x ].size() ) {
    ++xi;
    yi = 0;
    continue;
    }
   const int y = f_inc[ x ][ yi++ ];
   if( f_mark[ y ] != in ) {
    const std::uint64_t k = key( x , y );
    const Weight & w = f_E.at( k ).w;
    if( ! found || ( w < bw ) || ( ! ( bw < w ) && ( k < best ) ) ) {
     found = true;
     bw = w;
     best = k;
     }
    }
   }
  if( scan == f_off.end() )               // no edge across the cut
   return( false );
  const auto [ a , b ] = ends( scan->second );
  if( ( f_lct.findRoot( f_vid[ a ] ) == ru ) !=
      ( f_lct.findRoot( f_vid[ b ] ) == ru ) ) {
   best = scan->second;
   return( true );
   }
  ++scan;
  }
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
 const bool many = this->tooMany( P.size() , detail::edgesOf( f_g ) );
 grow( f_g.bound() );

 // each event in turn: the copy, and the forest unless there are many
 bool changed = false;                    // another edge entered F
 for( const auto & e : P ) {
  int u , v;
  std::optional< Weight > w0 , w1;
  if( ! detail::edgeChange( e , u , v , w0 , w1 ) )
   continue;                              // a node: grow() did it
  const std::uint64_t k = key( u , v );
  auto it = f_E.find( k );
  const bool tree = ( it != f_E.end() ) && ( it->second.node != NIL );
  if( ! w1 ) {                            // deleted
   if( it == f_E.end() )
    continue;
   if( tree && ! many )
    removeTree( k );                      // into f_off, and out again
   f_off.erase( std::pair< Weight , std::uint64_t >( it->second.w , k ) );
   for( auto [ a , b ] : { std::pair< int , int >( u , v ) ,
                           std::pair< int , int >( v , u ) } ) {
    auto & L = f_inc[ a ];
    *std::find( L.begin() , L.end() , b ) = L.back();
    L.pop_back();
    }
   f_E.erase( it );
   if( tree && ! many ) {
    std::uint64_t f = 0;
    if( replacement( u , v , f ) ) {
     addTree( f );
     changed = true;
     }
    }
   continue;
   }
  if( it == f_E.end() ) {                 // inserted
   it = f_E.emplace( k , Rec{ *w1 , NIL , -1 } ).first;
   f_inc[ u ].push_back( v );
   f_inc[ v ].push_back( u );
   if( ! many )
    f_off.emplace( *w1 , k );
   }
  else {
   if constexpr( SUBTRACT )
    if( tree )                            // the total follows the weight
     f_total = f_total - it->second.w + *w1;
   if( ! tree && ! many ) {
    f_off.erase( std::pair< Weight , std::uint64_t >( it->second.w , k ) );
    f_off.emplace( *w1 , k );
    }
   it->second.w = *w1;
   }
  if( many )
   continue;
  if( tree ) {
   if( ! ( *w0 < *w1 ) ) {                // lowered: harmless
    f_lct.setValue( it->second.node ,
                    Heavy{ true , *w1 , it->second.node } );
    continue;
    }
   removeTree( k );                       // raised: its cut decides,
   std::uint64_t f = k;                   // k kept on a tie
   if( replacement( u , v , f ) && ! ( f_E.at( f ).w < *w1 ) )
    f = k;
   addTree( f );
   changed = changed || ( f != k );
   continue;
   }
  if( w0 && ! ( *w1 < *w0 ) )
   continue;                              // raised off F: harmless
  if( ! f_lct.connected( f_vid[ u ] , f_vid[ v ] ) ) {
   addTree( k );
   changed = true;
   continue;
   }
  const Heavy h = f_lct.aggregate( f_vid[ u ] , f_vid[ v ] );
  if( h.has && ( *w1 < h.w ) ) {          // the heaviest of its cycle out
   removeTree( f_edgeOf[ h.node ] );
   addTree( k );
   changed = true;
   }
  }
 if( many ) {
  scratch( false );                       // Kruskal on the copy
  this->done( Reopt::SCRATCH );
  }
 else
  this->done( changed ? Reopt::WARM : Reopt::CERTIFIED );
 ADS_AUDIT( ( [ & ]( void ) {
  std::vector< WeightedEdge< Weight > > A , F;
  for( const auto & [ k , r ] : f_E ) {
   const auto [ u , v ] = ends( k );
   A.push_back( { u , v , r.w } );
   if( r.node != NIL )
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
 if constexpr( SUBTRACT ) {
  solve();
  return( f_total );
  }
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
 return( ( it != f_E.end() ) && ( it->second.node != NIL ) );
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
