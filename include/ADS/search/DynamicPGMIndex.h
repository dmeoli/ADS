/*--------------------------------------------------------------------------*/
/*------------------------- File DynamicPGMIndex.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The dynamic PGM-index of Ferragina and Vinciguerra, beyond the book: an
 * ordered map from arithmetic keys to values, with insertions and removals,
 * made of static PGM-indices [see `PGMIndex.h`] by the logarithmic method of
 * Bentley and Saxe (see \ref theory_search_learned).
 *
 * The pairs lie in a buffer of \f$ b_0 = 128 \f$ pairs, sorted, and in
 * levels \f$ L_1 , L_2 , \ldots \f$, the level \f$ L_i \f$ a sorted array of
 * at most \f$ b_0 B^i \f$ pairs with a PGM-index on its keys, each key at
 * most once per level; a key may be in several levels, and its pair in the
 * newest one, the buffer first and then by increasing \f$ i \f$, is the one
 * that counts. A removal is the insertion of a <em>tombstone</em>, a pair
 * that marks the key as absent. An insertion that finds the buffer full
 * merges it, with the new pair, into the first level \f$ L_t \f$ that has
 * room for it and for the levels before it, merging those too (the newest
 * pair of each key kept), and rebuilds the PGM-index of \f$ L_t \f$; the
 * tombstones are dropped when \f$ L_t \f$ is the last level, since no older
 * pair is left for them to hide. Hence:
 *
 * - a pair is merged into a level at most once per level, \f$ B \f$ times
 *   at most before the level overflows into the next one, and the merge and
 *   the construction of a PGM-index are linear: an insertion or a removal
 *   costs \f$ O( B \log_B( n / b_0 ) ) \f$ amortized time;
 * - a lookup searches the buffer and then the levels from the newest, each
 *   by its PGM-index, and stops at the first one that has the key:
 *   \f$ O( \log_B( n / b_0 ) ) \f$ searches of
 *   \f$ O( \log \epsilon ) \f$ time each, besides the levels of the
 *   PGM-indices;
 * - `lowerBound()` and `forEach()` merge the levels from their places of
 *   the key, the newest pair of each key deciding, and skip the keys whose
 *   newest pair is a tombstone.
 *
 * As in the implementation of the paper, the levels too small for a model
 * to pay (by default below \f$ 2^{20} \f$ pairs, \f$ 2^{24} \f$ there) have no
 * PGM-index and are searched by a binary search with no branch; the keys of
 * a level with an index are those of its PGM-index, with no other copy, and
 * the keys, the values and the tombstones of a level are three arrays, so
 * that a search reads the keys alone. The number of distinct keys is not
 * kept: it would cost a lookup per insertion; `entries()` counts the pairs
 * stored, tombstones and older pairs included.
 *
 * The contracts [see `Contract.h`]: \f$ B \ge 2 \f$, the pairs of the
 * constructor sorted by key with no key twice (an audit). The functions are
 * defined in `DynamicPGMIndex.tpp`.
 *
 * \see P. Ferragina, G. Vinciguerra, The PGM-index: a fully-dynamic
 *      compressed learned index with provable worst-case bounds, Proceedings
 *      of the VLDB Endowment 13(8), 1162-1175, 2020,
 *      doi:10.14778/3389133.3389135
 * \see J. L. Bentley, J. B. Saxe, Decomposable searching problems I:
 *      static-to-dynamic transformation, Journal of Algorithms 1(4),
 *      301-358, 1980, doi:10.1016/0196-6774(80)90015-2
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

#ifndef __ADS_DynamicPGMIndex
 #define __ADS_DynamicPGMIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/search/PGMIndex.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS DynamicPGMIndex --------------------------*/
/*--------------------------------------------------------------------------*/

/// the ordered map from the arithmetic keys `K` to the values `V` of the
/// dynamic PGM-index, with the error \f$ \epsilon = E \f$ in the levels,
/// the ratio \f$ B \f$ of the sizes of two levels and the least size
/// \f$ I \f$ of a level with a PGM-index
template< class K , class V , std::size_t E = 16 , std::size_t B = 8 ,
          std::size_t I = ( std::size_t( 1 ) << 20 ) >
 requires std::is_arithmetic_v< K >
class DynamicPGMIndex {

 static_assert( B >= 2 , "the ratio of the levels is at least 2" );

/*--------------------------------------------------------------------------*/
 public:

 using key_type = K;      ///< the type of the keys
 using mapped_type = V;   ///< the type of the values

 /// the pairs in the buffer, \f$ b_0 \f$
 static constexpr std::size_t kBuffer = 128;

 /// the least size of a level with a PGM-index, \f$ I \f$, the smaller
 /// ones being searched by a binary search with no branch
 static constexpr std::size_t kIndexed = I;

 /// an empty map
 DynamicPGMIndex( void ) = default;

 /// the map of the pairs \f$ P \f$, put in the one level that holds them
 /// \pre \f$ P \f$ sorted by key, no key twice (an audit)
 explicit DynamicPGMIndex( const std::vector< std::pair< K , V > > & P );

 /// maps \f$ k \f$ to \f$ v \f$, whether \f$ k \f$ was there or not
 ///
 /// In pseudocode (the logarithmic method of Bentley and Saxe, beyond the
 /// book), \f$ \| L \| \f$ the pairs of a level and
 /// \f$ c_i = b_0 B^i \f$ the capacity of \f$ L_i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ k \text{ is in } L_0\ \textbf{then}\ \text{set }
 ///     \text{its value to } v;\ \textbf{return} \cr
 /// 3 & \quad \textbf{if}\ \| L_0 \| < b_0\ \textbf{then}\ \text{insert }
 ///     ( k , v ) \text{ in } L_0 \text{ in order};\ \textbf{return} \cr
 /// 4 & \quad s \gets \| L_0 \| + 1;\ t \gets 1 \cr
 /// 5 & \quad \textbf{while}\ L_t \text{ exists and } s > c_t - \| L_t \|
 ///     \ \textbf{do}\ s \gets s + \| L_t \|;\ t \gets t + 1 \cr
 /// 6 & \quad M \gets L_0 \text{ with } ( k , v ) \cr
 /// 7 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ t\ \textbf{do}\ M \gets
 ///     \mathrm{merge}( M , L_i )\quad \triangleright\ \text{the pair of }
 ///     M \text{ wins} \cr
 /// 8 & \quad \textbf{if}\ L_t \text{ is the last level}\ \textbf{then}
 ///     \ \text{drop the tombstones of } M \cr
 /// 9 & \quad L_0 , \ldots , L_{t-1} \gets \emptyset;\ L_t \gets M
 ///     \text{, with a PGM-index on its keys if } \| M \| \ge I
 /// \end{array}}
 /// \f]
 void insert( const K & k , const V & v );

 /// removes \f$ k \f$, if there, by the insertion of a tombstone
 void erase( const K & k );

 /// the value of \f$ k \f$, `nullptr` if absent; valid until the next
 /// insertion or removal
 ///
 /// In pseudocode (the logarithmic method of Bentley and Saxe, beyond the
 /// book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{find}( k ) \cr
 /// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ \text{the last level}
 ///     \ \textbf{do}\quad \triangleright\ \text{the newest first} \cr
 /// 3 & \qquad r \gets \mathrm{lowerBound}( L_i , k )\quad \triangleright
 ///     \ \text{by the PGM-index of } L_i \text{, if it has one} \cr
 /// 4 & \qquad \textbf{if}\ L_i[ r ] \text{ has the key } k\ \textbf{then}
 ///     \cr
 /// 5 & \qquad\quad \textbf{return}\ \textbf{nil}\ \text{if it is a }
 ///     \text{tombstone, else its value} \cr
 /// 6 & \quad \textbf{return}\ \textbf{nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] const V * find( const K & k ) const;

 /// whether \f$ k \f$ is there, by `find()`
 [[nodiscard]] bool contains( const K & k ) const;

 /// the pair of the least key \f$ \ge k \f$, none if there is none
 ///
 /// In pseudocode (the logarithmic method of Bentley and Saxe, beyond the
 /// book), a place \f$ r_i \f$ per level:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( k ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{level } i\ \textbf{do}\ r_i \gets
 ///     \mathrm{lowerBound}( L_i , k ) \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{if}\ \text{every } r_i \text{ is past its level}
 ///     \ \textbf{then return}\ \textbf{nil} \cr
 /// 5 & \qquad \kappa \gets \min_i L_i[ r_i ].\mathit{key};\ j \gets
 ///     \text{the least } i \text{ with } L_i[ r_i ].\mathit{key} = \kappa \cr
 /// 6 & \qquad \textbf{if}\ L_j[ r_j ] \text{ is not a tombstone}
 ///     \ \textbf{then return}\ L_j[ r_j ] \cr
 /// 7 & \qquad \textbf{foreach}\ i \text{ with } L_i[ r_i ].\mathit{key} =
 ///     \kappa\ \textbf{do}\ r_i \gets r_i + 1
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< std::pair< K , V > > lowerBound(
                                                       const K & k ) const;

 /// calls \f$ f( k , v ) \f$ on the pairs with \f$ lo \le k < hi \f$, by
 /// increasing key, by the merge of `lowerBound()`
 template< class F >
 void forEach( const K & lo , const K & hi , F && f ) const;

 /// the pairs stored, the tombstones and the pairs hidden by newer ones
 /// included
 [[nodiscard]] std::size_t entries( void ) const;

 /// the levels below the buffer, empty or not
 [[nodiscard]] std::size_t levels( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a pair, or a tombstone
 struct Entry {
  K key;          ///< the key
  V val;          ///< the value
  bool dead;      ///< whether it is a tombstone
  };

 /// a level: its keys, in the PGM-index if it has one, the values and the
 /// tombstones
 struct Level {
  std::vector< K > key;        ///< the keys, without an index
  PGMIndex< K , E , 4 > idx;   ///< the keys, with their index
  bool indexed = false;        ///< whether the keys are in idx
  std::vector< V > val;        ///< the values
  std::vector< std::uint8_t > dead;   ///< the tombstones, 1 for each

  /// the number of pairs
  [[nodiscard]] std::size_t size( void ) const {
   return( indexed ? idx.size() : key.size() );
   }
  /// the key \f$ i \f$
  [[nodiscard]] const K & at( std::size_t i ) const {
   return( indexed ? idx[ i ] : key[ i ] );
   }
  /// the keys, one after the other
  [[nodiscard]] const K * keys( void ) const {
   return( indexed ? ( idx.empty() ? nullptr : &idx[ 0 ] ) : key.data() );
   }
  /// the place of the first key \f$ \ge k \f$, by the PGM-index or by
  /// the binary search with no branch that prefetches the two places the
  /// next step may probe [see `branchlessLowerBound()`]
  [[nodiscard]] std::size_t lowerBound( const K & k ) const;
  };

 /// the sorted run of a merge, a key at a place of each vector
 struct Run {
  std::vector< K > key;        ///< the keys
  std::vector< V > val;        ///< the values
  std::vector< std::uint8_t > dead;   ///< the tombstones, 1 for each
  };

 /// the capacity of the level \f$ i + 1 \f$, \f$ b_0 B^{i+1} \f$
 [[nodiscard]] static std::size_t capacity( std::size_t i );

 /// inserts the pair \f$ e \f$, the body of `insert()` and `erase()`
 void put( const Entry & e );

 /// the merge of the run \f$ M \f$ with the older level \f$ L \f$ into
 /// \f$ T \f$, the pair of \f$ M \f$ winning a tie
 static void merge( const Run & M , const Level & L , Run & T );

 /// calls \f$ f( k , v ) \f$, while it returns true, on the pairs from the
 /// least key \f$ \ge lo \f$ on
 template< class F >
 void scan( const K & lo , F && f ) const;

 std::vector< Entry > f_buf;      ///< the buffer, sorted
 std::vector< Level > f_lev;      ///< the levels, the newest first

 };  // end( class DynamicPGMIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/DynamicPGMIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DynamicPGMIndex.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File DynamicPGMIndex.h -----------------------*/
/*--------------------------------------------------------------------------*/
