/*--------------------------------------------------------------------------*/
/*----------------------- File TwoLevelDictionary.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The two-level dictionary of strings, beyond the book: the lower level is
 * the front coding of the sorted strings with anchors [see
 * `FrontCodedDictionary` in `FrontCoding.h`], the strings that are stored
 * whole, either one every \f$ b \f$ (the bucketing) or as the
 * locality-preserving coding decides, and the upper level a Patricia trie
 * [see `PatriciaTrie.h`] on the anchors alone. A search by prefix is the
 * blind search of the pattern among the anchors, which reads one anchor,
 * and the scan of the one bucket where the pattern falls, decoded on the
 * fly: with the trie in memory and the coding on disk,
 * \f$ O( p / B + | s | / B ) \f$ transfers of blocks for a pattern of
 * \f$ p \f$ characters, \f$ s \f$ being the string read by the blind
 * search, plus the bucket (chapter 9 of Ferragina), in
 * \f$ O( n_a ) \f$ words of memory for \f$ n_a \f$ anchors and
 * \f$ ( 1 + \epsilon ) \mathrm{FC}( D ) \f$ characters on disk with the
 * locality-preserving coding. The upper level is independent of the lower
 * one: a binary search on the anchors [see
 * `FrontCodedDictionary::lexicographicPosition()`] costs
 * \f$ O( ( p / B ) \log n_a ) \f$ transfers instead. When even the trie
 * does not fit in memory, the string B-tree of Ferragina and Grossi splits
 * it into Patricia tries of a block each, linked as a B-tree, for
 * \f$ O( ( p + N_{occ} ) / B + \log_B n ) \f$ transfers.
 *
 * The contracts [see `Contract.h`]: those of `FrontCodedDictionary` and of
 * `PatriciaTrie`; each position \f$ r \f$ is such that the strings before
 * \f$ r \f$ are smaller than the pattern (or than \f$ P\# \f$) and those from
 * \f$ r \f$ on are not, a postcondition. The functions are defined in
 * `TwoLevelDictionary.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9, Searching Strings by Prefix,
 *      doi:10.1017/9781009128933
 * \see P. Ferragina, R. Grossi, The string B-tree: a new data structure
 *      for string search in external memory and its applications, Journal
 *      of the ACM 46(2), 236-280, 1999, doi:10.1145/301970.301973
 * \see M. A. Bender, M. Farach-Colton, B. C. Kuszmaul, Cache-oblivious
 *      string B-trees, Proceedings of PODS 2006, 233-242, 2006,
 *      doi:10.1145/1142351.1142385
 * \see D. R. Morrison, PATRICIA: practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
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

#ifndef __ADS_TwoLevelDictionary
 #define __ADS_TwoLevelDictionary
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/PatriciaTrie.h"
#include "ADS/string/FrontCoding.h"

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
/*----------------------- CLASS TwoLevelDictionary -------------------------*/
/*--------------------------------------------------------------------------*/

/// a Patricia trie on the anchors of a front-coded dictionary
class TwoLevelDictionary {

/*--------------------------------------------------------------------------*/
 public:

 /// the trie on the anchors of the coding \f$ F \f$
 explicit TwoLevelDictionary( FrontCodedDictionary F );

 /// the number of strings smaller than \f$ Q \f$
///
/// In pseudocode (chapter 9 of Ferragina), the trie in memory
/// and the buckets of the coding on disk:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lexicographicPosition}( Q ) \cr
/// 2 & \quad j \gets T.\mathrm{lexicographicPosition}( Q )\quad
///     \triangleright\ \text{the blind search among the anchors} \cr
/// 3 & \quad \textbf{return}\ \text{the rank of } Q \text{ in the bucket }
///     \text{of the anchor } j \text{, decoded on the fly}
/// \end{array}}
/// \f]
/// \post the strings before the result are smaller than \f$ Q \f$, the
///       others are not
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than \f$ P\# \f$, \f$ \# \f$ larger than
/// every character, i.e., of the strings smaller than \f$ P \f$ or prefixed
/// by it; as `lexicographicPosition()`, with \f$ P\# \f$ in the place of
/// \f$ Q \f$
/// \post the strings before the result precede \f$ P\# \f$, the others do
///       not
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range \f$ [ l , r ) \f$ of the strings prefixed by \f$ P \f$
///
/// In pseudocode (chapter 9 of Ferragina):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{prefixRange}( P ) \cr
/// 2 & \quad \textbf{return}\ ( \mathrm{lexicographicPosition}( P ) ,
///     \mathrm{prefixEnd}( P ) )
/// \end{array}}
/// \f]
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the string \f$ i \f$, decoded from its bucket
 /// \pre \f$ i \f$ is less than `size()`
 [[nodiscard]] std::string access( std::size_t i ) const;

 /// the number of strings
 [[nodiscard]] std::size_t size( void ) const;

 /// the upper level, the trie on the anchors
 [[nodiscard]] const PatriciaTrie & upper( void ) const;

 /// the lower level, the front coding of the strings
 [[nodiscard]] const FrontCodedDictionary & lower( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the strings in their order, those of the lower level

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
 using const_iterator = FrontCodedDictionary::const_iterator;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_F.begin() ); }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_F.end() ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the anchors of \f$ F \f$, whole
 [[nodiscard]] static std::vector< std::string > anchorsOf(
                                              const FrontCodedDictionary & F );

 FrontCodedDictionary f_F;         ///< the lower level
 PatriciaTrie f_T;                 ///< the upper level, on the anchors

 };  // end( class TwoLevelDictionary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/TwoLevelDictionary.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TwoLevelDictionary.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File TwoLevelDictionary.h ----------------------*/
/*--------------------------------------------------------------------------*/
