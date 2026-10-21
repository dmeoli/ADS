/*--------------------------------------------------------------------------*/
/*--------------------- File engineered_combinatorial.cpp ------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered combinatorial structures, beyond the book: the
 * operations on arrays of words, the generation of permutations and
 * combinations, the exact cover on sparse sets, and the algorithms of the
 * book on the structures of std, Boost and ADS. Each one is checked
 * against the algorithm of the book or a reference of the standard
 * library, on the limits (empty, one item, the largest sizes) and on
 * random and adversarial inputs, at the level 3 of the contracts.
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
#include "ADS/backtrack/FastExactCover.h"
#include "ADS/backtrack/NQueens.h"

#include "ADS/combinatorial/Bitwise.h"
#include "ADS/combinatorial/FastBitwise.h"
#include "ADS/combinatorial/FastGeneration.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/backtrack/SatSolvers.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/intractable/FifteenPuzzle.h"
#include "ADS/intractable/Pseudopolynomial.h"
#include "ADS/priority/DaryHeap.h"
#include "ADS/succinct/BitVector.h"

#if __has_include( <boost/heap/d_ary_heap.hpp> )
 #include <boost/dynamic_bitset.hpp>
 #include <boost/heap/d_ary_heap.hpp>
 #include <boost/heap/pairing_heap.hpp>
 #include <boost/unordered/unordered_flat_map.hpp>
 #define ADS_TEST_BOOST 1
#endif

#include <algorithm>
#include <bit>
#include <cstdint>
#include <random>
#include <set>
#include <functional>
#include <string>
#include <unordered_map>
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

// the counts of the arrays against std::popcount at every length up to
// past two blocks of Harley-Seal and at every misalignment, all ones and
// all zeros; pdep, pext and select against loops over the bits, at the
// words 0, all ones and the top bit; the Morton codes against those of
// Bitwise.h
ADS_TEST( "fast-bitwise" )
{
 std::mt19937_64 g( 7 );
 std::vector< Word > A( 700 ) , B( 700 );
 for( auto & w : A ) w = g();
 for( auto & w : B ) w = g();
 for( std::size_t off = 0 ; off < 3 ; ++off )
  for( std::size_t n = 0 ; n + off <= A.size() ; n += ( n < 300 ? 1 : 37 ) ) {
   std::uint64_t p = 0 , q = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    p += std::uint64_t( std::popcount( A[ off + i ] ) );
    q += std::uint64_t( std::popcount( A[ off + i ] & B[ off + i ] ) );
    }
   CHECK( popcount( A.data() + off , n ) == p );
   CHECK( popcountAnd( A.data() + off , B.data() + off , n ) == q );
   }
 std::vector< Word > ones( 1000 , ~Word( 0 ) ) , zeros( 1000 , 0 );
 CHECK( popcount( ones.data() , ones.size() ) == 64000 );
 CHECK( popcount( zeros.data() , zeros.size() ) == 0 );
 CHECK( popcountAnd( ones.data() , zeros.data() , 1000 ) == 0 );
 CHECK( popcount( nullptr , 0 ) == 0 );

 auto slowDep = []( Word x , Word m ) {
  Word r = 0;
  for( int i = 0 , k = 0 ; i < 64 ; ++i )
   if( ( m >> i ) & 1 ) r |= ( ( x >> k++ ) & 1 ) << i;
  return( r );
  };
 auto slowExt = []( Word x , Word m ) {
  Word r = 0;
  for( int i = 0 , k = 0 ; i < 64 ; ++i )
   if( ( m >> i ) & 1 ) r |= ( ( x >> i ) & 1 ) << k++;
  return( r );
  };
 std::vector< Word > W = { 0 , 1 , ~Word( 0 ) , Word( 1 ) << 63 ,
                           0x8000000000000001ULL };
 for( int i = 0 ; i < 2000 ; ++i ) W.push_back( g() & g() );
 for( Word x : W )
  for( Word m : { W[ 2 ] , W[ 3 ] , g() , g() & g() , Word( 0 ) } ) {
   CHECK( pdep( x , m ) == slowDep( x , m ) );
   CHECK( pext( x , m ) == slowExt( x , m ) );
   }
 for( Word x : W )
  for( int r = 0 , p = 0 ; r < std::popcount( x ) ; ++r , ++p ) {
   while( ! ( ( x >> p ) & 1 ) ) ++p;
   CHECK( select( x , r ) == p );
   }
 CHECK_THROWS( (void) select( 0 , 0 ) , contract_violation );
 CHECK_THROWS( (void) select( 6 , 2 ) , contract_violation );
 for( int i = 0 ; i < 1000 ; ++i ) {
  const auto x = std::uint32_t( g() ) , y = std::uint32_t( g() );
  CHECK( mortonEncode( x , y ) == interleave( x , y ) );
  CHECK( mortonDecode( mortonEncode( x , y ) ) == std::pair( x , y ) );
  }
 CHECK( mortonEncode( ~0u , ~0u ) == ~Word( 0 ) );
 }

// the permutations of Heap's method: n! distinct ones for n = 0 to 8,
// items with duplicates too, and a stop at each place; the lexicographic
// ones against std::next_permutation on multisets with 0, 1, 2 and 3
// equal items among the last three, and the stop
ADS_TEST( "fast-generation" )
{
 long f = 1;
 for( int n = 0 ; n <= 8 ; f *= ++n ) {
  std::vector< int > a( n );
  for( int i = 0 ; i < n ; ++i ) a[ i ] = i;
  std::set< std::vector< int > > seen;
  CHECK( forEachPermutation( a , [ & ]( const std::vector< int > & p ) {
   seen.insert( p ); } ) == f );
  CHECK( long( seen.size() ) == f );
  for( long s = 1 ; s <= std::min( f , 30L ) ; ++s ) {
   long k = 0;
   CHECK( forEachPermutation( a , [ & ]( const std::vector< int > & ) {
    return( ++k == s ); } ) == s );
   }
  }
 std::vector< char > dup = { 'a' , 'a' , 'b' , 'b' };
 CHECK( forEachPermutation( dup , []( const std::vector< char > & ) {} )
        == 24 );

 std::mt19937 g( 3 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = int( g() % 9 );
  std::vector< int > a( n );
  for( auto & x : a ) x = int( g() % ( 1 + g() % 5 ) );
  std::vector< std::vector< int > > want , got;
  auto b = a;
  std::sort( b.begin() , b.end() );
  do want.push_back( b );
  while( std::next_permutation( b.begin() , b.end() ) );
  CHECK( forEachPermutationLex( a , [ & ]( const std::vector< int > & p ) {
   got.push_back( p ); } ) == long( want.size() ) );
  CHECK( got == want );
  CHECK( std::is_sorted( a.begin() , a.end() ) );
  const long s = 1 + long( g() % want.size() );
  long k = 0;
  CHECK( forEachPermutationLex( a , [ & ]( const std::vector< int > & ) {
   return( ++k == s ); } ) == s );
  }

 for( int n = 0 ; n <= 12 ; ++n )
  for( int t = 0 ; t <= n ; ++t ) {
   std::vector< std::vector< int > > want , got;
   combinationsLex( n , t , [ & ]( const Tuple & c ) {
    want.push_back( c ); return( false ); } );
   CHECK( forEachCombination( n , t , [ & ]( const std::vector< int > & c ) {
    got.push_back( c ); } ) == long( want.size() ) );
   CHECK( got == want );
   std::vector< Word > m;
   CHECK( forEachCombinationMask( n , t , [ & ]( Word x ) {
    m.push_back( x ); } ) == long( want.size() ) );
   std::vector< Word > w;
   for( const auto & c : want ) {
    Word x = 0;
    for( int v : c ) x |= Word( 1 ) << v;
    w.push_back( x );
    }
   std::sort( w.begin() , w.end() );        // the words: increasing order
   CHECK( m == w );
   long k = 0;
   const long s = 1 + long( want.size() ) / 2;
   CHECK( forEachCombination( n , t , [ & ]( const std::vector< int > & ) {
    return( ++k == s ); } ) == s );
   }
 CHECK( forEachCombinationMask( 64 , 64 , []( Word x ) {
  return( x != ~Word( 0 ) ); } ) == 1 );
 CHECK( forEachCombinationMask( 64 , 1 , []( Word ) {} ) == 64 );
 CHECK( forEachCombinationMask( 64 , 63 , []( Word ) {} ) == 64 );
 CHECK( nextCombination( 0b10110 ) == 0b11001 );
 for( Word x = 1 ; x < 5000 ; ++x )
  CHECK( nextCombination( x ) == nextSameWeight( x ) );
 CHECK_THROWS( (void) nextCombination( ~Word( 0 ) ) , contract_violation );
 CHECK_THROWS( forEachCombination( 3 , 4 , []( const std::vector< int > & )
               {} ) , contract_violation );
 }


// the exact cover against ExactCover of the book, the same solutions, on
// random instances with secondary items, on the queens
// and on an instance with no item; the colors against an enumeration of
// all the sets of options on random instances; the stop, and the links
// back as they were (a second solve() finds the same)
ADS_TEST( "fast-exact-cover" )
{
 std::mt19937 g( 11 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int p = 1 + int( g() % 8 ) , s = int( g() % 4 ) ,
            m = int( g() % 25 );
  ExactCover A( p , s );
  FastExactCover B( p , s );
  for( int o = 0 ; o < m ; ++o ) {
   std::vector< int > it;
   for( int i = 0 ; i < p + s ; ++i )
    if( g() % 3 == 0 ) it.push_back( i );
   if( std::none_of( it.begin() , it.end() , [ & ]( int i ) {
    return( i < p ); } ) ) it.push_back( int( g() % p ) );
   std::shuffle( it.begin() , it.end() , g );
   A.addOption( it );
   B.addOption( it );
   }
  std::vector< std::vector< int > > a , b , c;
  auto sorted = []( std::vector< int > S ) {
   std::sort( S.begin() , S.end() );
   return( S ); };
  A.solve( [ & ]( const std::vector< int > & S ) {
   a.push_back( sorted( S ) ); return( false ); } );
  B.solve( [ & ]( const std::vector< int > & S ) {
   b.push_back( sorted( S ) ); } );
  B.solve( [ & ]( const std::vector< int > & S ) {
   c.push_back( sorted( S ) ); } );
  std::sort( a.begin() , a.end() );
  CHECK( b.size() == a.size() );
  CHECK( b == c );
  std::sort( b.begin() , b.end() );
  CHECK( a == b );
  if( ! b.empty() ) {
   long k = 0;
   CHECK( B.solve( [ & ]( const std::vector< int > & ) {
    return( ++k == 1 ); } ) == 1 );
   std::vector< std::vector< int > > d;
   B.solve( [ & ]( const std::vector< int > & S ) {
    d.push_back( sorted( S ) ); } );
   std::sort( d.begin() , d.end() );
   CHECK( d == b );
   }
  }
 for( int n = 1 ; n <= 8 ; ++n ) {          // rows, columns; diagonals
  FastExactCover Q( 2 * n , 4 * n - 2 );
  for( int r = 0 ; r < n ; ++r )
   for( int c = 0 ; c < n ; ++c )
    Q.addOption( { r , n + c , 2 * n + r + c , 5 * n - 1 + r - c - 1 } );
  CHECK( Q.solve( []( const std::vector< int > & ) {} ) ==
         nQueens( n , []( const std::vector< int > & ) {
          return( false ); } ) );
  }
 FastExactCover E( 0 , 2 );
 CHECK( E.solve( []( const std::vector< int > & S ) {
  return( ! S.empty() ); } ) == 1 );

 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int p = 1 + int( g() % 4 ) , s = 1 + int( g() % 3 ) ,
            m = int( g() % 12 );
  FastExactCover X( p , s );
  std::vector< std::vector< std::pair< int , int > > > opt;
  for( int o = 0 ; o < m ; ++o ) {
   std::vector< int > it , co;
   for( int i = 0 ; i < p + s ; ++i )
    if( ( g() % 2 == 0 ) || ( i == int( g() % p ) ) ) {
     it.push_back( i );
     co.push_back( i >= p ? int( g() % 3 ) : 0 );
     }
   if( std::none_of( it.begin() , it.end() , [ & ]( int i ) {
    return( i < p ); } ) ) {
    it.push_back( 0 );
    co.push_back( 0 );
    }
   X.addOption( it , co );
   opt.emplace_back();
   for( std::size_t k = 0 ; k < it.size() ; ++k )
    opt.back().emplace_back( it[ k ] , co[ k ] );
   }
  long brute = 0;                           // every set of options
  for( long S = 0 ; S < ( 1L << m ) ; ++S ) {
   std::vector< int > cnt( p + s , 0 ) , col( p + s , -1 );
   bool ok = true;
   for( int o = 0 ; ( o < m ) && ok ; ++o )
    if( ( S >> o ) & 1 )
     for( auto [ i , c ] : opt[ o ] ) {
      if( ( i < p ) || ( c == 0 ) ) {
       ok = ok && ( ++cnt[ i ] == 1 ) && ( col[ i ] < 0 );
       col[ i ] = 0;
       }
      else {
       ok = ok && ( ( col[ i ] < 0 ) || ( col[ i ] == c ) );
       col[ i ] = c;
       }
      }
   for( int i = 0 ; i < p ; ++i ) ok = ok && ( cnt[ i ] == 1 );
   brute += ok;
   }
  CHECK( X.solve( []( const std::vector< int > & ) {} ) == brute );
  }
 CHECK_THROWS( E.addOption( { 0 } ) , contract_violation );
 FastExactCover P( 2 , 1 );
 CHECK_THROWS( P.addOption( { 0 , 0 } ) , contract_violation );
 CHECK_THROWS( P.addOption( { 0 , 2 } , { 1 , 0 } ) , contract_violation );
 CHECK_THROWS( P.addOption( { 2 } ) , contract_violation );
 }
/// the hash of a configuration, its bytes
struct BoardHash {
 using is_avalanching = void;
 std::size_t operator()( const Board & b ) const {
  return( std::size_t( hashBytes( b.data() , b.size() * sizeof( int ) ) ) );
  }
 };

// the algorithms of the book on the structures of std, Boost and ADS: the
// clause learning on the heaps of std, of Boost and of ADS, with the same
// decisions and conflicts; the A* of the fifteen puzzle on std::map,
// std::unordered_map, Boost and FlatHashTable, with the same nodes; the
// subset sum on std::vector< bool >, boost::dynamic_bitset and BitVector
ADS_TEST( "book-structures" )
{
 using Pair = std::pair< double , int >;
 std::mt19937 g( 41 );
 for( int trial = 0 ; trial < 30 ; ++trial ) {
  const int n = 20 + int( g() % 60 );
  Cnf F;
  for( int i = 0 ; i < int( 4.2 * n ) ; ++i ) {
   Clause c;
   for( int j = 0 ; j < 3 ; ++j )
    c.push_back( ( 1 + int( g() % n ) ) * ( g() % 2 ? 1 : -1 ) );
   F.push_back( c );
   }
  Cdcl A( F , n );
  BasicCdcl< DaryHeap< Pair , std::greater< Pair > , 4 > > B( F , n );
  const auto a = A.solve();
  CHECK( int( B.solve() ) == int( a ) );
  CHECK( ( A.decisions() == B.decisions() ) &&
         ( A.conflicts() == B.conflicts() ) );
  if( a == Cdcl::Result::SAT ) CHECK( A.model() == B.model() );
#ifdef ADS_TEST_BOOST
  BasicCdcl< boost::heap::d_ary_heap< Pair , boost::heap::arity< 4 > > > C(
                                                                     F , n );
  BasicCdcl< boost::heap::pairing_heap< Pair > > D( F , n );
  CHECK( int( C.solve() ) == int( a ) );
  CHECK( int( D.solve() ) == int( a ) );
  CHECK( ( C.decisions() == A.decisions() ) &&
         ( D.conflicts() == A.conflicts() ) );
#endif
  }
 const Board goal = puzzleGoal( 3 );
 for( int trial = 0 ; trial < 20 ; ++trial ) {
  Board B = goal;
  for( int i = 0 ; i < 25 ; ++i ) {
   const auto M = puzzleMoves( B , 3 );
   B = M[ g() % M.size() ];
   }
  long v1 = 0 , v2 = 0 , v3 = 0;
  const auto s1 = solvePuzzle( B , 3 , &v1 );
  const auto s2 = solvePuzzle< std::unordered_map< Board , int , BoardHash > >(
                   B , 3 , &v2 );
  const auto s3 = solvePuzzle< FlatHashTable< Board , int , BoardHash > >(
                   B , 3 , &v3 );
  CHECK( s1 && ( s1 == s2 ) && ( s1 == s3 ) && ( v1 == v2 ) && ( v1 == v3 ) );
#ifdef ADS_TEST_BOOST
  long v4 = 0;
  const auto s4 = solvePuzzle< boost::unordered_flat_map< Board , int ,
                                                           BoardHash > >(
                   B , 3 , &v4 );
  CHECK( ( s4 == s1 ) && ( v4 == v1 ) );
#endif
  }
 for( int trial = 0 ; trial < 100 ; ++trial ) {
  const int n = int( g() % 20 );
  std::vector< long > A( n );
  for( auto & a : A ) a = long( g() % 200 );
  const long k = long( g() % 1500 );
  const bool want = subsetSumDP( A , k );
  CHECK( subsetSumDP< BitVector >( A , k ) == want );
#ifdef ADS_TEST_BOOST
  CHECK( subsetSumDP< boost::dynamic_bitset<> >( A , k ) == want );
#endif
  }
 std::vector< std::vector< bool > > r1 , r2;
 subsetSumDP( { 3 , 5 , 2 } , 9 , &r1 );
 subsetSumDP< BitVector >( { 3 , 5 , 2 } , 9 , &r2 );
 CHECK( r1 == r2 );
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
/*------------------- End File engineered_combinatorial.cpp ----------------*/
/*--------------------------------------------------------------------------*/
