/*--------------------------------------------------------------------------*/
/*------------------------------ File Graph.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH of chapter 9 and its realizations. A graph G = ( V , E ) has n
 * nodes, named by integers, and m edges, ordered pairs ( u , v ) if it is
 * oriented and unordered pairs [ u , v ] otherwise; an unoriented graph is
 * the oriented one with both ( u , v ) and ( v , u ) (section 9.2), and so
 * adj( u ) gives v for each edge [ u , v ]. The edges may have a weight of
 * type W and the nodes a label of type T, both std::monostate if absent;
 * there are no loops ( u , u ), so \f$ 0 \leq m \leq n ( n - 1 ) \f$.
 *
 * The nodes of a graph are the integers in [ 0 , bound() ): insertNode()
 * gives the next one and deleteNode() leaves its name unused, so that the
 * vectors indexed by the nodes of the visits have bound() places, and n()
 * = |V()| is the G.n of the book. The concept Graph asks for V(), adj( u ),
 * which gives the adjacent nodes in a vector, as the SET of the book, and
 * forEachAdj( u , f ), which calls f on them without building it.
 *
 * incidence() gives the n x m matrix B of incidence nodes-edges (section
 * 9.3): \f$ b_{uk} = -1 \f$ if the edge k leaves u and +1 if it enters it,
 * for an oriented graph, and 1 if the edge k touches u, for an unoriented
 * one.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 9.1-9.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf

 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Graph
 #define __ADS_Graph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// GRAPH (section 9.2), as far as the visits need it: the nodes, the
/// adjacent nodes of a node, the range [ 0 , bound() ) of the names
template< class G >
concept Graph = requires( const G g , int u ) {
 { g.n() } -> std::convertible_to< int >;
 { g.bound() } -> std::convertible_to< int >;
 { g.directed() } -> std::same_as< bool >;
 { g.hasNode( u ) } -> std::same_as< bool >;
 { g.V() } -> std::same_as< std::vector< int > >;
 { g.adj( u ) } -> std::same_as< std::vector< int > >;
 g.forEachAdj( u , []( int ) {} );
 };

/// a GRAPH with the insertions and the deletions of its specification
template< class G >
concept DynamicGraph = Graph< G > && requires( G g , int u , int v ) {
 { g.insertNode() } -> std::same_as< int >;
 g.insertEdge( u , v );
 g.deleteNode( u );
 g.deleteEdge( u , v );
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS GraphNodes ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the nodes, their labels and the counts shared by the realizations
template< class T >
class GraphNodes {

/*--------------------------------------------------------------------------*/
 public:

 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 /// the number of nodes, the G.n of the book
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the number of edges, an unoriented one counted once
 [[nodiscard]] int m( void ) const { return( f_m ); }

 /// the names of the nodes are in [ 0 , bound() )
 [[nodiscard]] int bound( void ) const { return( int( f_alive.size() ) ); }

 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < bound() ) && f_alive[ u ] );
  }

 /// the set of the nodes, in increasing order
 [[nodiscard]] std::vector< int > V( void ) const {
  std::vector< int > out;
  for( int u = 0 ; u < bound() ; ++u )
   if( f_alive[ u ] )
    out.push_back( u );
  return( out );
  }

 /// the label of the node u
 [[nodiscard]] const T & label( int u ) const {
  return( f_label[ check( u ) ] );
  }

 void setLabel( int u , const T & t ) { f_label[ check( u ) ] = t; }

/*--------------------------------------------------------------------------*/
 protected:

 GraphNodes( int n , bool directed ) : f_directed( directed ) {
  if( n < 0 )
   throw( std::invalid_argument( "ADS::Graph: n < 0" ) );
  f_alive.assign( n , true );
  f_label.resize( n );
  f_n = n;
  }

 int addNode( const T & t ) {
  f_alive.push_back( true );
  f_label.push_back( t );
  ++f_n;
  return( bound() - 1 );
  }

 int check( int u ) const {
  if( ! hasNode( u ) )
   throw( std::out_of_range( "ADS::Graph: no node " + std::to_string( u ) ) );
  return( u );
  }

 void edge( int u , int v ) const {
  check( u );
  check( v );
  if( u == v )
   throw( std::invalid_argument( "ADS::Graph: a loop ( u , u )" ) );
  }

 bool f_directed;              ///< oriented or not
 std::vector< bool > f_alive;  ///< whether a name is a node
 std::vector< T > f_label;     ///< the labels
 int f_n = 0;                  ///< the number of nodes
 int f_m = 0;                  ///< the number of edges

 };  // end( class GraphNodes )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the matrix of incidence nodes-edges, the edges in the order of V() and
/// of adj(), an unoriented one when met from its smaller end
template< Graph G >
std::vector< std::vector< int > > incidence( const G & g )
{
 std::vector< std::pair< int , int > > E;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   if( g.directed() || ( u < v ) )
    E.emplace_back( u , v );
   } );
 std::vector< std::vector< int > > B( g.bound() ,
                                      std::vector< int >( E.size() ) );
 for( std::size_t k = 0 ; k < E.size() ; ++k ) {
  B[ E[ k ].first ][ k ] = g.directed() ? -1 : 1;
  B[ E[ k ].second ][ k ] = 1;
  }
 return( B );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Graph.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Graph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
