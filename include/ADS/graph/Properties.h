/*--------------------------------------------------------------------------*/
/*--------------------------- File Properties.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Properties of a GRAPH and of its trees, decided or measured by the
 * visits of section 9.5:
 *
 * - isTree(), whether an unoriented graph is a free tree: connected and
 *   with n - 1 edges, by a visit from any node, \f$ O( n + m ) \f$;
 * - diameter(), the largest distance in edges between two nodes, by a visit in
 *   breadth from each node, \f$ O( n ( n + m ) ) \f$; for an unoriented graph
 *   the matrix product of Seidel gives all the distances in
 *   \f$ O( n^{\omega} \log n ) \f$, iFUB of Crescenzi et al. finds the
 *   diameter of most real graphs with few visits, and on an unoriented graph
 *   one visit from any node u gives it within a factor 2, the eccentricity e(
 *   u ) <= D <= 2 e( u );
 * - treeDiameter(), the diameter of a free tree by two visits, \f$ O( n ) \f$:
 *   the node farthest from any node is an end of a longest path, and the node
 *   farthest from it is the other end (the double sweep, proved by Bulterman
 *   et al.); and that of a binary tree with its height, in a postvisit: the
 *   longest path within a subtree or through the root, ( hl + 1 ) + ( hr + 1
 *   ), \f$ O( n ) \f$;
 * - countTriangles(), the triangles of an unoriented graph by the ordering
 *   of Chiba and Nishizeki: each edge goes from the end of smaller degree
 *   (ties by name) to the other, and each triangle is counted once, at its
 *   first node, as an edge between two of its out-neighbours. Each node
 *   has \f$ O( \sqrt{m} ) \f$ out-neighbours, so the time is
 *   \f$ O( m \sqrt{m} ) \f$, against \f$ O( m n ) \f$ for the merge of the
 *   lists of the ends of each edge and \f$ \Theta( n^3 ) \f$ on the matrix;
 *   the product of matrices of Itai and Rodeh takes
 *   \f$ O( n^{\omega} ) \f$;
 * - twoColoring(), a colouring 0 / 1 of the nodes of an unoriented graph with
 *   the ends of each edge apart, or nothing if there is none: by the theorem
 *   of Koenig, if and only if there is no odd cycle, which the visit in
 *   breadth from each node not coloured finds as an edge between two nodes of
 *   the same level parity, \f$ O( n + m ) \f$ (the twoColoring() of
 *   EasyCases.h takes a matrix of adjacency, in \f$ \Theta( n^2 ) \f$);
 * - universalSink(), the node of an oriented graph with an edge from every
 *   other node and no edge out, NIL if there is none, from the matrix of
 *   adjacency in \f$ \Theta( n ) \f$ entries read out of the
 *   \f$ n^2 \f$, by the elimination of King and Smith-Thomas: the entry
 *   ( c , j ) rules out c (an edge out) or j (an edge missing in), so one
 *   scan leaves one candidate, checked on its row and its column, at most
 *   3 ( n - 1 ) entries;
 * - uniqueTopologicalOrder(), the topological order of an oriented graph
 *   if it is the only one, nothing if there is a cycle or more orders: by
 *   the algorithm of Kahn, the order is unique if and only if the set of
 *   the nodes with no entering edge never has two of them, i.e., if and
 *   only if every two consecutive nodes of the order are joined by an
 *   edge, a Hamiltonian path, \f$ O( n + m ) \f$;
 * - treeVertexCover(), a minimum vertex cover of a forest, the fewest nodes
 *   that touch every edge: by the greedy on the leaves, the parent of a leaf
 *   taken (a cover has the leaf or its parent, and the parent covers more), on
 *   the nodes in the reverse order of a visit in breadth, \f$ O( n ) \f$. The
 *   complement is a maximum independent set. A vertex cover is NP-hard on
 *   general graphs (chapter 18), and on a tree with weights on the nodes a
 *   dynamic programming on the subtrees, with the root taken or not, solves it
 *   in \f$ O( n ) \f$.
 *
 * The contracts [see Contract.h]: isTree(), countTriangles(), twoColoring()
 * and treeVertexCover() need an unoriented graph, the last one acyclic
 * (checked by its visit); diameter() a node and every node reaching all the
 * others, treeDiameter() a free tree; uniqueTopologicalOrder() an oriented
 * graph; universalSink() a square matrix. At the level of the audits,
 * twoColoring() ensures that its colouring is proper, uniqueTopologicalOrder()
 * that its order is topological, and treeVertexCover() that its nodes
 * cover the edges. The functions are defined in Properties.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 9.5 and
 *      18, https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf
 * \see D. Konig, Grafok es matrixok, Matematikai es Fizikai Lapok 38,
 *      116-119, 1931
 * \see R. Seidel, On the all-pairs-shortest-path problem in unweighted
 *      undirected graphs, Journal of Computer and System Sciences 51(3),
 *      400-403, 1995, doi:10.1006/jcss.1995.1078
 * \see P. Crescenzi, R. Grossi, M. Habib, L. Lanzi, A. Marino, On computing
 *      the diameter of real-world undirected graphs, Theoretical Computer
 *      Science 514, 84-95, 2013, doi:10.1016/j.tcs.2012.09.018
 * \see R. W. Bulterman, F. W. van der Sommen, G. Zwaan, T. Verhoeff,
 *      A. J. M. van Gasteren, W. H. J. Feijen, On computing a longest path
 *      in a tree, Information Processing Letters 81(2), 93-96, 2002,
 *      doi:10.1016/S0020-0190(01)00198-3
 * \see N. Chiba, T. Nishizeki, Arboricity and subgraph listing algorithms,
 *      SIAM Journal on Computing 14(1), 210-223, 1985, doi:10.1137/0214017
 * \see A. Itai, M. Rodeh, Finding a minimum circuit in a graph, SIAM
 *      Journal on Computing 7(4), 413-423, 1978, doi:10.1137/0207033
 * \see K. N. King, B. Smith-Thomas, An optimal algorithm for sink-finding,
 *      Information Processing Letters 14(3), 109-111, 1982,
 *      doi:10.1016/0020-0190(82)90065-6
 * \see A. B. Kahn, Topological sorting of large networks, Communications of
 *      the ACM 5(11), 558-562, 1962, doi:10.1145/368996.369025
 * \see R. M. Karp, Reducibility among combinatorial problems, Complexity of
 *      Computer Computations, 85-103, 1972, doi:10.1007/978-1-4684-2001-2_9
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

#ifndef __ADS_Properties
 #define __ADS_Properties
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"
#include "ADS/graph/Visit.h"
#include "ADS/tree/BinaryTree.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------------- TREES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// isTree(): whether the unoriented graph is a free tree, connected and
/// with n - 1 edges
/// \pre the graph is unoriented, with a node
template< Graph G >
bool isTree( const G & g );

/// diameter(): the largest distance in edges between two nodes
/// \pre the graph has a node, and every node reaches all the others
template< Graph G >
int diameter( const G & g );

/// treeDiameter(): the edges of a longest path of the free tree
/// \pre the graph is a free tree
template< Graph G >
int treeDiameter( const G & g );

/// treeDiameter() of a binary tree: its height and the edges of its
/// longest path, ( -1 , 0 ) for the empty tree
template< class T >
std::pair< int , int > treeDiameter( const BinaryTree< T > * t );

/// treeVertexCover(): the nodes of a minimum vertex cover of the forest,
/// in increasing order
/// \pre the graph is unoriented and acyclic
/// \post each edge has an end in the cover (audit)
template< Graph G >
std::vector< int > treeVertexCover( const G & g );

/*--------------------------------------------------------------------------*/
/*------------------------------- STRUCTURE --------------------------------*/
/*--------------------------------------------------------------------------*/

/// countTriangles(): the triangles of the unoriented graph
/// \pre the graph is unoriented
template< Graph G >
std::int64_t countTriangles( const G & g );

/// twoColoring(): a colour 0 or 1 for each node, the ends of each edge
/// apart, -1 on the names that are not nodes; nothing if there is an odd
/// cycle
/// \pre the graph is unoriented
/// \post the ends of each edge have different colours (audit)
template< Graph G >
std::optional< std::vector< int > > twoColoring( const G & g );

/// universalSink(): the node with an edge from every other one and no
/// edge out, NIL if there is none, from the matrix of adjacency A, whose
/// entries convert to bool (the diagonal is not read)
/// \pre A is square
template< class T >
int universalSink( const std::vector< std::vector< T > > & A );

/// uniqueTopologicalOrder(): the topological order of the oriented graph
/// if it is the only one, nothing if it has a cycle or more orders
/// \pre the graph is oriented
/// \post an order given is topological (audit)
template< Graph G >
std::optional< std::vector< int > > uniqueTopologicalOrder( const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Properties.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Properties.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Properties.h --------------------------*/
/*--------------------------------------------------------------------------*/
