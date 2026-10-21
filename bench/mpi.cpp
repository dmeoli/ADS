/*--------------------------------------------------------------------------*/
/*------------------------------ File mpi.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the sort over the processes of MPI [see
 * DistributedSort.h], to be run by mpirun on P processes: n items in all,
 * n / P in each process, random 64-bit integers or few distinct ones,
 * sorted by distributed::sort() with one thread per process, or with a
 * pool of t threads per process (-t, the hybrid scheme). A run starts
 * after a barrier, and its time is the largest among the processes; each
 * cell is the median of r runs, in nanoseconds per item, the copy of the
 * input excluded, and its benchmark "mpi sort P<P> t<t>". The first run
 * of each cell is not timed, and the row "A/A" repeats the cell. Rank 0
 * prints the lines.
 *
 *   mpirun -np P mpi [ -r runs ] [ -s seed ] [ -t threads ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/distributed/DistributedSort.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------------- MAIN -----------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 int provided = 0;
 MPI_Init_thread( &argc , &argv , MPI_THREAD_FUNNELED , &provided );
 int P = 1 , me = 0;
 MPI_Comm_size( MPI_COMM_WORLD , &P );
 MPI_Comm_rank( MPI_COMM_WORLD , &me );
 int threads = 1;
 std::vector< char * > rest{ argv[ 0 ] };
 for( int i = 1 ; i < argc ; ++i )
  if( ( std::string( argv[ i ] ) == "-t" ) && ( i + 1 < argc ) )
   threads = std::max( std::atoi( argv[ ++i ] ) , 1 );
  else
   rest.push_back( argv[ i ] );
 ADS::bench::Options o( int( rest.size() ) , rest.data() ,
                        { 10000000 } );
 ADS::ThreadPool pool{ std::size_t( threads ) };
 const std::string b = "mpi sort P" + std::to_string( P ) + " t" +
                       std::to_string( threads );
 for( std::size_t n : o.sizes )
  for( int pattern = 0 ; pattern < 2 ; ++pattern ) {
   std::mt19937_64 g( o.seed + 1000003u * unsigned( me ) );
   const std::size_t p = std::size_t( P ) , m = std::size_t( me );
   std::vector< std::uint64_t > A( n / p + ( m < n % p ) );
   for( auto & x : A )
    x = pattern ? g() % 16 : g();
   auto cell = [ & ]( void ) {
    std::vector< double > t;
    for( int i = 0 ; i <= o.runs ; ++i ) {
     std::vector< std::uint64_t > B = A;
     MPI_Barrier( MPI_COMM_WORLD );
     const double a = MPI_Wtime();
     if( threads == 1 )
      ADS::distributed::sort( MPI_COMM_WORLD , B );
     else
      ADS::distributed::sort( pool , MPI_COMM_WORLD , B );
     double d = MPI_Wtime() - a , m = 0;
     MPI_Reduce( &d , &m , 1 , MPI_DOUBLE , MPI_MAX , 0 , MPI_COMM_WORLD );
     if( i > 0 )                            // the first one not timed
      t.push_back( m * 1e9 / double( std::max< std::size_t >( n , 1 ) ) );
     }
    std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
    return( t[ t.size() / 2 ] );
    };
   const char * c = pattern ? "few" : "random";
   const double x = cell() , y = cell();
   if( me == 0 ) {
    ADS::bench::print( b.c_str() , c , n , "ADS::distributed::sort" , x );
    ADS::bench::print( b.c_str() , c , n , "ADS::distributed::sort A/A" ,
                       y );
    }
   }
 MPI_Finalize();
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File mpi.cpp ----------------------------*/
/*--------------------------------------------------------------------------*/
