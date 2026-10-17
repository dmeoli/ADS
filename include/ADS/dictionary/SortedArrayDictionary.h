/*--------------------------------------------------------------------------*/
/*---------------------- File SortedArrayDictionary.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A DICTIONARY [see ADT.h] realized with a vector of the pairs ( key ,
 * value ) sorted by key, beyond the book: lookup() is the binary search of
 * section 1.2 [see BinarySearch.h], in O( log n ) time, while insert() and
 * remove() move the pairs after the place of the key, in O( n ). It is the
 * realization to prefer when the lookups far outnumber the changes, and it
 * also gives the keys in order, and the range of keys between two values,
 * at no further cost.
 *
 * The contracts [see Contract.h]: insert() ensures that k has the value v,
 * remove() that k is not there; the invariant is that the keys are
 * strictly increasing. The operations are defined in
 * SortedArrayDictionary.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.2 and 3.2.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
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

#ifndef __ADS_SortedArrayDictionary
 #define __ADS_SortedArrayDictionary
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS SortedArrayDictionary -----------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY realized with a vector sorted by key
template< std::totally_ordered K , class V >
class SortedArrayDictionary {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// the value of the key k, or nothing, O( log n )
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value v to the key k, O( n )
 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// removes the key k, if it is there, O( n )
 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool isEmpty( void ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the pairs, sorted by key
 [[nodiscard]] const std::vector< std::pair< K , V > > & pairs( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the place of the first key not smaller than k, by binary search
 [[nodiscard]] std::size_t place( const K & k ) const;

 /// the keys are strictly increasing, O( n )
 [[nodiscard]] bool sorted( void ) const;

 std::vector< std::pair< K , V > > f_A;   ///< the pairs, sorted by key

 };  // end( class SortedArrayDictionary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/SortedArrayDictionary.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SortedArrayDictionary.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File SortedArrayDictionary.h --------------------*/
/*--------------------------------------------------------------------------*/
