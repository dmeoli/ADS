/*--------------------------------------------------------------------------*/
/*-------------------------- File CountMinSketch.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sketches of the frequencies of a stream, beyond the book: the
 * Count-Min sketch of Cormode and Muthukrishnan, with the conservative
 * update of Estan and Varghese, and the Count sketch of Charikar, Chen and
 * Farach-Colton. Both keep \f$ d \f$ rows of \f$ w \f$ counters, and an
 * item adds its count to one counter per row, chosen by a hash of the row:
 *
 * - `CountMinSketch` adds the count \f$ c \ge 0 \f$ to the \f$ d \f$
 *   counters of the item and estimates its frequency \f$ f_x \f$ by the
 *   least of them, which is never below \f$ f_x \f$ and, with \f$ w =
 *   \lceil e / \varepsilon \rceil \f$ and \f$ d = \lceil \ln( 1 / \delta )
 *   \rceil \f$, exceeds it by more than \f$ \varepsilon N \f$ with
 *   probability at most \f$ \delta \f$, \f$ N \f$ the sum of the counts; the
 *   conservative update raises each of the \f$ d \f$ counters only up to
 *   the new estimate, \f$ \max( C_i , \min_j C_j + c ) \f$, so that its
 *   counters are those of the plain update or less and its estimates keep
 *   the same bound;
 * - `CountSketch` adds \f$ \pm c \f$ by a sign of the row, a counter of
 *   any sign, and estimates \f$ f_x \f$ by the median of the \f$ d \f$
 *   signed counters, unbiased in each row with a variance of at most
 *   \f$ \| f \|_2^2 / w \f$; it also estimates the second moment
 *   \f$ \| f \|_2^2 \f$ by the median of the sums of the squares of the
 *   rows (the estimator of Alon, Matias and Szegedy).
 *
 * The \f$ d \f$ counters of an item come from one hash \f$ h \f$ of 64
 * bits, by the double hashing of Kirsch and Mitzenmacher: the row \f$ i \f$
 * takes \f$ g_i = h + i h' \bmod 2^{64} \f$, \f$ h' \f$ an odd hash of
 * \f$ h \f$, and its counter is the high half of the product \f$ g_i w \f$
 * on 128 bits (the reduction of Lemire, with no division), while the sign
 * of `CountSketch` is the top bit of the low half. Two sketches of the
 * same shape add counter by counter (`merge()`): for the plain Count-Min
 * and for the Count sketch the sum is the sketch of the union of the
 * streams, while for the conservative update it is at least the frequency
 * and at most the plain sketch of the union, so that the bound holds.
 *
 * The hash of an item is the word of 64 bits of `hash64()` of `H` [see
 * `FastHash.h`] finished by `splitmix64()` [see `RandomHashing.h`],
 * which turns the hashes of consecutive integers, regular in their top
 * bits, into random words; `insertHash()` and `estimateHash()` take a
 * hash of 64 bits that is already random. The counters are of the type
 * `C`, which a stream must not overflow.
 *
 * The contracts [see `Contract.h`]: at least one row and one counter;
 * \f$ 0 < \varepsilon \f$ and \f$ 0 < \delta < 1 \f$; the sketches merged
 * have the same rows and counters, and the same update. The operations are
 * defined in `CountMinSketch.tpp`.
 *
 * \see G. Cormode, S. Muthukrishnan, An improved data stream summary: the
 *      count-min sketch and its applications, Journal of Algorithms 55(1),
 *      58-75, 2005, doi:10.1016/j.jalgor.2003.12.001
 * \see C. Estan, G. Varghese, New directions in traffic measurement and
 *      accounting, Proceedings of SIGCOMM 2002, 323-336, 2002,
 *      doi:10.1145/633025.633056 (the conservative update)
 * \see M. Charikar, K. Chen, M. Farach-Colton, Finding frequent items in
 *      data streams, Theoretical Computer Science 312(1), 3-15, 2004,
 *      doi:10.1016/S0304-3975(03)00400-6
 * \see A. Kirsch, M. Mitzenmacher, Less hashing, same performance:
 *      building a better Bloom filter, Proceedings of ESA 2006, LNCS 4168,
 *      456-467, 2006, doi:10.1007/11841036_42
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

#ifndef __ADS_CountMinSketch
 #define __ADS_CountMinSketch
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/external/MappedFile.h"
#include "ADS/set/impl/SketchHash.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the update of the counters of a `CountMinSketch`
enum class CountMinUpdate : std::uint8_t {
 plain ,        ///< each counter of the item raised by its count
 conservative  ///< each counter raised only up to the new estimate
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CountMinSketch ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the frequencies of a stream, overestimated by \f$ d \f$ rows of
/// \f$ w \f$ counters
template< class T , class C = std::uint64_t , class H = DefaultHash< T > >
class CountMinSketch
{

/*--------------------------------------------------------------------------*/
 public:
 /// the empty sketch of one counter, the plain update, e.g., for
 /// `restore()` [see `Persistence.h`]
 CountMinSketch( void ) : CountMinSketch( 1 , 1 ) { }

 /// the empty sketch of \f$ d \f$ rows of \f$ w \f$ counters
 /// \pre \f$ w \ge 1 \f$ and \f$ d \ge 1 \f$
 CountMinSketch( std::size_t w , std::size_t d ,
		 CountMinUpdate u = CountMinUpdate::plain , H h = H() );

 /// the empty sketch whose estimates exceed the frequencies by more than
 /// \f$ \varepsilon N \f$ with probability at most \f$ \delta \f$:
 /// \f$ w = \lceil e / \varepsilon \rceil \f$ and \f$ d = \lceil \ln( 1 /
 /// \delta ) \rceil \f$
 /// \pre \f$ \varepsilon > 0 \f$ and \f$ 0 < \delta < 1 \f$
 [[nodiscard]] static CountMinSketch
 forError( double epsilon , double delta ,
	   CountMinUpdate u = CountMinUpdate::plain , H h = H() );

 /// \f$ c \f$ occurrences of \f$ x \f$, by `insertHash()` of its hash
 void insert( const T & x , C c = 1 );

 /// \f$ c \f$ occurrences of the hash \f$ h \f$ of 64 bits, which must be
 /// random
 ///
 /// In pseudocode (Cormode and Muthukrishnan; Estan and Varghese for the
 /// conservative update), \f$ j_i \f$ the counter of the row \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertHash}( h , c ) \cr
 /// 2 & \quad N \gets N + c \cr
 /// 3 & \quad \textbf{if}\ \text{the update is plain}\ \textbf{then}
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ d - 1\ \textbf{do}
 ///     \ C[ i ][ j_i ] \gets C[ i ][ j_i ] + c \cr
 /// 4 & \quad \textbf{else}\ e \gets \min_i C[ i ][ j_i ] + c ;
 ///     \ \textbf{for}\ i \gets 0\ \textbf{to}\ d - 1\ \textbf{do}
 ///     \ C[ i ][ j_i ] \gets \max( C[ i ][ j_i ] , e )
 /// \end{array}}
 /// \f]
 void insertHash( std::uint64_t h , C c = 1 );

 /// the estimate of the frequency of \f$ x \f$, by `estimateHash()`
 [[nodiscard]] C estimate( const T & x ) const;

 /// the estimate of the frequency of the hash \f$ h \f$,
 /// \f$ \min_i C[ i ][ j_i ] \f$
 [[nodiscard]] C estimateHash( std::uint64_t h ) const;

 /// the counts of `o` added, counter by counter
 /// \pre `o` has the rows, the counters and the update of this sketch
 void merge( const CountMinSketch & o );

 /// the sum \f$ N \f$ of the counts inserted
 [[nodiscard]] C total( void ) const;

 /// the counters per row, \f$ w \f$
 [[nodiscard]] std::size_t width( void ) const;

 /// the rows, \f$ d \f$
 [[nodiscard]] std::size_t depth( void ) const;

 /// the update of the counters
 [[nodiscard]] CountMinUpdate update( void ) const;

 /// the additive error \f$ \varepsilon N \f$ of the estimates,
 /// \f$ \varepsilon = e / w \f$, exceeded with probability at most
 /// \f$ e^{-d} \f$
 [[nodiscard]] double errorBound( void ) const;

 /// removes all the counts
 void clear( void );

 /// whether no count was inserted
 [[nodiscard]] bool empty( void ) const;

 /// whether the two sketches have the same shape, update and counters
 [[nodiscard]] bool operator==( const CountMinSketch & o ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "CountMinSketch";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix(
    { sizeof( C ) , std::uint64_t( std::is_signed_v< C > ) } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 std::vector< C > f_C;  ///< the counters, row after row
 std::size_t f_w , f_d;  ///< the counters per row and the rows
 C f_N{};               ///< the sum of the counts
 CountMinUpdate f_u;    ///< the update
 H f_h;                 ///< the hash of the items

 };  // end( class CountMinSketch )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS CountSketch ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the frequencies of a stream, estimated by the median of \f$ d \f$ rows
/// of \f$ w \f$ signed counters
template< class T , class C = std::int64_t , class H = DefaultHash< T > >
class CountSketch
{

/*--------------------------------------------------------------------------*/
 public:
 /// the empty sketch of one counter, e.g., for `restore()` [see
 /// `Persistence.h`]
 CountSketch( void ) : CountSketch( 1 , 1 ) { }

 /// the empty sketch of \f$ d \f$ rows of \f$ w \f$ counters
 /// \pre \f$ w \ge 1 \f$ and \f$ d \ge 1 \f$
 CountSketch( std::size_t w , std::size_t d , H h = H() );

 /// \f$ c \f$ occurrences of \f$ x \f$ (removals if \f$ c < 0 \f$), by
 /// `insertHash()` of its hash
 void insert( const T & x , C c = 1 );

 /// \f$ c \f$ occurrences of the hash \f$ h \f$ of 64 bits, which must be
 /// random
 ///
 /// In pseudocode (Charikar, Chen and Farach-Colton), \f$ j_i \f$ the
 /// counter and \f$ s_i \in \{ -1 , 1 \} \f$ the sign of the row \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertHash}( h , c ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ d - 1\ \textbf{do}
 ///     \ C[ i ][ j_i ] \gets C[ i ][ j_i ] + s_i c
 /// \end{array}}
 /// \f]
 void insertHash( std::uint64_t h , C c = 1 );

 /// the estimate of the frequency of \f$ x \f$, by `estimateHash()`
 [[nodiscard]] double estimate( const T & x ) const;

 /// the estimate of the frequency of the hash \f$ h \f$, the median of
 /// \f$ s_i C[ i ][ j_i ] \f$ over the rows (the mean of the two middle
 /// ones for \f$ d \f$ even)
 [[nodiscard]] double estimateHash( std::uint64_t h ) const;

 /// the estimate of the second moment \f$ \sum_x f_x^2 \f$, the median of
 /// \f$ \sum_j C[ i ][ j ]^2 \f$ over the rows
 [[nodiscard]] double secondMoment( void ) const;

 /// the counts of `o` added, counter by counter
 /// \pre `o` has the rows and the counters of this sketch
 void merge( const CountSketch & o );

 /// the counters per row, \f$ w \f$
 [[nodiscard]] std::size_t width( void ) const;

 /// the rows, \f$ d \f$
 [[nodiscard]] std::size_t depth( void ) const;

 /// removes all the counts
 void clear( void );

 /// whether the two sketches have the same shape and counters
 [[nodiscard]] bool operator==( const CountSketch & o ) const;

/*----------------------------- THE PERSISTENCE ----------------------------*/

 /// the name of the structure in a file [see `Persistence.h`]
 static constexpr std::string_view persistName = "CountSketch";

 /// the parameters of the layout, in a file
 [[nodiscard]] static constexpr std::uint64_t persistTag( void ) {
  return( detail::persistMix(
    { sizeof( C ) , std::uint64_t( std::is_signed_v< C > ) } ) );
  }

 /// the members written to, or read from, the archive \f$ a \f$ [see
 /// `Persistence.h`]
 template< class A >
 void persist( A & a );

/*--------------------------------------------------------------------------*/
 private:
 /// the median of the d values from v, which it reorders
 [[nodiscard]] static double median( double * v , std::size_t d );

 std::vector< C > f_C;  ///< the counters, row after row
 std::size_t f_w , f_d;  ///< the counters per row and the rows
 H f_h;                 ///< the hash of the items

 };  // end( class CountSketch )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/set/impl/CountMinSketch.tpp"

/*--------------------------------------------------------------------------*/

#endif /* CountMinSketch.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File CountMinSketch.h ------------------------*/
/*--------------------------------------------------------------------------*/
