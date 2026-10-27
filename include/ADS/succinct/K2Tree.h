/*--------------------------------------------------------------------------*/
/*----------------------------- File K2Tree.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The \f$ k^2 \f$-tree of Brisaboa, Ladra and Navarro (chapter 15, Compressed
 * Data Structures, of Ferragina's book): the adjacency matrix of a graph of
 * \f$ n \f$ nodes, padded to the side \f$ N = k^h \ge n \f$, is split into
 * \f$ k^2 \f$ submatrices of side \f$ N / k \f$, each one a child of the root
 * marked 1 if it has an edge and 0 otherwise, and each child marked 1 is split
 * again, down to the cells. The marks are written by levels: \f$ T \f$ has
 * those of all the levels but the last, \f$ L \f$ those of the cells, and with
 * rank on \f$ T \f$ [see `RankSelect.h`] the children of the node marked at
 * the place \f$ p \f$ of \f$ T \f$ start at the place
 * \f$ \mathrm{rank}_1( p + 1 ) \, k^2 \f$ of \f$ T \f$ followed by \f$ L \f$,
 * as in LOUDS [see `LoudsTree.h`]: no pointer is stored.
 *
 * `neighbors( u )` walks down the children that cross the row \f$ u \f$
 * (\f$ k \f$ of the \f$ k^2 \f$ at each level) and `reverseNeighbors( v )`
 * those that cross the column \f$ v \f$, so that the same bits give the
 * successors and the predecessors, which the lists give only by doubling them;
 * `hasEdge( u , v )` follows one path, in \f$ O( h ) \f$ time. A row is read
 * in \f$ O( \sqrt{m} ) \f$ time on average for \f$ m \f$ edges spread
 * uniformly and in \f$ O( n ) \f$ in the worst case, and the tree takes
 * \f$ k^2 m ( \log_{k^2} ( n^2 / m ) + O( 1 ) ) \f$ bits in the worst case
 * (both bounds of Brisaboa, Ladra and Navarro), far fewer when the edges are
 * clustered, as in the Web graphs ordered by URL, where the submatrices of
 * zeros are large. A larger \f$ k \f$ gives fewer levels and faster queries, a
 * smaller one fewer bits; the authors use a larger \f$ k \f$ at the top levels
 * and a smaller one below, and the construction here works from the edges, by
 * levels, in \f$ O( m h ) \f$ time, without the matrix that the recursive
 * construction of the same chapter visits whole.
 *
 * The contracts [see `Contract.h`]: \f$ k \ge 2 \f$, the edges join nodes
 * in \f$ [ 0 , n ) \f$, and the nodes asked for exist; the postcondition of
 * the construction is that the tree has as many cells marked as distinct
 * edges. The functions are defined in `K2Tree.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see N. R. Brisaboa, S. Ladra, G. Navarro, k2-trees for compact web graph
 *      representation, Proc. 16th SPIRE, LNCS 5721, 18-30, 2009,
 *      doi:10.1007/978-3-642-03784-9_3
 * \see N. R. Brisaboa, S. Ladra, G. Navarro, Compact representation of Web
 *      graphs with extended functionality, Information Systems 39, 152-174,
 *      2014, doi:10.1016/j.is.2013.08.003
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

#ifndef __ADS_K2Tree
 #define __ADS_K2Tree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS K2Tree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the adjacency matrix of a static directed graph as a \f$ k^2 \f$-tree
class K2Tree {

/*--------------------------------------------------------------------------*/
 public:

 /// an edge \f$ ( u , v ) \f$, from \f$ u \f$ to \f$ v \f$
 using Edge = std::pair< std::size_t , std::size_t >;

 /// the graph with no node
 K2Tree( void ) = default;

 /// the graph of \f$ n \f$ nodes and the edges \f$ e \f$ (repeated ones
 /// counted once)
 ///
 /// In pseudocode (Brisaboa, Ladra and Navarro, beyond the book), by levels
 /// from the edges, each submatrix holding the run of its edges, which a
 /// counting sort by child splits among its \f$ k^2 \f$ children:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{build}( n , E , k ) \cr
 /// 2 & \quad E \gets \text{the distinct edges, sorted};\ N \gets \text{the }
 ///     \text{least power of } k \text{ not below } \max\{ n , k \} \cr
 /// 3 & \quad \mathit{level} \gets ( \text{the whole matrix with } E );\ s
 ///     \gets N / k;\ T \gets ();\ L \gets () \cr
 /// 4 & \quad \textbf{while}\ \mathit{level} \ne ()\ \textbf{do} \cr
 /// 5 & \qquad \mathit{next} \gets () \cr
 /// 6 & \qquad \textbf{foreach}\ \text{submatrix } x \text{ of }
 ///     \mathit{level} \text{, in order}\ \textbf{do} \cr
 /// 7 & \qquad\quad \text{split the edges of } x \text{ among its } k^2
 ///     \text{ children of side } s \text{, by counting} \cr
 /// 8 & \qquad\quad \textbf{for}\ j \gets 0\ \textbf{to}\ k^2 - 1
 ///     \ \textbf{do} \cr
 /// 9 & \qquad\qquad \beta \gets 1 \text{ if the child } j \text{ has an }
 ///     \text{edge, else } 0 \cr
 /// 10 & \qquad\qquad \textbf{if}\ s = 1\ \textbf{then}\ L \gets L \cdot
 ///     \beta\ \textbf{else}\ T \gets T \cdot \beta \cr
 /// 11 & \qquad\qquad \textbf{if}\ s > 1\ \textbf{and}\ \beta = 1
 ///     \ \textbf{then}\ \text{append the child } j \text{ to }
 ///     \mathit{next} \cr
 /// 12 & \qquad \mathit{level} \gets \mathit{next};\ s \gets s / k \cr
 /// 13 & \quad \textbf{return}\ ( T , L )
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \ge 2 \f$ and the nodes of the edges in \f$ [ 0 , n ) \f$
 /// \post the cells marked are the distinct edges
 K2Tree( std::size_t n , const std::vector< Edge > & e , std::size_t k = 2 );

 /// the number of nodes, \f$ n \f$
 [[nodiscard]] std::size_t order( void ) const;

 /// the number of distinct edges
 [[nodiscard]] std::size_t edges( void ) const;

 /// the successors of \f$ u \f$, in increasing order
 ///
 /// In pseudocode (Brisaboa, Ladra and Navarro, beyond the book), with
 /// \f$ \mathit{mark}( p ) \f$ the bit \f$ p \f$ of \f$ T \f$ followed by
 /// \f$ L \f$, and the children of the node marked at \f$ p \f$ from the
 /// place \f$ \mathrm{rank}_1( p + 1 ) \, k^2 \f$, the root's from 0:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{neighbors}( u ) \cr
 /// 2 & \quad \mathit{out} \gets ();\ \mathrm{walk}( N , u , 0 , 0 );
 ///     \ \textbf{return}\ \mathit{out} \cr
 /// 3 & \textbf{procedure}\ \mathrm{walk}( s , r , o , \mathit{base} ) \cr
 /// 4 & \quad c \gets s / k\quad \triangleright\ \text{the side of a child}
 ///     \cr
 /// 5 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}\quad
 ///     \triangleright\ \text{the children that cross the row } r \cr
 /// 6 & \qquad p \gets \mathit{base} + \lfloor r / c \rfloor k + j \cr
 /// 7 & \qquad \textbf{if}\ \mathit{mark}( p ) = 1\ \textbf{then} \cr
 /// 8 & \qquad\quad \textbf{if}\ c = 1\ \textbf{then}\ \text{append } o + j
 ///     \text{ to } \mathit{out} \cr
 /// 9 & \qquad\quad \textbf{else}\ \mathrm{walk}( c , r \bmod c , o + j c
 ///     , \mathrm{rank}_1( p + 1 ) \, k^2 )
 /// \end{array}}
 /// \f]
 /// \pre \f$ u < \mathrm{order}() \f$
 [[nodiscard]] std::vector< std::size_t > neighbors( std::size_t u ) const;

 /// the predecessors of \f$ v \f$, in increasing order: the walk of
 /// `neighbors()` on the children that cross the column \f$ v \f$, the
 /// place \f$ \mathit{base} + j k + \lfloor r / c \rfloor \f$ in its line 6
 /// \pre \f$ v < \mathrm{order}() \f$
 [[nodiscard]] std::vector< std::size_t > reverseNeighbors(
                                                       std::size_t v ) const;

 /// whether \f$ ( u , v ) \f$ is an edge
 ///
 /// In pseudocode (Brisaboa, Ladra and Navarro, beyond the book), one path
 /// down, in \f$ O( h ) \f$ time:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{hasEdge}( u , v ) \cr
 /// 2 & \quad s \gets N;\ \mathit{base} \gets 0 \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad c \gets s / k;\ p \gets \mathit{base} + \lfloor u / c \rfloor
 ///     k + \lfloor v / c \rfloor \cr
 /// 5 & \qquad \textbf{if}\ \mathit{mark}( p ) = 0
 ///     \ \textbf{then return false} \cr
 /// 6 & \qquad \textbf{if}\ c = 1\ \textbf{then return true} \cr
 /// 7 & \qquad \mathit{base} \gets \mathrm{rank}_1( p + 1 ) \, k^2;\ u
 ///     \gets u \bmod c;\ v \gets v \bmod c;\ s \gets c
 /// \end{array}}
 /// \f]
 /// \pre \f$ u , v < \mathrm{order}() \f$
 [[nodiscard]] bool hasEdge( std::size_t u , std::size_t v ) const;

 /// \f$ T \f$, the marks of the levels but the last
 [[nodiscard]] const BitVector & levels( void ) const;

 /// \f$ L \f$, the marks of the cells
 [[nodiscard]] const BitVector & leaves( void ) const;

 /// the bits of \f$ T \f$, \f$ L \f$ and the rank directory of \f$ T \f$
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the mark at the place \f$ p \f$ of \f$ T \f$ followed by \f$ L \f$
 [[nodiscard]] bool mark( std::size_t p ) const;

 /// the place of the first child of the node marked at \f$ p \f$, 0 for the
 /// root
 [[nodiscard]] std::size_t children( std::size_t p ) const;

 /// the cells of the row (or the column, if not `row`) \f$ r \f$ of the
 /// submatrix of side \f$ s \f$ whose children start at `base`, offset by
 /// \f$ o \f$
 void walk( std::size_t s , std::size_t r , std::size_t o ,
            std::size_t base , bool row ,
            std::vector< std::size_t > & out ) const;

 std::size_t f_n = 0;         ///< the number of nodes
 std::size_t f_k = 2;         ///< the arity \f$ k \f$
 std::size_t f_side = 1;      ///< \f$ N = k^h \f$, the side of the matrix
 std::size_t f_m = 0;         ///< the number of distinct edges
 RankSelect f_t;              ///< \f$ T \f$
 BitVector f_l;               ///< \f$ L \f$

 };  // end( class K2Tree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/K2Tree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* K2Tree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File K2Tree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
