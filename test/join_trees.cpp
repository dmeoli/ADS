/*--------------------------------------------------------------------------*/
/*-------------------------- File join_trees.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the balanced trees based on join, beyond the book: split(),
 * join() and join2() on the AVL tree, the Red-Black tree and the treap of
 * ADS and on the four schemes of JoinTree, the bulk operations of the
 * generic layer (union, intersection, difference, batches of insertions
 * and of removals, filter, map and reduce, extraction of a range) against
 * std::set and std::map, the augmented maps against brute force, the
 * interval tree and the range tree, and the same operations on the
 * executors Sequential and ThreadPool with 1, 2 and 8 threads, whose
 * results must be equal node by node. The invariant of each scheme (the
 * heights of AVL, the black heights and the colors of Red-Black, the
 * weights, the heap order of the priorities) and the sizes of the
 * subtrees are checked after every operation by the contracts at the
 * level 3; the heights are also checked against the bound of each scheme.
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

#include "ADS/geometry/IntervalTree.h"
#include "ADS/geometry/RangeTree.h"
#include "ADS/parallel/ThreadPool.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/JoinTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/Treap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Pairs = std::vector< std::pair< int , int > >;

/// the pairs of a tree by increasing keys
template< class T >
Pairs pairs( const T & t )
{
 Pairs P;
 t.forEach( [ & ]( const int & k , const int & v ) {
  P.emplace_back( k , v ); } );
 return( P );
 }

/// the pairs of a std::map
Pairs pairs( const std::map< int , int > & m )
{
 return( Pairs( m.begin() , m.end() ) );
 }

/// a tree of the pairs of m, one insertion at a time up to 1000 pairs
/// (the invariant, checked after each one, costs \f$ O( n ) \f$), by
/// multiInsert() beyond
template< class T >
T make( const std::map< int , int > & m )
{
 T t;
 if( m.size() > 1000 ) {
  const Pairs P( m.begin() , m.end() );
  t.multiInsert( P );
  }
 else
  for( const auto & [ k , v ] : m )
   t.insert( k , v );
 return( t );
 }

/// n random pairs with keys in [ 0 , range )
std::map< int , int > randomMap( std::mt19937_64 & g , std::size_t n ,
                                 int range )
{
 std::map< int , int > m;
 for( std::size_t i = 0 ; i < n ; ++i )
  m[ int( g() % std::uint64_t( range ) ) ] = int( g() % 1000 );
 return( m );
 }

/// the height of a tree, -1 if empty; by the links for a SearchTree
template< class T >
int heightOf( const T & t )
{
 if constexpr( requires { t.height(); } )
  return( t.height() );
 else {
  auto h = [ & ]( auto & self , const auto * u ) -> int {
   return( t.isNil( u ) ? -1 : 1 + std::max( self( self , u->left ) ,
                                             self( self , u->right ) ) );
   };
  return( h( h , t.root() ) );
  }
 }

/// the bound on the height of a tree of n keys, by its scheme
template< class T >
void checkHeight( const T & t )
{
 const double n = double( t.size() );
 const int h = heightOf( t );
 if( t.size() == 0 ) {
  CHECK( h == -1 );
  return;
  }
 if constexpr( std::is_same_v< T , AVLTree< int , int > > ||
               std::is_same_v< T , JoinAVLTree< int , int > > )
  CHECK( h <= 1.4405 * std::log2( n + 2 ) - 0.3277 + 1e-9 );
 else if constexpr( std::is_same_v< T , RedBlackTree< int , int > > ||
                    std::is_same_v< T , JoinRedBlackTree< int , int > > )
  CHECK( h <= 2 * std::log2( n + 1 ) );
 else if constexpr( std::is_same_v< T , WeightBalancedTree< int , int > > )
  CHECK( h + 1 <= std::log( n + 1 ) / std::log( 1 / ( 1 - 0.29 ) ) + 1 );
 else                                     // a treap: with high probability
  CHECK( h + 1 <= 6 * std::log2( n + 2 ) );
 }


/*--------------------------------------------------------------------------*/
/*--------------------------- SPLIT AND JOIN -------------------------------*/
/*--------------------------------------------------------------------------*/

/// split() and join() at every place of a tree of keys 0, 2, ..., 2 n - 2,
/// the absent keys in between and beyond included
template< class T >
void splitJoinEdges( void )
{
 for( int n : { 0 , 1 , 2 , 3 , 7 , 40 } ) {
  std::map< int , int > m;
  for( int i = 0 ; i < n ; ++i )
   m[ 2 * i ] = i;
  for( int k = -2 ; k <= 2 * n ; ++k ) {
   T t = make< T >( m );
   auto [ l , v , r ] = std::move( t ).split( k );
   checkHeight( l );
   checkHeight( r );
   CHECK( v.has_value() == m.contains( k ) );
   if( v )
    CHECK( *v == m.at( k ) );
   for( const auto & [ a , b ] : pairs( l ) )
    CHECK( a < k );
   for( const auto & [ a , b ] : pairs( r ) )
    CHECK( a > k );
   CHECK( l.size() + r.size() + ( v ? 1 : 0 ) == m.size() );
   T u = v ? T::join( std::move( l ) , k , *v , std::move( r ) )
           : T::join2( std::move( l ) , std::move( r ) );
   CHECK( pairs( u ) == pairs( m ) );
   checkHeight( u );
   }
  }
 // a join with an empty side, both ways, and of two empty trees
 for( int n : { 1 , 5 , 100 , 1000 } ) {
  std::map< int , int > m;
  for( int i = 1 ; i <= n ; ++i )
   m[ i ] = -i;
  T a = T::join( make< T >( m ) , n + 1 , 7 , T() );
  checkHeight( a );
  CHECK( a.size() == std::size_t( n + 1 ) );
  CHECK( a.lookup( n + 1 ) == 7 );
  T b = T::join( T() , 0 , 9 , make< T >( m ) );
  checkHeight( b );
  CHECK( b.lookup( 0 ) == 9 );
  T c = T::join2( make< T >( m ) , T() );
  CHECK( pairs( c ) == pairs( m ) );
  T d = T::join2( T() , make< T >( m ) );
  CHECK( pairs( d ) == pairs( m ) );
  }
 T e = T::join2( T() , T() );
 CHECK( e.size() == 0 );
 T f = T::join( T() , 5 , 6 , T() );
 CHECK( ( f.size() == 1 ) && ( f.lookup( 5 ) == 6 ) );
 // trees of very different heights: 1 key against 2^14
 std::map< int , int > big;
 for( int i = 0 ; i < ( 1 << 14 ) ; ++i )
  big[ i ] = i;
 std::map< int , int > one = { { 1 << 20 , 1 } };
 T g = T::join( make< T >( big ) , 1 << 19 , 0 , make< T >( one ) );
 checkHeight( g );
 CHECK( g.size() == big.size() + 2 );
 std::map< int , int > two = { { ( 1 << 20 ) + 2 , 2 } };
 T h = T::join( T() , ( 1 << 20 ) + 1 , 0 , make< T >( two ) );
 h = T::join2( std::move( g ) , std::move( h ) );
 checkHeight( h );
 CHECK( h.size() == big.size() + 4 );
 // the preconditions: keys not in order
 CHECK_THROWS( T::join( make< T >( big ) , 5 , 0 , T() ) ,
               contract_violation );
 CHECK_THROWS( T::join2( make< T >( big ) , make< T >( big ) ) ,
               contract_violation );
 }

/// random splits and joins against std::map
template< class T >
void splitJoinRandom( void )
{
 std::mt19937_64 g( 17 );
 for( int it = 0 ; it < 60 ; ++it ) {
  const auto m = randomMap( g , g() % 2000 , 4000 );
  const int k = int( g() % 4200 ) - 100;
  T t = make< T >( m );
  auto [ l , v , r ] = std::move( t ).split( k );
  std::map< int , int > ml( m.begin() , m.lower_bound( k ) ) ,
                        mr( m.upper_bound( k ) , m.end() );
  CHECK( pairs( l ) == pairs( ml ) );
  CHECK( pairs( r ) == pairs( mr ) );
  checkHeight( l );
  checkHeight( r );
  // the two sides joined in the other order of their roles
  T u = T::join2( std::move( l ) , std::move( r ) );
  if( v ) ml[ k ] = *v;
  ml.insert( mr.begin() , mr.end() );
  if( v ) u.insert( k , *v );
  CHECK( pairs( u ) == pairs( m ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- SET OPERATIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// union, intersection and difference of a and b by T, and by std::map
template< class T , class E >
void setOps( E & ex , const std::map< int , int > & a ,
             const std::map< int , int > & b )
{
 auto sum = []( const int & x , const int & y ) { return( x + 10 * y ); };
 {
  T x = make< T >( a ) , y = make< T >( b );
  x.unionWith( ex , std::move( y ) , sum );
  std::map< int , int > r = a;
  for( const auto & [ k , v ] : b )
   r[ k ] = a.contains( k ) ? sum( a.at( k ) , v ) : v;
  CHECK( pairs( x ) == pairs( r ) );
  checkHeight( x );
  }
 {
  T x = make< T >( a ) , y = make< T >( b );
  x.unionWith( ex , std::move( y ) );   // the value of b by default
  std::map< int , int > r = a;
  for( const auto & [ k , v ] : b )
   r[ k ] = v;
  CHECK( pairs( x ) == pairs( r ) );
  }
 {
  T x = make< T >( a ) , y = make< T >( b );
  x.intersectWith( ex , std::move( y ) , sum );
  std::map< int , int > r;
  for( const auto & [ k , v ] : a )
   if( b.contains( k ) )
    r[ k ] = sum( v , b.at( k ) );
  CHECK( pairs( x ) == pairs( r ) );
  checkHeight( x );
  }
 {
  T x = make< T >( a ) , y = make< T >( b );
  x.intersectWith( ex , std::move( y ) );   // the value of a by default
  std::map< int , int > r;
  for( const auto & [ k , v ] : a )
   if( b.contains( k ) )
    r[ k ] = v;
  CHECK( pairs( x ) == pairs( r ) );
  }
 {
  T x = make< T >( a ) , y = make< T >( b );
  x.differenceWith( ex , std::move( y ) );
  std::map< int , int > r;
  for( const auto & [ k , v ] : a )
   if( ! b.contains( k ) )
    r[ k ] = v;
  CHECK( pairs( x ) == pairs( r ) );
  checkHeight( x );
  }
 }

/// the set operations on the edge cases and on random sets
template< class T >
void setOperations( void )
{
 Sequential seq;
 const std::map< int , int > none , one = { { 5 , 1 } } ,
                             same = { { 5 , 2 } };
 std::map< int , int > evens , odds , all , few;
 for( int i = 0 ; i < 3000 ; ++i ) {
  ( i % 2 ? odds : evens )[ i ] = i;
  all[ i ] = -i;
  }
 for( int i : { -7 , 0 , 1500 , 2999 , 5000 } )
  few[ i ] = 1;
 const std::vector< std::pair< std::map< int , int > ,
                              std::map< int , int > > > cases = {
  { none , none } , { none , all } , { all , none } , { one , one } ,
  { one , same } , { one , all } , { all , one } , { evens , odds } ,
  { odds , evens } , { all , all } , { evens , all } , { all , evens } ,
  { few , all } , { all , few } , { few , few } };
 for( const auto & [ a , b ] : cases )
  setOps< T >( seq , a , b );
 std::mt19937_64 g( 3 );
 for( int it = 0 ; it < 40 ; ++it ) {
  const std::size_t n = g() % 3000 , m = it % 4 ? g() % 3000 : g() % 10;
  setOps< T >( seq , randomMap( g , n , 5000 ) , randomMap( g , m , 5000 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BATCHES ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// multiInsert(), multiRemove(), filter(), mapReduce() and extractRange()
/// against std::map
template< class T , class E >
void batchOps( E & ex , std::mt19937_64 & g , std::size_t n , std::size_t m )
{
 const auto a = randomMap( g , n , int( 2 * ( n + m ) + 1 ) );
 const auto b = randomMap( g , m , int( 2 * ( n + m ) + 1 ) );
 const Pairs B( b.begin() , b.end() );
 {
  T x = make< T >( a );
  x.multiInsert( ex , B );
  std::map< int , int > r = a;
  for( const auto & [ k , v ] : b )
   r[ k ] = v;
  CHECK( pairs( x ) == pairs( r ) );
  checkHeight( x );
  }
 {
  T x = make< T >( a );
  x.multiInsert( ex , B , []( const int & u , const int & w ) {
   return( u - w ); } );
  std::map< int , int > r = a;
  for( const auto & [ k , v ] : b )
   r[ k ] = a.contains( k ) ? a.at( k ) - v : v;
  CHECK( pairs( x ) == pairs( r ) );
  }
 {
  T x = make< T >( a );
  std::vector< int > K;
  for( const auto & kv : b )
   K.push_back( kv.first );
  x.multiRemove( ex , K );
  std::map< int , int > r = a;
  for( int k : K )
   r.erase( k );
  CHECK( pairs( x ) == pairs( r ) );
  checkHeight( x );
  }
 {
  T x = make< T >( a );
  x.filter( ex , []( const int & k , const int & v ) {
   return( ( k + v ) % 3 != 0 ); } );
  std::map< int , int > r;
  for( const auto & [ k , v ] : a )
   if( ( k + v ) % 3 != 0 )
    r[ k ] = v;
  CHECK( pairs( x ) == pairs( r ) );
  checkHeight( x );
  const long long s = x.mapReduce( ex ,
   []( const int & k , const int & v ) { return( 1LL * k * v ); } ,
   []( long long u , long long w ) { return( u + w ); } , 0LL );
  long long t = 0;
  for( const auto & [ k , v ] : r )
   t += 1LL * k * v;
  CHECK( s == t );
  // the order of the reduction: the keys concatenated
  const std::string c = x.mapReduce( ex ,
   []( const int & k , const int & ) { return( std::to_string( k ) + ";" ); } ,
   []( std::string u , const std::string & w ) { return( u + w ); } ,
   std::string() );
  std::string d;
  for( const auto & kv : r )
   d += std::to_string( kv.first ) + ";";
  CHECK( c == d );
  }
 }

/// the batches on their edge cases and at random
template< class T >
void batches( void )
{
 Sequential seq;
 std::mt19937_64 g( 5 );
 for( auto [ n , m ] : std::vector< std::pair< std::size_t ,
                                               std::size_t > >{
       { 0 , 0 } , { 0 , 1 } , { 1 , 0 } , { 0 , 500 } , { 500 , 0 } ,
       { 1 , 1 } , { 2000 , 3 } , { 3 , 2000 } , { 1500 , 1500 } } )
  batchOps< T >( seq , g , n , m );
 // every key of the batch already there, and all of the tree removed
 std::map< int , int > a;
 for( int i = 0 ; i < 1000 ; ++i )
  a[ i ] = i;
 T x = make< T >( a );
 const Pairs B( a.begin() , a.end() );
 x.multiInsert( B , []( const int & u , const int & w ) {
  return( u + w ); } );
 for( const auto & [ k , v ] : pairs( x ) )
  CHECK( v == 2 * k );
 std::vector< int > K;
 for( int i = -5 ; i < 1005 ; ++i )
  K.push_back( i );
 x.multiRemove( K );
 CHECK( x.size() == 0 );
 // a batch not sorted, or with a key twice: an audit
 T y;
 const Pairs U = { { 3 , 0 } , { 1 , 0 } } , D = { { 1 , 0 } , { 1 , 1 } };
 CHECK_THROWS( y.multiInsert( U ) , contract_violation );
 CHECK_THROWS( y.multiInsert( D ) , contract_violation );
 }

/// extractRange() at the edges: empty, outside, one key, all, lo > hi
template< class T >
void ranges( void )
{
 std::map< int , int > m;
 for( int i = 0 ; i < 200 ; i += 2 )
  m[ i ] = i;
 const std::vector< std::pair< int , int > > Q = {
  { -10 , -1 } , { -10 , 0 } , { 0 , 0 } , { 1 , 1 } , { 0 , 198 } ,
  { -5 , 500 } , { 198 , 198 } , { 199 , 300 } , { 50 , 20 } , { 31 , 77 } ,
  { 30 , 78 } };
 for( const auto & [ lo , hi ] : Q ) {
  T t = make< T >( m );
  T r = t.extractRange( lo , hi );
  std::map< int , int > in , out;
  for( const auto & [ k , v ] : m )
   ( ( lo <= k ) && ( k <= hi ) ? in : out )[ k ] = v;
  CHECK( pairs( r ) == pairs( in ) );
  CHECK( pairs( t ) == pairs( out ) );
  checkHeight( r );
  checkHeight( t );
  }
 T e;
 T r = e.extractRange( 0 , 10 );
 CHECK( ( r.size() == 0 ) && ( e.size() == 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXECUTORS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the shape of a JoinTree, by a preorder of keys and subtree sizes
template< class T >
std::vector< std::pair< int , std::size_t > > shape( const T & t )
{
 std::vector< std::pair< int , std::size_t > > S;
 auto rec = [ & ]( auto & self , const auto * u ) -> void {
  if( ! u ) return;
  S.emplace_back( u->key , u->size );
  self( self , u->left );
  self( self , u->right );
  };
 rec( rec , t.root() );
 return( S );
 }

/// the bulk operations on large trees, sequential and on the pool, which
/// must give the same pairs (and, for JoinTree, the same shape)
template< class T >
void executors( void )
{
 ThreadPool p1( 1 ) , p2( 2 ) , p8( 8 );
 Sequential seq;
 std::mt19937_64 g( 11 );
 for( int it = 0 ; it < 3 ; ++it ) {
  const auto a = randomMap( g , 30000 , 200000 ) ,
             b = randomMap( g , it == 2 ? 50 : 20000 , 200000 );
  setOps< T >( seq , a , b );
  setOps< T >( p1 , a , b );
  setOps< T >( p2 , a , b );
  setOps< T >( p8 , a , b );
  std::mt19937_64 g1( it ) , g2( it ) , g3( it );
  batchOps< T >( p2 , g1 , 20000 , 15000 );
  batchOps< T >( p8 , g2 , 20000 , 15000 );
  batchOps< T >( p8 , g3 , 50 , 30000 );
  }
 if constexpr( requires( const T & t ) { t.root()->key; } ) {
  const auto a = randomMap( g , 40000 , 100000 ) ,
             b = randomMap( g , 40000 , 100000 );
  T x1 = make< T >( a ) , x2 = make< T >( a );
  x1.unionWith( seq , make< T >( b ) );
  x2.unionWith( p8 , make< T >( b ) );
  CHECK( shape( x1 ) == shape( x2 ) );
  const Pairs B( b.begin() , b.end() );
  T y1 = make< T >( a ) , y2 = make< T >( a );
  y1.multiInsert( seq , B );
  y2.multiInsert( p8 , B );
  CHECK( shape( y1 ) == shape( y2 ) );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ AUGMENTED ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// a monoid that is not commutative: the hash of the sequence of the keys
/// and its length, ( h , l ) * ( h' , l' ) = ( h B^{l'} + h' , l + l' )
struct SequenceHash {
 struct Value {
  std::uint64_t h = 0 , p = 1;           // the hash and B^length
  bool operator==( const Value & ) const = default;
  };
 static constexpr std::uint64_t kB = 1000003;
 static Value identity( void ) { return( Value() ); }
 static Value from( const int & k , const int & ) {
  return( Value{ std::uint64_t( k ) + 1 , kB } );
  }
 static Value combine( const Value & a , const Value & b ) {
  return( Value{ a.h * b.p + b.h , a.p * b.p } );
  }
 };

/// augRange(), augPrefix() and augValue() against the brute force, after
/// bulk operations too
template< class B >
void augmented( void )
{
 using SumMap = JoinTree< int , int , B , SumAugmentation< long long > >;
 using MaxMap = JoinTree< int , int , B , MaxAugmentation< int > >;
 using SeqMap = JoinTree< int , int , B , SequenceHash >;
 static_assert( Augmentation< SumAugmentation< long long > , int , int > );
 std::mt19937_64 g( 23 );
 for( int it = 0 ; it < 12 ; ++it ) {
  auto a = randomMap( g , g() % 600 , 1000 ) ,
       b = randomMap( g , g() % 600 , 1000 );
  SumMap s = make< SumMap >( a );
  MaxMap x = make< MaxMap >( a );
  SeqMap q = make< SeqMap >( a );
  if( it % 3 == 1 ) {              // after a union
   s.unionWith( make< SumMap >( b ) );
   x.unionWith( make< MaxMap >( b ) );
   q.unionWith( make< SeqMap >( b ) );
   for( const auto & [ k , v ] : b ) a[ k ] = v;
   }
  if( it % 3 == 2 ) {              // after a difference
   s.differenceWith( make< SumMap >( b ) );
   x.differenceWith( make< MaxMap >( b ) );
   q.differenceWith( make< SeqMap >( b ) );
   for( const auto & kv : b ) a.erase( kv.first );
   }
  for( int t = 0 ; t < 200 ; ++t ) {
   const int lo = int( g() % 1100 ) - 50 , hi = int( g() % 1100 ) - 50;
   long long sum = 0;
   int mx = std::numeric_limits< int >::lowest();
   SequenceHash::Value h;
   for( const auto & [ k , v ] : a )
    if( ( lo <= k ) && ( k <= hi ) ) {
     sum += v;
     mx = std::max( mx , v );
     h = SequenceHash::combine( h , SequenceHash::from( k , v ) );
     }
   CHECK( s.augRange( lo , hi ) == sum );
   CHECK( x.augRange( lo , hi ) == mx );
   CHECK( q.augRange( lo , hi ) == h );
   long long pre = 0;
   for( const auto & [ k , v ] : a )
    if( k <= hi ) pre += v;
   CHECK( s.augPrefix( hi ) == pre );
   }
  long long all = 0;
  for( const auto & kv : a ) all += kv.second;
  CHECK( s.augValue() == all );
  }
 SumMap e;
 CHECK( ( e.augValue() == 0 ) && ( e.augRange( 0 , 10 ) == 0 ) &&
        ( e.augPrefix( 3 ) == 0 ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "split-join-edges" )
{
 splitJoinEdges< AVLTree< int , int > >();
 splitJoinEdges< RedBlackTree< int , int > >();
 splitJoinEdges< Treap< int , int > >();
 splitJoinEdges< JoinAVLTree< int , int > >();
 splitJoinEdges< JoinRedBlackTree< int , int > >();
 splitJoinEdges< WeightBalancedTree< int , int > >();
 splitJoinEdges< JoinTreap< int , int > >();
 }

ADS_TEST( "split-join-random" )
{
 splitJoinRandom< AVLTree< int , int > >();
 splitJoinRandom< RedBlackTree< int , int > >();
 splitJoinRandom< Treap< int , int > >();
 splitJoinRandom< JoinAVLTree< int , int > >();
 splitJoinRandom< JoinRedBlackTree< int , int > >();
 splitJoinRandom< WeightBalancedTree< int , int > >();
 splitJoinRandom< JoinTreap< int , int > >();
 }

ADS_TEST( "set-operations" )
{
 setOperations< AVLTree< int , int > >();
 setOperations< RedBlackTree< int , int > >();
 setOperations< Treap< int , int > >();
 setOperations< JoinAVLTree< int , int > >();
 setOperations< JoinRedBlackTree< int , int > >();
 setOperations< WeightBalancedTree< int , int > >();
 setOperations< JoinTreap< int , int > >();
 }

ADS_TEST( "batches" )
{
 batches< AVLTree< int , int > >();
 batches< RedBlackTree< int , int > >();
 batches< Treap< int , int > >();
 batches< JoinAVLTree< int , int > >();
 batches< JoinRedBlackTree< int , int > >();
 batches< WeightBalancedTree< int , int > >();
 batches< JoinTreap< int , int > >();
 }

ADS_TEST( "extract-range" )
{
 ranges< AVLTree< int , int > >();
 ranges< RedBlackTree< int , int > >();
 ranges< Treap< int , int > >();
 ranges< JoinAVLTree< int , int > >();
 ranges< JoinRedBlackTree< int , int > >();
 ranges< WeightBalancedTree< int , int > >();
 ranges< JoinTreap< int , int > >();
 }

ADS_TEST( "join-tree-dictionary" )
{
 // the dictionary of JoinTree, its copies and its iterators, against
 // std::map, on the four schemes
 auto run = []< class T >( T * ) {
  std::mt19937_64 g( 29 );
  T t;
  std::map< int , int > m;
  for( int i = 0 ; i < 3000 ; ++i ) {
   const int k = int( g() % 700 );
   if( g() % 3 ) { t.insert( k , i ); m[ k ] = i; }
   else { t.remove( k ); m.erase( k ); }
   CHECK( t.size() == m.size() );
   }
  for( int k = -1 ; k < 702 ; ++k )
   CHECK( t.lookup( k ) == ( m.contains( k ) ? std::optional< int >(
                                m.at( k ) ) : std::nullopt ) );
  checkHeight( t );
  T c( t );
  c.insert( 1000 , 0 );
  CHECK( ( c.size() == t.size() + 1 ) && ( ! t.contains( 1000 ) ) );
  Pairs P;
  for( const auto & [ k , v ] : t )
   P.emplace_back( k , v );
  CHECK( P == pairs( m ) );
  t.clear();
  CHECK( t.empty() && ( t.height() == -1 ) );
  };
 run( ( JoinAVLTree< int , int > * ) nullptr );
 run( ( JoinRedBlackTree< int , int > * ) nullptr );
 run( ( WeightBalancedTree< int , int > * ) nullptr );
 run( ( JoinTreap< int , int > * ) nullptr );
 }

ADS_TEST( "augmented-maps" )
{
 augmented< AVLBalance >();
 augmented< RedBlackBalance >();
 augmented< WeightBalance >();
 augmented< TreapBalance >();
 }

ADS_TEST( "interval-tree" )
{
 std::mt19937_64 g( 31 );
 for( int it = 0 ; it < 20 ; ++it ) {
  std::vector< std::pair< int , int > > I;
  const std::size_t n = it == 0 ? 0 : g() % 400;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const int a = int( g() % 1000 ) , b = a + int( g() % ( it % 2 ? 5 : 200 ) );
   I.emplace_back( a , b );
   if( g() % 5 == 0 ) I.emplace_back( a , b );   // copies
   }
  ThreadPool pool( 4 );
  IntervalTree< int > T1( I ) , T2( pool , I );
  for( int r = 0 ; r < 30 && ! I.empty() ; ++r ) {   // updates
   const auto i = I[ g() % I.size() ];
   T1.remove( i.first , i.second );
   T2.remove( i.first , i.second );
   I.erase( std::find( I.begin() , I.end() , i ) );
   const int a = int( g() % 1000 );
   T1.insert( a , a + 3 );
   T2.insert( a , a + 3 );
   I.emplace_back( a , a + 3 );
   }
  CHECK( ( T1.size() == I.size() ) && ( T2.size() == I.size() ) );
  for( int x = -2 ; x < 1250 ; x += ( it % 3 ) + 1 ) {
   std::vector< std::pair< int , int > > S , R1 , R2;
   for( const auto & i : I )
    if( ( i.first <= x ) && ( x <= i.second ) )
     S.push_back( i );
   std::sort( S.begin() , S.end() );
   T1.stab( x , [ & ]( int a , int b ) { R1.emplace_back( a , b ); } );
   T2.stab( x , [ & ]( int a , int b ) { R2.emplace_back( a , b ); } );
   CHECK( ( R1 == S ) && ( R2 == S ) );
   CHECK( T1.stabbed( x ) == ! S.empty() );
   CHECK( T1.count( x ) == S.size() );
   }
  }
 IntervalTree< double > D;
 D.insert( 0.5 , 0.5 );                      // a point
 CHECK( D.stabbed( 0.5 ) && ! D.stabbed( 0.49 ) && ! D.stabbed( 0.51 ) );
 CHECK_THROWS( D.insert( 2 , 1 ) , contract_violation );
 }

ADS_TEST( "range-tree" )
{
 std::mt19937_64 g( 37 );
 for( int it = 0 ; it < 12 ; ++it ) {
  std::vector< std::pair< int , int > > P;
  const std::size_t n = it == 0 ? 0 : g() % 500;
  const int side = it % 2 ? 20 : 1000;      // many equal coordinates
  for( std::size_t i = 0 ; i < n ; ++i )
   P.emplace_back( int( g() % side ) , int( g() % side ) );
  ThreadPool pool( 4 );
  RangeTree< int > R( P ) , Q( pool , P );
  for( int r = 0 ; r < 10 && ! P.empty() ; ++r ) {
   const auto p = P[ g() % P.size() ];
   R.remove( p.first , p.second );
   P.erase( std::find( P.begin() , P.end() , p ) );
   R.insert( r , -r );
   P.emplace_back( r , -r );
   }
  CHECK( ( R.size() == P.size() ) );
  for( int q = 0 ; q < 300 ; ++q ) {
   const auto w = std::uint64_t( side + 10 );
   int x1 = int( g() % w ) - 5 , x2 = int( g() % w ) - 5 ,
       y1 = int( g() % ( w + 10 ) ) - 15 , y2 = int( g() % w ) - 5;
   if( q % 7 == 0 ) std::swap( x1 , x2 );   // empty rectangles too
   std::size_t c = 0;
   for( const auto & [ x , y ] : P )
    c += ( x1 <= x ) && ( x <= x2 ) && ( y1 <= y ) && ( y <= y2 );
   CHECK( R.count( x1 , x2 , y1 , y2 ) == c );
   }
  }
 RangeTree< int > E;
 CHECK( E.count( std::numeric_limits< int >::lowest() ,
                 std::numeric_limits< int >::max() ,
                 std::numeric_limits< int >::lowest() ,
                 std::numeric_limits< int >::max() ) == 0 );
 E.insert( std::numeric_limits< int >::max() ,
           std::numeric_limits< int >::lowest() );
 CHECK( E.count( 0 , std::numeric_limits< int >::max() ,
                 std::numeric_limits< int >::lowest() , 0 ) == 1 );
 }

ADS_TEST( "infinite-bounds" )
{
 // the points, the intervals and the values at the infinities of a
 // floating type, which no finite sentinel bounds
 constexpr double inf = std::numeric_limits< double >::infinity();
 RangeTree< double > R( { { 0 , -inf } , { 0 , inf } , { 1 , -inf } ,
                          { 1 , 0 } , { 1 , inf } , { 2 , inf } ,
                          { -inf , 5 } , { inf , 5 } , { -inf , -inf } ,
                          { inf , inf } } );
 CHECK( R.count( 0 , 1 , -inf , inf ) == 5 );
 CHECK( R.count( 1 , 1 , -inf , -inf ) == 1 );
 CHECK( R.count( 0 , 2 , inf , inf ) == 3 );
 CHECK( R.count( -inf , inf , 5 , 5 ) == 2 );
 CHECK( R.count( -inf , -inf , -inf , inf ) == 2 );
 CHECK( R.count( inf , inf , -inf , inf ) == 2 );
 CHECK( R.count( -inf , inf , -inf , inf ) == 10 );
 IntervalTree< double > I( { { -inf , 0 } , { 5 , inf } , { inf , inf } ,
                             { -inf , -inf } } );
 CHECK( ( I.count( -inf ) == 2 ) && I.stabbed( -inf ) );
 CHECK( ( I.count( inf ) == 2 ) && I.stabbed( inf ) );
 CHECK( ( I.count( 3 ) == 0 ) && ! I.stabbed( 3 ) );
 JoinTree< int , double , AVLBalance , MaxAugmentation< double > > M;
 M.insert( 1 , -inf );
 M.insert( 2 , -inf );
 CHECK( M.augValue() == -inf );
 CHECK( M.augRange( 1 , 2 ) == -inf );
 CHECK( M.augPrefix( 1 ) == -inf );
 M.insert( 3 , std::numeric_limits< double >::lowest() );
 CHECK( M.augValue() == std::numeric_limits< double >::lowest() );
 }

ADS_TEST( "join-executors" )
{
 executors< AVLTree< int , int > >();
 executors< RedBlackTree< int , int > >();
 executors< Treap< int , int > >();
 executors< JoinAVLTree< int , int > >();
 executors< JoinRedBlackTree< int , int > >();
 executors< WeightBalancedTree< int , int > >();
 executors< JoinTreap< int , int > >();
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ MAIN --------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- End File join_trees.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
