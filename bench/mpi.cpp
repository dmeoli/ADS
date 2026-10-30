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
 * prints the lines. With -m it measures instead the mailbox of the
 * processes [see MPIMailbox.h], benchmark "mpi mailbox P<P>": the round
 * trips of a message between the ranks 0 and 1 in batches of 1 (case
 * "round-trip", per round trip, n of them), a random tree of about n nodes
 * spread over the processes by hashing, until the detection of the
 * termination (case "tree", per node), and the time from a start with no
 * work to the moment all the processes see the termination (case "end",
 * in ns, the mailbox built before), each the median of r runs after one
 * not timed.
 *
 *   mpirun -np P mpi [ -r runs ] [ -s seed ] [ -t threads ] [ -m ] n1 ...
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

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/distributed/DistributedSort.h"
#include "ADS/distributed/MPIMailbox.h"
#include "ADS/parallel/ThreadPool.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <random>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- MAILBOX ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace {

/// the median of r runs of work() after one not timed, the time of each
/// the largest among the processes, in ns per op
template< class W >
double cellOf( W && work , int r , std::size_t ops )
{
 std::vector< double > t;
 for( int i = 0 ; i <= r ; ++i ) {
  MPI_Barrier( MPI_COMM_WORLD );
  const double a = MPI_Wtime();
  work();
  double d = MPI_Wtime() - a , m = 0;
  MPI_Allreduce( &d , &m , 1 , MPI_DOUBLE , MPI_MAX , MPI_COMM_WORLD );
  if( i > 0 )
   t.push_back( m * 1e9 / double( std::max< std::size_t >( ops , 1 ) ) );
  }
 std::nth_element( t.begin() , t.begin() + t.size() / 2 , t.end() );
 return( t[ t.size() / 2 ] );
 }

/// the mixing of 64 bits of SplitMix64
std::uint64_t mixOf( std::uint64_t x )
{
 x += 0x9E3779B97F4A7C15ull;
 x = ( x ^ ( x >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
 x = ( x ^ ( x >> 27 ) ) * 0x94D049BB133111EBull;
 return( x ^ ( x >> 31 ) );
 }

/// a node of the random tree
struct Node {
 std::uint64_t key;
 std::uint32_t depth;
 };

/// the cells of the mailbox, printed by the rank 0
void mailboxCells( const std::vector< std::size_t > & sizes , int r ,
                   unsigned seed )
{
 int P = 1 , me = 0;
 MPI_Comm_size( MPI_COMM_WORLD , &P );
 MPI_Comm_rank( MPI_COMM_WORLD , &me );
 const std::size_t p = std::size_t( P ) , m = std::size_t( me );
 const std::string b = "mpi mailbox P" + std::to_string( P );
 for( std::size_t n : sizes ) {
  // round trips between the ranks 0 and 1
  const std::size_t k = std::min< std::size_t >( n , 100000 );
  auto trips = [ & ]( void ) {
   ADS::MPIMailbox< std::uint64_t > mb( MPI_COMM_WORLD , 1 );
   std::uint64_t seen = 0;
   if( ( p >= 2 ) && ( m == 0 ) )
    mb.send( 1 , 0 );
   while( ( p >= 2 ) && ( m < 2 ) && ( seen < k ) )
    mb.poll( [ & ]( const std::uint64_t & x ) {
     ++seen;
     if( x + 1 < 2 * k )
      mb.send( 1 - m , x + 1 );
     } );
   while( ! mb.quiescent() )
    mb.poll( []( const std::uint64_t & ) {} );
   };
  // the random tree, 2 to 4 children per node above the depth
  std::uint32_t d = 0;
  for( std::size_t q = 1 ; q * 3 < n ; q *= 3 )
   ++d;
  auto children = [ d ]( const Node & v , auto && f ) {
   if( v.depth >= d )
    return;
   const std::uint64_t h = mixOf( v.key );
   for( unsigned j = 0 ; j < 2 + unsigned( h % 3 ) ; ++j )
    f( Node{ mixOf( h + j ) , v.depth + 1 } );
   };
  std::size_t size = 0;
  {
   std::vector< Node > stack{ Node{ seed , 0 } };
   while( ! stack.empty() ) {
    const Node v = stack.back();
    stack.pop_back();
    ++size;
    children( v , [ & ]( Node w ) { stack.push_back( w ); } );
    }
   }
  auto tree = [ & ]( void ) {
   ADS::MPIMailbox< Node > mb( MPI_COMM_WORLD , 256 );
   std::vector< Node > stack;
   if( ADS::ownerOf( std::uint64_t( seed ) , p ) == m )
    stack.push_back( Node{ seed , 0 } );
   for( ;; ) {
    mb.poll( [ & ]( const Node & v ) { stack.push_back( v ); } );
    if( ! stack.empty() ) {
     for( int j = 0 ; ( j < 64 ) && ! stack.empty() ; ++j ) {
      const Node v = stack.back();
      stack.pop_back();
      children( v , [ & ]( Node w ) {
       const std::size_t o = ADS::ownerOf( w.key , p );
       if( o == m )
        stack.push_back( w );
       else
        mb.send( o , w );
       } );
      }
     continue;
     }
    if( mb.quiescent() )
     break;
    }
   };
  // no work: the waves alone, the mailboxes built before (each one
  // duplicates the communicator)
  std::vector< std::unique_ptr< ADS::MPIMailbox< std::uint64_t > > > boxes;
  for( int i = 0 ; i < 2 * ( r + 1 ) ; ++i )
   boxes.push_back( std::make_unique< ADS::MPIMailbox< std::uint64_t > >(
                                                          MPI_COMM_WORLD ) );
  std::size_t used = 0;
  auto end = [ & ]( void ) {
   auto & mb = *boxes[ used++ ];
   while( ! mb.quiescent() )
    ;
   };
  const double t1 = cellOf( trips , r , k ) , t2 = cellOf( trips , r , k );
  const double u1 = cellOf( tree , r , size ) ,
               u2 = cellOf( tree , r , size );
  const double e1 = cellOf( end , r , 1 ) , e2 = cellOf( end , r , 1 );
  if( me == 0 ) {
   if( p >= 2 ) {
    ADS::bench::print( b.c_str() , "round-trip" , k , "MPIMailbox" , t1 );
    ADS::bench::print( b.c_str() , "round-trip" , k , "MPIMailbox A/A" ,
                       t2 );
    }
   ADS::bench::print( b.c_str() , "tree" , n , "MPIMailbox" , u1 );
   ADS::bench::print( b.c_str() , "tree" , n , "MPIMailbox A/A" , u2 );
   ADS::bench::print( b.c_str() , "end" , p , "MPIMailbox" , e1 );
   ADS::bench::print( b.c_str() , "end" , p , "MPIMailbox A/A" , e2 );
   }
  }
 }

}  // end( namespace )

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
 bool mailbox = false;
 std::vector< char * > rest{ argv[ 0 ] };
 for( int i = 1 ; i < argc ; ++i )
  if( ( std::string( argv[ i ] ) == "-t" ) && ( i + 1 < argc ) )
   threads = std::max( std::atoi( argv[ ++i ] ) , 1 );
  else if( std::string( argv[ i ] ) == "-m" )
   mailbox = true;
  else
   rest.push_back( argv[ i ] );
 ADS::bench::Options o( int( rest.size() ) , rest.data() ,
                        { mailbox ? std::size_t( 1000000 ) :
                                    std::size_t( 10000000 ) } );
 if( mailbox ) {
  mailboxCells( o.sizes , o.runs , o.seed );
  MPI_Finalize();
  return( 0 );
  }
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
