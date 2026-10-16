/*--------------------------------------------------------------------------*/
/*---------------------------- File Problems.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems in NP of chapter 18, each with its polynomial certificate,
 * which checks a proposed solution, and an enumeration by backtrack, which
 * decides the problem in exponential time and returns a solution if there
 * is one. The graphs are adjacency matrices, symmetric when unoriented;
 * the formulas in conjunctive normal form are clauses of literals, +( v +
 * 1 ) for the variable v and -( v + 1 ) for its negation; a system of 0/1
 * linear programming is A x <= b.
 *
 * - clique() (CRICCA, enCricca()), independentSet(), vertexCover(),
 *   coloring() (exercise 18.6), bisection();
 * - tsp() (COMMESSO VIAGGIATORE, enTsp(), pruned when the partial length
 *   exceeds k), hamiltonianCircuit() (unoriented), hamiltonianCycle() and
 *   hamiltonianPath() (oriented, exercises 18.12 and 18.13),
 *   hamiltonianChain() (unoriented, exercise 18.14), longestPath() and
 *   longestPathDag(), polynomial on an acyclic graph (exercise 18.15),
 *   boundedSpanningTree() (exercise 18.16);
 * - satisfiable() (SODDISFATTIBILITA'), lp01() (PROGRAMMAZIONE LINEARE
 *   0/1);
 * - partition(), subsetSumExists(), knapsackDecision() (ZAINO, exercise
 *   18.5), multiprocessorScheduling(), exactCover3() (exercise 18.7),
 *   threeDimensionalMatching(), bribes() (exercises 18.1 and 18.4).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 18,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf
 * \see R. M. Karp, Reducibility among combinatorial problems, Complexity of
 *      Computer Computations, Plenum Press, 85-103, 1972,
 *      doi:10.1007/978-1-4684-2001-2_9
 * \see M. R. Garey, D. S. Johnson, Computers and Intractability: A Guide to
 *      the Theory of NP-Completeness, W. H. Freeman, 1979
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Problems
 #define __ADS_Problems
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <optional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

using Adjacency = std::vector< std::vector< bool > >;   ///< an adjacency
using Clause = std::vector< int >;                      ///< +-( v + 1 )
using Cnf = std::vector< Clause >;                      ///< a formula
using Nodes = std::vector< int >;                       ///< a set, a path

/*--------------------------------------------------------------------------*/
/*------------------------------ GRAPHS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// certificatoCricca(): S, of distinct nodes, has k of them or more, all
/// adjacent
inline bool cliqueCertificate( const Adjacency & G , const Nodes & S , int k )
{
 if( int( S.size() ) < k ) return( false );
 for( std::size_t a = 0 ; a < S.size() ; ++a )
  for( std::size_t b = a + 1 ; b < S.size() ; ++b )
   if( ( S[ a ] == S[ b ] ) || ! G[ S[ a ] ][ S[ b ] ] ) return( false );
 return( true );
 }

namespace detail {

/// the sets of k nodes of [ 0 , n ), extended only while ok( S , v )
template< class Ok , class Done >
bool chooseK( int n , int k , Ok && ok , Done && done )
{
 Nodes S;
 auto rec = [ & ]( auto & self , int i ) -> bool {
  if( int( S.size() ) == k ) return( done( S ) );
  if( int( S.size() ) + ( n - i ) < k ) return( false );   // pruned
  for( int v = i ; v < n ; ++v )
   if( ok( S , v ) ) {
    S.push_back( v );
    if( self( self , v + 1 ) ) return( true );
    S.pop_back();
    }
  return( false );
  };
 return( rec( rec , 0 ) );
 }

}  // end( namespace detail )

/// enCricca(): k nodes all adjacent, or nothing
inline std::optional< Nodes > clique( const Adjacency & G , int k )
{
 Nodes R;
 if( k <= 0 ) return( Nodes() );
 if( detail::chooseK( int( G.size() ) , k , [ & ]( const Nodes & S , int v ) {
       for( int u : S ) if( ! G[ u ][ v ] ) return( false );
       return( true ); } ,
       [ & ]( const Nodes & S ) { R = S; return( true ); } ) )
  return( R );
 return( std::nullopt );
 }

/// k nodes no two adjacent, or nothing
inline std::optional< Nodes > independentSet( const Adjacency & G , int k )
{
 Nodes R;
 if( k <= 0 ) return( Nodes() );
 if( detail::chooseK( int( G.size() ) , k , [ & ]( const Nodes & S , int v ) {
       for( int u : S ) if( G[ u ][ v ] ) return( false );
       return( true ); } ,
       [ & ]( const Nodes & S ) { R = S; return( true ); } ) )
  return( R );
 return( std::nullopt );
 }

/// at most k nodes that touch every edge, or nothing
inline std::optional< Nodes > vertexCover( const Adjacency & G , int k )
{
 const int n = int( G.size() );
 for( int s = 0 ; s <= std::min( k , n ) ; ++s ) {
  Nodes R;
  if( detail::chooseK( n , s , []( const Nodes & , int ) { return( true ); } ,
       [ & ]( const Nodes & S ) {
        std::vector< bool > in( n , false );
        for( int u : S ) in[ u ] = true;
        for( int u = 0 ; u < n ; ++u )
         for( int v = u + 1 ; v < n ; ++v )
          if( G[ u ][ v ] && ! in[ u ] && ! in[ v ] ) return( false );
        R = S;
        return( true ); } ) )
   return( R );
  }
 return( std::nullopt );
 }

/// a coloring of the nodes with at most k colors, adjacent nodes of
/// different colors, by backtrack, or nothing (exercise 18.6)
inline std::optional< std::vector< int > > coloring( const Adjacency & G ,
                                                     int k )
{
 const int n = int( G.size() );
 std::vector< int > c( n , -1 );
 auto rec = [ & ]( auto & self , int u ) -> bool {
  if( u == n ) return( true );
  int used = 0;                                 // the colors seen so far
  for( int v = 0 ; v < u ; ++v ) used = std::max( used , c[ v ] + 1 );
  for( int x = 0 ; x < std::min( k , used + 1 ) ; ++x ) {   // no symmetry
   bool ok = true;
   for( int v = 0 ; v < u && ok ; ++v ) ok = ! ( G[ u ][ v ] && c[ v ] == x );
   if( ok ) {
    c[ u ] = x;
    if( self( self , u + 1 ) ) return( true );
    }
   }
  c[ u ] = -1;
  return( false );
  };
 if( rec( rec , 0 ) ) return( c );
 return( std::nullopt );
 }

/// a partition of the nodes in two halves cut by at most k edges, or
/// nothing; the half of node 0
inline std::optional< Nodes > bisection( const Adjacency & G , int k )
{
 const int n = int( G.size() );
 if( n % 2 ) return( std::nullopt );
 Nodes R;
 if( detail::chooseK( n , n / 2 , [ & ]( const Nodes & S , int v ) {
       return( ! S.empty() || v == 0 ); } , [ & ]( const Nodes & S ) {
        std::vector< bool > in( n , false );
        for( int u : S ) in[ u ] = true;
        int cut = 0;
        for( int u = 0 ; u < n ; ++u )
         for( int v = u + 1 ; v < n ; ++v )
          cut += G[ u ][ v ] && in[ u ] != in[ v ];
        if( cut > k ) return( false );
        R = S;
        return( true ); } ) )
  return( R );
 return( std::nullopt );
 }

/// certificatoTsp(): the tour P, a permutation, is at most k long
template< class T >
bool tspCertificate( const std::vector< std::vector< T > > & D , T k ,
                     const Nodes & P )
{
 const std::size_t n = P.size();
 if( n != D.size() ) return( false );
 std::vector< bool > seen( n , false );
 for( int c : P ) {
  if( ( c < 0 ) || ( c >= int( n ) ) || seen[ c ] ) return( false );
  seen[ c ] = true;
  }
 T t = D[ P[ n - 1 ] ][ P[ 0 ] ];
 for( std::size_t i = 1 ; i < n ; ++i ) t += D[ P[ i - 1 ] ][ P[ i ] ];
 return( t <= k );
 }

/// enTsp(): a tour of length at most k from city 0, the partial tours
/// longer than k pruned, or nothing
template< class T >
std::optional< Nodes > tsp( const std::vector< std::vector< T > > & D , T k )
{
 const int n = int( D.size() );
 if( n == 0 ) return( Nodes() );
 Nodes S = { 0 };
 std::vector< bool > used( n , false );
 used[ 0 ] = true;
 auto rec = [ & ]( auto & self , T len ) -> bool {
  if( len > k ) return( false );                 // pruned
  if( int( S.size() ) == n ) return( len + D[ S.back() ][ 0 ] <= k );
  for( int c = 1 ; c < n ; ++c )
   if( ! used[ c ] ) {
    used[ c ] = true;
    S.push_back( c );
    if( self( self , len + D[ S[ S.size() - 2 ] ][ c ] ) ) return( true );
    S.pop_back();
    used[ c ] = false;
    }
  return( false );
  };
 if( rec( rec , T( 0 ) ) ) return( S );
 return( std::nullopt );
 }

namespace detail {

/// a path through all the nodes, from the first one in starts, with
/// adjacent consecutive nodes, closed if cycle, ending in end if >= 0
inline std::optional< Nodes > hamilton( const Adjacency & G ,
                                        const Nodes & starts , bool cycle ,
                                        int end = -1 )
{
 const int n = int( G.size() );
 if( n == 0 ) return( Nodes() );
 Nodes P;
 std::vector< bool > used( n , false );
 auto rec = [ & ]( auto & self ) -> bool {
  if( int( P.size() ) == n )
   return( ( ! cycle || G[ P.back() ][ P[ 0 ] ] ) &&
           ( end < 0 || P.back() == end ) );
  for( int v = 0 ; v < n ; ++v )
   if( ! used[ v ] && G[ P.back() ][ v ] ) {
    used[ v ] = true;
    P.push_back( v );
    if( self( self ) ) return( true );
    P.pop_back();
    used[ v ] = false;
    }
  return( false );
  };
 for( int s : starts ) {
  P = { s };
  used.assign( n , false );
  used[ s ] = true;
  if( n == 1 )
   return( ( ! cycle && ( end < 0 || end == s ) ) ?
           std::optional< Nodes >( P ) : std::nullopt );
  if( rec( rec ) ) return( P );
  }
 return( std::nullopt );
 }

inline Nodes all( int n )
{
 Nodes S( n );
 std::iota( S.begin() , S.end() , 0 );
 return( S );
 }

}  // end( namespace detail )

/// an unoriented circuit through every node once (n >= 3), or nothing
inline std::optional< Nodes > hamiltonianCircuit( const Adjacency & G )
{
 if( G.size() < 3 ) return( std::nullopt );
 return( detail::hamilton( G , { 0 } , true ) );
 }

/// an oriented cycle through every node once (n >= 2), or nothing
inline std::optional< Nodes > hamiltonianCycle( const Adjacency & G )
{
 if( G.size() < 2 ) return( std::nullopt );
 return( detail::hamilton( G , { 0 } , true ) );
 }

/// an oriented path through every node once, from r and to t if given
inline std::optional< Nodes > hamiltonianPath( const Adjacency & G ,
                                               int r = -1 , int t = -1 )
{
 return( detail::hamilton( G , r < 0 ? detail::all( int( G.size() ) ) :
                                       Nodes( { r } ) , false , t ) );
 }

/// an unoriented chain through every node once
inline std::optional< Nodes > hamiltonianChain( const Adjacency & G )
{
 return( detail::hamilton( G , detail::all( int( G.size() ) ) , false ) );
 }

/// a simple path from r to t of at least k edges, by backtrack
inline std::optional< Nodes > longestPath( const Adjacency & G , int r ,
                                           int t , int k )
{
 const int n = int( G.size() );
 Nodes P = { r };
 std::vector< bool > used( n , false );
 used[ r ] = true;
 auto rec = [ & ]( auto & self ) -> bool {
  if( ( P.back() == t ) && ( int( P.size() ) - 1 >= k ) ) return( true );
  if( P.back() == t ) return( false );
  for( int v = 0 ; v < n ; ++v )
   if( ! used[ v ] && G[ P.back() ][ v ] ) {
    used[ v ] = true;
    P.push_back( v );
    if( self( self ) ) return( true );
    P.pop_back();
    used[ v ] = false;
    }
  return( false );
  };
 if( rec( rec ) ) return( P );
 return( std::nullopt );
 }

/// the most edges of a path from r to t in an acyclic oriented graph, -1
/// if t is not reachable; O( n^2 ) by the topological order
inline int longestPathDag( const Adjacency & G , int r , int t )
{
 const int n = int( G.size() );
 Nodes order;
 std::vector< int > state( n , 0 );
 auto dfs = [ & ]( auto & self , int u ) -> void {
  state[ u ] = 1;
  for( int v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] && ! state[ v ] ) self( self , v );
  order.push_back( u );
  };
 for( int u = 0 ; u < n ; ++u ) if( ! state[ u ] ) dfs( dfs , u );
 std::reverse( order.begin() , order.end() );
 std::vector< int > L( n , -1 );
 L[ r ] = 0;
 for( int u : order )
  if( L[ u ] >= 0 )
   for( int v = 0 ; v < n ; ++v )
    if( G[ u ][ v ] ) L[ v ] = std::max( L[ v ] , L[ u ] + 1 );
 return( L[ t ] );
 }

/// a spanning tree of the unoriented graph with at most k edges at each
/// node, as n - 1 edges, by enumeration, or nothing
inline std::optional< std::vector< std::pair< int , int > > >
boundedSpanningTree( const Adjacency & G , int k )
{
 const int n = int( G.size() );
 std::vector< std::pair< int , int > > E , T;
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v ) if( G[ u ][ v ] ) E.emplace_back( u , v );
 std::vector< int > deg( n , 0 ) , comp( n );
 auto find = [ & ]( int x ) {
  while( comp[ x ] != x ) x = comp[ x ];
  return( x ); };
 auto rec = [ & ]( auto & self , std::size_t i ) -> bool {
  if( int( T.size() ) == n - 1 ) return( true );
  if( ( i == E.size() ) || ( int( T.size() + E.size() - i ) < n - 1 ) )
   return( false );
  auto [ u , v ] = E[ i ];
  if( deg[ u ] < k && deg[ v ] < k ) {
   std::iota( comp.begin() , comp.end() , 0 );
   for( auto [ a , b ] : T ) comp[ find( a ) ] = find( b );
   if( find( u ) != find( v ) ) {
    T.push_back( E[ i ] ); ++deg[ u ]; ++deg[ v ];
    if( self( self , i + 1 ) ) return( true );
    T.pop_back(); --deg[ u ]; --deg[ v ];
    }
   }
  return( self( self , i + 1 ) );
  };
 if( n == 0 || rec( rec , 0 ) ) return( T );
 return( std::nullopt );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- LOGIC AND LINEAR PROGRAMMING ----------------------*/
/*--------------------------------------------------------------------------*/

/// the certificate: x satisfies every clause
inline bool satCertificate( const Cnf & F , const std::vector< bool > & x )
{
 for( const Clause & c : F ) {
  bool sat = false;
  for( int l : c ) sat = sat || ( l > 0 ? x[ l - 1 ] : ! x[ -l - 1 ] );
  if( ! sat ) return( false );
  }
 return( true );
 }

/// an assignment of the n variables that satisfies F, by backtrack with
/// a clause checked as soon as its variables are set, or nothing
inline std::optional< std::vector< bool > > satisfiable( const Cnf & F ,
                                                         int n )
{
 std::vector< int > last( F.size() , 0 );         // its largest variable
 for( std::size_t c = 0 ; c < F.size() ; ++c )
  for( int l : F[ c ] ) last[ c ] = std::max( last[ c ] , std::abs( l ) );
 std::vector< bool > x( n , false );
 auto rec = [ & ]( auto & self , int v ) -> bool {
  for( std::size_t c = 0 ; c < F.size() ; ++c )
   if( last[ c ] == v ) {                         // all its variables set
    bool sat = false;
    for( int l : F[ c ] ) sat = sat || ( l > 0 ? x[ l - 1 ] : ! x[ -l - 1 ] );
    if( ! sat ) return( false );
    }
  if( v == n ) return( true );
  for( bool b : { false , true } ) {
   x[ v ] = b;
   if( self( self , v + 1 ) ) return( true );
   }
  return( false );
  };
 if( rec( rec , 0 ) ) return( x );
 return( std::nullopt );
 }

/// a 0/1 vector x with A x <= b, by enumeration, or nothing
inline std::optional< std::vector< int > > lp01(
                              const std::vector< std::vector< long > > & A ,
                              const std::vector< long > & b , int n )
{
 std::vector< int > x( n , 0 );
 auto rec = [ & ]( auto & self , int j ) -> bool {
  if( j == n ) {
   for( std::size_t i = 0 ; i < A.size() ; ++i ) {
    long s = 0;
    for( int k = 0 ; k < n ; ++k ) s += A[ i ][ k ] * x[ k ];
    if( s > b[ i ] ) return( false );
    }
   return( true );
   }
  for( int v : { 0 , 1 } ) {
   x[ j ] = v;
   if( self( self , j + 1 ) ) return( true );
   }
  return( false );
  };
 if( rec( rec , 0 ) ) return( x );
 return( std::nullopt );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- NUMBERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a set of the places of A of sum k, by backtrack, or nothing
inline std::optional< Nodes > subsetSumExists( const std::vector< long > & A ,
                                               long k )
{
 Nodes S;
 auto rec = [ & ]( auto & self , std::size_t i , long s ) -> bool {
  if( s == k ) return( true );
  if( i == A.size() || s > k ) return( false );
  S.push_back( int( i ) );
  if( self( self , i + 1 , s + A[ i ] ) ) return( true );
  S.pop_back();
  return( self( self , i + 1 , s ) );
  };
 if( rec( rec , 0 , 0 ) ) return( S );
 return( std::nullopt );
 }

/// the places of a half of A of half the sum, or nothing
inline std::optional< Nodes > partition( const std::vector< long > & A )
{
 const long s = std::accumulate( A.begin() , A.end() , 0L );
 if( s % 2 ) return( std::nullopt );
 return( subsetSumExists( A , s / 2 ) );
 }

/// enZaino(): objects of volume at most C and profit at least k
inline std::optional< Nodes > knapsackDecision( const std::vector< long > & p ,
                                                const std::vector< long > & v ,
                                                long C , long k )
{
 Nodes S;
 auto rec = [ & ]( auto & self , std::size_t i , long tp , long tv ) -> bool {
  if( tv > C ) return( false );
  if( tp >= k ) return( true );
  if( i == p.size() ) return( false );
  S.push_back( int( i ) );
  if( self( self , i + 1 , tp + p[ i ] , tv + v[ i ] ) ) return( true );
  S.pop_back();
  return( self( self , i + 1 , tp , tv ) );
  };
 if( rec( rec , 0 , 0 , 0 ) ) return( S );
 return( std::nullopt );
 }

/// the processor of each program, so that m processors run them all within
/// time d, or nothing
inline std::optional< std::vector< int > > multiprocessorScheduling(
                        const std::vector< long > & t , int m , long d )
{
 std::vector< int > a( t.size() , -1 );
 std::vector< long > load( m , 0 );
 auto rec = [ & ]( auto & self , std::size_t i ) -> bool {
  if( i == t.size() ) return( true );
  for( int q = 0 ; q < m ; ++q ) {
   if( load[ q ] + t[ i ] > d ) continue;
   load[ q ] += t[ i ];
   a[ i ] = q;
   if( self( self , i + 1 ) ) return( true );
   load[ q ] -= t[ i ];
   if( load[ q ] == 0 ) break;            // the empty processors are equal
   }
  return( false );
  };
 if( rec( rec , 0 ) ) return( a );
 return( std::nullopt );
 }

/// enCopertura(): sets of three of { 0 , ... , m - 1 } that partition it,
/// by places, the ones that meet those chosen pruned, or nothing
inline std::optional< Nodes > exactCover3(
                      const std::vector< std::array< int , 3 > > & Y , int m )
{
 std::vector< bool > covered( m , false );
 Nodes S;
 auto rec = [ & ]( auto & self , std::size_t j , int left ) -> bool {
  if( left == 0 ) return( true );
  if( j == Y.size() ) return( false );
  const auto & y = Y[ j ];
  if( ! covered[ y[ 0 ] ] && ! covered[ y[ 1 ] ] && ! covered[ y[ 2 ] ] &&
      y[ 0 ] != y[ 1 ] && y[ 0 ] != y[ 2 ] && y[ 1 ] != y[ 2 ] ) {
   for( int e : y ) covered[ e ] = true;
   S.push_back( int( j ) );
   if( self( self , j + 1 , left - 3 ) ) return( true );
   S.pop_back();
   for( int e : y ) covered[ e ] = false;
   }
  return( self( self , j + 1 , left ) );
  };
 if( rec( rec , 0 , m ) ) return( S );
 return( std::nullopt );
 }

/// a matching of q triples ( w , x , v ) of M with distinct w, x and v, the
/// three sets being [ 0 , q ), or nothing
inline std::optional< Nodes > threeDimensionalMatching(
                     const std::vector< std::array< int , 3 > > & M , int q )
{
 std::vector< std::array< int , 3 > > Y;
 for( const auto & t : M )
  Y.push_back( { t[ 0 ] , q + t[ 1 ] , 2 * q + t[ 2 ] } );
 return( exactCover3( Y , 3 * q ) );
 }

/// mazzette(): the n bribes M to the n politicians, ps[ i ] if the i-th
/// is of the PS, so that the PS gets twice the PDF; the bribe of each
/// politician, or nothing
inline std::optional< std::vector< long > > bribes(
                      std::vector< long > M , const std::vector< bool > & ps )
{
 std::sort( M.begin() , M.end() );
 do {
  long a = 0 , b = 0;
  for( std::size_t i = 0 ; i < M.size() ; ++i ) ( ps[ i ] ? a : b ) += M[ i ];
  if( a == 2 * b ) return( M );
  } while( std::next_permutation( M.begin() , M.end() ) );
 return( std::nullopt );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Problems.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Problems.h ---------------------------*/
/*--------------------------------------------------------------------------*/
