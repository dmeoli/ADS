/*--------------------------------------------------------------------------*/
/*------------------------- File DualArrayDeque.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The dual array deque of section 2.5 of Morin's book, beyond the book: a
 * LIST by rank made of two stacks back to back, \f$ F \f$ with the first
 * items from the top and \f$ B \f$ with the others from the bottom, so that
 * the item of rank \f$ i \f$ is \f$ F[ |F| - 1 - i ] \f$ for \f$ i < |F| \f$
 * and \f$ B[ i - |F| ] \f$ otherwise. An insertion or a removal at the rank
 * \f$ i \f$ moves the items between \f$ i \f$ and the top of its stack,
 * which are \f$ O( 1 + \min\{ i , n - i \} ) \f$ while the two stacks are
 * balanced, \f$ 3 |F| \ge |B| \f$ and \f$ 3 |B| \ge |F| \f$; when an
 * operation breaks the balance, the items are dealt again into two stacks
 * of \f$ \lfloor n / 2 \rfloor \f$ and \f$ \lceil n / 2 \rceil \f$ items, in
 * \f$ O( n ) \f$ time, which the \f$ \Omega( n ) \f$ operations since the
 * last balance pay for (section 2.5.1 of Morin's book).
 *
 * The stacks are of the type `S`, a vector by default; with
 * `S = RootishArrayStack< T >` [see `RootishArrayStack.h`] the deque wastes
 * \f$ O( \sqrt{n} ) \f$ places outside a balance and inserts and removes
 * at the rank \f$ i \f$ in \f$ O( 1 + \min\{ i , n - i \} ) \f$ amortized
 * time (exercise 2.9). A balance is done in place: the \f$ t \f$ items at
 * the bottom of the larger stack go to the bottom of the other, which grows
 * by \f$ t \le \lceil n / 2 \rceil \f$ places (fewer than \f$ n / 2 \f$
 * unless it was empty), before the larger one shrinks, i.e., the
 * places of \f$ n + t \f$ items at the peak instead of those of
 * \f$ 2 n \f$, and \f$ n \f$ moves (see \ref theory_sequence_ods for why it
 * is not spread over the operations). A stack `S` has `size()`, `operator[]`,
 * `push_back()`, `pop_back()` and either the `insert()` and `erase()` of a
 * rank or those of an iterator, as `std::vector`.
 *
 * `rotate( r )` moves the item of rank \f$ i \f$ to the rank
 * \f$ ( i + r ) \bmod n \f$ by \f$ \min\{ r , n - r \} \f$ moves from one end
 * to the other, in \f$ O( 1 + \min\{ r , n - r \} ) \f$ amortized time
 * (exercise 2.5).
 *
 * Against Morin's code, the claims for the benchmarks: the stacks are
 * `std::vector`, whose shifts are one `memmove()` for the items trivially
 * copyable and whose growth moves the items, where Morin's `ArrayStack`
 * copies them one by one; a balance moves each item once, in place.
 *
 * The contracts [see `Contract.h`]: a rank is below `size()` (at most
 * `size()` for an insertion), and the removals at the ends need a deque
 * that is not empty; the invariant is the balance of the two stacks, for
 * \f$ n \ge 2 \f$. The operations are defined in `DualArrayDeque.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 2.5 and exercises
 *      2.5 and 2.9
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

#ifndef __ADS_DualArrayDeque
 #define __ADS_DualArrayDeque
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS DualArrayDeque --------------------------*/
/*--------------------------------------------------------------------------*/

/// a LIST by rank on two stacks `S` back to back, balanced
template< class T , class S = std::vector< T > >
class DualArrayDeque
{

/*--------------------------------------------------------------------------*/
 public:
 using value_type = T;                    ///< the type of the items
 using size_type = std::size_t;           ///< the type of the sizes
 using difference_type = std::ptrdiff_t;  ///< the type of the distances
 using reference = T &;                   ///< a reference to an item
 using const_reference = const T &;       ///< a constant reference
 /// the iterator on the items
 using iterator = RankIterator< DualArrayDeque , T , false >;
 /// the iterator on the constant items
 using const_iterator = RankIterator< DualArrayDeque , T , true >;

 /// the empty deque
 DualArrayDeque( void ) = default;

 /// the items of `l`, in order
 DualArrayDeque( std::initializer_list< T > l );

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const {
  return( ( f_F.size() == 0 ) && ( f_B.size() == 0 ) );
  }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const {
  return( f_F.size() + f_B.size() );
  }

 /// the item of rank \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i );

 /// the item of rank \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// the first item
 /// \pre the deque is not empty
 [[nodiscard]] const T & front( void ) const { return( ( *this )[ 0 ] ); }

 /// the last item
 /// \pre the deque is not empty
 [[nodiscard]] const T & back( void ) const {
  return( ( *this )[ size() - 1 ] );
  }

 /// puts \f$ x \f$ at the rank \f$ i \f$, in
 /// \f$ O( 1 + \min\{ i , n - i \} ) \f$ amortized time
 ///
 /// In pseudocode (Morin, section 2.5), \f$ F \f$ and \f$ B \f$ the stacks:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ i < |F|\ \textbf{then}\ F.\mathrm{insert}( |F| -
 ///     i , x ) \cr
 /// 3 & \quad \textbf{else}\ B.\mathrm{insert}( i - |F| , x ) \cr
 /// 4 & \quad \mathrm{balance}()
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void insert( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$, in
 /// \f$ O( 1 + \min\{ i , n - i \} ) \f$ amortized time
 ///
 /// In pseudocode (Morin, section 2.5):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad \textbf{if}\ i < |F|\ \textbf{then}\ F.\mathrm{erase}( |F| -
 ///     1 - i ) \cr
 /// 3 & \quad \textbf{else}\ B.\mathrm{erase}( i - |F| ) \cr
 /// 4 & \quad \mathrm{balance}()
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

 /// \f$ x \f$ at the front, `insert( 0 , x )`
 void push_front( const T & x ) { insert( 0 , x ); }

 /// \f$ x \f$ at the back, `insert( size() , x )`
 void push_back( const T & x ) { insert( size() , x ); }

 /// removes the first item
 /// \pre the deque is not empty
 void pop_front( void ) { erase( 0 ); }

 /// removes the last item
 /// \pre the deque is not empty
 void pop_back( void ) { erase( size() - 1 ); }

 /// the item of rank \f$ i \f$ moved to the rank \f$ ( i + r ) \bmod n \f$,
 /// in \f$ O( 1 + \min\{ r , n - r \} ) \f$ amortized time (exercise 2.5 of
 /// Morin's book)
 ///
 /// In pseudocode (Morin, exercise 2.5), \f$ r \f$ reduced modulo
 /// \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{rotate}( r ) \cr
 /// 2 & \quad \textbf{if}\ r \le n - r\ \textbf{then}\ \textbf{repeat}
 ///     \ r\ \text{ times: }\mathrm{insert}( 0 , \mathrm{erase}( n - 1 ) )
 ///     \cr
 /// 3 & \quad \textbf{else}\ \textbf{repeat}\ n - r\ \text{ times: }
 ///     \mathrm{insert}( n - 1 , \mathrm{erase}( 0 ) )
 /// \end{array}}
 /// \f]
 void rotate( std::size_t r );

 /// removes all the items
 void clear( void );

 /// the number of items of the front stack \f$ F \f$
 [[nodiscard]] std::size_t frontSize( void ) const { return( f_F.size() ); }

 /// the same items in the same order
 [[nodiscard]] bool operator==( const DualArrayDeque & o ) const;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( iterator( this , 0 ) ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator( this , size() ) ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , size() ) );
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

 /// puts \f$ x \f$ at the place \f$ j \f$ of the stack \f$ s \f$
 static void put( S & s , std::size_t j , T && x );

 /// removes the place \f$ j \f$ of the stack \f$ s \f$
 static void take( S & s , std::size_t j );

 /// the items dealt again into two halves if a stack has more than three
 /// times the items of the other, in \f$ O( n ) \f$ time, in place: the
 /// \f$ t \f$ items at the bottom of the larger stack pass to the bottom
 /// of the other, by \f$ n \f$ moves
 ///
 /// In pseudocode (Morin, section 2.5, the deal in place beyond the book),
 /// \f$ G \f$ the larger stack, \f$ S \f$ the other one and
 /// \f$ t = \bigl| |F| - m \bigr| \f$, so that \f$ F \f$ is left with
 /// \f$ m = \lfloor n / 2 \rfloor \f$ items:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{balance}() \cr
 /// 2 & \quad \textbf{if}\ 3 |F| < |B|\ \textbf{or}\ 3 |B| < |F|
 ///     \ \textbf{then} \cr
 /// 3 & \qquad m \gets \lfloor n / 2 \rfloor;\ t \gets \bigl| |F| - m
 ///     \bigr| \cr
 /// 4 & \qquad \text{push } t \text{ places on } S;\ \textbf{for}\ j \gets
 ///     |S| - 1\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 5 & \qquad\quad \textbf{if}\ j \ge t\ \textbf{then}\ S[ j ] \gets
 ///     S[ j - t ]\ \textbf{else}\ S[ j ] \gets G[ t - 1 - j ] \cr
 /// 6 & \qquad \textbf{for}\ k \gets 0\ \textbf{to}\ |G| - t - 1
 ///     \ \textbf{do}\ G[ k ] \gets G[ k + t ] \cr
 /// 7 & \qquad \text{pop } t \text{ places from } G
 /// \end{array}}
 /// \f]
 void balance( void );

 /// the \f$ t \f$ items at the bottom of `big` moved to the bottom of
 /// `small` in the order of the ranks, by \f$ |\mathit{big}| +
 /// |\mathit{small}| \f$ moves and \f$ t \f$ places more on `small` at
 /// most
 static void deal( S & big , S & small , std::size_t t );

 /// the two stacks are balanced
 [[nodiscard]] bool balanced( void ) const;

 S f_F;  ///< the front stack, the item of rank 0 on top
 S f_B;  ///< the back stack, the last item on top

 };  // end( class DualArrayDeque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/DualArrayDeque.tpp"

/*--------------------------------------------------------------------------*/

#endif /* DualArrayDeque.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File DualArrayDeque.h ------------------------*/
/*--------------------------------------------------------------------------*/
