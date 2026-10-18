/*--------------------------------------------------------------------------*/
/*---------------------------- File interop.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the standard interface of the containers: the iterators model
 * the concepts of C++20 that their structure allows (contiguous for the
 * sequences on a vector, bidirectional for the doubly linked lists, forward
 * for the singly linked ones), the ranges are common, so that the
 * algorithms of C++17 and of Boost take them, and the algorithms of std,
 * of std::ranges and, when it is installed, of Boost run on them and agree
 * with the operations of the book.
 *
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, chapter 33 (the iterators)
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/DirectTable.h"
#include "ADS/dictionary/KnuthHashing.h"
#include "ADS/dictionary/OpenHashTable.h"
#include "ADS/dictionary/PointerTable.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/SortedArrayDictionary.h"
#include "ADS/dictionary/Trie.h"
#include "ADS/graph/ArrayGraph.h"
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/MatrixGraph.h"
#include "ADS/memory/Arena.h"
#include "ADS/mfset/ListMFSet.h"
#include "ADS/numeric/SparseMatrix.h"
#include "ADS/priority/BucketQueue.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"
#include "ADS/priority/HeapPriorityQueue.h"
#include "ADS/priority/LinkedHeapPriorityQueue.h"
#include "ADS/priority/ListPriorityQueue.h"
#include "ADS/priority/TreePriorityQueue.h"
#include "ADS/queue/ArrayQueue.h"
#include "ADS/queue/Dequeue.h"
#include "ADS/queue/LinkedQueue.h"
#include "ADS/queue/UniqueQueue.h"
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/SequenceRange.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/DictionarySet.h"
#include "ADS/set/HashMultiset.h"
#include "ADS/set/LinkedHashSet.h"
#include "ADS/set/ListSet.h"
#include "ADS/set/SortedListSet.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/MultiStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/ArrayBinaryTree.h"
#include "ADS/tree/BPlusTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/BinaryTree.h"
#include "ADS/tree/CursorBinaryTree.h"
#include "ADS/tree/CursorTree.h"
#include "ADS/tree/PreorderArrayTree.h"
#include "ADS/tree/SearchTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/ThreadedTree.h"
#include "ADS/tree/Treap.h"
#include "ADS/tree/Tree.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <ranges>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#if __has_include( <boost/range/algorithm/count.hpp> )
#include <boost/range/algorithm/count.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/sort/pdqsort/pdqsort.hpp>
#define ADS_HAS_BOOST 1
#else
#define ADS_HAS_BOOST 0
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*--------------------------------- USING ----------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

// the category of each iterator, and the ranges common, checked at compile
// time

static_assert( std::ranges::contiguous_range< ArraySequence< int > > );
static_assert( std::ranges::contiguous_range< const ArraySequence< int > > );
static_assert( std::ranges::common_range< ArraySequence< int > > );
static_assert( std::ranges::bidirectional_range< List< int > > );
static_assert( ! std::ranges::random_access_range< List< int > > );
static_assert( std::ranges::common_range< List< int > > );
static_assert( std::ranges::bidirectional_range< const List< int > > );
static_assert( std::ranges::forward_range< SinglyList< int > > );
static_assert( std::ranges::common_range< SinglyList< int > > );
static_assert( std::ranges::forward_range< CursorList< int > > );
static_assert( std::ranges::common_range< const CursorList< int > > );
static_assert( std::ranges::bidirectional_range< Dequeue< int > > );
static_assert(
 std::ranges::forward_range< SequenceRange< SinglyList< int > > > );
static_assert( std::ranges::common_range< SequenceRange< List< int > > > );
static_assert( std::ranges::view< SequenceRange< List< int > > > );
static_assert( std::ranges::borrowed_range< SequenceRange< List< int > > > );
static_assert( std::ranges::contiguous_range< const ArrayStack< int > > );
static_assert( std::ranges::forward_range< SequenceStack< List< int > > > );
static_assert( std::ranges::random_access_range< ArrayQueue< int > > );
static_assert( ! std::ranges::contiguous_range< ArrayQueue< int > > );
static_assert( std::ranges::forward_range< LinkedQueue< int > > );
static_assert( std::ranges::bidirectional_range< UniqueQueue< int > > );
static_assert( std::ranges::contiguous_range< DaryHeap< int > > );
static_assert( std::ranges::random_access_range<
                HeapPriorityQueue< int , int > > );
static_assert( std::ranges::bidirectional_range<
                ListPriorityQueue< int , int > > );
static_assert( std::ranges::forward_range< BucketQueue< int > > );
static_assert( std::ranges::forward_range< decltype(
                std::declval< const MultiStack< int > & >().stack( 0 ) ) > );
static_assert( ! std::output_iterator< ArrayQueue< int >::iterator , int > );
static_assert( std::ranges::forward_range< ChainedHashTable< int , int > > );
static_assert(
 std::ranges::bidirectional_range< OpenHashTable< int , int > > );
static_assert(
 std::ranges::bidirectional_range< BrentHashTable< int , int > > );
static_assert( std::ranges::bidirectional_range<
                CoalescedHashTable< int , int > > );
static_assert( std::ranges::bidirectional_range< DirectTable< int > > );
static_assert( std::ranges::forward_range< SkipList< int , int > > );
static_assert( std::ranges::contiguous_range<
                SortedArrayDictionary< int , int > > );
static_assert( std::ranges::forward_range< Trie< int > > );
static_assert( std::ranges::forward_range< RadixTree< int > > );
static_assert( std::ranges::bidirectional_range< BoolSet > );
static_assert( std::ranges::bidirectional_range< ListSet< int > > );
static_assert( std::ranges::bidirectional_range< SortedListSet< int > > );
static_assert( std::ranges::bidirectional_range< LinkedHashSet< int > > );
static_assert( std::ranges::forward_range< HashMultiset< int > > );
static_assert( std::ranges::forward_range< ChainedHashSet< int > > );
static_assert( std::ranges::bidirectional_range< OpenHashSet< int > > );
static_assert( std::ranges::common_range< Trie< int > > );
static_assert( std::ranges::bidirectional_range< RedBlackTree< int , int > > );
static_assert( std::ranges::bidirectional_range< AVLTree< int , int > > );
static_assert( std::ranges::bidirectional_range< Treap< int , int > > );
static_assert( std::ranges::bidirectional_range< SplayTree< int , int > > );
static_assert( std::ranges::bidirectional_range< ThreadedTree< int > > );
static_assert( std::ranges::forward_range< BTree< int , int > > );
static_assert( std::ranges::bidirectional_range< BPlusTree< int , int > > );
static_assert( std::ranges::bidirectional_range< TreeSet< int > > );
static_assert( std::ranges::bidirectional_range<
                LinkedHeapPriorityQueue< int , int > > );
static_assert( std::ranges::forward_range<
                FibonacciHeapPriorityQueue< int , int > > );
static_assert( std::ranges::bidirectional_range<
                TreePriorityQueue< int , int > > );
static_assert( std::ranges::random_access_range< Arena< int > > );
static_assert( std::ranges::random_access_range<
                decltype( std::declval< const PreorderArrayTree< int > & >()
                           .values() ) > );
static_assert( std::ranges::contiguous_range<
                decltype( std::declval< const ArrayGraph<> & >()
                           .neighbors( 0 ) ) > );
static_assert( std::output_iterator< List< int >::iterator , int > );
static_assert( std::convertible_to< List< int >::iterator ,
                                    List< int >::const_iterator > );
static_assert( ! std::convertible_to< List< int >::const_iterator ,
                                      List< int >::iterator > );

/*--------------------------------------------------------------------------*/
/*--------------------------------- TESTS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the items of s in order by the positions of the book
template< class S >
std::vector< int > byPositions( const S & s )
{
 std::vector< int > out;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  out.push_back( s.read( p ) );
 return( out );
 }

// the sequences: built from a pair of iterators and by the inserters, then
// sorted, reversed, searched and accumulated by std, std::ranges and Boost,
// each time against the positions of the book and against std::vector;
// the empty sequences and those of one item, visited both ways
ADS_TEST( "sequences" )
{
 std::mt19937 g( 22 );
 std::vector< int > R( 200 );
 for( auto & x : R ) x = int( g() % 1000 );
 ArraySequence< int > A( R.begin() , R.end() );
 CHECK( byPositions( A ) == R && std::ranges::equal( A , R ) );
 std::sort( A.begin() , A.end() );
 auto S = R;
 std::sort( S.begin() , S.end() );
 CHECK( byPositions( A ) == S );
 std::ranges::sort( A , std::greater<>() );
 CHECK( std::ranges::equal( A , S | std::views::reverse ) );
 CHECK( std::accumulate( A.cbegin() , A.cend() , 0L ) ==
        std::accumulate( R.begin() , R.end() , 0L ) );
 A[ 0 ] = -1;
 CHECK( A.read( 0 ) == -1 && *A.data() == -1 );
 ArraySequence< int > B;
 std::ranges::copy( R , std::back_inserter( B ) );
 CHECK( std::ranges::equal( B , R ) && B.size() == long( R.size() ) );
 CHECK( std::equal( B.rbegin() , B.rend() , R.rbegin() ) );
#if ADS_HAS_BOOST
 boost::sort::pdqsort( B.begin() , B.end() );
 CHECK( byPositions( B ) == S );
 boost::range::reverse( B );
 CHECK( std::ranges::equal( B , S | std::views::reverse ) );
#endif

 List< int > L( R.begin() , R.end() );
 CHECK( byPositions( L ) == R );
 std::reverse( L.begin() , L.end() );   // bidirectional, in place
 CHECK( std::ranges::equal( L , R | std::views::reverse ) );
 std::ranges::reverse( L );
 CHECK( byPositions( L ) == R );
 for( auto & x : L ) x *= 2;
 CHECK( std::ranges::equal( L , R | std::views::transform( []( int x ) {
  return( 2 * x ); } ) ) );
 const auto it = std::ranges::find( L , 2 * R[ 17 ] );
 CHECK( it != L.end() && *it == 2 * R[ 17 ] );
 CHECK( L.read( it.cell() ) == *it );   // the iterator is a position
 List< int > F;
 std::ranges::copy( R , std::front_inserter( F ) );
 CHECK( std::equal( F.rbegin() , F.rend() , R.begin() , R.end() ) );
 std::list< int > SL( L.begin() , L.end() );   // into a container of std
 CHECK( std::ranges::equal( SL , L ) );
#if ADS_HAS_BOOST
 CHECK( boost::range::count( L , 2 * R[ 0 ] ) ==
        std::ranges::count( R , R[ 0 ] ) );
#endif

 SinglyList< int > W( R.begin() , R.end() );
 CHECK( byPositions( W ) == R && std::ranges::equal( W , R ) );
 for( auto & x : W ) ++x;
 CHECK( std::ranges::equal( W , R | std::views::transform( []( int x ) {
  return( x + 1 ); } ) ) );
 CHECK( std::ranges::distance( W ) == long( R.size() ) );
 SinglyList< int > E;
 CHECK( E.begin() == E.end() && std::ranges::empty( E ) );

 CursorList< int > C( R.begin() , R.end() );
 CHECK( byPositions( C ) == R && std::ranges::equal( C , R ) );
 std::ranges::fill( C , 7 );
 CHECK( std::ranges::all_of( C , []( int x ) { return( x == 7 ); } ) );
 CursorList< int > D( std::make_shared< CursorSpace< int > >() );
 std::ranges::copy( R , std::front_inserter( D ) );   // a shared space
 CHECK( std::ranges::equal( D , R | std::views::reverse ) );

 Dequeue< int > Q;
 std::ranges::copy( R , std::back_inserter( Q ) );
 CHECK( std::ranges::equal( Q , R ) && Q.head() == R.front() &&
        Q.tail() == R.back() );
 std::ranges::replace( Q , R.front() , -5 );
 CHECK( Q.head() == -5 );

 const auto V = range( W );                // the view of the book
 CHECK( std::ranges::equal( V , W ) );
 CHECK( std::count( V.begin() , V.end() , *W.begin() ) >= 1 );
 auto firstTwo = V | std::views::take( 2 );
 CHECK( std::ranges::distance( firstTwo ) == 2 );

 ArraySequence< int > A0;                  // the empty sequences
 std::sort( A0.begin() , A0.end() );
 CHECK( std::ranges::empty( A0 ) && A0.rbegin() == A0.rend() );
 List< int > L0;
 std::ranges::reverse( L0 );
 CHECK( L0.begin() == L0.end() && L0.rbegin() == L0.rend() );
 CHECK( std::ranges::find( L0 , 0 ) == L0.end() );
 CursorList< int > C0;
 Dequeue< int > Q0;
 CHECK( std::ranges::empty( C0 ) && std::ranges::empty( Q0 ) );
 CHECK( std::ranges::empty( range( E ) ) );
 List< int > L1;                           // one item, both ways
 L1.insert( L1.head() , 9 );
 std::ranges::reverse( L1 );
 CHECK( std::ranges::equal( L1 , std::vector< int >( { 9 } ) ) );
 CHECK( *L1.rbegin() == 9 && std::next( L1.rbegin() ) == L1.rend() );
 SinglyList< int > W1( R.begin() , R.begin() + 1 );
 CHECK( std::ranges::distance( W1 ) == 1 && *W1.begin() == R[ 0 ] );
 }

// the restricted containers as constant views: each one visited in its
// order (bottom to top, front to back, the order of the heap) against what
// its own operations give, and the algorithms of std on the views; the
// empty views, a stack index out of range, the bucket queue after pops
// that leave its lazy bounds behind, a circular queue full and wrapped
ADS_TEST( "restricted" )
{
 std::mt19937 g( 23 );
 std::vector< int > R( 100 );
 for( auto & x : R ) x = int( g() % 50 );

 ArrayStack< int > S( 200 );
 for( int x : R ) S.push( x );
 CHECK( std::ranges::equal( S , R ) );       // bottom to top
 CHECK( std::ranges::max( S ) == std::ranges::max( R ) );
 SequenceStack< List< int > > T;
 for( int x : R ) T.push( x );
 CHECK( std::ranges::equal( T , R | std::views::reverse ) );   // top first

 MultiStack< int > M( 3 );
 for( std::size_t i = 0 ; i < R.size() ; ++i ) M.push( int( i % 3 ) , R[ i ] );
 for( int i = 0 ; i < 3 ; ++i ) {
  std::vector< int > top;                   // the items of stack i, top
  for( std::size_t k = R.size() ; k-- > 0 ; )   // first
   if( int( k % 3 ) == i ) top.push_back( R[ k ] );
  CHECK( std::ranges::equal( M.stack( i ) , top ) );
  }

 ArrayQueue< int > Q( 64 );                 // wrapped around
 for( int k = 0 ; k < 50 ; ++k ) Q.enqueue( k );
 for( int k = 0 ; k < 40 ; ++k ) (void) Q.dequeue();
 for( int k = 50 ; k < 90 ; ++k ) Q.enqueue( k );
 CHECK( std::ranges::equal( Q , std::views::iota( 40 , 90 ) ) );
 CHECK( Q.begin()[ 10 ] == 50 && Q.end() - Q.begin() == 50 );
 CHECK( std::ranges::binary_search( Q , 77 ) );   // random access
 CHECK( *std::ranges::lower_bound( Q , 64 ) == 64 );
 LinkedQueue< int > L;
 for( int x : R ) L.enqueue( x );
 CHECK( std::ranges::equal( L , R ) );
 UniqueQueue< int > U;
 for( int x : R ) U.enqueue( x );
 std::vector< int > firsts;                 // the first occurrences
 for( int x : R ) if( std::ranges::find( firsts , x ) == firsts.end() )
  firsts.push_back( x );
 CHECK( std::ranges::equal( U , firsts ) );

 DaryHeap< int > H;
 for( int x : R ) H.push( x );
 CHECK( *H.begin() == std::ranges::min( R ) );
 CHECK( std::ranges::is_permutation( H , R ) );
 HeapPriorityQueue< int , int > P( 200 );
 for( std::size_t i = 0 ; i < R.size() ; ++i ) P.insert( int( i ) , R[ i ] );
 CHECK( P.begin()->priority == std::ranges::min( R ) );
 std::vector< int > pr;
 for( const auto & it : P ) pr.push_back( it.priority );
 CHECK( std::ranges::is_permutation( pr , R ) );
 ListPriorityQueue< int , int > O;
 for( std::size_t i = 0 ; i < R.size() ; ++i ) O.insert( int( i ) , R[ i ] );
 CHECK( std::ranges::is_sorted( O | std::views::keys ) );
 BucketQueue< int > B;
 for( std::size_t i = 0 ; i < R.size() ; ++i )
  B.push( int( i ) , R[ i ] % 7 , R[ i ] % 3 );
 std::vector< int > seen( B.begin() , B.end() ) , popped;
 for( auto C = B ; ! C.isEmpty() ; C.pop() ) popped.push_back( C.top() );
 CHECK( seen == popped );                   // the order of the extractions
 std::ranges::sort( seen );
 CHECK( std::ranges::equal( seen , std::views::iota( 0 , int( R.size() ) ) ) );
 BucketQueue< int > E;
 CHECK( E.begin() == E.end() );
 for( int k = 0 ; k < 30 ; ++k ) B.pop();   // stale lazy bounds
 std::vector< int > rest( B.begin() , B.end() );
 popped.clear();
 for( auto C = B ; ! C.isEmpty() ; C.pop() ) popped.push_back( C.top() );
 CHECK( rest == popped && rest.size() == R.size() - 30 );
 while( ! B.isEmpty() ) B.pop();
 CHECK( B.begin() == B.end() );

 ArrayStack< int > S0( 1 );                 // the empty views
 SequenceStack< List< int > > T0;
 LinkedQueue< int > L0;
 UniqueQueue< int > U0;
 DaryHeap< int > H0;
 HeapPriorityQueue< int , int > P0( 4 );
 ListPriorityQueue< int , int > O0;
 CHECK( std::ranges::empty( S0 ) && std::ranges::empty( T0 ) );
 CHECK( std::ranges::empty( L0 ) && std::ranges::empty( U0 ) );
 CHECK( std::ranges::empty( H0 ) && P0.begin() == P0.end() &&
        O0.begin() == O0.end() );
 MultiStack< int > M0( 2 );
 CHECK( std::ranges::empty( M0.stack( 1 ) ) );
 CHECK_THROWS( (void) M0.stack( 2 ) , contract_violation );
 CHECK_THROWS( (void) M0.stack( -1 ) , contract_violation );
 ArrayQueue< int > F( 4 );                  // full, wrapped, then empty
 for( int k = 0 ; k < 4 ; ++k ) F.enqueue( k );
 (void) F.dequeue();
 (void) F.dequeue();
 F.enqueue( 4 );
 F.enqueue( 5 );
 CHECK( std::ranges::equal( F , std::views::iota( 2 , 6 ) ) );
 CHECK( F.end() - F.begin() == 4 && F.begin()[ 3 ] == 5 );
 while( ! F.isEmpty() ) (void) F.dequeue();
 CHECK( F.begin() == F.end() );
 for( int k = 0 ; k < 5 ; ++k ) U0.enqueue( 3 );   // all equal
 CHECK( std::ranges::equal( U0 , std::vector< int >( { 3 } ) ) );
 }

// the dictionaries and the sets as constant views: each one gives the
// pairs or the items that std::map or std::set has, in its own order (the
// order of the keys where it keeps one), and the algorithms of std run on
// them; the empty views, an open table of tombstones only, the keys at the
// bounds of a direct table, a trie of the empty word alone, a set of bits
// with its first and last member, then full, then emptied
ADS_TEST( "dictionaries" )
{
 std::mt19937 g( 24 );
 std::map< int , int > ref;
 ChainedHashTable< int , int > C( 37 );
 OpenHashTable< int , int > O( 211 , Probing::linear() );
 BrentHashTable< int , int > B( 211 );
 CoalescedHashTable< int , int > K( 180 , 31 );
 DirectTable< int > D( -50 , 149 );
 SkipList< int , int > S;
 SortedArrayDictionary< int , int > A;
 for( int t = 0 ; t < 120 ; ++t ) {
  const int k = int( g() % 200 ) - 50 , v = int( g() % 1000 );
  if( ref.count( k ) ) continue;            // first come, first kept
  ref[ k ] = v;
  C.insert( k , v );
  O.insert( k , v );
  B.insert( k , v );
  K.insert( k , v );
  D.insert( k , v );
  S.insert( k , v );
  A.insert( k , v );
  }
 auto sorted = []( const auto & T ) {       // the pairs of T, by key
  std::vector< std::pair< int , int > > out;
  for( const auto & [ k , v ] : T ) out.emplace_back( k , v );
  std::ranges::sort( out );
  return( out );
  };
 const std::vector< std::pair< int , int > > R( ref.begin() , ref.end() );
 CHECK( sorted( C ) == R && sorted( O ) == R && sorted( B ) == R );
 CHECK( sorted( K ) == R && sorted( D ) == R );
 CHECK( std::ranges::equal( S , R , []( const auto & a , const auto & b ) {
  return( ( a.first == b.first ) && ( a.second == b.second ) ); } ) );
 CHECK( std::ranges::equal( A , R ) );       // contiguous, in order
 CHECK( std::ranges::distance( C ) == long( R.size() ) &&
        std::ranges::distance( O ) == long( R.size() ) );
 const auto big = std::ranges::count_if( O , []( const auto & p ) {
  return( p.second >= 500 ); } );
 CHECK( big == std::ranges::count_if( R , []( const auto & p ) {
  return( p.second >= 500 ); } ) );
 O.remove( R[ 0 ].first );                  // a tombstone is skipped
 CHECK( std::ranges::distance( O ) == long( R.size() ) - 1 );
 auto last = O.end();
 --last;                                    // bidirectional
 std::pair< int , int > seen;
 for( const auto & [ k , v ] : O ) seen = { k , v };
 CHECK( ( *last ).first == seen.first && ( *last ).second == seen.second );

 Trie< int > T;
 RadixTree< int > X;
 std::map< std::string , int > words;
 for( const char * w : { "tea" , "ten" , "to" , "inn" , "in" , "i" , "" ,
                         "team" , "tenet" } ) {
  words[ w ] = int( std::string( w ).size() );
  T.insert( w , words[ w ] );
  X.insert( w , words[ w ] );
  }
 std::vector< std::pair< std::string , int > > TW , XW;
 for( const auto & [ k , v ] : T ) TW.emplace_back( k , v );
 for( const auto & [ k , v ] : X ) XW.emplace_back( k , v );
 const std::vector< std::pair< std::string , int > > W( words.begin() ,
                                                        words.end() );
 CHECK( TW == W && XW == W );               // lexicographic
 Trie< int > E;
 CHECK( E.begin() == E.end() );

 BoolSet Z( 64 );
 std::set< long > zr;
 for( int t = 0 ; t < 30 ; ++t ) {
  const long x = long( g() % 64 );
  Z.insert( x );
  zr.insert( x );
  }
 CHECK( std::ranges::equal( Z , zr ) );
 CHECK( *std::ranges::prev( Z.end() ) == *zr.rbegin() );   // C++20 only
 ListSet< int > LS;
 SortedListSet< int > SS;
 LinkedHashSet< int > LH;
 HashMultiset< int > HM;
 ChainedHashSet< int > CS( 31 );
 std::vector< int > order;
 std::map< int , std::size_t > counts;
 for( int t = 0 ; t < 60 ; ++t ) {
  const int x = int( g() % 25 );
  LS.insert( x );
  SS.insert( x );
  LH.insert( x );
  HM.insert( x );
  CS.insert( x );
  if( std::ranges::find( order , x ) == order.end() ) order.push_back( x );
  ++counts[ x ];
  }
 std::set< int > sr( order.begin() , order.end() );
 CHECK( std::ranges::is_permutation( LS , order ) );
 CHECK( std::ranges::equal( SS , sr ) );     // increasing
 CHECK( std::ranges::equal( LH , order ) );  // the order of insertion
 CHECK( std::ranges::is_permutation( CS , order ) );
 std::map< int , std::size_t > hc;
 for( const auto & [ x , c ] : HM ) hc[ x ] = c;
 CHECK( hc == counts );

 ChainedHashTable< int , int > C0( 7 );     // the empty views
 OpenHashTable< int , int > O0( 11 , Probing::linear() );
 SkipList< int , int > S0;
 SortedArrayDictionary< int , int > A0;
 RadixTree< int > X0;
 BoolSet Z0( 0 );
 HashMultiset< int > M0;
 CHECK( C0.begin() == C0.end() && O0.begin() == O0.end() );
 CHECK( S0.begin() == S0.end() && A0.begin() == A0.end() );
 CHECK( X0.begin() == X0.end() && Z0.begin() == Z0.end() );
 CHECK( M0.begin() == M0.end() );
 for( const auto & [ k , v ] : R ) O.remove( k );   // all tombstones
 CHECK( O.begin() == O.end() && std::ranges::distance( O ) == 0 );
 DirectTable< int > D1( 3 , 3 );            // one key, both bounds
 D1.insert( 3 , 8 );
 CHECK( std::ranges::distance( D1 ) == 1 && ( *D1.begin() ).second == 8 );
 D.insert( -50 , 1 );                       // the first and last keys
 D.insert( 149 , 2 );
 CHECK_THROWS( D.insert( 150 , 0 ) , contract_violation );
 CHECK_THROWS( D.insert( -51 , 0 ) , contract_violation );
 CHECK( ( *D.begin() ).first == -50 &&                 // std::prev would
        ( *std::ranges::prev( D.end() ) ).first == 149 );   // be forward
 Trie< int > T1;                            // only the empty word
 T1.insert( "" , 5 );
 CHECK( std::ranges::distance( T1 ) == 1 && ( *T1.begin() ).first == "" );
 BoolSet Z1( 64 );                          // the extremes, then all
 Z1.insert( 0 );
 Z1.insert( 63 );
 CHECK( std::ranges::equal( Z1 , std::vector< long >( { 0 , 63 } ) ) );
 for( long x = 0 ; x < 64 ; ++x ) Z1.insert( x );
 CHECK( std::ranges::equal( Z1 , std::views::iota( 0L , 64L ) ) );
 for( long x = 0 ; x < 64 ; ++x ) Z1.remove( x );
 CHECK( Z1.begin() == Z1.end() );
 }

/// the pairs of a dictionary T as a vector, in the order of its view
template< class T >
std::vector< std::pair< int , int > > pairs( const T & D )
{
 std::vector< std::pair< int , int > > out;
 for( const auto & [ k , v ] : D ) out.emplace_back( k , v );
 return( out );
 }

// the trees: each one gives the pairs of std::map in increasing order of
// the keys, forward and (where bidirectional) backward, also after
// removals that rebalance it; the heaps give their items, each once; the
// empty trees, one key both ways, all keys removed, and the unbalanced
// search tree as a chain of 2000 keys inserted in order
ADS_TEST( "trees" )
{
 std::mt19937 g( 25 );
 std::map< int , int > ref;
 RedBlackTree< int , int > RB;
 BinarySearchTree< int , int > BS;
 AVLTree< int , int > AV;
 Treap< int , int > TR;
 SplayTree< int , int > SP;
 BTree< int , int > BT( 2 );
 BPlusTree< int , int > BP( 4 );
 ThreadedTree< int > TT;
 for( int t = 0 ; t < 300 ; ++t ) {
  const int k = int( g() % 500 ) , v = int( g() % 1000 );
  if( ref.count( k ) ) continue;
  ref[ k ] = v;
  RB.insert( k , v );
  BS.insert( k , v );
  AV.insert( k , v );
  TR.insert( k , v );
  SP.insert( k , v );
  BT.insert( k , v );
  BP.insert( k , v );
  TT.insert( k );
  }
 std::vector< std::pair< int , int > > R( ref.begin() , ref.end() );
 CHECK( pairs( RB ) == R && pairs( BS ) == R && pairs( AV ) == R );
 CHECK( pairs( TR ) == R && pairs( SP ) == R && pairs( BT ) == R );
 CHECK( pairs( BP ) == R );
 CHECK( std::ranges::equal( TT , ref | std::views::keys ) );
 auto backward = []( const auto & D ) {     // by --, from the end
  std::vector< std::pair< int , int > > out;
  for( auto it = D.end() ; it != D.begin() ; ) {
   --it;
   out.emplace_back( ( *it ).first , ( *it ).second );
   }
  return( out );
  };
 const std::vector< std::pair< int , int > > Rr( R.rbegin() , R.rend() );
 CHECK( backward( RB ) == Rr && backward( AV ) == Rr );
 CHECK( backward( TR ) == Rr && backward( SP ) == Rr );
 CHECK( backward( BP ) == Rr );
 CHECK( std::ranges::equal( TT | std::views::reverse ,
                            ref | std::views::keys | std::views::reverse ) );
 for( int t = 0 ; t < 100 ; ++t ) {         // removals that rebalance
  const int k = R[ g() % R.size() ].first;
  if( ! ref.erase( k ) ) continue;
  RB.remove( k );
  AV.remove( k );
  TR.remove( k );
  SP.remove( k );
  BT.remove( k );
  }
 R.assign( ref.begin() , ref.end() );
 CHECK( pairs( RB ) == R && pairs( AV ) == R && pairs( TR ) == R );
 CHECK( pairs( SP ) == R && pairs( BT ) == R );
 RedBlackTree< int , int > E;
 BTree< int , int > EB;
 CHECK( E.begin() == E.end() && EB.begin() == EB.end() );
 AVLTree< int , int > EA;                   // the other empty trees
 Treap< int , int > ET;
 SplayTree< int , int > ES;
 BPlusTree< int , int > EP( 4 );
 ThreadedTree< int > EH;
 BinarySearchTree< int , int > EZ;
 CHECK( EA.begin() == EA.end() && ET.begin() == ET.end() );
 CHECK( ES.begin() == ES.end() && EP.begin() == EP.end() );
 CHECK( EH.begin() == EH.end() && EZ.begin() == EZ.end() );
 const std::vector< std::pair< int , int > > one = { { 4 , 40 } };
 E.insert( 4 , 40 );                        // one key, both ways
 EA.insert( 4 , 40 );
 EP.insert( 4 , 40 );
 CHECK( pairs( E ) == one && backward( E ) == one );
 CHECK( backward( EA ) == one && backward( EP ) == one );
 EA.remove( 4 );
 CHECK( EA.begin() == EA.end() );
 for( const auto & [ k , v ] : R ) {        // all removed
  RB.remove( k );
  BT.remove( k );
  }
 CHECK( RB.begin() == RB.end() && BT.begin() == BT.end() );
 std::vector< std::pair< int , int > > chain;   // a sorted chain
 for( int k = 0 ; k < 2000 ; ++k ) {
  EZ.insert( k , k );
  chain.emplace_back( k , k );
  }
 CHECK( pairs( EZ ) == chain );

 TreeSet< int > S;                          // a set on the tree
 for( const auto & [ k , v ] : R ) S.insert( k );
 CHECK( std::ranges::equal( S , ref | std::views::keys ) );

 LinkedHeapPriorityQueue< int , int > LH;
 FibonacciHeapPriorityQueue< int , int > FH;
 TreePriorityQueue< int , int > TP;
 std::multiset< int > pr;
 for( int t = 0 ; t < 200 ; ++t ) {
  const int p = int( g() % 100 );
  LH.insert( t , p );
  FH.insert( t , p );
  TP.insert( t , p );
  pr.insert( p );
  if( t % 7 == 6 ) {                        // the Fibonacci heap gets
   FH.deleteMin();                          // children by consolidation
   LH.deleteMin();
   TP.deleteMin();
   pr.erase( pr.begin() );
   }
  }
 auto priorities = []( const auto & H ) {
  std::multiset< int > out;
  for( const auto & [ p , x ] : H ) out.insert( p );
  return( out );
  };
 CHECK( priorities( LH ) == pr && priorities( FH ) == pr );
 auto FM = std::move( FH );                 // the move leaves FH empty
 CHECK( priorities( FM ) == pr && FH.isEmpty() && FH.begin() == FH.end() );
 std::multiset< int > tp;
 for( const auto & [ key , x ] : TP ) tp.insert( key.first );
 CHECK( tp == pr );
 CHECK( std::ranges::is_sorted( TP | std::views::transform(
         []( const auto & e ) { return( e.first.first ); } ) ) );
 LinkedHeapPriorityQueue< int , int > L0;   // the empty heaps, and one
 TreePriorityQueue< int , int > T0;
 CHECK( L0.begin() == L0.end() && T0.begin() == T0.end() );
 L0.insert( 1 , 5 );
 FH.insert( 1 , 5 );
 CHECK( priorities( L0 ) == std::multiset< int >( { 5 } ) &&
        priorities( FH ) == std::multiset< int >( { 5 } ) );
 L0.deleteMin();
 FH.deleteMin();
 CHECK( L0.begin() == L0.end() && FH.begin() == FH.end() );
 }

// the trees of nodes, the graphs, the sets of an MFSet, the sparse matrix
// and the arena: each range against the visit or the vector of the book
// that it replaces; the degenerate cases: a tree of one node, graphs with
// no edges, a sparse matrix of zeros and its indices out of range, an
// arena empty and then with its last block exactly full
ADS_TEST( "nodes-and-graphs" )
{
 std::mt19937 g( 26 );
 std::unique_ptr< Tree< int > > T( new Tree< int >( 0 ) );
 std::vector< Tree< int > * > all = { T.get() };
 for( int k = 1 ; k < 40 ; ++k ) {          // a random tree
  auto * u = new Tree< int >( k );
  all[ g() % all.size() ]->insertChild( u );
  all.push_back( u );
  }
 std::vector< Tree< int > * > pre;
 pre_visit( T.get() , [ & ]( Tree< int > * u ) { pre.push_back( u ); } );
 CHECK( std::ranges::equal( preorder( T.get() ) , pre ) );
 for( auto * u : all ) {
  std::vector< Tree< int > * > kids;
  for( auto * c = u->leftmostChild() ; c ; c = c->rightSibling() )
   kids.push_back( c );
  CHECK( std::ranges::equal( children( u ) , kids ) );
  std::vector< Tree< int > * > sub;         // a subtree stops at its root
  pre_visit( u , [ & ]( Tree< int > * w ) { sub.push_back( w ); } );
  CHECK( std::ranges::equal( preorder( u ) , sub ) );
  }

 std::unique_ptr< BinaryTree< int > > Bt( new BinaryTree< int >( 0 ) );
 std::vector< BinaryTree< int > * > bn = { Bt.get() };
 for( int k = 1 ; k < 40 ; ++k ) {
  BinaryTree< int > * u = bn[ g() % bn.size() ];
  auto * w = new BinaryTree< int >( k );
  if( ! u->left() && ( g() % 2 || u->right() ) ) u->insertLeft( w );
  else if( ! u->right() ) u->insertRight( w );
  else { delete w; continue; }
  bn.push_back( w );
  }
 std::vector< BinaryTree< int > * > bpre , bin;
 pre_visit( Bt.get() , [ & ]( BinaryTree< int > * u ) {
  bpre.push_back( u ); } );
 in_visit( Bt.get() , [ & ]( BinaryTree< int > * u ) { bin.push_back( u ); } );
 CHECK( std::ranges::equal( preorder( Bt.get() ) , bpre ) );
 CHECK( std::ranges::equal( inorder( Bt.get() ) , bin ) );

 CursorTree< int > C( 0 );
 std::vector< int > cn = { C.root() };
 for( int k = 1 ; k < 30 ; ++k )
  cn.push_back( C.insertChild( cn[ g() % cn.size() ] , k ) );
 for( int u : cn ) {
  std::vector< int > kids;
  for( int c = C.leftmostChild( u ) ; c != C.NIL ; c = C.rightSibling( c ) )
   kids.push_back( c );
  CHECK( std::ranges::equal( C.children( u ) , kids ) );
  }
 std::vector< int > used( C.nodes().begin() , C.nodes().end() );
 std::ranges::sort( used );                 // is_permutation of libstdc++
 std::ranges::sort( cn );                   // dangles on values
 CHECK( used == cn );
 PreorderArrayTree< int > P( 0 );
 P.insertChild( 0 , PreorderArrayTree< int >( 1 ) );
 P.insertChild( 0 , PreorderArrayTree< int >( 2 ) );
 std::vector< int > pv;
 for( int i = 0 ; i < P.size() ; ++i ) pv.push_back( P.read( i ) );
 CHECK( std::ranges::equal( P.values() , pv ) && pv.size() == 3 );

 ListGraph<> L( 12 , true );
 MatrixGraph<> M( 12 , true );
 std::vector< std::tuple< int , int , std::monostate > > E;
 for( int t = 0 ; t < 40 ; ++t ) {
  const int u = int( g() % 12 ) , v = int( g() % 12 );
  if( ( u == v ) || L.hasEdge( u , v ) ) continue;
  L.insertEdge( u , v );
  M.insertEdge( u , v );
  E.emplace_back( u , v , std::monostate() );
  }
 L.deleteNode( 5 );
 M.deleteNode( 5 );
 CHECK( std::ranges::equal( L.nodes() , L.V() ) );
 CHECK( std::ranges::equal( M.nodes() , M.V() ) );
 for( int u : L.V() ) {
  CHECK( std::ranges::equal( L.neighbors( u ) , L.adj( u ) ) );
  CHECK( std::ranges::equal( M.neighbors( u ) , M.adj( u ) ) );
  }
 ArrayGraph<> A( 12 , E );
 CHECK( std::ranges::equal( A.nodes() , A.V() ) );
 for( int u : A.V() )
  CHECK( std::ranges::equal( A.neighbors( u ) , A.adj( u ) ) );

 ListMFSet F( 10 );
 F.merge( 1 , 2 );
 F.merge( 3 , 2 );
 auto m = F.members( 3 );
 std::ranges::sort( m );
 CHECK( m == std::vector< int >( { 1 , 2 , 3 } ) &&
        F.members( 0 ).size() == 1 );

 SparseMatrix S( 6 , 7 );
 for( int t = 0 ; t < 20 ; ++t )
  S.set( g() % 6 , g() % 7 , double( 1 + g() % 9 ) );
 for( std::size_t i = 0 ; i < 6 ; ++i )
  CHECK( std::ranges::equal( S.rowEntries( i ) , S.row( i ) ) );
 for( std::size_t j = 0 ; j < 7 ; ++j )
  CHECK( std::ranges::equal( S.colEntries( j ) , S.col( j ) ) );

 Arena< int > R( 8 );                       // across several blocks
 std::vector< int * > made;
 for( int k = 0 ; k < 30 ; ++k ) made.push_back( R.make( k ) );
 CHECK( std::ranges::equal( R , std::views::iota( 0 , 30 ) ) );
 CHECK( &R.begin()[ 17 ] == made[ 17 ] && R.end() - R.begin() == 30 );
 std::ranges::reverse( R );                 // random access, mutable
 CHECK( *made[ 0 ] == 29 && *made[ 29 ] == 0 );

 Tree< int > one( 7 );                      // the degenerate cases
 CHECK( std::ranges::distance( preorder( &one ) ) == 1 &&
        std::ranges::empty( children( &one ) ) );
 BinaryTree< int > b1( 7 );
 CHECK( std::ranges::distance( inorder( &b1 ) ) == 1 );
 CursorTree< int > c1( 7 );
 CHECK( std::ranges::empty( c1.children( c1.root() ) ) &&
        std::ranges::distance( c1.nodes() ) == 1 );
 ListGraph<> G0( 3 , true );                // no edges
 for( int u : G0.V() ) CHECK( std::ranges::empty( G0.neighbors( u ) ) );
 ArrayGraph<> A0( 3 , std::vector< std::tuple< int , int ,
                                              std::monostate > >() );
 for( int u : A0.V() ) CHECK( std::ranges::empty( A0.neighbors( u ) ) );
 CHECK( std::ranges::distance( A0.nodes() ) == 3 );
 SparseMatrix S0( 2 , 3 );                  // all zero
 CHECK( std::ranges::empty( S0.rowEntries( 1 ) ) &&
        std::ranges::empty( S0.colEntries( 2 ) ) );
 CHECK_THROWS( (void) S0.rowEntries( 2 ) , contract_violation );
 CHECK_THROWS( (void) S0.colEntries( 3 ) , contract_violation );
 Arena< int > R0( 4 );                      // empty, then exactly full
 CHECK( R0.begin() == R0.end() );
 for( int k = 0 ; k < 8 ; ++k ) R0.make( k );
 CHECK( std::ranges::equal( R0 , std::views::iota( 0 , 8 ) ) );
 CHECK( *std::prev( R0.end() ) == 7 && R0.end() - R0.begin() == 8 );
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
/*-------------------------- End File interop.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
