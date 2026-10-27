/*--------------------------------------------------------------------------*/
/*---------------------------- File Anagrams.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The classes of anagrams among \f$ n \f$ words of length at most \f$ k \f$:
 * two words are <em>anagrams</em> if one is a permutation of the other.
 * `anagramGroups()` gives the signature of each word, its items sorted, and
 * gathers the words of equal signature in a hash table,
 * \f$ O( n k \log k ) \f$ expected time and \f$ O( n k ) \f$ space; the
 * groups come in the order of their first word, and the words of a group in
 * their order. The signature by counting sort over an alphabet of
 * \f$ \sigma \f$ symbols, or the vector of the counts of the symbols, costs
 * \f$ O( k + \sigma ) \f$ per word instead; sorting the pairs
 * \f$ ( \mathit{signature} , \mathit{word} ) \f$ in place of hashing them
 * gives the classes in \f$ O( n k \log n ) \f$ worst-case time.
 *
 * `anagramGroups()` takes the words as any sequence `S` with an iterator
 * constructor, sortable items and a hash, `std::hash< S >` by default; the
 * overload on `std::string` also takes a list of literals.
 *
 * The contracts [see `Contract.h`]: the groups partition the words, and the
 * words of a group are anagrams of its first (an audit). The functions are
 * defined in `Anagrams.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see J. Bentley, Programming Pearls, 2nd ed., Addison-Wesley, 2000,
 *      column 2
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

#ifndef __ADS_Anagrams
 #define __ADS_Anagrams
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the classes of anagrams among the words, in the order of their first
/// word, each in the order of the words
///
/// In pseudocode (chapter 13 and Bentley's column 2), \f$ H \f$ a hash
/// table from a signature to the number of its group:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{anagramGroups}( W ) \cr
/// 2 & \quad H \gets \text{an empty table};\ G \gets \text{an empty }
///     \text{sequence of groups} \cr
/// 3 & \quad \textbf{foreach}\ w \in W\ \textbf{do} \cr
/// 4 & \qquad \kappa \gets \text{the items of } w \text{ sorted} \cr
/// 5 & \qquad \textbf{if}\ \kappa \notin H\ \textbf{then}\ H[ \kappa ] \gets
///     |G|;\ \text{append an empty group to } G \cr
/// 6 & \qquad \text{append } w \text{ to } G[ H[ \kappa ] ] \cr
/// 7 & \quad \textbf{return}\ G
/// \end{array}}
/// \f]
/// \post the groups partition the words, each one of anagrams (audit)
template< class S , class Hash = std::hash< S > >
[[nodiscard]] std::vector< std::vector< S > > anagramGroups(
                                             const std::vector< S > & words );

/// the classes of `anagramGroups()` above, on `std::string` (or a list of
/// literals)
[[nodiscard]] inline std::vector< std::vector< std::string > > anagramGroups(
                                   const std::vector< std::string > & words );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Anagrams.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Anagrams.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Anagrams.h ---------------------------*/
/*--------------------------------------------------------------------------*/
