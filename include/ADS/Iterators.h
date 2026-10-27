/*--------------------------------------------------------------------------*/
/*---------------------------- File Iterators.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The iterators that the containers of the library have in common, beyond
 * the book: they give a container the interface of the standard library
 * (`begin()`, `end()`, the categories of the iterators), so that the
 * range-for, the algorithms of `std` and of `std::ranges`, and those of
 * Boost visit it.
 *
 * - `LinkedIterator` walks cells linked by a pointer, the member `Next`, and
 *   reads the member `Value` of each, the end being nil: a forward iterator
 *   for the singly linked lists, queues and stacks.
 * - `IndexIterator` walks cells of a vector linked by an index, the member
 *   `Next`, the end being -1: a forward iterator for the structures whose
 *   cells are a vector with a free list (the stacks sharing one space).
 * - `CircularIterator` is the random-access iterator on the items of a
 *   circular vector, the item \f$ i \f$ from the head being at the place
 *   \f$ ( \mathit{head} + i ) \bmod c \f$, \f$ c \f$ the capacity, so that
 *   the order of the items is that of the queue and not that of the places.
 *
 * - `IndirectIterator` reads through an iterator on pointers (smart or
 *   not), with its category: a random-access view of a vector of pointers to
 *   the items, as the heaps of handles keep them.
 *
 * - `SlotIterator` walks the places \f$ 0 , \ldots , m - 1 \f$ of a table,
 *   skipping those that a policy `P` declares empty (or deleted, for the
 *   open addressing with lazy deletion), and reads each through `P`:
 *   `P::size()`, `P::occupied()` and `P::read()` take the owner and the
 *   place, so that a policy nested in the owner reads its private fields;
 *   the reading may build its result (a pair of references, a key computed
 *   from the place), in which case the iterator is a forward iterator of
 *   C++20 and only an input iterator for the older algorithms, as the views
 *   of `std` are. A whole visit costs \f$ \Theta( m ) \f$.
 * - `KeyIterator` reads the member `first` of what another iterator reads,
 *   the keys of a dictionary as the items of a set.
 *
 * - `PathIterator` visits a binary tree in symmetric order keeping the path
 *   from the root to the current node, so that it needs no link to the
 *   parent and goes both ways; a policy `P` gives the root, the children and
 *   the reading of a node, as for `SlotIterator`. A step costs
 *   \f$ O( 1 ) \f$ amortized and \f$ O( h ) \f$ at most, the path
 *   \f$ O( h ) \f$ space.
 *
 * - `WalkIterator` follows the nodes of a tree (pointers or indices) by a
 *   policy `P` whose `next( o , n , root )`, `o` a pointer to the owner (nil
 *   if the nodes need none), gives the node after `n` in the walk below
 *   `root`, or `P::none`: the children of a node, the preorder and the
 *   symmetric order by the links to the parents, with no stack.
 *
 * Each has a constant and a mutable form, by the parameter `Const`, and the
 * mutable one converts to the constant one. An iterator stays valid as long
 * as the cells (or the vector) it refers to do; the containers that keep an
 * invariant on their items (a heap, a set) expose only the constant form.
 * These are adaptors with no contracts of their own: the containers check
 * theirs.
 *
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, chapter 33 (the iterators)
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

#ifndef __ADS_Iterators
 #define __ADS_Iterators
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <compare>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS LinkedIterator ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a forward iterator along the cells `C` linked by the member `Next`, on
/// the items `V` in the member `Value`; nil is the end
template< class C , class V , C * C::* Next , V C::* Value ,
          bool Const = true >
class LinkedIterator {
 using Cell = std::conditional_t< Const , const C , C >;

 public:
 using iterator_category = std::forward_iterator_tag;   ///< forward
 using value_type = V;                                 ///< the items
 using difference_type = std::ptrdiff_t;               ///< the distances
 /// a reference to an item, constant if `Const`
 using reference = std::conditional_t< Const , const V & , V & >;
 /// a pointer to an item, constant if `Const`
 using pointer = std::conditional_t< Const , const V * , V * >;

 /// the end
 LinkedIterator( void ) = default;

 /// on the item of the cell \f$ c \f$, the end if \f$ c \f$ is nil
 explicit LinkedIterator( Cell * c ) : f_c( c ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 LinkedIterator( const LinkedIterator< C , V , Next , Value , K > & o )
  : f_c( o.cell() ) {}

 /// the item
 reference operator*( void ) const { return( f_c->*Value ); }

 /// the address of the item
 pointer operator->( void ) const { return( &( f_c->*Value ) ); }

 /// to the next cell, by the link `Next`
 LinkedIterator & operator++( void ) {
  f_c = f_c->*Next;
  return( *this );
  }

 /// to the next cell, returning the iterator before the step
 LinkedIterator operator++( int ) {
  auto t = *this;
  ++*this;
  return( t );
  }

 /// equal on the same cell
 bool operator==( const LinkedIterator & o ) const = default;

 /// the cell of the item, nil at the end
 [[nodiscard]] Cell * cell( void ) const { return( f_c ); }

 private:
 Cell * f_c = nullptr;

 };  // end( class LinkedIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS IndexIterator ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a forward iterator along the cells `C` of a vector linked by the index in
/// the member `Next`, on the items `V` in the member `Value`; -1 is the end
template< class C , class V , int C::* Next , V C::* Value ,
          bool Const = true >
class IndexIterator {
 using Cells = std::conditional_t< Const , const std::vector< C > ,
                                   std::vector< C > >;

 public:
 using iterator_category = std::forward_iterator_tag;   ///< forward
 using value_type = V;                                 ///< the items
 using difference_type = std::ptrdiff_t;               ///< the distances
 /// a reference to an item, constant if `Const`
 using reference = std::conditional_t< Const , const V & , V & >;
 /// a pointer to an item, constant if `Const`
 using pointer = std::conditional_t< Const , const V * , V * >;

 /// the end
 IndexIterator( void ) = default;

 /// on the item of the cell \f$ A[ c ] \f$, the end if \f$ c = -1 \f$
 IndexIterator( Cells * A , int c ) : f_A( A ) , f_c( c ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 IndexIterator( const IndexIterator< C , V , Next , Value , K > & o )
  : f_A( o.cells() ) , f_c( o.index() ) {}

 /// the item
 reference operator*( void ) const { return( ( *f_A )[ f_c ].*Value ); }

 /// the address of the item
 pointer operator->( void ) const { return( &**this ); }

 /// to the next cell, by the index `Next`
 IndexIterator & operator++( void ) {
  f_c = ( *f_A )[ f_c ].*Next;
  return( *this );
  }

 /// to the next cell, returning the iterator before the step
 IndexIterator operator++( int ) {
  auto t = *this;
  ++*this;
  return( t );
  }

 /// equal on the same cell; any two ends are equal
 bool operator==( const IndexIterator & o ) const {
  return( ( f_c == o.f_c ) && ( ( f_c < 0 ) || ( f_A == o.f_A ) ) );
  }

 /// the vector of the cells
 [[nodiscard]] Cells * cells( void ) const { return( f_A ); }

 /// the index of the cell, -1 at the end
 [[nodiscard]] int index( void ) const { return( f_c ); }

 private:
 Cells * f_A = nullptr;
 int f_c = -1;

 };  // end( class IndexIterator )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS CircularIterator --------------------------*/
/*--------------------------------------------------------------------------*/

/// the random-access iterator on the items of a circular vector of the
/// capacity \f$ c \f$ whose first item is at the place \f$ \mathit{head} \f$,
/// the item \f$ i \f$ at the place \f$ ( \mathit{head} + i ) \bmod c \f$
template< class T , bool Const = true >
class CircularIterator {
 using Item = std::conditional_t< Const , const T , T >;

 public:
 /// random access
 using iterator_category = std::random_access_iterator_tag;
 using value_type = T;                     ///< the items
 using difference_type = std::ptrdiff_t;   ///< the distances
 using reference = Item &;                 ///< a reference to an item
 using pointer = Item *;                   ///< a pointer to an item

 /// an iterator on no vector
 CircularIterator( void ) = default;

 /// on the item \f$ i \f$ from the head of the vector \f$ A \f$
 CircularIterator( Item * A , std::size_t head , std::size_t capacity ,
                   difference_type i )
  : f_A( A ) , f_head( head ) , f_cap( capacity ) , f_i( i ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 CircularIterator( const CircularIterator< T , K > & o )
  : f_A( o.base() ) , f_head( o.head() ) , f_cap( o.capacity() ) ,
    f_i( o.index() ) {}

 /// the item
 reference operator*( void ) const { return( ( *this )[ 0 ] ); }

 /// the address of the item
 pointer operator->( void ) const { return( &**this ); }

 /// the item \f$ k \f$ places on, at the place
 /// \f$ ( \mathit{head} + i + k ) \bmod c \f$
 reference operator[]( difference_type k ) const {
  return( f_A[ ( f_head + std::size_t( f_i + k ) ) % f_cap ] );
  }

 /// to the next item
 CircularIterator & operator++( void ) { ++f_i; return( *this ); }

 /// to the previous item
 CircularIterator & operator--( void ) { --f_i; return( *this ); }

 /// to the next item, returning the iterator before the step
 CircularIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }

 /// to the previous item, returning the iterator before the step
 CircularIterator operator--( int ) { auto t = *this; --f_i; return( t ); }

 /// \f$ k \f$ items on
 CircularIterator & operator+=( difference_type k ) {
  f_i += k;
  return( *this );
  }

 /// \f$ k \f$ items back
 CircularIterator & operator-=( difference_type k ) {
  f_i -= k;
  return( *this );
  }

 friend CircularIterator operator+( CircularIterator a , difference_type k ) {
  return( a += k );
  }
 friend CircularIterator operator+( difference_type k , CircularIterator a ) {
  return( a += k );
  }
 friend CircularIterator operator-( CircularIterator a , difference_type k ) {
  return( a -= k );
  }
 friend difference_type operator-( const CircularIterator & a ,
                                   const CircularIterator & b ) {
  return( a.f_i - b.f_i );
  }

 /// equal on the same index from the head
 bool operator==( const CircularIterator & o ) const {
  return( f_i == o.f_i );
  }

 /// the order of the indices from the head
 std::strong_ordering operator<=>( const CircularIterator & o ) const {
  return( f_i <=> o.f_i );
  }

 /// the vector
 [[nodiscard]] Item * base( void ) const { return( f_A ); }

 /// the place of the first item
 [[nodiscard]] std::size_t head( void ) const { return( f_head ); }

 /// the capacity \f$ c \f$ of the vector
 [[nodiscard]] std::size_t capacity( void ) const { return( f_cap ); }

 /// the index \f$ i \f$ of the item from the head
 [[nodiscard]] difference_type index( void ) const { return( f_i ); }

 private:
 Item * f_A = nullptr;
 std::size_t f_head = 0 , f_cap = 1;
 difference_type f_i = 0;     ///< the item from the head

 };  // end( class CircularIterator )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS IndirectIterator --------------------------*/
/*--------------------------------------------------------------------------*/

/// the iterator `I` on pointers read through them, `*it` being `**base`,
/// with the category of `I` (at most random access); the items are constant
template< std::random_access_iterator I >
class IndirectIterator {
 public:
 /// random access
 using iterator_category = std::random_access_iterator_tag;
 /// the items, those pointed to
 using value_type = std::remove_cvref_t< decltype( **std::declval< I >() ) >;
 /// the distances, those of `I`
 using difference_type = std::iter_difference_t< I >;
 using reference = const value_type &;   ///< a constant reference to an item
 using pointer = const value_type *;     ///< a constant pointer to an item

 /// an iterator on nothing
 IndirectIterator( void ) = default;

 /// on the item pointed to by `*i`
 explicit IndirectIterator( I i ) : f_i( i ) {}

 /// the item pointed to
 reference operator*( void ) const { return( **f_i ); }

 /// the address of the item pointed to
 pointer operator->( void ) const { return( &**f_i ); }

 /// the item pointed to \f$ k \f$ places on
 reference operator[]( difference_type k ) const { return( *f_i[ k ] ); }

 /// to the next pointer
 IndirectIterator & operator++( void ) { ++f_i; return( *this ); }

 /// to the previous pointer
 IndirectIterator & operator--( void ) { --f_i; return( *this ); }

 /// to the next pointer, returning the iterator before the step
 IndirectIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }

 /// to the previous pointer, returning the iterator before the step
 IndirectIterator operator--( int ) { auto t = *this; --f_i; return( t ); }

 /// \f$ k \f$ pointers on
 IndirectIterator & operator+=( difference_type k ) {
  f_i += k;
  return( *this );
  }

 /// \f$ k \f$ pointers back
 IndirectIterator & operator-=( difference_type k ) {
  f_i -= k;
  return( *this );
  }

 friend IndirectIterator operator+( IndirectIterator a , difference_type k ) {
  return( a += k );
  }
 friend IndirectIterator operator+( difference_type k , IndirectIterator a ) {
  return( a += k );
  }
 friend IndirectIterator operator-( IndirectIterator a , difference_type k ) {
  return( a -= k );
  }
 friend difference_type operator-( const IndirectIterator & a ,
                                   const IndirectIterator & b ) {
  return( a.f_i - b.f_i );
  }

 /// equal on the same pointer
 bool operator==( const IndirectIterator & o ) const = default;

 /// the order of the iterators on the pointers
 auto operator<=>( const IndirectIterator & o ) const = default;

 /// the iterator on the pointers
 [[nodiscard]] I base( void ) const { return( f_i ); }

 private:
 I f_i{};

 };  // end( class IndirectIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SlotIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bidirectional iterator on the occupied places of the table `O`, by the
/// policy `P` with the static functions `size( o )`, `occupied( o , i )` and
/// `read( o , i )`; the items are constant
template< class O , class P >
class SlotIterator {
 public:
 /// what `P::read()` gives
 using reference = decltype( P::read( std::declval< const O & >() , 0 ) );
 using value_type = std::remove_cvref_t< reference >;   ///< the items
 using difference_type = std::ptrdiff_t;                ///< the distances
 /// bidirectional for the concepts of C++20
 using iterator_concept = std::bidirectional_iterator_tag;
 /// bidirectional for the older algorithms if `P::read()` gives a
 /// reference, input otherwise
 using iterator_category = std::conditional_t<
  std::is_reference_v< reference > , std::bidirectional_iterator_tag ,
  std::input_iterator_tag >;

 /// an iterator on no table
 SlotIterator( void ) = default;

 /// on the first occupied place from \f$ i \f$, or at the end
 SlotIterator( const O * o , std::size_t i ) : f_o( o ) , f_i( i ) {
  forward();
  }

 /// the item of the place, read by `P::read()`
 reference operator*( void ) const { return( P::read( *f_o , f_i ) ); }

 /// to the next occupied place, or to the end \f$ m \f$
 ///
 /// In pseudocode (beyond the book), \f$ i \f$ the place and \f$ m \f$ the
 /// size of the table; a whole visit examines each place once:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{next}() \cr
 /// 2 & \quad i \gets i + 1 \cr
 /// 3 & \quad \textbf{while}\ i < m\ \textbf{and not}\ \mathrm{occupied}( i
 ///     )\ \textbf{do}\ i \gets i + 1
 /// \end{array}}
 /// \f]
 SlotIterator & operator++( void ) {
  ++f_i;
  forward();
  return( *this );
  }

 /// to the previous occupied place
 ///
 /// In pseudocode (beyond the book), the step of `operator++()` backwards:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{prev}() \cr
 /// 2 & \quad \textbf{repeat}\ i \gets i - 1\ \textbf{until}
 ///     \ \mathrm{occupied}( i )
 /// \end{array}}
 /// \f]
 /// \pre an occupied place precedes the current one
 SlotIterator & operator--( void ) {
  do --f_i; while( ! P::occupied( *f_o , f_i ) );
  return( *this );
  }

 /// to the next occupied place, returning the iterator before the step
 SlotIterator operator++( int ) { auto t = *this; ++*this; return( t ); }

 /// to the previous occupied place, returning the iterator before the step
 SlotIterator operator--( int ) { auto t = *this; --*this; return( t ); }

 /// equal on the same place
 bool operator==( const SlotIterator & o ) const { return( f_i == o.f_i ); }

 /// the place
 [[nodiscard]] std::size_t place( void ) const { return( f_i ); }

 private:
 void forward( void ) {
  const std::size_t m = P::size( *f_o );
  while( ( f_i < m ) && ! P::occupied( *f_o , f_i ) ) ++f_i;
  }

 const O * f_o = nullptr;
 std::size_t f_i = 0;

 };  // end( class SlotIterator )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS KeyIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the member `first` of what the iterator `I` reads, with the category of
/// `I` up to bidirectional
template< std::forward_iterator I >
class KeyIterator {
 public:
 /// a constant reference to a key
 using reference = const std::remove_cvref_t<
  decltype( ( *std::declval< I >() ).first ) > &;
 using value_type = std::remove_cvref_t< reference >;   ///< the keys
 /// the distances, those of `I`
 using difference_type = std::iter_difference_t< I >;
 /// bidirectional if `I` is, forward otherwise
 using iterator_concept = std::conditional_t<
  std::bidirectional_iterator< I > , std::bidirectional_iterator_tag ,
  std::forward_iterator_tag >;
 using iterator_category = iterator_concept;   ///< as `iterator_concept`

 /// an iterator on nothing
 KeyIterator( void ) = default;

 /// on the key of what `i` reads
 explicit KeyIterator( I i ) : f_i( i ) {}

 /// the key
 reference operator*( void ) const { return( ( *f_i ).first ); }

 /// the address of the key
 const value_type * operator->( void ) const { return( &**this ); }

 /// to the next key
 KeyIterator & operator++( void ) { ++f_i; return( *this ); }

 /// to the next key, returning the iterator before the step
 KeyIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }

 /// to the previous key
 KeyIterator & operator--( void ) requires std::bidirectional_iterator< I > {
  --f_i;
  return( *this );
  }

 /// to the previous key, returning the iterator before the step
 KeyIterator operator--( int ) requires std::bidirectional_iterator< I > {
  auto t = *this;
  --f_i;
  return( t );
  }

 /// equal when the iterators on the pairs are
 bool operator==( const KeyIterator & o ) const = default;

 private:
 I f_i{};

 };  // end( class KeyIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PathIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bidirectional iterator on the nodes of a binary tree of the owner `O`
/// in symmetric order, by the policy `P` with the static functions
/// `root( o )`, `left( n )`, `right( n )` (nil for no child) and
/// `read( n )`; the items are constant, and the end is the empty path
template< class O , class P >
class PathIterator {
 using N = std::remove_pointer_t<
  decltype( P::root( std::declval< const O & >() ) ) >;

 public:
 /// what `P::read()` gives
 using reference = decltype( P::read( std::declval< N * >() ) );
 using value_type = std::remove_cvref_t< reference >;   ///< the items
 using difference_type = std::ptrdiff_t;                ///< the distances
 /// bidirectional for the concepts of C++20
 using iterator_concept = std::bidirectional_iterator_tag;
 /// bidirectional for the older algorithms if `P::read()` gives a
 /// reference, input otherwise
 using iterator_category = std::conditional_t<
  std::is_reference_v< reference > , std::bidirectional_iterator_tag ,
  std::input_iterator_tag >;

 /// the end of no tree
 PathIterator( void ) = default;

 /// on the first node of the tree of `o` if `first`, at the end otherwise
 PathIterator( const O * o , bool first ) : f_o( o ) {
  if( first ) descend( P::root( *f_o ) , true );
  }

 /// the item of the current node, read by `P::read()`
 reference operator*( void ) const { return( P::read( f_path.back() ) ); }

 /// to the next node in symmetric order, \f$ O( 1 ) \f$ amortized over a
 /// whole visit and \f$ O( h ) \f$ at most
 ///
 /// In pseudocode (beyond the book), \f$ \pi \f$ the path from the root to
 /// the current node, which is its last node: the leftmost node of the right
 /// subtree if there is one, else the lowest ancestor reached from a left
 /// child:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{next}() \cr
 /// 2 & \quad x \gets \text{the last node of } \pi \cr
 /// 3 & \quad \textbf{if}\ x.\mathit{right} \ne \textbf{nil}\ \textbf{then}
 ///     \cr
 /// 4 & \qquad y \gets x.\mathit{right} \cr
 /// 5 & \qquad \textbf{while}\ y \ne \textbf{nil}\ \textbf{do}\ \text{append }
 ///     y \text{ to } \pi;\ y \gets y.\mathit{left} \cr
 /// 6 & \quad \textbf{else} \cr
 /// 7 & \qquad \textbf{repeat} \cr
 /// 8 & \qquad\quad y \gets \text{the last node of } \pi;
 ///     \ \text{remove it from } \pi \cr
 /// 9 & \qquad \textbf{until}\ \pi \text{ is empty }\ \textbf{or}
 ///     \ y \ne ( \text{the last node of } \pi ).\mathit{right}
 /// \end{array}}
 /// \f]
 /// \pre not at the end
 PathIterator & operator++( void ) {
  step( true );
  return( *this );
  }

 /// to the previous node in symmetric order, from the end to the last node
 ///
 /// In pseudocode (beyond the book), `operator++()` with left and right
 /// swapped, the end going to the rightmost node:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{prev}() \cr
 /// 2 & \quad \textbf{if}\ \pi \text{ is empty }\ \textbf{then}
 ///     \ \text{append to } \pi \text{ the rightmost path from the root} \cr
 /// 3 & \quad \textbf{else}\ \mathrm{next}() \text{ with left and right }
 ///     \text{swapped}
 /// \end{array}}
 /// \f]
 /// \pre not on the first node
 PathIterator & operator--( void ) {
  if( f_path.empty() ) descend( P::root( *f_o ) , false );
  else step( false );
  return( *this );
  }

 /// to the next node, returning the iterator before the step
 PathIterator operator++( int ) { auto t = *this; ++*this; return( t ); }

 /// to the previous node, returning the iterator before the step
 PathIterator operator--( int ) { auto t = *this; --*this; return( t ); }

 /// equal on the same node, or both at the end
 bool operator==( const PathIterator & o ) const {
  return( f_path.empty() ? o.f_path.empty()
                         : ( ! o.f_path.empty() &&
                             ( f_path.back() == o.f_path.back() ) ) );
  }

 private:
 static N * child( N * n , bool right ) {
  return( right ? P::right( n ) : P::left( n ) );
  }

 /// from `n` down to its first node (or last, if not `first`)
 void descend( N * n , bool first ) {
  for( ; n ; n = child( n , ! first ) ) f_path.push_back( n );
  }

 /// to the next node (or the previous one, if not `forward`)
 void step( bool forward ) {
  if( N * c = child( f_path.back() , forward ) ) {
   descend( c , forward );
   return;
   }
  N * from;                                 // up while coming from the
  do {                                      // side we move toward
   from = f_path.back();
   f_path.pop_back();
   } while( ! f_path.empty() && ( child( f_path.back() , forward ) == from ) );
  }

 const O * f_o = nullptr;
 std::vector< N * > f_path;   ///< from the root to the current node

 };  // end( class PathIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS WalkIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a forward iterator on the nodes `H` (pointers or indices) of the owner
/// `O`, from a first node, by the policy `P` with `next( o , n , root )` and
/// the constant `none`, the end, `o` being a pointer to the owner; `*it` is
/// the node
template< class O , class H , class P >
class WalkIterator {
 public:
 using value_type = H;                              ///< the nodes
 using reference = H;                               ///< a node, by value
 using difference_type = std::ptrdiff_t;            ///< the distances
 /// forward for the concepts of C++20
 using iterator_concept = std::forward_iterator_tag;
 /// input for the older algorithms, since `*it` is a value
 using iterator_category = std::input_iterator_tag;

 /// the end
 WalkIterator( void ) = default;

 /// on the node `n` of the walk below `root` of the owner `o`
 WalkIterator( const O * o , H n , H root )
  : f_o( o ) , f_n( n ) , f_root( root ) {}

 /// the node
 reference operator*( void ) const { return( f_n ); }

 /// to the next node of the walk, `P::next( o , n , root )`
 WalkIterator & operator++( void ) {
  f_n = P::next( f_o , f_n , f_root );
  return( *this );
  }

 /// to the next node, returning the iterator before the step
 WalkIterator operator++( int ) { auto t = *this; ++*this; return( t ); }

 /// equal on the same node
 bool operator==( const WalkIterator & o ) const { return( f_n == o.f_n ); }

 private:
 const O * f_o = nullptr;
 H f_n = P::none;
 H f_root = P::none;

 };  // end( class WalkIterator )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Iterators.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Iterators.h --------------------------*/
/*--------------------------------------------------------------------------*/
