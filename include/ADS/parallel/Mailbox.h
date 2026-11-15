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
 * No operation waits for another worker. A bounded family (the mesh) may
 * take only part of a batch, or none of it, when the queue of the pair is
 * full: the rest stays in the outbox, `send()` tries it again only when a
 * batch more has come for that destination (so that a full queue costs a
 * push per batch and not per message), and `flush()` tries all the outboxes
 * and returns false while something is left in them; `sent()` counts the
 * messages pushed and `batches()` the pushes that the channels took. A
 * worker whose `quiescent()` returns false must keep polling its inbox,
 * which empties the queues that the others are waiting to push into.
 *
 * `stop()`, from any thread, ends the computation at once, whatever the
 * messages on their way (a limit on the work, or a bound that closes the
 * search): from then on `quiescent()` and `terminated()` are true for every
 * worker, `poll()` drains nothing, `send()` only appends to the outbox and
 * `flush()` pushes nothing. Once the workers have returned from `run()`,
 * `leftovers( me , f )` hands to \f$ f \f$ the messages left to `me`, those
 * of its outboxes and those of its inbox, and `leftovers( f )` those of
 * all; every message sent is then handed to a function exactly once, by a
 * `poll()` or by `leftovers()`, also when `stop()` races with the sends
 * (see \ref theory_parallel_quiescence), so that the nodes of a search
 * stopped by a limit can go back to its frontier, where they bound the
 * optimum.
 *
 * A mailbox serves a sequence of computations separated by barriers, e.g.,
 * the phases of an episode of an incremental search, or its episodes:
 * `rearm()` makes it ready for the next one once the last has ended (or
 * has been stopped and its messages recovered by `leftovers()`), all the
 * workers active again, or those that `active( me )` tells, so that the
 * channels and the outboxes, \f$ p^2 \f$ queues in the mesh, are made
 * once. The barrier is the caller's: `rearm()` runs in one thread while no
 * worker calls the mailbox, after every worker has returned from its last
 * call and before any makes its first of the next computation (in the
 * completion function of a `std::barrier`, or between two `run()` of the
 * executor), which orders its writes with the calls of all the workers.
 * The counts of `sent()`, `batches()`, `merged()` and `received()` go on
 * across the computations.
 *
 * A mailbox of the policy `CoalesceBy< K , M >` (the third parameter,
 * `NoCoalescing` by default, which adds no member and no code) merges in
 * the outbox the messages that carry the same key: `send( me , to , x ,
 * merge )` looks up the key `K()( x )` in a map `M` of the worker for the
 * destination, from the key to the place in the outbox of its last message
 * sent so, and if there is one calls `merge( pending , x )`, which merges
 * \f$ x \f$ into it (e.g., keeps the least of two distances) and returns
 * true, or returns false and leaves both, \f$ x \f$ then becoming the last
 * of its key; a merge with no result always merges. One probe of the map
 * per send, which is emptied at each push that the channels take some of
 * the outbox from, so that no place in it outlives a shift of the outbox.
 * A merge removes a message before it is counted, so that the detection of
 * the termination, the stop and `leftovers()` stay as they are, with each
 * message sent either handed once or merged into one that is (see \ref
 * theory_parallel_quiescence); `merged()` counts the messages merged. A
 * message sent by `send( me , to , x )` is unknown to the map, so that a
 * later merge may move a message of its key ahead of it: a message whose
 * order with those of its key matters is sent with a merge that refuses
 * it (e.g., an invalidation between two relaxations of a search).
 *
 * At the level of the audits of `Contract.h`, `audit( pred )`, called by
 * one thread where `rearm()` may be called (the computation ended, or
 * stopped and its messages recovered by `leftovers()`, and no worker
 * calling the mailbox), checks the predicate of the consumer on the state
 * that the workers have left, `pred()` once or `pred( me )` for each worker,
 * e.g., the comparison of an incremental state with its recomputation from
 * scratch by `Shadow` of `Modification.h`; a predicate false is a violated
 * audit. The barrier orders every write of the workers before the call,
 * so that the predicate reads the state of all of them exactly; below the
 * level of the audits the call costs nothing.
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
 * from the messages, or it was there from the start), or after `stop()`;
 * `leftovers()` is called when no worker calls the mailbox any more; `f`
 * does not throw; a mailbox serves one computation between two calls of
 * `rearm()`, which needs the last one ended or stopped, the outboxes
 * empty and, after a stop, the inboxes drained by `leftovers()` (an audit,
 * which drains them), and so does `audit()`; a merge does not throw, and
 * changes neither the key of the message pending nor the mailbox.
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
#include "ADS/Portable.h"
#include "ADS/parallel/Channels.h"

#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

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
/*--------------------- THE POLICIES OF THE COALESCING ---------------------*/
/*--------------------------------------------------------------------------*/

/// no coalescing of the messages in the outboxes of `Mailbox`: no member
/// and no code, and no `send()` with a merge; the default
struct NoCoalescing
{ };

/// the coalescing of the messages of one key in an outbox of `Mailbox`:
/// `K()( x )` the key of the message \f$ x \f$, and `M` the map from a key
/// to the place of its last message in an outbox (`try_emplace()`,
/// `clear()` and `empty()` as those of `std::unordered_map`, or of
/// `DenseHashMap` and `FlatHashTable`, which allocate no node per key), by
/// default (`void`) a `std::unordered_map` of the keys to `std::size_t`
template< class K , class M = void >
struct CoalesceBy
{
 using KeyOf = K;  ///< the function of the key
 using Map = M;    ///< the map, or void for the default
 };

namespace detail
{

/// the function of the key and the map of the policy C, if it merges
template< class C >
struct CoalescePolicy
{
 static constexpr bool on = false;
 };

/// those of CoalesceBy< K , M >
template< class K , class M >
struct CoalescePolicy< CoalesceBy< K , M > >
{
 static constexpr bool on = true;
 using KeyOf = K;
 };

/// the coalescing of the policy C for the messages Msg: none
template< class C , class Msg >
struct Coalescing
{
 static constexpr bool on = false;
 struct Slots
 { };  ///< no map
 };

/// the coalescing of CoalesceBy< K , M >: the key, the map, a map per
/// outbox
template< class K , class M , class Msg >
struct Coalescing< CoalesceBy< K , M > , Msg >
{
 static constexpr bool on = true;
 using KeyOf = K;
 using Key =
   std::remove_cvref_t< std::invoke_result_t< const K & , const Msg & > >;
 using Map = std::conditional_t< std::is_void_v< M > ,
				 std::unordered_map< Key , std::size_t > , M >;
 using Slots = std::vector< Map >;
 };

/// the value that `try_emplace()` of a map gives: the second member of
/// the iterator of a map of the standard, or the value a pointer of a flat
/// map of ADS points to [see `DenseHashMap`, `FlatHashTable`]
template< class I >
[[nodiscard]] constexpr auto & mappedOf( const I & it )
{
 if constexpr( std::is_pointer_v< I > )
  return( *it );
 else
  return( it->second );
 }

}  // end( namespace detail )

/// a policy of the coalescing of `Mailbox` for the messages `Msg`:
/// `NoCoalescing`, or `CoalesceBy< K , M >` with `K` a function object made
/// by default that gives the key of a message, and a map from the key to a
/// place of `std::size_t` by `try_emplace()`, `clear()` and `empty()`
template< class C , class Msg >
concept MailboxCoalescing =
  std::same_as< C , NoCoalescing > ||
  ( detail::CoalescePolicy< C >::on &&
    std::default_initializable<
      typename detail::CoalescePolicy< C >::KeyOf > &&
    std::invocable< const typename detail::CoalescePolicy< C >::KeyOf & ,
		    const Msg & > &&
    requires( typename detail::Coalescing< C , Msg >::Map & m ,
	      const typename detail::Coalescing< C , Msg >::Key & k ,
	      std::size_t i )
{
     {
      detail::mappedOf( m.try_emplace( k , i ).first )
      } -> std::same_as< std::size_t & >;
     { m.try_emplace( k , i ).second } -> std::convertible_to< bool >;
     m.clear();
     { m.empty() } -> std::convertible_to< bool >;
     } );

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Mailbox ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the mailbox of \f$ p \f$ workers on the channels `Ch` (by default the
/// mesh, the family that `DefaultChannels` gives every executor), with
/// outboxes of batches, the detection of the termination, the stop and the
/// recovery of the messages left, and the coalescing of the messages of one
/// key in an outbox by the policy `Co`
template< class Msg , ChannelFamily Ch = MeshChannels< Msg > ,
	  class Co = NoCoalescing >
class Mailbox
{

 static_assert( MailboxCoalescing< Co , Msg > ,
		"the policy of the coalescing is NoCoalescing or a "
		"CoalesceBy with a key and a map" );

 /// the coalescing of the policy
 using Coalesce = detail::Coalescing< Co , Msg >;

 /// whether the policy merges
 static constexpr bool coalescing = Coalesce::on;

 /// whether \f$ f \f$ merges a message into one pending
 template< class F >
 static constexpr bool merges = std::invocable< F & , Msg & , const Msg & >;

/*--------------------------------------------------------------------------*/
 public:
 using Message = Msg;    ///< the type of the messages
 using Channels = Ch;    ///< the family of the channels
 using Coalescing = Co;  ///< the policy of the coalescing

 /// the mailbox of \f$ p \f$ workers, all active, pushing batches of
 /// `batch` messages, on channels built from \f$ p \f$ and `args`
 /// \pre \f$ p \ge 1 \f$, \f$ \mathit{batch} \ge 1 \f$
 template< class... A >
 explicit Mailbox( std::size_t p , std::size_t batch = 64 , A &&... args )
     : f_ch( p , std::forward< A >( args )... ) , f_worker( p ) ,
       f_batch( batch ) {
  ADS_PRE( ( p >= 1 ) && ( batch >= 1 ) );
  for( auto & w : f_worker ) {
   w = std::make_unique< Worker >();
   w->out.resize( p );
   w->next.assign( p , batch );
   w->listed.assign( p , 0 );
   if constexpr( coalescing )
    w->slots.resize( p );
   }
  f_count.value.store( std::int64_t( p ) , std::memory_order_relaxed );
  }

 /// the number \f$ p \f$ of the workers
 [[nodiscard]] std::size_t size( void ) const { return( f_worker.size() ); }

 /// \f$ x \f$ appended to the outbox of `me` for `to`, which is pushed when
 /// it holds a batch more than a push left in it; never waits, and after
 /// `stop()` only appends
 ///
 /// In pseudocode (beyond the book), \f$ O_{\mathit{to}} \f$ the outbox and
 /// \f$ N_{\mathit{to}} \f$ its threshold, at first \f$ \mathit{batch} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{send}( \mathit{me} , \mathit{to} , x )
 ///     \cr
 /// 2 & \quad \text{append } x \text{ to } O_{\mathit{to}} \cr
 /// 3 & \quad \textbf{if}\ |O_{\mathit{to}}| \ge N_{\mathit{to}}
 ///     \ \textbf{and not stopped then}\ \mathrm{push}( \mathit{me} ,
 ///     \mathit{to} )
 /// \end{array}}
 /// \f]
 /// \pre `me` and `to` are workers, `me` is active or the mailbox stopped
 void send( std::size_t me , std::size_t to , Msg x ) {
  ADS_PRE( ( me < size() ) && ( to < size() ) );
  Worker & w = *f_worker[ me ];
  ADS_PRE( w.active || stopped() );
  auto & o = w.out[ to ];
  if( ! w.listed[ to ] ) {
   w.listed[ to ] = 1;
   w.dirty.push_back( to );
   }
  o.push_back( std::move( x ) );
  if( ( o.size() >= w.next[ to ] ) && ! stopped() )
   push( me , to );
  }

 /// `send( me , to , x )` with the coalescing of the policy: \f$ x \f$
 /// merged into the last message of its key in the outbox sent with a
 /// merge since the last push, if there is one and `merge( pending , x )`
 /// returns true (or nothing), appended otherwise; one probe of the map of
 /// the outbox
 ///
 /// In pseudocode (beyond the book), \f$ S_{\mathit{to}} \f$ the map of the
 /// outbox, from a key to a place in it, and \f$ \kappa \f$ the key:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{send}( \mathit{me} , \mathit{to} , x ,
 ///     \mathit{merge} ) \cr
 /// 2 & \quad \text{append } x \text{ to } O_{\mathit{to}};\ i \gets
 ///     |O_{\mathit{to}}| \cr
 /// 3 & \quad \textbf{if}\ S_{\mathit{to}}[ \kappa( x ) ] = j
 ///     \ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ \mathit{merge}( O_{\mathit{to}}[ j ] , x )
 ///     \ \textbf{then}\ \text{remove } x \text{ from } O_{\mathit{to}};
 ///     \ \mathit{merged} \gets \mathit{merged} + 1;\ \textbf{return} \cr
 /// 5 & \quad S_{\mathit{to}}[ \kappa( x ) ] \gets i\quad
 ///     \triangleright\ \text{one probe for lines 3 and 5} \cr
 /// 6 & \quad \textbf{if}\ |O_{\mathit{to}}| \ge N_{\mathit{to}}
 ///     \ \textbf{and not stopped then}\ \mathrm{push}( \mathit{me} ,
 ///     \mathit{to} )
 /// \end{array}}
 /// \f]
 /// and \f$ \mathrm{push} \f$ empties \f$ S_{\mathit{to}} \f$ when the
 /// channel takes a message.
 /// \pre as `send( me , to , x )`; `merge` does not throw and does not
 /// change the key of the message pending
 template< class F >
  requires( coalescing && merges< F > )
 void send( std::size_t me , std::size_t to , Msg x , F && merge ) {
  ADS_PRE( ( me < size() ) && ( to < size() ) );
  Worker & w = *f_worker[ me ];
  ADS_PRE( w.active || stopped() );
  auto & o = w.out[ to ];
  if( ! w.listed[ to ] ) {
   w.listed[ to ] = 1;
   w.dirty.push_back( to );
   }
  o.push_back( std::move( x ) );
  const std::size_t i = o.size() - 1;
  auto [ it , fresh ] =
    w.slots[ to ].try_emplace( typename Coalesce::KeyOf()( o.back() ) , i );
  if( ! fresh ) {
   ADS_INVARIANT( detail::mappedOf( it ) < i );
   if( absorb( merge , o[ detail::mappedOf( it ) ] , o.back() ) ) {
    o.pop_back();  // never counted, never pushed
    ++w.merged;
    return;
    }
   detail::mappedOf( it ) = i;
   }
  if( ( o.size() >= w.next[ to ] ) && ! stopped() )
   push( me , to );
  }

 /// the outboxes of `me` pushed, true if all of them are empty after it:
 /// a bounded family may refuse some messages, which stay in their outbox
 /// for a later push, and the call never waits; after `stop()` nothing is
 /// pushed
 ///
 /// In pseudocode (beyond the book), \f$ C \f$ the shared counter:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{flush}( \mathit{me} ) \cr
 /// 2 & \quad \textbf{if}\ \textbf{not}\ \text{stopped}\ \textbf{then}
 ///     \ \textbf{foreach}\ \mathit{to} \text{ with } O_{\mathit{to}}
 ///     \ne \emptyset\ \textbf{do}\ \mathrm{push}( \mathit{me} ,
 ///     \mathit{to} ) \cr
 /// 3 & \quad \textbf{return}\ \text{all the outboxes are empty} \cr
 /// 4 & \textbf{procedure}\ \mathrm{push}( \mathit{me} , \mathit{to} ) \cr
 /// 5 & \quad k \gets |O_{\mathit{to}}|;\ C \gets C + k\quad
 ///     \triangleright\ \text{before the messages can be drained} \cr
 /// 6 & \quad j \gets \mathrm{tryPush}( \mathit{me} , \mathit{to} ,
 ///     O_{\mathit{to}} );\ C \gets C - ( k - j ) \cr
 /// 7 & \quad \text{remove the first } j \text{ messages of }
 ///     O_{\mathit{to}};\ N_{\mathit{to}} \gets |O_{\mathit{to}}| +
 ///     \mathit{batch}
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 bool flush( std::size_t me ) {
  ADS_PRE( me < size() );
  Worker & w = *f_worker[ me ];
  const bool go = ! stopped();
  std::size_t keep = 0;
  for( std::size_t i = 0 ; i < w.dirty.size() ; ++i ) {
   const std::size_t to = w.dirty[ i ];
   if( go )
    push( me , to );
   if( w.out[ to ].empty() )
    w.listed[ to ] = 0;
   else
    w.dirty[ keep++ ] = to;
   }
  w.dirty.resize( keep );
  return( keep == 0 );
  }

 /// the calls `f( x )` on the messages of the inbox of `me`, and their
 /// number; a passive worker that finds messages becomes active; after
 /// `stop()` nothing is drained, and the messages stay for `leftovers()`
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{poll}( \mathit{me} , f ) \cr
 /// 2 & \quad \textbf{if}\ \text{stopped}\ \textbf{then return}\ 0 \cr
 /// 3 & \quad k \gets \mathrm{drain}( \mathit{me} , x \mapsto \textbf{if}
 ///     \ \textbf{not}\ \mathit{active}\ \textbf{then}\ C \gets C + 1;
 ///     \ \mathit{active} \gets \textbf{true}; \cr
 /// & \qquad f( x ) ) \cr
 /// 4 & \quad \textbf{if}\ k > 0\ \textbf{then}\ C \gets C - k \cr
 /// 5 & \quad \textbf{return}\ k
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 template< class F >
 std::size_t poll( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  if( stopped() )
   return( 0 );
  Worker & w = *f_worker[ me ];
  const std::size_t k = f_ch.drain( me , [ & ]( Msg && x ) {
   if( ! w.active ) {  // back to work, counted
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

 /// true if the computation has ended or the mailbox has stopped; called
 /// by `me` with no work left, it first pushes its outboxes and, if they
 /// are all empty, becomes passive; while it returns false, `me` must keep
 /// polling its inbox, which lets the others push what they retain
 ///
 /// In pseudocode (the deficit counting of Dijkstra and Scholten on a
 /// shared counter, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{quiescent}( \mathit{me} ) \cr
 /// 2 & \quad \textbf{if}\ \text{stopped}\ \textbf{then return true} \cr
 /// 3 & \quad \textbf{if}\ \mathit{active}\ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ \textbf{not}\ \mathrm{flush}( \mathit{me} )
 ///     \ \textbf{then}\ \textbf{return false} \cr
 /// 5 & \qquad \mathit{active} \gets \textbf{false};\ C \gets C - 1 \cr
 /// 6 & \quad \textbf{return}\ C = 0
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker
 bool quiescent( std::size_t me ) {
  ADS_PRE( me < size() );
  if( stopped() )
   return( true );
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

 /// true once the computation has ended or the mailbox has stopped (any
 /// thread may ask)
 [[nodiscard]] bool terminated( void ) const {
  return( stopped() ||
	   ( f_count.value.load( std::memory_order_acquire ) == 0 ) );
  }

 /// ends the computation at once, whatever the messages on their way: from
 /// then on `quiescent()` and `terminated()` are true for every worker,
 /// `poll()` drains nothing and `send()` and `flush()` push nothing, so
 /// that each message sent is either handed to \f$ f \f$ by a `poll()` or
 /// left for `leftovers()`, once; any thread may call it, any number of
 /// times (see \ref theory_parallel_quiescence)
 void stop( void ) { f_stop.value.store( true , std::memory_order_release ); }

 /// true once `stop()` has been called
 [[nodiscard]] bool stopped( void ) const {
  return( f_stop.value.load( std::memory_order_acquire ) );
  }

 /// the mailbox ready for the next computation, with the workers for
 /// which `active( me )` is true active and the others passive: the counter
 /// set to the number of the active ones, the stop withdrawn, the channels
 /// and the outboxes kept; called by one thread at a barrier, while no
 /// worker calls the mailbox
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rearm}( \mathit{active} ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{worker}\ \mathit{me}\ \textbf{do}
 ///     \ \mathit{active}_{\mathit{me}} \gets \mathit{active}(
 ///     \mathit{me} ) \cr
 /// 3 & \quad C \gets |\{ \mathit{me} : \mathit{active}_{\mathit{me}}
 ///     \}|;\ \text{stopped} \gets \textbf{false}
 /// \end{array}}
 /// \f]
 /// \pre the outboxes are empty, and the computation has ended (\f$ C = 0
 /// \f$) or the mailbox has stopped and `leftovers()` has emptied it
 /// (audit); no worker calls the mailbox meanwhile
 template< class F >
  requires std::predicate< F & , std::size_t >
 void rearm( F && active ) {
  ADS_PRE( rearmable() );
  ADS_AUDIT( drained() );
  std::int64_t n = 0;
  for( std::size_t me = 0 ; me < size() ; ++me ) {
   Worker & w = *f_worker[ me ];
   w.active = bool( active( me ) );
   n += w.active;
   }
  f_count.value.store( n , std::memory_order_release );
  f_stop.value.store( false , std::memory_order_release );
  }

 /// `rearm()` above with all the workers active, as at the construction
 /// \pre as `rearm( active )`
 void rearm( void ) {
  rearm( []( std::size_t ) { return( true ); } );
  }

 /// at the level of the audits, the predicate of the consumer checked at a
 /// quiescent point: `pred( me )` for each worker, if it takes one, or
 /// `pred()`, a violated audit if it is false; called by one thread at a
 /// barrier, while no worker calls the mailbox, under the precondition of
 /// `rearm()`, so that the predicate reads the state that the workers left
 /// with no message on its way; nothing below the level of the audits
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{audit}( \mathit{pred} ) \cr
 /// 2 & \quad \textbf{assert}\ \text{the outboxes and the inboxes are}
 ///     \text{ empty, and } C = 0 \text{ or stopped} \cr
 /// 3 & \quad \textbf{foreach}\ \text{worker}\ \mathit{me}
 ///     \ \textbf{do}\ \textbf{assert}\ \mathit{pred}( \mathit{me} )
 /// \end{array}}
 /// \f]
 /// \pre as `rearm()`, checked at the level of the audits
 template< class P >
  requires( std::predicate< P & , std::size_t > || std::predicate< P & > )
 void audit( [[maybe_unused]] P && pred ) {
  if constexpr( ADS_CONTRACTS >= 3 ) {
   ADS_AUDIT( rearmable() );
   ADS_AUDIT( drained() );
   if constexpr( std::predicate< P & , std::size_t > ) {
    for( std::size_t me = 0 ; me < size() ; ++me )
     ADS_AUDIT( pred( me ) );
    }
   else
    ADS_AUDIT( pred() );
   }
  }

 /// the calls `f( x )` on the messages left to `me`: those of its outboxes,
 /// for each destination in the order of their sends, then those of its
 /// inbox; their number. Called after the workers have returned from
 /// `run()` (or have all stopped calling the mailbox), by any thread, it
 /// empties both, so that with `stop()` every message sent has been handed
 /// to \f$ f \f$ exactly once, by a `poll()` or here; after a termination
 /// detected by `quiescent()` there is none
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{leftovers}( \mathit{me} , f ) \cr
 /// 2 & \quad \textbf{foreach}\ \mathit{to} \text{ with } O_{\mathit{to}}
 ///     \ne \emptyset\ \textbf{do}\ \textbf{foreach}\ x \in O_{\mathit{to}}
 ///     \ \textbf{do}\ f( x );\ O_{\mathit{to}} \gets \emptyset \cr
 /// 3 & \quad \textbf{return}\ \text{those and}\ \mathrm{drain}(
 ///     \mathit{me} , f )
 /// \end{array}}
 /// \f]
 /// \pre `me` is a worker; no worker calls the mailbox meanwhile
 template< class F >
 std::size_t leftovers( std::size_t me , F && f ) {
  ADS_PRE( me < size() );
  Worker & w = *f_worker[ me ];
  std::size_t n = 0;
  for( const std::size_t to : w.dirty ) {
   auto & o = w.out[ to ];
   n += o.size();
   for( auto & x : o )
    f( std::move( x ) );
   o.clear();
   if constexpr( coalescing )
    w.slots[ to ].clear();
   w.listed[ to ] = 0;
   w.next[ to ] = f_batch;
   }
  w.dirty.clear();
  return( n + f_ch.drain( me , [ & ]( Msg && x ) { f( std::move( x ) ); } ) );
  }

 /// `leftovers( me , f )` of every worker, in the order of the workers,
 /// and the number of all the messages
 /// \pre no worker calls the mailbox meanwhile
 template< class F >
 std::size_t leftovers( F && f ) {
  std::size_t n = 0;
  for( std::size_t me = 0 ; me < size() ; ++me )
   n += leftovers( me , f );
  return( n );
  }

 /// the messages that `me` has pushed so far
 [[nodiscard]] std::uint64_t sent( std::size_t me ) const {
  ADS_PRE( me < size() );
  return( f_worker[ me ]->sent );
  }

 /// the batches that `me` has pushed so far, i.e., the pushes that the
 /// channels took at least one message of
 [[nodiscard]] std::uint64_t batches( std::size_t me ) const {
  ADS_PRE( me < size() );
  return( f_worker[ me ]->batches );
  }

 /// the messages that `me` has merged so far into one pending, by
 /// `send()` with a merge; 0 with `NoCoalescing`
 [[nodiscard]] std::uint64_t merged( std::size_t me ) const {
  ADS_PRE( me < size() );
  return( f_worker[ me ]->merged );
  }

 /// the messages that `me` has drained so far by `poll()`
 [[nodiscard]] std::uint64_t received( std::size_t me ) const {
  ADS_PRE( me < size() );
  return( f_worker[ me ]->received );
  }

 /// the channels underneath
 [[nodiscard]] Ch & channels( void ) { return( f_ch ); }

/*--------------------------------------------------------------------------*/
 private:
 /// the state of a worker, which only the worker touches
 struct alignas( 128 ) Worker
 {
  std::vector< std::vector< Msg > > out;  ///< the outboxes
  std::vector< std::size_t > next;        ///< the size of the next push
  std::vector< char > listed;             ///< whether in dirty
  std::vector< std::size_t > dirty;       ///< those that may not be empty
  bool active = true;                     ///< holds an activity of C
  std::uint64_t sent = 0;                 ///< the messages pushed
  std::uint64_t batches = 0;              ///< the pushes taken
  std::uint64_t received = 0;             ///< the messages drained
  std::uint64_t merged = 0;               ///< the messages merged
  /// the maps of the outboxes, from a key to its place; nothing with
  /// NoCoalescing
  ADS_NO_UNIQUE_ADDRESS typename Coalesce::Slots slots{};
  };

 /// the shared counter, on lines of its own
 struct alignas( 128 ) Counter
 {
  std::atomic< std::int64_t > value{ 0 };  ///< activities and messages
  };

 /// the flag of the stop, on lines of its own, read at each send
 struct alignas( 128 ) Flag
 {
  std::atomic< bool > value{ false };  ///< set by stop()
  };

 /// whether the computation has ended or stopped and no outbox holds a
 /// message, \f$ O( p ) \f$
 [[nodiscard]] bool rearmable( void ) const {
  for( const auto & w : f_worker )
   for( const std::size_t to : w->dirty )
    if( ! w->out[ to ].empty() )
     return( false );
  return( stopped() ||
	   ( f_count.value.load( std::memory_order_acquire ) == 0 ) );
  }

 /// whether no inbox holds a message; drains them, so that it is only
 /// asked where they must be empty
 [[nodiscard]] bool drained( void ) {
  std::size_t n = 0;
  for( std::size_t me = 0 ; me < size() ; ++me )
   n += f_ch.drain( me , []( Msg && ) { } );
  return( n == 0 );
  }

 /// `merge( p , x )`, true if it merged \f$ x \f$ into \f$ p \f$: its
 /// result, or true if it has none
 template< class F >
 static bool absorb( F & merge , Msg & p , const Msg & x ) {
  if constexpr( std::is_void_v<
		  std::invoke_result_t< F & , Msg & , const Msg & > > ) {
   merge( p , x );
   return( true );
   }
  else
   return( bool( merge( p , x ) ) );
  }

 /// the outbox of `me` for `to` pushed, as many messages as the channel
 /// takes; the next push when a batch more has come
 void push( std::size_t me , std::size_t to ) {
  Worker & w = *f_worker[ me ];
  auto & o = w.out[ to ];
  const std::size_t k = o.size();
  if( k == 0 )
   return;
  f_count.value.fetch_add( std::int64_t( k ) , std::memory_order_acq_rel );
  const std::size_t j = f_ch.tryPush( me , to , std::span< const Msg >( o ) );
  if( j < k )
   f_count.value.fetch_sub( std::int64_t( k - j ) ,
			    std::memory_order_acq_rel );
  if( j > 0 ) {
   w.sent += j;
   ++w.batches;
   o.erase( o.begin() , o.begin() + std::ptrdiff_t( j ) );
   if constexpr( coalescing )  // the places have shifted
    if( ! w.slots[ to ].empty() )
     w.slots[ to ].clear();
   }
  w.next[ to ] = o.size() + f_batch;
  }

 Ch f_ch;                                            ///< the channels
 std::vector< std::unique_ptr< Worker > > f_worker;  ///< one per worker
 std::size_t f_batch;  ///< the messages of a batch
 Counter f_count;      ///< the shared counter
 Flag f_stop;          ///< the stop
 };  // end( class Mailbox )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* Mailbox.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Mailbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
