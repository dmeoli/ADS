/*--------------------------------------------------------------------------*/
/*------------------------ File ChainedHashTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash table with lists of overflow of section 7.2.1, a realization of
 * the DICTIONARY [see `ADT.h`]: the place \f$ i \f$ of a vector of size
 * \f$ m \f$ holds a LIST with the pairs whose keys have \f$ H( k ) = i \f$,
 * `insert()` puts a new key at the head of its list, `lookup()` and
 * `remove()` go along it. A list may also bring to its head the key just
 * found, so that the keys looked up often stay close to it (exercise 7.5),
 * or be kept sorted, so that an unsuccessful search stops at the first
 * larger key (exercise 7.10) [see `Chain`].
 *
 * With the load factor \f$ \alpha = n / m \f$, an unsuccessful search
 * accesses \f$ 1 + \alpha \f$ cells on average and a successful one
 * \f$ 1 + \alpha / 2 \f$; `probes()` gives the cells that the last
 * operation examined, the nil that ends an unsuccessful search included.
 * The table may restructure itself (section 7.4), doubling \f$ m \f$ when
 * \f$ \alpha > 2 \f$ and halving it, down to the initial size, when
 * \f$ \alpha < 1 / 2 \f$: the two thresholds are 4 times apart, so that
 * between two restructurings there are at least \f$ m / 4 \f$ operations and
 * the cost stays \f$ O( 1 ) \f$ amortized [see `TableSize.h`].
 *
 * The contracts [see `Contract.h`]: \f$ m > 0 \f$ and a list that exists for
 * `chain()`; after `insert()` the key is in the table and after `remove()`
 * it is not; the invariant is that each key is in the list of its own
 * place, that the lists are sorted when they must be, that `size()` counts
 * the keys, and that the key of the operation is in its list at most once.
 * On a table of more than 1024 lists the invariant is checked on the list
 * of that key only, so that a long sequence of operations at the level 3
 * stays linear; the checks do not change `probes()`. The operations are
 * defined in `ChainedHashTable.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.2-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see W. W. Peterson, Addressing for random-access storage, IBM Journal
 *      of Research and Development 1(2), 130-146, 1957,
 *      doi:10.1147/rd.12.0130
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

#ifndef __ADS_ChainedHashTable
 #define __ADS_ChainedHashTable
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/TableSize.h"
#include "ADS/sequence/List.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ChainedHashTable -------------------------*/
/*--------------------------------------------------------------------------*/

/// how a `ChainedHashTable` keeps its lists
enum class Chain {
 FRONT ,          ///< a new key at the head, nothing moves
 MOVE_TO_FRONT ,  ///< also the key found by `lookup()` goes to the head
 SORTED           ///< the keys of a list in increasing order
 };

/// a hash table with lists of overflow
template< class K , class V , Chain C = Chain::FRONT ,
          class H = std::hash< K > >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
class ChainedHashTable {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the empty table of \f$ m \f$ lists, which restructures itself or not
 /// \pre \f$ m > 0 \f$
 explicit ChainedHashTable( std::size_t m , bool restructure = false ,
                            H h = H() );

/*--------------------------------------------------------------------------*/

 /// the value of the key \f$ k \f$, or nothing
 ///
 /// In pseudocode (section 7.2.1, with the move to the front of exercise
 /// 7.5), \f$ T[ 0 .. m - 1 ] \f$ the lists; with `Chain::SORTED` the scan
 /// stops also at the first key larger than \f$ k \f$ (exercise 7.10):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lookup}( k ) \cr
 /// 2 & \quad L \gets T[ H( k ) ] \cr
 /// 3 & \quad \textbf{foreach}\ ( k' , v ) \in L\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ k' = k\ \textbf{then} \cr
 /// 5 & \qquad\quad \textbf{if}\ \text{the list moves to the front }
 ///     \ \textbf{then}\ \text{move } ( k , v ) \text{ to the head of } L \cr
 /// 6 & \qquad\quad \textbf{return}\ v \cr
 /// 7 & \quad \textbf{return nil}
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$
 ///
 /// In pseudocode (sections 7.2.1 and 7.4), the table doubling past the load
 /// 2 if it restructures itself [see `resized()`]:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad L \gets T[ H( k ) ] \cr
 /// 3 & \quad \textbf{if}\ k \text{ is in } L\ \textbf{then}\ \text{give }
 ///     \text{it the value } v;\ \textbf{return} \cr
 /// 4 & \quad \text{put } ( k , v ) \text{ at the head of } L \text{, or in}
 ///     \text{ order if } L \text{ is sorted} \cr
 /// 5 & \quad n \gets n + 1 \cr
 /// 6 & \quad \textbf{if}\ n > 2 m\ \textbf{then}\ \text{move every pair }
 ///     \text{to a table of } \mathrm{resized}( m ) \text{ lists}
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ is in the table
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, from its list; the table, if
 /// it restructures itself, halves below the load \f$ 1 / 2 \f$, down to its
 /// initial size
 /// \post \f$ k \f$ is not in the table
 void remove( const K & k );

 /// true if the key \f$ k \f$ is there
 [[nodiscard]] bool contains( const K & k ) const;

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the number \f$ m \f$ of lists
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor \f$ n / m \f$
 [[nodiscard]] double load( void ) const;

 /// the foreach of the book: `f( k , v )` on the pairs, list after list,
 /// \f$ O( m + n ) \f$ time
 template< class F >
 void forEach( F && f ) const;

 /// the keys of the list \f$ i \f$, from its head
 /// \pre \f$ i \f$ is less than `capacity()`
 [[nodiscard]] std::vector< K > chain( std::size_t i ) const;

 /// the cells of the list examined by the last operation, plus one for
 /// the nil that ends an unsuccessful search
 [[nodiscard]] std::size_t probes( void ) const;

 /// the number of restructurings so far
 [[nodiscard]] std::size_t restructures( void ) const;

 /// the number of pairs moved by the restructurings so far
 [[nodiscard]] std::size_t moved( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Pair = std::pair< K , V >;
 using Bucket = List< Pair , false >;   ///< a chain, short: no pool
 using Pos = typename Bucket::Pos;

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), list by list, forward;
 // with Chain::MOVE_TO_FRONT even a lookup moves a pair, and with the
 // restructuring an insertion or a removal rebuilds the table, both of
 // which invalidate it

 /// the iterator on the pairs, list by list
 class const_iterator {
  using Lists = std::vector< Bucket >;

  public:
  /// the category of the iterator
  using iterator_category = std::forward_iterator_tag;
  /// the type of the items, for the standard library
  using value_type = Pair;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// a reference to an item
  using reference = const Pair &;
  /// a pointer to an item
  using pointer = const Pair *;

  /// the end of no table
  const_iterator( void ) = default;

  /// on the first pair from the list \f$ b \f$, or at the end
  const_iterator( const Lists * T , std::size_t b ) : f_T( T ) , f_b( b ) {
   if( f_b < f_T->size() ) f_c = ( *f_T )[ f_b ].begin();
   settle();
   }

  /// the pair
  reference operator*( void ) const { return( *f_c ); }

  /// the address of the pair
  pointer operator->( void ) const { return( &*f_c ); }

  /// to the next pair, on the next list that is not empty after the last
  /// pair of a list
  const_iterator & operator++( void ) {
   ++f_c;
   settle();
   return( *this );
   }

  /// to the next pair, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// equal on the same pair, or both at the end
  bool operator==( const const_iterator & o ) const {
   return( ( f_b == o.f_b ) &&
           ( ( f_T == nullptr ) || ( f_b >= f_T->size() ) ||
             ( f_c == o.f_c ) ) );
   }

  private:
  void settle( void ) {                     // past the empty lists
   while( ( f_b < f_T->size() ) && ( f_c == ( *f_T )[ f_b ].end() ) )
    if( ++f_b < f_T->size() ) f_c = ( *f_T )[ f_b ].begin();
   }

  const Lists * f_T = nullptr;
  std::size_t f_b = 0;
  typename Bucket::const_iterator f_c;
  };

 /// the type of the items, for the standard library
 using value_type = Pair;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = const Pair &;
 /// a constant reference to an item
 using const_reference = const Pair &;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_T , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( &f_T , f_T.size() ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 /// the list of \f$ k \f$, \f$ H( k ) = h( k ) \bmod m \f$
[[nodiscard]] std::size_t place( const K & k ) const;

 /// the cell of \f$ k \f$, or where the search stopped: the end of the list,
 /// or for a sorted list the first larger key
 Pos find( const Bucket & L , const K & k ) const;

 /// whether \f$ k \f$ is in the table, without counting the probes
 [[nodiscard]] bool present( const K & k ) const;

 /// each key in the list of its place, the lists sorted if they must be,
 /// `size()` their total and \f$ k \f$ at most once, \f$ O( n + m ) \f$; on
 /// more than 1024 lists, only the list of \f$ k \f$
 [[nodiscard]] bool consistent( const K & k ) const;

 /// the keys of the list \f$ i \f$ in it, sorted if they must be, \f$ k \f$ at
 /// most once; their number goes to `n`
 [[nodiscard]] bool listOk( std::size_t i , const K & k ,
                            std::size_t & n ) const;

 /// every pair moved to a table of \f$ m \f$ lists
void rebuild( std::size_t m );

 mutable std::vector< Bucket > f_T;       ///< the lists
 std::size_t f_m0;                         ///< the initial size
 bool f_restructure;                       ///< whether it restructures
 H f_h;                                    ///< the hash of the keys
 std::size_t f_size = 0;                   ///< the number of keys
 mutable std::size_t f_probes = 0;         ///< see `probes()`
 std::size_t f_rebuilds = 0;               ///< see `restructures()`
 std::size_t f_moved = 0;                  ///< see `moved()`

 };  // end( class ChainedHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/ChainedHashTable.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ChainedHashTable.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File ChainedHashTable.h -----------------------*/
/*--------------------------------------------------------------------------*/
