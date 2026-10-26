/*--------------------------------------------------------------------------*/
/*------------------------- File exam_compiti.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the problems of the written exams that are not in the thematic
 * collections: for each one the solution of the text, in the namespace
 * exams, or the function of the library that already solves it, checked
 * on the examples of the text, against an independent oracle (a brute
 * force, an exhaustive enumeration, the standard library) on random small
 * instances, and on the edge cases; the recurrences of the exercises on
 * complexity are evaluated numerically and their orders checked by ratios
 * and by master(). Where the text has an error, the function is the
 * correct version and its comment says what differs.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025,
 *      https://algoritmicamente.it/materiali/libro/
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/algorithm/MasterTheorem.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Matching.h"
#include "ADS/graph/Properties.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/Visit.h"
#include "ADS/numeric/Combinatorics.h"
#include "ADS/optimization/HouseProblems.h"
#include "ADS/optimization/Intervals.h"
#include "ADS/optimization/ShortestJobFirst.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/search/BinarySearch.h"
#include "ADS/search/Selection.h"
#include "ADS/search/SortedSearches.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/set/LinkedHashSet.h"
#include "ADS/sort/Heapsort.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/MergeSort.h"
#include "ADS/sort/Sort.h"
#include "ADS/string/Segmentations.h"
#include "ADS/string/SubsequenceProblems.h"
#include "ADS/tree/BinaryTreeAlgorithms.h"
#include "ADS/tree/SearchTree.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*----------------------------- THE SOLUTIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

namespace exams {

/// true if the items of A are distinct
inline bool distinct( std::vector< long > A )
{
 std::sort( A.begin() , A.end() );
 return( std::adjacent_find( A.begin() , A.end() ) == A.end() );
 }

/// the integer square root, floor( sqrt( n ) )
inline std::size_t isqrt( std::size_t n )
{
 auto r = std::size_t( std::sqrt( double( n ) ) );
 while( r * r > n )
  --r;
 while( ( r + 1 ) * ( r + 1 ) <= n )
  ++r;
 return( r );
 }

/// a node of the TREE of the texts: the key, the color, the links;
/// nullptr is nil, a black leaf
struct TreeNode {
 long key = 0;                   ///< the key
 Color color = Color::BLACK;     ///< the color
 TreeNode * left = nullptr;      ///< the left child
 TreeNode * right = nullptr;     ///< the right child
 TreeNode * parent = nullptr;    ///< the parent
 };

/// the nodes of the trees, at fixed addresses
using Pool = std::deque< TreeNode >;

/// a new node of P with the children l and r, whose parent it becomes
inline TreeNode * makeNode( Pool & P , long key , TreeNode * l = nullptr ,
                            TreeNode * r = nullptr ,
                            Color c = Color::BLACK )
{
 TreeNode & t = P.emplace_back();
 t.key = key;
 t.color = c;
 t.left = l;
 t.right = r;
 if( l )
  l->parent = &t;
 if( r )
  r->parent = &t;
 return( &t );
 }

/// the insertion of key in the binary search tree of root T
inline void bstInsert( Pool & P , TreeNode * & T , long key )
{
 TreeNode * p = nullptr;
 TreeNode ** u = &T;
 while( *u ) {
  p = *u;
  u = ( key < p->key ) ? &p->left : &p->right;
  }
 *u = makeNode( P , key );
 ( *u )->parent = p;
 }

/// the height of T, in edges, -1 for the empty tree
inline int height( const TreeNode * T )
{
 if( ! T )
  return( -1 );
 return( 1 + std::max( height( T->left ) , height( T->right ) ) );
 }

/// true if t is a red node (nil is black)
inline bool isRed( const TreeNode * t )
{
 return( t && ( t->color == Color::RED ) );
 }

/*---------------- 2011-04-11 A1, minimum and maximum gaps -----------------*/

/// minGap(): a pair of places i < j of A minimizing | A[ i ] - A[ j ] |;
/// the items with their places are sorted by value by heapsort(), and a
/// smallest gap is between two consecutive ones; O( n log n ) time,
/// O( n ) space. The gaps are computed as unsigned long, which holds the
/// difference of any two long in order (LONG_MAX - LONG_MIN overflows a
/// long)
/// \pre n >= 2
inline std::pair< std::size_t , std::size_t > minGap(
                                                const std::vector< long > & A )
{
 ADS_PRE( A.size() >= 2 );
 std::vector< std::pair< long , std::size_t > > B( A.size() );
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  B[ i ] = std::pair( A[ i ] , i );
 heapsort( B , []( const auto & x , const auto & y ) {
  return( x.first < y.first );
  } );
 auto gap = [ & ]( std::size_t i ) {
  return( static_cast< unsigned long >( B[ i ].first ) -
          static_cast< unsigned long >( B[ i - 1 ].first ) );
  };
 std::size_t best = 1;
 for( std::size_t i = 2 ; i < B.size() ; ++i )
  if( gap( i ) < gap( best ) )
   best = i;
 const std::size_t i = B[ best - 1 ].second , j = B[ best ].second;
 return( std::pair( std::min( i , j ) , std::max( i , j ) ) );
 }

/// maxGap(): a pair of places i < j of A maximizing | A[ i ] - A[ j ] |,
/// a minimum and a maximum found in one scan; the two places start
/// distinct, which is an answer when all the items are equal; O( n ) time
/// \pre n >= 2
inline std::pair< std::size_t , std::size_t > maxGap(
                                                const std::vector< long > & A )
{
 ADS_PRE( A.size() >= 2 );
 std::size_t lo = 0 , hi = 1;
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  if( A[ i ] < A[ lo ] )
   lo = i;
  if( A[ i ] > A[ hi ] )
   hi = i;
  }
 return( std::pair( std::min( lo , hi ) , std::max( lo , hi ) ) );
 }

/*--------------- 2011-04-11 A2, minimum height search tree ----------------*/

/// buildMinHeightBST(): the binary search tree of the keys of A, with
/// parents, of the least height, ceil( log2( n + 1 ) ) - 1, the middle
/// item at the root and the two halves built recursively by
/// minimumHeightSearchTree() of the library; Theta( n ) time, O( log n )
/// stack
/// \pre n > 0 and A strictly increasing
/// \post the height is the least one for n nodes
inline TreeNode * buildMinHeightBST( Pool & P , const std::vector< long > & A )
{
 ADS_PRE( ! A.empty() );
 TreeNode * T = minimumHeightSearchTree( A , [ & ]( long k , TreeNode * l ,
                                                    TreeNode * r ) {
  return( makeNode( P , k , l , r ) ); } );
 ADS_POST( ( std::size_t( 1 ) << ( height( T ) + 1 ) ) - 1 >= A.size() );
 ADS_POST( ( std::size_t( 1 ) << height( T ) ) - 1 < A.size() );
 return( T );
 }

/*--------- 2011-06-06 B2 and 2012-07-09 B2, nearest to the median ---------*/

/// nearest(): k items of V closest to its median, the median excluded,
/// ties broken by the order in V: the median m and then the distance d of
/// the k-th closest one by selection(), then the items closer than d and
/// those at d until there are k; O( n ) expected time. The text takes as
/// d the k-th smallest distance of V (selezione( V , ... ), meaning B),
/// which counts the distance 0 of the median itself; here d is the
/// ( k + 1 )-th one, and k < n since the median is left out
/// \pre 1 <= k < n, the items of V distinct
/// \post k items
inline std::vector< long > nearest( const std::vector< long > & V ,
                                    std::size_t k , std::mt19937 & g )
{
 const std::size_t n = V.size();
 ADS_PRE( ( k >= 1 ) && ( k < n ) );
 ADS_AUDIT( distinct( V ) );
 auto W = V;
 const long m = selection( W , ( n + 1 ) / 2 , &g );
 std::vector< long > B( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  B[ i ] = std::abs( V[ i ] - m );
 auto C = B;
 const long d = selection( C , k + 1 , &g );
 std::vector< long > R;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( ( B[ i ] < d ) && ( V[ i ] != m ) )
   R.push_back( V[ i ] );
 for( std::size_t i = 0 ; ( i < n ) && ( R.size() < k ) ; ++i )
  if( B[ i ] == d )
   R.push_back( V[ i ] );
 ADS_POST( R.size() == k );
 return( R );
 }

/// kmediana(): the k or k + 1 items of A closest to the median, the
/// median included, those at a distance not larger than the k-th
/// smallest one; O( n ) expected time. The text selects the item of rank
/// floor( n / 2 ), which is 0 for n = 1; here the rank is at least 1
/// \pre 1 <= k <= n, the items of A distinct
/// \post k or k + 1 items
inline std::vector< long > kMedian( const std::vector< long > & A ,
                                    std::size_t k , std::mt19937 & g )
{
 const std::size_t n = A.size();
 ADS_PRE( ( k >= 1 ) && ( k <= n ) );
 ADS_AUDIT( distinct( A ) );
 auto W = A;
 const long m = selection( W , std::max< std::size_t >( n / 2 , 1 ) , &g );
 std::vector< long > B( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  B[ i ] = std::abs( A[ i ] - m );
 auto C = B;
 const long d = selection( C , k , &g );
 std::vector< long > R;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( B[ i ] <= d )
   R.push_back( A[ i ] );
 ADS_POST( ( R.size() == k ) || ( R.size() == k + 1 ) );
 return( R );
 }

/*---------- 2012-02-01 A1 and 2012-06-18 A2, merge of many runs -----------*/

/// the runs A[ lo .. lo + len ) , A[ lo + len .. lo + 2 len ) , ... of
/// A[ lo .. hi ), each sorted, merged by taking each time the smallest
/// head, which costs one comparison less than the runs not exhausted;
/// the comparisons are added to cmp
/// \pre lo <= hi <= n and len >= 1
inline void mergeRuns( std::vector< long > & A , std::size_t lo ,
                       std::size_t hi , std::size_t len , long & cmp )
{
 ADS_PRE( ( lo <= hi ) && ( hi <= A.size() ) && ( len >= 1 ) );
 std::vector< std::size_t > head , end;
 for( std::size_t s = lo ; s < hi ; s += len ) {
  head.push_back( s );
  end.push_back( std::min( s + len , hi ) );
  }
 std::vector< long > B;
 B.reserve( hi - lo );
 while( B.size() < hi - lo ) {
  std::size_t best = head.size();
  for( std::size_t j = 0 ; j < head.size() ; ++j )
   if( head[ j ] < end[ j ] ) {
    if( best == head.size() )
     best = j;
    else {
     ++cmp;
     if( A[ head[ j ] ] < A[ head[ best ] ] )
      best = j;
     }
    }
  B.push_back( A[ head[ best ]++ ] );
  }
 std::copy( B.begin() , B.end() , A.begin() + std::ptrdiff_t( lo ) );
 }

/// MergeSortK of A[ lo .. hi ): at most k parts of ceil( n / k ) items
/// sorted recursively and merged by mergeRuns()
inline void mergeSortKRec( std::vector< long > & A , std::size_t lo ,
                           std::size_t hi , std::size_t k , long & cmp )
{
 const std::size_t n = hi - lo;
 if( n <= 1 )
  return;
 const std::size_t p = std::min( k , n ) , len = ( n + p - 1 ) / p;
 for( std::size_t s = lo ; s < hi ; s += len )
  mergeSortKRec( A , s , std::min( s + len , hi ) , k , cmp );
 mergeRuns( A , lo , hi , len , cmp );
 }

/// MergeSortK: k parts sorted recursively and merged in O( k n ) time,
/// T( n ) = k T( n / k ) + O( k n ) = O( k n log_k n ); the comparisons
/// are given back
/// \pre k >= 2
/// \post A sorted
inline long mergeSortK( std::vector< long > & A , std::size_t k )
{
 ADS_PRE( k >= 2 );
 long cmp = 0;
 mergeSortKRec( A , 0 , A.size() , k , cmp );
 ADS_POST( std::is_sorted( A.begin() , A.end() ) );
 return( cmp );
 }

/// MergeSort( A , i , j ) of 18/06/12 on A[ i .. j ]: an insertion sort
/// below 4 items, otherwise the blocks of step = floor( sqrt( n ) ) items
/// sorted recursively and merged by mergeRuns(); T( n ) = sqrt( n )
/// T( sqrt( n ) ) + O( n sqrt( n ) ) = O( n sqrt( n ) )
inline void sqrtMergeSortRec( std::vector< long > & A , std::size_t i ,
                              std::size_t j , long & cmp )
{
 const std::size_t n = j - i + 1;
 if( n < 4 ) {
  for( std::size_t a = i + 1 ; a <= j ; ++a )
   for( std::size_t b = a ; b > i ; --b ) {
    ++cmp;
    if( ! ( A[ b ] < A[ b - 1 ] ) )
     break;
    std::swap( A[ b ] , A[ b - 1 ] );
    }
  return;
  }
 const std::size_t step = isqrt( n );
 for( std::size_t s = i ; s <= j ; s += step )
  sqrtMergeSortRec( A , s , std::min( s + step - 1 , j ) , cmp );
 mergeRuns( A , i , j + 1 , step , cmp );
 }

/// the merge sort by sqrt( n ) blocks; the comparisons are given back
/// \post A sorted
inline long sqrtMergeSort( std::vector< long > & A )
{
 long cmp = 0;
 if( ! A.empty() )
  sqrtMergeSortRec( A , 0 , A.size() - 1 , cmp );
 ADS_POST( std::is_sorted( A.begin() , A.end() ) );
 return( cmp );
 }

/*---------------------- 2012-05-03 A2, the node Roma ----------------------*/

/// isRoma(): v is reached from every node of G, i.e., a visit of the
/// transposed graph GT from v reaches all the nodes; O( n + m )
/// \pre v is a node of GT
template< Graph G >
bool isRoma( const G & GT , int v )
{
 ADS_PRE( GT.hasNode( v ) );
 std::vector< bool > visited( GT.bound() , false );
 int count = 0;
 dfs( GT , v , visited , [ & ]( int ) { ++count; } , []( int , int ) {} ,
      []( int ) {} );
 return( count == GT.n() );
 }

/// a node Roma exists in g: the last node closed by a depth-first search
/// of the whole transposed graph is the only candidate, checked by
/// isRoma(); O( n + m )
template< Graph G >
bool hasRoma( const G & g )
{
 if( g.n() == 0 )
  return( false );
 const auto GT = transpose( g );
 std::vector< bool > visited( GT.bound() , false );
 int last = NIL;
 for( int u : GT.V() )
  if( ! visited[ u ] )
   dfs( GT , u , visited , []( int ) {} , []( int , int ) {} ,
        [ & ]( int x ) { last = x; } );
 return( isRoma( GT , last ) );
 }

/*----------------------- 2012-09-10 A1, SortinoSort -----------------------*/

/// SortinoSort( A , i , j ): insertionSort() up to 6 items, otherwise,
/// with s = ceil( n / 3 ), the first two thirds, the last two and the
/// first two again; T( n ) = 3 T( 2 n / 3 ) + 1 = Theta( n^{log_{3/2} 3} )
inline void sortinoSortRec( std::vector< long > & A , std::size_t i ,
                            std::size_t j , long & calls )
{
 ++calls;
 const std::size_t n = j - i + 1;
 if( n <= 6 ) {
  insertionSort( A , i , j + 1 );
  return;
  }
 const std::size_t s = ( n + 2 ) / 3;
 sortinoSortRec( A , i , i + 2 * s - 1 , calls );
 sortinoSortRec( A , i + s , j , calls );
 sortinoSortRec( A , i , i + 2 * s - 1 , calls );
 }

/// SortinoSort of A; the calls are given back
/// \post A sorted
inline long sortinoSort( std::vector< long > & A )
{
 long calls = 0;
 if( ! A.empty() )
  sortinoSortRec( A , 0 , A.size() - 1 , calls );
 ADS_POST( std::is_sorted( A.begin() , A.end() ) );
 return( calls );
 }

/*--------- 2013-01-07 A1 and A2, disjoint vectors and sum of two ----------*/

/// A and B have no common value: both sorted, then a scan as in the merge
/// that advances the smaller head; O( n log n ). The text writes A[ j ]
/// for B[ j ] in the scan
/// \pre the items of A, and those of B, distinct
inline bool disjoint( std::vector< long > A , std::vector< long > B )
{
 ADS_AUDIT( distinct( A ) && distinct( B ) );
 ADS::sort( A );
 ADS::sort( B );
 std::size_t i = 0 , j = 0;
 while( ( i < A.size() ) && ( j < B.size() ) )
  if( A[ i ] == B[ j ] )
   return( false );
  else if( A[ i ] < B[ j ] )
   ++i;
  else
   ++j;
 return( true );
 }

/// two items of S at distinct places sum to x: pairWithSum() of the
/// library, two indices from the ends, the right one moving left when the
/// sum is larger than x and the left one right when it is smaller; O( n ).
/// The text moves the left index with i = i - 1
/// \pre S sorted
inline bool twoSum( const std::vector< long > & S , long x )
{
 return( pairWithSum( S , x ).has_value() );
 }

/*------------------------ 2013-06-17 A2, four rows ------------------------*/

/// the 4 n items of V in the rows of a 4 x n matrix, every item of a row
/// smaller than every item of the next ones: the items of rank n, 2 n
/// and 3 n by selection() bound the rows; O( n ) expected time
/// \pre | V | multiple of 4, the items distinct
/// \post four rows of n items
inline std::array< std::vector< long > , 4 > fourRows(
                           const std::vector< long > & V , std::mt19937 & g )
{
 ADS_PRE( V.size() % 4 == 0 );
 ADS_AUDIT( distinct( V ) );
 const std::size_t n = V.size() / 4;
 std::array< std::vector< long > , 4 > B;
 if( n == 0 )
  return( B );
 std::array< long , 3 > bound;
 for( std::size_t r = 0 ; r < 3 ; ++r ) {
  auto W = V;
  bound[ r ] = selection( W , ( r + 1 ) * n , &g );
  }
 for( long x : V ) {
  std::size_t r = 0;
  while( ( r < 3 ) && ( x > bound[ r ] ) )
   ++r;
  B[ r ].push_back( x );
  }
 ADS_POST( ( B[ 0 ].size() == n ) && ( B[ 1 ].size() == n ) &&
           ( B[ 2 ].size() == n ) && ( B[ 3 ].size() == n ) );
 return( B );
 }

/*---------- 2013-09-02 A2 and 2019-08-22 A2, red-black colorings ----------*/

/// verifyRB(): the coloring of T is red-black: the root black, no red
/// node with a red child, the same black height on all the paths, by the
/// postvisit of isRedBlackTree() of the library; O( n )
inline bool verifyRB( const TreeNode * T )
{
 return( isRedBlackTree( T , isRed ) );
 }

/// blackHeight(): the black height of t, the leaf included, -1 if the
/// subtree breaks a rule (a red node with a red parent, or two different
/// heights)
inline int blackHeight( const TreeNode * t )
{
 if( ! t )
  return( 1 );
 if( isRed( t ) && isRed( t->parent ) )
  return( -1 );
 const int l = blackHeight( t->left ) , r = blackHeight( t->right );
 if( ( l < 0 ) || ( r < 0 ) || ( l != r ) )
  return( -1 );
 return( l + ( isRed( t ) ? 0 : 1 ) );
 }

/// isRedBlack(): the root black and blackHeight() positive; Theta( n )
/// \pre the parents of the nodes of T are set
inline bool isRedBlack( const TreeNode * T )
{
 if( isRed( T ) )
  return( false );
 return( blackHeight( T ) > 0 );
 }

/*---------------------- 2014-10-31 A2, almost sorted ----------------------*/

/// floor( n^{4/5} ), the length of the part of the vector not sorted
inline std::size_t unsortedTail( std::size_t n )
{
 return( std::size_t( std::floor( std::pow( double( n ) , 0.8 ) ) ) );
 }

/// almostSort(): the last floor( n^{4/5} ) items by mergeSortRec(), in
/// Theta( n^{4/5} log n ) = o( n ), then merged with the first ones by
/// mergeParts() in Theta( n ); Theta( n ) time
/// \pre V[ 0 .. n - floor( n^{4/5} ) ) sorted
/// \post V sorted
inline void almostSort( std::vector< long > & V )
{
 const std::size_t n = V.size();
 if( n < 2 )
  return;
 const std::size_t p = n - unsortedTail( n );
 ADS_AUDIT( std::is_sorted( V.begin() , V.begin() + std::ptrdiff_t( p ) ) );
 std::vector< long > B( n );
 mergeSortRec( V , B , p , n - 1 , 1 );
 if( p > 0 )
  mergeParts( V , B , 0 , p - 1 , n - 1 );
 ADS_POST( std::is_sorted( V.begin() , V.end() ) );
 }

/*--------------------- 2014-12-19 B1, unit intervals ----------------------*/

/// minIntervals(): the fewest intervals [ x , x + 1 ] covering the values
/// of V: by increasing value, a value not covered by the last interval
/// opens a new one starting at it; O( n log n ). It is towers() with
/// d = 1 / 2
inline int minIntervals( std::vector< double > V )
{
 if( V.empty() )
  return( 0 );
 ADS::sort( V );
 int count = 1;
 double start = V[ 0 ];
 for( std::size_t i = 1 ; i < V.size() ; ++i )
  if( V[ i ] > start + 1 ) {
   ++count;
   start = V[ i ];
   }
 return( count );
 }

/*--------------------- 2015-06-08 A2, connected order ---------------------*/

/// connected-order(): the nodes by increasing finishing time of a
/// depth-first search, so that deleting them in this order leaves the
/// graph connected: the first one closed is a leaf of the tree of the
/// search; O( n + m )
/// \pre g undirected and connected
template< Graph G >
std::vector< int > connectedOrder( const G & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< bool > visited( g.bound() , false );
 std::vector< int > order;
 for( int u : g.V() )
  if( ! visited[ u ] )
   dfs( g , u , visited , []( int ) {} , []( int , int ) {} ,
        [ & ]( int x ) { order.push_back( x ); } );
 return( order );
 }

/*----------------- 2015-07-14 B3.2, univocal set of edges -----------------*/

/// an edge ( u , v ) of weight w
using WeightedEdge = std::tuple< int , int , long >;

/// univoco(): a set of edges of the graph of n nodes, no two with a
/// common end, of the largest weight, and the weight: maxWeightMatching()
/// of the library, a backtracking that leaves the first free node alone or
/// matches it to each free neighbour, cutting the branches that cannot
/// beat the best set found. The text branches on the edges of a node u and
/// recurses on the other end, which misses the sets whose edges do not
/// form a path from u
/// \pre the ends of each edge are distinct nodes in [ 0 , n ), weights
///      positive
/// \post no two edges with a common end
inline std::pair< long , std::vector< std::size_t > > maxMatching(
                                int n , const std::vector< WeightedEdge > & E )
{
 for( const auto & e : E )
  ADS_PRE( std::get< 2 >( e ) > 0 );
 return( maxWeightMatching( n , E ) );
 }

/*------ 2015-11-05 A2 and A4, difference in a subtree and two values ------*/

/// computeDiffRec(): the minimum and the maximum key below t
inline std::pair< long , long > computeDiffRec( const TreeNode * t )
{
 if( ! t )
  return( std::pair( std::numeric_limits< long >::max() ,
                     std::numeric_limits< long >::min() ) );
 const auto [ ml , Ml ] = computeDiffRec( t->left );
 const auto [ mr , Mr ] = computeDiffRec( t->right );
 return( std::pair( std::min( { t->key , ml , mr } ) ,
                    std::max( { t->key , Ml , Mr } ) ) );
 }

/// computeDiff(): the largest key minus the smallest one in the subtree
/// of t, by a postorder visit; O( n ). The text takes the minimum of the
/// key, min_L and max_R
/// \pre t is not nil
inline long computeDiff( const TreeNode * t )
{
 ADS_PRE( t );
 const auto [ m , M ] = computeDiffRec( t );
 return( M - m );
 }

/// binarySearchAB(): the place of the last a in A[ i .. j ], where
/// A[ i ] = a and A[ j ] = b; O( log n )
inline std::size_t binarySearchAB( const std::vector< long > & A ,
                                   std::size_t i , std::size_t j )
{
 if( j == i + 1 )
  return( i );
 const std::size_t m = ( i + j ) / 2;
 if( A[ m ] == A[ i ] )
  return( binarySearchAB( A , m , j ) );
 return( binarySearchAB( A , i , m ) );
 }

/// the occurrences of the most frequent value of a sorted vector of two
/// values a < b, both present; with the places from 0 the a are the last
/// place of an a plus one; O( log n )
/// \pre A sorted, of two distinct values, both present
inline std::size_t mostFrequentAB( const std::vector< long > & A )
{
 ADS_PRE( ( A.size() >= 2 ) && ( A.front() < A.back() ) );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) &&
            std::all_of( A.begin() , A.end() , [ & ]( long x ) {
             return( ( x == A.front() ) || ( x == A.back() ) );
             } ) );
 const std::size_t k = binarySearchAB( A , 0 , A.size() - 1 ) + 1;
 return( std::max( k , A.size() - k ) );
 }

/*----------------- 2016-01-26 B3, shuffle of two strings ------------------*/

// isShuffle(): Z interleaves X and Y keeping the order of each: DP[ i ][
// j ] is true if Z[ 0 .. i + j ) is a shuffle of X[ 0 .. i ) and Y[ 0 ..
// j ), isInterleaving() of the library [see SubsequenceProblems.h]

/*----------------- 2016-07-06 A2, sorted rows and columns -----------------*/

/// the items of the m x n matrix M rearranged so that rows and columns
/// are nondecreasing: sorted by sort() and written back row by row;
/// O( n m log( n m ) )
/// \pre the rows of M of the same length
inline void sortMatrix( std::vector< std::vector< long > > & M )
{
 std::vector< long > V;
 for( const auto & r : M ) {
  ADS_PRE( r.size() == M.front().size() );
  V.insert( V.end() , r.begin() , r.end() );
  }
 ADS::sort( V );
 std::size_t k = 0;
 for( auto & r : M )
  for( auto & x : r )
   x = V[ k++ ];
 }

/*-------------- 2016-08-29 B4, occurrences of a subsequence ---------------*/

// count(): the occurrences of P as a subsequence of T, the tuples of
// places j_1 < ... < j_m with T[ j_k ] = P[ k ], by the recursion on
// DP[ p ][ t ] of the text: subsequenceOccurrences() of the library [see
// SubsequenceProblems.h]

/*--------------------- 2017-01-10 B4, files on a tape ---------------------*/

/// the expected cost of reading the files of lengths l and probabilities
/// f stored in the order pi: sum_i f_{pi(i)} sum_{j <= i} l_{pi(j)}
/// \pre pi a permutation of the places of l and f
inline double tapeCost( const std::vector< double > & l ,
                        const std::vector< double > & f ,
                        const std::vector< std::size_t > & pi )
{
 ADS_PRE( ( l.size() == f.size() ) && ( pi.size() == l.size() ) );
 double c = 0 , s = 0;
 for( std::size_t i : pi ) {
  s += l[ i ];
  c += f[ i ] * s;
  }
 return( c );
 }

/// the order of the places of l by the key: x before y if key( x ) <
/// key( y ), stable
template< class C >
std::vector< std::size_t > orderBy( std::size_t n , C less )
{
 std::vector< std::size_t > pi( n );
 std::iota( pi.begin() , pi.end() , std::size_t( 0 ) );
 stableSort( pi , less );
 return( pi );
 }

/// the order of the files that minimizes tapeCost(): by increasing
/// l_i / f_i (the rule of Smith), compared as l_a f_b < l_b f_a; an
/// exchange of two adjacent files a , b changes the cost by f_a l_b -
/// f_b l_a; O( n log n ), smithOrder() of the library
/// \pre lengths positive, probabilities not negative
inline std::vector< std::size_t > tapeOrder( const std::vector< double > & l ,
                                             const std::vector< double > & f )
{
 ADS_PRE( l.size() == f.size() );
 for( std::size_t i = 0 ; i < l.size() ; ++i )
  ADS_PRE( ( l[ i ] > 0 ) && ( f[ i ] >= 0 ) );
 return( smithOrder( l , f ) );
 }

/*--------- 2017-06-08 A2 and B4, topological orders and decodings ---------*/

/// the number of topological orders of g, 0, 1, or 2 for more than one:
/// 0 if hasCycle(), otherwise uniqueTopologicalOrder() of the library, the
/// algorithm of Kahn, the order being unique as long as the queue of the
/// nodes with no entering edge never holds two of them; O( n + m ). The
/// text enqueues u where it means v
inline int topologicalOrders( const ListGraph<> & g )
{
 ADS_PRE( g.directed() );
 if( hasCycle( g ) )
  return( 0 );
 return( uniqueTopologicalOrder( g ) ? 1 : 2 );
 }

// the interpretations of the digits T as letters A = 1, ..., Z = 26:
// DP[ i ], those of T[ i .. n ), is 0 if T[ i ] = 0, otherwise DP[ i + 1 ]
// plus DP[ i + 2 ] if T[ i ] T[ i + 1 ] is in 10 .. 26; DP[ n ] = 1, as
// countDecodings() of the library [see Segmentations.h]. The text sets
// DP[ i ] = 0 for T[ i ] = 0 and then overwrites it, lacking an else

/*--------------------- 2018-01-31 A2, keys in a range ---------------------*/

/// occurrences(): the keys of the binary search tree T in [ lo , hi ],
/// the visit pruned where the key leaves the range on one side, by
/// searchTreeRange() of the library; O( n ), O( h + k ) for k keys in the
/// range; the nodes visited are added to *visits, if given
/// \pre lo <= hi
inline long occurrences( const TreeNode * T , long lo , long hi ,
                         long * visits = nullptr )
{
 long k = 0;
 const std::size_t seen = searchTreeRange( T , lo , hi ,
                                           [ & ]( const TreeNode * ) {
  ++k; } );
 if( visits )
  *visits += long( seen );
 return( k );
 }

/*-------------------- 2019-02-07 A2, first occurrence ---------------------*/

/// first(): the place, from 1 as in the text, of the first occurrence of
/// v in the sorted A, by lowerBound(); Theta( log n )
/// \pre A sorted, v in A
inline std::size_t first( const std::vector< long > & A , long v )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) &&
            ( std::find( A.begin() , A.end() , v ) != A.end() ) );
 return( lowerBound( A , v ) + 1 );
 }

/*------------------- 2019-08-22 A1, merge by insertion --------------------*/

/// the Merge() of 22/08/19, an insertion sort of A[ start .. end ]; the
/// comparisons are added to cmp. The text stops the inner loop at j > 1,
/// which on A[ start .. end ] is j > start
inline void insertionMerge( std::vector< long > & A , std::size_t start ,
                            std::size_t end , long & cmp )
{
 for( std::size_t i = start + 1 ; i <= end ; ++i ) {
  const long temp = A[ i ];
  std::size_t j = i;
  while( j > start ) {
   ++cmp;
   if( ! ( A[ j - 1 ] > temp ) )
    break;
   A[ j ] = A[ j - 1 ];
   --j;
   }
  A[ j ] = temp;
  }
 }

/// MergeSort with insertionMerge(): T( n ) = 2 T( n / 2 ) + n on sorted
/// input, 2 T( n / 2 ) + n^2 on reversed input
inline void insertionMergeSort( std::vector< long > & A , std::size_t start ,
                                std::size_t end , long & cmp )
{
 if( start >= end )
  return;
 const std::size_t mid = ( start + end ) / 2;
 insertionMergeSort( A , start , mid , cmp );
 insertionMergeSort( A , mid + 1 , end , cmp );
 insertionMerge( A , start , end , cmp );
 }

/*------------------ 2020-02-04 A1, heaps in a recursion -------------------*/

/// funr( A , i , j ) of 04/02/20: A[ i .. j ] copied into B and made a
/// heap by heapBuild(), then the two halves; the items copied are added to
/// copied, T( n ) = 2 T( n / 2 ) + n = Theta( n log n )
inline long funr( const std::vector< long > & A , std::size_t i ,
                  std::size_t j , long & copied )
{
 if( i + 1 >= j )
  return( 0 );
 std::vector< long > B( A.begin() + std::ptrdiff_t( i ) ,
                        A.begin() + std::ptrdiff_t( j ) + 1 );
 copied += long( B.size() );
 heapBuild( B );
 const std::size_t m = ( i + j ) / 2;
 return( funr( A , i , m , copied ) + funr( A , m + 1 , j , copied ) +
         B[ 1 ] );
 }

/*----------------- 2021-02-08 A3, doubling and decrement ------------------*/

/// minOps() by a breadth-first search on the integers in [ 1 , 2 m ],
/// the neighbours of u being 2 u and u - 1; Theta( m )
/// \pre 1 <= n and 1 <= m
inline int minOpsBfs( long n , long m )
{
 ADS_PRE( ( n >= 1 ) && ( m >= 1 ) );
 const long top = std::max( 2 * m , n );
 std::vector< int > d( std::size_t( top + 1 ) , -1 );
 std::queue< long > Q;
 d[ std::size_t( n ) ] = 0;
 Q.push( n );
 while( d[ std::size_t( m ) ] < 0 ) {
  const long u = Q.front();
  Q.pop();
  for( long v : { 2 * u , u - 1 } )
   if( ( v >= 1 ) && ( v <= top ) && ( d[ std::size_t( v ) ] < 0 ) ) {
    d[ std::size_t( v ) ] = d[ std::size_t( u ) ] + 1;
    Q.push( v );
    }
  }
 return( d[ std::size_t( m ) ] );
 }

/// minOps() backwards from m: n - m if m <= n, otherwise one step more
/// than for m / 2 if m is even, two more than for ( m + 1 ) / 2 if odd;
/// O( log m )
/// \pre 1 <= n and 1 <= m
inline long minOps( long n , long m )
{
 ADS_PRE( ( n >= 1 ) && ( m >= 1 ) );
 if( m <= n )
  return( n - m );
 if( m % 2 == 0 )
  return( minOps( n , m / 2 ) + 1 );
 return( minOps( n , ( m + 1 ) / 2 ) + 2 );
 }

/*-------------------- 2021-07-27 A2, A3, B1, B2 and B3 --------------------*/

/// true if each item of A is at most k places away from its place in the
/// stable sorted order
inline bool kSorted( const std::vector< long > & A , std::size_t k )
{
 std::vector< std::size_t > pi( A.size() );
 std::iota( pi.begin() , pi.end() , std::size_t( 0 ) );
 std::stable_sort( pi.begin() , pi.end() , [ & ]( auto a , auto b ) {
  return( A[ a ] < A[ b ] );
  } );
 for( std::size_t r = 0 ; r < pi.size() ; ++r )
  if( ( pi[ r ] > r + k ) || ( r > pi[ r ] + k ) )
   return( false );
 return( true );
 }

/// sortK(): A sorted when each item is at most k places away from its
/// place: sortKSorted() of the library, a heap of the next k + 1 items
/// gives the item of each place; O( n log k ) time, O( k ) space
/// \pre A k-sorted
/// \post A sorted
inline void sortK( std::vector< long > & A , std::size_t k )
{
 ADS_AUDIT( kSorted( A , k ) );
 sortKSorted( A , k );
 ADS_POST( std::is_sorted( A.begin() , A.end() ) );
 }

/// wbRic(): the nodes below t and whether all of them are well balanced
inline std::pair< long , bool > wbRic( const TreeNode * t )
{
 if( ! t )
  return( std::pair( 0L , true ) );
 const auto [ nl , bl ] = wbRic( t->left );
 const auto [ nr , br ] = wbRic( t->right );
 return( std::pair( nl + nr + 1 , bl && br && ( std::abs( nl - nr ) <= 1 ) ) );
 }

/// wellBalanced(): the subtrees of each node of T have numbers of nodes
/// differing by at most one, by a postorder visit; O( n )
inline bool wellBalanced( const TreeNode * T )
{
 return( wbRic( T ).second );
 }

/// meetingPoint(): a node reached from s and from t by shortest paths of
/// the same finite cost: the distances from s and from t by johnson()
/// (Dijkstra with a heap) if no weight is negative, otherwise by
/// bellmanFordMoore(), then a scan; O( m log n ) or O( m n )
/// \pre s and t nodes, no negative cycle reachable from them (audit)
template< Graph G >
bool meetingPoint( const G & g , int s , int t )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) );
 ADS_AUDIT( ( ! reachesNegativeCycle( g , s ) ) &&
            ( ! reachesNegativeCycle( g , t ) ) );
 using D = typename G::Weight;
 bool positive = true;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int , const D & w ) {
   if( w < D( 0 ) )
    positive = false;
   } );
 const auto ds = positive ? johnson( g , s ) : bellmanFordMoore( g , s );
 const auto dt = positive ? johnson( g , t ) : bellmanFordMoore( g , t );
 for( int u : g.V() )
  if( ( ds.d[ u ] != infinity< D >() ) && ( ds.d[ u ] == dt.d[ u ] ) )
   return( true );
 return( false );
 }

/// printBitsRec(): the strings completing S[ 0 .. i ), a 1 only after a 0
inline void printBitsRec( std::string & S , std::size_t i ,
                          std::vector< std::string > & out )
{
 if( i == S.size() ) {
  out.push_back( S );
  return;
  }
 if( ( i == 0 ) || ( S[ i - 1 ] != '1' ) ) {
  S[ i ] = '1';
  printBitsRec( S , i + 1 , out );
  }
 S[ i ] = '0';
 printBitsRec( S , i + 1 , out );
 }

/// printBits(): the strings of n bits with no two consecutive 1, by
/// backtracking; they are F_{n+2} (from F_1 = F_2 = 1), O( n F_n ) time
inline std::vector< std::string > printBits( std::size_t n )
{
 std::string S( n , '0' );
 std::vector< std::string > out;
 printBitsRec( S , 0 , out );
 return( out );
 }

/// maxSumKRec(): DP[ i ][ j ], the best value from the first i items when
/// j more consecutive items can still be deleted, memoized
inline long maxSumKRec( const std::vector< long > & V , std::size_t k ,
                        std::size_t i , std::size_t j ,
                        std::vector< std::vector< std::optional< long > > > &
                        DP )
{
 if( i == 0 )
  return( 0 );
 if( ! DP[ i ][ j ] ) {
  long b = maxSumKRec( V , k , i - 1 , k , DP ) + V[ i - 1 ];
  if( j > 0 )
   b = std::max( b , maxSumKRec( V , k , i - 1 , j - 1 , DP ) );
  DP[ i ][ j ] = b;
  }
 return( *DP[ i ][ j ] );
 }

/// maxSumK(): the largest sum of a k-contiguous subsequence of V, one
/// not obtained by deleting more than k consecutive items, by
/// maxSumKRec(), answer DP[ n ][ k ]; O( n k ) time and space. The text
/// gives the table the columns 1 .. k where 0 .. k are used, and marks the
/// empty places with -1, which is a possible value of a sum
inline long maxSumK( const std::vector< long > & V , std::size_t k )
{
 std::vector< std::vector< std::optional< long > > > DP(
           V.size() + 1 , std::vector< std::optional< long > >( k + 1 ) );
 return( maxSumKRec( V , k , V.size() , k , DP ) );
 }

/// maxSumK() in O( n ) space: DP[ i ], the best sum of a k-contiguous
/// subsequence of the first i items ending with item i, is V[ i ] plus the
/// largest DP[ j ], i - k - 1 <= j < i, DP[ 0 ] = 0; answer the largest of
/// DP[ n - k .. n ]; O( n k ) time. The code of the text starts j from
/// i - k, which allows only k - 1 deletions, while its prose says
/// i - k - 1
inline long maxSumKLinear( const std::vector< long > & V , std::size_t k )
{
 const std::size_t n = V.size();
 std::vector< long > DP( n + 1 , 0 );
 for( std::size_t i = 1 ; i <= n ; ++i ) {
  long b = std::numeric_limits< long >::min();
  for( std::size_t j = ( i > k + 1 ) ? i - k - 1 : 0 ; j < i ; ++j )
   b = std::max( b , DP[ j ] );
  DP[ i ] = b + V[ i - 1 ];
  }
 return( *std::max_element( DP.begin() + std::ptrdiff_t( n > k ? n - k : 0 ) ,
                            DP.end() ) );
 }

/*--------------------- 2022-08-24 A3, sum of two keys ---------------------*/

/// containsSum() of the subtree of t: a key x with k - x already met
inline bool containsSumRec( const TreeNode * t , long k ,
                            LinkedHashSet< long > & S )
{
 if( ! t )
  return( false );
 if( S.contains( k - t->key ) )
  return( true );
 S.insert( t->key );
 return( containsSumRec( t->left , k , S ) ||
         containsSumRec( t->right , k , S ) );
 }

/// containsSum(): two nodes of T whose keys sum to k, by a visit that
/// keeps the keys met in a hash set; O( n ) expected time and space
inline bool containsSum( const TreeNode * T , long k )
{
 LinkedHashSet< long > S;
 return( containsSumRec( T , k , S ) );
 }

/*------------------- 2026-06-22 B3, painting the houses -------------------*/

/// hateville(): the cheapest coloring of n houses in a row with k colors,
/// cost[ h ][ c ] for the house h of color c, adjacent houses of different
/// colors: DP[ h ][ c ] = cost[ h ][ c ] + min_{p != c} DP[ h - 1 ][ p ];
/// O( n k^2 ) as written, O( n k ) by paintHouses() of the library
/// \pre k >= 2, every row of cost of k items
inline long hateville( const std::vector< std::vector< long > > & cost )
{
 if( cost.empty() )
  return( 0 );
 ADS_PRE( cost.front().size() >= 2 );
 return( paintHouses( cost ).first );
 }

/// hatevilleFast(): hateville() keeping of the previous row only its
/// minimum, its color and the second minimum; O( n k ) time, as
/// paintHouses() of the library does
/// \pre k >= 2, every row of cost of k items
inline long hatevilleFast( const std::vector< std::vector< long > > & cost )
{
 return( hateville( cost ) );
 }

}  // end( namespace exams )

namespace {

using namespace exams;

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// n distinct values in [ 0 , 4 n ), shuffled
std::vector< long > randomDistinct( std::size_t n , std::mt19937 & g )
{
 std::vector< long > P( 4 * n );
 std::iota( P.begin() , P.end() , 0L );
 std::shuffle( P.begin() , P.end() , g );
 P.resize( n );
 return( P );
 }

/// n values in [ lo , hi ]
std::vector< long > randomVector( std::size_t n , long lo , long hi ,
                                  std::mt19937 & g )
{
 std::uniform_int_distribution< long > U( lo , hi );
 std::vector< long > A( n );
 for( auto & x : A )
  x = U( g );
 return( A );
 }

/// a random binary tree of n nodes, keys in [ lo , hi ]
TreeNode * randomTree( Pool & P , std::size_t n , long lo , long hi ,
                       std::mt19937 & g )
{
 if( n == 0 )
  return( nullptr );
 const std::size_t l = g() % n;
 TreeNode * L = randomTree( P , l , lo , hi , g );
 TreeNode * R = randomTree( P , n - 1 - l , lo , hi , g );
 return( makeNode( P , lo + long( g() % std::uint64_t( hi - lo + 1 ) ) , L ,
                   R ) );
 }

/// the shapes of the binary trees of n nodes, each as the sizes of the
/// left subtrees in preorder
std::vector< std::vector< std::size_t > > shapes( std::size_t n )
{
 if( n == 0 )
  return( std::vector< std::vector< std::size_t > >( 1 ) );
 std::vector< std::vector< std::size_t > > R;
 for( std::size_t l = 0 ; l < n ; ++l )
  for( const auto & a : shapes( l ) )
   for( const auto & b : shapes( n - 1 - l ) ) {
    std::vector< std::size_t > s( 1 , l );
    s.insert( s.end() , a.begin() , a.end() );
    s.insert( s.end() , b.begin() , b.end() );
    R.push_back( s );
    }
 return( R );
 }

/// the tree of n nodes of the shape s from the place i, keys 0 , 1 , ...
/// in preorder
TreeNode * buildShape( Pool & P , const std::vector< std::size_t > & s ,
                       std::size_t & i , std::size_t n , long & key )
{
 if( n == 0 )
  return( nullptr );
 const std::size_t l = s[ i++ ];
 const long k = key++;
 TreeNode * L = buildShape( P , s , i , l , key );
 TreeNode * R = buildShape( P , s , i , n - 1 - l , key );
 return( makeNode( P , k , L , R ) );
 }

/// the nodes of T in preorder
void nodes( TreeNode * T , std::vector< TreeNode * > & out )
{
 if( ! T )
  return;
 out.push_back( T );
 nodes( T->left , out );
 nodes( T->right , out );
 }

/// the keys of T in order
void inorder( const TreeNode * T , std::vector< long > & out )
{
 if( ! T )
  return;
 inorder( T->left , out );
 out.push_back( T->key );
 inorder( T->right , out );
 }

/// the number of nodes of T
long countNodes( const TreeNode * T )
{
 return( T ? 1 + countNodes( T->left ) + countNodes( T->right ) : 0 );
 }

/// the tree of the nodes of a SearchTree from u, with their colors
template< class S >
TreeNode * copyTree( Pool & P , const S & T , const typename S::Link * u )
{
 if( T.isNil( u ) )
  return( nullptr );
 return( makeNode( P , T.key( u ) , copyTree( P , T , u->left ) ,
                   copyTree( P , T , u->right ) , u->color ) );
 }

/// the red-black rules checked path by path: every root-to-leaf path with
/// the same number of black nodes, no red node with a red child, the root
/// black
bool redBlackByPaths( const TreeNode * T )
{
 if( isRed( T ) )
  return( false );
 std::set< int > blacks;
 bool ok = true;
 std::function< void( const TreeNode * , int ) > walk =
  [ & ]( const TreeNode * t , int b ) {
   if( ! t ) {
    blacks.insert( b );
    return;
    }
   if( isRed( t ) && ( isRed( t->left ) || isRed( t->right ) ) )
    ok = false;
   walk( t->left , b + ( isRed( t ) ? 0 : 1 ) );
   walk( t->right , b + ( isRed( t ) ? 0 : 1 ) );
   };
 walk( T , 0 );
 return( ok && ( blacks.size() == 1 ) );
 }

/// the directed or undirected graph of n nodes and of the edges E
ListGraph<> makeGraph( int n ,
                       const std::vector< std::pair< int , int > > & E ,
                       bool directed )
{
 ListGraph<> g( n , directed );
 for( auto [ u , v ] : E )
  g.insertEdge( u , v );
 return( g );
 }

/// the random edges ( u , v ), u != v, each with probability p / 100
std::vector< std::pair< int , int > > randomEdges( int n , int p ,
                                                   std::mt19937 & g ,
                                                   bool directed )
{
 std::vector< std::pair< int , int > > E;
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && ( int( g() % 100 ) < p ) )
    E.emplace_back( u , v );
 return( E );
 }

/// the nodes reached from r through the edges E of n nodes, not entering
/// the deleted ones
std::vector< bool > reach( int n , const std::vector< std::pair< int , int > >
                           & E , int r , const std::vector< bool > & deleted ,
                           bool directed )
{
 std::vector< std::vector< int > > A( n );
 for( auto [ u , v ] : E ) {
  A[ u ].push_back( v );
  if( ! directed )
   A[ v ].push_back( u );
  }
 std::vector< bool > seen( n , false );
 std::queue< int > Q;
 seen[ r ] = true;
 Q.push( r );
 while( ! Q.empty() ) {
  const int u = Q.front();
  Q.pop();
  for( int v : A[ u ] )
   if( ( ! seen[ v ] ) && ( ! deleted[ v ] ) ) {
    seen[ v ] = true;
    Q.push( v );
    }
  }
 return( seen );
 }

/// true if x is in [ lo , hi ]
bool within( double x , double lo , double hi )
{
 return( ( x >= lo ) && ( x <= hi ) );
 }

/// the growth Theta( n^p log^q n )
Growth theta( double p , double q = 0 )
{
 Growth e;
 e.p = p;
 e.q = q;
 return( e );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/*---------------- 2011-04-11 A1, minimum and maximum gaps -----------------*/

// minGap() and maxGap() against all the pairs, on random vectors of
// positive values, with many repeated ones, of two items, of equal items,
// of the largest values and of gaps across the whole range of long
ADS_TEST( "compiti-2011-04-11-A1-minGap" )
{
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = 2 + g() % 30;
  const long top = ( t % 3 == 0 ) ? 5 : ( t % 3 == 1 ) ? 1000 :
                   std::numeric_limits< long >::max() / 2;
  const auto A = randomVector( n , 1 , top , g );
  long best = std::numeric_limits< long >::max();
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = i + 1 ; j < n ; ++j )
    best = std::min( best , std::abs( A[ i ] - A[ j ] ) );
  const auto [ i , j ] = minGap( A );
  CHECK( ( i < j ) && ( j < n ) && ( std::abs( A[ i ] - A[ j ] ) == best ) );
  }
 CHECK( minGap( { 7 , 3 } ) == std::pair( std::size_t( 0 ) ,
                                         std::size_t( 1 ) ) );
 const std::vector< long > E( 10 , 4 );
 const auto [ i , j ] = minGap( E );
 CHECK( ( i < j ) && ( E[ i ] == E[ j ] ) );
 const long M = std::numeric_limits< long >::max();
 CHECK( minGap( { M , 1 , M - 1 } ) == std::pair( std::size_t( 0 ) ,
                                                 std::size_t( 2 ) ) );
 // the gaps of the whole range of long, which overflow a long: the two
 // extremes alone, and with -1 in between (the gap -1 - LONG_MIN is the
 // smaller one by 1)
 const long m = std::numeric_limits< long >::min();
 CHECK( minGap( { M , m } ) == std::pair( std::size_t( 0 ) ,
                                         std::size_t( 1 ) ) );
 CHECK( minGap( { M , m , -1 } ) == std::pair( std::size_t( 1 ) ,
                                              std::size_t( 2 ) ) );
 CHECK( minGap( { m , 0 , m + 1 , M } ) == std::pair( std::size_t( 0 ) ,
                                                     std::size_t( 2 ) ) );
 CHECK( minGap( { 3 , -3 } ) == std::pair( std::size_t( 0 ) ,
                                          std::size_t( 1 ) ) );
 CHECK_THROWS( minGap( { 1 } ) , contract_violation );
 CHECK_THROWS( minGap( {} ) , contract_violation );
 }

ADS_TEST( "compiti-2011-04-11-A1-maxGap" )
{
 std::mt19937 g( 2 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = 2 + g() % 30;
  const auto A = randomVector( n , 1 , ( t % 2 ) ? 4 : 1000 , g );
  long best = 0;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = i + 1 ; j < n ; ++j )
    best = std::max( best , std::abs( A[ i ] - A[ j ] ) );
  const auto [ i , j ] = maxGap( A );
  CHECK( ( i < j ) && ( j < n ) && ( std::abs( A[ i ] - A[ j ] ) == best ) );
  CHECK( best == *std::max_element( A.begin() , A.end() ) -
                 *std::min_element( A.begin() , A.end() ) );
  }
 for( std::size_t n = 2 ; n < 8 ; ++n ) {
  const auto [ i , j ] = maxGap( std::vector< long >( n , 9 ) );
  CHECK( i < j );
  }
 CHECK( maxGap( { 5 , 3 } ) == std::pair( std::size_t( 0 ) ,
                                         std::size_t( 1 ) ) );
 const long M = std::numeric_limits< long >::max();
 CHECK( maxGap( { 2 , M , 1 , M } ) == std::pair( std::size_t( 1 ) ,
                                                 std::size_t( 2 ) ) );
 const long m = std::numeric_limits< long >::min();
 CHECK( maxGap( { 0 , M , -5 , m , 7 } ) == std::pair( std::size_t( 1 ) ,
                                                      std::size_t( 3 ) ) );
 CHECK( maxGap( { 4 , 4 , 9 } ) == std::pair( std::size_t( 0 ) ,
                                             std::size_t( 2 ) ) );
 CHECK( maxGap( { 9 , 4 , 4 } ) == std::pair( std::size_t( 0 ) ,
                                             std::size_t( 1 ) ) );
 CHECK_THROWS( maxGap( { 1 } ) , contract_violation );
 CHECK_THROWS( maxGap( {} ) , contract_violation );
 }

/*--------------- 2011-04-11 A2, minimum height search tree ----------------*/

// the tree of every size up to 300 and of random increasing vectors: in
// order the keys of A, the order of a search tree, the parents, and the
// least height ceil( log2( n + 1 ) ) - 1
ADS_TEST( "compiti-2011-04-11-A2" )
{
 std::mt19937 g( 3 );
 for( std::size_t n = 1 ; n <= 300 ; ++n ) {
  std::vector< long > A( n );
  long x = -long( g() % 100 );
  for( auto & a : A )
   a = ( x += 1 + long( g() % 5 ) );
  Pool P;
  TreeNode * T = buildMinHeightBST( P , A );
  std::vector< long > in;
  inorder( T , in );
  CHECK( in == A );
  CHECK( T->parent == nullptr );
  std::vector< TreeNode * > N;
  nodes( T , N );
  for( auto u : N ) {
   if( u->left )
    CHECK( ( u->left->parent == u ) && ( u->left->key < u->key ) );
   if( u->right )
    CHECK( ( u->right->parent == u ) && ( u->right->key > u->key ) );
   }
  int h = 0;
  while( ( std::size_t( 1 ) << ( h + 1 ) ) - 1 < n )
   ++h;
  CHECK( height( T ) == h );
  }
 Pool P;
 const long M = std::numeric_limits< long >::max();
 CHECK( buildMinHeightBST( P , { M } )->key == M );
 CHECK_THROWS( buildMinHeightBST( P , {} ) , contract_violation );
 CHECK_THROWS( buildMinHeightBST( P , { 1 , 1 } ) , contract_violation );
 }

/*--------- 2011-06-06 B2 and 2012-07-09 B2, nearest to the median ---------*/

// the example of 06/06/11, median 8, the answer 6 , 9 or 9 , 10; nearest()
// against the sorted distances of the items but the median, kMedian()
// against those of all the items, on random vectors of distinct items;
// ties on the two sides of the median, k = n - 1 and k = n, one item, and
// k = 0 or k too large
ADS_TEST( "compiti-2011-06-06-B2" )
{
 std::mt19937 g( 4 );
 const std::vector< long > V = { 1 , 6 , 4 , 3 , 9 , 12 , 15 , 2 , 8 , 10 ,
                                 11 };
 auto R = nearest( V , 2 , g );
 std::sort( R.begin() , R.end() );
 CHECK( ( R == std::vector< long >( { 6 , 9 } ) ) ||
        ( R == std::vector< long >( { 9 , 10 } ) ) );
 for( int t = 0 ; t < 600 ; ++t ) {
  const std::size_t n = 1 + g() % 40;
  auto A = randomDistinct( n , g );
  for( auto & a : A )
   a -= long( n );
  auto S = A;
  std::sort( S.begin() , S.end() );
  const std::size_t k = 1 + g() % n;
  if( k < n ) {                                         // nearest()
   const long m = S[ ( n + 1 ) / 2 - 1 ];
   std::vector< long > D;
   for( long a : A )
    if( a != m )
     D.push_back( std::abs( a - m ) );
   std::sort( D.begin() , D.end() );
   D.resize( k );
   const auto N = nearest( A , k , g );
   std::vector< long > E;
   for( long a : N )
    E.push_back( std::abs( a - m ) );
   std::sort( E.begin() , E.end() );
   CHECK( E == D );
   CHECK( std::find( N.begin() , N.end() , m ) == N.end() );
   CHECK( distinct( N ) );
   for( long a : N )
    CHECK( std::find( A.begin() , A.end() , a ) != A.end() );
   }
  const long m = S[ std::max< std::size_t >( n / 2 , 1 ) - 1 ];  // kMedian()
  std::vector< long > D;
  for( long a : A )
   D.push_back( std::abs( a - m ) );
  auto Ds = D;
  std::sort( Ds.begin() , Ds.end() );
  std::vector< long > E;
  for( std::size_t i = 0 ; i < n ; ++i )
   if( D[ i ] <= Ds[ k - 1 ] )
    E.push_back( A[ i ] );
  CHECK( kMedian( A , k , g ) == E );
  CHECK( ( E.size() == k ) || ( E.size() == k + 1 ) );
  }
 CHECK( kMedian( { 5 } , 1 , g ) == std::vector< long >( { 5 } ) );
 // a tie at the same distance on the two sides goes by the order in V;
 // k = n - 1 takes all but the median, k = n for kMedian() all
 CHECK( nearest( { 3 , 2 , 1 } , 1 , g ) == std::vector< long >( { 3 } ) );
 CHECK( nearest( { 1 , 2 , 3 } , 1 , g ) == std::vector< long >( { 1 } ) );
 CHECK( nearest( { 9 , -4 , 0 , 7 } , 3 , g ) ==
        std::vector< long >( { -4 , 7 , 9 } ) );   // closer ones, then at d
 CHECK( kMedian( { 9 , -4 , 0 , 7 } , 4 , g ) ==
        std::vector< long >( { 9 , -4 , 0 , 7 } ) );
 CHECK( kMedian( { 2 , 1 } , 1 , g ) == std::vector< long >( { 1 } ) );
 CHECK_THROWS( nearest( { 1 , 2 } , 0 , g ) , contract_violation );
 CHECK_THROWS( kMedian( { 1 , 2 } , 0 , g ) , contract_violation );
 CHECK_THROWS( kMedian( {} , 1 , g ) , contract_violation );
 CHECK_THROWS( nearest( { 5 } , 1 , g ) , contract_violation );
 CHECK_THROWS( nearest( { 1 , 2 , 2 } , 1 , g ) , contract_violation );
 CHECK_THROWS( kMedian( { 1 , 2 } , 3 , g ) , contract_violation );
 }

/*----------------------- 2011-09-07 A4, inversions ------------------------*/

// countInversions() of the library against the double loop, on random
// vectors, empty, of one item, of equal items, sorted and reversed, of two
// items, with ties, of the extremes of long
ADS_TEST( "compiti-2011-09-07-A4" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 400 ; ++t ) {
  auto A = randomVector( g() % 60 , -20 , ( t % 2 ) ? 3 : 100 , g );
  long inv = 0;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   for( std::size_t j = i + 1 ; j < A.size() ; ++j )
    inv += ( A[ i ] > A[ j ] );
  CHECK( countInversions( A ) == inv );
  CHECK( std::is_sorted( A.begin() , A.end() ) );
  }
 std::vector< long > E;
 CHECK( countInversions( E ) == 0 );
 std::vector< long > O( 1 , 7 ) , Q( 50 , 7 ) , R( 1000 );
 CHECK( countInversions( O ) == 0 );
 CHECK( countInversions( Q ) == 0 );
 std::iota( R.rbegin() , R.rend() , 0L );
 CHECK( countInversions( R ) == 1000L * 999 / 2 );
 CHECK( countInversions( R ) == 0 );
 const long M = std::numeric_limits< long >::max();
 std::vector< long > T2 = { 2 , 1 } , S2 = { 1 , 2 } ,
                     X = { M , 0 , -M - 1 , M , -M - 1 } , Ti = { 2 , 2 , 1 };
 CHECK( countInversions( T2 ) == 1 && countInversions( S2 ) == 0 );
 CHECK( countInversions( X ) == 3 + 2 + 0 + 1 );   // equal: no
 CHECK( countInversions( Ti ) == 2 );
 CHECK( X == std::vector< long >( { -M - 1 , -M - 1 , 0 , M , M } ) );
 }

/*----------------------- 2012-02-01 A1, MergeSortK ------------------------*/

// T( n ) = k T( n / k ) + k n on n = k^j is n + j k n, between k n log_k n
// and ( 1 + 1 / k ) k n log_k n; master( k , k , 1 ) is Theta( n log n );
// MergeSortK sorts and makes at most ( k - 1 ) n ceil( log_k n )
// comparisons; empty, one item, k = n and k > n (a merge of n runs of
// one item), the extreme values, k < 2
ADS_TEST( "compiti-2012-02-01-A1" )
{
 for( std::size_t k = 2 ; k <= 8 ; ++k ) {
  CHECK( master( double( k ) , double( k ) , 1 ) == theta( 1 , 1 ) );
  double T = 1 , n = 1;
  for( int j = 1 ; j <= 12 ; ++j ) {
   n *= double( k );
   T = double( k ) * T + double( k ) * n;
   const double r = T / ( double( k ) * n * j );
   CHECK( within( r , 1 , 1 + 1.0 / double( k ) + 1e-9 ) );
   }
  }
 std::mt19937 g( 6 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t k = 2 + g() % 7 , n = g() % 400;
  auto A = randomVector( n , -50 , 50 , g ) , B = A;
  const long cmp = mergeSortK( A , k );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  std::size_t levels = 0;
  for( std::size_t p = 1 ; p < n ; p *= k )
   ++levels;
  CHECK( cmp <= long( ( k - 1 ) * n * levels ) );
  }
 std::vector< long > E;
 CHECK( mergeSortK( E , 2 ) == 0 );
 CHECK_THROWS( mergeSortK( E , 1 ) , contract_violation );
 CHECK_THROWS( mergeSortK( E , 0 ) , contract_violation );
 // one item, k = n (one merge of n runs of one item), k > n, sorted and
 // reversed input, the extreme values
 std::vector< long > O( 1 , 5 );
 CHECK( ( mergeSortK( O , 3 ) == 0 ) && ( O[ 0 ] == 5 ) );
 for( const std::size_t k : { std::size_t( 6 ) , std::size_t( 100 ) } ) {
  std::vector< long > R = { 6 , 5 , 4 , 3 , 2 , 1 };
  CHECK( mergeSortK( R , k ) == 5 + 4 + 3 + 2 + 1 );
  CHECK( R == std::vector< long >( { 1 , 2 , 3 , 4 , 5 , 6 } ) );
  }
 const long M = std::numeric_limits< long >::max() ,
            m = std::numeric_limits< long >::min();
 std::vector< long > X = { M , m , 0 , M , m };
 mergeSortK( X , 2 );
 CHECK( X == std::vector< long >( { m , m , 0 , M , M } ) );
 }

/*---------------------- 2012-05-03 A2, the node Roma ----------------------*/

// isRoma() against a search from every node, on random directed graphs;
// a node alone, a cycle, a star into the center and two nodes apart
ADS_TEST( "compiti-2012-05-03-A2-i" )
{
 std::mt19937 g( 7 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = 1 + int( g() % 9 );
  const auto E = randomEdges( n , 10 + int( g() % 50 ) , g , true );
  const auto G = makeGraph( n , E , true );
  const auto GT = transpose( G );
  const std::vector< bool > none( n , false );
  for( int v = 0 ; v < n ; ++v ) {
   bool all = true;
   for( int u = 0 ; u < n ; ++u )
    all = all && reach( n , E , u , none , true )[ v ];
   CHECK( isRoma( GT , v ) == all );
   }
  }
 CHECK( isRoma( transpose( makeGraph( 1 , {} , true ) ) , 0 ) );
 const auto C = makeGraph( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } ,
                                 { 3 , 0 } } , true );
 for( int v = 0 ; v < 4 ; ++v )
  CHECK( isRoma( transpose( C ) , v ) );
 const auto S = makeGraph( 4 , { { 1 , 0 } , { 2 , 0 } , { 3 , 0 } } , true );
 CHECK( isRoma( transpose( S ) , 0 ) && ! isRoma( transpose( S ) , 1 ) );
 CHECK( ! isRoma( transpose( makeGraph( 2 , {} , true ) ) , 0 ) );
 CHECK_THROWS( isRoma( transpose( S ) , 4 ) , contract_violation );
 }

// hasRoma() against isRoma() on every node, on random directed graphs;
// the empty graph has none; the Roma first or last, a chain of 2000
// nodes, strongly connected parts joined one way or not at all
ADS_TEST( "compiti-2012-05-03-A2-ii" )
{
 std::mt19937 g( 8 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const int n = 1 + int( g() % 9 );
  const auto E = randomEdges( n , 5 + int( g() % 50 ) , g , true );
  const auto G = makeGraph( n , E , true );
  const std::vector< bool > none( n , false );
  bool any = false;
  for( int v = 0 ; v < n ; ++v ) {
   bool all = true;
   for( int u = 0 ; u < n ; ++u )
    all = all && reach( n , E , u , none , true )[ v ];
   any = any || all;
   }
  CHECK( hasRoma( G ) == any );
  }
 CHECK( ! hasRoma( makeGraph( 0 , {} , true ) ) );
 CHECK( hasRoma( makeGraph( 1 , {} , true ) ) );
 CHECK( ! hasRoma( makeGraph( 2 , {} , true ) ) );
 CHECK( hasRoma( makeGraph( 3 , { { 0 , 1 } , { 2 , 1 } } , true ) ) );
 CHECK( ! hasRoma( makeGraph( 3 , { { 1 , 0 } , { 1 , 2 } } , true ) ) );
 // a Roma that is the first node, the last one, and a chain of 2000
 // nodes ending in it; two strongly connected parts joined one way
 CHECK( hasRoma( makeGraph( 3 , { { 1 , 0 } , { 2 , 0 } } , true ) ) );
 CHECK( hasRoma( makeGraph( 3 , { { 0 , 2 } , { 1 , 2 } } , true ) ) );
 std::vector< std::pair< int , int > > chain;
 for( int v = 0 ; v + 1 < 2000 ; ++v )
  chain.emplace_back( v , v + 1 );
 const auto Ch = makeGraph( 2000 , chain , true );
 CHECK( hasRoma( Ch ) && isRoma( transpose( Ch ) , 1999 ) &&
        ! isRoma( transpose( Ch ) , 1998 ) );
 CHECK( hasRoma( makeGraph( 4 , { { 0 , 1 } , { 1 , 0 } , { 2 , 3 } ,
                                  { 3 , 2 } , { 0 , 2 } } , true ) ) );
 CHECK( ! hasRoma( makeGraph( 4 , { { 0 , 1 } , { 1 , 0 } , { 2 , 3 } ,
                                    { 3 , 2 } } , true ) ) );
 }

/*--------------- 2012-06-18 A2, merge sort by square roots ----------------*/

// T( n ) = sqrt( n ) T( sqrt( n ) ) + n^{3/2} on n = 2^{2^j}, T( n ) = 1
// below 4, stays within [ 1 , 2 ] n^{3/2}; the sort agrees with std::sort
// and its comparisons grow as n^{3/2}, well beyond n log2 n; empty, one
// item, equal items, every permutation up to 7 items, and the comparisons
// of the insertion sort of 2 and 3 items
ADS_TEST( "compiti-2012-06-18-A2" )
{
 double T = 1 , n = 2;
 for( int j = 1 ; j <= 5 ; ++j ) {
  const double s = n;
  n = s * s;
  T = s * T + n * s;
  CHECK( within( T / std::pow( n , 1.5 ) , 1 , 2 ) );
  }
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 300 ; ++t ) {
  auto A = randomVector( g() % 300 , -30 , ( t % 2 ) ? 3 : 1000 , g ) ,
       B = A;
  sqrtMergeSort( A );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 for( std::size_t m = 1024 ; m <= 65536 ; m *= 4 ) {
  auto A = randomVector( m , 0 , 1000000 , g );
  const double c = double( sqrtMergeSort( A ) );
  CHECK( within( c / std::pow( double( m ) , 1.5 ) , 0.5 , 2 ) );
  if( m == 65536 )
   CHECK( c > 4 * double( m ) * std::log2( double( m ) ) );
  }
 std::vector< long > E , O( 1 , 3 ) , Q( 100 , 3 );
 CHECK( sqrtMergeSort( E ) == 0 );
 CHECK( sqrtMergeSort( O ) == 0 );
 sqrtMergeSort( Q );
 CHECK( Q == std::vector< long >( 100 , 3 ) );
 // every permutation up to 7 items, across the passage from the insertion
 // sort (n < 4) to the blocks; the insertion sort of 2 and 3 items
 for( std::size_t n = 2 ; n <= 7 ; ++n ) {
  std::vector< long > P( n );
  std::iota( P.begin() , P.end() , -2L );
  const auto S = P;
  do {
   auto A = P;
   sqrtMergeSort( A );
   CHECK( A == S );
   }
  while( std::next_permutation( P.begin() , P.end() ) );
  }
 std::vector< long > R2 = { 2 , 1 } , R3 = { 3 , 2 , 1 } , S3 = { 1 , 2 , 3 };
 CHECK( ( sqrtMergeSort( R2 ) == 1 ) && ( sqrtMergeSort( R3 ) == 3 ) );
 CHECK( sqrtMergeSort( S3 ) == 2 );
 }

/*----------------------- 2012-09-10 A1, SortinoSort -----------------------*/

// master( 3 , 3 / 2 , 0 ) is Theta( n^{log_{3/2} 3} ), the exponent in
// ( 2 , 3 ); the calls of SortinoSort are T( n ) = 1 + 2 T( 2 s ) +
// T( n - s ), whose slope in log-log between 10^4 and 10^6 is that
// exponent; the sort agrees with std::sort, also with repeated items
ADS_TEST( "compiti-2012-09-10-A1-i" )
{
 const double alpha = std::log( 3.0 ) / std::log( 1.5 );
 const Growth G = master( 3 , 1.5 , 0 );
 CHECK( std::abs( G.p - alpha ) < 1e-12 && ( G.q == 0 ) );
 CHECK( ( alpha > 2 ) && ( alpha < 3 ) && ( std::abs( alpha - 2.7095 ) <
                                            1e-4 ) );
 const std::size_t N = 1000000;
 std::vector< double > T( N + 1 , 1 );
 for( std::size_t n = 7 ; n <= N ; ++n ) {
  const std::size_t s = ( n + 2 ) / 3;
  T[ n ] = 1 + 2 * T[ 2 * s ] + T[ n - s ];
  }
 const double slope = std::log( T[ N ] / T[ N / 100 ] ) / std::log( 100.0 );
 CHECK( std::abs( slope - alpha ) < 0.1 );
 for( std::size_t n : { 1 , 6 , 7 , 20 , 100 , 250 } ) {
  std::vector< long > A( n );
  std::iota( A.begin() , A.end() , 0L );
  CHECK( sortinoSort( A ) == long( T[ n ] ) );
  }
 }

// SortinoSort against std::sort on random vectors with repeated items, on
// the empty one, a reversed one, and every permutation of up to 9 items
ADS_TEST( "compiti-2012-09-10-A1-ii" )
{
 std::mt19937 g( 10 );
 for( int t = 0 ; t < 150 ; ++t ) {
  auto A = randomVector( g() % 200 , -10 , ( t % 3 ) ? 1000 : 4 , g ) ,
       B = A;
  sortinoSort( A );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 std::vector< long > E , R( 150 );
 CHECK( sortinoSort( E ) == 0 );
 std::iota( R.rbegin() , R.rend() , 0L );
 sortinoSort( R );
 CHECK( std::is_sorted( R.begin() , R.end() ) );
 // every permutation of up to 9 items, across the insertion sort (up to 6)
 // and the three recursive calls (7 and more), and one item
 for( std::size_t n = 1 ; n <= 9 ; ++n ) {
  std::vector< long > P( n );
  std::iota( P.begin() , P.end() , 0L );
  const auto S = P;
  do {
   auto A = P;
   CHECK( sortinoSort( A ) == ( n <= 6 ? 1 : 4 ) );   // thirds of <= 6
   CHECK( A == S );
   }
  while( std::next_permutation( P.begin() , P.end() ) );
  }
 }

/*--------- 2013-01-07 A1 and A2, disjoint vectors and sum of two ----------*/

// disjoint() against a set, on random vectors of distinct items, empty
// and with one common item at the ends
ADS_TEST( "compiti-2013-01-07-A1" )
{
 std::mt19937 g( 11 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const auto A = randomDistinct( g() % 20 , g ) ,
             B = randomDistinct( g() % 40 , g );
  const std::set< long > S( A.begin() , A.end() );
  bool common = false;
  for( long b : B )
   common = common || S.count( b );
  CHECK( disjoint( A , B ) == ! common );
  }
 CHECK( disjoint( {} , {} ) && disjoint( {} , { 1 } ) );
 CHECK( ! disjoint( { 1 , 9 } , { 9 , 2 , 3 } ) );
 CHECK( ! disjoint( { 1 , 9 } , { 0 , 1 } ) );
 const long M = std::numeric_limits< long >::max();
 CHECK( disjoint( { M } , { M - 1 , -M } ) );
 CHECK_THROWS( disjoint( { 1 , 1 } , { 2 } ) , contract_violation );
 }

// twoSum() against the double loop, on random sorted vectors with
// repeated items; empty, one item, x twice an item present once or twice,
// the pair at the ends, negative sums, the extremes of long
ADS_TEST( "compiti-2013-01-07-A2" )
{
 std::mt19937 g( 12 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  auto S = randomVector( g() % 25 , -20 , 20 , g );
  std::sort( S.begin() , S.end() );
  const long x = long( g() % 81 ) - 40;
  bool any = false;
  for( std::size_t i = 0 ; i < S.size() ; ++i )
   for( std::size_t j = i + 1 ; j < S.size() ; ++j )
    any = any || ( S[ i ] + S[ j ] == x );
  CHECK( twoSum( S , x ) == any );
  }
 CHECK( ! twoSum( {} , 0 ) && ! twoSum( { 3 } , 6 ) );
 CHECK( twoSum( { 3 , 3 } , 6 ) && ! twoSum( { 2 , 3 , 5 } , 6 ) );
 CHECK( twoSum( std::vector< long >( 10 , 7 ) , 14 ) );
 // the pair at the two ends, negative sums, the extremes of long whose sum
 // fits (-LONG_MAX + LONG_MAX = 0)
 const long M = std::numeric_limits< long >::max();
 CHECK( twoSum( { 1 , 5 , 6 , 9 } , 10 ) && twoSum( { -7 , -2 , 4 } , -9 ) );
 CHECK( twoSum( { -M , 0 , M } , 0 ) && twoSum( { -M , 0 , M } , M ) );
 CHECK( ! twoSum( { -M , M } , 1 ) );
 CHECK_THROWS( twoSum( { 2 , 1 } , 3 ) , contract_violation );
 }

/*------------------------ 2013-06-17 A2, four rows ------------------------*/

// fourRows() against the four blocks of the sorted vector, on random
// vectors of distinct items, empty, of four items, of the extremes of long
ADS_TEST( "compiti-2013-06-17-A2" )
{
 std::mt19937 g( 13 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const std::size_t n = g() % 30;
  const auto V = randomDistinct( 4 * n , g );
  auto S = V;
  std::sort( S.begin() , S.end() );
  auto B = fourRows( V , g );
  for( std::size_t r = 0 ; r < 4 ; ++r ) {
   std::sort( B[ r ].begin() , B[ r ].end() );
   const auto b = S.begin() + std::ptrdiff_t( r * n );
   CHECK( B[ r ] == std::vector< long >( b , b + std::ptrdiff_t( n ) ) );
   }
  }
 const auto B = fourRows( { 4 , 3 , 2 , 1 } , g );
 for( long r = 0 ; r < 4 ; ++r )
  CHECK( B[ r ] == std::vector< long >( 1 , r + 1 ) );
 for( const auto & r : fourRows( {} , g ) )
  CHECK( r.empty() );
 const long M = std::numeric_limits< long >::max();
 auto X = fourRows( { M , -M , 0 , -1 , 1 , M - 1 , -M + 1 , 2 } , g );
 for( auto & r : X )
  std::sort( r.begin() , r.end() );
 CHECK( ( X[ 0 ] == std::vector< long >( { -M , -M + 1 } ) ) &&
        ( X[ 1 ] == std::vector< long >( { -1 , 0 } ) ) &&
        ( X[ 2 ] == std::vector< long >( { 1 , 2 } ) ) &&
        ( X[ 3 ] == std::vector< long >( { M - 1 , M } ) ) );
 CHECK_THROWS( fourRows( { 1 , 2 , 3 } , g ) , contract_violation );
 CHECK_THROWS( fourRows( { 1 , 2 , 3 , 3 } , g ) , contract_violation );
 }

/*---------- 2013-09-02 A2 and 2019-08-22 A2, red-black colorings ----------*/

// verifyRB() against the rules checked path by path: on every shape of up
// to 6 nodes with every coloring, on the trees of RedBlackTree after
// random insertions and removals, and on them with a color changed
ADS_TEST( "compiti-2013-09-02-A2" )
{
 for( std::size_t n = 0 ; n <= 6 ; ++n )
  for( const auto & s : shapes( n ) )
   for( unsigned c = 0 ; c < ( 1u << n ) ; ++c ) {
    Pool P;
    std::size_t i = 0;
    long key = 0;
    TreeNode * T = buildShape( P , s , i , n , key );
    std::vector< TreeNode * > N;
    nodes( T , N );
    for( std::size_t j = 0 ; j < n ; ++j )
     N[ j ]->color = ( c >> j & 1 ) ? Color::RED : Color::BLACK;
    CHECK( verifyRB( T ) == redBlackByPaths( T ) );
    }
 std::mt19937 g( 14 );
 for( int t = 0 ; t < 200 ; ++t ) {
  RedBlackTree< long , int > R;
  for( int i = 0 , m = int( g() % 80 ) ; i < m ; ++i )
   if( g() % 4 )
    R.insert( long( g() % 100 ) , i );
   else
    R.remove( long( g() % 100 ) );
  Pool P;
  TreeNode * T = copyTree( P , R , R.root() );
  CHECK( verifyRB( T ) && redBlackByPaths( T ) );
  std::vector< TreeNode * > N;
  nodes( T , N );
  if( ! N.empty() ) {
   auto u = N[ g() % N.size() ];
   u->color = isRed( u ) ? Color::BLACK : Color::RED;
   CHECK( verifyRB( T ) == redBlackByPaths( T ) );
   }
  }
 CHECK( verifyRB( nullptr ) );
 }

// isRedBlack() against the rules checked path by path, as verifyRB()
ADS_TEST( "compiti-2019-08-22-A2" )
{
 for( std::size_t n = 0 ; n <= 6 ; ++n )
  for( const auto & s : shapes( n ) )
   for( unsigned c = 0 ; c < ( 1u << n ) ; ++c ) {
    Pool P;
    std::size_t i = 0;
    long key = 0;
    TreeNode * T = buildShape( P , s , i , n , key );
    std::vector< TreeNode * > N;
    nodes( T , N );
    for( std::size_t j = 0 ; j < n ; ++j )
     N[ j ]->color = ( c >> j & 1 ) ? Color::RED : Color::BLACK;
    CHECK( isRedBlack( T ) == redBlackByPaths( T ) );
    }
 std::mt19937 g( 15 );
 for( int t = 0 ; t < 200 ; ++t ) {
  RedBlackTree< long , int > R;
  for( int i = 0 , m = int( g() % 80 ) ; i < m ; ++i )
   R.insert( long( g() % 1000 ) , i );
  Pool P;
  TreeNode * T = copyTree( P , R , R.root() );
  CHECK( isRedBlack( T ) );
  std::vector< TreeNode * > N;
  nodes( T , N );
  if( ! N.empty() ) {
   auto u = N[ g() % N.size() ];
   u->color = isRed( u ) ? Color::BLACK : Color::RED;
   CHECK( isRedBlack( T ) == redBlackByPaths( T ) );
   }
  }
 CHECK( isRedBlack( nullptr ) );
 }

/*---------------------- 2014-10-31 A2, almost sorted ----------------------*/

// almostSort() against std::sort on vectors whose first n - floor(
// n^{4/5} ) items are sorted, up to n = 10^6; n = 0 .. 3, a tail all
// smaller than the head; the precondition is checked
ADS_TEST( "compiti-2014-10-31-A2" )
{
 std::mt19937 g( 16 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t n = ( t == 0 ) ? 1000000 : g() % 3000;
  auto V = randomVector( n , -1000 , 1000 , g );
  const std::size_t p = n - unsortedTail( n );
  std::sort( V.begin() , V.begin() + std::ptrdiff_t( p ) );
  auto S = V;
  std::sort( S.begin() , S.end() );
  almostSort( V );
  CHECK( V == S );
  }
 std::vector< long > E , O( 1 , 2 ) , R = { 4 , 5 , 3 , 2 , 1 , 0 };
 almostSort( E );
 almostSort( O );
 CHECK( E.empty() && ( O == std::vector< long >( 1 , 2 ) ) );
 almostSort( R );                                       // 6 - 4 = 2 sorted
 CHECK( std::is_sorted( R.begin() , R.end() ) );
 // n = 2 and 3: one item sorted and the rest not (floor( 2^0.8 ) = 1,
 // floor( 3^0.8 ) = 2); the tail all smaller than the sorted head
 CHECK( ( unsortedTail( 2 ) == 1 ) && ( unsortedTail( 3 ) == 2 ) );
 std::vector< long > T2 = { 2 , 1 } , T3 = { 5 , 9 , -1 } ,
                     H = { 10 , 11 , 12 , 13 , 3 , 2 , 1 , 0 , -1 , -2 };
 almostSort( T2 );
 almostSort( T3 );
 CHECK( ( T2 == std::vector< long >( { 1 , 2 } ) ) &&
        ( T3 == std::vector< long >( { -1 , 5 , 9 } ) ) );
 CHECK( unsortedTail( 10 ) == 6 );
 almostSort( H );
 CHECK( H == std::vector< long >( { -2 , -1 , 0 , 1 , 2 , 3 , 10 , 11 , 12 ,
                                    13 } ) );
 std::vector< long > B = { 9 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
 CHECK_THROWS( almostSort( B ) , contract_violation );
 }

/*--------------------- 2014-12-19 B1, unit intervals ----------------------*/

// the example, 2 intervals; minIntervals() against towers() with d = 1 / 2
// and against all the sets of intervals starting at the values, on random
// multiples of 1 / 4; empty, one value, equal values, values 1 apart,
// negative values, unsorted input, a gap just above 1
ADS_TEST( "compiti-2014-12-19-B1" )
{
 CHECK( minIntervals( { 2.5 , 3.8 , 1.5 , 3.1 , 1.8 } ) == 2 );
 std::mt19937 g( 17 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 11;
  std::vector< double > V( n );
  for( auto & v : V )
   v = double( g() % 40 ) / 4;
  int best = int( n );
  for( unsigned s = 0 ; s < ( 1u << n ) ; ++s ) {
   bool covered = true;
   for( double v : V ) {
    bool c = false;
    for( std::size_t i = 0 ; i < n ; ++i )
     c = c || ( ( s >> i & 1 ) && ( V[ i ] <= v ) && ( v <= V[ i ] + 1 ) );
    covered = covered && c;
    }
   if( covered )
    best = std::min( best , std::popcount( s ) );
   }
  CHECK( minIntervals( V ) == best );
  CHECK( minIntervals( V ) == int( towers( V , 0.5 ).size() ) );
  }
 CHECK( minIntervals( {} ) == 0 && minIntervals( { 1e9 } ) == 1 );
 CHECK( minIntervals( std::vector< double >( 20 , 3 ) ) == 1 );
 CHECK( minIntervals( { 0 , 1 } ) == 1 && minIntervals( { 0 , 1.25 } ) == 2 );
 CHECK( minIntervals( { 0 , 1 , 2 , 3 , 4 } ) == 3 );
 // negative values, unsorted input, a gap just above 1
 CHECK( minIntervals( { -0.5 , -3 , -2 , 0.5 } ) == 2 );
 CHECK( minIntervals( { 4 , 0 , 2 } ) == 3 );
 CHECK( minIntervals( { 0 , std::nextafter( 1.0 , 2.0 ) } ) == 2 );
 }

/*--------------------- 2015-06-08 A2, connected order ---------------------*/

// after deleting each prefix of the order the other nodes stay connected,
// on random connected undirected graphs; no node, one node, two nodes, a
// path, a star, a complete graph
ADS_TEST( "compiti-2015-06-08-A2" )
{
 std::mt19937 g( 18 );
 for( int t = 0 ; t < 400 ; ++t ) {
  const int n = 1 + int( g() % 12 );
  auto E = randomEdges( n , int( g() % 40 ) , g , false );
  for( int v = 1 ; v < n ; ++v )                        // a spanning tree
   E.emplace_back( int( g() % unsigned( v ) ) , v );
  std::set< std::pair< int , int > > U;
  for( auto [ u , v ] : E )
   U.insert( std::minmax( u , v ) );
  E.assign( U.begin() , U.end() );
  const auto order = connectedOrder( makeGraph( n , E , false ) );
  std::vector< int > S = order;
  std::sort( S.begin() , S.end() );
  std::vector< int > I( n );
  std::iota( I.begin() , I.end() , 0 );
  CHECK( S == I );
  std::vector< bool > deleted( n , false );
  for( int i = 0 ; i + 1 < n ; ++i ) {
   deleted[ order[ i ] ] = true;
   const auto seen = reach( n , E , order[ n - 1 ] , deleted , false );
   for( int v = 0 ; v < n ; ++v )
    CHECK( deleted[ v ] || seen[ v ] );
   }
  }
 CHECK( connectedOrder( makeGraph( 1 , {} , false ) ) ==
        std::vector< int >( 1 , 0 ) );
 const auto Pa = connectedOrder( makeGraph( 4 , { { 0 , 1 } , { 1 , 2 } ,
                                                  { 2 , 3 } } , false ) );
 CHECK( Pa == std::vector< int >( { 3 , 2 , 1 , 0 } ) );
 const auto St = connectedOrder( makeGraph( 4 , { { 0 , 1 } , { 0 , 2 } ,
                                                  { 0 , 3 } } , false ) );
 CHECK( St.back() == 0 );
 CHECK( connectedOrder( makeGraph( 0 , {} , false ) ).empty() );
 CHECK( connectedOrder( makeGraph( 2 , { { 0 , 1 } } , false ) ) ==
        std::vector< int >( { 1 , 0 } ) );
 const auto Cl = connectedOrder( makeGraph( 4 , { { 0 , 1 } , { 0 , 2 } ,
                                                  { 0 , 3 } , { 1 , 2 } ,
                                                  { 1 , 3 } , { 2 , 3 } } ,
                                            false ) );
 CHECK( Cl.size() == 4 );                   // complete: any order works
 CHECK_THROWS( connectedOrder( makeGraph( 2 , { { 0 , 1 } } , true ) ) ,
               contract_violation );
 }

/*----------------- 2015-07-14 B3.2, univocal set of edges -----------------*/

// maxMatching() against all the subsets of the edges, on random graphs
// of up to 14 edges; no node, no edge, one edge, a triangle, paths of
// three where the middle edge wins or loses; a loop, a weight not
// positive, an end out of the nodes
ADS_TEST( "compiti-2015-07-14-B3.2" )
{
 std::mt19937 g( 19 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 2 + int( g() % 7 );
  std::vector< WeightedEdge > E;
  for( auto [ u , v ] : randomEdges( n , 50 , g , false ) )
   if( E.size() < 14 )
    E.emplace_back( u , v , 1 + long( g() % 20 ) );
  long best = 0;
  for( unsigned s = 0 ; s < ( 1u << E.size() ) ; ++s ) {
   std::vector< bool > used( n , false );
   bool ok = true;
   long w = 0;
   for( std::size_t i = 0 ; i < E.size() ; ++i )
    if( s >> i & 1 ) {
     const auto [ u , v , c ] = E[ i ];
     ok = ok && ! used[ u ] && ! used[ v ];
     used[ u ] = used[ v ] = true;
     w += c;
     }
   if( ok )
    best = std::max( best , w );
   }
  const auto [ w , S ] = maxMatching( n , E );
  CHECK( w == best );
  std::vector< bool > used( n , false );
  long sw = 0;
  for( auto i : S ) {
   const auto [ u , v , c ] = E[ i ];
   CHECK( ! used[ u ] && ! used[ v ] );
   used[ u ] = used[ v ] = true;
   sw += c;
   }
  CHECK( sw == w );
  }
 CHECK( maxMatching( 3 , {} ).first == 0 );
 CHECK( maxMatching( 2 , { { 0 , 1 , 5 } } ).first == 5 );
 CHECK( maxMatching( 3 , { { 0 , 1 , 2 } , { 1 , 2 , 3 } ,
                           { 0 , 2 , 4 } } ).first == 4 );
 CHECK( maxMatching( 4 , { { 0 , 1 , 2 } , { 1 , 2 , 3 } ,
                           { 2 , 3 , 2 } } ).first == 4 );
 CHECK( maxMatching( 0 , {} ).first == 0 );
 CHECK( maxMatching( 1 , {} ).second.empty() );
 // a heavy middle edge beats the two ends, and the other way round
 CHECK( maxMatching( 4 , { { 0 , 1 , 2 } , { 1 , 2 , 5 } ,
                           { 2 , 3 , 2 } } ).first == 5 );
 CHECK( maxMatching( 4 , { { 0 , 1 , 3 } , { 1 , 2 , 5 } ,
                           { 2 , 3 , 3 } } ).first == 6 );
 CHECK_THROWS( maxMatching( 2 , { { 0 , 0 , 1 } } ) , contract_violation );
 CHECK_THROWS( maxMatching( 2 , { { 0 , 1 , 0 } } ) , contract_violation );
 CHECK_THROWS( maxMatching( 2 , { { 0 , 1 , -4 } } ) , contract_violation );
 CHECK_THROWS( maxMatching( 2 , { { 0 , 2 , 1 } } ) , contract_violation );
 }

/*------ 2015-11-05 A2 and A4, difference in a subtree and two values ------*/

// the example of the figure, 12 - 2 = 10; computeDiff() against the keys
// of each subtree, on random trees; one node, the largest keys, equal
// keys, a chain of 10^4 nodes, the extremes in leaves on both sides
ADS_TEST( "compiti-2015-11-05-A2b" )
{
 Pool P;
 TreeNode * F = makeNode( P , 2 , makeNode( P , 6 , makeNode( P , 10 ) ,
                                            makeNode( P , 12 ) ) ,
                          makeNode( P , 8 , makeNode( P , 4 ) ) );
 CHECK( computeDiff( F ) == 10 );
 CHECK( computeDiff( F->left ) == 6 && computeDiff( F->right ) == 4 );
 std::mt19937 g( 20 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Pool Q;
  TreeNode * T = randomTree( Q , 1 + g() % 40 , -100 , 100 , g );
  std::vector< TreeNode * > N;
  nodes( T , N );
  for( auto u : N ) {
   std::vector< long > K;
   inorder( u , K );
   const auto [ m , M ] = std::minmax_element( K.begin() , K.end() );
   CHECK( computeDiff( u ) == *M - *m );
   }
  }
 const long M = std::numeric_limits< long >::max() / 2;
 CHECK( computeDiff( makeNode( P , 7 ) ) == 0 );
 CHECK( computeDiff( makeNode( P , M , makeNode( P , -M ) ) ) == 2 * M );
 // equal keys, a chain of 10^4 nodes (the visit of the text is recursive,
 // its depth the height), the extremes in a leaf on each side
 CHECK( computeDiff( makeNode( P , 3 , makeNode( P , 3 ) ,
                               makeNode( P , 3 ) ) ) == 0 );
 TreeNode * chain = nullptr;
 for( long k = 0 ; k < 10000 ; ++k )
  chain = makeNode( P , k % 2 ? k : -k , chain );
 CHECK( computeDiff( chain ) == 9999 + 9998 );
 CHECK( computeDiff( makeNode( P , 0 , makeNode( P , 5 , makeNode( P , -M ) ) ,
                               makeNode( P , 1 , nullptr ,
                                         makeNode( P , M - 1 ) ) ) ) ==
        2 * M - 1 );
 CHECK_THROWS( computeDiff( nullptr ) , contract_violation );
 }

// the example, 6; mostFrequentAB() against std::count and upperBound(), on
// every split of every length up to 300; two items
ADS_TEST( "compiti-2015-11-05-A4b" )
{
 CHECK( mostFrequentAB( { 1 , 1 , 1 , 1 , 1 , 1 , 2 , 2 , 2 , 2 } ) == 6 );
 for( std::size_t n = 2 ; n <= 300 ; ++n )
  for( std::size_t k = 1 ; k < n ; ++k ) {
   std::vector< long > A( n , 9 );
   std::fill( A.begin() , A.begin() + std::ptrdiff_t( k ) , -3L );
   const auto c = std::size_t( std::count( A.begin() , A.end() , -3L ) );
   CHECK( upperBound( A , -3L ) == c );
   CHECK( mostFrequentAB( A ) == std::max( c , n - c ) );
   }
 CHECK( mostFrequentAB( { 0 , 1 } ) == 1 );
 CHECK_THROWS( mostFrequentAB( { 1 , 1 } ) , contract_violation );
 CHECK_THROWS( mostFrequentAB( { 1 , 2 , 3 } ) , contract_violation );
 }

/*----------------- 2016-01-26 B3, shuffle of two strings ------------------*/

// the examples of the text; isInterleaving() against the recursion on both
// choices, on random strings of a small alphabet; empty strings, equal
// strings, z too short, all one letter
ADS_TEST( "compiti-2016-01-26-B3" )
{
 for( const char * z : { "ABCD" , "ACBD" , "ACDB" , "CABD" , "CADB" ,
                         "CDAB" } )
  CHECK( isInterleaving( "AB" , "CD" , z ) );
 CHECK( ! isInterleaving( "AB" , "CD" , "BACD" ) );
 CHECK( isInterleaving( "123" , "345" , "123435" ) );
 CHECK( ! isInterleaving( "123" , "345" , "143235" ) );
 CHECK( ! isInterleaving( "123" , "345" , "123455" ) );
 std::function< bool( const std::string & , const std::string & ,
                      const std::string & ) > rec =
  [ & ]( const std::string & x , const std::string & y ,
         const std::string & z ) {
   if( z.size() != x.size() + y.size() )
    return( false );
   if( z.empty() )
    return( true );
   return( ( ! x.empty() && ( x[ 0 ] == z[ 0 ] ) &&
             rec( x.substr( 1 ) , y , z.substr( 1 ) ) ) ||
           ( ! y.empty() && ( y[ 0 ] == z[ 0 ] ) &&
             rec( x , y.substr( 1 ) , z.substr( 1 ) ) ) );
   };
 std::mt19937 g( 21 );
 auto word = [ & ]( std::size_t n ) {
  std::string s;
  for( std::size_t i = 0 ; i < n ; ++i )
   s += char( 'a' + g() % 2 );
  return( s );
  };
 for( int t = 0 ; t < 1000 ; ++t ) {
  const auto x = word( g() % 7 ) , y = word( g() % 7 );
  std::string z;
  if( t % 2 ) {                                         // a true shuffle
   std::size_t i = 0 , j = 0;
   while( ( i < x.size() ) || ( j < y.size() ) )
    if( ( j == y.size() ) || ( ( i < x.size() ) && ( g() % 2 ) ) )
     z += x[ i++ ];
    else
     z += y[ j++ ];
   }
  else
   z = word( x.size() + y.size() + ( g() % 5 == 0 ) );
  CHECK( isInterleaving( x , y , z ) == rec( x , y , z ) );
  if( t % 2 )
   CHECK( isInterleaving( x , y , z ) );
  }
 CHECK( isInterleaving( "" , "" , "" ) && isInterleaving( "ab" , "" , "ab" ) );
 CHECK( ! isInterleaving( "" , "" , "a" ) &&
        ! isInterleaving( "ab" , "" , "ba" ) );
 // equal strings (both choices at each step), z too short, all one letter
 CHECK( isInterleaving( "ab" , "ab" , "aabb" ) &&
        isInterleaving( "ab" , "ab" , "abab" ) );
 CHECK( ! isInterleaving( "ab" , "ab" , "abb" ) &&
        ! isInterleaving( "ab" , "ab" , "bbaa" ) );
 CHECK( isInterleaving( "aaa" , "aa" , "aaaaa" ) &&
        ! isInterleaving( "aaa" , "aa" , "aaaa" ) );
 }

/*----------------- 2016-07-06 A2, sorted rows and columns -----------------*/

// the example: the matrix of the text sorted row by row; random matrices
// keep their items with rows and columns nondecreasing; one row, one
// column, empty, rows of no item, negative and extreme values, rows of
// different lengths
ADS_TEST( "compiti-2016-07-06-A2" )
{
 std::vector< std::vector< long > > M = { { 5 , 10 , 4 , 14 } ,
                                          { 2 , 16 , 12 , 24 } ,
                                          { 7 , 9 , 8 , 8 } ,
                                          { 1 , 3 , 21 , 18 } };
 sortMatrix( M );
 const std::vector< std::vector< long > > R = { { 1 , 2 , 3 , 4 } ,
                                                { 5 , 7 , 8 , 8 } ,
                                                { 9 , 10 , 12 , 14 } ,
                                                { 16 , 18 , 21 , 24 } };
 CHECK( M == R );
 std::mt19937 g( 22 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t m = 1 + g() % 8 , n = 1 + g() % 8;
  std::vector< std::vector< long > > A( m );
  for( auto & r : A )
   r = randomVector( n , 0 , ( t % 2 ) ? 3 : 100 , g );
  std::multiset< long > before , after;
  for( const auto & r : A )
   before.insert( r.begin() , r.end() );
  sortMatrix( A );
  for( const auto & r : A )
   after.insert( r.begin() , r.end() );
  CHECK( before == after );
  for( std::size_t i = 0 ; i < m ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j ) {
    if( j + 1 < n )
     CHECK( A[ i ][ j ] <= A[ i ][ j + 1 ] );
    if( i + 1 < m )
     CHECK( A[ i ][ j ] <= A[ i + 1 ][ j ] );
    }
  }
 std::vector< std::vector< long > > E , C = { { 3 } , { 1 } , { 2 } };
 sortMatrix( E );
 sortMatrix( C );
 const std::vector< std::vector< long > > C3 = { { 1 } , { 2 } , { 3 } };
 CHECK( E.empty() && ( C == C3 ) );
 std::vector< std::vector< long > > Z = { {} , {} } , W = { { 2 , 1 } } ,
                                    X = { { 0 , -1 } , { std::numeric_limits<
                                          long >::min() , 5 } };
 sortMatrix( Z );
 sortMatrix( W );
 sortMatrix( X );
 CHECK( Z == std::vector< std::vector< long > >( 2 ) );   // empty rows
 CHECK( W == std::vector< std::vector< long > >( { { 1 , 2 } } ) );
 CHECK( X == std::vector< std::vector< long > >( {
  { std::numeric_limits< long >::min() , -1 } , { 0 , 5 } } ) );
 std::vector< std::vector< long > > J = { { 1 , 2 } , { 3 } };
 CHECK_THROWS( sortMatrix( J ) , contract_violation );
 }

/*-------------- 2016-08-29 B4, occurrences of a subsequence ---------------*/

// the example, abc in aabdcc 4 times; subsequenceOccurrences() against
// all the m-subsets of the places, on random strings; the empty pattern once,
// C( n , m ) for one letter, up to C( 60 , 30 ); P longer than T, P = T,
// the empty T, the letters of P in the wrong order
ADS_TEST( "compiti-2016-08-29-B4" )
{
 CHECK( subsequenceOccurrences( "abc" , "aabdcc" ) == 4 );
 std::mt19937 g( 23 );
 for( int t = 0 ; t < 400 ; ++t ) {
  std::string P , T;
  for( std::size_t i = 0 , m = g() % 5 ; i < m ; ++i )
   P += char( 'a' + g() % 2 );
  for( std::size_t i = 0 , n = g() % 13 ; i < n ; ++i )
   T += char( 'a' + g() % 3 );
  std::uint64_t c = 0;
  for( unsigned s = 0 ; s < ( 1u << T.size() ) ; ++s )
   if( std::size_t( std::popcount( s ) ) == P.size() ) {
    std::string q;
    for( std::size_t i = 0 ; i < T.size() ; ++i )
     if( s >> i & 1 )
      q += T[ i ];
    c += ( q == P );
    }
  CHECK( subsequenceOccurrences( P , T ) == c );
  }
 CHECK( subsequenceOccurrences( "" , "" ) == 1 );
 CHECK( subsequenceOccurrences( "" , "x" ) == 1 );
 CHECK( subsequenceOccurrences( "ab" , "a" ) == 0 );
 CHECK( subsequenceOccurrences( "abc" , "abc" ) == 1 );    // P = T
 CHECK( subsequenceOccurrences( "x" , "" ) == 0 );
 CHECK( subsequenceOccurrences( "ba" , "aabb" ) == 0 );    // order matters
 CHECK( subsequenceOccurrences( "z" , "azbzcz" ) == 3 );
 const auto C = tartaglia( 60 );
 for( int n = 0 ; n <= 60 ; n += 6 )
  for( int m = 0 ; m <= n ; m += 3 )
   CHECK( subsequenceOccurrences( std::string( m , 'a' ) ,
                                  std::string( n , 'a' ) ) == C[ n ][ m ] );
 }

/*--------------------- 2017-01-10 B4, files on a tape ---------------------*/

// B4.1-B4.3: the two counterexamples, 2.8 against 2.1 for the order by
// length and 10.4 against 7.0 for that by probability; the order by
// l / f against all the permutations; with equal probabilities it is the
// shortest job first, and the cost is f times totalCompletion(); no file,
// one file, all probabilities 0, ties of l / f, the contracts
ADS_TEST( "compiti-2017-01-10-B4.1" )
{
 const std::vector< double > l1 = { 1 , 2 } , f1 = { 0.1 , 0.9 } ,
                             l2 = { 10 , 1 } , f2 = { 0.6 , 0.4 };
 CHECK( std::abs( tapeCost( l1 , f1 , { 0 , 1 } ) - 2.8 ) < 1e-12 );
 CHECK( std::abs( tapeCost( l1 , f1 , { 1 , 0 } ) - 2.1 ) < 1e-12 );
 CHECK( std::abs( tapeCost( l2 , f2 , { 0 , 1 } ) - 10.4 ) < 1e-12 );
 CHECK( std::abs( tapeCost( l2 , f2 , { 1 , 0 } ) - 7.0 ) < 1e-12 );
 const auto byLength = orderBy( 2 , [ & ]( std::size_t a , std::size_t b ) {
  return( l1[ a ] < l1[ b ] );
  } );
 const auto byProbability = orderBy( 2 , [ & ]( std::size_t a ,
                                                std::size_t b ) {
  return( f2[ a ] > f2[ b ] );
  } );
 CHECK( tapeCost( l1 , f1 , byLength ) > tapeCost( l1 , f1 , { 1 , 0 } ) );
 CHECK( tapeCost( l2 , f2 , byProbability ) >
        tapeCost( l2 , f2 , { 1 , 0 } ) );
 CHECK( tapeOrder( l1 , f1 ) == std::vector< std::size_t >( { 1 , 0 } ) );
 CHECK( tapeOrder( l2 , f2 ) == std::vector< std::size_t >( { 1 , 0 } ) );
 std::mt19937 g( 24 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const std::size_t n = g() % 8;
  std::vector< double > l( n ) , f( n );
  double sum = 0;
  for( std::size_t i = 0 ; i < n ; ++i ) {
   l[ i ] = 1 + double( g() % 10 );
   f[ i ] = double( g() % 10 );
   sum += f[ i ];
   }
  for( auto & x : f )
   x = ( sum > 0 ) ? x / sum : x;
  std::vector< std::size_t > pi( n );
  std::iota( pi.begin() , pi.end() , std::size_t( 0 ) );
  double best = std::numeric_limits< double >::infinity();
  do
   best = std::min( best , tapeCost( l , f , pi ) );
  while( std::next_permutation( pi.begin() , pi.end() ) );
  CHECK( std::abs( tapeCost( l , f , tapeOrder( l , f ) ) - best ) < 1e-9 );
  const std::vector< double > e( n , 1 );
  CHECK( std::abs( tapeCost( l , e , tapeOrder( l , e ) ) -
                   totalCompletion( l , shortestJobFirst( l ) ) ) < 1e-9 );
  }
 // no file, one file, all probabilities 0 (every order costs 0), ties of
 // l / f (every order of the tied ones costs the same)
 CHECK( tapeOrder( {} , {} ).empty() && tapeCost( {} , {} , {} ) == 0 );
 CHECK( tapeOrder( { 3 } , { 1 } ) == std::vector< std::size_t >( 1 , 0 ) );
 CHECK( tapeCost( { 3 } , { 1 } , { 0 } ) == 3 );
 const std::vector< double > lz = { 4 , 1 , 2 } , fz( 3 , 0.0 );
 CHECK( tapeCost( lz , fz , tapeOrder( lz , fz ) ) == 0 );
 const std::vector< double > lt = { 2 , 1 , 4 } , ft = { 0.4 , 0.2 , 0.4 };
 CHECK( std::abs( tapeCost( lt , ft , { 0 , 1 , 2 } ) -
                  tapeCost( lt , ft , { 1 , 0 , 2 } ) ) < 1e-12 );
 CHECK( std::abs( tapeCost( lt , ft , tapeOrder( lt , ft ) ) -
                  tapeCost( lt , ft , { 0 , 1 , 2 } ) ) < 1e-12 );
 CHECK_THROWS( tapeOrder( { 0 } , { 1 } ) , contract_violation );
 CHECK_THROWS( tapeOrder( { 1 } , { -1 } ) , contract_violation );
 CHECK_THROWS( tapeOrder( { 1 , 2 } , { 1 } ) , contract_violation );
 CHECK_THROWS( tapeCost( { 1 , 2 } , { 1 , 0 } , { 0 } ) ,
               contract_violation );
 }

/*--------- 2017-06-08 A2 and B4, topological orders and decodings ---------*/

// topologicalOrders() against the count of the topological permutations,
// on random directed graphs of up to 7 nodes; empty, one node, a path,
// two nodes apart, a cycle, a cycle in a part apart, a path with a
// shortcut, an undirected graph
ADS_TEST( "compiti-2017-06-08-A2" )
{
 std::mt19937 g( 25 );
 for( int t = 0 ; t < 600 ; ++t ) {
  const int n = int( g() % 8 );
  auto E = randomEdges( n , int( g() % 60 ) , g , true );
  if( t % 2 ) {                                         // acyclic
   std::vector< std::pair< int , int > > F;
   for( auto [ u , v ] : E )
    if( u < v )
     F.emplace_back( u , v );
   E = F;
   }
  std::vector< int > pi( n );
  std::iota( pi.begin() , pi.end() , 0 );
  int count = 0;
  do {
   std::vector< int > pos( n );
   for( int i = 0 ; i < n ; ++i )
    pos[ pi[ i ] ] = i;
   bool ok = true;
   for( auto [ u , v ] : E )
    ok = ok && ( pos[ u ] < pos[ v ] );
   count += ok;
   }
  while( std::next_permutation( pi.begin() , pi.end() ) );
  CHECK( topologicalOrders( makeGraph( n , E , true ) ) ==
         std::min( count , 2 ) );
  }
 CHECK( topologicalOrders( makeGraph( 0 , {} , true ) ) == 1 );
 CHECK( topologicalOrders( makeGraph( 1 , {} , true ) ) == 1 );
 CHECK( topologicalOrders( makeGraph( 3 , { { 2 , 0 } , { 0 , 1 } } ,
                                      true ) ) == 1 );
 CHECK( topologicalOrders( makeGraph( 2 , {} , true ) ) == 2 );
 CHECK( topologicalOrders( makeGraph( 2 , { { 0 , 1 } , { 1 , 0 } } ,
                                      true ) ) == 0 );
 // a cycle away from a unique chain, a path with a shortcut (still
 // unique), and an undirected graph refused
 CHECK( topologicalOrders( makeGraph( 5 , { { 0 , 1 } , { 2 , 3 } ,
                                            { 3 , 4 } , { 4 , 2 } } ,
                                      true ) ) == 0 );
 CHECK( topologicalOrders( makeGraph( 3 , { { 0 , 1 } , { 1 , 2 } ,
                                            { 0 , 2 } } , true ) ) == 1 );
 CHECK_THROWS( topologicalOrders( makeGraph( 2 , { { 0 , 1 } } , false ) ) ,
               contract_violation );
 }

// the examples, 011 and 301 not decodable and 13135 in 4 ways;
// countDecodings() against the recursion on pieces of 1 and 2 digits, on
// random strings of digits; empty, 10, 26, 27, and n ones in F_n ways; a
// lone 0, 00, 100, 20, 30, a trailing 0, a digit out of 0 .. 9
ADS_TEST( "compiti-2017-06-08-B4" )
{
 CHECK( countDecodings( { 0 , 1 , 1 } ) == 0 &&
        countDecodings( { 3 , 0 , 1 } ) == 0 );
 CHECK( countDecodings( { 1 , 3 , 1 , 3 , 5 } ) == 4 );
 std::function< std::uint64_t( const std::vector< int > & , std::size_t ) >
  rec = [ & ]( const std::vector< int > & T , std::size_t i ) {
   if( i == T.size() )
    return( std::uint64_t( 1 ) );
   std::uint64_t c = 0;
   if( T[ i ] != 0 ) {
    c += rec( T , i + 1 );
    if( ( i + 1 < T.size() ) && ( 10 * T[ i ] + T[ i + 1 ] <= 26 ) )
     c += rec( T , i + 2 );
    }
   return( c );
   };
 std::mt19937 g( 26 );
 for( int t = 0 ; t < 1000 ; ++t ) {
  std::vector< int > T( g() % 15 );
  for( auto & d : T )
   d = ( t % 2 ) ? int( g() % 3 ) : int( g() % 10 );
  CHECK( countDecodings( T ) == rec( T , 0 ) );
  }
 CHECK( countDecodings( {} ) == 1 && countDecodings( { 1 , 0 } ) == 1 );
 CHECK( countDecodings( { 2 , 6 } ) == 2 && countDecodings( { 2 , 7 } ) == 1 );
 for( int n = 1 ; n <= 90 ; ++n )
  CHECK( countDecodings( std::vector< int >( n , 1 ) ) == fibonacci( n ) );
 // a lone 0, 00, 100 (the 0 cannot close two pieces), 20, 30, one digit,
 // a trailing 0
 CHECK( countDecodings( { 0 } ) == 0 && countDecodings( { 0 , 0 } ) == 0 );
 CHECK( countDecodings( { 1 , 0 , 0 } ) == 0 );
 CHECK( countDecodings( { 2 , 0 } ) == 1 && countDecodings( { 3 , 0 } ) == 0 );
 CHECK( countDecodings( { 9 } ) == 1 && countDecodings( { 1 , 1 , 0 } ) == 1 );
 CHECK_THROWS( countDecodings( { 10 } ) , contract_violation );
 CHECK_THROWS( countDecodings( { -1 } ) , contract_violation );
 }

/*--------------------- 2018-01-31 A2, keys in a range ---------------------*/

// the example, 3 keys in [ 11 , 18 ]; occurrences() against the count of
// the keys in order, on random search trees and ranges, visiting at most
// the keys in the range and two paths; the empty tree, a range of one key
// at either end, an empty range, repeated keys, lo > hi
ADS_TEST( "compiti-2018-01-31-A2" )
{
 Pool P;
 TreeNode * T = nullptr;
 for( long k : { 15 , 10 , 20 , 3 , 12 , 18 , 23 } )
  bstInsert( P , T , k );
 CHECK( occurrences( T , 11 , 18 ) == 3 );
 std::mt19937 g( 27 );
 for( int t = 0 ; t < 500 ; ++t ) {
  Pool Q;
  TreeNode * R = nullptr;
  for( long k : randomDistinct( g() % 60 , g ) )
   bstInsert( Q , R , k );
  std::vector< long > K;
  inorder( R , K );
  long lo = long( g() % 250 ) - 5 , hi = long( g() % 250 ) - 5;
  if( lo > hi )
   std::swap( lo , hi );
  const auto k = std::count_if( K.begin() , K.end() , [ & ]( long x ) {
   return( ( lo <= x ) && ( x <= hi ) );
   } );
  long visits = 0;
  CHECK( occurrences( R , lo , hi , &visits ) == k );
  CHECK( visits <= k + 2 * ( height( R ) + 1 ) );
  }
 CHECK( occurrences( nullptr , 0 , 0 ) == 0 );
 CHECK( occurrences( T , 15 , 15 ) == 1 && occurrences( T , 24 , 99 ) == 0 );
 const long M = std::numeric_limits< long >::max();
 CHECK( occurrences( T , -M , M ) == 7 );
 // the range at the smallest and at the largest key, empty between two
 // keys, and repeated keys (inserted on the right of their equals)
 CHECK( occurrences( T , 3 , 3 ) == 1 && occurrences( T , 23 , 23 ) == 1 );
 CHECK( occurrences( T , 16 , 17 ) == 0 && occurrences( T , -M , 2 ) == 0 );
 TreeNode * D = nullptr;
 for( long k : { 5 , 5 , 3 , 5 , 8 , 5 , 3 } )
  bstInsert( P , D , k );
 CHECK( occurrences( D , 5 , 5 ) == 4 && occurrences( D , 3 , 5 ) == 6 );
 CHECK( occurrences( D , 4 , 4 ) == 0 && occurrences( D , 3 , 3 ) == 2 );
 CHECK_THROWS( occurrences( T , 2 , 1 ) , contract_violation );
 }

/*-------------------- 2019-02-07 A2, first occurrence ---------------------*/

// the examples, 3 for v = 2 and 7 for v = 4; first() against a scan on
// random sorted vectors; equal items, one item, the first and the last
// run, the extremes of long; v absent, empty, A not sorted
ADS_TEST( "compiti-2019-02-07-A2" )
{
 const std::vector< long > A = { 1 , 1 , 2 , 2 , 2 , 3 , 4 , 5 , 6 , 6 };
 CHECK( first( A , 2 ) == 3 && first( A , 4 ) == 7 );
 std::mt19937 g( 28 );
 for( int t = 0 ; t < 500 ; ++t ) {
  auto S = randomVector( 1 + g() % 50 , -5 , 5 , g );
  std::sort( S.begin() , S.end() );
  const long v = S[ g() % S.size() ];
  const auto f = std::find( S.begin() , S.end() , v ) - S.begin();
  CHECK( first( S , v ) == std::size_t( f ) + 1 );
  }
 CHECK( first( std::vector< long >( 1000 , 8 ) , 8 ) == 1 );
 CHECK( first( { 4 } , 4 ) == 1 );
 CHECK( first( A , 1 ) == 1 && first( A , 6 ) == 9 );   // first, last run
 const long M = std::numeric_limits< long >::max();
 CHECK( first( { -M - 1 , -M - 1 , M } , M ) == 3 );
 CHECK( first( { -M - 1 , -M - 1 , M } , -M - 1 ) == 1 );
 CHECK_THROWS( first( A , 7 ) , contract_violation );
 CHECK_THROWS( first( {} , 0 ) , contract_violation );
 CHECK_THROWS( first( { 2 , 1 } , 1 ) , contract_violation );
 }

/*------------------- 2019-08-22 A1, merge by insertion --------------------*/

// the comparisons of the sort with insertionMerge(): on sorted input
// C( n ) = 2 C( n / 2 ) + n - 1 = n log2 n - n + 1, on reversed input
// C( n ) = 2 C( n / 2 ) + n^2 / 4 + n - 2, for n a power of 2; their
// doubling ratios tend to 2 and 4; master() gives Theta( n log n ) and
// Theta( n^2 ); the sort agrees with std::sort; one item, equal items (as
// sorted ones), two items reversed
ADS_TEST( "compiti-2019-08-22-A1" )
{
 CHECK( master( 2 , 2 , 1 ) == theta( 1 , 1 ) );
 CHECK( master( 2 , 2 , 2 ) == theta( 2 ) );
 double best = 0 , worst = 0 , pb = 0 , pw = 0;
 for( std::size_t n = 2 ; n <= 4096 ; n *= 2 ) {
  best = 2 * best + double( n ) - 1;
  worst = 2 * worst + double( n * n ) / 4 + double( n ) - 2;
  std::vector< long > S( n ) , R( n );
  std::iota( S.begin() , S.end() , 0L );
  std::iota( R.rbegin() , R.rend() , 0L );
  long cs = 0 , cr = 0;
  insertionMergeSort( S , 0 , n - 1 , cs );
  insertionMergeSort( R , 0 , n - 1 , cr );
  CHECK( double( cs ) == best && double( cr ) == worst );
  CHECK( std::is_sorted( R.begin() , R.end() ) );
  CHECK( best == double( n ) * std::log2( double( n ) ) - double( n ) + 1 );
  if( n >= 1024 ) {
   CHECK( within( best / pb , 2 , 2.3 ) );
   CHECK( within( worst / pw , 3.9 , 4 ) );
   }
  pb = best;
  pw = worst;
  }
 std::mt19937 g( 29 );
 for( int t = 0 ; t < 200 ; ++t ) {
  auto A = randomVector( 1 + g() % 200 , -9 , 9 , g ) , B = A;
  long c = 0;
  insertionMergeSort( A , 0 , A.size() - 1 , c );
  std::sort( B.begin() , B.end() );
  CHECK( A == B );
  }
 // one item: no comparison; equal items cost as sorted ones (the merge is
 // stable, an equal item never moves); 2 items reversed
 std::vector< long > O( 1 , 4 ) , Q( 64 , 4 ) , R2 = { 2 , 1 };
 long c1 = 0 , ce = 0 , c2 = 0;
 insertionMergeSort( O , 0 , 0 , c1 );
 insertionMergeSort( Q , 0 , 63 , ce );
 insertionMergeSort( R2 , 0 , 1 , c2 );
 CHECK( ( c1 == 0 ) && ( ce == 64 * 6 - 64 + 1 ) && ( c2 == 1 ) );
 CHECK( R2 == std::vector< long >( { 1 , 2 } ) );
 }

/*-------------- 2020-01-14 A1, insertion sorts in four parts --------------*/

// master( 4 , 4 , 2 ) is Theta( n^2 ); T( n ) = 4 T( floor( n / 4 ) ) + n^2,
// T( n ) = 1 for n <= 1, stays within [ 1 , 2 ] n^2 up to 10^6
ADS_TEST( "compiti-2020-01-14-A1" )
{
 CHECK( master( 4 , 4 , 2 ) == theta( 2 ) );
 const std::size_t N = 1000000;
 std::vector< double > T( N + 1 , 1 );
 for( std::size_t n = 2 ; n <= N ; ++n ) {
  T[ n ] = 4 * T[ n / 4 ] + double( n ) * double( n );
  CHECK( within( T[ n ] / ( double( n ) * double( n ) ) , 1 , 2 ) );
  }
 }

/*------------------ 2020-02-04 A1, heaps in a recursion -------------------*/

// master( 2 , 2 , 1 ) is Theta( n log n ); T( n ) = 2 T( floor( n / 2 ) )
// + n, T( n ) = 1 for n <= 2, stays within [ 1 / 2 , 2 ] n log2 n and its
// ratio drifts less than 0.05 on a doubling past 4096; the items copied by
// funr() are C( n ) = n + C( ceil( n / 2 ) ) + C( floor( n / 2 ) ), 0 up to
// 2 items
ADS_TEST( "compiti-2020-02-04-A1" )
{
 CHECK( master( 2 , 2 , 1 ) == theta( 1 , 1 ) );
 const std::size_t N = 1 << 20;
 std::vector< double > T( N + 1 , 1 ) , C( N + 1 , 0 );
 for( std::size_t n = 3 ; n <= N ; ++n ) {
  T[ n ] = 2 * T[ n / 2 ] + double( n );
  C[ n ] = double( n ) + C[ ( n + 1 ) / 2 ] + C[ n / 2 ];
  }
 auto r = [ & ]( std::size_t n ) {
  return( T[ n ] / ( double( n ) * std::log2( double( n ) ) ) );
  };
 for( std::size_t n = 16 ; n <= N ; ++n )
  CHECK( within( r( n ) , 0.5 , 2 ) );
 for( std::size_t n = 4096 ; 2 * n <= N ; n *= 2 )
  CHECK( std::abs( r( 2 * n ) - r( n ) ) < 0.05 );
 std::mt19937 g( 30 );
 for( std::size_t n : { 1 , 2 , 3 , 7 , 64 , 1000 , 4097 } ) {
  const auto A = randomVector( n , 0 , 99 , g );
  long copied = 0;
  funr( A , 0 , n - 1 , copied );
  CHECK( double( copied ) == C[ n ] );
  }
 }

/*----------------- 2021-02-08 A3, doubling and decrement ------------------*/

// the example, 3 to 7 in 4 steps; the search on [ 1 , 2 m ] against the
// rule backwards for every n , m <= 200; m <= n takes n - m steps; n = m,
// m = 1, m = n + 1, m = 2 n, 2^20 - 1, and n or m below 1
ADS_TEST( "compiti-2021-02-08-A3" )
{
 CHECK( minOpsBfs( 3 , 7 ) == 4 && minOps( 3 , 7 ) == 4 );
 for( long n = 1 ; n <= 200 ; ++n )
  for( long m = 1 ; m <= 200 ; ++m )
   CHECK( long( minOpsBfs( n , m ) ) == minOps( n , m ) );
 CHECK( minOps( 1 , 1L << 30 ) == 30 );
 CHECK( minOps( 5 , 1 ) == 4 );
 CHECK( minOps( 1 , 1000007 ) == long( minOpsBfs( 1 , 1000007 ) ) );
 // n = m, m = 1, m = n + 1 (down to 6, double, decrement), m = 2 n, a
 // power of 2 minus 1
 CHECK( minOps( 1 , 1 ) == 0 && minOps( 77 , 77 ) == 0 );
 CHECK( minOps( 1000000 , 1 ) == 999999 );
 CHECK( minOps( 10 , 11 ) == 4 + 1 + 1 && minOpsBfs( 10 , 11 ) == 6 );
 CHECK( minOps( 10 , 20 ) == 1 && minOps( 1 , 2 ) == 1 );
 CHECK( minOps( 1 , ( 1L << 20 ) - 1 ) == 21 );
 CHECK( minOps( 1 , ( 1L << 20 ) - 1 ) ==
        long( minOpsBfs( 1 , ( 1L << 20 ) - 1 ) ) );
 CHECK_THROWS( minOps( 0 , 3 ) , contract_violation );
 CHECK_THROWS( minOps( 3 , 0 ) , contract_violation );
 CHECK_THROWS( minOpsBfs( 0 , 3 ) , contract_violation );
 CHECK_THROWS( minOpsBfs( 3 , -1 ) , contract_violation );
 }

/*------------------ 2021-07-27 A1, A2, A3, B1, B2 and B3 ------------------*/

// T( n ) = 3 T( floor( n / 2 ) ) + 3 T( floor( n / 4 ) ) + n^2, T( n ) = 1
// below 4: T( n ) <= 16 n^2 (the constant of the proof) for every n up to
// 10^6, and T( n ) >= n^2 from 4 on
ADS_TEST( "compiti-2021-07-27-A1" )
{
 const std::size_t N = 1000000;
 std::vector< double > T( N + 1 , 1 );
 for( std::size_t n = 1 ; n <= N ; ++n ) {
  if( n >= 4 )
   T[ n ] = 3 * T[ n / 2 ] + 3 * T[ n / 4 ] + double( n ) * double( n );
  const double r = T[ n ] / ( double( n ) * double( n ) );
  CHECK( r <= 16 );
  if( n >= 4 )
   CHECK( r >= 1 );
  }
 }

// the example is 2-sorted and not 1-sorted; sortK() against std::sort on
// vectors shuffled within blocks of k + 1 items and by random exchanges at
// distance at most k; k = 0, k = n - 1, k >= n, empty, one item, repeated
// and equal items, the extremes of long
ADS_TEST( "compiti-2021-07-27-A2" )
{
 const std::vector< long > A = { 1 , 4 , 5 , 2 , 3 , 7 , 8 , 6 , 10 , 9 };
 CHECK( kSorted( A , 2 ) && ! kSorted( A , 1 ) );
 auto B = A;
 sortK( B , 2 );
 CHECK( B == std::vector< long >( { 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 ,
                                    10 } ) );
 std::mt19937 g( 31 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 200 , k = g() % 8;
  std::vector< long > V( n );
  std::iota( V.begin() , V.end() , -50L );
  if( t % 2 )
   for( std::size_t s = 0 ; s < n ; s += k + 1 )
    std::shuffle( V.begin() + std::ptrdiff_t( s ) ,
                  V.begin() + std::ptrdiff_t( std::min( s + k + 1 , n ) ) ,
                  g );
  else
   for( std::size_t e = 0 ; ( e < n ) && ( k > 0 ) ; ++e ) {
    const std::size_t i = g() % n ,
                      j = std::min( n - 1 , i + g() % ( k + 1 ) );
    std::swap( V[ i ] , V[ j ] );
    if( ! kSorted( V , k ) )
     std::swap( V[ i ] , V[ j ] );
    }
  CHECK( kSorted( V , k ) );
  auto S = V;
  std::sort( S.begin() , S.end() );
  auto I = V;
  insertionSort( I , 0 , n );
  sortK( V , k );
  CHECK( V == S && I == S );
  }
 std::vector< long > E , O( 1 , 4 ) , R = { 3 , 2 , 1 };
 sortK( E , 0 );
 sortK( O , 0 );
 sortK( R , 5 );
 CHECK( E.empty() && ( O[ 0 ] == 4 ) );
 CHECK( R == std::vector< long >( { 1 , 2 , 3 } ) );
 // repeated items (k-sorted by the stable order), all equal, k = n - 1,
 // the extremes of long
 const long M = std::numeric_limits< long >::max();
 std::vector< long > D = { 2 , 1 , 2 , 1 , 3 , 3 } , Q( 9 , 5 ) ,
                     X = { M , -M - 1 , 0 } , K = { 4 , 3 , 2 , 1 };
 CHECK( kSorted( D , 2 ) && ! kSorted( D , 1 ) );
 sortK( D , 2 );
 sortK( Q , 0 );
 sortK( X , 2 );
 sortK( K , 3 );
 CHECK( D == std::vector< long >( { 1 , 1 , 2 , 2 , 3 , 3 } ) );
 CHECK( Q == std::vector< long >( 9 , 5 ) );
 CHECK( X == std::vector< long >( { -M - 1 , 0 , M } ) );
 CHECK( K == std::vector< long >( { 1 , 2 , 3 , 4 } ) );
 std::vector< long > W = { 3 , 2 , 1 };
 CHECK_THROWS( sortK( W , 1 ) , contract_violation );
 }

// wellBalanced() against the sizes counted at each node, on every shape of
// up to 8 nodes and on random trees; the empty tree
ADS_TEST( "compiti-2021-07-27-A3" )
{
 auto oracle = []( TreeNode * T ) {
  std::vector< TreeNode * > N;
  nodes( T , N );
  for( auto u : N )
   if( std::abs( countNodes( u->left ) - countNodes( u->right ) ) > 1 )
    return( false );
  return( true );
  };
 long balanced = 0;
 for( std::size_t n = 0 ; n <= 8 ; ++n )
  for( const auto & s : shapes( n ) ) {
   Pool P;
   std::size_t i = 0;
   long key = 0;
   TreeNode * T = buildShape( P , s , i , n , key );
   CHECK( wellBalanced( T ) == oracle( T ) );
   balanced += wellBalanced( T );
   }
 CHECK( balanced > 9 );
 std::mt19937 g( 32 );
 for( int t = 0 ; t < 300 ; ++t ) {
  Pool P;
  TreeNode * T = randomTree( P , g() % 30 , 0 , 9 , g );
  CHECK( wellBalanced( T ) == oracle( T ) );
  }
 Pool P;
 std::vector< long > K( 100 );
 std::iota( K.begin() , K.end() , 0L );
 CHECK( wellBalanced( buildMinHeightBST( P , K ) ) );
 CHECK( wellBalanced( nullptr ) );
 }

// meetingPoint() against the distances of Floyd and Warshall, on random
// graphs with positive weights and with negative ones (no negative
// cycle); s = t, t not reachable
ADS_TEST( "compiti-2021-07-27-B1" )
{
 std::mt19937 g( 33 );
 const long INF = std::numeric_limits< long >::max() / 4;
 for( int t = 0 ; t < 600 ; ++t ) {
  const int n = 1 + int( g() % 8 );
  ListGraph< long > G( n , true );
  std::vector< std::vector< long > > d( n , std::vector< long >( n , INF ) );
  for( int u = 0 ; u < n ; ++u )
   d[ u ][ u ] = 0;
  for( auto [ u , v ] : randomEdges( n , 10 + int( g() % 40 ) , g , true ) ) {
   const long w = ( t % 2 ) ? 1 + long( g() % 9 ) : long( g() % 12 ) - 2;
   G.insertEdge( u , v , w );
   d[ u ][ v ] = std::min( d[ u ][ v ] , w );
   }
  for( int k = 0 ; k < n ; ++k )
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j )
     if( ( d[ i ][ k ] < INF ) && ( d[ k ][ j ] < INF ) )
      d[ i ][ j ] = std::min( d[ i ][ j ] , d[ i ][ k ] + d[ k ][ j ] );
  bool negative = false;
  for( int u = 0 ; u < n ; ++u )
   negative = negative || ( d[ u ][ u ] < 0 );
  if( negative )
   continue;
  const int s = int( g() % unsigned( n ) ) , r = int( g() % unsigned( n ) );
  bool any = false;
  for( int u = 0 ; u < n ; ++u )
   any = any || ( ( d[ s ][ u ] < INF ) && ( d[ s ][ u ] == d[ r ][ u ] ) );
  CHECK( meetingPoint( G , s , r ) == any );
  }
 ListGraph< long > H( 3 , true );
 H.insertEdge( 0 , 1 , 5 );
 CHECK( meetingPoint( H , 2 , 2 ) );
 CHECK( ! meetingPoint( H , 0 , 1 ) && ! meetingPoint( H , 0 , 2 ) );
 H.insertEdge( 1 , 2 , 1 );
 H.insertEdge( 0 , 2 , 1 );
 CHECK( meetingPoint( H , 0 , 1 ) );
 CHECK_THROWS( meetingPoint( H , 0 , 3 ) , contract_violation );
 }

// the 13 strings of the example for n = 5; printBits() against the strings
// of n bits without 11, F_{n+2} of them (fibonacci( n + 1 ) in the
// numbering of the library from F_0 = F_1 = 1), up to n = 20; n = 0
ADS_TEST( "compiti-2021-07-27-B2" )
{
 auto S = printBits( 5 );
 std::sort( S.begin() , S.end() );
 const std::vector< std::string > E = { "00000" , "00001" , "00010" ,
                                        "00100" , "00101" , "01000" ,
                                        "01001" , "01010" , "10000" ,
                                        "10001" , "10010" , "10100" ,
                                        "10101" };
 CHECK( S == E );
 for( std::size_t n = 0 ; n <= 20 ; ++n ) {
  auto R = printBits( n );
  std::sort( R.begin() , R.end() );
  std::vector< std::string > F;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   std::string s;
   for( std::size_t i = n ; i-- > 0 ; )
    s += char( '0' + ( m >> i & 1 ) );
   if( s.find( "11" ) == std::string::npos )
    F.push_back( s );
   }
  CHECK( R == F );
  CHECK( R.size() == fibonacci( int( n ) + 1 ) );
  }
 CHECK( printBits( 0 ) == std::vector< std::string >( 1 , "" ) );
 }

// the examples of k-contiguity; maxSumK() and maxSumKLinear() against all
// the subsets whose gaps (before, between and after the items taken) are
// at most k, on random vectors with negative items; empty, k >= n, all
// negative, k = 0
ADS_TEST( "compiti-2021-07-27-B3" )
{
 auto contiguous = []( std::size_t n , const std::vector< std::size_t > & I ,
                       std::size_t k ) {
  std::size_t prev = 0;                       // the first place not checked
  for( auto i : I ) {
   if( i - prev > k )
    return( false );
   prev = i + 1;
   }
  return( n - prev <= k );
  };
 CHECK( contiguous( 6 , { 0 , 2 , 4 } , 1 ) );
 CHECK( ! contiguous( 6 , { 0 , 2 , 4 } , 0 ) );
 CHECK( contiguous( 6 , { 0 , 3 , 5 } , 2 ) );
 CHECK( ! contiguous( 6 , { 0 , 3 , 5 } , 1 ) );
 CHECK( contiguous( 6 , { 0 , 4 } , 3 ) && ! contiguous( 6 , { 0 , 4 } , 2 ) );
 std::mt19937 g( 34 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 14 , k = g() % 5;
  const auto V = randomVector( n , -10 , 10 , g );
  long best = std::numeric_limits< long >::min();
  for( unsigned s = 0 ; s < ( 1u << n ) ; ++s ) {
   std::vector< std::size_t > I;
   long w = 0;
   for( std::size_t i = 0 ; i < n ; ++i )
    if( s >> i & 1 ) {
     I.push_back( i );
     w += V[ i ];
     }
   if( contiguous( n , I , k ) )
    best = std::max( best , w );
   }
  CHECK( maxSumK( V , k ) == best );
  CHECK( maxSumKLinear( V , k ) == best );
  }
 CHECK( maxSumK( {} , 0 ) == 0 && maxSumKLinear( {} , 3 ) == 0 );
 CHECK( maxSumK( { -1 , -2 } , 2 ) == 0 );
 CHECK( maxSumKLinear( { -1 , -2 } , 2 ) == 0 );
 CHECK( maxSumK( { -1 , -2 , -3 } , 1 ) == -2 );
 CHECK( maxSumKLinear( { -1 , -2 , -3 } , 1 ) == -2 );
 CHECK( maxSumK( { 4 , -1 , 2 } , 0 ) == 5 && maxSumKLinear( { 4 , -1 , 2 } ,
                                                             0 ) == 5 );
 }

/*---- 2022-08-24 A1 and A3, bounds of a recurrence and sum of two keys ----*/

// T( n ) = 2 T( floor( n / 2 ) ) + 2 T( floor( n / 3 ) ) + n, T( n ) = 1
// below 3, against the bounds of the solution, Omega( n^{log_3 4} ) and
// O( n^2 ), and against the exponent p of Akra and Bazzi, 2 / 2^p + 2 /
// 3^p = 1: T( n ) / n^p stays in a band from 1000 to 10^6
ADS_TEST( "compiti-2022-08-24-A1" )
{
 CHECK( master( 2 , 2 , 1 ) == theta( 1 , 1 ) );
 CHECK( std::abs( master( 4 , 3 , 1 ).p - std::log( 4.0 ) / std::log( 3.0 ) )
        < 1e-12 );
 CHECK( master( 4 , 2 , 1 ) == theta( 2 ) );
 double lo = 1 , hi = 2;
 for( int i = 0 ; i < 100 ; ++i ) {
  const double p = ( lo + hi ) / 2;
  if( 2 / std::pow( 2 , p ) + 2 / std::pow( 3 , p ) > 1 )
   lo = p;
  else
   hi = p;
  }
 const double p = lo , q = std::log( 4.0 ) / std::log( 3.0 );
 CHECK( ( p > q ) && ( p < 2 ) );
 const std::size_t N = 1000000;
 std::vector< double > T( N + 1 , 1 );
 double rmin = std::numeric_limits< double >::infinity() , rmax = 0;
 for( std::size_t n = 3 ; n <= N ; ++n ) {
  T[ n ] = 2 * T[ n / 2 ] + 2 * T[ n / 3 ] + double( n );
  CHECK( T[ n ] >= std::pow( double( n ) , q ) );
  CHECK( T[ n ] <= 2 * double( n ) * double( n ) );
  if( n >= 1000 ) {
   const double r = T[ n ] / std::pow( double( n ) , p );
   rmin = std::min( rmin , r );
   rmax = std::max( rmax , r );
   }
  }
 CHECK( rmax / rmin < 3 );
 }

// the example, k = 19 true; the text says that no pair sums to 15, but
// 7 + 8 = 15, and 14 is a sum of no pair; containsSum() against all the
// pairs of nodes, on random search trees; empty, one node, x + x = k on
// one node and on two, negative keys and sums, zero, root and leaf
ADS_TEST( "compiti-2022-08-24-A3" )
{
 Pool P;
 TreeNode * T = nullptr;
 for( long k : { 10 , 7 , 15 , 1 , 8 , 12 , 18 } )
  bstInsert( P , T , k );
 CHECK( containsSum( T , 19 ) && containsSum( T , 15 ) );
 CHECK( ! containsSum( T , 14 ) );
 std::mt19937 g( 35 );
 for( int t = 0 ; t < 500 ; ++t ) {
  Pool Q;
  TreeNode * R = nullptr;
  for( long k : randomDistinct( g() % 25 , g ) )
   bstInsert( Q , R , k - 30 );
  std::vector< long > K;
  inorder( R , K );
  const long k = long( g() % 120 ) - 60;
  bool any = false;
  for( std::size_t i = 0 ; i < K.size() ; ++i )
   for( std::size_t j = i + 1 ; j < K.size() ; ++j )
    any = any || ( K[ i ] + K[ j ] == k );
  CHECK( containsSum( R , k ) == any );
  }
 CHECK( ! containsSum( nullptr , 0 ) );
 CHECK( ! containsSum( makeNode( P , 5 ) , 10 ) );
 // x + x = k with x on two nodes, negative keys and sums, zero, the pair
 // root and deepest leaf
 TreeNode * D = nullptr;
 for( long k : { 5 , 2 , 5 } )
  bstInsert( P , D , k );
 CHECK( containsSum( D , 10 ) && containsSum( D , 7 ) );
 CHECK( ! containsSum( D , 4 ) && ! containsSum( D , 5 ) );
 TreeNode * N = nullptr;
 for( long k : { -3 , -8 , 0 , 6 , 0 } )
  bstInsert( P , N , k );
 CHECK( containsSum( N , -11 ) && containsSum( N , 0 ) );
 CHECK( containsSum( N , 3 ) && ! containsSum( N , -16 ) );
 CHECK( containsSum( T , 10 + 18 ) && containsSum( T , 1 + 8 ) );
 }

/*------------------- 2026-06-22 B3, painting the houses -------------------*/

// hateville() and hatevilleFast() against all the colorings with adjacent
// houses of different colors; no house, one house, equal costs, large
// costs
ADS_TEST( "compiti-2026-06-22-B3" )
{
 std::mt19937 g( 36 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const std::size_t n = g() % 7 , k = 2 + g() % 3;
  std::vector< std::vector< long > > cost( n );
  for( auto & r : cost )
   r = randomVector( k , 0 , 20 , g );
  long best = std::numeric_limits< long >::max();
  std::vector< std::size_t > c( n , 0 );
  for( ; ; ) {
   bool ok = true;
   long w = 0;
   for( std::size_t h = 0 ; h < n ; ++h ) {
    w += cost[ h ][ c[ h ] ];
    ok = ok && ( ( h == 0 ) || ( c[ h ] != c[ h - 1 ] ) );
    }
   if( ok )
    best = std::min( best , w );
   std::size_t h = 0;
   while( ( h < n ) && ( ++c[ h ] == k ) )
    c[ h++ ] = 0;
   if( h == n )
    break;
   }
  if( n == 0 )
   best = 0;
  CHECK( hateville( cost ) == best );
  CHECK( hatevilleFast( cost ) == best );
  }
 CHECK( hateville( {} ) == 0 && hatevilleFast( {} ) == 0 );
 CHECK( hateville( { { 4 , 2 , 9 } } ) == 2 );
 CHECK( hatevilleFast( { { 4 , 2 , 9 } } ) == 2 );
 const std::vector< std::vector< long > > E( 10 ,
                                             std::vector< long >( 2 , 3 ) );
 CHECK( hateville( E ) == 30 && hatevilleFast( E ) == 30 );
 const long M = std::numeric_limits< long >::max() / 64;
 const std::vector< std::vector< long > > L = { { 1 , M } , { 1 , M } ,
                                                { 1 , M } };
 CHECK( hateville( L ) == M + 2 && hatevilleFast( L ) == M + 2 );
 CHECK_THROWS( hateville( { { 1 } } ) , contract_violation );
 CHECK_THROWS( hatevilleFast( { { 1 , 2 } , { 3 } } ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*---------------------------------- MAIN ----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- End File exam_compiti.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
