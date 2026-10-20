/*--------------------------------------------------------------------------*/
/*----------------------- File BalancedParentheses.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A static sequence of balanced parentheses, 1 for '(' and 0 for ')', with
 * the matching of Munro and Raman by the range min-max tree of Navarro and
 * Sadakane: the base of the trees by parentheses [see BPTree.h], which the
 * notes count among the codes of 2n bits of a tree (the footnote on the
 * binary trees of section 15.2.1) and which, unlike the codes by levels,
 * also give the size of a subtree and the depth of a node.
 *
 * With the excess \f$ E( i ) \f$, the number of '(' minus the number of
 * ')' in \f$ B[ 0 , i ] \f$ (and \f$ E( -1 ) = 0 \f$), every operation is a
 * search of the first place, forward or backward from i, whose excess is at
 * most a target t:
 *
 * - findClose( i ), the ')' of the '(' at i, is the first \f$ j > i \f$
 *   with \f$ E( j ) = E( i ) - 1 \f$;
 * - findOpen( j ), the '(' of the ')' at j, is \f$ k + 1 \f$ for the last
 *   \f$ k < j \f$ with \f$ E( k ) = E( j ) \f$;
 * - enclose( i ), the '(' of the pair that encloses the one at i, is
 *   \f$ k + 1 \f$ for the last \f$ k < i \f$ with \f$ E( k ) = E( i ) - 2 \f$;
 *
 * and since the excess moves by \f$ \pm 1 \f$, the first place with an
 * excess at most t has exactly t. The range min-max tree keeps, for each
 * leaf of 512 bits, for each group of 16 leaves and for each node of a
 * complete binary tree over the groups, the total excess of its range and
 * its minimum prefix excess;
 * every search runs on the excess relative to its start, so that it needs
 * no rank: it scans the leaf of 512 bits of i (a byte at a time, with the
 * excess, the minimum and the place of each target of each of the 256
 * bytes in tables), then the leaves of its group of 16 by their excess
 * and minimum in 16 bits, then climbs the tree, whose leaves are the
 * groups, to the first range on its side whose minimum reaches the target
 * and goes down to its group, its leaf and its byte, in
 * \f$ O( \log ( n / 8192 ) + 16 + 512 / 8 ) \f$ time; most searches of a
 * tree end in the leaf of i, at the cost of a miss of the cache. The tree
 * takes \f$ 32 / 512 \f$ bits per parenthesis for the leaves and at most
 * \f$ 4 \cdot 128 / 8192 \f$ for the groups, besides the rank directory
 * [see CompactRankSelect.h], 3.5% of the bits, since no search needs a
 * rank. The excess of any place is
 * \f$ E( i ) = 2 rank_1( i + 1 ) - ( i + 1 ) \f$, in O( 1 ) time.
 *
 * Jacobson gives findClose() in O( log n ) time with the pioneers, and Munro
 * and Raman in O( 1 ) time with two levels of pioneers and tables, in
 * 2n + o( n ) bits; the range min-max tree of Navarro and Sadakane reaches
 * O( 1 ) time with a tree of degree \f$ \Theta( \log n ) \f$ and tables,
 * and its binary form with blocks of a few hundred bits, the one here, is
 * the fastest in practice (Arroyuelo, Canovas, Navarro and Sadakane;
 * the bp_support of sdsl).
 *
 * The contracts [see Contract.h]: the sequence is balanced (an audit),
 * findClose() and enclose() get a '(' and findOpen() a ')'; the
 * postconditions are the excesses of the definitions. The functions are
 * defined in BalancedParentheses.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
 * \see J. I. Munro, V. Raman, Succinct representation of balanced
 *      parentheses and static trees, SIAM Journal on Computing 31(3),
 *      762-776, 2001, doi:10.1137/S0097539799364092
 * \see G. Navarro, K. Sadakane, Fully functional static and dynamic
 *      succinct trees, ACM Transactions on Algorithms 10(3), 16, 2014,
 *      doi:10.1145/2601073
 * \see D. Arroyuelo, R. Canovas, G. Navarro, K. Sadakane, Succinct trees in
 *      practice, Proc. 12th ALENEX, 84-97, 2010,
 *      doi:10.1137/1.9781611972900.9
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

 BalancedParentheses( void );

 /// the parentheses b, 1 for '(' and 0 for ')'
 /// \pre b is balanced (an audit)
 explicit BalancedParentheses( BitVector b );

 [[nodiscard]] std::size_t size( void ) const;

 /// whether the place i has a '('
 /// \pre i < size()
 [[nodiscard]] bool operator[]( std::size_t i ) const;

 /// E( i ), the '(' minus the ')' in [ 0 , i ]
 /// \pre i < size()
 [[nodiscard]] std::int64_t excess( std::size_t i ) const;

 /// the ')' of the '(' at i
 /// \pre the place i has a '('
 /// \post the place after i with the excess E( i ) - 1, the first one
 [[nodiscard]] std::size_t findClose( std::size_t i ) const;

 /// the '(' of the ')' at j
 /// \pre the place j has a ')'
 /// \post findClose() of it is j
 [[nodiscard]] std::size_t findOpen( std::size_t j ) const;

 /// the '(' of the pair that encloses the '(' at i, nil if none
 /// \pre the place i has a '('
 [[nodiscard]] std::size_t enclose( std::size_t i ) const;

 /// the parentheses with their rank and select
 [[nodiscard]] const CompactRankSelect & bits( void ) const;

 /// the bits of the range min-max tree
 [[nodiscard]] std::size_t treeBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kLeaf = 512;    ///< bits in a leaf
 static constexpr std::size_t kGroup = 16;    ///< leaves in a group

 /// from e = E( a - 1 ) - r, the first j in [ a , b ) with
 /// E( j ) - r <= d, nil if none; e becomes E( b - 1 ) - r
 [[nodiscard]] std::size_t scanForward( std::size_t a , std::size_t b ,
                                        std::int64_t & e ,
                                        std::int64_t d ) const;

 /// from e = E( b ) - r, the last k in [ a - 1 , b ) with E( k ) - r <= d,
 /// plus 1, nil if none; e becomes E( a - 1 ) - r
 [[nodiscard]] std::size_t scanBackward( std::size_t a , std::size_t b ,
                                         std::int64_t & e ,
                                         std::int64_t d ) const;

 /// the first j > i with E( j ) - E( i ) = d < 0, nil if none
 [[nodiscard]] std::size_t forwardSearch( std::size_t i ,
                                          std::int64_t d ) const;

 /// the last k < i with E( k ) - E( i ) = d <= 0, plus 1, counting
 /// E( -1 ) = 0; nil if none
 [[nodiscard]] std::size_t backwardSearch( std::size_t i ,
                                           std::int64_t d ) const;

 /// the bits of the leaf l
 [[nodiscard]] std::size_t leafEnd( std::size_t l ) const {
  return( std::min( size() , ( l + 1 ) * kLeaf ) );
  }

 /// the excess never negative and 0 at the end
 [[nodiscard]] bool balanced( void ) const;

 CompactRankSelect f_b;
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
