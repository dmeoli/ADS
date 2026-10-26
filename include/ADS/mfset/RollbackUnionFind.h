/*--------------------------------------------------------------------------*/
/*------------------------ File RollbackUnionFind.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET of section 10.2 with the undo of its merges, beyond the book
 * [see MFSet.h], and on it the connectivity of a graph whose edges come
 * and go, when all the changes and the questions are known in advance.
 *
 * RollbackUnionFind is the forest of the parents with the union by rank
 * and without any compression of the paths: merge( x , y ) hangs the root
 * of lower rank under the other, so that a tree of rank k has at least
 * \f$ 2^k \f$ nodes and height at most k, and
 * \f[
 *   \mathit{find}( x ) \in O( \log n )
 * \f]
 * in the worst case, not only amortized. Each merge() changes at most two
 * words (the parent of a root, and the rank of the other), which a record
 * on a stack keeps: undo() puts them back in \f$ O( 1 ) \f$, and
 * rollback( t ) undoes the merges back to the checkpoint t, the number of
 * merges done when checkpoint() gave it (a merge of two elements already
 * together leaves a record too, so that the checkpoints count the calls).
 * The compression of the paths would change \f$ \Theta( \log n ) \f$
 * parents per find(), all to be recorded, and its amortized bound does not
 * survive the undo: Westbrook and Tarjan show that with backtracking the
 * union by rank alone, \f$ \Theta( \log n ) \f$ per operation, is as good
 * as any of the usual compressions, and Mannila and Ukkonen gave the first
 * analysis of the problem. The space is \f$ 12 n \f$ bytes and
 * 12 bytes per record.
 *
 * offlineConnectivity() answers, for a sequence of k operations on n
 * nodes, each the insertion of an edge, its deletion or a question
 * (whether u and v are connected, or how many components there are), the
 * questions in their order. Each edge lives on an interval of times
 * \f$ [ s , e ) \f$, from its insertion to its deletion (or to the end);
 * the times of the q questions are the leaves of a segment tree, whose
 * node of range \f$ [ a , b ) \f$ holds the edges that live on all of it
 * and not on all of its parent's, so that an interval is split into at
 * most \f$ 2 \lceil \log_2 q \rceil \f$ nodes. A visit in depth of the
 * tree merges the edges of a node when it enters it and rolls them back
 * when it leaves: at a leaf the forest holds exactly the edges alive at
 * its time, and the question is a find(). Each edge is merged and undone
 * \f$ O( \log q ) \f$ times, at \f$ O( \log n ) \f$ each, i.e.,
 * \f[
 *   O( k \log k \log n )
 * \f]
 * time and \f$ O( k \log k ) \f$ space in all. The divide and conquer on
 * the times is the one of Eppstein for the dynamic minimum spanning tree;
 * with the contraction of the graph at each level (Eppstein, and Peng,
 * Sandlund and Sleator) the bound falls to \f$ O( k \log n ) \f$, at the
 * cost of a far longer code, which the rollback leaves out.
 * <pre>
 *   offlineConnectivity( n , ops )
 *  1  for each edge e inserted at the time s and deleted at the time e
 *     (or never: e = k), add( 1 , [ 0 , q ) , [ q( s ) , q( e ) ) , e ),
 *     q( t ) the questions before the time t
 *  2  visit( 1 , [ 0 , q ) )
 *   add( x , [ a , b ) , [ s , e ) , e )
 *  1  if \f$ [ s , e ) \cap [ a , b ) = \emptyset \f$ then return
 *  2  if \f$ [ a , b ) \subseteq [ s , e ) \f$ then put e in L( x ); return
 *  3  c ← \f$ \lfloor ( a + b ) / 2 \rfloor \f$; add( 2 x , [ a , c ) , ...
 *     ); add( 2 x + 1 , [ c , b ) , ... )
 *   visit( x , [ a , b ) )
 *  1  t ← checkpoint()
 *  2  for each [ u , v ] ∈ L( x ): merge( u , v )
 *  3  if b - a = 1 then answer the question a by find()
 *  4  else visit( 2 x , [ a , c ) ); visit( 2 x + 1 , [ c , b ) )
 *  5  rollback( t )
 * </pre>
 *
 * The contracts [see Contract.h]: n >= 0, the elements in [ 0 , n ); a
 * rollback goes back to a checkpoint not beyond the merges done, and an
 * undo() needs one; a deletion in offlineConnectivity() needs the edge
 * alive (of parallel edges, the last inserted dies first), and a loop
 * [ u , u ] is allowed and joins nothing. The invariant is that the
 * parents make a forest of count() trees, the rank of each root bounding
 * its height, and that the records undo the merges. The operations are
 * defined in RollbackUnionFind.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see H. Mannila, E. Ukkonen, The set union problem with backtracking,
 *      Proceedings of ICALP, LNCS 226, 236-243, 1986,
 *      doi:10.1007/3-540-16761-7_73
 * \see J. Westbrook, R. E. Tarjan, Amortized analysis of algorithms for set
 *      union with backtracking, SIAM Journal on Computing 18(1), 1-11,
 *      1989, doi:10.1137/0218001
 * \see A. Apostolico, G. F. Italiano, G. Gambosi, M. Talamo, The set union
 *      problem with unlimited backtracking, SIAM Journal on Computing
 *      23(1), 50-70, 1994, doi:10.1137/S0097539789173597
 * \see D. Eppstein, Offline algorithms for dynamic minimum spanning tree
 *      problems, Journal of Algorithms 17(2), 237-250, 1994,
 *      doi:10.1006/jagm.1994.1033
 * \see R. Peng, B. Sandlund, D. D. Sleator, Optimal offline dynamic
 *      2,3-edge/vertex connectivity, Proceedings of WADS, LNCS 11646,
 *      553-565, 2019, doi:10.1007/978-3-030-24766-9_40
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

#ifndef __ADS_RollbackUnionFind
 #define __ADS_RollbackUnionFind
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS RollbackUnionFind -------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets of [ 0 , n ) by rank, with no compression,
/// whose merges can be undone
class RollbackUnionFind {

/*--------------------------------------------------------------------------*/
 public:

 /// the n components {0} , ... , {n - 1}, no merge done
 /// \pre n >= 0
 explicit RollbackUnionFind( int n = 0 );

 /// back to the n components, no merge done
 /// \pre n >= 0
 void reset( int n );

 [[nodiscard]] int n( void ) const { return( int( f_p.size() ) ); }

 /// the number of components
 [[nodiscard]] int count( void ) const { return( f_count ); }

 /// the representative of the component of x, \f$ O( \log n ) \f$
 /// \pre x in [ 0 , n )
 [[nodiscard]] int find( int x ) const;

 /// joins the components of x and y, the root of lower rank under the
 /// other, and records it; true if they were distinct
 /// \pre x , y in [ 0 , n )
 bool merge( int x , int y );

 /// whether x and y are in the same component
 /// \pre x , y in [ 0 , n )
 [[nodiscard]] bool same( int x , int y ) const {
  return( find( x ) == find( y ) );
  }

 /// the number of elements of the component of x
 /// \pre x in [ 0 , n )
 [[nodiscard]] int size( int x ) const { return( f_size[ find( x ) ] ); }

 /// the merges done and not undone, the time to roll back to
 [[nodiscard]] int checkpoint( void ) const {
  return( int( f_stack.size() ) );
  }

 /// undoes the last k merges
 /// \pre 0 <= k <= checkpoint()
 void undo( int k = 1 );

 /// undoes the merges back to the checkpoint t
 /// \pre 0 <= t <= checkpoint()
 void rollback( int t );

/*--------------------------------------------------------------------------*/
 private:

 /// a merge: the root hung (-1 if nothing was joined) and whether the
 /// rank of the other grew
 struct Record {
  int child , parent;
  bool grew;
  };

 /// a forest of count() trees, each of height at most the rank of its
 /// root and of the size it holds
 [[nodiscard]] bool forest( void ) const;

 std::vector< int > f_p;          ///< the parents, x itself at a root
 std::vector< int > f_rank;       ///< the ranks of the roots
 std::vector< int > f_size;       ///< the sizes of the roots
 std::vector< Record > f_stack;   ///< the merges, to undo
 int f_count = 0;                 ///< the number of components

 };  // end( class RollbackUnionFind )

/*--------------------------------------------------------------------------*/
/*----------------------- OFFLINE DYNAMIC CONNECTIVITY ---------------------*/
/*--------------------------------------------------------------------------*/

/// an operation on a graph whose edges come and go
struct ConnectivityOp {
 /// the insertion or the deletion of the edge [ u , v ], whether u and v
 /// are connected, how many components there are
 enum Kind { INSERT , DELETE , CONNECTED , COMPONENTS };
 Kind kind;
 int u = 0 , v = 0;
 };

/// the answers to the questions of ops on n nodes, in their order: 1 or 0
/// for CONNECTED, the number of components for COMPONENTS
/// \pre n >= 0, the nodes in [ 0 , n ), each DELETE of an edge alive
[[nodiscard]] inline std::vector< int > offlineConnectivity(
                          int n , const std::vector< ConnectivityOp > & ops );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/RollbackUnionFind.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RollbackUnionFind.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RollbackUnionFind.h ----------------------*/
/*--------------------------------------------------------------------------*/
