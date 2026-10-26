/*--------------------------------------------------------------------------*/
/*----------------------- File TwoLevelDictionary.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The two-level dictionary of strings, beyond the book: the lower level is
 * the front coding of the sorted strings with anchors [see
 * FrontCodedDictionary in FrontCoding.h], the strings that are stored
 * whole, either one every b (the bucketing) or as the locality-preserving
 * coding decides, and the upper level a Patricia trie [see
 * PatriciaTrie.h] on the anchors alone. A search by prefix is the blind
 * search of the pattern among the anchors, which reads one anchor, and the
 * scan of the one bucket where the pattern falls, decoded on the fly: with
 * the trie in memory and the coding on disk, \f$ O( p / B + | s | / B )
 * \f$ transfers of blocks for a pattern of p characters, s being the
 * string read by the blind search, plus the bucket (Theorem 9.9 of the
 * notes), in \f$ O( n_a ) \f$ words of memory for \f$ n_a \f$ anchors and
 * \f$ ( 1 + \epsilon ) FC( D ) \f$ characters on disk with the
 * locality-preserving coding. The upper level is independent of the lower
 * one: a binary search on the anchors [see
 * FrontCodedDictionary::lexicographicPosition()] costs \f$ O( ( p / B )
 * \log n_a ) \f$ transfers instead. When even the trie does not fit in
 * memory, the string B-tree of Ferragina and Grossi splits it into
 * Patricia tries of a block each, linked as a B-tree, for \f$ O( ( p +
 * N_{occ} ) / B + \log_B n ) \f$ transfers.
 *
 * The contracts [see Contract.h]: those of FrontCodedDictionary and of
 * PatriciaTrie; each position r is such that the strings before r are
 * smaller than the pattern (or than P#) and those from r on are not, a
 * postcondition. The functions are defined in TwoLevelDictionary.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9 (sections 9.1, 9.5 and 9.6)
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

 /// the trie on the anchors of the coding F
 explicit TwoLevelDictionary( FrontCodedDictionary F );

 /// the number of strings smaller than Q
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than P#
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range [ l , r ) of the strings prefixed by P
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the string i
 /// \pre i < size()
 [[nodiscard]] std::string access( std::size_t i ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] const PatriciaTrie & upper( void ) const;

 [[nodiscard]] const FrontCodedDictionary & lower( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the strings in their order, those of the lower level

 using value_type = std::string;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const std::string &;
 using const_reference = const std::string &;
 using const_iterator = FrontCodedDictionary::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_F.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_F.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the anchors of F, whole
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
