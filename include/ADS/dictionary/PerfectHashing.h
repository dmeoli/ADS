/*--------------------------------------------------------------------------*/
/*------------------------- File PerfectHashing.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Perfect hashing of a static set, beyond the book: a function is perfect
 * on a set S of n keys if it has no collision on S, minimal if its range
 * is \f$ \{ 0 , \ldots , n - 1 \} \f$, and order preserving if it maps the
 * keys in their order; a table built on it searches in O( 1 ) time in the
 * worst case, as the table with direct access [see DirectTable.h], but in
 * O( n ) space instead of the size of the universe.
 *
 * - PerfectHashTable is the two-level scheme of Fredman, Komlós and
 *   Szemerédi: the n keys are hashed into n lists by a function h of the
 *   class of Carter and Wegman [see CarterWegmanHash], drawn again until
 *   \f$ L = \sum_j n_j^2 < 2 n \f$, \f$ n_j \f$ being the keys of the list
 *   j (the expected L is below 2 n, since \f$ \sum_j \binom{n_j}{2} \f$
 *   counts the collisions of h); the list j becomes a table of \f$ n_j^2
 *   \f$ places with a function \f$ h_j \f$ of its own, drawn again until it
 *   has no collision, which happens with probability at least 1 / 2 since
 *   the expected collisions of \f$ n_j \f$ keys in \f$ n_j^2 \f$ places are
 *   below 1 / 2. A search computes h and \f$ h_j \f$ and compares one key:
 *   two accesses in the worst case, \f$ O( n ) \f$ expected time to build,
 *   below 3 n places in all. Its dynamic version, by Dietzfelbinger et al.,
 *   also inserts and removes in O( 1 ) amortized expected time; the
 *   compressed schemes of today (CHD of Belazzougui, Botelho and
 *   Dietzfelbinger, RecSplit of Esposito, Mueller Graf and Vigna) need
 *   about 2 and 1.6 bits per key instead of words.
 * - OrderedMinimalPerfectHash is the scheme of Czech, Havas and Majewski: r
 *   = 2 functions map each key t to the edge \f$ ( h_1( t ) , h_2( t ) )
 *   \f$ of a random graph of \f$ m' = c n \f$ vertices, labelled by the
 *   value wanted for t (its place in the input, i.e., its rank if the input
 *   is sorted); if the graph is acyclic, a visit of each tree from a root
 *   with \f$ g = 0 \f$ solves \f$ g( u ) + g( v ) = h( t ) \bmod n \f$ on
 *   each edge (Algorithms 8.1 and 8.2 of the notes, labelAcyclicGraph()),
 *   and \f$ h( t ) = ( g( h_1( t ) ) + g( h_2( t ) ) ) \bmod n \f$ is the
 *   function. For \f$ c > 2 \f$ the graph is acyclic with probability about
 *   \f$ \sqrt{( c - 2 ) / c} \f$, i.e., about two graphs for c = 3; with
 *   r = 3 functions into three blocks of vertices the edges are triples,
 *   the hypergraph is solved in the reverse order of its peeling (the
 *   removal of the vertices of degree 1, labelByPeeling()), and \f$ c >
 *   1.23 \f$ is enough, i.e., \f$ 1.23 n \f$ values instead of \f$ 3 n \f$.
 *   It stores no key: O( m' ) words, O( 1 ) time to evaluate, O( n )
 *   expected time to build; unlike a trie [see PatriciaTrie.h] it gives
 *   nothing meaningful for a key out of S, but its space does not depend on
 *   the length of the keys.
 *
 * The keys of PerfectHashTable are integers below \f$ 2^{61} - 1 \f$, the
 * prime of its functions; those of OrderedMinimalPerfectHash are integers
 * or strings, turned into integers by StringHash [see UniversalHashing.h]
 * or splitmix64() before the functions of Carter and Wegman.
 *
 * The contracts [see Contract.h]: the keys distinct (an audit), below
 * \f$ 2^{61} - 1 \f$ for PerfectHashTable and fewer than \f$ 2^{32} \f$
 * for OrderedMinimalPerfectHash, r = 2 with c > 2 or r = 3 with c > 1.23,
 * the vertices of an edge distinct for labelByPeeling(); after a build
 * each key is found, or mapped to its value, an audit; a labelling
 * satisfies every edge, a postcondition. A build that draws 1000 functions
 * of the first level, or 1000 graphs, with no success throws
 * std::runtime_error, which for distinct keys has a probability below
 * \f$ 2^{-100} \f$. The functions are defined in PerfectHashing.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8 (sections 8.4 and 8.5)
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
 *         Universita' di Pisa \n
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

 /// the table of the keys of S
 /// \pre the keys are distinct (an audit) and below 2^61 - 1
 /// \post each key of S is there (an audit), and sum_j n_j^2 < 2 n
 explicit PerfectHashTable( const std::vector< std::uint64_t > & S ,
                            std::uint64_t seed = 1 );

 /// any k, in O( 1 ) time in the worst case
 [[nodiscard]] bool contains( std::uint64_t k ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the places of the second level, sum_j n_j^2
 [[nodiscard]] std::size_t space( void ) const;

 /// the functions drawn by the build, of both levels
 [[nodiscard]] std::size_t trials( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::uint64_t kEmpty = ~std::uint64_t( 0 );
 static constexpr std::size_t kMaxTrials = 1000;

 struct Bucket {
  std::size_t offset = 0;          ///< of its table in f_T
  std::size_t size = 0;            ///< n_j^2, 0 for no key
  CarterWegmanHash h = CarterWegmanHash( 1 );
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the keys, in the order of the places, bidirectional

 /// the places of the second level, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ] != kEmpty );
   }
  static const std::uint64_t & read( const auto & o , std::size_t i ) {
   return( o.f_T[ i ] );
   }
  };

 using value_type = std::uint64_t;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const std::uint64_t &;
 using const_reference = const std::uint64_t &;
 using const_iterator = SlotIterator< PerfectHashTable , Places >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_T.size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 CarterWegmanHash f_h;             ///< the first level, n lists
 std::vector< Bucket > f_B;        ///< the tables of the second level
 std::vector< std::uint64_t > f_T; ///< their places, kEmpty if empty
 std::size_t f_n = 0;
 std::size_t f_trials = 0;

 };  // end( class PerfectHashTable )

/*--------------------------------------------------------------------------*/
/*-------------------- CLASS OrderedMinimalPerfectHash ---------------------*/
/*--------------------------------------------------------------------------*/

/// the function of Czech, Havas and Majewski that maps keys[ i ] to i
template< class K >
 requires( std::integral< K > ||
           std::convertible_to< const K & , std::string_view > )
class OrderedMinimalPerfectHash {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;   ///< the type of the keys

 /// the function of the distinct keys, on r = 2 or 3 functions and c n
 /// vertices; c = 0 stands for 3 if r = 2 and for 1.25 if r = 3
 /// \pre the keys are distinct (an audit) and fewer than 2^32, r = 2 with
 ///      c > 2 or r = 3 with c > 1.23 (or c = 0)
 /// \post ( *this )( keys[ i ] ) == i for each i (an audit)
 explicit OrderedMinimalPerfectHash( const std::vector< K > & keys ,
                                     int r = 2 , double c = 0 ,
                                     std::uint64_t seed = 1 );

 /// the value of k, in [ 0 , n ); anything in it for a key out of S
 /// \pre size() >= 1
 [[nodiscard]] std::size_t operator()( const K & k ) const;

 /// the number n of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the number m' of vertices, i.e., of values of g
 [[nodiscard]] std::size_t vertices( void ) const;

 /// the graphs drawn by the build
 [[nodiscard]] std::size_t trials( void ) const;

 /// Algorithms 8.1 and 8.2: g on the m vertices such that g[ u ] + g[ v ]
 /// = h[ e ] mod n on each edge e = ( E[ 2 e ] , E[ 2 e + 1 ] ), each tree
 /// labelled from its least vertex with g = 0; nothing if a visit finds an
 /// edge that it cannot satisfy (a cycle whose values do not add up)
 /// \pre n >= 1, E has 2 h.size() vertices below m
 /// \post every edge is satisfied, if a labelling is returned
 [[nodiscard]] static std::optional< std::vector< std::uint32_t > >
  labelAcyclicGraph( std::size_t m , const std::vector< std::size_t > & E ,
                     const std::vector< std::size_t > & h , std::size_t n );

 /// g on the m vertices such that the g of the r vertices of each
 /// hyperedge e add up to h[ e ] mod n, by the reverse order of the
 /// peeling; nothing if the hypergraph has a 2-core (is not peelable)
 /// \pre n >= 1, r >= 2, E has r h.size() vertices below m, those of an
 ///      edge distinct
 /// \post every edge is satisfied, if a labelling is returned
 [[nodiscard]] static std::optional< std::vector< std::uint32_t > >
  labelByPeeling( std::size_t m , const std::vector< std::size_t > & E ,
                  int r , const std::vector< std::size_t > & h ,
                  std::size_t n );

/*--------------------------------------------------------------------------*/
 private:

 static constexpr std::size_t kMaxTrials = 1000;

 /// the integer of k for the function i, below 2^61 - 1
 [[nodiscard]] std::uint64_t fingerprint( const K & k , int i ) const;

 /// the vertex of k for the function i
 [[nodiscard]] std::size_t vertex( const K & k , int i ) const;

 int f_r;
 std::size_t f_n = 0;
 std::size_t f_m = 0;              ///< the vertices
 std::size_t f_block = 0;          ///< the vertices of a function
 std::size_t f_trials = 0;
 std::vector< std::uint64_t > f_seed;          ///< of each fingerprint
 std::vector< CarterWegmanHash > f_h;          ///< the r functions
 std::vector< std::uint32_t > f_g;

 };  // end( class OrderedMinimalPerfectHash )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/PerfectHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PerfectHashing.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File PerfectHashing.h ------------------------*/
/*--------------------------------------------------------------------------*/
