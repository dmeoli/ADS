/*--------------------------------------------------------------------------*/
/*------------------------- File SpeculativeFor.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The deterministic reservations of Blelloch, Fineman, Gibbons and Shun,
 * beyond the book: speculativeFor( ex , step , s , e ) runs, on the
 * threads of an executor [see Executor.h], a loop whose iterations
 * \f$ s , s + 1 , \ldots , e - 1 \f$ a sequential program would run in
 * this order, each depending on the ones before it (a greedy algorithm:
 * an edge enters the forest of Kruskal if no earlier edge joined its
 * ends), and gives the result of the sequential loop, whatever the
 * executor and the number of its threads.
 *
 * An iteration i is split into two steps of the object step:
 * - step.reserve( i ) reads the state that the earlier iterations left,
 *   and either finds that i has nothing to do (it returns false: i is
 *   done), or writes its index into a Reservation of each place of the
 *   state that it would change, keeping the least index there
 *   (reserve(), a priority write by compare-and-swap), and returns true;
 * - step.commit( i ) changes the state if i holds all its reservations,
 *   i.e., no earlier iteration of the round wants the same places, and
 *   returns true (i is done), else false (i is tried again in the next
 *   round); the holder of a reservation resets it, so that all of them
 *   are free at the end of the round.
 * A round runs the reserves of a prefix of the iterations not yet done,
 * in parallel, then their commits in parallel, and keeps the failed ones,
 * in their order, at the head of the next prefix. The least iteration of
 * a round holds all its reservations and commits, so that each round
 * makes progress; an iteration commits only when no earlier iteration
 * still to do touches its places, which is when the sequential loop
 * would see the same state, and hence the result is the sequential one.
 * The size of the prefix starts at \f$ r_{\max} = \lfloor ( e - s ) / g
 * \rfloor + 1 \f$, g the granularity, and it is halved (down to
 * \f$ r_{\max} / 64 + 1 \f$) when more than 20% of a round fails, and
 * doubled (up to \f$ r_{\max} \f$) when less than 10% fails; since the
 * failures are themselves deterministic, so is the whole sequence of the
 * rounds.
 * <pre>
 *   speculativeFor( ex , step , s , e , g )
 *  1  r ← \f$ r_{\max} \f$; I ← \f$ \emptyset \f$ (the iterations kept);
 *     d ← s (the next iteration never tried)
 *  2  while I ≠ \f$ \emptyset \f$ or d < e
 *  3   I ← I followed by d , d + 1 , ... up to r iterations; d advanced
 *  4   in parallel for each i ∈ I: \f$ k_i \f$ ← step.reserve( i )
 *  5   in parallel for each i ∈ I with \f$ k_i \f$: \f$ k_i \f$ ← not
 *      step.commit( i )
 *  6   I ← the i ∈ I with \f$ k_i \f$, in their order (a parallel pack)
 *  7   if |I| > r / 5 then r ← max( r / 2 , \f$ r_{\max} / 64 + 1 \f$ ,
 *      |I| ) else if |I| < r / 10 then r ← min( 2 r , \f$ r_{\max} \f$ )
 * </pre>
 * With rounds of r iterations the work is \f$ O( W + \rho ) \f$, W that
 * of the sequential loop and \f$ \rho \f$ that of the iterations tried
 * again, and the depth is that of the rounds; for the greedy maximal
 * independent set and matching on a random order the iterations that
 * depend on each other form chains of length \f$ O( \log^2 n ) \f$ with
 * high probability (Blelloch, Fineman and Shun; \f$ O( \log n ) \f$ by
 * Fischer and Noever), so that few of them fail. The phase-parallel
 * framework of Shen, Wan, Gu and Sun generalizes the scheme to the loops
 * whose dependences are not conflicts on places (e.g., the longest
 * increasing subsequence); for the greedy algorithms on graphs the
 * reservations remain the method of the benchmark suites (PBBS, GBBS).
 *
 * The contracts [see Contract.h]: \f$ s \leq e \f$, \f$ g \geq 1 \f$; the
 * reserve() of distinct iterations write only reservations (and their own
 * places), their commit() only the places whose reservations they hold,
 * and the least iteration of a round commits (else the loop would stop;
 * a round with no commit is a violated precondition).
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

 /// keeps i if it is less than the iteration there (a priority write)
 void reserve( std::size_t i ) noexcept {
  std::size_t c = f_r.load( std::memory_order_relaxed );
  while( ( i < c ) &&
         ! f_r.compare_exchange_weak( c , i , std::memory_order_relaxed ) )
   ;
  }

 /// whether i holds the place
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

 /// frees the place if i holds it; whether it did
 bool checkReset( std::size_t i ) noexcept {
  if( ! check( i ) )
   return( false );
  reset();
  return( true );
  }

 private:

 std::atomic< std::size_t > f_r{ NONE };
 };  // end( class Reservation )

/// what speculativeFor() did: its rounds and the iterations tried, the
/// failed ones counted again
struct SpeculativeCounts {
 long long rounds = 0 , tried = 0;
 };

/// the two steps of an iteration [see speculativeFor()]
template< class S >
concept SpeculativeStep = requires( S & s , std::size_t i ) {
 { s.reserve( i ) } -> std::convertible_to< bool >;
 { s.commit( i ) } -> std::convertible_to< bool >;
 };

/// the iterations [ s , e ) of step by deterministic reservations, with
/// the result of the sequential loop; the rounds of at most
/// ( e - s ) / g + 1 iterations; its work in *c, if given
/// \pre s <= e, g >= 1, the least iteration of a round commits
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
