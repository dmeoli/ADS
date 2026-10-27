/*--------------------------------------------------------------------------*/
/*------------------------- File SpeculativeFor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The deterministic reservations of Blelloch, Fineman, Gibbons and Shun,
 * beyond the book: `speculativeFor( ex , step , s , e )` runs, on the
 * threads of an executor [see `Executor.h`], a loop whose iterations
 * \f$ s , s + 1 , \ldots , e - 1 \f$ a sequential program would run in this
 * order, each depending on the ones before it, and gives the result of the
 * sequential loop whatever the executor and the number of its threads (see
 * \ref theory_parallel_reserve).
 *
 * An iteration \f$ i \f$ is split into the two steps of the object `step`:
 * `step.reserve( i )` reads the state that the earlier iterations left, and
 * either finds that \f$ i \f$ has nothing to do (it returns false, and
 * \f$ i \f$ is done) or writes \f$ i \f$ into a `Reservation` of each place
 * of the state that it would change, and returns true; `step.commit( i )`
 * changes the state if \f$ i \f$ holds all its reservations and returns
 * true, or returns false to be tried again in the next round, and the
 * holder of a reservation frees it, so that all of them are free at the end
 * of a round. By Lemma 4 of \ref theory_parallel_reserve the least
 * iteration of a round commits and the result is the sequential one. The
 * phase-parallel framework of Shen, Wan, Gu and Sun generalizes the scheme
 * to the loops whose dependences are not conflicts on places (e.g., the
 * longest increasing subsequence); for the greedy algorithms on graphs the
 * reservations remain the method of the benchmark suites (PBBS, GBBS).
 *
 * The contracts [see `Contract.h`]: \f$ s \le e \f$, \f$ g \ge 1 \f$; the
 * `reserve()` of distinct iterations write only reservations (and their
 * own places), their `commit()` only the places whose reservations they
 * hold, and the least iteration of a round commits (else the loop would
 * stop; a round with no commit is a violated precondition).
 *
 * \see G. E. Blelloch, J. T. Fineman, P. B. Gibbons, J. Shun, Internally
 *      deterministic parallel algorithms can be fast, Proceedings of
 *      PPoPP, 181-192, 2012, doi:10.1145/2145816.2145840
 * \see G. E. Blelloch, J. T. Fineman, J. Shun, Greedy sequential maximal
 *      independent set and matching are parallel on average, Proceedings
 *      of SPAA, 308-317, 2012, doi:10.1145/2312005.2312058
 * \see M. Fischer, A. Noever, Tight analysis of parallel randomized greedy
 *      MIS, Proceedings of SODA, 2152-2160, 2018,
 *      doi:10.1137/1.9781611975031.140
 * \see Z. Shen, Z. Wan, Y. Gu, Y. Sun, Many sequential iterative
 *      algorithms can be parallel and (nearly) work-efficient, Proceedings
 *      of SPAA, 273-286, 2022, doi:10.1145/3490148.3538574
 * \see D. Anderson, G. E. Blelloch, L. Dhulipala, M. Dobson, Y. Sun, The
 *      problem-based benchmark suite (PBBS), V2, Proceedings of PPoPP,
 *      445-447, 2022, doi:10.1145/3503221.3508422
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

#ifndef __ADS_SpeculativeFor
 #define __ADS_SpeculativeFor
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <atomic>
#include <cstddef>
#include <limits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Reservation ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a place reserved by the least of the iterations that want it
class Reservation {
 public:

 /// no iteration
 static constexpr std::size_t NONE =
                              std::numeric_limits< std::size_t >::max();

 /// keeps \f$ i \f$ if it is less than the iteration there (a priority
 /// write, by compare-and-swap)
 void reserve( std::size_t i ) noexcept {
  std::size_t c = f_r.load( std::memory_order_relaxed );
  while( ( i < c ) &&
         ! f_r.compare_exchange_weak( c , i , std::memory_order_relaxed ) )
   ;
  }

 /// whether \f$ i \f$ holds the place
 [[nodiscard]] bool check( std::size_t i ) const noexcept {
  return( f_r.load( std::memory_order_relaxed ) == i );
  }

 /// whether some iteration holds the place
 [[nodiscard]] bool reserved( void ) const noexcept {
  return( f_r.load( std::memory_order_relaxed ) != NONE );
  }

 /// frees the place
 void reset( void ) noexcept {
  f_r.store( NONE , std::memory_order_relaxed );
  }

 /// frees the place if \f$ i \f$ holds it; whether it did
 bool checkReset( std::size_t i ) noexcept {
  if( ! check( i ) )
   return( false );
  reset();
  return( true );
  }

 private:

 std::atomic< std::size_t > f_r{ NONE };        ///< the iteration there
 };  // end( class Reservation )

/// what `speculativeFor()` did: its rounds and the iterations tried, the
/// failed ones counted again
struct SpeculativeCounts {
 long long rounds = 0;                           ///< the rounds
 long long tried = 0;                            ///< the iterations tried
 };

/// the two steps of an iteration [see `speculativeFor()`]
template< class S >
concept SpeculativeStep = requires( S & s , std::size_t i ) {
 { s.reserve( i ) } -> std::convertible_to< bool >;
 { s.commit( i ) } -> std::convertible_to< bool >;
 };

/// the iterations \f$ [ s , e ) \f$ of `step` by deterministic
/// reservations, with the result of the sequential loop; the rounds of at
/// most \f$ r_{\mathrm{max}} = \lfloor ( e - s ) / g \rfloor + 1 \f$
/// iterations; its work in `*c`, if given
///
/// In pseudocode (Blelloch, Fineman, Gibbons and Shun, beyond the book; it
/// is correct by Lemma 4 of \ref theory_parallel_reserve):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{speculativeFor}( \mathit{ex} ,
///     \mathit{step} , s , e , g ) \cr
/// 2 & \quad r \gets r_{\mathrm{max}};\ I \gets \text{an empty sequence};
///     \ d \gets s \cr
/// 3 & \quad \textbf{while}\ I \ne \emptyset \textbf{ or } d < e
///     \ \textbf{do} \cr
/// 4 & \qquad \text{append } d , d + 1 , \ldots \text{ to } I \text{ up to }
///     \min( r , |I| + e - d ) \text{ iterations};\ \text{advance } d \cr
/// 5 & \qquad \textbf{foreach}\ i \in I \text{ in parallel}\ \textbf{do}
///     \ k_i \gets \mathit{step}.\mathrm{reserve}( i ) \cr
/// 6 & \qquad \textbf{foreach}\ i \in I \text{ with } k_i \text{ in parallel}
///     \ \textbf{do}\ k_i \gets \textbf{not}\ \mathit{step}.\mathrm{commit}(
///     i ) \cr
/// 7 & \qquad I \gets \text{the } i \in I \text{ with } k_i \text{, in their}
///     \text{ order (a parallel pack)} \cr
/// 8 & \qquad \textbf{if}\ |I| > r / 5\ \textbf{then}\ r \gets \max( r / 2 ,
///     r_{\mathrm{max}} / 64 + 1 , |I| ) \cr
/// 9 & \qquad \textbf{else if}\ |I| < r / 10\ \textbf{then}\ r \gets
///     \min( 2 r , r_{\mathrm{max}} )
/// \end{array}}
/// \f]
/// \pre \f$ s \le e \f$, \f$ g \ge 1 \f$, the least iteration of a round
///      commits
template< Executor E , SpeculativeStep S >
void speculativeFor( E & ex , S & step , std::size_t s , std::size_t e ,
                     std::size_t g = 50 , SpeculativeCounts * c = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/SpeculativeFor.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SpeculativeFor.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File SpeculativeFor.h ------------------------*/
/*--------------------------------------------------------------------------*/
