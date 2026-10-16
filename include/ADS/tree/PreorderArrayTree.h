/*--------------------------------------------------------------------------*/
/*------------------------ File PreorderArrayTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An ordered tree in a vector in preorder, beyond the book: the place i
 * holds the value of the i-th node of the previsit and the place of its
 * parent, NIL for the root in the place 0. The subtree of a node is the
 * stretch of the vector that starts with it and ends before the first
 * place whose parent precedes it, so that the first child of i, if any, is
 * i + 1, and the next sibling is the place after its subtree. Reading and
 * writing take O( 1 ) time, moving along the tree and inserting or removing
 * a subtree O( n ), since the places after it shift and their parents are
 * renumbered. It extends the vector of parents of section 5.4 [see
 * ParentTree.h] with the values and the order of the children.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 5.4,
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

#ifndef __ADS_PreorderArrayTree
 #define __ADS_PreorderArrayTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <stdexcept>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS PreorderArrayTree -------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree in a vector of ( parent , value ) in preorder
template< class T >
class PreorderArrayTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;              ///< the type of the values
 static constexpr int NIL = -1;

 PreorderArrayTree( void ) = default;

 /// a tree of one node of value v
 explicit PreorderArrayTree( const T & v ) : f_A( 1 , { NIL , v } ) {}

 [[nodiscard]] bool isEmpty( void ) const { return( f_A.empty() ); }

 [[nodiscard]] int size( void ) const { return( int( f_A.size() ) ); }

 [[nodiscard]] int root( void ) const { return( f_A.empty() ? NIL : 0 ); }

 [[nodiscard]] int parent( int i ) const { return( cell( i ).parent ); }

 [[nodiscard]] const T & read( int i ) const { return( cell( i ).value ); }

 void write( int i , const T & v ) { f_A.at( i ).value = v; }

 /// the first child, i + 1 if its parent is i
 [[nodiscard]] int leftmostChild( int i ) const {
  cell( i );
  return( ( i + 1 < size() ) && ( f_A[ i + 1 ].parent == i ) ? i + 1 : NIL );
  }

 /// the next sibling, the place after the subtree of i if it has the same
 /// parent
 [[nodiscard]] int rightSibling( int i ) const {
  const int e = end( i );
  return( ( e < size() ) && ( f_A[ e ].parent == f_A[ i ].parent ) &&
          ( i > 0 ) ? e : NIL );
  }

 [[nodiscard]] bool isLeaf( int i ) const {
  return( leftmostChild( i ) == NIL );
  }

 /// the place after the subtree of i
 [[nodiscard]] int end( int i ) const {
  cell( i );
  int e = i + 1;
  while( ( e < size() ) && ( f_A[ e ].parent >= i ) ) ++e;
  return( e );
  }

 /// a copy of t as the first child of i
 void insertChild( int i , const PreorderArrayTree & t ) {
  cell( i );
  splice( i + 1 , i , t );
  }

 /// a copy of t as the next sibling of i, not the root
 void insertSibling( int i , const PreorderArrayTree & t ) {
  if( cell( i ).parent == NIL )
   throw( std::logic_error(
                "ADS::PreorderArrayTree::insertSibling: the root" ) );
  splice( end( i ) , f_A[ i ].parent , t );
  }

 /// removes the subtree of i
 void remove( int i ) {
  const int e = end( i ) , k = e - i;
  f_A.erase( f_A.begin() + i , f_A.begin() + e );
  for( int j = i ; j < size() ; ++j )
   if( f_A[ j ].parent >= e ) f_A[ j ].parent -= k;
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  int parent;
  T value;
  };

 const Cell & cell( int i ) const {
  if( ( i < 0 ) || ( i >= size() ) )
   throw( std::out_of_range( "ADS::PreorderArrayTree: no node" ) );
  return( f_A[ i ] );
  }

 /// the cells of t to the place at, its root a child of p: the places
 /// from at on shift right by the size of t
 void splice( int at , int p , const PreorderArrayTree & t ) {
  const int k = t.size();
  if( k == 0 ) return;
  for( int j = at ; j < size() ; ++j )
   if( f_A[ j ].parent >= at ) f_A[ j ].parent += k;
  std::vector< Cell > C( t.f_A );
  C[ 0 ].parent = p;
  for( int j = 1 ; j < k ; ++j ) C[ j ].parent += at;
  f_A.insert( f_A.begin() + at , C.begin() , C.end() );
  }

 std::vector< Cell > f_A;   ///< the nodes in preorder

 };  // end( class PreorderArrayTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* PreorderArrayTree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File PreorderArrayTree.h ----------------------*/
/*--------------------------------------------------------------------------*/
