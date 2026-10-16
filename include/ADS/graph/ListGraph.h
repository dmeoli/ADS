/*--------------------------------------------------------------------------*/
/*---------------------------- File ListGraph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The GRAPH with lists of adjacency of section 9.4 [see Graph.h]: for each
 * node the LIST of its adjacent nodes and of the weights of the edges,
 * Theta( n + m ) space, adj( u ) in Theta( |adj( u )| ), hasEdge( u , v )
 * in O( |adj( u )| ), all the edges in Theta( n + m ), deleteNode() in
 * O( n + m ).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.4,
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

#ifndef __ADS_ListGraph
 #define __ADS_ListGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/Graph.h"
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
/*---------------------------- CLASS ListGraph -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a graph with lists of adjacency
template< class W = std::monostate , class T = std::monostate >
class ListGraph : public GraphNodes< T > {

 using B = GraphNodes< T >;

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights
 using Label = T;    ///< the type of the labels

 /// Graph(): the graph of the nodes 0 , ... , n - 1, with no edge
 explicit ListGraph( int n = 0 , bool directed = true )
  : B( n , directed ) , f_adj( n ) {}

 /// a new node, with no edge, and its name
 int insertNode( const T & t = T() ) {
  f_adj.emplace_back();
  return( B::addNode( t ) );
  }

 /// the edge ( u , v ), or [ u , v ], of weight w; only the weight changes
 /// if it is there
 void insertEdge( int u , int v , const W & w = W() ) {
  B::edge( u , v );
  if( ! set( u , v , w ) ) {
   f_adj[ u ].insert( f_adj[ u ].next( f_adj[ u ].tail() ) , { v , w } );
   if( ! B::f_directed )
    f_adj[ v ].insert( f_adj[ v ].next( f_adj[ v ].tail() ) , { u , w } );
   ++B::f_m;
   }
  else if( ! B::f_directed )
   set( v , u , w );
  }

 /// removes the edge ( u , v ), or [ u , v ], if it is there
 void deleteEdge( int u , int v ) {
  B::edge( u , v );
  if( erase( u , v ) ) {
   if( ! B::f_directed )
    erase( v , u );
   --B::f_m;
   }
  }

 /// removes the node u and its edges, O( n + m ) time
 void deleteNode( int u ) {
  B::check( u );
  for( int v = 0 ; v < B::bound() ; ++v )
   if( B::f_alive[ v ] && ( v != u ) && erase( v , u ) && B::f_directed )
    --B::f_m;
  for( auto p = f_adj[ u ].head() ; ! f_adj[ u ].finished( p ) ;
       p = f_adj[ u ].remove( p ) )
   --B::f_m;
  B::f_alive[ u ] = false;
  --B::f_n;
  }

 [[nodiscard]] bool hasEdge( int u , int v ) const {
  return( B::hasNode( u ) && B::hasNode( v ) && find( u , v ) );
  }

 /// the weight of ( u , v ), nothing if there is no such edge
 [[nodiscard]] std::optional< W > weight( int u , int v ) const {
  if( ! B::hasNode( u ) )
   return( std::nullopt );
  const auto & L = f_adj[ u ];
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   if( L.read( p ).first == v )
    return( L.read( p ).second );
  return( std::nullopt );
  }

 /// the set of the nodes adjacent to u, in the order of insertion
 [[nodiscard]] std::vector< int > adj( int u ) const {
  std::vector< int > out;
  forEachAdj( u , [ & ]( int v ) { out.push_back( v ); } );
  return( out );
  }

 /// f( v ) on the nodes adjacent to u
 template< class F >
 void forEachAdj( int u , F && f ) const {
  const auto & L = f_adj[ B::check( u ) ];
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   f( L.read( p ).first );
  }

 /// f( v , w ) on the nodes adjacent to u and the weights of the edges
 template< class F >
 void forEachEdge( int u , F && f ) const {
  const auto & L = f_adj[ B::check( u ) ];
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   f( L.read( p ).first , L.read( p ).second );
  }

/*--------------------------------------------------------------------------*/
 private:

 bool find( int u , int v ) const { return( weight( u , v ).has_value() ); }

 bool set( int u , int v , const W & w ) {
  auto & L = f_adj[ u ];
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   if( L.read( p ).first == v ) {
    L.write( p , { v , w } );
    return( true );
    }
  return( false );
  }

 bool erase( int u , int v ) {
  auto & L = f_adj[ u ];
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   if( L.read( p ).first == v ) {
    L.remove( p );
    return( true );
    }
  return( false );
  }

 std::vector< List< std::pair< int , W > > > f_adj;   ///< the lists

 };  // end( class ListGraph )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ListGraph.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ListGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
