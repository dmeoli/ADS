/*--------------------------------------------------------------------------*/
/*---------------------------- File RadixSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts by the digits of the keys, beyond the book (which has Counting
 * Sort and Pigeonhole Sort, the one-digit cases, in CountingSort.h and
 * PigeonholeSort.h):
 *
 * - radixSort() sorts integers and floating point numbers by their bytes,
 *   from the least significant (LSD), each pass a stable Counting Sort on
 *   one byte between the vector and a buffer: \f$ O( w ( n + 256 ) ) \f$
 *   for keys of w bytes, no comparison. The key of a number is its bits
 *   made unsigned so that their order is the order of the numbers: the sign
 *   bit flipped for a signed integer, and for a floating point number all
 *   the bits flipped if it is negative and the sign bit otherwise (the
 *   order of IEEE 754, -0 before +0, the NaN at the ends). A first sweep
 *   finds the bytes in which some keys differ, and only those are counted
 *   (all the histograms in a second sweep) and make a pass, so that small
 *   keys in wide types cost only the bytes they use; when they differ in
 *   one byte alone, the key tells the number, and a counting sort writes
 *   the numbers back from one of each value, with no buffer. A vector
 *   larger than the cache (1 MiB), whose keys differ in 3 bytes or more,
 *   first goes by its top byte into the buffer, the one pass that scatters
 *   over the whole memory, and each part is sorted in the same way and
 *   then by the other bytes while it fits, so that their passes stay in
 *   the cache (an MSD pass before the LSD ones);
 * - radixSortBy() is the same on any items, by an unsigned key that a
 *   function gives, stable, e.g., the records by one field;
 * - americanFlagSort() is the radix sort from the most significant byte (MSD),
 *   in place: the items are permuted into their 256 buckets by following the
 *   cycles of the permutation, as the flag of the name divides into stripes,
 *   and each bucket is sorted recursively by the next byte, the small ones by
 *   insertion; \f$ O( n ) \f$ extra space for no buffer of the items, not
 *   stable;
 * - msdRadixSort() sorts strings by their characters from the first one, the
 *   end of a string before every character: the first pass permutes them into
 *   their buckets in place, as the American flag sort does, and then each
 *   bucket has a buffer, made only if some bucket is not all of equal strings,
 *   between which and the vector the strings go back and forth, one move per
 *   pass, the character of each string read once per pass and kept aside, and
 *   the small groups sorted by insertion, so that it reads only the
 *   distinguishing prefixes of the strings: \f$ O( D + n \log 256 ) \f$ for D
 *   the total length of those prefixes; the prefix that all the strings of a
 *   group share is first skipped by comparing them with one of them (one
 *   memcmp() for a string that keeps the whole prefix), so that a group of
 *   equal strings, or of strings with a long common prefix, costs one
 *   comparison instead of one pass per character;
 * - bucketSort() sorts numbers in [ 0 , 1 ) into n buckets by their value,
 *   each sorted by insertion: \f$ O( n ) \f$ expected time on numbers drawn
 *   uniformly, \f$ O( n^2 ) \f$ in the worst case.
 *
 * The contracts [see Contract.h]: each sort ends with the same items
 * sorted, an audit; bucketSort() needs numbers in [ 0 , 1 ). The functions
 * are defined in RadixSort.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 12.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf
 * \see P. M. McIlroy, K. Bostic, M. D. McIlroy, Engineering radix sort,
 *      Computing Systems 6(1), 5-27, 1993
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, sections 8.3 and 8.4
 * \see M. Herf, Radix tricks, 2001, http://stereopsis.com/radix.html (the
 *      key of a floating point number, the histograms in one sweep)
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

#ifndef __ADS_RadixSort
 #define __ADS_RadixSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/InsertionSort.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// an integer or a floating point number of at most 8 bytes
template< class T >
concept RadixKey = ( std::is_integral_v< T > ||
                     std::is_floating_point_v< T > ) && ( sizeof( T ) <= 8 );

/// the unsigned key of x, whose order is the order of the numbers
template< RadixKey T >
[[nodiscard]] auto radixKey( T x );

/// the LSD radix sort of numbers
/// \post A is sorted, with the same items (an audit)
template< RadixKey T >
void radixSort( std::vector< T > & A );

/// the LSD radix sort of any items by the unsigned key( x ), stable
/// \post A is sorted by the key, with the same items (an audit)
template< class T , class K >
void radixSortBy( std::vector< T > & A , K key );

/// the MSD radix sort of numbers in place
/// \post A is sorted, with the same items (an audit)
template< RadixKey T >
void americanFlagSort( std::vector< T > & A );

/// the MSD radix sort of strings
/// \post A is sorted, with the same items (an audit)
inline void msdRadixSort( std::vector< std::string > & A );

/// the bucket sort of numbers in [ 0 , 1 )
/// \pre each number is in [ 0 , 1 )
/// \post A is sorted, with the same items (an audit)
template< std::floating_point T >
void bucketSort( std::vector< T > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/RadixSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RadixSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RadixSort.h --------------------------*/
/*--------------------------------------------------------------------------*/
