/*--------------------------------------------------------------------------*/
/*--------------------------- File SuffixTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The suffix tree of a text T of n symbols, beyond the book: the compact trie
 * [see RadixTree in Trie.h] of the n + 1 suffixes of T$, where $ is a
 * terminator smaller than every symbol, so that no suffix is a prefix of
 * another and each one ends in a leaf of its own. Every internal node but the
 * root has at least two children, hence there are at most n internal nodes;
 * the label of an edge is a substring of T, kept as the place of one
 * occurrence and the string depths of its two ends, \f$ O( 1 ) \f$ space, and
 * the tree takes \f$ O( n ) \f$ space (Fact 10.3), though some 10 to 20 n
 * bytes in practice against the n integers of a suffix array [see
 * SuffixArray.h]. The children of a node are kept by their first symbol in a
 * balanced search tree (std::map), so that branching costs
 * \f$ O( \log \sigma ) \f$ for an alphabet of \f$ \sigma \f$ symbols, and they
 * are visited in the lexicographic order. The node u spells s[ u ], the string
 * on the path from the root, and it is an internal node exactly when s[ u ]
 * occurs at least twice followed by distinct symbols (Property 10.6); the
 * lowest common ancestor of the leaves i and j spells the longest common
 * prefix of the suffixes i and j (Property 10.9).
 *
 * The constructions, each giving the same tree:
 *
 * - Build::naive inserts the suffixes from the longest one, each by a walk
 *   down from the root that compares all its symbols, \f$ \Theta( n^2 ) \f$
 *   time for the text \f$ a^{n-1} \f$;
 * - Build::mcCreight is the algorithm of McCreight: the suffixes are inserted
 *   from the longest one too, but the walk of suffix i starts from the suffix
 *   link of the node of head_{i-1}, the longest prefix of the suffix i - 1
 *   that occurs earlier, which spells head_{i-1} without its first symbol;
 *   when that link is not set yet, it is found from the link of the parent by
 *   a rescan that compares only the first symbol of each edge, whose total is
 *   \f$ O( n ) \f$ since the depth of the tree drops by at most 2 per link.
 *   \f$ O( n \log \sigma ) \f$ time (Theorem 10.10);
 * - Build::suffixArray builds the suffix array by SA-IS and the LCP array by
 *   Kasai et al., then inserts the leaves in the order of SA, each one
 *   climbing from the previous leaf, along the rightmost path, to the
 *   deepest node of string depth at most LCP[ i - 1 ], and splitting the
 *   edge below it when its depth is larger; each edge climbed is left to
 *   the left for good, hence \f$ O( n ) \f$ steps (Fact 10.6).
 *
 * Weiner gave the first linear construction (1973), inserting the suffixes
 * from the shortest one; Ukkonen's algorithm is the online form of
 * McCreight's, which reads T from left to right and keeps the tree of each
 * prefix; Farach reached \f$ O( n ) \f$ on integer alphabets by merging the
 * trees of the odd and of the even suffixes, the idea that the Skew algorithm
 * turned into the 2/3 : 1/3 split of the suffix arrays. Since the linear
 * suffix arrays (SA-IS, divsufsort) are smaller and faster, the construction
 * through SA and LCP is the usual one; McCreight's pays where the average lcp
 * is small.
 *
 * locus() finds the extended locus of a pattern P of p symbols by a walk down
 * from the root, \f$ O( p \log \sigma ) \f$ time, and locate() the occ
 * occurrences below it in \f$ O( \mathit{occ} ) \f$, since that subtree has
 * occ leaves and fewer internal nodes (Fact 10.4). lca() answers the lowest
 * common ancestor of two nodes in \f$ O( 1 ) \f$ time by the Euler tour of the
 * tree and the range minima of its depths [see PlusMinusOneRangeMinimum in
 * RangeMinimum.h], hence lce() the longest common extension of two suffixes
 * (Theorem 10.12). toSuffixArray() reads SA and LCP back by a visit in order,
 * the leaves giving SA and the internal node met between two leaves their lcp
 * (Fact 10.5).
 *
 * The contracts [see Contract.h]: the invariant of the tree (n + 1 leaves,
 * at least two children at each internal node but the root, the first
 * symbol of each edge equal to its key, the string depths increasing
 * downward) after each construction; the places and the nodes of the
 * queries exist. The functions are defined in SuffixTree.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 10
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

/// the suffix tree of T$, its nodes in a vector, the root at 0
class SuffixTree {

/*--------------------------------------------------------------------------*/
 public:

 /// no node, no suffix
 static constexpr std::size_t npos = std::numeric_limits< std::size_t >::max();

 /// the construction
 enum class Build { naive , mcCreight , suffixArray };

 /// a node u; the edge into it is labelled by the symbols of T from
 /// pos + depth of the parent to pos + depth (the terminator at n)
 struct Node {
  std::size_t pos = 0;          ///< the start of an occurrence of s[ u ]
  std::size_t depth = 0;        ///< the string depth | s[ u ] |
  std::size_t parent = npos;    ///< npos at the root
  std::size_t suffix = npos;    ///< the suffix of a leaf, npos inside
  std::size_t leaves = 0;       ///< the leaves below, occ[ u ]
  std::map< std::size_t , std::size_t > child;   ///< by first symbol
  };

 SuffixTree( void ) = default;

 /// the suffix tree of the bytes of T
 explicit SuffixTree( std::string_view T , Build b = Build::mcCreight );

 /// the suffix tree of an integer text
 /// \pre every symbol of T is less than K
 SuffixTree( const Symbols & T , std::size_t K ,
             Build b = Build::mcCreight );

 /// the length n of T, without the terminator
 [[nodiscard]] std::size_t textSize( void ) const;

 [[nodiscard]] std::size_t root( void ) const { return( 0 ); }

 /// \pre u < size()
 [[nodiscard]] const Node & node( std::size_t u ) const;

 /// the leaf of the suffix i, the terminator being the suffix n
 /// \pre i <= textSize()
 [[nodiscard]] std::size_t leaf( std::size_t i ) const;

 /// the symbol at the place i of T$ as stored: the symbols of T plus 1,
 /// and 0 for the terminator at n
 /// \pre i <= textSize()
 [[nodiscard]] std::size_t symbol( std::size_t i ) const;

 /// the extended locus of P, or npos if P does not occur
 [[nodiscard]] std::size_t locus( std::string_view P ) const;
 [[nodiscard]] std::size_t locus( const Symbols & P ) const;

 /// the number of occurrences of P
 [[nodiscard]] std::size_t count( std::string_view P ) const;

 /// the places of the occurrences of P, in lexicographic order of their
 /// suffixes; the empty pattern at the n places of the suffixes
 [[nodiscard]] std::vector< std::size_t > locate( std::string_view P ) const;

 /// the lowest common ancestor of u and v, \f$ O( 1 ) \f$
 /// \pre u < size() and v < size()
 [[nodiscard]] std::size_t lca( std::size_t u , std::size_t v ) const;

 /// the length of the longest common prefix of the suffixes i and j of T
 /// \pre i <= textSize() and j <= textSize()
 [[nodiscard]] std::size_t lce( std::size_t i , std::size_t j ) const;

 /// SA and LCP of T, by a visit in order (Fact 10.5)
 [[nodiscard]] std::pair< std::vector< std::size_t > ,
                          std::vector< std::size_t > > toSuffixArray( void )
                                                                       const;

 /// the nodes in preorder, the children by increasing first symbol
 [[nodiscard]] std::vector< std::size_t > preorder( void ) const;

 /// the number of nodes
 [[nodiscard]] std::size_t size( void ) const { return( f_N.size() ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the nodes, random access, the root first

 using value_type = Node;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using const_reference = const Node &;
 using reference = const_reference;
 using const_iterator = std::vector< Node >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_N.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_N.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// T$ stored, the constructions, and the leaves, the counts and the
 /// Euler tour at the end
 void construct( const Symbols & T , std::size_t K , Build b );
 void buildNaive( void );
 void buildMcCreight( void );
 void buildFromSuffixArray( const Symbols & T , std::size_t K );
 void finish( void );

 /// a new node
 std::size_t newNode( std::size_t pos , std::size_t depth ,
                      std::size_t parent );

 /// the leaf of the suffix i below u
 std::size_t attachLeaf( std::size_t u , std::size_t i );

 /// a new node at string depth d on the edge from u to its child c
 std::size_t split( std::size_t u , std::size_t c , std::size_t d );

 /// the invariant of the tree
 [[nodiscard]] bool valid( void ) const;

 Symbols f_T;                          ///< T$ stored
 std::vector< Node > f_N;
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
