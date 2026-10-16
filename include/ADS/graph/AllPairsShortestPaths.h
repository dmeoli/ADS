/*--------------------------------------------------------------------------*/
/*---------------------- File AllPairsShortestPaths.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths between all the pairs of nodes, section 13.5: with
 * the nodes v_0 , ... , v_{n-1}, a path is k-bound if it passes through no
 * node of { v_k , ... , v_{n-1} } but its ends, and the least cost of the
 * k-bound paths from u to v is
 * \f[
 *   d^k[ u ][ v ] = \min\{ d^{k-1}[ u ][ v ] , d^{k-1}[ u ][ k - 1 ] +
 *                   d^{k-1}[ k - 1 ][ v ] \} ,
 * \f]
 * from \f$ d^0 \f$ the costs of the edges, 0 on the diagonal and
 * \f$ +\infty \f$ elsewhere, with p[ u ][ v ] the predecessor of v on the
 * path from u. floydWarshall() overwrites d and p for k = 1 , ... , n,
 * Theta( n^3 ) time and Theta( n^2 ) space, with costs of any sign; there
 * is a negative cycle through u if and only if d[ u ][ u ] < 0, and then
 * negativeCycle is true. The row u of p is the tree of the shortest paths
 * from u, as a vector of parents.
 *
 * addEdge() updates d and p after the insertion of an edge ( a , b ) of
 * cost w in O( n^2 ), since a path that improves goes through it once:
 * \f$ d[ u ][ v ] = \min\{ d[ u ][ v ] , d[ u ][ a ] + w + d[ b ][ v ] \}
 * \f$; it throws std::logic_error if the edge closes a negative cycle,
 * i.e., if w + d[ b ][ a ] < 0 (exercise 13.5). A deletion has no such
 * update: the paths that used the edge may need any other one.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 13.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see R. W. Floyd, Algorithm 97: Shortest path, Communications of the ACM
 *      5(6), 345, 1962, doi:10.1145/367766.368168
 * \see S. Warshall, A theorem on Boolean matrices, Journal of the ACM 9(1),
 *      11-12, 1962, doi:10.1145/321105.321107
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

#ifndef __ADS_AllPairsShortestPaths
 #define __ADS_AllPairsShortestPaths
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"
#include "ADS/graph/ShortestPaths.h"

#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the distances, the predecessors and whether there is a negative cycle
template< class D >
struct AllPairs {
 std::vector< std::vector< D > > d;
 std::vector< std::vector< int > > p;
 bool negativeCycle = false;
 };

/// floydWarshall(): the shortest paths between all the pairs of nodes
template< Graph G >
AllPairs< typename G::Weight > floydWarshall( const G & g )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 const int n = g.bound();
 AllPairs< D > R;
 R.d.assign( n , std::vector< D >( n , INF ) );
 R.p.assign( n , std::vector< int >( n , NIL ) );
 for( int u : g.V() ) {
  R.d[ u ][ u ] = D( 0 );
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   R.d[ u ][ v ] = w;
   R.p[ u ][ v ] = u;
   } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int u = 0 ; u < n ; ++u ) {
   if( R.d[ u ][ k ] == INF )
    continue;
   for( int v = 0 ; v < n ; ++v )
    if( ( R.d[ k ][ v ] != INF ) &&
        ( R.d[ u ][ k ] + R.d[ k ][ v ] < R.d[ u ][ v ] ) ) {
     R.d[ u ][ v ] = R.d[ u ][ k ] + R.d[ k ][ v ];
     R.p[ u ][ v ] = R.p[ k ][ v ];
     }
   }
 for( int u = 0 ; u < n ; ++u )
  if( R.d[ u ][ u ] < D( 0 ) )
   R.negativeCycle = true;
 return( R );
 }

/// the insertion of the edge ( a , b ) of cost w, O( n^2 )
template< class D >
void addEdge( AllPairs< D > & R , int a , int b , const D & w )
{
 const D INF = infinity< D >();
 const int n = int( R.d.size() );
 if( ( R.d[ b ][ a ] != INF ) && ( w + R.d[ b ][ a ] < D( 0 ) ) )
  throw( std::logic_error( "ADS::addEdge: a negative cycle" ) );
 if( w >= R.d[ a ][ b ] )
  return;
 const auto d = R.d;
 const auto p = R.p;
 for( int u = 0 ; u < n ; ++u ) {
  if( ( u != a ) && ( d[ u ][ a ] == INF ) )
   continue;
  const D du = u == a ? D( 0 ) : d[ u ][ a ];
  for( int v = 0 ; v < n ; ++v ) {
   if( ( v != b ) && ( d[ b ][ v ] == INF ) )
    continue;
   const D dv = v == b ? D( 0 ) : d[ b ][ v ];
   if( du + w + dv < R.d[ u ][ v ] ) {
    R.d[ u ][ v ] = du + w + dv;
    R.p[ u ][ v ] = v == b ? a : p[ b ][ v ];
    }
   }
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* AllPairsShortestPaths.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File AllPairsShortestPaths.h --------------------*/
/*--------------------------------------------------------------------------*/
