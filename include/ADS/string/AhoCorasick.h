/*--------------------------------------------------------------------------*/
/*--------------------------- File AhoCorasick.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The search of many patterns at once, beyond the book: the automaton of
 * Aho and Corasick of a set of patterns \f$ P_0 , \ldots , P_{k-1} \f$ of
 * M bytes in all finds all their occurrences in a text T of n bytes in one
 * scan, \f$ O( n + \mathit{occ} ) \f$ time.
 *
 * The states are the nodes of the trie of the patterns, a state being the
 * string u spelt from the root to it. The failure of u is the state of the
 * longest proper suffix of u that is in the trie, computed in breadth
 * first order from \f$ \mathit{fail}( u c ) = \delta( \mathit{fail}( u ) ,
 * c ) \f$; with it the automaton is made deterministic,
 * \f[
 *   \delta( u , c ) = \left\{ \begin{array}{ll}
 *     u c & \mbox{if } u c \mbox{ is in the trie} \\
 *     \delta( \mathit{fail}( u ) , c ) & \mbox{otherwise, } u \ne
 *       \varepsilon \\
 *     \varepsilon & \mbox{otherwise,}
 *   \end{array} \right.
 * \f]
 * a table of 256 transitions per state, so that a byte of T costs one
 * lookup, after a preparation of \f$ O( 256 M ) \f$ time and space. After
 * reading \f$ T[ 0 .. j ] \f$ the state is the longest suffix of it that is
 * a prefix of some pattern (the invariant of the automaton), and the
 * patterns that end at j are those of the state and of the chain of its
 * output links, each the nearest proper suffix of a state that is a whole
 * pattern, so that each occurrence costs O( 1 ).
 *
 * add() gives each pattern its number, in the order of the calls (equal
 * patterns get one number each); build() prepares the automaton, which
 * findAll() and count() then use: findAll() gives the pairs ( start ,
 * number ) of the occurrences, by increasing end, and the longer pattern
 * first for one end. The empty pattern is not allowed.
 *
 * The contracts [see Contract.h]: a pattern is not empty, and no search
 * comes before build(); findAll() gives, for each pattern, the occurrences
 * of naiveMatching() [see StringMatching.h] (an audit). The functions are
 * defined in AhoCorasick.tpp.
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

 /// the patterns P, numbered 0 , 1 , ... in their order, built
 /// \pre no pattern is empty
 explicit AhoCorasick( const std::vector< std::string > & P );

 /// adds the pattern P, the automaton to be built again; its number
 /// \pre P is not empty
 std::size_t add( std::string_view P );

 /// the failures, the transitions and the output links, \f$ O( 256 M ) \f$
 void build( void );

 /// the occurrences ( start , number ) of the patterns in T, by increasing
 /// end, the longer pattern first for one end
 /// \pre built
 /// \post the occurrences of naiveMatching() for each pattern (an audit)
 [[nodiscard]] std::vector< std::pair< std::size_t , std::size_t > >
  findAll( std::string_view T ) const;

 /// the number of the occurrences of the patterns in T
 /// \pre built
 [[nodiscard]] std::size_t count( std::string_view T ) const;

 /// each occurrence ( start , number ) of the patterns in T given to
 /// report(), by increasing end, the longer pattern first for one end
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
