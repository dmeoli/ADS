/*--------------------------------------------------------------------------*/
/*------------------------------ File Sdsl.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The vectors of bits and of fields of ADS to and from those of sdsl-lite
 * (its version 3, https://github.com/xxsds/sdsl-lite, header-only and
 * under the BSD license), so that a structure of either library is built
 * on the data of the other: an FM-index, a wavelet tree or a compressed
 * suffix array of sdsl over a `BitVector` or a `PackedArray` of ADS, a
 * `RankSelect`, an `EliasFano` or a `WaveletMatrix` of ADS over a
 * `bit_vector` or an `int_vector` of sdsl.
 *
 * The two libraries keep the bits in the same words: the bit \f$ i \f$ of
 * a `BitVector` and of a `bit_vector` is the bit \f$ i \bmod 64 \f$ of the
 * word \f$ \lfloor i / 64 \rfloor \f$, and the field \f$ i \f$ of a
 * `PackedArray< B >` and of an `int_vector< B >` (or of an `int_vector<>`
 * of width \f$ B \f$) takes the bits \f$ i B \f$ to \f$ ( i + 1 ) B - 1 \f$,
 * which never cross a word since \f$ B \f$ divides 64. Hence a conversion
 * copies the words at once, \f$ \lceil n B / 64 \rceil \f$ of them, and
 * clears the bits past the last field; only an `int_vector` of another
 * width is read field by field. Neither library can take the words of
 * the other without copy, since each one owns its words (ADS aligns them
 * to a line of the cache, and sdsl allocates them by its own memory
 * manager).
 *
 * The same vectors of bits are also written to and read from the
 * archives of cereal by both libraries, in the same form [see
 * `ADS/interop/cereal/Cereal.h`], so that a `BitVector` saved by ADS is
 * loaded as a `bit_vector` by sdsl and the converse.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs the headers of sdsl-lite 3, found by CMake when
 * `ADS_WITH_SDSL` is AUTO or ON.
 *
 * \see S. Gog, T. Beller, A. Moffat, M. Petri, From theory to practice:
 *      plug and play with succinct data structures, Proc. SEA 2014, LNCS
 *      8504, 326-337, 2014, doi:10.1007/978-3-319-07959-2_28
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

#ifndef __ADS_Sdsl
 #define __ADS_Sdsl
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/PackedArray.h"

#include <sdsl/int_vector.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the bits of \f$ b \f$ as a `bit_vector` of sdsl-lite, by a copy of the
/// words
[[nodiscard]] inline sdsl::bit_vector toSdsl( const BitVector & b )
{
 sdsl::bit_vector v( b.size() , 0 );
 if( b.size() > 0 )
  std::memcpy( v.data() , b.words().data() ,
	       ( b.size() + 63 ) / 64 * sizeof( Word ) );
 return( v );
 }

/// the bits of the `bit_vector` \f$ v \f$ of sdsl-lite as a `BitVector`,
/// by a copy of the words
[[nodiscard]] inline BitVector toBitVector( const sdsl::bit_vector & v )
{
 if( v.size() == 0 )
  return( BitVector() );
 return( BitVector(
   std::span< const Word >( v.data() , ( v.size() + 63 ) / 64 ) , v.size() ) );
 }

/// the fields of \f$ a \f$ as an `int_vector< B >` of sdsl-lite, by a copy
/// of the words
template< unsigned B , bool S >
[[nodiscard]] sdsl::int_vector< B > toSdsl( const PackedArray< B , S > & a )
{
 sdsl::int_vector< B > v( a.size() , 0 , B );
 if( a.size() > 0 )
  std::memcpy( v.data() , a.words().data() ,
	       ( a.size() * B + 63 ) / 64 * sizeof( Word ) );
 return( v );
 }

/// the fields of the `int_vector` \f$ v \f$ of sdsl-lite as a
/// `PackedArray< B , S >`: by a copy of the words if its width is
/// \f$ B \f$, and else field by field, each clamped to \f$ 2^B - 1 \f$ if
/// \f$ S \f$
/// \throws std::invalid_argument if \f$ v \f$ has a field of more than
///         \f$ B \f$ bits and not \f$ S \f$
template< unsigned B , bool S = false , std::uint8_t W >
[[nodiscard]] PackedArray< B , S >
toPackedArray( const sdsl::int_vector< W > & v )
{
 using P = PackedArray< B , S >;
 const std::size_t n = v.size();
 if( v.width() == B ) {
  if( n == 0 )
   return( P() );
  return( P( std::span< const Word >( v.data() , ( n * B + 63 ) / 64 ) , n ) );
  }
 if constexpr( ! S )
  for( std::size_t i = 0 ; i < n ; ++i )
   if( std::uint64_t( v[ i ] ) > P::MAX )
    throw( std::invalid_argument( "ADS::toPackedArray: a field of the "
				  "int_vector of more bits than B" ) );
 Sequential q;
 return( P(
   q , n ,
   [ & ]( std::size_t i ) { return( typename P::Word( v[ i ] ) ); } ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Sdsl.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Sdsl.h -----------------------------*/
/*--------------------------------------------------------------------------*/
