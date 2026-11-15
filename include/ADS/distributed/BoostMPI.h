/*--------------------------------------------------------------------------*/
/*---------------------------- File BoostMPI.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The processes of MPI of a program of Boost.MPI, beyond the book, compiled
 * only with `ADS_WITH_MPI` and the headers of Boost.MPI: the executor, the
 * sort and the mailbox of the processes [see `MPIExecutor.h`,
 * `DistributedSort.h` and `MPIMailbox.h`] take the communicator of MPI
 * (`MPI_Comm`), to which a `boost::mpi::communicator` converts by itself,
 * so that `MPIExecutor ex( world )` and
 * `distributed::sort( pool , world , A )` need nothing more; the
 * environment of Boost.MPI takes the place of `MPIEnvironment` when it asks
 * for the funneled level (`boost::mpi::threading::funneled`), the one that
 * the mailbox needs. What the header adds is `BoostMPICodec< Msg >`, the
 * bytes of a message by Boost.Serialization through the packed archives of
 * Boost.MPI (whose representation is that of `MPI_Pack()`, and therefore
 * holds between processes of different machines), so that a message that
 * a program of Boost.MPI already serializes goes through `MPIMailbox` by one
 * specialization of `MPICodec`:
 *
 *     template<> struct ADS::MPICodec< Msg > : ADS::BoostMPICodec< Msg > { };
 *
 * The program links Boost.MPI and Boost.Serialization (the CMake target
 * `ADS::BoostMPI` does).
 *
 * The contracts [see `Contract.h`]: MPI is initialized while the codec is
 * used; \f$ \mathit{Msg} \f$ is default constructible and serializable by
 * Boost.Serialization.
 *
 * \see D. Gregor, M. Troyer, Boost.MPI, https://www.boost.org/libs/mpi
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

#ifndef __ADS_BoostMPI
 #define __ADS_BoostMPI
/* self-identification: #endif constant */

#if defined( ADS_WITH_MPI ) &&                                                \
  ( defined( ADS_WITH_BOOST_MPI ) || __has_include( <boost/mpi.hpp> ) )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/distributed/MPIExecutor.h"
#include "ADS/distributed/MPIMailbox.h"

#include <boost/mpi/communicator.hpp>
#include <boost/mpi/packed_iarchive.hpp>
#include <boost/mpi/packed_oarchive.hpp>

#include <concepts>
#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BoostMPICodec ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the bytes of a message by Boost.Serialization, in the packed archives of
/// Boost.MPI, for `MPICodec< Msg >`
template< std::default_initializable Msg >
struct BoostMPICodec
{
 /// the bytes of the packed archive of \f$ x \f$ appended to `out`
 static void encode( const Msg & x , std::vector< std::byte > & out ) {
  boost::mpi::communicator world;
  boost::mpi::packed_oarchive::buffer_type buffer;
  boost::mpi::packed_oarchive archive( world , buffer );
  archive << x;
  const std::size_t at = out.size();
  out.resize( at + buffer.size() );
  if( ! buffer.empty() )
   std::memcpy( out.data() + at , buffer.data() , buffer.size() );
  }

 /// the message of the bytes \f$ b \f$ of a packed archive
 static Msg decode( std::span< const std::byte > b ) {
  boost::mpi::communicator world;
  boost::mpi::packed_iarchive::buffer_type buffer( b.size() );
  if( ! b.empty() )
   std::memcpy( buffer.data() , b.data() , b.size() );
  boost::mpi::packed_iarchive archive( world , buffer );
  Msg x;
  archive >> x;
  return( x );
  }
 };  // end( struct BoostMPICodec )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* ADS_WITH_MPI and Boost.MPI */

/*--------------------------------------------------------------------------*/

#endif /* BoostMPI.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BoostMPI.h ---------------------------*/
/*--------------------------------------------------------------------------*/
