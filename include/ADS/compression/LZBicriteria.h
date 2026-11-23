/*--------------------------------------------------------------------------*/
/*-------------------------- File LZBicriteria.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The bicriteria parse of LZ77 of Farruggia, Ferragina, Frangioni and
 * Venturini (beyond the book): among the parses of the text into copies
 * and runs of literals, the one of least decompression time within a bound
 * on the size of the code, or the one of least size within a bound on the
 * time, both measured by a model of the costs of a phrase that is additive
 * over the phrases. A parse is a path from 0 to \f$ n \f$ in the graph of
 * the parses, whose node \f$ i \f$ is the place \f$ i \f$ of the text and
 * whose arcs are the phrases, each with a space \f$ s( e ) \f$ and a time
 * \f$ t( e ) \f$; the problem is therefore a shortest path with one
 * constraint on a second weight, NP-hard on a general graph, which the
 * structure of this graph makes approximable within an additive term.
 *
 * The code, `lzBicriteriaEncode()`, is byte-oriented, with the
 * variable-byte code of `IntegerCodes.h` for every field: the length
 * \f$ n \f$ of the text, then for each phrase a token, which is
 * \f$ 2 ( \ell - 1 ) + 1 \f$ for a copy of \f$ \ell \f$ symbols, followed
 * by \f$ d - 1 \f$ for its distance \f$ d \f$, and
 * \f$ 2 ( k - 1 ) \f$ for a run of \f$ k \f$ literals, followed by the
 * \f$ k \f$ bytes themselves. With \f$ v( x ) \f$ the bytes of the
 * variable-byte code of \f$ x \f$ (1 below \f$ 2^7 \f$, 2 below
 * \f$ 2^{14} \f$, and so on), the space of a phrase is
 * \f[
 *   s( \langle d , \ell \rangle ) = v( 2 \ell - 1 ) + v( d - 1 ) , \qquad
 *   s( \langle k \text{ literals} \rangle ) = v( 2 k - 2 ) + k ,
 * \f]
 * and its time, in the units of `LZBicriteriaModel`, is
 * \f[
 *   t( e ) = \tau_p + \tau_b\, s( e ) + \tau_{q( d )} ,
 * \f]
 * with \f$ \tau_p \f$ the cost of a phrase, \f$ \tau_b \f$ that of a byte
 * of the code read, and \f$ \tau_q \f$ that of the access to a copy whose
 * distance takes \f$ q + 1 \f$ bytes, i.e., \f$ d \le 2^{7 ( q + 1 )} \f$
 * (0 for a run of literals). This is the model of the paper,
 * \f$ t( d , \ell ) = t( d ) + s( d , \ell ) \f$, with a cost per phrase
 * added and with the classes of the distances aligned to those of the
 * code: the default costs \f$ \tau_0 = \tau_1 = 0 \f$ (a copy within
 * 16 KiB, in the first level of the cache), \f$ \tau_2 = 20 \f$ (within
 * 2 MiB, in the second level), \f$ \tau_3 = 100 \f$ and \f$ \tau_4 = 200 \f$
 * (in the memory, and past the reach of its translation buffer), with
 * \f$ \tau_p = 8 \f$ and \f$ \tau_b = 1 \f$, are rough cycles of a current
 * processor, to be calibrated on the machine that decompresses (BC-ZIP of
 * the same authors takes three classes, the boundaries near
 * \f$ 2^{14} \f$ and \f$ 2^{21} \f$). The time of copying the \f$ n \f$
 * symbols is left out, the same for every parse. Both weights do not
 * decrease with \f$ d \f$ and \f$ \ell \f$, and a run of literals is cut at
 * 64 symbols (`lzBicriteriaMaxLiterals`), so that its token takes one byte
 * and no arc weighs more than \f$ s_{\max} \f$ in space and \f$ t_{\max} \f$
 * in time, the largest weights of a phrase.
 *
 * The graph. A copy of \f$ \ell \f$ symbols from the place \f$ i \f$ is
 * taken from the closest source the parse knows in each class of
 * distances: with \f$ L_q( i ) \f$ the longest copy found within
 * \f$ 2^{7 ( q + 1 )} \f$ places, the weights of the copy of length
 * \f$ \ell \f$ are those of the least class \f$ q \f$ with
 * \f$ L_q( i ) \ge \ell \f$, a step function of \f$ \ell \f$, and only the
 * right ends of its steps (the *maximal* arcs of the paper) are relaxed:
 * the lengths \f$ L_q( i ) \f$ and the lengths 64, 8192, \f$ 2^{20} \f$ and
 * \f$ 2^{27} \f$ where the token grows. The source with no window is the
 * longest previous factor of Crochemore and Ilie, from the suffix array
 * and the LCP array [see `SuffixArray.h`] by one scan with a stack; the
 * closer ones are found by walking the suffix array away from the rank of
 * \f$ i \f$, in the order of the decreasing lcp, for at most
 * `LZBicriteriaModel::candidates` suffixes; and each class inherits from
 * \f$ i - 1 \f$ the copy \f$ L_q( i - 1 ) - 1 \f$ from the same distance.
 * This last rule makes the graph *suffix-complete* (the suffix of an arc
 * is an arc, no heavier), which the theory needs; the lengths of the
 * bounded windows are those of the sources found, hence exact when the
 * walk ends before its bound and lower bounds otherwise, and the
 * guarantees below hold w.r.t. the best parse of this graph.
 *
 * The solution. The Lagrangian relaxation of the bound \f$ R \f$ on the
 * resource \f$ r \f$ (the space or the time) with the multiplier
 * \f$ \lambda \ge 0 \f$, \f$ \varphi( \lambda ) = \min_\pi c( \pi ) +
 * \lambda ( r( \pi ) - R ) \f$, is a shortest path for the weight
 * \f$ c + \lambda r \f$, in \f$ O( n ) \f$ time on the maximal arcs, the
 * runs of literals relaxed all together by the minimum over a sliding
 * window; its maximum over \f$ \lambda \f$, a lower bound on the optimum,
 * is found by the cutting planes of Handler and Zang (the hull approach of
 * Mehlhorn and Ziegelmann), which keeps two paths, \f$ \pi_L \f$ above the
 * bound and \f$ \pi_R \f$ within it, and moves \f$ \lambda \f$ to the
 * crossing of their lines. At the optimal \f$ \lambda^* \f$ the two paths
 * are joined: the *path swap* at the place \f$ v \f$ takes the prefix of
 * \f$ \pi_L \f$ up to \f$ v \f$ and the suffix of \f$ \pi_R \f$ from
 * \f$ v \f$, the arcs across \f$ v \f$ cut there, and the best swap within
 * the bound is chosen by one scan of the places of the two paths. With
 * \f$ c_{\max} \f$ and \f$ r_{\max} \f$ the largest weights of an arc, the
 * paper shows a swap within \f$ R + 2 r_{\max} \f$ of cost at most
 * \f$ \varphi( \lambda^* ) + c_{\max} \f$, and the same argument gives one
 * within \f$ R \f$ of cost at most \f$ \varphi( \lambda^* ) + c_{\max} + 2
 * \lambda^* r_{\max} \f$; `lzBicriteriaParse()` takes the slack over the
 * bound as a parameter, 0 by default, and reports the bound it proves.
 *
 * The costs: the suffix array and the LCP array in \f$ O( n ) \f$ time and
 * four arrays of \f$ n \f$ words, the walk in \f$ O( n K ) \f$ for \f$ K \f$
 * candidates, the graph in at most five arcs of copies per place, and each
 * of the shortest paths in \f$ O( n ) \f$, at most
 * `LZBicriteriaModel::iterations` of them besides the two of the
 * extremes. The decoder, `lzBicriteriaDecode()`, copies 16 bytes at a time
 * when the distance is at least 16 and the buffer has room, one byte at a
 * time otherwise, which is right on an overlapping copy too.
 *
 * The contracts [see `Contract.h`]: \f$ n < 2^{32} - 1 \f$; the decoder
 * gets a code of the encoder (each field within the code, each copy within
 * the symbols already decoded), and the decoding of a code gives back its
 * text, an audit. The functions are defined in `LZBicriteria.tpp`.
 *
 * \see A. Farruggia, P. Ferragina, A. Frangioni, R. Venturini, Bicriteria
 *      data compression, Proc. 25th SODA, 1582-1595, 2014,
 *      doi:10.1137/1.9781611973402.115
 * \see A. Farruggia, P. Ferragina, A. Frangioni, R. Venturini, Bicriteria
 *      data compression, SIAM Journal on Computing 48(5), 1603-1642, 2019,
 *      doi:10.1137/17M1121457
 * \see A. Farruggia, P. Ferragina, R. Venturini, Bicriteria data
 *      compression: efficient and usable, Proc. 22nd ESA, LNCS 8737,
 *      406-417, 2014, doi:10.1007/978-3-662-44777-2_34
 * \see P. Ferragina, I. Nitto, R. Venturini, On the bit-complexity of
 *      Lempel-Ziv compression, SIAM Journal on Computing 42(4), 1521-1541,
 *      2013, doi:10.1137/120869511
 * \see G. Y. Handler, I. Zang, A dual algorithm for the constrained
 *      shortest path problem, Networks 10(4), 293-309, 1980,
 *      doi:10.1002/net.3230100403
 * \see K. Mehlhorn, M. Ziegelmann, Resource constrained shortest paths,
 *      Proc. 8th ESA, LNCS 1879, 326-337, 2000,
 *      doi:10.1007/3-540-45253-2_30
 * \see M. Crochemore, L. Ilie, Computing longest previous factor in linear
 *      time and applications, Information Processing Letters 106(2),
 *      75-80, 2008, doi:10.1016/j.ipl.2007.10.006
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

#ifndef __ADS_LZBicriteria
 #define __ADS_LZBicriteria
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/IntegerCodes.h"
#include "ADS/string/SuffixArray.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// what the bicriteria parse minimizes, the other quantity being bounded:
/// the time within a bound on the bytes of the code, or the bytes within a
/// bound on the time
enum class LZBicriteriaGoal {
 time ,  ///< the least time within a bound on the space
 space  ///< the least space within a bound on the time
 };

/// the longest run of literals of a bicriteria parse, whose token then
/// takes one byte
inline constexpr std::size_t lzBicriteriaMaxLiterals = 64;

/// the model of the time of the phrases, and the effort of the search of
/// the sources and of the multiplier
struct LZBicriteriaModel
{
 std::uint64_t phrase = 8;  ///< the time of a phrase, besides its bytes
 std::uint64_t byte = 1;    ///< the time of a byte of the code read
 /// the time of the access to a copy whose distance takes 1 to 5 bytes
 std::array< std::uint64_t , 5 > distance = { 0 , 0 , 20 , 100 , 200 };
 /// the suffixes nearest in the suffix array tried for the sources of the
 /// bounded windows of a place
 std::size_t candidates = 64;
 /// the most shortest paths of the cutting planes
 std::size_t iterations = 128;
 };

/// a phrase of a bicriteria parse: a copy of `length` symbols from
/// `distance` back, or the run of the next `length` symbols of the text as
/// literals when `distance` is 0
struct LZBicriteriaPhrase
{
 std::size_t distance ,  ///< how far back the copy starts, 0 for literals
   length;              ///< the symbols of the phrase
 /// whether the two phrases are equal
 friend bool operator==( const LZBicriteriaPhrase & a ,
			 const LZBicriteriaPhrase & b ) = default;
 };

/// a bicriteria parse with its weights and the certificate of its quality
struct LZBicriteriaParse
{
 std::vector< LZBicriteriaPhrase > phrases;  ///< the phrases, in order
 std::uint64_t space = 0;  ///< the bytes of the code, the header included
 std::uint64_t time = 0;   ///< the time of the phrases in the model
 /// a lower bound on the minimized quantity of every parse of the graph
 /// within the bound, \f$ \varphi( \lambda^* ) \f$ (infinite when none is)
 double lowerBound = 0;
 double multiplier = 0;  ///< the multiplier \f$ \lambda^* \f$ of the bound
 /// the proven bound on the minimized quantity less `lowerBound` (infinite
 /// when the cutting planes stopped at their limit of iterations)
 double gapBound = 0;
 bool feasible = true;  ///< whether some parse of the graph is within the
			///< bound (if not, the parse is the one of least
			///< bounded quantity)
 };

/// the bicriteria parse of \f$ s \f$: the least time within `bound` bytes
/// of the code, or the least space within `bound` units of time, the
/// bounded quantity allowed to exceed `bound` by `slack`
///
/// In pseudocode (the algorithm of Farruggia, Ferragina, Frangioni and
/// Venturini, beyond the book), with \f$ c \f$ the minimized weight,
/// \f$ r \f$ the bounded one, \f$ R \f$ its bound for the phrases and
/// \f$ \Lambda( \pi , \lambda ) = c( \pi ) + \lambda ( r( \pi ) - R ) \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzBicriteriaParse}( s , R , \sigma ) \cr
/// 2 & \quad G \gets \mathrm{parseGraph}( s );\ \pi_L \gets \text{the path }
///     \text{of least } ( c , r ) \text{ in } G;\ \pi_R \gets \text{the }
///     \text{path of least } ( r , c ) \cr
/// 3 & \quad \textbf{if}\ r( \pi_R ) > R\ \textbf{then return}\ \pi_R
///     \quad \triangleright\ \text{no parse within the bound} \cr
/// 4 & \quad \textbf{if}\ r( \pi_L ) \le R\ \textbf{then return}\ \pi_L
///     \quad \triangleright\ \text{optimal} \cr
/// 5 & \quad \textbf{loop} \cr
/// 6 & \qquad \lambda \gets ( c( \pi_R ) - c( \pi_L ) ) / ( r( \pi_L ) -
///     r( \pi_R ) );\ \varphi_B \gets \Lambda( \pi_L , \lambda ) \cr
/// 7 & \qquad \pi \gets \text{the shortest path of } G \text{ for the }
///     \text{weight } c + \lambda r \cr
/// 8 & \qquad \textbf{if}\ \Lambda( \pi , \lambda ) \ge \varphi_B
///     \ \textbf{then}\ \textbf{break}\quad \triangleright\ ( \pi_L ,
///     \pi_R ) \text{ is an optimal basis} \cr
/// 9 & \qquad \textbf{if}\ r( \pi ) = R\ \textbf{then return}\ \pi \cr
/// 10 & \qquad \textbf{if}\ r( \pi ) > R\ \textbf{then}\ \pi_L \gets \pi
///     \ \textbf{else}\ \pi_R \gets \pi \cr
/// 11 & \quad \textbf{return}\ \text{the swap } \mathrm{ps}( \pi_L , \pi_R ,
///     v ) \text{ of least } c \text{ with } r \le R + \sigma \text{, over }
///     \text{the places } v \text{ of } \pi_L \text{ and } \pi_R \cr
/// 12 & \textbf{function}\ \mathrm{parseGraph}( s ) \cr
/// 13 & \quad \mathit{SA} , \mathit{LCP} \gets \text{the suffix and the }
///     \text{LCP arrays of } s;\ ( \ell_\infty , j_\infty ) \gets \text{the }
///     \text{longest previous factors} \cr
/// 14 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
/// 15 & \qquad \textbf{foreach}\ q\ \textbf{do}\ ( L_q , D_q ) \gets ( L_q -
///     1 , D_q )\quad \triangleright\ \text{the copies of } i - 1 \text{,}
///     \text{ one symbol shorter} \cr
/// 16 & \qquad \text{raise } L_q \text{ to } \ell_\infty[ i ] \text{ for the }
///     \text{classes } q \ge q( i - j_\infty[ i ] ) \cr
/// 17 & \qquad \text{walk } \mathit{SA} \text{ from the rank of } i \text{ }
///     \text{by decreasing lcp } h \text{, while } h > L_0 \text{, for at }
///     \text{most } K \text{ suffixes } j \cr
/// 18 & \qquad\quad \textbf{if}\ j < i\ \textbf{then}\ \text{raise } L_q
///     \text{ to } h \text{ from } D_q = i - j \text{ for the classes } q \ge
///     q( i - j ) \cr
/// 19 & \qquad \text{the copies from } i \text{: } ( L_q , D_q ) \text{ for }
///     \text{the classes where } L_q \text{ grows and } q( D_q ) \text{ is }
///     \text{less than for every longer one} \cr
/// 20 & \quad \textbf{return}\ \text{the graph of these copies and of the }
///     \text{runs of 1 to 64 literals}
/// \end{array}}
/// \f]
/// \pre \f$ |s| < 2^{32} - 1 \f$
/// \post `lzBicriteriaDecode( lzBicriteriaEncode( s , result.phrases ) )
///       == s` (an audit), `result.space` the size of that code and
///       `result.time == lzBicriteriaTime( result.phrases , m )`; if
///       `result.feasible`, the bounded quantity is at most
///       `bound + slack` and the minimized one at most
///       `result.lowerBound + result.gapBound`
[[nodiscard]] inline LZBicriteriaParse
lzBicriteriaParse( std::string_view s , LZBicriteriaGoal goal ,
		   std::uint64_t bound , const LZBicriteriaModel & m = {} ,
		   std::uint64_t slack = 0 );

/// the time of the phrases \f$ P \f$ in the model \f$ m \f$,
/// \f$ \sum_{e \in P} t( e ) \f$
[[nodiscard]] inline std::uint64_t
lzBicriteriaTime( const std::vector< LZBicriteriaPhrase > & P ,
		  const LZBicriteriaModel & m = {} );

/// the code of the phrases \f$ P \f$ of \f$ s \f$
///
/// In pseudocode (the byte-oriented code, beyond the book), with
/// \f$ \mathrm{vb}( x ) \f$ the variable-byte code of \f$ x \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzBicriteriaEncode}( s , P ) \cr
/// 2 & \quad C \gets \mathrm{vb}( n );\ p \gets 0 \cr
/// 3 & \quad \textbf{foreach}\ \langle d , \ell \rangle \in P\ \textbf{do} \cr
/// 4 & \qquad \textbf{if}\ d = 0\ \textbf{then}\ C \gets C \cdot \mathrm{vb}(
///     2 ( \ell - 1 ) ) \cdot s[ p .. p + \ell - 1 ] \cr
/// 5 & \qquad \textbf{else}\ C \gets C \cdot \mathrm{vb}( 2 ( \ell - 1 ) + 1 )
///     \cdot \mathrm{vb}( d - 1 ) \cr
/// 6 & \qquad p \gets p + \ell \cr
/// 7 & \quad \textbf{return}\ C
/// \end{array}}
/// \f]
/// \pre the phrases cover \f$ s \f$, each of positive length and each copy
///      within the symbols before it
/// \post `lzBicriteriaDecode( result ) == s` (an audit)
[[nodiscard]] inline std::vector< std::uint8_t >
lzBicriteriaEncode( std::string_view s ,
		    const std::vector< LZBicriteriaPhrase > & P );

/// the length of the text of the code \f$ C \f$, read from its header
/// \pre \f$ C \f$ starts with a code of the variable-byte code
[[nodiscard]] inline std::size_t
lzBicriteriaSize( const std::vector< std::uint8_t > & C );

/// the text of the code \f$ C \f$, written to `out`, whose first
/// `lzBicriteriaSize( C )` bytes it fills; their number
///
/// In pseudocode (the byte-oriented decoder, beyond the book), the copies
/// of distance at least 16 by blocks of 16 bytes, which never overlap the
/// bytes they write:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lzBicriteriaDecode}( C ) \cr
/// 2 & \quad n \gets \text{the first integer of } C;\ o \gets 0 \cr
/// 3 & \quad \textbf{while}\ o < n\ \textbf{do} \cr
/// 4 & \qquad x \gets \text{the next integer of } C;\ \ell \gets \lfloor x /
///     2 \rfloor + 1 \cr
/// 5 & \qquad \textbf{if}\ x \text{ is even}\ \textbf{then}\ \text{copy the }
///     \text{next } \ell \text{ bytes of } C \text{ to } \mathit{out}[ o .. ]
///     \cr
/// 6 & \qquad \textbf{else} \cr
/// 7 & \qquad\quad d \gets \text{the next integer of } C \text{, plus } 1 \cr
/// 8 & \qquad\quad \textbf{if}\ d \ge 16\ \textbf{then}\ \text{copy }
///     \mathit{out}[ o - d .. o - d + \ell - 1 ] \text{ to } \mathit{out}[ o
///     .. ] \text{ by blocks of 16} \cr
/// 9 & \qquad\quad \textbf{else}\ \textbf{for}\ k \gets 0\ \textbf{to}\ \ell -
///     1\ \textbf{do}\ \mathit{out}[ o + k ] \gets \mathit{out}[ o - d + k ]
///     \cr
/// 10 & \qquad o \gets o + \ell \cr
/// 11 & \quad \textbf{return}\ n
/// \end{array}}
/// \f]
/// \pre \f$ C \f$ is a code of `lzBicriteriaEncode()` and `out` has at least
///      `lzBicriteriaSize( C )` bytes
inline std::size_t lzBicriteriaDecode( const std::vector< std::uint8_t > & C ,
				       std::span< char > out );

/// the text of the code \f$ C \f$, by the decoder above
/// \pre \f$ C \f$ is a code of `lzBicriteriaEncode()`
[[nodiscard]] inline std::string
lzBicriteriaDecode( const std::vector< std::uint8_t > & C );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/compression/impl/LZBicriteria.tpp"

/*--------------------------------------------------------------------------*/

#endif /* LZBicriteria.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File LZBicriteria.h -------------------------*/
/*--------------------------------------------------------------------------*/
