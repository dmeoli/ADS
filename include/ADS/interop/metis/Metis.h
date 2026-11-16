/*--------------------------------------------------------------------------*/
/*------------------------------ File Metis.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The partitions of METIS (https://github.com/KarypisLab/METIS) of a
 * `StaticGraph`, beyond the book: `partition( g , k , o )` calls
 * `METIS_PartGraphKway()`, or `METIS_PartGraphRecursive()` when
 * `o.recursive`, on the compressed rows of \f$ g \f$, and gives back a
 * `Partition` [see `interop/Csr.h`], the part of each node and the weight
 * of the edges cut, whose `order()` relabels the graph part by part through
 * `StaticGraph::relabel()`.
 *
 * The rows of a `StaticGraph` are those of METIS, `xadj` the offsets and
 * `adjncy` the heads, both arcs of an edge of an unoriented graph present:
 * when `idx_t` is `int` (the 32-bit build of METIS, that of the
 * distributions) and the graph is of the separate layout, METIS reads
 * `offsets()` and `heads()` in place, and the weights of `weightData()` as
 * `adjwgt` if they are of the type `idx_t` too; it reads every other
 * layout through a copy [see `CsrInput`]: a 64-bit `idx_t` (copies of the
 * offsets and of the heads), weights of another type (a copy of them,
 * converted to `idx_t`), an interleaved graph (a copy of its heads and of
 * its weights). METIS takes its arrays as pointers to non-constant data but
 * writes none of them with the numbering of C, which `partition()` sets.
 *
 * ADS has no partitioner of graphs, and this header gives none: it lets
 * METIS run on the graphs of ADS, whose rows it shares.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs METIS 5, found by CMake when `ADS_WITH_METIS` is AUTO or ON
 * (the target `ADS::METIS`). The contracts [see `Contract.h`]: the graph
 * is unoriented, without loops, with positive integral weights if any;
 * \f$ 1 \le k \le n \f$; a failure of METIS throws `std::runtime_error`.
 *
 * \see G. Karypis, V. Kumar, A fast and high quality multilevel scheme for
 *      partitioning irregular graphs, SIAM Journal on Scientific Computing
 *      20(1), 359-392, 1998, doi:10.1137/S1064827595287997
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

#ifndef __ADS_interop_metis_Metis
 #define __ADS_interop_metis_Metis
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <metis.h> )
 #error "the interoperation with METIS needs the header of METIS"
#endif

#include "ADS/Contract.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/Csr.h"

#include <metis.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::metis
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE OPTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the options of a partition by METIS
struct Options
{
 bool recursive = false;   ///< recursive bisection instead of k-way
 bool weights = true;      ///< the weights of the arcs, if any, as `adjwgt`
 double imbalance = 1.03;  ///< the largest part over the mean, `ubvec`
 int seed = 0;             ///< the seed of METIS
 };

/*--------------------------------------------------------------------------*/
/*----------------------------- THE PARTITION ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the arrays of a graph for METIS: in place where the layouts agree
using Input = CsrInput< idx_t , idx_t >;

/// the partition of the nodes of \f$ g \f$ into \f$ k \f$ parts by METIS,
/// on the input \f$ I \f$ of \f$ g \f$ (whose arrays it reads in place, or
/// the copies of `CsrInput`)
/// \pre \f$ g \f$ unoriented, without loops, \f$ 1 \le k \le n \f$
template< class G >
[[nodiscard]] Partition partition( const G & g , const Input & I , int k ,
				   const Options & o = Options() )
{
 ADS_PRE( ( ! g.directed() ) && ( k >= 1 ) && ( k <= g.n() ) );
 Partition P;
 P.k = k;
 if( g.n() == 0 )
  return( P );
 idx_t n = idx_t( g.n() ) , ncon = 1 , parts = idx_t( k ) , cut = 0;
 real_t ub = real_t( o.imbalance );
 idx_t opt[ METIS_NOPTIONS ];
 METIS_SetDefaultOptions( opt );
 opt[ METIS_OPTION_NUMBERING ] = 0;
 opt[ METIS_OPTION_SEED ] = idx_t( o.seed );
 std::vector< idx_t > part( std::size_t( g.n() ) , 0 );
 // METIS reads the arrays and writes none of them
 auto * xadj = const_cast< idx_t * >( I.offsets() );
 auto * adjncy = const_cast< idx_t * >( I.heads() );
 auto * adjwgt = o.weights ? const_cast< idx_t * >( I.weights() ) : nullptr;
 int r = METIS_OK;
 if( k > 1 )
  r = ( o.recursive ? METIS_PartGraphRecursive : METIS_PartGraphKway )(
    &n , &ncon , xadj , adjncy , nullptr , nullptr , adjwgt , &parts ,
    nullptr , &ub ,
    opt , &cut , part.data() );
 if( r != METIS_OK )
  throw( std::runtime_error( "ADS::metis::partition: METIS returned " +
			     std::to_string( r ) ) );
 P.part.assign( part.begin() , part.end() );
 P.cut = static_cast< long long >( cut );
 return( P );
 }

/// the partition of the nodes of \f$ g \f$ into \f$ k \f$ parts by METIS,
/// on the rows of \f$ g \f$ read in place where the layouts agree
/// \pre \f$ g \f$ unoriented, without loops, \f$ 1 \le k \le n \f$
template< class G >
[[nodiscard]] Partition partition( const G & g , int k ,
				   const Options & o = Options() )
{
 const Input I( g , o.weights );
 return( partition( g , I , k , o ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::metis )

/*--------------------------------------------------------------------------*/

#endif /* Metis.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Metis.h ----------------------------*/
/*--------------------------------------------------------------------------*/
