/*--------------------------------------------------------------------------*/
/*---------------------------- File Matching.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Matchings, the sets of edges of a graph no two of which share an end
 * beyond the capacities of the ends:
 *
 * - `bipartiteBMatching()`, the b-matching of largest size of a bipartite
 *   graph, each node u on the left in at most capL[ u ] pairs and each v on
 *   the right in at most capR[ v ], each edge used once: a maximum flow of
 *   section 15.2 from a source joined to each u by capL[ u ], through the
 *   edges of capacity 1, to a sink joined from each v by capR[ v ]. The flow
 *   is integral, and its edges of flow 1 between the two sides are the pairs;
 *   with `DINIC` of `MaxFlow.h` it takes \f$ O( N^2 M ) \f$ for the N = n + 2
 *   nodes and the M = m + n edges of the network, and less on such layered
 *   networks. With all the capacities 1 it is the `bipartiteMatching()` of
 *   `EasyCases.h`, for which the algorithm of Hopcroft and Karp takes
 *   \f$ O( m \sqrt{n} ) \f$; copying each node as many times as its capacity
 *   reduces the general case to it, and Gabow shows how to do so without the
 *   blow up of the copies. It decides, e.g., whether the tasks can all be
 *   given to workers who can do them, each worker taking a bounded number of
 *   them;
 * - `maxCardinalityMatching()`, a matching of largest size of a general
 *   graph, by the blossoms of Edmonds (beyond the book): a search grows an
 *   alternating tree from a free node, shrinks each odd cycle it closes (a
 *   blossom) into its base, and stops at the first free node it reaches,
 *   along whose path the matching grows by one. The blossoms are kept as
 *   the sets of a union-find whose representative knows the base, as in the
 *   labels of Gabow, so that a search costs \f$ O( m \alpha( m , n ) ) \f$
 *   and the n searches at most \f$ O( n m \alpha( m , n ) ) \f$, within the
 *   \f$ O( n^3 ) \f$ of Gabow's implementation; a search that fails leaves
 *   its tree out of every later one, and a greedy matching starts them;
 * - `maxWeightMatching()`, a matching of largest total weight of a general
 *   graph, or of largest weight among those of largest size, by the
 *   primal-dual algorithm of Edmonds on the blossoms (beyond the book) in
 *   the \f$ O( n^3 ) \f$ of Gabow and Lawler, as Galil surveys it: the
 *   duals of the nodes and of the odd sets keep every edge of nonnegative
 *   slack and the matched ones tight, and each stage grows alternating
 *   trees on the tight edges from every free node, shrinking the blossoms
 *   and changing the duals by the least slack until an augmenting path or
 *   the optimum appears; the least slack towards each blossom is kept per
 *   blossom, so that a change of the duals costs \f$ O( n ) \f$. With
 *   integer weights all the arithmetic is on integers, the duals being
 *   kept doubled. Among those of largest size it gives, e.g., a perfect
 *   matching of least cost with the costs negated;
 * - `branchAndBoundMatching()`, the matching of largest weight of a graph
 *   of a few nodes by a branch and bound: the first node not yet decided is
 *   left unmatched or matched to each free neighbour, and a branch whose
 *   weight plus half the best weight at each node left cannot beat the best
 *   matching found is cut. It takes time proportional to the number of
 *   matchings of the graph in the worst case, the telephone numbers
 *   \f$ \sum_k \binom{n}{2k} ( 2k - 1 )!! \f$ on the complete graph, i.e.,
 *   it suits graphs of some tens of nodes; it is kept as an independent
 *   check of the blossoms, which share none of its code.
 *
 * The contracts [see `Contract.h`]: the nodes on the right of the edges are
 * in [ 0 , nr ), the capacities are not negative, an edge joins two
 * distinct nodes of the graph, the graph of `maxCardinalityMatching()` is
 * unoriented; at the level of the audits the matchings given respect the
 * capacities of the ends. The functions are defined in
 * `Matching.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 15.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf
 * \see J. E. Hopcroft, R. M. Karp, An n^{5/2} algorithm for maximum
 *      matchings in bipartite graphs, SIAM Journal on Computing 2(4),
 *      225-231, 1973, doi:10.1137/0202019
 * \see H. N. Gabow, An efficient reduction technique for degree-constrained
 *      subgraph and bidirected network flow problems, Proceedings of the
 *      15th ACM Symposium on Theory of Computing, 448-456, 1983,
 *      doi:10.1145/800061.808776
 * \see C. Berge, Two theorems in graph theory, Proceedings of the National
 *      Academy of Sciences 43(9), 842-844, 1957, doi:10.1073/pnas.43.9.842
 * \see J. Edmonds, Paths, trees, and flowers, Canadian Journal of
 *      Mathematics 17, 449-467, 1965, doi:10.4153/CJM-1965-045-4
 * \see H. N. Gabow, An efficient implementation of Edmonds' algorithm for
 *      maximum matching on graphs, Journal of the ACM 23(2), 221-234, 1976,
 *      doi:10.1145/321941.321942
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
 * \see J. Edmonds, Maximum matching and a polyhedron with 0,1-vertices,
 *      Journal of Research of the National Bureau of Standards 69B,
 *      125-130, 1965, doi:10.6028/jres.069B.013
 * \see H. N. Gabow, Implementation of algorithms for maximum matching on
 *      nonbipartite graphs, PhD thesis, Stanford University, 1973
 * \see E. L. Lawler, Combinatorial Optimization: Networks and Matroids,
 *      Holt, Rinehart and Winston, 1976, chapter 6
 * \see Z. Galil, Efficient algorithms for finding maximum matching in
 *      graphs, ACM Computing Surveys 18(1), 23-38, 1986,
 *      doi:10.1145/6462.6502
 * \see H. N. Gabow, Data structures for weighted matching and nearest
 *      common ancestors with linking, Proceedings of the 1st ACM-SIAM
 *      Symposium on Discrete Algorithms, 434-443, 1990
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

#ifndef __ADS_Matching
 #define __ADS_Matching
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/MaxFlow.h"
#include "ADS/graph/Visit.h"

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the pairs \f$ ( u , v ) \f$ of a \f$ b \f$-matching
/// of largest size, \f$ A[ u ] \f$ the nodes on the right adjacent to the node
/// \f$ u \f$ on the left, \f$ u \f$ in at most \f$ \mathit{capL}[ u ] \f$
/// pairs and \f$ v \f$ in at most \f$ \mathit{capR}[ v ] \f$
///
/// In pseudocode (a maximum flow of section 15.2):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bipartiteBMatching}( A , n_r , \mathit{capL}
///     , \mathit{capR} ) \cr
/// 2 & \quad N \gets \text{a network of the nodes on the left, those on the }
///     \text{right, a source } s \text{ and a sink } t \cr
/// 3 & \quad \textbf{foreach}\ u\ \textbf{do}\  \text{add } ( s , u ) \text{ }
///     \text{with capacity } \mathit{capL}[ u ] \cr
/// 4 & \quad \textbf{foreach}\ u , v \in A[ u ]\ \textbf{do}\  \text{add } ( u
///     , v ) \text{ with capacity } 1 \cr
/// 5 & \quad \textbf{foreach}\ v\ \textbf{do}\  \text{add } ( v , t ) \text{ }
///     \text{with capacity } \mathit{capR}[ v ] \cr
/// 6 & \quad N.\mathrm{maxFlow}( s , t ) \cr
/// 7 & \quad \textbf{return}\  \text{the pairs } ( u , v ) \text{ whose edge }
///     \text{carries flow 1}
/// \end{array}}
/// \f]
/// \pre the items of \f$ A \f$ in \f$ [ 0 , \mathit{nr} ) \f$, `capL` has a
///      place per node on the left and `capR` \f$ \mathit{nr} \f$, all of them
///      \f$ \ge 0 \f$
/// \post the pairs are edges, within the capacities of their ends (audit)
inline std::vector< std::pair< int , int > >
bipartiteBMatching( const std::vector< std::vector< int > > & A , int nr ,
		    const std::vector< long long > & capL ,
		    const std::vector< long long > & capR );

/// the mate of each node in a matching of largest size of the unoriented
/// graph \f$ g \f$, `NIL` for a node left unmatched and for a name that is
/// not a node
///
/// In pseudocode (Edmonds' algorithm, beyond the book, with the blossoms
/// as the sets of a union-find): \f$ B( v ) \f$ is the base of the
/// outermost blossom of \f$ v \f$, a node is <em>even</em> if it is the
/// root, the mate of an odd node or in a blossom, and an odd node \f$ y \f$
/// keeps the even node \f$ p[ y ] \f$ that reached it; a node of a cycle
/// that was odd keeps the edge \f$ ( a , b ) \f$ that closed it, its side
/// first, from which the path to the root is rebuilt (see
/// \ref theory_graph_flow).
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxCardinalityMatching}( G ) \cr
/// 2 & \quad M \gets \text{a greedy matching} \cr
/// 3 & \quad \textbf{foreach}\ r \in V\ \textbf{with}\ r \text{ free }
///     \textbf{do} \cr
/// 4 & \qquad \textbf{if not}\ \mathrm{search}( r )\ \textbf{then}
///     \ \text{delete the nodes of its tree from } G \cr
/// 5 & \quad \textbf{return}\ M \cr
/// 6 & \textbf{function}\ \mathrm{search}( r ) \cr
/// 7 & \quad Q \gets ( r );\ r \text{ even} \cr
/// 8 & \quad \textbf{foreach}\ x \in Q ,\ y \in G.\mathrm{adj}( x )
///     \ \textbf{with}\ y \text{ not odd}\ \textbf{do} \cr
/// 9 & \qquad \textbf{if}\ y \text{ is free and unlabeled }
///     \ \textbf{then}\ \text{augment along } y , P( x );
///     \ \textbf{return true} \cr
/// 10 & \qquad \textbf{if}\ y \text{ is unlabeled }\ \textbf{then}
///     \ y \text{ odd};\ p[ y ] \gets x;\ \text{append } M( y )
///     \text{ to } Q \text{, even} \cr
/// 11 & \qquad \textbf{else if}\ B( x ) \ne B( y )\ \textbf{then} \cr
/// 12 & \qquad\quad b \gets \text{the first common base on the paths }
///     \text{from } B( x ) \text{ and } B( y ) \text{ to } r \cr
/// 13 & \qquad\quad \mathrm{shrink}( x , y , b );\ \mathrm{shrink}( y , x ,
///     b ) \cr
/// 14 & \quad \textbf{return false} \cr
/// 15 & \textbf{procedure}\ \mathrm{shrink}( x , y , b ) \cr
/// 16 & \quad v \gets B( x ) \cr
/// 17 & \quad \textbf{while}\ v \ne b\ \textbf{do} \cr
/// 18 & \qquad o \gets M( v );\ o \text{ even, with the edge } ( x , y );
///     \ \text{append } o \text{ to } Q \cr
/// 19 & \qquad v' \gets B( p[ o ] );\ \text{merge the blossoms of } v
///     \text{ and } o \text{ into that of } b;\ v \gets v'
/// \end{array}}
/// \f]
/// The path \f$ P( v ) \f$ from an even node to the root is
/// \f$ v , M( v ) , P( p[ M( v ) ] ) \f$ for a node even from the start,
/// and the part of
/// \f$ P( a ) \f$ from \f$ v \f$ back to \f$ a \f$ followed by
/// \f$ P( b ) \f$ for a node that was odd, with the edge \f$ ( a , b ) \f$;
/// the augmentation rematches it with a stack of the pairs (node, new
/// mate), stopping each part where a node has a new mate already:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{augment}( x , y ) \cr
/// 2 & \quad M( y ) \gets x;\ K \gets ( ( x , y ) ) \cr
/// 3 & \quad \textbf{while}\ K \ne ()\ \textbf{do} \cr
/// 4 & \qquad ( v , w ) \gets \text{pop } K \cr
/// 5 & \qquad \textbf{loop} \cr
/// 6 & \qquad\quad o \gets M( v );\ M( v ) \gets w \cr
/// 7 & \qquad\quad \textbf{if}\ o = \textbf{nil}\ \textbf{or}
///     \ M( o ) \ne v\ \textbf{then break}\quad \triangleright
///     \ \text{the root, or a part done} \cr
/// 8 & \qquad\quad \textbf{if}\ v \text{ is even from the start}
///     \ \textbf{then}\ M( o ) \gets p[ o ];\ w \gets o;
///     \ v \gets p[ o ] \cr
/// 9 & \qquad\quad \textbf{else}\ ( a , b ) \gets \text{the edge that}
///     \text{ made } v \text{ even, } a \text{ on its side};
///     \ \text{push } ( b , a ) \text{ on } K;\ w \gets b;\ v \gets a
/// \end{array}}
/// \f]
/// \pre the graph is unoriented
/// \post the mates are symmetric and joined by edges (audit)
template< Graph G >
std::vector< int > maxCardinalityMatching( const G & g );

/// the largest total weight of a matching of the graph of \f$ n \f$ nodes
/// and of the edges \f$ E = ( u , v , w ) \f$, or, if `maxCardinality`,
/// the largest weight of a matching of largest size, and the places in
/// \f$ E \f$ of its edges, in increasing order
///
/// In pseudocode (the primal-dual algorithm of Edmonds in the form of
/// Galil, beyond the book; see \ref theory_graph_flow): \f$ y_v \f$ is the
/// dual of a node, \f$ z_B \f$ that of a blossom, \f$ \pi( e ) \f$ the
/// slack of an edge, \f$ y_u + y_v - 2 w( e ) \f$ plus \f$ 2 z_B \f$ for
/// each blossom that holds both ends (the duals doubled), and \f$ S \f$ and
/// \f$ T \f$ are the outer and the inner blossoms of the trees.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{maxWeightMatching}( n , E ) \cr
/// 2 & \quad M \gets \emptyset;\ y_v \gets \max\{ 0 , \max_e w( e ) \}
///     \ \forall v \cr
/// 3 & \quad \textbf{repeat}\quad \triangleright\ \text{a stage} \cr
/// 4 & \qquad \text{every free node is the root of a tree, label } S \cr
/// 5 & \qquad \textbf{loop} \cr
/// 6 & \qquad\quad \textbf{foreach}\ \text{tight edge } ( u , v )
///     \text{ from an } S\text{-node } u \text{ in another blossom}
///     \ \textbf{do} \cr
/// 7 & \qquad\qquad \textbf{if}\ v \text{ is unlabeled }\ \textbf{then}
///     \ \text{label its blossom } T \text{ and the mate of its base } S
///     \cr
/// 8 & \qquad\qquad \textbf{else if}\ v \text{ is } S \text{ in the }
///     \text{same tree }\ \textbf{then}\ \text{shrink the blossom, } S \cr
/// 9 & \qquad\qquad \textbf{else if}\ v \text{ is } S\ \textbf{then}
///     \ \text{augment } M \text{ along the two paths};\ \textbf{break}
///     \text{ the stage} \cr
/// 10 & \qquad\quad \delta \gets \min\{ \min_S y_v , \min \pi( S ,
///     \text{free} ) , \min \pi( S , S ) / 2 , \min_T z_B \} \cr
/// 11 & \qquad\quad y_v \gets y_v \mp \delta \text{ on } S \text{ and }
///     T \text{ nodes};\ z_B \gets z_B \pm \delta \text{ on } S
///     \text{ and } T \text{ blossoms} \cr
/// 12 & \qquad\quad \textbf{if}\ \delta = \min_S y_v\ \textbf{then}
///     \ \textbf{return}\ M\quad \triangleright\ \text{optimal} \cr
/// 13 & \qquad\quad \textbf{if}\ \delta = z_B \text{ for a } T
///     \text{-blossom } B\ \textbf{then}\ \text{expand } B \cr
/// 14 & \qquad \text{expand the } S\text{-blossoms with } z_B = 0 \cr
/// 15 & \quad \textbf{until}\ \text{a stage does not augment}
/// \end{array}}
/// \f]
/// With `maxCardinality` the first term of \f$ \delta \f$ is left out, and
/// a stage that finds no augmenting path ends the search. The edges of
/// weight at most 0 are left out of the matching of largest weight, which
/// they cannot make heavier.
/// \pre \f$ n \ge 0 \f$, each edge joins two distinct nodes in
///      \f$ [ 0 , n ) \f$, twice a weight in the range of `W`
/// \post no two edges given have an end in common (audit), and, with
///       integer weights, the duals certify the optimum: every slack and
///       dual of a blossom nonnegative, the matched edges tight, the
///       blossoms of positive dual full and the free nodes of dual 0 (audit)
template< class W >
 requires std::is_arithmetic_v< W > && std::is_signed_v< W >
std::pair< W , std::vector< std::size_t > >
maxWeightMatching( int n ,
		   const std::vector< std::tuple< int , int , W > > & E ,
		   bool maxCardinality = false );

/// the largest total weight of a matching of the graph of \f$ n \f$ nodes
/// and of the edges \f$ E = ( u , v , w ) \f$ by a branch and bound, and
/// the places in \f$ E \f$ of its edges, in increasing order
///
/// In pseudocode (a branch and bound): the nodes are decided in increasing
/// order, and a branch is cut when twice its weight plus the best weight at
/// each node left cannot beat twice the best one found, since each edge
/// touches two nodes.
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{search}( u ) \cr
/// 2 & \quad \textbf{while}\ u < n\ \textbf{and}\ \mathit{used}[ u ]
///     \ \textbf{do}\ u \gets u + 1 \cr
/// 3 & \quad \textbf{if}\ u = n\ \textbf{then} \cr
/// 4 & \qquad \textbf{if}\ w( S ) > w( B )\ \textbf{then}\ B \gets S \cr
/// 5 & \qquad \textbf{return} \cr
/// 6 & \quad \textbf{if}\ 2 w( S ) + \sum_{x \ge u} \mathit{top}[ x ] \le 2 w(
///     B )\ \textbf{then return} \cr
/// 7 & \quad \textbf{foreach}\  \text{edge } e = [ u , v ] \text{ with } v >
///     u\ \textbf{and not}\ \mathit{used}[ v ]\ \textbf{do} \cr
/// 8 & \qquad \mathit{used}[ u ] , \mathit{used}[ v ] \gets \textbf{true};\ S
///     \gets S \cup \{ e \};\ \mathrm{search}( u + 1 ) \cr
/// 9 & \qquad S \gets S \setminus \{ e \};\ \mathit{used}[ u ] ,
///     \mathit{used}[ v ] \gets \textbf{false} \cr
/// 10 & \quad \mathrm{search}( u + 1 )\quad \triangleright\ u \text{ left }
///     \text{unmatched}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$, each edge joins two distinct nodes in
///      \f$ [ 0 , n ) \f$
/// \post no two edges given have an end in common (audit)
template< class W >
std::pair< W , std::vector< std::size_t > >
branchAndBoundMatching( int n ,
			const std::vector< std::tuple< int , int , W > > & E );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Matching.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Matching.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Matching.h ---------------------------*/
/*--------------------------------------------------------------------------*/
