/*--------------------------------------------------------------------------*/
/*-------------------------- File KnuthHashing.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two hash tables of Knuth, beyond the book [see `ChainedHashTable.h` and
 * `OpenHashTable.h` for those of chapter 7], both without removal:
 *
 * - `CoalescedHashTable` (Algorithm 6.4C, with the cellar of Vitter): the
 *   lists of overflow live in the table itself, a key that collides being
 *   put in the empty place of largest index (the first ones taken being
 *   those of a cellar beyond the \f$ m \f$ places addressed by the hash) and
 *   linked at the end of the list of its place, so that the lists may merge
 *   (coalesce); with a cellar of about 14 % of the table the searches are
 *   nearly as fast as with separate lists, with no memory out of the table;
 * - `BrentHashTable`, the double hashing [see `Probing`] with the
 *   improvement of Brent to the insertions: the key \f$ K \f$ is placed at
 *   the \f$ j \f$-th place of its inspection, and the key found there moved
 *   forward by \f$ s - j \f$ places of its own inspection, for the least
 *   \f$ s \f$, and then the least \f$ j \f$, that find an empty place; this
 *   lowers the expected number of probes of a successful search below 2.5
 *   even in a full table, against \f$ - \ln( 1 - \alpha ) / \alpha \f$ of
 *   the plain double hashing, at no cost for the searches.
 *
 * The contracts [see `Contract.h`]: the table is not full on `insert()`;
 * `insert()` ensures that the key is there; the invariant of
 * `CoalescedHashTable` is that each key is on the list of its place, an
 * audit. The operations are defined in `KnuthHashing.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 6.4 (Algorithm C and
 *      exercise 6.4-49 on Brent's variation)
 * \see J. S. Vitter, Implementations for coalesced hashing, Communications
 *      of the ACM 25(12), 911-926, 1982, doi:10.1145/358728.358745
 * \see R. P. Brent, Reducing the retrieval time of scatter storage
 *      techniques, Communications of the ACM 16(2), 105-109, 1973,
 *      doi:10.1145/361952.361964
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

#ifndef __ADS_KnuthHashing
 #define __ADS_KnuthHashing
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS CoalescedHashTable ------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with coalesced lists in the table and a cellar
template< class K , class V , class H = std::hash< K > >
class CoalescedHashTable {

/*--------------------------------------------------------------------------*/
 public:

 /// a table of \f$ m \f$ places addressed by the hash and of `cellar` more
 /// \pre \f$ m \ge 1 \f$
 explicit CoalescedHashTable( std::size_t m , std::size_t cellar = 0 ,
                              H h = H() );

 /// the value of the key \f$ k \f$, or nothing, along the list of the
 /// place \f$ h( k ) \bmod m \f$
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$
 ///
 /// In pseudocode (Algorithm 6.4C of TAOCP, beyond the book), \f$ T \f$ the
 /// places, the cellar last, and \f$ r \f$ a pointer that only goes down,
 /// below which the empty places are:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad i \gets h( k ) \bmod m \cr
 /// 3 & \quad \textbf{if}\ T[ i ] \text{ is used }\ \textbf{then} \cr
 /// 4 & \qquad \textbf{loop} \cr
 /// 5 & \qquad\quad \textbf{if}\ T[ i ].\mathit{key} = k\ \textbf{then}\ T[ i
 ///     ].\mathit{value} \gets v;\ \textbf{return} \cr
 /// 6 & \qquad\quad \textbf{if}\ T[ i ].\mathit{link} = \textbf{nil}
 ///     \ \textbf{then}\ \textbf{break}\ \textbf{else}\ i \gets T[ i
 ///     ].\mathit{link} \cr
 /// 7 & \qquad \textbf{repeat}\ r \gets r - 1\ \textbf{until}\ T[ r ]
 ///     \text{ is empty} \cr
 /// 8 & \qquad T[ i ].\mathit{link} \gets r;\ i \gets r\quad \triangleright
 ///     \ \text{linked at the end of the list} \cr
 /// 9 & \quad T[ i ] \gets ( k , v , \textbf{nil} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \f$ is there, or the table is not full
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the places examined by the last `lookup()` or `insert()`
 [[nodiscard]] std::size_t probes( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places (the cellar last), bidirectional

 /// the places of the table, for `SlotIterator`
 struct Places {
  /// the number of places
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  /// whether the place \f$ i \f$ holds a key
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ].used );
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
 using const_iterator = SlotIterator< CoalescedHashTable , Places >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 struct Slot {
  bool used = false;             ///< whether the place holds a key
  K key{};                       ///< the key
  V value{};                     ///< its value
  long link = -1;                ///< the next place on the list, or -1
  };

 /// each key is on the list of its place, \f$ O( n^2 ) \f$
 [[nodiscard]] bool listed( void ) const;

 std::vector< Slot > f_T;        ///< the m places and the cellar
 std::size_t f_m;                ///< the places addressed by the hash
 long f_free;                    ///< the empty places are below it
 std::size_t f_size = 0;         ///< the number of keys
 mutable std::size_t f_probes = 0;   ///< the places of the last operation
 H f_h;                          ///< the hash function

 };  // end( class CoalescedHashTable )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BrentHashTable --------------------------*/
/*--------------------------------------------------------------------------*/

/// a hash table with double hashing and the insertion of Brent
template< class K , class V , class H = std::hash< K > >
class BrentHashTable {

/*--------------------------------------------------------------------------*/
 public:

 /// a table of \f$ m \f$ places
 /// \pre \f$ m \ge 2 \f$ is prime
 explicit BrentHashTable( std::size_t m , H h = H() );

 /// the value of the key \f$ k \f$, or nothing, along the places
 /// \f$ \mathrm{at}( k , 0 ) , \mathrm{at}( k , 1 ) , \ldots \f$ of its
 /// inspection up to an empty one
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates \f$ v \f$ to \f$ k \f$
 ///
 /// In pseudocode (the variation of Brent, exercise 6.4-49 of TAOCP, beyond
 /// the book), \f$ \mathrm{at}( x , i ) \f$ the place \f$ i \f$ of the
 /// inspection of \f$ x \f$ and \f$ i_y \f$ the index of the key \f$ y \f$ in
 /// its own inspection; the key at the place \f$ j \f$ of the inspection of
 /// \f$ k \f$ moves \f$ s - j \f$ places on in its own:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \textbf{if}\ k \text{ is there }\ \textbf{then}\ \text{give it }
 ///     \text{the value } v;\ \textbf{return} \cr
 /// 3 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ m - 1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{for}\ j \gets 0\ \textbf{to}\ s\ \textbf{do} \cr
 /// 5 & \qquad\quad p \gets \mathrm{at}( k , j ) \cr
 /// 6 & \qquad\quad \textbf{if}\ j = s\ \textbf{then} \cr
 /// 7 & \qquad\qquad \textbf{if}\ T[ p ] \text{ is empty }\ \textbf{then}\ T[
 ///     p ] \gets ( k , v );\ \textbf{return} \cr
 /// 8 & \qquad\quad \textbf{else} \cr
 /// 9 & \qquad\qquad y \gets T[ p ].\mathit{key};\ q \gets \mathrm{at}( y ,
 ///     i_y + s - j ) \cr
 /// 10 & \qquad\qquad \textbf{if}\ T[ q ] \text{ is empty }\ \textbf{then}\ T[
 ///     q ] \gets T[ p ];\ T[ p ] \gets ( k , v );\ \textbf{return}
 /// \end{array}}
 /// \f]
 /// \pre \f$ k \f$ is there, or the table is not full
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the places examined by the last `lookup()`
 [[nodiscard]] std::size_t probes( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ), in the order of the
 // places, bidirectional; an insertion may move the pairs

 /// the places of the table, for `SlotIterator`
 struct Places {
  /// the number of places
  static std::size_t size( const auto & o ) { return( o.f_T.size() ); }
  /// whether the place \f$ i \f$ holds a key
  static bool occupied( const auto & o , std::size_t i ) {
   return( o.f_T[ i ].has_value() );
   }
  /// the pair of the place \f$ i \f$
 static const std::pair< K , V > & read( const auto & o , std::size_t i ) {
   return( *o.f_T[ i ] );
   }
  };

 /// the type of the items, for the standard library
 using value_type = std::pair< K , V >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = const std::pair< K , V > &;
 /// a constant reference to an item
 using const_reference = const std::pair< K , V > &;
 /// the iterator on the items, constant
 using const_iterator = SlotIterator< BrentHashTable , Places >;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , Places::size( *this ) ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the place \f$ i \f$ of the inspection of \f$ k \f$, by double hashing
 [[nodiscard]] std::size_t at( const K & k , std::size_t i ) const;

 /// the places
std::vector< std::optional< std::pair< K , V > > > f_T;
 std::size_t f_size = 0;         ///< the number of keys
 mutable std::size_t f_probes = 0;   ///< the places of the last operation
 H f_h;                          ///< the hash function

 };  // end( class BrentHashTable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/KnuthHashing.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* KnuthHashing.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File KnuthHashing.h -------------------------*/
/*--------------------------------------------------------------------------*/
