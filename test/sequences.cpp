/*--------------------------------------------------------------------------*/
/*--------------------------- File sequences.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered paths of the rearrangements of a vector: the
 * Dutch national flag with no branch on the items moved as bytes and the
 * sort of a k-sorted vector by one sift per item, each against the path
 * for the other types and against the standard library, and the natural
 * merge sort, the merge and the sublists of the sequences, which append
 * at a position they keep, on every realization of the SEQUENCE; on
 * random and adversarial inputs; and the vector with room inside itself
 * against std::vector; at the level 3 of the contracts.
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

#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/ListSort.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sequence/SequenceAlgorithms.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sequence/SmallVector.h"
#include "ADS/sequence/TieredVector.h"

#include <algorithm>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <utility>
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

/// the flag of the colours C by the path with no branch (integers) and by
/// the one with swaps (strings): the same order and the same swaps
void flag( const std::vector< int > & C )
{
 std::vector< int > A( C );
 std::vector< std::string > S;
 for( int c : C )
  S.push_back( std::string( 1 , char( '0' + c ) ) );
 std::size_t sa = 0 , ss = 0;
 const auto a = dutchNationalFlag( A , []( int c ) { return( c ); } , &sa );
 const auto s = dutchNationalFlag( S , []( const std::string & x ) {
  return( x[ 0 ] - '0' ); } , &ss );
 CHECK( a == s && sa == ss );
 for( std::size_t h = 0 ; h < A.size() ; ++h )
  CHECK( A[ h ] == S[ h ][ 0 ] - '0' );
 std::vector< int > R( C );
 std::sort( R.begin() , R.end() );
 CHECK( A == R && a.first == std::size_t( std::count( C.begin() , C.end() ,
                                                       0 ) ) );
 }

// the Dutch national flag, with no branch on the integers and by swaps on
// the strings, the same order and the same number of swaps, on: no items,
// one item of each colour, all of one colour, sorted, reversed, the
// colours alternating, and random vectors; a colour out of 0..2 violates
// the contract
ADS_TEST( "dutch-flag-branchless" )
{
 flag( {} );
 for( int c = 0 ; c < 3 ; ++c ) {
  flag( { c } );
  flag( std::vector< int >( 50 , c ) );
  }
 flag( { 0 , 0 , 1 , 1 , 2 , 2 } );
 flag( { 2 , 2 , 1 , 1 , 0 , 0 } );
 std::vector< int > Z;
 for( int i = 0 ; i < 99 ; ++i ) Z.push_back( i % 3 );
 flag( Z );
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 50 ; ++t ) {
  std::vector< int > C( g() % 300 );
  for( auto & c : C ) c = int( g() % 3 );
  flag( C );
  }
 std::vector< int > B{ 0 , 3 , 1 };
 CHECK_THROWS( dutchNationalFlag( B , []( int c ) { return( c ); } ) ,
               contract_violation );
 }

// the sort of a k-sorted vector by one sift per item against std::sort, on
// k = 0, 1, 2, 16 and k >= n, n = 0, 1, 2, 17, 1000, many equal items,
// strings, and the reversed order, k-sorted for k = n - 1 only
ADS_TEST( "k-sorted-sift" )
{
 std::mt19937 g( 9 );
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                        std::size_t( 2 ) , std::size_t( 17 ) ,
                        std::size_t( 1000 ) } )
  for( std::size_t k : { std::size_t( 0 ) , std::size_t( 1 ) ,
                         std::size_t( 2 ) , std::size_t( 16 ) ,
                         std::size_t( 5000 ) } ) {
   for( int few = 0 ; few < 2 ; ++few ) {
    std::vector< int > A( n );
    for( auto & x : A ) x = few ? int( g() % 3 ) : int( g() % 100000 );
    std::sort( A.begin() , A.end() );
    for( std::size_t i = 0 ; i + k + 1 <= n ; i += k + 1 )   // k-sorted
     std::shuffle( A.begin() + std::ptrdiff_t( i ) ,
                   A.begin() + std::ptrdiff_t( i + k + 1 ) , g );
    std::vector< int > R( A );
    std::sort( R.begin() , R.end() );
    sortKSorted( A , k );
    CHECK( A == R );
    }
   }
 std::vector< std::string > S{ "b" , "a" , "d" , "c" , "f" , "e" };
 sortKSorted( S , 1 );
 CHECK( std::is_sorted( S.begin() , S.end() ) );
 std::vector< int > V{ 5 , 4 , 3 , 2 , 1 };
 sortKSorted( V , 4 );
 CHECK( std::is_sorted( V.begin() , V.end() ) );
 std::vector< int > W{ 5 , 4 , 3 , 2 , 1 };
 CHECK_THROWS( sortKSorted( W , 3 ) , contract_violation );
 }

/// the natural merge sort, the merge and a sublist on the sequence S of the
/// items X, against std::sort; the passes are at most ceil( log2 runs )
template< class S >
void merging( const std::vector< int > & X )
{
 S s;
 for( int x : X )
  s.insert( s.next( s.tail() ) , x );
 long passes = -1;
 naturalMergeSort( s , &passes );
 std::vector< int > R( X );
 std::sort( R.begin() , R.end() );
 std::vector< int > Y;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  Y.push_back( s.read( p ) );
 CHECK( Y == R );
 long runs = X.empty() ? 0 : 1;
 for( std::size_t i = 1 ; i < X.size() ; ++i )
  runs += X[ i ] < X[ i - 1 ];
 long expected = 0;
 for( long r = runs ; r > 1 ; r = ( r + 1 ) / 2 ) ++expected;
 CHECK( ( passes >= 0 ) && ( passes <= expected ) );
 const S m = mergeSorted( s , s );           // every item twice
 std::vector< int > Z;
 for( auto p = m.head() ; ! m.finished( p ) ; p = m.next( p ) )
  Z.push_back( m.read( p ) );
 CHECK( Z.size() == 2 * R.size() && std::is_sorted( Z.begin() , Z.end() ) );
 if( ! X.empty() ) {                        // the whole sequence
  const S t = sublist( s , s.head() , s.tail() );
  std::vector< int > W;
  for( auto p = t.head() ; ! t.finished( p ) ; p = t.next( p ) )
   W.push_back( t.read( p ) );
  CHECK( W == R );
  }
 }

template< class S >
void merging_all( void )
{
 merging< S >( {} );
 merging< S >( { 4 } );
 merging< S >( { 1 , 2 , 3 , 4 } );          // one run
 merging< S >( { 4 , 3 , 2 , 1 } );          // n runs
 merging< S >( { 2 , 2 , 2 , 1 , 1 , 1 } );  // equal items
 std::mt19937 g( 3 );
 for( int t = 0 ; t < 10 ; ++t ) {
  std::vector< int > X( g() % 400 );
  for( auto & x : X ) x = int( g() % 50 );
  merging< S >( X );
  }
 }

// the natural merge sort, the merge and the sublists on ArraySequence,
// List, SinglyList and CursorList, which append at the position kept
// after the last item (a scan of the list at each insertion before):
// sorted as by std::sort within the passes of the analysis, on no
// items, one, one run, n runs of one item, equal items and random ones
ADS_TEST( "merge-sort-appender" )
{
 merging_all< ArraySequence< int > >();
 merging_all< List< int > >();
 merging_all< SinglyList< int > >();
 merging_all< CursorList< int > >();
 }

/// an item whose copy throws when the countdown reaches 0
struct Bomb {
 static inline int left = -1;
 int v;
 explicit Bomb( int x ) : v( x ) {}
 Bomb( const Bomb & o ) : v( o.v ) {
  if( ( left >= 0 ) && ( left-- == 0 ) )
   throw std::runtime_error( "copy" );
  }
 Bomb & operator=( const Bomb & ) = default;
 };

/// an item aligned beyond the words
struct alignas( 64 ) Wide {
 int v;
 };

/// random pushes, pops, resizes, copies and moves of a SmallVector of
/// room N against std::vector
template< class T , std::size_t N , class Make >
void small( unsigned seed , int steps , Make && make )
{
 SmallVector< T , N > A;
 std::vector< T > R;
 std::mt19937 g( seed );
 for( int t = 0 ; t < steps ; ++t ) {
  const unsigned c = g() % 24;
  if( ( c < 12 ) || R.empty() ) {
   A.push_back( make( t ) );
   R.push_back( make( t ) );
   }
  else if( c < 16 ) {
   A.pop_back();
   R.pop_back();
   }
  else if( c < 17 ) {
   const std::size_t n = g() % ( 2 * N + 3 );
   A.resize( n , make( -t ) );
   R.resize( n , make( -t ) );
   }
  else if( c < 18 ) {                       // an item of itself
   A.push_back( A[ 0 ] );
   R.push_back( R[ 0 ] );
   }
  else if( c < 19 ) {                       // a copy, and a move back
   SmallVector< T , N > B( A );
   CHECK( B == A );
   A = std::move( B );
   CHECK( B.empty() && B.small() );
   }
  else if( c < 20 ) {                       // a swap with a short one
   SmallVector< T , N > B{ make( 1 ) };
   A.swap( B );
   B.swap( A );
   CHECK( B.size() == 1 );
   }
  else if( c < 21 )
   A.clear() , R.clear();
  CHECK( A.size() == R.size() && A.capacity() >= std::max( A.size() , N ) );
  CHECK( A.small() == ( A.capacity() == N ) );
  }
 CHECK( std::equal( A.begin() , A.end() , R.begin() , R.end() ) );
 }

// the vector with room inside itself against std::vector, for N = 1, 4 and
// 16, on integers and strings; the edge cases: the empty vector violates
// the contracts of front(), back(), pop_back() and of an index, the items
// stay inside up to N and move out at N + 1, an item of the vector pushed
// as it moves out, copies and moves of small and large vectors, a
// self-assignment, items aligned to 64 bytes inside, and a copy that
// throws as the vector moves out, which leaves it as it was
ADS_TEST( "small-vector" )
{
 SmallVector< int , 4 > E;
 CHECK( E.empty() && E.small() && E.capacity() == 4 );
 CHECK_THROWS( (void) E.front() , contract_violation );
 CHECK_THROWS( (void) E.back() , contract_violation );
 CHECK_THROWS( E.pop_back() , contract_violation );
 CHECK_THROWS( (void) E[ 0 ] , contract_violation );
 for( int i = 0 ; i < 4 ; ++i ) E.push_back( i );
 CHECK( E.small() && E.capacity() == 4 );
 E.push_back( E[ 2 ] );                     // out, with an item of its own
 CHECK( ! E.small() && E.capacity() == 8 && E.back() == 2 );
 auto & self = E;
 E = self;
 CHECK( E.size() == 5 && E[ 4 ] == 2 );
 SmallVector< int , 4 > M( std::move( E ) );   // the memory taken
 CHECK( E.empty() && E.small() && M.size() == 5 );

 small< int , 1 >( 1 , 3000 , []( int t ) { return( t ); } );
 small< int , 4 >( 2 , 3000 , []( int t ) { return( t ); } );
 small< std::string , 4 >( 3 , 3000 , []( int t ) {
  return( std::to_string( t ) + std::string( 25 , 'z' ) ); } );
 small< std::string , 16 >( 4 , 3000 , []( int t ) {
  return( std::to_string( t ) ); } );

 SmallVector< Wide , 3 > W;
 for( int i = 0 ; i < 10 ; ++i ) {
  W.push_back( Wide{ i } );
  CHECK( reinterpret_cast< std::uintptr_t >( &W.back() ) % 64 == 0 );
  }
 for( int i = 0 ; i < 10 ; ++i ) CHECK( W[ i ].v == i );

 SmallVector< Bomb , 2 > X;                 // a copy that throws as the
 X.push_back( Bomb( 0 ) );                  // items move out
 X.push_back( Bomb( 1 ) );
 Bomb::left = 1;
 CHECK_THROWS( X.push_back( Bomb( 9 ) ) , std::runtime_error );
 Bomb::left = -1;
 CHECK( X.small() && X.size() == 2 && X[ 0 ].v == 0 && X[ 1 ].v == 1 );
 }

/// listSort() on the items X of a List against std::stable_sort; the
/// positions of the items stay on them
template< class T , class C >
void sorted( const std::vector< T > & X , C less )
{
 List< T > L( X.begin() , X.end() );
 std::vector< typename List< T >::Pos > P;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  P.push_back( p );
 listSort( L , less );
 std::vector< T > R( X );
 std::stable_sort( R.begin() , R.end() , less );
 std::vector< T > Y;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  Y.push_back( L.read( p ) );
 CHECK( Y == R );
 for( std::size_t i = 0 ; i < X.size() ; ++i )   // the cells kept
  CHECK( P[ i ]->value == X[ i ] );
 std::vector< T > B;                         // and linked both ways
 for( auto p = L.tail() ; ! L.finished( p ) ; p = L.prev( p ) )
  B.push_back( L.read( p ) );
 CHECK( std::equal( B.rbegin() , B.rend() , R.begin() , R.end() ) );
 }

/// the patterns of n items made by key( i , g )
template< class T , class C , class Key >
void patterns( std::size_t n , C less , Key key )
{
 std::mt19937 g{ unsigned( n ) };
 std::vector< T > A( n );
 for( std::size_t i = 0 ; i < n ; ++i ) A[ i ] = key( i , g );
 sorted( A , less );                         // random
 std::vector< T > S( A );
 std::stable_sort( S.begin() , S.end() , less );
 sorted( S , less );                         // sorted
 std::reverse( S.begin() , S.end() );
 sorted( S , less );                         // reversed (equal items too)
 }

// the sort of a List by relinking its cells against std::stable_sort, by
// the radix sort on integers (negative ones too) and doubles and by the
// merge of the natural runs on strings and on pairs ordered by their first
// item only, which checks the stability; on no items, one, two, sorted,
// reversed, all equal and random lists; the positions stay on their items
// and the predecessors are relinked
ADS_TEST( "list-sort" )
{
 for( std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
                        std::size_t( 2 ) , std::size_t( 3 ) ,
                        std::size_t( 100 ) , std::size_t( 5000 ) } ) {
  patterns< int >( n , std::less<>() , []( std::size_t , auto & g ) {
   return( int( g() % 2001 ) - 1000 ); } );
  patterns< double >( n , std::less< double >() , []( std::size_t ,
                                                      auto & g ) {
   return( double( int( g() % 200 ) - 100 ) / 8 ); } );
  patterns< std::string >( n , std::less<>() , []( std::size_t ,
                                                  auto & g ) {
   return( std::to_string( g() % 300 ) ); } );
  using Pair = std::pair< int , int >;
  patterns< Pair >( n , []( const Pair & a , const Pair & b ) {
   return( a.first < b.first ); } , []( std::size_t i , auto & g ) {
   return( Pair( int( g() % 7 ) , int( i ) ) ); } );
  patterns< int >( n , std::greater<>() , []( std::size_t , auto & g ) {
   return( int( g() % 50 ) ); } );          // not the natural order
  sorted( std::vector< int >( n , 4 ) , std::less<>() );   // all equal
  }
 }

/// random insertions, removals and reads of a TieredVector against
/// std::vector, the insertions a fraction ins of 10, up to steps of them
template< class T , class Make >
void tiered( unsigned seed , int steps , unsigned ins , Make && make )
{
 TieredVector< T > A;
 std::vector< T > R;
 std::mt19937 g( seed );
 for( int t = 0 ; t < steps ; ++t ) {
  const unsigned c = g() % 10;
  if( R.empty() || ( c < ins ) ) {
   const std::size_t i = g() % ( R.size() + 1 );
   const T x = make( t );
   A.insert( i , x );
   R.insert( R.begin() + std::ptrdiff_t( i ) , x );
   }
  else if( c < 9 ) {
   const std::size_t i = g() % R.size();
   A.erase( i );
   R.erase( R.begin() + std::ptrdiff_t( i ) );
   }
  else {
   const std::size_t i = g() % R.size();
   CHECK( A[ i ] == R[ i ] );
   }
  }
 CHECK( A.size() == R.size() &&
        std::equal( A.begin() , A.end() , R.begin() , R.end() ) );
 }

// the tiered vector against std::vector: random insertions, removals and
// reads that grow it past the sizes where the blocks double (64, 128,
// 256 items) and shrink it back, on integers and strings; the edge cases:
// the empty vector violates the contracts of erase(), pop_back() and of an
// index, insertions at the front and at the end, an item of the vector
// inserted into it, the iterators of the standard interface (std::sort
// and a reverse walk), and clear()
ADS_TEST( "tiered-vector" )
{
 TieredVector< int > E;
 CHECK( E.empty() && E.block() == 64 && E.begin() == E.end() );
 CHECK_THROWS( E.erase( 0 ) , contract_violation );
 CHECK_THROWS( E.pop_back() , contract_violation );
 CHECK_THROWS( (void) E[ 0 ] , contract_violation );
 CHECK_THROWS( E.insert( 1 , 5 ) , contract_violation );
 for( int i = 0 ; i < 1000 ; ++i )           // the front and the end
  if( i % 2 ) E.insert( 0 , -i ); else E.push_back( i );
 CHECK( E.size() == 1000 && E[ 0 ] == -999 && E[ 999 ] == 998 );
 E.insert( 500 , E[ 0 ] );                   // an item of itself
 CHECK( E[ 500 ] == -999 && E.size() == 1001 );
 std::sort( E.begin() , E.end() );
 CHECK( std::is_sorted( E.begin() , E.end() ) );
 int prev = E[ E.size() - 1 ];
 for( auto it = E.end() ; it != E.begin() ; ) {
  --it;
  CHECK( *it <= prev );
  prev = *it;
  }
 E.pop_back();
 CHECK( E.size() == 1000 );
 E.clear();
 CHECK( E.empty() && E.block() == 64 );
 tiered< int >( 1 , 5000 , 5 , []( int t ) { return( t ); } );
 tiered< int >( 2 , 150000 , 7 , []( int t ) { return( t ); } );   // grow
 TieredVector< int > G;                      // past 4 b^2 and back
 for( int i = 0 ; i < 70000 ; ++i ) G.insert( std::size_t( i ) / 2 , i );
 CHECK( G.block() == 256 && G.size() == 70000 );
 while( G.size() > 100 ) G.erase( G.size() / 3 );
 CHECK( G.block() == 64 && G.size() == 100 );
 tiered< std::string >( 3 , 4000 , 6 , []( int t ) {
  return( std::to_string( t ) + std::string( 20 , 'w' ) ); } );
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
/*------------------------- End File sequences.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
