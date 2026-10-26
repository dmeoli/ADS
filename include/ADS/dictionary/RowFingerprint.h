/*--------------------------------------------------------------------------*/
/*------------------------- File RowFingerprint.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fingerprints of the rows of doubles, beyond the book: a hash of 64
 * bits of a dense row \f$ ( x_0 , \dots , x_{n-1} ) \f$ or of a sparse one,
 * its indices and values \f$ ( j_k , v_k ) \f$ in their order, for finding
 * the rows already seen, e.g., a cut generated twice, by a table of the
 * fingerprints; sameRow() then tells whether two rows with the same
 * fingerprint are really equal.
 *
 * The values are compared as numbers, not as bit patterns: +0 and -0 are
 * equal, and so are all the NaNs (whatever their sign and payload), which
 * the comparison of the doubles does not do. A value is hence mapped to a
 * canonical word before it is hashed: 0 for both zeros, the quiet NaN
 * \f$ 7FF8 \ldots 0_{16} \f$ for every NaN, its own bits otherwise; two
 * rows equal in this sense have the same words, hence the same
 * fingerprint. The words are canonical a block of 64 at a time, by
 * comparisons and blends that the compiler turns into vector operations,
 * and each block of 512 bytes goes through hashBytes() [see FastHash.h],
 * the hash of the previous block as its seed, so that a row costs about
 * what its bytes cost to hash; a sparse row hashes its indices, as bytes,
 * then its values.
 *
 * Two rows with different words collide with probability about
 * \f$ 2^{-64} \f$ for a hash that behaves at random, so that a table of
 * \f$ k \f$ fingerprints holds a false match with probability about
 * \f$ k^2 2^{-65} \f$ [the birthday bound]; but hashBytes() is not
 * cryptographic, so a match is confirmed by sameRow(), in time linear in
 * the row: first a comparison of the bytes of the two rows, which the
 * library does by vectors and which settles the common case of two copies
 * of the same row, then, if some byte differs, the comparison of the
 * values as numbers. The fingerprint of a dense row differs from that of
 * the same row as a sparse one, and the order of the indices of a sparse
 * row counts.
 *
 * The contracts [see Contract.h]: a sparse row has pointers that are not
 * nil when it has entries. The functions are defined in
 * RowFingerprint.tpp.
 *
 * \see M. O. Rabin, Fingerprinting by random polynomials, Technical Report
 *      TR-15-81, Center for Research in Computing Technology, Harvard
 *      University, 1981 (the identity of objects by short fingerprints)
 * \see IEEE Standard for Floating-Point Arithmetic, IEEE Std 754-2019,
 *      2019, doi:10.1109/IEEESTD.2019.8766229 (the zeros and the NaNs)
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

#ifndef __ADS_RowFingerprint
 #define __ADS_RowFingerprint
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ FUNCTIONS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the canonical word of x: 0 for +0 and -0, one quiet NaN for every NaN,
/// the bits of x otherwise
[[nodiscard]] inline std::uint64_t canonicalWord( double x );

/// the fingerprint of the dense row x under the seed
[[nodiscard]] std::uint64_t rowFingerprint( std::span< const double > x ,
                                            std::uint64_t seed = 0 );

/// the fingerprint of the sparse row of the n entries ( ind[ k ] ,
/// val[ k ] ) under the seed
/// \pre ind and val are not nil if n > 0
template< class I >
[[nodiscard]] std::uint64_t rowFingerprint( std::size_t n , const I * ind ,
                                            const double * val ,
                                            std::uint64_t seed = 0 );

/// true if the dense rows a and b have the same length and equal values,
/// +0 equal to -0 and every NaN equal to every NaN
[[nodiscard]] bool sameRow( std::span< const double > a ,
                            std::span< const double > b );

/// true if the sparse rows of na and nb entries have the same indices in
/// the same order and equal values, in the sense above
/// \pre the pointers are not nil where there are entries
template< class I >
[[nodiscard]] bool sameRow( std::size_t na , const I * ia ,
                            const double * va , std::size_t nb ,
                            const I * ib , const double * vb );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/RowFingerprint.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RowFingerprint.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RowFingerprint.h -----------------------*/
/*--------------------------------------------------------------------------*/
