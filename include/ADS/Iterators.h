/*--------------------------------------------------------------------------*/
/*---------------------------- File Iterators.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The iterators that the containers of the library have in common, beyond
 * the book: they give a container the interface of the standard library
 * (begin(), end(), the categories of the iterators), so that the range-for,
 * the algorithms of std and of std::ranges, and those of Boost visit it.
 *
 * - LinkedIterator walks cells linked by a pointer, the member Next, and
 *   reads the member Value of each, the end being nil: a forward iterator
 *   for the singly linked lists, queues and stacks.
 * - IndexIterator walks cells of a vector linked by an index, the member
 *   Next, the end being -1: a forward iterator for the structures whose
 *   cells are a vector with a free list (the stacks sharing one space).
 * - CircularIterator is the random-access iterator on the items of a
 *   circular vector, the item i from the head being at
 *   ( head + i ) mod capacity, so that the order of the items is that of
 *   the queue and not that of the places.
 *
 * - IndirectIterator reads through an iterator on pointers (smart or not),
 *   with its category: a random-access view of a vector of pointers to the
 *   items, as the heaps of handles keep them.
 *
 * - SlotIterator walks the places 0 , ... , m - 1 of a table, skipping
 *   those that a policy P declares empty (or deleted, for the open
 *   addressing with lazy deletion), and reads each through P: P::size(),
 *   P::occupied() and P::read() take the owner and the place, so that a
 *   policy nested in the owner reads its private fields; the reading may
 *   build its result (a pair of references, a key computed from the place),
 *   in which case the iterator is a forward iterator of C++20 and only an
 *   input iterator for the older algorithms, as the views of std are.
 * - KeyIterator reads the member first of what another iterator reads, the
 *   keys of a dictionary as the items of a set.
 *
 * - PathIterator visits a binary tree in symmetric order keeping the path
 *   from the root to the current node, so that it needs no link to the
 *   parent and goes both ways; a policy P gives the root, the children and
 *   the reading of a node, as for SlotIterator. A step costs O( 1 )
 *   amortized and O( h ) at most, the path O( h ) space.
 *
 * - WalkIterator follows the nodes of a tree (pointers or indices) by a
 *   policy P whose next( o , n , root ), o a pointer to the owner (nil if
 *   the nodes need none), gives the node after n in the walk
 *   below root, or P::none: the children of a node, the preorder and the
 *   symmetric order by the links to the parents, with no stack.
 *
 * Each has a constant and a mutable form, by the parameter Const, and the
 * mutable one converts to the constant one. An iterator stays valid as
 * long as the cells (or the vector) it refers to do; the containers that
 * keep an invariant on their items (a heap, a set) expose only the constant
 * form. These are adaptors with no contracts of their own: the containers
 * check theirs.
 *
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, chapter 33 (the iterators)
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

/// a forward iterator along the cells C linked by the member Next, on the
/// items V in the member Value; nil is the end
template< class C , class V , C * C::* Next , V C::* Value ,
          bool Const = true >
class LinkedIterator {
 using Cell = std::conditional_t< Const , const C , C >;

 public:
 using iterator_category = std::forward_iterator_tag;
 using value_type = V;
 using difference_type = std::ptrdiff_t;
 using reference = std::conditional_t< Const , const V & , V & >;
 using pointer = std::conditional_t< Const , const V * , V * >;

 LinkedIterator( void ) = default;
 explicit LinkedIterator( Cell * c ) : f_c( c ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 LinkedIterator( const LinkedIterator< C , V , Next , Value , K > & o )
  : f_c( o.cell() ) {}

 reference operator*( void ) const { return( f_c->*Value ); }
 pointer operator->( void ) const { return( &( f_c->*Value ) ); }

 LinkedIterator & operator++( void ) {
  f_c = f_c->*Next;
  return( *this );
  }

 LinkedIterator operator++( int ) {
  auto t = *this;
  ++*this;
  return( t );
  }

 bool operator==( const LinkedIterator & o ) const = default;

 /// the cell of the item, nil at the end
 [[nodiscard]] Cell * cell( void ) const { return( f_c ); }

 private:
 Cell * f_c = nullptr;

 };  // end( class LinkedIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS IndexIterator ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a forward iterator along the cells C of a vector linked by the index in
/// the member Next, on the items V in the member Value; -1 is the end
template< class C , class V , int C::* Next , V C::* Value ,
          bool Const = true >
class IndexIterator {
 using Cells = std::conditional_t< Const , const std::vector< C > ,
                                   std::vector< C > >;

 public:
 using iterator_category = std::forward_iterator_tag;
 using value_type = V;
 using difference_type = std::ptrdiff_t;
 using reference = std::conditional_t< Const , const V & , V & >;
 using pointer = std::conditional_t< Const , const V * , V * >;

 IndexIterator( void ) = default;
 IndexIterator( Cells * A , int c ) : f_A( A ) , f_c( c ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 IndexIterator( const IndexIterator< C , V , Next , Value , K > & o )
  : f_A( o.cells() ) , f_c( o.index() ) {}

 reference operator*( void ) const { return( ( *f_A )[ f_c ].*Value ); }
 pointer operator->( void ) const { return( &**this ); }

 IndexIterator & operator++( void ) {
  f_c = ( *f_A )[ f_c ].*Next;
  return( *this );
  }

 IndexIterator operator++( int ) {
  auto t = *this;
  ++*this;
  return( t );
  }

 /// equal on the same cell; any two ends are equal
 bool operator==( const IndexIterator & o ) const {
  return( ( f_c == o.f_c ) && ( ( f_c < 0 ) || ( f_A == o.f_A ) ) );
  }

 [[nodiscard]] Cells * cells( void ) const { return( f_A ); }
 [[nodiscard]] int index( void ) const { return( f_c ); }

 private:
 Cells * f_A = nullptr;
 int f_c = -1;

 };  // end( class IndexIterator )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS CircularIterator --------------------------*/
/*--------------------------------------------------------------------------*/

/// the random-access iterator on the items of a circular vector of the
/// given capacity whose first item is at the place head
template< class T , bool Const = true >
class CircularIterator {
 using Item = std::conditional_t< Const , const T , T >;

 public:
 using iterator_category = std::random_access_iterator_tag;
 using value_type = T;
 using difference_type = std::ptrdiff_t;
 using reference = Item &;
 using pointer = Item *;

 CircularIterator( void ) = default;
 CircularIterator( Item * A , std::size_t head , std::size_t capacity ,
                   difference_type i )
  : f_A( A ) , f_head( head ) , f_cap( capacity ) , f_i( i ) {}

 /// the constant iterator from the mutable one
 template< bool K > requires ( Const && ! K )
 CircularIterator( const CircularIterator< T , K > & o )
  : f_A( o.base() ) , f_head( o.head() ) , f_cap( o.capacity() ) ,
    f_i( o.index() ) {}

 reference operator*( void ) const { return( ( *this )[ 0 ] ); }
 pointer operator->( void ) const { return( &**this ); }

 reference operator[]( difference_type k ) const {
  return( f_A[ ( f_head + std::size_t( f_i + k ) ) % f_cap ] );
  }

 CircularIterator & operator++( void ) { ++f_i; return( *this ); }
 CircularIterator & operator--( void ) { --f_i; return( *this ); }
 CircularIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 CircularIterator operator--( int ) { auto t = *this; --f_i; return( t ); }
 CircularIterator & operator+=( difference_type k ) {
  f_i += k;
  return( *this );
  }
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

 bool operator==( const CircularIterator & o ) const {
  return( f_i == o.f_i );
  }
 std::strong_ordering operator<=>( const CircularIterator & o ) const {
  return( f_i <=> o.f_i );
  }

 [[nodiscard]] Item * base( void ) const { return( f_A ); }
 [[nodiscard]] std::size_t head( void ) const { return( f_head ); }
 [[nodiscard]] std::size_t capacity( void ) const { return( f_cap ); }
 [[nodiscard]] difference_type index( void ) const { return( f_i ); }

 private:
 Item * f_A = nullptr;
 std::size_t f_head = 0 , f_cap = 1;
 difference_type f_i = 0;     ///< the item from the head

 };  // end( class CircularIterator )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS IndirectIterator --------------------------*/
/*--------------------------------------------------------------------------*/

/// the iterator I on pointers read through them, *it being **base, with
/// the category of I (at most random access); the items are constant
template< std::random_access_iterator I >
class IndirectIterator {
 public:
 using iterator_category = std::random_access_iterator_tag;
 using value_type = std::remove_cvref_t< decltype( **std::declval< I >() ) >;
 using difference_type = std::iter_difference_t< I >;
 using reference = const value_type &;
 using pointer = const value_type *;

 IndirectIterator( void ) = default;
 explicit IndirectIterator( I i ) : f_i( i ) {}

 reference operator*( void ) const { return( **f_i ); }
 pointer operator->( void ) const { return( &**f_i ); }
 reference operator[]( difference_type k ) const { return( *f_i[ k ] ); }

 IndirectIterator & operator++( void ) { ++f_i; return( *this ); }
 IndirectIterator & operator--( void ) { --f_i; return( *this ); }
 IndirectIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 IndirectIterator operator--( int ) { auto t = *this; --f_i; return( t ); }
 IndirectIterator & operator+=( difference_type k ) {
  f_i += k;
  return( *this );
  }
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

 bool operator==( const IndirectIterator & o ) const = default;
 auto operator<=>( const IndirectIterator & o ) const = default;

 /// the iterator on the pointers
 [[nodiscard]] I base( void ) const { return( f_i ); }

 private:
 I f_i{};

 };  // end( class IndirectIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SlotIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bidirectional iterator on the occupied places of the table O, by the
/// policy P with the static functions size( o ), occupied( o , i ) and
/// read( o , i ); the items are constant
template< class O , class P >
class SlotIterator {
 public:
 using reference = decltype( P::read( std::declval< const O & >() , 0 ) );
 using value_type = std::remove_cvref_t< reference >;
 using difference_type = std::ptrdiff_t;
 using iterator_concept = std::bidirectional_iterator_tag;
 using iterator_category = std::conditional_t<
  std::is_reference_v< reference > , std::bidirectional_iterator_tag ,
  std::input_iterator_tag >;

 SlotIterator( void ) = default;

 /// on the first occupied place from i, or at the end
 SlotIterator( const O * o , std::size_t i ) : f_o( o ) , f_i( i ) {
  forward();
  }

 reference operator*( void ) const { return( P::read( *f_o , f_i ) ); }

 SlotIterator & operator++( void ) {
  ++f_i;
  forward();
  return( *this );
  }

 SlotIterator & operator--( void ) {
  do --f_i; while( ! P::occupied( *f_o , f_i ) );
  return( *this );
  }

 SlotIterator operator++( int ) { auto t = *this; ++*this; return( t ); }
 SlotIterator operator--( int ) { auto t = *this; --*this; return( t ); }

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

/// the member first of what the iterator I reads, with the category of I
/// up to bidirectional
template< std::forward_iterator I >
class KeyIterator {
 public:
 using reference = const std::remove_cvref_t<
  decltype( ( *std::declval< I >() ).first ) > &;
 using value_type = std::remove_cvref_t< reference >;
 using difference_type = std::iter_difference_t< I >;
 using iterator_concept = std::conditional_t<
  std::bidirectional_iterator< I > , std::bidirectional_iterator_tag ,
  std::forward_iterator_tag >;
 using iterator_category = iterator_concept;

 KeyIterator( void ) = default;
 explicit KeyIterator( I i ) : f_i( i ) {}

 reference operator*( void ) const { return( ( *f_i ).first ); }
 const value_type * operator->( void ) const { return( &**this ); }

 KeyIterator & operator++( void ) { ++f_i; return( *this ); }
 KeyIterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
 KeyIterator & operator--( void ) requires std::bidirectional_iterator< I > {
  --f_i;
  return( *this );
  }
 KeyIterator operator--( int ) requires std::bidirectional_iterator< I > {
  auto t = *this;
  --f_i;
  return( t );
  }

 bool operator==( const KeyIterator & o ) const = default;

 private:
 I f_i{};

 };  // end( class KeyIterator )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PathIterator ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a bidirectional iterator on the nodes of a binary tree of the owner O in
/// symmetric order, by the policy P with the static functions root( o ),
/// left( n ), right( n ) (nil for no child) and read( n ); the items are
/// constant, and the end is the empty path
template< class O , class P >
class PathIterator {
 using N = std::remove_pointer_t<
  decltype( P::root( std::declval< const O & >() ) ) >;

 public:
 using reference = decltype( P::read( std::declval< N * >() ) );
 using value_type = std::remove_cvref_t< reference >;
 using difference_type = std::ptrdiff_t;
 using iterator_concept = std::bidirectional_iterator_tag;
 using iterator_category = std::conditional_t<
  std::is_reference_v< reference > , std::bidirectional_iterator_tag ,
  std::input_iterator_tag >;

 PathIterator( void ) = default;

 /// on the first node of the tree of o if first, at the end otherwise
 PathIterator( const O * o , bool first ) : f_o( o ) {
  if( first ) descend( P::root( *f_o ) , true );
  }

 reference operator*( void ) const { return( P::read( f_path.back() ) ); }

 PathIterator & operator++( void ) {
  step( true );
  return( *this );
  }

 PathIterator & operator--( void ) {
  if( f_path.empty() ) descend( P::root( *f_o ) , false );
  else step( false );
  return( *this );
  }

 PathIterator operator++( int ) { auto t = *this; ++*this; return( t ); }
 PathIterator operator--( int ) { auto t = *this; --*this; return( t ); }

 bool operator==( const PathIterator & o ) const {
  return( f_path.empty() ? o.f_path.empty()
                         : ( ! o.f_path.empty() &&
                             ( f_path.back() == o.f_path.back() ) ) );
  }

 private:
 static N * child( N * n , bool right ) {
  return( right ? P::right( n ) : P::left( n ) );
  }

 /// from n down to its first node (or last, if not first)
 void descend( N * n , bool first ) {
  for( ; n ; n = child( n , ! first ) ) f_path.push_back( n );
  }

 /// to the next node (or the previous one, if not forward)
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

/// a forward iterator on the nodes H (pointers or indices) of the owner O,
/// from a first node, by the policy P with next( o , n , root ) and the
/// constant none, the end, o being a pointer to the owner; *it is the
/// node
template< class O , class H , class P >
class WalkIterator {
 public:
 using value_type = H;
 using reference = H;
 using difference_type = std::ptrdiff_t;
 using iterator_concept = std::forward_iterator_tag;
 using iterator_category = std::input_iterator_tag;

 WalkIterator( void ) = default;
 WalkIterator( const O * o , H n , H root )
  : f_o( o ) , f_n( n ) , f_root( root ) {}

 reference operator*( void ) const { return( f_n ); }

 WalkIterator & operator++( void ) {
  f_n = P::next( f_o , f_n , f_root );
  return( *this );
  }

 WalkIterator operator++( int ) { auto t = *this; ++*this; return( t ); }

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
