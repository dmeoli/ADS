/*--------------------------------------------------------------------------*/
/*---------------------------- File taocp.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the structures and the algorithms of Knuth's The Art of Computer
 * Programming that the book of the library does not have, and of their
 * relatives: each one against a reference of the standard library on random
 * operations, at the level 3 of the contracts, and its bounds (heights,
 * probes, false positives) against those of the analysis.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1-4,
 *      Addison-Wesley
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

#include "ADS/backtrack/DancingLinks.h"
#include "ADS/backtrack/NQueens.h"
#include "ADS/backtrack/SatSolvers.h"
#include "ADS/backtrack/Sudoku.h"
#include "ADS/combinatorial/Bitwise.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/dictionary/KnuthHashing.h"
#include "ADS/dictionary/SkipList.h"
#include "ADS/dictionary/Trie.h"
#include "ADS/memory/Allocators.h"
#include "ADS/memory/CellHeap.h"
#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/FFT.h"
#include "ADS/numeric/RandomNumbers.h"
#include "ADS/numeric/Rational.h"
#include "ADS/numeric/SparseMatrix.h"
#include "ADS/search/KnuthSearches.h"
#include "ADS/set/BloomFilter.h"
#include "ADS/sort/ExternalSort.h"
#include "ADS/sort/KnuthSorts.h"
#include "ADS/tree/AVLTree.h"
#include "ADS/tree/BTree.h"
#include "ADS/tree/SplayTree.h"
#include "ADS/tree/ThreadedTree.h"
#include "ADS/tree/Treap.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the Fibonacci search and the uniform binary search against
// std::binary_search on every sorted vector of 0 to 60 distinct even items,
// for every item and every gap; at most about 1.44 log2 n + 2 comparisons;
// the extremes of int, all equal items (a place of v), and an unsorted
// vector
ADS_TEST( "knuth-searches" )
{
 for( int n = 0 ; n <= 60 ; ++n ) {
  std::vector< int > A( n );
  for( int i = 0 ; i < n ; ++i ) A[ i ] = 2 * i;
  const double bound = n ? 1.4405 * std::log2( n + 2.0 ) + 2 : 1;
  for( int v = -1 ; v <= 2 * n ; ++v ) {
   long cf = 0 , cu = 0;
   const long f = fibonacciSearch( A , v , &cf ) ,
              u = uniformBinarySearch( A , v , &cu );
   const long r = ( v % 2 == 0 ) && ( v >= 0 ) && ( v < 2 * n ) ? v / 2 : -1;
   CHECK( f == r && u == r );
   CHECK( double( cf ) <= bound && double( cu ) <= bound );
   }
  }
 constexpr int lo = std::numeric_limits< int >::min() ,
               hi = std::numeric_limits< int >::max();
 const std::vector< int > X = { lo , lo + 1 , 0 , hi - 1 , hi };
 for( int i = 0 ; i < 5 ; ++i )                  // the extremes of int
  CHECK( fibonacciSearch( X , X[ i ] ) == i &&
         uniformBinarySearch( X , X[ i ] ) == i );
 CHECK( fibonacciSearch( X , 1 ) == -1 &&
        uniformBinarySearch( X , -1 ) == -1 );
 for( int n = 1 ; n <= 20 ; ++n ) {              // all equal: some place
  const std::vector< int > E( n , 7 );
  const long f = fibonacciSearch( E , 7 ) , u = uniformBinarySearch( E , 7 );
  CHECK( f >= 0 && f < n && u >= 0 && u < n );
  CHECK( fibonacciSearch( E , 6 ) == -1 &&
         uniformBinarySearch( E , 8 ) == -1 );
  }
 CHECK_THROWS( fibonacciSearch( std::vector< int >( { 2 , 1 } ) , 1 ) ,
               contract_violation );
 CHECK_THROWS( uniformBinarySearch( std::vector< int >( { 2 , 1 } ) , 1 ) ,
               contract_violation );
 }

// a dictionary against std::map on random insertions and removals of keys
// in [ 0 , U ), each lookup checked, and the keys in order at the end
template< class D >
void againstMap( D & T , unsigned seed , int U = 500 , int ops = 6000 )
{
 std::mt19937 g( seed );
 std::map< int , int > R;
 for( int op = 0 ; op < ops ; ++op ) {
  const int k = int( g() % unsigned( U ) );
  if( g() % 3 ) {
   T.insert( k , op );
   R[ k ] = op;
   }
  else {
   T.remove( k );
   R.erase( k );
   }
  const int q = int( g() % unsigned( U ) );
  const auto it = R.find( q );
  const auto got = T.lookup( q );
  CHECK( got.has_value() == ( it != R.end() ) );
  if( got ) CHECK( *got == it->second );
  }
 CHECK( T.size() == R.size() );
 std::vector< std::pair< int , int > > a , b( R.begin() , R.end() );
 T.forEach( [ & ]( int k , int v ) { a.emplace_back( k , v ); } );
 CHECK( a == b );
 }

// a dictionary on the degenerate cases: empty, an absent key removed, one
// key inserted twice (the second value kept), the extremes of int, and
// everything removed in decreasing order
template< class D >
void degenerate( D & T )
{
 CHECK( T.size() == 0 && ! T.lookup( 0 ) );
 T.remove( 0 );
 int seen = 0;
 T.forEach( [ & ]( int , int ) { ++seen; } );
 CHECK( seen == 0 );
 T.insert( 5 , 1 );
 T.insert( 5 , 2 );
 CHECK( T.size() == 1 && T.lookup( 5 ) == 2 );
 T.remove( 6 );
 T.remove( 5 );
 CHECK( T.size() == 0 && ! T.lookup( 5 ) );
 constexpr int lo = std::numeric_limits< int >::min() ,
               hi = std::numeric_limits< int >::max();
 T.insert( hi , 1 );
 T.insert( lo , 2 );
 CHECK( T.lookup( hi ) == 1 && T.lookup( lo ) == 2 && ! T.lookup( 0 ) );
 T.remove( hi );
 T.remove( lo );
 for( int i = 0 ; i < 300 ; ++i ) T.insert( 299 - i , i );   // decreasing
 for( int i = 299 ; i >= 0 ; --i ) T.remove( i );
 CHECK( T.size() == 0 && ! T.lookup( 7 ) );
 }

// the balanced trees: AVL within its height bound, the B-tree for three
// minimum degrees, its leaves at one depth (the invariant), the skip list,
// the treap and the splay tree, which also brings each key to the root;
// every one on the degenerate cases, the heights of the empty trees and of
// one key, and the root of an empty splay tree
ADS_TEST( "balanced-trees" )
{
 {
  AVLTree< int , int > a;
  degenerate( a );
  BTree< int , int > b( 2 );
  degenerate( b );
  CHECK( b.height() == -1 );
  b.insert( 1 , 1 );
  CHECK( b.height() == 0 );
  SkipList< int , int > l( 9 );
  degenerate( l );
  Treap< int , int > t( 9 );
  degenerate( t );
  CHECK( t.height() == -1 );
  SplayTree< int , int > y;
  degenerate( y );
  CHECK( ! y.root() );
  AVLTree< int , int > d;                        // decreasing insertions
  for( int i = 4095 ; i > 0 ; --i ) d.insert( i , i );
  CHECK( d.height() <= int( 1.4405 * std::log2( 4095 + 2.0 ) ) );
  }
 AVLTree< int , int > A;
 againstMap( A , 1 );
 AVLTree< int , int > S;                         // sorted insertions
 for( int i = 0 ; i < 4095 ; ++i ) S.insert( i , i );
 CHECK( S.height() <= int( 1.4405 * std::log2( 4095 + 2.0 ) ) );
 for( int t : { 2 , 3 , 7 } ) {
  BTree< int , int > B( t );
  againstMap( B , 2 + unsigned( t ) );
  }
 BTree< int , int > B2( 2 );
 for( int i = 0 ; i < 1000 ; ++i ) B2.insert( i , i );
 CHECK( B2.height() <= int( std::log2( ( 1000 + 1 ) / 2.0 ) ) );
 CHECK_THROWS( ( BTree< int , int >( 1 ) ) , contract_violation );
 SkipList< int , int > L( 3 );
 againstMap( L , 3 );
 Treap< int , int > T( 4 );
 againstMap( T , 4 );
 Treap< int , int > TS( 5 );                     // sorted, yet shallow
 for( int i = 0 ; i < 4096 ; ++i ) TS.insert( i , i );
 CHECK( TS.height() < 4 * 12 );
 SplayTree< int , int > Y;
 againstMap( Y , 5 );
 Y.insert( 42 , 1 );
 CHECK( Y.root() == 42 );
 (void) Y.lookup( 7 );
 SplayTree< int , int > P;                       // a path, freed without
 for( int i = 0 ; i < 20000 ; ++i ) P.insert( i , i );    // recursion
 CHECK( P.size() == 20000 );
 }

// the trie and the compact trie of strings against std::map, the keys
// with a prefix, and at most 2 n + 1 nodes in the compact one
template< class D >
void stringDictionary( D & T , unsigned seed )
{
 std::mt19937 g( seed );
 std::map< std::string , int > R;
 auto word = [ & ]( void ) {
  std::string s;
  for( int k = int( g() % 6 ) ; k > 0 ; --k ) s += char( 'a' + g() % 3 );
  return( s ); };
 for( int op = 0 ; op < 4000 ; ++op ) {
  const std::string k = word();
  if( g() % 3 ) {
   T.insert( k , op );
   R[ k ] = op;
   }
  else {
   T.remove( k );
   R.erase( k );
   }
  const std::string q = word();
  const auto it = R.find( q );
  CHECK( T.lookup( q ).has_value() == ( it != R.end() ) );
  }
 CHECK( T.size() == R.size() );
 for( std::string p : { "" , "a" , "ab" , "cab" , "zz" } ) {
  std::vector< std::string > want;
  for( const auto & kv : R )
   if( kv.first.compare( 0 , p.size() , p ) == 0 ) want.push_back( kv.first );
  CHECK( T.keysWithPrefix( p ) == want );
  }
 }

// the tries on random words (above), the compact trie of seven long keys,
// and both tries empty, with the empty key, with keys prefixes of one
// another (a chain), a key inserted again, and removals inside the chain
ADS_TEST( "tries" )
{
 Trie< int > T;
 stringDictionary( T , 6 );
 RadixTree< int > R;
 stringDictionary( R , 7 );
 CHECK( R.nodes() <= 2 * R.size() + 1 );
 RadixTree< int > W;                             // long keys, few nodes
 for( std::string s : { "romane" , "romanus" , "romulus" , "rubens" ,
                        "ruber" , "rubicon" , "rubicundus" } )
  W.insert( s , 1 );
 CHECK( W.nodes() == 14 && W.keysWithPrefix( "rub" ).size() == 4 );
 W.remove( "rubens" );
 CHECK( ! W.contains( "rubens" ) && W.contains( "ruber" ) );
 for( int kind = 0 ; kind < 2 ; ++kind ) {      // the empty key, prefixes
  Trie< int > A;
  RadixTree< int > B;
  auto run = [ & ]( auto & D ) {
   CHECK( D.size() == 0 && ! D.lookup( "" ) &&
          D.keysWithPrefix( "" ).empty() );
   D.remove( "x" );
   D.insert( "" , 1 );
   D.insert( "a" , 2 );
   D.insert( "aaaa" , 3 );
   D.insert( "aa" , 4 );
   D.insert( "aa" , 5 );                       // again: replaced
   CHECK( D.size() == 4 && D.lookup( "" ) == 1 && D.lookup( "aa" ) == 5 &&
          ! D.lookup( "aaa" ) && ! D.lookup( "aaaaa" ) );
   CHECK( D.keysWithPrefix( "aa" ) ==
          std::vector< std::string >( { "aa" , "aaaa" } ) );
   CHECK( D.keysWithPrefix( "" ).size() == 4 &&
          D.keysWithPrefix( "aaaaa" ).empty() );
   D.remove( "aa" );                           // inside the chain
   D.remove( "" );
   CHECK( D.size() == 2 && D.lookup( "aaaa" ) == 3 && ! D.lookup( "" ) );
   D.remove( "aaaa" );
   D.remove( "a" );
   CHECK( D.size() == 0 && D.keysWithPrefix( "" ).empty() );
   };
  if( kind ) run( B ); else run( A );
  }
 }

// the coalesced hashing with and without a cellar, and the double hashing
// with the insertion of Brent, whose successful searches take fewer than
// 2.5 probes on average even with the table 95 % full; the empty tables,
// a key inserted again into a full table (replaced), tables of one and of
// two places, and the sizes that are not allowed
ADS_TEST( "knuth-hashing" )
{
 std::mt19937 g( 8 );
 std::vector< int > keys;
 for( int i = 0 ; i < 950 ; ++i ) keys.push_back( int( g() ) );
 for( std::size_t cellar : { 0u , 140u } ) {
  CoalescedHashTable< int , int > C( 1000 - cellar , cellar );
  for( std::size_t i = 0 ; i < keys.size() ; ++i ) C.insert( keys[ i ] ,
                                                             int( i ) );
  for( std::size_t i = 0 ; i < keys.size() ; ++i )
   CHECK( C.lookup( keys[ i ] ) == int( i ) );
  CHECK( ! C.lookup( 3 ) || std::count( keys.begin() , keys.end() , 3 ) );
  }
 BrentHashTable< int , int > B( 1009 );
 for( std::size_t i = 0 ; i < keys.size() ; ++i ) B.insert( keys[ i ] ,
                                                            int( i ) );
 double probes = 0;
 for( std::size_t i = 0 ; i < keys.size() ; ++i ) {
  CHECK( B.lookup( keys[ i ] ) == int( i ) );
  probes += double( B.probes() );
  }
 CHECK( probes / double( keys.size() ) < 2.5 );
 CHECK_THROWS( ( BrentHashTable< int , int >( 1000 ) ) , contract_violation );
 CoalescedHashTable< int , int > F( 2 );
 CHECK( ! F.lookup( 1 ) && F.size() == 0 );     // empty
 F.insert( 1 , 1 );
 F.insert( 2 , 2 );
 F.insert( 2 , 20 );                             // full, a key again
 CHECK( F.size() == 2 && F.lookup( 2 ) == 20 && ! F.lookup( 3 ) );
 CHECK_THROWS( F.insert( 3 , 3 ) , contract_violation );   // full
 CoalescedHashTable< int , int > O( 1 );         // one place
 O.insert( -7 , 1 );
 CHECK( O.lookup( -7 ) == 1 && ! O.lookup( 7 ) );
 CHECK_THROWS( O.insert( 7 , 1 ) , contract_violation );
 BrentHashTable< int , int > T( 2 );             // the smallest prime
 CHECK( ! T.lookup( 0 ) );
 T.insert( 0 , 0 );
 T.insert( 1 , 1 );
 T.insert( 1 , 10 );
 CHECK( T.lookup( 0 ) == 0 && T.lookup( 1 ) == 10 && ! T.lookup( 2 ) );
 CHECK_THROWS( T.insert( 2 , 2 ) , contract_violation );   // full
 CHECK_THROWS( ( BrentHashTable< int , int >( 1 ) ) , contract_violation );
 CHECK_THROWS( ( CoalescedHashTable< int , int >( 0 ) ) ,
               contract_violation );
 }

// the filter of Bloom: no false negative, and false positives near
// ( 1 - e^( -k n / m ) )^k
ADS_TEST( "bloom-filter" )
{
 auto F = BloomFilter< int >::forItems( 10000 , 0.01 );
 for( int i = 0 ; i < 10000 ; ++i ) F.insert( 2 * i );
 for( int i = 0 ; i < 10000 ; ++i ) CHECK( F.maybe( 2 * i ) );
 int fp = 0;
 for( int i = 0 ; i < 100000 ; ++i ) fp += F.maybe( 2 * i + 1 );
 const double rate = fp / 100000.0;
 CHECK( std::abs( rate - F.falsePositiveRate() ) < 0.005 && rate < 0.02 );
 CHECK( F.hashes() == 7 );
 CHECK_THROWS( BloomFilter< int >( 0 , 1 ) , contract_violation );
 }

// the internal sorts of Knuth against std::sort on all the permutations
// of up to 7 items and on random vectors with repeated keys, the stable
// ones also on pairs; merge insertion within the bound of Ford and Johnson
// on every permutation of up to 8 items, and binary insertion within
// sum ceil( log2 k ); the extremes of int, all equal and decreasing keys,
// the address calculation with the infinities and 1 or 1000 lists
ADS_TEST( "knuth-sorts" )
{
 using Ints = std::vector< int >;
 std::vector< Ints > I;
 for( int n = 0 ; n <= 7 ; ++n ) {
  Ints p( n );
  std::iota( p.begin() , p.end() , 0 );
  do I.push_back( p ); while( std::next_permutation( p.begin() , p.end() ) );
  }
 std::mt19937 g( 9 );
 for( std::size_t n : { 50u , 333u , 2000u } ) {
  Ints r( n );
  for( auto & x : r ) x = int( g() % ( n / 3 + 1 ) ) - int( n / 6 );
  I.push_back( r );
  }
 constexpr int lo = std::numeric_limits< int >::min() ,
               hi = std::numeric_limits< int >::max();
 I.push_back( Ints( { hi , lo , 0 , -1 , hi , lo , 1 } ) );   // extremes
 I.push_back( Ints( 100 , lo ) );                // all equal
 Ints desc( 300 );                               // decreasing
 std::iota( desc.rbegin() , desc.rend() , -150 );
 I.push_back( desc );
 const double inf = std::numeric_limits< double >::infinity();
 std::vector< double > W = { 3.5 , -inf , 1e308 , -1e308 , inf , 0.0 ,
                             -0.5 };
 auto Ws = W;
 std::sort( Ws.begin() , Ws.end() );
 addressCalculationSort( W );                    // a span past a double
 CHECK( W == Ws );
 for( std::size_t M : { std::size_t( 1 ) , std::size_t( 1000 ) } ) {
  Ints A = desc;
  addressCalculationSort( A , M );               // one list, many lists
  CHECK( std::is_sorted( A.begin() , A.end() ) );
  }
 for( const auto & x : I ) {
  Ints B = x;
  std::sort( B.begin() , B.end() );
  for( int a = 0 ; a < 6 ; ++a ) {
   Ints A = x;
   switch( a ) {
    case( 0 ): binaryInsertionSort( A ); break;
    case( 1 ): listInsertionSort( A ); break;
    case( 2 ): addressCalculationSort( A ); break;
    case( 3 ): radixExchangeSort( A ); break;
    case( 4 ): tournamentSort( A ); break;
    default: mergeInsertionSort( A );
    }
   CHECK( A == B );
   }
  using P = std::pair< int , std::size_t >;
  std::vector< P > Q;
  for( std::size_t i = 0 ; i < x.size() ; ++i ) Q.emplace_back( x[ i ] % 3 ,
                                                                i );
  auto byKey = []( const P & u , const P & w ) {
   return( u.first < w.first ); };
  auto R = Q;
  std::stable_sort( R.begin() , R.end() , byKey );
  for( int a = 0 ; a < 3 ; ++a ) {
   auto S = Q;
   if( a == 0 ) binaryInsertionSort( S , byKey );
   else if( a == 1 ) listInsertionSort( S , byKey );
   else tournamentSort( S , byKey );
   CHECK( S == R );
   }
  }
 long c = 0;
 auto count = [ & ]( int u , int w ) {
  c += ! checkingContract();
  return( u < w ); };
 for( int n = 1 ; n <= 8 ; ++n ) {
  Ints p( n );
  std::iota( p.begin() , p.end() , 0 );
  long worst = 0;
  do {
   Ints A = p;
   c = 0;
   mergeInsertionSort( A , count );
   worst = std::max( worst , c );
   } while( std::next_permutation( p.begin() , p.end() ) );
  CHECK( worst <= fordJohnsonBound( std::size_t( n ) ) );
  }
 const long fj[] = { 0 , 1 , 3 , 5 , 7 , 10 , 13 , 16 , 19 , 22 , 26 , 30 };
 for( int n = 1 ; n <= 12 ; ++n )
  CHECK( fordJohnsonBound( std::size_t( n ) ) == fj[ n - 1 ] );
 Ints R( 1000 );
 for( auto & x : R ) x = int( g() );
 c = 0;
 binaryInsertionSort( R , count );
 long bound = 0;
 for( long k = 1 ; k < 1000 ; ++k ) {
  long b = 0;
  while( ( 1L << b ) < k + 1 ) ++b;
  bound += b;
  }
 CHECK( c <= bound );
 }

// the external sort: runs of about 2 M by replacement selection on random
// input and a single run on a sorted one, the polyphase merge on 3 to 6
// tapes, fewer phases with more tapes; a decreasing input (runs of M), M
// = 1, all equal keys, no input, no run and one run, a memory larger than
// the input, and the preconditions (2 tapes, M = 0, a run not sorted)
ADS_TEST( "external-sort" )
{
 std::mt19937 g( 10 );
 std::vector< int > A( 20000 );
 for( auto & x : A ) x = int( g() % 100000 );
 const auto runs = replacementSelection( A , 100 );
 const double mean = double( A.size() ) / double( runs.size() );
 CHECK( mean > 1.6 * 100 && mean < 2.4 * 100 );
 std::vector< int > S( 5000 );
 std::iota( S.begin() , S.end() , 0 );
 CHECK( replacementSelection( S , 10 ).size() == 1 );
 auto B = A;
 std::sort( B.begin() , B.end() );
 std::size_t prev = std::size_t( -1 );
 for( std::size_t T = 3 ; T <= 6 ; ++T ) {
  std::size_t phases = 0;
  CHECK( polyphaseMerge( runs , T , std::less<>() , &phases ) == B );
  CHECK( phases <= prev );
  prev = phases;
  }
 CHECK( externalSort( A , 50 , 4 ) == B );
 CHECK( externalSort( std::vector< int >() , 5 , 3 ).empty() );
 std::vector< int > D( 500 );                    // decreasing: runs of M
 std::iota( D.rbegin() , D.rend() , 0 );
 const auto rd = replacementSelection( D , 10 );
 CHECK( rd.size() == 50 && rd.front().size() == 10 );
 const auto r1 = replacementSelection( A , 1 );   // M = 1: the ascents
 for( const auto & r : r1 ) CHECK( std::is_sorted( r.begin() , r.end() ) );
 CHECK( polyphaseMerge( r1 , 3 ) == B );
 CHECK( replacementSelection( std::vector< int >( 77 , 4 ) , 3 ).size() ==
        1 );                                     // all equal: one run
 CHECK( replacementSelection( std::vector< int >() , 3 ).empty() );
 CHECK( polyphaseMerge( std::vector< std::vector< int > >() , 3 ).empty() );
 CHECK( polyphaseMerge( std::vector< std::vector< int > >( { { 1 , 2 } } ) ,
                        3 ) == std::vector< int >( { 1 , 2 } ) );
 CHECK( externalSort( D , 1 , 3 ) == std::vector< int >( D.rbegin() ,
                                                         D.rend() ) );
 CHECK( externalSort( A , 100000 , 3 ) == B );   // all in memory
 CHECK_THROWS( polyphaseMerge( runs , 2 ) , contract_violation );
 CHECK_THROWS( replacementSelection( A , 0 ) , contract_violation );
 CHECK_THROWS( polyphaseMerge( std::vector< std::vector< int > >(
               { { 2 , 1 } } ) , 3 ) , contract_violation );
 }

// the threaded tree: the keys by the threads, forward and backward, as
// std::set has them; the empty tree, one key inserted twice, and a chain
// of 10^4 decreasing keys (freed and walked without recursion)
ADS_TEST( "threaded-tree" )
{
 ThreadedTree< int > T;
 std::set< int > R;
 std::mt19937 g( 11 );
 for( int i = 0 ; i < 2000 ; ++i ) {
  const int k = int( g() % 700 );
  T.insert( k );
  R.insert( k );
  CHECK( T.contains( k ) && ! T.contains( 1000 + k ) );
  }
 std::vector< int > a;
 T.forEach( [ & ]( int k ) { a.push_back( k ); } );
 CHECK( a == std::vector< int >( R.begin() , R.end() ) &&
        T.size() == R.size() );
 CHECK( T.reversed() == std::vector< int >( R.rbegin() , R.rend() ) );
 ThreadedTree< int > E;
 CHECK( E.reversed().empty() && ! E.contains( 1 ) && E.size() == 0 );
 int seen = 0;
 E.forEach( [ & ]( int ) { ++seen; } );
 CHECK( seen == 0 );
 ThreadedTree< int > O;                          // one key, given twice
 O.insert( 3 );
 O.insert( 3 );
 CHECK( O.size() == 1 && O.reversed() == std::vector< int >( { 3 } ) );
 ThreadedTree< int > C;                          // a deep chain
 for( int i = 10000 ; i > 0 ; --i ) C.insert( i );
 std::vector< int > c;
 C.forEach( [ & ]( int k ) { c.push_back( k ); } );
 CHECK( c.size() == 10000 && c.front() == 1 && c.back() == 10000 &&
        std::is_sorted( c.begin() , c.end() ) );
 CHECK( C.reversed().front() == 10000 );
 }

// the allocators on random requests and releases, their invariants checked
// at each operation: the boundary tags with the three fits, the memory all
// free and in one block at the end; the buddy system, its blocks powers of
// 2 at least as large as the requests; the smallest memories (4 words, 1
// word), requests larger than the memory up to the largest long (refused
// before n + 2 or 2^k could overflow a long), a block freed twice,
// a request of 0, and the sizes not allowed
ADS_TEST( "allocators" )
{
 using F = BoundaryTagAllocator::Fit;
 for( F fit : { F::FIRST , F::BEST , F::NEXT } ) {
  BoundaryTagAllocator A( 4096 , fit );
  std::mt19937 g( 12 );
  std::vector< long > used;
  for( int op = 0 ; op < 3000 ; ++op )
   if( used.empty() || ( g() % 3 ) ) {
    const long a = A.allocate( 1 + long( g() % 60 ) );
    if( a >= 0 ) used.push_back( a );
    }
   else {
    const std::size_t i = g() % used.size();
    A.free( used[ i ] );
    used[ i ] = used.back();
    used.pop_back();
    }
  for( long a : used ) A.free( a );
  CHECK( A.freeWords() == 4096 && A.freeBlocks() == 1 );
  CHECK_THROWS( A.free( 1 ) , contract_violation );
  }
 BuddyAllocator B( 12 );
 std::mt19937 g( 13 );
 std::vector< std::pair< long , long > > used;
 for( int op = 0 ; op < 3000 ; ++op )
  if( used.empty() || ( g() % 3 ) ) {
   const long n = 1 + long( g() % 100 );
   const long a = B.allocate( n );
   if( a >= 0 ) {
    CHECK( B.blockSize( a ) >= n && B.blockSize( a ) < 2 * n + 1 );
    used.emplace_back( a , n );
    }
   }
  else {
   const std::size_t i = g() % used.size();
   B.free( used[ i ].first );
   used[ i ] = used.back();
   used.pop_back();
   }
 for( const auto & u : used ) B.free( u.first );
 CHECK( B.freeWords() == 4096 && B.allocate( 4096 ) == 0 &&
        B.allocate( 1 ) == -1 );
 constexpr long huge = std::numeric_limits< long >::max();
 BuddyAllocator O( 0 );                          // one word
 CHECK( O.allocate( 2 ) == -1 && O.allocate( huge ) == -1 &&
        O.allocate( 1 ) == 0 && O.blockSize( 0 ) == 1 &&
        O.allocate( 1 ) == -1 );
 O.free( 0 );
 CHECK( O.freeWords() == 1 );
 CHECK( B.allocate( huge ) == -1 );             // no 2^k past a long
 BoundaryTagAllocator T( 4 );                    // the smallest memory
 CHECK( T.allocate( huge ) == -1 && T.allocate( 5 ) == -1 );
 const long a = T.allocate( 1 );
 CHECK( a >= 0 && T.allocate( 1 ) == -1 );
 T.free( a );
 CHECK( T.freeWords() == 4 && T.freeBlocks() == 1 );
 CHECK_THROWS( T.allocate( 0 ) , contract_violation );
 CHECK_THROWS( T.free( a ) , contract_violation );      // twice
 CHECK_THROWS( O.allocate( 0 ) , contract_violation );
 CHECK_THROWS( O.free( 0 ) , contract_violation );
 CHECK_THROWS( BoundaryTagAllocator( 3 ) , contract_violation );
 CHECK_THROWS( BuddyAllocator( 41 ) , contract_violation );
 CHECK_THROWS( BuddyAllocator( -1 ) , contract_violation );
 }

// the garbage collector on random structures with cycles and sharing: the
// marking by pointer reversal frees the same cells as the one with a stack
// and leaves the reachable cells as they were (the audit); cons() collects
// by itself when the memory runs out; one cell pointing to itself, a
// collection with nothing to free, a list of 10^5 cells marked without
// recursion, a free cell read, a negative atom, and no cell
ADS_TEST( "cell-heap" )
{
 for( bool reversal : { true , false } ) {
  CellHeap H( 500 );
  std::mt19937 g( 14 );
  std::vector< CellHeap::Value > cells;
  for( int i = 0 ; i < 400 ; ++i )
   cells.push_back( H.cons( CellHeap::atom( i ) , CellHeap::nil ) );
  for( int i = 0 ; i < 600 ; ++i ) {        // random links, with cycles
   const auto a = cells[ g() % cells.size() ] ,
              b = cells[ g() % cells.size() ];
   if( g() % 2 ) H.setCar( a , b ); else H.setCdr( a , b );
   }
  std::set< CellHeap::Value > roots;
  for( int i = 0 ; i < 5 ; ++i ) {
   roots.insert( cells[ g() % cells.size() ] );
   }
  for( auto r : roots ) H.addRoot( r );
  const std::size_t freed = H.collect( reversal );
  CHECK( H.freeCells() == 100 + freed );
  for( auto r : roots ) H.removeRoot( r );
  H.collect( reversal );
  CHECK( H.freeCells() == 500 );
  }
 CellHeap S( 3 );                           // cons() collects by itself
 const auto keep = S.cons( CellHeap::atom( 1 ) , CellHeap::nil );
 S.addRoot( keep );
 for( int i = 0 ; i < 10 ; ++i ) S.cons( CellHeap::atom( 2 ) , keep );
 CHECK( S.car( keep ) == CellHeap::atom( 1 ) );
 const auto x = S.cons( keep , CellHeap::nil ) , y = S.cons( x , keep );
 S.addRoot( y );
 CHECK_THROWS( S.cons( CellHeap::nil , CellHeap::nil ) , contract_violation );
 for( bool reversal : { true , false } ) {
  CellHeap O( 1 );                               // one cell, a self-loop
  const auto c = O.cons( CellHeap::nil , CellHeap::nil );
  O.setCar( c , c );
  O.setCdr( c , c );
  O.addRoot( c );
  CHECK( O.collect( reversal ) == 0 && O.car( c ) == c && O.cdr( c ) == c );
  O.removeRoot( c );
  CHECK( O.collect( reversal ) == 1 && O.freeCells() == 1 );
  CHECK( O.collect( reversal ) == 0 );           // nothing to collect
  CHECK_THROWS( O.car( c ) , contract_violation );       // a free cell
  CellHeap L( 100000 );                          // a deep list: no stack
  auto head = CellHeap::nil;                     // overflow in the marks
  for( long i = 0 ; i < 100000 ; ++i )
   head = L.cons( CellHeap::atom( i ) , head );
  L.addRoot( head );
  CHECK( L.collect( reversal ) == 0 && L.freeCells() == 0 &&
         L.car( head ) == CellHeap::atom( 99999 ) );
  }
 CHECK_THROWS( CellHeap::atom( -1 ) , contract_violation );
 CHECK_THROWS( CellHeap( 0 ) , contract_violation );
 }

// the pivot step on orthogonal lists against the dense one, and the
// inverse of a matrix by a pivot step on each diagonal place; zeros set
// and removed, empty rows and columns, a 1 x 1 matrix, and the bounds of
// every access and of a pivot (also on a zero)
ADS_TEST( "sparse-matrix" )
{
 std::mt19937 g( 15 );
 const std::size_t n = 12;
 SparseMatrix S( n , n );
 std::vector< std::vector< double > > D( n , std::vector< double >( n , 0 ) );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  D[ i ][ i ] = 10 + double( g() % 5 );     // diagonally dominant
  for( int k = 0 ; k < 3 ; ++k ) {
   const std::size_t j = g() % n;
   if( j != i ) D[ i ][ j ] = double( int( g() % 5 ) - 2 );
   }
  for( std::size_t j = 0 ; j < n ; ++j ) S.set( i , j , D[ i ][ j ] );
  }
 const auto A = D;
 for( std::size_t k = 0 ; k < n ; ++k ) {   // the dense pivot step
  const double a = D[ k ][ k ];
  auto E = D;
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    if( ( i != k ) && ( j != k ) )
     E[ i ][ j ] = D[ i ][ j ] - D[ k ][ j ] * D[ i ][ k ] / a;
  for( std::size_t j = 0 ; j < n ; ++j )
   if( j != k ) E[ k ][ j ] = D[ k ][ j ] / a;
  for( std::size_t i = 0 ; i < n ; ++i )
   if( i != k ) E[ i ][ k ] = - D[ i ][ k ] / a;
  E[ k ][ k ] = 1 / a;
  D = E;
  S.pivot( k , k );
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    CHECK( std::abs( S.get( i , j ) - D[ i ][ j ] ) < 1e-9 );
  }
 for( std::size_t i = 0 ; i < n ; ++i )     // A times the result is I
  for( std::size_t j = 0 ; j < n ; ++j ) {
   double s = 0;
   for( std::size_t k = 0 ; k < n ; ++k ) s += A[ i ][ k ] * S.get( k , j );
   CHECK( std::abs( s - ( i == j ? 1 : 0 ) ) < 1e-9 );
   }
 SparseMatrix Z( 3 , 4 );
 Z.set( 1 , 2 , 5 );
 Z.set( 1 , 0 , 1 );
 Z.set( 1 , 2 , 0 );
 CHECK( Z.nonzeros() == 1 && Z.row( 1 ).size() == 1 && Z.col( 2 ).empty() );
 CHECK( Z.get( 2 , 3 ) == 0 && Z.row( 0 ).empty() && Z.col( 3 ).empty() );
 Z.set( 0 , 0 , 0 );                             // a zero not stored
 Z.set( 1 , 0 , 0 );                             // the only one removed
 CHECK( Z.nonzeros() == 0 );
 SparseMatrix O( 1 , 1 );                        // 1 x 1: the reciprocal
 O.set( 0 , 0 , -4 );
 O.pivot( 0 , 0 );
 CHECK( O.get( 0 , 0 ) == -0.25 && O.nonzeros() == 1 );
 CHECK_THROWS( Z.pivot( 0 , 0 ) , contract_violation );
 CHECK_THROWS( Z.pivot( 3 , 0 ) , contract_violation );
 CHECK_THROWS( Z.set( 3 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( Z.set( 0 , 4 , 1 ) , contract_violation );
 CHECK_THROWS( Z.get( 3 , 0 ) , contract_violation );
 CHECK_THROWS( Z.row( 3 ) , contract_violation );
 CHECK_THROWS( Z.col( 4 ) , contract_violation );
 }

// the broadword tricks against the loops over the bits on random words;
// the words 0 and all 1s, the extreme bits, the bytes of 0, and the words
// with no next one of their weight
ADS_TEST( "bitwise" )
{
 std::mt19937_64 g( 16 );
 for( int t = 0 ; t < 20000 ; ++t ) {
  Word x = g() >> ( g() % 64 );
  if( t < 64 ) x = Word( 1 ) << t;
  int pop = 0 , low = -1 , high = -1;
  Word rev = 0;
  for( int i = 0 ; i < 64 ; ++i )
   if( x >> i & 1 ) {
    ++pop;
    if( low < 0 ) low = i;
    high = i;
    rev |= Word( 1 ) << ( 63 - i );
    }
  CHECK( sidewaysSum( x ) == pop && reverseBits( x ) == rev );
  if( x ) {
   CHECK( ruler( x ) == low && lambda( x ) == high );
   CHECK( rightmostOne( x ) == ( Word( 1 ) << low ) );
   CHECK( smearRight( x ) == ( x | ( ( Word( 1 ) << low ) - 1 ) ) );
   }
  CHECK( grayInverse( gray( x ) ) == x );
  CHECK( sidewaysSum( gray( x ) ^ gray( x + 1 ) ) == 1 || x == ~Word( 0 ) );
  const std::uint32_t lo = std::uint32_t( x ) , hi = std::uint32_t( x >> 32 );
  const auto [ a , b ] = deinterleave( interleave( lo , hi ) );
  CHECK( a == lo && b == hi );
  bool zero = false;
  for( int i = 0 ; i < 8 ; ++i )
   zero = zero || ( ( x >> ( 8 * i ) & 0xFF ) == 0 );
  CHECK( hasZeroByte( x ) == zero );
  }
 Word w = 0b111;                            // the 3-subsets of 10 items
 int k = 1;
 while( ( w = nextSameWeight( w ) ) < ( Word( 1 ) << 10 ) ) {
  CHECK( sidewaysSum( w ) == 3 );
  ++k;
  }
 CHECK( k == 120 );
 const Word all = ~Word( 0 );                    // the words 0 and all 1s
 CHECK( sidewaysSum( 0 ) == 0 && sidewaysSum( all ) == 64 &&
        reverseBits( 0 ) == 0 && reverseBits( all ) == all );
 CHECK( gray( 0 ) == 0 && grayInverse( 0 ) == 0 &&
        grayInverse( gray( all ) ) == all && rightmostOne( 0 ) == 0 );
 CHECK( ruler( all ) == 0 && lambda( all ) == 63 && lambda( 1 ) == 0 &&
        ruler( Word( 1 ) << 63 ) == 63 && rightmostOne( all ) == 1 );
 CHECK( hasZeroByte( 0 ) && ! hasZeroByte( all ) &&
        hasZeroByte( all >> 8 ) && ! hasZeroByte( 0x0101010101010101ULL ) );
 CHECK( interleave( ~std::uint32_t( 0 ) , ~std::uint32_t( 0 ) ) == all &&
        interleave( 0 , 0 ) == 0 );
 CHECK( nextSameWeight( Word( 1 ) << 62 ) == ( Word( 1 ) << 63 ) &&
        nextSameWeight( 1 ) == 2 );
 CHECK_THROWS( (void) ruler( 0 ) , contract_violation );
 CHECK_THROWS( (void) lambda( 0 ) , contract_violation );
 CHECK_THROWS( (void) nextSameWeight( 0 ) , contract_violation );
 CHECK_THROWS( (void) nextSameWeight( Word( 1 ) << 63 ) ,
               contract_violation );                   // no next word
 CHECK_THROWS( (void) nextSameWeight( all ) , contract_violation );
 }

// the generators: their counts (the products, the multinomial, n!, the
// binomial, p( n ), the Bell and the Catalan numbers), all distinct, in
// their order, and the Gray property of the Gray orders; a visit that
// stops at once for each generator, no digit, radices of 1, all equal
// items or none, and the sizes not allowed
ADS_TEST( "generation" )
{
 auto collect = []( auto gen ) {
  std::vector< Tuple > out;
  gen( [ & ]( const Tuple & t ) { out.push_back( t ); return( false ); } );
  return( out );
  };
 const std::vector< int > m = { 2 , 3 , 4 };
 const auto L = collect( [ & ]( auto v ) { return( mixedRadix( m , v ) ); } );
 const auto G = collect( [ & ]( auto v ) { return( grayTuples( m , v ) ); } );
 CHECK( L.size() == 24 && std::is_sorted( L.begin() , L.end() ) );
 CHECK( std::set< Tuple >( G.begin() , G.end() ).size() == 24 );
 for( std::size_t i = 1 ; i < G.size() ; ++i ) {
  int d = 0;
  for( std::size_t j = 0 ; j < 3 ; ++j )
   d += std::abs( G[ i ][ j ] - G[ i - 1 ][ j ] );
  CHECK( d == 1 );
  }
 const auto P = collect( []( auto v ) {
  return( permutationsLex( Tuple( { 1 , 1 , 2 , 3 } ) , v ) ); } );
 CHECK( P.size() == 12 && std::is_sorted( P.begin() , P.end() ) );
 for( int n = 0 ; n <= 6 ; ++n ) {
  long fact = 1;
  for( int i = 2 ; i <= n ; ++i ) fact *= i;
  const auto S = collect( [ n ]( auto v ) {
   return( plainChanges( n , v ) ); } );
  const auto H = collect( [ n ]( auto v ) {
   return( heapPermutations( n , v ) ); } );
  CHECK( long( S.size() ) == fact && long( H.size() ) == fact );
  CHECK( long( std::set< Tuple >( S.begin() , S.end() ).size() ) == fact );
  CHECK( long( std::set< Tuple >( H.begin() , H.end() ).size() ) == fact );
  for( std::size_t i = 1 ; i < S.size() ; ++i ) {
   std::vector< int > diff;                 // an adjacent swap, and a swap
   for( int j = 0 ; j < n ; ++j ) if( S[ i ][ j ] != S[ i - 1 ][ j ] )
    diff.push_back( j );
   CHECK( diff.size() == 2 && diff[ 1 ] == diff[ 0 ] + 1 );
   int dh = 0;
   for( int j = 0 ; j < n ; ++j ) dh += H[ i ][ j ] != H[ i - 1 ][ j ];
   CHECK( dh == 2 );
   }
  }
 for( int n = 0 ; n <= 8 ; ++n )
  for( int t = 0 ; t <= n ; ++t ) {
   long binom = 1;
   for( int i = 1 ; i <= t ; ++i ) binom = binom * ( n - t + i ) / i;
   const auto C = collect( [ n , t ]( auto v ) {
    return( combinationsLex( n , t , v ) ); } );
   const auto R = collect( [ n , t ]( auto v ) {
    return( revolvingDoor( n , t , v ) ); } );
   CHECK( long( C.size() ) == binom && std::is_sorted( C.begin() , C.end() ) );
   CHECK( long( std::set< Tuple >( R.begin() , R.end() ).size() ) == binom );
   for( std::size_t i = 1 ; i < R.size() ; ++i ) {
    std::set< int > a( R[ i ].begin() , R[ i ].end() ) ,
                    b( R[ i - 1 ].begin() , R[ i - 1 ].end() ) , d;
    std::set_symmetric_difference( a.begin() , a.end() , b.begin() ,
                                   b.end() , std::inserter( d , d.begin() ) );
    CHECK( d.size() == 2 );                 // one in, one out
    }
   }
 const long p[] = { 1 , 1 , 2 , 3 , 5 , 7 , 11 , 15 , 22 , 30 , 42 };
 const long bell[] = { 1 , 1 , 2 , 5 , 15 , 52 , 203 , 877 , 4140 };
 const long catalan[] = { 1 , 1 , 2 , 5 , 14 , 42 , 132 , 429 , 1430 };
 for( int n = 0 ; n <= 10 ; ++n ) {
  const auto I = collect( [ n ]( auto v ) {
   return( integerPartitions( n , v ) ); } );
  CHECK( long( I.size() ) == p[ n ] );
  for( const auto & q : I )
   CHECK( std::accumulate( q.begin() , q.end() , 0 ) == n &&
          std::is_sorted( q.rbegin() , q.rend() ) );
  CHECK( std::is_sorted( I.rbegin() , I.rend() ) );   // reverse lexicographic
  }
 for( int n = 0 ; n <= 8 ; ++n ) {
  CHECK( setPartitions( n , []( const Tuple & ) { return( false ); } ) ==
         bell[ n ] );
  std::vector< std::string > W;
  nestedParentheses( n , [ & ]( const std::string & w ) {
   W.push_back( w ); return( false ); } );
  CHECK( long( W.size() ) == catalan[ n ] &&
         std::is_sorted( W.begin() , W.end() ) );
  }
 CHECK( combinationsLex( 5 , 2 , []( const Tuple & ) { return( true ); } ) ==
        1 );                                // a visit that stops
 auto stop = []( const auto & ) { return( true ); };
 CHECK( mixedRadix( m , stop ) == 1 && grayTuples( m , stop ) == 1 &&
        plainChanges( 4 , stop ) == 1 && heapPermutations( 4 , stop ) == 1 &&
        revolvingDoor( 5 , 2 , stop ) == 1 &&
        integerPartitions( 5 , stop ) == 1 && setPartitions( 5 , stop ) == 1 &&
        nestedParentheses( 3 , stop ) == 1 &&
        permutationsLex( Tuple( { 3 , 1 } ) , stop ) == 1 );
 const auto E = collect( []( auto v ) {         // no digit: one tuple
  return( mixedRadix( std::vector< int >() , v ) ); } );
 CHECK( E.size() == 1 && E[ 0 ].empty() );
 const auto O = collect( []( auto v ) {         // radices 1: one tuple
  return( mixedRadix( std::vector< int >( { 1 , 1 , 1 } ) , v ) ); } );
 CHECK( O == std::vector< Tuple >( { Tuple( 3 , 0 ) } ) );
 const auto GE = collect( []( auto v ) {
  return( grayTuples( std::vector< int >() , v ) ); } );
 CHECK( GE.size() == 1 && GE[ 0 ].empty() );
 const auto PE = collect( []( auto v ) {         // all equal, or none
  return( permutationsLex( Tuple( 5 , 2 ) , v ) ); } );
 CHECK( PE.size() == 1 && collect( []( auto v ) {
  return( permutationsLex( Tuple() , v ) ); } ).size() == 1 );
 CHECK_THROWS( mixedRadix( std::vector< int >( { 2 , 0 } ) , stop ) ,
               contract_violation );
 CHECK_THROWS( grayTuples( std::vector< int >( { 2 , 1 } ) , stop ) ,
               contract_violation );
 CHECK_THROWS( combinationsLex( 3 , 4 , stop ) , contract_violation );
 CHECK_THROWS( combinationsLex( 3 , -1 , stop ) , contract_violation );
 CHECK_THROWS( revolvingDoor( 3 , 4 , stop ) , contract_violation );
 CHECK_THROWS( plainChanges( -1 , stop ) , contract_violation );
 CHECK_THROWS( heapPermutations( -1 , stop ) , contract_violation );
 CHECK_THROWS( integerPartitions( -1 , stop ) , contract_violation );
 CHECK_THROWS( setPartitions( -1 , stop ) , contract_violation );
 CHECK_THROWS( nestedParentheses( -1 , stop ) , contract_violation );
 }

// the random numbers of volume 2: the period of Hull and Dobell, minstd at
// its known value after 10000 steps, the chi-square and the
// Kolmogorov-Smirnov tests passed by a good generator and failed by a bad
// one, the spectral test separating the two multipliers, the shuffle and
// the samplings uniform
ADS_TEST( "random-numbers" )
{
 CHECK( LinearCongruential( 5 , 3 , 64 , 0 ).period( 1000 ) == 64 );
 CHECK( LinearCongruential( 5 , 2 , 64 , 0 ).period( 1000 ) < 64 );
 CHECK( LinearCongruential( 3 , 3 , 64 , 0 ).period( 1000 ) < 64 );
 auto ms = LinearCongruential::minstd( 1 );
 std::uint64_t x = 0;
 for( int i = 0 ; i < 10000 ; ++i ) x = ms();
 CHECK( x == 1043618065 );                  // as Park and Miller report it
 auto mm = LinearCongruential::mmix( 7 );
 std::mt19937_64 ref( 3 );
 std::vector< double > U , V;
 for( int i = 0 ; i < 20000 ; ++i ) {
  U.push_back( double( mm() >> 11 ) * 0x1.0p-53 );
  V.push_back( double( i % 100 ) / 100 );   // too regular
  }
 std::vector< long > Y( 10 , 0 );
 for( double u : U ) ++Y[ std::size_t( u * 10 ) ];
 const double V9 = chiSquare( Y , std::vector< double >( 10 , 0.1 ) );
 CHECK( V9 < 27.88 );                       // the 99.9% point, 9 freedoms
 const auto [ kp , km ] = kolmogorovSmirnov( U );
 CHECK( kp < 1.95 && km < 1.95 );           // the 99.9% point
 CHECK( serialTest( U , 8 ) < 103.4 );      // 63 freedoms
 CHECK( serialTest( V , 8 ) > 1000 );       // the pairs lie on lines
 const auto [ bp , bm ] =
  kolmogorovSmirnov( std::vector< double >( 100 , 0.5 ) );
 CHECK( std::max( bp , bm ) > 4 );
 CHECK( spectralTest2( 16807 , 2147483647 ) > 10000 );
 CHECK( spectralTest2( 1 , 2147483647 ) < 2 );   // X_{n+1} = X_n
 CHECK( std::abs( spectralTest2( 3 , 64 ) - std::sqrt( 10.0 ) ) < 1e-9 );
 std::map< std::vector< int > , long > perm;
 for( int t = 0 ; t < 24000 ; ++t ) {
  std::vector< int > A = { 0 , 1 , 2 , 3 };
  shuffle( A , ref );
  ++perm[ A ];
  }
 std::vector< long > P;
 for( const auto & [ k , c ] : perm ) P.push_back( c );
 CHECK( P.size() == 24 &&
        chiSquare( P , std::vector< double >( 24 , 1.0 / 24 ) ) < 49.73 );
 std::vector< long > S( 10 , 0 ) , R( 10 , 0 );
 std::vector< int > stream( 10 );
 std::iota( stream.begin() , stream.end() , 0 );
 for( int t = 0 ; t < 10000 ; ++t ) {
  const auto s = selectionSampling( 3 , 10 , ref );
  CHECK( s.size() == 3 && std::is_sorted( s.begin() , s.end() ) );
  for( auto i : s ) ++S[ i ];
  for( int i : reservoirSampling( stream , 3 , ref ) ) ++R[ i ];
  }
 CHECK( chiSquare( S , std::vector< double >( 10 , 0.1 ) ) < 27.88 );
 CHECK( chiSquare( R , std::vector< double >( 10 , 0.1 ) ) < 27.88 );
 std::mt19937 g32( 4 );                     // a generator of 32 bits too
 std::vector< long > S32( 10 , 0 );
 for( int t = 0 ; t < 10000 ; ++t )
  for( auto i : selectionSampling( 3 , 10 , g32 ) ) ++S32[ i ];
 CHECK( chiSquare( S32 , std::vector< double >( 10 , 0.1 ) ) < 27.88 );
 CHECK( selectionSampling( 10 , 10 , ref ).size() == 10 );
 CHECK( selectionSampling( 0 , 0 , ref ).empty() &&
        selectionSampling( 0 , 10 , ref ).empty() );
 const std::uint64_t top = ~std::uint64_t( 0 );  // the largest modulus
 LinearCongruential X( top - 1 , top - 1 , top , top - 1 );
 CHECK( X() == 0 && X() == top - 1 && X() == 0 );   // ( -1 ) ( -1 ) - 1
 CHECK( LinearCongruential( 0 , 0 , 1 , 0 ).period( 10 ) == 1 );   // m = 1
 CHECK( LinearCongruential( 1 , 0 , 0 , 5 ).period( 10 ) == 1 );   // fixed
 CHECK( LinearCongruential( 1 , 1 , 0 , 0 ).period( 1000 ) == 0 );  // long
 std::vector< double > A1 = { 0.5 };
 const auto [ k1 , k2 ] = kolmogorovSmirnov( A1 );
 CHECK( std::abs( k1 - 0.5 ) < 1e-12 && std::abs( k2 - 0.5 ) < 1e-12 );
 CHECK( chiSquare( std::vector< long >( { 5 , 5 } ) ,
                   std::vector< double >( { 0.5 , 0.5 } ) ) == 0 );
 CHECK_THROWS( (void) selectionSampling( 11 , 10 , ref ) ,
               contract_violation );
 CHECK_THROWS( LinearCongruential( 64 , 0 , 64 , 0 ) , contract_violation );
 CHECK_THROWS( LinearCongruential( 5 , 0 , 64 , 64 ) , contract_violation );
 CHECK_THROWS( chiSquare( std::vector< long >( { 1 } ) ,
                          std::vector< double >( { 0.5 , 0.5 } ) ) ,
               contract_violation );
 CHECK_THROWS( chiSquare( std::vector< long >( { 1 , 1 } ) ,
                          std::vector< double >( { 1 , 0 } ) ) ,
               contract_violation );
 CHECK_THROWS( kolmogorovSmirnov( std::vector< double >() ) ,
               contract_violation );
 CHECK_THROWS( serialTest( U , 1 ) , contract_violation );
 CHECK_THROWS( spectralTest2( 0 , 64 ) , contract_violation );
 CHECK_THROWS( spectralTest2( 3 , 1 ) , contract_violation );
 }

// the FFT against the transform by its definition, the inverse of the
// transform, and the product of polynomials against the naive one; no
// point, an empty polynomial, two constants, the zero polynomial, the
// impulse, and sizes that are not powers of 2
ADS_TEST( "fft" )
{
 std::mt19937 g( 17 );
 for( std::size_t n : { 1 , 2 , 8 , 64 , 256 } ) {
  std::vector< Complex > a( n );
  for( auto & x : a )
   x = Complex( int( g() % 21 ) - 10 , int( g() % 21 ) - 10 );
  auto f = a;
  fft( f , false );
  const auto d = dft( a , false );
  for( std::size_t k = 0 ; k < n ; ++k )
   CHECK( std::abs( f[ k ] - d[ k ] ) < 1e-8 );
  fft( f , true );
  for( std::size_t k = 0 ; k < n ; ++k )
   CHECK( std::abs( f[ k ] - a[ k ] ) < 1e-9 );
  }
 std::vector< double > p( 300 ) , q( 200 );
 for( auto & x : p ) x = double( g() % 1000 );
 for( auto & x : q ) x = double( g() % 1000 );
 const auto c = convolve( p , q );
 CHECK( c.size() == 499 );
 for( std::size_t k = 0 ; k < c.size() ; ++k ) {
  double s = 0;
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   if( ( k >= i ) && ( k - i < q.size() ) ) s += p[ i ] * q[ k - i ];
  CHECK( std::abs( c[ k ] - s ) < 1e-3 );
  }
 std::vector< Complex > none;                    // no point: nothing
 fft( none , false );
 CHECK( none.empty() && dft( none , true ).empty() );
 CHECK( convolve( p , std::vector< double >() ).empty() &&
        convolve( std::vector< double >() , q ).empty() );
 const auto one = convolve( { 3 } , { -2 } );    // constants
 CHECK( one.size() == 1 && std::abs( one[ 0 ] + 6 ) < 1e-12 );
 const auto z = convolve( std::vector< double >( 5 , 0 ) , q );
 CHECK( z.size() == 204 && std::all_of( z.begin() , z.end() ,
        []( double v ) { return( std::abs( v ) < 1e-9 ); } ) );
 std::vector< Complex > imp( 16 );               // the impulse: all ones
 imp[ 0 ] = 1;
 fft( imp , false );
 for( const auto & v : imp ) CHECK( std::abs( v - Complex( 1 ) ) < 1e-12 );
 std::vector< Complex > bad( 3 );
 CHECK_THROWS( fft( bad , false ) , contract_violation );
 std::vector< Complex > bad6( 6 );
 CHECK_THROWS( fft( bad6 , true ) , contract_violation );
 }

// the integers of any length: against long long where it fits, the
// identities of the division on random operands of many sizes (the audit
// checks a = q b + r), the FFT product against Algorithm M, the
// conversions of base back and forth, 2^200 and 100! as written, and the
// trial quotient corrected by the add back of D6 on a case built for it;
// the extremes of long long, zero and its sign, the four signs of the
// division, the powers and gcds of 0, the bases 2 and 36, and the strings,
// bases and divisors not allowed
ADS_TEST( "big-integer" )
{
 std::mt19937_64 g( 18 );
 for( int t = 0 ; t < 3000 ; ++t ) {
  const long long x = ( long long )( g() >> 34 ) - ( 1LL << 29 ) ,
                  y = ( long long )( g() >> 34 ) - ( 1LL << 29 );
  const BigInteger X( x ) , Y( y );
  CHECK( X + Y == BigInteger( x + y ) && X - Y == BigInteger( x - y ) );
  CHECK( X * Y == BigInteger( x * y ) );
  CHECK( ( X < Y ) == ( x < y ) && ( X == Y ) == ( x == y ) );
  if( y != 0 )
   CHECK( X / Y == BigInteger( x / y ) && X % Y == BigInteger( x % y ) );
  CHECK( X.toString() == std::to_string( x ) );
  }
 auto random = [ & ]( std::size_t digits ) {   // a random hexadecimal number
  std::string s = ( g() % 2 ) ? "-" : "";
  s += "123456789abcdef"[ g() % 15 ];
  for( std::size_t i = 1 ; i < digits ; ++i )
   s += "0123456789abcdef"[ g() % 16 ];
  return( BigInteger( s , 16 ) );
  };
 for( int t = 0 ; t < 300 ; ++t ) {
  const BigInteger a = random( 1 + g() % 400 ) , b = random( 1 + g() % 200 );
  const auto [ q , r ] = BigInteger::divMod( a , b );
  CHECK( q * b + r == a && r.abs() < b.abs() );
  CHECK( ( a + b ) - b == a && a * b == b * a );
  CHECK( BigInteger( a.toString( 7 ) , 7 ) == a );
  CHECK( BigInteger( a.toString() ) == a );
  }
 for( int t = 0 ; t < 40 ; ++t ) {         // Karatsuba, also unbalanced
  const BigInteger a = random( 1 + g() % 3000 ) , b = random( 1 + g() % 900 );
  CHECK( BigInteger::multiplyKaratsuba( a , b ) ==
         BigInteger::multiplySchool( a , b ) );
  }
 for( std::size_t n : { 256 , 300 , 1000 , 2000 } ) {   // the FFT product
  const BigInteger a = random( 8 * n ) , b = random( 8 * n + 13 );
  CHECK( BigInteger::multiplyFFT( a , b ) ==
         BigInteger::multiplySchool( a , b ) );
  }
 const BigInteger two200 = BigInteger::pow( 2 , 200 );
 CHECK( two200.toString() ==
        "1606938044258990275541962092341162602522202993782792835301376" );
 CHECK( two200.toString( 16 ) == "1" + std::string( 50 , '0' ) );
 BigInteger f = 1;
 for( int i = 2 ; i <= 100 ; ++i ) f *= i;
 CHECK( f.toString().size() == 158 &&
        f.toString().substr( 0 , 10 ) == "9332621544" );
 CHECK( f / BigInteger::pow( 10 , 24 ) % 10 != 0 );   // 24 trailing zeros
 CHECK( f % BigInteger::pow( 10 , 24 ) == 0 );
 // digits at the edges of their range, where the trial quotient of D3 is
 // too large and D6 adds back
 const char * edge[] = { "00000000" , "00000001" , "7fffffff" , "80000000" ,
                         "fffffffe" , "ffffffff" };
 for( int t = 0 ; t < 3000 ; ++t ) {
  std::string su = "1" , sv = "1";
  for( std::size_t i = 1 + g() % 6 ; i-- > 0 ; ) su += edge[ g() % 6 ];
  for( std::size_t i = 1 + g() % 3 ; i-- > 0 ; ) sv += edge[ g() % 6 ];
  const BigInteger u( su , 16 ) , v( sv , 16 );
  const auto [ q , r ] = BigInteger::divMod( u , v );
  CHECK( q * v + r == u && r < v && r.sign() >= 0 );
  }
 CHECK( BigInteger::gcd( BigInteger::pow( 6 , 50 ) ,
                         BigInteger::pow( 15 , 40 ) ) ==
        BigInteger::pow( 3 , 40 ) );
 CHECK( ( -BigInteger( 7 ) ) / 2 == BigInteger( -3 ) &&
        ( -BigInteger( 7 ) ) % 2 == BigInteger( -1 ) );
 CHECK( BigInteger( "-0" ) == BigInteger( 0 ) &&
        BigInteger( "-0" ).sign() == 0 );
 CHECK_THROWS( (void) ( BigInteger( 1 ) / BigInteger( 0 ) ) ,
               contract_violation );
 CHECK_THROWS( BigInteger( "12z" ) , contract_violation );
 constexpr long long lmin = std::numeric_limits< long long >::min() ,
                     lmax = std::numeric_limits< long long >::max();
 const BigInteger Lmin( lmin ) , Lmax( lmax );   // the extremes of long long
 CHECK( Lmin.toString() == "-9223372036854775808" &&
        Lmax.toString() == "9223372036854775807" );
 CHECK( Lmin + Lmax == BigInteger( -1 ) && -Lmin == Lmax + 1 &&
        Lmin.abs().toString( 16 ) == "8000000000000000" );
 CHECK( Lmin * Lmin == BigInteger::pow( 2 , 126 ) &&
        Lmin / BigInteger( -1 ) == Lmax + 1 );
 const BigInteger zero;                          // zero and the signs
 CHECK( zero.sign() == 0 && zero.toString() == "0" &&
        zero.toString( 2 ) == "0" && ( -zero ).sign() == 0 );
 CHECK( zero * Lmin == zero && ( zero * Lmin ).sign() == 0 &&
        Lmin - Lmin == zero && ( Lmin - Lmin ).sign() == 0 &&
        zero / Lmax == zero && zero % Lmax == zero );
 for( long long a : { 7LL , -7LL } )             // truncated division
  for( long long b : { 2LL , -2LL , 7LL , -7LL , 8LL , -8LL } ) {
   const auto [ q , r ] = BigInteger::divMod( a , b );
   CHECK( q == BigInteger( a / b ) && r == BigInteger( a % b ) );
   }
 CHECK( BigInteger::pow( 12345 , 0 ) == 1 && BigInteger::pow( 0 , 0 ) == 1 &&
        BigInteger::pow( 0 , 5 ) == 0 && BigInteger::pow( -2 , 63 ) == Lmin );
 CHECK( BigInteger::gcd( 0 , 0 ) == 0 && BigInteger::gcd( 0 , -9 ) == 9 &&
        BigInteger::gcd( Lmin , Lmin ) == Lmax + 1 );
 CHECK( BigInteger( "zz" , 36 ) == BigInteger( 1295 ) &&
        BigInteger( 1295 ).toString( 36 ) == "zz" &&
        BigInteger( "-101" , 2 ) == BigInteger( -5 ) &&
        BigInteger( "000123" ) == BigInteger( 123 ) );
 CHECK_THROWS( BigInteger( "" ) , contract_violation );
 CHECK_THROWS( BigInteger( "-" ) , contract_violation );
 CHECK_THROWS( BigInteger( "2" , 2 ) , contract_violation );
 CHECK_THROWS( BigInteger( "1" , 1 ) , contract_violation );
 CHECK_THROWS( BigInteger( "1" , 37 ) , contract_violation );
 CHECK_THROWS( BigInteger( 5 ).toString( 1 ) , contract_violation );
 CHECK_THROWS( BigInteger::divMod( 5 , 0 ) , contract_violation );
 CHECK_THROWS( (void) ( BigInteger( 1 ) % BigInteger( 0 ) ) ,
               contract_violation );
 }

// the numbers of the tests of the fast algorithms, built from their digits
// in base 2^32: random, all 0xFFFFFFFF, or with a given top digit
namespace {

/// a number of n digits of 32 bits: random, all 0xFFFFFFFF if ones, the
/// top digit top if not 0
BigInteger digits( std::size_t n , std::mt19937_64 & g , bool ones = false ,
                   std::uint32_t top = 0 )
{
 BigInteger x , B = BigInteger( 1LL << 32 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  std::uint32_t d = ones ? 0xFFFFFFFFu : std::uint32_t( g() );
  if( ( i == 0 ) && ( top != 0 ) ) d = top;
  if( ( i == 0 ) && ( d == 0 ) ) d = 1;
  x = x * B + BigInteger( ( long long )( d ) );
  }
 return( x );
 }

/// a number of n digits of 32 bits drawn from the edges of their range,
/// where the trial quotients are too large and the corrections run
BigInteger edges( std::size_t n , std::mt19937_64 & g )
{
 const long long e[] = { 0 , 1 , 0x7FFFFFFF , 0x80000000LL , 0xFFFFFFFELL ,
                         0xFFFFFFFFLL };
 BigInteger x = 1 , B = BigInteger( 1LL << 32 );
 for( std::size_t i = 1 ; i < n ; ++i ) x = x * B + BigInteger( e[ g() % 6 ] );
 return( x );
 }

}  // end( namespace )

// the products by their methods just below, at and above kKaratsuba and
// kFFT, balanced and not, against Algorithm M (and the audits), the
// squares, which take their own paths, the digits all 0xFFFFFFFF, where
// the carries are longest, a square of 2^17 such digits, where the old
// convolution in doubles rounded wrong, and the zero factor
ADS_TEST( "big-integer-product" )
{
 std::mt19937_64 g( 20 );
 const std::size_t K = BigInteger::kKaratsuba , F = BigInteger::kFFT;
 for( std::size_t n : { K - 1 , K , K + 1 , 2 * K - 1 , 2 * K , 2 * K + 1 ,
                        F - 1 , F , F + 1 } )
  for( bool ones : { false , true } ) {
   const BigInteger a = digits( n , g , ones ) , b = digits( n , g , ones ) ,
                    c = digits( 3 * n + 1 , g , ones ) , s = -a;
   const BigInteger ab = BigInteger::multiplySchool( a , b );
   CHECK( a * b == ab && BigInteger::multiplyKaratsuba( a , b ) == ab &&
          BigInteger::multiplyFFT( a , b ) == ab );
   CHECK( a * c == BigInteger::multiplySchool( a , c ) &&
          c * s == BigInteger::multiplyKaratsuba( c , s ) &&
          BigInteger::multiplyFFT( s , c ) == -( a * c ) );
   const BigInteger aa = BigInteger::multiplySchool( a , a );
   CHECK( a * a == aa && s * s == aa &&
          BigInteger::multiplyKaratsuba( a , a ) == aa &&
          BigInteger::multiplyFFT( s , s ) == aa );
   CHECK( a * BigInteger() == BigInteger() &&
          BigInteger::multiplyFFT( BigInteger() , a ) == BigInteger() );
   }
 for( int t = 0 ; t < 20 ; ++t ) {          // random lengths across both
  const BigInteger a = digits( 1 + g() % ( 2 * F ) , g ) ,
                   b = digits( 1 + g() % ( 2 * F ) , g );
  CHECK( a * b == BigInteger::multiplySchool( a , b ) );
  }
 const std::size_t n = std::size_t( 1 ) << 17;
 const BigInteger p = BigInteger::pow( 2 , 32 * n ) , x = p - 1;
 CHECK( x.limbs() == n && x * x + x + x + 1 == p * p );
 }

// the division by its methods: the recursive one against Algorithm D (and
// the audit) with divisors and quotients just below, at and above kDivide
// and its multiples, the quotients longer than the divisor taken by
// blocks, the divisors with the top digit 1 and 0xFFFFFFFF, the digits at
// the edges of their range, where D6 adds back and the recursion corrects
// its halves, the exact multiples and those plus b - 1, the powers of the
// base, zero and the four signs
ADS_TEST( "big-integer-division" )
{
 std::mt19937_64 g( 21 );
 const std::size_t D = BigInteger::kDivide;
 for( std::size_t n : { std::size_t( 2 ) , std::size_t( 3 ) , D - 1 , D ,
                        D + 1 , 2 * D - 1 , 2 * D , 2 * D + 1 , 3 * D + 5 } )
  for( std::size_t m : { std::size_t( 1 ) , D - 1 , D , D + 1 , 2 * D ,
                         n - 1 , n , n + 1 , 2 * n + 1 , 3 * n + 7 } ) {
   if( m == 0 ) continue;
   for( std::uint32_t top : { 0u , 1u , 0xFFFFFFFFu } ) {
    const BigInteger b = digits( n , g , false , top ) ,
                     a = digits( n + m - 1 , g ) , q0 = digits( m , g );
    const auto [ q , r ] = BigInteger::divMod( a , b );
    CHECK( std::pair( q , r ) == BigInteger::divModSchool( a , b ) );
    CHECK( q * b + r == a && r.sign() >= 0 && r < b );
    const BigInteger e = q0 * b;            // exact, and the largest rest
    CHECK( BigInteger::divMod( e , b ) == std::pair( q0 , BigInteger() ) );
    CHECK( BigInteger::divMod( e + b - 1 , b ) == std::pair( q0 , b - 1 ) );
    CHECK( BigInteger::divMod( -e , b ) == std::pair( -q0 , BigInteger() ) );
    CHECK( BigInteger::divMod( -a , -b ) == std::pair( q , -r ) &&
           BigInteger::divMod( a , -b ) == std::pair( -q , r ) );
    }
   }
 for( std::size_t n : { D - 1 , D + 1 , 2 * D + 3 } )   // quotients all
  for( std::size_t m : { D , 2 * D + 1 , 3 * n } ) {    // 0xFFFFFFFF
   const BigInteger b = digits( n , g , true , 0x80000000u ) ,
                    c = digits( n , g , true ) , q = digits( m , g , true );
   CHECK( BigInteger::divMod( q * b + b - 1 , b ) == std::pair( q , b - 1 ) &&
          BigInteger::divMod( q * c + c - 1 , c ) == std::pair( q , c - 1 ) );
   }
 for( int t = 0 ; t < 200 ; ++t ) {         // the edges, at every length
  const BigInteger b = edges( 2 + g() % ( 3 * D ) , g ) ,
                   a = edges( b.limbs() + g() % ( 4 * D ) , g );
  CHECK( BigInteger::divMod( a , b ) == BigInteger::divModSchool( a , b ) );
  }
 for( std::size_t k : { D , 2 * D + 1 , 5 * D } ) {   // powers of the base
  const BigInteger B = BigInteger::pow( 2 , 32 * k ) ,
                   C = BigInteger::pow( 2 , 32 * ( k / 2 + 1 ) );
  CHECK( BigInteger::divMod( B * B , B ) == std::pair( B , BigInteger() ) );
  CHECK( BigInteger::divMod( B * B - 1 , B - 1 ) ==
         BigInteger::divModSchool( B * B - 1 , B - 1 ) );
  CHECK( BigInteger::divMod( B * B , C ) ==
         std::pair( B * ( B / C ) , BigInteger() ) );
  CHECK( ( B * B - 1 ) / ( B - 1 ) == B + 1 &&
         ( B * B - 1 ) % ( B + 1 ) == BigInteger() );
  }
 const BigInteger b = digits( 3 * D , g );
 CHECK( BigInteger::divMod( 0 , b ) == std::pair( BigInteger() ,
                                                  BigInteger() ) );
 CHECK( BigInteger::divMod( b , b ) == std::pair( BigInteger( 1 ) ,
                                                  BigInteger() ) &&
        BigInteger::divMod( b - 1 , b ) == std::pair( BigInteger() , b - 1 ) );
 CHECK_THROWS( BigInteger::divModSchool( b , 0 ) , contract_violation );
 }

// the conversions: 10^k - 1 and 10^k, and their kin in the bases 2 and 36,
// for k around the lengths where the divide and conquer splits and around
// kConvert digits, as written and back, negative too; random numbers
// across kConvert in many bases against toStringSchool() and
// fromStringSchool() (and the audits), the strings with leading zeros,
// and zero
ADS_TEST( "big-integer-radix" )
{
 std::mt19937_64 g( 22 );
 const std::size_t C = BigInteger::kConvert;
 for( int base : { 2 , 10 , 36 } ) {
  std::size_t k = 1;                        // the figures in one digit
  for( long long p = base ; p * base <= 0xFFFFFFFFLL ; p *= base ) ++k;
  const char top = "0123456789abcdefghijklmnopqrstuvwxyz"[ base - 1 ];
  std::vector< std::size_t > lengths;
  for( std::size_t s = k ; s <= 64 * k * C ; s *= 2 )   // k 2^i
   for( std::size_t d : { s - 1 , s , s + 1 } ) lengths.push_back( d );
  for( std::size_t d : { C - 1 , C , C + 1 } )   // kConvert digits
   lengths.push_back( std::size_t( double( 32 * d ) /
                                   std::log2( double( base ) ) ) );
  for( std::size_t d : lengths ) {
   const BigInteger P = BigInteger::pow( base , d ) , Q = P - 1;
   const std::string sp = "1" + std::string( d , '0' ) ,
                     sq = std::string( d , top );
   CHECK( P.toString( base ) == sp && Q.toString( base ) == sq );
   CHECK( ( -Q ).toString( base ) == "-" + sq );
   CHECK( BigInteger( sp , base ) == P && BigInteger( sq , base ) == Q &&
          BigInteger( "-" + sq , base ) == -Q );
   }
  }
 for( int t = 0 ; t < 40 ; ++t ) {          // random, 8 and 32 first
  const int base = t < 8 ? 1 << ( 3 + 2 * ( t % 2 ) ) : 2 + int( g() % 35 );
  const BigInteger x = ( t % 2 ? -1 : 1 ) * digits( C - 2 + g() % ( 9 * C ) ,
                                                    g );
  const std::string s = x.toString( base );
  CHECK( s == x.toStringSchool( base ) );
  CHECK( BigInteger( s , base ) == x &&
         BigInteger::fromStringSchool( s , base ) == x );
  }
 const BigInteger y = digits( 4 * C , g );   // the leading zeros
 for( int base : { 2 , 10 , 16 , 36 } ) {
  const std::string s = y.toString( base );
  CHECK( BigInteger( std::string( 1000 , '0' ) + s , base ) == y &&
         BigInteger( "-000" + s , base ) == -y &&
         BigInteger( "+0" + s , base ) == y );
  CHECK( BigInteger( std::string( 1000 , '0' ) , base ).sign() == 0 &&
         BigInteger( "-" + std::string( 1000 , '0' ) , base ).sign() == 0 &&
         BigInteger( 0 ).toString( base ) == "0" &&
         BigInteger( 0 ).toStringSchool( base ) == "0" );
  }
 CHECK_THROWS( (void) BigInteger( 5 ).toStringSchool( 1 ) ,
               contract_violation );
 CHECK_THROWS( BigInteger::fromStringSchool( "12" , 2 ) ,
               contract_violation );
 }

// the greatest common divisor of Lehmer against Euclid (and the audit):
// a zero argument, equal and opposite arguments, the coprime neighbours
// of Fibonacci, where every quotient is 1, gcd( F_m , F_n ) = F_gcd(m,n),
// the multiples of a common factor, the lengths so different that a long
// division runs, and the last steps on one and two digits
ADS_TEST( "big-integer-gcd" )
{
 std::mt19937_64 g( 23 );
 const BigInteger x = digits( 50 , g );
 CHECK( BigInteger::gcd( 0 , x ) == x && BigInteger::gcd( -x , 0 ) == x &&
        BigInteger::gcd( 0 , 0 ) == 0 && BigInteger::gcd( x , x ) == x &&
        BigInteger::gcd( x , -x ) == x );
 std::vector< BigInteger > F = { 0 , 1 };
 while( F.size() <= 6000 )
  F.push_back( F[ F.size() - 1 ] + F[ F.size() - 2 ] );
 for( std::size_t k : { 10 , 100 , 1000 , 3000 , 5999 } )
  CHECK( BigInteger::gcd( F[ k ] , F[ k + 1 ] ) == 1 &&
         BigInteger::gcd( F[ k + 1 ] , -F[ k ] ) == 1 );
 CHECK( BigInteger::gcd( F[ 6000 ] , F[ 4500 ] ) == F[ 1500 ] &&
        BigInteger::gcd( F[ 2310 ] , F[ 5005 ] ) == F[ 385 ] );
 for( int t = 0 ; t < 30 ; ++t ) {
  const BigInteger c = digits( 1 + g() % 40 , g ) ,
                   a = c * digits( 1 + g() % 200 , g ) ,
                   b = c * digits( 1 + g() % 200 , g ) ,
                   d = BigInteger::gcd( a , b );
  CHECK( d == BigInteger::gcdEuclid( a , b ) && d % c == 0 );
  }
 for( std::size_t n : { 1 , 2 , 3 } ) {     // a long and a short one
  const BigInteger a = digits( 300 , g ) , b = digits( n , g );
  CHECK( BigInteger::gcd( a , b ) == BigInteger::gcdEuclid( a , b ) &&
         BigInteger::gcd( a * b , b ) == b );
  }
 }

// the rationals: the harmonic number H_30 as written, the sums and the
// products in lowest terms (the audit), and the order; the sums by each
// way of reducing, and H_400 in two orders
ADS_TEST( "rational" )
{
 Rational H;
 for( int i = 1 ; i <= 30 ; ++i ) H = H + Rational( 1 , i );
 CHECK( H.toString() == "9304682830147/2329089562800" );
 CHECK( Rational( 6 , -4 ).toString() == "-3/2" &&
        Rational( 0 , 5 ).toString() == "0" );
 CHECK( Rational( 1 , 3 ) + Rational( 1 , 6 ) == Rational( 1 , 2 ) );
 CHECK( Rational( 2 , 3 ) * Rational( 9 , 4 ) == Rational( 3 , 2 ) );
 CHECK( Rational( 2 , 3 ) / Rational( -4 , 9 ) == Rational( -3 , 2 ) );
 CHECK( Rational( 1 , 3 ) - Rational( 1 , 3 ) == Rational() );
 CHECK( Rational( 1 , 3 ) < Rational( 1 , 2 ) &&
        Rational( -1 , 2 ) < Rational( -1 , 3 ) );
 std::mt19937 g( 19 );
 for( int t = 0 ; t < 500 ; ++t ) {
  const Rational a( int( g() % 201 ) - 100 , 1 + int( g() % 50 ) ) ,
                 b( int( g() % 201 ) - 100 , 1 + int( g() % 50 ) );
  CHECK( ( a + b ) - b == a && ( a - b ) + b == a );
  if( b.num().sign() != 0 ) CHECK( ( a / b ) * b == a );
  }
 constexpr long long lmin = std::numeric_limits< long long >::min();
 const Rational M( lmin , -1 );                  // -lmin past a long long
 CHECK( M.toString() == "9223372036854775808" && M.den() == BigInteger( 1 ) );
 CHECK( Rational( lmin , lmin ) == Rational( 1 ) &&
        Rational( 0 , -7 ).toString() == "0" &&
        Rational( 0 , -7 ) == Rational( 0 , 3 ) );
 CHECK( Rational( -1 , 2 ) < Rational() && Rational() < Rational( 1 , 2 ) );
 // the reductions: d1 = 1, d1 > 1 with d2 = 1 and with d2 > 1, a zero sum;
 // H_400 summed in a row and by halves, with numbers of many digits
 CHECK( Rational( 1 , 4 ) + Rational( 1 , 4 ) == Rational( 1 , 2 ) &&
        Rational( 1 , 6 ) + Rational( 1 , 10 ) == Rational( 4 , 15 ) &&
        Rational( 1 , 6 ) + Rational( 1 , 9 ) == Rational( 5 , 18 ) &&
        Rational( 1 , 6 ) - Rational( 1 , 6 ) == Rational() );
 Rational S;
 std::vector< Rational > h;
 for( int i = 1 ; i <= 400 ; ++i ) {
  S = S + Rational( 1 , i );
  h.push_back( Rational( 1 , i ) );
  }
 while( h.size() > 1 ) {
  std::vector< Rational > n;
  for( std::size_t i = 0 ; i + 1 < h.size() ; i += 2 )
   n.push_back( h[ i ] + h[ i + 1 ] );
  if( h.size() % 2 ) n.push_back( h.back() );
  h = std::move( n );
  }
 CHECK( S == h[ 0 ] && S.den().limbs() > 5 );
 const BigInteger u = BigInteger::pow( 3 , 700 ) * 1001 ,
                  v = BigInteger::pow( 7 , 400 ) * 1001;
 const Rational x( u , v ) , y( v + 1 , u - 1 );
 CHECK( x.den() == BigInteger::pow( 7 , 400 ) && x * ( Rational( 1 ) / x ) ==
        Rational( 1 ) && ( x / y ) * y == x && ( x + y ) - y == x );
 CHECK_THROWS( Rational( 1 , 0 ) , contract_violation );
 CHECK_THROWS( Rational( 1 ) / Rational() , contract_violation );
 CHECK_THROWS( Rational( 0 , 0 ) , contract_violation );
 }

// the exact cover: the n queens as the backtrack of chapter 16 counts
// them, the Sudoku as sudoku() solves it, and random small instances
// against the enumeration of the subsets of options; no option, no item
// (the empty cover), two equal options, a visit that stops, secondary
// items, and the options and sizes not allowed
ADS_TEST( "dancing-links" )
{
 for( int n = 1 ; n <= 8 ; ++n ) {
  ExactCover Q( 2 * n , 2 * ( 2 * n - 1 ) );   // rows, columns; diagonals
  for( int r = 0 ; r < n ; ++r )
   for( int c = 0 ; c < n ; ++c )
    Q.addOption( { r , n + c , 2 * n + r + c ,
                   2 * n + ( 2 * n - 1 ) + r - c + n - 1 } );
  auto none = []( const std::vector< int > & ) { return( false ); };
  CHECK( Q.solve( none ) == nQueens( n , none ) );
  }
 Grid S = { { 5 , 3 , 0 , 0 , 7 , 0 , 0 , 0 , 0 } ,
            { 6 , 0 , 0 , 1 , 9 , 5 , 0 , 0 , 0 } ,
            { 0 , 9 , 8 , 0 , 0 , 0 , 0 , 6 , 0 } ,
            { 8 , 0 , 0 , 0 , 6 , 0 , 0 , 0 , 3 } ,
            { 4 , 0 , 0 , 8 , 0 , 3 , 0 , 0 , 1 } ,
            { 7 , 0 , 0 , 0 , 2 , 0 , 0 , 0 , 6 } ,
            { 0 , 6 , 0 , 0 , 0 , 0 , 2 , 8 , 0 } ,
            { 0 , 0 , 0 , 4 , 1 , 9 , 0 , 0 , 5 } ,
            { 0 , 0 , 0 , 0 , 8 , 0 , 0 , 7 , 9 } };
 ExactCover E( 324 );                       // cell, row, column, box
 std::vector< std::array< int , 3 > > what;
 for( int r = 0 ; r < 9 ; ++r )
  for( int c = 0 ; c < 9 ; ++c )
   for( int d = 1 ; d <= 9 ; ++d ) {
    if( S[ r ][ c ] && ( S[ r ][ c ] != d ) ) continue;
    const int b = 3 * ( r / 3 ) + c / 3;
    E.addOption( { 9 * r + c , 81 + 9 * r + d - 1 , 162 + 9 * c + d - 1 ,
                   243 + 9 * b + d - 1 } );
    what.push_back( { r , c , d } );
    }
 Grid D = S;
 CHECK( E.solve( [ & ]( const std::vector< int > & opts ) {
  for( int o : opts ) D[ what[ o ][ 0 ] ][ what[ o ][ 1 ] ] = what[ o ][ 2 ];
  return( false ); } ) == 1 );
 Grid B = S;
 CHECK( sudoku( B ) && B == D );
 std::mt19937 g( 20 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const int items = 1 + int( g() % 7 ) , sec = int( g() % 3 ) ,
            m = 1 + int( g() % 10 );
  ExactCover X( items , sec );
  std::vector< std::vector< int > > O;
  for( int o = 0 ; o < m ; ++o ) {
   std::vector< int > it;
   for( int i = 0 ; i < items + sec ; ++i )
    if( g() % 3 == 0 ) it.push_back( i );
   if( it.empty() || ( it.front() >= items ) )   // a primary item
    it.insert( it.begin() , int( g() % items ) );
   O.push_back( it );
   X.addOption( it );
   }
  long brute = 0;
  for( unsigned mask = 0 ; mask < ( 1u << m ) ; ++mask ) {
   std::vector< int > cnt( items + sec , 0 );
   for( int o = 0 ; o < m ; ++o )
    if( mask >> o & 1 ) for( int i : O[ o ] ) ++cnt[ i ];
   bool ok = true;
   for( int i = 0 ; i < items + sec ; ++i )
    ok = ok && ( i < items ? cnt[ i ] == 1 : cnt[ i ] <= 1 );
   brute += ok;
   }
  CHECK( X.solve( []( const std::vector< int > & ) { return( false ); } ) ==
         brute );
  }
 auto all = []( const std::vector< int > & ) { return( false ); };
 ExactCover N( 3 );                              // no option: no cover
 CHECK( N.solve( all ) == 0 );
 ExactCover Zero( 0 );                           // no item: the empty cover
 CHECK( Zero.solve( all ) == 1 );
 ExactCover U( 1 );                              // one item, two options
 U.addOption( { 0 } );
 U.addOption( { 0 } );
 CHECK( U.solve( all ) == 2 &&
        U.solve( []( const std::vector< int > & ) { return( true ); } ) ==
        1 );                                     // a visit that stops
 ExactCover S2( 1 , 2 );                         // secondary items, in
 S2.addOption( { 0 , 1 } );                      // some options
 S2.addOption( { 0 , 1 , 2 } );
 CHECK( S2.solve( all ) == 2 );
 ExactCover Z( 2 );
 CHECK_THROWS( Z.addOption( {} ) , contract_violation );
 CHECK_THROWS( Z.addOption( { -1 } ) , contract_violation );
 CHECK_THROWS( ExactCover( -1 ) , contract_violation );
 CHECK_THROWS( Z.addOption( { 0 , 0 } ) , contract_violation );
 CHECK_THROWS( Z.addOption( { 2 } ) , contract_violation );
 ExactCover W( 1 , 1 );
 CHECK_THROWS( W.addOption( { 1 } ) , contract_violation );
 }

// the SAT solvers: random 3-SAT at the threshold against the backtrack of
// Problems.h, the pigeonholes unsatisfiable, the queens satisfiable, and
// the corner cases (no clause, an empty clause, units in conflict, no
// variable, a literal repeated in a clause, the literals 0 and past n)
ADS_TEST( "sat-solvers" )
{
 std::mt19937 g( 21 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 3 + int( g() % 16 ) , m = int( 4.26 * n );
  Cnf F;
  for( int j = 0 ; j < m ; ++j ) {
   Clause c;
   for( int k = 0 ; k < 3 ; ++k )
    c.push_back( ( 1 + int( g() % n ) ) * ( g() % 2 ? 1 : -1 ) );
   F.push_back( c );
   }
  const bool sat = satisfiable( F , n ).has_value();
  const auto d = dpll( F , n );
  Cdcl C( F , n );
  const auto r = C.solve();
  CHECK( d.has_value() == sat );
  CHECK( ( r == Cdcl::Result::SAT ) == sat &&
         ( r == Cdcl::Result::UNSAT ) == ! sat );
  if( sat ) CHECK( satCertificate( F , C.model() ) );
  }
 for( int h = 2 ; h <= 6 ; ++h ) {          // h + 1 pigeons, h holes
  auto x = [ h ]( int p , int k ) { return( p * h + k + 1 ); };
  Cnf P;
  for( int p = 0 ; p <= h ; ++p ) {
   Clause c;
   for( int k = 0 ; k < h ; ++k ) c.push_back( x( p , k ) );
   P.push_back( c );
   }
  for( int k = 0 ; k < h ; ++k )
   for( int p = 0 ; p <= h ; ++p )
    for( int q = p + 1 ; q <= h ; ++q )
     P.push_back( { -x( p , k ) , -x( q , k ) } );
  CHECK( Cdcl( P , ( h + 1 ) * h ).solve() == Cdcl::Result::UNSAT );
  if( h <= 4 ) CHECK( ! dpll( P , ( h + 1 ) * h ) );
  }
 const int n = 8;                           // the queens as SAT
 auto q = []( int r , int c ) { return( r * n + c + 1 ); };
 Cnf Q;
 for( int r = 0 ; r < n ; ++r ) {
  Clause c;
  for( int k = 0 ; k < n ; ++k ) c.push_back( q( r , k ) );
  Q.push_back( c );
  }
 for( int a = 0 ; a < n * n ; ++a )
  for( int b = a + 1 ; b < n * n ; ++b ) {
   const int r1 = a / n , c1 = a % n , r2 = b / n , c2 = b % n;
   if( ( r1 == r2 ) || ( c1 == c2 ) ||
       ( std::abs( r1 - r2 ) == std::abs( c1 - c2 ) ) )
    Q.push_back( { -q( r1 , c1 ) , -q( r2 , c2 ) } );
   }
 Cdcl S( Q , n * n );
 CHECK( S.solve() == Cdcl::Result::SAT && satCertificate( Q , S.model() ) );
 CHECK( dpll( Q , n * n ).has_value() );
 CHECK( Cdcl( Cnf() , 3 ).solve() == Cdcl::Result::SAT );
 CHECK( Cdcl( Cnf( { Clause() } ) , 1 ).solve() == Cdcl::Result::UNSAT );
 CHECK( Cdcl( Cnf( { { 1 } , { -1 } } ) , 1 ).solve() == Cdcl::Result::UNSAT );
 CHECK( ! dpll( Cnf( { { 1 } , { -1 } } ) , 1 ) );
 CHECK( Cdcl( Cnf( { { 1 , -1 } } ) , 1 ).solve() == Cdcl::Result::SAT );
 CHECK( Cdcl( Cnf() , 0 ).solve() == Cdcl::Result::SAT &&
        dpll( Cnf() , 0 ).has_value() );         // no variable
 CHECK( ! dpll( Cnf( { Clause() } ) , 1 ) );
 Cdcl R( Cnf( { { 1 , 1 , -2 } , { 2 } } ) , 2 );   // a repeated literal
 CHECK( R.solve() == Cdcl::Result::SAT &&
        satCertificate( Cnf( { { 1 , 1 , -2 } , { 2 } } ) , R.model() ) );
 CHECK_THROWS( Cdcl( Cnf( { { 3 } } ) , 2 ) , contract_violation );
 CHECK_THROWS( Cdcl( Cnf( { { 0 } } ) , 2 ) , contract_violation );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File taocp.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
