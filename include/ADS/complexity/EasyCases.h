/*--------------------------------------------------------------------------*/
/*---------------------------- File EasyCases.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The polynomial halves of the pairs of Tabella 19.1 of the Conclusione,
 * problems close to NP-hard ones but solvable in polynomial time, and the
 * special cases that the Conclusione names, the interval graphs:
 *
 * - twoSat(), SODDISFATTIBILITA' with at most two literals per clause, by
 *   the graph of the implications \f$ \neg a \Rightarrow b \f$ and
 *   \f$ \neg b \Rightarrow a \f$ of each clause \f$ a \vee b \f$: the formula
 *   is satisfiable if and only if no variable is in the strongly connected
 *   component of its negation, and a variable is true if its component
 *   comes after that of its negation in topological order, O( n + m );
 * - twoColoring(), the 2-COLORAZIONE, by a breadth first visit of each
 *   component, O( n^2 ) on the adjacency matrix;
 * - bipartiteMatching(), the ABBINAMENTO 2-DIMENSIONALE, by a maximum flow
 *   from a source to the left nodes, along the edges, to a sink;
 * - intervalGraph(), the graph whose nodes are intervals [ a , b ) and
 *   whose edges join those that intersect, and on it intervalClique(),
 *   the largest number of intervals through a point, by a sweep in
 *   \f$ O( n \log n ) \f$; the independent set and the coloring of intervals
 *   are in Intervals.h. The other easy halves are linearTiling() in
 *   Domino.h (tested in chapter 18), eulerianCycle() in EulerianCycle.h,
 *   fractionalKnapsack() in FractionalKnapsack.h, kruskal() in
 *   MinimumSpanningTree.h and shortestPaths() in ShortestPaths.h.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, Conclusione,
 *      https://algoritmicamente.it
 * \see B. Aspvall, M. F. Plass, R. E. Tarjan, A linear-time algorithm for
 *      testing the truth of certain quantified boolean formulas,
 *      Information Processing Letters 8(3), 121-123, 1979,
 *      doi:10.1016/0020-0190(79)90002-4
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

#ifndef __ADS_EasyCases
 #define __ADS_EasyCases
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Problems.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Visit.h"
#include "ADS/queue/ArrayQueue.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <set>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the node of the literal +-( v + 1 ) in the graph of the implications
inline int literalNode( int l )
{
 return( 2 * ( std::abs( l ) - 1 ) + ( l < 0 ) );
 }

/// an assignment of the n variables that satisfies F, each clause of one
/// or two literals, or nothing
inline std::optional< std::vector< bool > > twoSat( const Cnf & F , int n )
{
 ListGraph<> g( 2 * n );
 std::set< std::pair< int , int > > E;       // each implication once
 auto imply = [ & ]( int u , int v ) {
  if( ( u != v ) && E.insert( { u , v } ).second ) g.insertEdge( u , v );
  };
 for( const Clause & c : F ) {
  const int a = c[ 0 ] , b = c.size() > 1 ? c[ 1 ] : c[ 0 ];
  imply( literalNode( -a ) , literalNode( b ) );
  imply( literalNode( -b ) , literalNode( a ) );
  }
 // the components in topological order of the condensation
 const auto S = topsort( g );
 const auto id = scc( g );
 std::vector< int > rank( 2 * n + 1 , 0 );   // of each component
 for( int i = 0 , r = 0 ; i < int( S.size() ) ; ++i )
  if( ! rank[ id[ S[ i ] ] ] ) rank[ id[ S[ i ] ] ] = ++r;
 std::vector< bool > x( n );
 for( int v = 0 ; v < n ; ++v ) {
  const int p = id[ 2 * v ] , q = id[ 2 * v + 1 ];
  if( p == q ) return( std::nullopt );
  x[ v ] = rank[ p ] > rank[ q ];
  }
 return( x );
 }

/// the side, 0 or 1, of each node of G, the ends of each edge apart, or
/// nothing if G has an odd cycle
inline std::optional< std::vector< int > > twoColoring( const Adjacency & G )
{
 const int n = int( G.size() );
 std::vector< int > col( n , -1 );
 ArrayQueue< int > Q( std::max( n , 1 ) );
 for( int r = 0 ; r < n ; ++r ) {
  if( col[ r ] >= 0 ) continue;
  col[ r ] = 0;
  Q.enqueue( r );
  while( ! Q.isEmpty() ) {
   const int u = Q.dequeue();
   for( int v = 0 ; v < n ; ++v ) {
    if( ! G[ u ][ v ] ) continue;
    if( col[ v ] == col[ u ] ) return( std::nullopt );
    if( col[ v ] < 0 ) {
     col[ v ] = 1 - col[ u ];
     Q.enqueue( v );
     }
    }
   }
  }
 return( col );
 }

/// the pairs ( i , j ) of a largest matching of the bipartite graph with
/// p left and q right nodes and the edges E ( i , j ), by a maximum flow
inline std::vector< std::pair< int , int > > bipartiteMatching(
                        int p , int q ,
                        const std::vector< std::pair< int , int > > & E )
{
 FlowNetwork N( p + q + 2 );
 const int s = p + q , t = p + q + 1;
 for( int i = 0 ; i < p ; ++i ) N.addEdge( s , i , 1 );
 for( int j = 0 ; j < q ; ++j ) N.addEdge( p + j , t , 1 );
 std::vector< int > e;
 for( auto [ i , j ] : E ) e.push_back( N.addEdge( i , p + j , 1 ) );
 N.maxFlow( s , t );
 std::vector< std::pair< int , int > > M;
 for( std::size_t k = 0 ; k < E.size() ; ++k )
  if( N.flow( e[ k ] ) > 0 ) M.push_back( E[ k ] );
 return( M );
 }

/// the graph of the intervals [ a , b ), an edge if two intersect
inline Adjacency intervalGraph(
                        const std::vector< std::pair< long , long > > & I )
{
 const std::size_t n = I.size();
 Adjacency G( n , std::vector< bool >( n , false ) );
 for( std::size_t u = 0 ; u < n ; ++u )
  for( std::size_t v = u + 1 ; v < n ; ++v )
   if( ( I[ u ].first < I[ v ].second ) && ( I[ v ].first < I[ u ].second ) )
    G[ u ][ v ] = G[ v ][ u ] = true;
 return( G );
 }

/// the largest clique of the graph of the intervals [ a , b ): the most
/// intervals through one point, the right ends before the left ones
inline int intervalClique( const std::vector< std::pair< long , long > > & I )
{
 std::vector< std::pair< long , int > > E;
 for( auto [ a , b ] : I ) {
  E.emplace_back( a , 1 );
  E.emplace_back( b , -1 );
  }
 std::sort( E.begin() , E.end() );
 int c = 0 , best = 0;
 for( auto [ x , d ] : E ) best = std::max( best , c += d );
 return( best );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* EasyCases.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File EasyCases.h --------------------------*/
/*--------------------------------------------------------------------------*/
