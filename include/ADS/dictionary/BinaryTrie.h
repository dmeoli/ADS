/*--------------------------------------------------------------------------*/
/*---------------------------- File BinaryTrie.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The binary tries of the integers of \f$ w \f$ bits, beyond the book: a
 * SORTED SET of keys \f$ x \in \{ 0 , \ldots , 2^w - 1 \} \f$, each with a
 * value, in which `lowerBound( x )` gives the smallest key \f$ \ge x \f$
 * (the successor, the `find( x )` of the SSet of Morin's book), in time
 * that depends on \f$ w \f$ and not on the number \f$ n \f$ of keys.
 *
 * - `BinaryTrie` is the digital search tree of section 13.1 of Morin's
 *   book: the node at depth \f$ i \f$ stands for a prefix of \f$ i \f$ bits,
 *   its child 0 and 1 for the prefixes one bit longer, and the leaves, all
 *   at depth \f$ w \f$, for the keys; the leaves are also a doubly linked
 *   list in increasing order, and each internal node with one child keeps
 *   a jump pointer to the smallest leaf below it, if its child 0 is
 *   missing, or to the largest, if its child 1 is. A search that falls off
 *   the trie at depth \f$ i \f$ thus reaches the successor through the jump
 *   pointer and at most one step of the list; `lowerBound()`, `insert()` and
 *   `remove()` take \f$ O( w ) \f$ time, and the trie has at most
 *   \f$ n w + 1 \f$ nodes. The leaves are the nodes themselves, their two
 *   children used as the links of the list, and the nodes come from an
 *   `ObjectPool` [see `ObjectPool.h`].
 * - `SimpleBinaryTrie` is the trie of exercise 13.1 of Morin's book, with
 *   neither the list nor the jump pointers: a search that falls off the
 *   trie takes the leftmost path from the deepest node of its path where a
 *   larger branch departs, \f$ O( w ) \f$ time as well; its nodes are two
 *   pointers each, a leaf also a key and a value.
 *
 * Against the code of Morin's book: the nodes of `BinaryTrie` come from a
 * pool instead of one call to the allocator each, and those made one
 * after another lie next to each other, so that the \f$ w \f$ nodes of a
 * new path seldom cost an allocation and the walk from the root misses
 * the cache less (where it is meant to be faster); the keys have any
 * width up to 64 bits, where the book fixes \f$ w = 32 \f$; and
 * `remove()` reads the links of the leaf before it frees it, while the
 * code of the book reads them from the freed leaf. `SimpleBinaryTrie`,
 * with two pointers per internal node instead of five, takes less memory,
 * at the price of the walk down to the leftmost leaf.
 *
 * The x-fast trie of section 13.2 shares the code of `BinaryTrie` [see
 * `XFastTrie.h`]. The contracts [see `Contract.h`]: the keys are below
 * \f$ 2^w \f$, `insert()` ensures that the key is there and `remove()`
 * that it is not; the invariant (the depth of the leaves, the order of the
 * list, the jump pointers and, for the x-fast trie, the tables of the
 * prefixes) is checked in \f$ O( n w ) \f$ time at the level 3 while
 * \f$ n \le 256 \f$, and at the leaf of the operation beyond. The
 * operations are defined in `BinaryTrie.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 13.1 and exercise
 *      13.1
 * \see E. Fredkin, Trie memory, Communications of the ACM 3(9), 490-499,
 *      1960, doi:10.1145/367390.367400
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

#ifndef __ADS_BinaryTrie
 #define __ADS_BinaryTrie
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Portable.h"
#include "ADS/dictionary/FlatHashTable.h"
#include "ADS/memory/ObjectPool.h"

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the number of bits of the keys of a trie on `U`: \f$ w \f$, if it is
/// between 1 and the bits of `U`
template< class U , int W >
concept TrieWidth = std::unsigned_integral< U > && ( W >= 1 ) &&
		    ( W <= std::numeric_limits< U >::digits );

namespace detail
{

/// the prefix of the \f$ i \f$ most significant of the \f$ w \f$ bits of
/// \f$ x \f$, \f$ \lfloor x / 2^{w-i} \rfloor \f$
template< class U , int W >
[[nodiscard]] constexpr U triePrefix( U x , int i )
{
 return( i == 0 ? U( 0 ) : U( x >> ( W - i ) ) );
 }

/// the bit \f$ i \f$ of \f$ x \f$, counted from the most significant of its
/// \f$ w \f$ bits
template< class U , int W >
[[nodiscard]] constexpr int trieBit( U x , int i )
{
 return( int( ( x >> ( W - 1 - i ) ) & 1u ) );
 }

/// true if \f$ x < 2^w \f$
template< class U , int W >
[[nodiscard]] constexpr bool trieKey( U x )
{
 if constexpr( W == std::numeric_limits< U >::digits )
  return( true );
 else
  return( ( x >> W ) == 0 );
 }

/// the tables of the prefixes of an x-fast trie, one per depth: nothing
/// for a binary trie
template< class U , class N , int W , bool Hashed >
struct TrieTables
{ };

/// the tables of the prefixes of an x-fast trie, \f$ t[ i ] \f$ mapping the
/// prefix of \f$ i \f$ bits to its node, for \f$ i = 1 , \ldots , w \f$
template< class U , class N , int W >
struct TrieTables< U , N , W , true >
{
 std::array< FlatHashTable< U , N * > , W + 1 > t;  ///< \f$ t[ 0 ] \f$ unused
 };

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS JumpTrie --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the binary trie with the list of the leaves and the jump pointers of
/// `BinaryTrie` and, if `Hashed`, the tables of the prefixes of `XFastTrie`,
/// which find the deepest node of the path of a key by a binary search on
/// the depth
template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
class JumpTrie
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = U;    ///< the type of the keys
 using Item = U;   ///< the items of the SSet, the keys
 using Value = V;  ///< the type of the values

 /// the empty trie
 JumpTrie( void );

 /// the trie of `o`, which is left empty
 JumpTrie( JumpTrie && o );

 /// the trie of `o`, which is left empty; the nodes of this one are freed
 JumpTrie & operator=( JumpTrie && o ) noexcept;

 JumpTrie( const JumpTrie & ) = delete;
 JumpTrie & operator=( const JumpTrie & ) = delete;

 /// frees the nodes
 ~JumpTrie();

/*--------------------------------------------------------------------------*/

 /// the smallest key \f$ \ge x \f$, nil if none (the `find( x )` of the
 /// SSet of Morin's book), in \f$ O( w ) \f$ time, or
 /// \f$ O( \log w ) \f$ expected for the x-fast trie
 ///
 /// In pseudocode (section 13.1 of Morin's book), \f$ u \f$ the deepest
 /// node of the path of \f$ x \f$, at depth \f$ i \f$, and
 /// \f$ c \f$ the bit \f$ i \f$ of \f$ x \f$, whose child is missing:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( x ) \cr
 /// 2 & \quad ( u , i ) \gets \text{the deepest node of the path of } x \cr
 /// 3 & \quad \textbf{if}\ i = w\ \textbf{then return}\ x \cr
 /// 4 & \quad \textbf{if}\ c = 0\ \textbf{then}\ l \gets u.\mathit{jump}
 ///     \quad \triangleright\ \text{the smallest leaf below } u \cr
 /// 5 & \quad \textbf{else}\ l \gets u.\mathit{jump}.\mathit{next}
 ///     \quad \triangleright\ \text{after the largest leaf below } u \cr
 /// 6 & \quad \textbf{return}\ l \text{, or } \textbf{nil} \text{ at the end }
 ///     \text{of the list}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * lowerBound( U x ) const;

 /// the key \f$ x \f$, nil if it is not there (the `find( x )` of the USet
 /// of Morin's book), in \f$ O( w ) \f$ time, or \f$ O( 1 ) \f$ expected
 /// for the x-fast trie
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * find( U x ) const;

 /// the value of \f$ x \f$, or nothing
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] std::optional< V > lookup( U x ) const;

 /// true if \f$ x \f$ is there
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] bool contains( U x ) const;

 /// associates \f$ v \f$ to \f$ x \f$; true if \f$ x \f$ was not there, in
 /// \f$ O( w ) \f$ time (expected for the x-fast trie)
 ///
 /// In pseudocode (section 13.1 of Morin's book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( x , v ) \cr
 /// 2 & \quad ( u , i ) \gets \text{the deepest node of the path of } x \cr
 /// 3 & \quad \textbf{if}\ i = w\ \textbf{then}\ u.\mathit{value} \gets v;
 ///     \ \textbf{return false} \cr
 /// 4 & \quad p \gets \text{the predecessor of } x \text{, by }
 ///     u.\mathit{jump} \text{ as in } \mathrm{lowerBound} \cr
 /// 5 & \quad \text{add the nodes of the path of } x \text{ from depth }
 ///     i + 1 \text{ to } w \cr
 /// 6 & \quad \text{link the leaf } \ell \text{ of } x \text{ after } p \cr
 /// 7 & \quad \textbf{foreach}\ \text{ancestor } a \text{ of } \ell
 ///     \ \textbf{do} \cr
 /// 8 & \qquad \textbf{if}\ a \text{ has no child 0 and } x < a.\mathit{jump}
 ///     \text{, or no child 1 and } x > a.\mathit{jump}\ \textbf{then}
 ///     \ a.\mathit{jump} \gets \ell \cr
 /// 9 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is there
 bool insert( U x , const V & v = V() );

 /// removes \f$ x \f$; true if it was there, in \f$ O( w ) \f$ time
 /// (expected for the x-fast trie)
 ///
 /// In pseudocode (section 13.1 of Morin's book), \f$ \ell \f$ the leaf of
 /// \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( x ) \cr
 /// 2 & \quad \textbf{if}\ x \text{ is not there}\ \textbf{then return false}
 ///     \cr
 /// 3 & \quad \text{unlink } \ell \text{ from the list} \cr
 /// 4 & \quad \text{remove the nodes of the path of } x \text{ from } \ell
 ///     \text{ up, while they have no child} \cr
 /// 5 & \quad u \gets \text{the lowest node left, with one child or none} \cr
 /// 6 & \quad u.\mathit{jump} \gets \ell.\mathit{next} \text{ if its child }
 ///     0 \text{ is missing, } \ell.\mathit{prev} \text{ otherwise} \cr
 /// 7 & \quad \textbf{foreach}\ \text{ancestor } a \text{ of } u
 ///     \text{ with } a.\mathit{jump} = \ell\ \textbf{do} \cr
 /// 8 & \qquad a.\mathit{jump} \gets \ell.\mathit{next} \text{ if its child }
 ///     0 \text{ is missing, } \ell.\mathit{prev} \text{ otherwise} \cr
 /// 9 & \quad \textbf{return true}
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is not there
 bool remove( U x );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of nodes, the root and the leaves included
 [[nodiscard]] std::size_t nodes( void ) const { return( f_nodes ); }

 /// the smallest key, or nothing
 [[nodiscard]] std::optional< U > min( void ) const;

 /// the largest key, or nothing
 [[nodiscard]] std::optional< U > max( void ) const;

 /// removes every key
 void clear( void );

/*--------------------------------------------------------------------------*/
 protected:
 /// a node: an internal node has its two children, its parent and its jump
 /// pointer; a leaf uses its two children as the links of the list
 struct Node
 {
  Node * child[ 2 ] = { nullptr , nullptr };  ///< the children, or prev
					     ///< and next for a leaf
  Node * jump = nullptr;                     ///< the jump pointer
  Node * parent = nullptr;                   ///< the parent
  U key = 0;                                 ///< the key of a leaf
  V value{};                                 ///< the value of a leaf
  };

 public:
/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs ( key , value ) by increasing key, along
 // the list of the leaves

 /// the bidirectional iterator along the list of the leaves
 class const_iterator
 {
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

  /// on the leaf \f$ l \f$
  explicit const_iterator( const Node * l ) : f_l( l ) { }

  /// the pair of the leaf
  reference operator*( void ) const {
   return( reference( f_l->key , f_l->value ) );
   }

  /// to the next leaf
  const_iterator & operator++( void ) {
   f_l = f_l->child[ 1 ];
   return( *this );
   }

  /// to the next leaf, returning the iterator before the step
  const_iterator operator++( int ) {
   auto t = *this;
   ++*this;
   return( t );
   }

  /// to the previous leaf
  const_iterator & operator--( void ) {
   f_l = f_l->child[ 0 ];
   return( *this );
   }

  /// to the previous leaf, returning the iterator before the step
  const_iterator operator--( int ) {
   auto t = *this;
   --*this;
   return( t );
   }

  /// equal on the same leaf
  bool operator==( const const_iterator & o ) const = default;

  private:
  const Node * f_l = nullptr;
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
  return( const_iterator( f_dummy->child[ 1 ] ) );
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
 protected:
 /// the deepest node of the path of \f$ x \f$ and its depth, by a walk
 /// from the root or, in the x-fast trie, by a binary search on the depth
 /// between \f$ \mathit{lo} \f$, a depth whose prefix of \f$ x \f$ is
 /// there, and \f$ \mathit{hi} \f$, one whose prefix is not (or
 /// \f$ w + 1 \f$)
 [[nodiscard]] std::pair< Node * , int >
 deepest( U x , int lo = 0 , int hi = W + 1 , Node * u = nullptr ) const;

 /// the leaf of the smallest key \f$ \ge x \f$, or the dummy, from the
 /// deepest node \f$ u \f$ of the path of \f$ x \f$, at depth \f$ i \f$
 [[nodiscard]] Node * successor( U x , Node * u , int i ) const;

 /// the node of the prefix \f$ p \f$ of \f$ i \f$ bits, or nil (the
 /// x-fast trie only)
 [[nodiscard]] Node * node( U p , int i ) const;

 /// the leftmost leaf below \f$ u \f$, at depth \f$ i \f$, by the
 /// children 0 and a jump pointer, in \f$ O( w - i ) \f$ time
 /// \pre \f$ u \f$ has a leaf below it
 [[nodiscard]] Node * leftmost( Node * u , int i ) const;

 /// the leaves in order, the jump pointers, the parents and the tables;
 /// \f$ O( n w ) \f$ for \f$ n \le 256 \f$, around the leaf of \f$ x \f$
 /// beyond
 [[nodiscard]] bool consistent( U x ) const;

 /// whether the subtree of \f$ u \f$, at depth \f$ i \f$ and of prefix
 /// \f$ p \f$, is well formed;
 /// its smallest and largest leaf in `lo` and `hi`, its leaves in `n`
 [[nodiscard]] bool subtree( const Node * u , int i , U p , const Node *& lo ,
			     const Node *& hi , std::size_t & n ,
			     std::size_t & m ) const;

 /// frees every node but the root and the dummy
 void destroy( Node * u , int i );

 ObjectPool< Node > f_pool{ 1 << 16 };  ///< the nodes
 Node * f_root = nullptr;               ///< the root
 Node * f_dummy = nullptr;              ///< the head of the list
 std::size_t f_n = 0;                   ///< the number of keys
 std::size_t f_nodes = 1;               ///< the number of nodes
 ADS_NO_UNIQUE_ADDRESS TrieTables< U , Node , W , Hashed > f_t;  ///< tables

 };  // end( class JumpTrie )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BinaryTrie ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary trie of the integers of \f$ w \f$ bits with the list of the
/// leaves and the jump pointers, a SORTED SET with a value per key
template< std::unsigned_integral U , class V = std::monostate ,
	  int W = std::numeric_limits< U >::digits >
 requires TrieWidth< U , W >
class BinaryTrie : public detail::JumpTrie< U , V , W , false >
{
 public:
 using detail::JumpTrie< U , V , W , false >::JumpTrie;
 };

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS SimpleBinaryTrie --------------------------*/
/*--------------------------------------------------------------------------*/

/// a binary trie of the integers of \f$ w \f$ bits with no list and no jump
/// pointers (exercise 13.1 of Morin's book)
template< std::unsigned_integral U , class V = std::monostate ,
	  int W = std::numeric_limits< U >::digits >
 requires TrieWidth< U , W >
class SimpleBinaryTrie
{

/*--------------------------------------------------------------------------*/
 public:
 using Key = U;    ///< the type of the keys
 using Item = U;   ///< the items of the SSet, the keys
 using Value = V;  ///< the type of the values

 /// the empty trie
 SimpleBinaryTrie( void ) = default;

 /// the trie of `o`, which is left empty
 SimpleBinaryTrie( SimpleBinaryTrie && o ) noexcept;

 /// the trie of `o`, which is left empty; the nodes of this one are freed
 SimpleBinaryTrie & operator=( SimpleBinaryTrie && o ) noexcept;

 SimpleBinaryTrie( const SimpleBinaryTrie & ) = delete;
 SimpleBinaryTrie & operator=( const SimpleBinaryTrie & ) = delete;

 /// frees the nodes
 ~SimpleBinaryTrie();

 /// the smallest key \f$ \ge x \f$, nil if none (the `find( x )` of the
 /// SSet of Morin's book), in \f$ O( w ) \f$ time
 ///
 /// In pseudocode (exercise 13.1 of Morin's book), \f$ a \f$ the child 1
 /// of the deepest node of the path of \f$ x \f$ where the path goes to
 /// the child 0 and the child 1 is there, i.e., the root of the larger keys
 /// nearest to \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( x ) \cr
 /// 2 & \quad u \gets \mathit{root};\ a \gets \textbf{nil} \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ w - 1\ \textbf{do} \cr
 /// 4 & \qquad c \gets \text{the bit } i \text{ of } x \cr
 /// 5 & \qquad \textbf{if}\ c = 0\ \textbf{and}\ u.\mathit{child}[ 1 ] \ne
 ///     \textbf{nil}\ \textbf{then}\ a \gets u.\mathit{child}[ 1 ] \cr
 /// 6 & \qquad \textbf{if}\ u.\mathit{child}[ c ] = \textbf{nil}
 ///     \ \textbf{then break} \cr
 /// 7 & \qquad u \gets u.\mathit{child}[ c ] \cr
 /// 8 & \quad \textbf{if}\ i = w\ \textbf{then return}\ x \cr
 /// 9 & \quad \textbf{if}\ a = \textbf{nil}\ \textbf{then return nil} \cr
 /// 10 & \quad \textbf{return}\ \text{the leftmost leaf below } a
 /// \end{array}}
 /// \f]
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * lowerBound( U x ) const;

 /// the key \f$ x \f$, nil if it is not there, in \f$ O( w ) \f$ time
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] const U * find( U x ) const;

 /// the value of \f$ x \f$, or nothing
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] std::optional< V > lookup( U x ) const;

 /// true if \f$ x \f$ is there
 /// \pre \f$ x < 2^w \f$
 [[nodiscard]] bool contains( U x ) const;

 /// associates \f$ v \f$ to \f$ x \f$; true if \f$ x \f$ was not there, by
 /// adding the missing nodes of its path, \f$ O( w ) \f$ time
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is there
 bool insert( U x , const V & v = V() );

 /// removes \f$ x \f$; true if it was there, by removing the nodes of its
 /// path left with no child, \f$ O( w ) \f$ time
 /// \pre \f$ x < 2^w \f$
 /// \post \f$ x \f$ is not there
 bool remove( U x );

 /// the number \f$ n \f$ of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of nodes, the root and the leaves included
 [[nodiscard]] std::size_t nodes( void ) const { return( f_nodes ); }

 /// `f( x , v )` on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
 private:
 /// an internal node, two children
 struct Node
 {
  Node * child[ 2 ] = { nullptr , nullptr };  ///< the children
  };

 /// a leaf, its key and its value
 struct Leaf : Node
 {
  U key = 0;  ///< the key
  V value{};  ///< the value
  };

 /// the leaf of \f$ x \f$, or nil
 [[nodiscard]] const Leaf * leaf( U x ) const;

 /// frees the subtree of \f$ u \f$, at depth \f$ i \f$
 static void destroy( Node * u , int i );

 /// `f( x , v )` on the leaves below \f$ u \f$, at depth \f$ i \f$
 template< class F >
 static void visit( const Node * u , int i , F & f );

 Node f_root;              ///< the root
 std::size_t f_n = 0;      ///< the number of keys
 std::size_t f_nodes = 1;  ///< the number of nodes

 };  // end( class SimpleBinaryTrie )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/BinaryTrie.tpp"

/*--------------------------------------------------------------------------*/

#endif /* BinaryTrie.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BinaryTrie.h --------------------------*/
/*--------------------------------------------------------------------------*/
