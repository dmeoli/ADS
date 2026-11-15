/*--------------------------------------------------------------------------*/
/*------------------------- File KokkosExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on an execution space of the host of
 * Kokkos, beyond the book: `KokkosExecutor< Space >` runs `bulk( n , f )` as
 * a `Kokkos::parallel_for` of a `RangePolicy` of the space on the
 * \f$ n \f$ indices with chunks of 1, followed by the `fence()` of the
 * space, so that the parallel algorithms of ADS run on the threads that a
 * program of Kokkos already has (those of its backend of OpenMP, of
 * `std::thread` or of HPX), and `concurrency()` is that of the space. The
 * space is `Kokkos::DefaultHostExecutionSpace` by default; the spaces of
 * the devices do not fit, since the calls of ADS run code of the host on
 * memory of the host. Kokkos must be initialized (`Kokkos::initialize()` or
 * a `Kokkos::ScopeGuard`) while the executor is used.
 *
 * A dispatch of Kokkos does not nest: on its backend of OpenMP it takes a
 * lock of the instance for the whole kernel and aborts beyond the levels
 * that it supports, and a dispatch from inside a kernel of the space is not
 * allowed. A call of `bulk()` made inside a call of the executor, or inside
 * a kernel of the program on the space (`in_parallel()`), therefore runs
 * its calls in order on that thread [see `detail::InsideCall`], as for
 * the senders [see `SenderExecutor.h`]: the parallelism of the nested calls
 * is lost, that of the loops, the reductions, the prefix sums and the top
 * level of the sorts is kept. There is no `run()`.
 *
 * The header is compiled where the headers of Kokkos are found
 * (`<Kokkos_Core.hpp>`, or `ADS_WITH_KOKKOS`, which the CMake target
 * `ADS::Kokkos` defines, with the flags that Kokkos asks of the compiler).
 *
 * The contracts [see `Contract.h`]: an exception thrown by \f$ f \f$ stops
 * the claims and is thrown again by `bulk()`, since an exception may not
 * leave a kernel of Kokkos.
 *
 * \see C. R. Trott, D. Lebrun-Grandié, D. Arndt, J. Ciesko, V. Dang,
 *      N. Ellingwood, R. Gayatri, E. Harvey, D. S. Hollman, D. Ibanez,
 *      N. Liber, J. Madsen, J. Miles, D. Poliakoff, A. Powell,
 *      S. Rajamanickam, M. Simberg, D. Sunderland, B. Turcksin, J. Wilke,
 *      Kokkos 3: programming model extensions for the exascale era, IEEE
 *      Transactions on Parallel and Distributed Systems 33(4), 805-817,
 *      2022, doi:10.1109/TPDS.2021.3097283
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

#ifndef __ADS_KokkosExecutor
 #define __ADS_KokkosExecutor
/* self-identification: #endif constant */

#if defined( ADS_WITH_KOKKOS ) || __has_include( <Kokkos_Core.hpp> )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <Kokkos_Core.hpp>

#include <cstddef>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS KokkosExecutor ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on an execution space of the host of Kokkos
template< class Space = Kokkos::DefaultHostExecutionSpace >
class KokkosExecutor
{

/*--------------------------------------------------------------------------*/
 public:
 /// the executor on the instance \f$ s \f$ of the space (the default one if
 /// none is given)
 explicit KokkosExecutor( Space s = Space() ) : f_s( std::move( s ) ) { }

 /// the concurrency of the space
 [[nodiscard]] std::size_t concurrency( void ) const {
  const int c = f_s.concurrency();
  return( std::size_t( c > 0 ? c : 1 ) );
  }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ by a `parallel_for` of
 /// the space, in order on the calling thread inside a call
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{or}
 ///     \ \text{inside a call or a kernel of the space}\ \textbf{then} \cr
 /// 3 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}
 ///     \ f( i ) \cr
 /// 4 & \quad \textbf{else}\ \mathrm{parallelFor}( \mathrm{RangePolicy}(
 ///     0 , n , \text{chunks of 1} ) , i \mapsto f( i ) \text{ marked }
 ///     \text{inside} );\ \mathrm{fence}() \cr
 /// 5 & \quad \text{throw again the first exception of the calls, if any}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( concurrency() == 1 ) || Inside::active() ||
      inKernel( f_s ) ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  detail::FirstError error;
  auto * pe = &error;
  auto * pf = &f;
  Kokkos::parallel_for(
    "ADS::KokkosExecutor::bulk" ,
    Kokkos::RangePolicy< Space , Kokkos::Schedule< Kokkos::Dynamic > ,
			 Kokkos::IndexType< std::size_t > >( f_s , 0 , n )
      .set_chunk_size( 1 ) ,
    [ = ]( std::size_t i ) {
     if( pe->failed() )
      return;
     Inside mark;
     try {
      ( *pf )( i );
      }
     catch( ... ) {
      pe->keep();
      }
     } );
  f_s.fence( "ADS::KokkosExecutor::bulk" );
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:
 struct Tag;  ///< the tag of the mark of the calls
 using Inside = detail::InsideCall< Tag >;

 /// whether the calling thread runs a kernel of the space \f$ s \f$: its
 /// `in_parallel()`, which takes the instance on OpenMP, none on Serial,
 /// and which the releases 4 of Kokkos deprecate; false on a space that
 /// has none, whose kernels then run the calls of `bulk()` in order only
 /// if it has one thread
 static bool inKernel( [[maybe_unused]] const Space & s ) {
#if defined( __GNUC__ )
 #pragma GCC diagnostic push
 #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
  if constexpr( requires { Space::in_parallel( s ); } )
   return( bool( Space::in_parallel( s ) ) );
  else if constexpr( requires { Space::in_parallel(); } )
   return( bool( Space::in_parallel() ) );
  else
   return( false );
#if defined( __GNUC__ )
 #pragma GCC diagnostic pop
#endif
  }

 Space f_s;  ///< the instance of the space
 };  // end( class KokkosExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* ADS_WITH_KOKKOS */

/*--------------------------------------------------------------------------*/

#endif /* KokkosExecutor.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File KokkosExecutor.h ------------------------*/
/*--------------------------------------------------------------------------*/
