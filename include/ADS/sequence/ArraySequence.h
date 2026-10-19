/*--------------------------------------------------------------------------*/
/*-------------------------- File ArraySequence.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A SEQUENCE [see ADT.h] realized with a vector: the position of an item
 * is its index, \f$ pos_i = i \f$, with \f$ pos_{-1} = -1 \f$ and
 * \f$ pos_n = n \f$. Moving along the sequence, reading and writing take
 * O( 1 ) time, while insert() and remove() take O( n ) time, since the
 * items after the position move by one place; inserting at the end takes
 * O( 1 ) amortized time, since a full vector is replaced by one of twice
 * its size. If the i-th insertion costs \f$ c_i = i \f$ when it finds the
 * vector full, i.e., when \f$ i = 2^k + 1 \f$, and 1 otherwise, then n
 * insertions cost
 * \f[
 *   C( n ) = \sum_{i=1}^n c_i \leq n + \sum_{k=0}^{\lfloor \log_2 n \rfloor}
 *            2^k \leq n + 2^{\lfloor \log_2 n \rfloor + 1} - 1 \leq 3 n \; ,
 * \f]
 * whereas growing the vector by a constant d makes them cost
 * \f$ \Theta( n^2 ) \f$. Symmetrically, a vector that remove() leaves
 * with \f$ n \leq capacity / 4 \f$ is replaced by one of half its size,
 * so that the space stays O( n ) and any sequence of insertions and
 * removals costs O( 1 ) amortized time each: after a resize the vector is
 * half full, and \f$ \Omega( capacity ) \f$ operations must come before
 * the next one. The empty sequence has no vector, also when remove()
 * empties it, the first insertion makes one of one place, and moving a
 * sequence takes its vector in O( 1 ) time, leaving the one moved from
 * empty.
 *
 * The contracts [see Contract.h]: a position is an index from -1 to n;
 * next() needs one from -1 to n - 1 and prev() one from 0 to n, since
 * there is nothing after \f$ pos_n \f$ nor before \f$ pos_{-1} \f$;
 * insert() needs one from 0 to n, and read(), write() and remove() one
 * that holds an item, from 0 to n - 1. insert() and write() ensure that v
 * is read back in its place, and that the other items are where they were
 * (or one place on, after the insertion), the latter costing O( n ) and
 * therefore an audit; remove() likewise. The invariant is that the n items
 * fit in the vector, \f$ 0 \leq n \leq capacity \f$. The operations are
 * defined in ArraySequence.tpp.
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
 *         Universita' di Pisa \n
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
 using Pos = long; ///< a position is an index, from -1 to n

 /// the empty sequence, with no vector
 ArraySequence( void );

 /// the sequence of the given items, in their order
 ArraySequence( std::initializer_list< T > items );

 /// the sequence of the items of [ first , last ), in their order
 template< std::input_iterator I , std::sentinel_for< I > S >
 ArraySequence( I first , S last );

 ArraySequence( const ArraySequence & other );

 /// the items of other, which is left empty
 ArraySequence( ArraySequence && other ) noexcept;

 ArraySequence & operator=( ArraySequence other ) noexcept;

/*--------------------------------------------------------------------------*/
/*-------------------------- THE SEQUENCE ----------------------------------*/
/*--------------------------------------------------------------------------*/

 [[nodiscard]] bool empty( void ) const;

 /// true if p is -1 or n
 [[nodiscard]] bool finished( Pos p ) const;

 [[nodiscard]] Pos head( void ) const;

 [[nodiscard]] Pos tail( void ) const;

 /// \pre -1 <= p < n
 [[nodiscard]] Pos next( Pos p ) const;

 /// \pre 0 <= p <= n
 [[nodiscard]] Pos prev( Pos p ) const;

 /// puts v at index p, the items from index p moving one place on, which
 /// takes O( n - p ) time, O( 1 ) amortized at the end
 /// \pre 0 <= p <= n
 /// \post one item more, v at index p, the others where they were before
 ///       p and one place on from p (an audit)
 Pos insert( Pos p , const T & v );

 /// removes the item at index p, the following ones moving one place back;
 /// a vector left a quarter full is halved
 /// \pre 0 <= p < n
 /// \post one item less, the others where they were before p and one place
 ///       back after it (an audit)
 Pos remove( Pos p );

 /// \pre 0 <= p < n
 [[nodiscard]] const T & read( Pos p ) const;

 /// \pre 0 <= p < n
 /// \post read( p ) is v, the other items unchanged (an audit)
 void write( Pos p , const T & v );

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the number of items
 [[nodiscard]] Pos size( void ) const;

 /// the size of the vector, which the analysis of the doubling is about
 [[nodiscard]] Pos capacity( void ) const;

 /// the same items in the same order
 [[nodiscard]] bool operator==( const ArraySequence & o ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // the items are contiguous, so that the iterators are pointers and every
 // algorithm of the standard library and of Boost applies; a change made
 // through them bypasses the audits of write()

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;
 using pointer = T *;
 using const_pointer = const T *;
 using iterator = T *;
 using const_iterator = const T *;
 using reverse_iterator = std::reverse_iterator< iterator >;
 using const_reverse_iterator = std::reverse_iterator< const_iterator >;

 [[nodiscard]] iterator begin( void ) { return( f_A.get() ); }
 [[nodiscard]] iterator end( void ) { return( f_A.get() + f_size ); }
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.get() ); }
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A.get() + f_size );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }
 [[nodiscard]] reverse_iterator rbegin( void ) {
  return( reverse_iterator( end() ) );
  }
 [[nodiscard]] reverse_iterator rend( void ) {
  return( reverse_iterator( begin() ) );
  }
 [[nodiscard]] const_reverse_iterator rbegin( void ) const {
  return( const_reverse_iterator( end() ) );
  }
 [[nodiscard]] const_reverse_iterator rend( void ) const {
  return( const_reverse_iterator( begin() ) );
  }

 [[nodiscard]] T * data( void ) { return( f_A.get() ); }
 [[nodiscard]] const T * data( void ) const { return( f_A.get() ); }

 /// \pre 0 <= i < n
 [[nodiscard]] T & operator[]( Pos i );
 [[nodiscard]] const T & operator[]( Pos i ) const;

 /// v at the end, for std::back_inserter
 void push_back( const T & v );

/*--------------------------------------------------------------------------*/
 private:

 /// the items fit in the vector
 [[nodiscard]] bool fits( void ) const;

 /// the k items of o from index i are those of this sequence from index j
 [[nodiscard]] bool same( const ArraySequence & o , Pos i , Pos j ,
                          Pos k ) const;

 /// replaces the vector with one of c places
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
