/*--------------------------------------------------------------------------*/
/*--------------------------- File Heuristics.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The heuristics of section 19.4 and of the exercises 19.13 and 19.16 to
 * 19.19: feasible solutions, neither optimal nor approximate, greedy or
 * by local search.
 *
 * - greedyTsp(): the edges by increasing weight, each added if its ends
 *   have fewer than two edges and are in different components of an
 *   MFSET, until a Hamiltonian chain, closed by the edge of its two ends,
 *   \f$ O( n^2 \log n ) \f$;
 * - minMaxGreedy() for the BISEZIONE: from a node in \f$ V_0 \f$ and one in
 *   \f$ V_1 \f$, a node added to \f$ V_0 \f$ and then one to \f$ V_1 \f$, the
 *   one with fewest edges to the other side and, among those, most edges
 *   to its own; the counts are kept by the structure with buckets of
 *   exercise 19.17 [see Buckets];
 * - twoOpt(): the local search on the neighbourhood \f$ I_2( \pi ) \f$, the
 *   circuits made of \f$ \pi \f$ by replacing two edges that do not touch
 *   with the two that close it again, \f$ n ( n - 1 ) / 2 - n \f$ of them;
 *   kOptNeighbours() counts \f$ I_k( \pi ) \f$ (exercise 19.15);
 * - partitionLocalSearch(): the PARTIZIONE that minimizes the difference
 *   of the sums, moving an element or swapping two (exercise 19.16);
 * - greedySuperstring(): the greedy of greedyTsp() on the graph of the
 *   overlaps, an oriented path (exercise 19.13);
 * - starAlignment(): the multiple alignment against the string with the
 *   best sum of the global scores (exercise 19.18);
 * - dsatur(): the coloring by saturation degree (exercise 19.19).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.4 and
 *      exercises 19.13-19.19,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see B. W. Kernighan, S. Lin, An efficient heuristic procedure for
 *      partitioning graphs, Bell System Technical Journal 49(2), 291-307,
 *      1970, doi:10.1002/j.1538-7305.1970.tb01770.x
 * \see S. Lin, Computer solutions of the traveling salesman problem, Bell
 *      System Technical Journal 44(10), 2245-2269, 1965,
 *      doi:10.1002/j.1538-7305.1965.tb04146.x
 * \see D. Gusfield, Efficient methods for multiple sequence alignment with
 *      guaranteed error bounds, Bulletin of Mathematical Biology 55(1),
 *      141-154, 1993, doi:10.1007/BF02460299
 * \see D. Brelaz, New methods to color the vertices of a graph,
 *      Communications of the ACM 22(4), 251-256, 1979,
 *      doi:10.1145/359094.359101
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

#ifndef __ADS_Heuristics
 #define __ADS_Heuristics
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/mfset/MFSet.h"
#include "ADS/string/Alignment.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ GREEDY TSP --------------------------------*/
/*--------------------------------------------------------------------------*/

/// greedyTsp(): the edges chosen, in the order of insertion, the last one
/// closing the chain, and the circuit from 0, for the symmetric distances
/// D of n >= 3 cities
template< class T >
std::pair< std::vector< std::pair< int , int > > , Nodes > greedyTsp(
                        const std::vector< std::vector< T > > & D )
{
 const int n = int( D.size() );
 std::vector< WeightedEdge< T > > E;
 for( int u = 0 ; u < n ; ++u )
  for( int v = u + 1 ; v < n ; ++v ) E.push_back( { u , v , D[ u ][ v ] } );
 std::stable_sort( E.begin() , E.end() , []( const auto & a ,
                   const auto & b ) { return( a.w < b.w ); } );
 std::vector< std::pair< int , int > > S;
 std::vector< int > in( n , 0 );
 std::vector< Nodes > adj( n );
 MFSet M( n );
 for( const auto & e : E ) {
  if( int( S.size() ) == n - 1 ) break;
  if( ( in[ e.u ] < 2 ) && ( in[ e.v ] < 2 ) &&
      ( M.find( e.u ) != M.find( e.v ) ) ) {
   S.emplace_back( e.u , e.v );
   ++in[ e.u ];
   ++in[ e.v ];
   adj[ e.u ].push_back( e.v );
   adj[ e.v ].push_back( e.u );
   M.merge( e.u , e.v );
   }
  }
 int u = 0;
 while( in[ u ] != 1 ) ++u;
 int v = u + 1;
 while( in[ v ] != 1 ) ++v;
 S.emplace_back( u , v );
 adj[ u ].push_back( v );
 adj[ v ].push_back( u );
 Nodes C( 1 , 0 );                       // the circuit, along adj
 for( int prev = -1 ; int( C.size() ) < n ; ) {
  const int x = C.back();
  const int y = ( adj[ x ][ 0 ] != prev ) ? adj[ x ][ 0 ] : adj[ x ][ 1 ];
  prev = x;
  C.push_back( y );
  }
 return( std::pair( S , C ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- LOCAL SEARCH --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the circuit of I2( pi ) without the edges ( pi[ i ] , pi[ i + 1 ] ) and
/// ( pi[ j ] , pi[ j + 1 ] ), 0 <= i < j < n, not consecutive: the stretch
/// pi[ i + 1 .. j ] reversed
inline Nodes twoOptMove( Nodes pi , int i , int j )
{
 std::reverse( pi.begin() + i + 1 , pi.begin() + j + 1 );
 return( pi );
 }

/// the local search on I2 from pi, for the symmetric distances D, taking
/// the best circuit of the neighbourhood while it improves; the costs of
/// the circuits visited go to *trace, if given
template< class T >
Nodes twoOpt( const std::vector< std::vector< T > > & D , Nodes pi ,
              std::vector< T > * trace = nullptr )
{
 const int n = int( pi.size() );
 T c = tourCost( D , pi );
 if( trace ) trace->push_back( c );
 for( ; ; ) {
  T best = c;
  int bi = -1 , bj = -1;
  for( int i = 0 ; i < n ; ++i )
   for( int j = i + 2 ; j < n ; ++j ) {
    if( ( i == 0 ) && ( j == n - 1 ) ) continue;      // consecutive
    const int a = pi[ i ] , b = pi[ i + 1 ] , x = pi[ j ] ,
              y = pi[ ( j + 1 ) % n ];
    const T d = c - D[ a ][ b ] - D[ x ][ y ] + D[ a ][ x ] + D[ b ][ y ];
    if( d < best ) {
     best = d;
     bi = i;
     bj = j;
     }
    }
  if( bi < 0 ) return( pi );            // a local optimum
  pi = twoOptMove( pi , bi , bj );
  c = best;
  if( trace ) trace->push_back( c );
  }
 }

/// the size of I_k( pi ) on n >= 3 nodes: the circuits that share exactly
/// n - k edges with pi, by enumeration of all the circuits, O( n! )
inline long kOptNeighbours( int n , int k )
{
 Nodes pi( n );
 std::iota( pi.begin() , pi.end() , 0 );
 std::set< std::pair< int , int > > E;
 for( int i = 0 ; i < n ; ++i ) {
  const int a = pi[ i ] , b = pi[ ( i + 1 ) % n ];
  E.insert( { std::min( a , b ) , std::max( a , b ) } );
  }
 long count = 0;
 Nodes s( pi.begin() + 1 , pi.end() );
 do {                                    // each circuit twice, by direction
  int shared = 0;
  for( int i = 0 ; i < n ; ++i ) {
   const int a = i == 0 ? 0 : s[ i - 1 ] , b = i == n - 1 ? 0 : s[ i ];
   shared += int( E.count( { std::min( a , b ) , std::max( a , b ) } ) );
   }
  if( shared == n - k ) ++count;
  } while( std::next_permutation( s.begin() , s.end() ) );
 return( count / 2 );
 }

/// the side, 0 or 1, of each element of A, by the local search from side
/// that moves an element or swaps two while the difference of the sums
/// decreases, the best move first; O( n^2 ) each step
inline std::vector< int > partitionLocalSearch( const std::vector< long > & A ,
                                                std::vector< int > side )
{
 const int n = int( A.size() );
 long diff = 0;                          // sum of side 0 - sum of side 1
 for( int i = 0 ; i < n ; ++i ) diff += side[ i ] ? -A[ i ] : A[ i ];
 for( ; ; ) {
  long best = std::labs( diff );
  int bi = -1 , bj = -1;
  for( int i = 0 ; i < n ; ++i ) {
   const long di = side[ i ] ? A[ i ] : -A[ i ];   // the change of moving i
   if( std::labs( diff + 2 * di ) < best ) {
    best = std::labs( diff + 2 * di );
    bi = i;
    bj = -1;
    }
   for( int j = i + 1 ; j < n ; ++j ) {
    if( side[ i ] == side[ j ] ) continue;
    const long dj = side[ j ] ? A[ j ] : -A[ j ];
    if( std::labs( diff + 2 * ( di + dj ) ) < best ) {
     best = std::labs( diff + 2 * ( di + dj ) );
     bi = i;
     bj = j;
     }
    }
   }
  if( bi < 0 ) return( side );
  for( int x : { bi , bj } )
   if( x >= 0 ) {
    diff += 2 * ( side[ x ] ? A[ x ] : -A[ x ] );
    side[ x ] = 1 - side[ x ];
    }
  }
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BISECTION -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the structure with buckets of exercise 19.17: for k = 0 , 1 the nodes
/// by the number h of their edges to \f$ V_k \f$, bucket( k , h ), its size
/// dim( k , h ), and the least and largest bucket not empty; a node leaves
/// a bucket in O( 1 ) by its place in it
class Buckets {

/*--------------------------------------------------------------------------*/
 public:

 explicit Buckets( int n ) : f_n( n ) {
  for( int k = 0 ; k < 2 ; ++k ) {
   f_bucket[ k ].assign( n + 1 , Nodes() );
   f_h[ k ].assign( n , 0 );
   f_place[ k ].assign( n , -1 );
   f_min[ k ] = n + 1;
   f_max[ k ] = -1;
   }
  }

 /// u with no edges to either side
 void insert( int u ) {
  for( int k = 0 ; k < 2 ; ++k ) {
   f_h[ k ][ u ] = 0;
   put( k , u );
   }
  }

 void remove( int u ) {
  for( int k = 0 ; k < 2 ; ++k ) take( k , u );
  }

 /// u has one more edge to \f$ V_k \f$
 void inc( int k , int u ) {
  take( k , u );
  ++f_h[ k ][ u ];
  put( k , u );
  }

 [[nodiscard]] bool contains( int u ) const {
  return( f_place[ 0 ][ u ] >= 0 );
  }

 [[nodiscard]] int count( int k , int u ) const { return( f_h[ k ][ u ] ); }

 [[nodiscard]] const Nodes & bucket( int k , int h ) const {
  return( f_bucket[ k ][ h ] );
  }

 [[nodiscard]] int dim( int k , int h ) const {
  return( int( f_bucket[ k ][ h ].size() ) );
  }

 [[nodiscard]] int min( int k ) const { return( f_min[ k ] ); }

 [[nodiscard]] int max( int k ) const { return( f_max[ k ] ); }

 /// the node to add to \f$ V_k \f$: in the least bucket of \f$ V_{1-k} \f$,
 /// the one with most edges to \f$ V_k \f$, the first of the bucket on ties
 [[nodiscard]] int choose( int k ) const {
  int u = -1;
  for( int v : f_bucket[ 1 - k ][ f_min[ 1 - k ] ] )
   if( ( u < 0 ) || ( f_h[ k ][ v ] > f_h[ k ][ u ] ) ) u = v;
  return( u );
  }

/*--------------------------------------------------------------------------*/
 private:

 void put( int k , int u ) {
  const int h = f_h[ k ][ u ];
  f_place[ k ][ u ] = int( f_bucket[ k ][ h ].size() );
  f_bucket[ k ][ h ].push_back( u );
  f_min[ k ] = std::min( f_min[ k ] , h );
  f_max[ k ] = std::max( f_max[ k ] , h );
  }

 void take( int k , int u ) {
  const int h = f_h[ k ][ u ];
  Nodes & B = f_bucket[ k ][ h ];
  const int p = f_place[ k ][ u ];
  B[ p ] = B.back();
  f_place[ k ][ B[ p ] ] = p;
  B.pop_back();
  f_place[ k ][ u ] = -1;
  if( B.empty() ) {                     // the bounds move, amortized
   if( f_min[ k ] == h )
    while( ( f_min[ k ] <= f_n ) && f_bucket[ k ][ f_min[ k ] ].empty() )
     ++f_min[ k ];
   if( f_max[ k ] == h )
    while( ( f_max[ k ] >= 0 ) && f_bucket[ k ][ f_max[ k ] ].empty() )
     --f_max[ k ];
   }
  }

 int f_n;
 std::vector< Nodes > f_bucket[ 2 ];
 std::vector< int > f_h[ 2 ] , f_place[ 2 ];
 int f_min[ 2 ] , f_max[ 2 ];

 };  // end( class Buckets )

/// min-max-greedy: the side of each node of G, n even, from a in
/// \f$ V_0 \f$ and b in \f$ V_1 \f$
inline std::vector< int > minMaxGreedy( const Adjacency & G , int a , int b )
{
 const int n = int( G.size() );
 Buckets B( n );
 for( int u = 0 ; u < n ; ++u ) B.insert( u );
 std::vector< int > side( n , -1 );
 auto add = [ & ]( int u , int k ) {
  side[ u ] = k;
  B.remove( u );
  for( int v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] && B.contains( v ) ) B.inc( k , v );
  };
 add( a , 0 );
 add( b , 1 );
 for( int k = 0 , left = n - 2 ; left > 0 ; k = 1 - k , --left )
  add( B.choose( k ) , k );
 return( side );
 }

/// the edges across the two sides
inline int cutSize( const Adjacency & G , const std::vector< int > & side )
{
 int c = 0;
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = u + 1 ; v < G.size() ; ++v )
   c += G[ u ][ v ] && ( side[ u ] != side[ v ] );
 return( c );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- STRINGS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the longest suffix of a that is a proper prefix of b
inline int overlap( const std::string & a , const std::string & b )
{
 for( std::size_t l = std::min( a.size() , b.size() ) ; l > 0 ; --l )
  if( a.compare( a.size() - l , l , b , 0 , l ) == 0 )
   return( int( l ) );
 return( 0 );
 }

/// a common superstring of P, by the greedy on the overlaps: the oriented
/// edges ( i , j ) by decreasing overlap, each added if out[ i ] and
/// in[ j ] are 0 and i , j are apart, and the path joined; the strings
/// contained in another are dropped first
inline std::string greedySuperstring( const std::vector< std::string > & P )
{
 std::vector< std::string > Q;
 for( std::size_t i = 0 ; i < P.size() ; ++i ) {
  bool inside = false;
  for( std::size_t j = 0 ; j < P.size() && ! inside ; ++j )
   inside = ( j != i ) && ( P[ j ].find( P[ i ] ) != std::string::npos ) &&
            ( ( P[ j ] != P[ i ] ) || ( j < i ) );
  if( ! inside ) Q.push_back( P[ i ] );
  }
 const int n = int( Q.size() );
 std::vector< WeightedEdge< int > > E;
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < n ; ++j )
   if( i != j ) E.push_back( { i , j , - overlap( Q[ i ] , Q[ j ] ) } );
 std::stable_sort( E.begin() , E.end() , []( const auto & a ,
                   const auto & b ) { return( a.w < b.w ); } );
 std::vector< int > in( n , 0 ) , out( n , 0 ) , next( n , -1 );
 MFSet M( std::max( n , 1 ) );
 for( const auto & e : E )
  if( ! out[ e.u ] && ! in[ e.v ] && ( M.find( e.u ) != M.find( e.v ) ) ) {
   out[ e.u ] = in[ e.v ] = 1;
   next[ e.u ] = e.v;
   M.merge( e.u , e.v );
   }
 std::string T;
 for( int s = 0 ; s < n ; ++s )
  if( ! in[ s ] )                       // the start of a path
   for( int u = s , prev = -1 ; u >= 0 ; prev = u , u = next[ u ] )
    T += prev < 0 ? Q[ u ] : Q[ u ].substr( overlap( Q[ prev ] , Q[ u ] ) );
 return( T );
 }

/// star alignment: the center h, the string with the largest sum of the
/// global scores against the others, and its global alignment with each
/// string, the center against itself included; O( k^2 n^2 )
inline std::pair< int , std::vector< std::pair< std::string , std::string > > >
starAlignment( const std::vector< std::string > & P )
{
 const int k = int( P.size() );
 int h = 0;
 long best = 0;
 for( int i = 0 ; i < k ; ++i ) {
  long s = 0;
  for( int j = 0 ; j < k ; ++j )
   if( j != i ) s += alignmentScore( P[ i ] , P[ j ] , Alignment::GLOBAL );
  if( ( i == 0 ) || ( s > best ) ) {
   best = s;
   h = i;
   }
  }
 std::vector< std::pair< std::string , std::string > > A;
 for( int j = 0 ; j < k ; ++j )
  A.push_back( globalAlignment( P[ h ] , P[ j ] , '-' ) );
 return( std::pair( h , A ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- COLORING -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// saturation degree: the color, from 0, of each node of G, colored one
/// at a time with the least color not on its neighbours, the next node the
/// one with most distinct colors among its neighbours, then with most
/// neighbours colored, then the first; O( n^3 )
inline std::vector< int > dsatur( const Adjacency & G )
{
 const int n = int( G.size() );
 std::vector< int > col( n , -1 );
 for( int step = 0 ; step < n ; ++step ) {
  int u = -1 , su = -1 , du = -1;
  for( int v = 0 ; v < n ; ++v ) {
   if( col[ v ] >= 0 ) continue;
   std::vector< bool > seen( n , false );
   int s = 0 , d = 0;
   for( int w = 0 ; w < n ; ++w )
    if( G[ v ][ w ] && ( col[ w ] >= 0 ) ) {
     ++d;
     if( ! seen[ col[ w ] ] ) {
      seen[ col[ w ] ] = true;
      ++s;
      }
     }
   if( ( s > su ) || ( ( s == su ) && ( d > du ) ) ) {
    u = v;
    su = s;
    du = d;
    }
   }
  std::vector< bool > used( n + 1 , false );
  for( int w = 0 ; w < n ; ++w )
   if( G[ u ][ w ] && ( col[ w ] >= 0 ) ) used[ col[ w ] ] = true;
  int c = 0;
  while( used[ c ] ) ++c;
  col[ u ] = c;
  }
 return( col );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Heuristics.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Heuristics.h --------------------------*/
/*--------------------------------------------------------------------------*/
