/*--------------------------------------------------------------------------*/
/*-------------------------- File EulerianCycle.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Eulerian cycle of an oriented graph (exercise 15.3), a closed path
 * that uses each edge exactly once: by the theorem of Euler it exists if
 * and only if each node has as many edges entering as leaving and all the
 * edges are in one component, the orientation ignored. The book finds it
 * by a local search on the partitions of the edges into closed paths, each
 * edge choosing its successor and a swap of two successors merging two
 * paths; here the algorithm of Hierholzer follows unused edges from a node
 * until it comes back, and splices in the closed paths found from the
 * nodes along the way, in O( n + m ) time.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, exercise 15.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see C. Hierholzer, C. Wiener, Ueber die Moeglichkeit, einen
 *      Linienzug ohne Wiederholung und ohne Unterbrechung zu umfahren,
 *      Mathematische Annalen 6(1), 30-32, 1873, doi:10.1007/BF01442866
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

#ifndef __ADS_EulerianCycle
 #define __ADS_EulerianCycle
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"
#include "ADS/mfset/MFSet.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the nodes of an Eulerian cycle of the oriented graph, the first node
/// repeated at the end, or nothing if there is none; empty with no edge
template< Graph G >
std::optional< std::vector< int > > eulerianCycle( const G & g )
{
 const int n = g.bound();
 std::vector< std::vector< int > > out( n );
 std::vector< int > in( n , 0 );
 MFSet M( n );
 int m = 0 , start = -1;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   out[ u ].push_back( v );
   ++in[ v ];
   M.merge( u , v );
   ++m;
   start = u;
   } );
 if( m == 0 )
  return( std::vector< int >() );
 for( int u = 0 ; u < n ; ++u ) {
  if( int( out[ u ].size() ) != in[ u ] )
   return( std::nullopt );
  if( ! out[ u ].empty() && ( M.find( u ) != M.find( start ) ) )
   return( std::nullopt );
  }
 std::vector< std::size_t > next( n , 0 );
 std::vector< int > S = { start } , cycle;
 while( ! S.empty() ) {
  const int u = S.back();
  if( next[ u ] < out[ u ].size() )
   S.push_back( out[ u ][ next[ u ]++ ] );
  else {
   cycle.push_back( u );
   S.pop_back();
   }
  }
 std::reverse( cycle.begin() , cycle.end() );
 return( cycle );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* EulerianCycle.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File EulerianCycle.h ------------------------*/
/*--------------------------------------------------------------------------*/
