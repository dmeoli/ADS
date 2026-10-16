/*--------------------------------------------------------------------------*/
/*--------------------------- File MatrixGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH with an adjacency matrix of section 9.3 [see Graph.h]: the
 * weight of ( u , v ), or nothing, the infinity of the book, in place
 * ( u , v ); Theta( n^2 ) space, hasEdge() in O( 1 ), adj( u ) in
 * Theta( n ), insertNode() and deleteNode() in Theta( n ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.3,
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

#ifndef __ADS_MatrixGraph
 #define __ADS_MatrixGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"

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
/*--------------------------- CLASS MatrixGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph with an adjacency matrix
template< class W = std::monostate , class T = std::monostate >
class MatrixGraph : public GraphNodes< T > {

 using B = GraphNodes< T >;

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights
 using Label = T;    ///< the type of the labels

 /// Graph(): the graph of the nodes 0 , ... , n - 1, with no edge
 explicit MatrixGraph( int n = 0 , bool directed = true )
  : B( n , directed ) , f_M( n , std::vector< std::optional< W > >( n ) ) {}

 /// a new node, a new row and a new column, O( n ) time
 int insertNode( const T & t = T() ) {
  for( auto & r : f_M )
   r.emplace_back();
  f_M.emplace_back( f_M.size() + 1 );
  return( B::addNode( t ) );
  }

 void insertEdge( int u , int v , const W & w = W() ) {
  B::edge( u , v );
  if( ! f_M[ u ][ v ] )
   ++B::f_m;
  f_M[ u ][ v ] = w;
  if( ! B::f_directed )
   f_M[ v ][ u ] = w;
  }

 void deleteEdge( int u , int v ) {
  B::edge( u , v );
  if( f_M[ u ][ v ] )
   --B::f_m;
  f_M[ u ][ v ].reset();
  if( ! B::f_directed )
   f_M[ v ][ u ].reset();
  }

 /// removes the node u and its edges, Theta( n ) time
 void deleteNode( int u ) {
  B::check( u );
  for( int v = 0 ; v < B::bound() ; ++v )
   if( B::f_alive[ v ] && ( v != u ) ) {
    if( f_M[ u ][ v ] ) deleteEdge( u , v );
    if( f_M[ v ][ u ] ) deleteEdge( v , u );
    }
  B::f_alive[ u ] = false;
  --B::f_n;
  }

 /// O( 1 ) time
 [[nodiscard]] bool hasEdge( int u , int v ) const {
  return( B::hasNode( u ) && B::hasNode( v ) && f_M[ u ][ v ] );
  }

 /// the weight of ( u , v ), nothing (the infinity of the book) if there is
 /// no such edge
 [[nodiscard]] std::optional< W > weight( int u , int v ) const {
  if( ! B::hasNode( u ) || ! B::hasNode( v ) )
   return( std::nullopt );
  return( f_M[ u ][ v ] );
  }

 /// the set of the nodes adjacent to u, in increasing order, Theta( n ) time
 [[nodiscard]] std::vector< int > adj( int u ) const {
  std::vector< int > out;
  forEachAdj( u , [ & ]( int v ) { out.push_back( v ); } );
  return( out );
  }

 template< class F >
 void forEachAdj( int u , F && f ) const {
  const auto & r = f_M[ B::check( u ) ];
  for( int v = 0 ; v < B::bound() ; ++v )
   if( r[ v ] )
    f( v );
  }

 template< class F >
 void forEachEdge( int u , F && f ) const {
  const auto & r = f_M[ B::check( u ) ];
  for( int v = 0 ; v < B::bound() ; ++v )
   if( r[ v ] )
    f( v , *r[ v ] );
  }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< std::vector< std::optional< W > > > f_M;   ///< the matrix

 };  // end( class MatrixGraph )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* MatrixGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MatrixGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
