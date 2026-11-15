/*--------------------------------------------------------------------------*/
/*------------------------- File StdParExecutor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The executor [see `Executor.h`] on the parallel algorithms of the standard
 * library, beyond the book: `StdParExecutor< Policy >` runs
 * `bulk( n , f )` as `std::for_each( Policy() , first , last , g )` on the
 * indices \f$ 0 , \ldots , n - 1 \f$, so that the threads are those of the
 * backend of the vendor (oneTBB under libstdc++, the pool of MSVC, the
 * backends of libc++, the threads of `nvc++ -stdpar=multicore`), and a
 * program that runs only on the standard library does not start the pool of
 * ADS beside them. The policy is `std::execution::par` by default; `par_unseq`
 * is not allowed, since the calls of ADS take locks and wait for each other,
 * which a vectorized policy forbids. The indices come from an iterator of
 * ADS, `detail::IndexIterator`, since those of `std::views::iota` are
 * random-access iterators of C++20 but only input iterators of C++17, and
 * the parallel algorithms ask for forward ones. The standard has no query of
 * the threads of a policy, so that `concurrency()` is the number given to
 * the constructor, the hardware threads by default. The header is compiled
 * where the standard library has the parallel algorithms
 * (`__cpp_lib_parallel_algorithm`); with libstdc++ and oneTBB on the path
 * the program links oneTBB (`-ltbb`), which the CMake target `ADS::StdPar`
 * does.
 *
 * Nothing is added to the call of the standard library: the calls of `bulk()`
 * are tasks of the backend, and a call nested in another is a nested
 * `for_each`, which the backends run as they run their own (oneTBB as its
 * tasks, the others possibly in order on the calling thread). There is no
 * `run()`, since the standard does not promise that the calls of a parallel
 * algorithm run at the same time.
 *
 * The contracts [see `Contract.h`]: \f$ p \ge 1 \f$; an exception thrown by
 * \f$ f \f$ stops the claims and is thrown again by `bulk()`, since the
 * standard library calls `std::terminate()` on an exception that leaves the
 * function of a parallel algorithm.
 *
 * \see ISO/IEC 14882:2020, Programming languages, C++, section 25.3
 *      [algorithms.parallel]
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

#ifndef __ADS_StdParExecutor
 #define __ADS_StdParExecutor
/* self-identification: #endif constant */

#include <version>

#if defined( __cpp_lib_parallel_algorithm ) || defined( ADS_WITH_STDPAR )

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <execution>
#include <iterator>
#include <thread>
#include <type_traits>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// a random-access iterator over the indices, of C++17 as well as of C++20
struct IndexIterator
{
 using iterator_category = std::random_access_iterator_tag;
 using value_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using pointer = const std::size_t *;
 using reference = std::size_t;

 std::size_t i = 0;  ///< the index

 std::size_t operator*() const { return( i ); }
 std::size_t operator[]( difference_type k ) const {
  return( i + std::size_t( k ) );
  }
 IndexIterator & operator++() {
  ++i;
  return( *this );
  }
 IndexIterator operator++( int ) {
  IndexIterator t = *this;
  ++i;
  return( t );
  }
 IndexIterator & operator--() {
  --i;
  return( *this );
  }
 IndexIterator operator--( int ) {
  IndexIterator t = *this;
  --i;
  return( t );
  }
 IndexIterator & operator+=( difference_type k ) {
  i += std::size_t( k );
  return( *this );
  }
 IndexIterator & operator-=( difference_type k ) {
  i -= std::size_t( k );
  return( *this );
  }
 friend IndexIterator operator+( IndexIterator a , difference_type k ) {
  return( a += k );
  }
 friend IndexIterator operator+( difference_type k , IndexIterator a ) {
  return( a += k );
  }
 friend IndexIterator operator-( IndexIterator a , difference_type k ) {
  return( a -= k );
  }
 friend difference_type operator-( IndexIterator a , IndexIterator b ) {
  return( difference_type( a.i ) - difference_type( b.i ) );
  }
 friend bool operator==( IndexIterator , IndexIterator ) = default;
 friend auto operator<=>( IndexIterator , IndexIterator ) = default;
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS StdParExecutor ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the executor on a policy of the parallel algorithms of the standard
/// library
template< class Policy = std::execution::parallel_policy >
 requires(
   std::is_execution_policy_v< Policy > &&
   ! std::is_same_v< Policy , std::execution::parallel_unsequenced_policy > )
class StdParExecutor
{

/*--------------------------------------------------------------------------*/
 public:
 /// the executor of the policy, which tells the algorithms of ADS that it
 /// has \f$ p \f$ threads (the hardware threads if \f$ p = 0 \f$)
 explicit StdParExecutor( std::size_t p = 0 )
     : f_p( p ? p
	      : std::max< std::size_t >( std::thread::hardware_concurrency() ,
					 1 ) ) { }

 /// the number of threads given to the constructor
 [[nodiscard]] std::size_t concurrency( void ) const { return( f_p ); }

 /// the calls \f$ f( 0 ) , \ldots , f( n - 1 ) \f$ by the `std::for_each`
 /// of the policy
 ///
 /// In pseudocode (beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{bulk}( n , f ) \cr
 /// 2 & \quad \textbf{if}\ n \le 1\ \textbf{or}\ p = 1\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do}\ f( i ) \cr
 /// 3 & \quad \textbf{else}\ \mathrm{std{::}for\_each}( \mathit{policy} ,
 ///     0 , n , i \mapsto \textbf{if}\ \text{no call has thrown}
 ///     \ \textbf{then}\ f( i ) ) \cr
 /// 4 & \quad \text{throw again the first exception of the calls, if any}
 /// \end{array}}
 /// \f]
 template< class F >
 void bulk( std::size_t n , F && f ) {
  if( ( n <= 1 ) || ( f_p == 1 ) ) {
   for( std::size_t i = 0 ; i < n ; ++i )
    f( i );
   return;
   }
  detail::FirstError error;
  // a policy by name: the parallel algorithms of libstdc++ 13 do not take
  // a temporary one
  Policy policy{};
  std::for_each( policy , detail::IndexIterator{ 0 } ,
		 detail::IndexIterator{ n } , [ & ]( std::size_t i ) {
		  if( error.failed() )
		   return;
		  try {
		   f( i );
		   }
		  catch( ... ) {
		   error.keep();
		   }
		  } );
  error.rethrow();
  }

/*--------------------------------------------------------------------------*/
 private:
 std::size_t f_p;  ///< the threads
 };  // end( class StdParExecutor )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

#endif /* __cpp_lib_parallel_algorithm */

/*--------------------------------------------------------------------------*/

#endif /* StdParExecutor.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File StdParExecutor.h ------------------------*/
/*--------------------------------------------------------------------------*/
