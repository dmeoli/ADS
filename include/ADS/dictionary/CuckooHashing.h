/*--------------------------------------------------------------------------*/
/*-------------------------- File CuckooHashing.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cuckoo hashing of Pagh and Rodler, beyond the book: each key k is in one
 * of the two places \f$ h_1( k ) \f$ and \f$ h_2( k ) \f$ of a table of m
 * places, so that a search and a removal look at two places, \f$ O( 1 ) \f$
 * time in the worst case. An insertion puts k in the first empty one of its
 * two places, else in \f$ h_1( k ) \f$, evicting the key y there, which moves
 * to its other place, possibly evicting another key, and so on; after MaxLoop
 * evictions the insertion gives up and the table is rebuilt with two new
 * functions (a rehash). In the cuckoo graph, whose vertices are the places and
 * whose edges are the keys, the evictions follow a path from a place of k, and
 * an insertion fails only if the component of k has more than one cycle; for
 * \f$ m \ge 2 c n \f$ two places are joined by a shortest path of length L
 * with probability at most \f$ c^{-L} / m \f$ (Theorem 8.9 of the notes), a
 * key is in the bucket of another with probability \f$ O( 1 / m ) \f$, and a
 * cycle in the final graph has probability at most \f$ 1 / ( c - 1 ) \f$, at
 * most 1 / 2 for \f$ m \ge 6 n ( 1 + \epsilon ) \f$: an insertion takes
 * \f$ O( 1 ) \f$ amortized expected time. With one table and both functions on
 * it the graph is a random graph of m vertices and n edges, whose components
 * have at most one cycle with high probability while \f$ n / m < 1 / 2 \f$;
 * the table doubles past the load maxLoad (by default 0.4) and halves below
 * maxLoad / 4, the global rebuilding of the notes.
 *
 * A stash of s places (Kirsch, Mitzenmacher and Wieder) keeps the keys
 * whose insertion fails, lowering the probability of a rehash from
 * \f$ \Theta( 1 / n ) \f$ to \f$ \Theta( 1 / n^{s+1} ) \f$; a search also
 * looks at the stash. The two functions are simple tabulation [see
 * TabulationHash in RandomHashing.h] of the hash of the key, which, as
 * Patrascu and Thorup proved, keeps the failure probability of cuckoo
 * hashing at \f$ O( 1 / n^{1/3} ) \f$, while the analysis of Pagh and
 * Rodler asks for \f$ O( \log n ) \f$-independent functions (Aumueller,
 * Dietzfelbinger and Woelfel give explicit ones that suffice with a
 * stash). Compared with the open addressing [see OpenHashTable.h], a
 * search has no cluster to walk; the variants of today put several keys in
 * a place (blocked cuckoo hashing, loads above 90 %) or use d > 2
 * functions (Fotakis et al.).
 *
 * MaxLoop is \f$ 8 \lceil \log_2 m \rceil + 16 \f$, against the \f$ 3
 * \log_{1+\epsilon} m \f$ of the analysis of Pagh and Rodler.
 *
 * The contracts [see Contract.h]: m >= 2 and 0 < maxLoad < 1 / 2; insert()
 * ensures that the key is there and remove() that it is not; the invariant
 * is that each key is in one of its two places or in the stash and that the
 * keys are counted right, checked on the key of the operation only past
 * 1024 places, so that a long sequence of operations at the level 3 stays
 * linear. The operations are defined in CuckooHashing.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 8 (section 8.6)
 * \see R. Pagh, F. F. Rodler, Cuckoo hashing, Journal of Algorithms 51(2),
 *      122-144, 2004, doi:10.1016/j.jalgor.2003.12.002
 * \see A. Kirsch, M. Mitzenmacher, U. Wieder, More robust hashing: cuckoo
 *      hashing with a stash, SIAM Journal on Computing 39(4), 1543-1561,
 *      2009, doi:10.1137/080728743
 * \see M. Aumueller, M. Dietzfelbinger, P. Woelfel, Explicit and efficient
 *      hash families suffice for cuckoo hashing with a stash,
 *      Algorithmica 70(3), 428-456, 2014, doi:10.1007/s00453-013-9840-x
 * \see M. Patrascu, M. Thorup, The power of simple tabulation hashing,
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

 /// the empty table of m places and a stash of s places
 /// \pre m >= 2 and 0 < maxLoad < 1 / 2
 explicit CuckooHashTable( std::size_t m = 16 , double maxLoad = 0.4 ,
                           std::size_t s = 0 , std::uint64_t seed = 1 ,
                           H h = H() );

 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 /// the number m of places, the stash excluded
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
  bool full = false;
  K key{};
  V value{};
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places and then of the stash, bidirectional; an insertion or a removal
 // invalidates it

 /// the places and the stash, for SlotIterator
 struct Places {
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ].full );
   }
  static auto read( const auto & o , std::size_t i ) {
   return( std::pair< const K & , const V & >( o.f_T[ i ].key ,
                                              o.f_T[ i ].value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = std::pair< const K & , const V & >;
 using const_reference = std::pair< const K & , const V & >;
 using const_iterator = SlotIterator< CuckooHashTable , Places >;
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

 /// the place of k for the function i (0 or 1)
 [[nodiscard]] std::size_t place( const K & k , int i ) const;

 /// the place of k, in the table or in the stash, or none
 [[nodiscard]] std::optional< std::size_t > find( const K & k ) const;

 /// the walk of the evictions from c; true if c found a place, false if
 /// the key left homeless is in c
 bool walk( Cell & c );

 /// c in the table or in the stash; false if it is homeless
 bool settle( Cell & c );

 /// the table rebuilt on m places with new functions, with the extra key
 void rebuild( std::size_t m , std::optional< Cell > extra );

 /// the functions of the seed
 void draw( void );

 /// k in one of its places or in the stash and at most once, and, up to
 /// 1024 places, every key so and counted right
 [[nodiscard]] bool placed( const K & k ) const;

 std::vector< Cell > f_T;          ///< the m places, then the stash
 std::size_t f_m;                  ///< the places of the table
 std::size_t f_m0;                 ///< the least capacity
 std::size_t f_s;                  ///< the places of the stash
 double f_maxLoad;
 std::size_t f_n = 0;
 std::size_t f_rehashes = 0;
 std::size_t f_evictions = 0;
 SplitMix64 f_seeds;               ///< the seeds of the functions
 TabulationHash f_g1 , f_g2;      ///< the two functions
 H f_h;

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
