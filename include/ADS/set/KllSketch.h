/*--------------------------------------------------------------------------*/
/*---------------------------- File KllSketch.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sketch of the quantiles of Karnin, Lang and Liberty (KLL), beyond the
 * book: the ranks and the quantiles of a stream of \f$ n \f$ items of a
 * total order, within \f$ \pm \varepsilon n \f$, in space independent of
 * \f$ n \f$ but for a logarithm, from comparisons alone. The sketch is a
 * stack of compactors, the level \f$ h \f$ holding items of weight
 * \f$ 2^h \f$: an item enters the level 0, and a level that is full is
 * compacted, i.e., sorted, cut into the pairs of consecutive items and
 * replaced by one item of each pair, the first or the second by a fair coin
 * for the whole level, which go up to the level \f$ h + 1 \f$ with twice
 * the weight. A compaction changes the weight below a value \f$ x \f$ by
 * \f$ \pm 2^h \f$ with probability 1/2 each when an odd number of the
 * items of the level are at most \f$ x \f$, and by nothing otherwise, so
 * that the estimate of the rank of \f$ x \f$ is the sum of independent
 * errors of mean 0.
 *
 * The capacity of the level \f$ h \f$ of \f$ H \f$ levels is
 * \f$ \max( 8 , \lfloor k ( 2 / 3 )^{H - h - 1} \rceil ) \f$ (the top level
 * holds \f$ k \f$, the lower ones geometrically less, and none less than
 * 8), so that the sketch holds about \f$ 3 k \f$ items and its normalized
 * rank error falls about as \f$ 1 / k \f$: \f$ k = 200 \f$ gives about
 * 1.3 per cent (`normalizedRankError()`, the fit of DataSketches at the
 * confidence of 99 per cent). The compaction is lazy, as in the library
 * DataSketches: the levels share one buffer of their total capacity, the
 * level 0 at its end growing towards its start, and only when the buffer is
 * full the lowest level at or above its capacity is compacted, a new level
 * being added on top when that is the top one; the level 0 is sorted only
 * when it is compacted or read. Two sketches merge by joining their
 * levels and compacting from the bottom while the total exceeds the
 * capacity (`merge()`), with the same guarantee as the sketch of the union
 * of the streams (KLL is fully mergeable).
 *
 * The coins come from a generator SplitMix64 of the sketch, seeded by its
 * constructor, so that a sketch and its copies give the same answers;
 * `reseed()` changes it, which the sketches of a parallel construction
 * need, since equal coins on two parts of a stream do not average out [see
 * `ParallelSketch.h`]. The items are `std::semiregular`, compared by
 * `Compare`, which is not stored in a file.
 *
 * The contracts [see `Contract.h`]: \f$ 8 \le k \f$; the minimum, the
 * maximum and the quantiles of a sketch that is not empty;
 * \f$ 0 \le q \le 1 \f$. The operations are defined in `KllSketch.tpp`.
 *
 * \see Z. Karnin, K. Lang, E. Liberty, Optimal quantile approximation in
 *      streams, Proceedings of FOCS 2016, 71-78, 2016,
 *      doi:10.1109/FOCS.2016.17
 * \see N. Ivkin, E. Liberty, K. Lang, Z. Karnin, V. Braverman, Streaming
 *      quantiles algorithms with small space and update time, Sensors
 *      22(24), 9612, 2022, doi:10.3390/s22249612 (the lazy compaction)
 * \see The Apache Software Foundation, Apache DataSketches, the KLL sketch
 *      of datasketches-cpp, https://datasketches.apache.org
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

#ifndef __ADS_KllSketch
 #define __ADS_KllSketch
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/external/MappedFile.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS KllSketch -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the ranks and the quantiles of a stream, within \f$ \pm \varepsilon n
/// \f$, by compactors of weights \f$ 1 , 2 , 4 , \ldots \f$
template< std::semiregular T , class Compare = std::less< T > >
class KllSketch
{

/*--------------------------------------------------------------------------*/
 public:
 /// the sorted items of a sketch with their cumulative weights, which
 /// answer the ranks and the quantiles in \f$ O( \log s ) \f$ time
 class View
 {
  public:
  /// the fraction of the items of the stream at most \f$ x \f$, estimated
  [[nodiscard]] double rank( const T & x ) const;

  /// the least item whose estimated rank is at least \f$ q \f$
  /// \pre the sketch was not empty and \f$ 0 \le q \le 1 \f$
  [[nodiscard]] const T & quantile( double q ) const;

  /// the items retained
  [[nodiscard]] std::size_t size( void ) const;

  private:
  friend class KllSketch;
  std::vector< T > f_I;              ///< the items, sorted
  std::vector< std::uint64_t > f_W;  ///< the cumulative weights
  std::uint64_t f_n = 0;             ///< the items of the stream
  Compare f_cmp;                     ///< the order
  };

 /// the empty sketch of parameter \f$ k \f$, its coins from the seed
 /// \pre \f$ k \ge 8 \f$
 explicit KllSketch( std::uint16_t k = 200 , std::uint64_t seed = 0 ,
		     Compare cmp = Compare() );

 /// \f$ x \f$ into the sketch
 ///
 /// In pseudocode (Karnin, Lang and Liberty, with the lazy compaction of
 /// DataSketches), \f$ L_0 \f$ the start of the level 0 in the buffer,
 /// whose free places are those before it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x ) \cr
 /// 2 & \quad \textbf{if}\ L_0 = 0\ \textbf{then} \cr
 /// 3 & \qquad h \gets \text{the lowest level with at least its capacity}
 ///     \cr
 /// 4 & \qquad \textbf{if}\ h = H - 1\ \textbf{then}\ \text{add an empty}
 ///     \text{ top level, the buffer growing at its start} \cr
 /// 5 & \qquad \mathrm{compact}( h ) \cr
 /// 6 & \quad L_0 \gets L_0 - 1;\ \mathit{buffer}[ L_0 ] \gets x;
 ///     \ n \gets n + 1
 /// \end{array}}
 /// \f]
 void insert( const T & x );

 /// the items of `o` added, with the guarantee of the sketch of the union
 /// of the two streams
 ///
 /// In pseudocode (Karnin, Lang and Liberty; DataSketches), the general
 /// compaction of the joined levels from the bottom:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{merge}( o ) \cr
 /// 2 & \quad \textbf{for each}\ x\ \text{of the level 0 of } o
 ///     \ \textbf{do}\ \mathrm{insert}( x ) \cr
 /// 3 & \quad \textbf{for}\ h \gets 1\ \textbf{to}\ H_o - 1\ \textbf{do}
 ///     \ \text{merge the sorted level } h \text{ of } o \text{ into the }
 ///     \text{level } h \cr
 /// 4 & \quad \textbf{for}\ h \gets 0 , 1 , \ldots \textbf{while}\ h < H
 ///     \ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ \text{the items exceed the capacity and the}
 ///     \text{ level } h \text{ holds at least its own}\ \textbf{then} \cr
 /// 6 & \qquad\quad \mathrm{compact}( h ),\ \text{adding a level on top}
 ///     \text{ when } h = H - 1
 /// \end{array}}
 /// \f]
 void merge( const KllSketch & o );

 /// the generator of the coins changed by the salt
 void reseed( std::uint64_t salt );

 /// the items of the stream, \f$ n \f$
 [[nodiscard]] std::uint64_t size( void ) const;

 /// whether no item was inserted
 [[nodiscard]] bool empty( void ) const;

 /// the items retained, at most the capacity of the levels
 [[nodiscard]] std::size_t retained( void ) const;

 /// the parameter \f$ k \f$
 [[nodiscard]] std::uint16_t k( void ) const;

 /// the levels \f$ H \f$
 [[nodiscard]] unsigned levels( void ) const;

 /// the least item of the stream
 /// \pre the sketch is not empty
 [[nodiscard]] const T & min( void ) const;

 /// the largest item of the stream
 /// \pre the sketch is not empty
 [[nodiscard]] const T & max( void ) const;

 /// the fraction of the items of the stream at most \f$ x \f$, estimated,
 /// by a `view()` made for the call
 [[nodiscard]] double rank( const T & x ) const;

 /// the least item whose estimated rank is at least \f$ q \f$: `min()` for
 /// \f$ q = 0 \f$ and `max()` for \f$ q = 1 \f$, by a `view()` otherwise
 /// \pre the sketch is not empty and \f$ 0 \le q \le 1 \f$
 [[nodiscard]] T quantile( double q ) const;

 /// the sorted items with their cumulative weights, in
 /// \f$ O( s \log s ) \f$ time for \f$ s \f$ items retained
 [[nodiscard]] View view( void ) const;

 /// the normalized error of the ranks at the confidence of 99 per cent,
 /// \f$ 2.296 / k^{0.9723} \f$, the fit of DataSketches
 [[nodiscard]] static double normalizedRankError( std::uint16_t k );

 /// removes all the items, keeping \f$ k \f$ and the generator
 void clear( void );

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "KllSketch";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix( { sizeof( T ) } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 /// the capacity of the level \f$ h \f$ of \f$ H \f$ levels
 [[nodiscard]] std::uint32_t capacity( unsigned H , unsigned h ) const;

 /// the capacity of \f$ H \f$ levels
 [[nodiscard]] std::uint32_t capacity( unsigned H ) const;

 /// a fair coin
 [[nodiscard]] bool coin( void );

 /// the level \f$ h \f$ of the buffer compacted into the level
 /// \f$ h + 1 \f$, in place, the levels below moved up by the places freed
 ///
 /// In pseudocode (Karnin, Lang and Liberty), \f$ c \f$ a fair coin:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{compact}( h ) \cr
 /// 2 & \quad \text{sort the level } h \text{ if } h = 0 ;\ \text{keep its}
 ///     \text{ first item there if its size is odd} \cr
 /// 3 & \quad \text{of the others, } y_0 \le y_1 \le \cdots \le
 ///     y_{2 t - 1} ,\ \text{move } y_c , y_{c + 2} , \ldots ,
 ///     y_{c + 2 t - 2} \text{ to the level } h + 1 \cr
 /// 4 & \quad \text{merge them with the sorted items of the level }
 ///     h + 1 \cr
 /// 5 & \quad \text{move the levels below } h \text{ up by the } t
 ///     \text{ places freed}
 /// \end{array}}
 /// \f]
 void compact( unsigned h );

 /// the levels of `o` above the 0 joined to these and compacted, after the
 /// items of its level 0 were inserted
 void mergeHigher( const KllSketch & o );

 std::vector< T > f_I;              ///< the buffer of the levels
 std::vector< std::uint32_t > f_L;  ///< the starts of the levels, and its end
 std::uint64_t f_n = 0;             ///< the items of the stream
 T f_min{} , f_max{};                ///< the extremes, if f_n > 0
 std::uint64_t f_s;                 ///< the state of the generator
 std::uint64_t f_bits = 0;          ///< the coins not yet used
 std::uint32_t f_nbits = 0;         ///< their number
 std::uint16_t f_k;                 ///< the parameter k
 bool f_sorted0 = true;             ///< whether the level 0 is sorted
 Compare f_cmp;                     ///< the order

 };  // end( class KllSketch )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/KllSketch.tpp"

/*--------------------------------------------------------------------------*/

#endif /* KllSketch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File KllSketch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
