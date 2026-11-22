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
 * each producer come out in the order in which it sent them, and the
 * consumer never competes with the producers on single items; since the
 * consumer only takes the whole stack, and never pops a single cell, the ABA
 * problem of the stack of Treiber cannot occur (a cell used again is pushed
 * as a new one, above the top that its pusher read).
 *
 * The cells come from blocks of 64 that each producing thread owns, one set
 * for each type of message, shared by all the inboxes it sends to: the
 * producer takes the cells of its current block in order, with no atomic
 * operation, and a consumer that has taken some messages of a block adds
 * their number to a count of the block, once per run of them; the consumer
 * that brings it to 64 gives the block back to its owner, on a stack that
 * the owner empties at once (an exchange) when it needs a block, before it
 * allocates one. Thus a producer in a steady state cycles over the same
 * blocks, with no allocation, and the consumer frees nothing. Hence
 * `take()` costs one exchange however many messages there are, one
 * fetch-and-add per run of messages of one block, a loop of compare-and-swap
 * per block given back, and \f$ O( k ) \f$ time for the \f$ k \f$
 * messages taken: it is lock-free, and wait-free but for the blocks given
 * back. A block is given back only after its consumers have moved out the
 * messages of all its cells, so that a cell is never written again while a
 * consumer reads it, nor pushed while it is in a stack. A producer
 * keeps for reuse all the blocks it ever held at once, at least
 * \f$ \lceil m / 64 \rceil \f$ for \f$ m \f$ of its messages in the
 * inboxes. A thread that ends frees the blocks it holds and closes its
 * stack, and the consumer that empties one of its blocks still in some
 * inbox frees it.
 *
 * A push wakes the consumer only when it finds the stack empty: a push onto
 * a stack that is not empty is taken by the same `take()` as the push that
 * found it empty, which woke the consumer, so that a burst of messages
 * costs one fence of the event count instead of one per message.
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
#include <cstddef>
#include <cstdint>
#include <new>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPSCInbox -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an inbox filled by many threads and emptied by one, lock-free
template< class T >
class MPSCInbox
{

/*--------------------------------------------------------------------------*/
 public:
 using Item = T;  ///< the type of the messages

 /// the empty inbox
 MPSCInbox( void ) = default;
 /// no copy, since the threads share it by its address
 MPSCInbox( const MPSCInbox & ) = delete;
 /// no copy, since the threads share it by its address
 MPSCInbox & operator=( const MPSCInbox & ) = delete;

 /// frees the messages not taken
 ~MPSCInbox();

 /// sends \f$ v \f$, and wakes the consumer if it sleeps in `waitTake()` and
 /// the stack was empty (a fence and the read of a line when it does not);
 /// any thread may call it
 ///
 /// In pseudocode (the push of Treiber, beyond the book), with
 /// \f$ \mathit{top} \f$ the top of the stack:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{push}( v ) \cr
 /// 2 & \quad c \gets \text{the next cell of the block of the thread,}
 ///     \text{ holding } v \cr
 /// 3 & \quad \textbf{repeat}\ c.\mathit{next} \gets \mathit{top}
 ///     \ \textbf{until}\ \mathrm{CAS}( \mathit{top} , c.\mathit{next} , c
 ///     )\quad \triangleright\ \text{release} \cr
 /// 4 & \quad \textbf{if}\ c.\mathit{next} = \textbf{nil}\ \textbf{then}
 ///     \ \mathrm{notify}()
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
 /// 4 & \quad \text{add to the count of each block the cells taken from}
 ///     \text{ it};\ \text{give back to its owner a block whose count}
 ///     \text{ is } 64 \cr
 /// 5 & \quad \textbf{return}\ L \text{ reversed}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::vector< T > take( void );

 /// the messages of `take()` appended to `out`, which keeps its room from
 /// call to call, and their number; only the consumer calls it
 std::size_t takeInto( std::vector< T > & out );

 /// the messages of `take()`, sleeping until at least one comes [see
 /// `EventCount.h`]; only the consumer calls it
 [[nodiscard]] std::vector< T > waitTake( void ) {
  std::vector< T > v;
  f_wait.await( [ & ] {
   v = take();
   return( ! v.empty() );
   } );
  return( v );
  }

 /// true if no message was waiting at some moment of the call
 [[nodiscard]] bool empty( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 static constexpr std::size_t kCells = 64;  ///< the cells of a block

 struct Block;
 struct Owner;

 /// a cell of the stack
 struct Cell
 {
  /// the room of the message
  alignas( T ) unsigned char raw[ sizeof( T ) ];
  /// the cell below on the stack
  Cell * next;
  /// the block of the cell
  Block * home;
  /// the message
  [[nodiscard]] T * value( void ) {
   return( std::launder( reinterpret_cast< T * >( raw ) ) );
   }
  };

 /// a block of cells of a producer, with the count of the cells taken
 struct Block
 {
  /// the cells taken by the consumers, or never to be used
  alignas( 64 ) std::atomic< std::size_t > taken{ 0 };
  Owner * owner;            ///< the thread that fills it
  Block * spare = nullptr;  ///< in the stacks of the owner
  Cell cells[ kCells ];     ///< the cells

  /// a block of o
  explicit Block( Owner * o ) : owner( o ) {
   for( Cell & c : cells )
    c.home = this;
   }
  };

 /// the blocks of a producing thread
 struct Owner
 {
  Block * cur = nullptr;    ///< the block it fills
  std::size_t used = 0;     ///< the cells of cur given out
  Block * spare = nullptr;  ///< its blocks to reuse
  /// the blocks given back, closed once the thread has ended
  alignas( 64 ) std::atomic< Block * > given{ nullptr };
  /// the thread, and each block not freed
  std::atomic< std::size_t > refs{ 1 };
  };

 /// the owner of the running thread, freed with it
 class Home;

 /// the owner of the running thread
 static Owner & owner( void );

 /// a block for o to fill: one of its own, one given back, or a new one
 static Block * fresh( Owner & o );

 /// k cells of b taken; b given back to its owner if all are
 static void release( Block * b , std::size_t k );

 /// frees b, and its owner after its thread and its last block
 static void destroy( Block * b );

 /// the stack given back of a thread that has ended
 static Block * closed( void ) {
  return( reinterpret_cast< Block * >( std::uintptr_t( 1 ) ) );
  }

 /// the cells from c, oldest first: their number to `count`, then each
 /// message to f before its block may be given back
 template< class G , class F >
 static void drain( Cell * c , G && count , F && f );

 std::atomic< Cell * > f_top{ nullptr };  ///< the top of the stack
 EventCount f_wait;                       ///< the sleep of the consumer

 };  // end( class MPSCInbox )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/concurrent/impl/MPSCInbox.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MPSCInbox.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MPSCInbox.h --------------------------*/
/*--------------------------------------------------------------------------*/
