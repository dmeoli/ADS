/*--------------------------------------------------------------------------*/
/*------------------------- File ShortestPaths.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths of ShortestPaths.h [see ShortestPaths.h], with their
 * contracts.
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
/*------------------------------ PROTOTYPE ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G , class S >
requires PrototypeFrontier< S , typename G::Weight >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                     S & frontier , FrontierCounts * c )
{
 constexpr bool fifo = requires { requires S::fifo; };
 ADS_PRE( g.hasNode( r ) && frontier.empty() );
 ADS_AUDIT( fifo || ! reachesNegativeCycle( g , r ) );
 using D = typename G::Weight;
 const D INF = infinity< D >();
 const int n = g.bound();
 ShortestPathTree< D > R;
 R.d.assign( n , INF );
 R.T.assign( n , NIL );
 std::vector< bool > b( n , false );
 std::vector< int > count( fifo ? n : 0 , 0 );
 FrontierCounts k;

 R.d[ r ] = D( 0 );
 b[ r ] = true;
 frontier.add( r , D( 0 ) , true );                        // (1)
 ++k.adds;
 while( ! frontier.empty() ) {
  const int u = frontier.extract();                        // (2)
  ++k.extracts;
  b[ u ] = false;
  R.order.push_back( u );
  if constexpr( fifo )
   if( ++count[ u ] > n ) {
    // a node is extracted at most n times by a queue, unless there is a
    // negative cycle; the other frontiers have no such bound
    R.negativeCycle = true;
    if( c ) *c = k;
    return( R );
    }
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   ++k.edges;
   const D nd = R.d[ u ] + w;
   if( nd < R.d[ v ] ) {
    const bool first = ( R.d[ v ] == INF );
    R.d[ v ] = nd;
    R.T[ v ] = u;
    if( ! b[ v ] ) {
     frontier.add( v , nd , first );                       // (3)
     ++k.adds;
     b[ v ] = true;
     }
    else {
     frontier.update( v , nd );                            // (4)
     ++k.decreases;
     }
    }
   } );
  }
 if( c ) *c = k;
 ADS_AUDIT( isShortestPathTree( g , r , R ) );
 return( R );
 }

template< Graph G >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                                      Frontier how )
{
 ADS_PRE( g.hasNode( r ) );
 using D = typename G::Weight;
 const int n = g.bound();
 switch( how ) {
  case( Frontier::LIST ): {
   ListPriorityQueue< int , D , false > Q;
   PriorityFrontier S( Q , n );
   return( shortestPaths( g , r , S ) );
   }
  case( Frontier::HEAP ): {
   HeapPriorityQueue< int , D > Q( std::max( n , 1 ) );
   PriorityFrontier S( Q , n );
   return( shortestPaths( g , r , S ) );
   }
  case( Frontier::FIBONACCI ): {
   FibonacciHeapPriorityQueue< int , D > Q;
   PriorityFrontier S( Q , n );
   return( shortestPaths( g , r , S ) );
   }
  case( Frontier::QUEUE ): {
   ArrayQueue< int > Q( std::max( n , 1 ) );
   QueueFrontier S( Q );
   return( shortestPaths( g , r , S ) );
   }
  case( Frontier::STACK ): {
   ArrayStack< int > Q( std::max( n , 1 ) );
   StackFrontier S( Q );
   return( shortestPaths( g , r , S ) );
   }
  default: {
   Dequeue< int > Q;
   DequeFrontier S( Q );
   return( shortestPaths( g , r , S ) );
   }
  }
 }

template< Graph G >
auto dijkstra( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::LIST ) );
 }

template< Graph G >
auto johnson( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::HEAP ) );
 }

template< Graph G >
auto fredmanTarjan( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::FIBONACCI ) );
 }

template< Graph G >
auto bellmanFordMoore( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::QUEUE ) );
 }

template< Graph G >
auto papeDEsopo( const G & g , int r )
{
 return( shortestPaths( g , r , Frontier::DEQUE ) );
 }

template< Graph G >
ShortestPathTree< typename G::Weight > dagShortestPaths( const G & g ,
                                                         int r )
{
 ADS_PRE( g.hasNode( r ) );
 ADS_AUDIT( ! hasCycle( g ) );
 using D = typename G::Weight;
 const D INF = infinity< D >();
 ShortestPathTree< D > R;
 R.d.assign( g.bound() , INF );
 R.T.assign( g.bound() , NIL );
 R.d[ r ] = D( 0 );
 for( int u : topsort( g ) ) {
  if( R.d[ u ] == INF )
   continue;
  R.order.push_back( u );
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   if( R.d[ u ] + w < R.d[ v ] ) {
    R.d[ v ] = R.d[ u ] + w;
    R.T[ v ] = u;
    }
   } );
  }
 ADS_AUDIT( isShortestPathTree( g , r , R ) );
 return( R );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- CHECKS -----------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
bool isShortestPathTree( const G & g , int r ,
                         const ShortestPathTree< typename G::Weight > & R )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 if( ( int( R.d.size() ) != g.bound() ) || ( int( R.T.size() ) != g.bound() )
     || ( R.d[ r ] != D( 0 ) ) || ( R.T[ r ] != NIL ) )
  return( false );
 bool ok = true;
 for( int v : g.V() ) {
  if( v == r )
   continue;
  if( R.d[ v ] == INF ) {
   ok = ok && ( R.T[ v ] == NIL );
   continue;
   }
  const int u = R.T[ v ];
  if( ! g.hasNode( u ) || ( R.d[ u ] == INF ) )
   return( false );
  bool found = false;                    // d[ v ] = d[ u ] + w( u , v )
  g.forEachEdge( u , [ & ]( int x , const D & w ) {
   if( ( x == v ) && ( R.d[ u ] + w == R.d[ v ] ) )
    found = true;
   } );
  ok = ok && found;
  }
 for( int u : g.V() )                    // no edge violates the condition
  if( R.d[ u ] != INF )
   g.forEachEdge( u , [ & ]( int v , const D & w ) {
    ok = ok && !( R.d[ u ] + w < R.d[ v ] );
    } );
 return( ok );
 }

template< Graph G >
bool reachesNegativeCycle( const G & g , int r )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 std::vector< D > d( g.bound() , INF );
 d[ r ] = D( 0 );
 auto relax = [ & ]( void ) {
  bool changed = false;
  for( int u : g.V() )
   if( d[ u ] != INF )
    g.forEachEdge( u , [ & ]( int v , const D & w ) {
     if( d[ u ] + w < d[ v ] ) {
      d[ v ] = d[ u ] + w;
      changed = true;
      }
     } );
  return( changed );
  };
 for( int k = 1 ; k < g.n() ; ++k )
  if( ! relax() )
   return( false );
 return( relax() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ShortestPaths.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
