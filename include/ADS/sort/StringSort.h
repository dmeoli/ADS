/*--------------------------------------------------------------------------*/
/*--------------------------- File StringSort.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of strings by integer keys of their prefixes, beyond the book:
 * prefixKeySort() reads each string a few times in all, instead of once
 * per pass or per comparison, and leaves the work to the quicksort of
 * integers in the vector registers [see VectorQuickSort.h].
 *
 * A group of n strings that agree on their first d characters is sorted
 * in one step:
 * - the longest prefix L that all the strings of the group share from d
 *   on is found by comparing each with the first one, and skipped:
 *   \f$ d \leftarrow d + L \f$; if it is the whole of each, the strings
 *   are all equal and the group is done;
 * - each string i gets the integer key \f$ k_i = \lfloor w_i / 2^b
 *   \rfloor 2^b + i \f$, where \f$ b = \lceil \log_2 n \rceil \f$ and
 *   \f$ w_i \f$ is the number of 64 bits whose bytes, from the most
 *   significant one, are the characters d, d + 1, ..., d + 7 of the
 *   string, 0 past its end: the order of the keys is that of the strings
 *   on those \f$ 64 - b \f$ bits, and then that of their places, so that
 *   the sort is stable;
 * - the keys are sorted by vectorQuickSort(), and the strings moved once
 *   to their places through a buffer;
 * - the strings with the same \f$ \lfloor k_i / 2^b \rfloor \f$ are next
 *   to each other and agree on the characters up to
 *   \f$ d + \lfloor ( 64 - b ) / 8 \rfloor \f$, a group sorted in turn
 *   from there; a group of at most 8 strings is sorted by insertion.
 *
 * Each step reads the characters of each string of the group once, at
 * the place of the string, in the order of the vector, and moves each
 * string once, while a radix sort reads one character per pass and a
 * comparison sort two strings per comparison, at random places of the
 * memory once the strings have moved. A step that leaves a group of more
 * than 3 / 4 of its strings counts against a budget of \f$ 2 \log_2 n \f$
 * such steps, past which a group is sorted by std::stable_sort() on the
 * characters from d: so the steps are \f$ O( \log n ) \f$ deep and their
 * work \f$ O( n \log n ) \f$ beyond the reads of the characters, which
 * are \f$ O( D + n \log n ) \f$ for D the total length of the
 * distinguishing prefixes. The sort is stable, and takes n keys of 64
 * bits and the room of n strings besides the vector.
 *
 * The strings ending inside a key are padded with 0, which a string
 * holding the character 0 shares: such strings may agree on every key of
 * every step, and a step that splits no group sends it to std::stable_sort()
 * at once, which compares them whole.
 *
 * The contracts [see Contract.h]: the vector ends with the same strings
 * sorted, an audit. The function is defined in StringSort.tpp.
 *
 * \see J. Karkkainen, T. Rantala, Engineering radix sort for strings,
 *      Proceedings of the 15th International Symposium on String
 *      Processing and Information Retrieval (SPIRE), Lecture Notes in
 *      Computer Science 5280, 3-14, 2008,
 *      doi:10.1007/978-3-540-89097-3_3
 * \see T. Bingmann, P. Sanders, Parallel string sample sort, Proceedings
 *      of the 21st European Symposium on Algorithms (ESA), Lecture Notes
 *      in Computer Science 8125, 169-180, 2013,
 *      doi:10.1007/978-3-642-40450-4_15
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

#ifndef __ADS_StringSort
 #define __ADS_StringSort
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"
#include "ADS/sort/VectorQuickSort.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the stable sort of strings by integer keys of their prefixes
/// \post A is sorted, with the same strings (an audit)
inline void prefixKeySort( std::vector< std::string > & A );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/StringSort.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StringSort.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File StringSort.h -------------------------*/
/*--------------------------------------------------------------------------*/
