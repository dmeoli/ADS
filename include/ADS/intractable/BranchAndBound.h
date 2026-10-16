/*--------------------------------------------------------------------------*/
/*------------------------- File BranchAndBound.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The branch-&-bound algorithms of section 19.3 and of the exercises 19.9
 * to 19.12: the enumeration of chapter 16 that skips the subtree of a
 * sequence of choices S whose lower bound lb( S ) is not below the cost
 * minCost of the best solution found so far. The worst case stays
 * superpolynomial; the time in practice depends on the starting solution,
 * on the rule of branch, on the bound and on the order of visit.
 *
 * bbTsp() chooses the cities of the circuit one after the other from
 * S[ 0 ] = 0, in increasing order, depth first. The bound of the text is
 * \f[
 *   lb( S ) = C_i + \left\lceil \Big( A + B + \sum_{h \notin S} D_h \Big)
 *             / 2 \right\rceil ,
 * \f]
 * \f$ C_i \f$ the length of the path S[ 0 .. i ], A and B the least
 * distances to S[ 0 ] and from S[ i ] of a city out of S (the text has
 * from S[ 0 ], the same for symmetric distances), \f$ D_h \f$ the
 * least distance into and out of h over all the cities, as in the
 * example of the text (lb = 18 for S = 1 , 3 of Figura 19.5). Two values
 * of Figura 19.6 do not follow from this formula, 21 for 1 2 3 and for 1 5
 * (it gives 19 and 20), and no rule on p and q that I tried gives all the
 * values of the figure; with the formula the tree has 25 nodes instead of
 * 23 and ends with the same circuit of cost 19. The bound of the 1-tree
 * (exercise
 * 19.10) replaces the sum by the weight of a minimum spanning tree of the
 * cities out of S, for the symmetric distances. littleTsp() (exercise
 * 19.11, the algorithm of Little, Murty, Sweeney and Karel) branches on
 * including or excluding an edge, the one of reduced cost 0 whose
 * exclusion raises the bound the most, visits by least bound, and bounds
 * by reducing the rows and the columns of the matrix of the distances to
 * a least entry 0. maxWeightClique() (exercise 19.9) bounds by the weight
 * of the candidates, knapsackBB() (exercise 19.12) by the fractional
 * knapsack of chapter 14.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.3 and
 *      exercises 19.9-19.12,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see J. D. C. Little, K. G. Murty, D. W. Sweeney, C. Karel, An algorithm
 *      for the traveling salesman problem, Operations Research 11(6),
 *      972-989, 1963, doi:10.1287/opre.11.6.972
 * \see M. Held, R. M. Karp, The traveling-salesman problem and minimum
 *      spanning trees: part II, Mathematical Programming 1, 6-25, 1971,
 *      doi:10.1007/BF01584070
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

#ifndef __ADS_BranchAndBound
 #define __ADS_BranchAndBound
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"

#include <algorithm>
#include <concepts>
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
/*------------------------------ BOUNDS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the lower bound of a circuit that starts with S
enum class TspBound { BOOK , ONE_TREE };

/// the weight of a minimum spanning tree of the nodes in V, by the
/// symmetric distances D
template< class T >
T spanningTreeWeight( const std::vector< std::vector< T > > & D ,
                      const Nodes & V )
{
 std::vector< WeightedEdge< T > > E;
 for( std::size_t a = 0 ; a < V.size() ; ++a )
  for( std::size_t b = a + 1 ; b < V.size() ; ++b )
   E.push_back( { int( a ) , int( b ) , D[ V[ a ] ][ V[ b ] ] } );
 T w = T( 0 );
 for( const auto & e : kruskal( int( V.size() ) , E ) ) w += e.w;
 return( w );
 }

/// the weight of a minimum 1-tree of r: a minimum spanning tree of the
/// other nodes and the two least edges of r; each circuit is a 1-tree, so
/// that the weight bounds the optimum from below
template< class T >
T oneTree( const std::vector< std::vector< T > > & D , int r )
{
 Nodes V;
 std::vector< T > e;
 for( int h = 0 ; h < int( D.size() ) ; ++h )
  if( h != r ) {
   V.push_back( h );
   e.push_back( D[ r ][ h ] );
   }
 std::sort( e.begin() , e.end() );
 T w = spanningTreeWeight( D , V );
 for( std::size_t k = 0 ; k < std::min< std::size_t >( 2 , e.size() ) ; ++k )
  w += e[ k ];
 return( w );
 }

/// lb( S ): a lower bound of the circuits that start with the path S,
/// the cost of the circuit if S has all the cities
template< class T >
T tspLowerBound( const std::vector< std::vector< T > > & D , const Nodes & S ,
                 TspBound how = TspBound::BOOK )
{
 const int n = int( D.size() ) , i = int( S.size() ) - 1;
 T C = T( 0 );
 for( int k = 0 ; k < i ; ++k ) C += D[ S[ k ] ][ S[ k + 1 ] ];
 if( i == n - 1 )
  return( C + D[ S[ i ] ][ S[ 0 ] ] );
 std::vector< bool > inS( n , false );
 for( int s : S ) inS[ s ] = true;
 const T INF = std::numeric_limits< T >::max();
 T A = INF , B = INF;
 Nodes R;
 for( int h = 0 ; h < n ; ++h )
  if( ! inS[ h ] ) {
   R.push_back( h );
   A = std::min( A , D[ h ][ S[ 0 ] ] );   // the return to S[ 0 ]
   B = std::min( B , D[ S[ i ] ][ h ] );
   }
 if( how == TspBound::ONE_TREE )
  return( C + A + B + spanningTreeWeight( D , R ) );
 T sum = A + B;
 for( int h : R ) {                     // D[ h ]: the least p -> h -> q
  T best = INF;
  for( int p = 0 ; p < n ; ++p )
   for( int q = 0 ; q < n ; ++q )
    if( ( p != h ) && ( q != h ) && ( p != q ) )
     best = std::min( best , D[ p ][ h ] + D[ h ][ q ] );
  sum += best;
  }
 if constexpr( std::integral< T > )
  return( C + ( sum + 1 ) / 2 );
 else
  return( C + sum / 2 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BBTSP ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// bbTsp(): the cost and a minimum circuit from 0, starting from the
/// circuit start if not empty; the number of nodes of the tree of the
/// choices generated, the root included, goes to *generated, if given
template< class T >
std::pair< T , Nodes > bbTsp( const std::vector< std::vector< T > > & D ,
                              const Nodes & start = {} ,
                              TspBound how = TspBound::BOOK ,
                              long * generated = nullptr )
{
 const int n = int( D.size() );
 T minCost = start.empty() ? std::numeric_limits< T >::max()
                           : tourCost( D , start );
 Nodes minSol = start;
 if( generated ) *generated = 1;
 if( n <= 1 )
  return( std::pair( T( 0 ) , Nodes( n , 0 ) ) );
 Nodes S( 1 , 0 );
 std::vector< bool > inS( n , false );
 inS[ 0 ] = true;
 auto rec = [ & ]( auto & self , int i ) -> void {
  for( int c = 1 ; c < n ; ++c ) {      // R in increasing order
   if( inS[ c ] ) continue;
   S.push_back( c );
   inS[ c ] = true;
   const T lb = tspLowerBound( D , S , how );
   if( generated ) ++*generated;
   if( lb < minCost ) {
    if( i < n - 1 )
     self( self , i + 1 );
    else {
     minSol = S;
     minCost = lb;
     }
    }
   S.pop_back();
   inS[ c ] = false;
   }
  };
 rec( rec , 1 );
 return( std::pair( minCost , minSol ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ LITTLE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// littleTsp(): the cost and a minimum circuit from 0, by the reduction
/// of the matrix; the number of nodes of the tree generated goes to
/// *generated, if given
template< class T >
std::pair< T , Nodes > littleTsp( const std::vector< std::vector< T > > & D ,
                                  long * generated = nullptr )
{
 const int n = int( D.size() );
 const T INF = std::numeric_limits< T >::max();
 if( generated ) *generated = 1;
 if( n <= 1 )
  return( std::pair( T( 0 ) , Nodes( n , 0 ) ) );

 struct Node {
  std::vector< std::vector< T > > M;     // the reduced costs, INF if out
  std::vector< bool > row , col;         // done
  std::vector< int > succ , pred;        // the edges included
  int edges = 0;
  T lb = T( 0 );
  };
 // the reduction: false if a row or a column has no edge left
 auto reduce = [ & ]( Node & x ) {
  for( int i = 0 ; i < n ; ++i ) {
   if( x.row[ i ] ) continue;
   T m = INF;
   for( int j = 0 ; j < n ; ++j )
    if( ! x.col[ j ] ) m = std::min( m , x.M[ i ][ j ] );
   if( m == INF ) return( false );
   for( int j = 0 ; j < n ; ++j )
    if( ! x.col[ j ] && ( x.M[ i ][ j ] != INF ) ) x.M[ i ][ j ] -= m;
   x.lb += m;
   }
  for( int j = 0 ; j < n ; ++j ) {
   if( x.col[ j ] ) continue;
   T m = INF;
   for( int i = 0 ; i < n ; ++i )
    if( ! x.row[ i ] ) m = std::min( m , x.M[ i ][ j ] );
   if( m == INF ) return( false );
   for( int i = 0 ; i < n ; ++i )
    if( ! x.row[ i ] && ( x.M[ i ][ j ] != INF ) ) x.M[ i ][ j ] -= m;
   x.lb += m;
   }
  return( true );
  };

 std::vector< Node > nodes;
 FibonacciHeapPriorityQueue< int , T > Q;
 {
  Node r;
  r.M = D;
  for( int i = 0 ; i < n ; ++i ) r.M[ i ][ i ] = INF;
  r.row.assign( n , false );
  r.col.assign( n , false );
  r.succ.assign( n , -1 );
  r.pred.assign( n , -1 );
  if( reduce( r ) ) {
   nodes.push_back( std::move( r ) );
   Q.insert( 0 , nodes[ 0 ].lb );
   }
  }
 while( ! Q.isEmpty() ) {
  const int k = Q.deleteMin();
  if( nodes[ k ].edges == n ) {          // a circuit, the best
   Nodes S( 1 , 0 );
   while( int( S.size() ) < n ) S.push_back( nodes[ k ].succ[ S.back() ] );
   return( std::pair( tourCost( D , S ) , S ) );
   }
  // the entry 0 whose exclusion raises the bound the most
  const Node & x = nodes[ k ];
  int bi = -1 , bj = -1;
  T best = T( -1 );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) {
    if( x.row[ i ] || x.col[ j ] || ( x.M[ i ][ j ] != T( 0 ) ) ) continue;
    T a = INF , b = INF;
    for( int h = 0 ; h < n ; ++h ) {
     if( ( h != j ) && ! x.col[ h ] ) a = std::min( a , x.M[ i ][ h ] );
     if( ( h != i ) && ! x.row[ h ] ) b = std::min( b , x.M[ h ][ j ] );
     }
    const T p = ( ( a == INF ) || ( b == INF ) ) ? INF : a + b;
    if( ( bi < 0 ) || ( p > best ) ) {
     best = p;
     bi = i;
     bj = j;
     }
    }
  if( bi < 0 ) continue;
  Node in = x , out = x;
  // the child with ( bi , bj )
  in.row[ bi ] = in.col[ bj ] = true;
  in.succ[ bi ] = bj;
  in.pred[ bj ] = bi;
  ++in.edges;
  if( in.edges < n - 1 ) {               // no circuit before the end
   int s = bi , e = bj;
   while( in.pred[ s ] >= 0 ) s = in.pred[ s ];
   while( in.succ[ e ] >= 0 ) e = in.succ[ e ];
   in.M[ e ][ s ] = INF;
   }
  // the child without it
  out.M[ bi ][ bj ] = INF;
  for( Node * c : { & in , & out } ) {
   if( generated ) ++*generated;
   if( ( c->edges < n ) && ! reduce( *c ) ) continue;
   nodes.push_back( std::move( *c ) );
   Q.insert( int( nodes.size() ) - 1 , nodes.back().lb );
   }
  }
 return( std::pair( INF , Nodes() ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- CLIQUE AND KNAPSACK ------------------------------*/
/*--------------------------------------------------------------------------*/

/// CRICCA DI PESO MASSIMO: the weight and the nodes of a clique of G of
/// largest weight, the weights w not negative; a set is extended by the
/// candidates adjacent to all of it, and pruned when its weight and that of
/// all the candidates is not above the best
inline std::pair< long , Nodes > maxWeightClique(
                                            const Adjacency & G ,
                                            const std::vector< long > & w ,
                                            long * generated = nullptr )
{
 const int n = int( G.size() );
 long best = 0;
 Nodes bestSet , S;
 if( generated ) *generated = 1;
 auto rec = [ & ]( auto & self , const Nodes & cand , long weight ) -> void {
  if( weight > best ) {
   best = weight;
   bestSet = S;
   }
  long rest = 0;
  for( int v : cand ) rest += w[ v ];
  for( std::size_t a = 0 ; a < cand.size() ; ++a ) {
   if( weight + rest <= best ) return;   // the bound
   const int v = cand[ a ];
   rest -= w[ v ];
   Nodes next;
   for( std::size_t b = a + 1 ; b < cand.size() ; ++b )
    if( G[ v ][ cand[ b ] ] ) next.push_back( cand[ b ] );
   S.push_back( v );
   if( generated ) ++*generated;
   self( self , next , weight + w[ v ] );
   S.pop_back();
   }
  };
 Nodes all( n );
 std::iota( all.begin() , all.end() , 0 );
 rec( rec , all , 0 );
 std::sort( bestSet.begin() , bestSet.end() );
 return( std::pair( best , bestSet ) );
 }

/// ZAINO by branch-&-bound: the best profit and the objects, by places;
/// the objects by decreasing profit per volume, each in and then out,
/// pruned when the fractional knapsack of the rest does not beat the best
inline std::pair< long , Nodes > knapsackBB( const std::vector< long > & p ,
                                             const std::vector< long > & v ,
                                             long C ,
                                             long * generated = nullptr )
{
 const int n = int( p.size() );
 Nodes o( n );
 std::iota( o.begin() , o.end() , 0 );
 std::stable_sort( o.begin() , o.end() , [ & ]( int a , int b ) {
  return( p[ a ] * v[ b ] > p[ b ] * v[ a ] ); } );
 long best = 0;
 Nodes bestSet , S;
 if( generated ) *generated = 1;
 // the fractional knapsack of the objects o[ i .. ] within c
 auto bound = [ & ]( int i , long c ) {
  double b = 0;
  for( ; ( i < n ) && ( c > 0 ) ; ++i )
   if( v[ o[ i ] ] <= c ) {
    b += double( p[ o[ i ] ] );
    c -= v[ o[ i ] ];
    }
   else {
    b += double( p[ o[ i ] ] ) * double( c ) / double( v[ o[ i ] ] );
    c = 0;
    }
  return( b );
  };
 auto rec = [ & ]( auto & self , int i , long c , long profit ) -> void {
  if( profit > best ) {
   best = profit;
   bestSet = S;
   }
  if( ( i == n ) || ( double( profit ) + bound( i , c ) <= double( best ) ) )
   return;
  if( v[ o[ i ] ] <= c ) {
   S.push_back( o[ i ] );
   if( generated ) ++*generated;
   self( self , i + 1 , c - v[ o[ i ] ] , profit + p[ o[ i ] ] );
   S.pop_back();
   }
  if( generated ) ++*generated;
  self( self , i + 1 , c , profit );
  };
 rec( rec , 0 , C , 0 );
 std::sort( bestSet.begin() , bestSet.end() );
 return( std::pair( best , bestSet ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* BranchAndBound.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BranchAndBound.h ------------------------*/
/*--------------------------------------------------------------------------*/
