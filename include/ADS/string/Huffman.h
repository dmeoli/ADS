/*--------------------------------------------------------------------------*/
/*----------------------------- File Huffman.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The code of Huffman (reality check of section 14.4): a prefix code, no
 * code being a prefix of another one, so that a text can be decoded, of
 * least length for the frequencies of the characters. The characters go
 * into a min-heap by frequency; the two least frequent ones, c_1 and c_2,
 * are replaced by a new virtual character of frequency f_1 + f_2, whose
 * children in a binary tree are c_1 and c_2; after n - 1 steps the root is
 * left, and the code of a character is the path from the root to its leaf,
 * 0 to the left and 1 to the right; O( n log n ) time. The least frequent
 * characters get the longest codes.
 *
 * The contracts [see Contract.h]: huffman() needs at least one character and
 * no negative frequency, and gives a prefix-free code of each character
 * (checked at level 3); encode() needs a code for each character of the text,
 * and decode() bits that are a concatenation of codes. The operations are
 * defined in Huffman.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 14.4,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf
 * \see D. A. Huffman, A method for the construction of minimum-redundancy
 *      codes, Proceedings of the IRE 40(9), 1098-1101, 1952,
 *      doi:10.1109/JRPROC.1952.273898
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
/// least one; a single character gets the code "0"
/// \pre freq not empty, no frequency negative
/// \post a prefix-free code of each character
template< class C , class F >
std::map< C , std::string > huffman( const std::map< C , F > & freq );

/// the text encoded by the codes
/// \pre each character of text has a code
template< class C >
std::string encode( const std::basic_string< C > & text ,
                    const std::map< C , std::string > & code );

/// the bits decoded by the codes, a prefix code
/// \pre bits is a concatenation of codes of a prefix-free code
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
