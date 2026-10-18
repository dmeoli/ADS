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
 * block of 512 bits and then for each node of a complete binary tree over
 * the blocks, the total excess of its range and its minimum prefix excess;
 * a search scans the block of i (a byte at a time, with the excess and the
 * minimum of each of the 256 bytes in a table), climbs the tree to the
 * first range on its side whose minimum reaches t and goes down to its
 * block, in \f$ O( \log ( n / 512 ) + 512 / 8 ) \f$ time, with
 * \f$ 2 \cdot 128 / 512 \f$ bits per parenthesis for the tree besides the
 * rank directory [see RankSelect.h]. The excess of any place is
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
#include "ADS/succinct/RankSelect.h"

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
 [[nodiscard]] const RankSelect & bits( void ) const;

 /// the bits of the range min-max tree
 [[nodiscard]] std::size_t treeBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kBlock = 512;   ///< bits in a leaf

 /// E( i ), with E( -1 ) = 0, for i = p - 1
 [[nodiscard]] std::int64_t excessBefore( std::size_t p ) const;

 /// the first j in [ a , b ) with E( j ) <= t, nil if none
 [[nodiscard]] std::size_t scanForward( std::size_t a , std::size_t b ,
                                        std::int64_t t ) const;

 /// the last k in [ a , b ) with E( k ) <= t, plus 1; 0 if none
 [[nodiscard]] std::size_t scanBackward( std::size_t a , std::size_t b ,
                                         std::int64_t t ) const;

 /// the first j > i with E( j ) <= t, nil if none
 [[nodiscard]] std::size_t forwardSearch( std::size_t i ,
                                          std::int64_t t ) const;

 /// the last k < i with E( k ) <= t, plus 1, counting E( -1 ) = 0; nil
 /// if none
 [[nodiscard]] std::size_t backwardSearch( std::size_t i ,
                                           std::int64_t t ) const;

 /// the excess never negative and 0 at the end
 [[nodiscard]] bool balanced( void ) const;

 RankSelect f_b;
 std::size_t f_leaves = 1;              ///< leaves of the tree, a power of 2
 std::vector< std::int64_t > f_exc;     ///< the excess of a node's range
 std::vector< std::int64_t > f_min;     ///< its minimum prefix excess

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
