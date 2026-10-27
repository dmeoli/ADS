/*--------------------------------------------------------------------------*/
/*----------------------- File BalancedParentheses.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A static sequence \f$ B \f$ of balanced parentheses, 1 for an open one and
 * 0 for a closed one, with the matching of Munro and Raman by the range
 * min-max tree of Navarro and Sadakane: the base of the trees by parentheses
 * [see `BPTree.h`], which chapter 15, Compressed Data Structures, of
 * Ferragina's book counts among the codes of \f$ 2 n \f$ bits of a tree,
 * and which, unlike the codes by levels, also give the size of a subtree and
 * the depth of a node.
 *
 * With the <em>excess</em> \f$ E( i ) \f$, the number of open parentheses
 * minus the number of closed ones in \f$ B[ 0 .. i ] \f$ (and
 * \f$ E( -1 ) = 0 \f$), every operation is a search of the first place,
 * forward or backward from \f$ i \f$, whose excess is at most a target
 * \f$ t \f$:
 *
 * - `findClose( i )`, the closed parenthesis of the open one at \f$ i \f$,
 *   is the first \f$ j > i \f$ with \f$ E( j ) = E( i ) - 1 \f$;
 * - `findOpen( j )`, the open parenthesis of the closed one at \f$ j \f$, is
 *   \f$ k + 1 \f$ for the last \f$ k < j \f$ with \f$ E( k ) = E( j ) \f$;
 * - `enclose( i )`, the open parenthesis of the pair that encloses the one
 *   at \f$ i \f$, is \f$ k + 1 \f$ for the last \f$ k < i \f$ with
 *   \f$ E( k ) = E( i ) - 2 \f$;
 *
 * and since the excess moves by \f$ \pm 1 \f$, the first place with an
 * excess at most \f$ t \f$ has exactly \f$ t \f$. The range min-max tree
 * keeps, for each leaf of 512 bits, for each group of 16 leaves and for each
 * node of a complete binary tree over the groups, the total excess of its
 * range and its minimum prefix excess. Every search runs on the excess
 * relative to its start, so that it needs no rank: it scans the leaf of
 * 512 bits of \f$ i \f$ (a byte at a time, with the excess, the minimum and
 * the place of each target of each of the 256 bytes in tables), then the
 * leaves of its group of 16 by their excess and minimum in 16 bits, then
 * climbs the tree, whose leaves are the groups, to the first range on its
 * side whose minimum reaches the target, and goes down to its group, its
 * leaf and its byte, in \f$ O( \log ( n / 8192 ) + 16 + 512 / 8 ) \f$ time;
 * most searches of a tree end in the leaf of \f$ i \f$, at the cost of a
 * miss of the cache. The tree takes \f$ 32 / 512 \f$ bits per parenthesis
 * for the leaves and at most \f$ 4 \cdot 128 / 8192 \f$ for the groups,
 * besides the rank directory [see `CompactRankSelect.h`], 3.5% of the
 * bits, since no search needs a rank. The excess of any place is
 * \f$ E( i ) = 2 \, \mathrm{rank}_1( i + 1 ) - ( i + 1 ) \f$, in
 * \f$ O( 1 ) \f$ time.
 *
 * Jacobson gives `findClose()` in \f$ O( \log n ) \f$ time with the
 * pioneers, and Munro and Raman in \f$ O( 1 ) \f$ time with two levels of
 * pioneers and tables, in \f$ 2 n + o( n ) \f$ bits; the range min-max tree
 * of Navarro and Sadakane reaches \f$ O( 1 ) \f$ time with a tree of degree
 * \f$ \Theta( \log n ) \f$ and tables, and its binary form with blocks of a
 * few hundred bits, the one here, is the fastest in practice (Arroyuelo,
 * Cánovas, Navarro and Sadakane; Gog and Petri; Grossi and Ottaviano).
 *
 * The contracts [see `Contract.h`]: the sequence is balanced (an audit),
 * `findClose()` and `enclose()` get an open parenthesis and `findOpen()` a
 * closed one; the postconditions are the excesses of the definitions. The
 * functions are defined in `BalancedParentheses.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15, Compressed Data Structures,
 *      doi:10.1017/9781009128933
 * \see G. Jacobson, Space-efficient static trees and graphs, Proc. 30th
 *      FOCS, 549-554, 1989, doi:10.1109/SFCS.1989.63533
 * \see J. I. Munro, V. Raman, Succinct representation of balanced
 *      parentheses and static trees, SIAM Journal on Computing 31(3),
 *      762-776, 2001, doi:10.1137/S0097539799364092
 * \see G. Navarro, K. Sadakane, Fully functional static and dynamic
 *      succinct trees, ACM Transactions on Algorithms 10(3), 16, 2014,
 *      doi:10.1145/2601073
 * \see D. Arroyuelo, R. Cánovas, G. Navarro, K. Sadakane, Succinct trees
 *      in practice, Proc. 12th ALENEX, 84-97, 2010,
 *      doi:10.1137/1.9781611972900.9
 * \see S. Gog, M. Petri, Optimized succinct data structures for massive
 *      data, Software: Practice and Experience 44(11), 1287-1314, 2014,
 *      doi:10.1002/spe.2198
 * \see R. Grossi, G. Ottaviano, Design of practical succinct data
 *      structures for large data collections, Proc. 12th SEA, LNCS 7933,
 *      5-17, 2013, doi:10.1007/978-3-642-38527-8_3
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

#ifndef __ADS_BalancedParentheses
 #define __ADS_BalancedParentheses
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/CompactRankSelect.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS BalancedParentheses ------------------------*/
/*--------------------------------------------------------------------------*/

/// a static sequence of balanced parentheses with a range min-max tree
class BalancedParentheses {

/*--------------------------------------------------------------------------*/
 public:

 static constexpr std::size_t nil = std::size_t( -1 );   ///< no place

 /// the empty sequence
 BalancedParentheses( void );

 /// the parentheses \f$ b \f$, 1 for an open one and 0 for a closed one,
 /// and the range min-max tree on them
 ///
 /// In pseudocode (the range min-max tree of Navarro and Sadakane, beyond
 /// the book), with \f$ \ell \f$ leaves of 512 bits, \f$ g \f$ the least
 /// power of 2 not below the groups of 16 leaves, and the node \f$ v \f$ of
 /// the tree with the children \f$ 2 v \f$ and \f$ 2 v + 1 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( b ) \cr
 /// 2 & \quad \textbf{for}\ l \gets 0\ \textbf{to}\ \ell - 1\ \textbf{do} \cr
 /// 3 & \qquad e \gets 0;\ \mu \gets 0 \cr
 /// 4 & \qquad \textbf{foreach}\ \text{byte } y \text{ of the leaf } l
 ///     \ \textbf{do} \cr
 /// 5 & \qquad\quad \mu \gets \min\{ \mu , e + \mathit{min}_8[ y ] \};\ e
 ///     \gets e + \mathit{exc}_8[ y ]\quad \triangleright\ \text{by tables}
 ///     \cr
 /// 6 & \qquad \mathit{lexc}[ l ] \gets e;\ \mathit{lmin}[ l ] \gets \mu;\ v
 ///     \gets g + \lfloor l / 16 \rfloor \cr
 /// 7 & \qquad \mathit{min}[ v ] \gets \min\{ \mathit{min}[ v ] ,
 ///     \mathit{exc}[ v ] + \mu \};\ \mathit{exc}[ v ] \gets \mathit{exc}[ v ]
 ///     + e \cr
 /// 8 & \quad \textbf{for}\ v \gets g - 1\ \textbf{downto}\ 1\ \textbf{do} \cr
 /// 9 & \qquad \mathit{exc}[ v ] \gets \mathit{exc}[ 2 v ] + \mathit{exc}[ 2 v
 ///     + 1 ] \cr
 /// 10 & \qquad \mathit{min}[ v ] \gets \min\{ \mathit{min}[ 2 v ] ,
 ///     \mathit{exc}[ 2 v ] + \mathit{min}[ 2 v + 1 ] \}
 /// \end{array}}
 /// \f]
 /// \pre \f$ b \f$ is balanced (an audit)
 explicit BalancedParentheses( BitVector b );

 /// the number of parentheses
 [[nodiscard]] std::size_t size( void ) const;

 /// whether the place \f$ i \f$ has an open parenthesis
 /// \pre \f$ i < \mathrm{size}() \f$
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// \f$ E( i ) \f$, the open parentheses minus the closed ones in
 /// \f$ [ 0 , i ] \f$, as \f$ 2 \, \mathrm{rank}_1( i + 1 ) - ( i + 1 ) \f$
 /// \pre \f$ i < \mathrm{size}() \f$
 [[nodiscard]] std::int64_t excess( std::size_t i ) const;

 /// the closed parenthesis of the open one at \f$ i \f$
 ///
 /// In pseudocode (the forward search of Navarro and Sadakane, beyond the
 /// book), the first \f$ j > i \f$ with \f$ E( j ) - E( i ) \le d = -1 \f$,
 /// \f$ e \f$ the excess relative to \f$ E( i ) \f$ and each range skipped
 /// whole when its minimum stays above \f$ d \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findClose}( i ) \cr
 /// 2 & \quad d \gets -1;\ e \gets 0;\ l \gets \text{the leaf of } i \cr
 /// 3 & \quad \text{scan the leaf } l \text{ from } i + 1 \text{, a byte }
 ///     \text{at a time;}\ \textbf{if}\ \text{some } j \text{ has } e \le d
 ///     \ \textbf{then return}\ j \cr
 /// 4 & \quad \textbf{foreach}\ \text{next leaf } l \text{ of its group}
 ///     \ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ e + \mathit{lmin}[ l ] \le d
 ///     \ \textbf{then return}\ \text{the place found by the scan of } l \cr
 /// 6 & \qquad e \gets e + \mathit{lexc}[ l ] \cr
 /// 7 & \quad v \gets \text{the leaf of the tree for the group} \cr
 /// 8 & \quad \textbf{while}\ v > 1\ \textbf{and not}\ ( v \text{ is a left }
 ///     \text{child}\ \textbf{and}\ e + \mathit{min}[ v + 1 ] \le d )
 ///     \ \textbf{do} \cr
 /// 9 & \qquad \textbf{if}\ v \text{ is a left child }\ \textbf{then}\ e
 ///     \gets e + \mathit{exc}[ v + 1 ] \cr
 /// 10 & \qquad v \gets \lfloor v / 2 \rfloor \cr
 /// 11 & \quad \textbf{if}\ v = 1\ \textbf{then return nil} \cr
 /// 12 & \quad v \gets v + 1\quad \triangleright\ \text{the right sibling}
 ///     \text{ that reaches } d \cr
 /// 13 & \quad \textbf{while}\ v \text{ is not a leaf }\ \textbf{do}\quad
 ///     \triangleright\ \text{down to the leftmost range that reaches } d \cr
 /// 14 & \qquad \textbf{if}\ e + \mathit{min}[ 2 v ] \le d\ \textbf{then}\ v
 ///     \gets 2 v\ \textbf{else}\ e \gets e + \mathit{exc}[ 2 v ];\ v \gets
 ///     2 v + 1 \cr
 /// 15 & \quad l \gets \text{the first leaf of the group } v \text{ with }
 ///     e + \mathit{lmin}[ l ] \le d \text{, adding the } \mathit{lexc}
 ///     \text{ of those before} \cr
 /// 16 & \quad \textbf{return}\ \text{the place found by the scan of } l
 /// \end{array}}
 /// \f]
 /// \pre the place \f$ i \f$ has an open parenthesis
 /// \post the first place after \f$ i \f$ with the excess \f$ E( i ) - 1 \f$
 [[nodiscard]] std::size_t findClose( std::size_t i ) const;

 /// the first \f$ j > i \f$ with \f$ E( j ) - E( i ) = d < 0 \f$, `nil` if
 /// none: the end of a run of parentheses that goes \f$ -d \f$ levels
 /// below the one at \f$ i \f$, e.g., of a subtree of the code by the
 /// degrees in depth [see `SuccinctPatriciaTrie.h`]
 ///
 /// In pseudocode (the forward search of Navarro and Sadakane, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{forwardSearch}( i , d ) \cr
 /// 2 & \quad \textbf{return}\ \text{the place found by }
 ///     \mathrm{findClose}( i ) \text{ with the target } d \text{ for } -1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \mathrm{size}() \f$ and \f$ d < 0 \f$
 [[nodiscard]] std::size_t forwardSearch( std::size_t i ,
                                          std::int64_t d ) const;

 /// the open parenthesis of the closed one at \f$ j \f$
 ///
 /// In pseudocode (the backward search of Navarro and Sadakane, beyond the
 /// book), the mirror of the forward search of `findClose()`, on the ranges
 /// to the left and with the excess taken off:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findOpen}( j ) \cr
 /// 2 & \quad k \gets \text{the last } k < j \text{ with } E( k ) - E( j )
 ///     \le 0 \text{, } E( -1 ) = 0 \text{, by the leaf of } j \text{, its}
 ///     \text{ group and the tree} \cr
 /// 3 & \quad \textbf{return}\ k + 1
 /// \end{array}}
 /// \f]
 /// \pre the place \f$ j \f$ has a closed parenthesis
 /// \post `findClose()` of it is \f$ j \f$
 [[nodiscard]] std::size_t findOpen( std::size_t j ) const;

 /// the open parenthesis of the pair that encloses the open one at
 /// \f$ i \f$, `nil` if none
 ///
 /// In pseudocode (Navarro and Sadakane, beyond the book), by the backward
 /// search of `findOpen()` with the target 2 levels below:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{enclose}( i ) \cr
 /// 2 & \quad \textbf{if}\ \text{no } k < i \text{ has } E( k ) - E( i ) \le
 ///     -2\ \textbf{then return nil}\quad \triangleright\ \text{the outer }
 ///     \text{level} \cr
 /// 3 & \quad \textbf{return}\ k + 1 \text{ for the last such } k
 /// \end{array}}
 /// \f]
 /// \pre the place \f$ i \f$ has an open parenthesis
 [[nodiscard]] std::size_t enclose( std::size_t i ) const;

 /// the parentheses with their rank and select
 [[nodiscard]] const CompactRankSelect & bits( void ) const;

 /// the bits of the range min-max tree
 [[nodiscard]] std::size_t treeBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kLeaf = 512;    ///< bits in a leaf
 static constexpr std::size_t kGroup = 16;    ///< leaves in a group

 /// from \f$ e = E( a - 1 ) - r \f$, the first \f$ j \in [ a , b ) \f$ with
 /// \f$ E( j ) - r \le d \f$, `nil` if none; \f$ e \f$ becomes
 /// \f$ E( b - 1 ) - r \f$
 [[nodiscard]] std::size_t scanForward( std::size_t a , std::size_t b ,
                                        std::int64_t & e ,
                                        std::int64_t d ) const;

 /// from \f$ e = E( b ) - r \f$, the last \f$ k \in [ a - 1 , b ) \f$ with
 /// \f$ E( k ) - r \le d \f$, plus 1, `nil` if none; \f$ e \f$ becomes
 /// \f$ E( a - 1 ) - r \f$
 [[nodiscard]] std::size_t scanBackward( std::size_t a , std::size_t b ,
                                         std::int64_t & e ,
                                         std::int64_t d ) const;

 /// the last \f$ k < i \f$ with \f$ E( k ) - E( i ) = d \le 0 \f$, plus 1,
 /// counting \f$ E( -1 ) = 0 \f$; `nil` if none
 [[nodiscard]] std::size_t backwardSearch( std::size_t i ,
                                           std::int64_t d ) const;

 /// the end of the leaf \f$ l \f$, one past its last bit
 [[nodiscard]] std::size_t leafEnd( std::size_t l ) const {
  return( std::min( size() , ( l + 1 ) * kLeaf ) );
  }

 /// whether the excess is never negative and 0 at the end
 [[nodiscard]] bool balanced( void ) const;

 CompactRankSelect f_b;                  ///< the parentheses
 std::size_t f_nleaf = 0;                ///< the leaves
 std::size_t f_groups = 1;               ///< leaves of the tree, a power of 2
 /// the excess of a leaf and the least excess of its places and of the
 /// place before it, relative to that place
 std::vector< std::int16_t > f_lexc , f_lmin;
 /// the same for a node of the tree of the groups
 std::vector< std::int64_t > f_exc , f_min;

 };  // end( class BalancedParentheses )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/BalancedParentheses.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BalancedParentheses.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File BalancedParentheses.h ---------------------*/
/*--------------------------------------------------------------------------*/
