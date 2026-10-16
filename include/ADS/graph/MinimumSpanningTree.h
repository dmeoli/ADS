/*--------------------------------------------------------------------------*/
/*----------------------- File MinimumSpanningTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum spanning tree of an unoriented connected graph with weights
 * on the edges (section 14.3): a tree on all the nodes, of n - 1 edges,
 * of least total weight. Both algorithms are greedy, and correct by the
 * Teorema 14.1: if A is part of a minimum spanning tree and no edge of A
 * crosses the cut ( S , V - S ), then the lightest edge across it can be
 * added to A.
 *
 * - kruskal() scans the edges by increasing weight and keeps one if its
 *   ends are in two components of the forest, an MFSET with the rank and
 *   the compression of the paths: O( m log n ) for the sort and almost
 *   O( m ) for the rest; it stops after n - 1 edges. With integer weights
 *   in [ 0 , k ) the edges are sorted by Counting Sort, O( m + n + k )
 *   (exercise 14.2).
 * - prim() grows the tree from a root r, adding the lightest edge that
 *   leaves it, the nodes in a priority queue by the weight of their
 *   lightest edge to the tree: O( m log n ) with a binary heap,
 *   O( n^2 ) with an unsorted list, better on dense graphs, and
 *   O( m + n log n ) with a Fibonacci heap (exercise 14.4).
 *
 * A graph that is not connected gives a minimum spanning forest by
 * kruskal(), and the tree of the component of r by prim().
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see J. B. Kruskal, On the shortest spanning subtree of a graph and the
 *      traveling salesman problem, Proceedings of the American Mathematical
 *      Society 7(1), 48-50, 1956, doi:10.1090/S0002-9939-1956-0078686-7
 * \see R. C. Prim, Shortest connection networks and some generalizations,
 *      Bell System Technical Journal 36(6), 1389-1401, 1957,
 *      doi:10.1002/j.1538-7305.1957.tb01515.x
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

#ifndef __ADS_MinimumSpanningTree
 #define __ADS_MinimumSpanningTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <tuple>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// an edge [ u , v ] of weight w
template< class W >
struct WeightedEdge {
 int u , v;
 W w;
 bool operator==( const WeightedEdge & ) const = default;
 };

/// the edges of an unoriented graph, each once, from its smaller end
template< Graph G >
std::vector< WeightedEdge< typename G::Weight > > edges( const G & g )
{
 std::vector< WeightedEdge< typename G::Weight > > E;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const auto & w ) {
   if( u < v )
    E.push_back( { u , v , w } );
   } );
 return( E );
 }

/// kruskal(): the edges of a minimum spanning forest of the nodes
/// [ 0 , n ); with k > 0 the weights are integers in [ 0 , k ), sorted by
/// counting
template< class W >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          long k = 0 )
{
 if( k > 0 ) {                                  // Counting Sort, stable
  std::vector< std::size_t > c( k + 1 , 0 );
  for( const auto & e : A ) ++c[ std::size_t( e.w ) + 1 ];
  for( long i = 0 ; i < k ; ++i ) c[ i + 1 ] += c[ i ];
  std::vector< WeightedEdge< W > > B( A.size() );
  for( const auto & e : A ) B[ c[ std::size_t( e.w ) ]++ ] = e;
  A = std::move( B );
  }
 else
  std::stable_sort( A.begin() , A.end() , []( const auto & a ,
                    const auto & b ) { return( a.w < b.w ); } );
 std::vector< WeightedEdge< W > > T;
 MFSet M( n );
 for( std::size_t i = 0 ; ( i < A.size() ) && ( int( T.size() ) < n - 1 ) ;
      ++i )
  if( M.merge( A[ i ].u , A[ i ].v ) )         // the ends were apart
   T.push_back( A[ i ] );
 return( T );
 }

template< Graph G >
auto kruskal( const G & g , long k = 0 )
{
 return( kruskal( g.bound() , edges( g ) , k ) );
 }

/// prim(): the parents of the tree from r, NIL for r and the nodes out of
/// its component; how is LIST, HEAP or FIBONACCI
template< Graph G >
std::vector< int > prim( const G & g , int r , Frontier how = Frontier::HEAP )
{
 using W = typename G::Weight;
 const int n = g.bound();
 std::vector< int > p( n , NIL );
 std::vector< bool > in( n , false );                 // in the tree
 std::vector< W > key( n , infinity< W >() );
 using LQ = ListPriorityQueue< int , W , false >;
 using HQ = HeapPriorityQueue< int , W >;
 using FQ = FibonacciHeapPriorityQueue< int , W >;
 auto run = [ & ]( auto & Q , auto & pos ) {
  for( int u : g.V() )
   pos[ u ] = Q.insert( u , u == r ? W( 0 ) : infinity< W >() );
  key[ r ] = W( 0 );
  while( ! Q.isEmpty() ) {
   const int u = Q.deleteMin();
   in[ u ] = true;
   if( key[ u ] == infinity< W >() )               // another component
    continue;
   g.forEachEdge( u , [ & ]( int v , const W & w ) {
    if( ! in[ v ] && ( w < key[ v ] ) ) {
     key[ v ] = w;
     Q.decrease( pos[ v ] , w );
     p[ v ] = u;
     }
    } );
   }
  };
 if( how == Frontier::LIST ) {
  LQ Q;
  std::vector< typename LQ::Handle > pos( n );
  run( Q , pos );
  }
 else if( how == Frontier::FIBONACCI ) {
  FQ Q;
  std::vector< typename FQ::Handle > pos( n );
  run( Q , pos );
  }
 else {
  HQ Q( n );
  std::vector< typename HQ::Handle > pos( n );
  run( Q , pos );
  }
 return( p );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MinimumSpanningTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File MinimumSpanningTree.h ---------------------*/
/*--------------------------------------------------------------------------*/
