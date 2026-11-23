/*--------------------------------------------------------------------------*/
/*-------------------------- File ParallelSketch.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel construction of the mergeable sketches, beyond the book:
 * `parallelSketch( ex , first , last , s )` cuts the items of
 * \f$ [ \mathit{first} , \mathit{last} ) \f$ into the blocks of
 * `parallelReduce()` [see `Executor.h`], fills a copy of the empty sketch
 * \f$ s \f$ with each block on the threads of the executor, and merges the
 * copies in the order of the blocks. Any sketch whose `merge()` gives the
 * sketch of the union of the two streams, i.e., `UltraLogLog`,
 * `CountMinSketch` with the plain update, `CountSketch` and
 * `MinHashSketch` [see `UltraLogLog.h`, `CountMinSketch.h` and
 * `MinHashSketch.h`], is then built equal to the sequential one, register
 * by register; a Count-Min sketch with the conservative update gets
 * counters between the frequencies and those of the plain update, and a
 * `KllSketch` [see `KllSketch.h`] the guarantee of the sketch of the whole
 * stream, since KLL is fully mergeable.
 *
 * The blocks depend only on the number of items and on the grain, never on
 * the number of threads, so that the result is the same on every executor.
 * A sketch with a method `reseed( salt )`, as `KllSketch`, is reseeded in
 * each block by the index of its first item, since copies of one generator
 * would flip the same coins on all the blocks and their errors would not
 * average out.
 *
 * The contracts [see `Contract.h`]: \f$ s \f$ is empty, which the sketch
 * does not check. The function is defined in `ParallelSketch.tpp`.
 *
 * \see P. K. Agarwal, G. Cormode, Z. Huang, J. M. Phillips, Z. Wei, K. Yi,
 *      Mergeable summaries, ACM Transactions on Database Systems 38(4),
 *      26, 2013, doi:10.1145/2500128
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

#ifndef __ADS_ParallelSketch
 #define __ADS_ParallelSketch
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/Executor.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// a sketch that takes the items of `I` by `insert()` and the sketch of
/// another stream by `merge()`
template< class S , class I >
concept MergeableSketch =
  std::copyable< S > &&
  requires( S & s , const S & t , std::iter_reference_t< I > x )
{
   s.insert( x );
   s.merge( t );
   };

/// the sketch of the items of \f$ [ \mathit{first} , \mathit{last} ) \f$,
/// by the merge of the sketches of the blocks, filled in parallel from
/// copies of the empty sketch \f$ s \f$
///
/// In pseudocode (beyond the book), on the blocks
/// \f$ [ \mathit{lo}_b , \mathit{hi}_b ) \f$ of `parallelReduce()`:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{parallelSketch}( \mathit{ex} ,
///     \mathit{first} , \mathit{last} , s ) \cr
/// 2 & \quad \textbf{for each block}\ b\ \textbf{do in parallel} \cr
/// 3 & \qquad s_b \gets s ;\ \mathrm{reseed}( s_b , \mathit{lo}_b )
///     \ \text{if } s \text{ has it} \cr
/// 4 & \qquad \textbf{for}\ i \gets \mathit{lo}_b\ \textbf{to}
///     \ \mathit{hi}_b - 1\ \textbf{do}\ \mathrm{insert}( s_b ,
///     \mathit{first}[ i ] ) \cr
/// 5 & \quad r \gets s ;\ \textbf{for each block}\ b\ \textbf{in order}
///     \ \textbf{do}\ \mathrm{merge}( r , s_b ) \cr
/// 6 & \quad \textbf{return}\ r
/// \end{array}}
/// \f]
template< Executor E , std::random_access_iterator I , class S >
 requires MergeableSketch< S , I >
[[nodiscard]] S parallelSketch( E & ex , I first , I last , const S & s ,
				std::size_t grain = kGrain );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/parallel/impl/ParallelSketch.tpp"

/*--------------------------------------------------------------------------*/

#endif /* ParallelSketch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelSketch.h -----------------------*/
/*--------------------------------------------------------------------------*/
