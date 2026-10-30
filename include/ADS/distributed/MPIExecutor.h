/*--------------------------------------------------------------------------*/
/*--------------------------- File MPIExecutor.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The processes of MPI as the workers of a distributed computation, beyond
 * the book, compiled only with `ADS_WITH_MPI` (the CMake option of the same
 * name, AUTO by default, and its macro of the makefiles) and an
 * implementation of MPI (Open MPI or MPICH): `MPIExecutor` holds a
 * communicator and tells the rank of the process and the number of the
 * processes, and `run( f )` makes the call `f( rank )`, the worker of the
 * process, so that a program written for the workers of `run()` of the
 * executors of threads [see `Executor.h`] runs with one worker per process,
 * every process calling `run()`. `MPIEnvironment` starts and ends MPI around
 * a program (`MPI_Init_thread()` with the funneled level, which is what the
 * mailbox of the processes needs [see `MPIMailbox.h`], and
 * `MPI_Finalize()`), and `DistributedMin` is the minimum over the processes
 * of a value of each, e.g., the bound of a search that every process
 * improves, by a reduction that does not block (`MPI_Iallreduce`), which a
 * process starts and then tests while it works (see
 * \ref theory_distributed_messages).
 *
 * The contracts [see `Contract.h`]: MPI is initialized while the objects
 * live; the values of `DistributedMin` are arithmetic; every process of the
 * communicator starts the same number of reductions of a `DistributedMin`
 * (a collective operation), and destroys it after the last one completed.
 *
 * \see Message Passing Interface Forum, MPI: a message-passing interface
 *      standard, version 4.1, 2023, https://www.mpi-forum.org/docs/
 * \see T. Hoefler, A. Lumsdaine, W. Rehm, Implementation and performance
 *      analysis of non-blocking collective operations for MPI, ACM/IEEE
 *      Conference on Supercomputing (SC), 1-10, 2007,
 *      doi:10.1145/1362622.1362692
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

#ifndef __ADS_MPIExecutor
 #define __ADS_MPIExecutor
                      /* self-identification: #endif constant */

#if defined( ADS_WITH_MPI )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

// the C interface of MPI only: the bindings of C++, removed from the
// standard, do not compile cleanly
#ifndef OMPI_SKIP_MPICXX
 #define OMPI_SKIP_MPICXX 1
#endif
#ifndef MPICH_SKIP_MPICXX
 #define MPICH_SKIP_MPICXX 1
#endif
#include <mpi.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// throws if the call of MPI named failed
inline void mpiCheck( int code , const char * what )
{
 if( code != MPI_SUCCESS )
  throw( std::runtime_error( std::string( "MPI: " ) + what + " failed" ) );
 }

/// the type of MPI of the arithmetic type T
template< class T >
MPI_Datatype mpiType( void )
{
 static_assert( std::is_arithmetic_v< T > , "an arithmetic type" );
 if constexpr( std::is_floating_point_v< T > ) {
  if constexpr( sizeof( T ) == sizeof( float ) )
   return( MPI_FLOAT );
  else if constexpr( sizeof( T ) == sizeof( double ) )
   return( MPI_DOUBLE );
  else
   return( MPI_LONG_DOUBLE );
  }
 else if constexpr( std::is_signed_v< T > ) {
  if constexpr( sizeof( T ) == 1 )
   return( MPI_INT8_T );
  else if constexpr( sizeof( T ) == 2 )
   return( MPI_INT16_T );
  else if constexpr( sizeof( T ) == 4 )
   return( MPI_INT32_T );
  else
   return( MPI_INT64_T );
  }
 else {
  if constexpr( sizeof( T ) == 1 )
   return( MPI_UINT8_T );
  else if constexpr( sizeof( T ) == 2 )
   return( MPI_UINT16_T );
  else if constexpr( sizeof( T ) == 4 )
   return( MPI_UINT32_T );
  else
   return( MPI_UINT64_T );
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS MPIEnvironment --------------------------*/
/*--------------------------------------------------------------------------*/

/// MPI started for the life of the object, and ended with it
class MPIEnvironment {
 public:

 /// MPI started by `MPI_Init_thread()` at the level `level` (funneled by
 /// default), unless it already was
 MPIEnvironment( int & argc , char ** & argv ,
                 int level = MPI_THREAD_FUNNELED ) {
  int done = 0;
  MPI_Initialized( &done );
  if( done )
   return;
  detail::mpiCheck( MPI_Init_thread( &argc , &argv , level , &f_level ) ,
                    "MPI_Init_thread" );
  f_owner = true;
  }
 /// no copy, since MPI ends once
 MPIEnvironment( const MPIEnvironment & ) = delete;
 /// no copy, since MPI ends once
 MPIEnvironment & operator=( const MPIEnvironment & ) = delete;
 /// MPI ended by `MPI_Finalize()`, if this object started it
 ~MPIEnvironment() {
  if( f_owner )
   MPI_Finalize();
  }

 /// the level of threads that MPI provides, if this object started it
 [[nodiscard]] int level( void ) const { return( f_level ); }

 private:

 bool f_owner = false;                     ///< started MPI
 int f_level = MPI_THREAD_SINGLE;          ///< the level provided
 };  // end( class MPIEnvironment )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MPIExecutor ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the processes of a communicator, one worker each
class MPIExecutor {
 public:

 /// the executor of the processes of `comm`
 /// \pre MPI is initialized
 explicit MPIExecutor( MPI_Comm comm = MPI_COMM_WORLD ) : f_comm( comm ) {
  int done = 0;
  MPI_Initialized( &done );
  ADS_PRE( done );
  detail::mpiCheck( MPI_Comm_rank( comm , &f_rank ) , "MPI_Comm_rank" );
  detail::mpiCheck( MPI_Comm_size( comm , &f_size ) , "MPI_Comm_size" );
  }

 /// the rank of the process
 [[nodiscard]] std::size_t rank( void ) const {
  return( std::size_t( f_rank ) );
  }

 /// the number of the processes
 [[nodiscard]] std::size_t size( void ) const {
  return( std::size_t( f_size ) );
  }

 /// the communicator
 [[nodiscard]] MPI_Comm comm( void ) const { return( f_comm ); }

 /// the call `f( rank )`, the worker of this process; every process calls
 /// it
 template< class F >
 void run( F && f ) const { f( rank() ); }

 /// returns when all the processes have called it
 void barrier( void ) const {
  detail::mpiCheck( MPI_Barrier( f_comm ) , "MPI_Barrier" );
  }

 private:

 MPI_Comm f_comm;                          ///< the processes
 int f_rank = 0;                           ///< this one
 int f_size = 1;                           ///< how many
 };  // end( class MPIExecutor )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DistributedMin --------------------------*/
/*--------------------------------------------------------------------------*/

/// the minimum over the processes of a value of each, by reductions that
/// do not block
template< class T >
class DistributedMin {
 public:

 /// the minima over the processes of `comm`
 explicit DistributedMin( MPI_Comm comm = MPI_COMM_WORLD )
  : f_comm( comm ) {}
 /// no copy, since MPI writes into the object
 DistributedMin( const DistributedMin & ) = delete;
 /// no copy, since MPI writes into the object
 DistributedMin & operator=( const DistributedMin & ) = delete;
 /// waits for the reduction in flight, if any
 ~DistributedMin() {
  if( f_flight )
   MPI_Wait( &f_req , MPI_STATUS_IGNORE );
  }

 /// starts the reduction of the value \f$ v \f$ of this process, unless one
 /// is in flight, and tells whether it did
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{start}( v ) \cr
 /// 2 & \quad \textbf{if}\ \text{a reduction is in flight}\ \textbf{then}
 ///     \ \textbf{return false} \cr
 /// 3 & \quad \mathit{in} \gets v;\ \mathrm{MPI\_Iallreduce}( \mathit{in} ,
 ///     \mathit{out} , \min );\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool start( T v ) {
  if( f_flight )
   return( false );
  f_in = v;
  detail::mpiCheck( MPI_Iallreduce( &f_in , &f_out , 1 ,
                                    detail::mpiType< T >() , MPI_MIN ,
                                    f_comm , &f_req ) , "MPI_Iallreduce" );
  f_flight = true;
  return( true );
  }

 /// the minimum of the last reduction if it has completed, nothing if it
 /// is still in flight or none was started
 [[nodiscard]] std::optional< T > test( void ) {
  if( ! f_flight )
   return( std::nullopt );
  int flag = 0;
  detail::mpiCheck( MPI_Test( &f_req , &flag , MPI_STATUS_IGNORE ) ,
                    "MPI_Test" );
  if( ! flag )
   return( std::nullopt );
  f_flight = false;
  return( f_out );
  }

 /// the minimum of the reduction in flight, waiting for it
 /// \pre a reduction is in flight
 T wait( void ) {
  ADS_PRE( f_flight );
  detail::mpiCheck( MPI_Wait( &f_req , MPI_STATUS_IGNORE ) , "MPI_Wait" );
  f_flight = false;
  return( f_out );
  }

 /// whether a reduction is in flight
 [[nodiscard]] bool inFlight( void ) const { return( f_flight ); }

 private:

 MPI_Comm f_comm;                          ///< the processes
 MPI_Request f_req = MPI_REQUEST_NULL;     ///< the reduction in flight
 T f_in{};                                 ///< its value here
 T f_out{};                                ///< its minimum
 bool f_flight = false;                    ///< whether one is in flight
 };  // end( class DistributedMin )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif  /* ADS_WITH_MPI */

/*--------------------------------------------------------------------------*/

#endif  /* MPIExecutor.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MPIExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
