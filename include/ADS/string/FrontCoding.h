/*--------------------------------------------------------------------------*/
/*-------------------------- File FrontCoding.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The front coding of a sorted dictionary of strings, beyond the book:
 * adjacent strings share a prefix, and frontCode() writes the string
 * \f$ s_i \f$ as the pair \f$ ( \ell_i , \hat s_i ) \f$, \f$ \ell_i \f$
 * being the length of the longest common prefix of \f$ s_i \f$ and \f$
 * s_{i-1} \f$ (0 for the first one) and \f$ \hat s_i \f$ the rest of \f$
 * s_i \f$; frontDecode() copies \f$ \ell_i \f$ characters of the string
 * before and appends \f$ \hat s_i \f$. The length goes in the byte-aligned
 * code of the notes [see encodeLength()]: the binary of \f$ \ell \f$ padded
 * to a whole number of bytes, the first two bits of the padding giving the
 * number of bytes, so that \f$ \ell < 2^{30} \f$ takes 1 to 4 bytes instead
 * of the \f$ \ell \f$ characters of the prefix. On the dictionaries of URLs
 * the shared prefixes are up to 70 % of the characters.
 *
 * A string is decoded from the last one stored whole, and with no more of
 * them the decoding of \f$ s_i \f$ may scan the whole dictionary before it;
 * FrontCodedDictionary stores some strings whole (the anchors) and decodes
 * a string from the anchor before it, copying only the \f$ \ell \f$
 * characters that each pair takes from the previous one:
 *
 * - bucketed() is the front coding with bucketing \f$ FC_B \f$ of the
 *   notes: an anchor every b strings, i.e., buckets of b strings coded on
 *   their own; a larger b compresses better and scans more (up to \f$
 *   \Theta( b^2 ) \f$ characters for a bucket like a , aa , aaa , ...);
 * - localityPreserving() is the locality-preserving front coding of
 *   Bender, Farach-Colton and Kuszmaul: \f$ s_i \f$ is front coded only if
 *   the anchor before it begins within the last \f$ c | s_i | \f$
 *   characters of the coding, else it is stored whole; a string is then
 *   decoded by reading at most \f$ ( c + 1 ) | s_i | \f$ characters, the
 *   optimal \f$ O( | s_i | / B ) \f$ transfers of blocks, and the coding
 *   takes at most \f$ ( 1 + 2 / ( c - 2 ) ) FC( D ) \f$ characters for
 *   \f$ c > 2 \f$, FC( D ) being those of the front coding (Theorem 9.5 of
 *   the notes): the time controls the space.
 *
 * Both search by prefix as the two-level scheme of section 9.1 of the
 * notes: a binary search on the anchors, which are whole, and the scan of
 * the one bucket where the pattern falls, decoded on the fly; the binary
 * search may be replaced by a Patricia trie on the anchors [see
 * TwoLevelDictionary.h]. The string B-tree of Ferragina and Grossi and its
 * cache-oblivious version by Bender, Farach-Colton and Kuszmaul, which
 * also works on the locality-preserving coding, bring the search in
 * external memory to \f$ O( ( p + N_{occ} ) / B + \log_B n ) \f$ transfers.
 *
 * The contracts [see Contract.h]: \f$ \ell < 2^{30} \f$ for
 * encodeLength(), a pair that takes at most the characters of the string
 * before for frontDecode(), the dictionary sorted for FrontCodedDictionary
 * (an audit), b >= 1 and c > 0; the decoding of the coding gives the strings
 * back, an audit, and the locality-preserving coding is within its bounds
 * of space and time, a postcondition for c > 2. The functions are defined
 * in FrontCoding.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 9 (sections 9.1 and 9.3)
 * \see I. H. Witten, A. Moffat, T. C. Bell, Managing Gigabytes, 2nd ed.,
 *      Morgan Kaufmann, 1999 (the front coding of a lexicon)
 * \see M. A. Bender, M. Farach-Colton, B. C. Kuszmaul, Cache-oblivious
 *      string B-trees, Proceedings of PODS 2006, 233-242, 2006,
 *      doi:10.1145/1142351.1142385
 * \see P. Ferragina, R. Grossi, The string B-tree: a new data structure
 *      for string search in external memory and its applications, Journal
 *      of the ACM 46(2), 236-280, 1999, doi:10.1145/301970.301973
 * \see D. R. Morrison, PATRICIA - practical algorithm to retrieve
 *      information coded in alphanumeric, Journal of the ACM 15(4),
 *      514-534, 1968, doi:10.1145/321479.321481
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_FrontCoding
 #define __ADS_FrontCoding
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/string/PrefixSearch.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a pair ( l , rest ) of the front coding
using FrontPair = std::pair< std::size_t , std::string >;

/// the front coding of D
/// \post frontDecode( result ) == D (an audit)
[[nodiscard]] inline std::vector< FrontPair > frontCode(
                                       const std::vector< std::string > & D );

/// the strings of the front coding F
/// \pre the first pair has l = 0, and each l is at most the length of the
///      string before
[[nodiscard]] inline std::vector< std::string > frontDecode(
                                       const std::vector< FrontPair > & F );

/// appends to out the code of l in 1 to 4 bytes, the first two bits giving
/// the number of bytes minus 1
/// \pre l < 2^30
/// \post decodeLength() reads l back (an audit)
inline void encodeLength( std::uint32_t l , std::string & out );

/// the length coded at in[ pos ], pos moved past its code
/// \pre a code begins at pos and is within in
[[nodiscard]] inline std::uint32_t decodeLength( std::string_view in ,
                                                 std::size_t & pos );

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS FrontCodedDictionary ------------------------*/
/*--------------------------------------------------------------------------*/

/// a sorted dictionary of strings in the front coding, with anchors
class FrontCodedDictionary {

/*--------------------------------------------------------------------------*/
 public:

 /// the front coding with an anchor every b strings
 /// \pre D is sorted (an audit) and b >= 1
 [[nodiscard]] static FrontCodedDictionary bucketed(
                           const std::vector< std::string > & D ,
                           std::size_t b );

 /// the locality-preserving front coding of parameter c
 /// \pre D is sorted (an audit) and c > 0
 /// \post for c > 2, characters() <= ( 1 + 2 / ( c - 2 ) ) FC( D ) and
 ///       decodeCost( i ) <= ( c + 1 ) | s_i |
 [[nodiscard]] static FrontCodedDictionary localityPreserving(
                           const std::vector< std::string > & D , double c );

 /// the string i, decoded from the anchor before it
 /// \pre i < size()
 [[nodiscard]] std::string access( std::size_t i ) const;

 /// the characters that access( i ) reads
 /// \pre i < size()
 [[nodiscard]] std::size_t decodeCost( std::size_t i ) const;

 /// the number of strings smaller than Q
 [[nodiscard]] std::size_t lexicographicPosition( std::string_view Q ) const;

 /// the number of strings smaller than P#
 [[nodiscard]] std::size_t prefixEnd( std::string_view P ) const;

 /// the range [ l , r ) of the strings prefixed by P
 [[nodiscard]] std::pair< std::size_t , std::size_t > prefixRange(
                                                   std::string_view P ) const;

 /// the number of strings smaller than P (or P# if upper), given that k
 /// anchors are: the scan of the bucket k - 1
 /// \pre k <= anchors(), and k is right
 [[nodiscard]] std::size_t rankInBucket( std::size_t k , std::string_view P ,
                                         bool upper ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the number of anchors, the strings stored whole
 [[nodiscard]] std::size_t anchors( void ) const;

 /// the place in the dictionary of the anchor k
 /// \pre k < anchors()
 [[nodiscard]] std::size_t anchor( std::size_t k ) const;

 /// the anchor k
 /// \pre k < anchors()
 [[nodiscard]] std::string_view anchorString( std::size_t k ) const;

 /// the characters of the coding, those of the rests and of the anchors
 [[nodiscard]] std::size_t characters( void ) const;

 /// the bytes of the coding, the characters and the codes of the lengths
 [[nodiscard]] std::size_t bytes( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the strings in their order, decoded one after the
 // other, forward; *it is the string decoded in the iterator

 class const_iterator {
  public:
  using reference = const std::string &;
  using value_type = std::string;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::forward_iterator_tag;
  using iterator_category = std::input_iterator_tag;

  const_iterator( void ) = default;
  const_iterator( const FrontCodedDictionary * d , std::size_t i )
   : f_d( d ) , f_i( i ) { load(); }

  reference operator*( void ) const { return( f_s ); }
  const_iterator & operator++( void ) {
   ++f_i;
   load();
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }
  bool operator==( const const_iterator & o ) const {
   return( f_i == o.f_i );
   }

  private:
  /// the string f_i from the one before
  void load( void ) {
   if( f_d && ( f_i < f_d->size() ) ) {
    f_s.resize( f_d->f_l[ f_i ] );
    f_s.append( f_d->rest( f_i ) );
    }
   }

  const FrontCodedDictionary * f_d = nullptr;
  std::size_t f_i = 0;
  std::string f_s;
  };

 using value_type = std::string;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const std::string &;
 using const_reference = const std::string &;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 FrontCodedDictionary( void ) = default;

 /// the coding of D, s_i whole if anchor( i , l , chars , | s_i | ) says
 /// so, l being its prefix shared with s_{i-1} and chars the characters of
 /// the coding since the last anchor
 template< class A >
 static FrontCodedDictionary build( const std::vector< std::string > & D ,
                                    A anchor );

 /// the rest of the string i
 [[nodiscard]] std::string_view rest( std::size_t i ) const;

 /// the anchor at or before i
 [[nodiscard]] std::size_t anchorBefore( std::size_t i ) const;

 /// the number of anchors smaller than P (or P# if upper)
 [[nodiscard]] std::size_t anchorRank( std::string_view P ,
                                       bool upper ) const;

 std::vector< std::size_t > f_l;   ///< the shared prefixes, 0 at anchors
 std::vector< std::size_t > f_off; ///< where each rest begins in f_chars
 std::string f_chars;              ///< the rests, one after the other
 std::vector< std::size_t > f_a;   ///< the places of the anchors
 std::size_t f_bytes = 0;

 };  // end( class FrontCodedDictionary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/FrontCoding.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* FrontCoding.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File FrontCoding.h -------------------------*/
/*--------------------------------------------------------------------------*/
