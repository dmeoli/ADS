/*--------------------------------------------------------------------------*/
/*--------------------------- File IndexedList.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The interfaces of chapter 1 of Morin's book for the sequences, beyond the
 * book, as concepts of C++20 [see `ADT.h` for those of the book]:
 *
 * - `IndexedList`, the LIST of section 1.2.2 of Morin, a sequence
 *   \f$ x_0 , \ldots , x_{n-1} \f$ read and written by rank: `size()`,
 *   `operator[]` for `get( i )` and `set( i , x )`, `insert( i , x )` for
 *   `add( i , x )` and `erase( i )` for `remove( i )`, the names of the
 *   standard library for those of Morin; `RingDeque`, `DualArrayDeque`,
 *   `Treque`, `ContiguousDeque`, `RootishArrayStack`, `TieredVector`,
 *   `CubishArrayStack` and `SEList` are realizations;
 * - `IndexedDeque`, the DEQUE of section 1.2.1 of Morin, a sequence changed
 *   at both ends, by `push_front()`, `push_back()`, `pop_front()` and
 *   `pop_back()`, read by `front()` and `back()`.
 *
 * Two rotations come with them: `rotateArray()`, the item \f$ a[ i ] \f$ of
 * an array moved to \f$ a[ ( i + r ) \bmod n ] \f$ by three reversals
 * (exercise 2.4 of Morin), \f$ n \f$ swaps at most and no memory, and
 * `rotate()`, the same on an `IndexedList`, which calls the `rotate()` of
 * the list when it has one (\f$ O( 1 + \min\{ r , n - r \} ) \f$ on
 * `RingDeque`, `DualArrayDeque` and `List`, exercises 2.5 and 3.8) and
 * otherwise follows the \f$ \gcd( n , r ) \f$ cycles of the permutation by
 * `operator[]`, \f$ n + \gcd( n , r ) \f$ moves.
 *
 * The contracts [see `Contract.h`]: none but those of the lists; the
 * rotations ensure that the items are where they should be (an audit). The
 * algorithms are defined in `IndexedList.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Edition 0.1G, 2013,
 *      https://opendatastructures.org/ods-cpp.pdf, sections 1.2.1 and 1.2.2
 *      and exercises 2.4 and 2.5
 * \see J. Bentley, Programming Pearls, 2nd ed., Addison-Wesley, 2000,
 *      section 2.3 (the rotation by reversals and by cycles)
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

#ifndef __ADS_IndexedList
 #define __ADS_IndexedList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <concepts>
#include <cstddef>
#include <numeric>
#include <span>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------------- CONCEPTS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the LIST of Morin: a sequence read, written, extended and shortened by
/// rank
template< class L >
concept IndexedList = requires( L l , const L cl , std::size_t i ,
                                const typename L::value_type & x ) {
 typename L::value_type;
 { cl.size() } -> std::convertible_to< std::size_t >;
 { l[ i ] } -> std::convertible_to< const typename L::value_type & >;
 l.insert( i , x );
 l.erase( i );
 };

/// the DEQUE of Morin: a sequence changed and read at both ends
template< class D >
concept IndexedDeque = requires( D d , const typename D::value_type & x ) {
 typename D::value_type;
 { d.size() } -> std::convertible_to< std::size_t >;
 d.push_front( x );
 d.push_back( x );
 d.pop_front();
 d.pop_back();
 d.front();
 d.back();
 };

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the item \f$ a[ i ] \f$ moved to \f$ a[ ( i + r ) \bmod n ] \f$, for
/// every \f$ i \f$, by three reversals: \f$ n \f$ swaps at most and
/// \f$ O( 1 ) \f$ space
///
/// In pseudocode (Morin, exercise 2.4, by Bentley's reversals), with
/// \f$ r \f$ reduced modulo \f$ n \f$, the last \f$ r \f$ items becoming the
/// first ones:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{rotateArray}( a , r ) \cr
/// 2 & \quad \mathrm{reverse}( a[ 0 .. n - 1 ] ) \cr
/// 3 & \quad \mathrm{reverse}( a[ 0 .. r - 1 ] );\ \mathrm{reverse}( a[ r ..
///     n - 1 ] )
/// \end{array}}
/// \f]
/// \post each item moved by \f$ r \f$ places, circularly (an audit)
template< class T >
void rotateArray( std::span< T > a , std::size_t r );

/// the item of rank \f$ i \f$ of \f$ l \f$ moved to the rank
/// \f$ ( i + r ) \bmod n \f$: the `rotate()` of the list if it has one,
/// the cycles of the permutation otherwise, \f$ n + \gcd( n , r ) \f$ moves
///
/// In pseudocode (Morin, exercise 2.5, by Bentley's cycles), \f$ r \f$
/// reduced modulo \f$ n \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{rotate}( l , r ) \cr
/// 2 & \quad \textbf{for}\ s \gets 0\ \textbf{to}\ \gcd( n , r ) - 1
///     \ \textbf{do} \cr
/// 3 & \qquad x \gets l[ s ];\ j \gets s \cr
/// 4 & \qquad \textbf{loop} \cr
/// 5 & \qquad\quad p \gets ( j - r ) \bmod n \cr
/// 6 & \qquad\quad \textbf{if}\ p = s\ \textbf{then break} \cr
/// 7 & \qquad\quad l[ j ] \gets l[ p ];\ j \gets p \cr
/// 8 & \qquad l[ j ] \gets x
/// \end{array}}
/// \f]
/// \post each item moved by \f$ r \f$ ranks, circularly (an audit)
template< IndexedList L >
void rotate( L & l , std::size_t r );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/IndexedList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IndexedList.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File IndexedList.h -------------------------*/
/*--------------------------------------------------------------------------*/
