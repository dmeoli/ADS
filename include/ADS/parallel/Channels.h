/*--------------------------------------------------------------------------*/
/*---------------------------- File Channels.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The channels between the long-lived workers of an executor [see
 * `Executor.h`], beyond the book: \f$ p \f$ workers, each with an inbox,
 * where any worker pushes a batch of messages for another one and the owner
 * of the inbox drains it. All the families of channels have one interface,
 * the concept `ChannelFamily`: `tryPush( from , to , m )` pushes the
 * messages of the span \f$ m \f$ from the worker `from` to the inbox of the
 * worker `to` and returns how many it pushed (all of them but in a bounded
 * family, which pushes those that fit), `drain( me , f )` calls `f( x )` on
 * each message \f$ x \f$ found in the inbox of `me`, moved, and returns how
 * many, and `port( i )` binds both to the worker \f$ i \f$. The messages of
 * a worker to another come out in the order in which it pushed them; those
 * of different senders interleave in any order. The families differ in what
 * an inbox is (see \ref theory_parallel_channels):
 *
 * - `LockedChannels`, the portable baseline: an inbox is a vector under a
 *   lock, by default the lock of the executor (`lock_type`), which a push
 *   appends to and a drain swaps with an empty one, so that each takes the
 *   lock once per batch;
 * - `InboxChannels`: an inbox is an `MPSCInbox` [see `MPSCInbox.h`], a stack
 *   of Treiber on which a push puts the batch as one cell, by one
 *   compare-and-swap, and that a drain takes whole by one exchange;
 * - `LaneChannels`: an inbox is an `MPSCQueue` [see `MPSCQueue.h`], with a
 *   lane of blocks for each sender, so that the senders never meet;
 * - `MeshChannels`: the \f$ p^2 \f$ queues `SPSCQueue` [see `SPSCQueue.h`]
 *   of each ordered pair of workers, the mesh of the farms and of the
 *   all-to-all of FastFlow, bounded, a batch pushed by one store of a tail
 *   and a queue drained by one store of a head, the drain scanning the
 *   \f$ p \f$ queues of its worker.
 *
 * `DefaultChannels< Msg , E >` is the family to use on the executor `E`,
 * chosen by the measures of `bench/runtime.cpp` (see
 * \ref theory_parallel_channels): the mesh, on every executor measured.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$ workers, the indices of
 * the workers in \f$ [ 0 , p ) \f$; only the worker `from` pushes as
 * `from`, and only the worker `me` drains its inbox; `f` does not throw.
 * The messages are movable, and default constructible in `LaneChannels`.
 *
 * \see R. K. Treiber, Systems programming: coping with parallelism,
 *      Technical Report RJ 5118, IBM Almaden Research Center, 1986
 * \see M. Aldinucci, M. Danelutto, P. Kilpatrick, M. Meneghin, M.
 *      Torquati, An efficient unbounded lock-free queue for multi-core
 *      systems, Euro-Par 2012, LNCS 7484, 662-673, 2012,
 *      doi:10.1007/978-3-642-32820-6_65
 * \see M. Aldinucci, M. Danelutto, P. Kilpatrick, M. Torquati, FastFlow:
 *      high-level and efficient streaming on multicore, in S. Pllana,
 *      F. Xhafa (eds.), Programming Multi-core and Many-core Computing
 *      Systems, Wiley, 261-280, 2017, doi:10.1002/9781119332015.ch13
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

#ifndef __ADS_Channels
 #define __ADS_Channels
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/concurrent/MPSCInbox.h"
#include "ADS/concurrent/MPSCQueue.h"
#include "ADS/concurrent/SPSCQueue.h"
#include "ADS/parallel/Executor.h"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <memory>
#include <mutex>
#include <span>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the families of channels among \f$ p \f$ workers: `tryPush( from , to ,
/// m )` pushes a span of messages and returns how many, `drain( me , f )`
/// calls \f$ f \f$ on the messages of the inbox of `me` and returns how
/// many, `size()` is \f$ p \f$
template< class C >
concept ChannelFamily = requires( C & c , const C & k , std::size_t i ,
                                  std::span< const typename C::Message > m ,
                                  void ( & f )( typename C::Message && ) ) {
 { k.size() } -> std::convertible_to< std::size_t >;
 { c.tryPush( i , i , m ) } -> std::convertible_to< std::size_t >;
 { c.drain( i , f ) } -> std::convertible_to< std::size_t >;
 };

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS ChannelPort ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the channels of a family seen by one worker
template< class C >
class ChannelPort {
 public:

 /// the port of the worker \f$ i \f$ on the channels \f$ c \f$
 ChannelPort( C & c , std::size_t i ) : f_c( &c ) , f_me( i ) {}

 /// the messages of \f$ m \f$ pushed to the inbox of `to`, as many as fit
 std::size_t tryPush( std::size_t to ,
                      std::span< const typename C::Message > m ) {
  return( f_c->tryPush( f_me , to , m ) );
  }

 /// the calls `f( x )` on the messages of the inbox of the worker
 template< class F >
 std::size_t drain( F && f ) {
  return( f_c->drain( f_me , std::forward< F >( f ) ) );
  }

 /// the worker of the port
 [[nodiscard]] std::size_t index( void ) const { return( f_me ); }

 private:

 C * f_c;                                  ///< the channels
 std::size_t f_me;                         ///< the worker
 };  // end( class ChannelPort )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS LockedChannels --------------------------*/
/*--------------------------------------------------------------------------*/

/// the inboxes as vectors under a lock, the portable baseline
template< class Msg , class Lock = std::mutex >
class LockedChannels {

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages
 /// whether a push may push fewer messages than given
 static constexpr bool bounded = false;

 /// the inboxes of \f$ p \f$ workers
 /// \pre \f$ p \ge 1 \f$
 explicit LockedChannels( std::size_t p ) : f_box( p ) {
  ADS_PRE( p >= 1 );
  for( auto & b : f_box )
   b = std::make_unique< Box >();
  }

 /// the number \f$ p \f$ of the workers
 [[nodiscard]] std::size_t size( void ) const { return( f_box.size() ); }

 /// the messages of \f$ m \f$ appended to the inbox of `to` under its
 /// lock, all of them
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPush}( \mathit{from} , \mathit{to} , m
 ///     ) \cr
 /// 2 & \quad \text{under the lock of } \mathit{to} \text{: append } m
 ///     \text{ to its vector } V_{\mathit{to}} \text{, and publish }
 ///     |V_{\mathit{to}}| \cr
 /// 3 & \quad \textbf{return}\ |m|
 /// \end{array}}
 /// \f]
 /// \pre `from` and `to` are workers
 std::size_t tryPush( std::size_t from , std::size_t to ,
                      std::span< const Msg > m ) {
  ADS_PRE( ( from < size() ) && ( to < size() ) );
  if( m.empty() )
   return( 0 );
  Box & b = *f_box[ to ];
  std::lock_guard< Lock > g( b.lock );
  b.items.insert( b.items.end() , m.begin() , m.end() );
  b.count.store( b.items.size() , std::memory_order_release );
  return( m.size() );
  }

 /// the calls `f( x )` on the messages of the inbox of `me`, in the order
 /// of their pushes, and their number
 ///
 /// In pseudocode (beyond the book), \f$ W \f$ an empty vector of `me`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{drain}( \mathit{me} , f ) \cr
 /// 2 & \quad \textbf{if}\ \text{the published size is } 0\ \textbf{then}
 ///     \ \textbf{return}\ 0\quad \triangleright\ \text{no lock taken} \cr
 /// 3 & \quad \text{under the lock of } \mathit{me} \text{: swap }
 ///     V_{\mathit{me}} \text{ and } W \text{, and publish } 0 \cr
 /// 4 & \quad \textbf{foreach}\ x \in W\ \textbf{do}\ f( x ) \cr
 /// 5 & \quad \textbf{return}\ |W| \text{, after emptying } W
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 template< class F >
 std::size_t drain( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  Box & b = *f_box[ me ];
  if( b.count.load( std::memory_order_acquire ) == 0 )
   return( 0 );
  {
   std::lock_guard< Lock > g( b.lock );
   std::swap( b.items , b.taken );
   b.count.store( 0 , std::memory_order_relaxed );
   }
  const std::size_t n = b.taken.size();
  for( auto & x : b.taken )
   f( std::move( x ) );
  b.taken.clear();
  return( n );
  }

 /// the channels seen by the worker \f$ i \f$
 [[nodiscard]] ChannelPort< LockedChannels > port( std::size_t i ) {
  return( ChannelPort< LockedChannels >( *this , i ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// an inbox, on lines of its own
 struct alignas( 128 ) Box {
  Lock lock;                               ///< the lock of the inbox
  std::vector< Msg > items;                ///< the messages, under it
  std::atomic< std::size_t > count{ 0 };   ///< their number, published
  std::vector< Msg > taken;                ///< the owner's, drained
  };

 std::vector< std::unique_ptr< Box > > f_box;   ///< one per worker
 };  // end( class LockedChannels )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS InboxChannels ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the inboxes as stacks of Treiber of batches [see `MPSCInbox.h`]
template< class Msg >
class InboxChannels {

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages
 /// whether a push may push fewer messages than given
 static constexpr bool bounded = false;

 /// the inboxes of \f$ p \f$ workers
 /// \pre \f$ p \ge 1 \f$
 explicit InboxChannels( std::size_t p ) : f_box( p ) {
  ADS_PRE( p >= 1 );
  for( auto & b : f_box )
   b = std::make_unique< Box >();
  }

 /// the number \f$ p \f$ of the workers
 [[nodiscard]] std::size_t size( void ) const { return( f_box.size() ); }

 /// the messages of \f$ m \f$ pushed to the inbox of `to` as one batch,
 /// all of them
 ///
 /// In pseudocode (the push of Treiber, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPush}( \mathit{from} , \mathit{to} , m
 ///     ) \cr
 /// 2 & \quad \mathrm{push}( I_{\mathit{to}} , \text{a copy of } m )\quad
 ///     \triangleright\ \text{one cell, one CAS} \cr
 /// 3 & \quad \textbf{return}\ |m|
 /// \end{array}}
 /// \f]
 /// \pre `from` and `to` are workers
 std::size_t tryPush( std::size_t from , std::size_t to ,
                      std::span< const Msg > m ) {
  ADS_PRE( ( from < size() ) && ( to < size() ) );
  if( m.empty() )
   return( 0 );
  f_box[ to ]->inbox.push( std::vector< Msg >( m.begin() , m.end() ) );
  return( m.size() );
  }

 /// the calls `f( x )` on the messages of the inbox of `me`, the batches
 /// of each sender in the order of their pushes, and their number
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{drain}( \mathit{me} , f ) \cr
 /// 2 & \quad L \gets \mathrm{take}( I_{\mathit{me}} )\quad \triangleright
 ///     \ \text{one exchange, the oldest batch first} \cr
 /// 3 & \quad \textbf{foreach}\ \text{batch } b \in L\ \textbf{do}
 ///     \ \textbf{foreach}\ x \in b\ \textbf{do}\ f( x ) \cr
 /// 4 & \quad \textbf{return}\ \text{the messages of } L
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 template< class F >
 std::size_t drain( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  auto & in = f_box[ me ]->inbox;
  if( in.empty() )
   return( 0 );
  std::size_t n = 0;
  for( auto & b : in.take() ) {
   n += b.size();
   for( auto & x : b )
    f( std::move( x ) );
   }
  return( n );
  }

 /// the channels seen by the worker \f$ i \f$
 [[nodiscard]] ChannelPort< InboxChannels > port( std::size_t i ) {
  return( ChannelPort< InboxChannels >( *this , i ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// an inbox, on lines of its own
 struct alignas( 128 ) Box {
  MPSCInbox< std::vector< Msg > > inbox;   ///< the batches
  };

 std::vector< std::unique_ptr< Box > > f_box;   ///< one per worker
 };  // end( class InboxChannels )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS LaneChannels ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the inboxes as queues with a lane per sender [see `MPSCQueue.h`]
template< class Msg , std::size_t B = 256 >
class LaneChannels {

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages
 /// whether a push may push fewer messages than given
 static constexpr bool bounded = false;

 /// the inboxes of \f$ p \f$ workers, with the \f$ p \f$ lanes of each
 /// \pre \f$ p \ge 1 \f$
 explicit LaneChannels( std::size_t p ) : f_box( p ) , f_lane( p * p ) {
  ADS_PRE( p >= 1 );
  for( auto & b : f_box )
   b = std::make_unique< Box >();
  for( std::size_t to = 0 ; to < p ; ++to )
   for( std::size_t from = 0 ; from < p ; ++from )
    f_lane[ from * p + to ] = f_box[ to ]->queue.producer();
  }

 /// the number \f$ p \f$ of the workers
 [[nodiscard]] std::size_t size( void ) const { return( f_box.size() ); }

 /// the messages of \f$ m \f$ pushed to the lane of `from` in the inbox of
 /// `to`, all of them
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPush}( \mathit{from} , \mathit{to} , m
 ///     ) \cr
 /// 2 & \quad \textbf{foreach}\ x \in m\ \textbf{do}\ \mathrm{emplace}(
 ///     \text{the lane of } \mathit{from} \text{ in } Q_{\mathit{to}} , x )
 ///     \cr
 /// 3 & \quad \textbf{return}\ |m|
 /// \end{array}}
 /// \f]
 /// \pre `from` and `to` are workers
 std::size_t tryPush( std::size_t from , std::size_t to ,
                      std::span< const Msg > m ) {
  ADS_PRE( ( from < size() ) && ( to < size() ) );
  auto & l = f_lane[ from * size() + to ];
  for( const auto & x : m )
   l.push( x );
  return( m.size() );
  }

 /// the calls `f( x )` on the messages of the inbox of `me`, lane by lane,
 /// and their number
 ///
 /// In pseudocode (beyond the book), \f$ W \f$ a buffer of `me`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{drain}( \mathit{me} , f ) \cr
 /// 2 & \quad n \gets 0 \cr
 /// 3 & \quad \textbf{repeat} \cr
 /// 4 & \qquad k \gets \mathrm{popInto}( Q_{\mathit{me}} , W , |W| );
 ///     \ n \gets n + k \cr
 /// 5 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
 ///     \ f( W[ j ] ) \cr
 /// 6 & \quad \textbf{until}\ k = 0 \cr
 /// 7 & \quad \textbf{return}\ n
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 template< class F >
 std::size_t drain( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  Box & b = *f_box[ me ];
  std::size_t n = 0;
  for( ;; ) {
   const std::size_t k = b.queue.popInto( b.buffer , B );
   if( k == 0 )
    return( n );
   n += k;
   for( std::size_t j = 0 ; j < k ; ++j )
    f( std::move( b.buffer[ j ] ) );
   }
  }

 /// the channels seen by the worker \f$ i \f$
 [[nodiscard]] ChannelPort< LaneChannels > port( std::size_t i ) {
  return( ChannelPort< LaneChannels >( *this , i ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// an inbox and the buffer of its drains, on lines of their own
 struct alignas( 128 ) Box {
  MPSCQueue< Msg , B > queue;              ///< the lanes
  Msg buffer[ B ];                         ///< the owner's, drained
  };

 std::vector< std::unique_ptr< Box > > f_box;   ///< one per worker
 /// the lane of each sender in each inbox, the sender first
 std::vector< typename MPSCQueue< Msg , B >::Producer > f_lane;
 };  // end( class LaneChannels )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MeshChannels ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the \f$ p^2 \f$ bounded queues of the pairs of workers [see
/// `SPSCQueue.h`]
template< class Msg >
class MeshChannels {

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages
 /// whether a push may push fewer messages than given
 static constexpr bool bounded = true;

 /// the \f$ p^2 \f$ queues of \f$ p \f$ workers, of capacity \f$ c \f$
 /// each
 /// \pre \f$ p \ge 1 \f$, \f$ c \ge 2 \f$ a power of 2
 explicit MeshChannels( std::size_t p , std::size_t c = 1024 )
  : f_q( p * p ) , f_start( p ) {
  ADS_PRE( p >= 1 );
  for( auto & q : f_q )
   q = std::make_unique< SPSCQueue< Msg > >( c );
  }

 /// the number \f$ p \f$ of the workers
 [[nodiscard]] std::size_t size( void ) const { return( f_start.size() ); }

 /// the first messages of \f$ m \f$ that fit in the queue from `from` to
 /// `to`, pushed by one store of its tail, and their number
 ///
 /// In pseudocode (beyond the book), \f$ Q_{\mathit{from} ,
 /// \mathit{to}} \f$ the queue of the pair:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{tryPush}( \mathit{from} , \mathit{to} , m
 ///     ) \cr
 /// 2 & \quad \textbf{return}\ \mathrm{pushMany}( Q_{\mathit{from} ,
 ///     \mathit{to}} , m )
 /// \end{array}}
 /// \f]
 /// \pre `from` and `to` are workers
 std::size_t tryPush( std::size_t from , std::size_t to ,
                      std::span< const Msg > m ) {
  ADS_PRE( ( from < size() ) && ( to < size() ) );
  if( m.empty() )
   return( 0 );
  return( f_q[ to * size() + from ]->pushMany( m.begin() , m.size() ) );
  }

 /// the calls `f( x )` on the messages of the \f$ p \f$ queues to `me`,
 /// from a sender that turns at each call, and their number
 ///
 /// In pseudocode (beyond the book), \f$ s \f$ the first sender of `me`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{drain}( \mathit{me} , f ) \cr
 /// 2 & \quad n \gets 0 \cr
 /// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ p - 1\ \textbf{do}
 ///     \ n \gets n + \mathrm{consume}( Q_{( s + j ) \bmod p , \mathit{me}}
 ///     , f ) \cr
 /// 4 & \quad s \gets ( s + 1 ) \bmod p;\ \textbf{return}\ n
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 template< class F >
 std::size_t drain( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  const std::size_t p = size();
  std::size_t & s = f_start[ me ].s;
  std::size_t n = 0;
  for( std::size_t j = 0 , from = s ; j < p ;
       ++j , from = ( from + 1 == p ? 0 : from + 1 ) )
   n += f_q[ me * p + from ]->consume( f );
  s = ( s + 1 == p ? 0 : s + 1 );
  return( n );
  }

 /// the channels seen by the worker \f$ i \f$
 [[nodiscard]] ChannelPort< MeshChannels > port( std::size_t i ) {
  return( ChannelPort< MeshChannels >( *this , i ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the first sender of the next drain of a worker, on a line of its own
 struct alignas( 64 ) Start {
  std::size_t s = 0;                       ///< the sender
  };

 /// the queue from each sender to each receiver, the receiver first
 std::vector< std::unique_ptr< SPSCQueue< Msg > > > f_q;
 std::vector< Start > f_start;             ///< one per worker
 };  // end( class MeshChannels )

/*--------------------------------------------------------------------------*/
/*--------------------------- DEFAULT CHANNELS -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the family of channels of an executor, as measured by
/// `bench/runtime.cpp` (see \ref theory_parallel_channels): the mesh on
/// every executor, which won the batches of 1 and of 64 messages and the
/// round trips on the pool, OpenMP and FastFlow, with the trees a tie; a
/// specialization may give an executor a family of its own
template< class E >
struct ChannelsOf {
 /// the family for the messages `Msg`
 template< class Msg >
 using type = MeshChannels< Msg >;
 };

/// the family of channels to use on the executor `E`
template< class Msg , class E >
using DefaultChannels = typename ChannelsOf< E >::template type< Msg >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Channels.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Channels.h ----------------------------*/
/*--------------------------------------------------------------------------*/
