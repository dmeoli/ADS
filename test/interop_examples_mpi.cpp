/*--------------------------------------------------------------------------*/
/*---------------------- File interop_examples_mpi.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The examples of MPI and of Boost.MPI of section C.13 of the manual, run
 * over the processes of MPI, as interop_examples.cpp runs the others: each
 * test holds the code of one example as the manual prints it (`declcheck.py
 * --tests` checks that it does), followed by the checks of its result. The
 * program starts MPI at the funneled level, as the program of the examples
 * does; the test of Boost.MPI passes with nothing to check without
 * `ADS_WITH_BOOST_MPI`, and says so.
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

#include "ADS/distributed/DistributedSort.h"
#include "ADS/distributed/MPIExecutor.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <random>
#include <vector>

#if defined( ADS_WITH_BOOST_MPI )
 #include "ADS/distributed/BoostMPI.h"

 #include <boost/mpi/communicator.hpp>
 #include <boost/mpi/environment.hpp>
#endif

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace
{

/// whether A is sorted, and no item of the process r exceeds one of the
/// process r + 1 of comm
bool globallySorted( const std::vector< int > & A , MPI_Comm comm )
{
 int rank = 0 , size = 1;
 MPI_Comm_rank( comm , &rank );
 MPI_Comm_size( comm , &size );
 int ok = std::is_sorted( A.begin() , A.end() ) ? 1 : 0;
 // the largest item of each process, or the smallest int if it has none
 int last = A.empty() ? std::numeric_limits< int >::min() : A.back();
 std::vector< int > lasts( static_cast< std::size_t >( size ) );
 MPI_Allgather( &last , 1 , MPI_INT , lasts.data() , 1 , MPI_INT , comm );
 for( int q = 0 ; q < rank ; ++q )
  if( ! A.empty() && ( lasts[ std::size_t( q ) ] > A.front() ) )
   ok = 0;
 int all = 0;
 MPI_Allreduce( &ok , &all , 1 , MPI_INT , MPI_MIN , comm );
 return( all == 1 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// section C.13: the sort of ADS over a communicator of the program
ADS_TEST( "mpi-sort" )
{
 MPI_Comm comm;  // a communicator of the program
 MPI_Comm_dup( MPI_COMM_WORLD , &comm );
 const ADS::MPIExecutor ex( comm );
 std::vector< int > A( 1000 );
 std::mt19937 r( static_cast< unsigned >( ex.rank() ) );
 for( int & x : A )
  x = int( r() % 100000 );
 ADS::ThreadPool pool( 2 );  // the threads of each process
 ADS::distributed::sort( pool , comm , A );
 CHECK( globallySorted( A , comm ) );
 MPI_Comm_free( &comm );
 }

// section C.13: the same sort in a program of Boost.MPI
ADS_TEST( "boost-mpi-sort" )
{
#if defined( ADS_WITH_BOOST_MPI )
 boost::mpi::environment env( boost::mpi::threading::funneled );
 boost::mpi::communicator world;
 std::vector< int > A( 1000 );
 std::mt19937 r( static_cast< unsigned >( world.rank() ) );
 for( int & x : A )
  x = int( r() % 100000 );
 ADS::ThreadPool pool( 2 );
 ADS::distributed::sort( pool , world , A );  // world is an MPI_Comm
 CHECK( globallySorted( A , world ) );
#else
 std::cout << "  (Boost.MPI not compiled in: nothing to check)" << std::endl;
#endif
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 const ADS::MPIEnvironment env( argc , argv );
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File interop_examples_mpi.cpp --------------------*/
/*--------------------------------------------------------------------------*/
