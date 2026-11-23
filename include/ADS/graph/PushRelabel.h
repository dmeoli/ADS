/*--------------------------------------------------------------------------*/
/*--------------------------- File PushRelabel.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The push-relabel of Goldberg and Tarjan, the way `PUSH_RELABEL` of
 * `FlowNetwork::maxFlow()` [see `MaxFlow.h`], beyond the book: it keeps a
 * preflow (more flow may enter a node than leave it) and labels that bound
 * the distances from \f$ t \f$, with the node of the highest label
 * discharged first, \f$ O( n^2 \sqrt{m} ) \f$ (Cheriyan and Maheshwari),
 * and the global relabelings and the gaps of Cherkassky and Goldberg, on
 * which its speed depends in practice (see \ref theory_graph_flow). From
 * the flow there is it first looks for a residual path from \f$ s \f$ to
 * \f$ t \f$ in the side of \f$ s \f$, and stops if there is none, and it
 * takes the excesses that `setCapacity()` left where they are, so that a
 * change of the capacities costs less than a computation from scratch; the
 * visits of `DINIC`, which explore only the nodes reached from \f$ s \f$,
 * remain the cheaper repair when the network is large and the change small.
 *
 * The network, the other ways and the circulations are those of
 * `MaxFlow.h`, which includes this header, so that either of the two gives
 * the whole of `FlowNetwork`; this one defines its member `pushRelabel()`.
 *
 * \see A. V. Goldberg, R. E. Tarjan, A new approach to the maximum-flow
 *      problem, Journal of the ACM 35(4), 921-940, 1988,
 *      doi:10.1145/48014.61051
 * \see J. Cheriyan, S. N. Maheshwari, Analysis of preflow push algorithms
 *      for maximum network flow, SIAM Journal on Computing 18(6),
 *      1057-1086, 1989, doi:10.1137/0218072
 * \see B. V. Cherkassky, A. V. Goldberg, On implementing the push-relabel
 *      method for the maximum flow problem, Algorithmica 19(4), 390-410,
 *      1997, doi:10.1007/PL00009180
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_PushRelabel
 #define __ADS_PushRelabel
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/MaxFlow.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- THE PUSH-RELABEL ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the maximum flow from \f$ s \f$ to \f$ t \f$ by the push-relabel of
/// Goldberg and Tarjan, from the flow there is and the excesses left by
/// `setCapacity()`
///
/// In pseudocode (beyond the book; the highest label first, with the
/// global relabelings and the gaps of Cherkassky and Goldberg, the
/// labelings that stop at the last node with an excess of their HIPR,
/// and the two phases and the scan of the edges of a node that pushes
/// and finds its new label at once of the Preflow of LEMON), with
/// \f$ e( x ) \f$
/// the excess of \f$ x \f$ and \f$ d( x ) \f$ its label, a lower bound of
/// its distance from the target in the residual network, \f$ D \f$ the
/// label of the nodes out of the game:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{pushRelabel}( s , t ) \cr
/// 2 & \quad e \gets \text{the excesses left by } \mathrm{setCapacity}()
///     \cr
/// 3 & \quad \textbf{if}\ e = 0 \textbf{ and } | f | > 0 \textbf{ and }
///     t \text{ is not reached from } s \text{ in the residual network}
///     \ \textbf{then}\ \textbf{return} \cr
/// 4 & \quad \textbf{foreach}\ ( s , v )\ \textbf{with}\ c_f( s , v )
///     > 0\ \textbf{do}\ \text{push } c_f( s , v ) \text{ on it} \cr
/// 5 & \quad d \gets \mathrm{labels}( t ) \cr
/// 6 & \quad \mathrm{discharge}( t , n ) \quad \triangleright\ \text{a}
///     \text{ maximum preflow} \cr
/// 7 & \quad \textbf{if}\ \text{some } u \text{ of label } n \text{ has }
///     e( u ) > 0\ \textbf{then} \cr
/// 8 & \qquad d \gets \mathrm{labels}( s ) \text{ among the nodes of}
///     \text{ label } n;\ \mathrm{discharge}( s , 2 n ) \cr
/// 9 & \textbf{procedure}\ \mathrm{discharge}( z , D ) \cr
/// 10 & \quad \textbf{while}\ \text{some } u \notin \{ s , t \}
///     \text{ has } e( u ) > 0 \text{ and } d( u ) < D\ \textbf{do} \cr
/// 11 & \qquad u \gets \text{such a node of the highest label};
///     \ \lambda \gets D \cr
/// 12 & \qquad \textbf{foreach}\ ( u , v )\ \textbf{with}\ c_f( u , v )
///     > 0\ \textbf{while}\ e( u ) > 0\ \textbf{do} \cr
/// 13 & \qquad\quad \textbf{if}\ d( v ) < d( u )\ \textbf{then}
///     \ \text{push } \min( e( u ) , c_f( u , v ) ) \text{ on } ( u , v )
///     \ \textbf{else}\ \lambda \gets \min( \lambda , d( v ) ) \cr
/// 14 & \qquad \textbf{if}\ e( u ) > 0\ \textbf{then} \cr
/// 15 & \qquad\quad \textbf{if}\ u \text{ is the only node of label }
///     d( u )\ \textbf{then}\ d( x ) \gets D \text{ for } u \text{ and}
///     \text{ each } x \text{ with } d( x ) > d( u ) \cr
/// 16 & \qquad\quad \textbf{else}\ d( u ) \gets \min( D , \lambda + 1 )
///     \cr
/// 17 & \qquad \textbf{if}\ z = t \textbf{ and } \text{the relabelings}
///     \text{ since the last labels cost} > ( 6 n + m ) / 2 \cr
///     & \qquad\quad \textbf{then}\ d \gets \mathrm{labels}( z ) \cr
/// 18 & \textbf{function}\ \mathrm{labels}( z ) \cr
/// 19 & \quad \text{visit in breadth backwards from } z \text{, level by}
///     \text{ level, } d( u ) \text{ the level of } u \cr
/// 20 & \quad \text{stop after the level } k \text{ that labels the last}
///     \text{ node with an excess} \cr
/// 21 & \quad \textbf{return}\ d \text{, and } k + 2 \text{ for the}
///     \text{ nodes not reached} \quad \triangleright\ \text{a lower}
///     \text{ bound}
/// \end{array}}
/// \f]
inline void FlowNetwork::pushRelabel( int s , int t )
{
 // on the edges in place, each with its twin e ^ 1, no copy of them
 const int N = n();
 long long * const R = f_res.data();
 std::vector< long long > ex( N , 0 );
 // the excesses left by setCapacity(), where they are; with none and a
 // flow already, the flow is maximum unless a residual path leads from s
 // to t, which a visit of the side of s finds without touching the rest
 bool any = false;
 if( f_unbalanced ) {
  for( const int x : f_imbNodes ) {
   if( ( x != s ) && ( x != t ) && ( f_imb[ x ] > 0 ) ) {
    ex[ x ] = f_imb[ x ];
    any = true;
    }
   f_imb[ x ] = 0;
   }
  f_imbNodes.clear();
  f_unbalanced = false;
  }
 if( ! any && ( inflow( t ) != 0 ) ) {  // a flow already: warm
  std::vector< char > seen( N , 0 );
  std::vector< int > S = { s };
  seen[ s ] = 1;
  bool reach = false;
  while( ! S.empty() && ! reach ) {
   const int u = S.back();
   S.pop_back();
   for( const auto & [ e , v ] : f_adj[ u ] )
    if( ! seen[ v ] && ( R[ e ] > 0 ) ) {
     seen[ v ] = 1;
     reach = reach || ( v == t );
     S.push_back( v );
     }
   }
  if( ! reach )
   return;
  }
 std::vector< int > d( N , N );
 // the active nodes by label, as stacks, and all the nodes by label, as
 // doubly linked lists for the gaps
 std::vector< int > aFirst( 2 * N + 1 , -1 ) , aNext( N );
 std::vector< char > queued( N , 0 );
 std::vector< int > bFirst( 2 * N + 1 , -1 ) , bNext( N ) , bPrev( N );
 int aMax = -1 , bMax = -1;
 auto aPush = [ & ]( int v ) {
  if( queued[ v ] )
   return;
  queued[ v ] = 1;
  aNext[ v ] = aFirst[ d[ v ] ];
  aFirst[ d[ v ] ] = v;
  if( d[ v ] > aMax )
   aMax = d[ v ];
  };
 auto bAdd = [ & ]( int v ) {
  bPrev[ v ] = -1;
  bNext[ v ] = bFirst[ d[ v ] ];
  if( bNext[ v ] >= 0 )
   bPrev[ bNext[ v ] ] = v;
  bFirst[ d[ v ] ] = v;
  if( d[ v ] > bMax )
   bMax = d[ v ];
  };
 auto bDel = [ & ]( int v ) {
  if( bPrev[ v ] >= 0 )
   bNext[ bPrev[ v ] ] = bNext[ v ];
  else
   bFirst[ d[ v ] ] = bNext[ v ];
  if( bNext[ v ] >= 0 )
   bPrev[ bNext[ v ] ] = bPrev[ v ];
  };

 // the labels: the distances to the target in the residual network, by a
 // visit in breadth backwards over the nodes still in the game (those
 // below the top); the others stay at the top
 int target = t , top = N;
 std::vector< int > Q;
 std::vector< char > inGame( N , 1 );
 auto relabelAll = [ & ]( bool first ) {
  int * const D = d.data();
  const char * const G = inGame.data();
  int want = 0;  // the active ones to label
  for( int v = 0 ; v < N ; ++v )
   if( G[ v ] ) {
    D[ v ] = top;
    queued[ v ] = 0;
    want += ( ex[ v ] > 0 ) && ( v != s ) && ( v != t );
    }
  for( int k = 0 ; k <= aMax ; ++k )
   aFirst[ k ] = -1;
  for( int k = 0 ; k <= bMax ; ++k )
   bFirst[ k ] = -1;
  aMax = bMax = -1;
  // s and t out of the visit, the target at 0 and the other at the top
  D[ s ] = D[ t ] = -1;
  D[ target ] = 0;
  Q.resize( std::size_t( N ) );
  int * const q = Q.data();
  std::size_t head = 0 , tail = 0;
  q[ tail++ ] = target;
  const int tp = top;
  const bool all = target == t;  // the first phase: no node out
  std::size_t level = tail;      // the end of the level of now
  int last = 0;                  // its label
  while( head < tail ) {
   if( head == level ) {  // a level done: all the
    if( want == 0 )       // active ones labeled, the
     break;               // others above (the lazy
    level = tail;         // update of HIPR)
    ++last;
    }
   const int v = q[ head++ ];
   const int dv = D[ v ] + 1;
   const Incidence * a = f_adj[ v ].data();
   const Incidence * const z = a + f_adj[ v ].size();
   // no branch on the data in the first labeling: the node taken by a
   // conditional write, as its tests are hard to predict
   if( all && first )
    for( ; a != z ; ++a ) {
     const int u = a->to;
     const bool ok = ( D[ u ] == tp ) & ( R[ a->e ^ 1 ] > 0 );
     D[ u ] = ok ? dv : D[ u ];
     q[ tail ] = u;
     tail += ok;
     want -= ok & ( ex[ u ] > 0 );
     }
   else
    for( ; a != z ; ++a ) {
     const int u = a->to;
     if( D[ u ] != tp )  // labeled, or s or t
      continue;
     const int e = a->e;
     if( G[ u ] && ( R[ e ^ 1 ] > 0 ) ) {
      D[ u ] = dv;
      q[ tail++ ] = u;
      want -= ex[ u ] > 0;
      }
     }
   }
  D[ target == t ? s : t ] = tp;
  if( head < tail ) {            // stopped early: the nodes not
   const int above = last + 2;   // reached get a lower bound of
   for( int v = 0 ; v < N ; ++v )  // their distance
    if( G[ v ] && ( D[ v ] == tp ) && ( v != s ) && ( v != t ) &&
	( above < tp ) ) {
     D[ v ] = above;
     q[ tail++ ] = v;
     }
   }
  for( std::size_t i = 1 ; i < tail ; ++i ) {
   const int v = q[ i ];
   bAdd( v );
   if( ex[ v ] > 0 )
    aPush( v );
   }
  };

 const long long nm = 6LL * N + static_cast< long long >( f_to.size() ) / 2;
 long long work = 0;
 // the discharges of the highest active nodes: each scans its edges once,
 // pushing to the lower neighbors and taking the least label of the
 // others, which is its new label if an excess is left; the gaps and the
 // global relabelings as in Cherkassky and Goldberg
 auto run = [ & ]( bool global ) {
  while( aMax >= 0 ) {
   const int u = aFirst[ aMax ];
   if( u < 0 ) {
    --aMax;
    continue;
    }
   aFirst[ aMax ] = aNext[ u ];
   queued[ u ] = 0;
   const int du = d[ u ];
   if( ( du != aMax ) || ( ex[ u ] == 0 ) )
    continue;  // a stale entry
   long long x = ex[ u ];
   int nd = top;
   const auto & A = f_adj[ u ];
   for( const auto & [ e , v ] : A ) {
    const int dv = d[ v ];
    if( ( dv >= du ) && ( dv >= nd ) )
     continue;  // no use, read no edge
    const long long r = R[ e ];
    if( r <= 0 )
     continue;
    if( dv < du ) {  // dv = du - 1, the labels valid
     const long long y = r < x ? r : x;
     R[ e ] -= y;
     R[ e ^ 1 ] += y;
     if( ( ex[ v ] == 0 ) && ( v != s ) && ( v != t ) )
      aPush( v );
     ex[ v ] += y;
     x -= y;
     if( x == 0 )
      break;
     }
    else if( dv < nd )
     nd = dv;
    }
   ex[ u ] = x;
   if( x == 0 )
    continue;
   // an excess left: the relabeling of u, or a gap
   work += 12 + static_cast< long long >( A.size() );
   bDel( u );
   if( bFirst[ du ] < 0 ) {  // a gap at du: all above it
    for( int k = du + 1 ; k <= bMax ; ++k ) {
     for( int v = bFirst[ k ] ; v >= 0 ; v = bNext[ v ] )
      d[ v ] = top;
     bFirst[ k ] = -1;
     }
    bMax = du - 1;
    d[ u ] = top;
    continue;
    }
   d[ u ] = nd + 1 < top ? nd + 1 : top;
   if( d[ u ] == top )
    continue;
   bAdd( u );
   aPush( u );
   if( global && ( 2 * work > nm ) ) {  // a global relabeling
    work = 0;
    relabelAll( false );
    }
   }
  };

 // the first phase: the edges out of s saturated, the labels up to the
 // last node with an excess, and a maximum preflow; the excesses that
 // cannot reach t end at the top
 for( const auto & [ e , v ] : f_adj[ s ] ) {
  const long long r = R[ e ];
  if( ( r > 0 ) && ( v != s ) ) {
   R[ e ] = 0;
   R[ e ^ 1 ] += r;
   ex[ v ] += r;
   }
  }
 relabelAll( true );
 run( true );

 // the second phase, only if an excess missed t: the excesses back to s,
 // on the labels of the distances to s among the nodes out of the game of
 // the first phase, those at the top
 bool left = false;
 for( int v = 0 ; ( v < N ) && ! left ; ++v )
  left = ( ex[ v ] > 0 ) && ( v != s ) && ( v != t ) && ( d[ v ] == top );
 if( ! left )
  return;
 for( int v = 0 ; v < N ; ++v )
  inGame[ v ] = ( d[ v ] == top ) || ( v == s );
 inGame[ t ] = 0;
 target = s;
 top = 2 * N;
 relabelAll( false );
 run( false );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* PushRelabel.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File PushRelabel.h -------------------------*/
/*--------------------------------------------------------------------------*/
