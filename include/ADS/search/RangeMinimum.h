/*--------------------------------------------------------------------------*/
/*-------------------------- File RangeMinimum.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range minimum query, beyond the book: given an array \f$ A \f$ of
 * \f$ n \f$ items of an ordered universe, the place of a minimum of
 * \f$ A[ i .. j ] \f$ for any \f$ i \le j \f$, the leftmost one when the
 * minimum repeats. The problem is the backbone of the longest common
 * extensions of a text [see `SuffixArray.h`] and of the lowest common
 * ancestors of a tree [see `SuffixTree.h`].
 *
 * - `SparseTable` stores, for every place \f$ i \f$ and every \f$ L \f$, the
 *   place of the minimum of \f$ A[ i .. i + 2^L - 1 ] \f$, computed from the
 *   two halves of the range; a query covers \f$ [ i , j ] \f$ by the two
 *   (possibly overlapping) ranges of length \f$ 2^L \f$,
 *   \f$ L = \lfloor \log_2( j - i + 1 ) \rfloor \f$, that start at \f$ i \f$
 *   and end at \f$ j \f$. It takes \f$ O( n \log n ) \f$ time and space and
 *   answers in \f$ O( 1 ) \f$ time, against the \f$ O( n^2 ) \f$ of the table
 *   of all the answers;
 * - `cartesianTree()` builds the Cartesian tree of \f$ A \f$ with a stack in
 *   \f$ O( n ) \f$: its root is the leftmost minimum of \f$ A \f$, its left
 *   and right subtrees the Cartesian trees of the parts before and after it;
 *   a range of \f$ A \f$ is a subtree, and the minimum of
 *   \f$ A[ i .. j ] \f$ is the lowest common ancestor of \f$ i \f$ and
 *   \f$ j \f$;
 * - `PlusMinusOneRangeMinimum` answers the queries on an array whose
 *   adjacent items differ by exactly 1, as the depths along an Euler tour of
 *   a tree: the array is cut into blocks of
 *   \f$ d = \max\{ 1 , \lfloor \log_2 n \rfloor / 2 \} \f$ items, a sparse
 *   table on the minima of the blocks answers for the blocks covered
 *   entirely, and a block is described by the \f$ d - 1 \f$ signs of its
 *   differences, so that a table of the \f$ 2^{d-1} \le \sqrt{n} \f$
 *   signatures times the \f$ d^2 \f$ pairs of offsets (the trick of the Four
 *   Russians) answers inside a block: \f$ O( n ) \f$ time and space,
 *   \f$ O( 1 ) \f$ time per query;
 * - `RangeMinimum` is the solution of Bender and Farach-Colton for any
 *   array: the minimum of \f$ A[ i .. j ] \f$ is the lowest common ancestor
 *   of \f$ i \f$ and \f$ j \f$ in the Cartesian tree, i.e., the node of least
 *   depth on the Euler tour of the tree between the first visits of
 *   \f$ i \f$ and \f$ j \f$, a query on the \f$ 2 n - 1 \f$ depths of the
 *   tour, which differ by 1. Its time and space are \f$ O( n ) \f$,
 *   \f$ O( 1 ) \f$ time per query (Searching Strings by Substring, chapter
 *   10 of Ferragina), and the same reduction answers the lowest common
 *   ancestors of any tree (ibid.).
 *
 * The bound \f$ O( n ) \f$ of Bender and Farach-Colton was first reached by
 * Harel and Tarjan on the lowest common ancestors, with a far more involved
 * structure; Fischer and Heun need only \f$ 2 n + o( n ) \f$ bits besides
 * \f$ A \f$, by describing each block by its own Cartesian tree, which
 * removes the Euler tour and its factor 2. The sparse table is the simplest
 * and, on the arrays that fit in a cache, often the fastest in practice.
 *
 * The contracts [see `Contract.h`]: a query asks for \f$ i \le j < n \f$, and
 * its answer is a leftmost minimum of the range (an audit, linear in the
 * range); the items of a `PlusMinusOneRangeMinimum` differ by 1. The
 * functions are defined in `RangeMinimum.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10 (Searching Strings by Substring),
 *      doi:10.1017/9781009128933
 * \see M. A. Bender, M. Farach-Colton, The LCA problem revisited,
 *      Proceedings of the 4th Latin American Symposium on Theoretical
 *      Informatics (LATIN), LNCS 1776, 88-94, 2000,
 *      doi:10.1007/10719839_9
 * \see D. Harel, R. E. Tarjan, Fast algorithms for finding nearest common
 *      ancestors, SIAM Journal on Computing 13(2), 338-355, 1984,
 *      doi:10.1137/0213024
 * \see J. Fischer, V. Heun, Space-efficient preprocessing schemes for range
 *      minimum queries on static arrays, SIAM Journal on Computing 40(2),
 *      465-492, 2011, doi:10.1137/090779759
 * \see J. Vuillemin, A unifying look at data structures, Communications of
 *      the ACM 23(4), 229-239, 1980, doi:10.1145/358841.358852
 * \see V. L. Arlazarov, E. A. Dinic, M. A. Kronrod, I. A. Faradzev, On
 *      economical construction of the transitive closure of an oriented
 *      graph, Soviet Mathematics Doklady 11, 1209-1210, 1970 (the trick of
 *      the Four Russians)
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

#ifndef __ADS_RangeMinimum
 #define __ADS_RangeMinimum
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the parent of each item of \f$ A \f$ in its Cartesian tree, the root (the
/// leftmost minimum) having \f$ n \f$; equal items go to the right
///
/// In pseudocode (Vuillemin, beyond the book), the stack \f$ S \f$ holding
/// the right spine of the tree of \f$ A[ 0 .. i - 1 ] \f$, increasing:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{cartesianTree}( A ) \cr
/// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ P[ i ] \gets n \cr
/// 3 & \quad S \gets \text{an empty stack} \cr
/// 4 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 5 & \qquad \ell \gets \textbf{nil} \cr
/// 6 & \qquad \textbf{while}\ S \ne \emptyset\ \textbf{and}\ A[ i ] < A[
///     \mathrm{top}( S ) ]\ \textbf{do}\ \ell \gets \mathrm{pop}( S ) \cr
/// 7 & \qquad \textbf{if}\ \ell \ne \textbf{nil}\ \textbf{then}\ P[ \ell ]
///     \gets i\quad \triangleright\ \text{the left child of } i \cr
/// 8 & \qquad \textbf{if}\ S \ne \emptyset\ \textbf{then}\ P[ i ] \gets
///     \mathrm{top}( S ) \cr
/// 9 & \qquad \mathrm{push}( S , i ) \cr
/// 10 & \quad \textbf{return}\ P
/// \end{array}}
/// \f]
/// \post a heap: no item is smaller than its parent (audit)
template< class T , class Compare = std::less< T > >
[[nodiscard]] std::vector< std::size_t > cartesianTree(
                         const std::vector< T > & A , Compare less = {} );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS SparseTable ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the range minima of an array by the sparse table
template< class T , class Compare = std::less< T > >
class SparseTable {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty table
 SparseTable( void ) = default;

 /// the table of \f$ A \f$, \f$ O( n \log n ) \f$
 ///
 /// In pseudocode (Bender and Farach-Colton, beyond the book),
 /// \f$ M[ L ][ i ] \f$ the place of the leftmost minimum of
 /// \f$ A[ i .. i + 2^L - 1 ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( A ) \cr
 /// 2 & \quad \textbf{foreach}\ i\ \textbf{do}\ M[ 0 ][ i ] \gets i \cr
 /// 3 & \quad \textbf{for}\ L \gets 1\ \textbf{to}\ \lfloor \log_2 n \rfloor
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 2^L\ \textbf{do} \cr
 /// 5 & \qquad\quad M[ L ][ i ] \gets \text{the better of } M[ L - 1 ][ i ]
 ///     \text{ and } M[ L - 1 ][ i + 2^{L-1} ]
 /// \end{array}}
 /// \f]
 /// where the better place is the one of the smaller item, the left one on
 /// a tie.
 explicit SparseTable( std::vector< T > A , Compare less = {} );

 /// the place of the leftmost minimum of \f$ A[ i .. j ] \f$,
 /// \f$ O( 1 ) \f$
 ///
 /// In pseudocode (Bender and Farach-Colton, beyond the book), two ranges
 /// of length \f$ 2^L \f$ covering \f$ [ i , j ] \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{query}( i , j ) \cr
 /// 2 & \quad L \gets \lfloor \log_2 ( j - i + 1 ) \rfloor \cr
 /// 3 & \quad \textbf{return}\ \text{the better of } M[ L ][ i ] \text{ and }
 ///     M[ L ][ j - 2^L + 1 ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j < n \f$
 /// \post a leftmost minimum of \f$ A[ i .. j ] \f$ (audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// the item of place \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the array, random access

 using value_type = T;                   ///< the type of the items
 using size_type = std::size_t;          ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using const_reference = const T &;      ///< a reference to an item
 using reference = const_reference;      ///< the same, constant
 using const_iterator = typename std::vector< T >::const_iterator;
                                         ///< an iterator on the items
 using iterator = const_iterator;        ///< the same, constant

 /// the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.begin() ); }
 /// past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_A.end() ); }
 /// the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the one of \f$ a < b \f$ with the smaller item, \f$ a \f$ on a tie
 [[nodiscard]] std::size_t better( std::size_t a , std::size_t b ) const;

 std::vector< T > f_A;                           ///< the items
 std::vector< std::vector< std::size_t > > f_M;  ///< \f$ M[ L ][ i ] \f$
 Compare f_less;                                 ///< the order

 };  // end( class SparseTable )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS PlusMinusOneRangeMinimum ---------------------*/
/*--------------------------------------------------------------------------*/

/// the range minima of an array whose adjacent items differ by 1
class PlusMinusOneRangeMinimum {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty structure
 PlusMinusOneRangeMinimum( void ) = default;

 /// the blocks, their sparse table and the table of the signatures,
 /// \f$ O( n ) \f$
 ///
 /// In pseudocode (Bender and Farach-Colton, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( D ) \cr
 /// 2 & \quad d \gets \max\{ 1 , \lfloor \log_2 n \rfloor / 2 \} \cr
 /// 3 & \quad \textbf{foreach}\ \text{block } k \text{ of } d \text{ items}
 ///     \ \textbf{do} \cr
 /// 4 & \qquad \mathit{sig}[ k ] \gets \text{the bits of its rises, those }
 ///     \text{past the end set} \cr
 /// 5 & \qquad \mathit{min}[ k ] \gets \text{the place of its leftmost }
 ///     \text{minimum} \cr
 /// 6 & \quad T \gets \text{the sparse table of the minima of the blocks} \cr
 /// 7 & \quad \textbf{foreach}\ \sigma < 2^{d-1} \text{ and } a \le b < d
 ///     \ \textbf{do} \cr
 /// 8 & \qquad \mathit{table}[ \sigma ][ a ][ b ] \gets \text{the leftmost }
 ///     \text{minimum of the walk } \sigma \text{ from } a \text{ to } b
 /// \end{array}}
 /// \f]
 /// \pre \f$ | D[ i + 1 ] - D[ i ] | = 1 \f$ for every \f$ i \f$
 explicit PlusMinusOneRangeMinimum( std::vector< long > D );

 /// the place of the leftmost minimum of \f$ D[ i .. j ] \f$,
 /// \f$ O( 1 ) \f$
 ///
 /// In pseudocode (Bender and Farach-Colton, beyond the book), the leftmost
 /// of the smallest of at most three minima:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{query}( i , j ) \cr
 /// 2 & \quad b_i \gets \lfloor i / d \rfloor;\ b_j \gets \lfloor j / d
 ///     \rfloor \cr
 /// 3 & \quad \textbf{if}\ b_i = b_j\ \textbf{then return}\ b_i d +
 ///     \mathit{table}[ \mathit{sig}[ b_i ] ][ i \bmod d ][ j \bmod d ] \cr
 /// 4 & \quad r \gets \text{the minimum of the suffix of block } b_i
 ///     \text{ from } i \cr
 /// 5 & \quad \textbf{if}\ b_j > b_i + 1\ \textbf{then}\ r \gets
 ///     \text{the better of } r \text{ and } \mathit{min}[ T.\mathrm{query}(
 ///     b_i + 1 , b_j - 1 ) ] \cr
 /// 6 & \quad \textbf{return}\ \text{the better of } r \text{ and the }
 ///     \text{minimum of the prefix of } b_j \text{ to } j
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j < n \f$
 /// \post a leftmost minimum of \f$ D[ i .. j ] \f$ (audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const { return( f_D.size() ); }

 /// the number of items in a block
 [[nodiscard]] std::size_t blockSize( void ) const { return( f_d ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the place in \f$ D \f$ of the leftmost minimum of the offsets
 /// \f$ a .. b \f$ of the block \f$ k \f$
 [[nodiscard]] std::size_t inBlock( std::size_t k , std::size_t a ,
                                    std::size_t b ) const;

 std::vector< long > f_D;                   ///< the items
 std::size_t f_d = 1;                       ///< the size of a block
 std::vector< std::uint32_t > f_sig;        ///< the signature of a block
 std::vector< std::uint8_t > f_table;       ///< \f$ [ \sigma ][ a ][ b ] \f$
 std::vector< std::size_t > f_blockMin;     ///< the minimum of a block
 SparseTable< long > f_blocks;              ///< on the minima of blocks

 };  // end( class PlusMinusOneRangeMinimum )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RangeMinimum ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the range minima of an array by Bender and Farach-Colton: the Cartesian
/// tree, its Euler tour, and the minima of the depths along the tour
template< class T , class Compare = std::less< T > >
class RangeMinimum {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty structure
 RangeMinimum( void ) = default;

 /// the structure of \f$ A \f$, \f$ O( n ) \f$
 ///
 /// In pseudocode (Bender and Farach-Colton, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( A ) \cr
 /// 2 & \quad P \gets \mathrm{cartesianTree}( A ) \cr
 /// 3 & \quad E , D \gets \text{the nodes and the depths of the Euler tour}
 ///     \text{ of } P \cr
 /// 4 & \quad \mathit{first}[ v ] \gets \text{the first place of } v
 ///     \text{ in } E \text{, for each } v \cr
 /// 5 & \quad \text{build the } \pm 1 \text{ structure on } D
 /// \end{array}}
 /// \f]
 explicit RangeMinimum( std::vector< T > A , Compare less = {} );

 /// the place of the leftmost minimum of \f$ A[ i .. j ] \f$,
 /// \f$ O( 1 ) \f$
 ///
 /// In pseudocode (Bender and Farach-Colton, beyond the book), the lowest
 /// common ancestor of \f$ i \f$ and \f$ j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{query}( i , j ) \cr
 /// 2 & \quad a \gets \min\{ \mathit{first}[ i ] , \mathit{first}[ j ] \};
 ///     \ b \gets \max\{ \mathit{first}[ i ] , \mathit{first}[ j ] \} \cr
 /// 3 & \quad \textbf{return}\ E[ \text{the } \pm 1 \text{ query on } D[ a ..
 ///     b ] ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j < n \f$
 /// \post a leftmost minimum of \f$ A[ i .. j ] \f$ (audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// the item of place \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// the nodes of the Euler tour of the Cartesian tree, \f$ 2 n - 1 \f$ of
 /// them
 [[nodiscard]] const std::vector< std::size_t > & eulerTour( void ) const {
  return( f_E );
  }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the array, random access

 using value_type = T;                   ///< the type of the items
 using size_type = std::size_t;          ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using const_reference = const T &;      ///< a reference to an item
 using reference = const_reference;      ///< the same, constant
 using const_iterator = typename std::vector< T >::const_iterator;
                                         ///< an iterator on the items
 using iterator = const_iterator;        ///< the same, constant

 /// the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.begin() ); }
 /// past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_A.end() ); }
 /// the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< T > f_A;                 ///< the items
 std::vector< std::size_t > f_E;       ///< the Euler tour
 std::vector< std::size_t > f_first;   ///< the first visit of each item
 PlusMinusOneRangeMinimum f_tour;      ///< on the depths along the tour
 Compare f_less;                       ///< the order

 };  // end( class RangeMinimum )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/RangeMinimum.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RangeMinimum.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RangeMinimum.h -------------------------*/
/*--------------------------------------------------------------------------*/
