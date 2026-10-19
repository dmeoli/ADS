/*--------------------------------------------------------------------------*/
/*------------------------ File ChainedHashTable.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash table with lists of overflow of section 7.2.1, a realization of
 * the DICTIONARY [see ADT.h]: the place i of a vector of size m holds a
 * LIST with the pairs whose keys have H( k ) = i, insert() puts a new key
 * at the head of its list, lookup() and remove() go along it. A list may
 * also bring to its head the key just found, so that the keys looked up
 * often stay close to it (exercise 7.5), or be kept sorted, so that an
 * unsuccessful search stops at the first larger key (exercise 7.10) [see
 * Chain].
 *
 * With the load factor \f$ \alpha = n / m \f$, an unsuccessful search
 * accesses \f$ 1 + \alpha \f$ cells on average and a successful one
 * \f$ 1 + \alpha / 2 \f$; probes() gives the cells that the last
 * operation examined, the nil that ends an unsuccessful search included.
 * The table may restructure itself (section 7.4), doubling m when
 * \f$ \alpha > 2 \f$ and halving it, down to the initial size, when
 * \f$ \alpha < 1 / 2 \f$: the two thresholds are 4 times apart, so that
 * between two restructurings there are at least m / 4 operations and the
 * cost stays O( 1 ) amortized [see TableSize.h].
 *
 * The contracts [see Contract.h]: m > 0 and a list that exists for
 * chain(); after insert() the key is in the table and after remove() it is
 * not; the invariant is that each key is in the list of its own place,
 * that the lists are sorted when they must be, that size() counts the
 * keys, and that the key of the operation is in its list at most once. On
 * a table of more than 1024 lists the invariant is checked on the list of
 * that key only, so that a long sequence of operations at the level 3
 * stays linear; the checks do not change probes(). The operations are
 * defined in ChainedHashTable.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 7.2-7.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 * \see W. W. Peterson, Addressing for random-access storage, IBM Journal
 *      of Research and Development 1(2), 130-146, 1957,
 *      doi:10.1147/rd.12.0130
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

/// how a ChainedHashTable keeps its lists
enum class Chain {
 FRONT ,          ///< a new key at the head, nothing moves
 MOVE_TO_FRONT ,  ///< also the key found by lookup() goes to the head
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

 /// the empty table of m lists, which restructures itself or not
 /// \pre m > 0
 explicit ChainedHashTable( std::size_t m , bool restructure = false ,
                            H h = H() );

/*--------------------------------------------------------------------------*/

 /// the value of the key k, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value v to the key k
 /// \post k is in the table
 void insert( const K & k , const V & v );

 /// removes the key k, if it is there
 /// \post k is not in the table
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the number m of lists
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the load factor n / m
 [[nodiscard]] double load( void ) const;

 /// the foreach of the book: f( k , v ) on the pairs, list after list,
 /// O( m + n ) time
 template< class F >
 void forEach( F && f ) const;

 /// the keys of the list i, from its head
 /// \pre i < capacity()
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

 class const_iterator {
  using Lists = std::vector< Bucket >;

  public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = Pair;
  using difference_type = std::ptrdiff_t;
  using reference = const Pair &;
  using pointer = const Pair *;

  const_iterator( void ) = default;

  /// on the first pair from the list b, or at the end
  const_iterator( const Lists * T , std::size_t b ) : f_T( T ) , f_b( b ) {
   if( f_b < f_T->size() ) f_c = ( *f_T )[ f_b ].begin();
   settle();
   }

  reference operator*( void ) const { return( *f_c ); }
  pointer operator->( void ) const { return( &*f_c ); }

  const_iterator & operator++( void ) {
   ++f_c;
   settle();
   return( *this );
   }
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

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

 using value_type = Pair;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = const Pair &;
 using const_reference = const Pair &;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( &f_T , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( &f_T , f_T.size() ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 private:

 [[nodiscard]] std::size_t place( const K & k ) const;

 /// the cell of k, or where the search stopped: the end of the list, or for
 /// a sorted list the first larger key
 Pos find( const Bucket & L , const K & k ) const;

 /// whether k is in the table, without counting the probes
 [[nodiscard]] bool present( const K & k ) const;

 /// each key in the list of its place, the lists sorted if they must be,
 /// size() their total and k at most once, O( n + m ); on more than 1024
 /// lists, only the list of k
 [[nodiscard]] bool consistent( const K & k ) const;

 /// the keys of the list i in it, sorted if they must be, k at most once;
 /// their number goes to n
 [[nodiscard]] bool listOk( std::size_t i , const K & k ,
                            std::size_t & n ) const;

 void rebuild( std::size_t m );

 mutable std::vector< Bucket > f_T;       ///< the lists
 std::size_t f_m0;                         ///< the initial size
 bool f_restructure;                       ///< whether it restructures
 H f_h;                                    ///< the hash of the keys
 std::size_t f_size = 0;                   ///< the number of keys
 mutable std::size_t f_probes = 0;         ///< see probes()
 std::size_t f_rebuilds = 0;               ///< see restructures()
 std::size_t f_moved = 0;                  ///< see moved()

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
