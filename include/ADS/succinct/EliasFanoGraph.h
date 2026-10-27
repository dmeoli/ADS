/*--------------------------------------------------------------------------*/
/*-------------------------- File EliasFanoGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A static directed graph whose adjacency lists are coded by Elias-Fano [see
 * `EliasFano.h`] (chapter 15, Compressed Data Structures, of Ferragina's
 * book), the representation of the adjacency lists of large graphs in
 * Facebook's Unicorn and in the quasi-succinct indices of Vigna. The edge
 * \f$ ( u , v ) \f$ of a graph of \f$ n \f$ nodes is the integer
 * \f$ u n + v \f$, so that the lists, each one sorted, are one increasing
 * sequence in \f$ [ 0 , n^2 ) \f$, and the prefix sums of the outdegrees are
 * a second sequence that tells where the list of \f$ u \f$ starts; with
 * \f$ m \f$ edges the graph takes about
 * \f$ m ( 2 + \lceil \log_2 ( n^2 / m ) \rceil ) +
 * n ( 2 + \lceil \log_2 ( m / n ) \rceil ) \f$ bits, plus the directories,
 * and answers in \f$ O( 1 ) \f$ time the outdegree and the \f$ i \f$-th
 * successor of a node, and the existence of an edge by `nextGEQ()` in
 * \f$ O( 1 ) \f$ expected time.
 *
 * The same chapter codes each list on its own, with its own universe: since
 * \f$ \sum_u d_u \log ( n / d_u ) \le m \log ( n^2 / m ) \f$ by the
 * concavity of the logarithm, the lists alone can take fewer bits when the
 * degrees are skewed, at the price of a directory and an offset per list;
 * the single sequence here keeps one directory and the same \f$ O( 1 ) \f$
 * access. Neither exploits the locality and the similarity of the lists of
 * the Web, which the codes of Boldi and Vigna do [see `BVGraph.h`], nor the
 * clustering of the adjacency matrix, which the \f$ k^2 \f$-tree does [see
 * `K2Tree.h`].
 *
 * The contracts [see `Contract.h`]: each list is strictly increasing (an
 * audit) with its nodes in \f$ [ 0 , n ) \f$, \f$ n^2 \f$ fits in 64 bits,
 * and the nodes and the places asked for exist. The functions are defined in
 * `EliasFanoGraph.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see S. Vigna, Quasi-succinct indices, Proc. 6th WSDM, 83-92, 2013,
 *      doi:10.1145/2433396.2433409
 * \see M. Curtiss et al., Unicorn: a system for searching the social graph,
 *      Proc. VLDB Endowment 6(11), 1150-1161, 2013,
 *      doi:10.14778/2536222.2536239
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

#ifndef __ADS_EliasFanoGraph
 #define __ADS_EliasFanoGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS EliasFanoGraph --------------------------*/
/*--------------------------------------------------------------------------*/

/// a static directed graph with its adjacency lists in Elias-Fano
class EliasFanoGraph {

/*--------------------------------------------------------------------------*/
 public:

 /// the graph with no node
 EliasFanoGraph( void ) = default;

 /// the graph of the adjacency lists \f$ a \f$, the node \f$ u \f$ having
 /// the successors \f$ a[ u ] \f$
 ///
 /// In pseudocode (the lists of Elias-Fano of Vigna, beyond the book), as
 /// one sequence of edges and one of starts:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( a ) \cr
 /// 2 & \quad E \gets ();\ S \gets () \cr
 /// 3 & \quad \textbf{for}\ u \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 4 & \qquad \text{append } |E| \text{ to } S \cr
 /// 5 & \qquad \textbf{foreach}\ v \in a[ u ]\ \textbf{do}\ \text{append }
 ///     u n + v \text{ to } E \cr
 /// 6 & \quad \text{append } |E| \text{ to } S \cr
 /// 7 & \quad \textbf{return}\ ( \text{Elias-Fano of } E \text{ in } [ 0 ,
 ///     n^2 ) , \text{Elias-Fano of } S \text{ in } [ 0 , |E| ] )
 /// \end{array}}
 /// \f]
 /// \pre each \f$ a[ u ] \f$ is strictly increasing in \f$ [ 0 , n ) \f$,
 ///      \f$ n^2 < 2^{64} \f$
 explicit EliasFanoGraph(
                       const std::vector< std::vector< std::size_t > > & a );

 /// the number of nodes, \f$ n \f$
 [[nodiscard]] std::size_t order( void ) const;

 /// the number of edges, \f$ m \f$
 [[nodiscard]] std::size_t edges( void ) const;

 /// the number of successors of \f$ u \f$,
 /// \f$ \mathit{start}[ u + 1 ] - \mathit{start}[ u ] \f$
 /// \pre \f$ u < \mathrm{order}() \f$
 [[nodiscard]] std::size_t outdegree( std::size_t u ) const;

 /// the \f$ i \f$-th successor of \f$ u \f$, from 0, i.e.,
 /// \f$ E[ \mathit{start}[ u ] + i ] - u n \f$
 /// \pre \f$ u < \mathrm{order}() \f$ and \f$ i < \mathrm{outdegree}( u ) \f$
 [[nodiscard]] std::size_t successor( std::size_t u , std::size_t i ) const;

 /// the successors of \f$ u \f$, in increasing order, by `successor()`
 /// \pre \f$ u < \mathrm{order}() \f$
 [[nodiscard]] std::vector< std::size_t > successors( std::size_t u ) const;

 /// whether \f$ ( u , v ) \f$ is an edge, i.e., whether \f$ u n + v \f$ is
 /// in the sequence of the edges
 /// \pre \f$ u , v < \mathrm{order}() \f$
 [[nodiscard]] bool hasEdge( std::size_t u , std::size_t v ) const;

 /// the bits of the two sequences, with their directories
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_n = 0;        ///< the number of nodes
 /// \f$ u n + v \f$ for each edge \f$ ( u , v ) \f$
 EliasFano f_edges;
 /// where each list starts, and \f$ m \f$ at the end
 EliasFano f_start;

 };  // end( class EliasFanoGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/EliasFanoGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EliasFanoGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File EliasFanoGraph.h -----------------------*/
/*--------------------------------------------------------------------------*/
