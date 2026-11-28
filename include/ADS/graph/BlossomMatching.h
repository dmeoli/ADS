/*--------------------------------------------------------------------------*/
/*------------------------- File BlossomMatching.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The matchings of a general graph by the blossoms of Edmonds, beyond the
 * book, whose matchings of a bipartite graph and branch and bound are in
 * `Matching.h` [see `Matching.h`, which includes this header]:
 *
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
 *   the optimum appears; the least slack toward each blossom is kept per
 *   blossom, so that a change of the duals costs \f$ O( n ) \f$. With
 *   integer weights all the arithmetic is on integers, the duals being
 *   kept doubled. Among those of largest size it gives, e.g., a perfect
 *   matching of least cost with the costs negated.
 *
 * With `ADS_WITH_LEMON` [see \ref theory_graph_lemon],
 * `maxCardinalityMatching()` and `maxWeightMatching()` (without
 * `maxCardinality`) are delegated to the `MaxMatching` and to the
 * `MaxWeightedMatching` of abLEMON, on a copy of the graph in a
 * `lemon::SmartGraph`, in \f$ \Theta( n + m ) \f$ time and space more, with
 * the same contracts and audits, the matching given being one of the same
 * size or of the same weight, not necessarily the same edges;
 * `ADS_MATCHING_LEMON` defined to 0 keeps the code of ADS, which is
 * always there.
 *
 * The contracts [see `Contract.h`]: an edge joins two distinct nodes of
 * the graph, the graph of `maxCardinalityMatching()` is unoriented; at the
 * level of the audits the matchings given have no two edges with an end in
 * common, and the duals of `maxWeightMatching()` certify its optimum. The
 * functions are defined in `BlossomMatching.tpp`.
 *
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

#ifndef __ADS_BlossomMatching
 #define __ADS_BlossomMatching
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/Visit.h"

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

// the solves from scratch on the matchings of abLEMON with ADS_WITH_LEMON,
// unless ADS_MATCHING_LEMON is 0 (the benchmarks keep the code of ADS)
#if defined( ADS_WITH_LEMON ) && ! defined( ADS_MATCHING_LEMON )
 #define ADS_MATCHING_LEMON 1
#endif
#if defined( ADS_MATCHING_LEMON ) && ADS_MATCHING_LEMON
 #include <lemon/matching.h>
 #include <lemon/smart_graph.h>
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/BlossomMatching.tpp"

/*--------------------------------------------------------------------------*/

#endif /* BlossomMatching.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BlossomMatching.h -----------------------*/
/*--------------------------------------------------------------------------*/
