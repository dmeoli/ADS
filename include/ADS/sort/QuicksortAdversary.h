/*--------------------------------------------------------------------------*/
/*------------------------ File QuicksortAdversary.h -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The adversary of McIlroy against quicksort, beyond the book: a comparator
 * of the items \f$ 0 , \ldots , n - 1 \f$ that does not look at their
 * values, since they have none when the sort starts, and gives them their
 * values while the sort runs, so that the quicksort that it is passed to
 * makes \f$ \Theta( n^2 ) \f$ comparisons (exercise 11.6 of Morin's book).
 *
 * Every item starts as "gas", larger than every value given; when two gas
 * items are compared, one of them is frozen to the next value, the
 * smallest still free, and the adversary prefers to freeze the item that
 * was last compared with a frozen one, the candidate for the pivot, since
 * a quicksort compares its pivot with all the other items of its part.
 * The pivot is thus frozen at the first comparison of a partition, smaller
 * than all the gas of its part, so that every partition takes away the
 * pivot alone, and a part of \f$ k \f$ items costs \f$ k - 1 \f$
 * comparisons or more, \f$ \binom{n}{2} \f$ in all. This holds for any
 * quicksort that chooses its pivot among \f$ O( 1 ) \f$ items, also by a
 * random draw, also with the partition in three of Morin's book, since the
 * adversary answers after the draw; and since the answers are those of a
 * total order of the values given, the values at the end (the gas left
 * frozen in order) are an input on which the same deterministic sort makes
 * the same comparisons, a killer input for a fixed rule of the pivot, as
 * the first item (exercise 11.3), the middle one (exercise 11.4) or any
 * rule that does not look at the values (exercise 11.5).
 *
 * The contracts [see `Contract.h`]: the items compared are below
 * \f$ n \f$; `input()` is a permutation of \f$ 0 , \ldots , n - 1 \f$. The
 * operations are defined in `QuicksortAdversary.tpp`.
 *
 * \see M. D. McIlroy, A killer adversary for quicksort, Software: Practice
 *      and Experience 29(4), 341-344, 1999,
 *      doi:10.1002/(SICI)1097-024X(19990410)29:4<341::AID-SPE237>3.0.CO;2-R
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, section 11.1.2 and
 *      exercises 11.3 to 11.6
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

#ifndef __ADS_QuicksortAdversary
 #define __ADS_QuicksortAdversary
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS QuicksortAdversary -------------------------*/
/*--------------------------------------------------------------------------*/

/// the adversary of McIlroy: a comparator of \f$ n \f$ items that gives
/// them their values while a quicksort runs
class QuicksortAdversary
{

/*--------------------------------------------------------------------------*/
 public:
 /// the adversary of the items \f$ 0 , \ldots , n - 1 \f$, all gas
 explicit QuicksortAdversary( std::size_t n );

 /// the comparator to pass to a sort, by value: a pointer to the adversary,
 /// which must outlive it
 class Less
 {
  public:
  /// the comparator of the adversary \f$ a \f$
  explicit Less( QuicksortAdversary & a ) : f_a( &a ) { }

  /// true if the value of \f$ x \f$ is smaller than that of \f$ y \f$,
  /// after freezing one of them if both are gas
  ///
  /// In pseudocode (McIlroy's adversary, beyond the book), \f$ c \f$ the
  /// candidate for the pivot and \f$ v \f$ the values, \f$ \infty \f$
  /// the gas:
  /// \f[
  /// \class{pseudocode}{\begin{array}{r@{\quad}l}
  /// 1 & \textbf{function}\ \mathrm{less}( x , y ) \cr
  /// 2 & \quad \textbf{if}\ v[ x ] = v[ y ] = \infty\ \textbf{then}
  ///     \ \text{freeze } x \text{ if } x = c \text{, else } y
  ///     \text{, to the next value} \cr
  /// 3 & \quad \textbf{if}\ v[ x ] = \infty\ \textbf{then}\ c \gets x
  ///     \ \textbf{else if}\ v[ y ] = \infty\ \textbf{then}\ c \gets y \cr
  /// 4 & \quad \textbf{return}\ v[ x ] < v[ y ]
  /// \end{array}}
  /// \f]
  /// \pre \f$ x , y < n \f$
  bool operator()( std::size_t x , std::size_t y ) const;

  private:
  QuicksortAdversary * f_a;
  };

 /// the comparator of this adversary
 [[nodiscard]] Less less( void ) { return( Less( *this ) ); }

 /// the items to sort, \f$ 0 , \ldots , n - 1 \f$
 [[nodiscard]] std::vector< std::size_t > items( void ) const;

 /// the number of calls of the comparator so far
 [[nodiscard]] std::size_t comparisons( void ) const { return( f_count ); }

 /// the value of each item, the gas frozen in the order of the items
 /// after the frozen ones: an input of \f$ n \f$ distinct values on which
 /// a deterministic sort makes the comparisons it made here
 /// \post a permutation of \f$ 0 , \ldots , n - 1 \f$
 [[nodiscard]] std::vector< std::size_t > input( void ) const;

/*--------------------------------------------------------------------------*/
 private:
 std::vector< std::size_t > f_val;  ///< the values, \f$ n \f$ for the gas
 std::size_t f_solid = 0;           ///< the values given
 std::size_t f_candidate;           ///< the candidate for the pivot
 std::size_t f_count = 0;           ///< the comparisons

 };  // end( class QuicksortAdversary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/QuicksortAdversary.tpp"

/*--------------------------------------------------------------------------*/

#endif /* QuicksortAdversary.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File QuicksortAdversary.h ----------------------*/
/*--------------------------------------------------------------------------*/
