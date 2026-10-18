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

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <random>
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

// STACK on ArrayStack and on SequenceStack: the preconditions of the
// book, size > 0 for top() and pop() and size < capacity for push(), on
// the capacities 0 , 1 , 2, a stack emptied again, a NaN pushed (a
// regression: the postcondition compared it with itself), and random
// operations against std::vector
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
 CHECK_THROWS( (void) Z.top() , contract_violation );
 CHECK_THROWS( Z.pop() , contract_violation );
 ArrayStack< int > O( 1 );                     // full and empty in turn
 for( int i = 0 ; i < 3 ; ++i ) {
  O.push( i );
  CHECK_THROWS( O.push( i ) , contract_violation );
  CHECK( O.top() == i && O.size() == 1 && O.pop() == i );
  CHECK_THROWS( O.pop() , contract_violation );
  }
 // a NaN pushed: the postcondition of push() compares it with the top,
 // and must not refuse it (a regression)
 ArrayStack< double > N( 2 );
 N.push( std::nan( "" ) );
 N.push( -std::numeric_limits< double >::infinity() );
 CHECK( std::isinf( N.pop() ) && std::isnan( N.pop() ) );
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
 CHECK( P.pop() == 1 && P.isEmpty() );          // emptied again
 CHECK_THROWS( P.pop() , contract_violation );
 SequenceStack< List< double > > N;            // a NaN (a regression)
 N.push( std::nan( "" ) );
 CHECK( std::isnan( N.top() ) && std::isnan( N.pop() ) );
 SequenceStack< ArraySequence< double > > NA;
 NA.push( std::nan( "" ) );
 CHECK( std::isnan( NA.pop() ) );
 SequenceStack< ArraySequence< int > > A;
 CHECK_THROWS( (void) A.top() , contract_violation );
 CHECK_THROWS( A.pop() , contract_violation );
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
// on a stack that is not empty, each just inside ( 0 , k - 1 , k = 1 ) and
// just outside ( -1 , k , k = 0 , -1 ), a freed cell reused, and the
// partition of the cells checked at every operation against k vectors
ADS_TEST( "multistack" )
{
 CHECK_THROWS( MultiStack< int >( 0 ) , contract_violation );
 MultiStack< int > M( 3 );
 CHECK_THROWS( (void) M.isEmpty( 3 ) , contract_violation );
 CHECK_THROWS( (void) M.size( -1 ) , contract_violation );
 CHECK_THROWS( M.push( 3 , 0 ) , contract_violation );
 CHECK_THROWS( (void) M.top( 0 ) , contract_violation );
 CHECK_THROWS( M.pop( 1 ) , contract_violation );
 CHECK_THROWS( MultiStack< int >( -1 ) , contract_violation );
 CHECK( MultiStack< int >( 1 ).stacks() == 1 );   // k = 1 just inside
 // each i just inside ( 0 , k - 1 ) and just outside ( -1 , k )
 CHECK( M.isEmpty( 0 ) && M.isEmpty( 2 ) && M.size( 0 ) == 0 &&
        M.size( 2 ) == 0 && M.stack( 0 ).empty() && M.stack( 2 ).empty() );
 CHECK_THROWS( (void) M.isEmpty( -1 ) , contract_violation );
 CHECK_THROWS( (void) M.size( 3 ) , contract_violation );
 CHECK_THROWS( M.push( -1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) M.top( -1 ) , contract_violation );
 CHECK_THROWS( (void) M.top( 3 ) , contract_violation );
 CHECK_THROWS( M.pop( -1 ) , contract_violation );
 CHECK_THROWS( M.pop( 3 ) , contract_violation );
 CHECK_THROWS( (void) M.stack( -1 ) , contract_violation );
 CHECK_THROWS( (void) M.stack( 3 ) , contract_violation );
 M.push( 2 , 7 );                               // the last stack
 CHECK( M.top( 2 ) == 7 && M.pop( 2 ) == 7 && M.isEmpty( 2 ) );
 CHECK_THROWS( M.pop( 2 ) , contract_violation );   // emptied again
 M.push( 0 , 1 );                               // a freed cell reused
 CHECK( M.top( 0 ) == 1 && M.pop( 0 ) == 1 );
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
// each checked by its audit, on both stacks: empty, one item, all equal,
// sorted, reversed, the extremes of int, k above all and k the minimum;
// the visit from the bottom; palindromes of length 0 , 1 and odd
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
 // empty and one-character strings, odd length; empty stacks, one item,
 // all items equal, already sorted and reversed, k below and above all
 CHECK( isPalindrome( "" ) && isPalindrome( "x" ) && isPalindrome( "aba" ) &&
        ! isPalindrome( "abca" ) );
 ArrayStack< int > E( 4 );
 SequenceStack< List< int > > F;
 sortStack( E );
 removeGreater( F , 0 );
 int visits = 0;
 forEachBottomUp( E , [ & ]( int ) { ++visits; } );
 CHECK( E.isEmpty() && F.isEmpty() && visits == 0 );
 for( const std::vector< int > & items :
      { std::vector< int >{ 5 } , std::vector< int >( 6 , 3 ) ,
        std::vector< int >{ 1 , 2 , 3 , 4 } ,
        std::vector< int >{ 4 , 3 , 2 , 1 } ,
        std::vector< int >{ std::numeric_limits< int >::max() ,
                            std::numeric_limits< int >::min() } } ) {
  const int lo = std::numeric_limits< int >::min();
  ArrayStack< int > S( 10 );
  SequenceStack< List< int > > T , U;
  for( int x : items ) {
   S.push( x );
   T.push( x );
   U.push( x );
   }
  sortStack( S );
  std::vector< int > sorted = items , seen;
  std::sort( sorted.begin() , sorted.end() );
  for( int x : sorted ) CHECK( S.pop() == x );
  removeGreater( T , std::numeric_limits< int >::max() );   // none
  removeGreater( U , lo );                       // all but the minima
  forEachBottomUp( T , [ & ]( int x ) { seen.push_back( x ); } );
  CHECK( seen == items );                        // bottom up, unchanged
  for( std::size_t i = items.size() ; i-- > 0 ; )
   CHECK( T.pop() == items[ i ] );
  while( ! U.isEmpty() ) CHECK( U.pop() == lo );
  }
 }

// the expressions: an operator with fewer than two operands, more than one
// value left, a division by 0, a parenthesis in a postfix expression, an
// unknown character, unbalanced parentheses, and a pair of parentheses
// with no operator in a fully parenthesized expression; the empty
// expression, the associativity to the left, the truncation toward 0, and
// each operation just inside and just outside the range of long (LONG_MIN
// / -1 and LONG_MIN * -1 too), an operand beyond it
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
 // each precondition once more: an operator alone, a closed parenthesis
 // alone, the empty expression (no value left), a letter; an unclosed
 // operator in a parenthesized expression
 CHECK_THROWS( evalPostfix( "+" ) , contract_violation );
 CHECK_THROWS( evalPostfix( ")" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "   " ) , contract_violation );
 CHECK_THROWS( evalPostfix( "1 x +" ) , contract_violation );
 CHECK_THROWS( toPostfix( "a" ) , contract_violation );
 CHECK_THROWS( toPostfix( ")" ) , contract_violation );
 CHECK_THROWS( toPostfix( "(" ) , contract_violation );
 CHECK_THROWS( toPostfixParenthesized( "1 + 2" ) , contract_violation );
 CHECK_THROWS( toPostfixParenthesized( ")" ) , contract_violation );
 CHECK( toPostfix( "" ) == "" && toPostfixParenthesized( "" ) == "" );
 CHECK( toPostfix( "()" ) == "" && toPostfix( "((7))" ) == "7" );
 CHECK( toPostfix( "8 - 3 - 2" ) == "8 3 - 2 -" );       // to the left
 CHECK( evalPostfix( "8 3 - 2 -" ) == 3 && evalPostfix( "7 2 /" ) == 3 &&
        evalPostfix( "0 7 2 - /" ) == 0 && evalPostfix( "0 7 - 2 /" ) == -3 );
 // the operations at the bounds of long, just inside and just outside
 const std::string M = std::to_string( std::numeric_limits< long >::max() );
 CHECK( evalPostfix( M ) == std::numeric_limits< long >::max() );
 CHECK( evalPostfix( M + " 1 - 1 +" ) == std::numeric_limits< long >::max() );
 CHECK_THROWS( evalPostfix( M + " 1 +" ) , contract_violation );
 CHECK( evalPostfix( "0 " + M + " - 1 -" ) ==
        std::numeric_limits< long >::min() );
 CHECK_THROWS( evalPostfix( "0 " + M + " - 2 -" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "0 " + M + " - 1 - 1 -" ) , contract_violation );
 CHECK_THROWS( evalPostfix( M + " 2 *" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "0 " + M + " - 2 *" ) , contract_violation );
 CHECK_THROWS( evalPostfix( "0 " + M + " - 0 2 - *" ) ,
               contract_violation );
 CHECK( evalPostfix( "0 " + M + " - 0 1 - *" ) ==
        std::numeric_limits< long >::max() );
 CHECK( evalPostfix( "0 " + M + " - 1 - 0 *" ) == 0 );
 CHECK( evalPostfix( "0 " + M + " - 1 - 0 1 - 0 1 - * *" ) ==
        std::numeric_limits< long >::min() );
 CHECK_THROWS( evalPostfix( "0 " + M + " - 1 - 0 1 - /" ) ,
               contract_violation );                    // min / -1
 CHECK_THROWS( evalPostfix( "0 " + M + " - 1 - 0 1 - *" ) ,
               contract_violation );                    // min * -1
 CHECK( evalPostfix( "0 " + M + " - 1 - 1 /" ) ==
        std::numeric_limits< long >::min() );
 CHECK_THROWS( evalPostfix( M + "0" ) , std::out_of_range );
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
