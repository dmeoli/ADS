/*--------------------------------------------------------------------------*/
/*------------------------ File exam_backtracka.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems 1 to 27 of a collection of problems of enumeration by
 * backtrack (the problems 28 to 47 are in exam_backtrackb.cpp), each with the
 * algorithm of its solution and a test against an independent oracle: a brute
 * force over all the candidates (bit masks, tuples, permutations), a count by
 * dynamic programming or by a closed form, the examples of the statements
 * with their outputs and the edge cases. Where the library already has the
 * algorithm (coloring, nested parentheses, Hamiltonian paths, the numbers of
 * Fibonacci, subsets and permutations) the test calls it. The algorithms are
 * in the namespace backtrack, one per problem, each with its idea, its
 * complexity and its preconditions; a function marked as a candidate for the
 * library is general and missing from it.
 *
 * The contracts [see Contract.h]: each function checks the preconditions
 * of its statement (distinct items, square or symmetric matrices, the
 * ranges of the parameters) with ADS_PRE; the tests run at the level 3.
 *
 * \see A. Montresor, Esercizi Backtrack, Raccolte da compiti scritti,
 *      Algoritmicamente, https://algoritmicamente.it
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

#include "ADS/Contract.h"
#include "ADS/backtrack/Enumeration.h"
#include "ADS/combinatorial/Generation.h"
#include "ADS/complexity/Problems.h"
#include "ADS/numeric/Combinatorics.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

/*--------------------------------------------------------------------------*/
/*----------------------------- ALGORITHMS ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace backtrack {

using Grid = std::vector< std::vector< int > >;     ///< a matrix of cells
using Weights = std::vector< std::vector< int > >;  ///< 0 for no arc
using Cell = std::pair< int , int >;                ///< ( row , column )

namespace detail {

/// the items of A are distinct
inline bool distinct( std::vector< int > A )
{
 std::sort( A.begin() , A.end() );
 return( std::adjacent_find( A.begin() , A.end() ) == A.end() );
 }

/// M is a square matrix
template< class M >
bool square( const M & G )
{
 for( const auto & row : G )
  if( row.size() != G.size() ) return( false );
 return( true );
 }

/// G is square and symmetric, i.e., an unoriented graph
template< class M >
bool symmetric( const M & G )
{
 if( ! square( G ) ) return( false );
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = 0 ; v < G.size() ; ++v )
   if( G[ u ][ v ] != G[ v ][ u ] ) return( false );
 return( true );
 }

/// the primes <= n, increasing, by the sieve of Eratosthenes
inline std::vector< int > sieve( int n )
{
 std::vector< bool > composite( std::size_t( std::max( n + 1 , 2 ) ) ,
                                false );
 std::vector< int > P;
 for( int i = 2 ; i <= n ; ++i ) {
  if( composite[ i ] ) continue;
  P.push_back( i );
  for( long j = long( i ) * i ; j <= n ; j += i ) composite[ j ] = true;
  }
 return( P );
 }

}  // end( namespace detail )

/// a binary tree in arrays, the children -1 when nil
struct Tree {
 std::vector< int > value;   ///< the value of each node
 std::vector< int > left;    ///< the left child of each node
 std::vector< int > right;   ///< the right child of each node
 int root = -1;              ///< the root, -1 if empty

 /// a new node of value v with children l and r, its index
 int add( int v , int l = -1 , int r = -1 ) {
  value.push_back( v );
  left.push_back( l );
  right.push_back( r );
  return( int( value.size() ) - 1 );
  }

 /// t has no children
 bool leaf( int t ) const {
  return( ( left[ t ] < 0 ) && ( right[ t ] < 0 ) );
  }
 };

/*--------------------------------------------------------------------------*/
/* 4. 2018-06-04, Print sums                                                */
/*--------------------------------------------------------------------------*/

/// printSums(): the subsets of the distinct items X (also negative) of sum
/// w, f( Y ) on the items of each in the order of X; a 0/1 mask chosen item
/// by item, the sum tested only at the leaves, so that each subset comes
/// once; O( n 2^n )
/// \pre the items of X are distinct
template< class F >
void printSums( const std::vector< int > & X , int w , F && f )
{
 ADS_PRE( detail::distinct( X ) );
 std::vector< int > S( X.size() , 0 );
 auto rec = [ & ]( auto & self , std::size_t i , long long v ) -> void {
  if( i == X.size() ) {
   if( v == 0 ) {
    std::vector< int > Y;
    for( std::size_t j = 0 ; j < X.size() ; ++j )
     if( S[ j ] ) Y.push_back( X[ j ] );
    f( Y );
    }
   return;
   }
  for( int c = 0 ; c <= 1 ; ++c ) {
   S[ i ] = c;
   self( self , i + 1 , v - c * (long long)( X[ i ] ) );   // 64 bits
   }
  };
 rec( rec , 0 , w );
 }

/*--------------------------------------------------------------------------*/
/* 5. 2018-06-28, Print all, and 27. 2022-07-13, Stampa con spazi           */
/*--------------------------------------------------------------------------*/

/// printAll() and printWithSpaces(): the 2^{n-1} ways to write S with sep
/// or nothing between two consecutive characters, f( T ) on each; a boolean
/// stop[ i ] for each place before S[ i ], i >= 1, chosen from the last
/// one; \f$ \Theta( n 2^n ) \f$
/// \pre S is not empty
template< class F >
void printSeparated( const std::string & S , const std::string & sep ,
                     F && f )
{
 ADS_PRE( ! S.empty() );
 const int n = int( S.size() );
 std::vector< bool > stop( std::size_t( n ) , false );
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == 0 ) {
   std::string T( 1 , S[ 0 ] );
   for( int j = 1 ; j < n ; ++j ) {
    if( stop[ j ] ) T += sep;
    T += S[ j ];
    }
   f( T );
   return;
   }
  for( bool c : { true , false } ) {
   stop[ i ] = c;
   self( self , i - 1 );
   }
  };
 rec( rec , n - 1 );
 }

/*--------------------------------------------------------------------------*/
/* 6. 2018-07-25, Visit                                                     */
/*--------------------------------------------------------------------------*/

/// visit(): the simple paths of k edges from s, f( path ) on each; a depth
/// first visit that marks a node while it is on the path and unmarks it
/// when it leaves, so that other paths can use it; O( n^k k ) on a
/// complete graph, whose paths are ( n - 1 )! / ( n - 1 - k )!
/// \pre G is square, s is a node, k >= 0
template< class F >
void visit( const Adjacency & G , int s , int k , F && f )
{
 const int n = int( G.size() );
 ADS_PRE( detail::square( G ) );
 ADS_PRE( ( s >= 0 ) && ( s < n ) && ( k >= 0 ) );
 std::vector< bool > visited( std::size_t( n ) , false );
 std::vector< int > path( std::size_t( k ) + 1 );
 auto rec = [ & ]( auto & self , int u , int i ) -> void {
  path[ i ] = u;
  if( i == k ) {
   f( path );
   return;
   }
  visited[ u ] = true;
  for( int v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] && ! visited[ v ] ) self( self , v , i + 1 );
  visited[ u ] = false;
  };
 rec( rec , s , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 7. 2018-08-21, n-colorazione con gap                                     */
/*--------------------------------------------------------------------------*/

/// gapColoring(): a bijection color of the nodes on [ 1 , n ] with
/// | color[ u ] - color[ v ] | >= 2 on each edge, or nothing; the nodes
/// colored in the order of their index, each unused color tried when no
/// colored neighbor is closer than 2; O( n! n^2 ). The text moves only to
/// the uncolored neighbors of the node just colored, which fails when that
/// node has none left but other nodes are uncolored (a path with node 0 in
/// the middle): here the next node is always the next index.
/// \pre G is unoriented, i.e., square and symmetric
inline std::optional< std::vector< int > > gapColoring( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const int n = int( G.size() );
 std::vector< int > color( std::size_t( n ) , -1 );
 std::vector< bool > used( std::size_t( n ) + 1 , false );
 auto rec = [ & ]( auto & self , int u ) -> bool {
  if( u == n ) return( true );
  for( int c = 1 ; c <= n ; ++c ) {
   if( used[ c ] ) continue;
   bool ok = true;
   for( int v = 0 ; v < n && ok ; ++v )
    ok = ! ( G[ u ][ v ] && ( color[ v ] > 0 ) &&
             ( std::abs( color[ v ] - c ) < 2 ) );
   if( ! ok ) continue;
   color[ u ] = c;
   used[ c ] = true;
   if( self( self , u + 1 ) ) return( true );
   color[ u ] = -1;
   used[ c ] = false;
   }
  return( false );
  };
 if( ! rec( rec , 0 ) ) return( std::nullopt );
 return( color );
 }

/// the scheme of the text, which recurses only on the uncolored neighbors
/// of the node just colored, starting from node 0: sound, not complete
/// \pre G is unoriented, i.e., square and symmetric
inline bool gapColoringText( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const int n = int( G.size() );
 std::set< int > C;
 for( int c = 1 ; c <= n ; ++c ) C.insert( c );
 std::vector< int > color( std::size_t( n ) , -1 );
 auto rec = [ & ]( auto & self , int u ) -> bool {
  const std::set< int > choices = C;
  for( int c : choices ) {
   bool ok = true;
   for( int v = 0 ; v < n ; ++v )
    if( G[ u ][ v ] && ( color[ v ] > 0 ) &&
        ( std::abs( color[ v ] - c ) < 2 ) )
     ok = false;
   if( ok ) {
    color[ u ] = c;
    C.erase( c );
    if( C.empty() ) return( true );
    for( int v = 0 ; v < n ; ++v )
     if( G[ u ][ v ] && ( color[ v ] < 0 ) && self( self , v ) )
      return( true );
    C.insert( c );
    color[ u ] = -1;
    }
   }
  return( false );
  };
 return( ( n > 0 ) && rec( rec , 0 ) );
 }

/*--------------------------------------------------------------------------*/
/* 8. 2019-01-21, All sums rec                                              */
/*--------------------------------------------------------------------------*/

/// allSums(): the multisets of the values of A, each usable many times, of
/// sum v, f( S ) on each; from the last value, either skip A[ i ] or take
/// it and stay on i; O( v 2^{v + n} ), an upper bound
/// \pre the items of A are distinct and positive, v >= 0
template< class F >
void allSums( const std::vector< int > & A , int v , F && f )
{
 ADS_PRE( detail::distinct( A ) && ( v >= 0 ) );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( int x ) { return( x > 0 ); } ) );
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int target , int i ) -> void {
  if( target == 0 )
   f( S );
  else
   if( ( target > 0 ) && ( i > 0 ) ) {
    self( self , target , i - 1 );
    S.push_back( A[ i - 1 ] );
    self( self , target - A[ i - 1 ] , i );
    S.pop_back();
    }
  };
 rec( rec , v , int( A.size() ) );
 }

/*--------------------------------------------------------------------------*/
/* 9. 2019-02-07, Print all                                                 */
/*--------------------------------------------------------------------------*/

/// printAll(): the increasing positions i_1 < ... < i_m (from 1) with
/// T[ i_k ] = P[ k ], i.e., the occurrences of P as a subsequence of T,
/// f( I ) on each; on the prefixes T[ 1 .. i ] and P[ 1 .. j ], skip T[ i ]
/// or match it to P[ j ], pruned when i < j; O( 2^n )
template< class F >
void printAll( const std::string & T , const std::string & P , F && f )
{
 std::vector< int > S;                     // a stack, the top the smallest
 auto rec = [ & ]( auto & self , int i , int j ) -> void {
  if( j == 0 ) {
   f( std::vector< int >( S.rbegin() , S.rend() ) );
   return;
   }
  if( i >= j ) {
   self( self , i - 1 , j );
   if( T[ i - 1 ] == P[ j - 1 ] ) {
    S.push_back( i );
    self( self , i - 1 , j - 1 );
    S.pop_back();
    }
   }
  };
 rec( rec , int( T.size() ) , int( P.size() ) );
 }

/*--------------------------------------------------------------------------*/
/* 10. 2019-06-06, Print flags                                              */
/*--------------------------------------------------------------------------*/

/// printFlags(): the sequences of n colors in [ 1 , k ], two consecutive
/// ones compatible for A, f( S ) on each; the first stripe free, the other
/// ones among the colors compatible with the previous one; O( n k^n )
/// \pre A is symmetric with a false diagonal, n >= 0
template< class F >
void printFlags( const Adjacency & A , int n , F && f )
{
 ADS_PRE( detail::symmetric( A ) && ( n >= 0 ) );
 const int k = int( A.size() );
 for( int c = 0 ; c < k ; ++c ) ADS_PRE( ! A[ c ][ c ] );
 std::vector< int > S( std::size_t( n ) , 0 );
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == n ) {
   f( S );
   return;
   }
  for( int j = 1 ; j <= k ; ++j )
   if( ( i == 0 ) || A[ j - 1 ][ S[ i - 1 ] - 1 ] ) {
    S[ i ] = j;
    self( self , i + 1 );
    }
  };
 rec( rec , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 12. 2019-07-18, k-alternato                                              */
/*--------------------------------------------------------------------------*/

/// isKAlternate(): every strictly monotone run of S has at most k items;
/// countInc and countDec count the current increasing and decreasing runs.
/// The text lacks the else between the two cases.
/// \pre the items of S are distinct
inline bool isKAlternate( const std::vector< int > & S , int k )
{
 ADS_PRE( detail::distinct( S ) );
 int countInc = 1 , countDec = 1;
 for( std::size_t i = 1 ; i < S.size() ; ++i )
  if( S[ i - 1 ] < S[ i ] ) {
   if( ++countInc > k ) return( false );
   countDec = 1;
   }
  else {
   if( ++countDec > k ) return( false );
   countInc = 1;
   }
 return( true );
 }

/// kAlternate() by enumeration: the permutations of A that pass
/// isKAlternate(), by the permutations of the library; O( n n! )
/// \pre A is not empty with distinct items, k >= 2
inline long kAlternateAll( const std::vector< int > & A , int k )
{
 ADS_PRE( ! A.empty() && detail::distinct( A ) && ( k >= 2 ) );
 long tot = 0;
 permutations( A , [ & ]( const std::vector< int > & S ) {
  if( isKAlternate( S , k ) ) ++tot;
  return( false ); } );
 return( tot );
 }

/// kAlternate(): the permutations of A whose strictly monotone runs have at
/// most k items; the first item free, then an item only if it does not
/// make a run of k + 1; O( n! ). The text passes A.size() - 1 items still
/// to place after the first one, one less than the A.size() left.
/// \pre A is not empty with distinct items, k >= 2
inline long kAlternate( const std::vector< int > & A , int k )
{
 ADS_PRE( ! A.empty() && detail::distinct( A ) && ( k >= 2 ) );
 const int n = int( A.size() );
 std::vector< bool > used( std::size_t( n ) , false );
 auto rec = [ & ]( auto & self , int prev , int i , int countInc ,
                   int countDec ) -> long {
  if( i == 0 ) return( 1 );
  long tot = 0;
  for( int c = 0 ; c < n ; ++c ) {
   if( used[ c ] ) continue;
   used[ c ] = true;
   if( ( prev < A[ c ] ) && ( countInc < k ) )
    tot += self( self , A[ c ] , i - 1 , countInc + 1 , 1 );
   if( ( prev > A[ c ] ) && ( countDec < k ) )
    tot += self( self , A[ c ] , i - 1 , 1 , countDec + 1 );
   used[ c ] = false;
   }
  return( tot );
  };
 long tot = 0;
 for( int c = 0 ; c < n ; ++c ) {
  used[ c ] = true;
  tot += rec( rec , A[ c ] , n - 1 , 1 , 1 );
  used[ c ] = false;
  }
 return( tot );
 }

/*--------------------------------------------------------------------------*/
/* 13. 2020-01-14, Longest path                                             */
/*--------------------------------------------------------------------------*/

/// longestPath(): the most cells of a path of distinct adjacent cells of
/// value 1 from the cell ( 0 , 0 ), 0 if it is 0; a depth first visit that
/// marks a cell -1 while it is on the path; O( 4^{nm} ). The text calls
/// the recursion with n and m swapped.
/// \pre M is a rectangular matrix of 0 and 1
inline int longestPath( Grid M )
{
 const int n = int( M.size() ) , m = n ? int( M[ 0 ].size() ) : 0;
 for( const auto & row : M ) {
  ADS_PRE( int( row.size() ) == m );
  for( int x : row ) ADS_PRE( ( x == 0 ) || ( x == 1 ) );
  }
 auto rec = [ & ]( auto & self , int i , int j ) -> int {
  if( ( i < 0 ) || ( i >= n ) || ( j < 0 ) || ( j >= m ) ||
      ( M[ i ][ j ] != 1 ) )
   return( 0 );
  M[ i ][ j ] = -1;
  const int best = 1 + std::max( { self( self , i + 1 , j ) ,
                                   self( self , i , j + 1 ) ,
                                   self( self , i - 1 , j ) ,
                                   self( self , i , j - 1 ) } );
  M[ i ][ j ] = 1;
  return( best );
  };
 return( rec( rec , 0 , 0 ) );
 }

/*--------------------------------------------------------------------------*/
/* 14. 2020-06-12, Gen rec                                                  */
/*--------------------------------------------------------------------------*/

/// generate(): the palindromic compositions of n, f( a ) on each; the first
/// half S chosen part by part, each part j taking 2 j of what is missing,
/// and at every call S, then what is missing if positive, then S reversed;
/// one output per call, \f$ 2^{\lfloor n / 2 \rfloor} \f$ of them,
/// \f$ O( n 2^{n / 2} ) \f$
/// \pre n >= 0
template< class F >
void generate( int n , F && f )
{
 ADS_PRE( n >= 0 );
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int missing ) -> void {
  std::vector< int > a( S );
  if( missing > 0 ) a.push_back( missing );
  a.insert( a.end() , S.rbegin() , S.rend() );
  f( a );
  for( int j = 1 ; j <= missing / 2 ; ++j ) {
   S.push_back( j );
   self( self , missing - 2 * j );
   S.pop_back();
   }
  };
 rec( rec , n );
 }

/// the compositions of even length only, the simplified variant: an output
/// only when nothing is missing, none for n odd
/// \pre n >= 0
template< class F >
void generateEven( int n , F && f )
{
 ADS_PRE( n >= 0 );
 if( n % 2 ) return;
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int missing ) -> void {
  if( missing == 0 ) {
   std::vector< int > a( S );
   a.insert( a.end() , S.rbegin() , S.rend() );
   f( a );
   }
  for( int j = 1 ; j <= missing / 2 ; ++j ) {
   S.push_back( j );
   self( self , missing - 2 * j );
   S.pop_back();
   }
  };
 rec( rec , n );
 }

/*--------------------------------------------------------------------------*/
/* 15. 2020-07-03, Print positive                                           */
/*--------------------------------------------------------------------------*/

/// printPositive(): the root-to-leaf paths of T whose values are all
/// positive, f( path ) on the values of each; a depth first visit with a
/// stack that skips the subtree of a value <= 0; O( n h )
template< class F >
void printPositive( const Tree & T , F && f )
{
 std::vector< int > path;
 auto rec = [ & ]( auto & self , int t ) -> void {
  if( ( t < 0 ) || ( T.value[ t ] <= 0 ) ) return;
  path.push_back( T.value[ t ] );
  if( T.leaf( t ) )
   f( path );
  else {
   self( self , T.left[ t ] );
   self( self , T.right[ t ] );
   }
  path.pop_back();
  };
 rec( rec , T.root );
 }

/*--------------------------------------------------------------------------*/
/* 16. 2020-07-24, Print hamilton                                           */
/*--------------------------------------------------------------------------*/

/// printHamilton(): the oriented Hamiltonian cycles of G, each once, f( P )
/// on its nodes from node 0; a depth first visit from the fixed node 0 that
/// closes the cycle when all the nodes are on the path and the last one
/// has an arc to node 0; O( n! ) on a complete graph, with ( n - 1 )!
/// cycles. With one node the only cycle is the loop G[ 0 ][ 0 ], if any
/// (hamiltonianCycle() of the library asks n >= 2), with none no cycle
/// \pre G is square
template< class F >
void printHamilton( const Adjacency & G , F && f )
{
 ADS_PRE( detail::square( G ) );
 const int n = int( G.size() );
 if( n == 0 ) return;
 std::vector< bool > visited( std::size_t( n ) , false );
 std::vector< int > path( std::size_t( n ) , 0 );
 auto rec = [ & ]( auto & self , int u , int i ) -> void {
  path[ i ] = u;
  if( i == n - 1 ) {
   if( G[ u ][ path[ 0 ] ] ) f( path );
   return;
   }
  visited[ u ] = true;
  for( int v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] && ! visited[ v ] ) self( self , v , i + 1 );
  visited[ u ] = false;
  };
 rec( rec , 0 , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 17. 2020-09-04, Print increasing                                         */
/*--------------------------------------------------------------------------*/

/// printIncreasing(): the strictly increasing subsequences of A, the empty
/// one included, f( S ) on each; from the last item with a stack, A[ i ]
/// taken only if smaller than the top; O( n 2^n )
template< class F >
void printIncreasing( const std::vector< int > & A , F && f )
{
 std::vector< int > S;                     // a stack, the top the first
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == 0 ) {
   f( std::vector< int >( S.rbegin() , S.rend() ) );
   return;
   }
  if( S.empty() || ( A[ i - 1 ] < S.back() ) ) {
   S.push_back( A[ i - 1 ] );
   self( self , i - 1 );
   S.pop_back();
   }
  self( self , i - 1 );
  };
 rec( rec , int( A.size() ) );
 }

/*--------------------------------------------------------------------------*/
/* 19. 2021-02-08, Somma di primi                                           */
/*--------------------------------------------------------------------------*/

/// primes(): the sets of k distinct primes of sum n, f( S ) on each,
/// increasing; for the prime P[ p ], from the largest, skip it or put it
/// in S[ k ], while p , k , n > 0; \f$ O( \binom{p}{k} ) = O( p^k ) \f$
/// \pre n , k >= 0
template< class F >
void primes( int n , int k , F && f )
{
 ADS_PRE( ( n >= 0 ) && ( k >= 0 ) );
 const std::vector< int > P = detail::sieve( n );
 std::vector< int > S( std::size_t( k ) , 0 );
 auto rec = [ & ]( auto & self , int p , int kk , int nn ) -> void {
  if( ( nn == 0 ) && ( kk == 0 ) )
   f( S );
  else
   if( ( p > 0 ) && ( kk > 0 ) && ( nn > 0 ) ) {
    self( self , p - 1 , kk , nn );
    S[ kk - 1 ] = P[ p - 1 ];
    self( self , p - 1 , kk - 1 , nn - P[ p - 1 ] );
    }
  };
 rec( rec , int( P.size() ) , k , n );
 }

/*--------------------------------------------------------------------------*/
/* 20. 2021-06-16, Stampa percorsi                                          */
/*--------------------------------------------------------------------------*/

/// printPaths(): the monotone paths of an n x n grid as strings of n D
/// and n R, f( S ) on each; a D while some are left, then an R while some
/// are left; \f$ O( n \binom{2n}{n} ) = O( n 4^n ) \f$
/// \pre n >= 0
template< class F >
void printPaths( int n , F && f )
{
 ADS_PRE( n >= 0 );
 std::string S( std::size_t( 2 * n ) , ' ' );
 auto rec = [ & ]( auto & self , int i , int nd , int nr ) -> void {
  if( ( nd == 0 ) && ( nr == 0 ) ) {
   f( S );
   return;
   }
  if( nd > 0 ) {
   S[ i ] = 'D';
   self( self , i + 1 , nd - 1 , nr );
   }
  if( nr > 0 ) {
   S[ i ] = 'R';
   self( self , i + 1 , nd , nr - 1 );
   }
  };
 rec( rec , 0 , n , n );
 }

/*--------------------------------------------------------------------------*/
/* 21. 2021-07-26, PrintBits                                                */
/*--------------------------------------------------------------------------*/

/// printBits(): the strings of n bits without two consecutive 1, f( S ) on
/// each; a 1 only after a 0 or at the start, a 0 always;
/// \f$ O( n F_{n + 2} ) \f$
/// \pre n >= 0
template< class F >
void printBits( int n , F && f )
{
 ADS_PRE( n >= 0 );
 std::string S( std::size_t( n ) , '0' );
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == n ) {
   f( S );
   return;
   }
  if( ( i == 0 ) || ( S[ i - 1 ] != '1' ) ) {
   S[ i ] = '1';
   self( self , i + 1 );
   }
  S[ i ] = '0';
  self( self , i + 1 );
  };
 rec( rec , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 22. 2021-08-31, Sequenza k-limitata                                      */
/*--------------------------------------------------------------------------*/

/// printKSequence(): the subsequences of A whose consecutive items differ
/// by at most k, the empty one included, f( S ) on each; from the last item
/// with a stack, A[ i ] taken only if within k of the top; O( n 2^n ). The
/// difference is taken in long long, since that of two int may overflow
/// \pre k >= 0
template< class F >
void printKSequence( const std::vector< int > & A , int k , F && f )
{
 ADS_PRE( k >= 0 );
 std::vector< int > S;                     // a stack, the top the first
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == 0 ) {
   f( std::vector< int >( S.rbegin() , S.rend() ) );
   return;
   }
  if( S.empty() || ( std::llabs( (long long)( A[ i - 1 ] ) - S.back() ) <=
                     k ) ) {
   S.push_back( A[ i - 1 ] );
   self( self , i - 1 );
   S.pop_back();
   }
  self( self , i - 1 );
  };
 rec( rec , int( A.size() ) );
 }

/*--------------------------------------------------------------------------*/
/* 23. 2021-08-31, PrintBitsK                                               */
/*--------------------------------------------------------------------------*/

/// printBits( n , k ): the strings of n bits with at most k consecutive 1,
/// f( S ) on each; filled from the last bit, c1 the 1 still allowed in a
/// row, back to k after a 0; O( n 2^n )
/// \pre n , k >= 0
template< class F >
void printBitsK( int n , int k , F && f )
{
 ADS_PRE( ( n >= 0 ) && ( k >= 0 ) );
 std::string S( std::size_t( n ) , '0' );
 auto rec = [ & ]( auto & self , int i , int c1 ) -> void {
  if( i == 0 ) {
   f( S );
   return;
   }
  S[ i - 1 ] = '0';
  self( self , i - 1 , k );
  if( c1 > 0 ) {
   S[ i - 1 ] = '1';
   self( self , i - 1 , c1 - 1 );
   }
  };
 rec( rec , n , k );
 }

/*--------------------------------------------------------------------------*/
/* 24. 2022-01-17, Ottali                                                   */
/*--------------------------------------------------------------------------*/

/// octals(): the strings of n octal digits, no two consecutive ones equal,
/// f( S ) on each; filled from the last digit, each digit but the previous
/// one; \f$ 8 \cdot 7^{n - 1} \f$ strings, \f$ \Theta( n 7^n ) \f$
/// \pre n >= 0
template< class F >
void octals( int n , F && f )
{
 ADS_PRE( n >= 0 );
 std::string S( std::size_t( n ) , '0' );
 auto rec = [ & ]( auto & self , int i , int prev ) -> void {
  if( i == 0 ) {
   f( S );
   return;
   }
  for( int d = 0 ; d <= 7 ; ++d )
   if( d != prev ) {
    S[ i - 1 ] = char( '0' + d );
    self( self , i - 1 , d );
    }
  };
 rec( rec , n , -1 );
 }

/*--------------------------------------------------------------------------*/
/* 25. 2022-02-07, Bit consecutivi                                          */
/*--------------------------------------------------------------------------*/

/// binary(): the strings of n bits with at most n0 consecutive 0 and n1
/// consecutive 1, f( S ) on each; zerosLeft and onesLeft the bits of each
/// kind still allowed in a row, the other one reset when a bit is chosen;
/// \f$ \Theta( n 2^n ) \f$ in the worst case
/// \pre n , n0 , n1 > 0
template< class F >
void binary( int n , int n0 , int n1 , F && f )
{
 ADS_PRE( ( n > 0 ) && ( n0 > 0 ) && ( n1 > 0 ) );
 std::string S( std::size_t( n ) , '0' );
 auto rec = [ & ]( auto & self , int i , int zerosLeft ,
                   int onesLeft ) -> void {
  if( i == n ) {
   f( S );
   return;
   }
  if( zerosLeft > 0 ) {
   S[ i ] = '0';
   self( self , i + 1 , zerosLeft - 1 , n1 );
   }
  if( onesLeft > 0 ) {
   S[ i ] = '1';
   self( self , i + 1 , n0 , onesLeft - 1 );
   }
  };
 rec( rec , 0 , n0 , n1 );
 }

/*--------------------------------------------------------------------------*/
/* 26. 2022-06-21, Math ways                                                */
/*--------------------------------------------------------------------------*/

/// mathWays(): the sequences from k to n whose steps add 1, double or
/// square, f( S ) on each; each of the three steps while below n, k >= 3
/// keeping 2 k and k^2 apart; O( n 3^n ), a crude bound
/// \pre 3 <= k <= n
template< class F >
void mathWays( int n , int k , F && f )
{
 ADS_PRE( ( 3 <= k ) && ( k <= n ) );
 std::vector< std::int64_t > S;
 auto rec = [ & ]( auto & self , std::int64_t curr ) -> void {
  S.push_back( curr );
  if( curr == n )
   f( S );
  else
   if( curr < n ) {
    self( self , curr + 1 );
    self( self , 2 * curr );
    self( self , curr * curr );
    }
  S.pop_back();
  };
 rec( rec , k );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace backtrack )

namespace {

/*--------------------------------------------------------------------------*/
/*------------------------------- ORACLES ----------------------------------*/
/*--------------------------------------------------------------------------*/

using backtrack::Tree;

/// the items of v sorted, to compare two enumerations as multisets
template< class T >
std::vector< T > sorted( std::vector< T > v )
{
 std::sort( v.begin() , v.end() );
 return( v );
 }

/// each item of v sorted, then v sorted: multisets of multisets
std::vector< std::vector< int > > sortedEach(
                                      std::vector< std::vector< int > > v )
{
 for( auto & x : v ) std::sort( x.begin() , x.end() );
 return( sorted( v ) );
 }

/// the words of a string separated by spaces
std::vector< std::string > words( const std::string & s )
{
 std::istringstream in( s );
 std::vector< std::string > w;
 for( std::string x ; in >> x ; ) w.push_back( x );
 return( w );
 }

/// a random graph of n nodes, each arc with probability p, symmetric unless
/// directed, no loops
Adjacency randomGraph( int n , double p , std::mt19937 & g ,
                       bool directed = false )
{
 std::bernoulli_distribution coin( p );
 Adjacency G( std::size_t( n ) , std::vector< bool >( std::size_t( n ) ,
                                                     false ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = directed ? 0 : u + 1 ; v < n ; ++v )
   if( ( u != v ) && coin( g ) ) {
    G[ u ][ v ] = true;
    if( ! directed ) G[ v ][ u ] = true;
    }
 return( G );
 }

/// the complete graph of n nodes, no loops
Adjacency complete( int n )
{
 Adjacency G( std::size_t( n ) , std::vector< bool >( std::size_t( n ) ,
                                                     true ) );
 for( int u = 0 ; u < n ; ++u ) G[ u ][ u ] = false;
 return( G );
 }

/// the unoriented graph of n nodes with the given edges
Adjacency edges( int n , const std::vector< std::pair< int , int > > & E )
{
 Adjacency G( std::size_t( n ) , std::vector< bool >( std::size_t( n ) ,
                                                     false ) );
 for( auto [ u , v ] : E ) G[ u ][ v ] = G[ v ][ u ] = true;
 return( G );
 }

/// the cycle of n nodes
Adjacency cycle( int n )
{
 std::vector< std::pair< int , int > > E;
 for( int i = 0 ; i < n ; ++i ) E.emplace_back( i , ( i + 1 ) % n );
 return( edges( n , E ) );
 }

/// a random sequence of n distinct integers of [ lo , lo + range )
std::vector< int > randomDistinct( int n , int lo , int range ,
                                   std::mt19937 & g )
{
 std::vector< int > all( std::size_t( range ) , 0 );
 std::iota( all.begin() , all.end() , lo );
 std::shuffle( all.begin() , all.end() , g );
 all.resize( std::size_t( n ) );
 return( all );
 }

/// a random sequence of n integers of [ lo , hi ]
std::vector< int > randomInts( int n , int lo , int hi , std::mt19937 & g )
{
 std::uniform_int_distribution< int > d( lo , hi );
 std::vector< int > v( std::size_t( n ) , 0 );
 for( auto & x : v ) x = d( g );
 return( v );
 }

/// the items of A at the positions of the bits of mask, in order
std::vector< int > pick( const std::vector< int > & A , unsigned mask )
{
 std::vector< int > S;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  if( ( mask >> i ) & 1u ) S.push_back( A[ i ] );
 return( S );
 }

/// the string of the n bits of x, the bit n - 1 first
std::string bitString( unsigned x , int n )
{
 std::string s( std::size_t( n ) , '0' );
 for( int i = 0 ; i < n ; ++i )
  if( ( x >> ( n - 1 - i ) ) & 1u ) s[ i ] = '1';
 return( s );
 }

/// the longest run of the character c in s
int longestRun( const std::string & s , char c )
{
 int best = 0 , run = 0;
 for( char x : s ) {
  run = ( x == c ) ? run + 1 : 0;
  best = std::max( best , run );
  }
 return( best );
 }

/// the sequences of distinct nodes from s (s first, all lengths), by an
/// enumeration of the tuples over [ 0 , n ) filtered by distinctness
std::vector< std::vector< int > > distinctSequences( int n , int s ,
                                                     int maxLength )
{
 std::vector< std::vector< int > > out;
 for( int len = 1 ; len <= maxLength ; ++len ) {
  long total = 1;
  for( int i = 1 ; i < len ; ++i ) total *= n;
  for( long code = 0 ; code < total ; ++code ) {
   std::vector< int > t = { s };
   long c = code;
   for( int i = 1 ; i < len ; ++i , c /= n ) t.push_back( int( c % n ) );
   if( std::set< int >( t.begin() , t.end() ).size() == t.size() )
    out.push_back( t );
   }
  }
 return( out );
 }

/// a random binary tree of n nodes with values in [ lo , hi ]
Tree randomTree( int n , int lo , int hi , std::mt19937 & g )
{
 Tree T;
 std::uniform_int_distribution< int > d( lo , hi );
 for( int i = 0 ; i < n ; ++i ) {
  const int t = T.add( d( g ) );
  if( i == 0 ) {
   T.root = t;
   continue;
   }
  for( int p = T.root ; ; ) {                   // a random free child
   int & child = ( g() % 2 ) ? T.left[ p ] : T.right[ p ];
   if( child < 0 ) {
    child = t;
    break;
    }
   p = child;
   }
  }
 return( T );
 }

/// the root-to-leaf paths of T, by an explicit stack with parent pointers
std::vector< std::vector< int > > rootLeafPaths( const Tree & T )
{
 std::vector< std::vector< int > > out;
 if( T.root < 0 ) return( out );
 std::vector< int > parent( T.value.size() , -1 ) , todo = { T.root };
 while( ! todo.empty() ) {
  const int t = todo.back();
  todo.pop_back();
  for( int c : { T.left[ t ] , T.right[ t ] } )
   if( c >= 0 ) {
    parent[ c ] = t;
    todo.push_back( c );
    }
  if( T.leaf( t ) ) {
   std::vector< int > p;
   for( int x = t ; x >= 0 ; x = parent[ x ] ) p.push_back( T.value[ x ] );
   std::reverse( p.begin() , p.end() );
   out.push_back( p );
   }
  }
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* 1. 2014-06-05, Colora                                                    */
/*--------------------------------------------------------------------------*/

// coloring() of the library against all the k^n assignments on random
// graphs, each coloring returned proper, and the chromatic numbers of the
// complete graphs, of the odd and even cycles and of K_{3,3}; no node, no
// edge, k > n, k < 0, an oriented graph
ADS_TEST( "backtrack-1" )
{
 std::mt19937 g( 1 );
 auto colorable = []( const Adjacency & G , int k ) {
  const int n = int( G.size() );
  long total = 1;
  for( int i = 0 ; i < n ; ++i ) total *= k;
  for( long code = 0 ; code < total ; ++code ) {
   std::vector< int > c( std::size_t( n ) , 0 );
   long x = code;
   for( int i = 0 ; i < n ; ++i , x /= k ) c[ i ] = int( x % k );
   bool ok = true;
   for( int u = 0 ; u < n && ok ; ++u )
    for( int v = u + 1 ; v < n && ok ; ++v )
     ok = ! ( G[ u ][ v ] && ( c[ u ] == c[ v ] ) );
   if( ok ) return( true );
   }
  return( n == 0 ); };
 auto proper = []( const Adjacency & G , const std::vector< int > & c ,
                   int k ) {
  for( std::size_t u = 0 ; u < G.size() ; ++u ) {
   if( ( c[ u ] < 0 ) || ( c[ u ] >= k ) ) return( false );
   for( std::size_t v = 0 ; v < G.size() ; ++v )
    if( G[ u ][ v ] && ( c[ u ] == c[ v ] ) ) return( false );
   }
  return( true ); };
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n = 1 + int( g() % 7 ) , k = int( g() % 5 );
  const Adjacency G = randomGraph( n , 0.5 , g );
  const auto c = coloring( G , k );
  CHECK( c.has_value() == colorable( G , k ) );
  if( c ) CHECK( proper( G , *c , k ) );
  }
 for( int n = 1 ; n <= 7 ; ++n ) {
  CHECK( coloring( complete( n ) , n ).has_value() );
  CHECK( ! coloring( complete( n ) , n - 1 ).has_value() );
  }
 CHECK( coloring( cycle( 5 ) , 3 ) && ! coloring( cycle( 5 ) , 2 ) );
 CHECK( coloring( cycle( 6 ) , 2 ) );
 Adjacency K33 = edges( 6 , { { 0 , 3 } , { 0 , 4 } , { 0 , 5 } , { 1 , 3 } ,
                             { 1 , 4 } , { 1 , 5 } , { 2 , 3 } , { 2 , 4 } ,
                             { 2 , 5 } } );
 CHECK( coloring( K33 , 2 ) );
 CHECK( ! coloring( complete( 1 ) , 0 ) && coloring( complete( 1 ) , 1 ) );
 // no node (0 colors suffice), no edge (1 color), k larger than n
 CHECK( coloring( edges( 0 , {} ) , 0 ) == std::vector< int >() );
 CHECK( coloring( edges( 5 , {} ) , 1 ) == std::vector< int >( 5 , 0 ) );
 CHECK( coloring( complete( 3 ) , 10 ).has_value() );
 CHECK_THROWS( coloring( complete( 2 ) , -1 ) , contract_violation );
 Adjacency arc = edges( 2 , {} );
 arc[ 0 ][ 1 ] = true;                           // oriented: refused
 CHECK_THROWS( coloring( arc , 2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 2. 2015-12-17, Count domino                                              */
/*--------------------------------------------------------------------------*/

// countDomino( n ) = fibonacci( n ) of the library (F_0 = F_1 = 1) against
// a backtrack that tiles the leftmost empty cell of the 2 x n board, the
// example of the statement and the largest n of 64 bits; n = 0 and 1, 93
// (beyond 64 bits) and negative refused
ADS_TEST( "backtrack-2" )
{
 auto tilings = []( int n ) {
  std::vector< std::vector< bool > > B( 2 , std::vector< bool >(
                                                 std::size_t( n ) , false ) );
  long count = 0;
  auto rec = [ & ]( auto & self ) -> void {
   int r = -1 , c = -1;
   for( int j = 0 ; j < n && r < 0 ; ++j )
    for( int i = 0 ; i < 2 && r < 0 ; ++i )
     if( ! B[ i ][ j ] ) {
      r = i;
      c = j;
      }
   if( r < 0 ) {
    ++count;
    return;
    }
   if( ( r == 0 ) && ! B[ 1 ][ c ] ) {           // vertical
    B[ 0 ][ c ] = B[ 1 ][ c ] = true;
    self( self );
    B[ 0 ][ c ] = B[ 1 ][ c ] = false;
    }
   if( ( c + 1 < n ) && ! B[ r ][ c + 1 ] ) {    // horizontal
    B[ r ][ c ] = B[ r ][ c + 1 ] = true;
    self( self );
    B[ r ][ c ] = B[ r ][ c + 1 ] = false;
    }
   };
  rec( rec );
  return( count ); };
 CHECK( fibonacci( 4 ) == 5 );
 for( int n = 0 ; n <= 20 ; ++n )
  CHECK( fibonacci( n ) == std::uint64_t( tilings( n ) ) );
 std::uint64_t a = 1 , b = 1;                    // DP[ n - 2 ] , DP[ n - 1 ]
 for( int n = 2 ; n <= 92 ; ++n ) {
  const std::uint64_t c = a + b;
  a = b;
  b = c;
  }
 CHECK( fibonacci( 92 ) == b );
 CHECK( ( fibonacci( 0 ) == 1 ) && ( fibonacci( 1 ) == 1 ) );
 CHECK_THROWS( fibonacci( 93 ) , contract_violation );   // over 64 bits
 CHECK_THROWS( fibonacci( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 3. 2017-09-04, (()), and 18. 2021-01-18, Parentesizzazioni               */
/*--------------------------------------------------------------------------*/

// nestedParentheses() of the library against the 2^{2n} strings filtered by
// the prefix counter, as sets, the Catalan numbers, and the examples
// n = 2 and n = 3 of the two statements; n = 0, 1, and negative refused
ADS_TEST( "backtrack-3" )
{
 auto all = []( int n ) {
  std::vector< std::string > out;
  nestedParentheses( n , [ & ]( const std::string & s ) {
   out.push_back( s );
   return( false ); } );
  return( out ); };
 CHECK( sorted( all( 2 ) ) ==
        sorted( std::vector< std::string >( { "(())" , "()()" } ) ) );
 CHECK( sorted( all( 3 ) ) ==
        sorted( words( "()()() (()()) (())() ()(()) ((()))" ) ) );
 for( int n = 0 ; n <= 8 ; ++n ) {
  std::vector< std::string > want;
  for( unsigned x = 0 ; x < ( 1u << ( 2 * n ) ) ; ++x ) {
   std::string s;
   int open = 0;
   bool ok = true;
   for( int i = 2 * n - 1 ; i >= 0 ; --i ) {
    const bool o = ( x >> i ) & 1u;
    s += o ? '(' : ')';
    open += o ? 1 : -1;
    ok = ok && ( open >= 0 );
    }
   if( ok && ( open == 0 ) ) want.push_back( s );
   }
  const auto got = all( n );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( got.size() == binomial( 2 * n , n ) / std::uint64_t( n + 1 ) );
  }
 CHECK( all( 0 ) == std::vector< std::string >( 1 , "" ) );
 CHECK( all( 1 ) == std::vector< std::string >( 1 , "()" ) );
 CHECK_THROWS( all( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 4. 2018-06-04, Print sums                                                */
/*--------------------------------------------------------------------------*/

// printSums() on the example, against the 2^n masks on random distinct
// items also negative, against subsetSum() of the library on positive
// items, and the empty vector; a 0 item, the extremes of int (sums out of
// int), repeated items refused
ADS_TEST( "backtrack-4" )
{
 auto run = []( const std::vector< int > & X , int w ) {
  std::vector< std::vector< int > > out;
  backtrack::printSums( X , w , [ & ]( const std::vector< int > & Y ) {
   out.push_back( Y ); } );
  return( out ); };
 CHECK( sorted( run( { -1 , 3 , 6 , 4 , 5 , 7 , 8 } , 13 ) ) ==
        sorted( std::vector< std::vector< int > >( {
         { 5 , 8 } , { 6 , 7 } , { 3 , 6 , 4 } , { -1 , 6 , 8 } ,
         { -1 , 3 , 4 , 7 } , { -1 , 3 , 6 , 5 } } ) ) );
 std::mt19937 g( 4 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = int( g() % 11 );
  const auto X = randomDistinct( n , -8 , 17 , g );
  const int w = int( g() % 21 ) - 10;
  std::vector< std::vector< int > > want;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   const auto S = pick( X , m );
   if( std::accumulate( S.begin() , S.end() , 0 ) == w )
    want.push_back( S );
   }
  CHECK( sorted( run( X , w ) ) == sorted( want ) );
  }
 for( int trial = 0 ; trial < 100 ; ++trial ) {
  const auto X = randomDistinct( 1 + int( g() % 10 ) , 1 , 15 , g );
  const int w = int( g() % 30 );
  const long lib = subsetSum( X , w , []( const auto & ) {
   return( false ); } );
  CHECK( long( run( X , w ).size() ) == lib );
  }
 CHECK( run( {} , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( {} , 3 ).empty() );
 // a 0 among the items (each subset with and without it), the extremes of
 // int, whose sums overflow an int
 CHECK( run( { 0 , 2 } , 2 ).size() == 2 && run( { 0 } , 0 ).size() == 2 );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( run( { hi , lo } , -1 ) ==
        std::vector< std::vector< int > >( 1 , { hi , lo } ) );
 CHECK( run( { hi , hi - 1 , 1 } , hi ) ==
        std::vector< std::vector< int > >( { { hi - 1 , 1 } , { hi } } ) );
 CHECK( run( { hi , 1 } , lo ).empty() );       // hi + 1 is not lo
 CHECK_THROWS( run( { 1 , 1 } , 2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 5. 2018-06-28, Print all, and 27. 2022-07-13, Stampa con spazi           */
/*--------------------------------------------------------------------------*/

// printSeparated() with " + " on the digits and with " " on the letters,
// the examples of the two statements, against the 2^{n-1} masks, one
// character, two, an empty separator, equal characters, the empty string
ADS_TEST( "backtrack-5" )
{
 auto run = []( const std::string & S , const std::string & sep ) {
  std::vector< std::string > out;
  backtrack::printSeparated( S , sep , [ & ]( const std::string & T ) {
   out.push_back( T ); } );
  return( out ); };
 CHECK( sorted( run( "3412" , " + " ) ) ==
        sorted( std::vector< std::string >( {
         "3 + 4 + 1 + 2" , "34 + 1 + 2" , "3 + 41 + 2" , "341 + 2" ,
         "3 + 4 + 12" , "34 + 12" , "3 + 412" , "3412" } ) ) );
 CHECK( sorted( run( "CIAO" , " " ) ) ==
        sorted( std::vector< std::string >( {
         "CIAO" , "CIA O" , "CI AO" , "CI A O" , "C IAO" , "C IA O" ,
         "C I AO" , "C I A O" } ) ) );
 std::mt19937 g( 5 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 10 );
  std::string S;
  for( int i = 0 ; i < n ; ++i ) S += char( '0' + g() % 10 );
  std::vector< std::string > want;
  for( unsigned m = 0 ; m < ( 1u << ( n - 1 ) ) ; ++m ) {
   std::string T( 1 , S[ 0 ] );
   for( int i = 1 ; i < n ; ++i ) {
    if( ( m >> ( i - 1 ) ) & 1u ) T += " + ";
    T += S[ i ];
    }
   want.push_back( T );
   }
  const auto got = run( S , " + " );
  CHECK( got.size() == ( std::size_t( 1 ) << ( n - 1 ) ) );
  CHECK( sorted( got ) == sorted( want ) );
  }
 CHECK( run( "7" , " + " ) == std::vector< std::string >( 1 , "7" ) );
 // two characters, an empty separator (all the ways write S), equal
 // characters (equal outputs from different cuts are all given)
 CHECK( sorted( run( "ab" , "-" ) ) == words( "a-b ab" ) );
 CHECK( run( "abc" , "" ) == std::vector< std::string >( 4 , "abc" ) );
 CHECK( sorted( run( "aa" , " " ) ) ==
        sorted( std::vector< std::string >( { "a a" , "aa" } ) ) );
 CHECK_THROWS( run( "" , " " ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 6. 2018-07-25, Visit                                                     */
/*--------------------------------------------------------------------------*/

// visit() against the tuples of distinct nodes from s whose consecutive
// nodes are adjacent, on random graphs, the ( n - 1 )! / ( n - 1 - k )!
// paths of the complete graph, k = 0 and k >= n; no edge, a path from an
// end and from the middle, k = n - 1 on a cycle; s, k or G refused
ADS_TEST( "backtrack-6" )
{
 auto run = []( const Adjacency & G , int s , int k ) {
  std::vector< std::vector< int > > out;
  backtrack::visit( G , s , k , [ & ]( const std::vector< int > & P ) {
   out.push_back( P ); } );
  return( out ); };
 std::mt19937 g( 6 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 6 ) , s = int( g() % unsigned( n ) ) ,
            k = int( g() % 6 );
  const Adjacency G = randomGraph( n , 0.5 , g );
  std::vector< std::vector< int > > want;
  for( const auto & P : distinctSequences( n , s , k + 1 ) ) {
   if( int( P.size() ) != k + 1 ) continue;
   bool ok = true;
   for( int i = 0 ; i < k ; ++i ) ok = ok && G[ P[ i ] ][ P[ i + 1 ] ];
   if( ok ) want.push_back( P );
   }
  CHECK( sorted( run( G , s , k ) ) == sorted( want ) );
  }
 for( int n = 1 ; n <= 6 ; ++n )
  for( int k = 0 ; k < n ; ++k ) {
   long want = 1;
   for( int i = 1 ; i <= k ; ++i ) want *= n - i;
   CHECK( long( run( complete( n ) , 0 , k ).size() ) == want );
   }
 CHECK( run( complete( 3 ) , 1 , 0 ) ==
        std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK( run( complete( 3 ) , 1 , 3 ).empty() );
 // no edge (only k = 0), a path from an end and from the middle, the
 // largest k = n - 1 on a cycle (two directions), the contracts
 CHECK( run( edges( 3 , {} ) , 2 , 1 ).empty() );
 CHECK( run( edges( 3 , {} ) , 2 , 0 ).size() == 1 );
 const Adjacency P3 = edges( 3 , { { 0 , 1 } , { 1 , 2 } } );
 CHECK( run( P3 , 0 , 2 ) ==
        std::vector< std::vector< int > >( 1 , { 0 , 1 , 2 } ) );
 CHECK( run( P3 , 1 , 2 ).empty() && ( run( P3 , 1 , 1 ).size() == 2 ) );
 CHECK( run( cycle( 6 ) , 0 , 5 ).size() == 2 );
 CHECK_THROWS( run( complete( 3 ) , 3 , 1 ) , contract_violation );
 CHECK_THROWS( run( complete( 3 ) , -1 , 1 ) , contract_violation );
 CHECK_THROWS( run( complete( 3 ) , 0 , -1 ) , contract_violation );
 CHECK_THROWS( run( Adjacency( 2 , std::vector< bool >( 3 , false ) ) , 0 ,
                    1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 7. 2018-08-21, n-colorazione con gap                                     */
/*--------------------------------------------------------------------------*/

// gapColoring() against the n! bijections on random connected graphs, each
// coloring returned valid, the 5-cycle (yes) and the 4-cycle (no) of the
// solution, the complete graphs, and the scheme of the text: never wrong
// when it says yes, but no on a path with node 0 in the middle; no node,
// two nodes apart and joined, the paths of 3 (no) and 4 (yes), an
// oriented graph refused
ADS_TEST( "backtrack-7" )
{
 auto valid = []( const Adjacency & G , const std::vector< int > & c ) {
  const int n = int( G.size() );
  std::vector< int > s( c );
  std::sort( s.begin() , s.end() );
  for( int i = 0 ; i < n ; ++i )
   if( s[ i ] != i + 1 ) return( false );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( G[ u ][ v ] && ( std::abs( c[ u ] - c[ v ] ) < 2 ) )
     return( false );
  return( true ); };
 auto brute = [ & ]( const Adjacency & G ) {
  std::vector< int > c( G.size() );
  std::iota( c.begin() , c.end() , 1 );
  do
   if( valid( G , c ) ) return( true );
  while( std::next_permutation( c.begin() , c.end() ) );
  return( false ); };
 std::mt19937 g( 7 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n = 1 + int( g() % 7 );
  Adjacency G = randomGraph( n , 0.3 , g );
  for( int v = 1 ; v < n ; ++v ) {               // a random spanning tree
   const int u = int( g() % unsigned( v ) );
   G[ u ][ v ] = G[ v ][ u ] = true;
   }
  const auto c = backtrack::gapColoring( G );
  CHECK( c.has_value() == brute( G ) );
  if( c ) CHECK( valid( G , *c ) );
  if( backtrack::gapColoringText( G ) ) CHECK( c.has_value() );
  }
 CHECK( backtrack::gapColoring( cycle( 5 ) ) );
 CHECK( ! backtrack::gapColoring( cycle( 4 ) ) );
 for( int n = 2 ; n <= 6 ; ++n )
  CHECK( ! backtrack::gapColoring( complete( n ) ) );
 CHECK( backtrack::gapColoring( complete( 1 ) ) ==
        std::vector< int >( 1 , 1 ) );
 const Adjacency P = edges( 4 , { { 1 , 0 } , { 0 , 2 } , { 2 , 3 } } );
 CHECK( backtrack::gapColoring( P ) && ! backtrack::gapColoringText( P ) );
 // no node (the empty coloring, which the text cannot give), two nodes
 // apart (any bijection) or joined (no colors 2 apart), the path of 4
 CHECK( backtrack::gapColoring( edges( 0 , {} ) ) == std::vector< int >() );
 CHECK( ! backtrack::gapColoringText( edges( 0 , {} ) ) );
 CHECK( backtrack::gapColoring( edges( 2 , {} ) ) ==
        std::vector< int >( { 1 , 2 } ) );
 CHECK( ! backtrack::gapColoring( edges( 2 , { { 0 , 1 } } ) ) );
 CHECK( ! backtrack::gapColoring( edges( 3 , { { 0 , 1 } , { 1 , 2 } } ) ) );
 CHECK( backtrack::gapColoring( edges( 4 , { { 0 , 1 } , { 1 , 2 } ,
                                             { 2 , 3 } } ) ) );
 Adjacency arc = edges( 3 , {} );
 arc[ 0 ][ 2 ] = true;
 CHECK_THROWS( backtrack::gapColoring( arc ) , contract_violation );
 CHECK_THROWS( backtrack::gapColoringText( arc ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 8. 2019-01-21, All sums rec                                              */
/*--------------------------------------------------------------------------*/

// allSums() on the example (11 multisets), and on random values each
// multiset of sum v over the values of A, no two equal, as many as the
// ways of the coin change counted by dynamic programming; v = 0, no value,
// one value, a value above v; a value not positive, repeated, v < 0
ADS_TEST( "backtrack-8" )
{
 auto run = []( const std::vector< int > & A , int v ) {
  std::vector< std::vector< int > > out;
  backtrack::allSums( A , v , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sortedEach( run( { 1 , 3 , 2 , 4 } , 7 ) ) ==
        sortedEach( { { 1 , 1 , 1 , 1 , 1 , 1 , 1 } ,
                      { 1 , 1 , 1 , 1 , 1 , 2 } , { 1 , 1 , 1 , 1 , 3 } ,
                      { 1 , 1 , 1 , 2 , 2 } , { 1 , 1 , 1 , 4 } ,
                      { 1 , 1 , 3 , 2 } , { 1 , 3 , 3 } , { 1 , 2 , 2 , 2 } ,
                      { 1 , 2 , 4 } , { 3 , 2 , 2 } , { 3 , 4 } } ) );
 std::mt19937 g( 8 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const auto A = randomDistinct( 1 + int( g() % 5 ) , 1 , 8 , g );
  const int v = int( g() % 16 );
  std::vector< long > ways( std::size_t( v ) + 1 , 0 );
  ways[ 0 ] = 1;
  for( int a : A )
   for( int x = a ; x <= v ; ++x ) ways[ x ] += ways[ x - a ];
  const auto got = run( A , v );
  CHECK( long( got.size() ) == ways[ v ] );
  for( const auto & S : got ) {
   CHECK( std::accumulate( S.begin() , S.end() , 0 ) == v );
   for( int x : S )
    CHECK( std::find( A.begin() , A.end() , x ) != A.end() );
   }
  auto e = sortedEach( got );
  CHECK( std::adjacent_find( e.begin() , e.end() ) == e.end() );
  }
 CHECK( run( { 2 , 5 } , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( { 2 } , 3 ).empty() );
 // no value (only v = 0), one value taken v times, a value larger than v
 CHECK( run( {} , 0 ).size() == 1 && run( {} , 4 ).empty() );
 CHECK( run( { 1 } , 6 ) == std::vector< std::vector< int > >( 1 ,
                                         std::vector< int >( 6 , 1 ) ) );
 CHECK( run( { 9 , 3 } , 6 ) == std::vector< std::vector< int > >( 1 ,
                                                            { 3 , 3 } ) );
 CHECK_THROWS( run( { 0 , 1 } , 2 ) , contract_violation );
 CHECK_THROWS( run( { -1 , 1 } , 2 ) , contract_violation );
 CHECK_THROWS( run( { 2 , 2 } , 2 ) , contract_violation );
 CHECK_THROWS( run( { 1 } , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 9. 2019-02-07, Print all                                                 */
/*--------------------------------------------------------------------------*/

// printAll() on the two examples, against the m-combinations of the
// positions matching P on random strings, the count of the classic dynamic
// programming, P empty, P longer than T and all the characters equal;
// both empty, T = P, P at the end of T, P absent or in the wrong order
ADS_TEST( "backtrack-9" )
{
 auto run = []( const std::string & T , const std::string & P ) {
  std::vector< std::vector< int > > out;
  backtrack::printAll( T , P , [ & ]( const std::vector< int > & I ) {
   out.push_back( I ); } );
  return( out ); };
 const auto dog = run( "gooddog" , "dog" );
 CHECK( std::find( dog.begin() , dog.end() ,
                   std::vector< int >( { 4 , 6 , 7 } ) ) != dog.end() );
 CHECK( sorted( run( "gooddog" , "god" ) ) ==
        sorted( std::vector< std::vector< int > >( {
         { 1 , 2 , 4 } , { 1 , 2 , 5 } , { 1 , 3 , 4 } , { 1 , 3 , 5 } } ) ) );
 std::mt19937 g( 9 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = int( g() % 12 ) , m = int( g() % 5 );
  std::string T , P;
  for( int i = 0 ; i < n ; ++i ) T += char( 'a' + g() % 3 );
  for( int i = 0 ; i < m ; ++i ) P += char( 'a' + g() % 3 );
  std::vector< std::vector< int > > want;
  for( unsigned x = 0 ; x < ( 1u << n ) ; ++x ) {
   if( std::popcount( x ) != m ) continue;
   std::vector< int > I;
   std::string s;
   for( int i = 0 ; i < n ; ++i )
    if( ( x >> i ) & 1u ) {
     I.push_back( i + 1 );
     s += T[ i ];
     }
   if( s == P ) want.push_back( I );
   }
  std::vector< std::vector< long > > D( std::size_t( n ) + 1 ,
                               std::vector< long >( std::size_t( m ) + 1 ,
                                                    0 ) );
  for( int i = 0 ; i <= n ; ++i ) D[ i ][ 0 ] = 1;
  for( int i = 1 ; i <= n ; ++i )
   for( int j = 1 ; j <= m ; ++j )
    D[ i ][ j ] = D[ i - 1 ][ j ] +
                  ( T[ i - 1 ] == P[ j - 1 ] ? D[ i - 1 ][ j - 1 ] : 0 );
  const auto got = run( T , P );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( long( got.size() ) == D[ n ][ m ] );
  }
 CHECK( run( "abc" , "" ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( "ab" , "abc" ).empty() );
 CHECK( run( "aaaa" , "aa" ).size() == 6 );
 // both empty, T = P, P at the end of T, P absent
 CHECK( run( "" , "" ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( "" , "a" ).empty() );
 CHECK( run( "abc" , "abc" ) ==
        std::vector< std::vector< int > >( 1 , { 1 , 2 , 3 } ) );
 CHECK( run( "xxab" , "ab" ) ==
        std::vector< std::vector< int > >( 1 , { 3 , 4 } ) );
 CHECK( run( "abab" , "c" ).empty() && run( "ba" , "ab" ).empty() );
 }

/*--------------------------------------------------------------------------*/
/* 10. 2019-06-06, Print flags                                              */
/*--------------------------------------------------------------------------*/

// printFlags() on the example, against the k^n sequences filtered by A on
// random matrices, the count as the sum of the entries of A^{n-1}, one
// stripe, no stripe, and a matrix all false; no color, one color; a true
// diagonal, a matrix not symmetric, n < 0 refused
ADS_TEST( "backtrack-10" )
{
 auto run = []( const Adjacency & A , int n ) {
  std::vector< std::vector< int > > out;
  backtrack::printFlags( A , n , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 const Adjacency E = edges( 3 , { { 0 , 2 } , { 1 , 2 } } );
 CHECK( run( E , 3 ) == std::vector< std::vector< int > >( {
         { 1 , 3 , 1 } , { 1 , 3 , 2 } , { 2 , 3 , 1 } , { 2 , 3 , 2 } ,
         { 3 , 1 , 3 } , { 3 , 2 , 3 } } ) );
 std::mt19937 g( 10 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int k = 1 + int( g() % 4 ) , n = int( g() % 6 );
  const Adjacency A = randomGraph( k , 0.6 , g );
  std::vector< std::vector< int > > want;
  long total = 1;
  for( int i = 0 ; i < n ; ++i ) total *= k;
  for( long code = 0 ; code < total ; ++code ) {
   std::vector< int > S( std::size_t( n ) , 0 );
   long x = code;
   for( int i = n - 1 ; i >= 0 ; --i , x /= k ) S[ i ] = 1 + int( x % k );
   bool ok = true;
   for( int i = 1 ; i < n ; ++i )
    ok = ok && A[ S[ i - 1 ] - 1 ][ S[ i ] - 1 ];
   if( ok ) want.push_back( S );
   }
  const auto got = run( A , n );
  CHECK( got == want );                       // both lexicographic
  if( n >= 1 ) {
   std::vector< long > w( std::size_t( k ) , 1 );      // walks: A^{n-1} 1
   for( int step = 1 ; step < n ; ++step ) {
    std::vector< long > z( std::size_t( k ) , 0 );
    for( int a = 0 ; a < k ; ++a )
     for( int b = 0 ; b < k ; ++b )
      if( A[ a ][ b ] ) z[ a ] += w[ b ];
    w = z;
    }
   CHECK( long( got.size() ) ==
          std::accumulate( w.begin() , w.end() , 0L ) );
   }
  }
 CHECK( run( complete( 4 ) , 1 ).size() == 4 );
 CHECK( run( complete( 4 ) , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( edges( 3 , {} ) , 2 ).empty() );
 // no color: one empty flag, none of one stripe; one color alone
 CHECK( run( edges( 0 , {} ) , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( edges( 0 , {} ) , 1 ).empty() );
 CHECK( run( edges( 1 , {} ) , 1 ) ==
        std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK_THROWS( run( complete( 2 ) , -1 ) , contract_violation );
 Adjacency loop = complete( 2 );
 loop[ 1 ][ 1 ] = true;                          // a color next to itself
 CHECK_THROWS( run( loop , 2 ) , contract_violation );
 Adjacency arc = edges( 2 , {} );
 arc[ 0 ][ 1 ] = true;
 CHECK_THROWS( run( arc , 2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 11. 2019-06-17, Safe line                                                */
/*--------------------------------------------------------------------------*/

// hamiltonianPath( G , u ) of the library against the ( n - 1 )!
// permutations from u, each ordering returned a permutation from u along
// the edges, on random graphs; one node, a path from either end and from
// the middle, a star, two nodes apart and joined, an oriented arc, a start
// out of the nodes
ADS_TEST( "backtrack-11" )
{
 std::mt19937 g( 11 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n = 1 + int( g() % 7 ) , u = int( g() % unsigned( n ) );
  const Adjacency G = randomGraph( n , 0.45 , g );
  std::vector< int > rest;
  for( int v = 0 ; v < n ; ++v )
   if( v != u ) rest.push_back( v );
  bool exists = false;
  do {
   bool ok = ( n == 1 ) || G[ u ][ rest[ 0 ] ];
   for( int i = 1 ; i + 1 < n && ok ; ++i )
    ok = G[ rest[ i - 1 ] ][ rest[ i ] ];
   exists = exists || ok;
   }
  while( std::next_permutation( rest.begin() , rest.end() ) );
  const auto P = hamiltonianPath( G , u );
  CHECK( P.has_value() == exists );
  if( P ) {
   CHECK( ( int( P->size() ) == n ) && ( P->front() == u ) );
   auto s = sorted( *P );
   for( int i = 0 ; i < n ; ++i ) CHECK( s[ i ] == i );
   for( int i = 1 ; i < n ; ++i ) CHECK( G[ ( *P )[ i - 1 ] ][ ( *P )[ i ] ] );
   }
  }
 CHECK( hamiltonianPath( complete( 1 ) , 0 ) == Nodes( { 0 } ) );
 const Adjacency P4 = edges( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 3 } } );
 CHECK( hamiltonianPath( P4 , 0 ) && ! hamiltonianPath( P4 , 1 ) );
 const Adjacency star = edges( 4 , { { 0 , 1 } , { 0 , 2 } , { 0 , 3 } } );
 CHECK( ! hamiltonianPath( star , 0 ) && ! hamiltonianPath( star , 1 ) );
 // two nodes apart and joined, a path from its last node, an oriented
 // arc used only forward, a start out of the nodes
 CHECK( ! hamiltonianPath( edges( 2 , {} ) , 0 ) );
 CHECK( hamiltonianPath( edges( 2 , { { 0 , 1 } } ) , 1 ) ==
        Nodes( { 1 , 0 } ) );
 CHECK( hamiltonianPath( P4 , 3 ) == Nodes( { 3 , 2 , 1 , 0 } ) );
 Adjacency arc = edges( 2 , {} );
 arc[ 0 ][ 1 ] = true;
 CHECK( hamiltonianPath( arc , 0 ) && ! hamiltonianPath( arc , 1 ) );
 CHECK_THROWS( hamiltonianPath( P4 , 4 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 12. 2019-07-18, k-alternato                                              */
/*--------------------------------------------------------------------------*/

// kAlternate() and kAlternateAll() on the examples, against
// std::next_permutation with the runs computed apart on random sets, the
// counts 2 E_n for k = 2, k >= n, one item, two items; isKAlternate() on
// no item, one, runs of k and k + 1; repeated items, k < 2, no item refused
ADS_TEST( "backtrack-12" )
{
 CHECK( backtrack::kAlternate( { 2 , 7 , 5 } , 2 ) == 4 );
 CHECK( backtrack::kAlternateAll( { 2 , 7 , 5 } , 2 ) == 4 );
 const std::vector< int > S = { 2 , 4 , 6 , 8 , 7 , 5 , 3 , 10 , 12 };
 CHECK( backtrack::isKAlternate( S , 4 ) &&
        ! backtrack::isKAlternate( S , 3 ) );
 auto longestMonotone = []( const std::vector< int > & a ) {
  int best = 1 , up = 1 , down = 1;
  for( std::size_t i = 1 ; i < a.size() ; ++i ) {
   up = a[ i - 1 ] < a[ i ] ? up + 1 : 1;
   down = a[ i - 1 ] > a[ i ] ? down + 1 : 1;
   best = std::max( { best , up , down } );
   }
  return( best ); };
 std::mt19937 g( 12 );
 for( int trial = 0 ; trial < 200 ; ++trial ) {
  const int n = 1 + int( g() % 7 ) , k = 2 + int( g() % 4 );
  auto A = randomDistinct( n , -10 , 25 , g );
  auto P = sorted( A );
  long want = 0;
  do
   if( longestMonotone( P ) <= k ) ++want;
  while( std::next_permutation( P.begin() , P.end() ) );
  CHECK( backtrack::kAlternate( A , k ) == want );
  CHECK( backtrack::kAlternateAll( A , k ) == want );
  }
 const long twiceEuler[] = { 2 , 4 , 10 , 32 , 122 };
 for( int n = 2 ; n <= 6 ; ++n ) {
  std::vector< int > A( std::size_t( n ) , 0 );
  std::iota( A.begin() , A.end() , 1 );
  CHECK( backtrack::kAlternate( A , 2 ) == twiceEuler[ n - 2 ] );
  long fact = 1;
  for( int i = 2 ; i <= n ; ++i ) fact *= i;
  CHECK( backtrack::kAlternate( A , n ) == fact );
  }
 CHECK( backtrack::kAlternate( { 9 } , 2 ) == 1 );
 // isKAlternate() on no item, one, a run of exactly k, a run of k + 1
 // decreasing; two items for every k; repeated items refused
 CHECK( backtrack::isKAlternate( {} , 1 ) && backtrack::isKAlternate( { 4 } ,
                                                                      1 ) );
 CHECK( ! backtrack::isKAlternate( { 1 , 2 } , 1 ) );
 CHECK( backtrack::isKAlternate( { 1 , 2 , 3 } , 3 ) &&
        ! backtrack::isKAlternate( { 3 , 2 , 1 , 0 } , 3 ) );
 CHECK( backtrack::kAlternate( { 5 , -5 } , 2 ) == 2 &&
        backtrack::kAlternateAll( { 5 , -5 } , 9 ) == 2 );
 CHECK_THROWS( backtrack::kAlternate( { 1 , 2 } , 1 ) , contract_violation );
 CHECK_THROWS( backtrack::kAlternate( {} , 2 ) , contract_violation );
 CHECK_THROWS( backtrack::kAlternate( { 3 , 3 } , 2 ) , contract_violation );
 CHECK_THROWS( backtrack::kAlternateAll( { 3 , 3 } , 2 ) ,
               contract_violation );
 CHECK_THROWS( backtrack::isKAlternate( { 1 , 1 } , 2 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 13. 2020-01-14, Longest path                                             */
/*--------------------------------------------------------------------------*/

// longestPath() on the example (27), against a dynamic programming over
// the sets of visited cells on random grids of at most 16 cells, a start
// at 0, one cell, a full 4 x 4 grid (a snake of 16); no cell, a row and a
// column, an isolated start, a ring; ragged rows and values not 0 or 1
// refused
ADS_TEST( "backtrack-13" )
{
 const backtrack::Grid M = { { 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } ,
                             { 1 , 1 , 0 , 1 , 1 , 0 , 1 , 1 , 0 , 1 } ,
                             { 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 , 1 } };
 CHECK( backtrack::longestPath( M ) == 27 );
 std::mt19937 g( 13 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 4 ) , m = 1 + int( g() % 4 );
  backtrack::Grid A( std::size_t( n ) ,
                    std::vector< int >( std::size_t( m ) , 0 ) );
  for( auto & row : A )
   for( auto & x : row ) x = ( g() % 4 ) ? 1 : 0;
  const int cells = n * m;
  std::vector< char > R( ( std::size_t( 1 ) << cells ) * cells , 0 );
  auto r = [ & ]( unsigned s , int c ) -> char & {
   return( R[ s * unsigned( cells ) + unsigned( c ) ] ); };
  int want = 0;
  if( A[ 0 ][ 0 ] ) {
   r( 1 , 0 ) = 1;
   for( unsigned s = 1 ; s < ( 1u << cells ) ; ++s )
    for( int c = 0 ; c < cells ; ++c ) {
     if( ! r( s , c ) ) continue;
     want = std::max( want , std::popcount( s ) );
     const int i = c / m , j = c % m;
     const int di[] = { 1 , -1 , 0 , 0 } , dj[] = { 0 , 0 , 1 , -1 };
     for( int d = 0 ; d < 4 ; ++d ) {
      const int a = i + di[ d ] , b = j + dj[ d ];
      if( ( a < 0 ) || ( a >= n ) || ( b < 0 ) || ( b >= m ) ||
          ! A[ a ][ b ] || ( ( s >> ( a * m + b ) ) & 1u ) )
       continue;
      r( s | ( 1u << ( a * m + b ) ) , a * m + b ) = 1;
      }
     }
   }
  CHECK( backtrack::longestPath( A ) == want );
  }
 CHECK( backtrack::longestPath( { { 0 , 1 } , { 1 , 1 } } ) == 0 );
 CHECK( backtrack::longestPath( { { 1 } } ) == 1 );
 CHECK( backtrack::longestPath( backtrack::Grid( 4 ,
                                    std::vector< int >( 4 , 1 ) ) ) == 16 );
 // no cell, a row and a column of 1 (the whole line), a single 0, a 1
 // isolated by zeros, a ring around a 0 (all of it but not back)
 CHECK( backtrack::longestPath( {} ) == 0 );
 CHECK( backtrack::longestPath( { std::vector< int >( 9 , 1 ) } ) == 9 );
 CHECK( backtrack::longestPath( backtrack::Grid( 7 , { 1 } ) ) == 7 );
 CHECK( backtrack::longestPath( { { 0 } } ) == 0 );
 CHECK( backtrack::longestPath( { { 1 , 0 } , { 0 , 1 } } ) == 1 );
 CHECK( backtrack::longestPath( { { 1 , 1 , 1 } , { 1 , 0 , 1 } ,
                                  { 1 , 1 , 1 } } ) == 8 );
 CHECK_THROWS( backtrack::longestPath( { { 1 , 2 } } ) , contract_violation );
 CHECK_THROWS( backtrack::longestPath( { { 1 , 1 } , { 1 } } ) ,
               contract_violation );
 CHECK_THROWS( backtrack::longestPath( { { -1 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 14. 2020-06-12, Gen rec                                                  */
/*--------------------------------------------------------------------------*/

// generate() on the example n = 6 (8 compositions), against the 2^{n-1}
// compositions filtered by palindromy, 2^{n/2} of them, generateEven()
// against those of even length, n = 0, 1, 2, 3; n < 0 refused
ADS_TEST( "backtrack-14" )
{
 auto run = []( int n , bool even ) {
  std::vector< std::vector< int > > out;
  auto f = [ & ]( const std::vector< int > & a ) { out.push_back( a ); };
  if( even ) backtrack::generateEven( n , f );
  else backtrack::generate( n , f );
  return( out ); };
 CHECK( sorted( run( 6 , false ) ) ==
        sorted( std::vector< std::vector< int > >( {
         { 1 , 1 , 1 , 1 , 1 , 1 } , { 1 , 1 , 2 , 1 , 1 } ,
         { 1 , 2 , 2 , 1 } , { 1 , 4 , 1 } , { 2 , 1 , 1 , 2 } ,
         { 2 , 2 , 2 } , { 3 , 3 } , { 6 } } ) ) );
 for( int n = 1 ; n <= 16 ; ++n ) {
  std::vector< std::vector< int > > want , wantEven;
  for( unsigned cut = 0 ; cut < ( 1u << ( n - 1 ) ) ; ++cut ) {
   std::vector< int > a;
   int part = 1;
   for( int i = 1 ; i < n ; ++i )
    if( ( cut >> ( i - 1 ) ) & 1u ) {
     a.push_back( part );
     part = 1;
     }
    else
     ++part;
   a.push_back( part );
   if( std::equal( a.begin() , a.end() , a.rbegin() ) ) {
    want.push_back( a );
    if( a.size() % 2 == 0 ) wantEven.push_back( a );
    }
   }
  const auto got = run( n , false );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( got.size() == ( std::size_t( 1 ) << ( n / 2 ) ) );
  CHECK( sorted( run( n , true ) ) == sorted( wantEven ) );
  }
 CHECK( run( 0 , false ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( 0 , true ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( 1 , false ) == std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK( run( 1 , true ).empty() );
 CHECK( run( 2 , false ) == std::vector< std::vector< int > >( {
         { 2 } , { 1 , 1 } } ) );
 CHECK( run( 2 , true ) == std::vector< std::vector< int > >( 1 ,
                                                             { 1 , 1 } ) );
 CHECK( run( 3 , true ).empty() );
 CHECK_THROWS( run( -1 , false ) , contract_violation );
 CHECK_THROWS( run( -2 , true ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 15. 2020-07-03, Print positive                                           */
/*--------------------------------------------------------------------------*/

// printPositive() on the example, against all the root-to-leaf paths
// filtered by positivity on random trees, one node positive and not, the
// empty tree, a chain of 1000 nodes with and without a 0 at its leaf, one
// child positive and one not
ADS_TEST( "backtrack-15" )
{
 auto run = []( const Tree & T ) {
  std::vector< std::vector< int > > out;
  backtrack::printPositive( T , [ & ]( const std::vector< int > & p ) {
   out.push_back( p ); } );
  return( out ); };
 Tree T;                    // 25( 19( 13( 11 , 5 ) , -2( 21 , - ) ) , ... )
 const int a = T.add( 13 , T.add( 11 ) , T.add( 5 ) ) ,
           b = T.add( -2 , T.add( 21 ) ) ,
           c = T.add( 28 , T.add( 27 ) , T.add( -1 ) );
 T.root = T.add( 25 , T.add( 19 , a , b ) , c );
 CHECK( run( T ) == std::vector< std::vector< int > >( {
         { 25 , 19 , 13 , 11 } , { 25 , 19 , 13 , 5 } , { 25 , 28 , 27 } } ) );
 std::mt19937 g( 15 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const Tree R = randomTree( 1 + int( g() % 15 ) , -3 , 9 , g );
  std::vector< std::vector< int > > want;
  for( const auto & p : rootLeafPaths( R ) )
   if( std::all_of( p.begin() , p.end() ,
                    []( int x ) { return( x > 0 ); } ) )
    want.push_back( p );
  CHECK( sorted( run( R ) ) == sorted( want ) );
  }
 Tree one;
 one.root = one.add( 4 );
 CHECK( run( one ) == std::vector< std::vector< int > >( 1 , { 4 } ) );
 one.value[ 0 ] = 0;
 CHECK( run( one ).empty() );
 CHECK( run( Tree() ).empty() );
 // a chain of 1000 positive values (one path), a 0 at its leaf, a node
 // with one positive child and one not (a leaf only below the positive)
 Tree chain;
 for( int i = 0 ; i < 1000 ; ++i )
  chain.root = chain.add( 1000 - i , chain.root );
 const auto all = run( chain );
 CHECK( ( all.size() == 1 ) && ( all[ 0 ].size() == 1000 ) &&
        ( all[ 0 ].front() == 1 ) && ( all[ 0 ].back() == 1000 ) );
 chain.value[ 0 ] = 0;
 CHECK( run( chain ).empty() );
 Tree V;
 V.root = V.add( 1 , V.add( -1 ) , V.add( 2 ) );
 CHECK( run( V ) == std::vector< std::vector< int > >( 1 , { 1 , 2 } ) );
 }

/*--------------------------------------------------------------------------*/
/* 16. 2020-07-24, Print hamilton                                           */
/*--------------------------------------------------------------------------*/

// printHamilton() on a 4-node digraph with one cycle, against the
// ( n - 1 )! permutations after node 0 on random digraphs, each cycle once,
// the existence as hamiltonianCycle() of the library, the complete digraph;
// no node, one node with and without its loop, two nodes with two arcs or
// one, no arc, a matrix not square
ADS_TEST( "backtrack-16" )
{
 auto run = []( const Adjacency & G ) {
  std::vector< std::vector< int > > out;
  backtrack::printHamilton( G , [ & ]( const std::vector< int > & P ) {
   out.push_back( P ); } );
  return( out ); };
 Adjacency D = edges( 4 , {} );           // A -> B -> C -> D -> A, A -> C,
 D[ 0 ][ 1 ] = D[ 1 ][ 2 ] = D[ 2 ][ 3 ] = D[ 3 ][ 0 ] = true;   // B -> D
 D[ 0 ][ 2 ] = D[ 1 ][ 3 ] = true;
 CHECK( run( D ) == std::vector< std::vector< int > >( 1 ,
                                                      { 0 , 1 , 2 , 3 } ) );
 std::mt19937 g( 16 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n = 2 + int( g() % 6 );
  const Adjacency G = randomGraph( n , 0.5 , g , true );
  std::vector< int > P( std::size_t( n ) , 0 );
  std::iota( P.begin() , P.end() , 0 );
  std::vector< std::vector< int > > want;
  do {
   bool ok = G[ P[ n - 1 ] ][ 0 ];
   for( int i = 1 ; i < n && ok ; ++i ) ok = G[ P[ i - 1 ] ][ P[ i ] ];
   if( ok ) want.push_back( P );
   }
  while( std::next_permutation( P.begin() + 1 , P.end() ) );
  const auto got = run( G );
  CHECK( sorted( got ) == want );
  CHECK( got.empty() == ! hamiltonianCycle( G ).has_value() );
  }
 for( int n = 2 ; n <= 7 ; ++n ) {
  long fact = 1;
  for( int i = 2 ; i < n ; ++i ) fact *= i;
  CHECK( long( run( complete( n ) ).size() ) == fact );
  }
 CHECK( run( complete( 1 ) ).empty() && run( edges( 0 , {} ) ).empty() );
 // one node with its loop, two nodes with both arcs or one, no arc
 const Adjacency loop( 1 , std::vector< bool >( 1 , true ) );
 CHECK( run( loop ) == std::vector< std::vector< int > >( 1 , { 0 } ) );
 CHECK( run( complete( 2 ) ) ==
        std::vector< std::vector< int > >( 1 , { 0 , 1 } ) );
 Adjacency arc = edges( 2 , {} );
 arc[ 0 ][ 1 ] = true;
 CHECK( run( arc ).empty() && run( edges( 4 , {} ) ).empty() );
 CHECK_THROWS( run( Adjacency( 2 , std::vector< bool >( 1 , true ) ) ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 17. 2020-09-04, Print increasing                                         */
/*--------------------------------------------------------------------------*/

// printIncreasing() on the example (20 sequences), against the 2^n index
// sets filtered as multisets, the count of a quadratic dynamic
// programming, the empty vector, all equal, sorted, decreasing, the
// extremes of int, a tie
ADS_TEST( "backtrack-17" )
{
 auto run = []( const std::vector< int > & A ) {
  std::vector< std::vector< int > > out;
  backtrack::printIncreasing( A , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( { 1 , 3 , 6 , 4 , 5 } ) ) ==
        sorted( std::vector< std::vector< int > >( {
         { 1 , 3 , 4 , 5 } , { 3 , 4 , 5 } , { 1 , 4 , 5 } , { 4 , 5 } ,
         { 1 , 3 , 5 } , { 3 , 5 } , { 1 , 5 } , { 5 } , { 1 , 3 , 4 } ,
         { 3 , 4 } , { 1 , 4 } , { 4 } , { 1 , 3 , 6 } , { 3 , 6 } ,
         { 1 , 6 } , { 6 } , { 1 , 3 } , { 3 } , { 1 } , {} } ) ) );
 std::mt19937 g( 17 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = int( g() % 12 );
  const auto A = randomInts( n , 0 , 6 , g );
  std::vector< std::vector< int > > want;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   const auto S = pick( A , m );
   if( std::adjacent_find( S.begin() , S.end() ,
                           std::greater_equal< int >() ) == S.end() )
    want.push_back( S );
   }
  std::vector< long > end( std::size_t( n ) , 1 );
  long count = 1;
  for( int i = 0 ; i < n ; ++i ) {
   for( int j = 0 ; j < i ; ++j )
    if( A[ j ] < A[ i ] ) end[ i ] += end[ j ];
   count += end[ i ];
   }
  const auto got = run( A );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( long( got.size() ) == count );
  }
 CHECK( run( {} ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( { 3 , 3 , 3 } ).size() == 4 );
 CHECK( run( { 1 , 2 , 3 , 4 , 5 , 6 } ).size() == 64 );
 // decreasing (only the single items), the extremes of int, a tie that
 // breaks the increase
 CHECK( run( { 6 , 5 , 4 , 3 } ).size() == 1 + 4 );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( sorted( run( { lo , hi } ) ) ==
        sorted( std::vector< std::vector< int > >( { {} , { lo } , { hi } ,
                                                      { lo , hi } } ) ) );
 CHECK( run( { 1 , 2 , 2 } ).size() == 1 + 3 + 2 );
 }

/*--------------------------------------------------------------------------*/
/* 19. 2021-02-08, Somma di primi                                           */
/*--------------------------------------------------------------------------*/

// primes() on the example 24 = 2 + 3 + 19 = 2 + 5 + 17, against the
// k-combinations of the primes and a dynamic programming over ( count ,
// sum ), k = 0 and n = 0, a prime alone; n = 1, a prime twice, 2 + 3, 97
// alone, k above the primes; n or k negative refused
ADS_TEST( "backtrack-19" )
{
 auto run = []( int n , int k ) {
  std::vector< std::vector< int > > out;
  backtrack::primes( n , k , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 24 , 3 ) ) == std::vector< std::vector< int > >( {
         { 2 , 3 , 19 } , { 2 , 5 , 17 } } ) );
 for( int n = 0 ; n <= 60 ; ++n ) {
  std::vector< int > P;
  for( int x = 2 ; x <= n ; ++x ) {
   bool prime = true;
   for( int d = 2 ; d * d <= x ; ++d ) prime = prime && ( x % d != 0 );
   if( prime ) P.push_back( x );
   }
  const int p = int( P.size() );
  std::vector< std::vector< long > > W( std::size_t( p ) + 1 ,  // W[ c ][ s ]
                               std::vector< long >( std::size_t( n ) + 1 ,
                                                    0 ) );
  W[ 0 ][ 0 ] = 1;
  for( int x : P )
   for( int c = p ; c >= 1 ; --c )
    for( int s = n ; s >= x ; --s ) W[ c ][ s ] += W[ c - 1 ][ s - x ];
  for( int k = 0 ; k <= std::min( p , 5 ) ; ++k ) {
   const auto got = run( n , k );
   CHECK( long( got.size() ) == W[ k ][ n ] );
   if( p <= 12 ) {
    std::vector< std::vector< int > > want;
    for( unsigned m = 0 ; m < ( 1u << p ) ; ++m ) {
     if( std::popcount( m ) != k ) continue;
     const auto S = pick( P , m );
     if( std::accumulate( S.begin() , S.end() , 0 ) == n )
      want.push_back( S );
     }
    CHECK( sorted( got ) == sorted( want ) );
    }
   }
  }
 CHECK( run( 0 , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( 5 , 0 ).empty() && run( 0 , 2 ).empty() );
 CHECK( run( 2 , 1 ) == std::vector< std::vector< int > >( 1 , { 2 } ) );
 // no prime below 2, a sum of a prime twice (not distinct), the smallest
 // sum of two, n prime and k = 1, k larger than the primes
 CHECK( run( 1 , 1 ).empty() && run( 4 , 2 ).empty() );
 CHECK( run( 5 , 2 ) == std::vector< std::vector< int > >( 1 , { 2 , 3 } ) );
 CHECK( run( 97 , 1 ) == std::vector< std::vector< int > >( 1 , { 97 } ) );
 CHECK( run( 10 , 5 ).empty() );
 CHECK_THROWS( run( -1 , 1 ) , contract_violation );
 CHECK_THROWS( run( 5 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 20. 2021-06-16, Stampa percorsi                                          */
/*--------------------------------------------------------------------------*/

// printPaths() on the example n = 2, against the strings of 2n letters with
// n D, \binom{2n}{n} of them as binomial() and subsets() of the library,
// n = 0
ADS_TEST( "backtrack-20" )
{
 auto run = []( int n ) {
  std::vector< std::string > out;
  backtrack::printPaths( n , [ & ]( const std::string & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 2 ) ) ==
        sorted( words( "RRDD RDRD RDDR DRRD DRDR DDRR" ) ) );
 for( int n = 0 ; n <= 7 ; ++n ) {
  std::vector< std::string > want;
  for( unsigned x = 0 ; x < ( 1u << ( 2 * n ) ) ; ++x )
   if( std::popcount( x ) == n ) {
    std::string s = bitString( x , 2 * n );
    for( auto & ch : s ) ch = ( ch == '1' ) ? 'D' : 'R';
    want.push_back( s );
    }
  const auto got = run( n );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( got.size() == binomial( 2 * n , n ) );
  CHECK( long( got.size() ) == subsets( 2 * n , n , []( const auto & ) {
   return( false ); } ) );
  }
 CHECK( run( 0 ) == std::vector< std::string >( 1 , "" ) );
 CHECK_THROWS( run( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 21. 2021-07-26, PrintBits                                                */
/*--------------------------------------------------------------------------*/

// printBits() on the example n = 5 (13 strings), against the 2^n strings
// with x & ( x >> 1 ) = 0, F_{n+2} = fibonacci( n + 1 ) of them, n = 0,
// 1, 2; n < 0 refused
ADS_TEST( "backtrack-21" )
{
 auto run = []( int n ) {
  std::vector< std::string > out;
  backtrack::printBits( n , [ & ]( const std::string & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 5 ) ) == words( "00000 00001 00010 00100 00101 01000 "
                                     "01001 01010 10000 10001 10010 10100 "
                                     "10101" ) );
 for( int n = 0 ; n <= 16 ; ++n ) {
  std::vector< std::string > want;
  for( unsigned x = 0 ; x < ( 1u << n ) ; ++x )
   if( ( x & ( x >> 1 ) ) == 0 ) want.push_back( bitString( x , n ) );
  const auto got = run( n );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( got.size() == fibonacci( n + 1 ) );
  }
 CHECK( run( 0 ) == std::vector< std::string >( 1 , "" ) );
 CHECK( sorted( run( 1 ) ) == words( "0 1" ) );
 CHECK( sorted( run( 2 ) ) == words( "00 01 10" ) );
 CHECK_THROWS( run( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 22. 2021-08-31, Sequenza k-limitata                                      */
/*--------------------------------------------------------------------------*/

// printKSequence() on the examples, against the 2^n index sets filtered as
// multisets, the count of a dynamic programming on the last item, the
// empty vector, max - min <= k (all 2^n), k = 0, the extremes of int
ADS_TEST( "backtrack-22" )
{
 auto run = []( const std::vector< int > & A , int k ) {
  std::vector< std::vector< int > > out;
  backtrack::printKSequence( A , k , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 auto limited = []( const std::vector< int > & S , int k ) {
  for( std::size_t i = 1 ; i < S.size() ; ++i )
   if( std::abs( S[ i ] - S[ i - 1 ] ) > k ) return( false );
  return( true ); };
 CHECK( limited( { 1 , 3 , 2 , 4 } , 2 ) &&
        ! limited( { 1 , 3 , 2 , 4 } , 1 ) );
 const auto ex = run( { 4 , 6 , 3 , 1 , 4 } , 1 );
 for( const std::vector< int > & s : std::vector< std::vector< int > >( {
       { 4 , 3 , 4 } , { 4 , 3 } , { 4 , 4 } , { 4 } , { 1 } , { 3 } } ) )
  CHECK( std::find( ex.begin() , ex.end() , s ) != ex.end() );
 std::mt19937 g( 22 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = int( g() % 12 ) , k = int( g() % 4 );
  const auto A = randomInts( n , 0 , 8 , g );
  std::vector< std::vector< int > > want;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   const auto S = pick( A , m );
   if( limited( S , k ) ) want.push_back( S );
   }
  std::vector< long > end( std::size_t( n ) , 1 );
  long count = 1;
  for( int i = 0 ; i < n ; ++i ) {
   for( int j = 0 ; j < i ; ++j )
    if( std::abs( A[ j ] - A[ i ] ) <= k ) end[ i ] += end[ j ];
   count += end[ i ];
   }
  const auto got = run( A , k );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( long( got.size() ) == count );
  }
 CHECK( run( {} , 1 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( { 5 , 6 , 7 , 5 , 6 } , 2 ).size() == 32 );
 // k = 0: only runs of equal items; the extremes of int, whose difference
 // overflows an int
 CHECK( run( { 3 , 3 , 4 } , 0 ).size() == 1 + 3 + 1 );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( sorted( run( { lo , hi } , 0 ) ) ==
        sorted( std::vector< std::vector< int > >( { {} , { lo } ,
                                                      { hi } } ) ) );
 CHECK( run( { lo , hi } , hi ).size() == 3 );
 CHECK( run( { hi , 0 } , hi ).size() == 4 );
 CHECK( run( { hi , -1 } , hi ).size() == 3 );
 CHECK( run( { lo , 0 , hi } , hi ).size() == 1 + 3 + 1 );
 CHECK_THROWS( run( { 1 } , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 23. 2021-08-31, PrintBitsK                                               */
/*--------------------------------------------------------------------------*/

// printBitsK() on the example n = 5 , k = 2 (24 strings), against the 2^n
// strings filtered by their longest run of 1, the count of a dynamic
// programming on the current run, k = 0, k >= n, n = 0 (also with k = 0),
// k = 1 on 2 bits; n or k negative refused
ADS_TEST( "backtrack-23" )
{
 auto run = []( int n , int k ) {
  std::vector< std::string > out;
  backtrack::printBitsK( n , k , [ & ]( const std::string & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 5 , 2 ) ) == sorted( words(
         "00000 10000 01000 11000 00100 10100 01100 00010 10010 01010 "
         "11010 00110 10110 00001 10001 01001 11001 00101 10101 01101 "
         "00011 10011 01011 11011" ) ) );
 for( int n = 0 ; n <= 14 ; ++n )
  for( int k = 0 ; k <= n + 1 ; ++k ) {
   std::vector< std::string > want;
   for( unsigned x = 0 ; x < ( 1u << n ) ; ++x ) {
    const std::string s = bitString( x , n );
    if( longestRun( s , '1' ) <= k ) want.push_back( s );
    }
   std::vector< long > R( std::size_t( k ) + 1 , 0 );   // by the last run
   R[ 0 ] = 1;
   for( int i = 0 ; i < n ; ++i ) {
    std::vector< long > Z( std::size_t( k ) + 1 , 0 );
    for( int r = 0 ; r <= k ; ++r ) {
     Z[ 0 ] += R[ r ];
     if( r < k ) Z[ r + 1 ] += R[ r ];
     }
    R = Z;
    }
   const auto got = run( n , k );
   CHECK( sorted( got ) == sorted( want ) );
   CHECK( long( got.size() ) == std::accumulate( R.begin() , R.end() , 0L ) );
   }
 CHECK( run( 4 , 0 ) == std::vector< std::string >( 1 , "0000" ) );
 CHECK( run( 6 , 6 ).size() == 64 );
 CHECK( run( 0 , 3 ) == std::vector< std::string >( 1 , "" ) );
 CHECK( run( 0 , 0 ) == std::vector< std::string >( 1 , "" ) );
 CHECK( sorted( run( 2 , 1 ) ) == words( "00 01 10" ) );
 CHECK_THROWS( run( -1 , 1 ) , contract_violation );
 CHECK_THROWS( run( 3 , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 24. 2022-01-17, Ottali                                                   */
/*--------------------------------------------------------------------------*/

// octals() on the example n = 2, in the order of the solution, against the
// 8^n strings filtered, 8 7^{n-1} of them, n = 0 (one string, out of the
// formula), n = 1 in order; n < 0 refused
ADS_TEST( "backtrack-24" )
{
 auto run = []( int n ) {
  std::vector< std::string > out;
  backtrack::octals( n , [ & ]( const std::string & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( run( 2 ) == words(
         "10 20 30 40 50 60 70 01 21 31 41 51 61 71 02 12 32 42 52 62 72 "
         "03 13 23 43 53 63 73 04 14 24 34 54 64 74 05 15 25 35 45 65 75 "
         "06 16 26 36 46 56 76 07 17 27 37 47 57 67" ) );
 for( int n = 1 ; n <= 6 ; ++n ) {
  std::vector< std::string > want;
  long total = 1;
  for( int i = 0 ; i < n ; ++i ) total *= 8;
  for( long code = 0 ; code < total ; ++code ) {
   std::string s;
   for( long c = code , i = 0 ; i < n ; ++i , c /= 8 )
    s += char( '0' + c % 8 );
   bool ok = true;
   for( int i = 1 ; i < n ; ++i ) ok = ok && ( s[ i ] != s[ i - 1 ] );
   if( ok ) want.push_back( s );
   }
  const auto got = run( n );
  CHECK( sorted( got ) == sorted( want ) );
  long count = 8;
  for( int i = 1 ; i < n ; ++i ) count *= 7;
  CHECK( long( got.size() ) == count );
  }
 CHECK( run( 0 ) == std::vector< std::string >( 1 , "" ) );
 CHECK( run( 1 ) == words( "0 1 2 3 4 5 6 7" ) );   // 8, not 8 7^0 / 7
 CHECK_THROWS( run( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 25. 2022-02-07, Bit consecutivi                                          */
/*--------------------------------------------------------------------------*/

// binary() on the example n = 6 , n0 = 2 , n1 = 3 (36 strings), against the
// 2^n strings filtered by their runs, the count of a dynamic programming
// on ( last bit , run ), the bounds >= n (all 2^n), n = 1, both bounds 1
// (the two alternations), one bound 1; n, n0 or n1 not positive refused
ADS_TEST( "backtrack-25" )
{
 auto run = []( int n , int n0 , int n1 ) {
  std::vector< std::string > out;
  backtrack::binary( n , n0 , n1 , [ & ]( const std::string & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 6 , 2 , 3 ) ) == sorted( words(
         "100100 010100 110100 001100 101100 011100 010010 110010 001010 "
         "101010 011010 111010 100110 010110 110110 001110 101110 001001 "
         "101001 011001 111001 100101 010101 110101 001101 101101 011101 "
         "010011 110011 001011 101011 011011 111011 100111 010111 "
         "110111" ) ) );
 for( int n = 1 ; n <= 13 ; ++n )
  for( int n0 = 1 ; n0 <= 4 ; ++n0 )
   for( int n1 = 1 ; n1 <= 4 ; ++n1 ) {
    std::vector< std::string > want;
    for( unsigned x = 0 ; x < ( 1u << n ) ; ++x ) {
     const std::string s = bitString( x , n );
     if( ( longestRun( s , '0' ) <= n0 ) && ( longestRun( s , '1' ) <= n1 ) )
      want.push_back( s );
     }
    std::map< std::pair< int , int > , long > R = { { { 0 , 1 } , 1 } ,
                                                    { { 1 , 1 } , 1 } };
    for( int i = 1 ; i < n ; ++i ) {
     std::map< std::pair< int , int > , long > Z;
     for( auto [ key , w ] : R ) {
      const auto [ b , r ] = key;
      if( r < ( b ? n1 : n0 ) ) Z[ { b , r + 1 } ] += w;
      Z[ { 1 - b , 1 } ] += w;
      }
     R = Z;
     }
    long count = 0;
    for( auto [ key , w ] : R ) count += w;
    const auto got = run( n , n0 , n1 );
    CHECK( sorted( got ) == sorted( want ) );
    CHECK( long( got.size() ) == count );
    }
 CHECK( run( 7 , 7 , 7 ).size() == 128 );
 CHECK( sorted( run( 1 , 1 , 1 ) ) == words( "0 1" ) );
 // runs of 1: only the two alternations; one kind unbounded and the other
 // of 1
 CHECK( sorted( run( 5 , 1 , 1 ) ) == words( "01010 10101" ) );
 CHECK( run( 6 , 6 , 1 ).size() == 21 );        // no two 1 together: F_8
 CHECK_THROWS( run( 3 , 0 , 1 ) , contract_violation );
 CHECK_THROWS( run( 3 , 1 , 0 ) , contract_violation );
 CHECK_THROWS( run( 0 , 1 , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 26. 2022-06-21, Math ways                                                */
/*--------------------------------------------------------------------------*/

// mathWays() on the example n = 10 , k = 3 (5 sequences), and for every
// 3 <= k <= n <= 80 each step valid, no two sequences equal, as many as
// the ways counted by dynamic programming; k = n, k = n - 1, k and n at
// the largest int (2 k and k^2 beyond int), k < 3 and k > n refused
ADS_TEST( "backtrack-26" )
{
 using Seq = std::vector< std::int64_t >;
 auto run = []( int n , int k ) {
  std::vector< Seq > out;
  backtrack::mathWays( n , k , [ & ]( const Seq & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 10 , 3 ) ) == sorted( std::vector< Seq >( {
         { 3 , 4 , 5 , 6 , 7 , 8 , 9 , 10 } , { 3 , 4 , 5 , 10 } ,
         { 3 , 4 , 8 , 9 , 10 } , { 3 , 6 , 7 , 8 , 9 , 10 } ,
         { 3 , 9 , 10 } } ) ) );
 for( int n = 3 ; n <= 80 ; ++n ) {
  std::vector< long > W( std::size_t( n ) + 1 , 0 );
  W[ n ] = 1;
  for( int v = n - 1 ; v >= 1 ; --v ) {
   W[ v ] = W[ v + 1 ];
   if( 2 * v <= n ) W[ v ] += W[ 2 * v ];
   if( long( v ) * v <= n ) W[ v ] += W[ v * v ];
   }
  for( int k = 3 ; k <= n ; ++k ) {
   if( W[ k ] > 20000 ) continue;
   auto got = run( n , k );
   CHECK( long( got.size() ) == W[ k ] );
   for( const auto & S : got ) {
    CHECK( ( S.front() == k ) && ( S.back() == n ) );
    for( std::size_t i = 1 ; i < S.size() ; ++i )
     CHECK( ( S[ i ] == S[ i - 1 ] + 1 ) || ( S[ i ] == 2 * S[ i - 1 ] ) ||
            ( S[ i ] == S[ i - 1 ] * S[ i - 1 ] ) );
    }
   got = sorted( got );
   CHECK( std::adjacent_find( got.begin() , got.end() ) == got.end() );
   }
  }
 CHECK( run( 9 , 9 ) == std::vector< Seq >( 1 , { 9 } ) );
 // k = n - 1: one step of 1; the largest int, where 2 k and k^2 are out of
 // int (computed in 64 bits)
 CHECK( run( 80 , 79 ) == std::vector< Seq >( 1 , { 79 , 80 } ) );
 const int hi = std::numeric_limits< int >::max();
 CHECK( run( hi , hi - 1 ) == std::vector< Seq >( 1 , { hi - 1 , hi } ) );
 CHECK( run( hi , hi ) == std::vector< Seq >( 1 , { hi } ) );
 CHECK( run( 9 , 3 ).size() == 4 );        // 3..9, 3 4 8 9, 3 6..9, 3 9
 CHECK_THROWS( run( 5 , 2 ) , contract_violation );
 CHECK_THROWS( run( 5 , 6 ) , contract_violation );
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
/*---------------------- End File exam_backtracka.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
