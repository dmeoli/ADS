/*--------------------------------------------------------------------------*/
/*--------------------------- File BucketQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A priority queue for small nonnegative integer keys, beyond the book: a
 * two-level bucket queue, whose outer level is a vector indexed by the
 * first key f and whose inner one, for each f, a vector of LIFO lists
 * indexed by the second key g. top() is an item of least f and, among
 * those, of largest g, the last one pushed (the order that A* wants: least
 * f, ties broken towards the deeper nodes). The least f is found by a scan
 * that only moves forward between two pushes of a smaller f, and the
 * largest g of a layer likewise backward, so that push() takes O( 1 ) time
 * and pop() O( 1 ) amortized when the keys are bounded by C, against the
 * \f$ O( \log n ) \f$ of a heap: the queue of Dial for the shortest
 * paths with integer weights, nested as in the search code of Burns et al.
 * The lists of a layer are freed when it empties, so that the memory
 * follows the band of the keys in the queue.
 *
 * There is no decrease-key: as in the lazy deletion of A*, an item whose
 * key improves is pushed again, and the caller skips the stale copies when
 * they come out.
 *
 * The contracts [see Contract.h]: the keys are nonnegative, and top(),
 * topF(), topG() and pop() need a queue that is not empty; the invariant is
 * that the counts of the layers add up to size(). The operations are
 * defined in BucketQueue.tpp.
 *
 * \see R. B. Dial, Algorithm 360: shortest-path forest with topological
 *      ordering, Communications of the ACM 12(11), 632-633, 1969,
 *      doi:10.1145/363269.363610
 * \see E. Burns, M. Hatem, M. J. Leighton, W. Ruml, Implementing fast
 *      heuristic search code, Proceedings of the 5th Symposium on
 *      Combinatorial Search (SoCS), 25-32, 2012
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

#ifndef __ADS_BucketQueue
 #define __ADS_BucketQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <concepts>
#include <cstddef>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BucketQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// a two-level bucket queue: least f first, then largest g, then LIFO
template< class T , std::integral K = int >
class BucketQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Key = K;    ///< the type of the keys

 /// pushes x with the keys f and g
 /// \pre f >= 0 and g >= 0
 void push( const T & x , K f , K g );

 /// an item of least f and, among those, of largest g, the last pushed
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void );

 /// the f of top()
 /// \pre the queue is not empty
 [[nodiscard]] K topF( void );

 /// the g of top()
 /// \pre the queue is not empty
 [[nodiscard]] K topG( void );

 /// removes top() and returns it
 /// \pre the queue is not empty
 T pop( void );

 [[nodiscard]] bool isEmpty( void ) const;

 /// the number of items, the stale ones included
 [[nodiscard]] std::size_t size( void ) const;

 /// removes all the items and frees the layers
 void clear( void );

/*--------------------------------------------------------------------------*/
 private:

 /// the items of one f
 struct Layer {
  std::vector< std::vector< T > > byG;   ///< a LIFO list per g
  std::size_t count = 0;                 ///< the items in the layer
  std::size_t topG = 0;                  ///< no list above it has items
  };

 /// the layer of the least f, its topG moved to its largest g
 Layer & front( void );

 /// the counts of the layers add up to size(), O( layers )
 [[nodiscard]] bool counted( void ) const;

 std::vector< Layer > f_layers;   ///< a layer per f
 std::size_t f_minF = 0;          ///< no layer below it has items
 std::size_t f_size = 0;          ///< the number of items

 };  // end( class BucketQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/BucketQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BucketQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BucketQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
