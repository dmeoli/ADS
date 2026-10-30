/*--------------------------------------------------------------------------*/
/*--------------------------- File MPIMailbox.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The mailbox of the processes of MPI, beyond the book, compiled only with
 * `ADS_WITH_MPI`: the mailbox of the workers of `Mailbox.h` with a process
 * of a communicator as each worker, for messages that are trivially
 * copyable, and with the detection of the termination of the computation by
 * the four counters of Mattern (see \ref theory_distributed_messages). A
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
 * The contracts [see `Contract.h`]: `Msg` is trivially copyable; every
 * process of the communicator builds the mailbox and calls `quiescent()`
 * until it returns true (its waves are collective); a process sends only
 * while active, i.e., before its first `quiescent()` or after a `poll()` that
 * found messages, and becomes active only by receiving; `f` does not throw
 * and does not poll; one thread of the process calls the mailbox.
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

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MPIMailbox ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the mailbox of the processes of a communicator, with outboxes of batches
/// and the detection of the termination by the four counters of Mattern
template< class Msg >
class MPIMailbox {
 static_assert( std::is_trivially_copyable_v< Msg > ,
                "the messages are copied as bytes" );

/*--------------------------------------------------------------------------*/
 public:

 using Message = Msg;   ///< the type of the messages

 /// the mailbox of the processes of `comm`, which sends batches of `batch`
 /// messages; a collective operation
 /// \pre MPI is initialized, \f$ \mathit{batch} \ge 1 \f$
 explicit MPIMailbox( MPI_Comm comm = MPI_COMM_WORLD ,
                      std::size_t batch = 256 ) : f_batch( batch ) {
  ADS_PRE( batch >= 1 );
  detail::mpiCheck( MPI_Comm_dup( comm , &f_comm ) , "MPI_Comm_dup" );
  detail::mpiCheck( MPI_Comm_rank( f_comm , &f_rank ) , "MPI_Comm_rank" );
  detail::mpiCheck( MPI_Comm_size( f_comm , &f_size ) , "MPI_Comm_size" );
  f_out.resize( std::size_t( f_size ) );
  }
 /// no copy, since MPI holds the buffers
 MPIMailbox( const MPIMailbox & ) = delete;
 /// no copy, since MPI holds the buffers
 MPIMailbox & operator=( const MPIMailbox & ) = delete;
 /// waits for the sends and the wave in flight, and frees the duplicate
 /// of the communicator; a collective operation
 ~MPIMailbox() {
  if( ! f_req.empty() )
   MPI_Waitall( int( f_req.size() ) , f_req.data() , MPI_STATUSES_IGNORE );
  if( f_waving )
   MPI_Wait( &f_wave , MPI_STATUS_IGNORE );
  MPI_Comm_free( &f_comm );
  }

 /// the rank of this process
 [[nodiscard]] std::size_t rank( void ) const {
  return( std::size_t( f_rank ) );
  }

 /// the number of the processes
 [[nodiscard]] std::size_t size( void ) const {
  return( std::size_t( f_size ) );
  }

 /// \f$ x \f$ appended to the outbox for `to`, which is sent when it holds
 /// a batch
 ///
 /// In pseudocode (beyond the book), \f$ O_{\mathit{to}} \f$ the outbox:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{send}( \mathit{to} , x ) \cr
 /// 2 & \quad \text{append } x \text{ to } O_{\mathit{to}} \cr
 /// 3 & \quad \textbf{if}\ |O_{\mathit{to}}| \ge \mathit{batch}\ \textbf{then}
 ///     \ \mathrm{post}( \mathit{to} )
 /// \end{array}}
 /// \f]
 /// \pre `to` is a rank
 void send( std::size_t to , const Msg & x ) {
  ADS_PRE( to < size() );
  auto & o = f_out[ to ];
  o.push_back( x );
  if( o.size() >= f_batch )
   post( to );
  }

 /// the outboxes sent
 ///
 /// In pseudocode (beyond the book), \f$ s \f$ the messages sent:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{flush}() \cr
 /// 2 & \quad \textbf{foreach}\ \mathit{to} \text{ with } O_{\mathit{to}}
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
  for( std::size_t to = 0 ; to < f_out.size() ; ++to )
   if( ! f_out[ to ].empty() )
    post( to );
  }

 /// the calls `f( x )` on the messages received, and their number
 ///
 /// In pseudocode (beyond the book), \f$ r \f$ the messages received:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{poll}( f ) \cr
 /// 2 & \quad \text{free the buffers of the sends that }
 ///     \mathrm{MPI\_Testsome} \text{ finds completed} \cr
 /// 3 & \quad n \gets 0;\ \textbf{foreach}\ x \text{ among the messages to }
 ///     \text{itself}\ \textbf{do}\ f( x );\ n \gets n + 1 \cr
 /// 4 & \quad \textbf{while}\ \mathrm{MPI\_Iprobe}( \text{any source} )
 ///     \text{ finds a batch } b\ \textbf{do} \cr
 /// 5 & \qquad \mathrm{MPI\_Recv}( b );\ \textbf{foreach}\ x \in b
 ///     \ \textbf{do}\ f( x );\ n \gets n + |b| \cr
 /// 6 & \quad r \gets r + n;\ \textbf{return}\ n
 /// \end{array}}
 /// \f]
 template< class F >
 std::size_t poll( F && f ) {
  progress();
  std::size_t n = 0;
  if( ! f_self.empty() ) {
   std::swap( f_self , f_selfTaken );
   for( const auto & x : f_selfTaken )
    f( x );
   n += f_selfTaken.size();
   f_selfTaken.clear();
   }
  // a bounded number of batches, so that a poll returns
  for( int batches = 0 ; batches < 64 ; ++batches ) {
   int flag = 0;
   MPI_Status st;
   detail::mpiCheck( MPI_Iprobe( MPI_ANY_SOURCE , kTag , f_comm , &flag ,
                                 &st ) , "MPI_Iprobe" );
   if( ! flag )
    break;
   int bytes = 0;
   detail::mpiCheck( MPI_Get_count( &st , MPI_BYTE , &bytes ) ,
                     "MPI_Get_count" );
   const std::size_t k = std::size_t( bytes ) / sizeof( Msg );
   f_in.resize( k );
   detail::mpiCheck( MPI_Recv( f_in.data() , bytes , MPI_BYTE ,
                               st.MPI_SOURCE , kTag , f_comm ,
                               MPI_STATUS_IGNORE ) , "MPI_Recv" );
   for( const auto & x : f_in )
    f( x );
   n += k;
   }
  f_received += n;
  return( n );
  }

 /// true if the computation has ended; called with no work left, it sends
 /// the outboxes and joins the waves of the counters
 ///
 /// In pseudocode (Mattern's four counters on reductions that do not
 /// block, beyond the book), \f$ R' \f$ the receipts of the previous wave:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{quiescent}() \cr
 /// 2 & \quad \mathrm{flush}() \cr
 /// 3 & \quad \textbf{if}\ \text{a wave is in flight}\ \textbf{then} \cr
 /// 4 & \qquad \textbf{if}\ \text{it has not completed}\ \textbf{then}
 ///     \ \textbf{return false} \cr
 /// 5 & \qquad ( S , R ) \gets \text{its sums};\ \textbf{if}\ \text{a }
 ///     \text{previous wave exists }\textbf{and}\ S = R'\ \textbf{then}
 ///     \ \textbf{return true} \cr
 /// 6 & \qquad R' \gets R \cr
 /// 7 & \quad \mathrm{MPI\_Iallreduce}( ( s , r ) , + )\quad \triangleright
 ///     \ \text{the next wave, from a passive process} \cr
 /// 8 & \quad \textbf{return false}
 /// \end{array}}
 /// \f]
 bool quiescent( void ) {
  if( f_done )
   return( true );
  flush();
  progress();
  if( f_waving ) {
   int flag = 0;
   detail::mpiCheck( MPI_Test( &f_wave , &flag , MPI_STATUS_IGNORE ) ,
                     "MPI_Test" );
   if( ! flag )
    return( false );
   f_waving = false;
   ++f_waves;
   if( f_waves > 1 && ( f_sums[ 0 ] == f_lastR ) )
    return( f_done = true );
   f_lastR = f_sums[ 1 ];
   }
  f_mine[ 0 ] = f_sent;
  f_mine[ 1 ] = f_received;
  detail::mpiCheck( MPI_Iallreduce( f_mine , f_sums , 2 , MPI_UINT64_T ,
                                    MPI_SUM , f_comm , &f_wave ) ,
                    "MPI_Iallreduce" );
  f_waving = true;
  return( false );
  }

 /// true once `quiescent()` has seen the end of the computation
 [[nodiscard]] bool terminated( void ) const { return( f_done ); }

 /// the messages sent by this process so far
 [[nodiscard]] std::uint64_t sent( void ) const { return( f_sent ); }

 /// the messages received by this process so far
 [[nodiscard]] std::uint64_t received( void ) const {
  return( f_received );
  }

 /// the waves completed so far
 [[nodiscard]] std::uint64_t waves( void ) const { return( f_waves ); }

 /// the communicator of the mailbox, a duplicate of the one given
 [[nodiscard]] MPI_Comm comm( void ) const { return( f_comm ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the tag of the batches
 static constexpr int kTag = 1;

 /// the outbox for `to` sent, or moved to the messages to itself
 void post( std::size_t to ) {
  auto & o = f_out[ to ];
  f_sent += o.size();
  if( int( to ) == f_rank ) {
   f_self.insert( f_self.end() , o.begin() , o.end() );
   o.clear();
   return;
   }
  f_buf.push_back( std::move( o ) );
  o = std::vector< Msg >();
  o.reserve( f_batch );
  const auto & b = f_buf.back();
  f_req.push_back( MPI_REQUEST_NULL );
  detail::mpiCheck( MPI_Isend( b.data() , int( b.size() * sizeof( Msg ) ) ,
                               MPI_BYTE , int( to ) , kTag , f_comm ,
                               &f_req.back() ) , "MPI_Isend" );
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

 MPI_Comm f_comm = MPI_COMM_NULL;          ///< the duplicate
 int f_rank = 0;                           ///< this process
 int f_size = 1;                           ///< the processes
 std::size_t f_batch;                      ///< the messages of a batch
 std::vector< std::vector< Msg > > f_out;  ///< the outboxes
 std::vector< Msg > f_self;                ///< the messages to itself
 std::vector< Msg > f_selfTaken;           ///< those of a poll
 std::vector< Msg > f_in;                  ///< the batch received
 std::vector< MPI_Request > f_req;         ///< the sends pending
 std::vector< std::vector< Msg > > f_buf;  ///< their buffers
 std::vector< int > f_index;               ///< for MPI_Testsome
 std::uint64_t f_sent = 0;                 ///< the messages sent
 std::uint64_t f_received = 0;             ///< the messages received
 MPI_Request f_wave = MPI_REQUEST_NULL;    ///< the wave in flight
 bool f_waving = false;                    ///< whether one is
 std::uint64_t f_mine[ 2 ] = { 0 , 0 };    ///< its counters here
 std::uint64_t f_sums[ 2 ] = { 0 , 0 };    ///< their sums
 std::uint64_t f_lastR = 0;                ///< the receipts of the last
 std::uint64_t f_waves = 0;                ///< the waves completed
 bool f_done = false;                      ///< the end was seen
 };  // end( class MPIMailbox )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif  /* ADS_WITH_MPI */

/*--------------------------------------------------------------------------*/

#endif  /* MPIMailbox.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MPIMailbox.h --------------------------*/
/*--------------------------------------------------------------------------*/
