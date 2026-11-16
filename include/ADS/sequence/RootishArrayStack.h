/*--------------------------------------------------------------------------*/
/*------------------------ File RootishArrayStack.h ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The rootish array stack of Brodnik, Carlsson, Demaine, Munro and
 * Sedgewick, beyond the book: a LIST by rank, `operator[]`, `insert( i , x )`
 * and `erase( i )` (section 2.6 of Morin's book), whose items lie in
 * \f$ r \f$ blocks of \f$ 1 , 2 , \ldots , r \f$ places, so that a vector
 * that grows never copies its items and wastes only \f$ O( \sqrt{n} ) \f$
 * places, where a vector that doubles [see `ArraySequence.h`] wastes up to
 * \f$ 2 n \f$ of them and copies each item once on average.
 *
 * The item of rank \f$ i \f$ is in the block \f$ b \f$, the least with
 * \f$ ( b + 1 ) ( b + 2 ) / 2 > i \f$, i.e.,
 * \f[
 *   b( i ) = \Bigl\lceil \frac{ -3 + \sqrt{ 9 + 8 i } }{ 2 } \Bigr\rceil
 *          \; , \tag{1}
 * \f]
 * at the place \f$ i - b ( b + 1 ) / 2 \f$ of it; (1) is computed with the
 * square root of a `double`, exact on the integers below \f$ 2^{52} \f$, and
 * a step of correction, cheaper than the tables of section 2.6.4 of Morin's
 * book on a processor with a unit for the floating point. A block of
 * \f$ r + 1 \f$ places is added when the \f$ r ( r + 1 ) / 2 \f$ places are
 * full, and the last block is freed while
 * \f$ ( r - 2 ) ( r - 1 ) / 2 \ge n \f$, so that one empty block at most is
 * kept for the next insertions: an insertion or a removal at the rank
 * \f$ i \f$ moves \f$ n - i \f$ items, plus \f$ O( 1 ) \f$ amortized for the
 * blocks, and the places not used are at most
 * \f$ 2 r - 1 \le \sqrt{ 8 n } + 3 \f$ (section 2.6.2 of Morin's book), the
 * pointers to the blocks \f$ r \f$ more. The places are raw memory, the
 * items being built in them when inserted and destroyed when removed.
 *
 * Against Morin's code, the claims for the benchmarks: the blocks are raw
 * memory, so that `push_back()` builds one item where Morin's blocks of
 * `new T[ b + 1 ]` build \f$ b + 1 \f$ by default and assign them later;
 * the shifts move the items instead of copying them; `operator[]` costs the
 * same square root plus two comparisons, predicted, of the correction.
 *
 * The blocks and the vector of their addresses come from the allocator `A`,
 * `std::allocator` by default, with the rules of the standard [see
 * `AllocatorAware.h`].
 *
 * The contracts [see `Contract.h`]: a rank is below `size()` (at most
 * `size()` for an insertion), and `pop_back()` needs a list that is not
 * empty; the invariant is that the \f$ r \f$ blocks hold the \f$ n \f$
 * items with one block to spare at most,
 * \f$ ( r - 2 ) ( r - 1 ) / 2 < n \le r ( r + 1 ) / 2 \f$, and that the
 * empty list has no block. The operations are defined in
 * `RootishArrayStack.tpp`.
 *
 * \see A. Brodnik, S. Carlsson, E. D. Demaine, J. I. Munro, R. Sedgewick,
 *      Resizable arrays in optimal time and space, Algorithms and Data
 *      Structures (WADS 1999), Lecture Notes in Computer Science 1663,
 *      37-48, 1999, doi:10.1007/3-540-48447-7_4
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 2.6
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

#ifndef __ADS_RootishArrayStack
 #define __ADS_RootishArrayStack
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/AllocatorAware.h"
#include "ADS/Iterators.h"

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS RootishArrayStack -------------------------*/
/*--------------------------------------------------------------------------*/

/// a LIST by rank on blocks of \f$ 1 , 2 , \ldots , r \f$ places, with
/// \f$ O( \sqrt{n} ) \f$ places wasted
template< class T , class A = std::allocator< T > >
class RootishArrayStack
{

/*--------------------------------------------------------------------------*/
 public:
 using value_type = T;                    ///< the type of the items
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the type of the distances
 using reference = T &;                   ///< a reference to an item
 using const_reference = const T &;       ///< a constant reference
 /// the iterator on the items
 using iterator = RankIterator< RootishArrayStack , T , false >;
 /// the iterator on the constant items
 using const_iterator = RankIterator< RootishArrayStack , T , true >;
 /// the type of the allocator
 using allocator_type = A;
 /// the allocator of the blocks
 [[nodiscard]] allocator_type get_allocator( void ) const {
  return( allocator_type( f_blocks.get_allocator() ) );
  }

 /// the empty list, with no block
 RootishArrayStack( void ) = default;

 /// the empty sequence, its blocks from `alloc`
 explicit RootishArrayStack( const A & alloc ) : f_blocks( alloc ) { }

 /// the items of `l`, in order
 RootishArrayStack( std::initializer_list< T > l );

 /// a copy of `o`, on the allocator that
 /// `select_on_container_copy_construction()` gives
 RootishArrayStack( const RootishArrayStack & o );

 /// a copy of `o`, on `alloc`
 RootishArrayStack( const RootishArrayStack & o , const A & alloc );

 /// the blocks of `o`, which is left empty
 RootishArrayStack( RootishArrayStack && o ) noexcept;

 /// the items of `o`, which is left empty, on `alloc`: its blocks if the
 /// allocators are equal, else its items moved one by one
 RootishArrayStack( RootishArrayStack && o , const A & alloc );

 /// the items of `o`, by copy and swap, when the allocators are all equal
 RootishArrayStack & operator=( RootishArrayStack o ) noexcept
  requires detail::alwaysEqualAlloc< A >;

 /// the copy assignment, with the allocator of `o` if it propagates
 RootishArrayStack & operator=( const RootishArrayStack & o )
  requires( ! detail::alwaysEqualAlloc< A > )
 {
  return( detail::copyAssign( *this , o ) );
  }

 /// the move assignment: the memory of `o` if its allocator propagates or
 /// is equal, else its items moved one by one
 RootishArrayStack & operator=( RootishArrayStack && o )
  requires( ! detail::alwaysEqualAlloc< A > )
 {
  return( detail::moveAssign( *this , std::move( o ) ) );
  }

 /// destroys the items and frees the blocks
 ~RootishArrayStack();

 /// exchanges the blocks with those of `o`, and the allocators if they
 /// propagate on a swap
 /// \pre the allocators are equal, if they do not propagate
 void swap( RootishArrayStack & o ) noexcept;

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the places of the \f$ r \f$ blocks, \f$ r ( r + 1 ) / 2 \f$
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_blocks.size() * ( f_blocks.size() + 1 ) / 2 );
  }

 /// the number \f$ r \f$ of blocks
 [[nodiscard]] std::size_t blocks( void ) const { return( f_blocks.size() ); }

 /// the block of the rank \f$ i \f$, (1)
 ///
 /// In pseudocode, the root in floating point corrected by two loops:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{block}( i ) \cr
 /// 2 & \quad b \gets \lceil ( -3 + \sqrt{ 9 + 8 i } ) / 2 \rceil
 ///     \quad \triangleright\ \text{in } \mathtt{double} \cr
 /// 3 & \quad \textbf{while}\ b > 0\ \textbf{and}\ b ( b + 1 ) / 2 > i
 ///     \ \textbf{do}\ b \gets b - 1 \cr
 /// 4 & \quad \textbf{while}\ ( b + 1 ) ( b + 2 ) / 2 \le i
 ///     \ \textbf{do}\ b \gets b + 1 \cr
 /// 5 & \quad \textbf{return}\ b
 /// \end{array}}
 /// \f]
 [[nodiscard]] static std::size_t block( std::size_t i );

 /// the item of rank \f$ i \f$, in the block \f$ b( i ) \f$ of (1)
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i );

 /// the item of rank \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// the first item
 /// \pre the list is not empty
 [[nodiscard]] const T & front( void ) const;

 /// the last item
 /// \pre the list is not empty
 [[nodiscard]] const T & back( void ) const;

 /// puts \f$ x \f$ at the rank \f$ i \f$, the items from \f$ i \f$ one rank
 /// on, in \f$ O( 1 + n - i ) \f$ amortized time
 ///
 /// In pseudocode (Morin, section 2.6), \f$ B \f$ the blocks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ r ( r + 1 ) / 2 = n\ \textbf{then}
 ///     \ \text{a new block } B_r \text{ of } r + 1 \text{ places};
 ///     \ r \gets r + 1 \cr
 /// 3 & \quad \textbf{for}\ k \gets n\ \textbf{downto}\ i + 1\ \textbf{do}
 ///     \ \mathrm{set}( k , \mathrm{get}( k - 1 ) ) \cr
 /// 4 & \quad \mathrm{set}( i , x );\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void insert( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$, those after it one rank back, in
 /// \f$ O( 1 + n - i ) \f$ amortized time
 ///
 /// In pseudocode (Morin, section 2.6):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad \textbf{for}\ k \gets i\ \textbf{to}\ n - 2\ \textbf{do}
 ///     \ \mathrm{set}( k , \mathrm{get}( k + 1 ) ) \cr
 /// 3 & \quad n \gets n - 1 \cr
 /// 4 & \quad \textbf{while}\ r > 0\ \textbf{and}\ ( r - 2 ) ( r - 1 ) / 2
 ///     \ge n\ \textbf{do}\ \text{free } B_{r-1};\ r \gets r - 1
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
 template< class... Args >
 iterator emplace( const_iterator p , Args &&... a ) {
  return( insert( p , T( std::forward< Args >( a )... ) ) );
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

 /// \f$ x \f$ at the end, `insert( size() , x )`
 void push_back( const T & x ) { emplace_back( x ); }

 /// \f$ x \f$ at the end, moved
 void push_back( T && x ) { emplace_back( std::move( x ) ); }

 /// the item built from `args` at the end, \f$ O( 1 ) \f$ amortized
 template< class... Args >
 T & emplace_back( Args &&... args );

 /// removes the last item
 /// \pre the list is not empty
 void pop_back( void ) { erase( f_n - 1 ); }

 /// removes all the items and frees the blocks
 void clear( void );

 /// the same items in the same order
 [[nodiscard]] bool operator==( const RootishArrayStack & o ) const;

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

 /// the address of the place of rank \f$ i \f$, built or not
 [[nodiscard]] T * place( std::size_t i ) const;

 /// a block of \f$ r + 1 \f$ places at the end
 void grow( void );

 /// the last blocks freed while \f$ ( r - 2 ) ( r - 1 ) / 2 \ge n \f$
 void shrink( void );

 /// the blocks hold the items, as the invariant says
 [[nodiscard]] bool fits( void ) const;

 /// the allocator of the items, and its traits
 using Alloc = detail::RebindAlloc< A , T >;
 using Traits = std::allocator_traits< Alloc >;

 /// the block \f$ b \f$ of \f$ b + 1 \f$ places, for each \f$ b \f$
 detail::AllocVector< A , T * > f_blocks;
 std::size_t f_n = 0;  ///< the number of items

 };  // end( class RootishArrayStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/RootishArrayStack.tpp"

/*--------------------------------------------------------------------------*/

#endif /* RootishArrayStack.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File RootishArrayStack.h ----------------------*/
/*--------------------------------------------------------------------------*/
