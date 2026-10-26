/*--------------------------------------------------------------------------*/
/*-------------------------- File PatriciaTrie.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Patricia trie with the blind search of Ferragina and Grossi, beyond
 * the book: the compacted trie of a sorted dictionary of n strings [see
 * RadixTree in Trie.h, whose edges keep their whole labels] stripped of
 * the labels, each edge keeping only its first character (the branching
 * one) and each node the length of the prefix it stands for, so that it
 * takes O( n ) space whatever the length of the strings, which are stored
 * apart (e.g., on disk). The blind search of a pattern P of p characters
 * finds its lexicographic position (the number of strings smaller than P)
 * reading a single string, in three stages:
 *
 * - it goes down from the root comparing only the branching characters
 *   with the characters of P at the lengths of the nodes, possibly
 *   skipping characters that do not match, until a leaf or a node with no
 *   edge for P, and takes a leaf l below it;
 * - it computes the longest common prefix \f$ \ell \f$ of P and the string
 *   s of l, which shares with P a prefix as long as any other string does;
 * - it finds on the path the edge \f$ ( u , v ) \f$ where \f$ \ell \f$
 *   falls, \f$ | s[ u ] | \le \ell < | s[ v ] | \f$: if \f$ \ell = | s[ u ]
 *   | \f$ the position of P is among the children of u, by its character
 *   \f$ P[ \ell ] \f$, else it is just before or just after the strings
 *   below v, by the comparison of \f$ P[ \ell ] \f$ and \f$ s[ \ell ] \f$.
 *
 * The two traversals take \f$ O( p \log \sigma ) \f$ time with the edges of
 * a node in a sorted array (O( p ) with a perfect hash at each node [see
 * PerfectHashing.h]) and touch no string, and the comparison reads one
 * string, i.e., \f$ O( p / B ) \f$ transfers of blocks against the \f$ O(
 * p ) \f$ of the compacted trie, which reads a label per edge, and the
 * \f$ O( ( p / B ) \log n ) \f$ of the binary search [see PrefixSearch.h].
 * The positions of P and of P# give the range of the strings prefixed by
 * P (Theorem 9.8 of the notes). A string that is a prefix of another ends
 * with a character smaller than all the others, the leaf of a string s
 * having length \f$ | s | + 1 \f$. The trie is built from the sorted
 * strings and the longest common prefixes of adjacent ones in O( N ) time
 * with a stack, N being the characters of the strings.
 *
 * The contracts [see Contract.h]: the strings sorted and distinct (an
 * audit); each position r is such that the strings before r are smaller
 * than the pattern (or than P#) and those from r on are not, a
 * postcondition; the invariant is that every node but the root has at
 * least two children and that the children of a node have increasing
 * characters and consecutive strings. The functions are defined in
 * PatriciaTrie.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9 (sections 9.4 and 9.5)
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

 /// the trie of D
 /// \pre D is sorted and its strings distinct (an audit)
 explicit PatriciaTrie( std::vector< std::string > D );

 /// the number of strings smaller than P, by the blind search
 /// \post D[ r - 1 ] < P <= D[ r ]
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view P ) const;

 /// the number of strings smaller than P#, i.e., smaller than P or
 /// prefixed by P
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range [ l , r ) of the strings prefixed by P
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the leaf l of the first stage of the blind search of P, the only
 /// string that the search reads
 /// \pre size() >= 1
 [[nodiscard]] std::size_t blindLeaf( std::string_view P ) const;

 [[nodiscard]] bool contains( std::string_view P ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the number of nodes, the root and the leaves included
 [[nodiscard]] std::size_t nodes( void ) const;

 /// the string i
 /// \pre i < size()
 [[nodiscard]] const std::string & string( std::size_t i ) const;

 /// the length of the prefix of each node and its branching character, in
 /// preorder (-1 for the end of a string, and for the root)
 [[nodiscard]] std::vector< std::pair< std::size_t , int > > shape( void )
  const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the strings in their order, contiguous

 using value_type = std::string;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const std::string &;
 using const_reference = const std::string &;
 using const_iterator = std::vector< std::string >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_D.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_D.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  std::size_t depth;               ///< the length of its prefix
  std::size_t lo , hi;             ///< its strings [ lo , hi )
  bool leaf;                       ///< the string lo, if a leaf
  std::vector< std::pair< int , std::size_t > > child;  ///< by character
  };

 /// the character i of s, -1 past its end
 [[nodiscard]] static int at( std::string_view s , std::size_t i );

 /// the child of x by the character c, or none
 [[nodiscard]] std::size_t childOf( std::size_t x , int c ) const;

 /// the first stage of the blind search of P followed by the character
 /// term: the path down by the branching characters
 void descend( std::string_view P , int term ,
               std::vector< std::size_t > & path ) const;

 /// the blind search of P followed by the character term (-1 for P, 256
 /// for P#); the path of the first stage in path
 [[nodiscard]] std::size_t blind( std::string_view P , int term ,
                                  std::vector< std::size_t > & path ) const;

 /// the first leaf below each node, its strings and their depths, O( N )
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
