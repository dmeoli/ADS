/*--------------------------------------------------------------------------*/
/*------------------------ File contract_stack.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the contracts [see Contract.h] of the stacks: every operation of
 * the specification of the book, its result as the specification states it,
 * and the violations of its preconditions, which must throw
 * ADS::contract_violation. The tests are compiled at the level 3, so that each
 * operation also checks its postconditions and the invariant of the structure,
 * on random sequences of operations against a reference of the standard
 * library.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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
#include "ADS/sequence/List.h"
#include "ADS/stack/ArrayStack.h"
#include "ADS/stack/Expressions.h"
#include "ADS/stack/MultiStack.h"
#include "ADS/stack/SequenceStack.h"
#include "ADS/stack/StackAlgorithms.h"

#include <cstddef>
#include <random>
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

// STACK on ArrayStack and on SequenceStack: the preconditions of the
// book, size > 0 for top() and pop() and size < capacity for push(),
// and random operations against std::vector
template< class S >
void stack_against_vector( S & P , std::size_t cap , unsigned seed )
{
 std::mt19937 g( seed );
 std::vector< int > R;
 for( int op = 0 ; op < 2000 ; ++op )
  if( ( R.size() < cap ) && ( R.empty() || g() % 2 ) ) {
   P.push( op );
   R.push_back( op );
   }
  else {
   CHECK( P.top() == R.back() && P.pop() == R.back() );
   R.pop_back();
   }
 CHECK( P.isEmpty() == R.empty() );
 }

ADS_TEST( "array-stack" )
{
 ArrayStack< int > P( 2 );
 CHECK_THROWS( (void) P.top() , contract_violation );
 CHECK_THROWS( P.pop() , contract_violation );
 P.push( 1 );
 P.push( 2 );
 CHECK( P.size() == 2 && P.capacity() == 2 );
 CHECK_THROWS( P.push( 3 ) , contract_violation );
 CHECK( P.pop() == 2 && P.pop() == 1 && P.isEmpty() );
 ArrayStack< int > Z( 0 );
 CHECK_THROWS( Z.push( 0 ) , contract_violation );
 ArrayStack< int > Q( 50 );
 stack_against_vector( Q , 50 , 1 );
 }

ADS_TEST( "sequence-stack" )
{
 SequenceStack< List< int > > P;
 CHECK_THROWS( (void) P.top() , contract_violation );
 CHECK_THROWS( P.pop() , contract_violation );
 P.push( 1 );
 CHECK( P.top() == 1 && ! P.isEmpty() );
 SequenceStack< ArraySequence< int > > A;
 stack_against_vector( A , 1000000 , 2 );
 stack_against_vector( P , 1000000 , 3 );
 }

// the axioms of STACK [see ADT.h] on random stacks S and items v:
// isEmpty( create ) = true, isEmpty( push( S , v ) ) = false,
// top( push( S , v ) ) = v, pop( push( S , v ) ) = S, the last one through
// the copy of S and the equality of two stacks; top( create ) and
// pop( create ) are undefined, i.e., violate the precondition
template< class S >
void stack_axioms( S create , unsigned seed )
{
 std::mt19937 g( seed );
 CHECK( create.isEmpty() );
 CHECK_THROWS( (void) create.top() , contract_violation );
 CHECK_THROWS( create.pop() , contract_violation );
 S P = create;
 for( int t = 0 ; t < 200 ; ++t ) {
  const int v = int( g() % 1000 );
  S Q = P;                                      // copies are independent
  Q.push( v );
  CHECK( ! Q.isEmpty() && Q.top() == v );
  CHECK( Q.pop() == v && Q == P );
  S M( std::move( Q ) );                        // moving leaves Q usable
  CHECK( M == P && Q.isEmpty() );
  if( P.isEmpty() || g() % 3 )
   P.push( v );
  else
   P.pop();
  }
 S R = P;
 const S & r = R;
 R = r;                                         // self-assignment
 CHECK( R == P );
 }

ADS_TEST( "stack-axioms" )
{
 stack_axioms( ArrayStack< int >( 300 ) , 6 );
 stack_axioms( SequenceStack< List< int > >() , 7 );
 stack_axioms( SequenceStack< ArraySequence< int > >() , 8 );
 ArrayStack< int > A( 4 ) , B( 9 );
 A.push( 1 );
 B.push( 1 );
 CHECK( A == B );                               // whatever the capacities
 }

// k stacks in one vector: k > 0, the stack i in [ 0 , k ), top() and pop()
// on a stack that is not empty, and the partition of the cells checked at
// every operation against k vectors
ADS_TEST( "multistack" )
{
 CHECK_THROWS( MultiStack< int >( 0 ) , contract_violation );
 MultiStack< int > M( 3 );
 CHECK_THROWS( (void) M.isEmpty( 3 ) , contract_violation );
 CHECK_THROWS( (void) M.size( -1 ) , contract_violation );
 CHECK_THROWS( M.push( 3 , 0 ) , contract_violation );
 CHECK_THROWS( (void) M.top( 0 ) , contract_violation );
 CHECK_THROWS( M.pop( 1 ) , contract_violation );
 std::mt19937 g( 4 );
 std::vector< std::vector< int > > R( 3 );
 for( int op = 0 ; op < 2000 ; ++op ) {
  const int i = int( g() % 3 );
  if( R[ i ].empty() || g() % 2 ) {
   M.push( i , op );
   R[ i ].push_back( op );
   }
  else {
   CHECK( M.pop( i ) == R[ i ].back() );
   R[ i ].pop_back();
   }
  CHECK( M.size( i ) == int( R[ i ].size() ) );
  }
 }

// the sorting of a stack and the removal of the items larger than k,
// each checked by its audit, on both stacks
ADS_TEST( "stack-algorithms" )
{
 std::mt19937 g( 5 );
 for( int t = 0 ; t < 50 ; ++t ) {
  ArrayStack< int > A( 20 );
  SequenceStack< List< int > > B;
  for( int i = int( g() % 20 ) ; i > 0 ; --i ) {
   const int x = int( g() % 10 );
   A.push( x );
   B.push( x );
   }
  sortStack( A );
  removeGreater( B , 4 );
  int prev = -1;
  while( ! A.isEmpty() ) {
   const int x = A.pop();
   CHECK( x >= prev );
   prev = x;
   }
  while( ! B.isEmpty() ) CHECK( B.pop() <= 4 );
  }
 CHECK( isPalindrome( "abba" ) && ! isPalindrome( "ab" ) );
 }

// the expressions: an operator with fewer than two operands, more than one
// value left, a division by 0, a parenthesis in a postfix expression, an
// unknown character, unbalanced parentheses, and a pair of parentheses
// with no operator in a fully parenthesized expression
ADS_TEST( "expressions" )
{
 CHECK_THROWS( evalPostfix( "1 +" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "1 2" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "1 0 /" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "( 1 2 + )" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "1 2 %" ) , contract_violation );
 CHECK_THROWS( toPostfix( "( 1 + 2" ) , contract_violation );
 CHECK_THROWS( toPostfix( "1 + 2 )" ) , contract_violation );
 CHECK_THROWS( toPostfixParenthesized( "( ( 1 ) )" ) , contract_violation );
 CHECK( evalPostfix( "7" ) == 7 && toPostfix( "7" ) == "7" );
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
/*---------------------- End File contract_stack.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
