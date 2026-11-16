/*--------------------------------------------------------------------------*/
/*----------------------------- File Cereal.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The persistent structures of ADS [see `Persistence.h`] in the archives
 * of cereal (https://uscilab.github.io/cereal/), beside the format of
 * ADS: a program that includes this header writes a `RankSelect`, a
 * `WaveletMatrix` or a `BinaryFuseFilter` with `ar( x )` into its own
 * archives, binary,
 * portable binary, JSON or XML, next to its other data, and reads it back
 * in the same way. The non-member `save()` and `load()` below, found by
 * cereal by the lookup that depends on the arguments, run the same
 * `persist()` of the structure as `store()` and `restore()` do, through an
 * archive that hands each member to cereal: a number, a string, a
 * `std::array` and a `std::pair` as cereal writes them, an array of
 * numbers as its size and, in a binary archive, its bytes at once, a
 * nested structure as a structure of its own; and the invariants that the
 * structure requires are checked on a reading, which throws `PersistError`
 * if one fails. A mapped file has no equivalent here, since an archive of
 * cereal is read by parsing it.
 *
 * A `BitVector` is written as sdsl-lite writes its `bit_vector` (the
 * `int_vector< 1 >` of its version 3, which serializes by cereal): the
 * width 1 as an 8-bit size, the factor of growth as a `float`, the number
 * of bits as a 64-bit size, and the words that hold them, \f$ \lceil n /
 * 64 \rceil \f$, the bit \f$ i \f$ in the bit \f$ i \bmod 64 \f$ of the
 * word \f$ \lfloor i / 64 \rfloor \f$ in both libraries; in an archive that
 * is not binary, the bits one by one as integers of 64 bits (the
 * `value_type` of a `bit_vector`), under the names of sdsl. The two
 * libraries thus read the vectors of bits that the other writes, and the
 * structures built on them hold them in the same form; the directories of
 * rank and select, whose layouts differ, are not interchangeable, and
 * neither are `PackedArray` and `int_vector< w >`, since the former keeps
 * its fields within a word and the latter lets them cross.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs the headers of cereal (1.3 or later), found by CMake when
 * `ADS_WITH_CEREAL` is AUTO or ON.
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

#ifndef __ADS_Cereal
 #define __ADS_Cereal
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/Persistence.h"
#include "ADS/succinct/BitVector.h"

#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS CerealArchive ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the archive of `persist()` over the archive \f$ \mathit{Ar} \f$ of
/// cereal, which reads if \f$ L \f$ and writes if not
template< class Ar , bool L >
class CerealArchive
{
 public:
 /// whether it reads
 static constexpr bool loading = L;

 /// the archive over \f$ a \f$
 explicit CerealArchive( Ar & a ) : f_a( a ) { }

 /// the members \f$ x \f$ written, or read, in order
 template< class... X >
 void operator()( X &... x ) {
  ( item( x ) , ... );
  }

 /// on a reading, the archive rejected as corrupted in `what`, unless
 /// \f$ c \f$
 void require( bool c , const char * what ) const {
  if constexpr( L )
   if( ! c )
    throw( PersistError( std::string( "ADS: an archive of cereal: " ) +
			 "corrupted: " + what ) );
  }

 private:
 /// whether the archive takes the bytes of an array at once
 static constexpr bool kBinary =
   L ? cereal::traits::is_input_serializable< cereal::BinaryData< char * > ,
					      Ar >::value
     : cereal::traits::is_output_serializable< cereal::BinaryData< char * > ,
					       Ar >::value;

 /// a member
 template< class T >
 void item( T & x ) {
  using U = std::remove_cv_t< T >;
  if constexpr( Persistent< U > )
   f_a( x );  // a structure of its own, by save() and load() below
  else if constexpr( HasPersist< U > )
   x.persist( *this );
  else if constexpr( IsVector< U >::value )
   vector( x );
  else if constexpr( IsStdArray< U >::value || std::is_array_v< U > ) {
   for( auto & e : x )
    item( e );
   }
  else if constexpr( IsPair< U >::value ) {
   item( x.first );
   item( x.second );
   }
  else
   f_a( x );  // a number or a string
  }

 /// an array, in a node of its own [see `CerealArray`]
 template< class E , class A >
 void vector( std::vector< E , A > & v );

 Ar & f_a;

 public:
 /// the array \f$ v \f$ in the node of \f$ a \f$: its size, then its
 /// items, its bytes at once if they are numbers (or blocks of them) and
 /// the archive is binary
 template< class E , class A >
 static void array( Ar & a , std::vector< E , A > & v ) {
  cereal::size_type n = v.size();
  a( cereal::make_size_tag( n ) );
  if constexpr( L ) {
   if constexpr( std::is_same_v< E , bool > )
    v.assign( std::size_t( n ) , false );
   else
    v.resize( std::size_t( n ) );
   }
  if constexpr( PersistBlock< E > && kBinary ) {
   using N = typename PersistUnitOf< E >::type;  // the bytes of a number
   a( cereal::binary_data( reinterpret_cast< N * >( v.data() ) ,
			   std::size_t( n ) * sizeof( E ) ) );  // together
   }
  else if constexpr( PersistBlock< E > && ! PersistNumber< E > ) {
   using N = typename PersistUnitOf< E >::type;  // a block, by its numbers
   for( auto & e : v ) {
    N u[ sizeof( E ) / sizeof( N ) ];
    if constexpr( ! L )
     std::memcpy( u , &e , sizeof( E ) );
    for( auto & k : u )
     a( k );
    if constexpr( L )
     std::memcpy( &e , u , sizeof( E ) );
    }
   }
  else if constexpr( std::is_same_v< E , bool > )
   for( std::size_t i = 0 ; i < v.size() ; ++i ) {
    bool b = v[ i ];
    a( b );
    v[ i ] = b;
    }
  else {
   CerealArchive in( a );
   for( auto & e : v )
    in.item( e );
   }
  }
 };

/// an array of a structure, which cereal writes in a node of its own (as
/// a JSON array, say)
template< class V >
struct CerealArray
{
 V & v;  ///< the array

 /// the array written to, or read from, the archive \f$ a \f$ of cereal
 template< class Ar >
 void serialize( Ar & a ) {
  CerealArchive< Ar , Ar::is_loading::value >::array( a , v );
  }
 };

template< class Ar , bool L >
template< class E , class A >
inline void CerealArchive< Ar , L >::vector( std::vector< E , A > & v )
{
 CerealArray< std::vector< E , A > > w{ v };
 f_a( w );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure \f$ x \f$ written to the archive \f$ a \f$ of cereal
template< class Ar , Persistent T >
void save( Ar & a , const T & x )
{
 detail::CerealArchive< Ar , false > w( a );
 const_cast< T & >( x ).persist( w );  // which only reads it
 }

/// the structure \f$ x \f$ read from the archive \f$ a \f$ of cereal
/// \throws PersistError if the members break an invariant of \f$ T \f$
template< class Ar , Persistent T >
void load( Ar & a , T & x )
{
 detail::CerealArchive< Ar , true > r( a );
 x.persist( r );
 }

/// the bits \f$ b \f$ written to the archive \f$ a \f$ of cereal, as
/// sdsl-lite writes a `bit_vector`
template< class Ar >
void save( Ar & a , const BitVector & b )
{
 const std::uint64_t n = b.size();
 const std::size_t nw = ( b.size() + 63 ) / 64;
 if constexpr( cereal::traits::is_output_serializable<
		 cereal::BinaryData< char * > , Ar >::value ) {
  a( cereal::make_size_tag( std::uint8_t( 1 ) ) );
  a( 1.5f );
  a( cereal::make_size_tag( n ) );
  a( cereal::binary_data( b.words().data() , nw * sizeof( Word ) ) );
  }
 else {
  a( cereal::make_nvp( "m_width" , std::uint8_t( 1 ) ) );
  a( cereal::make_nvp( "growth_factor" , 1.5f ) );
  a( cereal::make_nvp( "m_size" , n ) );
  for( const bool v : b )
   a( std::uint64_t( v ) );  // the value_type of a bit_vector of sdsl
  }
 }

/// the bits \f$ b \f$ read from the archive \f$ a \f$ of cereal, as
/// sdsl-lite writes a `bit_vector`
/// \throws PersistError if they are not a vector of bits
template< class Ar >
void load( Ar & a , BitVector & b )
{
 std::uint8_t w = 0;
 float g = 0;
 std::uint64_t n = 0;
 const auto fail = []( const char * what ) {
  throw( PersistError( std::string( "ADS: an archive of cereal: " ) +
		       "corrupted: " + what ) );
  };
 if constexpr( cereal::traits::is_input_serializable<
		 cereal::BinaryData< char * > , Ar >::value ) {
  a( cereal::make_size_tag( w ) );
  a( g );
  a( cereal::make_size_tag( n ) );
  if( w != 1 )
   fail( "a vector of integers of more than 1 bit, not of bits" );
  std::vector< Word > W( std::size_t( ( n + 63 ) / 64 ) );
  a( cereal::binary_data( W.data() , W.size() * sizeof( Word ) ) );
  if( ( n % 64 ) && ( W.back() >> ( n % 64 ) ) )
   fail( "the bits past the size of a vector of bits" );
  b = BitVector( W , std::size_t( n ) );
  }
 else {
  a( cereal::make_nvp( "m_width" , w ) );
  a( cereal::make_nvp( "growth_factor" , g ) );
  a( cereal::make_nvp( "m_size" , n ) );
  if( w != 1 )
   fail( "a vector of integers of more than 1 bit, not of bits" );
  b.clear();
  b.reserve( std::size_t( n ) );
  for( std::uint64_t i = 0 ; i < n ; ++i ) {
   std::uint64_t v;
   a( v );
   if( v > 1 )
    fail( "a bit neither 0 nor 1 in a vector of bits" );
   b.push_back( v == 1 );
   }
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Cereal.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Cereal.h ----------------------------*/
/*--------------------------------------------------------------------------*/
