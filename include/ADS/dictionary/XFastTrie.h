/*--------------------------------------------------------------------------*/
/*---------------------------- File XFastTrie.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The x-fast tries of Willard, beyond the book: SORTED SETs of the integers
 * of \f$ w \f$ bits, each with a value, in which `lowerBound( x )` gives
 * the smallest key \f$ \ge x \f$ (the `find( x )` of the SSet of Morin's
 * book) in \f$ O( \log w ) \f$ expected time.
 *
 * - `XFastTrie` is the trie of section 13.2 of Morin's book: a
 *   `BinaryTrie` [see `BinaryTrie.h`] with, for each depth
 *   \f$ i = 1 , \ldots , w \f$, a hash table \f$ t[ i ] \f$ from the
 *   prefixes of \f$ i \f$ bits to their nodes. Since the prefixes of
 *   \f$ x \f$ that are in the trie are those of depth up to some
 *   \f$ \ell \f$, \f$ \ell \f$ is found by a binary search on the depth,
 *   \f$ O( \log w ) \f$ lookups, and the deepest node of the path of
 *   \f$ x \f$ gives the successor by its jump pointer as in the binary
 *   trie; `insert()` and `remove()` still change \f$ O( w ) \f$ nodes and
 *   entries of the tables. The space is \f$ O( n w ) \f$.
 * - `ListXFastTrie` is the x-fast trie of exercise 13.2 of Morin's book,
 *   with no trie: the keys are a doubly linked list in increasing order,
 *   and the table \f$ t[ i ] \f$ maps each prefix of \f$ i \f$ bits to the
 *   smallest and the largest key with that prefix. The binary search finds
 *   \f$ \ell \f$ as above; if the bit \f$ \ell \f$ of \f$ x \f$ is 0, every
 *   key with the prefix of depth \f$ \ell \f$ has 1 there and is larger
 *   than \f$ x \f$, so that the successor is the smallest of them, and if
 *   it is 1, every such key is smaller, so that the successor follows the
 *   largest of them in the list. A removal replaces the smallest key of a
 *   prefix by the next one in the list, the largest by the previous one,
 *   the keys of a prefix being consecutive in the list.
 *
 * Both have also `findNear( x )`, the search of exercise 13.4 of Morin's
 * book, whose time depends on the distance \f$ d( x ) \f$ between \f$ x \f$
 * and its successor (\f$ 2^w \f$ if there is none): with
 * \f$ k = \lceil \log_2( d( x ) + 1 ) \rceil \f$, the successor lies in
 * \f$ [ x , x + 2^k ) \f$, which meets at most two blocks of \f$ 2^k \f$
 * keys aligned to \f$ 2^k \f$, those of the prefixes \f$ p \f$ and
 * \f$ p + 1 \f$ of depth \f$ w - k \f$, with
 * \f$ p = \lfloor x / 2^k \rfloor \f$. The search tries
 * \f$ K = 1 , 2 , 4 , \ldots \f$ until one of the two prefixes of depth
 * \f$ w - K \f$ is there, \f$ O( \log k ) \f$ steps, and then either
 * searches \f$ \ell \f$ between \f$ w - K \f$ and \f$ w - K / 2 \f$, in
 * \f$ O( \log K ) \f$ lookups, or takes the smallest key of the prefix
 * \f$ p + 1 \f$: in \f$ O( 1 ) \f$ in `ListXFastTrie`, which keeps it,
 * and by a walk of \f$ O( K ) \f$ nodes in `XFastTrie`. The expected time
 * is \f$ O( 1 + \log \log d( x ) ) \f$ for `ListXFastTrie`, the second
 * point of the exercise, and \f$ O( 1 + \log d( x ) ) \f$ for
 * `XFastTrie`, the first one.
 *
 * Where it is meant to be faster than the code of Morin's book: the
 * tables are `FlatHashTable`s, which compare a fingerprint of the hash
 * with 15 places at once and touch about one line of cache per lookup,
 * where the book uses its `LinearHashTable` of pairs, whose probes compare
 * whole keys; a binary search makes
 * \f$ \lceil \log_2( w + 1 ) \rceil \f$ lookups that depend on each
 * other, so that the latency of each one is what counts. The nodes come
 * from a pool [see `BinaryTrie.h`].
 *
 * The tables are `FlatHashTable`s [see `FlatHashTable.h`], whose expected
 * time per operation is \f$ O( 1 ) \f$. The contracts [see `Contract.h`]:
 * the keys are below \f$ 2^w \f$, `insert()` ensures that the key is there
 * and `remove()` that it is not; the invariant (the order of the list and
 * the tables of the prefixes) is checked in \f$ O( n w ) \f$ time at the
 * level 3 while \f$ n \le 256 \f$, and around the key of the operation
 * beyond. The operations are defined in `XFastTrie.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, section 13.2 and
 *      exercises 13.2 and 13.4
 * \see D. E. Willard, Log-logarithmic worst-case range queries are possible
 *      in space Theta(N), Information Processing Letters 17(2), 81-84,
 *      1983, doi:10.1016/0020-0190(83)90075-3
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

#ifndef __ADS_XFastTrie
 #define __ADS_XFastTrie
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/BinaryTrie.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/memory/ObjectPool.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>
#include <variant>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS XFastTrie ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the x-fast trie: a binary trie with a hash table of the prefixes per
/// depth
template< std::unsigned_integral U , class V = std::monostate ,
          int W = std::numeric_limits< U >::digits >
 requires TrieWidth< U , W >
class XFastTrie : public detail::JumpTrie< U , V , W , true > {

 using Base = detail::JumpTrie< U , V , W , true >;

/*--------------------------------------------------------------------------*/
 public:

 using Base::Base;

 /// the smallest key \f$ \ge x \f$, nil if none, in
 /// \f$ O( 1 + \log d( x ) ) \f$ expected time
 ///
 /// In pseudocode (exercise 13.4 of Morin's book), \f$ t[ i ]( p ) \f$ the
 /// node of the prefix \f$ p \f$ of depth \f$ i \f$, or nil:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{findNear}( x ) \cr
 /// 2 & \quad \textbf{if}\ x \text{ is there}\ \textbf{then return}\ x \cr
 /// 3 & \quad h \gets w;\ K \gets 1 \cr
 /// 4 & \quad \textbf{loop} \cr
 /// 5 & \qquad d \gets w - K;\ p \gets \lfloor x / 2^K \rfloor \cr
 /// 6 & \qquad \textbf{if}\ t[ d ]( p ) \ne \textbf{nil}\ \textbf{then}
 ///     \cr
 /// 7 & \qquad\quad u \gets \text{the deepest node of the path of } x
 ///     \text{, by a binary search between } d \text{ and } h \cr
 /// 8 & \qquad\quad \textbf{return}\ \text{the successor of } x \text{ from }
 ///     u \text{, as in } \mathrm{lowerBound} \cr
 /// 9 & \qquad \textbf{if}\ t[ d ]( p + 1 ) \ne \textbf{nil}\ \textbf{then}
 ///     \ \textbf{return}\ \text{its smallest leaf} \cr
 /// 10 & \qquad h \gets d;\ K \gets \min\{ 2 K , w \}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * findNear( U x ) const;

 };  // end( class XFastTrie )

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ListXFastTrie ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the x-fast trie of a doubly linked list and \f$ w + 1 \f$ hash tables
/// (exercise 13.2 of Morin's book)
template< std::unsigned_integral U , class V = std::monostate ,
          int W = std::numeric_limits< U >::digits >
 requires TrieWidth< U , W >
class ListXFastTrie {

/*--------------------------------------------------------------------------*/
 public:

 using Key = U;     ///< the type of the keys
 using Item = U;    ///< the items of the SSet, the keys
 using Value = V;   ///< the type of the values

 /// the empty trie
 ListXFastTrie( void );

 /// the trie of `o`, which is left empty
 ListXFastTrie( ListXFastTrie && o );

 /// the trie of `o`, which is left empty; the keys of this one are freed
 ListXFastTrie & operator=( ListXFastTrie && o ) noexcept;

 ListXFastTrie( const ListXFastTrie & ) = delete;
 ListXFastTrie & operator=( const ListXFastTrie & ) = delete;

 /// frees the keys
 ~ListXFastTrie();

/*--------------------------------------------------------------------------*/

 /// the smallest key \f$ \ge x \f$, nil if none (the `find( x )` of the
 /// SSet of Morin's book), in \f$ O( \log w ) \f$ expected time
 ///
 /// In pseudocode (exercise 13.2 of Morin's book), \f$ t[ i ]( p ) \f$ the
 /// entry of the prefix \f$ p \f$ of depth \f$ i \f$, with the smallest
 /// and the largest key that have it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( x ) \cr
 /// 2 & \quad l \gets 0;\ h \gets w + 1 \cr
 /// 3 & \quad \textbf{while}\ h - l > 1\ \textbf{do} \cr
 /// 4 & \qquad i \gets \lfloor ( l + h ) / 2 \rfloor \cr
 /// 5 & \qquad \textbf{if}\ t[ i ]( \lfloor x / 2^{w-i} \rfloor ) \ne
 ///     \textbf{nil}\ \textbf{then}\ l \gets i\ \textbf{else}\ h \gets i
 ///     \cr
 /// 6 & \quad e \gets t[ l ]( \lfloor x / 2^{w-l} \rfloor ) \cr
 /// 7 & \quad \textbf{if}\ l = w\ \textbf{then return}\ x \cr
 /// 8 & \quad \textbf{if}\ \text{the bit } l \text{ of } x \text{ is } 0
 ///     \ \textbf{then return}\ e.\mathit{min} \cr
 /// 9 & \quad \textbf{return}\ e.\mathit{max}.\mathit{next}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * lowerBound( U x ) const;

 /// the key \f$ x \f$, nil if it is not there, in \f$ O( 1 ) \f$ expected
 /// time
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * find( U x ) const;

 /// the smallest key \f$ \ge x \f$, nil if none, in
 /// \f$ O( 1 + \log \log d( x ) ) \f$ expected time
 ///
 /// It is the pseudocode of `XFastTrie::findNear()`, the smallest key of
 /// the prefix \f$ p + 1 \f$ read from its entry (exercise 13.4 of Morin's
 /// book).
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * findNear( U x ) const;

 /// the value of \f$ x \f$, or nothing, in \f$ O( 1 ) \f$ expected time
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] std::optional< V > lookup( U x ) const;

 /// true if \f$ x \f$ is there, in \f$ O( 1 ) \f$ expected time
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] bool contains( U x ) const;

 /// associates \f$ v \f$ to \f$ x \f$; true if \f$ x \f$ was not there, in
 /// \f$ O( w ) \f$ expected time
 ///
 /// In pseudocode (exercise 13.2 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , v ) \cr
 /// 2 & \quad \textbf{if}\ t[ w ]( x ) \ne \textbf{nil}\ \textbf{then}
 ///     \ \text{give it the value } v;\ \textbf{return false} \cr
 /// 3 & \quad \text{link a new key } x \text{ before the one of }
 ///     \mathrm{lowerBound}( x ) \cr
 /// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ w\ \textbf{do} \cr
 /// 5 & \qquad e \gets t[ i ]( \lfloor x / 2^{w-i} \rfloor ) \cr
 /// 6 & \qquad \textbf{if}\ e = \textbf{nil}\ \textbf{then}\ e \gets ( x , x )
 ///     \ \textbf{else}\ e \gets ( \min\{ e.\mathit{min} , x \} ,
 ///     \max\{ e.\mathit{max} , x \} ) \cr
 /// 7 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is there
 bool insert( U x , const V & v = V() );

 /// removes \f$ x \f$; true if it was there, in \f$ O( w ) \f$ expected
 /// time
 ///
 /// In pseudocode (exercise 13.2 of Morin's book), \f$ \ell \f$ the key
 /// \f$ x \f$ in the list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad \textbf{if}\ t[ w ]( x ) = \textbf{nil}\ \textbf{then return}
 ///     \ \textbf{false} \cr
 /// 3 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ w\ \textbf{do} \cr
 /// 4 & \qquad e \gets t[ i ]( \lfloor x / 2^{w-i} \rfloor ) \cr
 /// 5 & \qquad \textbf{if}\ e = ( \ell , \ell )\ \textbf{then}\ \text{remove }
 ///     e \cr
 /// 6 & \qquad \textbf{else if}\ e.\mathit{min} = \ell\ \textbf{then}
 ///     \ e.\mathit{min} \gets \ell.\mathit{next} \cr
 /// 7 & \qquad \textbf{else if}\ e.\mathit{max} = \ell\ \textbf{then}
 ///     \ e.\mathit{max} \gets \ell.\mathit{prev} \cr
 /// 8 & \quad \text{unlink } \ell;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is not there
 bool remove( U x );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of entries of the tables
 [[nodiscard]] std::size_t entries( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// a key in the list
 struct Leaf {
  Leaf * prev = nullptr;   ///< the previous key, or the dummy
  Leaf * next = nullptr;   ///< the next key, or the dummy
  U key = 0;               ///< the key
  V value{};               ///< the value
  };

 /// the smallest and the largest key of a prefix
 struct Entry {
  Leaf * min;   ///< the smallest key with the prefix
  Leaf * max;   ///< the largest key with the prefix
  };

 public:

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key, along
 // the list

 /// the bidirectional iterator along the list
 class const_iterator {
  public:
  /// the category of the iterator
  using iterator_category = std::bidirectional_iterator_tag;
  /// the pair of references to a key and its value
  using value_type = std::pair< const U & , const V & >;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// a pair, by value
  using reference = value_type;
  /// no pointer: the pairs are made on the fly
  using pointer = void;

  /// the end of no trie
  const_iterator( void ) = default;

  /// on the key \f$ l \f$
  explicit const_iterator( const Leaf * l ) : f_l( l ) {}

  /// the pair of the key
  reference operator*( void ) const {
   return( reference( f_l->key , f_l->value ) );
   }

  /// to the next key
  const_iterator & operator++( void ) { f_l = f_l->next; return( *this ); }

  /// to the next key, returning the iterator before the step
  const_iterator operator++( int ) { auto t = *this; ++*this; return( t ); }

  /// to the previous key
  const_iterator & operator--( void ) { f_l = f_l->prev; return( *this ); }

  /// to the previous key, returning the iterator before the step
  const_iterator operator--( int ) { auto t = *this; --*this; return( t ); }

  /// equal on the same key
  bool operator==( const const_iterator & o ) const = default;

  private:
  const Leaf * f_l = nullptr;
  };

 /// the type of the items, for the standard library
 using value_type = typename const_iterator::value_type;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a pair, by value
 using reference = value_type;
 /// a pair, by value
 using const_reference = value_type;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the smallest key
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_dummy->next ) );
  }
 /// the iterator past the largest key
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_dummy ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the iterator on the smallest key \f$ \ge x \f$, `end()` if none
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const_iterator seek( U x ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the entry of the prefix \f$ p \f$ of depth \f$ i \f$ in `e`; false if
 /// there is none
 [[nodiscard]] bool entry( U p , int i , Entry & e ) const;

 /// the key that follows \f$ x \f$ given the entry \f$ e \f$ of its longest
 /// prefix in the tables, of depth \f$ \ell \f$
 [[nodiscard]] Leaf * follow( U x , const Entry & e , int l ) const;

 /// the smallest key \f$ \ge x \f$, or the dummy
 [[nodiscard]] Leaf * successor( U x ) const;

 /// the deepest prefix of \f$ x \f$ there between \f$ \mathit{lo} \f$,
 /// whose entry is `e`, and \f$ \mathit{hi} \f$, which is not there
 [[nodiscard]] int deepest( U x , int lo , int hi , Entry & e ) const;

 /// the list in order and the tables; \f$ O( n w ) \f$ for
 /// \f$ n \le 256 \f$, around \f$ x \f$ beyond
 [[nodiscard]] bool consistent( U x ) const;

 ObjectPool< Leaf > f_pool{ 1 << 16 };   ///< the keys
 Leaf * f_dummy = nullptr;                ///< the head of the list
 std::size_t f_n = 0;                     ///< the number of keys
 std::array< FlatHashTable< U , Entry > , W + 1 > f_t;  ///< \f$ t[ 0 ] \f$
                                                        ///< unused

 };  // end( class ListXFastTrie )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/XFastTrie.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* XFastTrie.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File XFastTrie.h ---------------------------*/
/*--------------------------------------------------------------------------*/
