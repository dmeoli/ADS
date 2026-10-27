/*--------------------------------------------------------------------------*/
/*------------------------- File SuffixAutomaton.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The suffix automaton (the directed acyclic word graph, DAWG) of a string
 * \f$ S \f$ of \f$ n \f$ bytes, beyond the book: the smallest deterministic
 * automaton that accepts the suffixes of \f$ S \f$, whose paths from the
 * start spell exactly the substrings of \f$ S \f$. Its states are the
 * classes of the substrings by their sets of end places,
 * \f[
 *   \mathrm{endpos}( u ) = \{ j : S[ j - |u| + 1 .. j ] = u \} ,
 * \f]
 * a class holding the suffixes of its longest member of lengths in
 * \f$ ( \mathit{len}( \mathit{link}( v ) ) , \mathit{len}( v ) ] \f$, where
 * the suffix link of \f$ v \f$ is the class of the longest suffix in
 * another class. It has at most \f$ 2 n - 1 \f$ states (\f$ n \ge 2 \f$)
 * and \f$ 3 n - 4 \f$ transitions (\f$ n \ge 3 \f$), and the construction
 * of Blumer et al. adds the bytes one at a time: the new state of
 * \f$ S c \f$ gets the transitions on \f$ c \f$ from the suffix path of the
 * last state, which stops at the first state \f$ p \f$ that has one; if it
 * leads to \f$ q \f$ with
 * \f$ \mathit{len}( q ) = \mathit{len}( p ) + 1 \f$, then \f$ q \f$ is the
 * new link, otherwise \f$ q \f$ is cloned with that length, in
 * \f$ O( n ) \f$ amortized time for a constant alphabet. Here the
 * transitions of a state are a short vector of pairs (byte, state),
 * searched linearly, so that the time is \f$ O( n \sigma ) \f$ for
 * \f$ \sigma \f$ distinct bytes.
 *
 * - `contains( P )` follows \f$ P \f$ from the start,
 *   \f$ O( |P| \sigma ) \f$;
 * - `occurrences( P )` is the size of the end set of the state of \f$ P \f$,
 *   counted once for all by adding the count of each state (1 if it ends a
 *   prefix of \f$ S \f$, 0 for a clone) to its link, in decreasing order of
 *   the lengths;
 * - `distinctSubstrings()` is
 *   \f$ \sum_{v \ne \mathit{start}} ( \mathit{len}( v ) - \mathit{len}(
 *   \mathit{link}( v ) ) ) \f$, the number of nonempty distinct substrings
 *   of \f$ S \f$;
 * - `longestCommonSubstring( T )` scans \f$ T \f$ keeping the longest
 *   suffix of \f$ T[ 0 .. j ] \f$ that is a substring of \f$ S \f$,
 *   following the links on a mismatch, \f$ O( |T| \sigma ) \f$.
 *
 * The contracts [see `Contract.h`]: at most \f$ 2 n \f$ states (an
 * invariant), and the results equal those of the definitions on the
 * substrings (the audits of the tests). The functions are defined in
 * `SuffixAutomaton.tpp`.
 *
 * \see A. Blumer, J. Blumer, D. Haussler, A. Ehrenfeucht, M. T. Chen,
 *      J. Seiferas, The smallest automaton recognizing the subwords of a
 *      text, Theoretical Computer Science 40, 31-55, 1985,
 *      doi:10.1016/0304-3975(85)90157-4
 * \see M. Crochemore, Transducers and repetitions, Theoretical Computer
 *      Science 45, 63-86, 1986, doi:10.1016/0304-3975(86)90041-1
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

#ifndef __ADS_SuffixAutomaton
 #define __ADS_SuffixAutomaton
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS SuffixAutomaton ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the suffix automaton of a string, built a byte at a time
class SuffixAutomaton {

/*--------------------------------------------------------------------------*/
 public:

 /// the automaton of the empty string, the start state alone
 SuffixAutomaton( void );

 /// the automaton of \f$ S \f$, by `extend()` on each of its bytes
 explicit SuffixAutomaton( std::string_view S );

 /// the automaton of \f$ S c \f$ from that of \f$ S \f$
 ///
 /// In pseudocode (the construction of Blumer et al., beyond the book),
 /// \f$ \mathit{last} \f$ the state of \f$ S \f$ and
 /// \f$ \delta( p , c ) \f$ the transition of \f$ p \f$ on \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{extend}( c ) \cr
 /// 2 & \quad \mathit{cur} \gets \text{a new state};\ \mathit{len}(
 ///     \mathit{cur} ) \gets \mathit{len}( \mathit{last} ) + 1;
 ///     \ \mathit{link}( \mathit{cur} ) \gets \mathit{start} \cr
 /// 3 & \quad p \gets \mathit{last} \cr
 /// 4 & \quad \textbf{while}\ p \ne \textbf{nil}\ \textbf{and}\ \delta( p , c
 ///     ) = \textbf{nil}\ \textbf{do}\ \delta( p , c ) \gets \mathit{cur};\ p
 ///     \gets \mathit{link}( p ) \cr
 /// 5 & \quad \textbf{if}\ p \ne \textbf{nil}\ \textbf{then} \cr
 /// 6 & \qquad q \gets \delta( p , c ) \cr
 /// 7 & \qquad \textbf{if}\ \mathit{len}( q ) = \mathit{len}( p ) + 1
 ///     \ \textbf{then}\ \mathit{link}( \mathit{cur} ) \gets q \cr
 /// 8 & \qquad \textbf{else} \cr
 /// 9 & \qquad\quad r \gets \text{a copy of } q \text{ with }
 ///     \mathit{len}( r ) = \mathit{len}( p ) + 1 \cr
 /// 10 & \qquad\quad \textbf{while}\ p \ne \textbf{nil}\ \textbf{and}
 ///     \ \delta( p , c ) = q\ \textbf{do}\ \delta( p , c ) \gets r;\ p \gets
 ///     \mathit{link}( p ) \cr
 /// 11 & \qquad\quad \mathit{link}( q ) \gets r;\ \mathit{link}( \mathit{cur}
 ///     ) \gets r \cr
 /// 12 & \quad \mathit{last} \gets \mathit{cur}
 /// \end{array}}
 /// \f]
 /// \post at most \f$ 2 |S c| \f$ states (an invariant)
 void extend( unsigned char c );

 /// whether \f$ P \f$ is a substring of \f$ S \f$, by following \f$ P \f$
 /// from the start
 [[nodiscard]] bool contains( std::string_view P ) const;

 /// the number of the occurrences of \f$ P \f$ in \f$ S \f$
 /// (\f$ n + 1 \f$ for the empty \f$ P \f$)
 ///
 /// In pseudocode (beyond the book), the counts made once after the last
 /// extension:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{occurrences}( P ) \cr
 /// 2 & \quad \textbf{if}\ P \text{ is empty}\ \textbf{then return}\ n + 1 \cr
 /// 3 & \quad v \gets \text{the state reached from the start by } P;
 ///     \ \textbf{if}\ v = \textbf{nil}\ \textbf{then return}\ 0 \cr
 /// 4 & \quad \textbf{if}\  \text{the counts are not made}\ \textbf{then} \cr
 /// 5 & \qquad \mathit{cnt}( u ) \gets 1 \text{ for the states that end a }
 ///     \text{prefix of } S \text{, 0 for the clones} \cr
 /// 6 & \qquad \textbf{foreach}\ u \ne \mathit{start} \text{ by decreasing }
 ///     \mathit{len}( u )\ \textbf{do}\ \mathit{cnt}( \mathit{link}( u ) )
 ///     \gets \mathit{cnt}( \mathit{link}( u ) ) + \mathit{cnt}( u ) \cr
 /// 7 & \quad \textbf{return}\ \mathit{cnt}( v )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t occurrences( std::string_view P ) const;

 /// the number of the nonempty distinct substrings of \f$ S \f$,
 /// \f$ \sum_{v \ne \mathit{start}} ( \mathit{len}( v ) - \mathit{len}(
 /// \mathit{link}( v ) ) ) \f$
 [[nodiscard]] std::size_t distinctSubstrings( void ) const;

 /// the length of a longest common substring of \f$ S \f$ and \f$ T \f$,
 /// and where it ends in \f$ T \f$ (one past its last byte), the first one
 /// in \f$ T \f$
 ///
 /// In pseudocode (beyond the book), \f$ l \f$ the length of the longest
 /// suffix of \f$ T[ 0 .. j ] \f$ that is a substring of \f$ S \f$ and
 /// \f$ v \f$ its state:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{longestCommonSubstring}( T ) \cr
 /// 2 & \quad v \gets \mathit{start};\ l \gets 0;\ \mathit{best} \gets 0;
 ///     \ \mathit{end} \gets 0 \cr
 /// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ |T| - 1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ v \ne \mathit{start}\ \textbf{and}\ \delta( v
 ///     , T[ j ] ) = \textbf{nil}\ \textbf{do}\ v \gets \mathit{link}( v );\ l
 ///     \gets \mathit{len}( v ) \cr
 /// 5 & \qquad \textbf{if}\ \delta( v , T[ j ] ) \ne \textbf{nil}
 ///     \ \textbf{then}\ v \gets \delta( v , T[ j ] );\ l \gets l + 1 \cr
 /// 6 & \qquad \textbf{if}\ l > \mathit{best}\ \textbf{then}\ \mathit{best}
 ///     \gets l;\ \mathit{end} \gets j + 1 \cr
 /// 7 & \quad \textbf{return}\ ( \mathit{best} , \mathit{end} )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::pair< std::size_t , std::size_t >
  longestCommonSubstring( std::string_view T ) const;

 /// the number of states
 [[nodiscard]] std::size_t states( void ) const { return( f_len.size() ); }

 /// the length \f$ n \f$ of \f$ S \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the state reached from \f$ v \f$ by \f$ c \f$, `none` if there is no
 /// transition
 [[nodiscard]] std::uint32_t next( std::uint32_t v , unsigned char c ) const;

 /// the counts of the end sets, made after the last extension
 void counts( void ) const;

 static constexpr std::uint32_t none = ~std::uint32_t( 0 );

 std::vector< std::size_t > f_len;                 ///< the longest member
 std::vector< std::uint32_t > f_link;              ///< the suffix links
 std::vector< std::vector< std::pair< unsigned char , std::uint32_t > > >
  f_next;                                          ///< the transitions
 std::vector< std::uint8_t > f_prefix;             ///< ends a prefix of S
 std::uint32_t f_last = 0;                         ///< the state of S
 std::size_t f_n = 0;                              ///< the length of S
 mutable std::vector< std::size_t > f_count;       ///< |endpos|, or empty

 };  // end( class SuffixAutomaton )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SuffixAutomaton.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SuffixAutomaton.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SuffixAutomaton.h -----------------------*/
/*--------------------------------------------------------------------------*/
