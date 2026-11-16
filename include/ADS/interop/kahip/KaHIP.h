/*--------------------------------------------------------------------------*/
/*------------------------------ File KaHIP.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The partitions of KaHIP (https://github.com/KaHIP/KaHIP) of a
 * `StaticGraph`, beyond the book: `partition( g , k , o )` calls `kaffpa()`,
 * the multilevel partitioner of KaHIP, on the compressed rows of \f$ g \f$
 * and gives back a `Partition` [see `interop/Csr.h`], the part of each node
 * and the weight of the edges cut, whose `order()` relabels the graph part
 * by part through `StaticGraph::relabel()`.
 *
 * KaHIP takes the arrays of METIS, `xadj`, `adjncy` and `adjcwgt` of the
 * type `kahip_idx`, i.e., `int32_t` unless KaHIP was built with
 * `KAHIP_64BIT`: in the first case it reads `offsets()` and `heads()` of a
 * graph of the separate layout in place, and `weightData()` too if the
 * weights are of that type; it reads every other layout through a copy
 * [see `CsrInput`], as the header of METIS does. `kaffpa()` takes its
 * arrays as pointers to non-constant data, and builds its own graph from
 * them.
 *
 * ADS has no partitioner of graphs, and this header gives none: it lets
 * KaHIP run on the graphs of ADS, whose rows it shares.
 *
 * This header is of the set interop: no entry point of ADS includes it,
 * and it needs KaHIP (3.0 or later), found by CMake when `ADS_WITH_KAHIP`
 * is AUTO or ON (the target `ADS::KaHIP`). The contracts [see
 * `Contract.h`]: the graph is unoriented, without loops, with positive
 * integral weights if any; \f$ 1 \le k \le n \f$; the library linked has
 * the `kahip_idx` of its header.
 *
 * \see P. Sanders, C. Schulz, Think locally, act globally: highly balanced
 *      graph partitioning, Proc. SEA 2013, LNCS 7933, 164-175, 2013,
 *      doi:10.1007/978-3-642-38527-8_16
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

#ifndef __ADS_interop_kahip_KaHIP
 #define __ADS_interop_kahip_KaHIP
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <kaHIP_interface.h> )
 #error "the interoperation with KaHIP needs the header of KaHIP"
#endif

#include "ADS/Contract.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/Csr.h"

#include <kaHIP_interface.h>

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::kahip
{

/*--------------------------------------------------------------------------*/
/*------------------------------ THE OPTIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the options of a partition by KaHIP
struct Options
{
 int mode = FAST;          ///< `FAST`, `ECO`, `STRONG` or their `*SOCIAL`
 bool weights = true;      ///< the weights of the arcs, if any, as `adjcwgt`
 double imbalance = 0.03;  ///< the largest part over the mean, less 1
 int seed = 0;             ///< the seed of KaHIP
 };

/*--------------------------------------------------------------------------*/
/*----------------------------- THE PARTITION ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the arrays of a graph for KaHIP: in place where the layouts agree
using Input = CsrInput< kahip_idx , kahip_idx >;

/// the partition of the nodes of \f$ g \f$ into \f$ k \f$ parts by
/// `kaffpa()`, on the input \f$ I \f$ of \f$ g \f$ (whose arrays it reads
/// in place, or the copies of `CsrInput`)
/// \pre \f$ g \f$ unoriented, without loops, \f$ 1 \le k \le n \f$
template< class G >
[[nodiscard]] Partition partition( const G & g , const Input & I , int k ,
				   const Options & o = Options() )
{
 ADS_PRE( ( ! g.directed() ) && ( k >= 1 ) && ( k <= g.n() ) &&
	  ( kahip_sizeof_idx() == int( sizeof( kahip_idx ) ) ) );
 Partition P;
 P.k = k;
 P.part.assign( std::size_t( g.n() ) , 0 );
 if( ( g.n() == 0 ) || ( k == 1 ) )
  return( P );
 int n = g.n() , parts = k;
 double eps = o.imbalance;
 kahip_idx cut = 0;
 // kaffpa() builds its graph from the arrays and writes none of them
 auto * xadj = const_cast< kahip_idx * >( I.offsets() );
 auto * adjncy = const_cast< kahip_idx * >( I.heads() );
 auto * adjcwgt =
   o.weights ? const_cast< kahip_idx * >( I.weights() ) : nullptr;
 kaffpa( &n , nullptr , xadj , adjcwgt , adjncy , &parts , &eps , true ,
	 o.seed ,
	 o.mode , &cut , P.part.data() );
 P.cut = static_cast< long long >( cut );
 return( P );
 }

/// the partition of the nodes of \f$ g \f$ into \f$ k \f$ parts by
/// `kaffpa()`, on the rows of \f$ g \f$ read in place where the layouts
/// agree
/// \pre \f$ g \f$ unoriented, without loops, \f$ 1 \le k \le n \f$
template< class G >
[[nodiscard]] Partition partition( const G & g , int k ,
				   const Options & o = Options() )
{
 const Input I( g , o.weights );
 return( partition( g , I , k , o ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::kahip )

/*--------------------------------------------------------------------------*/

#endif /* KaHIP.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File KaHIP.h ----------------------------*/
/*--------------------------------------------------------------------------*/
