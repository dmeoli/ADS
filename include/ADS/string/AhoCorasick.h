/*--------------------------------------------------------------------------*/
/*--------------------------- File AhoCorasick.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search of many patterns at once, beyond the book: the automaton of
 * Aho and Corasick of a set of patterns \f$ P_0 , \ldots , P_{k-1} \f$ of
 * \f$ M \f$ bytes in all finds all their occurrences in a text \f$ T \f$ of
 * \f$ n \f$ bytes in one scan, \f$ O( n + \mathit{occ} ) \f$ time.
 *
 * The states are the nodes of the trie of the patterns, a state being the
 * string \f$ u \f$ spelt from the root to it. The <em>failure</em> of
 * \f$ u \f$ is the state of the longest proper suffix of \f$ u \f$ that is
 * in the trie, computed in breadth first order from
 * \f$ \mathit{fail}( u c ) = \delta( \mathit{fail}( u ) , c ) \f$; with it
 * the automaton is made deterministic,
 * \f[
 *   \delta( u , c ) = \left\{ \begin{array}{ll}
 *     u c & \mbox{if } u c \mbox{ is in the trie} \cr
 *     \delta( \mathit{fail}( u ) , c ) & \mbox{otherwise, } u \ne
 *       \varepsilon \cr
 *     \varepsilon & \mbox{otherwise,}
 *   \end{array} \right.
 * \f]
 * a table of 256 transitions per state, so that a byte of \f$ T \f$ costs
 * one lookup, after a preparation of \f$ O( 256 M ) \f$ time and space.
 * After reading \f$ T[ 0 .. j ] \f$ the state is the longest suffix of it
 * that is a prefix of some pattern (the invariant of the automaton), and
 * the patterns that end at \f$ j \f$ are those of the state and of the
 * chain of its output links, each the nearest proper suffix of a state that
 * is a whole pattern, so that each occurrence costs \f$ O( 1 ) \f$.
 *
 * `add()` gives each pattern its number, in the order of the calls (equal
 * patterns get one number each); `build()` prepares the automaton, which
 * `findAll()` and `count()` then use: `findAll()` gives the pairs
 * \f$ ( \mathit{start} , \mathit{number} ) \f$ of the occurrences, by
 * increasing end, and the longer pattern first for one end. The empty
 * pattern is not allowed.
 *
 * The contracts [see `Contract.h`]: a pattern is not empty, and no search
 * comes before `build()`; `findAll()` gives, for each pattern, the
 * occurrences of `naiveMatching()` [see `StringMatching.h`] (an audit). The
 * functions are defined in `AhoCorasick.tpp`.
 *
 * \see A. V. Aho, M. J. Corasick, Efficient string matching: an aid to
 *      bibliographic search, Communications of the ACM 18(6), 333-340,
 *      1975, doi:10.1145/360825.360855
 * \see D. Gusfield, Algorithms on Strings, Trees and Sequences, Cambridge
 *      University Press, 1997, section 3.4, doi:10.1017/CBO9780511574931
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

#ifndef __ADS_AhoCorasick
 #define __ADS_AhoCorasick
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS AhoCorasick ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the automaton of Aho and Corasick of a set of patterns
class AhoCorasick {

/*--------------------------------------------------------------------------*/
 public:

 /// no pattern
 AhoCorasick( void );

 /// the automaton of the patterns \f$ P \f$, numbered
 /// \f$ 0 , 1 , \ldots \f$ in their order, built
 /// \pre no pattern is empty
 explicit AhoCorasick( const std::vector< std::string > & P );

 /// adds the pattern \f$ P \f$ to the trie, the automaton to be built
 /// again; its number
 ///
 /// In pseudocode (Aho and Corasick, beyond the book), the trie rebuilt
 /// from the patterns first if the automaton was built:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{add}( P ) \cr
 /// 2 & \quad u \gets \varepsilon\quad \triangleright\ \text{the root} \cr
 /// 3 & \quad \textbf{foreach}\ c \in P\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ u c \text{ is not in the trie }\ \textbf{then}
 ///     \ \text{add the state } u c \text{ as a child of } u \cr
 /// 5 & \qquad u \gets u c \cr
 /// 6 & \quad \text{append the number } k \text{ of } P \text{ to the }
 ///     \text{patterns of } u;\ k \gets k + 1 \cr
 /// 7 & \quad \textbf{return}\ k - 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ P \f$ is not empty
 std::size_t add( std::string_view P );

 /// the failures, the transitions and the output links, in
 /// \f$ O( 256 M ) \f$
 ///
 /// In pseudocode (Aho and Corasick, beyond the book), the states in
 /// breadth first order, so that the failure of a state and its transitions
 /// are complete before those of its children are computed:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}() \cr
 /// 2 & \quad Q \gets \text{an empty queue} \cr
 /// 3 & \quad \textbf{foreach}\ c\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ c \text{ is in the trie}\ \textbf{then}
 ///     \ \mathit{fail}( c ) \gets \varepsilon;\ \mathrm{enqueue}( Q , c ) \cr
 /// 5 & \qquad \textbf{else}\ \delta( \varepsilon , c ) \gets \varepsilon \cr
 /// 6 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 7 & \qquad u \gets \mathrm{dequeue}( Q );\ f \gets \mathit{fail}( u ) \cr
 /// 8 & \qquad \mathit{out}( u ) \gets f \text{ if } f \text{ is a pattern,}
 ///     \text{ else } \mathit{out}( f ) \cr
 /// 9 & \qquad \textbf{foreach}\ c\ \textbf{do} \cr
 /// 10 & \qquad\quad \textbf{if}\ u c \text{ is in the trie}\ \textbf{then}
 ///     \ \mathit{fail}( u c ) \gets \delta( f , c );\ \mathrm{enqueue}( Q , u
 ///     c ) \cr
 /// 11 & \qquad\quad \textbf{else}\ \delta( u , c ) \gets \delta( f , c )
 /// \end{array}}
 /// \f]
 void build( void );

 /// the occurrences \f$ ( \mathit{start} , \mathit{number} ) \f$ of the
 /// patterns in \f$ T \f$, by increasing end, the longer pattern first for
 /// one end
 ///
 /// In pseudocode (Aho and Corasick, beyond the book), one transition per
 /// byte and one step of the output links per occurrence:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findAll}( T ) \cr
 /// 2 & \quad s \gets \varepsilon;\ R \gets \emptyset \cr
 /// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 4 & \qquad s \gets \delta( s , T[ j ] ) \cr
 /// 5 & \qquad x \gets s \text{ if } s \text{ is a pattern, else }
 ///     \mathit{out}( s ) \cr
 /// 6 & \qquad \textbf{while}\ x \ne \varepsilon\ \textbf{do} \cr
 /// 7 & \qquad\quad \textbf{foreach}\ \text{pattern } P_i \text{ of } x
 ///     \ \textbf{do}\ R \gets R \cup \{ ( j + 1 - |P_i| , i ) \} \cr
 /// 8 & \qquad\quad x \gets \mathit{out}( x ) \cr
 /// 9 & \quad \textbf{return}\ R
 /// \end{array}}
 /// \f]
 /// \pre built
 /// \post the occurrences of `naiveMatching()` for each pattern (audit)
 [[nodiscard]] std::vector< std::pair< std::size_t , std::size_t > >
  findAll( std::string_view T ) const;

 /// the number of the occurrences of the patterns in \f$ T \f$, by the
 /// scan of `findAll()`
 /// \pre built
 [[nodiscard]] std::size_t count( std::string_view T ) const;

 /// each occurrence \f$ ( \mathit{start} , \mathit{number} ) \f$ of the
 /// patterns in \f$ T \f$ given to `report()`, by increasing end, the
 /// longer pattern first for one end
 /// \pre built
 template< class F >
 void forEach( std::string_view T , F && report ) const {
  scan( T , [ & ]( std::size_t j , std::uint32_t id ) {
   report( j + 1 - f_P[ id ].size() , std::size_t( id ) ); } );
  }

 /// the number of patterns
 [[nodiscard]] std::size_t size( void ) const { return( f_P.size() ); }

 /// the number of states, the nodes of the trie
 [[nodiscard]] std::size_t states( void ) const { return( f_go.size() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the occurrences in T, each ( end , number ) given to report()
 template< class F >
 void scan( std::string_view T , F && report ) const;

 std::vector< std::string > f_P;                 ///< the patterns
 std::vector< std::array< std::uint32_t , 256 > > f_go;  ///< delta
 std::vector< std::uint32_t > f_fail;            ///< the failures
 std::vector< std::uint32_t > f_out;  ///< the next state of a pattern, 0
 std::vector< std::vector< std::uint32_t > > f_ends;  ///< its patterns
 bool f_built = true;

 };  // end( class AhoCorasick )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/AhoCorasick.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* AhoCorasick.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File AhoCorasick.h --------------------------*/
/*--------------------------------------------------------------------------*/
