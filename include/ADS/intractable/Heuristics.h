/*--------------------------------------------------------------------------*/
/*--------------------------- File Heuristics.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The heuristics of section 19.4 and of the exercises 19.13 and 19.16 to
 * 19.19: feasible solutions, neither optimal nor approximate, greedy or
 * by local search.
 *
 * - `greedyTsp()`: the edges by increasing weight, each added if its ends
 *   have fewer than two edges and are in different components of an
 *   MFSET, until a Hamiltonian chain, closed by the edge of its two ends,
 *   \f$ O( n^2 \log n ) \f$;
 * - `minMaxGreedy()` for the BISECTION: from a node in \f$ V_0 \f$ and one
 *   in \f$ V_1 \f$, a node added to \f$ V_0 \f$ and then one to \f$ V_1 \f$,
 *   the one with fewest edges to the other side and, among those, most edges
 *   to its own; the counts are kept by the structure with buckets of
 *   exercise 19.17 [see `Buckets`];
 * - `twoOpt()`: the local search on the neighborhood \f$ I_2( \pi ) \f$,
 *   the circuits made of \f$ \pi \f$ by replacing two edges that do not
 *   touch with the two that close it again, \f$ n ( n - 1 ) / 2 - n \f$ of
 *   them; `kOptNeighbours()` counts \f$ I_k( \pi ) \f$ (exercise 19.15);
 * - `partitionLocalSearch()`: the PARTITION that minimizes the difference
 *   of the sums, moving an element or swapping two (exercise 19.16);
 * - `greedySuperstring()`: the greedy of `greedyTsp()` on the graph of the
 *   overlaps, an oriented path (exercise 19.13);
 * - `starAlignment()`: the multiple alignment against the string with the
 *   best sum of the global scores (exercise 19.18);
 * - `dsatur()`: the coloring by saturation degree (exercise 19.19).
 *
 * The contracts [see `Contract.h`]: the distances are square and symmetric,
 * the graphs unoriented, the circuits permutations; each heuristic checks
 * that its result is feasible, not that it is good, and the structure with
 * buckets keeps its invariant at level 3. The operations are defined in
 * `Heuristics.tpp`.
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
 *         Università di Pisa \n
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

/// the edges chosen, in the order of insertion, the last one closing the
/// chain, and the circuit from 0, for the symmetric distances \f$ D \f$ of
/// \f$ n \ge 3 \f$ cities
///
/// In pseudocode (section 19.4.1), the edges kept form paths, since no node
/// gets a third edge and no edge closes a cycle, until one path spans the
/// cities:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{greedyTsp}( D ) \cr
/// 2 & \quad S \gets \emptyset;\ M \gets \text{an MFSET of the } n
///     \text{ cities} \cr
/// 3 & \quad \textbf{foreach}\ [ u , v ] \text{ by increasing } D[ u ][ v ]
///     \text{, while } |S| < n - 1\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ d_S( u ) < 2 \text{, } d_S( v ) < 2 \text{ and }
///     \mathrm{find}( M , u ) \ne \mathrm{find}( M , v )\ \textbf{then} \cr
/// 5 & \qquad\quad S \gets S \cup \{ [ u , v ] \};\ \mathrm{merge}( M , u ,
///     v ) \cr
/// 6 & \quad S \gets S \cup \{ [ a , b ] \}\quad \triangleright\ a , b
///     \text{ the two ends of the chain} \cr
/// 7 & \quad \textbf{return}\ ( S , \text{the circuit of } S \text{ from }
///     0 )
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square and symmetric, and \f$ n \ge 3 \f$
/// \post \f$ n \f$ edges and a permutation of the cities
template< class T >
std::pair< std::vector< std::pair< int , int > > , Nodes > greedyTsp(
                        const std::vector< std::vector< T > > & D );

/*--------------------------------------------------------------------------*/
/*---------------------------- LOCAL SEARCH --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the circuit of \f$ I_2( \pi ) \f$ without the edges
/// \f$ ( \pi[ i ] , \pi[ i + 1 ] ) \f$ and
/// \f$ ( \pi[ j ] , \pi[ j + 1 ] ) \f$, \f$ 0 \le i < j < n \f$, not
/// consecutive: the stretch \f$ \pi[ i + 1 .. j ] \f$ reversed
/// \pre \f$ 0 \le i < j < n \f$
/// \post a circuit of the same cities
inline Nodes twoOptMove( Nodes pi , int i , int j );

/// the local search on \f$ I_2 \f$ from \f$ \pi \f$, for the symmetric
/// distances \f$ D \f$, taking the best circuit of the neighborhood while it
/// improves; the costs of the circuits visited go to `*trace`, if given
///
/// In pseudocode (section 19.4.2), the cost of a move computed in
/// \f$ O( 1 ) \f$ from the four edges that change, a step in
/// \f$ O( n^2 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{twoOpt}( D , \pi ) \cr
/// 2 & \quad c \gets \mathrm{tourCost}( D , \pi ) \cr
/// 3 & \quad \textbf{loop} \cr
/// 4 & \qquad ( b_i , b_j ) \gets \text{the } i < j \text{, not consecutive,}
///     \ \text{of least } c' = c - D[ a ][ b ] - D[ x ][ y ] + D[ a ][ x ] +
///     D[
///     b ][ y ] \text{, with } a = \pi[ i ] \text{, } b = \pi[ i + 1 ]
///     \text{, } x = \pi[ j ] \text{, } y = \pi[ ( j + 1 ) \bmod n ] \cr
/// 5 & \qquad \textbf{if}\ c' \ge c\ \textbf{then return}\ \pi\quad
///     \triangleright\ \text{a local optimum} \cr
/// 6 & \qquad \pi \gets \mathrm{twoOptMove}( \pi , b_i , b_j );\ c \gets c'
/// \end{array}}
/// \f]
/// \pre \f$ D \f$ is square and symmetric, and \f$ \pi \f$ a permutation
/// \post a permutation of the cities
template< class T >
Nodes twoOpt( const std::vector< std::vector< T > > & D , Nodes pi ,
              std::vector< T > * trace = nullptr );

/// the size of \f$ I_k( \pi ) \f$ on \f$ n \ge 3 \f$ nodes: the circuits that
/// share exactly \f$ n - k \f$ edges with \f$ \pi \f$, by enumeration of all
/// the circuits, \f$ O( n! ) \f$
///
/// In pseudocode (exercise 19.15), each circuit met twice, once per
/// direction:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kOptNeighbours}( n , k ) \cr
/// 2 & \quad E \gets \text{the edges of } \pi = ( 0 , 1 , \ldots , n - 1 ) \cr
/// 3 & \quad c \gets 0 \cr
/// 4 & \quad \textbf{foreach}\ \text{permutation } s \text{ of } 1 , \ldots ,
///     n - 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ \text{the circuit } ( 0 ) \cdot s \text{ has }
///     n - k\ \text{edges in } E\ \textbf{then}\ c \gets c + 1 \cr
/// 6 & \quad \textbf{return}\ c / 2
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 3 \f$ and \f$ 0 \le k \le n \f$
inline long kOptNeighbours( int n , int k );

/// the side, 0 or 1, of each element of \f$ A \f$, by the local search from
/// `side` that moves an element or swaps two while the difference of the
/// sums decreases, the best move first; \f$ O( n^2 ) \f$ each step
///
/// In pseudocode (exercise 19.16), \f$ \Delta \f$ the sum of the side 0 minus
/// that of the side 1, which a move of \f$ A[ i ] \f$ changes by
/// \f$ \pm 2 A[ i ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{partitionLocalSearch}( A , \mathit{side} )
///     \cr
/// 2 & \quad \textbf{loop} \cr
/// 3 & \qquad M \gets \text{the move of one } i \text{, or the swap of } i , j
///     \text{ on two sides,}\ \text{of least } | \Delta' | \cr
/// 4 & \qquad \textbf{if}\ | \Delta' | \ge | \Delta |
///     \ \textbf{then return}\ \mathit{side} \cr
/// 5 & \qquad \text{apply } M;\ \Delta \gets \Delta'
/// \end{array}}
/// \f]
/// \pre `side` gives each element 0 or 1
/// \post sides 0 or 1, the difference not larger than that of `side`
inline std::vector< int > partitionLocalSearch( const std::vector< long > & A ,
                                                std::vector< int > side );

/*--------------------------------------------------------------------------*/
/*---------------------------- BISECTION -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure with buckets of exercise 19.17: for \f$ k = 0 , 1 \f$ the
/// nodes by the number \f$ h \f$ of their edges to \f$ V_k \f$,
/// `bucket( k , h )`, its size `dim( k , h )`, and the least and largest
/// bucket not empty; a node leaves a bucket in \f$ O( 1 ) \f$ by its place
/// in it
class Buckets {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty structure for the nodes \f$ [ 0 , n ) \f$
 /// \pre \f$ n \ge 0 \f$
 explicit Buckets( int n );

 /// \f$ u \f$ with no edges to either side, in the buckets 0
 /// \pre \f$ u \f$ is a node out of the structure
 void insert( int u );

 /// takes \f$ u \f$ out of its two buckets, moving the least and the largest
 /// bucket not empty if they empty
 /// \pre \f$ u \f$ is in the structure
 void remove( int u );

 /// \f$ u \f$ has one more edge to \f$ V_k \f$, and moves to the next bucket
 /// \pre \f$ u \f$ is in the structure, \f$ k \f$ is 0 or 1, and \f$ u \f$
 ///      has fewer than \f$ n \f$ edges to \f$ V_k \f$
 void inc( int k , int u );

 /// whether \f$ u \f$ is in the structure
 /// \pre \f$ u \f$ is a node
 [[nodiscard]] bool contains( int u ) const;

 /// the edges of \f$ u \f$ to \f$ V_k \f$
 /// \pre \f$ k \f$ is 0 or 1 and \f$ u \f$ is a node
 [[nodiscard]] int count( int k , int u ) const;

 /// the nodes with \f$ h \f$ edges to \f$ V_k \f$
 /// \pre \f$ k \f$ is 0 or 1 and \f$ 0 \le h \le n \f$
 [[nodiscard]] const Nodes & bucket( int k , int h ) const;

 /// the number of the nodes with \f$ h \f$ edges to \f$ V_k \f$
 /// \pre \f$ k \f$ is 0 or 1 and \f$ 0 \le h \le n \f$
 [[nodiscard]] int dim( int k , int h ) const;

 /// the least \f$ h \f$ with `bucket( k , h )` not empty, \f$ n + 1 \f$ if
 /// none
 /// \pre \f$ k \f$ is 0 or 1
 [[nodiscard]] int min( int k ) const;

 /// the largest \f$ h \f$ with `bucket( k , h )` not empty, \f$ -1 \f$ if
 /// none
 /// \pre \f$ k \f$ is 0 or 1
 [[nodiscard]] int max( int k ) const;

 /// the node to add to \f$ V_k \f$: in the least bucket of \f$ V_{1-k} \f$,
 /// the one with most edges to \f$ V_k \f$, the first of the bucket on ties
 /// \pre \f$ k \f$ is 0 or 1 and the structure is not empty
 /// \post the result is in the structure
 [[nodiscard]] int choose( int k ) const;

/*--------------------------------------------------------------------------*/
 private:

 void put( int k , int u );

 void take( int k , int u );

 /// each node is in the bucket of its count at its place, and min( k ),
 /// max( k ) are the least and the largest bucket not empty, \f$ O( n ) \f$
 [[nodiscard]] bool consistent( void ) const;

 int f_n;
 std::vector< Nodes > f_bucket[ 2 ];
 std::vector< int > f_h[ 2 ] , f_place[ 2 ];
 int f_min[ 2 ] , f_max[ 2 ];

 };  // end( class Buckets )

/// the side of each node of \f$ G \f$, \f$ n \f$ even, from \f$ a \f$ in
/// \f$ V_0 \f$ and \f$ b \f$ in \f$ V_1 \f$ (the min-max-greedy of the book)
///
/// In pseudocode (section 19.4.1), the sides filled in turn, each time with
/// the node out of them that adds the fewest edges to the cut:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{minMaxGreedy}( G , a , b ) \cr
/// 2 & \quad B \gets \text{a structure with buckets holding all the nodes} \cr
/// 3 & \quad \mathrm{add}( a , 0 );\ \mathrm{add}( b , 1 );\ k \gets 0 \cr
/// 4 & \quad \textbf{for}\ \mathit{left} \gets n - 2\ \textbf{downto}\ 1
///     \ \textbf{do}\ \mathrm{add}( B.\mathrm{choose}( k ) , k );\ k \gets 1 -
///     k \cr
/// 5 & \quad \textbf{return}\ \mathit{side} \cr
/// 6 & \textbf{procedure}\ \mathrm{add}( u , k ) \cr
/// 7 & \quad \mathit{side}[ u ] \gets k;\ B.\mathrm{remove}( u ) \cr
/// 8 & \quad \textbf{foreach}\ v \text{ with } G[ u ][ v ] \text{ in } B
///     \ \textbf{do}\ B.\mathrm{inc}( k , v )
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric, \f$ n \f$ even,
///      \f$ a \f$ and \f$ b \f$ distinct nodes
/// \post two halves, \f$ a \f$ in the first and \f$ b \f$ in the second
inline std::vector< int > minMaxGreedy( const Adjacency & G , int a , int b );

/// the number of the edges across the two sides
/// \pre \f$ G \f$ is square and `side` has a side per node
inline int cutSize( const Adjacency & G , const std::vector< int > & side );

/*--------------------------------------------------------------------------*/
/*----------------------------- STRINGS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the length of the longest suffix of \f$ a \f$ that is a prefix of
/// \f$ b \f$, at most \f$ \min\{ |a| , |b| \} \f$: the whole of \f$ b \f$ if
/// \f$ a \f$ ends with it, 0 if either is empty
///
/// In pseudocode (exercise 19.13), \f$ O( \min\{ |a| , |b| \}^2 ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{overlap}( a , b ) \cr
/// 2 & \quad \textbf{for}\ \ell \gets \min\{ |a| , |b| \}\ \textbf{downto}
///     \ 1\ \textbf{do} \cr
/// 3 & \qquad \textbf{if}\ a[ |a| - \ell .. |a| - 1 ] = b[ 0 .. \ell - 1 ]
///     \ \textbf{then return}\ \ell \cr
/// 4 & \quad \textbf{return}\ 0
/// \end{array}}
/// \f]
inline int overlap( const std::string & a , const std::string & b );

/// a common superstring of \f$ P \f$, by the greedy on the overlaps: the
/// oriented edges \f$ ( i , j ) \f$ by decreasing overlap, each added if
/// \f$ \mathit{out}[ i ] \f$ and \f$ \mathit{in}[ j ] \f$ are 0 and
/// \f$ i , j \f$ are apart, and the path joined; the strings contained in
/// another are dropped first
///
/// In pseudocode (exercise 19.13), the greedy of `greedyTsp()` on oriented
/// edges, which leaves paths, each string written without its overlap with
/// the one before it:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{greedySuperstring}( P ) \cr
/// 2 & \quad Q \gets P \text{ without the strings contained in another}
///     \ \text{(and the repeated ones)} \cr
/// 3 & \quad \textbf{foreach}\ ( i , j ) \text{, } i \ne j
///     \text{, by decreasing}\ \mathrm{overlap}( Q[ i ] , Q[ j ] )
///     \ \textbf{do} \cr
/// 4 & \qquad \textbf{if not}\ \mathit{out}[ i ] \text{, not }
///     \mathit{in}[ j ] \text{ and }\ \mathrm{find}( i ) \ne \mathrm{find}( j
///     )\ \textbf{then} \cr
/// 5 & \qquad\quad \mathit{next}[ i ] \gets j;\ \mathit{out}[ i ] \gets
///     \mathit{in}[ j ] \gets \textbf{true};\ \mathrm{merge}( i , j ) \cr
/// 6 & \quad \textbf{return}\ \text{the concatenation along } \mathit{next}
///     \text{ from each}\ \text{start, overlaps written once}
/// \end{array}}
/// \f]
/// \post a string that contains each string of \f$ P \f$
inline std::string greedySuperstring( const std::vector< std::string > & P );

/// the star alignment: the center \f$ h \f$, the string with the largest sum
/// of the global scores against the others, and its global alignment with
/// each string, the center against itself included;
/// \f$ O( k^2 n^2 ) \f$
///
/// In pseudocode (exercise 19.18, after Gusfield):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{starAlignment}( P ) \cr
/// 2 & \quad h \gets \text{the first } i \text{ of largest } \sum_{j \ne i}
///     \mathrm{score}( P[ i ] , P[ j ] ) \cr
/// 3 & \quad \textbf{return}\ ( h , ( \mathrm{globalAlignment}( P[ h ] , P[ j
///     ] ) : j = 1 , \ldots , k ) )
/// \end{array}}
/// \f]
/// \pre \f$ P \f$ is not empty
/// \post a center of \f$ P \f$ and an alignment per string
inline std::pair< int , std::vector< std::pair< std::string , std::string > > >
starAlignment( const std::vector< std::string > & P );

/*--------------------------------------------------------------------------*/
/*----------------------------- COLORING -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the coloring by saturation degree: the color, from 0, of each node of
/// \f$ G \f$, colored one at a time with the least color not on its
/// neighbors, the next node the one with most distinct colors among its
/// neighbors, then with most neighbors colored, then the first;
/// \f$ O( n^3 ) \f$
///
/// In pseudocode (exercise 19.19, the algorithm of Brelaz):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dsatur}( G ) \cr
/// 2 & \quad \textbf{foreach}\ v \in V\ \textbf{do}\ c[ v ] \gets
///     \textbf{nil} \cr
/// 3 & \quad \textbf{repeat}\ n\ \text{times} \cr
/// 4 & \qquad u \gets \text{the node not colored of most distinct colors}
///     \ \text{among its neighbors, then of most neighbors colored, then}
///     \ \text{the first} \cr
/// 5 & \qquad c[ u ] \gets \text{the least color not on a neighbor of } u \cr
/// 6 & \quad \textbf{return}\ c
/// \end{array}}
/// \f]
/// \pre \f$ G \f$ is unoriented, i.e., square and symmetric
/// \post a proper coloring
inline std::vector< int > dsatur( const Adjacency & G );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/impl/Heuristics.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Heuristics.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Heuristics.h --------------------------*/
/*--------------------------------------------------------------------------*/
