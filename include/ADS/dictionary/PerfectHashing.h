/*--------------------------------------------------------------------------*/
/*------------------------- File PerfectHashing.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Perfect hashing of a static set, beyond the book: a function is perfect on
 * a set \f$ S \f$ of \f$ n \f$ keys if it has no collision on \f$ S \f$,
 * minimal if its range is \f$ \{ 0 , \ldots , n - 1 \} \f$, and order
 * preserving if it maps the keys in their order; a table built on it
 * searches in \f$ O( 1 ) \f$ time in the worst case, as the table with direct
 * access [see `DirectTable.h`], but in \f$ O( n ) \f$ space instead of the
 * size of the universe.
 *
 * - `PerfectHashTable` is the two-level scheme of Fredman, Komlós and
 *   Szemerédi: the \f$ n \f$ keys are hashed into \f$ n \f$ lists by a
 *   function \f$ h \f$ of the class of Carter and Wegman [see
 *   `CarterWegmanHash`], drawn again until \f$ L = \sum_j n_j^2 < 4 n \f$,
 *   \f$ n_j \f$ being the keys of the list \f$ j \f$ (the expected \f$ L \f$
 *   is below \f$ 2 n \f$, since \f$ \sum_j \binom{n_j}{2} \f$ counts the
 *   collisions of \f$ h \f$, so that by the inequality of Markov on
 *   \f$ L - n \ge 0 \f$, of expectation below \f$ n \f$, \f$ L \ge 4 n \f$
 *   with probability below \f$ 1 / 3 \f$ and \f$ h \f$ is drawn fewer than
 *   \f$ 3 / 2 \f$ times on average); the list \f$ j \f$ becomes a table of
 *   \f$ n_j^2 \f$ places with a function \f$ h_j \f$ of its own, drawn again
 *   until it has no collision, which happens with probability at least
 *   \f$ 1 / 2 \f$ since the expected collisions of \f$ n_j \f$ keys in
 *   \f$ n_j^2 \f$ places are below \f$ 1 / 2 \f$. A search computes
 *   \f$ h \f$ and \f$ h_j \f$ and compares one key: two accesses in the worst
 *   case, \f$ O( n ) \f$ expected time to build, below \f$ 5 n \f$ places in
 *   all. Its dynamic version, by Dietzfelbinger et al., also inserts and
 *   removes in \f$ O( 1 ) \f$ amortized expected time; the compressed
 *   schemes of today (CHD of Belazzougui, Botelho and Dietzfelbinger,
 *   RecSplit of Esposito, Mueller Graf and Vigna) need about 2 and 1.6 bits
 *   per key instead of words.
 * - `OrderedMinimalPerfectHash` is the scheme of Czech, Havas and Majewski:
 *   \f$ r = 2 \f$ functions map each key \f$ t \f$ to the edge
 *   \f$ ( h_1( t ) , h_2( t ) ) \f$ of a random graph of \f$ m' = c n \f$
 *   vertices, labeled by the value wanted for \f$ t \f$ (its place in the
 *   input, i.e., its rank if the input is sorted); if the graph is acyclic,
 *   a visit of each tree from a root with \f$ g = 0 \f$ solves
 *   \f$ g( u ) + g( v ) = h( t ) \bmod n \f$ on each edge
 *   (`labelAcyclicGraph()`), and
 *   \f$ h( t ) = ( g( h_1( t ) ) + g( h_2( t ) ) ) \bmod n \f$ is the
 *   function. For \f$ c > 2 \f$ the graph is acyclic with probability about
 *   \f$ \sqrt{( c - 2 ) / c} \f$, i.e., about two graphs for \f$ c = 3 \f$;
 *   with \f$ r = 3 \f$ functions into three blocks of vertices the edges are
 *   triples, the hypergraph is solved in the reverse order of its peeling
 *   (the removal of the vertices of degree 1, `labelByPeeling()`), and
 *   \f$ c > 1.23 \f$ is enough, i.e., \f$ 1.23 n \f$ values instead of
 *   \f$ 3 n \f$. It stores no key: \f$ O( m' ) \f$ words, \f$ O( 1 ) \f$ time
 *   to evaluate, \f$ O( n ) \f$ expected time to build; unlike a trie [see
 *   `PatriciaTrie.h`] it gives nothing meaningful for a key out of
 *   \f$ S \f$, but its space does not depend on the length of the keys.
 *
 * The keys of `PerfectHashTable` are integers below \f$ 2^{61} - 1 \f$, the
 * prime of its functions; those of `OrderedMinimalPerfectHash` are integers
 * or strings, turned into integers by `StringHash` [see
 * `UniversalHashing.h`] or `splitmix64()` before the functions of Carter and
 * Wegman.
 *
 * The contracts [see `Contract.h`]: the keys distinct (an audit), below
 * \f$ 2^{61} - 1 \f$ for `PerfectHashTable` and fewer than \f$ 2^{32} \f$
 * for `OrderedMinimalPerfectHash`, \f$ r = 2 \f$ with \f$ c > 2 \f$ or
 * \f$ r = 3 \f$ with \f$ c > 1.23 \f$, the vertices of an edge distinct for
 * `labelByPeeling()`; after a build each key is found, or mapped to its
 * value, an audit; a labeling satisfies every edge, a postcondition. A
 * build that draws 1000 functions of the first level, or 1000 graphs, with
 * no success throws `std::runtime_error`, which for distinct keys has a
 * probability below \f$ 2^{-100} \f$. The functions are defined in
 * `PerfectHashing.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8, The Dictionary Problem,
 *      doi:10.1017/9781009128933
 * \see M. L. Fredman, J. Komlós, E. Szemerédi, Storing a sparse table with
 *      O(1) worst case access time, Journal of the ACM 31(3), 538-544,
 *      1984, doi:10.1145/828.1884
 * \see Z. J. Czech, G. Havas, B. S. Majewski, An optimal algorithm for
 *      generating minimal perfect hash functions, Information Processing
 *      Letters 43(5), 257-264, 1992, doi:10.1016/0020-0190(92)90220-P
 * \see B. S. Majewski, N. C. Wormald, G. Havas, Z. J. Czech, A family of
 *      perfect hashing methods, The Computer Journal 39(6), 547-554, 1996,
 *      doi:10.1093/comjnl/39.6.547
 * \see M. Dietzfelbinger, A. Karlin, K. Mehlhorn, F. Meyer auf der Heide,
 *      H. Rohnert, R. E. Tarjan, Dynamic perfect hashing: upper and lower
 *      bounds, SIAM Journal on Computing 23(4), 738-761, 1994,
 *      doi:10.1137/S0097539791194094
 * \see D. Belazzougui, F. C. Botelho, M. Dietzfelbinger, Hash, displace,
 *      and compress, Proceedings of ESA 2009, LNCS 5757, 682-693, 2009,
 *      doi:10.1007/978-3-642-04128-0_61
 * \see E. Esposito, T. Mueller Graf, S. Vigna, RecSplit: minimal perfect
 *      hashing via recursive splitting, Proceedings of ALENEX 2020,
 *      175-185, 2020, doi:10.1137/1.9781611976007.14
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

#ifndef __ADS_PerfectHashing
 #define __ADS_PerfectHashing
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/dictionary/UniversalHashing.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS PerfectHashTable --------------------------*/
/*--------------------------------------------------------------------------*/

/// a static SET of integers on the two-level perfect hashing
class PerfectHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = std::uint64_t;   ///< the type of the keys

 /// the table of the keys of \f$ S \f$
 ///
 /// In pseudocode (Fredman, Komlós and Szemerédi, chapter 8 of
 /// Ferragina), \f$ n_j \f$ the keys of \f$ S \f$ with \f$ h( k ) = j \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( S ) \cr
 /// 2 & \quad \textbf{repeat}\ h \gets \text{a function of } H_{p,n}
 ///     \text{ at random}\ \textbf{until}\ \sum_j n_j^2 < 4 n \cr
 /// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n - 1\ \textbf{with}\ n_j
 ///     > 0\ \textbf{do} \cr
 /// 4 & \qquad \textbf{repeat}\ h_j \gets \text{a function of } H_{p,n_j^2}
 ///     \text{ at random} \cr
 /// 5 & \qquad \textbf{until}\ h_j \text{ has no collision on the keys of }
 ///     \text{the list } j \cr
 /// 6 & \qquad \text{put each key } k \text{ of the list in the place } h_j( k
 ///     ) \text{ of the table } j
 /// \end{array}}
 /// \f]
 /// Each table of the second level is found after fewer than 2 trials on
 /// average, since its expected collisions are below \f$ 1 / 2 \f$; the
 /// first level stops when \f$ \sum_j n_j^2 < 4 n \f$, which by the
 /// inequality of Markov on \f$ \sum_j n_j^2 - n \ge 0 \f$, of expectation
 /// below \f$ n \f$, happens with probability above \f$ 2 / 3 \f$ (a
 /// threshold of \f$ 2 n \f$ would guarantee only \f$ 1 / n \f$).
 /// \pre the keys are distinct (an audit) and below \f$ 2^{61} - 1 \f$
 /// \post each key of \f$ S \f$ is there (an audit), and
 ///       \f$ \sum_j n_j^2 < 4 n \f$
 explicit PerfectHashTable( const std::vector< std::uint64_t > & S ,
                            std::uint64_t seed = 1 );

 /// whether \f$ k \f$ is in \f$ S \f$, any \f$ k \f$, in \f$ O( 1 ) \f$ time
/// in the worst case: the key in the place \f$ h_j( k ) \f$ of the table
/// \f$ j = h( k ) \f$ compared with \f$ k \f$
 [[nodiscard]] bool contains( std::uint64_t k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the places of the second level, \f$ \sum_j n_j^2 \f$
 [[nodiscard]] std::size_t space( void ) const;

 /// the functions drawn by the build, of both levels
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::uint64_t kEmpty = ~std::uint64_t( 0 );
 static constexpr std::size_t kMaxTrials = 1000;

 struct Bucket {
  std::size_t offset = 0;          ///< of its table in `f_T`
  std::size_t size = 0;            ///< \f$ n_j^2 \f$, 0 for no key
  CarterWegmanHash h = CarterWegmanHash( 1 );   ///< its function \f$ h_j \f$
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the keys, in the order of the places, bidirectional

 /// the places of the second level, for `SlotIterator`
 struct Places {
  /// the number of places
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  /// whether the place \f$ i \f$ holds a key
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ] != kEmpty );
   }
  /// the key of the place \f$ i \f$
  static const std::uint64_t & read( const auto & o , std::size_t i ) {
   return( o.f_T[ i ] );
   }
  };

 /// the type of the items, for the standard library
 using value_type = std::uint64_t;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = const std::uint64_t &;
 /// a constant reference to an item
 using const_reference = const std::uint64_t &;
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< PerfectHashTable , Places >;
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

 CarterWegmanHash f_h;             ///< the first level, n lists
 std::vector< Bucket > f_B;        ///< the tables of the second level
 std::vector< std::uint64_t > f_T; ///< their places, `kEmpty` if empty
 std::size_t f_n = 0;              ///< the number of keys
 std::size_t f_trials = 0;         ///< the functions drawn

 };  // end( class PerfectHashTable )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS OrderedMinimalPerfectHash ---------------------*/
/*--------------------------------------------------------------------------*/

/// the function of Czech, Havas and Majewski that maps
/// \f$ \mathit{keys}[ i ] \f$ to \f$ i \f$
template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
class OrderedMinimalPerfectHash {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;   ///< the type of the keys

 /// the function of the distinct keys, on \f$ r = 2 \f$ or 3 functions and
 /// \f$ c n \f$ vertices; \f$ c = 0 \f$ stands for 3 if \f$ r = 2 \f$ and for
 /// 1.25 if \f$ r = 3 \f$
 ///
 /// In pseudocode (Czech, Havas and Majewski, chapter 8 of
 /// Ferragina), the key \f$ t_i \f$ an edge of value \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( t_0 , \ldots , t_{n-1} ) \cr
 /// 2 & \quad \textbf{repeat} \cr
 /// 3 & \qquad h_1 , \ldots , h_r \gets \text{functions at random} \cr
 /// 4 & \qquad E \gets \{ ( h_1( t_i ) , \ldots , h_r( t_i ) ) : 0 \le i < n
 ///     \} \cr
 /// 5 & \qquad g \gets \mathrm{labelAcyclicGraph}( E )\ \text{(}r = 2
 ///     \text{) or } \mathrm{labelByPeeling}( E )\ \text{(}r = 3\text{)} \cr
 /// 6 & \quad \textbf{until}\ g \ne \textbf{nil}
 /// \end{array}}
 /// \f]
 /// \pre the keys are distinct (an audit) and fewer than \f$ 2^{32} \f$,
 ///      \f$ r = 2 \f$ with \f$ c > 2 \f$ or \f$ r = 3 \f$ with
 ///      \f$ c > 1.23 \f$ (or \f$ c = 0 \f$)
 /// \post `( *this )( keys[ i ] ) == i` for each \f$ i \f$ (an audit)
 explicit OrderedMinimalPerfectHash( const std::vector< K > & keys ,
                                     int r = 2 , double c = 0 ,
                                     std::uint64_t seed = 1 );

 /// the value of \f$ k \f$,
 /// \f$ ( g( h_1( k ) ) + \cdots + g( h_r( k ) ) ) \bmod n \f$, in
 /// \f$ [ 0 , n ) \f$; anything in it for a key out of \f$ S \f$
 /// \pre `size()` \f$ \ge 1 \f$
 [[nodiscard]] std::size_t operator()( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the number \f$ m' \f$ of vertices, i.e., of values of \f$ g \f$
 [[nodiscard]] std::size_t vertices( void ) const;

 /// the graphs drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

 /// the labeling of an acyclic graph: \f$ g \f$ on the \f$ m \f$
 /// vertices such that \f$ g[ u ] + g[ v ] = h[ e ] \bmod n \f$ on each edge
 /// \f$ e = ( E[ 2 e ] , E[ 2 e + 1 ] ) \f$, each tree labeled from its least
 /// vertex with \f$ g = 0 \f$; nothing if a visit finds an edge that it cannot
 /// satisfy (a cycle whose values do not add up)
 ///
 /// In pseudocode (chapter 8 of Ferragina), a visit
 /// in depth of each tree with a stack:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{labelAcyclicGraph}( m , E , h , n ) \cr
 /// 2 & \quad \textbf{foreach}\ v\ \textbf{do}\ g[ v ] \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{for}\ v_0 \gets 0\ \textbf{to}\ m - 1\ \textbf{with}\ g[
 ///     v_0 ] = \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad g[ v_0 ] \gets 0;\ \mathrm{push}( S , v_0 ) \cr
 /// 5 & \qquad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
 /// 6 & \qquad\quad v \gets \mathrm{pop}( S ) \cr
 /// 7 & \qquad\quad \textbf{foreach}\ \text{edge } e = ( v , u )\ \textbf{do}
 ///     \cr
 /// 8 & \qquad\qquad x \gets ( h[ e ] - g[ v ] ) \bmod n \cr
 /// 9 & \qquad\qquad \textbf{if}\ g[ u ] = \textbf{nil}\ \textbf{then}\ g[ u ]
 ///     \gets x;\ \mathrm{push}( S , u ) \cr
 /// 10 & \qquad\qquad \textbf{else if}\ g[ u ] \ne x\ \textbf{then return nil}
 ///     \cr
 /// 11 & \quad \textbf{return}\ g
 /// \end{array}}
 /// \f]
 /// \pre \f$ n \ge 1 \f$, \f$ E \f$ has \f$ 2 |h| \f$ vertices below
 ///      \f$ m \f$
 /// \post every edge is satisfied, if a labeling is returned
 [[nodiscard]] static std::optional< std::vector< std::uint32_t > >
  labelAcyclicGraph( std::size_t m , const std::vector< std::size_t > & E ,
                     const std::vector< std::size_t > & h , std::size_t n );

 /// \f$ g \f$ on the \f$ m \f$ vertices such that the \f$ g \f$ of the
 /// \f$ r \f$ vertices of each hyperedge \f$ e \f$ add up to
 /// \f$ h[ e ] \bmod n \f$, by the reverse order of the peeling; nothing if
 /// the hypergraph has a 2-core (is not peelable)
 ///
 /// In pseudocode (Majewski, Wormald, Havas and Czech, beyond the book), a
 /// vertex of degree 1 knowing its edge as the XOR of the edges on it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{labelByPeeling}( m , E , r , h , n ) \cr
 /// 2 & \quad Q \gets \text{the vertices of degree 1};\ O \gets \text{an }
 ///     \text{empty sequence} \cr
 /// 3 & \quad \textbf{while}\ Q \ne \emptyset\ \textbf{do} \cr
 /// 4 & \qquad v \gets \mathrm{dequeue}( Q ) \cr
 /// 5 & \qquad \textbf{if}\ v \text{ has degree 1 }\ \textbf{then} \cr
 /// 6 & \qquad\quad e \gets \text{the edge of } v;\ \text{append } ( e , v )
 ///     \text{ to } O \cr
 /// 7 & \qquad\quad \text{remove } e \text{, enqueueing the vertices left of }
 ///     \text{degree 1} \cr
 /// 8 & \quad \textbf{if}\ |O| < |h|\ \textbf{then return nil}\quad
 ///     \triangleright\ \text{a 2-core is left} \cr
 /// 9 & \quad \textbf{foreach}\ ( e , v ) \in O \text{ backwards}
 ///     \ \textbf{do}\ g[ v ] \gets ( h[ e ] - \sum_{u \in e , u \ne v} g[ u ]
 ///     )
 ///     \bmod n \cr
 /// 10 & \quad \textbf{return}\ g
 /// \end{array}}
 /// \f]
 /// Each vertex \f$ v \f$ is labeled after all the other vertices of its edge
 /// \f$ e \f$, which are removed later or never, and no edge labeled later
 /// contains \f$ v \f$; hence every edge is satisfied.
 /// \pre \f$ n \ge 1 \f$, \f$ r \ge 2 \f$, \f$ E \f$ has \f$ r |h| \f$
 ///      vertices below \f$ m \f$, those of an edge distinct
 /// \post every edge is satisfied, if a labeling is returned
 [[nodiscard]] static std::optional< std::vector< std::uint32_t > >
  labelByPeeling( std::size_t m , const std::vector< std::size_t > & E ,
                  int r , const std::vector< std::size_t > & h ,
                  std::size_t n );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxTrials = 1000;

 /// the integer of \f$ k \f$ for the function \f$ i \f$, below
/// \f$ 2^{61} - 1 \f$
 [[nodiscard]] std::uint64_t fingerprint( const K & k , int i ) const;

 /// the vertex of \f$ k \f$ for the function \f$ i \f$
 [[nodiscard]] std::size_t vertex( const K & k , int i ) const;

 int f_r;                          ///< the number \f$ r \f$ of functions
 std::size_t f_n = 0;              ///< the number of keys
 std::size_t f_m = 0;              ///< the vertices
 std::size_t f_block = 0;          ///< the vertices of a function
 std::size_t f_trials = 0;         ///< the graphs drawn
 std::vector< std::uint64_t > f_seed;          ///< of each fingerprint
 std::vector< CarterWegmanHash > f_h;          ///< the r functions
 std::vector< std::uint32_t > f_g;             ///< the labels \f$ g \f$
 };  // end( class OrderedMinimalPerfectHash )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PerfectHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PerfectHashing.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File PerfectHashing.h ------------------------*/
/*--------------------------------------------------------------------------*/
