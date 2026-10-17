/*--------------------------------------------------------------------------*/
/*-------------------------- File SequenceRange.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range of the items of a SEQUENCE [see ADT.h], beyond the book, so that
 * the for on a range of C++ visits a sequence through its positions:
 * for( const auto & x : range( s ) ) is the loop p = head(), read( p ),
 * p = next( p ) until finished( p ) of the book. The iterator holds a
 * position, and the end is a sentinel that compares equal to it when the
 * position is finished, so that no position \f$ pos_n \f$ needs to be
 * computed (tail() costs O( n ) on a singly linked list). Each step costs
 * what next() costs.
 *
 * The contracts [see Contract.h] are those of the operations of the
 * sequence; the sequence must not change while it is visited. The functions
 * are defined in SequenceRange.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 3.2.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf
 * \see B. Stroustrup, The C++ Programming Language, 4th ed., Addison-Wesley,
 *      2013, section 33.1 (the iterators)
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

#ifndef __ADS_SequenceRange
 #define __ADS_SequenceRange
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/Contract.h"

#include <cstddef>
#include <iterator>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS SequenceRange ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the items of a SEQUENCE as a range of C++
template< Sequence S >
class SequenceRange {

/*--------------------------------------------------------------------------*/
 public:

 /// a position of the sequence, read by *
 class iterator {
  public:
  using value_type = typename S::Item;
  using difference_type = std::ptrdiff_t;

  iterator( void ) = default;
  iterator( const S * s , typename S::Pos p ) : f_s( s ) , f_p( p ) {}

  [[nodiscard]] const value_type & operator*( void ) const {
   return( f_s->read( f_p ) );
   }

  iterator & operator++( void ) {
   f_p = f_s->next( f_p );
   return( *this );
   }

  iterator operator++( int ) {
   iterator t = *this;
   ++*this;
   return( t );
   }

  /// at the end, i.e., on a finished position
  [[nodiscard]] bool operator==( std::default_sentinel_t ) const {
   return( f_s->finished( f_p ) );
   }

  [[nodiscard]] bool operator==( const iterator & o ) const {
   return( f_p == o.f_p );
   }

  private:
  const S * f_s = nullptr;
  typename S::Pos f_p{};
  };

 explicit SequenceRange( const S & s ) : f_s( s ) {}

 [[nodiscard]] iterator begin( void ) const {
  return( iterator( &f_s , f_s.head() ) );
  }

 [[nodiscard]] std::default_sentinel_t end( void ) const {
  return( std::default_sentinel );
  }

/*--------------------------------------------------------------------------*/
 private:

 const S & f_s;   ///< the sequence

 };  // end( class SequenceRange )

/// the range of the items of s
template< Sequence S >
SequenceRange< S > range( const S & s );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/SequenceRange.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceRange.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SequenceRange.h ------------------------*/
/*--------------------------------------------------------------------------*/
