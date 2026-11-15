/*--------------------------------------------------------------------------*/
/*---------------------------- File BitmapTrie.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The trie of exercise 13.3 of Morin's book, beyond the book: a SORTED SET
 * of strings of any length, each with a value, whose `lowerBound( s )` (the
 * smallest string \f$ \ge s \f$ in the lexicographic order of the bytes, a
 * prefix before its extensions), `insert( s )` and `remove( s )` take
 * \f$ O( |s| ) \f$ time, whatever the number of strings.
 *
 * It extends the `BinaryTrie` [see `BinaryTrie.h`] from strings of
 * \f$ w \f$ bits to strings of bytes: the node at depth \f$ i \f$ stands
 * for a prefix of \f$ i \f$ bytes, a string ends at the node of its last
 * byte, the strings are also a doubly linked list in increasing order,
 * and each node keeps the smallest and the largest string below it, which
 * play the part of the jump pointers. The exercise suggests a hash table
 * of the children in each node; here a node has instead a bitmap of 256
 * bits of the bytes of its children, and the children themselves in an
 * array sorted by their byte, so that the child of the byte \f$ c \f$ is
 * at the rank of \f$ c \f$ among the bits set (a population count, as in
 * the hash array mapped tries of Bagwell), with no hashing, and the next
 * child after \f$ c \f$ is the next bit set, which a hash table could not
 * give. A search that stops at the node \f$ u \f$ of depth \f$ i \f$,
 * whose child of \f$ s_i \f$ is missing, thus returns the smallest string
 * below the next child of \f$ u \f$, if there is one, and otherwise the
 * string after the largest one below \f$ u \f$, \f$ O( 1 ) \f$ time after
 * the walk of \f$ O( |s| ) \f$ nodes.
 *
 * An insertion adds the missing nodes of the path of \f$ s \f$, links the
 * new string before its successor, and, going back to the root, makes it
 * the smallest string of each node in which nothing precedes it (no string
 * ends there and no child has a smaller byte) and the largest of each node
 * in which nothing follows it; a removal unlinks it, removes the nodes of
 * its path left empty, and replaces it, where it was the smallest or the
 * largest string of a node, by the next or the previous one in the list,
 * since the strings below a node are consecutive in the list.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that the string is
 * there and `remove()` that it is not; the invariant (the list in order,
 * the smallest and the largest string of each node, the bitmaps) is
 * checked in \f$ O( N ) \f$ time, \f$ N \f$ the number of nodes, at the
 * level 3 while there are at most 256 strings. The operations are defined
 * in `BitmapTrie.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 13.1 and exercise
 *      13.3
 * \see E. Fredkin, Trie memory, Communications of the ACM 3(9), 490-499,
 *      1960, doi:10.1145/367390.367400
 * \see P. Bagwell, Ideal hash trees, Technical Report, EPFL, 2001
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

#ifndef __ADS_BitmapTrie
 #define __ADS_BitmapTrie
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BitmapTrie ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a trie of strings with the bitmaps of the children, the list of the
/// strings and the smallest and largest string below each node
template< class V = std::monostate >
class BitmapTrie
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = std::string;   ///< the type of the keys
 using Item = std::string;  ///< the items of the SSet, the strings
 using Value = V;           ///< the type of the values

 /// the empty trie
 BitmapTrie( void );

 /// the trie of `o`, which is left empty
 BitmapTrie( BitmapTrie && o );

 /// the trie of `o`, which gets the strings of this one
 BitmapTrie & operator=( BitmapTrie && o ) noexcept;

 BitmapTrie( const BitmapTrie & ) = delete;
 BitmapTrie & operator=( const BitmapTrie & ) = delete;

 /// frees the nodes and the strings
 ~BitmapTrie();

/*--------------------------------------------------------------------------*/

 /// the smallest string \f$ \ge s \f$, nil if none (the `find( s )` of the
 /// SSet of Morin's book), in \f$ O( |s| ) \f$ time
 ///
 /// In pseudocode (exercise 13.3 of Morin's book), \f$ u.\mathit{min} \f$
 /// and \f$ u.\mathit{max} \f$ the smallest and the largest string below
 /// \f$ u \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( s ) \cr
 /// 2 & \quad u \gets \mathit{root} \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ |s| - 1\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ u \text{ has a child of } s_i\ \textbf{then}
 ///     \ u \gets \text{that child};\ \textbf{continue} \cr
 /// 5 & \qquad \textbf{if}\ u \text{ has a child of a byte} > s_i
 ///     \ \textbf{then return}\ \text{the } \mathit{min} \text{ of the }
 ///     \text{first one} \cr
 /// 6 & \qquad \textbf{return}\ u.\mathit{max}.\mathit{next} \cr
 /// 7 & \quad \textbf{return}\ u.\mathit{min}
 /// \end{array}}
 /// \f]
 [[nodiscard]] const std::string * lowerBound( std::string_view s ) const;

 /// the string \f$ s \f$, nil if it is not there, in \f$ O( |s| ) \f$ time
 [[nodiscard]] const std::string * find( std::string_view s ) const;

 /// the value of \f$ s \f$, or nothing, in \f$ O( |s| ) \f$ time
 [[nodiscard]] std::optional< V > lookup( std::string_view s ) const;

 /// true if \f$ s \f$ is there, in \f$ O( |s| ) \f$ time
 [[nodiscard]] bool contains( std::string_view s ) const;

 /// associates \f$ v \f$ to \f$ s \f$; true if \f$ s \f$ was not there, in
 /// \f$ O( |s| ) \f$ time
 ///
 /// In pseudocode (exercise 13.3 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( s , v ) \cr
 /// 2 & \quad \textbf{if}\ s \text{ is there}\ \textbf{then}\ \text{give it }
 ///     \text{the value } v;\ \textbf{return false} \cr
 /// 3 & \quad \text{link } s \text{ before the string of }
 ///     \mathrm{lowerBound}( s ) \text{, add the missing nodes of its path}
 ///     \cr
 /// 4 & \quad \textbf{for}\ i \gets |s|\ \textbf{downto}\ 0\ \textbf{do}
 ///     \quad \triangleright\ u \text{ the node of depth } i \cr
 /// 5 & \qquad \textbf{if}\ \text{nothing of } u \text{ precedes the path }
 ///     \text{of } s \text{ and } s \text{ is the } \mathit{min} \text{ of }
 ///     \text{the next node}\ \textbf{then}\ u.\mathit{min} \gets s \cr
 /// 6 & \qquad \textbf{if}\ \text{nothing of } u \text{ follows the path }
 ///     \text{of } s \text{ and } s \text{ is the } \mathit{max} \text{ of }
 ///     \text{the next node}\ \textbf{then}\ u.\mathit{max} \gets s \cr
 /// 7 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post \f$ s \f$ is there
 bool insert( std::string_view s , const V & v = V() );

 /// removes \f$ s \f$; true if it was there, in \f$ O( |s| ) \f$ time
 ///
 /// In pseudocode (exercise 13.3 of Morin's book), \f$ \ell \f$ the item of
 /// \f$ s \f$ in the list:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( s ) \cr
 /// 2 & \quad \textbf{if}\ s \text{ is not there}\ \textbf{then return false}
 ///     \cr
 /// 3 & \quad \text{unlink } \ell;\ \text{remove the nodes of the path of }
 ///     s \text{ left with no string below} \cr
 /// 4 & \quad \textbf{foreach}\ \text{node } u \text{ left on the path}
 ///     \ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ u.\mathit{min} = \ell\ \textbf{then}
 ///     \ u.\mathit{min} \gets \ell.\mathit{next} \cr
 /// 6 & \qquad \textbf{if}\ u.\mathit{max} = \ell\ \textbf{then}
 ///     \ u.\mathit{max} \gets \ell.\mathit{prev} \cr
 /// 7 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \post \f$ s \f$ is not there
 bool remove( std::string_view s );

 /// the number of strings
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no strings
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of nodes, the root included
 [[nodiscard]] std::size_t nodes( void ) const { return( f_nodes ); }

/*--------------------------------------------------------------------------*/
 private:
 struct Node;

 /// a string in the list
 struct Cell
 {
  std::string key;        ///< the string
  V value{};              ///< its value
  Cell * prev = nullptr;  ///< the previous string, or the dummy
  Cell * next = nullptr;  ///< the next string, or the dummy
  };

 /// a node: the bitmap and the array of its children, the string that
 /// ends there and the smallest and the largest string below it
 struct Node
 {
  std::array< std::uint64_t , 4 > bits{};  ///< the bytes of the children
  std::vector< Node * > kids;             ///< the children, by byte
  Cell * term = nullptr;                  ///< the string ending here
  Cell * min = nullptr;                   ///< the smallest string below
  Cell * max = nullptr;                   ///< the largest string below
  };

 public:
/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( string , value ) in increasing order,
 // along the list

 /// the bidirectional iterator along the list
 class const_iterator
 {
  public:
  /// the category of the iterator
  using iterator_category = std::bidirectional_iterator_tag;
  /// the pairs ( key , value ) by value, which own their copies
  using value_type = std::pair< std::string , V >;
  /// the type of the keys
  using key_type = std::string;
  /// the type of the values
  using mapped_type = V;
  /// the type of the distances between two iterators
  using difference_type = std::ptrdiff_t;
  /// a pair of references, by value
  using reference = PairRef< const std::string & , const V & >;
  /// no pointer: the pairs are made on the fly
  using pointer = void;

  /// the end of no trie
  const_iterator( void ) = default;

  /// on the item \f$ l \f$
  explicit const_iterator( const Cell * l ) : f_l( l ) { }

  /// the pair of the item
  reference operator*( void ) const {
   return( reference( f_l->key , f_l->value ) );
   }

  /// to the next string
  const_iterator & operator++( void ) {
   f_l = f_l->next;
   return( *this );
   }

  /// to the next string, returning the iterator before the step
  const_iterator operator++( int ) {
   auto t = *this;
   ++*this;
   return( t );
   }

  /// to the previous string
  const_iterator & operator--( void ) {
   f_l = f_l->prev;
   return( *this );
   }

  /// to the previous string, returning the iterator before the step
  const_iterator operator--( int ) {
   auto t = *this;
   --*this;
   return( t );
   }

  /// equal on the same item
  bool operator==( const const_iterator & o ) const = default;

  private:
  const Cell * f_l = nullptr;
  };

 /// the type of the items, for the standard library
 using value_type = typename const_iterator::value_type;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a pair of references, by value
 using reference = typename const_iterator::reference;
 /// a pair of references, by value
 using const_reference = reference;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the smallest string
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( f_dummy->next ) );
  }
 /// the iterator past the largest string
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( f_dummy ) );
  }
 /// as `begin()`
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// as `end()`
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

 /// the iterator on the smallest string \f$ \ge s \f$, `end()` if none
 [[nodiscard]] const_iterator seek( std::string_view s ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// whether \f$ u \f$ has a child of the byte \f$ c \f$
 [[nodiscard]] static bool has( const Node * u , unsigned c );

 /// the rank of \f$ c \f$ among the bytes of the children of \f$ u \f$
 [[nodiscard]] static std::size_t rank( const Node * u , unsigned c );

 /// the child of \f$ u \f$ of the byte \f$ c \f$, or nil
 [[nodiscard]] static Node * child( const Node * u , unsigned c );

 /// the first child of \f$ u \f$ of a byte \f$ > c \f$, or nil
 [[nodiscard]] static Node * after( const Node * u , unsigned c );

 /// whether \f$ u \f$ has a child of a byte \f$ < c \f$
 [[nodiscard]] static bool before( const Node * u , unsigned c );

 /// the node of \f$ s \f$, or nil
 [[nodiscard]] Node * node( std::string_view s ) const;

 /// the item of the smallest string \f$ \ge s \f$, or the dummy
 [[nodiscard]] Cell * successor( std::string_view s ) const;

 /// frees the subtree of \f$ u \f$
 static void destroy( Node * u );

 /// the list in order and the smallest and largest strings below each node
 [[nodiscard]] bool consistent( void ) const;

 /// whether the subtree of \f$ u \f$ is well formed: its smallest and
 /// largest string in `lo` and `hi`, its strings in `n`
 [[nodiscard]] bool subtree( const Node * u , const Cell *& lo ,
			     const Cell *& hi , std::size_t & n ) const;

 Node * f_root;            ///< the root
 Cell * f_dummy;           ///< the head of the list
 std::size_t f_n = 0;      ///< the number of strings
 std::size_t f_nodes = 1;  ///< the number of nodes

 };  // end( class BitmapTrie )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/BitmapTrie.tpp"

/*--------------------------------------------------------------------------*/

#endif /* BitmapTrie.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BitmapTrie.h --------------------------*/
/*--------------------------------------------------------------------------*/
