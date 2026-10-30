/*--------------------------------------------------------------------------*/
/*--------------------------- File MPIMailbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The mailbox of the processes of MPI, beyond the book, compiled only with
 * `ADS_WITH_MPI`: the mailbox of the workers of `Mailbox.h` with a process
 * of a communicator as each worker, and with the detection of the
 * termination of the computation by the four counters of Mattern (see
 * \ref theory_distributed_messages). A
 * process sends by `send( to , x )`, which appends \f$ x \f$ to its outbox
 * for the rank `to` and, when the outbox holds `batch` messages, sends it
 * as one message of MPI that does not block (`MPI_Isend`), the buffer kept
 * in a list of pending sends until MPI has completed it (`MPI_Testsome`);
 * `poll( f )` frees the buffers of the sends completed, then receives the
 * batches that have come (`MPI_Iprobe` and `MPI_Recv`, from any process)
 * and calls `f( x )` on each message; `quiescent()`, called by a process
 * with no work left, sends its outboxes and tells whether the whole
 * computation has ended. The messages of a process to itself do not go
 * through MPI. The mailbox duplicates the communicator, so that its
 * messages and its reductions never meet those of the program.
 *
 * The messages come in three forms, all on the same waves: `MPIMailbox<
 * Msg >` of a trivially copyable `Msg` sends its batches as arrays of
 * messages, copied as bytes (several kinds of such messages go as one
 * `std::variant`, itself trivially copyable); `MPIByteMailbox` sends
 * messages of any length, each made of bytes and of a tag of 32 bits
 * (`send( to , tag , bytes )`, and `poll( f )` calling `f( tag , bytes )`),
 * packed into the records of a batch of `batch` bytes; and `MPIMailbox<
 * Msg >` of a `Msg` that is not trivially copyable is an `MPIByteMailbox`
 * whose messages are the bytes that the customization point `MPICodec<
 * Msg >` gives (`encode()` and `decode()`, given for `std::string` and for
 * the vectors of trivially copyable items). The waves count the messages,
 * whatever their kind.
 *
 * `stop()` ends the computation of all the processes at once, whatever the
 * messages on their way: the process sends a note (an empty batch) to each
 * other one, which stops when its `poll()` finds it; a process stopped
 * posts and receives nothing more, and contributes to the waves as stopped,
 * so that all the processes see the end at the first wave to which all
 * have contributed stopped. `leftovers( f )`, a collective operation after
 * `quiescent()` has returned true, then hands to \f$ f \f$ the messages left
 * to the process: those still on their way to it, whose number each
 * process learns by one reduction of the counts of all the senders
 * (`MPI_Reduce_scatter_block`), those to itself and those of its outboxes;
 * every message sent is so handed to a function exactly once, by a `poll()`
 * or by `leftovers()` (see \ref theory_distributed_messages). The
 * destructor makes the same exchange, dropping the messages, if
 * `leftovers()` was not called.
 *
 * The termination is detected by waves of counters: each process counts the
 * messages it has sent, \f$ s_i \f$, and those it has received,
 * \f$ r_i \f$; a wave is a reduction that does not block
 * (`MPI_Iallreduce`) of the pair \f$ ( s_i , r_i ) \f$ of each process into
 * the sums \f$ ( S , R ) \f$, to which a process contributes only when it is
 * passive, i.e., from `quiescent()`, and that it tests at each call of
 * `quiescent()` while it waits, still receiving and working. A process
 * starts a wave only after the previous one has completed, so that the
 * contributions to a wave all come after those to the previous one; when
 * the wave \f$ k \f$ completes with \f$ S_k = R_{k - 1} \f$ (the sends
 * counted by the second wave equal the receipts counted by the first), the
 * computation had ended when the wave \f$ k - 1 \f$ completed (Mattern's
 * four counters), and every process, having the same sums, stops at the
 * same wave. A wave costs \f$ O( \log P ) \f$ steps of latency on
 * \f$ P \f$ processes, and the termination is known at most two waves after
 * it happened.
 *
 * The contracts [see `Contract.h`]: `Msg` is trivially copyable or has
 * `MPICodec< Msg >`; every process of the communicator builds the mailbox
 * and calls `quiescent()` until it returns true (its waves are collective),
 * and `leftovers()` if any process calls it; a process sends only while
 * active, i.e., before its first `quiescent()` or after a `poll()` that
 * found messages, and becomes active only by receiving; a process that
 * sees `stopped()` leaves its work and calls `quiescent()`; `f` does not
 * throw and does not poll; one thread of the process calls the mailbox.
 *
 * \see F. Mattern, Algorithms for distributed termination detection,
 *      Distributed Computing 2(3), 161-175, 1987, doi:10.1007/BF01782776
 * \see T. Hoefler, C. Siebert, A. Lumsdaine, Scalable communication
 *      protocols for dynamic sparse data exchange, ACM SIGPLAN Symposium on
 *      Principles and Practice of Parallel Programming (PPoPP), 159-168,
 *      2010, doi:10.1145/1693453.1693476
 * \see B. Priest, T. Steil, G. Sanders, R. Pearce, You've Got Mail (YGM):
 *      building missing asynchronous communication primitives, IEEE
 *      International Parallel and Distributed Processing Symposium
 *      Workshops (IPDPSW), 221-230, 2019, doi:10.1109/IPDPSW.2019.00045
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

#ifndef __ADS_MPIMailbox
 #define __ADS_MPIMailbox
                      /* self-identification: #endif constant */

#if defined( ADS_WITH_MPI )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/distributed/MPIExecutor.h"
#include "ADS/parallel/Mailbox.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the customization point of the messages that are not trivially copyable
/// for `MPIMailbox`: a specialization `MPICodec< Msg >` has
/// `static void encode( const Msg & x , std::vector< std::byte > & out )`,
/// which appends the bytes of \f$ x \f$ to `out`, and
/// `static Msg decode( std::span< const std::byte > b )`, which rebuilds the
/// message from exactly the bytes that `encode()` appended; ADS gives those
/// of `std::string` and of `std::vector< T >` for a trivially copyable
/// \f$ T \f$
template< class Msg >
struct MPICodec;

/// the bytes of a string
template<>
struct MPICodec< std::string > {
 /// the characters appended to `out`
 static void encode( const std::string & x , std::vector< std::byte > & out )
 {
  const auto * b = reinterpret_cast< const std::byte * >( x.data() );
  out.insert( out.end() , b , b + x.size() );
  }
 /// the string of the bytes \f$ b \f$
 static std::string decode( std::span< const std::byte > b ) {
  return( std::string( reinterpret_cast< const char * >( b.data() ) ,
                       b.size() ) );
  }
 };

/// the bytes of a vector of trivially copyable items
template< class T >
 requires std::is_trivially_copyable_v< T >
struct MPICodec< std::vector< T > > {
 /// the bytes of the items appended to `out`
 static void encode( const std::vector< T > & x ,
                     std::vector< std::byte > & out ) {
  const std::size_t at = out.size();
  out.resize( at + x.size() * sizeof( T ) );
  if( ! x.empty() )
   std::memcpy( out.data() + at , x.data() , x.size() * sizeof( T ) );
  }
 /// the vector of the bytes \f$ b \f$
 static std::vector< T > decode( std::span< const std::byte > b ) {
  std::vector< T > x( b.size() / sizeof( T ) );
  if( ! x.empty() )
   std::memcpy( x.data() , b.data() , x.size() * sizeof( T ) );
  return( x );
  }
 };

namespace detail {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MPIMailboxCore --------------------------*/
/*--------------------------------------------------------------------------*/

/// what the mailboxes of MPI share, whatever their messages: the duplicate
/// of the communicator, the sends in flight with their buffers of items
/// `B`, the counters, the waves of Mattern, the stop and the closing; a
/// batch of no bytes is the note of a stop
template< class B >
class MPIMailboxCore {

/*--------------------------------------------------------------------------*/
 public:

 /// no copy, since MPI holds the buffers
 MPIMailboxCore( const MPIMailboxCore & ) = delete;
 /// no copy, since MPI holds the buffers
 MPIMailboxCore & operator=( const MPIMailboxCore & ) = delete;

 /// the rank of this process
 [[nodiscard]] std::size_t rank( void ) const {
  return( std::size_t( f_rank ) );
  }

 /// the number of the processes
 [[nodiscard]] std::size_t size( void ) const {
  return( std::size_t( f_size ) );
  }

 /// true once `quiescent()` has seen the end of the computation, or the
 /// end of its stop
 [[nodiscard]] bool terminated( void ) const { return( f_done ); }

 /// true once this process has called `stop()` or has received the note
 /// of a stop: it should leave its work and call `quiescent()`
 [[nodiscard]] bool stopped( void ) const { return( f_stopped ); }

 /// ends the computation of all the processes at once, whatever the
 /// messages on their way: a note to every other process, which stops when
 /// it polls it; from then on this process posts and receives nothing,
 /// and `quiescent()` returns true on every process at the first wave to
 /// which all have contributed stopped (see
 /// \ref theory_distributed_messages); a call after the end, or a second
 /// call, does nothing
 void stop( void ) {
  if( f_done || f_stopped )
   return;
  f_stopped = true;
  for( int q = 0 ; q < f_size ; ++q )
   if( q != f_rank ) {
    f_buf.emplace_back();
    f_req.push_back( MPI_REQUEST_NULL );
    detail::mpiCheck( MPI_Isend( nullptr , 0 , MPI_BYTE , q , kTag ,
                                 f_comm , &f_req.back() ) , "MPI_Isend" );
    ++f_to[ 2 * std::size_t( q ) + 1 ];
    }
  }

 /// the messages sent by this process so far
 [[nodiscard]] std::uint64_t sent( void ) const { return( f_sent ); }

 /// the batches sent to the other processes by MPI so far
 [[nodiscard]] std::uint64_t batches( void ) const { return( f_batches ); }

 /// the messages received by this process so far, by `poll()`
 [[nodiscard]] std::uint64_t received( void ) const {
  return( f_received );
  }

 /// the waves completed so far
 [[nodiscard]] std::uint64_t waves( void ) const { return( f_waves ); }

 /// the communicator of the mailbox, a duplicate of the one given
 [[nodiscard]] MPI_Comm comm( void ) const { return( f_comm ); }

/*--------------------------------------------------------------------------*/
 protected:

 /// the tag of the batches and of the notes
 static constexpr int kTag = 1;

 /// the core of the processes of `comm`; a collective operation
 explicit MPIMailboxCore( MPI_Comm comm ) {
  detail::mpiCheck( MPI_Comm_dup( comm , &f_comm ) , "MPI_Comm_dup" );
  detail::mpiCheck( MPI_Comm_rank( f_comm , &f_rank ) , "MPI_Comm_rank" );
  detail::mpiCheck( MPI_Comm_size( f_comm , &f_size ) , "MPI_Comm_size" );
  f_to.assign( 2 * std::size_t( f_size ) , 0 );
  }

 /// waits for the sends and the wave in flight, and frees the duplicate
 /// of the communicator; a collective operation
 ~MPIMailboxCore() {
  if( ! f_req.empty() )
   MPI_Waitall( int( f_req.size() ) , f_req.data() , MPI_STATUSES_IGNORE );
  if( f_waving )
   MPI_Wait( &f_wave , MPI_STATUS_IGNORE );
  MPI_Comm_free( &f_comm );
  }

 /// the batch \f$ b \f$ of \f$ k \f$ messages sent to the rank `to`, by a
 /// send that does not block, its buffer kept until MPI completes it
 void isend( std::vector< B > && b , std::size_t to , std::uint64_t k ) {
  f_buf.push_back( std::move( b ) );
  const auto & x = f_buf.back();
  f_req.push_back( MPI_REQUEST_NULL );
  detail::mpiCheck( MPI_Isend( x.data() , int( x.size() * sizeof( B ) ) ,
                               MPI_BYTE , int( to ) , kTag , f_comm ,
                               &f_req.back() ) , "MPI_Isend" );
  f_to[ 2 * to ] += k;
  ++f_batches;
  }

 /// the batches that have come, at most 64, each handed to `take`, which
 /// returns its messages; stops at the note of a stop; their messages
 template< class T >
 std::size_t receive( T && take ) {
  std::size_t n = 0;
  for( int batches = 0 ; batches < 64 ; ++batches ) {
   int flag = 0;
   MPI_Status st;
   detail::mpiCheck( MPI_Iprobe( MPI_ANY_SOURCE , kTag , f_comm , &flag ,
                                 &st ) , "MPI_Iprobe" );
   if( ! flag )
    break;
   const std::size_t k = receiveOne( st , take );
   if( k == 0 ) {                          // the note of a stop
    f_stopped = true;
    break;
    }
   n += k;
   }
  f_received += n;
  return( n );
  }

 /// the buffers of the sends completed freed
 void progress( void ) {
  if( f_req.empty() )
   return;
  int done = 0;
  f_index.resize( f_req.size() );
  detail::mpiCheck( MPI_Testsome( int( f_req.size() ) , f_req.data() ,
                                  &done , f_index.data() ,
                                  MPI_STATUSES_IGNORE ) , "MPI_Testsome" );
  if( ( done == MPI_UNDEFINED ) || ( done == 0 ) )
   return;
  // the requests completed are now MPI_REQUEST_NULL: keep the others
  std::size_t keep = 0;
  for( std::size_t i = 0 ; i < f_req.size() ; ++i )
   if( f_req[ i ] != MPI_REQUEST_NULL ) {
    if( keep != i ) {
     f_req[ keep ] = f_req[ i ];
     f_buf[ keep ] = std::move( f_buf[ i ] );
     }
    ++keep;
    }
  f_req.resize( keep );
  f_buf.resize( keep );
  }

 /// the waves of `quiescent()`, after the outboxes are sent: true at the
 /// end of the computation or of its stop
 bool wave( void ) {
  if( f_done )
   return( true );
  progress();
  if( f_waving ) {
   int flag = 0;
   detail::mpiCheck( MPI_Test( &f_wave , &flag , MPI_STATUS_IGNORE ) ,
                     "MPI_Test" );
   if( ! flag )
    return( false );
   f_waving = false;
   ++f_waves;
   if( f_sums[ 2 ] == std::uint64_t( f_size ) )
    return( f_done = true );               // all stopped
   if( f_waves > 1 && ( f_sums[ 0 ] == f_lastR ) )
    return( f_done = true );
   f_lastR = f_sums[ 1 ];
   }
  f_mine[ 0 ] = f_sent;
  f_mine[ 1 ] = f_received;
  f_mine[ 2 ] = f_stopped ? 1 : 0;
  detail::mpiCheck( MPI_Iallreduce( f_mine , f_sums , 3 , MPI_UINT64_T ,
                                    MPI_SUM , f_comm , &f_wave ) ,
                    "MPI_Iallreduce" );
  f_waving = true;
  return( false );
  }

 /// the closing, a collective operation after the end: the messages and
 /// the notes sent to each process summed over all of them, and those not
 /// yet received handed to `take` (the notes dropped); then the sends
 /// complete; the messages
 template< class T >
 std::size_t close( T && take ) {
  if( f_closed )
   return( 0 );
  f_closed = true;
  std::uint64_t in[ 2 ] = { 0 , 0 };
  detail::mpiCheck( MPI_Reduce_scatter_block( f_to.data() , in , 2 ,
                                              MPI_UINT64_T , MPI_SUM ,
                                              f_comm ) ,
                    "MPI_Reduce_scatter_block" );
  std::size_t n = 0;
  while( ( f_inMsgs < in[ 0 ] ) || ( f_inNotes < in[ 1 ] ) ) {
   MPI_Status st;
   detail::mpiCheck( MPI_Probe( MPI_ANY_SOURCE , kTag , f_comm , &st ) ,
                     "MPI_Probe" );
   n += receiveOne( st , take );
   }
  if( ! f_req.empty() )
   detail::mpiCheck( MPI_Waitall( int( f_req.size() ) , f_req.data() ,
                                  MPI_STATUSES_IGNORE ) , "MPI_Waitall" );
  f_req.clear();
  f_buf.clear();
  return( n );
  }

 /// whether the closing was done
 [[nodiscard]] bool closed( void ) const { return( f_closed ); }

 /// counts the messages sent, those to itself included
 void countSent( std::uint64_t k ) { f_sent += k; }

 /// counts the messages received by this process from itself
 void countSelf( std::uint64_t k ) { f_received += k; }

/*--------------------------------------------------------------------------*/
 private:

 /// the batch of the status received and handed to `take`; its messages,
 /// 0 for a note
 template< class T >
 std::size_t receiveOne( const MPI_Status & st , T & take ) {
  int bytes = 0;
  detail::mpiCheck( MPI_Get_count( &st , MPI_BYTE , &bytes ) ,
                    "MPI_Get_count" );
  f_in.resize( std::size_t( bytes ) / sizeof( B ) );
  detail::mpiCheck( MPI_Recv( f_in.data() , bytes , MPI_BYTE ,
                              st.MPI_SOURCE , kTag , f_comm ,
                              MPI_STATUS_IGNORE ) , "MPI_Recv" );
  if( bytes == 0 ) {
   ++f_inNotes;
   return( 0 );
   }
  const std::size_t k = take( std::span< const B >( f_in ) );
  f_inMsgs += k;
  return( k );
  }

 MPI_Comm f_comm = MPI_COMM_NULL;          ///< the duplicate
 int f_rank = 0;                           ///< this process
 int f_size = 1;                           ///< the processes
 std::vector< B > f_in;                    ///< the batch received
 std::vector< MPI_Request > f_req;         ///< the sends pending
 std::vector< std::vector< B > > f_buf;    ///< their buffers
 std::vector< int > f_index;               ///< for MPI_Testsome
 /// the messages and the notes sent to each process by MPI
 std::vector< std::uint64_t > f_to;
 std::uint64_t f_inMsgs = 0;               ///< messages received by MPI
 std::uint64_t f_inNotes = 0;              ///< notes received
 std::uint64_t f_sent = 0;                 ///< the messages sent
 std::uint64_t f_batches = 0;              ///< the batches sent by MPI
 std::uint64_t f_received = 0;             ///< the messages received
 MPI_Request f_wave = MPI_REQUEST_NULL;    ///< the wave in flight
 bool f_waving = false;                    ///< whether one is
 std::uint64_t f_mine[ 3 ] = { 0 , 0 , 0 };   ///< its counters here
 std::uint64_t f_sums[ 3 ] = { 0 , 0 , 0 };   ///< their sums
 std::uint64_t f_lastR = 0;                ///< the receipts of the last
 std::uint64_t f_waves = 0;                ///< the waves completed
 bool f_stopped = false;                   ///< stopped here
 bool f_done = false;                      ///< the end was seen
 bool f_closed = false;                    ///< the closing was done
 };  // end( class MPIMailboxCore )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MPIByteMailbox --------------------------*/
/*--------------------------------------------------------------------------*/

/// the mailbox of the processes of a communicator for messages of bytes,
/// each with a tag of its own, in batches of records
class MPIByteMailbox : public detail::MPIMailboxCore< std::byte > {

 using Core = detail::MPIMailboxCore< std::byte >;   ///< the core

/*--------------------------------------------------------------------------*/
 public:

 /// the mailbox of the processes of `comm`, which sends an outbox when it
 /// holds `batch` bytes; a collective operation
 /// \pre MPI is initialized, \f$ \mathit{batch} \ge 1 \f$
 explicit MPIByteMailbox( MPI_Comm comm = MPI_COMM_WORLD ,
                          std::size_t batch = 65536 )
  : Core( comm ) , f_batch( batch ) {
  ADS_PRE( batch >= 1 );
  f_out.resize( size() );
  f_count.assign( size() , 0 );
  }

 /// the closing, if `leftovers()` did not do it, its messages dropped; a
 /// collective operation
 ~MPIByteMailbox() {
  if( terminated() && ! closed() )
   try {
    close( []( std::span< const std::byte > b ) {
     return( records( b ) ); } );
    }
   catch( ... ) {                          // a destructor does not throw
    }
  }

 /// the message of the bytes \f$ b \f$ and of the tag \f$ t \f$ appended to
 /// the outbox for `to`, which is sent when it holds a batch; never waits,
 /// and after the stop only appends
 ///
 /// In pseudocode (beyond the book), \f$ O_{\mathit{to}} \f$ the outbox:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{send}( \mathit{to} , t , b ) \cr
 /// 2 & \quad \text{append to } O_{\mathit{to}} \text{ the record of } t
 ///     \text{, of } |b| \text{ and of } b \text{, padded to 8 bytes} \cr
 /// 3 & \quad \textbf{if}\ |O_{\mathit{to}}| \ge \mathit{batch}
 ///     \ \textbf{and not stopped then}\ \mathrm{post}( \mathit{to} )
 /// \end{array}}
 /// \f]
 /// \pre `to` is a rank, \f$ |b| < 2^{32} \f$
 void send( std::size_t to , std::uint32_t t ,
            std::span< const std::byte > b ) {
  ADS_PRE( ( to < size() ) && ( b.size() <= 0xFFFFFFFFull ) );
  auto & o = f_out[ to ];
  const std::uint32_t head[ 2 ] = { t , std::uint32_t( b.size() ) };
  const std::size_t at = o.size();
  o.resize( at + sizeof( head ) + padded( b.size() ) );
  std::memcpy( o.data() + at , head , sizeof( head ) );
  if( ! b.empty() )
   std::memcpy( o.data() + at + sizeof( head ) , b.data() , b.size() );
  ++f_count[ to ];
  if( ( o.size() >= f_batch ) && ! stopped() )
   post( to );
  }

 /// the outboxes sent, unless stopped
 void flush( void ) {
  if( stopped() )
   return;
  for( std::size_t to = 0 ; to < f_out.size() ; ++to )
   if( ! f_out[ to ].empty() )
    post( to );
  }

 /// the calls `f( t , b )` on the messages received, of tag \f$ t \f$ and
 /// bytes \f$ b \f$ (valid during the call, at an offset multiple of 8 of
 /// a buffer that `new` allocated), and their number; nothing after the
 /// stop
 ///
 /// In pseudocode (beyond the book), \f$ r \f$ the messages received:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{poll}( f ) \cr
 /// 2 & \quad \textbf{if}\ \text{stopped}\ \textbf{then return}\ 0 \cr
 /// 3 & \quad \text{free the buffers of the sends that }
 ///     \mathrm{MPI\_Testsome} \text{ finds completed} \cr
 /// 4 & \quad n \gets 0;\ \textbf{foreach}\ \text{record } ( t , b )
 ///     \text{ to itself}\ \textbf{do}\ f( t , b );\ n \gets n + 1 \cr
 /// 5 & \quad \textbf{while}\ \mathrm{MPI\_Iprobe}( \text{any source} )
 ///     \text{ finds a batch}\ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ \text{it is empty}\ \textbf{then}
 ///     \ \text{stopped};\ \textbf{break}\quad \triangleright
 ///     \ \text{the note of a stop} \cr
 /// 7 & \qquad \mathrm{MPI\_Recv};\ \textbf{foreach}\ \text{record }
 ///     ( t , b )\ \textbf{do}\ f( t , b );\ n \gets n + 1 \cr
 /// 8 & \quad r \gets r + n;\ \textbf{return}\ n
 /// \end{array}}
 /// \f]
 template< class F >
 std::size_t poll( F && f ) {
  if( stopped() )
   return( 0 );
  progress();
  std::size_t n = 0;
  if( ! f_self.empty() ) {
   std::swap( f_self , f_selfTaken );
   n = forEach( f_selfTaken , f );
   f_selfTaken.clear();
   countSelf( n );
   }
  return( n + receive( [ & ]( std::span< const std::byte > b ) {
   return( forEach( b , f ) ); } ) );
  }

 /// true if the computation has ended, or its stop; called with no work
 /// left, it sends the outboxes (unless stopped) and joins the waves
 bool quiescent( void ) {
  if( terminated() )
   return( true );
  flush();
  return( wave() );
  }

 /// the calls `f( t , b )` on the messages left to this process, those on
 /// their way to it, those to itself and those of its outboxes, and their
 /// number: a collective operation after `quiescent()` has returned true,
 /// after which every message sent has been handed to a function once, by
 /// a `poll()` or here
 /// \pre `terminated()`
 template< class F >
 std::size_t leftovers( F && f ) {
  ADS_PRE( terminated() );
  std::size_t n = close( [ & ]( std::span< const std::byte > b ) {
   return( forEach( b , f ) ); } );
  n += forEach( f_self , f );
  f_self.clear();
  for( auto & o : f_out ) {
   n += forEach( o , f );
   o.clear();
   }
  return( n );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// \f$ k \f$ rounded up to a multiple of 8
 static std::size_t padded( std::size_t k ) { return( ( k + 7 ) & ~7ull ); }

 /// the calls f( t , b ) on the records of the bytes x, and their number
 template< class F >
 static std::size_t forEach( std::span< const std::byte > x , F & f ) {
  std::size_t n = 0;
  for( std::size_t at = 0 ; at < x.size() ; ++n ) {
   std::uint32_t head[ 2 ];
   std::memcpy( head , x.data() + at , sizeof( head ) );
   at += sizeof( head );
   f( head[ 0 ] , x.subspan( at , head[ 1 ] ) );
   at += padded( head[ 1 ] );
   }
  return( n );
  }

 /// the records of the bytes x
 static std::size_t records( std::span< const std::byte > x ) {
  auto none = []( std::uint32_t , std::span< const std::byte > ) {};
  return( forEach( x , none ) );
  }

 /// the outbox for `to` sent, or moved to the messages to itself
 void post( std::size_t to ) {
  auto & o = f_out[ to ];
  countSent( f_count[ to ] );
  if( to == rank() )
   f_self.insert( f_self.end() , o.begin() , o.end() );
  else {
   std::vector< std::byte > b;
   b.reserve( f_batch + 64 );
   std::swap( b , o );
   isend( std::move( b ) , to , f_count[ to ] );
   }
  o.clear();
  f_count[ to ] = 0;
  }

 std::size_t f_batch;                      ///< the bytes of a batch
 std::vector< std::vector< std::byte > > f_out;   ///< the outboxes
 std::vector< std::uint64_t > f_count;     ///< their messages
 std::vector< std::byte > f_self;          ///< the messages to itself
 std::vector< std::byte > f_selfTaken;     ///< those of a poll
 };  // end( class MPIByteMailbox )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPIMailbox ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the mailbox of the processes of a communicator, with outboxes of batches
/// and the detection of the termination by the four counters of Mattern:
/// for a trivially copyable `Msg`, the batches are arrays of messages,
/// copied as bytes; otherwise [see the specialization below] each message
/// is a record of the bytes that `MPICodec< Msg >` gives
template< class Msg , bool = std::is_trivially_copyable_v< Msg > >
class MPIMailbox : public detail::MPIMailboxCore< Msg > {

 using Core = detail::MPIMailboxCore< Msg >;   ///< the core

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages

 /// the mailbox of the processes of `comm`, which sends batches of `batch`
 /// messages; a collective operation
 /// \pre MPI is initialized, \f$ \mathit{batch} \ge 1 \f$
 explicit MPIMailbox( MPI_Comm comm = MPI_COMM_WORLD ,
                      std::size_t batch = 256 )
  : Core( comm ) , f_batch( batch ) {
  ADS_PRE( batch >= 1 );
  f_out.resize( this->size() );
  }

 /// the closing, if `leftovers()` did not do it, its messages dropped; a
 /// collective operation
 ~MPIMailbox() {
  if( this->terminated() && ! this->closed() )
   try {
    this->close( []( std::span< const Msg > b ) { return( b.size() ); } );
    }
   catch( ... ) {                          // a destructor does not throw
    }
  }

 /// \f$ x \f$ appended to the outbox for `to`, which is sent when it holds
 /// a batch; never waits, and after the stop only appends
 ///
 /// In pseudocode (beyond the book), \f$ O_{\mathit{to}} \f$ the outbox:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{send}( \mathit{to} , x ) \cr
 /// 2 & \quad \text{append } x \text{ to } O_{\mathit{to}} \cr
 /// 3 & \quad \textbf{if}\ |O_{\mathit{to}}| \ge \mathit{batch}
 ///     \ \textbf{and not stopped then}\ \mathrm{post}( \mathit{to} )
 /// \end{array}}
 /// \f]
 /// \pre `to` is a rank
 void send( std::size_t to , const Msg & x ) {
  ADS_PRE( to < this->size() );
  auto & o = f_out[ to ];
  o.push_back( x );
  if( ( o.size() >= f_batch ) && ! this->stopped() )
   post( to );
  }

 /// the outboxes sent, unless stopped
 ///
 /// In pseudocode (beyond the book), \f$ s \f$ the messages sent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{flush}() \cr
 /// 2 & \quad \textbf{if}\ \textbf{not}\ \text{stopped}\ \textbf{then}
 ///     \ \textbf{foreach}\ \mathit{to} \text{ with } O_{\mathit{to}}
 ///     \ne \emptyset\ \textbf{do}\ \mathrm{post}( \mathit{to} ) \cr
 /// 3 & \textbf{procedure}\ \mathrm{post}( \mathit{to} ) \cr
 /// 4 & \quad s \gets s + |O_{\mathit{to}}| \cr
 /// 5 & \quad \textbf{if}\ \mathit{to} \text{ is this process}
 ///     \ \textbf{then}\ \text{move } O_{\mathit{to}} \text{ to the }
 ///     \text{messages to itself} \cr
 /// 6 & \quad \textbf{else}\ \mathrm{MPI\_Isend}( O_{\mathit{to}} ,
 ///     \mathit{to} ) \text{, the buffer kept with its request until it }
 ///     \text{completes}
 /// \end{array}}
 /// \f]
 void flush( void ) {
  if( this->stopped() )
   return;
  for( std::size_t to = 0 ; to < f_out.size() ; ++to )
   if( ! f_out[ to ].empty() )
    post( to );
  }

 /// the calls `f( x )` on the messages received, and their number; nothing
 /// after the stop
 ///
 /// In pseudocode (beyond the book), \f$ r \f$ the messages received:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{poll}( f ) \cr
 /// 2 & \quad \textbf{if}\ \text{stopped}\ \textbf{then return}\ 0 \cr
 /// 3 & \quad \text{free the buffers of the sends that }
 ///     \mathrm{MPI\_Testsome} \text{ finds completed} \cr
 /// 4 & \quad n \gets 0;\ \textbf{foreach}\ x \text{ among the messages to }
 ///     \text{itself}\ \textbf{do}\ f( x );\ n \gets n + 1 \cr
 /// 5 & \quad \textbf{while}\ \mathrm{MPI\_Iprobe}( \text{any source} )
 ///     \text{ finds a batch } b\ \textbf{do} \cr
 /// 6 & \qquad \textbf{if}\ b = \emptyset\ \textbf{then}\ \text{stopped};
 ///     \ \textbf{break}\quad \triangleright\ \text{the note of a stop} \cr
 /// 7 & \qquad \mathrm{MPI\_Recv}( b );\ \textbf{foreach}\ x \in b
 ///     \ \textbf{do}\ f( x );\ n \gets n + |b| \cr
 /// 8 & \quad r \gets r + n;\ \textbf{return}\ n
 /// \end{array}}
 /// \f]
 template< class F >
 std::size_t poll( F && f ) {
  if( this->stopped() )
   return( 0 );
  this->progress();
  std::size_t n = 0;
  if( ! f_self.empty() ) {
   std::swap( f_self , f_selfTaken );
   for( const auto & x : f_selfTaken )
    f( x );
   n += f_selfTaken.size();
   f_selfTaken.clear();
   this->countSelf( n );
   }
  return( n + this->receive( [ & ]( std::span< const Msg > b ) {
   for( const auto & x : b )
    f( x );
   return( b.size() ); } ) );
  }

 /// true if the computation has ended, or its stop; called with no work
 /// left, it sends the outboxes (unless stopped) and joins the waves
 ///
 /// In pseudocode (Mattern's four counters on reductions that do not
 /// block, beyond the book), \f$ R' \f$ the receipts of the previous wave,
 /// \f$ \sigma \f$ the processes stopped:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{quiescent}() \cr
 /// 2 & \quad \mathrm{flush}() \cr
 /// 3 & \quad \textbf{if}\ \text{a wave is in flight}\ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ \text{it has not completed}\ \textbf{then}
 ///     \ \textbf{return false} \cr
 /// 5 & \qquad ( S , R , \sigma ) \gets \text{its sums};\ \textbf{if}
 ///     \ \sigma = P\ \textbf{then return true} \cr
 /// 6 & \qquad \textbf{if}\ \text{a previous wave exists }\textbf{and}
 ///     \ S = R'\ \textbf{then}\ \textbf{return true} \cr
 /// 7 & \qquad R' \gets R \cr
 /// 8 & \quad \mathrm{MPI\_Iallreduce}( ( s , r , \text{stopped} ) , + )
 ///     \quad \triangleright\ \text{the next wave, from a passive process}
 ///     \cr
 /// 9 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 bool quiescent( void ) {
  if( this->terminated() )
   return( true );
  flush();
  return( this->wave() );
  }

 /// the calls `f( x )` on the messages left to this process, those on
 /// their way to it, those to itself and those of its outboxes, and their
 /// number: a collective operation after `quiescent()` has returned true,
 /// after which every message sent has been handed to a function once, by
 /// a `poll()` or here (none after a termination without stop)
 ///
 /// In pseudocode (beyond the book), \f$ m_{q , i} \f$ the messages and
 /// \f$ n_{q , i} \f$ the notes sent by MPI by \f$ q \f$ to \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{leftovers}( f ) \cr
 /// 2 & \quad ( M , N ) \gets \mathrm{MPI\_Reduce\_scatter\_block}( ( m_{
 ///     i , q } , n_{ i , q } )_q , + )\quad \triangleright\ \text{those to}
 ///     \text{ this process} \cr
 /// 3 & \quad \textbf{while}\ \text{fewer than } M \text{ messages or } N
 ///     \text{ notes were received}\ \textbf{do} \cr
 /// 4 & \qquad \mathrm{MPI\_Recv}( b ) \text{ from any source};
 ///     \ \textbf{foreach}\ x \in b\ \textbf{do}\ f( x ) \cr
 /// 5 & \quad \text{wait for its sends};\ \textbf{foreach}\ x \text{ to }
 ///     \text{itself or in an outbox}\ \textbf{do}\ f( x )
 /// \end{array}}
 /// \f]
 /// \pre `terminated()`
 template< class F >
 std::size_t leftovers( F && f ) {
  ADS_PRE( this->terminated() );
  std::size_t n = this->close( [ & ]( std::span< const Msg > b ) {
   for( const auto & x : b )
    f( x );
   return( b.size() ); } );
  for( auto * v : { &f_self , &f_selfTaken } ) {
   for( const auto & x : *v )
    f( x );
   n += v->size();
   v->clear();
   }
  for( auto & o : f_out ) {
   for( const auto & x : o )
    f( x );
   n += o.size();
   o.clear();
   }
  return( n );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// the outbox for `to` sent, or moved to the messages to itself
 void post( std::size_t to ) {
  auto & o = f_out[ to ];
  this->countSent( o.size() );
  if( to == this->rank() ) {
   f_self.insert( f_self.end() , o.begin() , o.end() );
   o.clear();
   return;
   }
  const std::uint64_t k = o.size();
  std::vector< Msg > b;
  b.reserve( f_batch );
  std::swap( b , o );
  this->isend( std::move( b ) , to , k );
  }

 std::size_t f_batch;                      ///< the messages of a batch
 std::vector< std::vector< Msg > > f_out;  ///< the outboxes
 std::vector< Msg > f_self;                ///< the messages to itself
 std::vector< Msg > f_selfTaken;           ///< those of a poll
 };  // end( class MPIMailbox )

/*--------------------------------------------------------------------------*/
/*--------------------- CLASS MPIMailbox< Msg , false > --------------------*/
/*--------------------------------------------------------------------------*/

/// the mailbox of the processes of a communicator for the messages that
/// are not trivially copyable, each the record of the bytes that
/// `MPICodec< Msg >` gives, on an `MPIByteMailbox` (the same waves, the
/// same stop and the same `leftovers()`)
template< class Msg >
class MPIMailbox< Msg , false > {

 static_assert( requires( const Msg & x , std::vector< std::byte > & out ,
                          std::span< const std::byte > b ) {
  MPICodec< Msg >::encode( x , out );
  { MPICodec< Msg >::decode( b ) } -> std::convertible_to< Msg >;
  } , "a message that is not trivially copyable needs MPICodec< Msg >" );

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages

 /// the mailbox of the processes of `comm`, which sends an outbox when it
 /// holds `batch` bytes; a collective operation
 /// \pre MPI is initialized, \f$ \mathit{batch} \ge 1 \f$
 explicit MPIMailbox( MPI_Comm comm = MPI_COMM_WORLD ,
                      std::size_t batch = 65536 ) : f_box( comm , batch ) {}

 /// the rank of this process
 [[nodiscard]] std::size_t rank( void ) const { return( f_box.rank() ); }
 /// the number of the processes
 [[nodiscard]] std::size_t size( void ) const { return( f_box.size() ); }

 /// the bytes of \f$ x \f$ appended to the outbox for `to`, as
 /// `MPIByteMailbox::send()`
 /// \pre `to` is a rank
 void send( std::size_t to , const Msg & x ) {
  f_bytes.clear();
  MPICodec< Msg >::encode( x , f_bytes );
  f_box.send( to , 0 , f_bytes );
  }

 /// the outboxes sent, unless stopped
 void flush( void ) { f_box.flush(); }

 /// the calls `f( x )` on the messages received, rebuilt, and their number
 template< class F >
 std::size_t poll( F && f ) {
  return( f_box.poll( [ & ]( std::uint32_t ,
                             std::span< const std::byte > b ) {
   f( MPICodec< Msg >::decode( b ) ); } ) );
  }

 /// as `MPIByteMailbox::quiescent()`
 bool quiescent( void ) { return( f_box.quiescent() ); }

 /// as `MPIByteMailbox::leftovers()`, the messages rebuilt
 /// \pre `terminated()`
 template< class F >
 std::size_t leftovers( F && f ) {
  return( f_box.leftovers( [ & ]( std::uint32_t ,
                                  std::span< const std::byte > b ) {
   f( MPICodec< Msg >::decode( b ) ); } ) );
  }

 /// as `MPIByteMailbox::stop()`
 void stop( void ) { f_box.stop(); }
 /// true once this process has stopped
 [[nodiscard]] bool stopped( void ) const { return( f_box.stopped() ); }
 /// true once `quiescent()` has seen the end
 [[nodiscard]] bool terminated( void ) const {
  return( f_box.terminated() );
  }
 /// the messages sent by this process so far
 [[nodiscard]] std::uint64_t sent( void ) const { return( f_box.sent() ); }
 /// the batches sent by MPI so far
 [[nodiscard]] std::uint64_t batches( void ) const {
  return( f_box.batches() );
  }
 /// the messages received by this process so far
 [[nodiscard]] std::uint64_t received( void ) const {
  return( f_box.received() );
  }
 /// the waves completed so far
 [[nodiscard]] std::uint64_t waves( void ) const { return( f_box.waves() ); }
 /// the communicator of the mailbox
 [[nodiscard]] MPI_Comm comm( void ) const { return( f_box.comm() ); }

/*--------------------------------------------------------------------------*/
 private:

 MPIByteMailbox f_box;                     ///< the mailbox of bytes
 std::vector< std::byte > f_bytes;         ///< the bytes of a send
 };  // end( class MPIMailbox< Msg , false > )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif  /* ADS_WITH_MPI */

/*--------------------------------------------------------------------------*/

#endif  /* MPIMailbox.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPIMailbox.h --------------------------*/
/*--------------------------------------------------------------------------*/
