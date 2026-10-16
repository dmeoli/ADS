/*--------------------------------------------------------------------------*/
/*----------------------------- File MaxFlow.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The maximum flow of section 15.2 in a network G = ( V , E , s , t , c ),
 * with integer capacities, and the minimum cut. The network keeps for
 * each edge its residual twin, so that the residual capacity
 * \f$ c_f( u , v ) = c( u , v ) - f( u , v ) \f$ of both directions is at
 * hand. By the Teorema 15.1 (maximum flow / minimum cut) f is maximum if
 * and only if there is no augmenting path, a path from s to t in the
 * residual network, and then the nodes S reached from s give a cut
 * ( S , V - S ) of capacity |f|. The local search augments the flow along
 * augmenting paths:
 *
 * - FORD_FULKERSON, any path, by a depth first visit: O( ( n + m ) |f*| );
 * - EDMONDS_KARP, a shortest path, by a breadth first visit: O( n m^2 );
 * - DINIC, the blocking flows of the levels of a breadth first visit:
 *   \f$ O( n^2 m ) \f$, and \f$ O( m \sqrt{n} ) \f$ with unit capacities,
 *   e.g., the bipartite matching (the bound that the book gives for the
 *   algorithm of the three Indians, Karzanov, Malhotra, Kumar and
 *   Maheshwari, of which this is the forerunner).
 *
 * The best bound that the book attributes to Karmarkar (1984),
 * \f$ O( n m \log( n^2 / m ) ) \f$, is the one of the push-relabel of
 * Goldberg and Tarjan (1988); Karmarkar's is the interior point method for
 * linear programming. feasibleCirculation() decides whether a circulation
 * with lower and upper bounds on the edges exists, by a maximum flow from a
 * new source that gives each node the lower bounds entering it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see L. R. Ford, D. R. Fulkerson, Maximal flow through a network,
 *      Canadian Journal of Mathematics 8, 399-404, 1956,
 *      doi:10.4153/CJM-1956-045-5
 * \see J. Edmonds, R. M. Karp, Theoretical improvements in algorithmic
 *      efficiency for network flow problems, Journal of the ACM 19(2),
 *      248-264, 1972, doi:10.1145/321694.321699
 * \see E. A. Dinic, Algorithm for solution of a problem of maximum flow in
 *      a network with power estimation, Soviet Mathematics Doklady 11,
 *      1277-1280, 1970
 * \see A. V. Goldberg, R. E. Tarjan, A new approach to the maximum-flow
 *      problem, Journal of the ACM 35(4), 921-940, 1988,
 *      doi:10.1145/48014.61051
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_MaxFlow
 #define __ADS_MaxFlow
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the way of finding the augmenting paths
enum class Augment { FORD_FULKERSON , EDMONDS_KARP , DINIC };

/// a network of flow with integer capacities
class FlowNetwork {

/*--------------------------------------------------------------------------*/
 public:

 /// the network of the nodes 0 , ... , n - 1, with no edge
 explicit FlowNetwork( int n ) : f_adj( n ) {
  if( n < 0 )
   throw( std::invalid_argument( "ADS::FlowNetwork: n < 0" ) );
  }

 [[nodiscard]] int n( void ) const { return( int( f_adj.size() ) ); }

 /// the edge ( u , v ) of capacity c >= 0, and its number
 int addEdge( int u , int v , long c ) {
  if( c < 0 )
   throw( std::invalid_argument( "ADS::FlowNetwork: negative capacity" ) );
  f_e.push_back( { v , c , 0 } );
  f_adj[ u ].push_back( int( f_e.size() ) - 1 );
  f_e.push_back( { u , 0 , 0 } );                  // the residual twin
  f_adj[ v ].push_back( int( f_e.size() ) - 1 );
  return( int( f_e.size() ) - 2 );
  }

 /// the flow on the edge number e
 [[nodiscard]] long flow( int e ) const { return( f_e[ e ].flow ); }

 /// the capacity of the edge number e
 [[nodiscard]] long capacity( int e ) const { return( f_e[ e ].cap ); }

 /// the maximum flow from s to t, starting from the flow there is; the
 /// augmenting paths found go to *paths, if given
 long maxFlow( int s , int t , Augment how = Augment::DINIC ,
               long * paths = nullptr ) {
  if( s == t )
   throw( std::invalid_argument( "ADS::FlowNetwork::maxFlow: s == t" ) );
  long total = 0;
  for( ; ; ) {
   long d = 0;
   if( how == Augment::DINIC ) {
    if( ! levels( s , t ) )
     break;
    f_next.assign( n() , 0 );
    while( ( d = blocking( s , t , INF ) ) > 0 ) {
     total += d;
     if( paths ) ++*paths;
     }
    continue;
    }
   d = how == Augment::EDMONDS_KARP ? bfsAugment( s , t ) :
                                      dfsAugment( s , t );
   if( d == 0 )
    break;
   total += d;
   if( paths ) ++*paths;
   }
  return( total );
  }

 /// the side S of a minimum cut, the nodes reached from s in the residual
 /// network, after maxFlow()
 [[nodiscard]] std::vector< bool > minCut( int s ) const {
  std::vector< bool > S( n() , false );
  std::vector< int > Q = { s };
  S[ s ] = true;
  for( std::size_t i = 0 ; i < Q.size() ; ++i )
   for( int e : f_adj[ Q[ i ] ] )
    if( ( residual( e ) > 0 ) && ! S[ f_e[ e ].to ] ) {
     S[ f_e[ e ].to ] = true;
     Q.push_back( f_e[ e ].to );
     }
  return( S );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Edge { int to; long cap , flow; };

 static constexpr long INF = std::numeric_limits< long >::max();

 long residual( int e ) const { return( f_e[ e ].cap - f_e[ e ].flow ); }

 void push( int e , long d ) {
  f_e[ e ].flow += d;
  f_e[ e ^ 1 ].flow -= d;                       // opposite symmetry
  }

 /// one augmenting path by a breadth first visit
 long bfsAugment( int s , int t ) {
  std::vector< int > via( n() , -1 );
  std::vector< int > Q = { s };
  std::vector< bool > seen( n() , false );
  seen[ s ] = true;
  for( std::size_t i = 0 ; ( i < Q.size() ) && ! seen[ t ] ; ++i )
   for( int e : f_adj[ Q[ i ] ] )
    if( ( residual( e ) > 0 ) && ! seen[ f_e[ e ].to ] ) {
     seen[ f_e[ e ].to ] = true;
     via[ f_e[ e ].to ] = e;
     Q.push_back( f_e[ e ].to );
     }
  if( ! seen[ t ] )
   return( 0 );
  long d = INF;
  for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
   d = std::min( d , residual( via[ v ] ) );
  for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
   push( via[ v ] , d );
  return( d );
  }

 /// one augmenting path by a depth first visit, iterative
 long dfsAugment( int s , int t ) {
  std::vector< int > via( n() , -1 );
  std::vector< bool > seen( n() , false );
  std::vector< int > S = { s };
  seen[ s ] = true;
  while( ! S.empty() && ! seen[ t ] ) {
   const int u = S.back();
   S.pop_back();
   for( int e : f_adj[ u ] )
    if( ( residual( e ) > 0 ) && ! seen[ f_e[ e ].to ] ) {
     seen[ f_e[ e ].to ] = true;
     via[ f_e[ e ].to ] = e;
     S.push_back( f_e[ e ].to );
     }
   }
  if( ! seen[ t ] )
   return( 0 );
  long d = INF;
  for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
   d = std::min( d , residual( via[ v ] ) );
  for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
   push( via[ v ] , d );
  return( d );
  }

 /// the levels of the breadth first visit from s; true if t is reached
 bool levels( int s , int t ) {
  f_level.assign( n() , -1 );
  std::vector< int > Q = { s };
  f_level[ s ] = 0;
  for( std::size_t i = 0 ; i < Q.size() ; ++i )
   for( int e : f_adj[ Q[ i ] ] )
    if( ( residual( e ) > 0 ) && ( f_level[ f_e[ e ].to ] < 0 ) ) {
     f_level[ f_e[ e ].to ] = f_level[ Q[ i ] ] + 1;
     Q.push_back( f_e[ e ].to );
     }
  return( f_level[ t ] >= 0 );
  }

 /// a path of the levels from u to t, of at most d units
 long blocking( int u , int t , long d ) {
  if( u == t )
   return( d );
  for( ; f_next[ u ] < f_adj[ u ].size() ; ++f_next[ u ] ) {
   const int e = f_adj[ u ][ f_next[ u ] ];
   const int v = f_e[ e ].to;
   if( ( residual( e ) > 0 ) && ( f_level[ v ] == f_level[ u ] + 1 ) )
    if( const long x = blocking( v , t , std::min( d , residual( e ) ) ) ) {
     push( e , x );
     return( x );
     }
   }
  return( 0 );
  }

 std::vector< std::vector< int > > f_adj;   ///< the edges out of each node
 std::vector< Edge > f_e;                   ///< the edges, each and its twin
 std::vector< int > f_level;                ///< the levels of Dinic
 std::vector< std::size_t > f_next;         ///< the next edge of Dinic

 };  // end( class FlowNetwork )

/// an edge ( u , v ) with lower and upper bounds on its flow
struct BoundedEdge { int u , v; long low , high; };

/// whether the edges admit a circulation, a flow conserved at every node
/// with low <= f( e ) <= high; then the flows in *flows, if given
inline bool feasibleCirculation( int n , const std::vector< BoundedEdge > & E ,
                                 std::vector< long > * flows = nullptr )
{
 FlowNetwork N( n + 2 );                     // n the source, n + 1 the sink
 std::vector< long > excess( n , 0 );
 std::vector< int > id;
 for( const auto & e : E ) {
  if( e.low > e.high )
   return( false );
  id.push_back( N.addEdge( e.u , e.v , e.high - e.low ) );
  excess[ e.v ] += e.low;
  excess[ e.u ] -= e.low;
  }
 long need = 0;
 for( int v = 0 ; v < n ; ++v )
  if( excess[ v ] > 0 ) {
   N.addEdge( n , v , excess[ v ] );
   need += excess[ v ];
   }
  else if( excess[ v ] < 0 )
   N.addEdge( v , n + 1 , -excess[ v ] );
 if( N.maxFlow( n , n + 1 ) != need )
  return( false );
 if( flows ) {
  flows->clear();
  for( std::size_t i = 0 ; i < E.size() ; ++i )
   flows->push_back( E[ i ].low + N.flow( id[ i ] ) );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MaxFlow.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File MaxFlow.h ---------------------------*/
/*--------------------------------------------------------------------------*/
