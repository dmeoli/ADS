/*--------------------------------------------------------------------------*/
/*-------------------------- File PatriciaTrie.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Patricia trie with the blind search of Ferragina and Grossi, beyond
 * the book: the compacted trie of a sorted dictionary of \f$ n \f$ strings
 * [see `RadixTree` in `Trie.h`, whose edges keep their whole labels]
 * stripped of the labels, each edge keeping only its first character (the
 * branching one) and each node the length of the prefix it stands for, so
 * that it takes \f$ O( n ) \f$ space whatever the length of the strings,
 * which are stored apart (e.g., on disk). The blind search of a pattern
 * \f$ P \f$ of \f$ p \f$ characters finds its lexicographic position (the
 * number of strings smaller than \f$ P \f$) reading a single string, in
 * three stages:
 *
 * - it goes down from the root comparing only the branching characters
 *   with the characters of \f$ P \f$ at the lengths of the nodes, possibly
 *   skipping characters that do not match, until a leaf or a node with no
 *   edge for \f$ P \f$, and takes a leaf \f$ l \f$ below it;
 * - it computes the longest common prefix \f$ \ell \f$ of \f$ P \f$ and the
 *   string \f$ s \f$ of \f$ l \f$, which shares with \f$ P \f$ a prefix as
 *   long as any other string does;
 * - it finds on the path the edge \f$ ( u , v ) \f$ where \f$ \ell \f$
 *   falls, \f$ | s[ u ] | \le \ell < | s[ v ] | \f$: if
 *   \f$ \ell = | s[ u ] | \f$ the position of \f$ P \f$ is among the
 *   children of \f$ u \f$, by its character \f$ P[ \ell ] \f$, else it is
 *   just before or just after the strings below \f$ v \f$, by the comparison
 *   of \f$ P[ \ell ] \f$ and \f$ s[ \ell ] \f$.
 *
 * The two traversals take \f$ O( p \log \sigma ) \f$ time with the edges of
 * a node in a sorted array (\f$ O( p ) \f$ with a perfect hash at each node
 * [see `PerfectHashing.h`]) and touch no string, and the comparison reads
 * one string, i.e., \f$ O( p / B ) \f$ transfers of blocks against the
 * \f$ O( p ) \f$ of the compacted trie, which reads a label per edge, and
 * the \f$ O( ( p / B ) \log n ) \f$ of the binary search [see
 * `PrefixSearch.h`]. The positions of \f$ P \f$ and of \f$ P\# \f$ give the
 * range of the strings prefixed by \f$ P \f$ (chapter 9 of Ferragina). A
 * string that is a prefix of another ends with a character smaller than all
 * the others, the leaf of a string \f$ s \f$ having length
 * \f$ | s | + 1 \f$. The trie is built from the sorted strings and the
 * longest common prefixes of adjacent ones in \f$ O( N ) \f$ time with a
 * stack, \f$ N \f$ being the characters of the strings.
 *
 * The contracts [see `Contract.h`]: the strings sorted and distinct (an
 * audit); each position \f$ r \f$ is such that the strings before \f$ r \f$
 * are smaller than the pattern (or than \f$ P\# \f$) and those from
 * \f$ r \f$ on are not, a postcondition; the invariant is that every node but
 * the root has at least two children and that the children of a node have
 * increasing characters and consecutive strings. The functions are defined
 * in `PatriciaTrie.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9, Searching Strings by Prefix,
 *      doi:10.1017/9781009128933
 * \see D. R. Morrison, PATRICIA: practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
 * \see P. Ferragina, R. Grossi, The string B-tree: a new data structure
 *      for string search in external memory and its applications, Journal
 *      of the ACM 46(2), 236-280, 1999, doi:10.1145/301970.301973
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

#ifndef __ADS_PatriciaTrie
 #define __ADS_PatriciaTrie
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/PrefixSearch.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PatriciaTrie ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a sorted dictionary of strings on a Patricia trie, with the blind search
class PatriciaTrie {

/*--------------------------------------------------------------------------*/
 public:

 /// the trie of \f$ D \f$, built in \f$ O( N ) \f$ time
 ///
 /// In pseudocode (chapter 9 of Ferragina), the nodes of the
 /// rightmost path on a stack, \f$ \ell_i \f$ the longest common prefix of
 /// \f$ D[ i - 1 ] \f$ and \f$ D[ i ] \f$ (0 for \f$ i = 0 \f$), the depth of
 /// a node the length of its prefix:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( D ) \cr
 /// 2 & \quad \mathit{st} \gets ( \mathit{root} ) \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{while}\ \text{the top of } \mathit{st} \text{ is }
 ///     \text{deeper than } \ell_i\ \textbf{do}\ \mathit{last} \gets
 ///     \mathrm{pop}( \mathit{st} ) \cr
 /// 5 & \qquad \textbf{if}\ \text{the top is shallower than } \ell_i
 ///     \ \textbf{then} \cr
 /// 6 & \qquad\quad \text{split its last edge by a node } u \text{ of }
 ///     \text{depth } \ell_i \text{, parent of } \mathit{last};
 ///     \ \mathrm{push}( \mathit{st} , u ) \cr
 /// 7 & \qquad \text{add the leaf of } D[ i ] \text{ as the last child }
 ///     \text{of the top};\ \mathrm{push}( \mathit{st} , \text{the leaf} ) \cr
 /// 8 & \quad \text{give each node the range of the strings below it, bottom }
 ///     \text{up}
 /// \end{array}}
 /// \f]
 /// \pre \f$ D \f$ is sorted and its strings distinct (an audit)
 explicit PatriciaTrie( std::vector< std::string > D );

 /// the number of strings smaller than \f$ P \f$, by the blind search
 ///
 /// In pseudocode (the blind search, chapter 9 of
 /// Ferragina), \f$ |x| \f$ the depth of the node \f$ x \f$, \f$ x.\mathit{lo}
 /// \f$ and \f$ x.\mathit{hi} \f$ the range of its strings, \f$ P \f$ followed
 /// by a character smaller than all the others:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{blind}( P ) \cr
 /// 2 & \quad \pi \gets \text{the path from the root down by the characters }
 ///     P[ |x| ] \text{ of the nodes} \cr
 /// 3 & \quad s \gets D[ ( \text{the last node of } \pi ).\mathit{lo} ]\quad
 ///     \triangleright\ \text{the one string read} \cr
 /// 4 & \quad \ell \gets \text{the longest common prefix of } P \text{ and } s
 ///     \cr
 /// 5 & \quad \textbf{if}\ \ell = |P| + 1\ \textbf{then return}\ \text{the }
 ///     \text{index of } s \cr
 /// 6 & \quad ( u , v ) \gets \text{the edge of } \pi \text{ with } |u| \le
 ///     \ell < |v| \cr
 /// 7 & \quad \textbf{if}\ |u| = \ell\ \textbf{then return}\ \text{the first }
 ///     \text{string of the first child of } u \text{ after } P[ \ell ] \cr
 /// 8 & \quad \textbf{if}\ P[ \ell ] > s[ \ell ]\ \textbf{then return}
 ///     \ v.\mathit{hi}\ \textbf{else return}\ v.\mathit{lo}
 /// \end{array}}
 /// \f]
 /// \post \f$ D[ r - 1 ] < P \le D[ r ] \f$
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view P ) const;

 /// the number of strings smaller than \f$ P\# \f$, i.e., smaller than
 /// \f$ P \f$ or prefixed by \f$ P \f$, by the blind search of `blind()`
 /// with \f$ P \f$ followed by a character larger than all the others
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range \f$ [ l , r ) \f$ of the strings prefixed by \f$ P \f$, i.e.,
/// `lexicographicPosition( P )` and `prefixEnd( P )`
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the leaf \f$ l \f$ of the first stage of the blind search of \f$ P \f$,
 /// the only string that the search reads
 /// \pre `size()` \f$ \ge 1 \f$
 [[nodiscard]] std::size_t blindLeaf( std::string_view P ) const;

 /// true if \f$ P \f$ is one of the strings
 [[nodiscard]] bool contains( std::string_view P ) const;

 /// the number of strings
 [[nodiscard]] std::size_t size( void ) const;

 /// the number of nodes, the root and the leaves included
 [[nodiscard]] std::size_t nodes( void ) const;

 /// the string \f$ i \f$
 /// \pre \f$ i \f$ is less than `size()`
 [[nodiscard]] const std::string & string( std::size_t i ) const;

 /// the length of the prefix of each node and its branching character, in
 /// preorder (-1 for the end of a string, and for the root)
 [[nodiscard]] std::vector< std::pair< std::size_t , int > > shape( void )
  const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the strings in their order, contiguous

 /// the type of the items, for the standard library
 using value_type = std::string;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = const std::string &;
 /// a constant reference to an item
 using const_reference = const std::string &;
 /// the iterator on the items, constant
 using const_iterator = std::vector< std::string >::const_iterator;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_D.begin() ); }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_D.end() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::size_t depth;               ///< the length of its prefix
  std::size_t lo ,                 ///< the first of its strings
              hi;                 ///< past the last of its strings
  bool leaf;                       ///< the string lo, if a leaf
  std::vector< std::pair< int , std::size_t > > child;  ///< by character
  };

 /// the character \f$ i \f$ of \f$ s \f$, -1 past its end
 [[nodiscard]] static int at( std::string_view s , std::size_t i );

 /// the child of \f$ x \f$ by the character \f$ c \f$, or none
 [[nodiscard]] std::size_t childOf( std::size_t x , int c ) const;

 /// the first stage of the blind search of \f$ P \f$ followed by the
 /// character `term`: the path down by the branching characters
 void descend( std::string_view P , int term ,
               std::vector< std::size_t > & path ) const;

 /// the blind search of \f$ P \f$ followed by the character `term` (-1 for
 /// \f$ P \f$, 256 for \f$ P\# \f$); the path of the first stage in `path`
 [[nodiscard]] std::size_t blind( std::string_view P , int term ,
                                  std::vector< std::size_t > & path ) const;

 /// the first leaf below each node, its strings and their depths,
 /// \f$ O( N ) \f$
 [[nodiscard]] bool wellFormed( void ) const;

 static constexpr std::size_t kNone = ~std::size_t( 0 );

 std::vector< std::string > f_D;
 std::vector< Node > f_N;          ///< the root at 0

 };  // end( class PatriciaTrie )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PatriciaTrie.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PatriciaTrie.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PatriciaTrie.h -------------------------*/
/*--------------------------------------------------------------------------*/
