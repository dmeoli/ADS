/*--------------------------------------------------------------------------*/
/*--------------------------- File TieredVector.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tiered vector of Goodrich and Kloss, beyond the book: a sequence with
 * access by index in \f$ O( 1 ) \f$ time, as a vector, and insertion and
 * removal at any index in \f$ O( \sqrt{n} ) \f$ time, where a vector [see
 * `ArraySequence.h`] moves \f$ O( n ) \f$ items.
 *
 * The items are in blocks of \f$ b \f$ items, \f$ b \f$ a power of 2, each a
 * circular buffer with the offset of its first item, all full but the first
 * and the last one, the first with \f$ g < b \f$ places free at its front;
 * the item of index \f$ i \f$ is the item \f$ ( g + i ) \bmod b \f$ of the
 * block \f$ \lfloor ( g + i ) / b \rfloor \f$, an addition, a shift, a mask
 * and two reads. An
 * insertion at \f$ i \f$ moves the items of its block after \f$ i \f$ by one
 * place, at most \f$ b - 1 \f$ of them, and then the last item of each
 * following block to the front of the next one, \f$ O( 1 ) \f$ per block
 * since a circular buffer grows at the front by moving its offset:
 * \f$ O( b + n / b ) \f$ moves. A removal is the symmetric. \f$ b \f$
 * follows the size, the least power of 2 not below \f$ \sqrt{n} \f$ and 64
 * at least: when \f$ n \f$ leaves \f$ [ b^2 / 4 , 4 b^2 ] \f$ the items are
 * copied into the blocks of the new \f$ b \f$, in \f$ O( n ) \f$ time once
 * every \f$ \Omega( n ) \f$ operations, so that an insertion or a removal
 * costs \f$ O( \sqrt{n} ) \f$ amortized and the space is
 * \f$ n + O( \sqrt{n} ) \f$ items.
 *
 * An insertion or a removal at \f$ i < n / 2 \f$ moves the items before
 * \f$ i \f$ instead, toward the front: the items of its block before
 * \f$ i \f$, and the first item of each block before it to the back of the
 * block that precedes it, which costs \f$ O( 1 ) \f$ per block as above;
 * the first block gives or takes a place of its gap \f$ g \f$, and a block
 * is added at the front when \f$ g = 0 \f$ (removed when \f$ g = b \f$),
 * once every \f$ b \f$ insertions at most. Hence an operation at
 * \f$ i \f$ moves \f$ O( 1 + \min\{ i , b , n - i \} + \min\{ i , n - i \}
 * / b ) \f$ items, i.e., \f$ O( 1 + \min\{ i , \sqrt{n} , n - i \} ) \f$
 * amortized, which is the bound of exercise 2.11 of Morin's book (that of
 * exercise 2.10, \f$ O( 1 + \min\{ \sqrt{n} , n - i \} ) \f$, holds already
 * for the shifts toward the back), with \f$ O( \sqrt{n} ) \f$ places wasted:
 * the gap, the free places of the last block and the directory of the
 * blocks.
 *
 * The contracts [see `Contract.h`]: an index is below `size()` (at most
 * `size()` for an insertion), and `pop_back()` needs a vector that is not
 * empty, as `pop_front()`; the invariant is that the blocks but the first
 * and the last are full, that the gap is below \f$ b \f$ and that they
 * count `size()` items. The operations are defined in `TieredVector.tpp`.
 *
 * \see M. T. Goodrich, J. G. Kloss II, Tiered vectors: efficient dynamic
 *      arrays for rank-based sequences, Algorithms and Data Structures
 *      (WADS 1999), Lecture Notes in Computer Science 1663, 205-216, 1999,
 *      doi:10.1007/3-540-48447-7_21
 * \see P. Bille, A. R. Christiansen, M. B. Ettienne, I. L. Gortz, Fast
 *      dynamic arrays, 25th Annual European Symposium on Algorithms (ESA
 *      2017), LIPIcs 87, 16:1-16:13, 2017,
 *      doi:10.4230/LIPIcs.ESA.2017.16
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, exercises 2.10 and 2.11
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

#ifndef __ADS_TieredVector
 #define __ADS_TieredVector
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <bit>
#include <cstddef>
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
/*-------------------------- CLASS TieredVector ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sequence with access in \f$ O( 1 ) \f$ and insertion and removal
/// anywhere in \f$ O( \sqrt{n} ) \f$, in circular blocks of \f$ b \f$ items
template< class T >
class TieredVector
{

/*--------------------------------------------------------------------------*/
 public:
 /// the type of the items
 using value_type = T;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = T &;
 /// a constant reference to an item
 using const_reference = const T &;

 /// the empty vector
 TieredVector( void ) = default;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number of items, \f$ n \f$
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the places \f$ b \f$ of a block
 [[nodiscard]] std::size_t block( void ) const { return( f_b ); }

 /// the places of the blocks, \f$ n + O( \sqrt{n} ) \f$
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_blocks.size() * f_b );
  }

 /// the item at the index \f$ i \f$, at the place
 /// \f$ ( \mathit{off} + g + i ) \bmod b \f$ of the block
 /// \f$ \lfloor ( g + i ) / b \rfloor \f$, \f$ \mathit{off} \f$ the offset
 /// of the block
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < f_n );
  const std::size_t j = f_g + i;
  Block & k = f_blocks[ j >> f_lg ];
  return( k.a[ ( k.off + j ) & ( f_b - 1 ) ] );
  }
 /// the item at the index \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < f_n );
  const std::size_t j = f_g + i;
  const Block & k = f_blocks[ j >> f_lg ];
  return( k.a[ ( k.off + j ) & ( f_b - 1 ) ] );
  }

 /// the first item
 /// \pre the vector is not empty
 [[nodiscard]] const T & front( void ) const { return( ( *this )[ 0 ] ); }

 /// the last item
 /// \pre the vector is not empty
 [[nodiscard]] const T & back( void ) const {
  return( ( *this )[ f_n - 1 ] );
  }

 /// \f$ x \f$ at the index \f$ i \f$, the items from \f$ i \f$ one place on,
 /// or those before \f$ i \f$ one place back if \f$ i < n / 2 \f$, in
 /// \f$ O( 1 + \min\{ i , \sqrt{n} , n - i \} ) \f$ amortized time
 ///
 /// In pseudocode (Goodrich and Kloss, beyond the book), \f$ B_j \f$ the
 /// blocks, \f$ B_j.\mathit{off} \f$ their offsets, the places of a block
 /// taken modulo \f$ b \f$ and the gap \f$ g = 0 \f$ (the shift toward the
 /// front is the mirror, the first item of each block to the back of the
 /// one before, with the gap in place of the free places of the last
 /// block):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ n + 1 > 4 b^2\ \textbf{then}\ \mathrm{rebuild}( n
 ///     + 1 )\quad \triangleright\ b \text{ the least power of 2 with } b^2
 ///     \ge n + 1 \cr
 /// 3 & \quad \textbf{if}\ \text{all the blocks are full}\ \textbf{then}
 ///     \ \text{add an empty block at the end} \cr
 /// 4 & \quad k \gets \lfloor i / b \rfloor;\ L \gets \lfloor n / b
 ///     \rfloor \cr
 /// 5 & \quad \textbf{for}\ j \gets L\ \textbf{downto}\ k + 1\ \textbf{do}
 ///     \quad \triangleright\ \text{the last item of } B_{j-1} \text{ to the }
 ///     \text{front of } B_j \cr
 /// 6 & \qquad B_j.\mathit{off} \gets B_j.\mathit{off} - 1;\ B_j[
 ///     B_j.\mathit{off} ] \gets B_{j-1}[ B_{j-1}.\mathit{off} + b - 1 ] \cr
 /// 7 & \quad \text{move the items of } B_k \text{ from the place } i \bmod b
 ///     \text{ one place on} \cr
 /// 8 & \quad B_k[ B_k.\mathit{off} + i \bmod b ] \gets x;\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 void insert( std::size_t i , const T & x );

 /// removes the item at the index \f$ i \f$, the items after it one place
 /// back, or those before it one place on if \f$ i < n / 2 \f$, in
 /// \f$ O( 1 + \min\{ i , \sqrt{n} , n - i \} ) \f$ amortized time
 ///
 /// In pseudocode (Goodrich and Kloss, beyond the book), the converse of
 /// `insert()` with \f$ g = 0 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad k \gets \lfloor i / b \rfloor;\ L \gets \lfloor ( n - 1 ) / b
 ///     \rfloor \cr
 /// 3 & \quad \text{move the items of } B_k \text{ after the place }
 ///     i \bmod b \text{ one place back} \cr
 /// 4 & \quad \textbf{for}\ j \gets k + 1\ \textbf{to}\ L\ \textbf{do}\quad
 ///     \triangleright\ \text{the first item of } B_j \text{ to the back }
 ///     \text{of } B_{j-1} \cr
 /// 5 & \qquad B_{j-1}[ B_{j-1}.\mathit{off} + b - 1 ] \gets B_j[
 ///     B_j.\mathit{off} ];\ B_j.\mathit{off} \gets B_j.\mathit{off} + 1 \cr
 /// 6 & \quad n \gets n - 1 \cr
 /// 7 & \quad \textbf{if}\ \text{the last block is empty}\ \textbf{then}
 ///     \ \text{remove it} \cr
 /// 8 & \quad \textbf{if}\ b > 64\ \textbf{and}\ 4 n < b^2\ \textbf{then}
 ///     \ \mathrm{rebuild}( n )
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 void erase( std::size_t i );

 /// \f$ x \f$ at the end, `insert( n , x )`
 void push_back( const T & x ) { insert( f_n , x ); }

 /// removes the last item, `erase( n - 1 )`
 /// \pre the vector is not empty
 void pop_back( void ) { erase( f_n - 1 ); }

 /// \f$ x \f$ at the front, `insert( 0 , x )`, \f$ O( 1 ) \f$ amortized
 void push_front( const T & x ) { insert( 0 , x ); }

 /// removes the first item, `erase( 0 )`, \f$ O( 1 ) \f$ amortized
 /// \pre the vector is not empty
 void pop_front( void ) { erase( 0 ); }

 /// removes the items
 void clear( void );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the iterator by index, random access, on items const if `Const`
 template< bool Const >
 class Iterator
 {
  using Owner = std::conditional_t< Const , const TieredVector ,
				    TieredVector >;

  public:
  /// the category of the iterator, random access
  using iterator_category = std::random_access_iterator_tag;
  /// the type of the items
  using value_type = T;
  /// the type of the distances
  using difference_type = std::ptrdiff_t;
  /// a reference to an item, constant if `Const`
  using reference = std::conditional_t< Const , const T & , T & >;
  /// a pointer to an item, constant if `Const`
  using pointer = std::conditional_t< Const , const T * , T * >;

  /// an iterator on no vector
  Iterator( void ) = default;

  /// on the item of index \f$ i \f$ of the vector \f$ v \f$
  Iterator( Owner * v , std::size_t i ) : f_v( v ) , f_i( i ) { }

  /// the const iterator from the other one
  template< bool K >
   requires( Const && ! K )
  Iterator( const Iterator< K > & o ) : f_v( o.f_v ) , f_i( o.f_i ) { }

  /// the item
  reference operator*( void ) const { return( ( *f_v )[ f_i ] ); }
  /// the address of the item
  pointer operator->( void ) const { return( &**this ); }
  /// the item \f$ k \f$ places on
  reference operator[]( difference_type k ) const {
   return( ( *f_v )[ std::size_t( difference_type( f_i ) + k ) ] );
   }
  /// to the next item
  Iterator & operator++( void ) {
   ++f_i;
   return( *this );
   }
  /// to the previous item
  Iterator & operator--( void ) {
   --f_i;
   return( *this );
   }
  /// to the next item, returning the iterator before the step
  Iterator operator++( int ) {
   auto t = *this;
   ++f_i;
   return( t );
   }
  /// to the previous item, returning the iterator before the step
  Iterator operator--( int ) {
   auto t = *this;
   --f_i;
   return( t );
   }
  /// \f$ k \f$ items on
  Iterator & operator+=( difference_type k ) {
   f_i = std::size_t( difference_type( f_i ) + k );
   return( *this );
   }
  /// \f$ k \f$ items back
  Iterator & operator-=( difference_type k ) { return( *this += -k ); }
  friend Iterator operator+( Iterator a , difference_type k ) {
   return( a += k );
   }
  friend Iterator operator+( difference_type k , Iterator a ) {
   return( a += k );
   }
  friend Iterator operator-( Iterator a , difference_type k ) {
   return( a -= k );
   }
  friend difference_type operator-( const Iterator & a , const Iterator & b ) {
   return( difference_type( a.f_i ) - difference_type( b.f_i ) );
   }
  /// equal on the same index
  bool operator==( const Iterator & o ) const { return( f_i == o.f_i ); }
  /// the order of the indices
  auto operator<=>( const Iterator & o ) const { return( f_i <=> o.f_i ); }

  private:
  Owner * f_v = nullptr;
  std::size_t f_i = 0;

  template< bool >
  friend class Iterator;
  };

 /// the iterator on the items
 using iterator = Iterator< false >;
 /// the iterator on the constant items
 using const_iterator = Iterator< true >;

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
 /// a block: \f$ b \f$ places, the first item at `off`
 struct Block
 {
  std::unique_ptr< T[] > a;  ///< the places
  std::size_t off = 0;       ///< the place of the first item
  };

 /// the blocks for the size \f$ n \f$, \f$ b \f$ the least power of 2 not
 /// below
 /// \f$ \sqrt{n} \f$
 void rebuild( std::size_t n );

 /// \f$ x \f$ at the index \f$ i \f$, the items before \f$ i \f$ one place
 /// back
 ///
 /// In pseudocode, \f$ w \f$ the place of \f$ x \f$ counted from the
 /// first place of \f$ B_0 \f$, the gap included:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insertFront}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ g = 0\ \textbf{then}\ \text{add an empty}
 ///     \text{ block at the front};\ g \gets b \cr
 /// 3 & \quad w \gets g + i - 1;\ k \gets \lfloor w / b \rfloor \cr
 /// 4 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ k - 1\ \textbf{do}
 ///     \quad \triangleright\ \text{the first item of } B_{j+1}
 ///     \text{ to the back of } B_j \cr
 /// 5 & \qquad B_j.\mathit{off} \gets B_j.\mathit{off} + 1;\ B_j[
 ///     B_j.\mathit{off} + b - 1 ] \gets B_{j+1}[ B_{j+1}.\mathit{off} ]
 ///     \cr
 /// 6 & \quad s \gets g \text{ if } k = 0 \text{, and 1 otherwise};
 ///     \ \text{move the items of } B_k \text{ at the places } s ..
 ///     w \bmod b \text{ one place back} \cr
 /// 7 & \quad B_k[ B_k.\mathit{off} + w \bmod b ] \gets x;\ g \gets g -
 ///     1;\ n \gets n + 1
 /// \end{array}}
 /// \f]
 void insertFront( std::size_t i , T && x );

 /// removes the item at the index \f$ i \f$, the items before it one place
 /// on
 void eraseFront( std::size_t i );

 /// removes the item at the index \f$ i \f$, the items after it one place
 /// back
 void eraseBack( std::size_t i );

 /// the blocks but the first and the last full, the gap below \f$ b \f$,
 /// and size() items in them
 [[nodiscard]] bool full( void ) const;

 std::vector< Block > f_blocks;  ///< the blocks
 std::size_t f_n = 0;            ///< the number of items
 std::size_t f_g = 0;            ///< the places free before the first item
 std::size_t f_b = 64;           ///< the places of a block
 unsigned f_lg = 6;              ///< \f$ \log_2 b \f$

 };  // end( class TieredVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/TieredVector.tpp"

/*--------------------------------------------------------------------------*/

#endif /* TieredVector.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File TieredVector.h -------------------------*/
/*--------------------------------------------------------------------------*/
