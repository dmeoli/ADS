/*--------------------------------------------------------------------------*/
/*---------------------------- File filters.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The filters of the module `ads`, on keys of 64 bits: the Bloom filter of
 * the book, the blocked one, the cuckoo filter and the prefix filter, which
 * take insertions, and the binary fuse filters of 8 and 16 bits, the
 * ribbon filter and the range filter Grafite, built once from an array of
 * the keys. A key or an array of them is inserted or asked for, the latter
 * with the GIL released, and every filter is stored and opened by the
 * persistence of ADS.
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

#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/BloomFilter.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/GrafiteFilter.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"

#include <nanobind/stl/tuple.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
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

using Key = std::uint64_t;

/// whether each key of \f$ a \f$ may be in the filter \f$ f \f$, with the
/// GIL released
template< class F >
Array< bool > contains( const F & f , const Input< Key > & a )
{
 const std::span< const Key > x = items( a );
 auto * p = new std::vector< std::uint8_t >( x.size() );
 {
  nb::gil_scoped_release r;
  for( std::size_t k = 0 ; k < x.size() ; ++k )
   ( *p )[ k ] = f.maybe( x[ k ] );
  }
 nb::capsule c( p , []( void * q ) noexcept {
  delete static_cast< std::vector< std::uint8_t > * >( q );
  } );
 return( Array< bool >( reinterpret_cast< bool * >( p->data() ) ,
			 { p->size() } , c ) );
 }

/// the keys of the array \f$ a \f$, copied
inline std::vector< Key > keys( const Input< Key > & a )
{
 const std::span< const Key > x = items( a );
 return( std::vector< Key >( x.begin() , x.end() ) );
 }

/// what every filter F has, in its class \f$ c \f$
template< class F , class C >
void bindQueries( C & c )
{
 c.def( "__contains__" , &F::maybe , "key"_a ,
	"Whether the key may be in the set (always if it is)." )
   .def( "contains" , &contains< F > , "keys"_a ,
	 "Whether each key may be in the set." );
 if constexpr( requires( const F & f ) { f.bits(); } )
  c.def_prop_ro( "bits" , &F::bits , "The bits of the filter." );
 else
  c.def_prop_ro( "bits" , &F::sizeInBits , "The bits of the filter." );
 if constexpr( requires( const F & f ) { f.size(); } )
  c.def( "__len__" , &F::size );
 bindPersistence< F >( c );
 }

/// the insertions of a filter F that takes them, in its class \f$ c \f$;
/// `insert()` of an array stops at the first key that does not fit
template< class F , class C >
void bindInsertions( C & c )
{
 using R = decltype( std::declval< F & >().insert( Key() ) );
 c.def( "add" , &F::insert , "key"_a ,
	std::is_same_v< R , bool >
	  ? "Inserts the key; False if the filter is full."
	  : "Inserts the key." );
 c.def(
   "update" ,
   []( F & f , const Input< Key > & a ) {
    const std::span< const Key > x = items( a );
    nb::gil_scoped_release r;
    for( std::size_t k = 0 ; k < x.size() ; ++k )
     if constexpr( std::is_same_v< R , bool > ) {
      if( ! f.insert( x[ k ] ) )
       return( k );
      }
     else
      f.insert( x[ k ] );
    return( x.size() );
    } ,
   "keys"_a ,
   "Inserts the keys in order; the number inserted, fewer than the keys "
   "only if the filter became full." );
 }

/// the static filter F, built from an array of keys, named \f$ s \f$
template< class F >
void bindStatic( nb::module_ & m , const char * s , const char * doc )
{
 nb::class_< F > c( m , s , doc );
 c.def(
   "__init__" ,
   []( F * t , const Input< Key > & a , std::uint64_t seed ) {
    std::vector< Key > k = keys( a );
    nb::gil_scoped_release r;
    new( t ) F( k , seed );
    } ,
   "keys"_a , "seed"_a = 1 , "The filter of the keys." );
 bindQueries< F >( c );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE AREA -------------------------------*/
/*--------------------------------------------------------------------------*/

void bindFilters( nb::module_ & m )
{
 using Bloom = ADS::BloomFilter< Key >;
 nb::class_< Bloom > bf( m , "BloomFilter" ,
			 "The Bloom filter of the book, m bits and k "
			 "hashes." );
 bf.def( nb::init< std::size_t , int >() , "m"_a , "k"_a )
   .def_static( "for_items" , &Bloom::forItems , "n"_a , "p"_a ,
		"The filter for n keys at a false positive rate p." )
   .def_prop_ro( "hashes" , &Bloom::hashes )
   .def( "false_positive_rate" , &Bloom::falsePositiveRate );
 bindInsertions< Bloom >( bf );
 bindQueries< Bloom >( bf );

 using Blocked = ADS::BlockedBloomFilter< Key >;
 nb::class_< Blocked > bb(
   m , "BlockedBloomFilter" ,
   "A Bloom filter whose hashes of a key fall in one line of the cache." );
 bb.def( nb::init< std::size_t , double >() , "n"_a ,
	 "bits_per_item"_a = 10.0 ,
	 "The filter for n keys." )
   .def( "false_positive_rate" , &Blocked::falsePositiveRate )
   .def( "clear" , &Blocked::clear );
 bindInsertions< Blocked >( bb );
 bindQueries< Blocked >( bb );

 using Cuckoo = ADS::CuckooFilter< Key >;
 nb::class_< Cuckoo > cf(
   m , "CuckooFilter" ,
   "The cuckoo filter of Fan et al., fingerprints of 8 bits in buckets "
   "of 4, which also takes deletions." );
 cf.def( nb::init< std::size_t >() , "n"_a , "The filter for n keys." )
   .def( "discard" , &Cuckoo::remove , "key"_a ,
	 "Removes a key inserted before; False if not found." )
   .def_prop_ro( "capacity" , &Cuckoo::capacity )
   .def_prop_ro( "load" , &Cuckoo::load )
   .def( "clear" , &Cuckoo::clear );
 bindInsertions< Cuckoo >( cf );
 bindQueries< Cuckoo >( cf );

 using Prefix = ADS::PrefixFilter< Key >;
 nb::class_< Prefix > pf(
   m , "PrefixFilter" ,
   "The prefix filter of Even, Even and Morrison: one line of the cache "
   "for almost every query." );
 pf.def( nb::init< std::size_t >() , "n"_a , "The filter for n keys." )
   .def_prop_ro( "capacity" , &Prefix::capacity )
   .def_prop_ro( "load" , &Prefix::load )
   .def( "false_positive_rate" , &Prefix::falsePositiveRate )
   .def( "clear" , &Prefix::clear );
 bindInsertions< Prefix >( pf );
 bindQueries< Prefix >( pf );

 bindStatic< ADS::BinaryFuseFilter< Key , std::uint8_t > >(
   m , "BinaryFuseFilter8" ,
   "The binary fuse filter of Graf and Lemire, fingerprints of 8 bits, "
   "about 9 bits per key." );
 bindStatic< ADS::BinaryFuseFilter< Key , std::uint16_t > >(
   m , "BinaryFuseFilter16" ,
   "The binary fuse filter of Graf and Lemire, fingerprints of 16 bits, "
   "about 18 bits per key." );
 bindStatic< ADS::RibbonFilter< Key , std::uint8_t > >(
   m , "RibbonFilter" ,
   "The standard ribbon filter of Dillinger and Walzer, results of 8 "
   "bits." );

 using Grafite = ADS::GrafiteFilter< Key >;
 nb::class_< Grafite > gf(
   m , "GrafiteFilter" ,
   "The range filter Grafite of Costa, Ferragina and Vinciguerra, which "
   "answers whether a key may lie in an interval." );
 gf.def(
     "__init__" ,
     []( Grafite * t , const Input< Key > & a , int B , std::uint64_t seed ) {
      if( ( B < 2 ) || ( B > 40 ) )
       throw( std::invalid_argument( "bits_per_key must be in [2, 40]" ) );
      std::vector< Key > k = keys( a );
      nb::gil_scoped_release r;
      new( t ) Grafite( k , B , seed );
      } ,
     "keys"_a , "bits_per_key"_a = 16 , "seed"_a = 1 ,
     "The filter of the keys." )
   .def( "may_contain_range" , &Grafite::mayContain , "a"_a , "b"_a ,
	 "Whether a key may lie in [a, b] (always if one does)." )
   .def( "false_positive_bound" , &Grafite::falsePositiveBound , "length"_a ,
	 "The bound of the false positive rate of the ranges of a length." );
 bindQueries< Grafite >( gf );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ads_python )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File filters.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
