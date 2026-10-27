/*--------------------------------------------------------------------------*/
/*-------------------------- File CuckooHashing.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cuckoo hashing of Pagh and Rodler, beyond the book: each key \f$ k \f$
 * is in one of the two places \f$ h_1( k ) \f$ and \f$ h_2( k ) \f$ of a
 * table of \f$ m \f$ places, so that a search and a removal look at two
 * places, \f$ O( 1 ) \f$ time in the worst case. An insertion puts \f$ k \f$
 * in the first empty one of its two places, else in \f$ h_1( k ) \f$,
 * evicting the key \f$ y \f$ there, which moves to its other place, possibly
 * evicting another key, and so on; after \f$ \mathit{MaxLoop} \f$ evictions
 * the insertion gives up and the table is rebuilt with two new functions (a
 * rehash). In the cuckoo graph, whose vertices are the places and whose
 * edges are the keys, the evictions follow a path from a place of \f$ k \f$,
 * and an insertion fails only if the component of \f$ k \f$ has more than
 * one cycle; for \f$ m \ge 2 c n \f$ two places are joined by a shortest
 * path of length \f$ L \f$ with probability at most \f$ c^{-L} / m \f$ (as
 * chapter 8 of Ferragina shows), a key is in the bucket of another with
 * probability \f$ O( 1 / m ) \f$, and a cycle in the final graph has
 * probability at most \f$ 1 / ( c - 1 ) \f$, at most \f$ 1 / 2 \f$ for
 * \f$ m \ge 6 n ( 1 + \epsilon ) \f$: an insertion takes \f$ O( 1 ) \f$
 * amortized expected time. With one table and both functions on it the
 * graph is a random graph of \f$ m \f$ vertices and \f$ n \f$ edges, whose
 * components have at most one cycle with high probability while
 * \f$ n / m < 1 / 2 \f$; the table doubles past the load `maxLoad` (by
 * default 0.4) and halves below \f$ \mathit{maxLoad} / 4 \f$, the global
 * rebuilding of chapter 8 of Ferragina.
 *
 * A stash of \f$ s \f$ places (Kirsch, Mitzenmacher and Wieder) keeps the
 * keys whose insertion fails, lowering the probability of a rehash from
 * \f$ \Theta( 1 / n ) \f$ to \f$ \Theta( 1 / n^{s+1} ) \f$; a search also
 * looks at the stash. The two functions are simple tabulation [see
 * `TabulationHash` in `RandomHashing.h`] of the hash of the key, which, as
 * Pătrașcu and Thorup proved, keeps the failure probability of cuckoo
 * hashing at \f$ O( 1 / n^{1/3} ) \f$, while the analysis of Pagh and
 * Rodler asks for \f$ O( \log n ) \f$-independent functions (Aumueller,
 * Dietzfelbinger and Woelfel give explicit ones that suffice with a
 * stash). Compared with the open addressing [see `OpenHashTable.h`], a
 * search has no cluster to walk; the variants of today put several keys in
 * a place (blocked cuckoo hashing, loads above 90 %) or use \f$ d > 2 \f$
 * functions (Fotakis et al.).
 *
 * \f$ \mathit{MaxLoop} \f$ is \f$ 8 \lceil \log_2 m \rceil + 16 \f$, against
 * the \f$ 3 \log_{1+\epsilon} m \f$ of the analysis of Pagh and Rodler.
 *
 * The contracts [see `Contract.h`]: \f$ m \ge 2 \f$ and
 * \f$ 0 < \mathit{maxLoad} < 1 / 2 \f$; `insert()` ensures that the key is
 * there and `remove()` that it is not; the invariant is that each key is in
 * one of its two places or in the stash and that the keys are counted
 * right, checked on the key of the operation only past 1024 places, so that
 * a long sequence of operations at the level 3 stays linear. The operations
 * are defined in `CuckooHashing.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8, The Dictionary Problem,
 *      doi:10.1017/9781009128933
 * \see R. Pagh, F. F. Rodler, Cuckoo hashing, Journal of Algorithms 51(2),
 *      122-144, 2004, doi:10.1016/j.jalgor.2003.12.002
 * \see A. Kirsch, M. Mitzenmacher, U. Wieder, More robust hashing: cuckoo
 *      hashing with a stash, SIAM Journal on Computing 39(4), 1543-1561,
 *      2009, doi:10.1137/080728743
 * \see M. Aumueller, M. Dietzfelbinger, P. Woelfel, Explicit and efficient
 *      hash families suffice for cuckoo hashing with a stash,
 *      Algorithmica 70(3), 428-456, 2014, doi:10.1007/s00453-013-9840-x
 * \see M. Pătrașcu, M. Thorup, The power of simple tabulation hashing,
 *      Journal of the ACM 59(3), article 14, 2012,
 *      doi:10.1145/2220357.2220361
 * \see D. Fotakis, R. Pagh, P. Sanders, P. Spirakis, Space efficient hash
 *      tables with worst case constant access time, Theory of Computing
 *      Systems 38(2), 229-248, 2005, doi:10.1007/s00224-004-1195-x
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

#ifndef __ADS_CuckooHashing
 #define __ADS_CuckooHashing
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CuckooHashTable --------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on the cuckoo hashing, with a stash
template< class K , class V , class H = std::hash< K > >
class CuckooHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty table of \f$ m \f$ places and a stash of \f$ s \f$ places
 /// \pre \f$ m \ge 2 \f$ and \f$ 0 < \mathit{maxLoad} < 1 / 2 \f$
 explicit CuckooHashTable( std::size_t m = 16 , double maxLoad = 0.4 ,
                           std::size_t s = 0 , std::uint64_t seed = 1 ,
                           H h = H() );

 /// the value of the key \f$ k \f$, or nothing, from its two places
 /// \f$ h_1( k ) \f$ and \f$ h_2( k ) \f$ and the stash, \f$ O( 1 ) \f$ in the
 /// worst case
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$, in \f$ O( 1 ) \f$
 /// amortized expected time
 ///
 /// In pseudocode (Pagh and Rodler, chapter 8 of Ferragina),
 /// \f$ T \f$ the table, \f$ c \f$ the key in hand and \f$ \mathit{maxLoop} =
 /// 8 \lceil \log_2 m \rceil + 16 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ k \text{ is there }\ \textbf{then}\ \text{give it }
 ///     \text{the value } v;\ \textbf{return} \cr
 /// 3 & \quad \textbf{if}\ n + 1 > \mathit{maxLoad} \cdot m\ \textbf{then}
 ///     \ \text{rebuild on } 2 m \text{ places} \cr
 /// 4 & \quad c \gets ( k , v ) \cr
 /// 5 & \quad \textbf{if}\ T[ h_1( k ) ] \text{ or } T[ h_2( k ) ] \text{ is }
 ///     \text{empty}\ \textbf{then}\ \text{put } c \text{ there};
 ///     \ \textbf{return} \cr
 /// 6 & \quad p \gets h_1( k ) \cr
 /// 7 & \quad \textbf{for}\ t \gets 1\ \textbf{to}\ \mathit{maxLoop}
 ///     \ \textbf{do} \cr
 /// 8 & \qquad \text{swap } c \text{ and } T[ p ]\quad \triangleright
 ///     \ c \text{ is the key evicted} \cr
 /// 9 & \qquad p \gets \text{the other place of } c \cr
 /// 10 & \qquad \textbf{if}\ T[ p ] \text{ is empty }\ \textbf{then}\ T[ p ]
 ///     \gets c;\ \textbf{return} \cr
 /// 11 & \quad \textbf{if}\ \text{the stash has an empty place }
 ///     \ \textbf{then}\ \text{put } c \text{ there} \cr
 /// 12 & \quad \textbf{else}\ \text{rebuild with new functions and } c
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, emptying its place; the table
 /// is halved below a load of \f$ \mathit{maxLoad} / 4 \f$
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the number \f$ m \f$ of places, the stash excluded
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the rebuilds with new functions after a failed insertion
 [[nodiscard]] std::size_t rehashes( void ) const;

 /// the evictions made by the insertions
 [[nodiscard]] std::size_t evictions( void ) const;

 /// the keys in the stash
 [[nodiscard]] std::size_t stashed( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 struct Cell {
  bool full = false;              ///< whether the cell holds a key
  K key{};                        ///< the key
  V value{};                      ///< its value
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places and then of the stash, bidirectional; an insertion or a removal
 // invalidates it

 /// the places and the stash, for `SlotIterator`
 struct Places {
  /// the number of places, the stash included
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  /// whether the place \f$ i \f$ holds a key
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ].full );
   }
  /// the pair of the place \f$ i \f$
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< const K & , const V & >( o.f_T[ i ].key ,
                                              o.f_T[ i ].value ) );
   }
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< const K & , const V & >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = std::pair< const K & , const V & >;
 /// a constant reference to an item
 using const_reference = std::pair< const K & , const V & >;
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< CuckooHashTable , Places >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_T.size() ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the place of \f$ k \f$ for the function \f$ i \f$ (0 or 1)
 [[nodiscard]] std::size_t place( const K & k , int i ) const;

 /// the place of \f$ k \f$, in the table or in the stash, or none
 [[nodiscard]] std::optional< std::size_t > find( const K & k ) const;

 /// the walk of the evictions from \f$ c \f$; true if \f$ c \f$ found a
 /// place, false if the key left homeless is in \f$ c \f$
 bool walk( Cell & c );

 /// \f$ c \f$ in the table or in the stash; false if it is homeless
 bool settle( Cell & c );

 /// the table rebuilt on \f$ m \f$ places with new functions, with the
/// `extra` key
 void rebuild( std::size_t m , std::optional< Cell > extra );

 /// the functions of the seed
 void draw( void );

 /// \f$ k \f$ in one of its places or in the stash and at most once, and,
 /// up to 1024 places, every key so and counted right
 [[nodiscard]] bool placed( const K & k ) const;

 std::vector< Cell > f_T;          ///< the m places, then the stash
 std::size_t f_m;                  ///< the places of the table
 std::size_t f_m0;                 ///< the least capacity
 std::size_t f_s;                  ///< the places of the stash
 double f_maxLoad;                 ///< the load past which it doubles
 std::size_t f_n = 0;              ///< the number of keys
 std::size_t f_rehashes = 0;       ///< the rebuilds with new functions
 std::size_t f_evictions = 0;      ///< the evictions
 SplitMix64 f_seeds;               ///< the seeds of the functions
 TabulationHash f_g1 , f_g2;      ///< the two functions
 H f_h;                            ///< the hash of the keys

 };  // end( class CuckooHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/CuckooHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* CuckooHashing.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File CuckooHashing.h ------------------------*/
/*--------------------------------------------------------------------------*/
