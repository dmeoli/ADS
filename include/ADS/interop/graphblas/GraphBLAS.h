/*--------------------------------------------------------------------------*/
/*---------------------------- File GraphBLAS.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The graphs and the sparse matrices of ADS as matrices of
 * SuiteSparse:GraphBLAS (https://github.com/DrTimothyAldenDavis/GraphBLAS),
 * so that the operations of GraphBLAS, and the algorithms of LAGraph
 * written on them, run on the structures of ADS, beyond the book:
 *
 * - `view( g )` of a graph whose rows are plain arrays (the concept
 *   `CsrArrays`: a `StaticGraph` of the separate layout, an `ArrayGraph`)
 *   is an \f$ n \times n \f$ `GrB_Matrix` by rows whose offsets and
 *   columns are `offsets()` and `heads()`, and whose values are
 *   `weightData()`, or one value `true` of a matrix iso-valued (all its
 *   entries equal) if the graph is unweighted;
 * - `view( A )` of a `BasicCsrMatrix< T , I , P >` is the
 *   \f$ m \times n \f$ `GrB_Matrix` by rows of its three arrays.
 *
 * Both make no copy: the arrays enter a `GxB_Container` of GraphBLAS 10 as
 * vectors loaded by `GxB_Vector_load()` with `GxB_IS_READONLY`, i.e., left
 * to their owner, which GraphBLAS reads and never writes nor frees, and the
 * container becomes the matrix by `GxB_load_Matrix_from_Container()`. The
 * integers of the offsets and of the columns are read as `GrB_UINT32` or
 * `GrB_UINT64`, by their size, which the integers of ADS, not negative,
 * allow; the values may be booleans, integers of 8 to 64 bits or floating
 * numbers. A graph of the interleaved layout, whose heads and weights
 * alternate in one array, has no such view, and `copy( g )` builds a matrix
 * of its arcs by `GrB_Matrix_build()` instead, in the layout that GraphBLAS
 * chooses, as for any graph of ADS with `out()`. Conversely, `toCsr( A )`
 * copies a `GrB_Matrix` into a `BasicCsrMatrix` by its triplets.
 *
 * A view is `Matrix`, the owner of the handle, which `GrB_Matrix_free()`
 * frees at its end, leaving the arrays to ADS. GraphBLAS has the semirings,
 * the masks and the algebraic algorithms (triangles, k-truss, centralities
 * in LAGraph), which ADS does not have and does not replace; the parallel
 * visits and the products of ADS stay its own.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs SuiteSparse:GraphBLAS 10 or later, found by CMake when
 * `ADS_WITH_GRAPHBLAS` is AUTO or ON (the target `ADS::GraphBLAS`). The
 * contracts [see `Contract.h`]: `GrB_init()` was called; the structure
 * viewed outlives its view and is not changed while it lives; a view is an
 * input of the operations of GraphBLAS, never their output nor changed by
 * `GrB_Matrix_setElement()` and the like; a failure of GraphBLAS throws
 * `std::runtime_error`.
 *
 * \see T. A. Davis, Algorithm 1000: SuiteSparse:GraphBLAS: graph algorithms
 *      in the language of sparse linear algebra, ACM Transactions on
 *      Mathematical Software 45(4), 1-25, 2019, doi:10.1145/3322125
 * \see T. A. Davis, Algorithm 1037: SuiteSparse:GraphBLAS: parallel graph
 *      algorithms in the language of sparse linear algebra, ACM
 *      Transactions on Mathematical Software 49(3), 1-30, 2023,
 *      doi:10.1145/3577195
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

#ifndef __ADS_interop_graphblas_GraphBLAS
 #define __ADS_interop_graphblas_GraphBLAS
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/numeric/CsrMatrix.h"

#if __has_include( <suitesparse/GraphBLAS.h> )
 #include <suitesparse/GraphBLAS.h>
#elif __has_include( <GraphBLAS.h> )
 #include <GraphBLAS.h>
#else
 #error "the interoperation with GraphBLAS needs the header of GraphBLAS"
#endif

// the macro I of C, the imaginary unit of <complex.h>, which the header of
// GraphBLAS includes: no program of C++ expects it, and it would rename the
// parameters I of the templates (of BasicCsrMatrix< T , I , P >, say)
#undef I

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::graphblas
{

static_assert( GxB_IMPLEMENTATION_MAJOR >= 10 ,
	       "the views need the containers of GraphBLAS 10" );

/*--------------------------------------------------------------------------*/
/*------------------------------ THE TYPES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// throws if \f$ r \f$ is not a success of GraphBLAS, naming `what`
inline void check( GrB_Info r , const char * what )
{
 if( ( r != GrB_SUCCESS ) && ( r != GrB_NO_VALUE ) )
  throw( std::runtime_error( std::string( "ADS::graphblas: " ) + what +
			     " returned " + std::to_string( int( r ) ) ) );
 }

/// the type of GraphBLAS of the numbers T: `GrB_BOOL`, the integers of
/// 8 to 64 bits with or without sign, `GrB_FP32` and `GrB_FP64`
template< class T >
[[nodiscard]] GrB_Type type( void )
{
 if constexpr( std::is_same_v< T , bool > )
  return( GrB_BOOL );
 else if constexpr( std::is_floating_point_v< T > ) {
  static_assert( ( sizeof( T ) == 4 ) || ( sizeof( T ) == 8 ) ,
		 "a floating type of 32 or 64 bits" );
  if constexpr( sizeof( T ) == 4 )
   return( GrB_FP32 );
  else
   return( GrB_FP64 );
  }
 else {
  static_assert( std::is_integral_v< T > , "a type of numbers" );
  constexpr bool s = std::is_signed_v< T >;
  if constexpr( sizeof( T ) == 1 )
   return( s ? GrB_INT8 : GrB_UINT8 );
  else if constexpr( sizeof( T ) == 2 )
   return( s ? GrB_INT16 : GrB_UINT16 );
  else if constexpr( sizeof( T ) == 4 )
   return( s ? GrB_INT32 : GrB_UINT32 );
  else {
   static_assert( sizeof( T ) == 8 , "an integer of 8 to 64 bits" );
   return( s ? GrB_INT64 : GrB_UINT64 );
   }
  }
 }

/// the type of GraphBLAS of the indices I, not negative, read by their
/// size
template< std::integral I >
[[nodiscard]] GrB_Type indexType( void )
{
 static_assert( ( sizeof( I ) == 4 ) || ( sizeof( I ) == 8 ) ,
		"the indices of GraphBLAS have 32 or 64 bits" );
 if constexpr( sizeof( I ) == 4 )
  return( GrB_UINT32 );
 else
  return( GrB_UINT64 );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Matrix -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the owner of a `GrB_Matrix`, freed at its end
class Matrix
{
 public:
 /// no matrix
 Matrix( void ) = default;

 /// the owner of \f$ A \f$
 explicit Matrix( GrB_Matrix A ) : f_A( A ) { }

 Matrix( const Matrix & ) = delete;
 Matrix & operator=( const Matrix & ) = delete;

 /// the matrix of \f$ o \f$, which keeps none
 Matrix( Matrix && o ) noexcept : f_A( std::exchange( o.f_A , nullptr ) ) { }

 /// the matrix of \f$ o \f$, which keeps none, this one freed
 Matrix & operator=( Matrix && o ) noexcept {
  if( this != &o ) {
   reset();
   f_A = std::exchange( o.f_A , nullptr );
   }
  return( *this );
  }

 ~Matrix( void ) { reset(); }

 /// the handle, for the calls of GraphBLAS
 [[nodiscard]] GrB_Matrix get( void ) const { return( f_A ); }

 /// the handle, for the calls of GraphBLAS
 operator GrB_Matrix( void ) const { return( f_A ); }

 /// the matrix freed, if any
 void reset( void ) noexcept {
  if( f_A != nullptr )
   GrB_Matrix_free( &f_A );
  f_A = nullptr;
  }

 private:
 GrB_Matrix f_A = nullptr;  ///< the handle
 };  // end( class Matrix )

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE VIEWS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

/// the vector \f$ v \f$ of a container over the \f$ k \f$ items of the type
/// \f$ t \f$ of \f$ s \f$ bytes each from \f$ x \f$, read only; a
/// placeholder \f$ z \f$ of one item if there is none
inline void loadReadOnly( GrB_Vector v , const void * x , GrB_Type t ,
			  std::size_t k , std::size_t s , const void * z )
{
 void * p = const_cast< void * >( ( k == 0 ) ? z : x );
 const std::size_t c = ( k == 0 ) ? 1 : k;
 check( GxB_Vector_load( v , &p , t , std::uint64_t( k ) ,
			 std::uint64_t( c * s ) ,
			 GxB_IS_READONLY , nullptr ) ,
	"GxB_Vector_load" );
 }

/// the \f$ m \times n \f$ matrix by rows of the \f$ \mathit{nnz} \f$
/// entries of the offsets \f$ p \f$, the columns \f$ i \f$ and the values
/// \f$ x \f$ (one if `iso`), all read in place
template< class P , class I , class T >
[[nodiscard]] Matrix csrView( std::size_t m , std::size_t n , std::size_t nnz ,
			      const P * p , const I * i , const T * x ,
			      bool iso )
{
 static constexpr P zp = 0;
 static constexpr I zi = 0;
 static constexpr T zx = T();
 GrB_Matrix A = nullptr;
 check( GrB_Matrix_new( &A , type< T >() , GrB_Index( m ) , GrB_Index( n ) ) ,
	"GrB_Matrix_new" );
 Matrix M( A );
 GxB_Container C = nullptr;
 check( GxB_Container_new( &C ) , "GxB_Container_new" );
 try {
  C->nrows = std::uint64_t( m );
  C->ncols = std::uint64_t( n );
  C->nrows_nonempty = -1;
  C->ncols_nonempty = -1;
  C->nvals = std::uint64_t( nnz );
  C->format = GxB_SPARSE;
  C->orientation = GrB_ROWMAJOR;
  C->iso = iso;
  C->jumbled = false;
  loadReadOnly( C->p , p , indexType< P >() , m + 1 , sizeof( P ) , &zp );
  loadReadOnly( C->i , i , indexType< I >() , nnz , sizeof( I ) , &zi );
  loadReadOnly( C->x , x , type< T >() , iso ? 1 : nnz , sizeof( T ) , &zx );
  check( GxB_load_Matrix_from_Container( A , C , nullptr ) ,
	 "GxB_load_Matrix_from_Container" );
  }
 catch( ... ) {
  GxB_Container_free( &C );
  throw;
  }
 GxB_Container_free( &C );
 return( M );
 }

}  // end( namespace detail )

/// the \f$ n \times n \f$ matrix of the arcs of \f$ g \f$ by rows, its
/// offsets, heads and weights read in place, iso-valued `true` if \f$ g \f$
/// is unweighted
template< CsrArrays G >
[[nodiscard]] Matrix view( const G & g )
{
 const std::size_t n = std::size_t( g.n() ) , a = std::size_t( g.arcs() );
 if constexpr( G::weighted )
  return( detail::csrView( n , n , a , g.offsets() , g.heads() ,
			   g.weightData() ,
			    false ) );
 else {
  static constexpr bool one = true;
  return( detail::csrView( n , n , a , g.offsets() , g.heads() , &one ,
			   true ) );
  }
 }

/// the \f$ m \times n \f$ matrix \f$ A \f$ by rows, its offsets, columns and
/// values read in place
template< class T , class I , class P >
[[nodiscard]] Matrix view( const BasicCsrMatrix< T , I , P > & A )
{
 return( detail::csrView( A.rows() , A.cols() , A.nonzeros() , A.rowStart() ,
			   A.columns() , A.values() , false ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE COPIES -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

/// the call of the typed function of GraphBLAS `F##_<T>`, for the type
/// `T` of the values of the arguments
#define ADS_GRB_TYPED( F, T, ... )                                            \
 [ & ]( void ) {                                                              \
 if constexpr( std::is_same_v< T, bool > )                                    \
  return( F##_BOOL( __VA_ARGS__ ) );                                          \
 else if constexpr( std::is_floating_point_v< T > ) {                         \
  if constexpr( sizeof( T ) == 4 )                                            \
   return( F##_FP32( __VA_ARGS__ ) );                                         \
  else                                                                        \
   return( F##_FP64( __VA_ARGS__ ) );                                         \
 }                                                                            \
 else if constexpr( std::is_signed_v< T > ) {                                 \
  if constexpr( sizeof( T ) == 1 )                                            \
   return( F##_INT8( __VA_ARGS__ ) );                                         \
  else if constexpr( sizeof( T ) == 2 )                                       \
   return( F##_INT16( __VA_ARGS__ ) );                                        \
  else if constexpr( sizeof( T ) == 4 )                                       \
   return( F##_INT32( __VA_ARGS__ ) );                                        \
  else                                                                        \
   return( F##_INT64( __VA_ARGS__ ) );                                        \
 }                                                                            \
 else {                                                                       \
  if constexpr( sizeof( T ) == 1 )                                            \
   return( F##_UINT8( __VA_ARGS__ ) );                                        \
  else if constexpr( sizeof( T ) == 2 )                                       \
   return( F##_UINT16( __VA_ARGS__ ) );                                       \
  else if constexpr( sizeof( T ) == 4 )                                       \
   return( F##_UINT32( __VA_ARGS__ ) );                                       \
  else                                                                        \
   return( F##_UINT64( __VA_ARGS__ ) );                                       \
 }                                                                            \
 }()

/// the integer of S bytes, with a sign if `Signed`
template< std::size_t S , bool Signed >
struct IntOf;
template<>
struct IntOf< 1 , true >
{
 using type = std::int8_t;  ///< the integer
 };
template<>
struct IntOf< 2 , true >
{
 using type = std::int16_t;  ///< the integer
 };
template<>
struct IntOf< 4 , true >
{
 using type = std::int32_t;  ///< the integer
 };
template<>
struct IntOf< 8 , true >
{
 using type = std::int64_t;  ///< the integer
 };
template<>
struct IntOf< 1 , false >
{
 using type = std::uint8_t;  ///< the integer
 };
template<>
struct IntOf< 2 , false >
{
 using type = std::uint16_t;  ///< the integer
 };
template<>
struct IntOf< 4 , false >
{
 using type = std::uint32_t;  ///< the integer
 };
template<>
struct IntOf< 8 , false >
{
 using type = std::uint64_t;  ///< the integer
 };

/// the numbers of the typed functions of GraphBLAS for T, which differ
/// from T at most in name (e.g., `std::int64_t` for `long long`)
template< class T >
using Number = typename std::conditional_t<
  std::is_same_v< T , bool > || std::is_floating_point_v< T > ,
  std::type_identity< T > ,
  IntOf< sizeof( T ) , std::is_signed_v< T > > >::type;

/// the entries \f$ ( R_q , C_q , X_q ) \f$ of the new matrix \f$ A \f$
template< class T >
[[nodiscard]] GrB_Info build( GrB_Matrix A , const GrB_Index * R ,
			      const GrB_Index * C , const T * X , GrB_Index k )
{
 const auto * x = reinterpret_cast< const Number< T > * >( X );
 return( ADS_GRB_TYPED( GrB_Matrix_build , T , A , R , C , x , k , nullptr ) );
 }

/// the entries of \f$ A \f$ into \f$ ( R_q , C_q , X_q ) \f$, \f$ k \f$ of
/// them at most, their number into \f$ k \f$
template< class T >
[[nodiscard]] GrB_Info extract( GrB_Index * R , GrB_Index * C , T * X ,
				GrB_Index * k , GrB_Matrix A )
{
 auto * x = reinterpret_cast< Number< T > * >( X );
 return( ADS_GRB_TYPED( GrB_Matrix_extractTuples , T , R , C , x , k , A ) );
 }

#undef ADS_GRB_TYPED

}  // end( namespace detail )

/// the \f$ n \times n \f$ matrix of the arcs of \f$ g \f$, copied by
/// `GrB_Matrix_build()` (for the layouts that `view()` does not take), of
/// the weights of \f$ g \f$, or `true` if it is unweighted
/// \pre no two arcs of \f$ g \f$ have the same ends
template< class W , ArcLayout L >
[[nodiscard]] Matrix copy( const StaticGraph< W , L > & g )
{
 constexpr bool weighted = StaticGraph< W , L >::weighted;
 using T = std::conditional_t< weighted , W , bool >;
 const std::size_t n = std::size_t( g.n() ) , a = std::size_t( g.arcs() );
 std::vector< GrB_Index > R( a ) , C( a );
 // not a std::vector, which has no data() for bool
 auto X = std::make_unique< T[] >( a + 1 );
 std::fill( X.get() , X.get() + a , T( 1 ) );
 const auto h = g.arcHeads();
 for( int u = 0 ; u < g.n() ; ++u )
  for( int k = g.offsets()[ u ] ; k < g.offsets()[ u + 1 ] ; ++k ) {
   R[ std::size_t( k ) ] = GrB_Index( u );
   C[ std::size_t( k ) ] = GrB_Index( h[ k ] );
   if constexpr( weighted )
    X[ std::size_t( k ) ] = g.arcWeights()[ k ];
   }
 GrB_Matrix A = nullptr;
 check( GrB_Matrix_new( &A , type< T >() , GrB_Index( n ) , GrB_Index( n ) ) ,
	"GrB_Matrix_new" );
 Matrix M( A );
 check( detail::build( A , R.data() , C.data() , X.get() , GrB_Index( a ) ) ,
	"GrB_Matrix_build" );
 return( M );
 }

/// the matrix \f$ A \f$ of GraphBLAS copied into a `BasicCsrMatrix` of the
/// values T, the columns I and the offsets P, by its triplets
/// \pre the sizes fit, the columns fit I and the entries P
template< class T , class I = std::uint32_t ,
	  class P = ADS::detail::CsrOffset< I > >
[[nodiscard]] BasicCsrMatrix< T , I , P > toCsr( GrB_Matrix A )
{
 GrB_Index m = 0 , n = 0 , k = 0;
 check( GrB_Matrix_nrows( &m , A ) , "GrB_Matrix_nrows" );
 check( GrB_Matrix_ncols( &n , A ) , "GrB_Matrix_ncols" );
 check( GrB_Matrix_nvals( &k , A ) , "GrB_Matrix_nvals" );
 std::vector< GrB_Index > R( k ) , C( k );
 auto X = std::make_unique< T[] >( std::size_t( k ) + 1 );
 GrB_Index got = k;
 check( detail::extract( R.data() , C.data() , X.get() , &got , A ) ,
	"GrB_Matrix_extractTuples" );
 std::vector< typename BasicCsrMatrix< T , I , P >::Triplet > E;
 E.reserve( std::size_t( got ) );
 for( GrB_Index q = 0 ; q < got ; ++q )
  E.emplace_back( std::size_t( R[ q ] ) , std::size_t( C[ q ] ) , X[ q ] );
 return( BasicCsrMatrix< T , I , P >( std::size_t( m ) , std::size_t( n ) ,
				     std::move( E ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::graphblas )

/*--------------------------------------------------------------------------*/

#endif /* GraphBLAS.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File GraphBLAS.h --------------------------*/
/*--------------------------------------------------------------------------*/
