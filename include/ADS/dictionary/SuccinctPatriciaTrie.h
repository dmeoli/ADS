/*--------------------------------------------------------------------------*/
/*----------------------- File SuccinctPatriciaTrie.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Patricia trie of a static dictionary of strings in succinct space,
 * beyond the book: the compacted trie of the strings [see `PatriciaTrie.h`
 * for the trie with pointers] coded by the degrees of its nodes in depth
 * (DFUDS) of Benoit, Demaine, Munro, Raman, Raman and Rao, with the first
 * character of each edge in a byte and the rest of its label in a string
 * apart, and a string mapped to its rank in the lexicographic order (see
 * \ref theory_dictionary_tries).
 *
 * The \f$ N \f$ nodes of the compacted trie (at most \f$ 2 n \f$ for
 * \f$ n \f$ strings, since each inner node but the root branches) are
 * visited in depth, the children of a node by their first character, and
 * the node of \f$ d \f$ children is written as \f$ d \f$ open parentheses
 * and one closed one, after an open parenthesis that balances the whole:
 * \f$ 2 N \f$ bits, in which the node of rank \f$ v \f$ in the visit begins
 * after the \f$ v \f$-th closed parenthesis, and with the matching of the
 * parentheses [see `BalancedParentheses.h`] the \f$ i \f$-th child of the
 * node at the place \f$ p \f$, of degree \f$ d \f$, begins after the closed
 * parenthesis that matches the open one at \f$ p + d - 1 - i \f$; the first
 * characters of the children of a node are consecutive, in the order of the
 * open parentheses, so that the child of a character is found by a binary
 * search on at most \f$ \sigma \f$ bytes. The visit in depth numbers the
 * nodes of a subtree consecutively, and the subtree of the node at
 * \f$ p \f$ ends where the excess first falls below the one at
 * \f$ p - 1 \f$ (`forwardSearch()`); hence, with a vector of bits that marks
 * the nodes where a string ends, the strings are numbered in the
 * lexicographic order (a node before its subtree, and the subtrees by their
 * first characters) and those of a subtree, i.e., those with a given
 * prefix, form a range of ranks.
 *
 * The space is \f$ 2 N + o( N ) \f$ bits for the parentheses with their
 * range min-max tree, \f$ 8 ( N - 1 ) \f$ for the first characters,
 * \f$ 2 N + o( N ) \f$ for the marks of the ends and of the rests, and the
 * rests of the labels, one after the other, with their places by
 * Elias-Fano [see `EliasFano.h`]; against the pointer trie, where a node
 * takes some tens of bytes, and against the front coding with buckets
 * [see `RearCoding.h`], which on strings with long shared prefixes may be
 * smaller still, since the rests of the labels are not compressed. A
 * lookup of \f$ Q \f$ walks down \f$ O( | Q | ) \f$ nodes, each in
 * \f$ O( \log \sigma ) \f$ time besides a rank, a select of Elias-Fano for
 * the rest of its label and, but for the first child, whose code follows
 * the closed parenthesis of the node, a `findClose()`; the closed
 * parenthesis of a node is found by the trailing ones of the words from its
 * place. `access()` walks up from the node of the string by `findOpen()`.
 *
 * The contracts [see `Contract.h`]: the strings sorted, with no string
 * twice (an audit); `access()` needs \f$ i < n \f$; the audit of the
 * construction is that `access()` and `lookup()` invert each other. The
 * functions are defined in `SuccinctPatriciaTrie.tpp`.
 *
 * \see D. Benoit, E. D. Demaine, J. I. Munro, R. Raman, V. Raman,
 *      S. S. Rao, Representing trees of higher degree, Algorithmica 43(4),
 *      275-292, 2005, doi:10.1007/s00453-004-1146-6
 * \see D. R. Morrison, PATRICIA - practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
 * \see R. Grossi, G. Ottaviano, Fast compressed tries through path
 *      decompositions, ACM Journal of Experimental Algorithmics 19, 2015,
 *      doi:10.1145/2656332
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

#ifndef __ADS_SuccinctPatriciaTrie
 #define __ADS_SuccinctPatriciaTrie
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/PrefixSearch.h"
#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/EliasFano.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SuccinctPatriciaTrie ------------------------*/
/*--------------------------------------------------------------------------*/

/// the compacted trie of a sorted dictionary of strings in DFUDS, the
/// strings numbered by their lexicographic rank
class SuccinctPatriciaTrie {

/*--------------------------------------------------------------------------*/
 public:

 /// the empty dictionary
 SuccinctPatriciaTrie( void );

 /// the trie of the strings of \f$ D \f$
 ///
 /// In pseudocode (Benoit et al., beyond the book), a node as the range
 /// \f$ [ l , r ) \f$ of the strings below it and the depth \f$ h \f$ of its
 /// end:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( D ) \cr
 /// 2 & \quad \mathrm{append}( \mathit{BP} , \text{(} );\ S \gets
 ///     \text{a stack with the root } ( 0 , n , 0 ) \cr
 /// 3 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
 /// 4 & \qquad ( l , r , h ) \gets \mathrm{pop}( S );\ \mathrm{append}(
 ///     T , | s_l | = h );\ \textbf{if}\ | s_l | = h\ \textbf{then}\ l
 ///     \gets l + 1 \cr
 /// 5 & \qquad \text{split } [ l , r ) \text{ into the runs } [ a_k , b_k )
 ///     \text{ of the same character } s_{a_k}[ h ] \cr
 /// 6 & \qquad \textbf{foreach}\ k\ \textbf{do}\ \mathrm{append}(
 ///     \mathit{BP} , \text{(} );\ \mathrm{append}( \Lambda , s_{a_k}[ h ] )
 ///     \cr
 /// 7 & \qquad \mathrm{append}( \mathit{BP} , \text{)} ) \cr
 /// 8 & \qquad \textbf{foreach}\ k \text{ from the last}\ \textbf{do} \cr
 /// 9 & \qquad\quad h' \gets | s_{a_k} | \text{ if } b_k = a_k + 1
 ///     \text{, else } \mathrm{commonPrefixLength}( s_{a_k} , s_{b_k - 1} )
 ///     \cr
 /// 10 & \qquad\quad \text{the rest of its label is } s_{a_k}[ h + 1 .. h'
 ///     - 1 ];\ \mathrm{push}( S , ( a_k , b_k , h' ) )
 /// \end{array}}
 /// \f]
 /// where the rest is written when the node is popped, so that the rests
 /// are in the order of the visit too.
 /// \pre \f$ D \f$ is sorted with no string twice (an audit)
 explicit SuccinctPatriciaTrie( const std::vector< std::string > & D );

 /// the number \f$ n \f$ of strings
 [[nodiscard]] std::size_t size( void ) const;

 /// whether there is no string
 [[nodiscard]] bool empty( void ) const;

 /// the nodes of the compacted trie, \f$ N \f$
 [[nodiscard]] std::size_t nodes( void ) const;

 /// the rank of \f$ Q \f$ in the dictionary, `size()` if it is not there
 ///
 /// In pseudocode (Benoit et al., beyond the book), \f$ \lambda( v ) \f$ the
 /// rest of the label of the node \f$ v \f$ and \f$ T \f$ the marks of the
 /// nodes where a string ends:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookup}( Q ) \cr
 /// 2 & \quad v \gets \text{the root};\ j \gets 0 \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{if}\ \lambda( v ) \text{ is not } Q[ j .. j +
 ///     | \lambda( v ) | - 1 ]\ \textbf{then return}\ n \cr
 /// 5 & \qquad j \gets j + | \lambda( v ) | \cr
 /// 6 & \qquad \textbf{if}\ j = | Q |\ \textbf{then return}
 ///     \ \mathrm{rank}_1( T , v ) \text{ if } T[ v ] = 1 \text{, else } n
 ///     \cr
 /// 7 & \qquad i \gets \text{the child of } v \text{ whose first character }
 ///     \text{is } Q[ j ] \text{, by a binary search};\ \textbf{if}
 ///     \ \text{none}\ \textbf{then return}\ n \cr
 /// 8 & \qquad v \gets \mathrm{findClose}( p_v + d_v - 1 - i ) + 1;\ j \gets
 ///     j + 1
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t lookup( std::string_view Q ) const;

 /// whether \f$ Q \f$ is a string of the dictionary, by `lookup()`
 [[nodiscard]] bool contains( std::string_view Q ) const;

 /// the string of rank \f$ i \f$
 ///
 /// In pseudocode (Benoit et al., beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{access}( i ) \cr
 /// 2 & \quad v \gets \mathrm{select}_1( T , i + 1 );\ s \gets \text{the }
 ///     \text{empty string} \cr
 /// 3 & \quad \textbf{while}\ v \text{ is not the root}\ \textbf{do} \cr
 /// 4 & \qquad q \gets \mathrm{findOpen}( p_v - 1 );\ u \gets \text{the }
 ///     \text{node of } q \cr
 /// 5 & \qquad s \gets \Lambda[ \text{the child } p_u + d_u - 1 - q
 ///     \text{ of } u ] \cdot \lambda( v ) \cdot s;\ v \gets u \cr
 /// 6 & \quad \textbf{return}\ s
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < \f$ `size()`
 [[nodiscard]] std::string access( std::size_t i ) const;

 /// the number of strings smaller than \f$ Q \f$, by the walk of
 /// `lookup()` up to the first mismatch and the end of a subtree there
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the range \f$ [ l , r ) \f$ of the ranks of the strings prefixed by
 /// \f$ P \f$, empty at `lexicographicPosition( P )` if there is none
 ///
 /// In pseudocode (Benoit et al., beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{prefixRange}( P ) \cr
 /// 2 & \quad v \gets \text{the highest node whose path from the root has }
 ///     P \text{ as a prefix, by the walk of } \mathrm{lookup}() \cr
 /// 3 & \quad \textbf{if}\ \text{none}\ \textbf{then return}\ [ x , x )
 ///     \text{ for } x = \mathrm{lexicographicPosition}( P ) \cr
 /// 4 & \quad e \gets \mathrm{forwardSearch}( p_v - 1 , -1 ) + 1\quad
 ///     \triangleright\ \text{the end of the subtree of } v \cr
 /// 5 & \quad \textbf{return}\ [ \mathrm{rank}_1( T , v ) , \mathrm{rank}_1(
 ///     T , \mathrm{rank}_0( \mathit{BP} , e ) ) )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the bytes of the structure: the parentheses with their directory and
 /// tree, the first characters, the marks and the rests of the labels
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the rank of the node beginning at the place \f$ p \f$
 [[nodiscard]] std::size_t node( std::size_t p ) const;

 /// the place where the node of rank \f$ v \f$ begins
 [[nodiscard]] std::size_t place( std::size_t v ) const;

 /// the rest of the label of the node of rank \f$ v \f$
 [[nodiscard]] std::string_view rest( std::size_t v ) const;

 /// the closed parenthesis of the node beginning at the place \f$ p \f$, the
 /// first 0 from \f$ p \f$ on, found by the words of the parentheses
 [[nodiscard]] std::size_t close( std::size_t p ) const;

 /// the place where the node begins whose description holds the open
 /// parenthesis at \f$ q \f$: after the closed parenthesis before \f$ q \f$,
 /// found by the leading zeros of the words down from it
 [[nodiscard]] std::size_t start( std::size_t q ) const;

 /// the place where the child \f$ i \f$ begins, for the node whose closed
 /// parenthesis is at \f$ c \f$: after \f$ c \f$ for the first child,
 /// after the closed parenthesis that matches the open one at
 /// \f$ c - 1 - i \f$ in general
 [[nodiscard]] std::size_t child( std::size_t c , std::size_t i ) const;

 /// the rank of the node after the subtree of the node at \f$ p \f$
 [[nodiscard]] std::size_t subtreeEnd( std::size_t p ) const;

 /// the walk of \f$ P \f$ down the trie: the range of `prefixRange()`
 [[nodiscard]] std::pair< std::size_t , std::size_t > walk(
                                                   std::string_view P ) const;

 std::size_t f_n = 0;            ///< the strings
 BalancedParentheses f_bp;       ///< the degrees in depth, DFUDS
 std::string f_first;            ///< the first characters of the children
 CompactRankSelect f_end;        ///< the nodes where a string ends
 CompactRankSelect f_hasRest;    ///< the nodes with a rest of their label
 std::string f_rest;             ///< the rests with their lengths, in order
 EliasFano f_restAt;             ///< where each rest begins

 };  // end( class SuccinctPatriciaTrie )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/SuccinctPatriciaTrie.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SuccinctPatriciaTrie.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File SuccinctPatriciaTrie.h --------------------*/
/*--------------------------------------------------------------------------*/
