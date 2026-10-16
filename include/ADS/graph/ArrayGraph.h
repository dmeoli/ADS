/*--------------------------------------------------------------------------*/
/*--------------------------- File ArrayGraph.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The static GRAPH with vectors of adjacency of section 9.4 [see Graph.h]:
 * the adjacent nodes of all the nodes in one vector, the ones of u in the
 * places [ off( u ) , off( u + 1 ) ); no insertion nor deletion,
 * Theta( n + m ) space and no pointer.
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

#ifndef __ADS_ArrayGraph
 #define __ADS_ArrayGraph
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
/*---------------------------- CLASS ArrayGraph ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a static graph with vectors of adjacency, all in one vector
template< class W = std::monostate >
class ArrayGraph {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the weights

 /// the graph of the nodes 0 , ... , n - 1 and of the given edges; an
 /// unoriented edge [ u , v ] is given once
 ArrayGraph( int n , const std::vector< std::tuple< int , int , W > > & E ,
             bool directed = true ) : f_directed( directed ) {
  if( n < 0 )
   throw( std::invalid_argument( "ADS::ArrayGraph: n < 0" ) );
  f_off.assign( n + 1 , 0 );
  for( const auto & [ u , v , w ] : E ) {
   if( ( u < 0 ) || ( u >= n ) || ( v < 0 ) || ( v >= n ) || ( u == v ) )
    throw( std::invalid_argument( "ADS::ArrayGraph: a wrong edge" ) );
   ++f_off[ u + 1 ];
   if( ! directed )
    ++f_off[ v + 1 ];
   }
  for( int u = 0 ; u < n ; ++u )
   f_off[ u + 1 ] += f_off[ u ];
  f_to.resize( f_off[ n ] );
  f_w.resize( f_off[ n ] );
  std::vector< int > next( f_off.begin() , f_off.end() - 1 );
  for( const auto & [ u , v , w ] : E ) {
   f_to[ next[ u ] ] = v;
   f_w[ next[ u ]++ ] = w;
   if( ! directed ) {
    f_to[ next[ v ] ] = u;
    f_w[ next[ v ]++ ] = w;
    }
   }
  f_m = int( E.size() );
  }

 /// the copy of any graph whose nodes are 0 , ... , n - 1
 template< Graph G >
 explicit ArrayGraph( const G & g )
  : ArrayGraph( g.bound() , edges( g ) , g.directed() ) {}

 [[nodiscard]] bool directed( void ) const { return( f_directed ); }

 [[nodiscard]] int n( void ) const { return( int( f_off.size() ) - 1 ); }

 [[nodiscard]] int m( void ) const { return( f_m ); }

 [[nodiscard]] int bound( void ) const { return( n() ); }

 [[nodiscard]] bool hasNode( int u ) const {
  return( ( u >= 0 ) && ( u < n() ) );
  }

 [[nodiscard]] std::vector< int > V( void ) const {
  std::vector< int > out( n() );
  for( int u = 0 ; u < n() ; ++u )
   out[ u ] = u;
  return( out );
  }

 [[nodiscard]] std::vector< int > adj( int u ) const {
  return( std::vector< int >( f_to.begin() + f_off[ u ] ,
                              f_to.begin() + f_off[ u + 1 ] ) );
  }

 template< class F >
 void forEachAdj( int u , F && f ) const {
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
   f( f_to[ k ] );
  }

 template< class F >
 void forEachEdge( int u , F && f ) const {
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
   f( f_to[ k ] , f_w[ k ] );
  }

/*--------------------------------------------------------------------------*/
 private:

 template< class G >
 static std::vector< std::tuple< int , int , W > > edges( const G & g ) {
  std::vector< std::tuple< int , int , W > > E;
  for( int u : g.V() )
   g.forEachEdge( u , [ & ]( int v , const auto & w ) {
    if( g.directed() || ( u < v ) )
     E.emplace_back( u , v , W( w ) );
    } );
  return( E );
  }

 bool f_directed;            ///< oriented or not
 std::vector< int > f_off;   ///< the adjacent nodes of u from f_off[ u ]
 std::vector< int > f_to;    ///< the adjacent nodes
 std::vector< W > f_w;       ///< the weights
 int f_m = 0;                ///< the number of edges

 };  // end( class ArrayGraph )

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ArrayGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ArrayGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
