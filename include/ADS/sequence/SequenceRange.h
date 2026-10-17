/*--------------------------------------------------------------------------*/
/*-------------------------- File SequenceRange.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The range of the items of a SEQUENCE [see ADT.h], beyond the book, so that
 * the for on a range of C++ visits a sequence through its positions:
 * for( const auto & x : range( s ) ) is the loop p = head(), read( p ),
 * p = next( p ) until finished( p ) of the book. The iterator holds a
 * position, and the end is an iterator marked as such that compares equal
 * to any iterator on a finished position, so that no position
 * \f$ pos_n \f$ needs to be computed (tail() costs O( n ) on a singly
 * linked list) while the range stays common, as the algorithms of C++17 and
 * of Boost want it; the range is a view, a pointer to the sequence. Each
 * step costs what next() costs.
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
#include <ranges>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS SequenceRange ----------------------------*/
/*--------------------------------------------------------------------------*/

/// the items of a SEQUENCE as a range of C++
template< Sequence S >
class SequenceRange
 : public std::ranges::view_interface< SequenceRange< S > > {

/*--------------------------------------------------------------------------*/
 public:

 /// a position of the sequence, read by *; the end is an iterator marked
 /// as such, equal to every iterator on a finished position
 class iterator {
  public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = typename S::Item;
  using difference_type = std::ptrdiff_t;
  using reference = const value_type &;
  using pointer = const value_type *;

  iterator( void ) = default;
  iterator( const S * s , typename S::Pos p ) : f_s( s ) , f_p( p ) {}

  /// the end of the range of s
  static iterator end( const S * s ) {
   iterator e;
   e.f_s = s;
   e.f_end = true;
   return( e );
   }

  [[nodiscard]] reference operator*( void ) const {
   return( f_s->read( f_p ) );
   }

  [[nodiscard]] pointer operator->( void ) const { return( &**this ); }

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
   return( atEnd() );
   }

  [[nodiscard]] bool operator==( const iterator & o ) const {
   const bool a = atEnd() , b = o.atEnd();
   return( ( a == b ) && ( a || ( f_p == o.f_p ) ) );
   }

  /// the position of the book
  [[nodiscard]] typename S::Pos position( void ) const { return( f_p ); }

  private:
  [[nodiscard]] bool atEnd( void ) const {
   return( f_end || ( f_s == nullptr ) || f_s->finished( f_p ) );
   }

  const S * f_s = nullptr;
  typename S::Pos f_p{};
  bool f_end = false;
  };

 SequenceRange( void ) = default;

 explicit SequenceRange( const S & s ) : f_s( &s ) {}

 [[nodiscard]] iterator begin( void ) const {
  return( iterator( f_s , f_s->head() ) );
  }

 /// an iterator, so that the range is common and the algorithms of C++17
 /// and of Boost take it, but no position pos_n is computed
 [[nodiscard]] iterator end( void ) const { return( iterator::end( f_s ) ); }

/*--------------------------------------------------------------------------*/
 private:

 const S * f_s = nullptr;   ///< the sequence

 };  // end( class SequenceRange )

/// the range of the items of s
template< Sequence S >
SequenceRange< S > range( const S & s );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/// the iterators point into the sequence, not into the view
template< ADS::Sequence S >
inline constexpr bool
std::ranges::enable_borrowed_range< ADS::SequenceRange< S > > = true;

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/SequenceRange.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceRange.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SequenceRange.h ------------------------*/
/*--------------------------------------------------------------------------*/
