/*--------------------------------------------------------------------------*/
/*------------------------ File exam_backtrackb.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems 28 to 47 of a collection of problems of enumeration by
 * backtrack (the problems 1 to 27 are in exam_backtracka.cpp), each with the
 * algorithm of its solution and a test against an independent oracle: a brute
 * force over all the candidates (bit masks, tuples, permutations), a count by
 * dynamic programming or by a closed form, the examples of the statements
 * with their outputs and the edge cases. Where the library already has the
 * algorithm (Hamiltonian circuits, derangements, binomial coefficients and
 * numbers of Fibonacci) the test calls it. The algorithms are in the
 * namespace backtrack, one per problem, each with its idea, its complexity
 * and its preconditions; a function marked as a candidate for the library is
 * general and missing from it.
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
 *         Universita' di Pisa \n
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
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Paths.h"
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
/* 28. 2022-08-24, Permutazioni eleganti                                    */
/*--------------------------------------------------------------------------*/

/// printElegant(): the permutations a_1 ... a_n of 1 ... n with a_i
/// divisible by i or i by a_i, f( S ) on each; the position i takes each
/// unused value compatible with i; O( n^2 n! )
/// \pre n >= 0
template< class F >
void printElegant( int n , F && f )
{
 ADS_PRE( n >= 0 );
 std::vector< bool > used( std::size_t( n ) + 1 , false );
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i > n ) {
   f( S );
   return;
   }
  for( int c = 1 ; c <= n ; ++c )
   if( ! used[ c ] && ( ( c % i == 0 ) || ( i % c == 0 ) ) ) {
    used[ c ] = true;
    S.push_back( c );
    self( self , i + 1 );
    S.pop_back();
    used[ c ] = false;
    }
  };
 rec( rec , 1 );
 }

/// printElegantFast(): the same permutations by swaps, the position i,
/// from n down to 2, takes any of S[ 1 .. i ] compatible with i; O( n n! )
/// \pre n >= 1
template< class F >
void printElegantFast( int n , F && f )
{
 ADS_PRE( n >= 1 );
 std::vector< int > S( std::size_t( n ) + 1 );
 std::iota( S.begin() , S.end() , 0 );
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == 1 ) {
   f( std::vector< int >( S.begin() + 1 , S.end() ) );
   return;
   }
  for( int j = 1 ; j <= i ; ++j )
   if( ( S[ j ] % i == 0 ) || ( i % S[ j ] == 0 ) ) {
    std::swap( S[ i ] , S[ j ] );
    self( self , i - 1 );
    std::swap( S[ i ] , S[ j ] );
    }
  };
 rec( rec , n );
 }

/*--------------------------------------------------------------------------*/
/* 29. 2023-01-17, Min-Cardinalita'                                         */
/*--------------------------------------------------------------------------*/

/// minCardinality(): the non-empty subsets of { 1 , ... , n } whose minimum
/// is their size, f( S ) on each, increasing; the minimum k fixed first, at
/// most \f$ \lceil n / 2 \rceil \f$, then k - 1 larger items increasing;
/// \f$ F_n \f$ sets
/// \pre n >= 0
template< class F >
void minCardinality( int n , F && f )
{
 ADS_PRE( n >= 0 );
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == S[ 0 ] ) {
   f( S );
   return;
   }
  for( int j = S[ i - 1 ] + 1 ; j <= n ; ++j ) {
   S.push_back( j );
   self( self , i + 1 );
   S.pop_back();
   }
  };
 for( int k = 1 ; k <= ( n + 1 ) / 2 ; ++k ) {
  S.assign( 1 , k );
  rec( rec , 1 );
  }
 }

/*--------------------------------------------------------------------------*/
/* 30. 2023-06-20, Misciotto 1                                              */
/*--------------------------------------------------------------------------*/

/// printMixing(): the interleavings of S1 and S2, the order inside each
/// kept, f( T ) on each; a character of S1 if any is left, then one of S2;
/// \f$ O( ( n_1 + n_2 ) \binom{n_1 + n_2}{n_1} ) \f$
template< class F >
void printMixing( const std::string & S1 , const std::string & S2 , F && f )
{
 const int n1 = int( S1.size() ) , n2 = int( S2.size() );
 std::string T( S1.size() + S2.size() , ' ' );
 auto rec = [ & ]( auto & self , int i , int j ) -> void {
  if( ( i == n1 ) && ( j == n2 ) ) {
   f( T );
   return;
   }
  if( i < n1 ) {
   T[ i + j ] = S1[ i ];
   self( self , i + 1 , j );
   }
  if( j < n2 ) {
   T[ i + j ] = S2[ j ];
   self( self , i , j + 1 );
   }
  };
 rec( rec , 0 , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 31. 2023-07-11, 10.000 passi                                             */
/*--------------------------------------------------------------------------*/

/// searchPath(): a walk from src to dest of weight exactly target that
/// never goes back along the edge just used, or nothing; no visited set,
/// only the previous node excluded, pruned when an edge exceeds what is
/// left; \f$ O( n^{target} ) \f$. The text pops the stack after the return
/// of a success, so the nodes of a failed branch stay on it.
/// \pre W is symmetric with weights >= 0 (0 for no edge), src and dest are
///      nodes, target >= 0
inline std::optional< std::vector< int > > searchPath( const Weights & W ,
                                                       int src , int dest ,
                                                       int target )
{
 const int n = int( W.size() );
 ADS_PRE( detail::symmetric( W ) && ( target >= 0 ) );
 ADS_PRE( ( src >= 0 ) && ( src < n ) && ( dest >= 0 ) && ( dest < n ) );
 std::vector< int > stack = { src };
 auto rec = [ & ]( auto & self , int curr , int prev ,
                   int remaining ) -> bool {
  if( ( curr == dest ) && ( remaining == 0 ) ) return( true );
  if( remaining > 0 )
   for( int next = 0 ; next < n ; ++next )
    if( ( W[ curr ][ next ] > 0 ) && ( next != prev ) &&
        ( W[ curr ][ next ] <= remaining ) ) {
     stack.push_back( next );
     if( self( self , next , curr , remaining - W[ curr ][ next ] ) )
      return( true );
     stack.pop_back();
     }
  return( false );
  };
 if( ! rec( rec , src , -1 , target ) ) return( std::nullopt );
 return( stack );
 }

/*--------------------------------------------------------------------------*/
/* 32. 2023-08-23, Sub-palindrome                                           */
/*--------------------------------------------------------------------------*/

/// subPalindrome(): the palindromic subsequences of S, one per set of
/// positions, the empty one included, f( T ) on each; all the 2^n
/// subsequences, each tested at the leaf by two pointers;
/// \f$ \Theta( n 2^n ) \f$
template< class F >
void subPalindrome( const std::string & S , F && f )
{
 const int n = int( S.size() );
 std::string sub;
 auto palindrome = [ & ]( void ) {
  for( int i = 0 , j = int( sub.size() ) - 1 ; i < j ; ++i , --j )
   if( sub[ i ] != sub[ j ] ) return( false );
  return( true ); };
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == n ) {
   if( palindrome() ) f( sub );
   return;
   }
  self( self , i + 1 );
  sub.push_back( S[ i ] );
  self( self , i + 1 );
  sub.pop_back();
  };
 rec( rec , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 33. 2024-01-09, Tutti i cicli!                                           */
/*--------------------------------------------------------------------------*/

/// printCycles(): the simple oriented cycles through start, f( C ) on the
/// nodes of each from start back to start: simpleCycles() of the library
/// on the graph of the matrix, a depth first visit with a stack, a node
/// blocked while no path from it to start avoids the stack, a cycle closed
/// by every arc back to start; \f$ \lfloor e ( n - 1 )! \rfloor - 1 \f$
/// cycles on a complete graph, O( ( n + m ) ( c + 1 ) ) for c cycles
/// \pre G is square, start is a node
template< class F >
void printCycles( const Adjacency & G , int start , F && f )
{
 const int n = int( G.size() );
 ADS_PRE( detail::square( G ) );
 ListGraph<> g( n , true );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( ( u != v ) && G[ u ][ v ] )
    g.insertEdge( u , v );
 simpleCycles( g , start , f );
 }

/*--------------------------------------------------------------------------*/
/* 34. 2024-02-01, Meta' o meno                                             */
/*--------------------------------------------------------------------------*/

/// lessThanHalf(): the sequences of positive integers from n, each item at
/// most half the previous one, f( S ) on each; a stack printed at every
/// call, then each i <= top / 2 pushed in turn;
/// \f$ T( n ) = 1 + \sum_{i = 1}^{\lfloor n / 2 \rfloor} T( i ) \f$
/// sequences, superpolynomial
/// \pre n > 0
template< class F >
void lessThanHalf( int n , F && f )
{
 ADS_PRE( n > 0 );
 std::vector< int > stack = { n };
 auto rec = [ & ]( auto & self ) -> void {
  f( stack );
  for( int i = 1 ; i <= stack.back() / 2 ; ++i ) {
   stack.push_back( i );
   self( self );
   stack.pop_back();
   }
  };
 rec( rec );
 }

/*--------------------------------------------------------------------------*/
/* 35. 2024-06-14, Magic square                                             */
/*--------------------------------------------------------------------------*/

/// isMagic(): the rows, the columns and the two diagonals of A have the
/// same sum; \f$ \Theta( n^2 ) \f$
/// \pre A is square
inline bool isMagic( const Grid & A )
{
 ADS_PRE( detail::square( A ) );
 const int n = int( A.size() );
 long long d1 = 0 , d2 = 0;                    // 64 bits: sums of int
 for( int i = 0 ; i < n ; ++i ) {
  d1 += A[ i ][ i ];
  d2 += A[ i ][ n - 1 - i ];
  }
 if( d1 != d2 ) return( false );
 for( int i = 0 ; i < n ; ++i ) {
  long long r = 0 , c = 0;
  for( int j = 0 ; j < n ; ++j ) {
   r += A[ i ][ j ];
   c += A[ j ][ i ];
   }
  if( ( r != d1 ) || ( c != d1 ) ) return( false );
  }
 return( true );
 }

/// printMagic(): the n x n magic squares of 1 ... n^2, f( A ) on each; the
/// permutations of 1 ... n^2 row by row, a row pruned when its sum is not
/// n ( n^2 + 1 ) / 2, isMagic() on each full square; O( n^2 ( n^2 )! )
/// without the pruning
/// \pre n > 0
template< class F >
void printMagic( int n , F && f )
{
 ADS_PRE( n > 0 );
 const long sum = long( n ) * ( long( n ) * n + 1 ) / 2;
 Grid A( std::size_t( n ) , std::vector< int >( std::size_t( n ) , 0 ) );
 std::vector< bool > left( std::size_t( n ) * n + 1 , true );
 auto rec = [ & ]( auto & self , int i , int j ) -> void {
  if( j == n ) {
   if( std::accumulate( A[ i ].begin() , A[ i ].end() , 0L ) != sum )
    return;
   j = 0;
   ++i;
   if( i == n ) {
    if( isMagic( A ) ) f( A );
    return;
    }
   }
  for( int k = 1 ; k <= n * n ; ++k )
   if( left[ k ] ) {
    A[ i ][ j ] = k;
    left[ k ] = false;
    self( self , i , j + 1 );
    left[ k ] = true;
    }
  };
 rec( rec , 0 , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 37. 2024-08-26, Zigzaghiamo!                                             */
/*--------------------------------------------------------------------------*/

/// printZigZag(): the subsequences x_0 , x_1 , ... of A with x_i < x_{i+1}
/// for i even and x_i > x_{i+1} for i odd, the empty one included, f( S )
/// on each; skip A[ iA ], or take it when its difference from the last
/// item has the sign opposite to dir, which flips at each item taken;
/// O( n 2^n ). The difference is taken in long long, since that of two int
/// may overflow
template< class F >
void printZigZag( const std::vector< int > & A , F && f )
{
 const int n = int( A.size() );
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int iA , int dir ) -> void {
  if( iA == n ) {
   f( S );
   return;
   }
  self( self , iA + 1 , dir );
  if( S.empty() || ( ( (long long)( A[ iA ] ) - S.back() ) * dir < 0 ) ) {
   S.push_back( A[ iA ] );
   self( self , iA + 1 , -dir );
   S.pop_back();
   }
  };
 rec( rec , 0 , 1 );
 }

/*--------------------------------------------------------------------------*/
/* 38. 2025-01-14, Radice-foglia                                            */
/*--------------------------------------------------------------------------*/

/// allPaths(): the root-to-leaf paths of T, f( path ) on the values of
/// each; a depth first visit with a stack, printed at each leaf; O( n^2 ),
/// reached by a chain of n / 2 nodes above a complete tree
template< class F >
void allPaths( const Tree & T , F && f )
{
 std::vector< int > path;
 auto rec = [ & ]( auto & self , int t ) -> void {
  if( t < 0 ) return;
  path.push_back( T.value[ t ] );
  if( T.leaf( t ) ) f( path );
  self( self , T.left[ t ] );
  self( self , T.right[ t ] );
  path.pop_back();
  };
 rec( rec , T.root );
 }

/*--------------------------------------------------------------------------*/
/* 39. 2025-02-03, Numeri irripetibili                                      */
/*--------------------------------------------------------------------------*/

/// printNumbers(): the integers of [ 0 , 10^n ) with distinct digits, f( x )
/// on each; 0, then each first digit 1 ... 9 and the available digits after
/// it, a number printed at every call, at most min( n , 10 ) digits; at
/// most 8877691 numbers, a constant
/// \pre n >= 0
template< class F >
void printNumbers( int n , F && f )
{
 ADS_PRE( n >= 0 );
 f( std::int64_t( 0 ) );
 const int limit = std::min( n , 10 );
 if( limit == 0 ) return;
 std::vector< bool > available( 10 , true );
 auto rec = [ & ]( auto & self , int i , std::int64_t x ) -> void {
  f( x );
  if( i < limit )
   for( int k = 0 ; k <= 9 ; ++k )
    if( available[ k ] ) {
     available[ k ] = false;
     self( self , i + 1 , 10 * x + k );
     available[ k ] = true;
     }
  };
 for( int k = 1 ; k <= 9 ; ++k ) {
  available[ k ] = false;
  rec( rec , 1 , std::int64_t( k ) );
  available[ k ] = true;
  }
 }

/*--------------------------------------------------------------------------*/
/* 40. 2025-06-13, Tuple ben ordinate                                       */
/*--------------------------------------------------------------------------*/

/// printTuples(): the subsequences of A of 2 to k items strictly increasing
/// or strictly decreasing, f( S ) on each; two backtracks, up and down,
/// each printing a tuple as soon as its last item is added; O( n 2^n )
/// \pre 1 <= k <= n
template< class F >
void printTuples( const std::vector< int > & A , int k , F && f )
{
 const int n = int( A.size() );
 ADS_PRE( ( 1 <= k ) && ( k <= n ) );
 std::vector< int > S;
 auto run = [ & ]( auto less ) {
  auto rec = [ & ]( auto & self , int j ) -> void {
   if( ( int( S.size() ) >= k ) || ( j >= n ) ) return;
   self( self , j + 1 );
   if( S.empty() || less( S.back() , A[ j ] ) ) {
    S.push_back( A[ j ] );
    if( S.size() > 1 ) f( S );
    self( self , j + 1 );
    S.pop_back();
    }
   };
  rec( rec , 0 ); };
 run( []( int a , int b ) { return( a < b ); } );
 run( []( int a , int b ) { return( a > b ); } );
 }

/*--------------------------------------------------------------------------*/
/* 41. 2025-07-14, All paths                                                */
/*--------------------------------------------------------------------------*/

/// printAllPaths(): the paths of adjacent cells from the start (1) to the
/// end (2) through every free cell (0) once, avoiding the obstacles (-1),
/// f( P ) on the cells of each; remaining counts the free cells and the
/// start, a cell marked -1 while on the path; \f$ O( n^2 3^{n^2} ) \f$
/// \pre A is square of -1 , 0 , 1 , 2, with exactly one 1 and one 2
template< class F >
void printAllPaths( Grid A , F && f )
{
 const int n = int( A.size() );
 ADS_PRE( detail::square( A ) );
 int remaining = 0 , sr = -1 , sc = -1 , ones = 0 , twos = 0;
 for( int r = 0 ; r < n ; ++r )
  for( int c = 0 ; c < n ; ++c ) {
   ADS_PRE( ( A[ r ][ c ] >= -1 ) && ( A[ r ][ c ] <= 2 ) );
   if( ( A[ r ][ c ] == 0 ) || ( A[ r ][ c ] == 1 ) ) ++remaining;
   if( A[ r ][ c ] == 1 ) {
    sr = r;
    sc = c;
    ++ones;
    }
   if( A[ r ][ c ] == 2 ) ++twos;
   }
 ADS_PRE( ( ones == 1 ) && ( twos == 1 ) );
 std::vector< Cell > stack;
 auto rec = [ & ]( auto & self , int r , int c , int rem ) -> void {
  if( ( r < 0 ) || ( r >= n ) || ( c < 0 ) || ( c >= n ) ||
      ( A[ r ][ c ] == -1 ) )
   return;
  if( A[ r ][ c ] == 2 ) {
   if( rem == 0 ) {
    stack.emplace_back( r , c );
    f( stack );
    stack.pop_back();
    }
   return;
   }
  const int oldValue = A[ r ][ c ];
  A[ r ][ c ] = -1;
  stack.emplace_back( r , c );
  self( self , r + 1 , c , rem - 1 );
  self( self , r - 1 , c , rem - 1 );
  self( self , r , c + 1 , rem - 1 );
  self( self , r , c - 1 , rem - 1 );
  A[ r ][ c ] = oldValue;
  stack.pop_back();
  };
 rec( rec , sr , sc , remaining );
 }

/*--------------------------------------------------------------------------*/
/* 42. 2025-08-25, Sottoinsiemi k-distanti                                  */
/*--------------------------------------------------------------------------*/

/// printDistantSets(): the distinct sets of values of A any two of which
/// differ by more than k, the empty one included, f( S ) on each,
/// increasing; A sorted and without duplicates, then a value taken only if
/// more than k above the last one; O( n log n + n 2^m ), m distinct values
/// \pre k > 0
template< class F >
void printDistantSets( std::vector< int > A , int k , F && f )
{
 ADS_PRE( k > 0 );
 std::sort( A.begin() , A.end() );
 A.erase( std::unique( A.begin() , A.end() ) , A.end() );
 const int m = int( A.size() );
 std::vector< int > S;
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == m ) {
   f( S );
   return;
   }
  self( self , i + 1 );
  if( S.empty() || ( (long long)( A[ i ] ) - S.back() > k ) ) {
   S.push_back( A[ i ] );
   self( self , i + 1 );
   S.pop_back();
   }
  };
 rec( rec , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 43. 2026-01-12, Numeri zerabili                                          */
/*--------------------------------------------------------------------------*/

/// printZerable(): the strings of n bits with more 0 than 1, f( S ) on
/// each; left to right, a 0 always, a 1 only if 2 ( ones + 1 ) < n;
/// \f$ \sum_{k < n / 2} \binom{n}{k} = \Theta( 2^{n - 1} ) \f$ strings
/// \pre n >= 1
template< class F >
void printZerable( int n , F && f )
{
 ADS_PRE( n >= 1 );
 std::string S( std::size_t( n ) , '0' );
 auto rec = [ & ]( auto & self , int i , int ones ) -> void {
  if( i == n ) {
   f( S );
   return;
   }
  S[ i ] = '0';
  self( self , i + 1 , ones );
  if( ones + 1 < n - ( ones + 1 ) ) {
   S[ i ] = '1';
   self( self , i + 1 , ones + 1 );
   }
  };
 rec( rec , 0 , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 44. 2026-02-02, New York Dreaming                                        */
/*--------------------------------------------------------------------------*/

/// printPaths( G , s , k ): the closed paths from s back to s with no
/// repeated node and length in [ k - tol , k + tol ], f( P ) on each; a
/// depth first visit with the missing length, an arc kept only while it
/// stays >= -tol (positive lengths), s closing the path only after another
/// node, so that a loop on s is not a closed path; O( n n! )
/// \pre W is square with lengths >= 0 (0 for no arc), s is a node,
///      tol >= 0
template< class F >
void printPaths( const Weights & W , int s , int k , int tol , F && f )
{
 const int n = int( W.size() );
 ADS_PRE( detail::square( W ) && ( s >= 0 ) && ( s < n ) && ( tol >= 0 ) );
 std::vector< bool > visited( std::size_t( n ) , false );
 std::vector< int > path = { s };
 visited[ s ] = true;
 auto rec = [ & ]( auto & self , int u , long missing ) -> void {
  for( int v = 0 ; v < n ; ++v ) {
   if( W[ u ][ v ] <= 0 ) continue;
   if( v == s ) {
    if( ( path.size() > 1 ) && ( -tol <= missing - W[ u ][ s ] ) &&
        ( missing - W[ u ][ s ] <= tol ) ) {
     path.push_back( s );
     f( path );
     path.pop_back();
     }
    }
   else
    if( ! visited[ v ] && ( missing - W[ u ][ v ] >= -tol ) ) {
     visited[ v ] = true;
     path.push_back( v );
     self( self , v , missing - W[ u ][ v ] );
     path.pop_back();
     visited[ v ] = false;
     }
   }
  };
 rec( rec , s , k );
 }

/*--------------------------------------------------------------------------*/
/* 45. 2026-06-22, Shakira                                                  */
/*--------------------------------------------------------------------------*/

/// setlists(): the sequences of m distinct songs, never three consecutive
/// ones of the same type, f( S ) on the indices of each; each unused song
/// whose type does not make three in a row with the last two; O( n n! /
/// ( n - m )! ). The text nests the recursion inside the test that rejects
/// the song, so that it is never reached.
/// \pre 0 <= m <= n
template< class F >
void setlists( const std::string & type , int m , F && f )
{
 const int n = int( type.size() );
 ADS_PRE( ( 0 <= m ) && ( m <= n ) );
 std::vector< bool > used( std::size_t( n ) , false );
 std::vector< int > S( std::size_t( m ) , 0 );
 auto rec = [ & ]( auto & self , int i ) -> void {
  if( i == m ) {
   f( S );
   return;
   }
  for( int song = 0 ; song < n ; ++song ) {
   if( used[ song ] ) continue;
   if( ( i >= 2 ) && ( type[ song ] == type[ S[ i - 1 ] ] ) &&
       ( type[ song ] == type[ S[ i - 2 ] ] ) )
    continue;
   used[ song ] = true;
   S[ i ] = song;
   self( self , i + 1 );
   used[ song ] = false;
   }
  };
 rec( rec , 0 );
 }

/*--------------------------------------------------------------------------*/
/* 46. 2026-07-20, Antipalindromo                                           */
/*--------------------------------------------------------------------------*/

/// noPalindrome(): the strings of length n over alphabet without a
/// palindromic substring of length >= 2, f( S ) on each; left to right,
/// after S[ curr ] only the substrings ending in curr are tested;
/// O( k^n n^2 ). The text advances start only when a palindrome is found,
/// so the loop does not end when there is none.
/// \pre the characters of alphabet are distinct, n >= 0
template< class F >
void noPalindrome( const std::string & alphabet , int n , F && f )
{
 ADS_PRE( n >= 0 );
 ADS_PRE( std::set< char >( alphabet.begin() , alphabet.end() ).size() ==
          alphabet.size() );
 std::string S( std::size_t( n ) , ' ' );
 auto palindrome = [ & ]( int i , int j ) {
  for( ; i < j ; ++i , --j )
   if( S[ i ] != S[ j ] ) return( false );
  return( true ); };
 auto rec = [ & ]( auto & self , int curr ) -> void {
  if( curr == n ) {
   f( S );
   return;
   }
  for( char c : alphabet ) {
   S[ curr ] = c;
   bool palindromeFree = true;
   for( int start = 0 ; palindromeFree && ( start < curr ) ; ++start )
    if( palindrome( start , curr ) ) palindromeFree = false;
   if( palindromeFree ) self( self , curr + 1 );
   }
  };
 rec( rec , 0 );
 }

/// noPalindromeFast(): the same strings, testing only the suffixes of
/// length 2 and 3, since every palindrome of length >= 2 contains one of
/// them; O( n k^n )
/// \pre the characters of alphabet are distinct, n >= 0
template< class F >
void noPalindromeFast( const std::string & alphabet , int n , F && f )
{
 ADS_PRE( n >= 0 );
 ADS_PRE( std::set< char >( alphabet.begin() , alphabet.end() ).size() ==
          alphabet.size() );
 std::string S( std::size_t( n ) , ' ' );
 auto rec = [ & ]( auto & self , int curr ) -> void {
  if( curr == n ) {
   f( S );
   return;
   }
  for( char c : alphabet ) {
   S[ curr ] = c;
   if( ( ( curr < 1 ) || ( S[ curr ] != S[ curr - 1 ] ) ) &&
       ( ( curr < 2 ) || ( S[ curr ] != S[ curr - 2 ] ) ) )
    self( self , curr + 1 );
   }
  };
 rec( rec , 0 );
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
/* 28. 2022-08-24, Permutazioni eleganti                                    */
/*--------------------------------------------------------------------------*/

// printElegant() and printElegantFast() on the example n = 5 (10
// permutations), against std::next_permutation filtered, the counts 1, 2,
// 3, 8, 10, 36, 41, 132, n = 0 for the first one, n = 1 for both; n < 0
// and, for the second one, n = 0 refused
ADS_TEST( "backtrack-28" )
{
 auto run = []( int n , bool fast ) {
  std::vector< std::vector< int > > out;
  auto f = [ & ]( const std::vector< int > & S ) { out.push_back( S ); };
  if( fast ) backtrack::printElegantFast( n , f );
  else backtrack::printElegant( n , f );
  return( out ); };
 const std::vector< std::vector< int > > five = {
  { 1 , 2 , 3 , 4 , 5 } , { 1 , 4 , 3 , 2 , 5 } , { 2 , 1 , 3 , 4 , 5 } ,
  { 2 , 4 , 3 , 1 , 5 } , { 3 , 2 , 1 , 4 , 5 } , { 3 , 4 , 1 , 2 , 5 } ,
  { 4 , 1 , 3 , 2 , 5 } , { 4 , 2 , 3 , 1 , 5 } , { 5 , 2 , 3 , 4 , 1 } ,
  { 5 , 4 , 3 , 2 , 1 } };
 CHECK( run( 5 , false ) == five );
 CHECK( sorted( run( 5 , true ) ) == five );
 const long count[] = { 1 , 2 , 3 , 8 , 10 , 36 , 41 , 132 };
 for( int n = 1 ; n <= 8 ; ++n ) {
  std::vector< int > P( std::size_t( n ) , 0 );
  std::iota( P.begin() , P.end() , 1 );
  std::vector< std::vector< int > > want;
  do {
   bool ok = true;
   for( int i = 1 ; i <= n && ok ; ++i )
    ok = ( P[ i - 1 ] % i == 0 ) || ( i % P[ i - 1 ] == 0 );
   if( ok ) want.push_back( P );
   }
  while( std::next_permutation( P.begin() , P.end() ) );
  CHECK( run( n , false ) == want );
  CHECK( sorted( run( n , true ) ) == want );
  CHECK( long( want.size() ) == count[ n - 1 ] );
  }
 CHECK( run( 0 , false ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( 1 , false ) == std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK( run( 1 , true ) == std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK_THROWS( run( 0 , true ) , contract_violation );
 CHECK_THROWS( run( -1 , false ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 29. 2023-01-17, Min-Cardinalita'                                         */
/*--------------------------------------------------------------------------*/

// minCardinality() on the example n = 6, against the 2^n subsets with
// minimum = size, F_n = fibonacci( n - 1 ) of them (1, 1, 2, 3, 5, ...),
// n = 0 (none, the empty set excluded), 1, 2, 3; n < 0 refused
ADS_TEST( "backtrack-29" )
{
 auto run = []( int n ) {
  std::vector< std::vector< int > > out;
  backtrack::minCardinality( n , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( run( 6 ) == std::vector< std::vector< int > >( {
         { 1 } , { 2 , 3 } , { 2 , 4 } , { 2 , 5 } , { 2 , 6 } ,
         { 3 , 4 , 5 } , { 3 , 4 , 6 } , { 3 , 5 , 6 } } ) );
 for( int n = 1 ; n <= 18 ; ++n ) {
  std::vector< int > all( std::size_t( n ) , 0 );
  std::iota( all.begin() , all.end() , 1 );
  std::vector< std::vector< int > > want;
  for( unsigned m = 1 ; m < ( 1u << n ) ; ++m ) {
   const auto S = pick( all , m );
   if( S[ 0 ] == int( S.size() ) ) want.push_back( S );
   }
  const auto got = run( n );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( got.size() == fibonacci( n - 1 ) );
  }
 CHECK( run( 0 ).empty() );
 CHECK( run( 1 ) == std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK( run( 2 ) == std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK( run( 3 ) == std::vector< std::vector< int > >( { { 1 } ,
                                                         { 2 , 3 } } ) );
 CHECK_THROWS( run( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 30. 2023-06-20, Misciotto 1                                              */
/*--------------------------------------------------------------------------*/

// printMixing() on the examples (ADBCEF is an interleaving of ABC and DEF,
// ABCCBA is not one of ABC and ABC), against the \binom{n_1 + n_2}{n_1}
// choices of the places of S1, each output checked by the dynamic
// programming of interleaving; empty strings (either and both), equal
// strings, the order of the outputs (S1 first)
ADS_TEST( "backtrack-30" )
{
 auto run = []( const std::string & S1 , const std::string & S2 ) {
  std::vector< std::string > out;
  backtrack::printMixing( S1 , S2 , [ & ]( const std::string & T ) {
   out.push_back( T ); } );
  return( out ); };
 auto interleaving = []( const std::string & a , const std::string & b ,
                         const std::string & t ) {
  const std::size_t n = a.size() , m = b.size();
  if( t.size() != n + m ) return( false );
  std::vector< std::vector< bool > > D( n + 1 , std::vector< bool >( m + 1 ,
                                                                    false ) );
  for( std::size_t i = 0 ; i <= n ; ++i )
   for( std::size_t j = 0 ; j <= m ; ++j )
    D[ i ][ j ] = ( ( i == 0 ) && ( j == 0 ) ) ||
                  ( ( i > 0 ) && D[ i - 1 ][ j ] &&
                    ( a[ i - 1 ] == t[ i + j - 1 ] ) ) ||
                  ( ( j > 0 ) && D[ i ][ j - 1 ] &&
                    ( b[ j - 1 ] == t[ i + j - 1 ] ) );
  return( bool( D[ n ][ m ] ) ); };
 const auto abc = run( "ABC" , "DEF" );
 CHECK( std::find( abc.begin() , abc.end() , "ADBCEF" ) != abc.end() );
 const auto twice = run( "ABC" , "ABC" );
 CHECK( std::find( twice.begin() , twice.end() , "ABCCBA" ) == twice.end() );
 CHECK( ! interleaving( "ABC" , "ABC" , "ABCCBA" ) );
 std::mt19937 g( 30 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n1 = int( g() % 6 ) , n2 = int( g() % 6 );
  std::string a , b;
  for( int i = 0 ; i < n1 ; ++i ) a += char( 'a' + g() % 3 );
  for( int i = 0 ; i < n2 ; ++i ) b += char( 'a' + g() % 3 );
  std::vector< std::string > want;
  for( unsigned x = 0 ; x < ( 1u << ( n1 + n2 ) ) ; ++x ) {
   if( std::popcount( x ) != n1 ) continue;
   std::string t;
   for( int p = 0 , i = 0 , j = 0 ; p < n1 + n2 ; ++p )
    t += ( ( x >> p ) & 1u ) ? a[ i++ ] : b[ j++ ];
   want.push_back( t );
   }
  const auto got = run( a , b );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( got.size() == binomial( n1 + n2 , n1 ) );
  for( const auto & t : got ) CHECK( interleaving( a , b , t ) );
  }
 CHECK( run( "" , "" ) == std::vector< std::string >( 1 , "" ) );
 CHECK( run( "xy" , "" ) == std::vector< std::string >( 1 , "xy" ) );
 CHECK( run( "" , "xy" ) == std::vector< std::string >( 1 , "xy" ) );
 // equal strings: the choices are all given, also when their outputs
 // coincide
 CHECK( run( "a" , "a" ) == std::vector< std::string >( 2 , "aa" ) );
 CHECK( run( "ab" , "c" ) ==
        std::vector< std::string >( { "abc" , "acb" , "cab" } ) );
 }

/*--------------------------------------------------------------------------*/
/* 31. 2023-07-11, 10.000 passi                                             */
/*--------------------------------------------------------------------------*/

// searchPath() with a small target in place of 5000: its existence against
// a dynamic programming over ( node , previous node , steps left ), each
// walk returned from src to dest of the target weight with no immediate
// reversal; src = dest with target 0, a single edge back and forth, target
// 0 away from dest, no edge, a triangle walked around, a weight above the
// target; nodes out of the graph, target < 0, W not symmetric refused
ADS_TEST( "backtrack-31" )
{
 auto check = []( const backtrack::Weights & W , int src , int dest ,
                  int target , const std::vector< int > & P ) {
  if( P.empty() || ( P.front() != src ) || ( P.back() != dest ) )
   return( false );
  int sum = 0;
  for( std::size_t i = 1 ; i < P.size() ; ++i ) {
   if( W[ P[ i - 1 ] ][ P[ i ] ] <= 0 ) return( false );
   if( ( i >= 2 ) && ( P[ i ] == P[ i - 2 ] ) ) return( false );
   sum += W[ P[ i - 1 ] ][ P[ i ] ];
   }
  return( sum == target ); };
 std::mt19937 g( 31 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = 1 + int( g() % 6 ) , target = int( g() % 13 );
  const int src = int( g() % unsigned( n ) ) ,
            dest = int( g() % unsigned( n ) );
  backtrack::Weights W( std::size_t( n ) , std::vector< int >(
                                                     std::size_t( n ) , 0 ) );
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( g() % 2 ) W[ u ][ v ] = W[ v ][ u ] = 1 + int( g() % 4 );
  // R[ s ][ u ][ p ]: ( u , p ) reachable with s steps used, p = n for nil
  using Layer = std::vector< std::vector< bool > >;
  std::vector< Layer > R( std::size_t( target ) + 1 ,
                          Layer( std::size_t( n ) ,
                                 std::vector< bool >( std::size_t( n ) + 1 ,
                                                      false ) ) );
  R[ 0 ][ src ][ n ] = true;
  for( int s = 0 ; s < target ; ++s )
   for( int u = 0 ; u < n ; ++u )
    for( int p = 0 ; p <= n ; ++p ) {
     if( ! R[ s ][ u ][ p ] || ( ( u == dest ) && ( s == target ) ) )
      continue;
     for( int v = 0 ; v < n ; ++v )
      if( ( W[ u ][ v ] > 0 ) && ( v != p ) && ( s + W[ u ][ v ] <= target ) )
       R[ s + W[ u ][ v ] ][ v ][ u ] = true;
     }
  bool exists = false;
  for( int p = 0 ; p <= n ; ++p ) exists = exists || R[ target ][ dest ][ p ];
  const auto P = backtrack::searchPath( W , src , dest , target );
  CHECK( P.has_value() == exists );
  if( P ) CHECK( check( W , src , dest , target , *P ) );
  }
 const backtrack::Weights E = { { 0 , 2 } , { 2 , 0 } };
 CHECK( backtrack::searchPath( E , 0 , 0 , 0 ) == std::vector< int >( 1 ) );
 CHECK( backtrack::searchPath( E , 0 , 1 , 2 ) ==
        std::vector< int >( { 0 , 1 } ) );
 CHECK( ! backtrack::searchPath( E , 0 , 0 , 4 ) );
 // target 0 away from dest, no edge, a triangle walked around (a cycle
 // is a walk without reversal), a weight above the target
 CHECK( ! backtrack::searchPath( E , 0 , 1 , 0 ) );
 const backtrack::Weights none( 2 , std::vector< int >( 2 , 0 ) );
 CHECK( ! backtrack::searchPath( none , 0 , 1 , 3 ) );
 const backtrack::Weights T3 = { { 0 , 1 , 1 } , { 1 , 0 , 1 } ,
                                 { 1 , 1 , 0 } };
 CHECK( backtrack::searchPath( T3 , 0 , 0 , 3 ) );
 CHECK( backtrack::searchPath( T3 , 0 , 0 , 6 ) );
 CHECK( ! backtrack::searchPath( T3 , 0 , 0 , 2 ) );
 CHECK( ! backtrack::searchPath( E , 0 , 1 , 1 ) );
 CHECK_THROWS( backtrack::searchPath( E , 0 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( backtrack::searchPath( E , -1 , 0 , 1 ) ,
               contract_violation );
 CHECK_THROWS( backtrack::searchPath( E , 0 , 1 , -1 ) ,
               contract_violation );
 const backtrack::Weights arc = { { 0 , 2 } , { 0 , 0 } };
 CHECK_THROWS( backtrack::searchPath( arc , 0 , 1 , 2 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 32. 2023-08-23, Sub-palindrome                                           */
/*--------------------------------------------------------------------------*/

// subPalindrome() on the example "casca" (12 subsequences), against the
// 2^n masks tested with std::equal, the count of the interval dynamic
// programming plus the empty one, the empty string, one character, all
// equal, all distinct
ADS_TEST( "backtrack-32" )
{
 auto run = []( const std::string & S ) {
  std::vector< std::string > out;
  backtrack::subPalindrome( S , [ & ]( const std::string & T ) {
   out.push_back( T ); } );
  return( out ); };
 CHECK( sorted( run( "casca" ) ) == sorted( std::vector< std::string >( {
         "" , "c" , "a" , "s" , "c" , "a" , "cc" , "aa" , "cac" , "csc" ,
         "asa" , "aca" } ) ) );
 std::mt19937 g( 32 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n = int( g() % 13 );
  std::string S;
  for( int i = 0 ; i < n ; ++i ) S += char( 'a' + g() % 3 );
  std::vector< std::string > want;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   std::string t;
   for( int i = 0 ; i < n ; ++i )
    if( ( m >> i ) & 1u ) t += S[ i ];
   if( std::equal( t.begin() , t.end() , t.rbegin() ) ) want.push_back( t );
   }
  std::vector< std::vector< long > > P( std::size_t( n ) + 1 ,
                               std::vector< long >( std::size_t( n ) + 1 ,
                                                    0 ) );
  auto at = [ & ]( int i , int j ) { return( i > j ? 0L : P[ i ][ j ] ); };
  for( int len = 1 ; len <= n ; ++len )
   for( int i = 0 ; i + len - 1 < n ; ++i ) {
    const int j = i + len - 1;
    P[ i ][ j ] = len == 1 ? 1 : at( i + 1 , j ) + at( i , j - 1 ) -
                                 at( i + 1 , j - 1 );
    if( ( len > 1 ) && ( S[ i ] == S[ j ] ) )
     P[ i ][ j ] += at( i + 1 , j - 1 ) + 1;
    }
  const auto got = run( S );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( long( got.size() ) == 1 + ( n ? P[ 0 ][ n - 1 ] : 0 ) );
  }
 CHECK( run( "" ) == std::vector< std::string >( 1 , "" ) );
 // one character, all equal (every one of the 2^n subsequences), all
 // distinct (only the empty one and the single characters)
 CHECK( sorted( run( "z" ) ) ==
        sorted( std::vector< std::string >( { "" , "z" } ) ) );
 CHECK( run( "aaaa" ).size() == 16 );
 CHECK( run( "abcdef" ).size() == 1 + 6 );
 }

/*--------------------------------------------------------------------------*/
/* 33. 2024-01-09, Tutti i cicli!                                           */
/*--------------------------------------------------------------------------*/

// printCycles() on the example (complete digraph on A, B, C), against the
// sequences of distinct nodes from start closed by an arc on random
// digraphs, the counts 1, 4, 15, 64, 325 of the complete digraphs, a
// digraph without cycles, a loop, two nodes, a cycle avoiding start; start
// out of the graph and a matrix not square refused
ADS_TEST( "backtrack-33" )
{
 auto run = []( const Adjacency & G , int s ) {
  std::vector< std::vector< int > > out;
  backtrack::printCycles( G , s , [ & ]( const std::vector< int > & C ) {
   out.push_back( C ); } );
  return( out ); };
 CHECK( sorted( run( complete( 3 ) , 0 ) ) ==
        sorted( std::vector< std::vector< int > >( {
         { 0 , 1 , 0 } , { 0 , 2 , 0 } , { 0 , 1 , 2 , 0 } ,
         { 0 , 2 , 1 , 0 } } ) ) );
 std::mt19937 g( 33 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 6 ) , s = int( g() % unsigned( n ) );
  const Adjacency G = randomGraph( n , 0.45 , g , true );
  std::vector< std::vector< int > > want;
  for( auto P : distinctSequences( n , s , n ) ) {
   if( P.size() < 2 ) continue;
   bool ok = G[ P.back() ][ s ];
   for( std::size_t i = 1 ; i < P.size() && ok ; ++i )
    ok = G[ P[ i - 1 ] ][ P[ i ] ];
   if( ok ) {
    P.push_back( s );
    want.push_back( P );
    }
   }
  CHECK( sorted( run( G , s ) ) == sorted( want ) );
  }
 const long count[] = { 1 , 4 , 15 , 64 , 325 };
 for( int n = 2 ; n <= 6 ; ++n )
  CHECK( long( run( complete( n ) , n - 1 ).size() ) == count[ n - 2 ] );
 Adjacency dag = edges( 3 , {} );
 dag[ 0 ][ 1 ] = dag[ 1 ][ 2 ] = dag[ 0 ][ 2 ] = true;
 CHECK( run( dag , 0 ).empty() && run( complete( 1 ) , 0 ).empty() );
 // a loop is not a cycle here (dropped with the graph), two arcs make the
 // only 2-cycle, a cycle that avoids start is not given
 const Adjacency loop( 1 , std::vector< bool >( 1 , true ) );
 CHECK( run( loop , 0 ).empty() );
 CHECK( run( complete( 2 ) , 1 ) ==
        std::vector< std::vector< int > >( 1 , { 1 , 0 , 1 } ) );
 Adjacency away = edges( 3 , { { 1 , 2 } } );
 away[ 0 ][ 1 ] = true;
 CHECK( run( away , 0 ).empty() && ( run( away , 1 ).size() == 1 ) );
 CHECK_THROWS( run( complete( 2 ) , 2 ) , contract_violation );
 CHECK_THROWS( run( complete( 2 ) , -1 ) , contract_violation );
 CHECK_THROWS( run( Adjacency( 2 , std::vector< bool >( 1 , true ) ) , 0 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 34. 2024-02-01, Meta' o meno                                             */
/*--------------------------------------------------------------------------*/

// lessThanHalf() on the example n = 7, and for n <= 150 each sequence
// halving from n, no two equal, as many as the recurrence
// T( n ) = 1 + \sum_{i <= n / 2} T( i ); n = 1, 2, 3; n <= 0 refused
ADS_TEST( "backtrack-34" )
{
 auto run = []( int n ) {
  std::vector< std::vector< int > > out;
  backtrack::lessThanHalf( n , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( run( 7 ) == std::vector< std::vector< int > >( {
         { 7 } , { 7 , 1 } , { 7 , 2 } , { 7 , 2 , 1 } , { 7 , 3 } ,
         { 7 , 3 , 1 } } ) );
 std::vector< long > T( 151 , 1 );
 for( int n = 2 ; n <= 150 ; ++n )
  for( int i = 1 ; i <= n / 2 ; ++i ) T[ n ] += T[ i ];
 for( int n = 1 ; n <= 150 ; ++n ) {
  auto got = run( n );
  CHECK( long( got.size() ) == T[ n ] );
  for( const auto & S : got ) {
   CHECK( S.front() == n );
   for( std::size_t i = 1 ; i < S.size() ; ++i )
    CHECK( ( S[ i ] > 0 ) && ( S[ i - 1 ] >= 2 * S[ i ] ) );
   }
  got = sorted( got );
  CHECK( std::adjacent_find( got.begin() , got.end() ) == got.end() );
  }
 CHECK( run( 1 ) == std::vector< std::vector< int > >( 1 , { 1 } ) );
 CHECK( run( 2 ) == std::vector< std::vector< int > >( { { 2 } ,
                                                         { 2 , 1 } } ) );
 CHECK( run( 3 ) == std::vector< std::vector< int > >( { { 3 } ,
                                                         { 3 , 1 } } ) );
 CHECK_THROWS( run( 0 ) , contract_violation );
 CHECK_THROWS( run( -4 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 35. 2024-06-14, Magic square                                             */
/*--------------------------------------------------------------------------*/

// printMagic() for n = 1 (one), n = 2 (none) and n = 3: the 8 squares of
// the example under rotations and reflections, the same as the 9!
// permutations tested by an independent check; isMagic() on the empty
// square, one cell, equal and negative values, sums beyond int, rows and
// columns equal with a diagonal not and the other way round, a matrix not
// square; n = 0 refused
ADS_TEST( "backtrack-35" )
{
 using backtrack::Grid;
 auto run = []( int n ) {
  std::vector< Grid > out;
  backtrack::printMagic( n , [ & ]( const Grid & A ) {
   out.push_back( A ); } );
  return( out ); };
 auto magic = []( const std::vector< int > & a ) {      // 3 x 3, row major
  const int L[ 8 ][ 3 ] = { { 0 , 1 , 2 } , { 3 , 4 , 5 } , { 6 , 7 , 8 } ,
                            { 0 , 3 , 6 } , { 1 , 4 , 7 } , { 2 , 5 , 8 } ,
                            { 0 , 4 , 8 } , { 2 , 4 , 6 } };
  for( const auto & l : L )
   if( a[ l[ 0 ] ] + a[ l[ 1 ] ] + a[ l[ 2 ] ] != 15 ) return( false );
  return( true ); };
 CHECK( run( 1 ) == std::vector< Grid >( 1 , Grid( 1 , { 1 } ) ) );
 CHECK( run( 2 ).empty() );
 std::vector< Grid > want;
 std::vector< int > a( 9 );
 std::iota( a.begin() , a.end() , 1 );
 do
  if( magic( a ) )
   want.push_back( { { a[ 0 ] , a[ 1 ] , a[ 2 ] } ,
                     { a[ 3 ] , a[ 4 ] , a[ 5 ] } ,
                     { a[ 6 ] , a[ 7 ] , a[ 8 ] } } );
 while( std::next_permutation( a.begin() , a.end() ) );
 std::set< Grid > symmetries;
 Grid S = { { 2 , 7 , 6 } , { 9 , 5 , 1 } , { 4 , 3 , 8 } };
 for( int r = 0 ; r < 4 ; ++r ) {
  Grid R( 3 , std::vector< int >( 3 ) ) , M( 3 , std::vector< int >( 3 ) );
  for( int i = 0 ; i < 3 ; ++i )
   for( int j = 0 ; j < 3 ; ++j ) {
    R[ j ][ 2 - i ] = S[ i ][ j ];
    M[ i ][ 2 - j ] = S[ i ][ j ];
    }
  symmetries.insert( S );
  symmetries.insert( M );
  S = R;
  }
 const auto got = run( 3 );
 CHECK( got.size() == 8 );
 CHECK( sorted( got ) == want );
 CHECK( std::set< Grid >( got.begin() , got.end() ) == symmetries );
 CHECK( backtrack::isMagic( { { 2 , 7 , 6 } , { 9 , 5 , 1 } ,
                             { 4 , 3 , 8 } } ) );
 CHECK( ! backtrack::isMagic( { { 1 , 2 } , { 3 , 4 } } ) );
 // isMagic() only compares the sums: the empty square and one cell are
 // magic, so is a square of equal or negative values; the rows and the
 // columns equal but a diagonal not, the diagonals equal but a row not
 CHECK( backtrack::isMagic( {} ) && backtrack::isMagic( { { -7 } } ) );
 CHECK( backtrack::isMagic( Grid( 4 , std::vector< int >( 4 , -3 ) ) ) );
 CHECK( ! backtrack::isMagic( { { 1 , 2 , 3 } , { 2 , 3 , 1 } ,
                                { 3 , 1 , 2 } } ) );   // diagonal 1 + 3 + 2
 CHECK( ! backtrack::isMagic( { { 1 , 0 } , { 1 , 0 } } ) );
 CHECK( ! backtrack::isMagic( { { 2 , 0 } , { 0 , 2 } } ) );
 const int big = std::numeric_limits< int >::max();   // sums beyond int
 CHECK( backtrack::isMagic( Grid( 3 , std::vector< int >( 3 , big ) ) ) );
 CHECK_THROWS( backtrack::isMagic( { { 1 , 2 } , { 3 } } ) ,
               contract_violation );
 CHECK_THROWS( run( 0 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 36. 2024-07-02, Hamilton                                                 */
/*--------------------------------------------------------------------------*/

// hamiltonianCircuit() of the library on K_{2,3} (5 nodes, none) and on
// K_{3,3} (6 nodes, one), against the ( n - 1 )! permutations fixing node 0
// on random graphs, the complete graphs and the cycles, a tree and a node
// of degree 1; the text answers yes on a single edge (n = 2), the library
// asks n >= 3; no node, the path of 3, two triangles apart or sharing a
// node; an oriented graph refused
ADS_TEST( "backtrack-36" )
{
 const Adjacency K23 = edges( 5 , { { 0 , 2 } , { 0 , 3 } , { 0 , 4 } ,
                                    { 1 , 2 } , { 1 , 3 } , { 1 , 4 } } );
 const Adjacency K33 = edges( 6 , { { 0 , 3 } , { 0 , 4 } , { 0 , 5 } ,
                                    { 1 , 3 } , { 1 , 4 } , { 1 , 5 } ,
                                    { 2 , 3 } , { 2 , 4 } , { 2 , 5 } } );
 CHECK( ! hamiltonianCircuit( K23 ) && hamiltonianCircuit( K33 ) );
 std::mt19937 g( 36 );
 for( int trial = 0 ; trial < 400 ; ++trial ) {
  const int n = 3 + int( g() % 5 );
  const Adjacency G = randomGraph( n , 0.5 , g );
  std::vector< int > P( std::size_t( n ) , 0 );
  std::iota( P.begin() , P.end() , 0 );
  bool exists = false;
  do {
   bool ok = G[ P[ n - 1 ] ][ 0 ];
   for( int i = 1 ; i < n && ok ; ++i ) ok = G[ P[ i - 1 ] ][ P[ i ] ];
   exists = exists || ok;
   }
  while( std::next_permutation( P.begin() + 1 , P.end() ) );
  CHECK( hamiltonianCircuit( G ).has_value() == exists );
  }
 for( int n = 3 ; n <= 8 ; ++n )
  CHECK( hamiltonianCircuit( complete( n ) ) &&
         hamiltonianCircuit( cycle( n ) ) );
 CHECK( ! hamiltonianCircuit( edges( 4 , { { 0 , 1 } , { 1 , 2 } ,
                                            { 1 , 3 } } ) ) );
 CHECK( ! hamiltonianCircuit( edges( 4 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } ,
                                            { 2 , 3 } } ) ) );
 CHECK( ! hamiltonianCircuit( complete( 1 ) ) &&
        ! hamiltonianCircuit( complete( 2 ) ) );
 // no node, the path of 3, two triangles apart, two triangles sharing a
 // node (a cut node), an oriented graph refused
 CHECK( ! hamiltonianCircuit( edges( 0 , {} ) ) );
 CHECK( ! hamiltonianCircuit( edges( 3 , { { 0 , 1 } , { 1 , 2 } } ) ) );
 CHECK( ! hamiltonianCircuit( edges( 6 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } ,
                                            { 3 , 4 } , { 4 , 5 } ,
                                            { 5 , 3 } } ) ) );
 CHECK( ! hamiltonianCircuit( edges( 5 , { { 0 , 1 } , { 1 , 2 } , { 2 , 0 } ,
                                            { 2 , 3 } , { 3 , 4 } ,
                                            { 4 , 2 } } ) ) );
 Adjacency arc = complete( 3 );
 arc[ 0 ][ 1 ] = false;
 CHECK_THROWS( hamiltonianCircuit( arc ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 37. 2024-08-26, Zigzaghiamo!                                             */
/*--------------------------------------------------------------------------*/

// printZigZag() on the example (20 subsequences), against the 2^n index
// sets tested by the definition, as multisets; the empty vector, all equal,
// one item, a fall first, the extremes of int
ADS_TEST( "backtrack-37" )
{
 auto run = []( const std::vector< int > & A ) {
  std::vector< std::vector< int > > out;
  backtrack::printZigZag( A , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 auto zigzag = []( const std::vector< int > & x ) {
  for( std::size_t i = 0 ; i + 1 < x.size() ; ++i )
   if( ( i % 2 == 0 ) ? ! ( x[ i ] < x[ i + 1 ] ) : ! ( x[ i ] > x[ i + 1 ] ) )
    return( false );
  return( true ); };
 const auto ex = run( { 1 , 3 , 6 , 4 , 5 } );
 const std::vector< std::vector< int > > listed = {
  {} , { 5 } , { 4 } , { 4 , 5 } , { 6 } , { 3 } , { 3 , 5 } , { 3 , 4 } ,
  { 3 , 6 } , { 3 , 6 , 5 } , { 3 , 6 , 4 } , { 3 , 6 , 4 , 5 } , { 1 } ,
  { 1 , 5 } , { 1 , 4 } , { 1 , 6 } , { 1 , 6 , 5 } , { 1 , 6 , 4 } ,
  { 1 , 6 , 4 , 5 } , { 1 , 3 } };
 CHECK( sorted( ex ) == sorted( listed ) );
 std::mt19937 g( 37 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = int( g() % 12 );
  const auto A = randomInts( n , 0 , 6 , g );
  std::vector< std::vector< int > > want;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   const auto S = pick( A , m );
   if( zigzag( S ) ) want.push_back( S );
   }
  CHECK( sorted( run( A ) ) == sorted( want ) );
  }
 CHECK( run( {} ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( { 2 , 2 , 2 } ).size() == 4 );
 // one item, a fall first (not a zigzag of two), the extremes of int whose
 // difference overflows an int, a rise and a fall at the extremes
 CHECK( sorted( run( { 7 } ) ) ==
        sorted( std::vector< std::vector< int > >( { {} , { 7 } } ) ) );
 CHECK( run( { 5 , 1 } ).size() == 3 );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( sorted( run( { lo , hi } ) ) ==
        sorted( std::vector< std::vector< int > >( { {} , { lo } , { hi } ,
                                                      { lo , hi } } ) ) );
 CHECK( run( { hi , lo } ).size() == 3 );
 const auto ext = run( { lo , hi , lo } );
 CHECK( std::find( ext.begin() , ext.end() ,
                   std::vector< int >( { lo , hi , lo } ) ) != ext.end() );
 CHECK( ext.size() == 1 + 3 + 1 + 1 );          // {}, 3 singles, lo hi
 }

/*--------------------------------------------------------------------------*/
/* 38. 2025-01-14, Radice-foglia                                            */
/*--------------------------------------------------------------------------*/

// allPaths() on the example A( B( D , E ) , C( - , F ) ), against the paths
// collected by an explicit stack with parent pointers on random trees, as
// many as the leaves; a chain, one node, the empty tree, a chain of 2000
// nodes, a full tree of depth 3 (its 8 paths in order)
ADS_TEST( "backtrack-38" )
{
 auto run = []( const Tree & T ) {
  std::vector< std::vector< int > > out;
  backtrack::allPaths( T , [ & ]( const std::vector< int > & p ) {
   out.push_back( p ); } );
  return( out ); };
 Tree T;
 const int b = T.add( 'B' , T.add( 'D' ) , T.add( 'E' ) ) ,
           c = T.add( 'C' , -1 , T.add( 'F' ) );
 T.root = T.add( 'A' , b , c );
 CHECK( run( T ) == std::vector< std::vector< int > >( {
         { 'A' , 'B' , 'D' } , { 'A' , 'B' , 'E' } , { 'A' , 'C' , 'F' } } ) );
 std::mt19937 g( 38 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const Tree R = randomTree( 1 + int( g() % 20 ) , 0 , 99 , g );
  const auto got = run( R );
  long leaves = 0;
  for( std::size_t t = 0 ; t < R.value.size() ; ++t )
   leaves += R.leaf( int( t ) );
  CHECK( long( got.size() ) == leaves );
  CHECK( sorted( got ) == sorted( rootLeafPaths( R ) ) );
  }
 Tree chain;
 chain.root = chain.add( 1 , chain.add( 2 , -1 , chain.add( 3 ) ) );
 CHECK( run( chain ) == std::vector< std::vector< int > >( 1 ,
                                                          { 1 , 2 , 3 } ) );
 CHECK( run( Tree() ).empty() );
 // one node, a chain of 2000 nodes on the right, a full tree of depth 3
 Tree one;
 one.root = one.add( -5 );
 CHECK( run( one ) == std::vector< std::vector< int > >( 1 , { -5 } ) );
 Tree deep;
 for( int i = 0 ; i < 2000 ; ++i )
  deep.root = deep.add( 2000 - i , -1 , deep.root );
 const auto d = run( deep );
 CHECK( ( d.size() == 1 ) && ( d[ 0 ].size() == 2000 ) &&
        ( d[ 0 ][ 1999 ] == 2000 ) );
 Tree full;
 auto build = [ & ]( auto & self , int h , int v ) -> int {
  return( h == 0 ? full.add( v ) : full.add( v , self( self , h - 1 , 2 * v ) ,
                                             self( self , h - 1 ,
                                                   2 * v + 1 ) ) ); };
 full.root = build( build , 3 , 1 );
 const auto fp = run( full );
 CHECK( fp.size() == 8 );
 for( int i = 0 ; i < 8 ; ++i )
  CHECK( fp[ i ] == std::vector< int >( { 1 , 2 + i / 4 , 4 + i / 2 ,
                                          8 + i } ) );
 }

/*--------------------------------------------------------------------------*/
/* 39. 2025-02-03, Numeri irripetibili                                      */
/*--------------------------------------------------------------------------*/

// printNumbers() on the example n = 2 (0 ... 99 but 11 ... 99), against
// the integers of [ 0 , 10^n ) with a mask of digits for n <= 6, the
// counts of the solution for n = 1 ... 10, n = 0 and n > 10
ADS_TEST( "backtrack-39" )
{
 auto run = []( int n ) {
  std::vector< std::int64_t > out;
  backtrack::printNumbers( n , [ & ]( std::int64_t x ) {
   out.push_back( x ); } );
  return( out ); };
 std::vector< std::int64_t > two;
 for( int x = 0 ; x < 100 ; ++x )
  if( ( x < 10 ) || ( x % 11 != 0 ) ) two.push_back( x );
 CHECK( sorted( run( 2 ) ) == two );
 std::int64_t limit = 1;
 for( int n = 0 ; n <= 6 ; ++n , limit *= 10 ) {
  std::vector< std::int64_t > want;
  for( std::int64_t x = 0 ; x < limit ; ++x ) {
   unsigned seen = 0;
   bool ok = true;
   std::int64_t y = x;
   do {
    const unsigned d = 1u << ( y % 10 );
    ok = ok && ! ( seen & d );
    seen |= d;
    y /= 10;
    }
   while( y > 0 );
   if( ok ) want.push_back( x );
   }
  CHECK( sorted( run( n ) ) == want );
  }
 const long count[] = { 10 , 91 , 739 , 5275 , 32491 , 168571 , 712891 ,
                        2345851 , 5611771 , 8877691 };
 for( int n = 1 ; n <= 11 ; ++n ) {
  long c = 0;
  backtrack::printNumbers( n , [ & ]( std::int64_t ) { ++c; } );
  CHECK( c == count[ std::min( n , 10 ) - 1 ] );
  }
 CHECK( run( 0 ) == std::vector< std::int64_t >( 1 , 0 ) );
 CHECK_THROWS( run( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 40. 2025-06-13, Tuple ben ordinate                                       */
/*--------------------------------------------------------------------------*/

// printTuples() on the example (13 tuples), against the index sets of 2 to
// k items strictly monotone, as multisets; k = 1, all equal, one item, two
// items, k = n decreasing, a tie, the extremes of int; k out of [ 1 , n ]
// refused
ADS_TEST( "backtrack-40" )
{
 auto run = []( const std::vector< int > & A , int k ) {
  std::vector< std::vector< int > > out;
  backtrack::printTuples( A , k , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( { 2 , 5 , 3 , 4 , 1 } , 3 ) ) ==
        sorted( std::vector< std::vector< int > >( {
         { 3 , 4 } , { 2 , 4 } , { 2 , 3 } , { 2 , 3 , 4 } , { 2 , 5 } ,
         { 4 , 1 } , { 3 , 1 } , { 5 , 1 } , { 5 , 4 } , { 5 , 4 , 1 } ,
         { 5 , 3 } , { 5 , 3 , 1 } , { 2 , 1 } } ) ) );
 std::mt19937 g( 40 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = 1 + int( g() % 11 ) , k = 1 + int( g() % unsigned( n ) );
  const auto A = randomInts( n , 0 , 7 , g );
  std::vector< std::vector< int > > want;
  for( unsigned m = 0 ; m < ( 1u << n ) ; ++m ) {
   const auto S = pick( A , m );
   if( ( S.size() < 2 ) || ( int( S.size() ) > k ) ) continue;
   if( ( std::adjacent_find( S.begin() , S.end() ,
                             std::greater_equal< int >() ) == S.end() ) ||
       ( std::adjacent_find( S.begin() , S.end() ,
                             std::less_equal< int >() ) == S.end() ) )
    want.push_back( S );
   }
  CHECK( sorted( run( A , k ) ) == sorted( want ) );
  }
 CHECK( run( { 3 , 1 , 2 } , 1 ).empty() );
 CHECK( run( { 4 , 4 , 4 } , 3 ).empty() );
 CHECK( run( { 8 } , 1 ).empty() );
 // two items rising and falling, k = n on a decreasing vector (all its
 // subsets of 2 items or more), a tie between two others, the extremes
 CHECK( run( { 1 , 2 } , 2 ) == std::vector< std::vector< int > >( 1 ,
                                                            { 1 , 2 } ) );
 CHECK( run( { 2 , 1 } , 2 ) == std::vector< std::vector< int > >( 1 ,
                                                            { 2 , 1 } ) );
 CHECK( run( { 4 , 3 , 2 , 1 } , 4 ).size() == 16 - 1 - 4 );
 CHECK( sorted( run( { 1 , 5 , 5 } , 3 ) ) ==
        sorted( std::vector< std::vector< int > >( { { 1 , 5 } ,
                                                      { 1 , 5 } } ) ) );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( run( { lo , hi } , 2 ) == std::vector< std::vector< int > >( 1 ,
                                                            { lo , hi } ) );
 CHECK_THROWS( run( { 1 , 2 } , 3 ) , contract_violation );
 CHECK_THROWS( run( { 1 , 2 } , 0 ) , contract_violation );
 CHECK_THROWS( run( {} , 1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 41. 2025-07-14, All paths                                                */
/*--------------------------------------------------------------------------*/

// printAllPaths() on the example (its path among the outputs), against a
// dynamic programming over the sets of visited cells counting the
// Hamiltonian paths from the start to the end, each path checked, on random
// grids with obstacles; start next to end, no free cell, no path, a
// corridor, a free cell cut off; no start, two starts or two ends, a value
// out of -1 .. 2, a matrix not square refused
ADS_TEST( "backtrack-41" )
{
 using backtrack::Cell;
 using backtrack::Grid;
 auto run = []( const Grid & A ) {
  std::vector< std::vector< Cell > > out;
  backtrack::printAllPaths( A , [ & ]( const std::vector< Cell > & P ) {
   out.push_back( P ); } );
  return( out ); };
 const Grid E = { { 1 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 } , { 0 , 0 , 0 , 0 } ,
                  { 0 , 0 , -1 , 2 } };
 const std::vector< Cell > path = { { 0 , 0 } , { 1 , 0 } , { 2 , 0 } ,
  { 3 , 0 } , { 3 , 1 } , { 2 , 1 } , { 1 , 1 } , { 0 , 1 } , { 0 , 2 } ,
  { 0 , 3 } , { 1 , 3 } , { 1 , 2 } , { 2 , 2 } , { 2 , 3 } , { 3 , 3 } };
 const auto ex = run( E );
 CHECK( std::find( ex.begin() , ex.end() , path ) != ex.end() );
 std::mt19937 g( 41 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 4 );
  if( n * n < 2 ) continue;
  Grid A( std::size_t( n ) , std::vector< int >( std::size_t( n ) , 0 ) );
  for( auto & row : A )
   for( auto & x : row ) x = ( g() % 5 ) ? 0 : -1;
  std::vector< int > cells( std::size_t( n ) * n );
  std::iota( cells.begin() , cells.end() , 0 );
  std::shuffle( cells.begin() , cells.end() , g );
  const int s = cells[ 0 ] , t = cells[ 1 ];
  A[ s / n ][ s % n ] = 1;
  A[ t / n ][ t % n ] = 2;
  std::vector< int > id( std::size_t( n ) * n , -1 );
  std::vector< int > where;
  for( int c = 0 ; c < n * n ; ++c )
   if( A[ c / n ][ c % n ] != -1 ) {
    id[ c ] = int( where.size() );
    where.push_back( c );
    }
  const int F = int( where.size() );
  std::vector< long > Dp( ( std::size_t( 1 ) << F ) * F , 0 );
  auto D = [ & ]( unsigned m , int v ) -> long & {
   return( Dp[ m * unsigned( F ) + unsigned( v ) ] ); };
  D( 1u << id[ s ] , id[ s ] ) = 1;
  for( unsigned m = 1 ; m < ( 1u << F ) ; ++m )
   for( int v = 0 ; v < F ; ++v ) {
    if( ! D( m , v ) || ( where[ v ] == t ) ) continue;
    const int r = where[ v ] / n , c = where[ v ] % n;
    const int dr[] = { 1 , -1 , 0 , 0 } , dc[] = { 0 , 0 , 1 , -1 };
    for( int d = 0 ; d < 4 ; ++d ) {
     const int a = r + dr[ d ] , b = c + dc[ d ];
     if( ( a < 0 ) || ( a >= n ) || ( b < 0 ) || ( b >= n ) ||
         ( id[ a * n + b ] < 0 ) || ( ( m >> id[ a * n + b ] ) & 1u ) )
      continue;
     D( m | ( 1u << id[ a * n + b ] ) , id[ a * n + b ] ) += D( m , v );
     }
    }
  const auto got = run( A );
  CHECK( long( got.size() ) == D( ( 1u << F ) - 1 , id[ t ] ) );
  for( const auto & P : got ) {
   CHECK( ( int( P.size() ) == F ) &&
          ( P.front() == Cell( s / n , s % n ) ) &&
          ( P.back() == Cell( t / n , t % n ) ) );
   for( std::size_t i = 1 ; i < P.size() ; ++i )
    CHECK( std::abs( P[ i ].first - P[ i - 1 ].first ) +
           std::abs( P[ i ].second - P[ i - 1 ].second ) == 1 );
   CHECK( std::set< Cell >( P.begin() , P.end() ).size() == P.size() );
   }
  }
 CHECK( run( { { 1 , 2 } , { -1 , -1 } } ).size() == 1 );
 CHECK( run( { { 1 , -1 } , { -1 , 2 } } ).empty() );
 CHECK( run( { { 1 , 0 } , { 0 , 2 } } ).empty() );
 // a corridor (one path), a free cell cut off (no path), two starts, two
 // ends, a value out of -1 .. 2, a matrix not square
 CHECK( run( { { 1 , 0 , 0 } , { -1 , -1 , 0 } , { 2 , 0 , 0 } } ).size() ==
        1 );
 CHECK( run( { { 1 , 2 , -1 } , { -1 , -1 , -1 } , { -1 , -1 , 0 } } )
        .empty() );
 CHECK_THROWS( run( { { 1 , 1 } , { 0 , 2 } } ) , contract_violation );
 CHECK_THROWS( run( { { 1 , 2 } , { 0 , 2 } } ) , contract_violation );
 CHECK_THROWS( run( { { 1 , 3 } , { 0 , 2 } } ) , contract_violation );
 CHECK_THROWS( run( { { 1 , 2 } } ) , contract_violation );
 CHECK_THROWS( run( { { 0 , 0 } , { 0 , 2 } } ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 42. 2025-08-25, Sottoinsiemi k-distanti                                  */
/*--------------------------------------------------------------------------*/

// printDistantSets() on the example (10 sets), against the subsets of the
// distinct values with all the pairs checked, the count of a dynamic
// programming on the sorted values; the empty vector, all equal, a gap of
// exactly k and of k + 1, k above every gap, the extremes of int; k <= 0
// refused
ADS_TEST( "backtrack-42" )
{
 auto run = []( const std::vector< int > & A , int k ) {
  std::vector< std::vector< int > > out;
  backtrack::printDistantSets( A , k , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( { 2 , 4 , -2 , 5 , 2 } , 2 ) ) ==
        sorted( std::vector< std::vector< int > >( {
         {} , { 2 } , { 4 } , { -2 } , { 5 } , { -2 , 2 } , { -2 , 4 } ,
         { -2 , 5 } , { 2 , 5 } , { -2 , 2 , 5 } } ) ) );
 std::mt19937 g( 42 );
 for( int trial = 0 ; trial < 500 ; ++trial ) {
  const int n = int( g() % 14 ) , k = 1 + int( g() % 3 );
  const auto A = randomInts( n , -6 , 10 , g );
  const std::set< int > V( A.begin() , A.end() );
  const std::vector< int > v( V.begin() , V.end() );
  const int m = int( v.size() );
  std::vector< std::vector< int > > want;
  for( unsigned x = 0 ; x < ( 1u << m ) ; ++x ) {
   const auto S = pick( v , x );
   bool ok = true;
   for( std::size_t i = 0 ; i < S.size() ; ++i )
    for( std::size_t j = i + 1 ; j < S.size() ; ++j )
     ok = ok && ( std::abs( S[ i ] - S[ j ] ) > k );
   if( ok ) want.push_back( S );
   }
  std::vector< long > f( std::size_t( m ) + 1 , 1 );  // f[ i ]: v[ 0 .. i )
  for( int i = 1 ; i <= m ; ++i ) {
   int j = i - 1;                            // the values below v - k
   while( ( j > 0 ) && ( v[ j - 1 ] >= v[ i - 1 ] - k ) ) --j;
   f[ i ] = f[ i - 1 ] + f[ j ];
   }
  const auto got = run( A , k );
  CHECK( sorted( got ) == sorted( want ) );
  CHECK( long( got.size() ) == f[ m ] );
  }
 CHECK( run( {} , 1 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( sorted( run( { 3 , 3 , 3 } , 1 ) ) ==
        std::vector< std::vector< int > >( { {} , { 3 } } ) );
 // two values exactly k apart (not more), k + 1 apart, a k above every
 // gap (only the single values), the extremes of int (a gap beyond int)
 CHECK( run( { 0 , 2 } , 2 ).size() == 3 && run( { 0 , 3 } , 2 ).size() == 4 );
 CHECK( run( { 5 , 1 , 9 , -3 } , 1000 ).size() == 1 + 4 );
 const int lo = std::numeric_limits< int >::min() ,
           hi = std::numeric_limits< int >::max();
 CHECK( run( { hi , lo } , hi ).size() == 4 );
 CHECK( sorted( run( { hi , lo , 0 } , hi ) ) ==
        sorted( std::vector< std::vector< int > >( { {} , { lo } , { 0 } ,
                                                      { hi } , { lo , 0 } ,
                                                      { lo , hi } } ) ) );
 CHECK_THROWS( run( { 1 } , 0 ) , contract_violation );
 CHECK_THROWS( run( { 1 } , -2 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 43. 2026-01-12, Numeri zerabili                                          */
/*--------------------------------------------------------------------------*/

// printZerable() on the example n = 4, against the 2^n strings with fewer
// 1 than 0, the counts of the solution for n = 1 ... 10 and the sums of
// binomials up to n = 20; n = 1, 2, 3 in the order of the solution; n <= 0
// refused
ADS_TEST( "backtrack-43" )
{
 auto run = []( int n ) {
  std::vector< std::string > out;
  backtrack::printZerable( n , [ & ]( const std::string & S ) {
   out.push_back( S ); } );
  return( out ); };
 CHECK( sorted( run( 4 ) ) == words( "0000 0001 0010 0100 1000" ) );
 const long count[] = { 1 , 1 , 4 , 5 , 16 , 22 , 64 , 93 , 256 , 386 };
 for( int n = 1 ; n <= 20 ; ++n ) {
  const auto got = run( n );
  std::uint64_t want = 0;
  for( int k = 0 ; 2 * k < n ; ++k ) want += binomial( n , k );
  CHECK( got.size() == want );
  if( n <= 10 ) CHECK( long( got.size() ) == count[ n - 1 ] );
  if( n <= 14 ) {
   std::vector< std::string > all;
   for( unsigned x = 0 ; x < ( 1u << n ) ; ++x )
    if( 2 * std::popcount( x ) < n ) all.push_back( bitString( x , n ) );
   CHECK( sorted( got ) == sorted( all ) );
   }
  }
 // n = 1 and 2 (no 1 at all: one 1 would tie), n = 3 (one 1 at most)
 CHECK( run( 1 ) == std::vector< std::string >( 1 , "0" ) );
 CHECK( run( 2 ) == std::vector< std::string >( 1 , "00" ) );
 CHECK( run( 3 ) == words( "000 001 010 100" ) );
 CHECK_THROWS( run( 0 ) , contract_violation );
 CHECK_THROWS( run( -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 44. 2026-02-02, New York Dreaming                                        */
/*--------------------------------------------------------------------------*/

// printPaths( G , s , k ) on the graph of the figure with k = 15000, the
// three paths of the statement among the outputs, against the sequences
// of distinct nodes closed through s, filtered by length, on random
// digraphs with a small tolerance; a loop-free 2-cycle, no arcs, the
// bounds k -/+ tol included and one beyond excluded, a loop on s, k = 0,
// another start; s out of the graph, tol < 0, W not square refused
ADS_TEST( "backtrack-44" )
{
 auto run = []( const backtrack::Weights & W , int s , int k , int tol ) {
  std::vector< std::vector< int > > out;
  backtrack::printPaths( W , s , k , tol ,
                         [ & ]( const std::vector< int > & P ) {
   out.push_back( P ); } );
  return( out ); };
 enum { S , B , C , D , E , F };            // the arcs read off the figure
 backtrack::Weights W( 6 , std::vector< int >( 6 , 0 ) );
 W[ S ][ B ] = 4000;  W[ S ][ C ] = 3500;  W[ B ][ D ] = 3000;
 W[ C ][ D ] = 3800;  W[ D ][ E ] = 3000;  W[ D ][ F ] = 3000;
 W[ E ][ S ] = 4500;  W[ F ][ S ] = 4900;  W[ B ][ E ] = 7200;
 W[ C ][ F ] = 3200;
 const auto ex = run( W , S , 15000 , 500 );
 for( const std::vector< int > & p : std::vector< std::vector< int > >( {
       { S , B , D , F , S } , { S , B , D , E , S } ,
       { S , C , D , F , S } } ) )
  CHECK( std::find( ex.begin() , ex.end() , p ) != ex.end() );
 std::mt19937 g( 44 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 6 ) , s = int( g() % unsigned( n ) );
  backtrack::Weights R( std::size_t( n ) , std::vector< int >(
                                                     std::size_t( n ) , 0 ) );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( ( u != v ) && ( g() % 2 ) ) R[ u ][ v ] = 1 + int( g() % 9 );
  const int k = int( g() % 30 ) , tol = int( g() % 4 );
  std::vector< std::vector< int > > want;
  for( auto P : distinctSequences( n , s , n ) ) {
   if( P.size() < 2 ) continue;
   P.push_back( s );
   long len = 0;
   bool ok = true;
   for( std::size_t i = 1 ; i < P.size() && ok ; ++i ) {
    ok = R[ P[ i - 1 ] ][ P[ i ] ] > 0;
    len += R[ P[ i - 1 ] ][ P[ i ] ];
    }
   if( ok && ( std::abs( len - k ) <= tol ) ) want.push_back( P );
   }
  CHECK( sorted( run( R , s , k , tol ) ) == sorted( want ) );
  }
 const backtrack::Weights two = { { 0 , 3 } , { 4 , 0 } };
 CHECK( run( two , 0 , 7 , 0 ) == std::vector< std::vector< int > >( 1 ,
                                                            { 0 , 1 , 0 } ) );
 CHECK( run( two , 0 , 9 , 1 ).empty() );
 CHECK( run( backtrack::Weights( 1 , { 0 } ) , 0 , 0 , 5 ).empty() );
 // the bounds k - tol and k + tol included, one beyond excluded; a loop
 // on s is not a closed path (s would repeat at once); k = 0 with a
 // tolerance
 CHECK( run( two , 0 , 5 , 2 ).size() == 1 && run( two , 0 , 9 , 2 ).size()
        == 1 );
 CHECK( run( two , 0 , 4 , 2 ).empty() && run( two , 0 , 10 , 2 ).empty() );
 CHECK( run( backtrack::Weights( 1 , { 6 } ) , 0 , 6 , 0 ).empty() );
 CHECK( run( two , 0 , 0 , 7 ).size() == 1 );
 CHECK( run( two , 1 , 7 , 0 ) == std::vector< std::vector< int > >( 1 ,
                                                            { 1 , 0 , 1 } ) );
 CHECK_THROWS( run( two , 2 , 7 , 0 ) , contract_violation );
 CHECK_THROWS( run( two , -1 , 7 , 0 ) , contract_violation );
 CHECK_THROWS( run( two , 0 , 7 , -1 ) , contract_violation );
 CHECK_THROWS( run( backtrack::Weights( 2 , { 0 } ) , 0 , 7 , 0 ) ,
               contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 45. 2026-06-22, Shakira                                                  */
/*--------------------------------------------------------------------------*/

// setlists() against the m-arrangements of the songs (all the m-tuples
// with distinct indices) filtered by three in a row, on random types; all
// types distinct (n! / ( n - m )!), m = 0, all of one type, no song, m = n
// with two or three of a type; m out of [ 0 , n ] refused
ADS_TEST( "backtrack-45" )
{
 auto run = []( const std::string & type , int m ) {
  std::vector< std::vector< int > > out;
  backtrack::setlists( type , m , [ & ]( const std::vector< int > & S ) {
   out.push_back( S ); } );
  return( out ); };
 std::mt19937 g( 45 );
 for( int trial = 0 ; trial < 300 ; ++trial ) {
  const int n = 1 + int( g() % 6 ) , m = int( g() % unsigned( n + 1 ) );
  std::string type;
  for( int i = 0 ; i < n ; ++i ) type += "BDLR"[ g() % 4 ];
  std::vector< std::vector< int > > want;
  long total = 1;
  for( int i = 0 ; i < m ; ++i ) total *= n;
  for( long code = 0 ; code < total ; ++code ) {
   std::vector< int > S( std::size_t( m ) , 0 );
   long x = code;
   for( int i = m - 1 ; i >= 0 ; --i , x /= n ) S[ i ] = int( x % n );
   if( std::set< int >( S.begin() , S.end() ).size() != S.size() ) continue;
   bool ok = true;
   for( int i = 2 ; i < m ; ++i )
    ok = ok && ! ( ( type[ S[ i ] ] == type[ S[ i - 1 ] ] ) &&
                   ( type[ S[ i ] ] == type[ S[ i - 2 ] ] ) );
   if( ok ) want.push_back( S );
   }
  CHECK( run( type , m ) == want );           // both lexicographic
  }
 for( int m = 0 ; m <= 4 ; ++m ) {
  long want = 1;
  for( int i = 0 ; i < m ; ++i ) want *= 4 - i;
  CHECK( long( run( "BDLR" , m ).size() ) == want );
  }
 CHECK( run( "BBB" , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( "BBBB" , 3 ).empty() && ( run( "BBBB" , 2 ).size() == 12 ) );
 // no song, m = n with two of a type (never three), three of a type in
 // five songs (the three never consecutive)
 CHECK( run( "" , 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( "BBD" , 3 ).size() == 6 );
 CHECK( run( "BBBDD" , 5 ).size() == ( 10 - 3 ) * 6 * 2 );   // no BBB
 CHECK_THROWS( run( "BD" , 3 ) , contract_violation );
 CHECK_THROWS( run( "BD" , -1 ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 46. 2026-07-20, Antipalindromo                                           */
/*--------------------------------------------------------------------------*/

// noPalindrome() and noPalindromeFast() on the example a, b, c with n = 5
// (6 strings), against the k^n strings with every substring of length >= 2
// checked by std::equal, k ( k - 1 ) ( k - 2 )^{n - 2} of them; abbab and
// acbc rejected, n = 0 and n = 1, the empty alphabet, two letters (none
// beyond length 2); repeated letters and n < 0 refused
ADS_TEST( "backtrack-46" )
{
 auto run = []( const std::string & alphabet , int n , bool fast ) {
  std::vector< std::string > out;
  auto f = [ & ]( const std::string & S ) { out.push_back( S ); };
  if( fast ) backtrack::noPalindromeFast( alphabet , n , f );
  else backtrack::noPalindrome( alphabet , n , f );
  return( out ); };
 auto palFree = []( const std::string & s ) {
  for( std::size_t i = 0 ; i < s.size() ; ++i )
   for( std::size_t len = 2 ; i + len <= s.size() ; ++len )
    if( std::equal( s.begin() + i , s.begin() + i + len ,
                    s.rbegin() + ( s.size() - i - len ) ) )
     return( false );
  return( true ); };
 const auto five = words( "abcab acbac bacba bcabc cabca cbacb" );
 CHECK( run( "abc" , 5 , false ) == five && run( "abc" , 5 , true ) == five );
 CHECK( ! palFree( "abbab" ) && ! palFree( "acbc" ) &&
        palFree( "abcab" ) );
 for( int k = 1 ; k <= 4 ; ++k )
  for( int n = 0 ; n <= 7 ; ++n ) {
   const std::string alphabet = std::string( "abcd" ).substr( 0 ,
                                                     std::size_t( k ) );
   std::vector< std::string > want;
   long total = 1;
   for( int i = 0 ; i < n ; ++i ) total *= k;
   for( long code = 0 ; code < total ; ++code ) {
    std::string s( std::size_t( n ) , ' ' );
    long x = code;
    for( int i = n - 1 ; i >= 0 ; --i , x /= k ) s[ i ] = alphabet[ x % k ];
    if( palFree( s ) ) want.push_back( s );
    }
   CHECK( run( alphabet , n , false ) == want );
   CHECK( run( alphabet , n , true ) == want );
   long count = n == 0 ? 1 : n == 1 ? k : long( k ) * ( k - 1 );
   for( int i = 2 ; i < n ; ++i ) count *= k - 2;
   CHECK( long( want.size() ) == count );
   }
 CHECK( run( "abc" , 0 , true ) == std::vector< std::string >( 1 , "" ) );
 CHECK( run( "abc" , 1 , false ).size() == 3 );
 // no letter: only the empty string; two letters stop at length 2
 CHECK( run( "" , 0 , false ) == std::vector< std::string >( 1 , "" ) );
 CHECK( run( "" , 1 , true ).empty() && run( "" , 3 , false ).empty() );
 CHECK( run( "ab" , 2 , false ) == words( "ab ba" ) );
 CHECK( run( "ab" , 3 , true ).empty() && run( "ab" , 3 , false ).empty() );
 CHECK_THROWS( run( "aab" , 2 , true ) , contract_violation );
 CHECK_THROWS( run( "aab" , 2 , false ) , contract_violation );
 CHECK_THROWS( run( "ab" , -1 , false ) , contract_violation );
 CHECK_THROWS( run( "ab" , -1 , true ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/
/* 47. 2026-08-28, Dismutazioni                                             */
/*--------------------------------------------------------------------------*/

// derangements() of the library on the example n = 3, against
// std::next_permutation without fixed points, the counts D_n of the
// recurrence D_n = ( n - 1 ) ( D_{n-1} + D_{n-2} ), n = 0, 1, 2; equal
// items (places deranged, not values), other values, a stop at the first
ADS_TEST( "backtrack-47" )
{
 auto run = []( int n ) {
  std::vector< int > A( std::size_t( n ) , 0 );
  std::iota( A.begin() , A.end() , 0 );
  std::vector< std::vector< int > > out;
  derangements( A , [ & ]( const std::vector< int > & P ) {
   out.push_back( P );
   return( false ); } );
  return( out ); };
 CHECK( run( 3 ) == std::vector< std::vector< int > >( {
         { 1 , 2 , 0 } , { 2 , 0 , 1 } } ) );
 const auto three = run( 3 );
 CHECK( std::find( three.begin() , three.end() ,
                   std::vector< int >( { 0 , 2 , 1 } ) ) == three.end() );
 long d2 = 1 , d1 = 0;                           // D_0 , D_1
 for( int n = 0 ; n <= 8 ; ++n ) {
  std::vector< int > P( std::size_t( n ) , 0 );
  std::iota( P.begin() , P.end() , 0 );
  std::vector< std::vector< int > > want;
  do {
   bool ok = true;
   for( int i = 0 ; i < n && ok ; ++i ) ok = P[ i ] != i;
   if( ok ) want.push_back( P );
   }
  while( std::next_permutation( P.begin() , P.end() ) );
  const auto got = run( n );
  CHECK( got == want );
  const long D = n == 0 ? 1 : n == 1 ? 0 : ( n - 1 ) * ( d1 + d2 );
  if( n >= 2 ) {
   d2 = d1;
   d1 = D;
   }
  CHECK( long( got.size() ) == D );
  }
 CHECK( run( 0 ) == std::vector< std::vector< int > >( 1 ) );
 CHECK( run( 1 ).empty() );
 CHECK( run( 2 ) == std::vector< std::vector< int > >( 1 , { 1 , 0 } ) );
 // other items: the places are deranged, not the values, so that two
 // equal items still give D_2 = 1 derangement; a stop at the first one
 auto places = []( const std::vector< int > & A , bool first ) {
  std::vector< std::vector< int > > out;
  const long c = derangements( A , [ & ]( const std::vector< int > & P ) {
   out.push_back( P );
   return( first ); } );
  return( std::pair( c , out ) ); };
 CHECK( places( { 7 , 7 } , false ).second ==
        std::vector< std::vector< int > >( 1 , { 7 , 7 } ) );
 CHECK( sorted( places( { 5 , 9 , 2 } , false ).second ) ==
        sorted( std::vector< std::vector< int > >( { { 9 , 2 , 5 } ,
                                                      { 2 , 5 , 9 } } ) ) );
 CHECK( places( { 0 , 1 , 2 , 3 } , true ).second.size() == 1 );
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
/*---------------------- End File exam_backtrackb.cpp ----------------------*/
/*--------------------------------------------------------------------------*/
