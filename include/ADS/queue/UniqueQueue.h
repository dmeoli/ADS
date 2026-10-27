/*--------------------------------------------------------------------------*/
/*--------------------------- File UniqueQueue.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A QUEUE [see `ADT.h`] with no duplicates, beyond the book, with one of two
 * policies for an item that is already in the queue: `IGNORE_NEW` does not
 * enqueue it again, `FORGET_OLD` removes the old copy and enqueues the item
 * at the back. The items are kept in a `List` [see `List.h`], so that
 * `enqueue()` takes \f$ O( n ) \f$ time for the search of the copy and the
 * other operations \f$ O( 1 ) \f$.
 *
 * The contracts [see `Contract.h`]: `top()` and `dequeue()` need a queue
 * that is not empty; `enqueue()` ensures that the item is in the queue, at
 * the back with `FORGET_OLD`; the invariant is that no item occurs twice.
 * The operations are defined in `UniqueQueue.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_UniqueQueue
 #define __ADS_UniqueQueue
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS UniqueQueue -----------------------------*/
/*--------------------------------------------------------------------------*/

/// what `enqueue()` does with an item already in the queue
enum class Duplicate {
 IGNORE_NEW ,   ///< the queue is left as it is
 FORGET_OLD     ///< the old copy is removed, the item goes to the back
 };

/// a QUEUE with no duplicates; the equality of `T` must be an equivalence,
/// so an item unequal to itself (e.g., a NaN) cannot be enqueued: the
/// postcondition of `enqueue()` refuses it
template< std::equality_comparable T , Duplicate D = Duplicate::IGNORE_NEW >
class UniqueQueue {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const;

 /// puts \f$ v \f$ at the back, unless the policy keeps an old copy of it
 ///
 /// In pseudocode (section 4.3.1, beyond the book), \f$ L \f$ the list of
 /// the items:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{enqueue}( v ) \cr
 /// 2 & \quad p \gets \text{the position of } v \text{ in } L \text{, by a }
 ///     \text{scan} \cr
 /// 3 & \quad \textbf{if not}\ L.\mathrm{finished}( p )\ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ D = \text{IGNORE\_NEW}\ \textbf{then return} \cr
 /// 5 & \qquad L.\mathrm{remove}( p ) \cr
 /// 6 & \quad \text{append } v \text{ to } L
 /// \end{array}}
 /// \f]
 /// \post \f$ v \f$ is in the queue, at the back with `FORGET_OLD`
 void enqueue( const T & v );

 /// removes the item at the front and returns it
 /// \pre the queue is not empty
 T dequeue( void );

 /// the item at the front
 /// \pre the queue is not empty
 [[nodiscard]] const T & top( void ) const;

 /// whether \f$ v \f$ is in the queue, \f$ O( n ) \f$
 [[nodiscard]] bool contains( const T & v ) const;

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items from the front to the back, bidirectional;
 // a change through it could break the uniqueness

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = const T &;              ///< a constant reference
 using const_reference = const T &;        ///< a constant reference
 /// a constant iterator of the list
 using iterator = typename List< T >::const_iterator;
 /// a constant iterator of the list
 using const_iterator = typename List< T >::const_iterator;

 /// the iterator on the item at the front
 [[nodiscard]] const_iterator begin( void ) const { return( f_L.begin() ); }

 /// the iterator past the item at the back
 [[nodiscard]] const_iterator end( void ) const { return( f_L.end() ); }

 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }

 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// no item occurs twice, \f$ O( n^2 ) \f$
 [[nodiscard]] bool unique( void ) const;

 /// the position of \f$ v \f$ in the list, or \f$ \mathit{pos}_n \f$
 [[nodiscard]] typename List< T >::Pos find( const T & v ) const;

 List< T > f_L;           ///< the items, the front first
 std::size_t f_size = 0;  ///< the number of items

 };  // end( class UniqueQueue )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/queue/impl/UniqueQueue.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* UniqueQueue.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File UniqueQueue.h -------------------------*/
/*--------------------------------------------------------------------------*/
