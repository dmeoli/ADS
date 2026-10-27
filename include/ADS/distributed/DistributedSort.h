/*--------------------------------------------------------------------------*/
/*------------------------- File DistributedSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of the items spread over the processes of MPI, beyond the book,
 * compiled only with ADS_WITH_MPI (the CMake option of the same name, OFF
 * by default) and an implementation of MPI (Open MPI or MPICH): each of
 * the P processes of a communicator holds a vector of items, and after
 * sort( comm , A , less ) each holds a sorted vector, those of the ranks
 * 0 , ... , P - 1 in this order being the sorted items of all; the
 * vectors change their sizes. The items are copied as bytes (trivially
 * copyable). An executor [see Executor.h] may be given as well, and then
 * the work inside each process runs on its threads (MPI between the
 * nodes, threads within each, the hybrid scheme of the clusters of today).
 *
 * The sort is the parallel sort by regular sampling of Shi and Schaeffer,
 * in one round of communication: each process sorts its n_i items (by
 * sort() of ADS, or its parallel version on the executor), takes P - 1
 * samples at the ranks \f$ \lfloor ( j + 1 ) n_i / P \rfloor \f$, and all
 * the \f$ P ( P - 1 ) \f$ samples, gathered by MPI_Allgather and sorted,
 * give the P - 1 splitters at the ranks \f$ ( j + 1 ) ( P - 1 ) \f$; each
 * process cuts its sorted items at the splitters by binary searches (the
 * items equal to a run of k equal splitters are cut into k + 1 parts of
 * the same size, so that a key repeated many times is spread over as many
 * processes as it fills), sends each piece to its process by one
 * MPI_Alltoallv (after an MPI_Alltoall of the counts), and merges the P
 * sorted pieces it receives by multiwayMerge() [see MultiwayMerge.h and
 * ParallelMerge.h]. With the
 * same number of items in each process, no process receives more than
 * \f$ 2 n / P \f$ items (the bound of regular sampling, for distinct
 * keys); the work is \f$ O( ( n / P ) \log n ) \f$ per process, the
 * volume of communication \f$ O( n / P + P^2 ) \f$ per process in 3
 * collective operations, i.e., \f$ O( \alpha P + \beta n / P ) \f$ time
 * for latency \f$ \alpha \f$ and time \f$ \beta \f$ per byte with a
 * direct all-to-all. Beyond some thousands of processes the \f$ P^2 \f$
 * samples and the \f$ P \f$ messages of each process dominate, and the
 * multi-level sorts of Axtmann, Bingmann, Sanders and Schulz (which group
 * the processes and exchange in \f$ O( \log P ) \f$ levels) take over.
 *
 * The contracts [see Contract.h]: T is trivially copyable; less is a
 * strict weak order, the same in every process; every process of comm
 * calls sort() (a collective operation); the local result is sorted (an
 * audit).
 * \see H. Shi, J. Schaeffer, Parallel sorting by regular sampling, Journal
 *      of Parallel and Distributed Computing 14(4), 361-372, 1992,
 *      doi:10.1016/0743-7315(92)90075-X
 * \see M. Axtmann, T. Bingmann, P. Sanders, C. Schulz, Practical massively
 *      parallel sorting, ACM Symposium on Parallelism in Algorithms and
 *      Architectures (SPAA), 13-23, 2015, doi:10.1145/2755573.2755595
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_DistributedSort
 #define __ADS_DistributedSort
                      /* self-identification: #endif constant */

#if defined( ADS_WITH_MPI )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelMerge.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/priority/MultiwayMerge.h"
#include "ADS/sort/Sort.h"

// the C interface of MPI only: the bindings of C++, removed from the
// standard, do not compile cleanly
#ifndef OMPI_SKIP_MPICXX
 #define OMPI_SKIP_MPICXX 1
#endif
#ifndef MPICH_SKIP_MPICXX
 #define MPICH_SKIP_MPICXX 1
#endif
#include <mpi.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS::distributed {

/// the items of A of all the processes of comm sorted by less, the
/// threads of ex sorting and merging within each process: afterwards A
/// holds the part of the process, the parts in the order of the ranks; it
/// throws std::length_error, in every process, if the bytes of a piece or
/// of the pieces a process receives exceed the counts of MPI (those of int
/// before MPI 4, whose MPI_Alltoallv_c counts them in 64 bits)
/// \post A is sorted (an audit)
template< Executor E , class T , class C = std::less<> >
void sort( E & ex , MPI_Comm comm , std::vector< T > & A , C less = C() );

/// the same, each process on one thread
template< class T , class C = std::less<> >
void sort( MPI_Comm comm , std::vector< T > & A , C less = C() )
{
 Sequential s;
 sort( s , comm , A , less );
 }

/*--------------------------------------------------------------------------*/

namespace detail {

/// throws if MPI did not succeed
inline void check( int code , const char * what )
{
 if( code != MPI_SUCCESS )
  throw( std::runtime_error( std::string( "ADS::distributed::sort: " ) +
                             what ) );
 }

// the counts and the displacements of the bytes in MPI_Alltoallv: those of
// MPI_Alltoallv_c, of 64 bits, with MPI 4, else those of int
#if defined( MPI_VERSION ) && ( MPI_VERSION >= 4 )
 #define ADS_MPI_LARGE_COUNTS
using Count = MPI_Count;
using Displacement = MPI_Aint;
#else
using Count = int;
using Displacement = int;
#endif

/// the type of MPI of Count
inline MPI_Datatype countType( void )
{
#ifdef ADS_MPI_LARGE_COUNTS
 return( MPI_COUNT );
#else
 return( MPI_INT );
#endif
 }

/// b bytes as a count or a displacement of type N of MPI, or 0 and ok
/// false if they do not fit in it
template< class N >
N fits( std::size_t b , bool & ok )
{
 if( b <= std::size_t( std::numeric_limits< N >::max() ) )
  return( N( b ) );
 ok = false;
 return( N( 0 ) );
 }

}  // end( namespace detail )

template< Executor E , class T , class C >
void sort( E & ex , MPI_Comm comm , std::vector< T > & A , C less )
{
 static_assert( std::is_trivially_copyable_v< T > ,
                "the items travel as bytes" );
 using detail::check;
 int P = 1 , me = 0;
 check( MPI_Comm_size( comm , &P ) , "MPI_Comm_size" );
 check( MPI_Comm_rank( comm , &me ) , "MPI_Comm_rank" );
 ADS::sort( ex , A , less );
 if( P == 1 )
  return;
 const std::size_t n = A.size() , p = std::size_t( P );
 // P - 1 regular samples of each process, and a flag for an empty one
 std::vector< T > mine( p - 1 );
 std::vector< int > has( p );
 const int full = n > 0;
 for( std::size_t j = 0 ; j + 1 < p && n ; ++j )
  mine[ j ] = A[ ( j + 1 ) * n / p ];
 check( MPI_Allgather( &full , 1 , MPI_INT , has.data() , 1 , MPI_INT ,
                       comm ) , "MPI_Allgather" );
 std::vector< T > all( p * ( p - 1 ) );
 bool ok = true;                            // all the bytes fit in MPI
 const int bytes = detail::fits< int >( ( p - 1 ) * sizeof( T ) , ok );
 if( ! ok )                                 // the same in every process
  throw( std::length_error( "ADS::distributed::sort: samples of more "
                            "bytes than MPI counts" ) );
 check( MPI_Allgather( mine.data() , bytes , MPI_BYTE , all.data() , bytes ,
                       MPI_BYTE , comm ) , "MPI_Allgather" );
 std::vector< T > S;                       // the samples of the full ones
 for( std::size_t q = 0 ; q < p ; ++q )
  if( has[ q ] )
   S.insert( S.end() , all.begin() + std::ptrdiff_t( q * ( p - 1 ) ) ,
             all.begin() + std::ptrdiff_t( ( q + 1 ) * ( p - 1 ) ) );
 ADS::sort( S , less );
 // the cuts: piece j of this process are its items in ( s_{j-1} , s_j ],
 // a run of splitters equal to x splitting the items equal to x evenly
 std::vector< std::size_t > cut( p + 1 , 0 );
 cut[ p ] = n;
 for( std::size_t j = 1 ; j < p ; ++j ) {
  if( S.empty() ) {
   cut[ j ] = n;
   continue;
   }
  const T & s = S[ std::min( S.size() - 1 , j * S.size() / p ) ];
  const auto lo = std::size_t( std::lower_bound( A.begin() , A.end() , s ,
                                                 less ) - A.begin() ) ,
             hi = std::size_t( std::upper_bound( A.begin() , A.end() , s ,
                                                 less ) - A.begin() );
  // the splitters equal to s: [ first , last ) of the indices j
  std::size_t first = j , last = j + 1;
  while( ( first > 1 ) &&
         ! less( S[ std::min( S.size() - 1 , ( first - 1 ) * S.size() /
                                             p ) ] , s ) )
   --first;
  while( ( last < p ) &&
         ! less( s , S[ std::min( S.size() - 1 , last * S.size() / p ) ] ) )
   ++last;
  // the equal items [ lo , hi ) shared among the cuts first .. last - 1
  cut[ j ] = lo + ( hi - lo ) * ( j - first + 1 ) / ( last - first + 1 );
  }
 for( std::size_t j = 1 ; j <= p ; ++j )
  cut[ j ] = std::max( cut[ j ] , cut[ j - 1 ] );
 using detail::Count;
 using detail::Displacement;
 using detail::fits;
 std::vector< Count > counts( p ) , got( p );
 for( std::size_t j = 0 ; j < p ; ++j )
  counts[ j ] = fits< Count >( ( cut[ j + 1 ] - cut[ j ] ) * sizeof( T ) ,
                               ok );
 check( MPI_Alltoall( counts.data() , 1 , detail::countType() , got.data() ,
                      1 , detail::countType() , comm ) , "MPI_Alltoall" );
 std::vector< Displacement > sd( p , 0 ) , rd( p , 0 );
 std::size_t total = 0;
 for( std::size_t j = 0 ; j < p ; ++j ) {
  sd[ j ] = fits< Displacement >( cut[ j ] * sizeof( T ) , ok );
  rd[ j ] = fits< Displacement >( total , ok );
  total += std::size_t( got[ j ] );
  }
 // a process whose bytes do not fit stops all of them, which would
 // otherwise wait for it in the exchange
 int fit = ok;
 check( MPI_Allreduce( MPI_IN_PLACE , &fit , 1 , MPI_INT , MPI_MIN , comm ) ,
        "MPI_Allreduce" );
 if( ! fit )
  throw( std::length_error( "ADS::distributed::sort: pieces of more bytes "
                            "than MPI counts" ) );
 std::vector< T > R( total / sizeof( T ) );
#ifdef ADS_MPI_LARGE_COUNTS
 check( MPI_Alltoallv_c( A.data() , counts.data() , sd.data() , MPI_BYTE ,
                         R.data() , got.data() , rd.data() , MPI_BYTE ,
                         comm ) , "MPI_Alltoallv_c" );
#else
 check( MPI_Alltoallv( A.data() , counts.data() , sd.data() , MPI_BYTE ,
                       R.data() , got.data() , rd.data() , MPI_BYTE ,
                       comm ) , "MPI_Alltoallv" );
#endif
 // the P sorted pieces received, merged
 std::vector< std::pair< const T * , const T * > > runs;
 for( std::size_t j = 0 ; j < p ; ++j )
  runs.emplace_back( R.data() + std::size_t( rd[ j ] ) / sizeof( T ) ,
                     R.data() + ( std::size_t( rd[ j ] ) +
                                  std::size_t( got[ j ] ) ) / sizeof( T ) );
 A.resize( R.size() );
 ADS::multiwayMerge( ex , runs , A.begin() , less );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::distributed )

#endif  /* ADS_WITH_MPI */

/*--------------------------------------------------------------------------*/

#endif  /* DistributedSort.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File DistributedSort.h ------------------------*/
/*--------------------------------------------------------------------------*/
