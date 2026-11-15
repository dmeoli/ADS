/*--------------------------------------------------------------------------*/
/*------------------------ File CubishArrayStack.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The cubish array stack of exercise 2.12 of Morin's book, beyond the book:
 * a LIST by rank in three levels, whose `operator[]` takes \f$ O( 1 ) \f$
 * time and whose insertions and removals take \f$ O( n^{1/3} ) \f$
 * amortized time, with \f$ O( n^{2/3} ) \f$ places wasted. It is the tiered
 * vector of Goodrich and Kloss with three levels [see `TieredVector.h` for
 * the one with two]: \f$ b \f$ a power of 2 with
 * \f$ b^3 / 8 \le n \le 8 b^3 \f$, the items are in leaves of \f$ b \f$
 * places, each a circular buffer with the offset of its first item; the
 * leaves are in middle blocks of \f$ b^2 \f$ items each, all full but the
 * last, and a middle block is a circular buffer of \f$ 2 b \f$ leaves whose
 * leaves are all full but the first, with \f$ g < b \f$ places free at its
 * front, and the last. The item of rank \f$ i \f$ is the item
 * \f$ r = i \bmod b^2 \f$ of the middle block
 * \f$ \lfloor i / b^2 \rfloor \f$, and the item of rank \f$ r \f$ of a
 * middle block is the item \f$ ( g + r ) \bmod b \f$ of its leaf
 * \f$ \lfloor ( g + r ) / b \rfloor \f$: shifts, masks and five reads.
 *
 * A middle block takes an item at its front or gives one from its back in
 * \f$ O( 1 ) \f$ time, by its gap and the offsets of its leaves, so that an
 * insertion at the rank \f$ i \f$ moves the last item of each full middle
 * block after the one of \f$ i \f$ to the front of the next one, at most
 * \f$ n / b^2 + 1 = O( n^{1/3} ) \f$ of them, and then inserts in its middle
 * block as a tiered vector does, \f$ O( b ) \f$ items of the leaf and one
 * item for each of the \f$ O( b ) \f$ leaves after it; a removal is the
 * symmetric. The size of the leaves follows \f$ n \f$, the items being
 * copied to leaves of the new \f$ b \f$ in \f$ O( n ) \f$ time once every
 * \f$ \Omega( n ) \f$ operations. The places wasted are those of the first
 * and the last leaf of each middle block and the directories of the
 * leaves, \f$ O( b ) \f$ for each of the \f$ O( n / b^2 ) \f$ middle
 * blocks, i.e., \f$ O( n / b ) = O( n^{2/3} ) \f$; a few leaves freed are
 * kept for the next ones, since an insertion frees a leaf at the back of a
 * middle block and takes one at the front of the next.
 *
 * The contracts [see `Contract.h`]: a rank is below `size()` (at most
 * `size()` for an insertion), and `pop_back()` needs a list that is not
 * empty; the invariant is that the middle blocks but the last hold
 * \f$ b^2 \f$ items and count `size()` items, each with its gap below
 * \f$ b \f$ and the leaves its items need. The operations are defined in
 * `CubishArrayStack.tpp`.
 *
 * \see M. T. Goodrich, J. G. Kloss II, Tiered vectors: efficient dynamic
 *      arrays for rank-based sequences, Algorithms and Data Structures
 *      (WADS 1999), Lecture Notes in Computer Science 1663, 205-216, 1999,
 *      doi:10.1007/3-540-48447-7_21
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercise 2.12
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

#ifndef __ADS_CubishArrayStack
 #define __ADS_CubishArrayStack
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <bit>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS CubishArrayStack -------------------------*/
/*--------------------------------------------------------------------------*/

/// a LIST by rank in three levels, \f$ O( 1 ) \f$ access and
/// \f$ O( n^{1/3} ) \f$ amortized insertions and removals
template< class T >
class CubishArrayStack
{

/*--------------------------------------------------------------------------*/
 public:
 using value_type = T;                    ///< the type of the items
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the type of the distances
 using reference = T &;                   ///< a reference to an item
 using const_reference = const T &;       ///< a constant reference
 /// the iterator on the items
 using iterator = RankIterator< CubishArrayStack , T , false >;
 /// the iterator on the constant items
 using const_iterator = RankIterator< CubishArrayStack , T , true >;

 /// the empty list
 CubishArrayStack( void ) = default;

 /// the items of `l`, in order
 CubishArrayStack( std::initializer_list< T > l );

 /// a copy of `o`
 CubishArrayStack( const CubishArrayStack & o );

 /// the blocks of `o`, which is left empty
 CubishArrayStack( CubishArrayStack && o ) noexcept;

 /// the items of `o`, by copy and swap
 CubishArrayStack & operator=( CubishArrayStack o ) noexcept;

 /// exchanges the blocks with those of `o`
 void swap( CubishArrayStack & o ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the places \f$ b \f$ of a leaf
 [[nodiscard]] std::size_t block( void ) const { return( f_b ); }

 /// the places of the leaves, those kept for later included
 [[nodiscard]] std::size_t capacity( void ) const;

 /// the item of rank \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < f_n );
  return( at( f_mids[ i >> ( 2 * f_lg ) ] , i & ( f_b * f_b - 1 ) ) );
  }

 /// the item of rank \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < f_n );
  return( at( f_mids[ i >> ( 2 * f_lg ) ] , i & ( f_b * f_b - 1 ) ) );
  }

 /// puts \f$ x \f$ at the rank \f$ i \f$, in \f$ O( n^{1/3} ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Morin, exercise 2.12), \f$ M_0 , \ldots , M_t \f$ the
 /// middle blocks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ n + 1 > 8 b^3\ \textbf{then}\ \mathrm{rebuild}(
 ///     n + 1 ) \cr
 /// 3 & \quad \textbf{if}\ |M_t| = b^2\ \textbf{then}
 ///     \ \text{a new empty middle block } M_{t+1};\ t \gets t + 1 \cr
 /// 4 & \quad k \gets \lfloor i / b^2 \rfloor \cr
 /// 5 & \quad \textbf{for}\ j \gets t\ \textbf{downto}\ k + 1\ \textbf{do}
 ///     \ M_j.\mathrm{pushFront}( M_{j-1}.\mathrm{popBack}() ) \cr
 /// 6 & \quad M_k.\mathrm{insert}( i \bmod b^2 , x )\quad \triangleright
 ///     \ \text{as } \mathrm{insert}() \text{ of a tiered vector} \cr
 /// 7 & \quad n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void insert( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$, in \f$ O( n^{1/3} ) \f$ amortized
 /// time
 ///
 /// In pseudocode (Morin, exercise 2.12):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad k \gets \lfloor i / b^2 \rfloor;\ M_k.\mathrm{erase}( i \bmod
 ///     b^2 ) \cr
 /// 3 & \quad \textbf{for}\ j \gets k + 1\ \textbf{to}\ t\ \textbf{do}
 ///     \ M_{j-1}.\mathrm{pushBack}( M_j.\mathrm{popFront}() ) \cr
 /// 4 & \quad \textbf{if}\ M_t = \emptyset\ \textbf{then}\ \text{remove }
 ///     M_t \cr
 /// 5 & \quad n \gets n - 1;\ \textbf{if}\ b > 16\ \textbf{and}\ 8 n < b^3
 ///     \ \textbf{then}\ \mathrm{rebuild}( n )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 /// \post one item less
 void erase( std::size_t i );

 /// \f$ x \f$ before the item of \f$ p \f$, i.e., `insert( i , x )` with
 /// \f$ i \f$ the rank of \f$ p \f$: the members by iterator of a
 /// SequenceContainer of the standard library, by which `std::flat_set` and
 /// `std::flat_map` keep their keys here; an iterator on \f$ x \f$
 /// \pre \f$ p \f$ is an iterator of this sequence or its end
 iterator insert( const_iterator p , const T & x ) {
  const std::size_t i = rankOf( p );
  insert( i , x );
  return( begin() + std::ptrdiff_t( i ) );
  }

 /// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$ before the item
 /// of \f$ p \f$, in their order, one `insert( i , x )` each; an iterator on
 /// the first of them
 template< std::input_iterator I , std::sentinel_for< I > E >
 iterator insert( const_iterator p , I first , E last ) {
  const std::size_t i0 = rankOf( p );
  for( std::size_t i = i0 ; first != last ; ++first )
   insert( i++ , T( *first ) );
  return( begin() + std::ptrdiff_t( i0 ) );
  }

 /// an item built from \f$ a \f$ before the item of \f$ p \f$
 template< class... A >
 iterator emplace( const_iterator p , A &&... a ) {
  return( insert( p , T( std::forward< A >( a )... ) ) );
  }

 /// the item of \f$ p \f$ removed, i.e., `erase( i )` with \f$ i \f$ its
 /// rank; an iterator on the item after it
 /// \pre \f$ p \f$ is an iterator on an item of this sequence
 iterator erase( const_iterator p ) {
  const std::size_t i = rankOf( p );
  erase( i );
  return( begin() + std::ptrdiff_t( i ) );
  }

 /// the items of \f$ [ \mathit{first} , \mathit{last} ) \f$ removed, one
 /// `erase( i )` each; an iterator on the item after them
 iterator erase( const_iterator first , const_iterator last ) {
  const std::size_t i = rankOf( first );
  for( std::size_t k = rankOf( last ) - i ; k > 0 ; --k )
   erase( i );
  return( begin() + std::ptrdiff_t( i ) );
  }

 /// \f$ x \f$ at the end, \f$ O( 1 ) \f$ amortized
 void push_back( const T & x ) { insert( f_n , x ); }

 /// removes the last item
 /// \pre the list is not empty
 void pop_back( void ) { erase( f_n - 1 ); }

 /// removes all the items and frees the leaves
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const CubishArrayStack & o ) const;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( iterator( this , 0 ) ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator( this , f_n ) ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:
 /// the rank of the item of \f$ p \f$
 std::size_t rankOf( const_iterator p ) const {
  return( std::size_t( p - begin() ) );
  }

 /// a leaf: \f$ b \f$ places, the first item at `off`
 struct Leaf
 {
  std::unique_ptr< T[] > a;  ///< the places
  std::size_t off = 0;       ///< the place of the first item
  };

 /// a middle block: a circular buffer of \f$ 2 b \f$ leaves
 struct Mid
 {
  std::unique_ptr< Leaf[] > L;  ///< the leaves, from `head`
  std::size_t head = 0;         ///< the place of the first leaf
  std::size_t nl = 0;           ///< the number of leaves
  std::size_t g = 0;            ///< the places free in the first leaf
  std::size_t n = 0;            ///< the number of items
  };

 /// the leaf \f$ j \f$ of \f$ M \f$
 Leaf & leaf( const Mid & M , std::size_t j ) const {
  return( M.L[ ( M.head + j ) & ( 2 * f_b - 1 ) ] );
  }

 /// the item of rank \f$ r \f$ of \f$ M \f$
 T & at( const Mid & M , std::size_t r ) const {
  const std::size_t v = M.g + r;
  const Leaf & f = leaf( M , v >> f_lg );
  return( f.a[ ( f.off + v ) & ( f_b - 1 ) ] );
  }

 /// a new middle block, with no leaf
 [[nodiscard]] Mid newMid( void ) const;

 /// the places of a leaf, kept or new
 std::unique_ptr< T[] > newLeaf( void );

 /// the places of a leaf kept for later, or freed
 void freeLeaf( std::unique_ptr< T[] > a );

 /// all the leaves of \f$ M \f$ freed
 void reset( Mid & M );

 /// \f$ x \f$ at the front of \f$ M \f$, \f$ O( 1 ) \f$
 void pushFront( Mid & M , T && x );

 /// \f$ x \f$ at the back of \f$ M \f$, \f$ O( 1 ) \f$
 void pushBack( Mid & M , T && x );

 /// the first item of \f$ M \f$, removed, \f$ O( 1 ) \f$
 T popFront( Mid & M );

 /// the last item of \f$ M \f$, removed, \f$ O( 1 ) \f$
 T popBack( Mid & M );

 /// \f$ x \f$ at the rank \f$ r \f$ of \f$ M \f$, the items after it one
 /// place on, \f$ O( b ) \f$
 /// \pre \f$ |M| < b^2 \f$
 void insert( Mid & M , std::size_t r , T && x );

 /// the item of rank \f$ r \f$ of \f$ M \f$ removed, \f$ O( b ) \f$
 void erase( Mid & M , std::size_t r );

 /// the leaves of size \f$ b \f$ for the size \f$ n \f$, the least power of
 /// 2 not below \f$ n^{1/3} \f$ and 16 at least
 void rebuild( std::size_t n );

 /// the middle blocks but the last full, and size() items in them
 [[nodiscard]] bool full( void ) const;

 std::vector< Mid > f_mids;                      ///< the middle blocks
 std::vector< std::unique_ptr< T[] > > f_spare;  ///< leaves kept
 std::size_t f_n = 0;                            ///< the number of items
 std::size_t f_b = 16;                           ///< the places of a leaf
 unsigned f_lg = 4;                              ///< \f$ \log_2 b \f$

 };  // end( class CubishArrayStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/CubishArrayStack.tpp"

/*--------------------------------------------------------------------------*/

#endif /* CubishArrayStack.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File CubishArrayStack.h -----------------------*/
/*--------------------------------------------------------------------------*/
