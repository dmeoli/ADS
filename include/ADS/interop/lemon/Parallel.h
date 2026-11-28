/*--------------------------------------------------------------------------*/
/*---------------------------- File Parallel.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The algorithms of abLEMON on the threads of ADS, beyond the book (see
 * \ref theory_graph_lemon):
 *
 * - `dijkstraFrom( ex , g , l , S )`, the shortest distances from each
 *   source of \f$ S \f$ by \f$ |S| \f$ runs of the `Dijkstra` of abLEMON on
 *   the threads of the executor \f$ \mathit{ex} \f$ [see `Executor.h`], all
 *   on the one digraph \f$ g \f$, which nobody changes meanwhile: each run
 *   takes its distances, its predecessors and its cross references in a
 *   `DenseMap` [see `Maps.h`] and its heap from `H`, `DaryHeap` of ADS by
 *   default [see `Heaps.h`], so that no run attaches a map to the notifier
 *   of the graph, whose lock would serialize them, and the runs share no
 *   mutable state; the work is that of the \f$ |S| \f$ runs, the time that
 *   of the longest share of them on each thread. It is, e.g., the pricing of
 *   a generation of columns by one shortest path per commodity;
 * - `sortedArcs( ex , g , c )`, the pairs ( item , cost ) of the edges (or
 *   of the arcs) of \f$ g \f$ in the order of their costs, sorted by the
 *   parallel sort of ADS [see `ParallelSort.h`], which `lemon::kruskal()`
 *   takes as already sorted (it sorts a map by `std::sort`, a sequence not),
 *   so that the sort, which dominates Kruskal's algorithm on a sparse graph,
 *   runs on the threads of the executor.
 *
 * The contracts [see `Contract.h`]: the graph does not change while the runs
 * read it; the lengths are not negative. The functions are defined in this
 * file.
 *
 * \see E. W. Dijkstra, A note on two problems in connexion with graphs,
 *      Numerische Mathematik 1, 269-271, 1959, doi:10.1007/BF01386390
 * \see J. B. Kruskal, On the shortest spanning subtree of a graph and the
 *      traveling salesman problem, Proceedings of the American Mathematical
 *      Society 7(1), 48-50, 1956, doi:10.1090/S0002-9939-1956-0078686-7
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

#ifndef __ADS_interop_lemon_Parallel
 #define __ADS_interop_lemon_Parallel
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#if ! __has_include( <lemon/core.h> )
 #error "the interoperation with abLEMON needs the headers of abLEMON"
#endif

#include "ADS/graph/ShortestPaths.h"
#include "ADS/interop/lemon/Heaps.h"
#include "ADS/interop/lemon/Maps.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelSort.h"

#include <lemon/bits/traits.h>
#include <lemon/core.h>
#include <lemon/dijkstra.h>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::lemon_interop
{

/*--------------------------------------------------------------------------*/
/*------------------------------ dijkstraFrom ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the distances from each node of \f$ S \f$ in the digraph \f$ g \f$ of
/// the lengths \f$ l \f$, by one run of the `Dijkstra` of abLEMON per source
/// on the threads of \f$ \mathit{ex} \f$: the vector of each source, by the
/// ids of the nodes, `infinity()` for those not reached [see
/// `ShortestPaths.h`]
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dijkstraFrom}( \mathit{ex} , g , l , S )
///     \cr
/// 2 & \quad \mathit{ex}.\mathrm{bulk}( |S| , i \mapsto \text{a run of }
///     \mathrm{Dijkstra}( g , l ) \text{ from } S[ i ] \text{ on maps}
///     \text{ of its own, its distances in } D[ i ] ) \cr
/// 3 & \quad \textbf{return}\ D
/// \end{array}}
/// \f]
/// \pre every length is not negative, \f$ g \f$ does not change meanwhile
template< class H = void , Executor E , class GR , class LM >
[[nodiscard]] std::vector< std::vector< typename LM::Value > >
dijkstraFrom( E & ex , const GR & g , const LM & l ,
	      const std::vector< typename GR::Node > & S )
{
 using V = typename LM::Value;
 using CR = DenseMap< GR , int >;
 using DM = DenseMap< GR , V >;
 using PM = DenseMap< GR , typename GR::Arc >;
 using Heap = std::conditional_t< std::is_void_v< H > , DaryHeap< V , CR > ,
				  H >;
 using D =
   typename ::lemon::Dijkstra< GR , LM >::template SetPredMap< PM >::Create::
     template SetDistMap< DM >::Create::template SetHeap< Heap , CR >::Create;
 std::vector< std::vector< V > > R( S.size() );
 ex.bulk( S.size() , [ & ]( std::size_t i ) {
  CR cr( g , -1 );
  DM dm( g );
  PM pm( g , ::lemon::INVALID );
  Heap h( cr );
  D d( g , l );
  d.predMap( pm ).distMap( dm ).heap( h , cr );
  d.run( S[ i ] );
  std::vector< V > r( std::size_t( g.maxNodeId() + 1 ) , infinity< V >() );
  for( typename GR::NodeIt v( g ) ; v != ::lemon::INVALID ; ++v )
   if( d.reached( v ) )
    r[ std::size_t( g.id( v ) ) ] = d.dist( v );
  R[ i ] = std::move( r );
  } );
 return( R );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- sortedArcs -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the pairs ( item , cost ) of the items `Item` of \f$ g \f$, the edges
/// of a graph by default, in the order of the costs of \f$ c \f$, sorted
/// on the threads of \f$ \mathit{ex} \f$, for `lemon::kruskal( g , S , t )`
template< Executor E , class GR , class CM , class Item = typename CM::Key >
[[nodiscard]] std::vector< std::pair< Item , typename CM::Value > >
sortedArcs( E & ex , const GR & g , const CM & c )
{
 using It = typename ::lemon::ItemSetTraits< GR , Item >::ItemIt;
 std::vector< std::pair< Item , typename CM::Value > > S;
 for( It x( g ) ; x != ::lemon::INVALID ; ++x )
  S.emplace_back( x , c[ x ] );
 ADS::sort( ex , S , []( const auto & a , const auto & b ) {
  return( a.second < b.second );
  } );
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::lemon_interop )

/*--------------------------------------------------------------------------*/

#endif /* Parallel.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Parallel.h ---------------------------*/
/*--------------------------------------------------------------------------*/
