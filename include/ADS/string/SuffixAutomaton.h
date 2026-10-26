/*--------------------------------------------------------------------------*/
/*------------------------- File SuffixAutomaton.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The suffix automaton (the directed acyclic word graph, DAWG) of a string
 * S of n bytes, beyond the book: the smallest deterministic automaton that
 * accepts the suffixes of S, whose paths from the start spell exactly the
 * substrings of S. Its states are the classes of the substrings by their
 * sets of end places,
 * \f[
 *   \mathrm{endpos}( u ) = \{ j : S[ j - |u| + 1 .. j ] = u \} ,
 * \f]
 * a class holding the suffixes of its longest member of lengths \f$ (
 * \mathit{len}( \mathit{link}( v ) ) , \mathit{len}( v ) ] \f$, where the
 * suffix link of v is the class of the longest suffix in another class. It has
 * at most \f$ 2 n - 1 \f$ states (n at least 2) and \f$ 3 n - 4 \f$
 * transitions (n at least 3), and the construction of Blumer et al. adds the
 * bytes one at a time: the new state of \f$ S c \f$ gets the transitions on c
 * from the suffix path of the last state, which stops at the first state p
 * that has one; if it leads to q with \f$ \mathit{len}( q ) = \mathit{len}( p
 * ) + 1 \f$, q is the new link, otherwise q is cloned with that length,
 * \f$ O( n ) \f$ amortized time for a constant alphabet. Here the transitions
 * of a state are a short vector of ( byte , state ) pairs, searched linearly,
 * so that the time is \f$ O( n \sigma ) \f$ for \f$ \sigma \f$ distinct bytes.
 *
 * - contains( P ) follows P from the start, \f$ O( |P| \sigma ) \f$;
 * - occurrences( P ) is the size of the end set of the state of P, counted
 *   once for all by adding the count of each state (1 if it ends a prefix
 *   of S, 0 for a clone) to its link, in decreasing order of the lengths;
 * - distinctSubstrings() is \f$ \sum_{v \ne \mathit{start}} (
 *   \mathit{len}( v ) - \mathit{len}( \mathit{link}( v ) ) ) \f$, the
 *   number of nonempty distinct substrings of S;
 * - longestCommonSubstring( T ) scans T keeping the longest suffix of
 *   \f$ T[ 0 .. j ] \f$ that is a substring of S, following the links on a
 *   mismatch, \f$ O( |T| \sigma ) \f$.
 *
 * The contracts [see Contract.h]: at most 2 n states (an invariant),
 * and the results equal those of the definitions on the substrings (the
 * audits of the tests). The functions are defined in SuffixAutomaton.tpp.
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

 /// the automaton of the empty string
 SuffixAutomaton( void );

 /// the automaton of S
 explicit SuffixAutomaton( std::string_view S );

 /// the automaton of S c from that of S
 /// \post at most 2 |S c| states (an invariant)
 void extend( unsigned char c );

 /// P is a substring of S
 [[nodiscard]] bool contains( std::string_view P ) const;

 /// the number of the occurrences of P in S (n + 1 for the empty P)
 [[nodiscard]] std::size_t occurrences( std::string_view P ) const;

 /// the number of the nonempty distinct substrings of S
 [[nodiscard]] std::size_t distinctSubstrings( void ) const;

 /// the length of a longest common substring of S and T, and where it
 /// ends in T (one past its last byte), the first one in T
 [[nodiscard]] std::pair< std::size_t , std::size_t >
  longestCommonSubstring( std::string_view T ) const;

 /// the number of states
 [[nodiscard]] std::size_t states( void ) const { return( f_len.size() ); }

 /// the length of S
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the state reached from v by c, none if there is no transition
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
 std::size_t f_n = 0;
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
