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
 *         Università di Pisa \n
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
#include <limits>
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

/// a chain of n ordered nodes, 0 the root and n - 1 the leaf, built from the
/// leaf up so that each insertion is under a root
Tree< int > * chain( int n )
{
 auto t = new Tree< int >( n - 1 );
 for( int i = n - 2 ; i >= 0 ; --i ) {
  auto u = new Tree< int >( i );
  u->insertChild( t );
  t = u;
  }
 return( t );
 }

/// a chain of n binary nodes, 0 the root, all left children or all right
BinaryTree< int > * bchain( int n , bool left )
{
 auto t = new BinaryTree< int >( n - 1 );
 for( int i = n - 2 ; i >= 0 ; --i ) {
  auto u = new BinaryTree< int >( i );
  if( left ) u->insertLeft( t );
  else u->insertRight( t );
  t = u;
  }
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
// a tree that is not a root cannot be inserted, nor a sibling of a root,
// nor nil, nor the node itself, nor the root of the tree of the node (a
// cycle); deleteChild() of a leaf and deleteSibling() of a last sibling do
// nothing; a chain of 10^5 nodes is visited and destroyed
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
 CHECK( r->leftmostChild()->isLastSibling() );
 r->leftmostChild()->deleteChild();                        // a leaf
 CHECK( r->leftmostChild()->isLeaf() && ! r->isLeaf() );
 r->deleteChild();
 CHECK( ! r->leftmostChild() && r->isLeaf() );
 r->deleteChild();                                         // a leaf
 CHECK( r->isLeaf() && ( r->read() == 'r' ) );

 // the contracts: nil, the node itself, the root of its own tree
 CHECK_THROWS( r->insertChild( nullptr ) , contract_violation );
 CHECK_THROWS( r->insertChild( r.get() ) , contract_violation );
 r->insertChild( new Tree< char >( 'p' ) );
 auto p = r->leftmostChild();
 p->insertChild( new Tree< char >( 'q' ) );
 CHECK_THROWS( p->insertChild( r.get() ) , contract_violation );
 CHECK_THROWS( p->leftmostChild()->insertChild( r.get() ) ,
               contract_violation );
 CHECK_THROWS( p->leftmostChild()->insertSibling( r.get() ) ,
               contract_violation );
 CHECK_THROWS( p->insertSibling( p ) , contract_violation );
 CHECK( pre( r.get() ) == std::vector< char >( { 'r' , 'p' , 'q' } ) );
 r->write( 's' );
 CHECK( r->read() == 's' );

 // deep and wide: no recursion in the destructor along the siblings, and
 // the visits of a chain of 10^5 nodes
 std::unique_ptr< Tree< int > > ch( chain( 100000 ) );
 long n = 0 , last = -1;
 pre_visit( ch.get() , [ & ]( Tree< int > * u ) {
  n += ( u->read() == last + 1 );
  last = u->read(); } );
 CHECK( n == 100000 );
 n = 0;
 post_visit( ch.get() , [ & ]( Tree< int > * u ) {
  n += ( u->read() == 99999 - n ); } );
 CHECK( n == 100000 );
 Tree< int > w( -1 );   // the invariant costs O( children ) per insertion
 for( int i = 0 ; i < 10000 ; ++i )
  w.insertChild( new Tree< int >( i ) );
 n = 0;
 for( auto u = w.leftmostChild() ; u ; u = u->rightSibling() )
  n += ( u->parent() == &w );
 CHECK( n == 10000 );
 }

// section 5.2: the four visits of the classification of the plants, as the
// book gives them; the visit by levels in the book reads "Taliofite" for
// "Tallofite". A node alone is the only node of each visit, a chain gives
// the same previsit and visit by levels, and its postvisit is the invisit
// with i >= 1; nil and i < 0 violate the contracts
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

 using M = Tree< int > *;
 auto all = []( M u ) {
  return( std::vector< Ints >( {
   order( u , []( M x , auto f ) { pre_visit( x , f ); } ) ,
   order( u , []( M x , auto f ) { post_visit( x , f ); } ) ,
   order( u , []( M x , auto f ) { in_visit( x , 0 , f ); } ) ,
   order( u , []( M x , auto f ) { in_visit( x , 1 , f ); } ) ,
   order( u , []( M x , auto f ) { bfs( x , f ); } ) } ) );
  };
 Tree< int > one( 7 );
 for( const Ints & v : all( &one ) )
  CHECK( v == Ints( { 7 } ) );
 std::unique_ptr< Tree< int > > c( chain( 5 ) );
 const auto v = all( c.get() );
 CHECK( ( v[ 0 ] == Ints( { 0 , 1 , 2 , 3 , 4 } ) ) && ( v[ 4 ] == v[ 0 ] ) &&
        ( v[ 2 ] == v[ 0 ] ) );
 CHECK( ( v[ 1 ] == Ints( { 4 , 3 , 2 , 1 , 0 } ) ) && ( v[ 3 ] == v[ 1 ] ) );
 auto none = []( M ) {};
 CHECK_THROWS( pre_visit( M( nullptr ) , none ) , contract_violation );
 CHECK_THROWS( bfs( M( nullptr ) , none ) , contract_violation );
 CHECK_THROWS( in_visit( &one , -1 , none ) , contract_violation );
 }

// section 5.4: the vector of parents of T1 of Figura 5.4, the nodes 1 to 5
// being 0 to 4; a node made a root again, the empty forest, and the
// contracts: nodes out of range, a node its own parent, and a cycle (the
// parent in the subtree of the node)
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

 CHECK_THROWS( T.setParent( 0 , 4 ) , contract_violation );   // a cycle
 CHECK_THROWS( T.setParent( 1 , 3 ) , contract_violation );
 CHECK_THROWS( T.setParent( 2 , 2 ) , contract_violation );
 CHECK_THROWS( T.setParent( 5 , 0 ) , contract_violation );
 CHECK_THROWS( T.setParent( -1 , 0 ) , contract_violation );
 CHECK_THROWS( T.setParent( 0 , 5 ) , contract_violation );
 CHECK_THROWS( T.setParent( 0 , -2 ) , contract_violation );
 CHECK_THROWS( (void) T.parent( 5 ) , contract_violation );
 CHECK_THROWS( (void) T.parent( -1 ) , contract_violation );
 T.setParent( 3 , 4 );                    // a sibling's subtree: no cycle
 T.setParent( 2 , -1 );                   // 2 a root again, with 4 and 3
 CHECK( ( T.parent( 2 ) == -1 ) && ( T.parent( 4 ) == 2 ) &&
        ( T.parent( 3 ) == 4 ) );
 T.setParent( 0 , 3 );                    // now 0 can go below 3
 CHECK( ( T.parent( 0 ) == 3 ) && ( T.parent( 1 ) == 0 ) );
 CHECK_THROWS( T.setParent( 2 , 1 ) , contract_violation );   // 1 below 2
 ParentTree E( 0 );
 CHECK( E.size() == 0 );
 CHECK_THROWS( (void) E.parent( 0 ) , contract_violation );
 CHECK_THROWS( ParentTree( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- THE BINARY TREE -------------------------------*/
/*--------------------------------------------------------------------------*/

// section 5.5: left and right children, the three visits, the
// preconditions of insertLeft() and insertRight(): the child missing, a
// root, not nil, not the node itself nor the root of its tree (a cycle);
// deleting and detaching a missing child, the empty tree, and chains of
// 10^5 nodes, all left or all right, visited and destroyed
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
 CHECK( border( empty , in_b< char > ).empty() &&
        border( empty , post_b< char > ).empty() );

 CHECK_THROWS( t->insertLeft( nullptr ) , contract_violation );
 CHECK_THROWS( t->insertLeft( t.get() ) , contract_violation );
 CHECK_THROWS( t->right()->insertLeft( t.get() ) , contract_violation );
 CHECK_THROWS( t->right()->insertRight( t.get() ) , contract_violation );
 t->deleteLeft();                          // missing: nothing
 CHECK( ! t->detachLeft() );
 auto c = t->detachRight();                // C, now a root of its own
 CHECK( c && ! c->parent() && ! t->right() && ( c->read() == 'C' ) );
 CHECK_THROWS( c->insertLeft( c ) , contract_violation );
 t->insertLeft( c );                       // and it can go back
 CHECK( border( t.get() , in_b< char > ) == V( { 'C' , 'A' } ) );
 t->deleteRight();                         // missing: nothing
 CHECK( ! t->right() && t->left() );

 for( bool left : { true , false } ) {
  std::unique_ptr< BinaryTree< int > > ch( bchain( 100000 , left ) );
  long n = 0 , k = 0;
  pre_visit( ch.get() , [ & ]( BinaryTree< int > * u ) {
   n += ( u->read() == k++ ); } );
  CHECK( n == 100000 );
  Ints in = border( ch.get() , in_b< int > );
  CHECK( ( in.front() == ( left ? 99999 : 0 ) ) && ( in.size() == 100000 ) );
  }
 }

// section 5.5, Figura 5.9: the binary tree of the first children and of the
// next siblings has the previsit of the ordered tree, and its invisit is the
// postvisit of the ordered tree; a node alone is a node alone, a chain a
// chain of left children, a star a chain of right children; nil violates
// the contract
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

 Tree< int > one( 1 );
 std::unique_ptr< BinaryTree< int > > o( to_binary( &one ) );
 CHECK( o->read() == 1 && ! o->left() && ! o->right() );
 std::unique_ptr< Tree< int > > c( chain( 4 ) );
 std::unique_ptr< BinaryTree< int > > bc( to_binary( c.get() ) );
 int lefts = 0;
 pre_visit( bc.get() , [ & ]( BinaryTree< int > * u ) {
  lefts += ! u->right() && ( u->left() || ( u->read() == 3 ) ); } );
 CHECK( lefts == 4 );
 Tree< int > star( 0 );
 for( int i = 5 ; i >= 1 ; --i )
  star.insertChild( new Tree< int >( i ) );
 std::unique_ptr< BinaryTree< int > > bs( to_binary( &star ) );
 CHECK( ! bs->right() && bs->left() && ! bs->left()->left() );
 int rights = 0;
 for( auto u = bs->left() ; u ; u = u->right() )
  rights += ! u->left() && ( u->read() == rights + 1 );
 CHECK( rights == 5 );
 CHECK_THROWS( (void) to_binary( ( const Tree< int > * ) nullptr ) ,
               contract_violation );
 }

// section 5.5, Figura 5.8: the tree of the choices sorts three numbers with
// at most 3 comparisons, also with ties (all the triples of { 0 , 1 , 2 }),
// and sorting n numbers needs log2( n! ) >= ( n / 2 ) log2( n / 2 ) of them
// in the worst case
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
 for( int a = 0 ; a < 3 ; ++a )            // ties, and all equal
  for( int b = 0 ; b < 3 ; ++b )
   for( int c = 0 ; c < 3 ; ++c ) {
    int cmp = 0;
    Ints s = { a , b , c };
    std::sort( s.begin() , s.end() );
    CHECK( ( sort3( a , b , c , cmp ) == s ) && ( cmp <= 3 ) );
    }

 double logfact = 0;
 for( int n = 2 ; n <= 64 ; ++n ) {
  logfact += std::log2( double( n ) );
  CHECK( logfact >= ( n / 2.0 ) * std::log2( n / 2.0 ) );
  }
 }

// section 5.6: in a complete d-ary tree stored in a vector the children of
// the place i are d i + 1 , ... , d i + d, and the places are all used once;
// d = 1 is a chain; the largest place whose child fits in a long, and the
// contracts: i < 0, d < 1, j out of 1 .. d, the root has no parent, and a
// child beyond the largest long
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
 CHECK( ( dary_child( 0 , 1 , 1 ) == 1 ) && ( dary_parent( 1 , 7 ) == 0 ) );
 CHECK( ( dary_child( 5 , 1 , 1 ) == 6 ) && ( dary_parent( 6 , 1 ) == 5 ) );
 const long M = std::numeric_limits< long >::max();
 CHECK( dary_child( ( M - 3 ) / 3 , 3 , 3 ) <= M );
 CHECK( dary_parent( dary_child( ( M - 3 ) / 3 , 3 , 3 ) , 3 ) ==
        ( M - 3 ) / 3 );
 CHECK( dary_child( M - 1 , 1 , 1 ) == M );
 CHECK_THROWS( (void) dary_child( M , 1 , 1 ) , contract_violation );
 CHECK_THROWS( (void) dary_child( M / 2 , 2 , 2 ) , contract_violation );
 CHECK_THROWS( (void) dary_child( -1 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) dary_child( 0 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( (void) dary_child( 0 , 2 , 0 ) , contract_violation );
 CHECK_THROWS( (void) dary_child( 0 , 2 , 3 ) , contract_violation );
 CHECK_THROWS( (void) dary_parent( 0 , 2 ) , contract_violation );
 CHECK_THROWS( (void) dary_parent( 1 , 0 ) , contract_violation );
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

// Esercizio 5.1: the plants have height 3, a node alone 0, a chain of n
// nodes n - 1, a star of n leaves 1
ADS_TEST( "ex5.1" )
{
 std::unique_ptr< Tree< std::string > > t( make( plants() ) );
 CHECK( depth( t.get() ) == 3 );
 Tree< int > one( 1 );
 CHECK( depth( &one ) == 0 );
 std::unique_ptr< Tree< int > > c( chain( 1000 ) );
 CHECK( depth( c.get() ) == 999 );
 for( int i = 0 ; i < 50 ; ++i )
  one.insertChild( new Tree< int >( i ) );
 CHECK( depth( &one ) == 1 );
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
// the node 5, a leaf only afterwards, stays. A k no path reaches changes
// nothing; the first children in a row, the siblings in a row and the last
// one go; with negative values a sum reaches k twice, only the leaf goes
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
 CHECK( ! leafK( &single , 5 , 0 ) );
 auto pre = []( Tree< int > * u ) {
  return( order( u , []( Tree< int > * x , auto f ) {
   pre_visit( x , f ); } ) ); };
 CHECK( ! leafK( t.get() , 100 , 0 ) );
 CHECK( pre( t.get() ) == Ints( { 1 , 2 , 4 , 5 , 6 } ) );
 std::unique_ptr< Tree< int > > s( make( S{ 0 , {
  S{ 2 , {} } , S{ 2 , {} } , S{ 7 , {} } , S{ 2 , {} } , S{ 2 , {} } ,
  S{ 7 , {} } , S{ 2 , {} } } } ) );
 CHECK( ! leafK( s.get() , 2 , 0 ) );
 CHECK( pre( s.get() ) == Ints( { 0 , 7 , 7 } ) );
 CHECK( ! leafK( s.get() , 7 , 0 ) );       // all the leaves: s stays
 CHECK( pre( s.get() ) == Ints( { 0 } ) );
 std::unique_ptr< Tree< int > > n( make( S{ 3 , {
  S{ -3 , { S{ 3 , {} } } } , S{ 0 , {} } } } ) );   // 3 , 0 , 3 , 3
 CHECK( ! leafK( n.get() , 3 , 0 ) );
 CHECK( pre( n.get() ) == Ints( { 3 , -3 } ) );
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
 while( ! Q.empty() ) {
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

// Esercizio 5.3: the levels of the plants have 1, 2, 4 and 13 nodes; a
// node alone and a chain have width 1, a star of n leaves n, and two levels
// of the same width give it once
ADS_TEST( "ex5.3" )
{
 std::unique_ptr< Tree< std::string > > t( make( plants() ) );
 CHECK( width( t.get() ) == 13 );
 Tree< int > one( 1 );
 CHECK( width( &one ) == 1 );
 std::unique_ptr< Tree< int > > c( chain( 1000 ) );
 CHECK( width( c.get() ) == 1 );
 for( int i = 0 ; i < 50 ; ++i )
  one.insertChild( new Tree< int >( i ) );
 CHECK( width( &one ) == 50 );
 using S = Spec< int >;
 std::unique_ptr< Tree< int > > e( make( S{ 0 , {
  S{ 1 , { S{ 3 , {} } } } , S{ 2 , { S{ 4 , {} } } } } } ) );
 CHECK( width( e.get() ) == 2 );
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
// invisit of the book has the postvisit the book gives; the empty visits
// give the empty tree, one value a node alone, the chains of left and of
// right children come back, and so do random trees of up to 90 nodes
ADS_TEST( "ex5.4" )
{
 std::unique_ptr< BinaryTree< char > > t( rebuild( "AEBFGCDIH" ,
                                                   "BEGFCADHI" ) );
 using V = std::vector< char >;
 CHECK( border( t.get() , post_b< char > ) ==
        V( { 'B' , 'G' , 'C' , 'F' , 'E' , 'H' , 'I' , 'D' , 'A' } ) );
 CHECK( border( t.get() , pre_b< char > ) ==
        V( { 'A' , 'E' , 'B' , 'F' , 'G' , 'C' , 'D' , 'I' , 'H' } ) );

 CHECK( ! rebuild( "" , "" ) );
 std::unique_ptr< BinaryTree< char > > o( rebuild( "X" , "X" ) );
 CHECK( o->read() == 'X' && ! o->left() && ! o->right() );
 std::unique_ptr< BinaryTree< char > > l( rebuild( "CBA" , "ABC" ) ) ,
                                       r( rebuild( "ABC" , "ABC" ) );
 CHECK( l->left()->left()->read() == 'A' && ! l->right() );
 CHECK( r->right()->right()->read() == 'C' && ! r->left() );

 std::mt19937 rng( 504 );
 for( int n = 1 ; n <= 90 ; n += 7 ) {
  int next = 0;
  std::unique_ptr< BinaryTree< int > > b( random_binary( n , rng , next ) );
  std::string pre , in , post;
  auto ch = []( BinaryTree< int > * u ) { return( char( 33 + u->read() ) ); };
  pre_visit( b.get() , [ & ]( BinaryTree< int > * u ) { pre += ch( u ); } );
  in_visit( b.get() , [ & ]( BinaryTree< int > * u ) { in += ch( u ); } );
  post_visit( b.get() , [ & ]( BinaryTree< int > * u ) { post += ch( u ); } );
  std::unique_ptr< BinaryTree< char > > c( rebuild( pre , in ) );
  const auto p = border( c.get() , post_b< char > );
  CHECK( std::string( p.begin() , p.end() ) == post );
  }
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

// Esercizio 5.5: each node holds the size of its subtree; a node alone 1,
// the empty tree 0, a chain of n nodes n , n - 1 , ... , 1
ADS_TEST( "ex5.5" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 0 ,
                                         B( 0 , B( 0 ) , B( 0 ) ) ,
                                         B( 0 , {} , B( 0 ) ) ) ) );
 CHECK( count( t.get() ) == 6 );
 CHECK( border( t.get() , pre_b< int > ) ==
        Ints( { 6 , 3 , 1 , 1 , 2 , 1 } ) );
 CHECK( count( nullptr ) == 0 );
 BinaryTree< int > one( 9 );
 CHECK( count( &one ) == 1 && one.read() == 1 );
 std::unique_ptr< BinaryTree< int > > c( bchain( 1000 , true ) );
 CHECK( count( c.get() ) == 1000 );
 int k = 1000 , ok = 0;
 pre_visit( c.get() , [ & ]( BinaryTree< int > * u ) {
  ok += ( u->read() == k-- ); } );
 CHECK( ok == 1000 );
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
// left one of different value stay. The empty tree and a node alone do not
// change; in a chain of left children all equal only the last goes, as the
// previsit has passed its parent when that becomes a leaf
ADS_TEST( "ex5.6" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 1 ,
                                         B( 1 ) ,                  // goes
                                         B( 2 , B( 2 ) , B( 2 ) ) ) ) );
 deleteLeaf( t.get() );
 CHECK( border( t.get() , pre_b< int > ) == Ints( { 1 , 2 , 2 } ) );
 CHECK( ! t->left() && ! t->right()->left() && t->right()->right() );
 deleteLeaf( nullptr );
 BinaryTree< int > one( 1 );
 deleteLeaf( &one );
 CHECK( ! one.left() && ! one.right() );
 std::unique_ptr< BinaryTree< int > > c( make( B( 1 , B( 1 , B( 1 ) ) ) ) );
 deleteLeaf( c.get() );
 CHECK( border( c.get() , pre_b< int > ) == Ints( { 1 , 1 } ) );
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

// Esercizio 5.7: each leaf gets a left child 0, and only the leaves; a node
// alone gets one, a chain only at its end, the empty tree nothing
ADS_TEST( "ex5.7" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 1 , B( 2 ) ,
                                                  B( 3 , {} , B( 4 ) ) ) ) );
 insertLeaf( t.get() );
 CHECK( border( t.get() , pre_b< int > ) ==
        Ints( { 1 , 2 , 0 , 3 , 4 , 0 } ) );
 CHECK( t->left()->left()->read() == 0 && ! t->right()->left() );
 insertLeaf( nullptr );
 BinaryTree< int > one( 1 );
 insertLeaf( &one );
 CHECK( border( &one , pre_b< int > ) == Ints( { 1 , 0 } ) );
 std::unique_ptr< BinaryTree< int > > c( bchain( 3 , false ) );
 insertLeaf( c.get() );
 CHECK( border( c.get() , pre_b< int > ) == Ints( { 0 , 1 , 2 , 0 } ) );
 CHECK( c->right()->right()->left() && ! c->left() );
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

// Esercizio 5.8: the leaves 1, 0, 1, 0 become 0, 0, 1, 1, the inner nodes
// unchanged; all 0, all 1, and a node alone stay as they are
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
 CHECK( ( t->read() == 0 ) && ( t->left()->read() == 0 ) &&
        ( t->right()->read() == 0 ) );
 for( int x : { 0 , 1 } ) {
  std::unique_ptr< BinaryTree< int > > u( make( B( 5 , B( x , B( x ) ) ,
                                                   B( x ) ) ) );
  group( u.get() );
  CHECK( border( u.get() , pre_b< int > ) == Ints( { 5 , x , x , x } ) );
  BinaryTree< int > one( x );
  group( &one );
  CHECK( one.read() == x );
  }
 group( nullptr );
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

// Esercizio 5.9: the invisit of the mirror is the reverse one, also for the
// empty tree, a node alone and the chains, and the mirror of the mirror is
// the tree
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
  std::unique_ptr< BinaryTree< int > > mm( mirror( m.get() ) );
  CHECK( border( mm.get() , pre_b< int > ) ==
         border( t.get() , pre_b< int > ) );
  CHECK( border( mm.get() , in_b< int > ) ==
         border( t.get() , in_b< int > ) );
  }
 std::unique_ptr< BinaryTree< int > > c( bchain( 50 , true ) );
 std::unique_ptr< BinaryTree< int > > m( mirror( c.get() ) );
 int rights = 0;
 for( auto u = m.get() ; u ; u = u->right() )
  rights += ! u->left();
 CHECK( rights == 50 );
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

// Esercizio 5.10: the leaves 3 (1 + 2 + 3) and 5 (1 + 5) get 6 and 6; a
// node alone gets its value, negative values sum to 0, the empty tree is
// left empty
ADS_TEST( "ex5.10" )
{
 std::unique_ptr< BinaryTree< int > > t( make( B( 1 , B( 2 , {} , B( 3 ) ) ,
                                                  B( 5 ) ) ) );
 addSums( t.get() , 0 );
 CHECK( border( t.get() , pre_b< int > ) ==
        Ints( { 1 , 2 , 3 , 6 , 5 , 6 } ) );
 BinaryTree< int > one( -4 );
 addSums( &one , 0 );
 CHECK( border( &one , pre_b< int > ) == Ints( { -4 , -4 } ) );
 std::unique_ptr< BinaryTree< int > > n( make( B( 3 , {} , B( -3 ) ) ) );
 addSums( n.get() , 0 );
 CHECK( border( n.get() , pre_b< int > ) == Ints( { 3 , -3 , 0 } ) );
 addSums( nullptr , 0 );
 }

// Esercizio 5.11: the leaves are the nodes with two children plus one, on
// random trees and on the chains (one leaf, no node with two children)
ADS_TEST( "ex5.11" )
{
 std::mt19937 rng( 511 );
 for( int n = 1 ; n <= 62 ; ++n ) {
  int next = 0;
  std::unique_ptr< BinaryTree< int > > t( n > 60 ? bchain( 40 , n == 61 ) :
                                          random_binary( n , rng , next ) );
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
 while( ! S.empty() ) {
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
 while( u || ! S.empty() ) {
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
 while( ! S.empty() ) {
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

// Esercizio 5.13: the iterative visits give the recursive ones, on random
// trees and on the chains of left and of right children, where the stack
// holds all the nodes (its size, n + 1 and 2 n + 1, is enough)
ADS_TEST( "ex5.13" )
{
 std::mt19937 rng( 513 );
 for( int n : { 0 , 1 , 2 , 3 , 10 , 100 , 1000 , -1000 , -1001 } ) {
  int next = 0;
  std::unique_ptr< BinaryTree< int > > t( n >= 0 ?
   random_binary( n , rng , next ) : bchain( 1000 , n == -1000 ) );
  if( n < 0 )
   n = 1000;
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
