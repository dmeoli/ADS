/*--------------------------------------------------------------------------*/
/*----------------------------- File BVGraph.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compression of the adjacency lists of Boldi and Vigna (chapter 15,
 * Compressed Data Structures, of Ferragina's book), designed for the Web
 * graphs, whose nodes are the pages sorted by URL: <em>locality</em> (most
 * links stay in the host, so that the successors of \f$ x \f$ are close to
 * \f$ x \f$ and to each other) and <em>similarity</em> (the pages of a host
 * link to the same pages). The list of \f$ x \f$ is written as:
 *
 * - its outdegree, in \f$ \gamma \f$ [see `IntegerCodes.h`];
 * - the <em>reference</em> \f$ r \f$: the list of \f$ x - r \f$, among the
 *   previous \f$ W \f$ lists (the <em>window</em>), that shares the most
 *   successors with that of \f$ x \f$, 0 for none;
 * - the <em>copy list</em> of the reference, a bit per successor of
 *   \f$ x - r \f$ telling whether it is a successor of \f$ x \f$ too,
 *   written as <em>copy blocks</em>: its first bit, then the lengths less 1
 *   of its runs, the last one omitted since the outdegree of the reference
 *   tells it;
 * - the <em>extra nodes</em>, the successors not copied: the runs of at
 *   least \f$ L_{\min} \f$ consecutive integers as intervals, the left
 *   extreme as a gap from the previous right extreme less 2 (the first one
 *   from \f$ x \f$, by `signedToNatural()`) and the length less
 *   \f$ L_{\min} \f$, then the <em>residuals</em> as gaps less 1 (the first
 *   one from \f$ x \f$, by `signedToNatural()`), in \f$ \delta \f$.
 *
 * A list is decoded from its place in the stream, kept for every node in an
 * Elias-Fano sequence [see `EliasFano.h`], after the list of its reference;
 * the chains of references are at most \f$ R \f$ long, so that a list is
 * decoded in \f$ O( R d ) \f$ time for outdegrees up to \f$ d \f$. With the
 * window \f$ W = 7 \f$, the chains \f$ R = 3 \f$ and \f$ L_{\min} = 4 \f$
 * (the defaults of the WebGraph framework) the Web graphs take about 3 bits
 * per edge, against the \f$ \log_2 n \f$ bits of a plain list and the
 * \f$ 2 + \log ( n^2 / m ) \f$ of Elias-Fano [see `EliasFanoGraph.h`]; the
 * \f$ k^2 \f$-tree [see `K2Tree.h`] is comparable in space and also gives
 * the predecessors. The WebGraph framework codes the gaps by the
 * \f$ \zeta \f$ codes, tuned to their power-law distribution, and reorders
 * the nodes (e.g., by layered label propagation) to make locality stronger;
 * neither is in the same chapter.
 *
 * `copyList()`, `copyBlocks()`, `intervals()` and `residualGaps()` are the
 * steps of the coding, given for the examples of the same chapter.
 *
 * The contracts [see `Contract.h`]: each list is strictly increasing (an
 * audit) with its nodes in \f$ [ 0 , n ) \f$, the window and the chains are
 * at least 1 and \f$ L_{\min} \ge 2 \f$; the audit of the construction is
 * that each list is decoded back. The functions are defined in
 * `BVGraph.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see P. Boldi, S. Vigna, The WebGraph framework I: compression
 *      techniques, Proc. 13th WWW, 595-602, 2004,
 *      doi:10.1145/988672.988752
 * \see P. Boldi, M. Rosa, M. Santini, S. Vigna, Layered label propagation:
 *      a multiresolution coordinate-free ordering for compressing social
 *      networks, Proc. 20th WWW, 587-596, 2011,
 *      doi:10.1145/1963405.1963488
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

#ifndef __ADS_BVGraph
 #define __ADS_BVGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/IntegerCodes.h"
#include "ADS/succinct/BitStream.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BVGraph ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a static directed graph with the lists compressed by reference, copy
/// blocks, intervals and gaps
class BVGraph {

/*--------------------------------------------------------------------------*/
 public:

 /// an adjacency list, strictly increasing
 using List = std::vector< std::size_t >;

 /// an interval of extra nodes, its left extreme and its length
 using Interval = std::pair< std::size_t , std::size_t >;

 /// the graph with no node
 BVGraph( void ) = default;

 /// the graph of the adjacency lists \f$ a \f$, with the window \f$ w \f$,
 /// chains of references at most \f$ r \f$ long and intervals at least
 /// \f$ \mathit{lmin} \f$ long
 ///
 /// In pseudocode (Boldi and Vigna, beyond the book), the list \f$ S \f$
 /// of each node \f$ x \f$ in turn, \f$ \gamma( v ) \f$ and
 /// \f$ \delta( v ) \f$ the codes of \f$ v + 1 \f$, \f$ \zeta( v ) \f$ for
 /// `signedToNatural()`, and \f$ \mathit{chain}[ x ] \f$ the length of the
 /// chain of references of \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{encode}( x , S ) \cr
 /// 2 & \quad \mathit{start}[ x ] \gets \text{the bits written so far};
 ///     \ \text{write } \gamma( |S| ) \cr
 /// 3 & \quad \textbf{if}\ S = ()\ \textbf{then return} \cr
 /// 4 & \quad b \gets \text{the } k \in [ 1 , \min\{ w , x \} ] \text{ with }
 ///     \mathit{chain}[ x - k ] < r \text{ and the most items of } a[ x - k
 ///     ] \text{ in } S \text{, 0 if none} \cr
 /// 5 & \quad \text{write } \gamma( b );\ E \gets S \cr
 /// 6 & \quad \textbf{if}\ b > 0\ \textbf{then} \cr
 /// 7 & \qquad ( f , \mathit{len} ) \gets \mathrm{copyBlocks}(
 ///     \mathrm{copyList}( S , a[ x - b ] ) ) \cr
 /// 8 & \qquad \text{write } \gamma( |\mathit{len}| ) \text{, the bit } f
 ///     \text{, and } \gamma( l ) \text{ for each } l \in \mathit{len} \cr
 /// 9 & \qquad E \gets S \setminus a[ x - b ];\ \mathit{chain}[ x ] \gets
 ///     \mathit{chain}[ x - b ] + 1 \cr
 /// 10 & \quad ( I , \mathit{res} ) \gets \mathrm{intervals}( E ,
 ///     \mathit{lmin} );\ \text{write } \gamma( |I| ) \cr
 /// 11 & \quad \textbf{foreach}\ ( \mathit{left} , \mathit{len} ) \in I
 ///     \ \textbf{do} \cr
 /// 12 & \qquad \text{write } \gamma( \zeta( \mathit{left} - x ) )
 ///     \text{ for the first, } \gamma( \mathit{left} - \mathit{right} - 2
 ///     ) \text{ for the others} \cr
 /// 13 & \qquad \text{write } \gamma( \mathit{len} - \mathit{lmin} );
 ///     \ \mathit{right} \gets \mathit{left} + \mathit{len} - 1 \cr
 /// 14 & \quad \textbf{foreach}\ g \in \mathrm{residualGaps}( \mathit{res} ,
 ///     x )\ \textbf{do}\ \text{write } \delta( g )
 /// \end{array}}
 /// \f]
 /// \pre each \f$ a[ u ] \f$ strictly increasing in \f$ [ 0 , n ) \f$,
 ///      \f$ w , r \ge 1 \f$ and \f$ \mathit{lmin} \ge 2 \f$
 explicit BVGraph( const std::vector< List > & a , std::size_t w = 7 ,
                   std::size_t r = 3 , std::size_t lmin = 4 );

 /// the number of nodes, \f$ n \f$
 [[nodiscard]] std::size_t order( void ) const;

 /// the number of edges, \f$ m \f$
 [[nodiscard]] std::size_t edges( void ) const;

 /// the number of successors of \f$ x \f$, the first code of its list
 /// \pre \f$ x < \mathrm{order}() \f$
 [[nodiscard]] std::size_t outdegree( std::size_t x ) const;

 /// the successors of \f$ x \f$, in increasing order
 ///
 /// In pseudocode (Boldi and Vigna, beyond the book), reading the codes
 /// that `BVGraph()` writes, from \f$ \mathit{start}[ x ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{successors}( x ) \cr
 /// 2 & \quad d \gets \text{the outdegree};\ \textbf{if}\ d = 0
 ///     \ \textbf{then return}\ () \cr
 /// 3 & \quad S \gets ();\ b \gets \text{the reference} \cr
 /// 4 & \quad \textbf{if}\ b > 0\ \textbf{then} \cr
 /// 5 & \qquad T \gets \mathrm{successors}( x - b );\ f \gets \text{the }
 ///     \text{first bit} \cr
 /// 6 & \qquad \textbf{foreach}\ \text{block, of the length read or of the }
 ///     \text{rest of } T \text{ for the last}\ \textbf{do} \cr
 /// 7 & \qquad\quad \textbf{if}\ f = 1\ \textbf{then}\ \text{append the }
 ///     \text{next items of } T \text{ to } S\ \textbf{else}\ \text{skip }
 ///     \text{them} \cr
 /// 8 & \qquad\quad f \gets 1 - f \cr
 /// 9 & \quad \textbf{foreach}\ \text{interval read}\ \textbf{do}
 ///     \ \text{append its integers to } S \cr
 /// 10 & \quad \textbf{while}\ |S| < d\ \textbf{do}\ \text{append the next }
 ///     \text{residual, from its gap, to } S \cr
 /// 11 & \quad \textbf{return}\ S \text{ sorted}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < \mathrm{order}() \f$
 [[nodiscard]] List successors( std::size_t x ) const;

 /// the reference of \f$ x \f$, 0 for none
 /// \pre \f$ x < \mathrm{order}() \f$
 [[nodiscard]] std::size_t reference( std::size_t x ) const;

 /// the bits of the lists
 [[nodiscard]] std::size_t codeBits( void ) const;

 /// the bits of the lists and of their places
 [[nodiscard]] std::size_t sizeInBits( void ) const;

 /// the copy list of \f$ s \f$ against the reference list `ref`: a bit per
 /// item of `ref`, 1 if it is in \f$ s \f$ (found by a binary search)
 [[nodiscard]] static BitVector copyList( const List & s , const List & ref );

 /// the copy blocks of the copy list \f$ c \f$: its first bit and the
 /// lengths less 1 of its runs but the last
 ///
 /// In pseudocode (Boldi and Vigna, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{copyBlocks}( c ) \cr
 /// 2 & \quad \textbf{if}\ c = ()\ \textbf{then return}\ ( 0 , () ) \cr
 /// 3 & \quad \mathit{len} \gets ();\ \mathit{run} \gets 1 \cr
 /// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ |c| - 1\ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ c[ i ] = c[ i - 1 ]\ \textbf{then}
 ///     \ \mathit{run} \gets \mathit{run} + 1 \cr
 /// 6 & \qquad \textbf{else}\ \text{append } \mathit{run} - 1 \text{ to }
 ///     \mathit{len};\ \mathit{run} \gets 1 \cr
 /// 7 & \quad \textbf{return}\ ( c[ 0 ] , \mathit{len} )\quad
 ///     \triangleright\ \text{the last run omitted}
 /// \end{array}}
 /// \f]
 [[nodiscard]] static std::pair< bool , std::vector< std::size_t > >
 copyBlocks( const BitVector & c );

 /// the runs of at least \f$ \mathit{lmin} \f$ consecutive integers of the
 /// strictly increasing \f$ e \f$, and the residuals
 ///
 /// In pseudocode (Boldi and Vigna, beyond the book), by maximal runs:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{intervals}( e , \mathit{lmin} ) \cr
 /// 2 & \quad I \gets ();\ \mathit{res} \gets ();\ i \gets 0 \cr
 /// 3 & \quad \textbf{while}\ i < |e|\ \textbf{do} \cr
 /// 4 & \qquad j \gets i + 1;\ \textbf{while}\ j < |e|\ \textbf{and}\ e[ j ]
 ///     = e[ j - 1 ] + 1\ \textbf{do}\ j \gets j + 1 \cr
 /// 5 & \qquad \textbf{if}\ j - i \ge \mathit{lmin}\ \textbf{then}
 ///     \ \text{append } ( e[ i ] , j - i ) \text{ to } I \cr
 /// 6 & \qquad \textbf{else}\ \text{append } e[ i .. j - 1 ] \text{ to }
 ///     \mathit{res} \cr
 /// 7 & \qquad i \gets j \cr
 /// 8 & \quad \textbf{return}\ ( I , \mathit{res} )
 /// \end{array}}
 /// \f]
 [[nodiscard]] static std::pair< std::vector< Interval > , List >
 intervals( const List & e , std::size_t lmin );

 /// the residuals `res` of \f$ x \f$ as written: the first as
 /// `signedToNatural()` of its difference from \f$ x \f$, the others as the
 /// gaps less 1, \f$ \mathit{res}[ i ] - \mathit{res}[ i - 1 ] - 1 \f$
 [[nodiscard]] static std::vector< std::uint64_t > residualGaps(
                                         const List & res , std::size_t x );

/*--------------------------------------------------------------------------*/
 private:

 /// the list of \f$ x \f$, decoded from the stream after that of its
 /// reference
 [[nodiscard]] List decode( std::size_t x ) const;

 std::size_t f_n = 0;        ///< the number of nodes
 std::size_t f_m = 0;        ///< the number of edges
 std::size_t f_lmin = 4;     ///< the least length of an interval
 BitVector f_bits;           ///< the lists, one after the other
 EliasFano f_start;          ///< the place of each list, and the end

 };  // end( class BVGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/BVGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BVGraph.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File BVGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
