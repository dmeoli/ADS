/*--------------------------------------------------------------------------*/
/*------------------------------- File Csr.h -------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compressed rows of a `StaticGraph` handed to a library that takes
 * them as plain arrays, and the partition that a partitioner of graphs
 * gives back, beyond the book: what the adapters of METIS, KaHIP and
 * GraphBLAS share, which includes no library.
 *
 * - `CsrInput< X , Y >` holds the offsets and the heads of a graph as
 *   arrays of the index type X, and its weights as an array of Y: a pointer
 *   into the graph itself when the types and the layout agree, i.e., X is
 *   `int` and the graph satisfies `CsrArrays` (a `StaticGraph` of the
 *   separate layout or an `ArrayGraph`), and Y is its type of the weights;
 *   a copy of \f$ n + 1 \f$ offsets, \f$ a \f$ heads or \f$ a \f$ weights
 *   otherwise, for \f$ a \f$ arcs, made in \f$ O( n + a ) \f$ time. Hence
 *   an unweighted graph of the separate layout reaches METIS (with
 *   `idx_t` of 32 bits) and KaHIP with no copy, a weighted one too if its
 *   weights are of the index type, and an interleaved graph, whose heads
 *   and weights alternate in one array, is copied. `copied()` tells which
 *   arrays were copied.
 * - `Partition` is the part of each node and the weight of the edges cut;
 *   `order()` is the permutation that numbers the nodes part by part, which
 *   `StaticGraph::relabel()` applies, so that the nodes of each part become
 *   consecutive in the rows (an order of locality for the parallel
 *   algorithms, and the blocks of the processes of the distributed layer).
 *
 * The contracts [see `Contract.h`]: the graph outlives a `CsrInput` and is
 * not changed while it lives; the parts are in \f$ [ 0 , k ) \f$. The
 * functions are defined in this file.
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

#ifndef __ADS_interop_Csr
 #define __ADS_interop_Csr
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/StaticGraph.h"

#include <cstddef>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS CsrInput -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the offsets and the heads of a graph as arrays of X, and its weights as
/// an array of Y, in place where the layouts agree and copied otherwise
template< class X , class Y = X >
class CsrInput
{
 public:
 /// which arrays were copied
 struct Copies
 {
  bool offsets = false;  ///< the offsets
  bool heads = false;    ///< the heads
  bool weights = false;  ///< the weights
  };

 /// the arrays of \f$ g \f$, with its weights if `weights` and if the
 /// graph is weighted; \f$ g \f$ must outlive the input and not change
 template< class G >
 explicit CsrInput( const G & g , bool weights = true ) {
  const std::size_t n = std::size_t( g.n() ) , a = std::size_t( g.arcs() );
  if constexpr( std::is_same_v< X , int > )
   f_off = g.offsets();
  else {
   f_offCopy.assign( g.offsets() , g.offsets() + n + 1 );
   f_off = f_offCopy.data();
   f_copies.offsets = true;
   }
  if constexpr( std::is_same_v< X , int > && CsrArrays< G > )
   f_head = g.heads();
  else {
   const auto h = g.arcHeads();
   f_headCopy.resize( a );
   for( std::size_t k = 0 ; k < a ; ++k )
    f_headCopy[ k ] = X( h[ std::ptrdiff_t( k ) ] );
   f_head = f_headCopy.data();
   f_copies.heads = true;
   }
  if constexpr( G::weighted ) {
   if( ! weights )
    return;
   using W = typename G::Weight;
   if constexpr( std::is_same_v< Y , W > && CsrArrays< G > )
    f_w = g.weightData();
   else {
    const auto w = g.arcWeights();
    f_wCopy.resize( a );
    for( std::size_t k = 0 ; k < a ; ++k )
     f_wCopy[ k ] = Y( w[ std::ptrdiff_t( k ) ] );
    f_w = f_wCopy.data();
    f_copies.weights = true;
    }
   }
  }

 CsrInput( const CsrInput & ) = delete;
 CsrInput & operator=( const CsrInput & ) = delete;

 /// the \f$ n + 1 \f$ offsets of the rows
 [[nodiscard]] const X * offsets( void ) const { return( f_off ); }
 /// the heads of the arcs, row by row
 [[nodiscard]] const X * heads( void ) const { return( f_head ); }
 /// the weights of the arcs in the places of the heads, null if the graph
 /// is unweighted or they were not asked
 [[nodiscard]] const Y * weights( void ) const { return( f_w ); }
 /// which arrays were copied
 [[nodiscard]] const Copies & copied( void ) const { return( f_copies ); }

 private:
 const X * f_off = nullptr;    ///< the offsets
 const X * f_head = nullptr;   ///< the heads
 const Y * f_w = nullptr;      ///< the weights
 std::vector< X > f_offCopy;   ///< the offsets, if copied
 std::vector< X > f_headCopy;  ///< the heads, if copied
 std::vector< Y > f_wCopy;     ///< the weights, if copied
 Copies f_copies;              ///< which were copied
 };  // end( class CsrInput )

/*--------------------------------------------------------------------------*/
/*---------------------------- STRUCT Partition ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a partition of the nodes of a graph into \f$ k \f$ parts
struct Partition
{
 std::vector< int > part;  ///< the part of each node, in \f$ [ 0 , k ) \f$
 long long cut = 0;        ///< the weight of the edges between two parts
 int k = 0;                ///< the number of the parts

 /// the name of each node when the nodes are numbered part by part, in
 /// the order of their names within a part: a permutation for
 /// `StaticGraph::relabel()`, in \f$ O( n + k ) \f$ time
 [[nodiscard]] std::vector< int > order( void ) const {
  std::vector< int > start( std::size_t( k ) + 1 , 0 );
  for( const int p : part ) {
   ADS_PRE( ( p >= 0 ) && ( p < k ) );
   ++start[ std::size_t( p ) + 1 ];
   }
  for( std::size_t q = 1 ; q < start.size() ; ++q )
   start[ q ] += start[ q - 1 ];
  std::vector< int > p( part.size() );
  for( std::size_t u = 0 ; u < part.size() ; ++u )
   p[ u ] = start[ std::size_t( part[ u ] ) ]++;
  return( p );
  }

 /// the number of the nodes of each part
 [[nodiscard]] std::vector< int > sizes( void ) const {
  std::vector< int > s( std::size_t( k ) , 0 );
  for( const int p : part )
   ++s[ std::size_t( p ) ];
  return( s );
  }
 };  // end( struct Partition )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Csr.h included */

/*--------------------------------------------------------------------------*/
/*----------------------------- End File Csr.h -----------------------------*/
/*--------------------------------------------------------------------------*/
