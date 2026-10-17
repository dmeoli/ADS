/*--------------------------------------------------------------------------*/
/*--------------------------- File Heuristics.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The heuristics of section 19.4 and of the exercises 19.13 and 19.16 to
 * 19.19: feasible solutions, neither optimal nor approximate, greedy or
 * by local search.
 *
 * - greedyTsp(): the edges by increasing weight, each added if its ends
 *   have fewer than two edges and are in different components of an
 *   MFSET, until a Hamiltonian chain, closed by the edge of its two ends,
 *   \f$ O( n^2 \log n ) \f$;
 * - minMaxGreedy() for the BISEZIONE: from a node in \f$ V_0 \f$ and one in
 *   \f$ V_1 \f$, a node added to \f$ V_0 \f$ and then one to \f$ V_1 \f$, the
 *   one with fewest edges to the other side and, among those, most edges
 *   to its own; the counts are kept by the structure with buckets of
 *   exercise 19.17 [see Buckets];
 * - twoOpt(): the local search on the neighbourhood \f$ I_2( \pi ) \f$, the
 *   circuits made of \f$ \pi \f$ by replacing two edges that do not touch
 *   with the two that close it again, \f$ n ( n - 1 ) / 2 - n \f$ of them;
 *   kOptNeighbours() counts \f$ I_k( \pi ) \f$ (exercise 19.15);
 * - partitionLocalSearch(): the PARTIZIONE that minimizes the difference
 *   of the sums, moving an element or swapping two (exercise 19.16);
 * - greedySuperstring(): the greedy of greedyTsp() on the graph of the
 *   overlaps, an oriented path (exercise 19.13);
 * - starAlignment(): the multiple alignment against the string with the
 *   best sum of the global scores (exercise 19.18);
 * - dsatur(): the coloring by saturation degree (exercise 19.19).
 *
 * The contracts [see Contract.h]: the distances are square and symmetric, the
 * graphs unoriented, the circuits permutations; each heuristic checks that its
 * result is feasible, not that it is good, and the structure with buckets
 * keeps its invariant at level 3. The operations are defined in
 * Heuristics.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.4 and
 *      exercises 19.13-19.19,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see B. W. Kernighan, S. Lin, An efficient heuristic procedure for
 *      partitioning graphs, Bell System Technical Journal 49(2), 291-307,
 *      1970, doi:10.1002/j.1538-7305.1970.tb01770.x
 * \see S. Lin, Computer solutions of the traveling salesman problem, Bell
 *      System Technical Journal 44(10), 2245-2269, 1965,
 *      doi:10.1002/j.1538-7305.1965.tb04146.x
 * \see D. Gusfield, Efficient methods for multiple sequence alignment with
 *      guaranteed error bounds, Bulletin of Mathematical Biology 55(1),
 *      141-154, 1993, doi:10.1007/BF02460299
 * \see D. Brelaz, New methods to color the vertices of a graph,
 *      Communications of the ACM 22(4), 251-256, 1979,
 *      doi:10.1145/359094.359101
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

#ifndef __ADS_Heuristics
 #define __ADS_Heuristics
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/string/Alignment.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ GREEDY TSP --------------------------------*/
/*--------------------------------------------------------------------------*/

/// greedyTsp(): the edges chosen, in the order of insertion, the last one
/// closing the chain, and the circuit from 0, for the symmetric distances
/// D of n >= 3 cities
/// \pre D is square and symmetric, and n >= 3
/// \post n edges and a permutation of the cities
template< class T >
std::pair< std::vector< std::pair< int , int > > , Nodes > greedyTsp(
                        const std::vector< std::vector< T > > & D );

/*--------------------------------------------------------------------------*/
/*---------------------------- LOCAL SEARCH --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the circuit of I2( pi ) without the edges ( pi[ i ] , pi[ i + 1 ] ) and
/// ( pi[ j ] , pi[ j + 1 ] ), 0 <= i < j < n, not consecutive: the stretch
/// pi[ i + 1 .. j ] reversed
/// \pre 0 <= i < j < n
/// \post a circuit of the same cities
inline Nodes twoOptMove( Nodes pi , int i , int j );

/// the local search on I2 from pi, for the symmetric distances D, taking
/// the best circuit of the neighbourhood while it improves; the costs of
/// the circuits visited go to *trace, if given
/// \pre D is square and symmetric, and pi a permutation
/// \post a permutation of the cities
template< class T >
Nodes twoOpt( const std::vector< std::vector< T > > & D , Nodes pi ,
              std::vector< T > * trace = nullptr );

/// the size of I_k( pi ) on n >= 3 nodes: the circuits that share exactly
/// n - k edges with pi, by enumeration of all the circuits, O( n! )
/// \pre n >= 3 and 0 <= k <= n
inline long kOptNeighbours( int n , int k );

/// the side, 0 or 1, of each element of A, by the local search from side
/// that moves an element or swaps two while the difference of the sums
/// decreases, the best move first; O( n^2 ) each step
/// \pre side gives each element 0 or 1
/// \post sides 0 or 1, the difference not larger than that of side
inline std::vector< int > partitionLocalSearch( const std::vector< long > & A ,
                                                std::vector< int > side );

/*--------------------------------------------------------------------------*/
/*---------------------------- BISECTION -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure with buckets of exercise 19.17: for k = 0 , 1 the nodes
/// by the number h of their edges to \f$ V_k \f$, bucket( k , h ), its size
/// dim( k , h ), and the least and largest bucket not empty; a node leaves
/// a bucket in O( 1 ) by its place in it
class Buckets {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty structure for the nodes [ 0 , n )
 /// \pre n >= 0
 explicit Buckets( int n );

 /// u with no edges to either side
 /// \pre u is a node out of the structure
 void insert( int u );

 /// \pre u is in the structure
 void remove( int u );

 /// u has one more edge to \f$ V_k \f$
 /// \pre u is in the structure, k is 0 or 1, and u has fewer than n edges
 ///      to \f$ V_k \f$
 void inc( int k , int u );

 /// \pre u is a node
 [[nodiscard]] bool contains( int u ) const;

 /// \pre k is 0 or 1 and u is a node
 [[nodiscard]] int count( int k , int u ) const;

 /// \pre k is 0 or 1 and 0 <= h <= n
 [[nodiscard]] const Nodes & bucket( int k , int h ) const;

 /// \pre k is 0 or 1 and 0 <= h <= n
 [[nodiscard]] int dim( int k , int h ) const;

 /// \pre k is 0 or 1
 [[nodiscard]] int min( int k ) const;

 /// \pre k is 0 or 1
 [[nodiscard]] int max( int k ) const;

 /// the node to add to \f$ V_k \f$: in the least bucket of \f$ V_{1-k} \f$,
 /// the one with most edges to \f$ V_k \f$, the first of the bucket on ties
 /// \pre k is 0 or 1 and the structure is not empty
 /// \post the result is in the structure
 [[nodiscard]] int choose( int k ) const;

/*--------------------------------------------------------------------------*/
 private:

 void put( int k , int u );

 void take( int k , int u );

 /// each node is in the bucket of its count at its place, and min( k ),
 /// max( k ) are the least and the largest bucket not empty, O( n )
 [[nodiscard]] bool consistent( void ) const;

 int f_n;
 std::vector< Nodes > f_bucket[ 2 ];
 std::vector< int > f_h[ 2 ] , f_place[ 2 ];
 int f_min[ 2 ] , f_max[ 2 ];

 };  // end( class Buckets )

/// min-max-greedy: the side of each node of G, n even, from a in
/// \f$ V_0 \f$ and b in \f$ V_1 \f$
/// \pre G is unoriented, i.e., square and symmetric, n even, a and b distinct
///      nodes
/// \post two halves, a in the first and b in the second
inline std::vector< int > minMaxGreedy( const Adjacency & G , int a , int b );

/// the edges across the two sides
/// \pre G is square and side has a side per node
inline int cutSize( const Adjacency & G , const std::vector< int > & side );

/*--------------------------------------------------------------------------*/
/*----------------------------- STRINGS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the longest suffix of a that is a proper prefix of b
inline int overlap( const std::string & a , const std::string & b );

/// a common superstring of P, by the greedy on the overlaps: the oriented
/// edges ( i , j ) by decreasing overlap, each added if out[ i ] and
/// in[ j ] are 0 and i , j are apart, and the path joined; the strings
/// contained in another are dropped first
/// \post a string that contains each string of P
inline std::string greedySuperstring( const std::vector< std::string > & P );

/// star alignment: the center h, the string with the largest sum of the
/// global scores against the others, and its global alignment with each
/// string, the center against itself included; O( k^2 n^2 )
/// \pre P is not empty
/// \post a center of P and an alignment per string
inline std::pair< int , std::vector< std::pair< std::string , std::string > > >
starAlignment( const std::vector< std::string > & P );

/*--------------------------------------------------------------------------*/
/*----------------------------- COLORING -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// saturation degree: the color, from 0, of each node of G, colored one
/// at a time with the least color not on its neighbours, the next node the
/// one with most distinct colors among its neighbours, then with most
/// neighbours colored, then the first; O( n^3 )
/// \pre G is unoriented, i.e., square and symmetric
/// \post a proper coloring
inline std::vector< int > dsatur( const Adjacency & G );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/Heuristics.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Heuristics.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Heuristics.h --------------------------*/
/*--------------------------------------------------------------------------*/
