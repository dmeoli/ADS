/*--------------------------------------------------------------------------*/
/*---------------------------- File MPSCInbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An inbox that many producer threads fill and one consumer thread empties,
 * lock-free, beyond the book: the messages to a worker of a parallel
 * search, e.g., the nodes that the other workers send to their owner. A
 * producer pushes a cell on a stack with a compare-and-swap on its top (the
 * stack of Treiber); the consumer takes the whole stack at once by
 * exchanging its top with nil, and reverses it, so that the messages of
 * each producer come out in the order in which it sent them. Thus `take()`
 * costs one atomic operation however many messages there are, plus
 * \f$ O( k ) \f$ time for the \f$ k \f$ messages taken, and the consumer
 * never competes with the producers on single items; since the consumer
 * only takes the whole stack, and never pops a single cell, the ABA problem
 * of the stack of Treiber cannot occur.
 *
 * The contracts [see `Contract.h`]: only one thread calls `take()`. The
 * operations are defined in `MPSCInbox.tpp`.
 *
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986
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

#ifndef __ADS_MPSCInbox
 #define __ADS_MPSCInbox
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/EventCount.h"

#include <atomic>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPSCInbox -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an inbox filled by many threads and emptied by one, lock-free
template< class T >
class MPSCInbox {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the messages

 /// the empty inbox
 MPSCInbox( void ) = default;
 /// no copy, since the threads share it by its address
 MPSCInbox( const MPSCInbox & ) = delete;
 /// no copy, since the threads share it by its address
 MPSCInbox & operator=( const MPSCInbox & ) = delete;

 /// frees the messages not taken
 ~MPSCInbox();

 /// sends \f$ v \f$, and wakes the consumer if it sleeps in `waitTake()` (a
 /// fence and the read of a line when it does not); any thread may call it
 ///
 /// In pseudocode (the push of Treiber, beyond the book), with
 /// \f$ \mathit{top} \f$ the top of the stack:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( v ) \cr
 /// 2 & \quad c \gets \text{a new cell of } v \cr
 /// 3 & \quad \textbf{repeat}\ c.\mathit{next} \gets \mathit{top}
 ///     \ \textbf{until}\ \mathrm{CAS}( \mathit{top} , c.\mathit{next} , c
 ///     )\quad \triangleright\ \text{release} \cr
 /// 4 & \quad \mathrm{notify}()
 /// \end{array}}
 /// \f]
 void push( T v );

 /// all the messages sent so far and not yet taken, those of each sender
 /// in the order of sending; only the consumer calls it
 ///
 /// In pseudocode (beyond the book), the whole stack taken at once:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{take}() \cr
 /// 2 & \quad c \gets \mathrm{exchange}( \mathit{top} , \textbf{nil} )\quad
 ///     \triangleright\ \text{acquire} \cr
 /// 3 & \quad L \gets \text{the values of the cells from } c \text{, newest}
 ///     \text{ first} \cr
 /// 4 & \quad \textbf{return}\ L \text{ reversed}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::vector< T > take( void );

 /// the messages of `take()`, sleeping until at least one comes [see
 /// `EventCount.h`]; only the consumer calls it
 [[nodiscard]] std::vector< T > waitTake( void ) {
  std::vector< T > v;
  f_wait.await( [ & ] { v = take(); return( ! v.empty() ); } );
  return( v );
  }

 /// true if no message was waiting at some moment of the call
 [[nodiscard]] bool empty( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a cell of the stack
 struct Cell {
  /// the message
  T value;
  /// the cell below on the stack
  Cell * next;
  };

 std::atomic< Cell * > f_top{ nullptr };   ///< the top of the stack
 EventCount f_wait;                        ///< the sleep of the consumer

 };  // end( class MPSCInbox )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/MPSCInbox.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MPSCInbox.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MPSCInbox.h --------------------------*/
/*--------------------------------------------------------------------------*/
