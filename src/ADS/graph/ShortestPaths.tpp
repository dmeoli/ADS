/*--------------------------------------------------------------------------*/
/*------------------------- File ShortestPaths.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths of ShortestPaths.h [see ShortestPaths.h], with their
 * contracts.
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ PROTOTYPE ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
ShortestPathTree< typename G::Weight > shortestPaths( const G & g , int r ,
                                                      Frontier how )
{
 ADS_PRE( g.hasNode( r ) );
 ADS_AUDIT( ( how == Frontier::QUEUE ) || ! reachesNegativeCycle( g , r ) );
 using D = typename G::Weight;
 const D INF = infinity< D >();
 const int n = g.bound();
 ShortestPathTree< D > R;
 R.d.assign( n , INF );
 R.T.assign( n , NIL );
 std::vector< bool > b( n , false );
 std::vector< int > count( n , 0 );

 // the six structures, only one of which is used
 using LQ = ListPriorityQueue< int , D , false >;
 using HQ = HeapPriorityQueue< int , D >;
 using FQ = FibonacciHeapPriorityQueue< int , D >;
 std::unique_ptr< LQ > lq;
 std::unique_ptr< HQ > hq;
 std::unique_ptr< FQ > fq;
 std::vector< typename LQ::Handle > lh;
 std::vector< typename HQ::Handle > hh;
 std::vector< typename FQ::Handle > fh;
 ArrayQueue< int > Q( std::max( n , 1 ) );
 ArrayStack< int > St( std::max( n , 1 ) );
 Dequeue< int > Dq;
 switch( how ) {
  case( Frontier::LIST ): lq = std::make_unique< LQ >(); lh.resize( n );
                          break;
  case( Frontier::HEAP ): hq = std::make_unique< HQ >( n ); hh.resize( n );
                          break;
  case( Frontier::FIBONACCI ): fq = std::make_unique< FQ >(); fh.resize( n );
                               break;
  default: break;
  }

 auto add = [ & ]( int v , D key , bool first ) {          // (1) and (3)
  switch( how ) {
   case( Frontier::LIST ): lh[ v ] = lq->insert( v , key ); break;
   case( Frontier::HEAP ): hh[ v ] = hq->insert( v , key ); break;
   case( Frontier::FIBONACCI ): fh[ v ] = fq->insert( v , key ); break;
   case( Frontier::QUEUE ): Q.enqueue( v ); break;
   case( Frontier::STACK ): St.push( v ); break;
   case( Frontier::DEQUE ):
    if( first ) Dq.insertTail( v ); else Dq.insertHead( v );
   }
  };
 auto empty = [ & ]( void ) {
  switch( how ) {
   case( Frontier::LIST ): return( lq->isEmpty() );
   case( Frontier::HEAP ): return( hq->isEmpty() );
   case( Frontier::FIBONACCI ): return( fq->isEmpty() );
   case( Frontier::QUEUE ): return( Q.isEmpty() );
   case( Frontier::STACK ): return( St.isEmpty() );
   default: return( Dq.isEmpty() );
   }
  };
 auto extract = [ & ]( void ) {                             // (2)
  switch( how ) {
   case( Frontier::LIST ): return( lq->deleteMin() );
   case( Frontier::HEAP ): return( hq->deleteMin() );
   case( Frontier::FIBONACCI ): return( fq->deleteMin() );
   case( Frontier::QUEUE ): return( Q.dequeue() );
   case( Frontier::STACK ): return( St.pop() );
   default: return( Dq.removeHead() );
   }
  };
 auto update = [ & ]( int v , D key ) {                     // (4)
  switch( how ) {
   case( Frontier::LIST ): lq->decrease( lh[ v ] , key ); break;
   case( Frontier::HEAP ): hq->decrease( hh[ v ] , key ); break;
   case( Frontier::FIBONACCI ): fq->decrease( fh[ v ] , key ); break;
   default: break;
   }
  };

 R.d[ r ] = D( 0 );
 b[ r ] = true;
 add( r , D( 0 ) , true );
 while( ! empty() ) {
  const int u = extract();
  b[ u ] = false;
  R.order.push_back( u );
  if( ( how == Frontier::QUEUE ) && ( ++count[ u ] > n ) ) {
   // a node is extracted at most n times by a queue, unless there is a
   // negative cycle; the other frontiers have no such bound
   R.negativeCycle = true;
   return( R );
   }
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   const D nd = R.d[ u ] + w;
   if( nd < R.d[ v ] ) {
    const bool first = ( R.d[ v ] == INF );
    R.d[ v ] = nd;
    R.T[ v ] = u;
    if( ! b[ v ] ) {
     add( v , nd , first );
     b[ v ] = true;
     }
    else
     update( v , nd );
    }
   } );
  }
 ADS_AUDIT( isShortestPathTree( g , r , R ) );
 return( R );
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
