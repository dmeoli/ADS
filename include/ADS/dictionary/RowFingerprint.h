/*--------------------------------------------------------------------------*/
/*------------------------- File RowFingerprint.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The fingerprints of the rows of doubles, beyond the book: a hash of 64
 * bits of a dense row \f$ ( x_0 , \dots , x_{n-1} ) \f$ or of a sparse one,
 * its indices and values \f$ ( j_k , v_k ) \f$ in their order, for finding
 * the rows already seen, e.g., a cut generated twice, by a table of the
 * fingerprints; `sameRow()` then tells whether two rows with the same
 * fingerprint are really equal.
 *
 * The values are compared as numbers, not as bit patterns: \f$ +0 \f$ and
 * \f$ -0 \f$ are equal, and so are all the NaNs (whatever their sign and
 * payload), which the comparison of the doubles does not do. A value is
 * hence mapped to a canonical word before it is hashed: 0 for both zeros,
 * the quiet NaN \f$ 7FF8 \ldots 0_{16} \f$ for every NaN, its own bits
 * otherwise; two rows equal in this sense have the same words, hence the
 * same fingerprint. The words are canonical a block of 64 at a time, by
 * comparisons and blends that the compiler turns into vector operations,
 * and each block of 512 bytes goes through `hashBytes()` [see
 * `FastHash.h`], the hash of the previous block as its seed, so that a row
 * costs about what its bytes cost to hash; a sparse row hashes its indices,
 * as bytes, then its values.
 *
 * Two rows with different words collide with probability about
 * \f$ 2^{-64} \f$ for a hash that behaves at random, so that a table of
 * \f$ k \f$ fingerprints holds a false match with probability about
 * \f$ k^2 2^{-65} \f$ [the birthday bound]; but `hashBytes()` is not
 * cryptographic, so a match is confirmed by `sameRow()`, in time linear in
 * the row: first a comparison of the bytes of the two rows, which the
 * library does by vectors and which settles the common case of two copies
 * of the same row, then, if some byte differs, the comparison of the
 * values as numbers. The fingerprint of a dense row differs from that of
 * the same row as a sparse one, and the order of the indices of a sparse
 * row counts.
 *
 * The contracts [see `Contract.h`]: a sparse row has pointers that are not
 * nil when it has entries. The functions are defined in
 * `RowFingerprint.tpp`.
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

/// the canonical word of \f$ x \f$: 0 for \f$ +0 \f$ and \f$ -0 \f$, one
/// quiet NaN for every NaN, the bits of \f$ x \f$ otherwise
[[nodiscard]] inline std::uint64_t canonicalWord( double x );

/// the fingerprint of the dense row \f$ x \f$ under the seed
///
/// In pseudocode (beyond the book), the canonical words hashed a block of 64
/// at a time, each block seeded by the hash of the one before:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{rowFingerprint}( x , h ) \cr
/// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ \mathrm{hashBytes}(
///     \text{nothing} , h ) \cr
/// 3 & \quad \textbf{foreach}\ \text{block } x_i , \ldots , x_{i + 63}
///     \text{ of } x\ \textbf{do} \cr
/// 4 & \qquad b \gets ( \mathrm{canonicalWord}( x_i ) , \ldots ,
///     \mathrm{canonicalWord}( x_{i + 63} ) ) \cr
/// 5 & \qquad h \gets \mathrm{hashBytes}( b , h ) \cr
/// 6 & \quad \textbf{return}\ h
/// \end{array}}
/// \f]
[[nodiscard]] std::uint64_t rowFingerprint( std::span< const double > x ,
                                            std::uint64_t seed = 0 );

/// the fingerprint of the sparse row of the \f$ n \f$ entries
/// \f$ ( \mathit{ind}[ k ] , \mathit{val}[ k ] ) \f$ under the seed: the
/// values hashed as for a dense row, from the hash of the bytes of the
/// indices
/// \pre `ind` and `val` are not nil if \f$ n > 0 \f$
template< class I >
[[nodiscard]] std::uint64_t rowFingerprint( std::size_t n , const I * ind ,
                                            const double * val ,
                                            std::uint64_t seed = 0 );

/// true if the dense rows \f$ a \f$ and \f$ b \f$ have the same length and
/// equal values, \f$ +0 \f$ equal to \f$ -0 \f$ and every NaN equal to
/// every NaN
///
/// In pseudocode (beyond the book), the bytes first, which settle the
/// common case of two copies of a row:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{sameRow}( a , b ) \cr
/// 2 & \quad \textbf{if}\ |a| \ne |b|\ \textbf{then return false} \cr
/// 3 & \quad \textbf{if}\ \text{the bytes of } a \text{ and } b \text{ are }
///     \text{equal}\ \textbf{then return true} \cr
/// 4 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |a| - 1\ \textbf{do} \cr
/// 5 & \qquad \textbf{if}\ a_i \ne b_i\ \textbf{and not}\ ( a_i \text{ and }
///     b_i \text{ are NaN} )\ \textbf{then return false} \cr
/// 6 & \quad \textbf{return true}
/// \end{array}}
/// \f]
[[nodiscard]] bool sameRow( std::span< const double > a ,
                            std::span< const double > b );

/// true if the sparse rows of \f$ n_a \f$ and \f$ n_b \f$ entries have the
/// same indices in the same order and equal values, in the sense above
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
