/*--------------------------------------------------------------------------*/
/*----------------------- File contract_sequence.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the sequences: every operation of
 * the specification of the book, its result as the specification states it,
 * and the violations of its preconditions, which must throw
 * ADS::contract_violation. The tests are compiled at the level 3, so that each
 * operation also checks its postconditions and the invariant of the structure,
 * on random sequences of operations against a reference of the standard
 * library.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 3.2 and
 *      4.1, https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/CursorList.h"
#include "ADS/sequence/List.h"
#include "ADS/sequence/Rearrangements.h"
#include "ADS/sequence/SinglyList.h"
#include "ADS/sequence/SequenceAlgorithms.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <random>
#include <set>
#include <utility>
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

// SEQUENCE on List: the operations as the specification states them, the
// preconditions violated, and random insertions and removals against
// std::list, each checking its postcondition and the invariant
ADS_TEST( "list" )
{
 List< int > L;
 CHECK( L.empty() && L.finished( L.head() ) && L.finished( L.tail() ) );
 const auto p = L.insert( L.next( L.tail() ) , 1 );   // insert at pos_n
 CHECK( ! L.empty() && L.head() == p && L.tail() == p && L.read( p ) == 1 );
 const auto q = L.insert( p , 0 );                    // the predecessor of p
 CHECK( L.next( q ) == p && L.prev( p ) == q && L.head() == q );
 const auto r = L.remove( q );                        // the successor of q
 CHECK( r == p && L.head() == p && L.finished( L.prev( p ) ) );
 L.write( p , 7 );
 CHECK( L.read( p ) == 7 );
 const auto end = L.next( L.tail() );                 // the sentinel
 CHECK_THROWS( (void) L.read( end ) , contract_violation );
 CHECK_THROWS( L.write( end , 0 ) , contract_violation );
 CHECK_THROWS( L.remove( end ) , contract_violation );
 CHECK_THROWS( L.insert( nullptr , 0 ) , contract_violation );
 CHECK_THROWS( (void) L.next( nullptr ) , contract_violation );
 std::mt19937 g( 1 );
 List< int > A;
 std::list< int > R;
 for( int op = 0 ; op < 2000 ; ++op ) {
  const int k = R.empty() ? 0 : int( g() % ( R.size() + 1 ) );
  auto pa = A.head();
  auto pr = R.begin();
  for( int i = 0 ; i < k ; ++i , pa = A.next( pa ) , ++pr ) {}
  if( ( g() % 3 ) || R.empty() ) {
   A.insert( pa , op );
   R.insert( pr , op );
   }
  else if( pr != R.end() ) {
   A.remove( pa );
   R.erase( pr );
   }
  }
 std::vector< int > a;
 for( auto x = A.head() ; ! A.finished( x ) ; x = A.next( x ) )
  a.push_back( A.read( x ) );
 CHECK( a == std::vector< int >( R.begin() , R.end() ) );
 }

// SEQUENCE on ArraySequence: the positions from -1 to n, the
// preconditions violated, a moved sequence left empty, and random
// insertions and removals against std::vector
ADS_TEST( "array-sequence" )
{
 ArraySequence< int > S;
 CHECK( S.empty() && S.finished( S.head() ) && S.finished( S.tail() ) );
 CHECK( S.insert( S.next( S.tail() ) , 1 ) == 0 );     // insert at pos_n
 CHECK( S.insert( 0 , 0 ) == 0 && S.read( 1 ) == 1 && S.size() == 2 );
 CHECK( S.remove( 0 ) == 0 && S.read( 0 ) == 1 );
 CHECK_THROWS( (void) S.read( 1 ) , contract_violation );
 CHECK_THROWS( (void) S.read( -1 ) , contract_violation );
 CHECK_THROWS( S.write( 1 , 0 ) , contract_violation );
 CHECK_THROWS( S.remove( 1 ) , contract_violation );
 CHECK_THROWS( S.insert( 2 , 0 ) , contract_violation );
 CHECK_THROWS( S.insert( -1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) S.next( 1 ) , contract_violation );   // past pos_n
 CHECK_THROWS( (void) S.prev( -1 ) , contract_violation );  // before pos_-1
 CHECK( S.next( -1 ) == 0 && S.prev( 1 ) == 0 );
 ArraySequence< int > T( std::move( S ) );
 CHECK( S.empty() && T.size() == 1 && T.read( 0 ) == 1 );
 S.insert( 0 , 5 );                                    // still usable
 CHECK( S.read( 0 ) == 5 );
 std::mt19937 g( 3 );
 ArraySequence< int > A;
 std::vector< int > R;
 for( int op = 0 ; op < 2000 ; ++op ) {
  const long k = long( g() % ( R.size() + 1 ) );
  if( ( g() % 3 ) || R.empty() ) {
   A.insert( k , op );
   R.insert( R.begin() + k , op );
   }
  else if( k < long( R.size() ) ) {
   A.remove( k );
   R.erase( R.begin() + k );
   }
  CHECK( A.size() == long( R.size() ) && A.capacity() >= A.size() );
  }
 for( long i = 0 ; i < A.size() ; ++i ) CHECK( A.read( i ) == R[ i ] );
 }

// copying, moving and assigning a sequence, on List and ArraySequence:
// the copy is deep, i.e., independent of the original; a sequence moved
// from is empty and usable; self-assignment and swap keep the items; a
// position of another list violates the audit of List
template< class S >
void copy_move( void )
{
 S a = { 1 , 2 , 3 };
 S b = a;
 b.write( b.head() , 9 );
 CHECK( a.read( a.head() ) == 1 && !( a == b ) );
 b = a;
 CHECK( a == b );
 const S & r = b;
 b = r;                                         // self-assignment
 CHECK( a == b );
 S c( std::move( b ) );
 CHECK( c == a && b.empty() && b.finished( b.head() ) );
 b.insert( b.head() , 7 );                      // still usable
 CHECK( b.read( b.head() ) == 7 );
 std::swap( b , c );
 CHECK( b == a && c.read( c.head() ) == 7 );
 b = std::move( c );
 CHECK( b.read( b.head() ) == 7 && ! b.empty() );
 }

ADS_TEST( "copy-move" )
{
 copy_move< List< int > >();
 copy_move< ArraySequence< int > >();
 copy_move< SinglyList< int > >();
 copy_move< CursorList< int > >();
 List< int > L = { 1 } , M = { 2 };
 CHECK_THROWS( L.remove( M.head() ) , contract_violation );
 CHECK_THROWS( L.write( M.head() , 0 ) , contract_violation );
 CHECK_THROWS( L.insert( M.head() , 0 ) , contract_violation );
 ArraySequence< int > A;                        // halved at a quarter
 for( int i = 0 ; i < 64 ; ++i ) A.insert( A.size() , i );
 CHECK( A.capacity() == 64 );
 while( A.size() > 3 ) A.remove( A.size() - 1 );
 CHECK( A.capacity() <= 4 * A.size() && A.capacity() >= A.size() );
 while( ! A.empty() ) A.remove( 0 );
 CHECK( A.capacity() <= 1 );
 ArraySequence< int > F = { 1 , 2 };            // full: insert() of its own
 F.insert( 0 , F.read( 1 ) );                   // item reallocates the vector
 CHECK( F == ArraySequence< int >( { 2 , 1 , 2 } ) );
 auto S = std::make_shared< CursorSpace< int > >( 2 );
 CursorList< int > C( S );
 C.insert( C.head() , 5 );
 C.insert( C.head() , 6 );                      // the space is full
 C.insert( C.head() , C.read( C.next( C.head() ) ) );
 CHECK( C.read( C.head() ) == 5 && S->capacity() == 4 );
 }

// the algorithms on sequences: the sorted input of searchOrdered() and of
// mergeSorted() audited, and the sorts, the merge and the removal of the
// duplicates checked by their own audits on random sequences; lowerBound()
// audited, the empty sequences, the sublists of one item, from pos_n and
// with q before p (refused), one item and all items equal
ADS_TEST( "sequence-algorithms" )
{
 List< int > u = { 3 , 1 , 2 } , s = { 1 , 2 , 3 };
 CHECK_THROWS( (void) searchOrdered( u , 2 ) , contract_violation );
 CHECK_THROWS( (void) mergeSorted( u , s ) , contract_violation );
 CHECK( searchOrdered( s , 2 ) == 1 );
 std::mt19937 g( 4 );
 for( int t = 0 ; t < 100 ; ++t ) {
  List< int > a , b;
  ArraySequence< int > c;
  for( int i = int( g() % 20 ) ; i > 0 ; --i ) {
   const int x = int( g() % 10 );
   a.insert( a.next( a.tail() ) , x );
   b.insert( b.next( b.tail() ) , x );
   c.insert( c.next( c.tail() ) , x );
   }
  naturalMergeSort( a );
  bubbleSort( b );
  naturalMergeSort( c );
  CHECK( order( a ) != Order::NONE && order( c ) != Order::NONE );
  const auto m = mergeSorted( a , b );
  removeDuplicates( b );
  CHECK( order( m ) != Order::NONE );
  CHECK( order( b ) == Order::EMPTY || order( b ) == Order::CONSTANT ||
         order( b ) == Order::INCREASING );
  }
 // lowerBound() audited too; the empty sequences, a sublist of one item,
 // from pos_n, and one whose q precedes p, which runs off the end
 CHECK_THROWS( (void) lowerBound( u , 2 ) , contract_violation );
 const List< int > e;
 CHECK( search( e , 1 ) < 0 && searchOrdered( e , 1 ) < 0 &&
        ! lowerBound( e , 1 ).second && order( e ) == Order::EMPTY );
 CHECK( mergeSorted( e , s ) == s && mergeSorted( s , e ) == s &&
        mergeSorted( e , e ).empty() );
 const auto h = s.head() , t = s.tail();
 CHECK( sublist( s , h , h ) == List< int >( { 1 } ) );
 CHECK( sublist( s , h , t ) == s );
 CHECK( sublist( s , s.next( t ) , h ).empty() );     // from pos_n
 CHECK_THROWS( (void) sublist( s , t , h ) , contract_violation );
 List< int > one = { 4 } , same = { 2 , 2 , 2 };
 naturalMergeSort( one );
 bubbleSort( same );
 removeDuplicates( same );
 CHECK( one == List< int >( { 4 } ) && same == List< int >( { 2 } ) );
 }

// the other realizations and the rearrangements: SinglyList and
// CursorList at pos_-1 and pos_n, the space of the cursors (dim >= 1, a
// cell in [ 1 , capacity ], a cell given back twice caught by the
// invariant), the indices of ArraySequence; the Dutch flag with a colour
// -1 and 3, the empty vector, one colour only; wiggleSort() and
// sortKSorted() on empty, one item, equal items, k = 0 and k >= n, a
// vector not k-sorted refused; minimumSwapSort() refusing repeated items
ADS_TEST( "other-sequences" )
{
 SinglyList< int > L;
 const auto before = L.prev( L.head() );        // pos_-1
 CHECK( L.finished( before ) && L.finished( L.head() ) );
 CHECK_THROWS( L.insert( before , 0 ) , contract_violation );
 CHECK_THROWS( (void) L.next( L.head() ) , contract_violation );  // pos_n
 CHECK_THROWS( (void) L.read( L.head() ) , contract_violation );
 CHECK_THROWS( L.remove( L.head() ) , contract_violation );
 CHECK_THROWS( L.write( L.head() , 1 ) , contract_violation );
 L.insert( L.head() , 1 );
 const auto end = L.next( L.head() );
 CHECK( L.read( L.head() ) == 1 && L.finished( end ) );
 CHECK_THROWS( (void) L.read( end ) , contract_violation );
 CHECK( L.next( before ) == L.head() );         // from pos_-1 to pos_0
 L.insert( end , 2 );                           // at pos_n: the back
 CHECK( L.read( L.tail() ) == 2 && L.remove( L.head() ) == L.head() );

 CHECK_THROWS( CursorSpace< int >( 0 ) , contract_violation );
 CHECK_THROWS( CursorSpace< int >( -1 ) , contract_violation );
 CHECK_THROWS( CursorList< int >( nullptr ) , contract_violation );
 auto S = std::make_shared< CursorSpace< int > >( 1 );   // dim = 1
 CHECK( S->capacity() == 1 );
 CHECK_THROWS( (void) ( *S )[ 0 ] , contract_violation );
 CHECK_THROWS( (void) ( *S )[ 2 ] , contract_violation );
 CHECK_THROWS( S->give( 1 ) , contract_violation );      // none in use
 CHECK_THROWS( S->give( 0 ) , contract_violation );
 const long a = S->take();
 (void) ( *S )[ a ];
 const long b = S->take();                      // the space grows
 S->give( a );
 CHECK_THROWS( S->give( a ) , contract_violation );       // twice
 (void) b;
 CursorList< int > C;
 CHECK_THROWS( C.insert( -1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) C.next( C.head() ) , contract_violation );
 CHECK_THROWS( (void) C.read( C.head() ) , contract_violation );
 CHECK_THROWS( C.remove( C.head() ) , contract_violation );
 CHECK_THROWS( C.write( C.head() , 0 ) , contract_violation );
 C.insert( C.head() , 3 );
 CHECK( C.read( C.head() ) == 3 && C.next( -1 ) == C.head() &&
        C.prev( C.head() ) == -1 );

 ArraySequence< int > A = { 5 , 6 };
 CHECK( A[ 0 ] == 5 && A[ 1 ] == 6 );
 CHECK_THROWS( (void) A[ -1 ] , contract_violation );
 CHECK_THROWS( (void) A[ 2 ] , contract_violation );
 const ArraySequence< int > & cA = A;
 CHECK_THROWS( (void) cA[ 2 ] , contract_violation );
 CHECK( A.next( 1 ) == 2 && A.prev( 2 ) == 1 && A.prev( 0 ) == -1 );
 CHECK_THROWS( (void) A.prev( 3 ) , contract_violation );
 CHECK_THROWS( (void) A.next( -2 ) , contract_violation );
 List< int > N;
 CHECK_THROWS( (void) N.prev( nullptr ) , contract_violation );
 CHECK_THROWS( (void) N.read( nullptr ) , contract_violation );
 CHECK_THROWS( N.write( nullptr , 0 ) , contract_violation );
 CHECK_THROWS( N.remove( nullptr ) , contract_violation );

 // the rearrangements
 std::vector< int > F = { 2 , 0 , 1 , 2 , 0 };
 const auto id = []( int x ) { return( x ); };
 CHECK( dutchNationalFlag( F , id ) == std::pair( std::size_t( 2 ) ,
                                                  std::size_t( 3 ) ) );
 std::vector< int > F3 = { 0 , 3 } , Fm = { -1 } , F0 , F1( 4 , 1 );
 CHECK_THROWS( dutchNationalFlag( F3 , id ) , contract_violation );
 CHECK_THROWS( dutchNationalFlag( Fm , id ) , contract_violation );
 CHECK( dutchNationalFlag( F0 , id ) == std::pair( std::size_t( 0 ) ,
                                                   std::size_t( 0 ) ) );
 CHECK( dutchNationalFlag( F1 , id ) == std::pair( std::size_t( 0 ) ,
                                                   std::size_t( 4 ) ) );
 for( std::vector< int > W : { std::vector< int >() , std::vector< int >{ 1 } ,
                               std::vector< int >( 5 , 3 ) ,
                               std::vector< int >{ 1 , 2 , 3 , 4 , 5 } ,
                               std::vector< int >{ 5 , 4 , 3 , 2 , 1 } } ) {
  auto X = W;
  wiggleSort( X );                              // its audit checks it
  auto Y = W;
  std::sort( Y.begin() , Y.end() );
  auto Z = Y;
  sortKSorted( Z , 0 );                         // sorted: 0-sorted
  CHECK( Z == Y );
  auto K = W;
  sortKSorted( K , W.size() );                  // k >= n: anything
  CHECK( K == Y );
  }
 std::vector< int > far = { 3 , 1 , 2 };
 CHECK_THROWS( sortKSorted( far , 1 ) , contract_violation );
 sortKSorted( far , 2 );
 CHECK( far == std::vector< int >( { 1 , 2 , 3 } ) );
 CHECK_THROWS( (void) minimumSwapSort( std::vector< int >{ 1 , 1 } ) ,
               contract_violation );
 CHECK( minimumSwapSort( std::vector< int >() ).first == 0 &&
        minimumSwapSort( std::vector< int >{ 7 } ).first == 0 );
 CHECK( minimumSwapSort( std::vector< int >{ 2 , 1 } ) ==
        std::pair( std::size_t( 1 ) , 3 ) );
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
/*--------------------- End File contract_sequence.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
