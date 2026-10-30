/*--------------------------------------------------------------------------*/
/*------------------------- File IndexedHeapPair.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A priority queue of the integers \f$ [ 0 , n ) \f$ by a key of selection
 * that also knows the least of a second key, the bound, among the ids it
 * holds, beyond the book: two heaps of `IndexedDaryHeap` [see
 * `IndexedDaryHeap.h`] on the same ids, one by the keys and one by the
 * bounds, so that `top()` is the id of least key and `minBound()` the least
 * bound in \f$ O( 1 ) \f$, and every change, `pop()` included, is made on
 * both heaps by the places of the id, in \f$ O( D \log_D s ) \f$. It is the
 * frontier of the searches that choose a node by one value and must know
 * the least of another one over the whole frontier: the bounded
 * suboptimal searches, which expand by a key of their own (the distance
 * to the goal, or an inflated cost) and need the least lower bound
 * \f$ f_{\mathrm{min}} \f$ to stop or to certify a factor of
 * suboptimality (the focal search of Pearl and Kim, the explicit
 * estimation search of Thayer and Ruml), and the branch and bound by best
 * estimate, which selects by the estimate of a node and prunes and stops
 * by the least bound of the open nodes.
 *
 * The alternative is a heap by the bounds with lazy deletion, in which the
 * ids popped by the key stay until they reach its top: it saves the
 * second walk of `pop()`, but `minBound()` must skip the stale ones and
 * an id pushed again needs a stamp to tell its copies apart, while here
 * the bound of an id is always the one it has, and `erase()` and
 * `update()` take an id out or change it anywhere in both orders (see
 * \ref theory_priority_heaps).
 *
 * The contracts [see `Contract.h`]: the ids are in \f$ [ 0 , n ) \f$,
 * `push()` needs an id not in the queue, `update()`, `erase()`, `key()` and
 * `bound()` one in it, `top()`, `topKey()`, `minBound()`, `minBoundId()`
 * and `pop()` a queue that is not empty; the invariant is that of the two
 * heaps, with the same ids in both. The operations are defined in
 * `IndexedHeapPair.tpp`.
 *
 * \see J. Pearl, J. H. Kim, Studies in semi-admissible heuristics, IEEE
 *      Transactions on Pattern Analysis and Machine Intelligence 4(4),
 *      392-399, 1982, doi:10.1109/TPAMI.1982.4767270
 * \see J. T. Thayer, W. Ruml, Bounded suboptimal search: a direct approach
 *      using inadmissible estimates, Proceedings of the 22nd International
 *      Joint Conference on Artificial Intelligence (IJCAI), 674-679, 2011,
 *      doi:10.5591/978-1-57735-516-8/IJCAI11-119
 * \see D. B. Johnson, Priority queues with update and finding minimum
 *      spanning trees, Information Processing Letters 4(3), 53-57, 1975,
 *      doi:10.1016/0020-0190(75)90001-0
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

#ifndef __ADS_IndexedHeapPair
 #define __ADS_IndexedHeapPair
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/IndexedDaryHeap.h"

#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS IndexedHeapPair --------------------------*/
/*--------------------------------------------------------------------------*/

/// the ids \f$ [ 0 , n ) \f$ by a key, the least on top, with the least of
/// their bounds
template< class K , class Bd = K , unsigned D = 4 >
class IndexedHeapPair {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys of selection
 using Bound = Bd;  ///< the type of the bounds

 /// the empty queue of the ids \f$ [ 0 , n ) \f$
 /// \pre \f$ n \ge 0 \f$
 explicit IndexedHeapPair( int n = 0 ) : f_key( n ) , f_bound( n ) {}

 /// the empty queue of the ids \f$ [ 0 , n ) \f$, the room kept
 /// \pre \f$ n \ge 0 \f$
 void reset( int n ) {
  f_key.reset( n );
  f_bound.reset( n );
  }

 /// the number of ids, \f$ n \f$
 [[nodiscard]] int ids( void ) const { return( f_key.ids() ); }

 /// whether the queue has no id
 [[nodiscard]] bool empty( void ) const { return( f_key.empty() ); }

 /// the number of ids in the queue, \f$ s \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_key.size() ); }

 /// whether the id is in the queue
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$
 [[nodiscard]] bool contains( int id ) const {
  return( f_key.contains( id ) );
  }

 /// the key of an id in the queue
 /// \pre the id is in the queue
 [[nodiscard]] const K & key( int id ) const { return( f_key.key( id ) ); }

 /// the bound of an id in the queue
 /// \pre the id is in the queue
 [[nodiscard]] const Bd & bound( int id ) const {
  return( f_bound.key( id ) );
  }

 /// inserts the id with the key \f$ k \f$ and the bound \f$ b \f$, in
 /// \f$ O( \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book), \f$ H_K \f$ and \f$ H_B \f$ the heaps
 /// by the keys and by the bounds:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( \mathit{id} , k , b ) \cr
 /// 2 & \quad H_K.\mathrm{push}( \mathit{id} , k );
 ///     \ H_B.\mathrm{push}( \mathit{id} , b )
 /// \end{array}}
 /// \f]
 /// \pre \f$ \mathit{id} \in [ 0 , n ) \f$, not in the queue
 void push( int id , const K & k , const Bd & b );

 /// sets the key of the id to \f$ k \f$ and its bound to \f$ b \f$, each
 /// heap moving the id up or down, in \f$ O( D \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{update}( \mathit{id} , k , b ) \cr
 /// 2 & \quad H_K.\mathrm{update}( \mathit{id} , k );
 ///     \ H_B.\mathrm{update}( \mathit{id} , b )
 /// \end{array}}
 /// \f]
 /// \pre the id is in the queue
 void update( int id , const K & k , const Bd & b );

 /// removes the id from the queue, in \f$ O( D \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( \mathit{id} ) \cr
 /// 2 & \quad H_K.\mathrm{erase}( \mathit{id} );
 ///     \ H_B.\mathrm{erase}( \mathit{id} )
 /// \end{array}}
 /// \f]
 /// \pre the id is in the queue
 void erase( int id );

 /// the id of least key
 /// \pre the queue is not empty
 [[nodiscard]] int top( void ) const { return( f_key.top().id ); }

 /// the least key, that of `top()`
 /// \pre the queue is not empty
 [[nodiscard]] const K & topKey( void ) const { return( f_key.top().key ); }

 /// the least bound of the ids in the queue
 /// \pre the queue is not empty
 [[nodiscard]] const Bd & minBound( void ) const {
  return( f_bound.top().key );
  }

 /// an id of least bound
 /// \pre the queue is not empty
 [[nodiscard]] int minBoundId( void ) const { return( f_bound.top().id ); }

 /// removes the id of least key and returns it, in
 /// \f$ O( D \log_D s ) \f$ time
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad \mathit{id} \gets H_K.\mathrm{pop}().\mathit{id};
 ///     \ H_B.\mathrm{erase}( \mathit{id} ) \cr
 /// 3 & \quad \textbf{return}\ \mathit{id}
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 int pop( void );

 /// removes the id of least bound and returns it, as `pop()` by the other
 /// heap
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{popMinBound}() \cr
 /// 2 & \quad \mathit{id} \gets H_B.\mathrm{pop}().\mathit{id};
 ///     \ H_K.\mathrm{erase}( \mathit{id} );\ \textbf{return}
 ///     \ \mathit{id}
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 int popMinBound( void );

 /// removes all the ids, in \f$ O( s ) \f$ time
 void clear( void ) {
  f_key.clear();
  f_bound.clear();
  }

/*--------------------------------------------------------------------------*/
 private:

 IndexedDaryHeap< K , D > f_key;     ///< the heap by the keys
 IndexedDaryHeap< Bd , D > f_bound;  ///< the heap by the bounds

 };  // end( class IndexedHeapPair )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/IndexedHeapPair.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IndexedHeapPair.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File IndexedHeapPair.h -----------------------*/
/*--------------------------------------------------------------------------*/
