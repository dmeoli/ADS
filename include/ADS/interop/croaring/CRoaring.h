/*--------------------------------------------------------------------------*/
/*----------------------------- File CRoaring.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sets of ADS as the bitmaps of CRoaring
 * (https://github.com/RoaringBitmap/CRoaring), and back: a program that
 * keeps a `RoaringBitmap` hands it to a library that takes a
 * `roaring_bitmap_t` (or a `roaring::Roaring` of its C++ interface), and
 * reads one that such a library returns, e.g., to combine it with the
 * other structures of ADS.
 *
 * Both sides pass through the portable format of Roaring, which
 * `RoaringBitmap` writes and reads [see `ADS/set/RoaringBitmap.h`] and
 * CRoaring deserializes and serializes by its own functions, so that this
 * header depends on no internal of either library: a conversion costs a
 * buffer of `portableSizeInBytes()` bytes and two linear passes over it,
 * and the containers of runs of a bitmap of CRoaring become arrays or
 * bitmaps in ADS, which has no runs (`roaring_bitmap_run_optimize()` turns
 * them back into runs on the other side, if wanted).
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs CRoaring (its headers and its library), found by CMake when
 * `ADS_WITH_CROARING` is AUTO or ON (the target `ADS::CRoaring`).
 *
 * \see D. Lemire, O. Kaser, N. Kurz, L. Deri, C. O'Hara, F. Saint-Jacques,
 *      G. Ssi-Yan-Kai, Roaring bitmaps: implementation of an optimized
 *      software library, Software: Practice and Experience 48(4), 867-895,
 *      2018, doi:10.1002/spe.2560
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

#ifndef __ADS_CRoaring
 #define __ADS_CRoaring
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/RoaringBitmap.h"

#include <roaring/roaring.h>
#include <roaring/roaring.hh>

#include <cstddef>
#include <new>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the set \f$ s \f$ as a new bitmap of CRoaring, which the caller frees
/// by `roaring_bitmap_free()`
/// \throws std::bad_alloc if CRoaring cannot allocate it
[[nodiscard]] inline roaring_bitmap_t * toCRoaring( const RoaringBitmap & s )
{
 const std::vector< std::byte > b = s.writePortable();
 roaring_bitmap_t * r = roaring_bitmap_portable_deserialize_safe(
   reinterpret_cast< const char * >( b.data() ) , b.size() );
 if( r == nullptr )  // the bytes are of ADS, only the memory may lack
  throw( std::bad_alloc() );
 return( r );
 }

/// the bitmap \f$ r \f$ of CRoaring as a `RoaringBitmap`
/// \pre \f$ r \f$ not null
[[nodiscard]] inline RoaringBitmap fromCRoaring( const roaring_bitmap_t * r )
{
 ADS_PRE( r != nullptr );
 std::vector< std::byte > b( roaring_bitmap_portable_size_in_bytes( r ) );
 roaring_bitmap_portable_serialize( r ,
				    reinterpret_cast< char * >( b.data() ) );
 return( RoaringBitmap::readPortable( b ) );
 }

/// the set \f$ s \f$ as a `roaring::Roaring` of the C++ interface of
/// CRoaring
[[nodiscard]] inline roaring::Roaring toRoaring( const RoaringBitmap & s )
{
 const std::vector< std::byte > b = s.writePortable();
 return( roaring::Roaring::readSafe(
   reinterpret_cast< const char * >( b.data() ) , b.size() ) );
 }

/// the `roaring::Roaring` \f$ r \f$ of the C++ interface of CRoaring as a
/// `RoaringBitmap`
[[nodiscard]] inline RoaringBitmap fromCRoaring( const roaring::Roaring & r )
{
 return( fromCRoaring( &r.roaring ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* CRoaring.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File CRoaring.h ---------------------------*/
/*--------------------------------------------------------------------------*/
