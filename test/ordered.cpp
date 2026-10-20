/*--------------------------------------------------------------------------*/
/*--------------------------- File ordered.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the engineered ordered structures, beyond the book: the B+-tree
 * in main memory, the static searches of sorted arrays, the selection, the
 * range minimum queries, the intersections of sorted sets and the union of
 * disjoint sets. Each one is checked against a reference of the standard
 * library on random operations and on the edge cases, at the level 3 of
 * the contracts.
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

#include "ADS/dictionary/AdaptiveRadixTree.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/mfset/RemUnionFind.h"
#include "ADS/search/NthElement.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/search/StaticSearch.h"
#include "ADS/search/WordRangeMinimum.h"
#include "ADS/set/BoolSet.h"
#include "ADS/set/RoaringBitmap.h"
#include "ADS/set/VectorIntersection.h"
#include "ADS/tree/BTreeMap.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// the pairs of a map in the order of its iterators, forward
template< class M >
auto forward( const M & m )
{
 std::vector< std::pair< typename M::Key , typename M::Value > > P;
 for( const auto & [ k , v ] : m )
  P.emplace_back( k , v );
 return( P );
 }

/// the pairs of a map in the order of its iterators, backward
template< class M >
auto backward( const M & m )
{
 std::vector< std::pair< typename M::Key , typename M::Value > > P;
 for( auto it = m.end() ; it != m.begin() ; ) {
  --it;
  P.emplace_back( ( *it ).first , ( *it ).second );
  }
 std::reverse( P.begin() , P.end() );
 return( P );
 }

template< class R >
auto pairs( const R & r )
{
 using K = std::remove_cv_t< typename R::key_type >;
 std::vector< std::pair< K , typename R::mapped_type > > P( r.begin() ,
                                                             r.end() );
 return( P );
 }

/// random insertions, removals and searches on M against std::map, with
/// keys in [ 0 , u )
template< class M >
void against( std::mt19937 & g , int ops , int u )
{
 M m;
 std::map< int , int > ref;
 std::uniform_int_distribution< int > key( 0 , u - 1 ) , op( 0 , 9 );
 for( int t = 0 ; t < ops ; ++t ) {
  const int k = key( g ) , o = op( g );
  if( o < 5 ) {
   CHECK( m.insert( k , t ) == ( ref.find( k ) == ref.end() ) );
   ref[ k ] = t;
   }
  else if( o < 8 ) {
   CHECK( m.remove( k ) == ( ref.erase( k ) == 1 ) );
   }
  else {
   const auto it = ref.find( k );
   const auto v = m.lookup( k );
   CHECK( v.has_value() == ( it != ref.end() ) );
   if( v )
    CHECK( *v == it->second );
   const auto lb = m.lowerBound( k );
   const auto rl = ref.lower_bound( k );
   CHECK( ( lb == m.end() ) == ( rl == ref.end() ) );
   if( rl != ref.end() )
    CHECK( ( *lb ).first == rl->first );
   const auto ub = m.upperBound( k );
   const auto ru = ref.upper_bound( k );
   CHECK( ( ub == m.end() ) == ( ru == ref.end() ) );
   if( ru != ref.end() )
    CHECK( ( *ub ).first == ru->first );
   }
  CHECK( m.size() == ref.size() );
  }
 CHECK( forward( m ) == pairs( ref ) );
 CHECK( backward( m ) == pairs( ref ) );
 for( const auto & [ k , v ] : ref )        // all out, the tree shrinks
  CHECK( m.remove( k ) );
 CHECK( m.empty() && ( m.height() == 0 ) && ( m.begin() == m.end() ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the B+-tree against std::map on random operations, with nodes of 4
// places (every operation splits or merges) and of the default size, on
// few and on many distinct keys; the empty tree, one key, the keys in
// increasing and decreasing order (the nodes of the edges filled), the
// largest and the smallest integer as keys (the padding), an infinite
// key, strings (the binary search), build() from sorted pairs, copies,
// moves, swaps, erase( it ), forRange(), a value of the tree inserted
// again, and a key not sorted in build()
ADS_TEST( "btree-map" )
{
 std::mt19937 g( 31 );
 against< BTreeMap< int , int , std::less< int > , 16 > >( g , 20000 , 50 );
 against< BTreeMap< int , int , std::less< int > , 16 > >( g , 20000 ,
                                                           5000 );
 against< BTreeMap< int , int > >( g , 30000 , 3000 );
 against< BTreeMap< int , int , std::less<> , 64 > >( g , 20000 , 400 );

 BTreeMap< int , int > e;
 CHECK( e.empty() && ! e.lookup( 1 ) && ! e.remove( 1 ) );
 CHECK( ( e.begin() == e.end() ) && ( e.lowerBound( 0 ) == e.end() ) &&
        ( e.upperBound( 0 ) == e.end() ) && ( e.height() == 0 ) );
 CHECK( e.insert( 7 , 70 ) && ! e.insert( 7 , 71 ) && ( *e.lookup( 7 ) ==
                                                         71 ) );
 CHECK( ( e.size() == 1 ) && ( e.height() == 1 ) );
 CHECK( ( ( *e.begin() ).first == 7 ) && ( e.upperBound( 7 ) == e.end() ) );
 CHECK( ( *--e.end() ).first == 7 );
 CHECK( e.remove( 7 ) && e.empty() && ( e.begin() == e.end() ) );

 for( int dir = 0 ; dir < 2 ; ++dir ) {     // in order: full nodes
  BTreeMap< int , int , std::less< int > , 64 > m;   // 8 pairs, 7 keys
  const int n = 5000;
  for( int i = 0 ; i < n ; ++i ) {
   const int k = dir ? n - 1 - i : i;
   CHECK( m.insert( k , -k ) );
   }
  CHECK( m.size() == std::size_t( n ) );
  int h = 1;                                // the height of full nodes
  for( long c = 8 ; c < n ; c *= 8 ) ++h;
  CHECK( m.height() <= h + 1 );
  int i = 0;
  for( const auto & [ k , v ] : m ) {
   CHECK( ( k == i ) && ( v == -i ) );
   ++i;
   }
  for( int k = 0 ; k < n ; k += 2 )
   CHECK( m.remove( k ) );
  for( int k = 0 ; k < n ; ++k )
   CHECK( m.contains( k ) == ( k % 2 == 1 ) );
  }

 {                                          // the limits of the type
  BTreeMap< std::int64_t , int , std::less< std::int64_t > , 16 > m;
  const auto M = std::numeric_limits< std::int64_t >::max();
  const auto m0 = std::numeric_limits< std::int64_t >::min();
  for( std::int64_t k : { M , m0 , std::int64_t( 0 ) , M - 1 , m0 + 1 } )
   CHECK( m.insert( k , 1 ) );
  for( int k = -20 ; k <= 20 ; ++k )
   m.insert( k , 2 );
  CHECK( m.contains( M ) && m.contains( m0 ) );
  CHECK( ( *m.lowerBound( M ) ).first == M );
  CHECK( m.upperBound( M ) == m.end() );
  CHECK( ( *m.upperBound( M - 1 ) ).first == M );
  CHECK( ( *m.begin() ).first == m0 );
  CHECK( m.remove( M ) && ! m.contains( M ) && m.contains( M - 1 ) );
  CHECK( m.lowerBound( M ) == m.end() );
  }

 {                                          // the infinity as a key
  BTreeMap< double , int , std::less< double > , 32 > m;
  const double inf = std::numeric_limits< double >::infinity();
  m.insert( inf , 1 );
  m.insert( -inf , 2 );
  for( int k = 0 ; k < 30 ; ++k ) m.insert( k * 0.5 , k );
  CHECK( m.contains( inf ) && m.contains( -inf ) && ! m.contains( 0.25 ) );
  CHECK( ( *m.lowerBound( 100.0 ) ).first == inf );
  CHECK( m.upperBound( inf ) == m.end() );
  }

 {                                          // strings: the binary search
  BTreeMap< std::string , int , std::less< std::string > , 128 > m;
  std::map< std::string , int > ref;
  for( int t = 0 ; t < 3000 ; ++t ) {
   const std::string k = "key" + std::to_string( g() % 700 );
   if( g() % 3 ) {
    m.insert( k , t );
    ref[ k ] = t;
    }
   else
    CHECK( m.remove( k ) == ( ref.erase( k ) == 1 ) );
   }
  CHECK( forward( m ) == pairs( ref ) );
  CHECK( ( *m.lowerBound( "key5" ) ).first ==
         ref.lower_bound( "key5" )->first );
  }

 {                                          // build, copies, erase( it )
  std::vector< std::pair< int , int > > P;
  for( int i = 0 ; i < 1000 ; ++i ) P.emplace_back( 3 * i , i );
  BTreeMap< int , int , std::less< int > , 64 > m;
  m.build( P.begin() , P.end() );
  CHECK( ( m.size() == 1000 ) && ( forward( m ) == P ) );
  auto c = m;                               // a copy is independent
  CHECK( forward( c ) == P );
  c.remove( 0 );
  CHECK( m.contains( 0 ) && ! c.contains( 0 ) );
  auto d = std::move( c );
  CHECK( ( d.size() == 999 ) && c.empty() );
  d.swap( c );
  CHECK( ( c.size() == 999 ) && d.empty() );
  d = c;
  CHECK( forward( d ) == forward( c ) );
  auto it = m.lowerBound( 1 );              // the pair of 3, erased
  it = m.erase( it );
  CHECK( ( ( *it ).first == 6 ) && ! m.contains( 3 ) );
  std::vector< int > R;
  m.forRange( 10 , 31 , [ & ]( int k , int ) { R.push_back( k ); } );
  CHECK( R == std::vector< int >( { 12 , 15 , 18 , 21 , 24 , 27 , 30 } ) );
  R.clear();
  m.forRange( 5000 , 6000 , [ & ]( int k , int ) { R.push_back( k ); } );
  CHECK( R.empty() );
  m.build( P.begin() , P.begin() );         // empty
  CHECK( m.empty() && ( m.begin() == m.end() ) );
  m.build( P.begin() , P.begin() + 1 );
  CHECK( ( m.size() == 1 ) && ( m.height() == 1 ) );
  std::swap( P[ 4 ] , P[ 5 ] );
  CHECK_THROWS( m.build( P.begin() , P.end() ) , contract_violation );
  }

 {                                          // a value of the tree itself
  BTreeMap< int , std::string , std::less< int > , 64 > m;
  for( int i = 0 ; i < 200 ; i += 2 )
   m.insert( i , std::string( 40 , char( 'a' + i % 26 ) ) );
  for( int i = 1 ; i < 200 ; i += 2 ) {
   const std::string * v = m.find( i - 1 );
   m.insert( i , *v );
   CHECK( *m.find( i ) == *m.find( i - 1 ) );
   }
  *m.find( 0 ) = "zero";
  CHECK( *m.lookup( 0 ) == "zero" );
  }
 }


// the static searches against std::lower_bound and std::upper_bound: the
// empty array, one item, every size up to 300 (the partial nodes of each
// level), many equal items, the limits of the type (the padding) as
// items and as keys, doubles with the infinities, strings and a reversed
// order (the scan in place of the count), copies, and an unsorted array
ADS_TEST( "static-search" )
{
 std::mt19937 g( 32 );
 auto check = [ & ]< class T , class C >( const std::vector< T > & A ,
                                          const std::vector< T > & Q ,
                                          C less ) {
  const SPlusTree< T , C > S( A , less );
  const SPlusTree< T , C > S2 = S;          // a copy
  CHECK( ( S.size() == A.size() ) && ( S.empty() == A.empty() ) );
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   CHECK( !( less( S[ i ] , A[ i ] ) || less( A[ i ] , S[ i ] ) ) );
  for( const T & q : Q ) {
   const auto lo = std::size_t( std::lower_bound( A.begin() , A.end() , q ,
                                                  less ) - A.begin() );
   const auto up = std::size_t( std::upper_bound( A.begin() , A.end() , q ,
                                                  less ) - A.begin() );
   CHECK( S.lowerBound( q ) == lo );
   CHECK( S2.lowerBound( q ) == lo );
   CHECK( S.upperBound( q ) == up );
   CHECK( S.contains( q ) == ( lo < up ) );
   CHECK( branchlessLowerBound( A , q , less ) == lo );
   }
  };
 for( std::size_t n = 0 ; n <= 300 ; n += ( n < 80 ? 1 : 29 ) ) {
  std::vector< std::uint32_t > A( n ) , Q;
  for( auto & a : A ) a = g() % ( 3 * n + 1 );
  std::sort( A.begin() , A.end() );
  for( std::uint32_t q = 0 ; q <= 3 * n + 2 ; ++q ) Q.push_back( q );
  check( A , Q , std::less< std::uint32_t >() );
  }
 {                                          // a few levels, the limits
  std::vector< std::uint64_t > A( 20000 ) , Q;
  for( auto & a : A ) a = g() % 5000;
  const auto M = std::numeric_limits< std::uint64_t >::max();
  A[ 0 ] = 0;
  A[ 1 ] = M;
  A[ 2 ] = M;
  std::sort( A.begin() , A.end() );
  for( int t = 0 ; t < 3000 ; ++t ) Q.push_back( g() % 5100 );
  for( std::uint64_t q : { std::uint64_t( 0 ) , M , M - 1 } )
   Q.push_back( q );
  check( A , Q , std::less< std::uint64_t >() );
  std::vector< std::uint64_t > E( 20000 , 7 );   // all equal
  check( E , std::vector< std::uint64_t >( { 6 , 7 , 8 } ) ,
         std::less< std::uint64_t >() );
  }
 {
  const double inf = std::numeric_limits< double >::infinity();
  std::vector< double > A = { -inf , -1.5 , 0 , 0 , 2.5 , inf , inf };
  check( A , std::vector< double >( { -inf , -2 , 0 , 1 , 3 , inf } ) ,
         std::less< double >() );
  std::vector< int > R( 1000 );             // a reversed order
  for( int i = 0 ; i < 1000 ; ++i ) R[ i ] = 2000 - 2 * i;
  std::vector< int > QR;
  for( int q = -3 ; q < 2004 ; ++q ) QR.push_back( q );
  check( R , QR , std::greater< int >() );
  std::vector< std::int64_t > N( 777 );     // negative keys, std::less<>
  for( int i = 0 ; i < 777 ; ++i ) N[ i ] = 3 * i - 1000;
  std::vector< std::int64_t > QN;
  for( int q = -1005 ; q < 1340 ; ++q ) QN.push_back( q );
  check( N , QN , std::less<>() );
  }
 const SPlusTree< int > E0;
 CHECK( E0.empty() && ( E0.lowerBound( 5 ) == 0 ) && ( E0.height() == 0 ) );
 std::vector< int > U = { 1 , 3 , 2 };
 CHECK_THROWS( SPlusTree< int >( U ) , contract_violation );
 CHECK_THROWS( (void) branchlessLowerBound( U , 2 ) , contract_violation );
 }

// the selection against std::nth_element on random, sorted, reversed,
// equal, few distinct and organ-pipe inputs of every size up to 100 and of
// 10^4 and 10^5 items (the sampling of Floyd and Rivest), every rank of
// the small ones, doubles, strings with a reversed order, the adversary
// that keeps the pivot at the end (the fallback to the median of the
// medians), and a rank out of the range
ADS_TEST( "nth-element" )
{
 std::mt19937 g( 33 );
 auto check = [ & ]( std::vector< int > A , std::size_t k ) {
  std::vector< int > B = A;
  std::sort( B.begin() , B.end() );
  CHECK( nthElement( A , k ) == B[ k ] );
  std::sort( A.begin() , A.end() );         // the same items
  CHECK( A == B );
  };
 for( std::size_t n = 1 ; n <= 100 ; ++n )
  for( int p = 0 ; p < 6 ; ++p ) {
   std::vector< int > A( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    A[ i ] = p == 0 ? int( g() % 1000 ) : p == 1 ? int( i ) :
             p == 2 ? int( n - i ) : p == 3 ? 5 : p == 4 ? int( g() % 3 ) :
             int( std::min( i , n - 1 - i ) );
   for( std::size_t k = 0 ; k < n ; ++k )
    check( A , k );
   }
 for( std::size_t n : { 10000 , 100000 } )
  for( int p = 0 ; p < 6 ; ++p ) {
   std::vector< int > A( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    A[ i ] = p == 0 ? int( g() ) : p == 1 ? int( i ) :
             p == 2 ? int( n - i ) : p == 3 ? 5 : p == 4 ? int( g() % 3 ) :
             int( std::min( i , n - 1 - i ) );
   for( std::size_t k : { std::size_t( 0 ) , n / 10 , n / 2 , n - 1 } )
    check( A , k );
   }
 {
  std::vector< double > D( 5000 );
  for( auto & d : D ) d = double( g() % 100000 ) / 7;
  auto E = D;
  std::sort( E.begin() , E.end() );
  CHECK( nthElement( D , 1234 ) == E[ 1234 ] );
  std::vector< std::string > S;
  for( int i = 0 ; i < 3000 ; ++i ) S.push_back( std::to_string( g() ) );
  auto T = S;
  std::sort( T.begin() , T.end() , std::greater<>() );
  CHECK( nthElement( S , 100 , std::greater<>() ) == T[ 100 ] );
  }
 {                                          // the pivot always the largest
  std::size_t calls = 0;
  std::vector< int > A( 3000 );
  for( int i = 0 ; i < 3000 ; ++i ) A[ i ] = i;
  auto rigged = [ & ]( int a , int b ) { ++calls; return( a < b ); };
  nthElement( A.begin() , A.begin() , A.end() , rigged );
  CHECK( A[ 0 ] == 0 );
  }
 std::vector< int > Z;
 nthElement( Z.begin() , Z.begin() , Z.end() );   // an empty range
 CHECK_THROWS( nthElement( Z , 0 ) , contract_violation );
 }

// the range minima against a scan: the empty array, one item, sizes around
// the blocks of 64 (63, 64, 65, 128, 129), all the ranges of the small
// arrays, random ranges of 10^5 items, equal items (the leftmost one),
// increasing and decreasing arrays, another order, and a range out of it
ADS_TEST( "word-range-minimum" )
{
 std::mt19937 g( 34 );
 auto scan = []( const std::vector< int > & A , std::size_t i ,
                 std::size_t j , auto less ) {
  std::size_t r = i;
  for( std::size_t t = i + 1 ; t <= j ; ++t )
   if( less( A[ t ] , A[ r ] ) ) r = t;
  return( r );
  };
 for( std::size_t n : { 1 , 2 , 63 , 64 , 65 , 128 , 129 , 200 } )
  for( int p = 0 ; p < 4 ; ++p ) {
   std::vector< int > A( n );
   for( std::size_t i = 0 ; i < n ; ++i )
    A[ i ] = p == 0 ? int( g() % 50 ) : p == 1 ? int( i ) :
             p == 2 ? -int( i ) : 3;
   const WordRangeMinimum< int > R( A );
   const WordRangeMinimum< int , std::greater< int > > M( A ,
                                                std::greater< int >() );
   for( std::size_t i = 0 ; i < n ; ++i )
    for( std::size_t j = i ; j < n ; ++j ) {
     CHECK( R.query( i , j ) == scan( A , i , j , std::less< int >() ) );
     CHECK( M.query( i , j ) == scan( A , i , j , std::greater< int >() ) );
     }
   }
 std::vector< int > A( 100000 );
 for( auto & a : A ) a = int( g() % 1000 );
 const WordRangeMinimum< int > R( A );
 CHECK( ( R.size() == A.size() ) && ( R[ 5 ] == A[ 5 ] ) );
 for( int t = 0 ; t < 2000 ; ++t ) {
  std::size_t i = g() % A.size() , j = g() % A.size();
  if( i > j ) std::swap( i , j );
  CHECK( R.query( i , j ) == scan( A , i , j , std::less< int >() ) );
  }
 const WordRangeMinimum< int > E( std::vector< int >{} );
 CHECK( E.size() == 0 );
 CHECK_THROWS( (void) R.query( 5 , 4 ) , contract_violation );
 CHECK_THROWS( (void) R.query( 0 , A.size() ) , contract_violation );
 }

// the intersection against std::set_intersection: empty lists, one item,
// equal and disjoint lists, lengths around the blocks of 8 (7, 8, 9, 16,
// 17), every ratio of lengths from 1 to 1000 (the merge by blocks and the
// galloping), common items at the ends, the largest and the smallest
// values (signed and unsigned), 64-bit integers and doubles (the merge
// with no branch), the vector form, and lists not sorted
ADS_TEST( "vector-intersection" )
{
 std::mt19937 g( 35 );
 auto check = [ & ]< class T >( const std::vector< T > & A ,
                                const std::vector< T > & B ) {
  std::vector< T > C;
  std::set_intersection( A.begin() , A.end() , B.begin() , B.end() ,
                         std::back_inserter( C ) );
  std::vector< T > out( std::min( A.size() , B.size() ) + 8 );
  const std::size_t k = intersect( A.data() , A.size() , B.data() ,
                                   B.size() , out.data() );
  out.resize( k );
  CHECK( out == C );
  CHECK( intersect( B , A ) == C );
  };
 auto list = [ & ]< class T >( std::size_t n , std::uint64_t u , T ) {
  std::set< T > S;
  while( S.size() < n ) S.insert( T( g() % u ) );
  return( std::vector< T >( S.begin() , S.end() ) );
  };
 check( std::vector< std::uint32_t >() , std::vector< std::uint32_t >() );
 check( std::vector< std::uint32_t >( { 5 } ) ,
        std::vector< std::uint32_t >() );
 check( std::vector< std::uint32_t >( { 5 } ) ,
        std::vector< std::uint32_t >( { 5 } ) );
 for( std::size_t n : { 1 , 7 , 8 , 9 , 16 , 17 , 100 , 1000 } )
  for( std::size_t m : { 1 , 7 , 8 , 9 , 16 , 17 , 100 , 1000 } ) {
   check( list( n , 3 * ( n + m ) , std::uint32_t() ) ,
          list( m , 3 * ( n + m ) , std::uint32_t() ) );
   check( list( n , 3 * ( n + m ) , std::int32_t() ) ,
          list( m , 3 * ( n + m ) , std::int32_t() ) );
   }
 for( std::size_t r : { 1 , 2 , 31 , 32 , 33 , 100 , 1000 } ) {
  const auto A = list( 20000 , 60000 , std::uint32_t() );
  const auto B = list( 20000 / r , 60000 , std::uint32_t() );
  check( A , B );
  }
 {
  std::vector< std::uint32_t > A( 5000 );
  std::iota( A.begin() , A.end() , 0u );
  check( A , A );                           // equal
  std::vector< std::uint32_t > B( 5000 );
  std::iota( B.begin() , B.end() , 5000u );
  check( A , B );                           // disjoint
  B.front() = 4999;                         // one in common, at the ends
  check( A , B );
  const std::vector< std::uint32_t > E = { 0 , 1 , 2 , 3 , 4 , 5 , 6 ,
                                           0xFFFFFFFE , 0xFFFFFFFF };
  check( E , std::vector< std::uint32_t >( { 0 , 0xFFFFFFFF } ) );
  const std::vector< std::int32_t > S = { INT32_MIN , -5 , -1 , 0 , 3 , 9 ,
                                          11 , 12 , INT32_MAX };
  check( S , std::vector< std::int32_t >( { INT32_MIN , -1 , 12 ,
                                            INT32_MAX } ) );
  }
 check( list( 3000 , 9000 , std::uint64_t() ) ,
        list( 2000 , 9000 , std::uint64_t() ) );
 check( std::vector< double >( { -1.5 , 0 , 2 , 7.25 } ) ,
        std::vector< double >( { -2 , 0 , 7.25 , 9 } ) );
 std::vector< int > U = { 3 , 1 } , V = { 1 , 3 };
 CHECK_THROWS( intersect( U , V ) , contract_violation );
 }

// the union of disjoint sets against MFSet on random merges and queries,
// on 1, 2 and 3000 elements, merges of an element with itself, of
// elements already together, chains in increasing and decreasing order,
// the count of the sets, and an element out of the range
ADS_TEST( "union-find" )
{
 std::mt19937 g( 36 );
 for( std::size_t n : { 1 , 2 , 3000 } ) {
  RemUnionFind U( n );
  MFSet M{ int( n ) };
  CHECK( ( U.n() == n ) && ( U.count() == n ) );
  for( int t = 0 ; t < 6000 ; ++t ) {
   const auto x = std::uint32_t( g() % n ) , y = std::uint32_t( g() % n );
   if( g() % 2 )
    CHECK( U.merge( x , y ) == M.merge( int( x ) , int( y ) ) );
   else
    CHECK( U.same( x , y ) == M.same( int( x ) , int( y ) ) );
   CHECK( U.count() == std::size_t( M.count() ) );
   const std::uint32_t r = U.find( x );
   CHECK( U.parent( r ) == r );
   }
  }
 for( int dir = 0 ; dir < 2 ; ++dir ) {     // chains
  const std::uint32_t n = 5000;
  RemUnionFind U( n );
  for( std::uint32_t i = 0 ; i + 1 < n ; ++i ) {
   const std::uint32_t a = dir ? n - 1 - i : i;
   const std::uint32_t b = dir ? a - 1 : a + 1;
   CHECK( U.merge( a , b ) );
   CHECK( ! U.merge( b , a ) );
   }
  CHECK( ( U.count() == 1 ) && U.same( 0 , n - 1 ) );
  CHECK( U.find( 0 ) == U.find( n - 1 ) );
  }
 RemUnionFind U( 4 );
 CHECK( ! U.merge( 2 , 2 ) && ( U.count() == 4 ) );
 RemUnionFind Z( 0 );
 CHECK( Z.count() == 0 );
 CHECK_THROWS( U.find( 4 ) , contract_violation );
 CHECK_THROWS( U.merge( 0 , 4 ) , contract_violation );
 }

// the boolean vector on words against std::set: the dimensions around the
// words of 64 bits (0, 1, 63, 64, 65, 128, 129), the four operations on
// sets of different dimensions, min(), max(), forEach() and the iterators
// on the empty set, on the first and on the last item
ADS_TEST( "bool-set-words" )
{
 std::mt19937 g( 37 );
 auto items = []( const BoolSet & S ) {
  std::vector< long > V;
  S.forEach( [ & ]( long x ) { V.push_back( x ); } );
  CHECK( std::vector< long >( S.begin() , S.end() ) == V );
  return( V );
  };
 for( long N : { 0 , 1 , 63 , 64 , 65 , 128 , 129 } )
  for( long M : { 0 , 1 , 63 , 64 , 65 , 128 , 129 } ) {
   BoolSet A( N ) , B( M );
   std::set< long > a , b;
   for( long t = 0 ; t < N / 2 ; ++t ) {
    const long x = long( g() % std::uint32_t( N ) );
    A.insert( x );
    a.insert( x );
    }
   for( long t = 0 ; t < M / 2 ; ++t ) {
    const long x = long( g() % std::uint32_t( M ) );
    B.insert( x );
    b.insert( x );
    }
   if( N > 0 ) {                            // the two ends
    A.insert( N - 1 );
    a.insert( N - 1 );
    }
   CHECK( items( A ) == std::vector< long >( a.begin() , a.end() ) );
   CHECK( A.size() == a.size() );
   CHECK( A.min() == ( a.empty() ? std::nullopt :
                       std::optional< long >( *a.begin() ) ) );
   CHECK( A.max() == ( a.empty() ? std::nullopt :
                       std::optional< long >( *a.rbegin() ) ) );
   std::vector< long > u , i , d , x;
   std::set_union( a.begin() , a.end() , b.begin() , b.end() ,
                   std::back_inserter( u ) );
   std::set_intersection( a.begin() , a.end() , b.begin() , b.end() ,
                          std::back_inserter( i ) );
   std::set_difference( a.begin() , a.end() , b.begin() , b.end() ,
                        std::back_inserter( d ) );
   std::set_symmetric_difference( a.begin() , a.end() , b.begin() ,
                                  b.end() , std::back_inserter( x ) );
   const BoolSet U = setUnion( A , B ) , I = setIntersection( A , B ) ,
                 D = setDifference( A , B ) ,
                 X = setSymmetricDifference( A , B );
   CHECK( ( items( U ) == u ) && ( U.size() == u.size() ) );
   CHECK( ( items( I ) == i ) && ( I.size() == i.size() ) );
   CHECK( ( items( D ) == d ) && ( D.size() == d.size() ) );
   CHECK( ( items( X ) == x ) && ( X.size() == x.size() ) );
   CHECK( ( U.dim() == std::max( N , M ) ) &&
          ( I.dim() == std::min( N , M ) ) && ( D.dim() == N ) );
   }
 }

// a map from strings to integers T against std::map on random
// insertions, removals and searches: keys on 2 letters (long shared paths,
// keys that are prefixes of others), keys of 0 to 40 bytes on all the 256
// values (null bytes, the four layouts of the nodes growing and
// shrinking), prefixes longer than the bytes kept in a node, the empty
// key, the visits in order by the iterators, forEach() and forPrefix(),
// copies and moves, and all the keys removed
template< class T >
void stringMap( void )
{
 std::mt19937 g( 38 );
 auto key = [ & ]( int sigma , int maxlen , const std::string & base ) {
  std::string k = base;
  for( int i = int( g() % std::uint32_t( maxlen + 1 ) ) ; i > 0 ; --i )
   k += char( sigma == 256 ? int( g() % 256 ) : 'a' + int( g() % 2 ) );
  return( k );
  };
 auto pairs = []( const auto & t ) {
  std::vector< std::pair< std::string , int > > P;
  for( const auto & [ k , v ] : t ) P.emplace_back( std::string( k ) , v );
  return( P );
  };
 for( int round = 0 ; round < 4 ; ++round ) {
  const int sigma = round % 2 ? 256 : 2;
  const std::string base = round >= 2 ? std::string( 20 , 'x' ) : "";
  T t;
  std::map< std::string , int > ref;
  for( int op = 0 ; op < 6000 ; ++op ) {
   const std::string k = key( sigma , sigma == 2 ? 12 : 3 , base );
   const int o = int( g() % 10 );
   if( o < 5 ) {
    CHECK( t.insert( k , op ) == ( ref.find( k ) == ref.end() ) );
    ref[ k ] = op;
    }
   else if( o < 8 )
    CHECK( t.remove( k ) == ( ref.erase( k ) == 1 ) );
   else {
    const auto v = t.lookup( k );
    const auto it = ref.find( k );
    CHECK( v.has_value() == ( it != ref.end() ) );
    if( v ) CHECK( *v == it->second );
    }
   CHECK( t.size() == ref.size() );
   }
  std::vector< std::pair< std::string , int > > R( ref.begin() ,
                                                   ref.end() );
  CHECK( pairs( t ) == R );
  std::vector< std::pair< std::string , int > > F;
  t.forEach( [ & ]( std::string_view k , int v ) {
   F.emplace_back( std::string( k ) , v ); } );
  CHECK( F == R );
  for( int q = 0 ; q < 200 ; ++q ) {        // the prefixes
   const std::string p = key( sigma , sigma == 2 ? 6 : 2 , base );
   std::vector< std::string > E;
   for( auto it = ref.lower_bound( p ) ; ( it != ref.end() ) &&
        ( it->first.compare( 0 , p.size() , p ) == 0 ) ; ++it )
    E.push_back( it->first );
   CHECK( t.keysWithPrefix( p ) == E );
   }
  CHECK( t.keysWithPrefix( "" ).size() == ref.size() );
  auto c = t;                               // a copy is independent
  CHECK( pairs( c ) == R );
  if( ! ref.empty() ) {
   c.remove( ref.begin()->first );
   CHECK( t.contains( ref.begin()->first ) );
   }
  auto m = std::move( c );
  CHECK( c.empty() );
  for( const auto & [ k , v ] : ref )       // all out
   CHECK( t.remove( k ) );
  CHECK( t.empty() && ( t.begin() == t.end() ) );
  }
 T t;
 CHECK( ! t.lookup( "" ) && ! t.remove( "a" ) && ( t.begin() == t.end() ) );
 CHECK( t.insert( "" , 1 ) && t.insert( std::string( "a\0b" , 3 ) , 2 ) );
 CHECK( t.insert( "a" , 3 ) && ! t.insert( "" , 4 ) );
 CHECK( ( *t.lookup( "" ) == 4 ) && ! t.contains( std::string( "a\0" , 2 ) ) );
 CHECK( ( *t.begin() ).first == "" );
 *t.find( "a" ) = 5;
 CHECK( *t.lookup( "a" ) == 5 );
 for( int b = 0 ; b < 256 ; ++b )           // a node of 256 children
  t.insert( std::string( 1 , char( b ) ) + "tail" , b );
 for( int b = 0 ; b < 256 ; b += 2 )
  CHECK( t.remove( std::string( 1 , char( b ) ) + "tail" ) );
 for( int b = 1 ; b < 256 ; b += 2 )
  CHECK( *t.lookup( std::string( 1 , char( b ) ) + "tail" ) == b );
 for( int b = 1 ; b < 256 ; b += 2 ) {      // down to 4 children, and 1
  CHECK( t.remove( std::string( 1 , char( b ) ) + "tail" ) );
  for( int c = b + 2 ; c < 256 ; c += 2 )
   CHECK( *t.lookup( std::string( 1 , char( c ) ) + "tail" ) == c );
  }
 CHECK( ( t.size() == 3 ) && ( *t.lookup( "a" ) == 5 ) );
 }

// the adaptive radix tree [see stringMap()]
ADS_TEST( "adaptive-radix-tree" )
{
 stringMap< AdaptiveRadixTree< int > >();
 }

// the PGM-index against std::lower_bound: the empty array, one item, two,
// random keys of 64 bits (the extremes included), consecutive keys (one
// segment), keys in clusters, many repeated keys (the gallop past the
// window), signed keys, doubles with the infinities, the errors 1 and 1
// (a segment every few keys, many levels), and an unsorted array
ADS_TEST( "pgm-index" )
{
 std::mt19937_64 g( 39 );
 auto check = [ & ]< class I >( const std::vector< typename I::Item > & A ,
                                const std::vector< typename I::Item > & Q ) {
  const I P( A );
  CHECK( P.size() == A.size() );
  for( const auto & q : Q ) {
   const auto r = std::size_t( std::lower_bound( A.begin() , A.end() , q ) -
                               A.begin() );
   CHECK( P.lowerBound( q ) == r );
   CHECK( P.contains( q ) == ( ( r < A.size() ) && !( q < A[ r ] ) ) );
   }
  return( P.segments() );
  };
 using U = std::uint64_t;
 check.operator()< PGMIndex< U > >( {} , { 0 , 5 } );
 check.operator()< PGMIndex< U > >( { 7 } , { 0 , 7 , 8 } );
 check.operator()< PGMIndex< U > >( { 7 , 9 } , { 0 , 7 , 8 , 9 , 10 } );
 for( int t = 0 ; t < 3 ; ++t ) {
  std::vector< U > A( 50000 ) , Q;
  for( std::size_t i = 0 ; i < A.size() ; ++i )
   A[ i ] = t == 0 ? g() : t == 1 ? 1000 + i :
            ( i / 500 ) * 1000000 + g() % 1000;   // clusters
  A.push_back( 0 );
  A.push_back( ~U( 0 ) );
  std::sort( A.begin() , A.end() );
  for( int q = 0 ; q < 20000 ; ++q )
   Q.push_back( q % 2 ? A[ g() % A.size() ] + ( q % 4 == 1 ? 0 : 1 ) :
                g() % ( A[ A.size() - 2 ] + 2 ) );
  Q.push_back( 0 );
  Q.push_back( ~U( 0 ) );
  const std::size_t s = check.operator()< PGMIndex< U > >( A , Q );
  if( t == 1 ) CHECK( s <= 3 );             // a line, and the extremes
  check.operator()< PGMIndex< U , 1 , 1 > >( A , Q );
  }
 {                                          // repeated keys
  std::vector< U > A;
  for( U k = 0 ; k < 50 ; ++k )
   A.insert( A.end() , std::size_t( 1 + g() % 3000 ) , 10 * k );
  std::vector< U > Q;
  for( U q = 0 ; q < 510 ; ++q ) Q.push_back( q );
  check.operator()< PGMIndex< U , 4 , 2 > >( A , Q );
  check.operator()< PGMIndex< U > >( A , Q );
  }
 {                                          // signed and floating keys
  std::vector< std::int32_t > A( 20000 ) , Q;
  for( auto & a : A ) a = std::int32_t( g() );
  std::sort( A.begin() , A.end() );
  for( int q = 0 ; q < 5000 ; ++q ) Q.push_back( std::int32_t( g() ) );
  Q.push_back( INT32_MIN );
  Q.push_back( INT32_MAX );
  check.operator()< PGMIndex< std::int32_t , 8 , 2 > >( A , Q );
  const double inf = std::numeric_limits< double >::infinity();
  std::vector< double > D = { -inf , -3.5 , -0.0 , 0.0 , 1e-300 , 2.25 ,
                              1e300 , inf };
  for( int i = 0 ; i < 3000 ; ++i ) D.push_back( double( g() % 100000 ) /
                                                 7 - 5000 );
  std::sort( D.begin() , D.end() );
  check.operator()< PGMIndex< double , 2 , 1 > >( D , { -inf , -1e301 ,
                    -3.5 , 0.0 , 1.0 , 2.25 , 1e299 , inf , -0.5 } );
  }
 std::vector< U > W = { 3 , 1 , 2 };
 CHECK_THROWS( PGMIndex< U >( W ) , contract_violation );
 }

// the Roaring bitmap against std::set: the kernels of two arrays of every
// length up to 70 (the blocks of 8 and their tails) on few and many values,
// sets of few and of many items per
// chunk (the arrays and the bitmaps), the four operations on every pair of
// them (results that change kind both ways), insertions and removals that
// cross the 4096 items of an array, the items 0, 65535, 65536 and the
// largest one, the iterators and forEach(), copies, and items not sorted
ADS_TEST( "roaring-bitmap" )
{
 {                                          // the kernels of the arrays
  std::mt19937_64 h( 41 );
  auto list = [ & ]( std::size_t n , unsigned u ) {
   std::set< std::uint16_t > S;
   while( S.size() < n ) S.insert( std::uint16_t( h() % u ) );
   return( std::vector< std::uint16_t >( S.begin() , S.end() ) );
   };
  for( std::size_t na = 0 ; na < 70 ; na += 1 + na / 8 )
   for( std::size_t nb = 0 ; nb < 70 ; nb += 1 + nb / 8 )
    for( unsigned u : { 80u , 400u , 65536u } ) {
     if( ( na > u ) || ( nb > u ) ) continue;
     for( int rep = 0 ; rep < 3 ; ++rep ) {
      const auto A = list( na , u ) , B = list( nb , u );
      std::vector< std::uint16_t > E , O( na + nb + 8 );
      std::set_union( A.begin() , A.end() , B.begin() , B.end() ,
                      std::back_inserter( E ) );
      O.resize( detail::union16( A.data() , na , B.data() , nb ,
                                 O.data() ) );
      CHECK( O == E );
      E.clear();
      O.assign( na + 8 , 0 );
      std::set_difference( A.begin() , A.end() , B.begin() , B.end() ,
                           std::back_inserter( E ) );
      O.resize( detail::difference16( A.data() , na , B.data() , nb ,
                                      O.data() ) );
      CHECK( O == E );
      E.clear();
      O.assign( std::min( na , nb ) + 8 , 0 );
      std::set_intersection( A.begin() , A.end() , B.begin() , B.end() ,
                             std::back_inserter( E ) );
      O.resize( detail::intersect16( A.data() , na , B.data() , nb ,
                                     O.data() ) );
      CHECK( O == E );
      }
     }
  }
 std::mt19937_64 g( 40 );
 auto make = [ & ]( int kind ) {
  std::set< std::uint32_t > S;
  const std::size_t n = kind == 0 ? 3000 : kind == 1 ? 30000 : 9000;
  while( S.size() < n ) {
   std::uint32_t x;
   if( kind == 0 ) x = std::uint32_t( g() );           // sparse
   else if( kind == 1 ) x = std::uint32_t( g() % 160000 );   // dense
   else x = std::uint32_t( ( g() % 3 ) << 16 | ( g() % 9000 ) );
   S.insert( x );
   }
  for( std::uint32_t x : { 0u , 65535u , 65536u , 0xFFFFFFFFu } )
   S.insert( x );
  return( S );
  };
 auto items = []( const RoaringBitmap & R ) {
  std::vector< std::uint32_t > V( R.begin() , R.end() );
  CHECK( V == R.items() );
  CHECK( V.size() == R.size() );
  return( V );
  };
 std::vector< std::set< std::uint32_t > > S;
 std::vector< RoaringBitmap > R;
 for( int k = 0 ; k < 3 ; ++k ) {
  S.push_back( make( k ) );
  const std::vector< std::uint32_t > V( S.back().begin() , S.back().end() );
  R.emplace_back( V.data() , V.size() );
  CHECK( items( R.back() ) == V );
  }
 CHECK( R[ 1 ].bitmaps() > 0 );
 for( int a = 0 ; a < 3 ; ++a )
  for( int b = 0 ; b < 3 ; ++b ) {
   std::vector< std::uint32_t > u , i , d , x;
   std::set_union( S[ a ].begin() , S[ a ].end() , S[ b ].begin() ,
                   S[ b ].end() , std::back_inserter( u ) );
   std::set_intersection( S[ a ].begin() , S[ a ].end() , S[ b ].begin() ,
                          S[ b ].end() , std::back_inserter( i ) );
   std::set_difference( S[ a ].begin() , S[ a ].end() , S[ b ].begin() ,
                        S[ b ].end() , std::back_inserter( d ) );
   std::set_symmetric_difference( S[ a ].begin() , S[ a ].end() ,
                                  S[ b ].begin() , S[ b ].end() ,
                                  std::back_inserter( x ) );
   CHECK( items( setUnion( R[ a ] , R[ b ] ) ) == u );
   CHECK( items( setIntersection( R[ a ] , R[ b ] ) ) == i );
   CHECK( items( setDifference( R[ a ] , R[ b ] ) ) == d );
   CHECK( items( setSymmetricDifference( R[ a ] , R[ b ] ) ) == x );
   }
 {                                          // across 4096, both ways
  RoaringBitmap T;
  std::set< std::uint32_t > ref;
  for( int t = 0 ; t < 30000 ; ++t ) {
   const std::uint32_t v = std::uint32_t( 70000 + g() % 9000 );
   if( t < 15000 || g() % 2 ) {
    CHECK( T.insert( v ) == ref.insert( v ).second );
    }
   else
    CHECK( T.remove( v ) == ( ref.erase( v ) == 1 ) );
   CHECK( T.size() == ref.size() );
   }
  for( std::uint32_t v = 69990 ; v < 79010 ; ++v )
   CHECK( T.contains( v ) == ( ref.count( v ) == 1 ) );
  CHECK( items( T ) == std::vector< std::uint32_t >( ref.begin() ,
                                                     ref.end() ) );
  for( auto v : std::vector< std::uint32_t >( ref.begin() , ref.end() ) )
   CHECK( T.remove( v ) );
  CHECK( T.empty() && ( T.containers() == 0 ) && ( T.begin() == T.end() ) );
  }
 RoaringBitmap C = R[ 1 ];                  // a copy is independent
 C.remove( 0 );
 CHECK( R[ 1 ].contains( 0 ) && ! C.contains( 0 ) );
 RoaringBitmap E;
 CHECK( E.empty() && setIntersection( E , R[ 0 ] ).empty() );
 CHECK( setUnion( E , R[ 2 ] ).size() == R[ 2 ].size() );
 const std::vector< std::uint32_t > U = { 5 , 3 };
 CHECK_THROWS( RoaringBitmap( U.data() , U.size() ) , contract_violation );
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
/*--------------------------- End File ordered.cpp -------------------------*/
/*--------------------------------------------------------------------------*/
