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
 *         Universita' di Pisa \n
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
#include <random>
#include <set>
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
// different dimension
ADS_TEST( "sec8.1.boolean" )
{
 BoolSet A( 10 ) , B( 6 );
 CHECK( A.isEmpty() && ! A.min() );
 for( long x : { 1 , 3 , 5 , 7 , 9 } )
  A.insert( x );
 A.insert( 3 );
 for( long x : { 0 , 1 , 2 , 3 } )
  B.insert( x );
 CHECK( A.size() == 5 && A.min() == 1 && A.max() == 9 );
 CHECK( ! A.contains( 10 ) && ! A.contains( -1 ) );
 CHECK_THROWS( A.insert( 10 ) , std::out_of_range );
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
 }

// section 8.2: the unsorted list, the difference of the book
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
 }

// section 8.3: the sorted list, min() and max() in O( 1 ), the
// intersection of the book
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
 }

// section 8.4: the sets on the hash tables
ADS_TEST( "sec8.4.hash" )
{
 against_std< ChainedHashSet< long > >(
  [] { return( ChainedHashSet< long >( 7 , true ) ); } , 4 );
 against_std< OpenHashSet< long > >(
  [] { return( OpenHashSet< long >( 7 , Probing::double_hashing() ,
                                    true ) ); } , 5 );
 against_std< OpenHashSet< long > >(
  [] { return( OpenHashSet< long >( 401 , Probing::linear() ) ); } , 6 );
 }

// section 8.5: the set on a Red-Black tree, whose foreach is in order
ADS_TEST( "sec8.5.tree" )
{
 TreeSet< long > A;
 for( long x : { 50 , 20 , 80 , 10 , 30 , 20 } )
  A.insert( x );
 CHECK( order( A ) == Ints( { 10 , 20 , 30 , 50 , 80 } ) );
 against_std< TreeSet< long > >( [] { return( TreeSet< long >() ); } , 7 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the intersection of unsorted lists: with only the test of equality each
// item of A must meet each one of B, O( n m ) is optimal
ADS_TEST( "ex8.1" )
{
 ListSet< long > A = { 1 , 2 , 3 , 4 } , B = { 6 , 4 , 2 } , E;
 CHECK( items( setIntersection( A , B ) ) == Ints( { 2 , 4 } ) );
 CHECK( setIntersection( A , E ).isEmpty() );
 CHECK( setIntersection( E , B ).isEmpty() );
 CHECK( items( setIntersection( A , A ) ) == items( A ) );
 }

// the union of unsorted lists: C starts as B, and the items of A not in B
// are inserted at the head without looking for them in C
ADS_TEST( "ex8.2" )
{
 ListSet< long > A = { 1 , 2 , 3 } , B = { 3 , 4 } , E;
 const auto C = setUnion( A , B );
 CHECK( order( C ) == Ints( { 1 , 2 , 4 , 3 } ) );
 CHECK( items( setUnion( E , B ) ) == items( B ) );
 CHECK( items( setUnion( A , E ) ) == items( A ) );
 }

// the union of sorted lists by merging, the leftover of either list
// appended at the end
ADS_TEST( "ex8.3" )
{
 SortedListSet< long > A = { 1 , 3 , 5 , 7 } , B = { 2 , 3 , 8 , 9 } , E;
 CHECK( order( setUnion( A , B ) ) == Ints( { 1 , 2 , 3 , 5 , 7 , 8 , 9 } ) );
 CHECK( order( setUnion( B , A ) ) == order( setUnion( A , B ) ) );
 CHECK( order( setUnion( A , E ) ) == order( A ) );
 CHECK( order( setUnion( E , A ) ) == order( A ) );
 }

// the difference of sorted lists by merging
ADS_TEST( "ex8.4" )
{
 SortedListSet< long > A = { 1 , 3 , 5 , 7 , 11 } , B = { 0 , 3 , 7 , 8 } ,
                       E;
 CHECK( order( setDifference( A , B ) ) == Ints( { 1 , 5 , 11 } ) );
 CHECK( order( setDifference( B , A ) ) == Ints( { 0 , 8 } ) );
 CHECK( order( setDifference( A , E ) ) == order( A ) );
 CHECK( setDifference( E , A ).isEmpty() );
 CHECK( setDifference( A , A ).isEmpty() );
 }

// the symmetric difference in the three realizations, against
// ( A u B ) - ( A n B )
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
 CHECK( setSymmetricDifference( SA , SA ).isEmpty() );
 }

// the operations on hash tables, with n_X keys in a table of size m_X: the
// union O( max( m_A , n_A ) + max( m_B , n_B ) ), the intersection and the
// difference O( max( m , n ) ) of the smaller table and of A; the result
// of the intersection is a copy of the smaller table. The solution of the
// book inserts into T_C "each key of the smaller table that does not
// appear in the larger one", which is the difference: the intersection
// keeps the keys that appear in both
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
 }

// the union with Red-Black trees: a copy of A, plus the items of B, in
// O( n + m log( n + m ) ) time, and the foreach of the result is in order
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
 }

// the codomain of a dictionary, the set of its values, by its foreach
template< class D >
ChainedHashSet< typename D::Value > codomain( const D & d )
{
 ChainedHashSet< typename D::Value > C( 7 , true );
 d.forEach( [ & ]( const auto & , const auto & v ) { C.insert( v ); } );
 return( C );
 }

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
 CHECK( codomain( RedBlackTree< std::string , long >() ).isEmpty() );
 }

// the LinkedHashSet: the foreach in the order of insertion, an item
// inserted again keeps its place, a removed one loses it
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
 }

// the multiset: a counter of occurrences per item
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
