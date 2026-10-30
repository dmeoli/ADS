/*--------------------------------------------------------------------------*/
/*----------------------------- File Mailbox.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The mailbox of the long-lived workers of an executor [see `Executor.h`],
 * beyond the book, on any family of channels [see `Channels.h`], with the
 * detection of the termination: a computation of \f$ p \f$ workers that
 * create work for each other by messages (the nodes of a search sent to the
 * worker that owns them, the vertices of a graph to the worker of their
 * part) ends when no worker has work and no message is on its way, a state
 * that no worker sees by itself (see \ref theory_parallel_quiescence). A
 * worker `me` sends by `send( me , to , x )`, which appends \f$ x \f$ to its
 * outbox for `to` and pushes the outbox as a batch when it holds `batch`
 * messages; `flush( me )` pushes all its outboxes; `poll( me , f )` drains
 * its inbox, calling `f( x )` on each message; `quiescent( me )`, called by a
 * worker that has no work left, flushes its outboxes and tells whether the
 * whole computation has ended. `ownerOf( k , p )` is the worker that owns
 * the key \f$ k \f$ when the keys are spread by hashing.
 *
 * The termination is detected by counting: a counter \f$ C \f$ shared by the
 * workers holds the number of the workers <em>active</em> (all of them at
 * the start) plus the number of the messages pushed and not yet drained. A
 * worker adds a batch of \f$ k \f$ messages to \f$ C \f$ before it pushes
 * it, takes back an activity (adds 1) when it drains messages while passive,
 * before it calls \f$ f \f$ on the first, subtracts the \f$ k \f$ messages it
 * drained once it has called \f$ f \f$ on them, and subtracts its activity
 * when it becomes passive, i.e., when it calls `quiescent()` with all its
 * outboxes pushed. A message is counted from before its push to after its
 * call, and a worker that sends is active, so that \f$ C = 0 \f$ only when
 * no worker is active and no message exists, a state from which nothing
 * leaves \f$ C \f$ at 0: the deficit counting of Dijkstra and Scholten, on a
 * counter in the shared memory instead of the acknowledgments of each
 * message. A batch costs one atomic addition at the sender and one at the
 * receiver, so that the counter, shared by all, is touched once per batch
 * and not per message; `quiescent()` of a passive worker only reads it.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$, a batch of at least 1;
 * only the worker `me` calls the operations with `me`; a worker sends only
 * while active, i.e., before its first `quiescent()` or after a `poll()` that
 * found messages, and becomes active only by receiving (its own work comes
 * from the messages, or it was there from the start); `f` does not throw; a
 * mailbox serves one computation.
 *
 * \see E. W. Dijkstra, C. S. Scholten, Termination detection for diffusing
 *      computations, Information Processing Letters 11(1), 1-4, 1980,
 *      doi:10.1016/0020-0190(80)90021-6
 * \see F. Mattern, Algorithms for distributed termination detection,
 *      Distributed Computing 2(3), 161-175, 1987, doi:10.1007/BF01782776
 * \see F. Mattern, Global quiescence detection based on credit
 *      distribution and recovery, Information Processing Letters 30(4),
 *      195-200, 1989, doi:10.1016/0020-0190(89)90212-3
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

#ifndef __ADS_Mailbox
 #define __ADS_Mailbox
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Channels.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the owner of the key \f$ k \f$ among \f$ p \f$ workers or processes,
/// \f$ h( k ) \bmod p \f$ for the hash function \f$ h \f$
/// \pre \f$ p \ge 1 \f$
template< class K , class H = std::hash< K > >
[[nodiscard]] std::size_t ownerOf( const K & k , std::size_t p ,
                                   const H & h = H() )
{
 ADS_PRE( p >= 1 );
 return( std::size_t( h( k ) ) % p );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Mailbox ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the mailbox of \f$ p \f$ workers on the channels `Ch` (by default the
/// mesh, the family that `DefaultChannels` gives every executor), with
/// outboxes of batches and the detection of the termination
template< class Msg , ChannelFamily Ch = MeshChannels< Msg > >
class Mailbox {

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages
 using Channels = Ch;   ///< the family of the channels

 /// the mailbox of \f$ p \f$ workers, all active, pushing batches of
 /// `batch` messages, on channels built from \f$ p \f$ and `args`
 /// \pre \f$ p \ge 1 \f$, \f$ \mathit{batch} \ge 1 \f$
 template< class ... A >
 explicit Mailbox( std::size_t p , std::size_t batch = 64 , A && ... args )
  : f_ch( p , std::forward< A >( args )... ) , f_worker( p ) ,
    f_batch( batch ) {
  ADS_PRE( ( p >= 1 ) && ( batch >= 1 ) );
  for( auto & w : f_worker ) {
   w = std::make_unique< Worker >();
   w->out.resize( p );
   }
  f_count.value.store( std::int64_t( p ) , std::memory_order_relaxed );
  }

 /// the number \f$ p \f$ of the workers
 [[nodiscard]] std::size_t size( void ) const { return( f_worker.size() ); }

 /// \f$ x \f$ appended to the outbox of `me` for `to`, which is pushed when
 /// it holds a batch
 ///
 /// In pseudocode (beyond the book), \f$ O_{\mathit{to}} \f$ the outbox:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{send}( \mathit{me} , \mathit{to} , x )
 ///     \cr
 /// 2 & \quad \text{append } x \text{ to } O_{\mathit{to}} \cr
 /// 3 & \quad \textbf{if}\ |O_{\mathit{to}}| \ge \mathit{batch}\ \textbf{then}
 ///     \ \mathrm{push}( \mathit{me} , \mathit{to} )
 /// \end{array}}
 /// \f]
 /// \pre `me` and `to` are workers, `me` is active
 void send( std::size_t me , std::size_t to , Msg x ) {
  ADS_PRE( ( me < size() ) && ( to < size() ) );
  Worker & w = *f_worker[ me ];
  ADS_PRE( w.active );
  auto & o = w.out[ to ];
  if( o.empty() )
   w.dirty.push_back( to );
  o.push_back( std::move( x ) );
  if( o.size() >= f_batch )
   push( me , to );
  }

 /// the outboxes of `me` pushed, true if all of them are empty after it
 /// (a bounded family may refuse some messages, which stay)
 ///
 /// In pseudocode (beyond the book), \f$ C \f$ the shared counter:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{flush}( \mathit{me} ) \cr
 /// 2 & \quad \textbf{foreach}\ \mathit{to} \text{ with } O_{\mathit{to}}
 ///     \ne \emptyset\ \textbf{do}\ \mathrm{push}( \mathit{me} ,
 ///     \mathit{to} ) \cr
 /// 3 & \quad \textbf{return}\ \text{all the outboxes are empty} \cr
 /// 4 & \textbf{procedure}\ \mathrm{push}( \mathit{me} , \mathit{to} ) \cr
 /// 5 & \quad k \gets |O_{\mathit{to}}|;\ C \gets C + k\quad
 ///     \triangleright\ \text{before the messages can be drained} \cr
 /// 6 & \quad j \gets \mathrm{tryPush}( \mathit{me} , \mathit{to} ,
 ///     O_{\mathit{to}} );\ C \gets C - ( k - j ) \cr
 /// 7 & \quad \text{remove the first } j \text{ messages of }
 ///     O_{\mathit{to}}
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 bool flush( std::size_t me ) {
  ADS_PRE( me < size() );
  Worker & w = *f_worker[ me ];
  std::size_t keep = 0;
  for( std::size_t i = 0 ; i < w.dirty.size() ; ++i ) {
   const std::size_t to = w.dirty[ i ];
   push( me , to );
   if( ! w.out[ to ].empty() )
    w.dirty[ keep++ ] = to;
   }
  w.dirty.resize( keep );
  return( keep == 0 );
  }

 /// the calls `f( x )` on the messages of the inbox of `me`, and their
 /// number; a passive worker that finds messages becomes active
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{poll}( \mathit{me} , f ) \cr
 /// 2 & \quad k \gets \mathrm{drain}( \mathit{me} , x \mapsto \textbf{if}
 ///     \ \textbf{not}\ \mathit{active}\ \textbf{then}\ C \gets C + 1;
 ///     \ \mathit{active} \gets \textbf{true}; \cr
 /// & \qquad f( x ) ) \cr
 /// 3 & \quad \textbf{if}\ k > 0\ \textbf{then}\ C \gets C - k \cr
 /// 4 & \quad \textbf{return}\ k
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 template< class F >
 std::size_t poll( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  Worker & w = *f_worker[ me ];
  const std::size_t k = f_ch.drain( me , [ & ]( Msg && x ) {
   if( ! w.active ) {                        // back to work, counted
    f_count.value.fetch_add( 1 , std::memory_order_acq_rel );
    w.active = true;
    }
   f( std::move( x ) );
   } );
  if( k > 0 ) {
   w.received += k;
   f_count.value.fetch_sub( std::int64_t( k ) , std::memory_order_acq_rel );
   }
  return( k );
  }

 /// true if the computation has ended; called by `me` with no work left,
 /// it first pushes its outboxes and, if they are all empty, becomes
 /// passive
 ///
 /// In pseudocode (the deficit counting of Dijkstra and Scholten on a
 /// shared counter, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{quiescent}( \mathit{me} ) \cr
 /// 2 & \quad \textbf{if}\ \mathit{active}\ \textbf{then} \cr
 /// 3 & \qquad \textbf{if}\ \textbf{not}\ \mathrm{flush}( \mathit{me} )
 ///     \ \textbf{then}\ \textbf{return false} \cr
 /// 4 & \qquad \mathit{active} \gets \textbf{false};\ C \gets C - 1 \cr
 /// 5 & \quad \textbf{return}\ C = 0
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 bool quiescent( std::size_t me ) {
  ADS_PRE( me < size() );
  Worker & w = *f_worker[ me ];
  if( w.active ) {
   if( ! flush( me ) )
    return( false );
   w.active = false;
   if( f_count.value.fetch_sub( 1 , std::memory_order_acq_rel ) == 1 )
    return( true );
   }
  return( f_count.value.load( std::memory_order_acquire ) == 0 );
  }

 /// true once the computation has ended (any thread may ask)
 [[nodiscard]] bool terminated( void ) const {
  return( f_count.value.load( std::memory_order_acquire ) == 0 );
  }

 /// the messages that `me` has pushed so far
 [[nodiscard]] std::uint64_t sent( std::size_t me ) const {
  ADS_PRE( me < size() );
  return( f_worker[ me ]->sent );
  }

 /// the messages that `me` has drained so far
 [[nodiscard]] std::uint64_t received( std::size_t me ) const {
  ADS_PRE( me < size() );
  return( f_worker[ me ]->received );
  }

 /// the channels underneath
 [[nodiscard]] Ch & channels( void ) { return( f_ch ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the state of a worker, which only the worker touches
 struct alignas( 128 ) Worker {
  std::vector< std::vector< Msg > > out;   ///< the outboxes
  std::vector< std::size_t > dirty;        ///< those not empty
  bool active = true;                      ///< holds an activity of C
  std::uint64_t sent = 0;                  ///< the messages pushed
  std::uint64_t received = 0;              ///< the messages drained
  };

 /// the shared counter, on lines of its own
 struct alignas( 128 ) Counter {
  std::atomic< std::int64_t > value{ 0 };   ///< activities and messages
  };

 /// the outbox of `me` for `to` pushed, as many messages as the channel
 /// takes
 void push( std::size_t me , std::size_t to ) {
  Worker & w = *f_worker[ me ];
  auto & o = w.out[ to ];
  const std::size_t k = o.size();
  if( k == 0 )
   return;
  f_count.value.fetch_add( std::int64_t( k ) , std::memory_order_acq_rel );
  const std::size_t j = f_ch.tryPush( me , to ,
                                      std::span< const Msg >( o ) );
  if( j < k )
   f_count.value.fetch_sub( std::int64_t( k - j ) ,
                            std::memory_order_acq_rel );
  w.sent += j;
  o.erase( o.begin() , o.begin() + std::ptrdiff_t( j ) );
  }

 Ch f_ch;                                  ///< the channels
 std::vector< std::unique_ptr< Worker > > f_worker;   ///< one per worker
 std::size_t f_batch;                      ///< the messages of a batch
 Counter f_count;                          ///< the shared counter
 };  // end( class Mailbox )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Mailbox.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Mailbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
