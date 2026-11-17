/*--------------------------------------------------------------------------*/
/*---------------------------- File Bindings.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * What the units of the extension module `ads._core` share: the arrays of
 * NumPy that the structures of ADS are read from and exposed as, the
 * persistence of a structure as methods of its class, the pool of threads
 * of the parallel algorithms, and the functions that bind each area. The
 * arrays given to Python are views when the bytes stay where the structure
 * keeps them (the owner of the view is then the Python object of the
 * structure, which lives as long as a view of it), and own a vector of the
 * result otherwise, so that no array is ever copied twice.
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

#ifndef __ADS_python_Bindings
 #define __ADS_python_Bindings
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/Persistence.h"
#include "ADS/parallel/ThreadPool.h"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/tuple.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ads_python
{

namespace nb = nanobind;

/// a one-dimensional array of NumPy of items T
template< class T >
using Array = nb::ndarray< nb::numpy , T , nb::ndim< 1 > >;

/// a one-dimensional array of items T read in place, contiguous, in the
/// memory of the host (converted by nanobind if it is not)
template< class T >
using Input =
  nb::ndarray< const T , nb::ndim< 1 > , nb::c_contig , nb::device::cpu >;

/// the vector \f$ v \f$ given to NumPy, which owns it from now on
template< class T >
Array< T > owned( std::vector< T > && v )
{
 auto * p = new std::vector< T >( std::move( v ) );
 nb::capsule c( p , []( void * q ) noexcept {
  delete static_cast< std::vector< T > * >( q );
  } );
 return( Array< T >( p->data() , { p->size() } , c ) );
 }

/// the \f$ n \f$ items at \f$ p \f$, \f$ s \f$ items apart, as a view
/// that cannot be written and keeps \f$ o \f$ alive
template< class T >
Array< const T > view( const T * p , std::size_t n , nb::handle o ,
		       std::int64_t s = 1 )
{
 return( Array< const T >( p , { n } , o , { s } ) );
 }

/// the items of the input \f$ a \f$ as a span
template< class T >
std::span< const T > items( const Input< T > & a )
{
 return( std::span< const T >( a.data() , a.shape( 0 ) ) );
 }

/// the pool of threads \f$ p \f$, or the one of the module, made at the
/// first call with a thread for each hardware thread and never destroyed
/// (its workers sleep when idle, and the process ends them)
inline ADS::ThreadPool & pool( ADS::ThreadPool * p )
{
 if( p )
  return( *p );
 static ADS::ThreadPool * f = new ADS::ThreadPool();
 return( *f );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ArrayArchive ----------------------------*/
/*--------------------------------------------------------------------------*/

/// an archive that reads the members of a structure from arrays and
/// numbers given in the order of its `persist()`, so that a structure is
/// filled from arrays of NumPy by one copy of each, with no file and no
/// parsing; a member that breaks an invariant of the structure throws
/// `std::invalid_argument`
class ArrayArchive
{
 public:
 /// a reader, never a writer
 static constexpr bool loading = true;

 /// an array: its items, their number and the bytes of one
 struct Items
 {
  const void * p;     ///< the first item
  std::size_t n;      ///< the items
  std::size_t width;  ///< the bytes of an item
  };

 /// a member: a number or an array
 using Member = std::variant< long long , double , Items >;

 /// the reader of the members \f$ m \f$, in order
 explicit ArrayArchive( std::vector< Member > m ) : f_m( std::move( m ) ) { }

 /// the array of the \f$ n \f$ items at \f$ p \f$
 template< class T >
 static Member of( const T * p , std::size_t n ) {
  return( Items{ p , n , sizeof( T ) } );
  }

 /// the members \f$ x \f$ read in order
 template< class... X >
 void operator()( X &... x ) {
  ( item( x ) , ... );
  }

 /// the structure rejected, as broken in `what`, unless \f$ c \f$
 void require( bool c , const char * what ) const {
  if( ! c )
   throw( std::invalid_argument( std::string( "not valid: " ) + what ) );
  }

 private:
 template< class T >
 struct IsVector : std::false_type
 { };

 template< class E , class A >
 struct IsVector< std::vector< E , A > > : std::true_type
 { };

 template< class T >
 void item( T & x ) {
  if( f_k == f_m.size() )
   throw( std::invalid_argument( "fewer members than the structure has" ) );
  const Member & s = f_m[ f_k++ ];
  if constexpr( IsVector< T >::value ) {
   using E = typename T::value_type;
   const Items & a = std::get< Items >( s );
   if( a.width != sizeof( E ) )
    throw( std::invalid_argument( "an array of items of another width" ) );
   const E * p = static_cast< const E * >( a.p );
   x.assign( p , p + a.n );
   }
  else if constexpr( std::is_floating_point_v< T > )
   x = T( std::get< double >( s ) );
  else
   x = T( std::get< long long >( s ) );
  }

 std::vector< Member > f_m;  ///< the members
 std::size_t f_k = 0;        ///< the next one
 };  // end( class ArrayArchive )

/*--------------------------------------------------------------------------*/
/*---------------------------- THE PERSISTENCE -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the persistence of the structure T as methods of its class \f$ c \f$:
/// `store( path )`, `restore( path )`, `restore_mapped( path , verify )`,
/// `to_bytes()`, `from_bytes( b )` and the state of pickle
template< class T , class C >
void bindPersistence( C & c )
{
 using namespace nb::literals;
 c.def(
    "store" ,
    []( const T & x , const std::filesystem::path & p ) {
     nb::gil_scoped_release r;
     ADS::store( x , p );
     } ,
    "path"_a , "Writes the structure to the file at path." )
   .def_static(
     "restore" ,
     []( const std::filesystem::path & p ) {
      nb::gil_scoped_release r;
      return( ADS::restore< T >( p ) );
      } ,
     "path"_a ,
     "Reads a structure from the file at path, its checksum checked." )
   .def_static(
     "restore_mapped" ,
     []( const std::filesystem::path & p , bool verify ) {
      nb::gil_scoped_release r;
      return( ADS::restoreMapped< T >( p , verify ) );
      } ,
     "path"_a , "verify"_a = false ,
     "Opens the file at path in place, in O(1) time: the large arrays of "
     "the structure are views of the mapped file, read at their first "
     "access; the checksum is checked only if verify." )
   .def(
     "to_bytes" ,
     []( const T & x ) {
      const std::vector< std::byte > b = ADS::store( x );
      return( nb::bytes( b.data() , b.size() ) );
      } ,
     "The bytes of the file that store() writes." )
   .def_static(
     "from_bytes" ,
     []( nb::bytes b ) {
      const auto * p = static_cast< const std::byte * >( b.data() );
      return(
	ADS::restore< T >( std::span< const std::byte >( p , b.size() ) ) );
      } ,
     "data"_a , "The structure of the bytes that to_bytes() gives." )
   .def( "__getstate__" ,
	 []( const T & x ) {
	  const std::vector< std::byte > b = ADS::store( x );
	  return( std::make_tuple( nb::bytes( b.data() , b.size() ) ) );
	  } )
   .def( "__setstate__" , []( T & x , const std::tuple< nb::bytes > & s ) {
    const nb::bytes & b = std::get< 0 >( s );
    const auto * p = static_cast< const std::byte * >( b.data() );
    new( &x )
      T( ADS::restore< T >( std::span< const std::byte >( p , b.size() ) ) );
    } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE AREAS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// `StaticGraph`, `CsrMatrix`, the pool of threads and the algorithms of
/// the graphs [see graphs.cpp]
void bindGraphs( nb::module_ & m );

/// the graph that tells its changes [see dynamic.cpp]
void bindDynamic( nb::module_ & m );

/// the succinct structures and `RoaringBitmap` [see succinct.cpp]
void bindSuccinct( nb::module_ & m );

/// the filters [see filters.cpp]
void bindFilters( nb::module_ & m );

/// the indexed heaps [see heaps.cpp]
void bindHeaps( nb::module_ & m );

/*--------------------------------------------------------------------------*/

}  // end( namespace ads_python )

/*--------------------------------------------------------------------------*/

#endif /* Bindings.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Bindings.h ---------------------------*/
/*--------------------------------------------------------------------------*/
