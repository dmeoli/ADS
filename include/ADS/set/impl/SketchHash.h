/*--------------------------------------------------------------------------*/
/*---------------------------- File SketchHash.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash of an item for the sketches of this folder [see
 * CountMinSketch.h, MinHashSketch.h]: the 64 bits of `hash64()` [see
 * FastHash.h] finished by `splitmix64()` [see RandomHashing.h], whatever
 * the hash declares. A sketch reads its hashes as independent uniform
 * words, while the folded product of `FastHash` on consecutive integers
 * gives the multiples of a constant modulo \f$ 2^{64} \f$ in their top
 * bits, a sequence of low discrepancy far from such words; the finalizer
 * of SplitMix64, made to turn such a sequence into random words, costs a
 * few cycles.
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

#ifndef __ADS_SketchHash
 #define __ADS_SketchHash
/* self-identification: #endif constant */

#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/RandomHashing.h"

#include <cstdint>

namespace ADS::detail
{

/// the hash of \f$ x \f$ by \f$ h \f$ for a sketch, `splitmix64()` of its
/// `hash64()`
template< class H , class T >
[[nodiscard]] inline std::uint64_t sketchHash( const H & h , const T & x )
{
 return( splitmix64( hash64( h , x ) ) );
 }

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/

#endif /* SketchHash.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SketchHash.h ---------------------------*/
/*--------------------------------------------------------------------------*/
