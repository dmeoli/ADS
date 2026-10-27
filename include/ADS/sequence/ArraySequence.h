/*--------------------------------------------------------------------------*/
/*-------------------------- File ArraySequence.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see `ADT.h`] realized with a vector: the position of an item is
 * its index, \f$ \mathit{pos}_i = i \f$, with \f$ \mathit{pos}_{-1} = -1 \f$
 * and \f$ \mathit{pos}_n = n \f$. Moving along the sequence, reading and
 * writing take \f$ O( 1 ) \f$ time, while `insert()` and `remove()` take
 * \f$ O( n ) \f$ time, since the items after the position move by one place;
 * inserting at the end takes \f$ O( 1 ) \f$ amortized time, since a full
 * vector is replaced by one of twice its size. If the \f$ i \f$-th insertion
 * costs \f$ c_i = i \f$ when it finds the vector full, i.e., when
 * \f$ i = 2^k + 1 \f$, and 1 otherwise, then \f$ n \f$ insertions cost
 * \f[
 *   C( n ) = \sum_{i=1}^n c_i \leq n + \sum_{k=0}^{\lfloor \log_2 n \rfloor}
 *            2^k \leq n + 2^{\lfloor \log_2 n \rfloor + 1} - 1 \leq 3 n \; ,
 * \f]
 * whereas growing the vector by a constant \f$ d \f$ makes them cost
 * \f$ \Theta( n^2 ) \f$. Symmetrically, a vector that `remove()` leaves with
 * \f$ n \leq c / 4 \f$, \f$ c \f$ its capacity, is replaced by one of half
 * its size, so that the space stays \f$ O( n ) \f$ and any sequence of
 * insertions and removals costs \f$ O( 1 ) \f$ amortized time each: after a
 * resize the vector is half full, and \f$ \Omega( c ) \f$ operations must
 * come before the next one. The empty sequence has no vector, also when
 * `remove()` empties it, the first insertion makes one of one place, and
 * moving a sequence takes its vector in \f$ O( 1 ) \f$ time, leaving the one
 * moved from empty.
 *
 * The contracts [see `Contract.h`]: a position is an index from -1 to
 * \f$ n \f$; `next()` needs one from -1 to \f$ n - 1 \f$ and `prev()` one
 * from 0 to \f$ n \f$, since there is nothing after \f$ \mathit{pos}_n \f$
 * nor before \f$ \mathit{pos}_{-1} \f$; `insert()` needs one from 0 to
 * \f$ n \f$, and `read()`, `write()` and `remove()` one that holds an item,
 * from 0 to \f$ n - 1 \f$. `insert()` and `write()` ensure that \f$ v \f$ is
 * read back in its place, and that the other items are where they were (or
 * one place on, after the insertion), the latter costing \f$ O( n ) \f$ and
 * therefore an audit; `remove()` likewise. The invariant is that the
 * \f$ n \f$ items fit in the vector, \f$ 0 \leq n \leq c \f$. The operations
 * are defined in `ArraySequence.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 4.2, 4.2.1
 *      and 4.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, section 17.4 (dynamic
 *      tables, the contraction at a quarter)
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

#ifndef __ADS_ArraySequence
 #define __ADS_ArraySequence
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS ArraySequence ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a SEQUENCE realized with a vector that doubles when it is full
template< class T >
class ArraySequence {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items
 using Pos = long; ///< a position is an index, from -1 to \f$ n \f$

 /// the empty sequence, with no vector
 ArraySequence( void );

 /// the sequence of the given items, in their order
 ArraySequence( std::initializer_list< T > items );

 /// the sequence of the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 ArraySequence( I first , S last );

 /// a copy of `other`
 ArraySequence( const ArraySequence & other );

 /// the items of `other`, which is left empty
 ArraySequence( ArraySequence && other ) noexcept;

 /// the items of `other`, by copy and swap
 ArraySequence & operator=( ArraySequence other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// true if there is no item, i.e., \f$ n = 0 \f$
 [[nodiscard]] bool empty( void ) const;

 /// true if \f$ p \f$ is -1 or \f$ n \f$
 [[nodiscard]] bool finished( Pos p ) const;

 /// the position of the first item, 0
 [[nodiscard]] Pos head( void ) const;

 /// the position of the last item, \f$ n - 1 \f$
 [[nodiscard]] Pos tail( void ) const;

 /// the position \f$ p + 1 \f$
 /// \pre \f$ -1 \le p < n \f$
 [[nodiscard]] Pos next( Pos p ) const;

 /// the position \f$ p - 1 \f$
 /// \pre \f$ 0 \le p \le n \f$
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts \f$ v \f$ at the index \f$ p \f$, the items from the index
 /// \f$ p \f$ moving one place on, which takes \f$ O( n - p ) \f$ time,
 /// \f$ O( 1 ) \f$ amortized at the end
 ///
 /// In pseudocode (sections 4.2 and 4.2.1), \f$ A[ 0 .. c - 1 ] \f$ the
 /// vector of capacity \f$ c \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{insert}( p , v ) \cr
 /// 2 & \quad \textbf{if}\ n = c\ \textbf{then}\ \mathrm{resize}( \max( 2 c ,
 ///     1 ) )\quad \triangleright\ \text{full: twice the size} \cr
 /// 3 & \quad \textbf{for}\ i \gets n\ \textbf{downto}\ p + 1\ \textbf{do}
 ///     \ A[ i ] \gets A[ i - 1 ] \cr
 /// 4 & \quad A[ p ] \gets v;\ n \gets n + 1 \cr
 /// 5 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le p \le n \f$
 /// \post one item more, \f$ v \f$ at the index \f$ p \f$, the others where
 ///       they were before \f$ p \f$ and one place on from \f$ p \f$ (an
 ///       audit)
 Pos insert( Pos p , const T & v );

 /// removes the item at the index \f$ p \f$, the following ones moving one
 /// place back; a vector left a quarter full is halved
 ///
 /// In pseudocode (sections 4.2 and 4.2.1), the halving at a quarter keeping
 /// \f$ O( 1 ) \f$ amortized time per operation:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{remove}( p ) \cr
 /// 2 & \quad \textbf{for}\ i \gets p\ \textbf{to}\ n - 2\ \textbf{do}
 ///     \ A[ i ] \gets A[ i + 1 ] \cr
 /// 3 & \quad n \gets n - 1 \cr
 /// 4 & \quad \textbf{if}\ n \le c / 4\ \textbf{then}\ \mathrm{resize}(
 ///     c / 2 )\quad \triangleright\ \text{no vector if } n = 0 \cr
 /// 5 & \quad \textbf{return}\ p
 /// \end{array}}
 /// \f]
 /// \pre \f$ 0 \le p < n \f$
 /// \post one item less, the others where they were before \f$ p \f$ and one
 ///       place back after it (an audit)
 Pos remove( Pos p );

 /// the item at the index \f$ p \f$
 /// \pre \f$ 0 \le p < n \f$
 [[nodiscard]] const T & read( Pos p ) const;

 /// writes \f$ v \f$ at the index \f$ p \f$
 /// \pre \f$ 0 \le p < n \f$
 /// \post `read( p )` is \f$ v \f$, the other items unchanged (an audit)
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the number of items, \f$ n \f$
 [[nodiscard]] Pos size( void ) const;

 /// the size \f$ c \f$ of the vector, which the analysis of the doubling
 /// is about
 [[nodiscard]] Pos capacity( void ) const;

 /// the same items in the same order
 [[nodiscard]] bool operator==( const ArraySequence & o ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the items are contiguous, so that the iterators are pointers and every
 // algorithm of the standard library and of Boost applies; a change made
 // through them bypasses the audits of write()

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
 /// a pointer to an item
 using pointer = T *;
 /// a constant pointer to an item
 using const_pointer = const T *;
 /// the iterator on the items
 using iterator = T *;
 /// the iterator on the constant items
 using const_iterator = const T *;
 /// the iterator on the items, backwards
 using reverse_iterator = std::reverse_iterator< iterator >;
 /// the iterator on the constant items, backwards
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( f_A.get() ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( f_A.get() + f_size ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.get() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A.get() + f_size );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }
 /// the iterator on the last item, backwards
 [[nodiscard]] reverse_iterator rbegin( void ) {
  return( reverse_iterator( end() ) );
  }
 /// the iterator before the first item, backwards
 [[nodiscard]] reverse_iterator rend( void ) {
  return( reverse_iterator( begin() ) );
  }
 /// the constant iterator on the last item, backwards
 [[nodiscard]] const_reverse_iterator rbegin( void ) const {
  return( const_reverse_iterator( end() ) );
  }
 /// the constant iterator before the first item, backwards
 [[nodiscard]] const_reverse_iterator rend( void ) const {
  return( const_reverse_iterator( begin() ) );
  }

 /// the vector of the items
 [[nodiscard]] T * data( void ) { return( f_A.get() ); }
 /// the vector of the items, constant
 [[nodiscard]] const T * data( void ) const { return( f_A.get() ); }

 /// the item at the index \f$ i \f$
 /// \pre \f$ 0 \le i < n \f$
 [[nodiscard]] T & operator[]( Pos i );

 /// the item at the index \f$ i \f$, constant
 /// \pre \f$ 0 \le i < n \f$
 [[nodiscard]] const T & operator[]( Pos i ) const;

 /// \f$ v \f$ at the end, for `std::back_inserter`
 void push_back( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// the items fit in the vector
 [[nodiscard]] bool fits( void ) const;

 /// the \f$ k \f$ items of `o` from the index \f$ i \f$ are those of this
 /// sequence from the index \f$ j \f$
 [[nodiscard]] bool same( const ArraySequence & o , Pos i , Pos j ,
                          Pos k ) const;

 /// replaces the vector with one of \f$ c \f$ places
 void resize( Pos c );

 std::unique_ptr< T[] > f_A;   ///< the vector
 Pos f_size;                   ///< the number of items
 Pos f_capacity;               ///< the size of the vector

 };  // end( class ArraySequence )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/ArraySequence.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ArraySequence.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File ArraySequence.h ------------------------*/
/*--------------------------------------------------------------------------*/
