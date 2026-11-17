/*--------------------------------------------------------------------------*/
/*---------------------------- File succinct.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The succinct structures of the module `ads`: `BitVector`, `RankSelect`,
 * the `PackedArray` of 1 to 32 bits, `EliasFano`, and `RoaringBitmap` with
 * the portable format of Roaring, which pyroaring and CRoaring read and
 * write. The words of a bit vector and of a packed array are views of
 * NumPy over the structure; the queries take one position or an array of
 * them, the latter answered with the GIL released.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "Bindings.h"

#include "ADS/set/RoaringBitmap.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/PackedArray.h"
#include "ADS/succinct/RankSelect.h"

#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ads_python
{

using namespace nb::literals;

namespace
{

using Word = std::uint64_t;

/// the answers of \f$ f \f$ to each position of \f$ a \f$, with the GIL
/// released
template< class R , class F >
Array< R > each( const Input< std::uint64_t > & a , F && f )
{
 const std::span< const std::uint64_t > s = items( a );
 std::vector< R > r( s.size() );
 {
  nb::gil_scoped_release g;
  for( std::size_t k = 0 ; k < s.size() ; ++k )
   r[ k ] = R( f( s[ k ] ) );
  }
 return( owned( std::move( r ) ) );
 }

/// the position \f$ i \f$ of a structure of \f$ n \f$ items checked
inline void checkIndex( std::size_t i , std::size_t n )
{
 if( i >= n )
  throw( nb::index_error( "position out of range" ) );
 }

/// the packed array of B bits, named \f$ s \f$
template< unsigned B >
void bindPacked( nb::module_ & m , const char * s )
{
 using P = ADS::PackedArray< B >;
 nb::class_< P > c( m , s );
 c.doc() = "An array of unsigned fields of a fixed number of bits, 64 / "
	   "bits of them in each word of 64 bits.";
 c.def( nb::init< std::size_t , Word >() , "n"_a = 0 , "value"_a = 0 ,
	"n fields, each of the value." )
   .def_static(
     "from_values" ,
     []( const Input< std::uint64_t > & v ) {
      const auto x = items( v );
      for( const std::uint64_t y : x )
       if( y > P::MAX )
	throw( std::invalid_argument( "a value does not fit in the field" ) );
      nb::gil_scoped_release r;
      P p( x.size() );
      for( std::size_t k = 0 ; k < x.size() ; ++k )
       p.set( k , x[ k ] );
      return( p );
      } ,
     "values"_a , "The fields of the values." )
   .def_static(
     "from_words" ,
     []( const Input< std::uint64_t > & w , std::size_t n ) {
      if( w.shape( 0 ) < ( n + P::PER - 1 ) / P::PER )
       throw( std::invalid_argument( "fewer words than the fields need" ) );
      return( P( items( w ) , n ) );
      } ,
     "words"_a , "n"_a ,
     "The n fields of the words, in the layout of the words property." )
   .def( "__len__" , &P::size )
   .def(
     "__getitem__" ,
     []( const P & p , std::size_t i ) {
      checkIndex( i , p.size() );
      return( p.get( i ) );
      } ,
     "i"_a )
   .def(
     "__setitem__" ,
     []( P & p , std::size_t i , Word v ) {
      checkIndex( i , p.size() );
      if( v > P::MAX )
       throw( std::invalid_argument( "the value does not fit in the field" ) );
      p.set( i , v );
      } ,
     "i"_a , "value"_a )
   .def(
     "append" ,
     []( P & p , Word v ) {
      if( v > P::MAX )
       throw( std::invalid_argument( "the value does not fit" ) );
      p.push_back( v );
      } ,
     "value"_a )
   .def(
     "get" ,
     []( const P & p , const Input< std::uint64_t > & a ) {
      for( const std::uint64_t i : items( a ) )
       checkIndex( i , p.size() );
      return( each< std::uint64_t >(
	a , [ & ]( std::uint64_t i ) { return( p.get( i ) ); } ) );
      } ,
     "positions"_a , "The fields at the positions." )
   .def(
     "to_numpy" ,
     []( const P & p ) {
      std::vector< std::uint64_t > v( p.size() );
      for( std::size_t k = 0 ; k < v.size() ; ++k )
       v[ k ] = p.get( k );
      return( owned( std::move( v ) ) );
      } ,
     "The fields, copied into an array of 64 bits." )
   .def_prop_ro(
     "words" ,
     []( const P & p ) {
      const auto & w = p.words();
      return( view( w.data() , w.size() , nb::find( p ) ) );
      } ,
     "The words of the fields, a view (no copy)." )
   .def_prop_ro( "nbytes" , &P::bytes , "The bytes of the words." );
 c.attr( "bits" ) = B;
 bindPersistence< P >( c );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE AREA -------------------------------*/
/*--------------------------------------------------------------------------*/

void bindSuccinct( nb::module_ & m )
{
 // BitVector
 nb::class_< ADS::BitVector > bv(
   m , "BitVector" ,
   "A vector of bits, bit i in bit i % 64 of word i / 64 from the least "
   "significant one (the layout of the bitmaps of Arrow and of "
   "sdsl::bit_vector)." );
 bv.def( nb::init< std::size_t , bool >() , "n"_a = 0 , "value"_a = false ,
	 "n bits, each of the value." )
   .def_static(
     "from_bools" ,
     []( const Input< bool > & a ) {
      const auto x = items( a );
      nb::gil_scoped_release r;
      ADS::BitVector b( x.size() );
      for( std::size_t k = 0 ; k < x.size() ; ++k )
       if( x[ k ] )
	b.set( k );
      return( b );
      } ,
     "bits"_a , "The bits of an array of booleans." )
   .def_static(
     "from_words" ,
     []( const Input< std::uint64_t > & w , std::size_t n ) {
      if( w.shape( 0 ) < ( n + 63 ) / 64 )
       throw( std::invalid_argument( "fewer words than the bits need" ) );
      return( ADS::BitVector( items( w ) , n ) );
      } ,
     "words"_a , "n"_a ,
     "The first n bits of the words (e.g., of numpy.packbits with "
     "bitorder='little', viewed as uint64)." )
   .def( "__len__" , &ADS::BitVector::size )
   .def(
     "__getitem__" ,
     []( const ADS::BitVector & b , std::size_t i ) {
      checkIndex( i , b.size() );
      return( b[ i ] );
      } ,
     "i"_a )
   .def(
     "__setitem__" ,
     []( ADS::BitVector & b , std::size_t i , bool v ) {
      checkIndex( i , b.size() );
      b.set( i , v );
      } ,
     "i"_a , "value"_a )
   .def( "append" , &ADS::BitVector::push_back , "value"_a )
   .def( "count" , &ADS::BitVector::count , "The number of 1s." )
   .def(
     "to_numpy" ,
     []( const ADS::BitVector & b ) {
      auto * p = new std::vector< std::uint8_t >( b.size() );
      for( std::size_t k = 0 ; k < b.size() ; ++k )
       ( *p )[ k ] = b[ k ];
      nb::capsule c( p , []( void * q ) noexcept {
       delete static_cast< std::vector< std::uint8_t > * >( q );
       } );
      return( Array< bool >( reinterpret_cast< bool * >( p->data() ) ,
			      { p->size() } , c ) );
      } ,
     "The bits, copied into an array of booleans." )
   .def_prop_ro(
     "words" ,
     []( const ADS::BitVector & b ) {
      const auto & w = b.words();
      return( view( w.data() , ( b.size() + 63 ) / 64 , nb::find( b ) ) );
      } ,
     "The words that hold the bits, a view (no copy)." )
   .def( "__eq__" , []( const ADS::BitVector & a ,
		       const ADS::BitVector & b ) { return( a == b ); } )
   .def( "__repr__" , []( const ADS::BitVector & b ) {
    return( "BitVector(n=" + std::to_string( b.size() ) +
	     ", ones=" + std::to_string( b.count() ) + ")" );
    } );
 bindPersistence< ADS::BitVector >( bv );

 // RankSelect
 nb::class_< ADS::RankSelect > rs(
   m , "RankSelect" ,
   "A bit vector with rank in O(1) time and select in O(1) expected "
   "time, the directory of the rank in 3.125% of the bits." );
 rs.def(
     "__init__" ,
     []( ADS::RankSelect * t , const ADS::BitVector & b ) {
      nb::gil_scoped_release r;
      new( t ) ADS::RankSelect( b );
      } ,
     "bits"_a , "The rank and the select of a copy of the bits." )
   .def_prop_ro( "bits" , &ADS::RankSelect::bits ,
		 nb::rv_policy::reference_internal , "The bits." )
   .def( "__len__" , &ADS::RankSelect::size )
   .def(
     "__getitem__" ,
     []( const ADS::RankSelect & s , std::size_t i ) {
      checkIndex( i , s.size() );
      return( s[ i ] );
      } ,
     "i"_a )
   .def_prop_ro( "ones" , &ADS::RankSelect::ones , "The number of 1s." )
   .def_prop_ro( "zeros" , &ADS::RankSelect::zeros , "The number of 0s." )
   .def(
     "rank1" ,
     []( const ADS::RankSelect & s , std::size_t i ) {
      checkIndex( i , s.size() + 1 );
      return( s.rank1( i ) );
      } ,
     "i"_a , "The 1s before position i, for 0 <= i <= len." )
   .def(
     "rank1" ,
     []( const ADS::RankSelect & s , const Input< std::uint64_t > & a ) {
      for( const std::uint64_t i : items( a ) )
       checkIndex( i , s.size() + 1 );
      return( each< std::uint64_t >(
	a , [ & ]( std::uint64_t i ) { return( s.rank1( i ) ); } ) );
      } ,
     "positions"_a , "The rank1 of each position." )
   .def(
     "rank0" ,
     []( const ADS::RankSelect & s , std::size_t i ) {
      checkIndex( i , s.size() + 1 );
      return( s.rank0( i ) );
      } ,
     "i"_a , "The 0s before position i, for 0 <= i <= len." )
   .def(
     "select1" ,
     []( const ADS::RankSelect & s , std::size_t k ) {
      if( ( k < 1 ) || ( k > s.ones() ) )
       throw( nb::index_error( "k must be in [1, ones]" ) );
      return( s.select1( k ) );
      } ,
     "k"_a , "The position of the k-th 1, for 1 <= k <= ones." )
   .def(
     "select1" ,
     []( const ADS::RankSelect & s , const Input< std::uint64_t > & a ) {
      for( const std::uint64_t k : items( a ) )
       if( ( k < 1 ) || ( k > s.ones() ) )
	throw( nb::index_error( "k must be in [1, ones]" ) );
      return( each< std::uint64_t >(
	a , [ & ]( std::uint64_t k ) { return( s.select1( k ) ); } ) );
      } ,
     "k"_a , "The select1 of each k." )
   .def(
     "select0" ,
     []( const ADS::RankSelect & s , std::size_t k ) {
      if( ( k < 1 ) || ( k > s.zeros() ) )
       throw( nb::index_error( "k must be in [1, zeros]" ) );
      return( s.select0( k ) );
      } ,
     "k"_a , "The position of the k-th 0, for 1 <= k <= zeros." )
   .def_prop_ro( "directory_bits" , &ADS::RankSelect::directoryBits ,
		 "The bits of the directories." );
 bindPersistence< ADS::RankSelect >( rs );

 // PackedArray
 bindPacked< 1 >( m , "PackedArray1" );
 bindPacked< 2 >( m , "PackedArray2" );
 bindPacked< 4 >( m , "PackedArray4" );
 bindPacked< 8 >( m , "PackedArray8" );
 bindPacked< 16 >( m , "PackedArray16" );
 bindPacked< 32 >( m , "PackedArray32" );

 // EliasFano
 nb::class_< ADS::EliasFano > ef(
   m , "EliasFano" ,
   "A non-decreasing sequence of integers of 64 bits in 2 + log(u / n) "
   "bits each, with access and next_geq in O(1) expected time." );
 ef.def( nb::init<>() )
   .def(
     "__init__" ,
     []( ADS::EliasFano * t , const Input< std::uint64_t > & v ,
	 std::optional< std::uint64_t > u ) {
      const auto x = items( v );
      for( std::size_t k = 1 ; k < x.size() ; ++k )
       if( x[ k - 1 ] > x[ k ] )
	throw( std::invalid_argument( "the values must not decrease" ) );
      if( ! x.empty() && ( x.back() == ~std::uint64_t( 0 ) ) )
       throw( std::invalid_argument( "a value of 2^64 - 1" ) );
      if( u && ! x.empty() && ( x.back() >= *u ) )
       throw( std::invalid_argument( "a value not below the universe" ) );
      const std::vector< std::uint64_t > y( x.begin() , x.end() );
      nb::gil_scoped_release r;
      if( u )
       new( t ) ADS::EliasFano( y , *u );
      else
       new( t ) ADS::EliasFano( y );
      } ,
     "values"_a , "universe"_a = nb::none() ,
     "The sequence of the values, non-decreasing, below the universe "
     "(the last value + 1 if None)." )
   .def( "__len__" , &ADS::EliasFano::size )
   .def_prop_ro( "universe" , &ADS::EliasFano::universe )
   .def(
     "__getitem__" ,
     []( const ADS::EliasFano & e , std::size_t i ) {
      checkIndex( i , e.size() );
      return( e.access( i ) );
      } ,
     "i"_a )
   .def(
     "access" ,
     []( const ADS::EliasFano & e , const Input< std::uint64_t > & a ) {
      for( const std::uint64_t i : items( a ) )
       checkIndex( i , e.size() );
      return( each< std::uint64_t >(
	a , [ & ]( std::uint64_t i ) { return( e.access( i ) ); } ) );
      } ,
     "positions"_a , "The values at the positions." )
   .def( "next_geq" , &ADS::EliasFano::nextGEQ , "x"_a ,
	 "The position of the first value >= x, len if none." )
   .def(
     "next_geq" ,
     []( const ADS::EliasFano & e , const Input< std::uint64_t > & a ) {
      return( each< std::uint64_t >(
	a , [ & ]( std::uint64_t x ) { return( e.nextGEQ( x ) ); } ) );
      } ,
     "x"_a , "The next_geq of each value." )
   .def( "__contains__" , &ADS::EliasFano::contains , "x"_a )
   .def(
     "to_numpy" ,
     []( const ADS::EliasFano & e ) { return( owned( e.decode() ) ); } ,
     "The values, decoded." )
   .def_prop_ro( "size_in_bits" , &ADS::EliasFano::sizeInBits ,
		 "The bits of the structure." );
 bindPersistence< ADS::EliasFano >( ef );

 // RoaringBitmap
 using R = ADS::RoaringBitmap;
 nb::class_< R > rb(
   m , "RoaringBitmap" ,
   "A compressed set of integers of 32 bits in the containers of "
   "Roaring, written and read in the portable format of Roaring, which "
   "pyroaring (BitMap.serialize and BitMap.deserialize) and CRoaring "
   "share." );
 rb.def( nb::init<>() )
   .def(
     "__init__" ,
     []( R * t , const Input< std::uint32_t > & a ) {
      const auto x = items( a );
      bool increasing = true;
      for( std::size_t k = 1 ; k < x.size() ; ++k )
       increasing = increasing && ( x[ k - 1 ] < x[ k ] );
      nb::gil_scoped_release r;
      if( increasing )
       new( t ) R( x.data() , x.size() );
      else {
       std::vector< std::uint32_t > y( x.begin() , x.end() );
       std::sort( y.begin() , y.end() );
       y.erase( std::unique( y.begin() , y.end() ) , y.end() );
       new( t ) R( y.data() , y.size() );
       }
      } ,
     "values"_a , "The set of the values, in any order." )
   .def( "add" , &R::insert , "x"_a , "Inserts x; False if it was there." )
   .def( "discard" , &R::remove , "x"_a ,
	 "Removes x; False if it was not there." )
   .def( "__contains__" , &R::contains , "x"_a )
   .def(
     "contains" ,
     []( const R & b , const Input< std::uint32_t > & a ) {
      const auto x = items( a );
      auto * p = new std::vector< std::uint8_t >( x.size() );
      {
       nb::gil_scoped_release r;
       for( std::size_t k = 0 ; k < x.size() ; ++k )
	( *p )[ k ] = b.contains( x[ k ] );
       }
      nb::capsule c( p , []( void * q ) noexcept {
       delete static_cast< std::vector< std::uint8_t > * >( q );
       } );
      return( Array< bool >( reinterpret_cast< bool * >( p->data() ) ,
			      { p->size() } , c ) );
      } ,
     "values"_a , "Whether each value is in the set." )
   .def( "__len__" , &R::size )
   .def( "rank" , &R::rank , "x"_a , "The items <= x." )
   .def(
     "to_numpy" , []( const R & b ) { return( owned( b.items() ) ); } ,
     "The items, increasing." )
   .def_prop_ro( "containers" , &R::containers , "The containers." )
   .def_prop_ro( "nbytes" , &R::bytes , "The bytes of the containers." )
   .def(
     "serialize" ,
     []( const R & b ) {
      const std::vector< std::byte > v = b.writePortable();
      return( nb::bytes( v.data() , v.size() ) );
      } ,
     "The set in the portable format of Roaring." )
   .def_static(
     "deserialize" ,
     []( nb::bytes s ) {
      const auto * p = static_cast< const std::byte * >( s.data() );
      return(
	R::readPortable( std::span< const std::byte >( p , s.size() ) ) );
      } ,
     "data"_a , "The set of bytes in the portable format of Roaring." )
   .def( "__or__" ,
	 []( const R & a , const R & b ) { return( setUnion( a , b ) ); } )
   .def( "__and__" , []( const R & a ,
			const R & b ) { return( setIntersection( a , b ) ); } )
   .def( "__sub__" ,
	 []( const R & a ,
	     const R & b ) { return( setDifference( a , b ) ); } )
   .def( "__xor__" ,
	 []( const R & a , const R & b ) {
	  return( setSymmetricDifference( a , b ) );
	  } )
   .def( "__eq__" ,
	 []( const R & a , const R & b ) {
	  return( ( a.size() == b.size() ) && ( a.items() == b.items() ) );
	  } )
   .def( "__repr__" ,
	 []( const R & b ) {
	  return( "RoaringBitmap(len=" + std::to_string( b.size() ) + ")" );
	  } )
   .def( "__getstate__" ,
	 []( const R & b ) {
	  const std::vector< std::byte > v = b.writePortable();
	  return( std::make_tuple( nb::bytes( v.data() , v.size() ) ) );
	  } )
   .def( "__setstate__" , []( R & b , const std::tuple< nb::bytes > & s ) {
    const nb::bytes & x = std::get< 0 >( s );
    const auto * p = static_cast< const std::byte * >( x.data() );
    new( &b )
      R( R::readPortable( std::span< const std::byte >( p , x.size() ) ) );
    } );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ads_python )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File succinct.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
