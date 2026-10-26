/*--------------------------------------------------------------------------*/
/*-------------------------- File RangeMinimum.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range minimum query, beyond the book: given an array A of n items of
 * an ordered universe, the place of a minimum of A[ i .. j ] for any i <=
 * j, the leftmost one when the minimum repeats. The problem is the backbone
 * of the longest common extensions of a text [see SuffixArray.h] and of the
 * lowest common ancestors of a tree [see SuffixTree.h].
 *
 * - SparseTable stores, for every place i and every L, the place of the
 *   minimum of A[ i .. i + 2^L - 1 ], computed from the two halves of the
 *   range; a query covers [ i , j ] by the two (possibly overlapping)
 *   ranges of length \f$ 2^L \f$, \f$ L = \lfloor \log_2( j - i + 1 )
 *   \rfloor \f$, that start at i and end at j. It takes \f$ O( n \log n )
 *   \f$ time and space and answers in O( 1 ) time, against the \f$ O( n^2 )
 *   \f$ of the table of all the answers.
 * - cartesianTree() builds the Cartesian tree of A with a stack in O( n ):
 *   its root is the leftmost minimum of A, its left and right subtrees the
 *   Cartesian trees of the parts before and after it; a range of A is a
 *   subtree, and the minimum of A[ i .. j ] is the lowest common ancestor
 *   of i and j.
 * - PlusMinusOneRangeMinimum answers the queries on an array whose adjacent
 *   items differ by exactly 1, as the depths along an Euler tour of a tree:
 *   the array is cut into blocks of \f$ d = \max\{ 1 , \lfloor \log_2 n
 *   \rfloor / 2 \} \f$ items, a sparse table on the minima of the blocks
 *   answers for the blocks covered entirely, and a block is described by the
 *   d - 1 signs of its differences, so that a table of the \f$ 2^{d-1} \le
 *   \sqrt{n} \f$ signatures times the \f$ d^2 \f$ pairs of offsets (the trick
 *   of the Four Russians) answers inside a block: O( n ) time and space,
 *   O( 1 ) time per query.
 * - RangeMinimum is the solution of Bender and Farach-Colton for any array:
 *   the minimum of A[ i .. j ] is the lowest common ancestor of i and j in
 *   the Cartesian tree, i.e., the node of least depth on the Euler tour of
 *   the tree between the first visits of i and j, a query on the 2 n - 1
 *   depths of the tour, which differ by 1. Its time and space are O( n ),
 *   O( 1 ) time per query (Theorem 10.11), and the same reduction answers
 *   the lowest common ancestors of any tree (Theorem 10.12).
 *
 * The bound O( n ) of Bender and Farach-Colton was first reached by Harel
 * and Tarjan on the lowest common ancestors, with a far more involved
 * structure; Fischer and Heun need only 2 n + o( n ) bits besides A, by
 * describing each block by its own Cartesian tree, which removes the Euler
 * tour and its factor 2. The sparse table is the simplest and, on the arrays
 * that fit in a cache, often the fastest in practice.
 *
 * The contracts [see Contract.h]: a query asks for i <= j < n, and its
 * answer is a leftmost minimum of the range (an audit, linear in the range);
 * the items of a PlusMinusOneRangeMinimum differ by 1. The functions are
 * defined in RangeMinimum.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10
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

/// the parent of each item of A in its Cartesian tree, the root (the
/// leftmost minimum) having A.size(); equal items go to the right
/// \post a heap: no item is smaller than its parent (an audit)
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

 SparseTable( void ) = default;

 /// the table of A, O( n log n )
 explicit SparseTable( std::vector< T > A , Compare less = {} );

 /// the place of the leftmost minimum of A[ i .. j ], O( 1 )
 /// \pre i <= j < size()
 /// \post a leftmost minimum of A[ i .. j ] (an audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// \pre i < size()
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the array, random access

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using const_reference = const T &;
 using reference = const_reference;
 using const_iterator = typename std::vector< T >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_A.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_A.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the one of a < b with the smaller item, a on a tie
 [[nodiscard]] std::size_t better( std::size_t a , std::size_t b ) const;

 std::vector< T > f_A;
 std::vector< std::vector< std::size_t > > f_M;  ///< f_M[ L ][ i ]
 Compare f_less;

 };  // end( class SparseTable )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS PlusMinusOneRangeMinimum ---------------------*/
/*--------------------------------------------------------------------------*/

/// the range minima of an array whose adjacent items differ by 1
class PlusMinusOneRangeMinimum {

/*--------------------------------------------------------------------------*/
 public:

 PlusMinusOneRangeMinimum( void ) = default;

 /// the blocks, their sparse table and the table of the signatures, O( n )
 /// \pre | D[ i + 1 ] - D[ i ] | = 1 for every i
 explicit PlusMinusOneRangeMinimum( std::vector< long > D );

 /// the place of the leftmost minimum of D[ i .. j ], O( 1 )
 /// \pre i <= j < size()
 /// \post a leftmost minimum of D[ i .. j ] (an audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 [[nodiscard]] std::size_t size( void ) const { return( f_D.size() ); }

 /// the number of items in a block
 [[nodiscard]] std::size_t blockSize( void ) const { return( f_d ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the place in D of the leftmost minimum of the offsets a .. b of the
 /// block k
 [[nodiscard]] std::size_t inBlock( std::size_t k , std::size_t a ,
                                    std::size_t b ) const;

 std::vector< long > f_D;
 std::size_t f_d = 1;                       ///< the size of a block
 std::vector< std::uint32_t > f_sig;        ///< the signature of a block
 std::vector< std::uint8_t > f_table;       ///< [ sig ][ a ][ b ]
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

 RangeMinimum( void ) = default;

 /// the structure of A, O( n )
 explicit RangeMinimum( std::vector< T > A , Compare less = {} );

 /// the place of the leftmost minimum of A[ i .. j ], O( 1 )
 /// \pre i <= j < size()
 /// \post a leftmost minimum of A[ i .. j ] (an audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// \pre i < size()
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// the nodes of the Euler tour of the Cartesian tree, 2 n - 1 of them
 [[nodiscard]] const std::vector< std::size_t > & eulerTour( void ) const {
  return( f_E );
  }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the array, random access

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using const_reference = const T &;
 using reference = const_reference;
 using const_iterator = typename std::vector< T >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_A.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_A.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< T > f_A;
 std::vector< std::size_t > f_E;       ///< the Euler tour
 std::vector< std::size_t > f_first;   ///< the first visit of each item
 PlusMinusOneRangeMinimum f_tour;      ///< on the depths along the tour
 Compare f_less;

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
