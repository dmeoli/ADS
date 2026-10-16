/*--------------------------------------------------------------------------*/
/*----------------------------- File ch05.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 5, Alberi: the specification of the ordered and of the
 * binary tree, the four visits on the classification of the plants
 * (Figura 5.1) against the orders the book gives, the vector of parents
 * (Figura 5.4), the binary representation of an ordered tree (Figura 5.9),
 * the tree of the choices for sorting three numbers (Figura 5.8), the
 * complete d-ary trees in a vector and the exercises 5.1 to 5.13, each a
 * test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/ParentTree.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <random>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Strings = std::vector< std::string >;
using Ints = std::vector< int >;

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// an ordered tree to build: a value and the subtrees of the children
template< class T >
struct Spec {
 T v;
 std::vector< Spec > c;
 };

/// builds the tree of a Spec; insertChild() puts a child first, hence the
/// children are inserted from the last one
template< class T >
Tree< T > * make( const Spec< T > & s )
{
 auto t = new Tree< T >( s.v );
 for( auto it = s.c.rbegin() ; it != s.c.rend() ; ++it )
  t->insertChild( make( *it ) );
 return( t );
 }

/// the values of an ordered tree in the order of a visit
template< class T , class Visit >
std::vector< T > order( Tree< T > * t , Visit visit )
{
 std::vector< T > out;
 visit( t , [ & ]( Tree< T > * u ) { out.push_back( u->read() ); } );
 return( out );
 }

/// a binary tree to build: a value and the two subtrees, possibly empty
template< class T >
struct BSpec {
 T v;
 std::shared_ptr< BSpec > l , r;
 };

template< class T >
std::shared_ptr< BSpec< T > > B( T v , std::shared_ptr< BSpec< T > > l = {} ,
                                 std::shared_ptr< BSpec< T > > r = {} )
{
 return( std::make_shared< BSpec< T > >( BSpec< T >{ v , l , r } ) );
 }

template< class T >
BinaryTree< T > * make( const std::shared_ptr< BSpec< T > > & s )
{
 if( ! s )
  return( nullptr );
 auto t = new BinaryTree< T >( s->v );
 if( s->l ) t->insertLeft( make( s->l ) );
 if( s->r ) t->insertRight( make( s->r ) );
 return( t );
 }

/// the values of a binary tree in the order of a visit
template< class T , class Visit >
std::vector< T > border( BinaryTree< T > * t , Visit visit )
{
 std::vector< T > out;
 visit( t , [ & ]( BinaryTree< T > * u ) { out.push_back( u->read() ); } );
 return( out );
 }

template< class T >
auto pre_b = []( BinaryTree< T > * t , auto f ) { pre_visit( t , f ); };
template< class T >
auto in_b = []( BinaryTree< T > * t , auto f ) { in_visit( t , f ); };
template< class T >
auto post_b = []( BinaryTree< T > * t , auto f ) { post_visit( t , f ); };

/// a random binary tree of n nodes, valued 0 , ... , n - 1 in previsit
BinaryTree< int > * random_binary( int n , std::mt19937 & rng , int & next )
{
 if( n == 0 )
  return( nullptr );
 auto t = new BinaryTree< int >( next++ );
 const int l = std::uniform_int_distribution< int >( 0 , n - 1 )( rng );
 if( auto a = random_binary( l , rng , next ) ) t->insertLeft( a );
 if( auto b = random_binary( n - 1 - l , rng , next ) ) t->insertRight( b );
 return( t );
 }

/// Figura 5.1: the classification of the plants
Spec< std::string > plants( void )
{
 using S = Spec< std::string >;
 return( S{ "Regno vegetale" , {
  S{ "Crittogame" , {
   S{ "Tallofite" , { S{ "Cianoficee" , {} } , S{ "Batteri" , {} } ,
                      S{ "Alghe" , {} } , S{ "Mixomiceti" , {} } ,
                      S{ "Funghi" , {} } } } ,
   S{ "Archegoniate" , { S{ "Briofite" , {} } , S{ "Pteridofite" , {} } } }
   } } ,
  S{ "Fanerogame" , {
   S{ "Gimnosperme" , { S{ "Cicadali" , {} } , S{ "Ginkgoali" , {} } ,
                        S{ "Coniferali" , {} } , S{ "Gnetali" , {} } } } ,
   S{ "Angiosperme" , { S{ "Dicotiledoni" , {} } ,
                        S{ "Monocotiledoni" , {} } } }
   } } } } );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*------------------------- THE ORDERED TREE -------------------------------*/
/*--------------------------------------------------------------------------*/

// sections 5.1 and 5.3, Figura 5.5: insertChild() puts a subtree first,
// insertSibling() after a node, deleteChild() destroys the first subtree;
// a tree that is not a root cannot be inserted, nor a sibling of a root
ADS_TEST( "spec.Tree" )
{
 std::unique_ptr< Tree< char > > r( make( Spec< char >{ 'r' , {
  Spec< char >{ 'a' , { { 'c' , {} } , { 'd' , {} } } } ,
  Spec< char >{ 'b' , {} } } } ) );
 auto pre = [ & ]( Tree< char > * t ) {
  return( order( t , []( Tree< char > * u , auto f ) {
   pre_visit( u , f ); } ) );
  };
 CHECK( pre( r.get() ) == std::vector< char >( { 'r' , 'a' , 'c' , 'd' ,
                                                 'b' } ) );
 auto a = r->leftmostChild();
 CHECK( a->read() == 'a' && a->parent() == r.get() );
 CHECK( a->rightSibling()->read() == 'b' );
 CHECK( ! r->parent() && ! a->rightSibling()->leftmostChild() );

 a->insertChild( new Tree< char >( 'z' ) );                // first child
 CHECK( a->leftmostChild()->read() == 'z' );
 auto c = a->leftmostChild()->rightSibling();
 c->insertSibling( new Tree< char >( 'x' ) );              // after c
 CHECK( pre( r.get() ) == std::vector< char >( { 'r' , 'a' , 'z' , 'c' ,
                                                 'x' , 'd' , 'b' } ) );
 CHECK_THROWS( r->insertChild( c ) , std::logic_error );   // not a root
 Tree< char > lonely( 'q' );
 CHECK_THROWS( r->insertSibling( &lonely ) , std::logic_error );
 r->deleteChild();                                         // the subtree a
 CHECK( pre( r.get() ) == std::vector< char >( { 'r' , 'b' } ) );
 r->leftmostChild()->deleteSibling();                      // no sibling
 r->deleteChild();
 CHECK( ! r->leftmostChild() );
 }

// section 5.2: the four visits of the classification of the plants, as the
// book gives them; the visit by levels in the book reads "Taliofite" for
// "Tallofite"
ADS_TEST( "sec5.2.visits" )
{
 std::unique_ptr< Tree< std::string > > t( make( plants() ) );
 using N = Tree< std::string > *;
 CHECK( order( t.get() , []( N u , auto f ) { pre_visit( u , f ); } ) ==
        Strings( { "Regno vegetale" , "Crittogame" , "Tallofite" ,
                   "Cianoficee" , "Batteri" , "Alghe" , "Mixomiceti" ,
                   "Funghi" , "Archegoniate" , "Briofite" , "Pteridofite" ,
                   "Fanerogame" , "Gimnosperme" , "Cicadali" , "Ginkgoali" ,
                   "Coniferali" , "Gnetali" , "Angiosperme" , "Dicotiledoni" ,
                   "Monocotiledoni" } ) );
 CHECK( order( t.get() , []( N u , auto f ) { post_visit( u , f ); } ) ==
        Strings( { "Cianoficee" , "Batteri" , "Alghe" , "Mixomiceti" ,
                   "Funghi" , "Tallofite" , "Briofite" , "Pteridofite" ,
                   "Archegoniate" , "Crittogame" , "Cicadali" , "Ginkgoali" ,
                   "Coniferali" , "Gnetali" , "Gimnosperme" , "Dicotiledoni" ,
                   "Monocotiledoni" , "Angiosperme" , "Fanerogame" ,
                   "Regno vegetale" } ) );
 CHECK( order( t.get() , []( N u , auto f ) { in_visit( u , 1 , f ); } ) ==
        Strings( { "Cianoficee" , "Tallofite" , "Batteri" , "Alghe" ,
                   "Mixomiceti" , "Funghi" , "Crittogame" , "Briofite" ,
                   "Archegoniate" , "Pteridofite" , "Regno vegetale" ,
                   "Cicadali" , "Gimnosperme" , "Ginkgoali" , "Coniferali" ,
                   "Gnetali" , "Fanerogame" , "Dicotiledoni" , "Angiosperme" ,
                   "Monocotiledoni" } ) );
 CHECK( order( t.get() , []( N u , auto f ) { bfs( u , f ); } ) ==
        Strings( { "Regno vegetale" , "Crittogame" , "Fanerogame" ,
                   "Tallofite" , "Archegoniate" , "Gimnosperme" ,
                   "Angiosperme" , "Cianoficee" , "Batteri" , "Alghe" ,
                   "Mixomiceti" , "Funghi" , "Briofite" , "Pteridofite" ,
                   "Cicadali" , "Ginkgoali" , "Coniferali" , "Gnetali" ,
                   "Dicotiledoni" , "Monocotiledoni" } ) );

 // the invisit with i = 0 is the previsit, and with i at least the largest
 // number of children it is the postvisit
 CHECK( order( t.get() , []( N u , auto f ) { in_visit( u , 0 , f ); } ) ==
        order( t.get() , []( N u , auto f ) { pre_visit( u , f ); } ) );
 CHECK( order( t.get() , []( N u , auto f ) { in_visit( u , 9 , f ); } ) ==
        order( t.get() , []( N u , auto f ) { post_visit( u , f ); } ) );
 }

// section 5.4: the vector of parents of T1 of Figura 5.4, the nodes 1 to 5
// being 0 to 4
ADS_TEST( "sec5.4.parents" )
{
 ParentTree T( 5 );
 for( int i = 0 ; i < 5 ; ++i )
  CHECK( T.parent( i ) == -1 );   // a forest of isolated nodes
 T.setParent( 1 , 0 );
 T.setParent( 2 , 1 );
 T.setParent( 3 , 1 );
 T.setParent( 4 , 2 );
 CHECK( Ints( { T.parent( 0 ) , T.parent( 1 ) , T.parent( 2 ) ,
                T.parent( 3 ) , T.parent( 4 ) } ) ==
        Ints( { -1 , 0 , 1 , 1 , 2 } ) );
 Ints path;   // from the node 5 up to the root
 for( int i = 4 ; i != -1 ; i = T.parent( i ) )
  path.push_back( i );
 CHECK( path == Ints( { 4 , 2 , 1 , 0 } ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE BINARY TREE -------------------------------*/
/*--------------------------------------------------------------------------*/

// section 5.5: left and right children, the three visits, the
// preconditions of insertLeft() and insertRight()
ADS_TEST( "spec.BinaryTree" )
{
 //        A
 //      B   C
 //       D
 std::unique_ptr< BinaryTree< char > > t( make( B( 'A' ,
                                          B( 'B' , {} , B( 'D' ) ) ,
                                          B( 'C' ) ) ) );
 using V = std::vector< char >;
 CHECK( border( t.get() , pre_b< char > ) == V( { 'A' , 'B' , 'D' , 'C' } ) );
 CHECK( border( t.get() , in_b< char > ) == V( { 'B' , 'D' , 'A' , 'C' } ) );
 CHECK( border( t.get() , post_b< char > ) == V( { 'D' , 'B' , 'C' , 'A' } ) );
 CHECK( ! t->left()->left() && t->left()->right()->read() == 'D' );
 CHECK( t->left()->right()->parent() == t->left() );
 BinaryTree< char > x( 'X' );   // refused, hence still the caller's
 CHECK_THROWS( t->insertLeft( &x ) , std::logic_error );   // there is one
 CHECK_THROWS( t->left()->insertLeft( t->right() ) , std::logic_error );
 t->deleteLeft();
 CHECK( border( t.get() , pre_b< char > ) == V( { 'A' , 'C' } ) );
 BinaryTree< char > * empty = nullptr;
 CHECK( border( empty , pre_b< char > ).empty() );
 }

// section 5.5, Figura 5.9: the binary tree of the first children and of the
// next siblings has the previsit of the ordered tree, and its invisit is the
// postvisit of the ordered tree
ADS_TEST( "sec5.5.binary-representation" )
{
 std::unique_ptr< Tree< std::string > > t( make( plants() ) );
 std::unique_ptr< BinaryTree< std::string > > b( to_binary( t.get() ) );
 using N = Tree< std::string > *;
 CHECK( border( b.get() , pre_b< std::string > ) ==
        order( t.get() , []( N u , auto f ) { pre_visit( u , f ); } ) );
 CHECK( border( b.get() , in_b< std::string > ) ==
        order( t.get() , []( N u , auto f ) { post_visit( u , f ); } ) );
 CHECK( ! b->right() );   // the root has no sibling
 }

// section 5.5, Figura 5.8: the tree of the choices sorts three numbers with
// at most 3 comparisons, and sorting n numbers needs log2( n! ) >=
// ( n / 2 ) log2( n / 2 ) of them in the worst case
ADS_TEST( "sec5.5.choices" )
{
 auto sort3 = []( int a , int b , int c , int & cmp ) -> Ints {
  ++cmp;
  if( a < b ) {
   ++cmp;
   if( b < c ) return( Ints{ a , b , c } );
   ++cmp;
   if( a < c ) return( Ints{ a , c , b } );
   return( Ints{ c , a , b } );
   }
  ++cmp;
  if( c < b ) return( Ints{ c , b , a } );
  ++cmp;
  if( a < c ) return( Ints{ b , a , c } );
  return( Ints{ b , c , a } );
  };
 Ints p = { 1 , 2 , 3 };
 int worst = 0;
 do {
  int cmp = 0;
  CHECK( sort3( p[ 0 ] , p[ 1 ] , p[ 2 ] , cmp ) == Ints( { 1 , 2 , 3 } ) );
  worst = std::max( worst , cmp );
  } while( std::next_permutation( p.begin() , p.end() ) );
 CHECK( worst == 3 );   // = ceil( log2( 3! ) )
 CHECK( worst >= std::ceil( std::log2( 6.0 ) ) );

 double logfact = 0;
 for( int n = 2 ; n <= 64 ; ++n ) {
  logfact += std::log2( double( n ) );
  CHECK( logfact >= ( n / 2.0 ) * std::log2( n / 2.0 ) );
  }
 }

// section 5.6: in a complete d-ary tree stored in a vector the children of
// the place i are d i + 1 , ... , d i + d, and the places are all used once
ADS_TEST( "sec5.6.dary" )
{
 for( long d = 1 ; d <= 4 ; ++d ) {
  const long n = 1 + d + d * d + d * d * d;   // height 3
  std::vector< int > seen( n , 0 );
  seen[ 0 ] = 1;
  for( long i = 0 ; dary_child( i , d , 1 ) < n ; ++i )
   for( long j = 1 ; j <= d ; ++j ) {
    const long c = dary_child( i , d , j );
    ++seen[ c ];
    CHECK( dary_parent( c , d ) == i );
    }
  CHECK( std::all_of( seen.begin() , seen.end() ,
                      []( int s ) { return( s == 1 ); } ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- EXERCISES --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

// Soluzione Esercizio 5.1: depth(), the height, by a postvisit
template< class T >
int depth( Tree< T > * t )
{
 int max = 0;
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() )
  max = std::max( max , depth( u ) + 1 );
 return( max );
 }

}  // end( namespace )

// Esercizio 5.1: the plants have height 3, a node alone 0
ADS_TEST( "ex5.1" )
{
 std::unique_ptr< Tree< std::string > > t( make( plants() ) );
 CHECK( depth( t.get() ) == 3 );
 Tree< int > one( 1 );
 CHECK( depth( &one ) == 0 );
 }

namespace {

// Soluzione Esercizio 5.2: leafK(), the leaves whose path from the root
// sums to k are deleted, by a previsit that carries the sum; it returns
// true if t itself is such a leaf, which its caller deletes
bool leafK( Tree< int > * t , int k , int somma )
{
 somma += t->read();
 if( ( somma == k ) && ! t->leftmostChild() )
  return( true );
 while( t->leftmostChild() && leafK( t->leftmostChild() , k , somma ) )
  t->deleteChild();
 Tree< int > * u = t->leftmostChild();
 Tree< int > * v = u ? u->rightSibling() : nullptr;
 while( v ) {
  if( leafK( v , k , somma ) )
   u->deleteSibling();   // u stays, its next sibling goes
  else
   u = v;
  v = u->rightSibling();
  }
 return( false );
 }

}  // end( namespace )

// Esercizio 5.2: with k = 6 the leaves 3 (1 + 2 + 3) and 0 (1 + 5 + 0) go;
// the node 5, a leaf only afterwards, stays
ADS_TEST( "ex5.2" )
{
 using S = Spec< int >;
 std::unique_ptr< Tree< int > > t( make( S{ 1 , {
  S{ 2 , { S{ 3 , {} } , S{ 4 , {} } } } ,
  S{ 5 , { S{ 0 , {} } } } ,
  S{ 6 , {} } } } ) );
 CHECK( ! leafK( t.get() , 6 , 0 ) );
 CHECK( order( t.get() , []( Tree< int > * u , auto f ) {
  pre_visit( u , f ); } ) == Ints( { 1 , 2 , 4 , 5 , 6 } ) );
 Tree< int > single( 6 );   // the whole tree is a leaf to delete
 CHECK( leafK( &single , 6 , 0 ) );
 }

namespace {

// Esercizio 5.3: the width, the most nodes on a level, by a visit by levels
// that counts the nodes of each level
template< class T >
int width( Tree< T > * t )
{
 std::vector< int > count;
 struct Item { Tree< T > * u; int level; };
 List< Item > Q;
 Q.insert( Q.next( Q.tail() ) , Item{ t , 0 } );
 while( ! Q.isEmpty() ) {
  const Item it = Q.read( Q.head() );
  Q.remove( Q.head() );
  if( int( count.size() ) <= it.level )
   count.push_back( 0 );
  ++count[ it.level ];
  for( auto u = it.u->leftmostChild() ; u ; u = u->rightSibling() )
   Q.insert( Q.next( Q.tail() ) , Item{ u , it.level + 1 } );
  }
 return( *std::max_element( count.begin() , count.end() ) );
 }

}  // end( namespace )

// Esercizio 5.3: the levels of the plants have 1, 2, 4 and 13 nodes
ADS_TEST( "ex5.3" )
{
 std::unique_ptr< Tree< std::string > > t( make( plants() ) );
 CHECK( width( t.get() ) == 13 );
 Tree< int > one( 1 );
 CHECK( width( &one ) == 1 );
 }

namespace {

// Esercizio 5.4: the binary tree of distinct values given its previsit and
// its invisit: the first value of the previsit is the root, which splits the
// invisit into the left and the right subtree
BinaryTree< char > * rebuild( const std::string & pre ,
                              const std::string & in )
{
 if( pre.empty() )
  return( nullptr );
 auto t = new BinaryTree< char >( pre[ 0 ] );
 const auto k = in.find( pre[ 0 ] );
 if( auto l = rebuild( pre.substr( 1 , k ) , in.substr( 0 , k ) ) )
  t->insertLeft( l );
 if( auto r = rebuild( pre.substr( k + 1 ) , in.substr( k + 1 ) ) )
  t->insertRight( r );
 return( t );
 }

}  // end( namespace )

// Esercizio 5.4: the tree of 9 nodes rebuilt from the previsit and the
// invisit of the book has the postvisit the book gives
ADS_TEST( "ex5.4" )
{
 std::unique_ptr< BinaryTree< char > > t( rebuild( "AEBFGCDIH" ,
                                                   "BEGFCADHI" ) );
 using V = std::vector< char >;
 CHECK( border( t.get() , post_b< char > ) ==
        V( { 'B' , 'G' , 'C' , 'F' , 'E' , 'H' , 'I' , 'D' , 'A' } ) );
 CHECK( border( t.get() , pre_b< char > ) ==
        V( { 'A' , 'E' , 'B' , 'F' , 'G' , 'C' , 'D' , 'I' , 'H' } ) );
 }

namespace {

// Soluzione Esercizio 5.5: count(), the size of each subtree in its root,
// by a postvisit
int count( BinaryTree< int > * t )
{
 if( ! t )
  return( 0 );
 const int somma = 1 + count( t->left() ) + count( t->right() );
 t->write( somma );
 return( somma );
 }

}  // end( namespace )

// Esercizio 5.5: each node holds the size of its subtree
ADS_TEST( "ex5.5" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 0 ,
                                         B( 0 , B( 0 ) , B( 0 ) ) ,
                                         B( 0 , {} , B( 0 ) ) ) ) );
 CHECK( count( t.get() ) == 6 );
 CHECK( border( t.get() , pre_b< int > ) ==
        Ints( { 6 , 3 , 1 , 1 , 2 , 1 } ) );
 CHECK( count( nullptr ) == 0 );
 }

namespace {

// Soluzione Esercizio 5.6: deleteLeaf(), by a previsit: a left child that
// is a leaf with the value of its parent goes
void deleteLeaf( BinaryTree< int > * t )
{
 if( t ) {
  auto u = t->left();
  if( u && ! u->left() && ! u->right() && ( t->read() == u->read() ) )
   t->deleteLeft();
  deleteLeaf( t->left() );
  deleteLeaf( t->right() );
  }
 }

}  // end( namespace )

// Esercizio 5.6: the left leaves equal to their parent go; a right one and a
// left one of different value stay
ADS_TEST( "ex5.6" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 1 ,
                                         B( 1 ) ,                  // goes
                                         B( 2 , B( 2 ) , B( 2 ) ) ) ) );
 deleteLeaf( t.get() );
 CHECK( border( t.get() , pre_b< int > ) == Ints( { 1 , 2 , 2 } ) );
 CHECK( ! t->left() && ! t->right()->left() && t->right()->right() );
 }

namespace {

// Soluzione Esercizio 5.7: insertLeaf(), by a postvisit: a left child 0 is
// added to each leaf
void insertLeaf( BinaryTree< int > * t )
{
 if( t ) {
  insertLeaf( t->left() );
  insertLeaf( t->right() );
  if( ! t->left() && ! t->right() )
   t->insertLeft( new BinaryTree< int >( 0 ) );
  }
 }

}  // end( namespace )

// Esercizio 5.7: each leaf gets a left child 0, and only the leaves
ADS_TEST( "ex5.7" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 1 , B( 2 ) ,
                                                  B( 3 , {} , B( 4 ) ) ) ) );
 insertLeaf( t.get() );
 CHECK( border( t.get() , pre_b< int > ) ==
        Ints( { 1 , 2 , 0 , 3 , 4 , 0 } ) );
 CHECK( t->left()->left()->read() == 0 && ! t->right()->left() );
 }

namespace {

// Esercizio 5.8: the leaves, read from left to right, get first all the 0
// and then all the 1, by two postvisits: one counts the leaves and the 1,
// the other writes the values in the order of the leaves
void leaves( BinaryTree< int > * t , int & n , int & ones )
{
 if( t ) {
  leaves( t->left() , n , ones );
  leaves( t->right() , n , ones );
  if( ! t->left() && ! t->right() ) {
   ++n;
   ones += t->read();
   }
  }
 }

void rewrite( BinaryTree< int > * t , int & zeros )
{
 if( t ) {
  rewrite( t->left() , zeros );
  rewrite( t->right() , zeros );
  if( ! t->left() && ! t->right() ) {
   t->write( zeros > 0 ? 0 : 1 );
   --zeros;
   }
  }
 }

void group( BinaryTree< int > * t )
{
 int n = 0 , ones = 0;
 leaves( t , n , ones );
 int zeros = n - ones;
 rewrite( t , zeros );
 }

}  // end( namespace )

// Esercizio 5.8: the leaves 1, 0, 1, 0 become 0, 0, 1, 1
ADS_TEST( "ex5.8" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 0 ,
                                         B( 0 , B( 1 ) , B( 0 ) ) ,
                                         B( 0 , B( 1 ) , B( 0 ) ) ) ) );
 group( t.get() );
 Ints lv;
 in_visit( t.get() , [ & ]( BinaryTree< int > * u ) {
  if( ! u->left() && ! u->right() ) lv.push_back( u->read() ); } );
 CHECK( lv == Ints( { 0 , 0 , 1 , 1 } ) );
 }

namespace {

// Esercizio 5.9: the mirror tree, the two subtrees of each node swapped
BinaryTree< int > * mirror( BinaryTree< int > * t )
{
 if( ! t )
  return( nullptr );
 auto m = new BinaryTree< int >( t->read() );
 if( auto r = mirror( t->right() ) ) m->insertLeft( r );
 if( auto l = mirror( t->left() ) ) m->insertRight( l );
 return( m );
 }

}  // end( namespace )

// Esercizio 5.9: the invisit of the mirror is the reverse one
ADS_TEST( "ex5.9" )
{
 std::mt19937 rng( 509 );
 for( int n : { 0 , 1 , 2 , 7 , 40 } ) {
  int next = 0;
  std::unique_ptr< BinaryTree< int > > t( random_binary( n , rng , next ) );
  std::unique_ptr< BinaryTree< int > > m( mirror( t.get() ) );
  auto a = border( t.get() , in_b< int > );
  std::reverse( a.begin() , a.end() );
  CHECK( border( m.get() , in_b< int > ) == a );
  }
 }

namespace {

// Esercizio 5.10: each leaf gets a child with the sum of its path from the
// root, by a previsit that carries the sum
void addSums( BinaryTree< int > * t , int somma )
{
 if( t ) {
  somma += t->read();
  if( ! t->left() && ! t->right() )
   t->insertLeft( new BinaryTree< int >( somma ) );
  else {
   addSums( t->left() , somma );
   addSums( t->right() , somma );
   }
  }
 }

}  // end( namespace )

// Esercizio 5.10: the leaves 3 (1 + 2 + 3) and 5 (1 + 5) get 6 and 6
ADS_TEST( "ex5.10" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 1 , B( 2 , {} , B( 3 ) ) ,
                                                  B( 5 ) ) ) );
 addSums( t.get() , 0 );
 CHECK( border( t.get() , pre_b< int > ) ==
        Ints( { 1 , 2 , 3 , 6 , 5 , 6 } ) );
 }

// Esercizio 5.11: the leaves are the nodes with two children plus one
ADS_TEST( "ex5.11" )
{
 std::mt19937 rng( 511 );
 for( int n = 1 ; n <= 60 ; ++n ) {
  int next = 0;
  std::unique_ptr< BinaryTree< int > > t( random_binary( n , rng , next ) );
  int leaves = 0 , two = 0;
  pre_visit( t.get() , [ & ]( BinaryTree< int > * u ) {
   if( ! u->left() && ! u->right() ) ++leaves;
   if( u->left() && u->right() ) ++two;
   } );
  CHECK( leaves == two + 1 );
  }
 }

namespace {

BinaryTree< int > * complete( int k )
{
 auto t = new BinaryTree< int >( k );
 if( k > 0 ) {
  t->insertLeft( complete( k - 1 ) );
  t->insertRight( complete( k - 1 ) );
  }
 return( t );
 }

}  // end( namespace )

// Esercizio 5.12: a complete binary tree of height k has 2^( k + 1 ) - 1
// nodes and 2^k leaves
ADS_TEST( "ex5.12" )
{
 for( int k = 0 ; k <= 12 ; ++k ) {
  std::unique_ptr< BinaryTree< int > > t( complete( k ) );
  long nodes = 0 , leaves = 0;
  pre_visit( t.get() , [ & ]( BinaryTree< int > * u ) {
   ++nodes;
   if( ! u->left() && ! u->right() ) ++leaves;
   } );
  CHECK( nodes == ( 2L << k ) - 1 );
  CHECK( leaves == ( 1L << k ) );
  }
 }

namespace {

// Esercizio 5.13: the three visits made iterative with a stack. The
// previsit pushes the right child before the left one; the invisit goes
// down the left children pushing them, then examines and moves right; the
// postvisit pushes each node with a flag, examined the second time it is
// popped, after its subtrees
Ints pre_iterative( BinaryTree< int > * t , int n )
{
 Ints out;
 ArrayStack< BinaryTree< int > * > S( n + 1 );
 if( t ) S.push( t );
 while( ! S.isEmpty() ) {
  auto u = S.pop();
  out.push_back( u->read() );
  if( u->right() ) S.push( u->right() );
  if( u->left() ) S.push( u->left() );
  }
 return( out );
 }

Ints in_iterative( BinaryTree< int > * t , int n )
{
 Ints out;
 ArrayStack< BinaryTree< int > * > S( n + 1 );
 auto u = t;
 while( u || ! S.isEmpty() ) {
  while( u ) {
   S.push( u );
   u = u->left();
   }
  u = S.pop();
  out.push_back( u->read() );
  u = u->right();
  }
 return( out );
 }

Ints post_iterative( BinaryTree< int > * t , int n )
{
 Ints out;
 ArrayStack< std::pair< BinaryTree< int > * , bool > > S( 2 * n + 1 );
 if( t ) S.push( { t , false } );
 while( ! S.isEmpty() ) {
  auto [ u , expanded ] = S.pop();
  if( expanded )
   out.push_back( u->read() );
  else {
   S.push( { u , true } );
   if( u->right() ) S.push( { u->right() , false } );
   if( u->left() ) S.push( { u->left() , false } );
   }
  }
 return( out );
 }

}  // end( namespace )

// Esercizio 5.13: the iterative visits give the recursive ones
ADS_TEST( "ex5.13" )
{
 std::mt19937 rng( 513 );
 for( int n : { 0 , 1 , 2 , 3 , 10 , 100 , 1000 } ) {
  int next = 0;
  std::unique_ptr< BinaryTree< int > > t( random_binary( n , rng , next ) );
  CHECK( pre_iterative( t.get() , n ) == border( t.get() , pre_b< int > ) );
  CHECK( in_iterative( t.get() , n ) == border( t.get() , in_b< int > ) );
  CHECK( post_iterative( t.get() , n ) == border( t.get() , post_b< int > ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch05.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
