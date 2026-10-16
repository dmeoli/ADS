/*--------------------------------------------------------------------------*/
/*-------------------------- File PointerGraph.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An oriented graph with pointers, beyond the book: each node is an
 * object with its label, a flag for the visits, the number of edges that
 * touch it and the list of its edges, each a pointer to the other end and
 * a weight. A node is reached by its address, which stays valid until it
 * is deleted, and found by label in O( n ). deleteNode() deletes only an
 * isolated node, forceDeleteNode() first deletes the edges that enter or
 * leave it, in O( n + m ); dfs() and bfs() mark the nodes with the flags,
 * cleared at the start of each visit. The graphs of chapter 9 number their
 * nodes from 0 [see ListGraph.h]; this one has neither numbers nor a bound.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 9.2,
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

#ifndef __ADS_PointerGraph
 #define __ADS_PointerGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <list>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PointerGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

/// an oriented graph of node objects linked by pointers
template< class T , class W = int >
class PointerGraph {

/*--------------------------------------------------------------------------*/
 public:

 struct Node;

 /// an edge, to its other end
 struct Edge {
  Node * to;
  W weight;
  };

 /// a node
 struct Node {
  T label;
  int degree = 0;              ///< the edges that enter or leave it
  bool seen = false;
  std::list< Edge > out;
  };

 PointerGraph( void ) = default;
 PointerGraph( const PointerGraph & ) = delete;
 PointerGraph & operator=( const PointerGraph & ) = delete;

 [[nodiscard]] int n( void ) const { return( int( f_V.size() ) ); }

 /// a new isolated node of label t
 Node * insertNode( const T & t ) {
  f_V.push_back( Node{ t , 0 , false , {} } );
  return( &f_V.back() );
  }

 /// the first node of label t, nullptr if none
 [[nodiscard]] Node * find( const T & t ) {
  for( auto & u : f_V ) if( u.label == t ) return( &u );
  return( nullptr );
  }

 [[nodiscard]] bool hasEdge( const Node * u , const Node * v ) const {
  for( const auto & e : u->out ) if( e.to == v ) return( true );
  return( false );
  }

 /// the edge ( u , v ) of weight w, if not there
 void insertEdge( Node * u , Node * v , const W & w = W() ) {
  if( hasEdge( u , v ) ) return;
  u->out.push_back( { v , w } );
  ++u->degree;
  ++v->degree;
  }

 void deleteEdge( Node * u , Node * v ) {
  for( auto it = u->out.begin() ; it != u->out.end() ; ++it )
   if( it->to == v ) {
    u->out.erase( it );
    --u->degree;
    --v->degree;
    return;
    }
  }

 /// deletes u if isolated; false otherwise
 bool deleteNode( Node * u ) {
  if( u->degree > 0 ) return( false );
  f_V.remove_if( [ & ]( const Node & x ) { return( &x == u ); } );
  return( true );
  }

 /// deletes the edges of u and then u
 void forceDeleteNode( Node * u ) {
  for( auto & x : f_V ) deleteEdge( &x , u );
  while( ! u->out.empty() ) deleteEdge( u , u->out.front().to );
  deleteNode( u );
  }

 /// the nodes adjacent to u
 [[nodiscard]] std::vector< Node * > adjacent( const Node * u ) const {
  std::vector< Node * > A;
  for( const auto & e : u->out ) A.push_back( e.to );
  return( A );
  }

 /// the depth first visit from r, f on each node reached
 template< class F >
 void dfs( Node * r , F && f ) {
  clear();
  auto rec = [ & ]( auto & self , Node * u ) -> void {
   u->seen = true;
   f( u );
   for( auto & e : u->out ) if( ! e.to->seen ) self( self , e.to );
   };
  rec( rec , r );
  }

 /// the breadth first visit from r
 template< class F >
 void bfs( Node * r , F && f ) {
  clear();
  std::queue< Node * > Q;
  r->seen = true;
  Q.push( r );
  while( ! Q.empty() ) {
   Node * u = Q.front();
   Q.pop();
   f( u );
   for( auto & e : u->out )
    if( ! e.to->seen ) {
     e.to->seen = true;
     Q.push( e.to );
     }
   }
  }

/*--------------------------------------------------------------------------*/
 private:

 void clear( void ) { for( auto & u : f_V ) u.seen = false; }

 std::list< Node > f_V;   ///< the nodes, at fixed addresses

 };  // end( class PointerGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* PointerGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PointerGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
