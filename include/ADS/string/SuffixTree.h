/*--------------------------------------------------------------------------*/
/*--------------------------- File SuffixTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The suffix tree of a text \f$ T \f$ of \f$ n \f$ symbols, beyond the
 * book: the compact trie [see `RadixTree` in `Trie.h`] of the \f$ n + 1 \f$
 * suffixes of \f$ T \$ \f$, where \f$ \$ \f$ is a terminator smaller than
 * every symbol, so that no suffix is a prefix of another and each one ends
 * in a leaf of its own. Every internal node but the root has at least two
 * children; hence there are at most \f$ n \f$ internal nodes. The label of
 * an edge is a substring of \f$ T \f$, kept as the place of one occurrence
 * and the string depths of its two ends, \f$ O( 1 ) \f$ space, and the tree
 * takes \f$ O( n ) \f$ space, though some
 * 10 to 20 \f$ n \f$ bytes in practice against the \f$ n \f$ integers of a
 * suffix array [see `SuffixArray.h`]. The children of a node are kept by
 * their first symbol in a balanced search tree (`std::map`), so that
 * branching costs \f$ O( \log \sigma ) \f$ for an alphabet of
 * \f$ \sigma \f$ symbols, and they are visited in the lexicographic order.
 * The node \f$ u \f$ spells \f$ s[ u ] \f$, the string on the path from the
 * root, and it is an internal node exactly when \f$ s[ u ] \f$ occurs at
 * least twice followed by distinct symbols; the lowest
 * common ancestor of the leaves \f$ i \f$ and \f$ j \f$ spells the longest
 * common prefix of the suffixes \f$ i \f$ and \f$ j \f$.
 *
 * The constructions, each giving the same tree:
 *
 * - `Build::naive` inserts the suffixes from the longest one, each by a walk
 *   down from the root that compares all its symbols, \f$ \Theta( n^2 ) \f$
 *   time for the text \f$ a^{n-1} \f$;
 * - `Build::mcCreight` is the algorithm of McCreight: the suffixes are
 *   inserted from the longest one too, but the walk of the suffix \f$ i \f$
 *   starts from the suffix link of the node of \f$ \mathit{head}_{i-1} \f$,
 *   the longest prefix of the suffix \f$ i - 1 \f$ that occurs earlier,
 *   which spells \f$ \mathit{head}_{i-1} \f$ without its first symbol; when
 *   that link is not set yet, it is found from the link of the parent by a
 *   rescan that compares only the first symbol of each edge, whose total is
 *   \f$ O( n ) \f$ since the depth of the tree drops by at most 2 per link.
 *   It takes \f$ O( n \log \sigma ) \f$ time;
 * - `Build::suffixArray` builds the suffix array by SA-IS and the LCP array
 *   by Kasai et al., then inserts the leaves in the order of
 *   \f$ \mathit{SA} \f$, each one climbing from the previous leaf, along
 *   the rightmost path, to the deepest node of string depth at most
 *   \f$ \mathit{LCP}[ r - 1 ] \f$, and splitting the edge below it when its
 *   depth is larger; each edge climbed is left to the left for good, hence
 *   \f$ O( n ) \f$ steps.
 *
 * Weiner gave the first linear construction (1973), inserting the suffixes
 * from the shortest one; Ukkonen's algorithm is the online form of
 * McCreight's, which reads \f$ T \f$ from left to right and keeps the tree
 * of each prefix; Farach reached \f$ O( n ) \f$ on integer alphabets by
 * merging the trees of the odd and of the even suffixes, the idea that the
 * Skew algorithm turned into the split \f$ 2/3 : 1/3 \f$ of the suffix
 * arrays. Since the linear suffix arrays (SA-IS, divsufsort) are smaller
 * and faster, the construction through \f$ \mathit{SA} \f$ and
 * \f$ \mathit{LCP} \f$ is the usual one; McCreight's pays where the
 * average lcp is small.
 *
 * `locus()` finds the extended locus of a pattern \f$ P \f$ of \f$ p \f$
 * symbols by a walk down from the root, \f$ O( p \log \sigma ) \f$ time,
 * and `locate()` the \f$ \mathit{occ} \f$ occurrences below it in
 * \f$ O( \mathit{occ} ) \f$, since that subtree has \f$ \mathit{occ} \f$
 * leaves and fewer internal nodes. `lca()` answers the lowest
 * common ancestor of two nodes in \f$ O( 1 ) \f$ time by the Euler tour of
 * the tree and the range minima of its depths [see
 * `PlusMinusOneRangeMinimum` in `RangeMinimum.h`], hence `lce()` the
 * longest common extension of two suffixes.
 * `toSuffixArray()` reads \f$ \mathit{SA} \f$ and \f$ \mathit{LCP} \f$ back
 * by a visit in order, the leaves giving \f$ \mathit{SA} \f$ and the
 * internal node met between two leaves their lcp.
 *
 * The contracts [see `Contract.h`]: the invariant of the tree
 * (\f$ n + 1 \f$ leaves, at least two children at each internal node but
 * the root, the first symbol of each edge equal to its key, the string
 * depths increasing downward) after each construction; the places and the
 * nodes of the queries exist. The functions are defined in
 * `SuffixTree.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10, Searching Strings by Substring,
 *      doi:10.1017/9781009128933
 * \see E. M. McCreight, A space-economical suffix tree construction
 *      algorithm, Journal of the ACM 23(2), 262-272, 1976,
 *      doi:10.1145/321941.321946
 * \see P. Weiner, Linear pattern matching algorithms, Proceedings of the
 *      14th Annual Symposium on Switching and Automata Theory (SWAT), 1-11,
 *      1973, doi:10.1109/SWAT.1973.13
 * \see E. Ukkonen, On-line construction of suffix trees, Algorithmica
 *      14(3), 249-260, 1995, doi:10.1007/BF01206331
 * \see M. Farach, Optimal suffix tree construction with large alphabets,
 *      Proceedings of the 38th Annual Symposium on Foundations of Computer
 *      Science (FOCS), 137-143, 1997, doi:10.1109/SFCS.1997.646102
 * \see D. Gusfield, Algorithms on Strings, Trees and Sequences, Cambridge
 *      University Press, 1997, doi:10.1017/CBO9780511574931
 * \see G. Nong, S. Zhang, W. H. Chan, Two efficient algorithms for linear
 *      time suffix array construction, IEEE Transactions on Computers
 *      60(10), 1471-1484, 2011, doi:10.1109/TC.2010.188
 * \see T. Kasai, G. Lee, H. Arimura, S. Arikawa, K. Park, Linear-time
 *      longest-common-prefix computation in suffix arrays and its
 *      applications, Proceedings of the 12th Symposium on Combinatorial
 *      Pattern Matching (CPM), LNCS 2089, 181-192, 2001,
 *      doi:10.1007/3-540-48194-X_17
 * \see M. A. Bender, M. Farach-Colton, The LCA problem revisited,
 *      Proceedings of the 4th Latin American Symposium on Theoretical
 *      Informatics (LATIN), LNCS 1776, 88-94, 2000,
 *      doi:10.1007/10719839_9
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

#ifndef __ADS_SuffixTree
 #define __ADS_SuffixTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/RangeMinimum.h"
#include "ADS/string/SuffixArray.h"

#include <cstddef>
#include <limits>
#include <map>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS SuffixTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the suffix tree of \f$ T \$ \f$, its nodes in a vector, the root at 0
class SuffixTree {

/*--------------------------------------------------------------------------*/
 public:

 /// no node, no suffix
 static constexpr std::size_t npos = std::numeric_limits< std::size_t >::max();

 /// the construction
 enum class Build {
  naive ,         ///< each suffix by a walk down from the root
  mcCreight ,     ///< the suffix links of McCreight
  suffixArray     ///< the leaves in the order of SA, climbing by LCP
  };

 /// a node \f$ u \f$; the edge into it is labelled by the symbols of
 /// \f$ T \f$ from `pos` plus the depth of the parent to `pos + depth` (the
 /// terminator at \f$ n \f$)
 struct Node {
  std::size_t pos = 0;          ///< where \f$ s[ u ] \f$ occurs once
  std::size_t depth = 0;        ///< the string depth \f$ |s[ u ]| \f$
  std::size_t parent = npos;    ///< `npos` at the root
  std::size_t suffix = npos;    ///< the suffix of a leaf, `npos` inside
  std::size_t leaves = 0;       ///< the leaves below
  std::map< std::size_t , std::size_t > child;   ///< by first symbol
  };

 /// the tree of no text, with no node
 SuffixTree( void ) = default;

 /// the suffix tree of the bytes of \f$ T \f$, built as `b` says
 ///
 /// In pseudocode (the algorithm of McCreight, beyond the book), the three
 /// constructions; \f$ \mathrm{scan}( u , i ) \f$ walks down from
 /// \f$ u \f$ along the suffix \f$ i \f$, comparing every symbol, splits the
 /// edge where they differ and returns the node reached, and
 /// \f$ \mathrm{leaf}( u , i ) \f$ hangs the leaf of the suffix \f$ i \f$
 /// below \f$ u \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{naive}( T ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n\ \textbf{do}
 ///     \ \mathrm{leaf}( \mathrm{scan}( \mathit{root} , i ) , i ) \cr
 /// 3 & \textbf{procedure}\ \mathrm{mcCreight}( T ) \cr
 /// 4 & \quad \mathrm{leaf}( \mathit{root} , 0 );\ h \gets \mathit{root}
 ///     \quad \triangleright\ \text{the node of } \mathit{head}_{i-1} \cr
 /// 5 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ n\ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ \mathit{link}( h ) = \textbf{nil}\ \textbf{then}
 ///     \cr
 /// 7 & \qquad\quad w \gets \text{the node of } s[ h ] \text{ without its }
 ///     \text{first symbol, rescanned from } \mathit{link}( \mathit{parent}(
 ///     h ) ) \cr
 ///   & \qquad\qquad \text{by the first symbol of each edge, splitting the }
 ///     \text{last one if needed};\ \mathit{link}( h ) \gets w \cr
 /// 8 & \qquad h \gets \mathrm{scan}( \mathit{link}( h ) , i );
 ///     \ \mathrm{leaf}( h , i ) \cr
 /// 9 & \textbf{procedure}\ \mathrm{fromSuffixArray}( T ) \cr
 /// 10 & \quad ( \mathit{SA} , \mathit{LCP} ) \gets \text{the arrays of }
 ///     \mathrm{suffixArraySais}() \text{ and } \mathrm{lcpArray}() \cr
 /// 11 & \quad x \gets \mathrm{leaf}( \mathit{root} , n )\quad
 ///     \triangleright\ \text{the terminator, the smallest suffix} \cr
 /// 12 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 13 & \qquad l \gets \mathit{LCP}[ r - 1 ] \text{, or 0 if } r = 0 \cr
 /// 14 & \qquad \textbf{while}\ \mathit{depth}( \mathit{parent}( x ) ) > l
 ///     \ \textbf{do}\ x \gets \mathit{parent}( x ) \cr
 /// 15 & \qquad u \gets \mathit{parent}( x ) \text{, or a new node at depth }
 ///     l \text{ on the edge to } x \text{ if } \mathit{depth}( u ) < l \cr
 /// 16 & \qquad x \gets \mathrm{leaf}( u , \mathit{SA}[ r ] )
 /// \end{array}}
 /// \f]
 explicit SuffixTree( std::string_view T , Build b = Build::mcCreight );

 /// the suffix tree of an integer text, built as the one of the bytes above
 /// \pre every symbol of \f$ T \f$ is less than \f$ K \f$
 SuffixTree( const Symbols & T , std::size_t K ,
             Build b = Build::mcCreight );

 /// the length \f$ n \f$ of \f$ T \f$, without the terminator
 [[nodiscard]] std::size_t textSize( void ) const;

 /// the root, the node 0
 [[nodiscard]] std::size_t root( void ) const { return( 0 ); }

 /// the node \f$ u \f$
 /// \pre \f$ u \f$ is less than `size()`
 [[nodiscard]] const Node & node( std::size_t u ) const;

 /// the leaf of the suffix \f$ i \f$, the terminator being the suffix
 /// \f$ n \f$
 /// \pre \f$ i \le n \f$
 [[nodiscard]] std::size_t leaf( std::size_t i ) const;

 /// the symbol at the place \f$ i \f$ of \f$ T \$ \f$ as stored: the
 /// symbols of \f$ T \f$ plus 1, and 0 for the terminator at \f$ n \f$
 /// \pre \f$ i \le n \f$
 [[nodiscard]] std::size_t symbol( std::size_t i ) const;

 /// the extended locus of \f$ P \f$, or `npos` if \f$ P \f$ does not occur
 ///
 /// In pseudocode (chapter 10, Searching Strings by Substring, of
 /// Ferragina's book, beyond the book), the extended locus being the
 /// highest node whose string has \f$ P \f$ as a prefix:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{locus}( P ) \cr
 /// 2 & \quad u \gets \mathit{root};\ k \gets 0 \cr
 /// 3 & \quad \textbf{while}\ k < p\ \textbf{do} \cr
 /// 4 & \qquad c \gets \text{the child of } u \text{ by } P[ k ];
 ///     \ \textbf{if}\ c = \textbf{nil}\ \textbf{then return}\ \textbf{nil}
 ///     \cr
 /// 5 & \qquad \textbf{foreach}\ \text{further symbol of the edge to } c
 ///     \text{, while } k < p\ \textbf{do} \cr
 /// 6 & \qquad\quad \textbf{if}\  \text{it differs from } P[ k ]
 ///     \ \textbf{then return}\ \textbf{nil} \cr
 /// 7 & \qquad u \gets c \cr
 /// 8 & \quad \textbf{return}\ u
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t locus( std::string_view P ) const;

 /// the locus of `locus()` above, of an integer pattern
 [[nodiscard]] std::size_t locus( const Symbols & P ) const;

 /// the number of occurrences of \f$ P \f$, the leaves below its locus
 /// (the terminator left out)
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of \f$ P \f$, in lexicographic order of
 /// their suffixes; the empty pattern at the \f$ n \f$ places of the
 /// suffixes
 ///
 /// In pseudocode (beyond the book), the subtree of the locus having as
 /// many leaves as occurrences and fewer internal nodes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{locate}( P ) \cr
 /// 2 & \quad u \gets \mathrm{locus}( P );\ \textbf{if}\ u = \textbf{nil}
 ///     \ \textbf{then return}\ (\,) \cr
 /// 3 & \quad \textbf{return}\ \text{the suffixes of the leaves below } u
 ///     \text{ but } n \text{, by a visit in preorder}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the lowest common ancestor of \f$ u \f$ and \f$ v \f$, \f$ O( 1 ) \f$
 ///
 /// In pseudocode (the reduction of Bender and Farach-Colton, beyond the
 /// book), \f$ E \f$ the Euler tour of the tree, \f$ D \f$ the depths of its
 /// nodes and \f$ F[ u ] \f$ the first place of \f$ u \f$ in it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lca}( u , v ) \cr
 /// 2 & \quad a \gets \min\{ F[ u ] , F[ v ] \};\ b \gets \max\{ F[ u ] ,
 ///     F[ v ] \} \cr
 /// 3 & \quad \textbf{return}\ E[ \arg\min_{a \le k \le b} D[ k ] ]\quad
 ///     \triangleright\ \text{a query of range minimum}
 /// \end{array}}
 /// \f]
 /// \pre \f$ u \f$ and \f$ v \f$ are less than `size()`
 [[nodiscard]] std::size_t lca( std::size_t u , std::size_t v ) const;

 /// the length of the longest common prefix of the suffixes \f$ i \f$ and
 /// \f$ j \f$ of \f$ T \f$
 ///
 /// In pseudocode (beyond the book), the lowest common ancestor of two
 /// leaves spelling the longest common prefix of their suffixes:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lce}( i , j ) \cr
 /// 2 & \quad \textbf{if}\ i = n\ \textbf{or}\ j = n\ \textbf{then return}
 ///     \ 0 \cr
 /// 3 & \quad \textbf{if}\ i = j\ \textbf{then return}\ n - i \cr
 /// 4 & \quad \textbf{return}\ \mathit{depth}( \mathrm{lca}( \mathrm{leaf}( i
 ///     ) , \mathrm{leaf}( j ) ) )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$ and \f$ j \le n \f$
 [[nodiscard]] std::size_t lce( std::size_t i , std::size_t j ) const;

 /// \f$ \mathit{SA} \f$ and \f$ \mathit{LCP} \f$ of \f$ T \f$, by a visit in
 /// order
 ///
 /// In pseudocode (beyond the book), the internal node met between two
 /// leaves of a visit in order spelling their lcp:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{toSuffixArray}() \cr
 /// 2 & \quad \textbf{foreach}\ \text{node } u \text{ in preorder, the }
 ///     \text{children by increasing first symbol}\ \textbf{do} \cr
 /// 3 & \qquad \textbf{foreach}\ \text{child } c \text{ of } u\ \textbf{do}
 ///     \cr
 /// 4 & \qquad\quad \textbf{if}\ c \text{ is not the first child}
 ///     \ \textbf{then}\ \text{append } \mathit{depth}( u ) \text{ to }
 ///     \mathit{LCP} \cr
 /// 5 & \qquad\quad \textbf{if}\ c \text{ is a leaf}\ \textbf{then}
 ///     \ \text{append its suffix to } \mathit{SA}\ \textbf{else}
 ///     \ \text{visit } c \cr
 /// 6 & \quad \text{drop the first entry of } \mathit{SA} \text{ and of }
 ///     \mathit{LCP}\quad \triangleright\ \text{the terminator} \cr
 /// 7 & \quad \textbf{return}\ ( \mathit{SA} , \mathit{LCP} )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::pair< std::vector< std::size_t > ,
                          std::vector< std::size_t > > toSuffixArray( void )
                                                                       const;

 /// the nodes in preorder, the children by increasing first symbol
 [[nodiscard]] std::vector< std::size_t > preorder( void ) const;

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const { return( f_N.size() ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the nodes, random access, the root first

 using value_type = Node;                       ///< a node
 using size_type = std::size_t;                 ///< the type of the names
 using difference_type = std::ptrdiff_t;        ///< a difference of names
 using const_reference = const Node &;          ///< a node
 using reference = const_reference;             ///< a node
 /// a constant iterator on the nodes
 using const_iterator = std::vector< Node >::const_iterator;
 using iterator = const_iterator;               ///< on the nodes

 /// the root
 [[nodiscard]] const_iterator begin( void ) const { return( f_N.begin() ); }
 /// one past the last node
 [[nodiscard]] const_iterator end( void ) const { return( f_N.end() ); }
 /// the root
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// one past the last node
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// \f$ T \$ \f$ stored, the constructions, and the leaves, the counts and
 /// the Euler tour at the end
 void construct( const Symbols & T , std::size_t K , Build b );
 void buildNaive( void );
 void buildMcCreight( void );
 void buildFromSuffixArray( const Symbols & T , std::size_t K );
 void finish( void );

 /// a new node
 std::size_t newNode( std::size_t pos , std::size_t depth ,
                      std::size_t parent );

 /// the leaf of the suffix \f$ i \f$ below \f$ u \f$
 std::size_t attachLeaf( std::size_t u , std::size_t i );

 /// a new node at string depth \f$ d \f$ on the edge from \f$ u \f$ to its
 /// child \f$ c \f$
 std::size_t split( std::size_t u , std::size_t c , std::size_t d );

 /// the invariant of the tree
 [[nodiscard]] bool valid( void ) const;

 Symbols f_T;                          ///< \f$ T \$ \f$ stored
 std::vector< Node > f_N;              ///< the nodes, the root at 0
 std::vector< std::size_t > f_leaf;    ///< the leaf of each suffix
 std::vector< std::size_t > f_E;       ///< the Euler tour of the nodes
 std::vector< std::size_t > f_first;   ///< the first visit of each node
 PlusMinusOneRangeMinimum f_tour;      ///< on the depths along the tour

 };  // end( class SuffixTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/SuffixTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SuffixTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SuffixTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
