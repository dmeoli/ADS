/*--------------------------------------------------------------------------*/
/*-------------------------- File Approximation.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The approximation algorithms of section 19.2 and of the exercises 19.6
 * and 19.8. An absolute approximation gives a feasible x' with
 * \f$ | c( x' ) - c( x^* ) | / | c( x^* ) | \leq e \f$, e a constant of the
 * algorithm; an \f$ \epsilon \f$-approximation takes \f$ \epsilon \f$ in
 * input, in a time polynomial in the size and in general exponential in
 * \f$ 1 / \epsilon \f$.
 *
 * - firstFit(), ff and, on the volumes in decreasing order, ffd for BIN
 *   PACKING: no two boxes are filled at most half, so that N < 2 N*;
 * - deltaTsp() for the salesman with the triangle inequality: the nodes
 *   of a minimum spanning tree in preorder, the tree walked twice with the
 *   cities already visited skipped, at most twice the optimum (the bound is
 *   reached by the two concentric polygons of Figura 19.3);
 * - tspGap(): the distances of Teorema 19.2, 1 on the edges of G and s n
 *   elsewhere, so that an approximation within s would decide CIRCUITO
 *   HAMILTONIANO;
 * - apSubsetSum(), the \f$ \epsilon \f$-approximation of the optimization
 *   SOMMA DI SOTTOINSIEME with \f$ \epsilon = 1 / ( h + 1 ) \f$: each set of
 *   at most h elements, completed greedily on the elements in decreasing
 *   order, \f$ O( h n^{h+1} ) \f$;
 * - vertexCoverApprox(), the two ends of each edge not yet covered, a
 *   matching whose ends are at most twice a minimum cover (exercise 19.6);
 * - knapsackApprox(), an \f$ \epsilon \f$-approximation of ZAINO in time
 *   polynomial in n and \f$ 1 / \epsilon \f$ (exercise 19.8): the profits
 *   divided by \f$ K = \epsilon p_{max} / n \f$ and rounded down, and the
 *   least volume of each rounded profit by dynamic programming, at least
 *   \f$ ( 1 - \epsilon ) \f$ times the optimum in \f$ O( n^3 / \epsilon ) \f$.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.2 and
 *      exercises 19.6, 19.8,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see D. S. Johnson, A. Demers, J. D. Ullman, M. R. Garey, R. L. Graham,
 *      Worst-case performance bounds for simple one-dimensional packing
 *      algorithms, SIAM Journal on Computing 3(4), 299-325, 1974,
 *      doi:10.1137/0203025
 * \see D. J. Rosenkrantz, R. E. Stearns, P. M. Lewis, An analysis of
 *      several heuristics for the traveling salesman problem, SIAM Journal
 *      on Computing 6(3), 563-581, 1977, doi:10.1137/0206041
 * \see O. H. Ibarra, C. E. Kim, Fast approximation algorithms for the
 *      knapsack and sum of subset problems, Journal of the ACM 22(4),
 *      463-468, 1975, doi:10.1145/321906.321909
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

#ifndef __ADS_Approximation
 #define __ADS_Approximation
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- BIN PACKING ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// ff: the box of each object, the first one with room for it, the
/// objects in the order given or, if decreasing, in decreasing order of
/// volume (ffd); the boxes are 0 , 1 , ...
inline std::vector< int > firstFit( const std::vector< long > & A , long k ,
                                    bool decreasing = false )
{
 std::vector< int > order( A.size() );
 std::iota( order.begin() , order.end() , 0 );
 if( decreasing )
  std::stable_sort( order.begin() , order.end() , [ & ]( int a , int b ) {
   return( A[ a ] > A[ b ] ); } );
 std::vector< int > box( A.size() );
 std::vector< long > free;
 for( int i : order ) {
  std::size_t b = 0;
  while( ( b < free.size() ) && ( free[ b ] < A[ i ] ) ) ++b;
  if( b == free.size() ) free.push_back( k );
  free[ b ] -= A[ i ];
  box[ i ] = int( b );
  }
 return( box );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- DELTA TSP ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cost of the circuit S of the distances D
template< class T >
T tourCost( const std::vector< std::vector< T > > & D , const Nodes & S )
{
 T c = T( 0 );
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  c += D[ S[ i ] ][ S[ ( i + 1 ) % S.size() ] ];
 return( c );
 }

/// the circuit of the nodes of a minimum spanning tree of the complete
/// graph of the symmetric distances D in preorder from 0, the children by
/// increasing name: at most twice the optimum if D is triangular
template< class T >
Nodes deltaTsp( const std::vector< std::vector< T > > & D )
{
 const int n = int( D.size() );
 std::vector< WeightedEdge< T > > E;
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v ) E.push_back( { u , v , D[ u ][ v ] } );
 std::vector< Nodes > adj( n );
 for( const auto & e : kruskal( n , E ) ) {
  adj[ e.u ].push_back( e.v );
  adj[ e.v ].push_back( e.u );
  }
 Nodes S;
 std::vector< bool > seen( n , false );
 auto pre = [ & ]( auto & self , int u ) -> void {
  seen[ u ] = true;
  S.push_back( u );
  std::sort( adj[ u ].begin() , adj[ u ].end() );
  for( int v : adj[ u ] ) if( ! seen[ v ] ) self( self , v );
  };
 if( n > 0 ) pre( pre , 0 );
 return( S );
 }

/// the distances of Teorema 19.2: 1 on the edges of G, s n elsewhere;
/// their optimum is n if G has a Hamiltonian circuit and more than s n
/// otherwise
inline std::vector< std::vector< long > > tspGap( const Adjacency & G ,
                                                  long s )
{
 const int n = int( G.size() );
 std::vector< std::vector< long > > D( n , std::vector< long >( n , 0 ) );
 for( int u = 0 ; u < n ; ++u )
  for( int v = 0 ; v < n ; ++v )
   if( u != v ) D[ u ][ v ] = G[ u ][ v ] ? 1 : s * n;
 return( D );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- SUBSET SUM AND ZAINO --------------------------*/
/*--------------------------------------------------------------------------*/

/// apSubsetSum(): the sum, at most k, and the places of A of the best
/// set found from the sets of at most h elements, \f$ \epsilon =
/// 1 / ( h + 1 ) \f$; the sets of (1) in the order of the text, by size and
/// then lexicographically on A in decreasing order
inline std::pair< long , Nodes > apSubsetSum( const std::vector< long > & A ,
                                              long k , int h )
{
 const int n = int( A.size() );
 Nodes order( n );                       // A[ order[ 0 ] ] >= ...
 std::iota( order.begin() , order.end() , 0 );
 std::stable_sort( order.begin() , order.end() , [ & ]( int a , int b ) {
  return( A[ a ] > A[ b ] ); } );
 long max = 0;
 Nodes maxSol;
 std::vector< bool > in( n , false );
 auto complete = [ & ]( void ) {         // the loop (2)
  long z = 0;
  for( int j = 0 ; j < n ; ++j ) if( in[ j ] ) z += A[ order[ j ] ];
  if( z > k ) return;
  std::vector< bool > S = in;
  for( int j = 0 ; j < n ; ++j )
   if( ! S[ j ] && ( z + A[ order[ j ] ] <= k ) ) {
    S[ j ] = true;
    z += A[ order[ j ] ];
    }
  if( z >= max ) {
   max = z;
   maxSol.clear();
   for( int j = 0 ; j < n ; ++j ) if( S[ j ] ) maxSol.push_back( order[ j ] );
   }
  };
 auto sets = [ & ]( auto & self , int from , int left ) -> void {
  if( left == 0 ) {
   complete();
   return;
   }
  for( int j = from ; j < n ; ++j ) {
   in[ j ] = true;
   self( self , j + 1 , left - 1 );
   in[ j ] = false;
   }
  };
 for( int m = 0 ; m <= std::min( h , n ) ; ++m ) sets( sets , 0 , m );  // (1)
 return( std::pair( max , maxSol ) );
 }

/// the best profit and the objects of an \f$ \epsilon \f$-approximation of
/// ZAINO, profits p and volumes v within C, by the rounded profits
inline std::pair< long , Nodes > knapsackApprox(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C , double eps )
{
 Nodes I;                                // the objects that fit
 long pmax = 0;
 for( std::size_t i = 0 ; i < p.size() ; ++i )
  if( v[ i ] <= C ) {
   I.push_back( int( i ) );
   pmax = std::max( pmax , p[ i ] );
   }
 if( I.empty() ) return( std::pair( 0L , Nodes() ) );
 const double K = std::max( 1.0 , eps * double( pmax ) / double( I.size() ) );
 std::vector< long > q( I.size() );
 for( std::size_t i = 0 ; i < I.size() ; ++i )
  q[ i ] = long( double( p[ I[ i ] ] ) / K );
 const long Q = std::accumulate( q.begin() , q.end() , 0L );
 const long INF = std::numeric_limits< long >::max() / 2;
 // V[ i ][ s ]: the least volume of the first i objects of rounded
 // profit s
 std::vector< std::vector< long > > V( I.size() + 1 ,
                                      std::vector< long >( Q + 1 , INF ) );
 V[ 0 ][ 0 ] = 0;
 for( std::size_t i = 0 ; i < I.size() ; ++i )
  for( long s = 0 ; s <= Q ; ++s ) {
   V[ i + 1 ][ s ] = V[ i ][ s ];
   if( ( s >= q[ i ] ) && ( V[ i ][ s - q[ i ] ] < INF ) )
    V[ i + 1 ][ s ] = std::min( V[ i + 1 ][ s ] ,
                                V[ i ][ s - q[ i ] ] + v[ I[ i ] ] );
   }
 long s = Q;
 while( V[ I.size() ][ s ] > C ) --s;
 Nodes S;
 long profit = 0;
 for( std::size_t i = I.size() ; i > 0 ; --i )
  if( V[ i ][ s ] != V[ i - 1 ][ s ] ) {
   S.push_back( I[ i - 1 ] );
   profit += p[ I[ i - 1 ] ];
   s -= q[ i - 1 ];
   }
 std::reverse( S.begin() , S.end() );
 return( std::pair( profit , S ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- VERTEX COVER --------------------------------*/
/*--------------------------------------------------------------------------*/

/// both ends of each edge [ u , v ], u < v, whose ends are both out of S,
/// at most twice a minimum vertex cover; O( n^2 ) on the adjacency matrix
inline Nodes vertexCoverApprox( const Adjacency & G )
{
 const int n = int( G.size() );
 std::vector< bool > in( n , false );
 Nodes S;
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v )
   if( G[ u ][ v ] && ! in[ u ] && ! in[ v ] ) {
    in[ u ] = in[ v ] = true;
    S.push_back( u );
    S.push_back( v );
    }
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Approximation.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File Approximation.h ------------------------*/
/*--------------------------------------------------------------------------*/
