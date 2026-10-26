/*--------------------------------------------------------------------------*/
/*----------------------------- File ch08.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 8, Insiemi e dizionari: each realization of the SET
 * (boolean vector, unsorted and sorted list, hash tables, Red-Black tree)
 * on the procedures of the book and against the operations of the standard
 * library on random sets, and the exercises 8.1 to 8.10, each a test named
 * after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf
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

#include "ADS/ADT.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/DictionarySet.h"
#include "ADS/set/HashMultiset.h"
#include "ADS/set/LinkedHashSet.h"
#include "ADS/set/ListSet.h"
#include "ADS/set/SetOperations.h"
#include "ADS/set/SortedListSet.h"

#include <algorithm>
#include <iterator>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

using Ints = std::vector< long >;

static_assert( Set< BoolSet > );
static_assert( Set< ListSet< long > > );
static_assert( Set< SortedListSet< long > > );
static_assert( IterableSet< ChainedHashSet< long > > );
static_assert( IterableSet< OpenHashSet< long > > );
static_assert( IterableSet< TreeSet< long > > );
static_assert( IterableSet< LinkedHashSet< long > > );
static_assert( Set< HashMultiset< long > > );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the items in the order of the foreach
template< class S >
Ints order( const S & s )
{
 Ints out;
 s.forEach( [ & ]( const auto & x ) { out.push_back( x ); } );
 return( out );
 }

/// the items in increasing order
template< class S >
Ints items( const S & s )
{
 Ints out = order( s );
 std::sort( out.begin() , out.end() );
 return( out );
 }

/// the four operations on random sets of [ 0 , 200 ) against those of the
/// standard library, each set built by make() and filled by insertions
template< class S , class M >
void against_std( M make , unsigned seed )
{
 std::mt19937 g( seed );
 for( int t = 0 ; t < 50 ; ++t ) {
  S A = make() , B = make();
  std::set< long > a , b;
  const int na = g() % 120 , nb = g() % 120;
  for( int i = 0 ; i < na ; ++i ) {
   const long x = g() % 200;
   A.insert( x );
   a.insert( x );
   }
  for( int i = 0 ; i < nb ; ++i ) {
   const long x = g() % 200;
   B.insert( x );
   b.insert( x );
   }
  for( int i = 0 ; i < 20 ; ++i ) {          // and some removals
   const long x = g() % 200;
   A.remove( x );
   a.erase( x );
   }
  CHECK( A.size() == a.size() && items( A ) == Ints( a.begin() , a.end() ) );
  Ints u , n , d , s;
  std::set_union( a.begin() , a.end() , b.begin() , b.end() ,
                  std::back_inserter( u ) );
  std::set_intersection( a.begin() , a.end() , b.begin() , b.end() ,
                         std::back_inserter( n ) );
  std::set_difference( a.begin() , a.end() , b.begin() , b.end() ,
                       std::back_inserter( d ) );
  std::set_symmetric_difference( a.begin() , a.end() , b.begin() , b.end() ,
                                 std::back_inserter( s ) );
  CHECK( items( setUnion( A , B ) ) == u );
  CHECK( items( setIntersection( A , B ) ) == n );
  CHECK( items( setDifference( A , B ) ) == d );
  CHECK( items( setSymmetricDifference( A , B ) ) == s );
  CHECK( setUnion( A , B ).size() == u.size() );
  }
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// section 8.1: the operations of the boolean vector, also on two sets of
// different dimension; the edges: N = 0, a set of nothing whose
// operations with others keep the dimensions, N = 1, the items 0 and
// N - 1 at the ends, the extremes of long, which are not items and are
// removed as no-ops, N < 0
ADS_TEST( "sec8.1.boolean" )
{
 BoolSet A( 10 ) , B( 6 );
 CHECK( A.empty() && ! A.min() );
 for( long x : { 1 , 3 , 5 , 7 , 9 } )
  A.insert( x );
 A.insert( 3 );
 for( long x : { 0 , 1 , 2 , 3 } )
  B.insert( x );
 CHECK( A.size() == 5 && A.min() == 1 && A.max() == 9 );
 CHECK( ! A.contains( 10 ) && ! A.contains( -1 ) );
 CHECK_THROWS( A.insert( 10 ) , contract_violation );
 const auto U = setUnion( A , B );
 const auto I = setIntersection( A , B );
 const auto D = setDifference( A , B );
 const auto E = setDifference( B , A );
 CHECK( U.dim() == 10 && items( U ) == Ints( { 0 , 1 , 2 , 3 , 5 , 7 , 9 } ) );
 CHECK( I.dim() == 6 && items( I ) == Ints( { 1 , 3 } ) );
 CHECK( D.dim() == 10 && items( D ) == Ints( { 5 , 7 , 9 } ) );
 CHECK( E.dim() == 6 && items( E ) == Ints( { 0 , 2 } ) );
 A.remove( 5 );
 A.remove( 5 );
 A.remove( 42 );
 CHECK( A.size() == 4 );
 against_std< BoolSet >( [] { return( BoolSet( 200 ) ); } , 1 );

 // the edges
 using LL = std::numeric_limits< long >;
 BoolSet Z( 0 ) , O( 1 );
 CHECK( Z.empty() && Z.dim() == 0 && ! Z.min() && ! Z.max() &&
        ! Z.contains( 0 ) && items( Z ).empty() );
 CHECK_THROWS( Z.insert( 0 ) , contract_violation );
 Z.remove( 0 );
 CHECK( setUnion( Z , Z ).dim() == 0 && setUnion( Z , Z ).empty() );
 CHECK( setUnion( Z , A ).dim() == 10 && items( setUnion( Z , A ) ) ==
        items( A ) );
 CHECK( setIntersection( A , Z ).dim() == 0 &&
        setIntersection( A , Z ).empty() );
 CHECK( items( setDifference( A , Z ) ) == items( A ) &&
        setDifference( Z , A ).dim() == 0 );
 CHECK( items( setSymmetricDifference( Z , A ) ) == items( A ) );
 O.insert( 0 );
 CHECK( O.size() == 1 && O.min() == 0 && O.max() == 0 );
 CHECK_THROWS( O.insert( 1 ) , contract_violation );
 A.insert( 0 );
 A.insert( 9 );
 CHECK( A.min() == 0 && A.max() == 9 );
 for( long x : { LL::min() , LL::max() , -1L } ) {
  CHECK( ! A.contains( x ) );
  A.remove( x );
  CHECK_THROWS( A.insert( x ) , contract_violation );
  }
 CHECK( A.size() == 5 );
 A.remove( 0 );
 A.remove( 9 );
 CHECK( A.min() == 1 && A.max() == 7 );
 CHECK_THROWS( BoolSet( -1 ) , contract_violation );
 }

// section 8.2: the unsorted list, the difference of the book; the edges:
// the empty set, one item, the item at the head and at the tail removed,
// the minimum at either end, insertNew() of an item that is there (an
// audit), the extremes of long
ADS_TEST( "sec8.2.unsorted-list" )
{
 ListSet< long > A = { 4 , 1 , 3 , 1 } , B = { 3 , 5 };
 CHECK( A.size() == 3 && order( A ) == Ints( { 3 , 1 , 4 } ) );
 CHECK( A.min() == 1 );
 CHECK( items( setDifference( A , B ) ) == Ints( { 1 , 4 } ) );
 A.remove( 1 );
 A.remove( 1 );
 CHECK( order( A ) == Ints( { 3 , 4 } ) );
 A.insertNew( 9 );
 CHECK( order( A ) == Ints( { 9 , 3 , 4 } ) );
 against_std< ListSet< long > >( [] { return( ListSet< long >() ); } , 2 );

 // the edges
 using LL = std::numeric_limits< long >;
 ListSet< long > E , S = { 6 };
 CHECK( E.empty() && ! E.min() && ! E.contains( 0 ) );
 E.remove( 0 );
 CHECK( E.size() == 0 && order( E ).empty() );
 CHECK( S.min() == 6 && S.size() == 1 );
 S.remove( 6 );
 CHECK( S.empty() && ! S.min() );
 CHECK_THROWS( A.insertNew( 3 ) , contract_violation );
 CHECK( A.size() == 3 );
 A.remove( 9 );                                  // the head
 A.remove( 4 );                                  // the tail
 CHECK( order( A ) == Ints( { 3 } ) );
 ListSet< long > X = { 0 , LL::max() , LL::min() };
 CHECK( X.min() == LL::min() && X.contains( LL::max() ) );
 X.remove( LL::min() );
 CHECK( X.min() == 0 );
 }

// section 8.3: the sorted list, min() and max() in O( 1 ), the
// intersection of the book; the edges: the empty set, insertLast() into
// it and of an item smaller than the largest, the only item removed, an
// insertion before the head, after the tail and in the middle, the
// extremes of long
ADS_TEST( "sec8.3.sorted-list" )
{
 SortedListSet< long > A = { 7 , 2 , 9 , 4 , 2 } , B = { 4 , 5 , 9 , 10 };
 CHECK( order( A ) == Ints( { 2 , 4 , 7 , 9 } ) );
 CHECK( A.min() == 2 && A.max() == 9 );
 CHECK( order( setIntersection( A , B ) ) == Ints( { 4 , 9 } ) );
 A.remove( 2 );
 A.remove( 9 );
 CHECK( A.min() == 4 && A.max() == 7 && A.contains( 7 ) &&
        ! A.contains( 2 ) );
 A.insertLast( 8 );
 CHECK_THROWS( A.insertLast( 8 ) , std::logic_error );
 CHECK( order( A ) == Ints( { 4 , 7 , 8 } ) );
 against_std< SortedListSet< long > >(
  [] { return( SortedListSet< long >() ); } , 3 );

 // the edges
 using LL = std::numeric_limits< long >;
 SortedListSet< long > E;
 CHECK( E.empty() && ! E.min() && ! E.max() && ! E.contains( 0 ) );
 E.remove( 0 );
 E.insertLast( LL::min() );
 CHECK( E.min() == LL::min() && E.max() == LL::min() && E.size() == 1 );
 E.remove( LL::min() );
 CHECK( E.empty() && ! E.min() && ! E.max() );
 CHECK_THROWS( A.insertLast( 5 ) , contract_violation );
 A.insert( 1 );
 A.insert( 100 );
 A.insert( 5 );
 A.insert( LL::max() );
 A.insert( LL::min() );
 A.insert( 5 );
 CHECK( order( A ) == Ints( { LL::min() , 1 , 4 , 5 , 7 , 8 , 100 ,
                              LL::max() } ) );
 CHECK( A.size() == 8 && A.min() == LL::min() && A.max() == LL::max() );
 CHECK_THROWS( A.insertLast( LL::max() ) , contract_violation );
 }

// section 8.4: the sets on the hash tables; the edges: tables of one
// place, a single list that holds everything and an open table that
// restructures from 1, the operations with the empty set and of a set
// with itself, an open table without restructuring that, full, refuses a
// new item and keeps the old ones
ADS_TEST( "sec8.4.hash" )
{
 against_std< ChainedHashSet< long > >(
  [] { return( ChainedHashSet< long >( 7 , true ) ); } , 4 );
 against_std< OpenHashSet< long > >(
  [] { return( OpenHashSet< long >( 7 , Probing::double_hashing() ,
                                    true ) ); } , 5 );
 against_std< OpenHashSet< long > >(
  [] { return( OpenHashSet< long >( 401 , Probing::linear() ) ); } , 6 );

 // the edges
 against_std< ChainedHashSet< long > >(
  [] { return( ChainedHashSet< long >( 1 ) ); } , 8 );
 against_std< OpenHashSet< long > >(
  [] { return( OpenHashSet< long >( 1 , Probing::linear() , true ) ); } ,
  9 );
 ChainedHashSet< long > A( 7 ) , E( 7 );
 for( long x : { 3 , 1 , 4 } )
  A.insert( x );
 CHECK( items( setUnion( A , E ) ) == items( A ) &&
        items( setUnion( E , A ) ) == items( A ) );
 CHECK( setIntersection( A , E ).empty() &&
        setIntersection( E , A ).empty() );
 CHECK( items( setDifference( A , E ) ) == items( A ) &&
        setDifference( E , A ).empty() && setDifference( A , A ).empty() );
 CHECK( items( setSymmetricDifference( E , A ) ) == items( A ) &&
        setSymmetricDifference( A , A ).empty() );
 CHECK( items( setUnion( A , A ) ) == items( A ) &&
        items( setIntersection( A , A ) ) == items( A ) );
 CHECK( setUnion( E , E ).empty() );
 OpenHashSet< long > F( 2 , Probing::linear() );
 F.insert( 0 );
 F.insert( 1 );
 F.insert( 1 );
 CHECK_THROWS( F.insert( 2 ) , std::overflow_error );
 CHECK( F.size() == 2 && items( F ) == Ints( { 0 , 1 } ) );
 F.remove( 0 );
 F.insert( 2 );
 CHECK( items( F ) == Ints( { 1 , 2 } ) );
 }

// section 8.5: the set on a Red-Black tree, whose foreach is in order; the
// edges: the empty tree, the root removed, an absent item removed, the
// items inserted in increasing and in decreasing order (a chain for a
// tree that is not balanced), the extremes of long
ADS_TEST( "sec8.5.tree" )
{
 TreeSet< long > A;
 for( long x : { 50 , 20 , 80 , 10 , 30 , 20 } )
  A.insert( x );
 CHECK( order( A ) == Ints( { 10 , 20 , 30 , 50 , 80 } ) );
 against_std< TreeSet< long > >( [] { return( TreeSet< long >() ); } , 7 );

 // the edges
 using LL = std::numeric_limits< long >;
 TreeSet< long > E , S;
 CHECK( E.empty() && order( E ).empty() && ! E.contains( 0 ) );
 E.remove( 0 );
 E.insert( 50 );
 E.remove( 50 );
 CHECK( E.empty() && order( E ).empty() );
 A.remove( 50 );
 A.remove( 51 );
 CHECK( order( A ) == Ints( { 10 , 20 , 30 , 80 } ) );
 Ints up;
 for( long x = 0 ; x < 3000 ; ++x ) {
  S.insert( x );
  S.insert( -1 - x );
  up.push_back( x );
  up.push_back( -1 - x );
  }
 S.insert( LL::max() );
 S.insert( LL::min() );
 up.push_back( LL::max() );
 up.push_back( LL::min() );
 std::sort( up.begin() , up.end() );
 CHECK( order( S ) == up && S.size() == up.size() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the intersection of unsorted lists: with only the test of equality each
// item of A must meet each one of B, O( n m ) is optimal; the edges: an
// empty operand or both, A with itself, disjoint lists, one item in
// common at the ends of the lists, A contained in B
ADS_TEST( "ex8.1" )
{
 ListSet< long > A = { 1 , 2 , 3 , 4 } , B = { 6 , 4 , 2 } , E;
 CHECK( items( setIntersection( A , B ) ) == Ints( { 2 , 4 } ) );
 CHECK( setIntersection( A , E ).empty() );
 CHECK( setIntersection( E , B ).empty() );
 CHECK( items( setIntersection( A , A ) ) == items( A ) );
 ListSet< long > D = { 7 , 8 } , H = { 4 , 9 } , S = { 2 };
 CHECK( setIntersection( E , E ).empty() );
 CHECK( setIntersection( A , D ).empty() &&
        setIntersection( D , A ).empty() );
 CHECK( order( setIntersection( A , H ) ) == Ints( { 4 } ) );
 CHECK( order( setIntersection( S , B ) ) == Ints( { 2 } ) &&
        order( setIntersection( B , S ) ) == Ints( { 2 } ) );
 }

// the union of unsorted lists: C starts as B, and the items of A not in B
// are inserted at the head without looking for them in C; the edges: an
// empty operand or both, disjoint lists (all of A at the head, in the
// order of its foreach), identical ones (C is B), A contained in B
ADS_TEST( "ex8.2" )
{
 ListSet< long > A = { 1 , 2 , 3 } , B = { 3 , 4 } , E;
 const auto C = setUnion( A , B );
 CHECK( order( C ) == Ints( { 1 , 2 , 4 , 3 } ) );
 CHECK( items( setUnion( E , B ) ) == items( B ) );
 CHECK( items( setUnion( A , E ) ) == items( A ) );
 ListSet< long > D = { 7 , 8 } , S = { 4 };
 CHECK( setUnion( E , E ).empty() );
 CHECK( order( setUnion( D , B ) ) == Ints( { 7 , 8 , 4 , 3 } ) );
 CHECK( order( setUnion( B , B ) ) == order( B ) );
 CHECK( order( setUnion( S , B ) ) == order( B ) );
 }

// the union of sorted lists by merging, the leftover of either list
// appended at the end; the edges: an empty operand or both, identical
// lists, all of A before all of B (one list left over whole), interleaved
// lists without ties, the extremes of long
ADS_TEST( "ex8.3" )
{
 SortedListSet< long > A = { 1 , 3 , 5 , 7 } , B = { 2 , 3 , 8 , 9 } , E;
 CHECK( order( setUnion( A , B ) ) == Ints( { 1 , 2 , 3 , 5 , 7 , 8 , 9 } ) );
 CHECK( order( setUnion( B , A ) ) == order( setUnion( A , B ) ) );
 CHECK( order( setUnion( A , E ) ) == order( A ) );
 CHECK( order( setUnion( E , A ) ) == order( A ) );
 using LL = std::numeric_limits< long >;
 SortedListSet< long > L = { -5 , -4 } , M = { 2 , 4 , 6 } ,
                       X = { LL::min() , LL::max() };
 CHECK( setUnion( E , E ).empty() );
 CHECK( order( setUnion( A , A ) ) == order( A ) );
 CHECK( order( setUnion( L , A ) ) == Ints( { -5 , -4 , 1 , 3 , 5 , 7 } ) &&
        order( setUnion( A , L ) ) == order( setUnion( L , A ) ) );
 CHECK( order( setUnion( A , M ) ) ==
        Ints( { 1 , 2 , 3 , 4 , 5 , 6 , 7 } ) );
 CHECK( order( setUnion( X , A ) ) ==
        Ints( { LL::min() , 1 , 3 , 5 , 7 , LL::max() } ) );
 }

// the difference of sorted lists by merging; the edges: an empty operand
// or both, A minus itself, disjoint lists, B containing A, B larger than
// all of A, B with the first and the last item of A only
ADS_TEST( "ex8.4" )
{
 SortedListSet< long > A = { 1 , 3 , 5 , 7 , 11 } , B = { 0 , 3 , 7 , 8 } ,
                       E;
 CHECK( order( setDifference( A , B ) ) == Ints( { 1 , 5 , 11 } ) );
 CHECK( order( setDifference( B , A ) ) == Ints( { 0 , 8 } ) );
 CHECK( order( setDifference( A , E ) ) == order( A ) );
 CHECK( setDifference( E , A ).empty() );
 CHECK( setDifference( A , A ).empty() );
 SortedListSet< long > D = { 2 , 4 } , S = { -1 , 1 , 3 , 5 , 7 , 11 , 12 } ,
                       G = { 100 } , F = { 1 , 11 };
 CHECK( setDifference( E , E ).empty() );
 CHECK( order( setDifference( A , D ) ) == order( A ) );
 CHECK( setDifference( A , S ).empty() );
 CHECK( order( setDifference( S , A ) ) == Ints( { -1 , 12 } ) );
 CHECK( order( setDifference( A , G ) ) == order( A ) );
 CHECK( order( setDifference( A , F ) ) == Ints( { 3 , 5 , 7 } ) );
 }

// the symmetric difference in the three realizations, against
// ( A u B ) - ( A n B ); the edges: an empty operand gives the other, two
// identical sets give nothing, two disjoint sets give the union, and for
// the boolean vectors also the dimensions 0 and 1
ADS_TEST( "ex8.5" )
{
 const Ints a = { 1 , 2 , 3 , 8 } , b = { 2 , 4 , 8 , 9 } ,
            s = { 1 , 3 , 4 , 9 };
 BoolSet A( 9 ) , B( 10 );
 ListSet< long > LA , LB;
 SortedListSet< long > SA , SB;
 for( long x : a ) { A.insert( x ); LA.insert( x ); SA.insert( x ); }
 for( long x : b ) { B.insert( x ); LB.insert( x ); SB.insert( x ); }
 CHECK( items( setSymmetricDifference( A , B ) ) == s );
 CHECK( items( setSymmetricDifference( LA , LB ) ) == s );
 CHECK( order( setSymmetricDifference( SA , SB ) ) == s );
 CHECK( items( setDifference( setUnion( SA , SB ) ,
                              setIntersection( SA , SB ) ) ) == s );
 CHECK( setSymmetricDifference( SA , SA ).empty() );
 // the edges
 BoolSet Z( 0 ) , O( 1 );
 ListSet< long > LE;
 SortedListSet< long > SE;
 CHECK( items( setSymmetricDifference( A , Z ) ) == a &&
        setSymmetricDifference( Z , Z ).empty() &&
        setSymmetricDifference( A , A ).empty() );
 O.insert( 0 );
 CHECK( items( setSymmetricDifference( O , B ) ) ==
        Ints( { 0 , 2 , 4 , 8 , 9 } ) );
 CHECK( items( setSymmetricDifference( LE , LB ) ) == b &&
        items( setSymmetricDifference( LA , LE ) ) == a &&
        setSymmetricDifference( LA , LA ).empty() &&
        setSymmetricDifference( LE , LE ).empty() );
 CHECK( order( setSymmetricDifference( SE , SB ) ) == b &&
        order( setSymmetricDifference( SA , SE ) ) == a &&
        setSymmetricDifference( SE , SE ).empty() );
 SortedListSet< long > SD = { 0 , 5 , 10 };
 ListSet< long > LD = { 0 , 5 , 10 };
 CHECK( order( setSymmetricDifference( SA , SD ) ) ==
        Ints( { 0 , 1 , 2 , 3 , 5 , 8 , 10 } ) );
 CHECK( items( setSymmetricDifference( LD , LA ) ) ==
        Ints( { 0 , 1 , 2 , 3 , 5 , 8 , 10 } ) );
 }

// the operations on hash tables, with n_X keys in a table of size m_X: the
// union O( max( m_A , n_A ) + max( m_B , n_B ) ), the intersection and the
// difference O( max( m , n ) ) of the smaller table and of A; the result
// of the intersection is a copy of the smaller table. The solution of the
// book inserts into T_C "each key of the smaller table that does not
// appear in the larger one", which is the difference: the intersection
// keeps the keys that appear in both. The edges: on a tie of the sizes
// the copy is of A, an empty B gives an empty copy of B, disjoint tables,
// B contained in A
ADS_TEST( "ex8.6" )
{
 ChainedHashSet< long > A( 101 ) , B( 11 );
 for( long x = 0 ; x < 100 ; ++x )
  A.insert( x );
 for( long x = 90 ; x < 110 ; x += 2 )
  B.insert( x );
 const auto I = setIntersection( A , B );
 CHECK( I.dictionary().capacity() == 11 );
 CHECK( items( I ) == Ints( { 90 , 92 , 94 , 96 , 98 } ) );
 CHECK( setUnion( A , B ).size() == 105 );
 CHECK( setDifference( A , B ).size() == 95 );
 // the edges
 ChainedHashSet< long > C( 13 ) , E( 3 ) , D( 17 );
 for( long x : { 1 , 2 , 3 , 4 , 5 } )
  C.insert( x );
 for( long x : { 3 , 4 , 5 , 6 , 7 } )
  D.insert( x );
 CHECK( setIntersection( C , D ).dictionary().capacity() == 13 &&
        setIntersection( D , C ).dictionary().capacity() == 17 );
 CHECK( items( setIntersection( D , C ) ) == Ints( { 3 , 4 , 5 } ) );
 const auto IE = setIntersection( A , E );
 CHECK( IE.empty() && IE.dictionary().capacity() == 3 );
 CHECK( setUnion( E , A ).size() == 100 && setDifference( A , E ).size() ==
        100 && setDifference( E , A ).empty() );
 ChainedHashSet< long > F( 7 );
 for( long x : { 1000 , 2000 } )
  F.insert( x );
 CHECK( setIntersection( A , F ).empty() &&
        setDifference( A , F ).size() == 100 &&
        setUnion( A , F ).size() == 102 );
 CHECK( items( setIntersection( A , C ) ) == items( C ) &&
        setDifference( C , A ).empty() );
 }

// the union with Red-Black trees: a copy of A, plus the items of B, in
// O( n + m log( n + m ) ) time, and the foreach of the result is in order;
// the edges: empty operands, a set with itself, B inside A, all of B
// below all of A
ADS_TEST( "ex8.7" )
{
 TreeSet< long > A , B;
 for( long x = 0 ; x < 1000 ; x += 3 )
  A.insert( x );
 for( long x = 0 ; x < 1000 ; x += 5 )
  B.insert( x );
 const auto C = setUnion( A , B );
 const Ints c = order( C );
 CHECK( std::is_sorted( c.begin() , c.end() ) );
 CHECK( c.size() == 334 + 200 - 67 );
 CHECK( order( A ).size() == 334 );               // A is untouched
 // the edges
 TreeSet< long > E , L;
 for( long x = -10 ; x < 0 ; ++x )
  L.insert( x );
 CHECK( setUnion( E , E ).empty() );
 CHECK( order( setUnion( E , A ) ) == order( A ) &&
        order( setUnion( A , E ) ) == order( A ) );
 CHECK( order( setUnion( A , A ) ) == order( A ) );
 CHECK( order( setUnion( C , B ) ) == c );
 Ints la = order( L );
 const Ints oa = order( A );
 la.insert( la.end() , oa.begin() , oa.end() );
 CHECK( order( setUnion( A , L ) ) == la );
 }

// the codomain of a dictionary, the set of its values, by its foreach
template< class D >
ChainedHashSet< typename D::Value > codomain( const D & d )
{
 ChainedHashSet< typename D::Value > C( 7 , true );
 d.forEach( [ & ]( const auto & , const auto & v ) { C.insert( v ); } );
 return( C );
 }

// the codomain of a Red-Black tree and of the two hash tables; the edges:
// the empty dictionaries, one pair, all the values equal, all distinct, a
// value overwritten that leaves the codomain
ADS_TEST( "ex8.8" )
{
 RedBlackTree< std::string , long > t;
 ChainedHashTable< std::string , long > c( 7 , true );
 OpenHashTable< std::string , long > o( 7 , Probing::double_hashing() ,
                                        true );
 const std::vector< std::pair< std::string , long > > pairs = {
  { "pisa" , 56 } , { "lucca" , 55 } , { "livorno" , 57 } ,
  { "cascina" , 56 } , { "viareggio" , 55 } };
 for( const auto & [ k , v ] : pairs ) {
  t.insert( k , v );
  c.insert( k , v );
  o.insert( k , v );
  }
 CHECK( items( codomain( t ) ) == Ints( { 55 , 56 , 57 } ) );
 CHECK( items( codomain( c ) ) == Ints( { 55 , 56 , 57 } ) );
 CHECK( items( codomain( o ) ) == Ints( { 55 , 56 , 57 } ) );
 CHECK( codomain( RedBlackTree< std::string , long >() ).empty() );
 // the edges
 CHECK( codomain( ChainedHashTable< std::string , long >( 7 ) ).empty() );
 CHECK( codomain( OpenHashTable< std::string , long >(
         7 , Probing::linear() ) ).empty() );
 RedBlackTree< long , long > one , same , all;
 one.insert( 1 , -1 );
 for( long k = 0 ; k < 100 ; ++k ) {
  same.insert( k , 42 );
  all.insert( k , 2 * k );
  }
 CHECK( items( codomain( one ) ) == Ints( { -1 } ) );
 CHECK( items( codomain( same ) ) == Ints( { 42 } ) );
 CHECK( codomain( all ).size() == 100 );
 c.insert( "livorno" , 56 );
 CHECK( items( codomain( c ) ) == Ints( { 55 , 56 } ) );
 }

// the LinkedHashSet: the foreach in the order of insertion, an item
// inserted again keeps its place, a removed one loses it; the edges: the
// empty set and its copy, the only item removed and inserted again, the
// first and the last item removed, an item inserted many times, the
// intersection and the difference that keep the order of the smaller set
// and of A
ADS_TEST( "ex8.9" )
{
 LinkedHashSet< long > s;
 for( long x : { 5 , 3 , 9 , 3 , 1 } )
  s.insert( x );
 CHECK( order( s ) == Ints( { 5 , 3 , 9 , 1 } ) );
 s.remove( 3 );
 s.remove( 42 );
 s.insert( 3 );
 CHECK( order( s ) == Ints( { 5 , 9 , 1 , 3 } ) );
 LinkedHashSet< long > t = s;
 t.remove( 5 );
 CHECK( order( s ) == Ints( { 5 , 9 , 1 , 3 } ) );
 CHECK( order( t ) == Ints( { 9 , 1 , 3 } ) );

 // against a vector on random operations
 std::mt19937 g( 9 );
 LinkedHashSet< long > u;
 Ints ref;
 for( int o = 0 ; o < 20000 ; ++o ) {
  const long x = g() % 500;
  if( g() % 3 ) {
   u.insert( x );
   if( std::find( ref.begin() , ref.end() , x ) == ref.end() )
    ref.push_back( x );
   }
  else {
   u.remove( x );
   ref.erase( std::remove( ref.begin() , ref.end() , x ) , ref.end() );
   }
  }
 CHECK( order( u ) == ref && u.size() == ref.size() );
 // the generic union keeps the order: A first, then the new items of B
 LinkedHashSet< long > a , b;
 for( long x : { 3 , 1 , 2 } ) a.insert( x );
 for( long x : { 4 , 2 , 0 } ) b.insert( x );
 CHECK( order( setUnion( a , b ) ) == Ints( { 3 , 1 , 2 , 4 , 0 } ) );

 // the edges
 LinkedHashSet< long > e;
 const LinkedHashSet< long > ec = e;
 CHECK( e.empty() && order( e ).empty() && ec.empty() );
 e.remove( 1 );
 e.insert( 1 );
 e.remove( 1 );
 CHECK( e.empty() && order( e ).empty() && ! e.contains( 1 ) );
 e.insert( 1 );
 CHECK( order( e ) == Ints( { 1 } ) );
 for( long x : { 2 , 3 , 4 } )
  e.insert( x );
 for( int i = 0 ; i < 100 ; ++i )
  e.insert( 2 );
 e.remove( 1 );
 e.remove( 4 );
 CHECK( order( e ) == Ints( { 2 , 3 } ) && e.size() == 2 );
 e.insert( 1 );
 CHECK( order( e ) == Ints( { 2 , 3 , 1 } ) );
 CHECK( order( setIntersection( a , b ) ) == Ints( { 2 } ) );
 CHECK( order( setDifference( a , b ) ) == Ints( { 3 , 1 } ) );
 CHECK( order( setSymmetricDifference( a , b ) ) ==
        Ints( { 3 , 1 , 4 , 0 } ) );
 }

// the multiset: a counter of occurrences per item; the edges: the empty
// multiset, whose foreach calls nothing and whose removal changes nothing,
// the empty string as an item, one item many times, down to zero, when it
// leaves the multiset
ADS_TEST( "ex8.10" )
{
 HashMultiset< std::string > m;
 for( const char * w : { "a" , "b" , "a" , "c" , "a" , "b" } )
  m.insert( w );
 CHECK( m.size() == 6 && m.distinct() == 3 );
 CHECK( m.count( "a" ) == 3 && m.count( "b" ) == 2 && m.count( "z" ) == 0 );
 m.remove( "a" );
 m.remove( "c" );
 m.remove( "c" );
 m.remove( "z" );
 CHECK( m.size() == 4 && m.distinct() == 2 );
 CHECK( m.count( "a" ) == 2 && ! m.contains( "c" ) );
 std::size_t total = 0;
 m.forEach( [ & ]( const std::string & , std::size_t c ) { total += c; } );
 CHECK( total == m.size() );

 // the edges
 HashMultiset< std::string > e;
 int calls = 0;
 e.forEach( [ & ]( const std::string & , std::size_t ) { ++calls; } );
 e.remove( "a" );
 CHECK( e.empty() && e.size() == 0 && e.distinct() == 0 && calls == 0 &&
        e.count( "a" ) == 0 && ! e.contains( "" ) );
 const long n = 10000;
 for( long i = 0 ; i < n ; ++i )
  e.insert( "" );
 CHECK( e.size() == std::size_t( n ) && e.distinct() == 1 &&
        e.count( "" ) == std::size_t( n ) );
 for( long i = 0 ; i < n ; ++i )
  e.remove( "" );
 CHECK( e.empty() && e.distinct() == 0 && ! e.contains( "" ) );
 e.remove( "" );
 CHECK( e.size() == 0 );
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
/*---------------------------- End File ch08.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
