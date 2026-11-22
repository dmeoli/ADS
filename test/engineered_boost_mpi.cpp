/*--------------------------------------------------------------------------*/
/*---------------------- File engineered_boost_mpi.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the processes of MPI in a program of Boost.MPI, beyond the book,
 * run on 1 to 4 processes by `mpirun` (not in tests.txt, as those of
 * `engineered_mpi.cpp`): the environment of Boost.MPI at the funneled level
 * in place of `MPIEnvironment`; `MPIExecutor` and `distributed::sort()` on a
 * `boost::mpi::communicator`, which converts to the communicator of MPI;
 * and `MPIMailbox` on messages serialized by Boost.Serialization through
 * `BoostMPICodec`, every message received once and equal to the one sent.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/distributed/BoostMPI.h"
#include "ADS/distributed/DistributedSort.h"
#include "ADS/distributed/MPIExecutor.h"
#include "ADS/distributed/MPIMailbox.h"
#include "ADS/parallel/ThreadPool.h"

#include <boost/mpi/collectives.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace
{

/// a message that Boost.Serialization writes: a name and some numbers
struct Note
{
 std::string name;
 std::vector< double > values;

 template< class Archive >
 void serialize( Archive & a , const unsigned ) {
  a & name;
  a & values;
  }

 bool operator==( const Note & ) const = default;
 };

/// the note k of the process q
Note noteOf( std::size_t q , std::uint64_t k )
{
 Note x;
 x.name = std::string( std::size_t( k % 7 ) , char( 'a' + q % 26 ) ) + "#" +
	  std::to_string( q ) + ":" + std::to_string( k );
 for( std::uint64_t i = 0 ; i < k % 5 ; ++i )
  x.values.push_back( double( q ) + 0.25 * double( k + i ) );
 return( x );
 }

}  // end( namespace )

template<>
struct ADS::MPICodec< Note > : ADS::BoostMPICodec< Note >
{ };

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the executor and the sort on the communicator of Boost.MPI
ADS_TEST( "boost-mpi-sort" )
{
 boost::mpi::communicator world;
 MPIExecutor ex( world );
 CHECK( ex.size() == std::size_t( world.size() ) );
 CHECK( ex.rank() == std::size_t( world.rank() ) );
 ThreadPool pool( 2 );
 std::mt19937_64 g( 17 + unsigned( world.rank() ) );
 std::vector< std::uint64_t > A( 20000 + 500 * std::size_t( world.rank() ) );
 for( auto & x : A )
  x = g() % 100000;
 std::uint64_t before = 0;
 boost::mpi::all_reduce( world , std::uint64_t( A.size() ) , before ,
			 std::plus<>() );
 distributed::sort( pool , world , A );
 CHECK( std::is_sorted( A.begin() , A.end() ) );
 std::uint64_t n = 0;
 boost::mpi::all_reduce( world , std::uint64_t( A.size() ) , n ,
			 std::plus<>() );
 CHECK( n == before );
 // the last key of each process is not above the first of the next
 const std::uint64_t last = A.empty() ? 0 : A.back();
 std::vector< std::uint64_t > lasts;
 boost::mpi::all_gather( world , last , lasts );
 std::vector< int > empty;
 boost::mpi::all_gather( world , int( A.empty() ) , empty );
 if( ! A.empty() )
  for( int q = 0 ; q < world.rank() ; ++q )
   CHECK( empty[ std::size_t( q ) ] ||
	  ( lasts[ std::size_t( q ) ] <= A.front() ) );
 }

// notes of Boost.Serialization through the mailbox, by BoostMPICodec
ADS_TEST( "boost-mpi-codec" )
{
 boost::mpi::communicator world;
 MPIExecutor ex( world );
 const std::size_t P = ex.size() , me = ex.rank();
 const std::uint64_t N = 300;
 std::uint64_t expect = 0;
 for( std::size_t q = 0 ; q < P ; ++q )
  for( std::uint64_t k = 0 ; k < N ; ++k )
   if( ( q + k ) % P == me )
    ++expect;
 MPIMailbox< Note > mb( ex.comm() , 64 );
 std::uint64_t got = 0;
 bool bad = false;
 auto take = [ & ]( Note && x ) {
  ++got;
  const std::size_t at = x.name.find( '#' ) , colon = x.name.find( ':' );
  const std::size_t q = std::stoull( x.name.substr( at + 1 , colon - at ) );
  const std::uint64_t k = std::stoull( x.name.substr( colon + 1 ) );
  if( ! ( x == noteOf( q , k ) ) )
   bad = true;
  };
 for( std::uint64_t k = 0 ; k < N ; ++k ) {
  mb.send( ( me + k ) % P , noteOf( me , k ) );
  if( k % 13 == 0 )
   mb.poll( take );
  }
 while( ! mb.quiescent() )
  mb.poll( take );
 CHECK( ! bad );
 CHECK( got == expect );
 CHECK( mb.leftovers( take ) == 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 boost::mpi::environment env( argc , argv , boost::mpi::threading::funneled );
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------- End File engineered_boost_mpi.cpp -------------------*/
/*--------------------------------------------------------------------------*/
