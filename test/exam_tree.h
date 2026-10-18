/*--------------------------------------------------------------------------*/
/*--------------------------- File exam_tree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The helpers of the tests of the exercises on trees of the written exams
 * [see exam_treea.cpp and exam_treeb.cpp]: the binary trees written as in
 * the statements, random trees of several kinds, and the small visits and
 * enumerations that the oracles of both programs share.
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

#ifndef __ADS_exam_tree
 #define __ADS_exam_tree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/ListGraph.h"
#include "ADS/tree/BinaryTree.h"

#include <algorithm>
#include <bit>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::examtree {

using BT = BinaryTree< int >;
using PT = std::unique_ptr< BT >;

/// the infinity of the sums and of the distances
constexpr long long INFTY = std::numeric_limits< long long >::max() / 4;

/// the infinity of the counts and of the heights
constexpr int IINF = std::numeric_limits< int >::max();

/*--------------------------------------------------------------------------*/
/*-------------------------------- TREES -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the parenthesized list of the binary tree written as in the statements,
/// v( L , R ) for the root v with the subtrees L and R, v for a leaf and -
/// for an empty subtree, e.g., 7( 8( 9, 5 ), 3( -, 5 ) )
inline std::string catList( std::string_view s , std::size_t & i )
{
 auto skip = [ & ]( void ) {
  while( ( i < s.size() ) && ( s[ i ] == ' ' ) ) ++i;
  };
 skip();
 if( s[ i ] == '-' ) {                      // an empty subtree, or not
  std::size_t j = i + 1;
  while( ( j < s.size() ) && ( s[ j ] == ' ' ) ) ++j;
  if( ( j == s.size() ) || ( s[ j ] == ',' ) || ( s[ j ] == ')' ) ) {
   i = j;
   return( "()" );
   }
  }
 const std::size_t b = i++;
 while( ( i < s.size() ) && ( s[ i ] != '(' ) && ( s[ i ] != ',' ) &&
        ( s[ i ] != ')' ) && ( s[ i ] != ' ' ) )
  ++i;
 std::string out = "( " + std::string( s.substr( b , i - b ) );
 skip();
 if( ( i < s.size() ) && ( s[ i ] == '(' ) ) {
  ++i;
  out += " " + catList( s , i );
  skip();
  ++i;                                      // the comma
  out += " " + catList( s , i );
  skip();
  ++i;                                      // the closing parenthesis
  }
 else
  out += " () ()";
 return( out + " )" );
 }

/// the binary tree written as in the statements [see catList()]
template< class T = int >
std::unique_ptr< BinaryTree< T > > cat( std::string_view s )
{
 std::size_t i = 0;
 return( std::unique_ptr< BinaryTree< T > >(
          from_list< T >( catList( s , i ) ) ) );
 }

/// the nodes of t in preorder, none if t is nil
template< class T >
std::vector< BinaryTree< T > * > nodesOf( BinaryTree< T > * t )
{
 std::vector< BinaryTree< T > * > N;
 pre_visit( t , [ & ]( BinaryTree< T > * u ) { N.push_back( u ); } );
 return( N );
 }

/// the first node of t in preorder with value v
template< class T >
BinaryTree< T > * findValue( BinaryTree< T > * t , const T & v )
{
 for( auto * u : nodesOf( t ) )
  if( u->read() == v )
   return( u );
 return( nullptr );
 }

/// a random binary tree of n nodes, the size of the left subtree uniform,
/// its values drawn by value()
template< class T , class F >
BinaryTree< T > * randomTree( std::mt19937 & g , int n , F && value )
{
 if( n == 0 )
  return( nullptr );
 const int l = std::uniform_int_distribution< int >( 0 , n - 1 )( g );
 auto * t = new BinaryTree< T >( value() );
 if( auto * L = randomTree< T >( g , l , value ) )
  t->insertLeft( L );
 if( auto * R = randomTree< T >( g , n - 1 - l , value ) )
  t->insertRight( R );
 return( t );
 }

/// a random binary tree of n nodes with values in [ lo , hi ]
inline PT randomInts( std::mt19937 & g , int n , int lo , int hi )
{
 std::uniform_int_distribution< int > V( lo , hi );
 return( PT( randomTree< int >( g , n , [ & ]( void ) {
  return( V( g ) ); } ) ) );
 }

/// a random binary tree of n nodes with the distinct values lo , ... ,
/// lo + n - 1 in random places
inline PT randomDistinct( std::mt19937 & g , int n , int lo = 1 )
{
 PT t = randomInts( g , n , 0 , 0 );
 std::vector< int > V( n );
 std::iota( V.begin() , V.end() , lo );
 std::shuffle( V.begin() , V.end() , g );
 int i = 0;
 for( auto * u : nodesOf( t.get() ) )
  u->write( V[ i++ ] );
 return( t );
 }

/// a random full binary tree (each node with 0 or 2 children) with k
/// internal nodes, all values v
template< class T >
BinaryTree< T > * randomFull( std::mt19937 & g , int k , const T & v )
{
 auto * t = new BinaryTree< T >( v );
 if( k > 0 ) {
  const int l = std::uniform_int_distribution< int >( 0 , k - 1 )( g );
  t->insertLeft( randomFull( g , l , v ) );
  t->insertRight( randomFull( g , k - 1 - l , v ) );
  }
 return( t );
 }

/// the binary tree of a complete tree stored by levels in A, from place i
template< class T >
BinaryTree< T > * completeTree( const std::vector< T > & A ,
                                std::size_t i = 0 )
{
 if( i >= A.size() )
  return( nullptr );
 auto * t = new BinaryTree< T >( A[ i ] );
 if( auto * L = completeTree( A , 2 * i + 1 ) )
  t->insertLeft( L );
 if( auto * R = completeTree( A , 2 * i + 2 ) )
  t->insertRight( R );
 return( t );
 }

/// the number of children of t
template< class T >
int degreeOf( const BinaryTree< T > * t )
{
 return( ( t->left() != nullptr ) + ( t->right() != nullptr ) );
 }

/// true if t has no child
template< class T >
bool isLeafNode( const BinaryTree< T > * t )
{
 return( degreeOf( t ) == 0 );
 }

/// the depth of t, by the links to the parents
template< class T >
int depthOf( const BinaryTree< T > * t )
{
 int d = 0;
 for( ; t->parent() ; t = t->parent() )
  ++d;
 return( d );
 }

/// the number of nodes of the subtree of t
template< class T >
int sizeOf( BinaryTree< T > * t )
{
 return( int( nodesOf( t ).size() ) );
 }

/// the height of t in edges, -1 if t is nil
template< class T >
int heightOf( BinaryTree< T > * t )
{
 int h = -1;
 for( auto * u : nodesOf( t ) )
  h = std::max( h , depthOf( u ) - depthOf( t ) );
 return( h );
 }

/// the sum of the values of the subtree of t
inline long long sumOf( BT * t )
{
 long long s = 0;
 for( auto * u : nodesOf( t ) )
  s += u->read();
 return( s );
 }

/// the leaves of the subtree of t
template< class T >
std::vector< BinaryTree< T > * > leavesOf( BinaryTree< T > * t )
{
 std::vector< BinaryTree< T > * > L;
 for( auto * u : nodesOf( t ) )
  if( isLeafNode( u ) )
   L.push_back( u );
 return( L );
 }

/// the shape of t, its values forgotten
template< class T >
std::string shapeOf( const BinaryTree< T > * t )
{
 if( ! t )
  return( "." );
 return( "(" + shapeOf( t->left() ) + shapeOf( t->right() ) + ")" );
 }

/// the shape of the mirror image of t
template< class T >
std::string mirrorShapeOf( const BinaryTree< T > * t )
{
 if( ! t )
  return( "." );
 return( "(" + mirrorShapeOf( t->right() ) + mirrorShapeOf( t->left() ) +
         ")" );
 }

/// true if a is an ancestor of u, u included
template< class T >
bool isAncestor( const BinaryTree< T > * a , const BinaryTree< T > * u )
{
 for( ; u ; u = u->parent() )
  if( u == a )
   return( true );
 return( false );
 }

/// the unoriented graph of the tree t, the nodes numbered as in N
inline ListGraph<> graphOf( const std::vector< BT * > & N )
{
 ListGraph<> G( int( N.size() ) , false );
 std::map< const BT * , int > id;
 for( std::size_t i = 0 ; i < N.size() ; ++i )
  id[ N[ i ] ] = int( i );
 for( std::size_t i = 0 ; i < N.size() ; ++i )
  if( N[ i ]->parent() )
   G.insertEdge( id[ N[ i ]->parent() ] , int( i ) );
 return( G );
 }

/// the heaviest path between two leaves of t, possibly the same if same,
/// through the lowest common ancestor of every pair
inline long long leafPathsBrute( BT * t , bool same )
{
 long long best = -INFTY;
 const auto L = leavesOf( t );
 for( auto * a : L )
  for( auto * b : L ) {
   if( ( a == b ) && ! same )
    continue;
   BT * c = a;
   while( ! isAncestor( c , b ) )
    c = c->parent();
   long long s = c->read();
   for( auto * u = a ; u != c ; u = u->parent() )
    s += u->read();
   for( auto * u = b ; u != c ; u = u->parent() )
    s += u->read();
   best = std::max( best , s );
   }
 return( best );
 }

}  // end( namespace ADS::examtree )

/*--------------------------------------------------------------------------*/

#endif  /* exam_tree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File exam_tree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
