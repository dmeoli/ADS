/*--------------------------------------------------------------------------*/
/*------------------------- File ArrayBinaryTree.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A binary tree in a positional vector, beyond the book: the root in
 * the place 0, the children of the place i in 2 i + 1 and 2 i + 2, its
 * parent in \f$ \lfloor ( i - 1 ) / 2 \rfloor \f$ [see dary_child()], an
 * empty place where no node is. Moving along the tree, reading and writing
 * take O( 1 ) time; the vector grows to the deepest place used, so that a
 * tree of height h takes \f$ O( 2^h ) \f$ space, which suits the trees
 * nearly complete. insertLeft() and insertRight() copy a whole tree under
 * a node and remove() empties a subtree, in time linear in the places of
 * the subtree. A node is a place, NIL if missing.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
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

#ifndef __ADS_ArrayBinaryTree
 #define __ADS_ArrayBinaryTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/BinaryTree.h"
#include "ADS/queue/ArrayQueue.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ArrayBinaryTree --------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary tree in a positional vector
template< class T >
class ArrayBinaryTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr long NIL = -1;

 /// the empty tree
 ArrayBinaryTree( void ) = default;

 /// a tree of one node of value v
 explicit ArrayBinaryTree( const T & v ) : f_A( 1 , v ) {}

 /// a new root of value v with copies of l and r as its subtrees
 ArrayBinaryTree( const T & v , const ArrayBinaryTree & l ,
                  const ArrayBinaryTree & r ) : ArrayBinaryTree( v ) {
  if( ! l.isEmpty() ) insertLeft( 0 , l );
  if( ! r.isEmpty() ) insertRight( 0 , r );
  }

 [[nodiscard]] bool isEmpty( void ) const { return( ! has( 0 ) ); }

 /// the root, NIL if the tree is empty
 [[nodiscard]] long root( void ) const { return( has( 0 ) ? 0 : NIL ); }

 [[nodiscard]] long parent( long i ) const {
  return( i > 0 ? dary_parent( i , 2 ) : NIL );
  }

 [[nodiscard]] long left( long i ) const {
  return( has( 2 * i + 1 ) ? 2 * i + 1 : NIL );
  }

 [[nodiscard]] long right( long i ) const {
  return( has( 2 * i + 2 ) ? 2 * i + 2 : NIL );
  }

 [[nodiscard]] bool isLeaf( long i ) const {
  return( ( left( i ) == NIL ) && ( right( i ) == NIL ) );
  }

 [[nodiscard]] const T & read( long i ) const { return( *at( i ) ); }

 void write( long i , const T & v ) { *at( i ) = v; }

 /// the root of value v of the empty tree
 void setRoot( const T & v ) {
  if( ! isEmpty() )
   throw( std::logic_error( "ADS::ArrayBinaryTree::setRoot: not empty" ) );
  place( 0 ) = v;
  }

 /// a copy of t as the left subtree of i, which must be missing
 void insertLeft( long i , const ArrayBinaryTree & t ) {
  graft( i , 2 * i + 1 , t , "insertLeft" );
  }

 /// a copy of t as the right subtree of i, which must be missing
 void insertRight( long i , const ArrayBinaryTree & t ) {
  graft( i , 2 * i + 2 , t , "insertRight" );
  }

 /// empties the subtree of i
 void remove( long i ) {
  if( ! has( i ) ) return;
  f_A[ i ].reset();
  remove( 2 * i + 1 );
  remove( 2 * i + 2 );
  }

 /// the places of the nodes in preorder
 [[nodiscard]] std::vector< long > preorder( void ) const {
  std::vector< long > P;
  auto rec = [ & ]( auto & self , long i ) -> void {
   if( ! has( i ) ) return;
   P.push_back( i );
   self( self , 2 * i + 1 );
   self( self , 2 * i + 2 );
   };
  rec( rec , 0 );
  return( P );
  }

/*--------------------------------------------------------------------------*/
 private:

 [[nodiscard]] bool has( long i ) const {
  return( ( i >= 0 ) && ( i < long( f_A.size() ) ) && f_A[ i ].has_value() );
  }

 std::optional< T > & place( long i ) {
  if( i >= long( f_A.size() ) ) f_A.resize( i + 1 );
  return( f_A[ i ] );
  }

 const std::optional< T > & at( long i ) const {
  if( ! has( i ) )
   throw( std::out_of_range( "ADS::ArrayBinaryTree: no node" ) );
  return( f_A[ i ] );
  }

 std::optional< T > & at( long i ) {
  if( ! has( i ) )
   throw( std::out_of_range( "ADS::ArrayBinaryTree: no node" ) );
  return( f_A[ i ] );
  }

 /// the place j of t to the place k of this tree, and so on down
 void copy( long k , const ArrayBinaryTree & t , long j ) {
  if( ! t.has( j ) ) return;
  place( k ) = t.f_A[ j ];
  copy( 2 * k + 1 , t , 2 * j + 1 );
  copy( 2 * k + 2 , t , 2 * j + 2 );
  }

 void graft( long i , long c , const ArrayBinaryTree & t , const char * who ) {
  if( ! has( i ) || has( c ) )
   throw( std::logic_error( std::string( "ADS::ArrayBinaryTree::" ) + who +
                            ": no node or child present" ) );
  copy( c , t , 0 );
  }

 std::vector< std::optional< T > > f_A;   ///< the places

 };  // end( class ArrayBinaryTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ArrayBinaryTree.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File ArrayBinaryTree.h -----------------------*/
/*--------------------------------------------------------------------------*/
