/*--------------------------------------------------------------------------*/
/*--------------------------- File RandomQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The random queue of exercise 2.3 of Morin's book, beyond the book: a bag
 * in which `push()` puts an item and `pop()` takes one out chosen uniformly
 * at random among those inside, both in \f$ O( 1 ) \f$ amortized time. The
 * items are the first \f$ n \f$ places of a vector that doubles [see
 * `ArraySequence.h`]; `pop()` draws a place \f$ j \f$ uniform in
 * \f$ [ 0 , n ) \f$, swaps it with the last one and removes the last one,
 * so that the order of the vector never matters and nothing moves but two
 * items. The place is drawn by the multiplication of Lemire, the high half
 * of the product of a word of SplitMix64 [see `RandomHashing.h`] and
 * \f$ n \f$, with the rejection of the few words that would bias it, and
 * no division in the common case (a division by \f$ n \f$ only when the low
 * half falls below \f$ n \f$); the generator is seeded, so that a run is
 * reproducible. `sample()` reads an item at random without removing it.
 *
 * Each item is equally likely: by induction, the items left after any
 * sequence of operations are a uniform sample of those pushed, whatever
 * their order in the vector, since the draw does not look at it.
 *
 * The contracts [see `Contract.h`]: `pop()` and `sample()` need a queue
 * that is not empty; `pop()` ensures one item less. The operations are
 * defined in `RandomQueue.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, exercise 2.3
 * \see D. Lemire, Fast random integer generation in an interval, ACM
 *      Transactions on Modeling and Computer Simulation 29(1), 3:1-3:12,
 *      2019, doi:10.1145/3230636
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

#ifndef __ADS_RandomQueue
 #define __ADS_RandomQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/dictionary/RandomHashing.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RandomQueue ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bag from which `pop()` takes an item uniformly at random
template< class T >
class RandomQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the empty queue, its draws from the seed
 explicit RandomQueue( std::uint64_t seed = 1 ) : f_g( seed ) {}

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_A.empty() ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// puts \f$ x \f$ in, \f$ O( 1 ) \f$ amortized (Morin's `add( x )`)
 void push( const T & x ) { f_A.push_back( x ); }

 /// puts \f$ x \f$ in, moved
 void push( T && x ) { f_A.push_back( std::move( x ) ); }

 /// takes out an item chosen uniformly at random and returns it,
 /// \f$ O( 1 ) \f$ (Morin's `remove()`)
 ///
 /// In pseudocode (Morin, exercise 2.3), \f$ A[ 0 .. n - 1 ] \f$ the items:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pop}() \cr
 /// 2 & \quad j \gets \text{uniform in } \{ 0 , \ldots , n - 1 \} \cr
 /// 3 & \quad \mathrm{swap}( A[ j ] , A[ n - 1 ] );\ n \gets n - 1 \cr
 /// 4 & \quad \textbf{return}\ A[ n ]
 /// \end{array}}
 /// \f]
 /// \pre the queue is not empty
 /// \post one item less
 T pop( void );

 /// an item chosen uniformly at random, left in
 /// \pre the queue is not empty
 [[nodiscard]] const T & sample( void );

 /// a place uniform in \f$ [ 0 , n ) \f$, by the multiplication of Lemire
 ///
 /// In pseudocode (Lemire, beyond the book), \f$ w \f$ the words of 64
 /// bits of the generator:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{uniform}( n ) \cr
 /// 2 & \quad ( h , l ) \gets \text{the two halves of } w \cdot n \cr
 /// 3 & \quad \textbf{if}\ l < n\ \textbf{then} \cr
 /// 4 & \qquad t \gets ( 2^{64} - n ) \bmod n \cr
 /// 5 & \qquad \textbf{while}\ l < t\ \textbf{do}\ ( h , l ) \gets
 ///     \text{the halves of } w \cdot n \text{ for a new } w \cr
 /// 6 & \quad \textbf{return}\ h
 /// \end{array}}
 /// \f]
 /// \pre \f$ n > 0 \f$
 [[nodiscard]] std::size_t uniform( std::size_t n );

 /// removes all the items
 void clear( void ) { f_A.clear(); }

 /// the items, in no order
 [[nodiscard]] const std::vector< T > & items( void ) const { return( f_A ); }

/*--------------------------------------------------------------------------*/
 private:

 std::vector< T > f_A;   ///< the items
 SplitMix64 f_g;         ///< the generator of the draws

 };  // end( class RandomQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/RandomQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RandomQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File RandomQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
