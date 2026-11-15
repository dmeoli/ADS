/*--------------------------------------------------------------------------*/
/*----------------------------- File Dijkstra.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The priority queues of ADS inside the visits of Boost.Graph, beyond the
 * book, compiled only with `ADS_WITH_BOOST`: `UpdatableQueue` gives a queue
 * of `PriorityQueue.h` the interface of the buffer that
 * `boost::breadth_first_visit()` takes (`push()`, `top()`, `pop()`,
 * `empty()`, and the `update()` of the updatable queue of Dijkstra's
 * algorithm), and `dijkstraShortestPaths()` is the algorithm of Dijkstra of
 * Boost.Graph on such a queue: the visit of Boost.Graph with a visitor that
 * relaxes the arcs, as `boost::dijkstra_shortest_paths()` does on its own
 * 4-ary heap, which that function does not let a caller replace.
 *
 * The queue is reached through `PriorityFrontier` of `ShortestPaths.h`, so
 * that every queue that the shortest paths of ADS take serves here as
 * well, with the same rule: the priority of a vertex is its distance, or
 * the pair ( distance , vertex ), which breaks the ties by the names; the
 * fall of a distance is `queue_decrease()` on a queue that lowers its keys
 * natively, a new insertion on the others, whose stale copies are skipped.
 * The vertices are the integers \f$ [ 0 , \mathrm{num\_vertices}( g ) ) \f$
 * of the vertex index, as in the graphs of ADS [see `Graph.h`],
 * `boost::adjacency_list` on `vecS` and `compressed_sparse_row_graph`.
 *
 * The contracts [see `Contract.h`]: the weights are not negative, which the
 * visit does not check (as in Boost.Graph); the queue is empty.
 *
 * \see E. W. Dijkstra, A note on two problems in connexion with graphs,
 *      Numerische Mathematik 1, 1959, 269-271, doi:10.1007/BF01386390
 * \see J. G. Siek, L.-Q. Lee, A. Lumsdaine, The Boost Graph Library: User
 *      Guide and Reference Manual, Addison-Wesley, 2002
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

#ifndef __ADS_interop_boost_Dijkstra
 #define __ADS_interop_boost_Dijkstra
/* self-identification: #endif constant */

#if defined( ADS_WITH_BOOST )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/ShortestPaths.h"

#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>

#include <limits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::bgl
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS UpdatableQueue --------------------------*/
/*--------------------------------------------------------------------------*/

/// the queue \f$ q \f$ of ADS as the updatable buffer of the visits of
/// Boost.Graph: the vertices in the order of their distances in the map
/// \f$ d \f$, read when a vertex is pushed or updated
template< FrontierQueue Q , class DistanceMap >
class UpdatableQueue
{

 public:
 using value_type = int;  ///< the vertices

 /// the buffer on the empty queue \f$ q \f$, for the vertices
 /// \f$ [ 0 , n ) \f$
 /// \pre \f$ q \f$ is empty, \f$ n \ge 0 \f$
 UpdatableQueue( Q & q , int n , DistanceMap d ) : f_S( q , n ) , f_d( d ) { }

 /// \f$ v \f$ in, at its distance
 void push( int v ) { f_S.add( v , get( f_d , v ) , true ); }

 /// the distance of \f$ v \f$, in the buffer, has fallen
 void update( int v ) { f_S.update( v , get( f_d , v ) ); }

 /// the vertex of least distance
 /// \pre the buffer is not empty
 int top( void ) {
  if( f_top < 0 )
   f_top = f_S.extract();
  return( f_top );
  }

 /// the vertex of least distance removed
 /// \pre the buffer is not empty
 void pop( void ) {
  ( void )top();
  f_top = -1;
  }

 /// whether the buffer is empty
 [[nodiscard]] bool empty( void ) const {
  return( ( f_top < 0 ) && f_S.empty() );
  }

 private:
 PriorityFrontier< Q > f_S;  ///< the queue, by its customization points
 DistanceMap f_d;            ///< the distances
 int f_top = -1;             ///< the vertex read by top(), not yet popped

 };  // end( class UpdatableQueue )

/*--------------------------------------------------------------------------*/
/*----------------------------- THE ALGORITHM ------------------------------*/
/*--------------------------------------------------------------------------*/

/// \cond
namespace detail
{

/// the visitor of the visit of Boost.Graph that makes it Dijkstra's
/// algorithm: an arc that lowers the distance of its head relaxes it, and
/// the head is updated in the queue if it is already there
template< class B , class WeightMap , class PredecessorMap ,
	  class DistanceMap >
struct RelaxVisitor : boost::default_bfs_visitor
{
 B * queue;         ///< the buffer of the visit
 WeightMap w;       ///< the weights
 PredecessorMap p;  ///< the parents
 DistanceMap d;     ///< the distances

 /// whether \f$ e \f$ lowers the distance of its head, then lowered

 template< class E , class G >
 bool relax( const E & e , const G & g ) const {
  const auto u = source( e , g ) , v = target( e , g );
  const auto du = get( d , u ) + get( w , e );
  if( du < get( d , v ) ) {
   put( d , v , du );
   put( p , v , u );
   return( true );
   }
  return( false );
  }

 /// an arc to a white vertex, which the visit then pushes
 template< class E , class G >
 void tree_edge( const E & e , const G & g ) const {
  ( void )relax( e , g );
  }

 /// an arc to a vertex in the queue
 template< class E , class G >
 void gray_target( const E & e , const G & g ) const {
  if( relax( e , g ) )
   queue->update( int( target( e , g ) ) );
  }
 };

}  // end( namespace detail )
/// \endcond

/// the distances from \f$ s \f$ in \f$ g \f$, in the map \f$ d \f$, and the
/// tree of the shortest paths in \f$ p \f$, by the visit of Boost.Graph on
/// the queue \f$ q \f$ of ADS, the weights read from the map \f$ w \f$: the
/// distances of the vertices not reached are left at
/// `std::numeric_limits< D >::max()` and their parents at themselves, as in
/// `boost::dijkstra_shortest_paths()`
///
/// In pseudocode (the prototype of section 11.2 on a priority queue), the
/// visit of Boost.Graph, white the vertices not reached, gray those in
/// \f$ Q \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{dijkstraShortestPaths}( g , s , q ) \cr
/// 2 & \quad d[ v ] \gets \infty ,\ p[ v ] \gets v \text{ for every }
///     v ;\ d[ s ] \gets 0 ;\ \mathrm{push}( Q , s ) \cr
/// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
/// 4 & \qquad u \gets \mathrm{top}( Q ) ;\ \mathrm{pop}( Q ) \cr
/// 5 & \qquad \textbf{foreach}\ ( u , v ) \textbf{ with }
///     d[ u ] + w( u , v ) < d[ v ]\ \textbf{do} \cr
/// 6 & \qquad \quad d[ v ] \gets d[ u ] + w( u , v ) ;\ p[ v ] \gets u \cr
/// 7 & \qquad \quad \textbf{if}\ v \text{ white}\ \textbf{then}
///     \ \mathrm{push}( Q , v )\ \textbf{else if}\ v \text{ gray}
///     \ \textbf{then}\ \mathrm{update}( Q , v )
/// \end{array}}
/// \f]
/// \pre \f$ s \f$ is a vertex, the weights are not negative, \f$ q \f$ is
///      empty
template< class G , FrontierQueue Q , class WeightMap , class PredecessorMap ,
	  class DistanceMap >
void dijkstraShortestPaths( const G & g , int s , Q & q , WeightMap w ,
			    PredecessorMap p , DistanceMap d )
{
 using D = typename boost::property_traits< DistanceMap >::value_type;
 const int n = int( num_vertices( g ) );
 ADS_PRE( ( s >= 0 ) && ( s < n ) && q.empty() );
 for( int v = 0 ; v < n ; ++v ) {
  put( d , v , std::numeric_limits< D >::max() );
  put( p , v , v );
  }
 put( d , s , D( 0 ) );
 using B = UpdatableQueue< Q , DistanceMap >;
 B buffer( q , n , d );
 std::vector< boost::default_color_type > color( std::size_t( n ) ,
						 boost::white_color );
 detail::RelaxVisitor< B , WeightMap , PredecessorMap , DistanceMap > vis;
 vis.queue = &buffer;
 vis.w = w;
 vis.p = p;
 vis.d = d;
 boost::breadth_first_visit(  // not color.data(), which another header of
   g , s , buffer , vis ,         // Boost.Graph may hide
   boost::make_iterator_property_map(
     color.data() , boost::typed_identity_property_map< int >() ) );
 }

/// `dijkstraShortestPaths()` above with the weights `get( edge_weight , g )`
/// of the graph
template< class G , FrontierQueue Q , class PredecessorMap ,
	  class DistanceMap >
void dijkstraShortestPaths( const G & g , int s , Q & q , PredecessorMap p ,
			    DistanceMap d )
{
 dijkstraShortestPaths( g , s , q , get( boost::edge_weight , g ) , p , d );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::bgl )

#endif /* ADS_WITH_BOOST */

/*--------------------------------------------------------------------------*/

#endif /* Dijkstra.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Dijkstra.h --------------------------*/
/*--------------------------------------------------------------------------*/
