/*--------------------------------------------------------------------------*/
/*----------------------------- File Huffman.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The code of Huffman (reality check of section 14.4): a <em>prefix
 * code</em>, no code being a prefix of another one, so that a text can be
 * decoded, of least length for the frequencies of the characters. The
 * characters go into a min-heap by frequency; the two least frequent ones,
 * \f$ c_1 \f$ and \f$ c_2 \f$, are replaced by a new virtual character of
 * frequency \f$ f_1 + f_2 \f$, whose children in a binary tree are
 * \f$ c_1 \f$ and \f$ c_2 \f$; after \f$ n - 1 \f$ steps the root is left,
 * and the code of a character is the path from the root to its leaf, 0 to
 * the left and 1 to the right, in \f$ O( n \log n ) \f$ time. The least
 * frequent characters get the longest codes.
 *
 * The contracts [see `Contract.h`]: `huffman()` needs at least one
 * character and no negative frequency, and gives a prefix-free code of each
 * character (checked at the level of the audits); `encode()` needs a code
 * for each character of the text, and `decode()` bits that are a
 * concatenation of codes. The operations are defined in `Huffman.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see D. A. Huffman, A method for the construction of minimum-redundancy
 *      codes, Proceedings of the IRE 40(9), 1098-1101, 1952,
 *      doi:10.1109/JRPROC.1952.273898
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

#ifndef __ADS_Huffman
 #define __ADS_Huffman
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/priority/HeapPriorityQueue.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the codes of Huffman of the characters with the given frequencies, at
/// least one; a single character gets the code `0`
///
/// In pseudocode (section 14.4), \f$ Q \f$ a min-heap of the nodes by
/// frequency:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{huffman}( \mathit{freq} ) \cr
/// 2 & \quad Q \gets \text{a min-heap};\ \textbf{foreach}\ c\ \textbf{do}
///     \ \mathrm{insert}( Q , \text{the leaf } c , \mathit{freq}[ c ] ) \cr
/// 3 & \quad \textbf{while}\ |Q| > 1\ \textbf{do} \cr
/// 4 & \qquad ( z_1 , f_1 ) \gets \mathrm{deleteMin}( Q );\ ( z_2 , f_2 )
///     \gets \mathrm{deleteMin}( Q ) \cr
/// 5 & \qquad \mathrm{insert}( Q , \text{a node with children } z_1 , z_2 ,
///     f_1 + f_2 ) \cr
/// 6 & \quad \textbf{foreach}\ \text{leaf } c\ \textbf{do}\ \mathit{code}[ c ]
///     \gets \text{the path from the root, 0 left and 1 right} \cr
/// 7 & \quad \textbf{return}\ \mathit{code}
/// \end{array}}
/// \f]
/// \pre `freq` is not empty, and no frequency is negative
/// \post a prefix-free code of each character
template< class C , class F >
std::map< C , std::string > huffman( const std::map< C , F > & freq );

/// the text encoded by the codes, the code of each character in turn
/// \pre each character of `text` has a code
template< class C >
std::string encode( const std::basic_string< C > & text ,
                    const std::map< C , std::string > & code );

/// the bits decoded by the codes, a prefix code
///
/// In pseudocode (section 14.4), the bits read until they spell a code,
/// which no other code extends:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{decode}( \mathit{bits} , \mathit{code} )
///     \cr
/// 2 & \quad s \gets \varepsilon;\ t \gets \varepsilon \cr
/// 3 & \quad \textbf{foreach}\ b \in \mathit{bits}\ \textbf{do} \cr
/// 4 & \qquad s \gets s b \cr
/// 5 & \qquad \textbf{if}\ s = \mathit{code}[ c ] \text{ for some } c
///     \ \textbf{then}\ t \gets t c;\ s \gets \varepsilon \cr
/// 6 & \quad \textbf{return}\ t
/// \end{array}}
/// \f]
/// \pre `bits` is a concatenation of codes of a prefix-free code
template< class C >
std::basic_string< C > decode( const std::string & bits ,
                               const std::map< C , std::string > & code );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/string/impl/Huffman.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Huffman.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Huffman.h ---------------------------*/
/*--------------------------------------------------------------------------*/
