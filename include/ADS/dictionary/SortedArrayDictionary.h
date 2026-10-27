/*--------------------------------------------------------------------------*/
/*---------------------- File SortedArrayDictionary.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A DICTIONARY [see `ADT.h`] realized with a vector of the pairs
 * \f$ ( \mathit{key} , \mathit{value} ) \f$ sorted by key, beyond the book:
 * `lookup()` is the binary search of section 1.2 [see `BinarySearch.h`], in
 * \f$ O( \log n ) \f$ time, while `insert()` and `remove()` move the pairs
 * after the place of the key, in \f$ O( n ) \f$. It is the realization to
 * prefer when the lookups far outnumber the changes, and it also gives the
 * keys in order, and the range of keys between two values, at no further
 * cost.
 *
 * The contracts [see `Contract.h`]: `insert()` ensures that \f$ k \f$ has
 * the value \f$ v \f$, `remove()` that \f$ k \f$ is not there; the invariant
 * is that the keys are strictly increasing. The operations are defined in
 * `SortedArrayDictionary.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, sections 1.2 and 3.2.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf
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

 /// the value of the key \f$ k \f$, or nothing, \f$ O( \log n ) \f$
///
/// In pseudocode (the binary search of section 1.2), \f$ A[ 0 .. n - 1 ] \f$
/// the pairs by increasing key, \f$ \mathit{lo} \f$ the first place whose
/// key is not smaller than \f$ k \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lookup}( k ) \cr
/// 2 & \quad \mathit{lo} \gets 0;\ \mathit{hi} \gets n \cr
/// 3 & \quad \textbf{while}\ \mathit{lo} < \mathit{hi}\ \textbf{do} \cr
/// 4 & \qquad m \gets \lfloor ( \mathit{lo} + \mathit{hi} ) / 2 \rfloor
///     \cr
/// 5 & \qquad \textbf{if}\ A[ m ].\mathit{key} < k\ \textbf{then}
///     \ \mathit{lo} \gets m + 1\ \textbf{else}\ \mathit{hi} \gets m \cr
/// 6 & \quad \textbf{if}\ \mathit{lo} < n\ \textbf{and}\ A[ \mathit{lo}
///     ].\mathit{key} = k\ \textbf{then return}\ A[ \mathit{lo}
///     ].\mathit{value} \cr
/// 7 & \quad \textbf{return nil}
/// \end{array}}
/// \f]
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates the value \f$ v \f$ to the key \f$ k \f$, \f$ O( n ) \f$
 ///
 /// In pseudocode (section 3.2.3), \f$ \mathit{lo} \f$ found as in
 /// `lookup()`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( k , v ) \cr
 /// 2 & \quad \mathit{lo} \gets \text{the first place with key not }
 ///     \text{smaller than } k \cr
 /// 3 & \quad \textbf{if}\ \mathit{lo} < n\ \textbf{and}\ A[ \mathit{lo}
 ///     ].\mathit{key} = k\ \textbf{then}\ A[ \mathit{lo} ].\mathit{value}
 ///     \gets v \cr
 /// 4 & \quad \textbf{else}\ \text{move } A[ \mathit{lo} .. n - 1 ]
 ///     \text{ one place on};\ A[ \mathit{lo} ] \gets ( k , v )
 /// \end{array}}
 /// \f]
 /// \post \f$ k \f$ has the value \f$ v \f$
 void insert( const K & k , const V & v );

 /// removes the key \f$ k \f$, if it is there, moving the pairs after it
 /// one place back, \f$ O( n ) \f$
 /// \post \f$ k \f$ is not there
 void remove( const K & k );

 /// true if there are no keys
 [[nodiscard]] bool empty( void ) const;

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const;

 /// the pairs, sorted by key
 [[nodiscard]] const std::vector< std::pair< K , V > > & pairs( void ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the pairs by increasing key, contiguous

 /// the type of the items, for the standard library
 using value_type = std::pair< K , V >;
 /// the type of the sizes
 using size_type = std::size_t;
 /// the type of the distances between two iterators
 using difference_type = std::ptrdiff_t;
 /// a reference to an item
 using reference = const std::pair< K , V > &;
 /// a constant reference to an item
 using const_reference = const std::pair< K , V > &;
 /// the iterator on the items, constant
 using const_iterator = const std::pair< K , V > *;
 /// the iterator on the items
 using iterator = const_iterator;

 /// the iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.data() ); }
 /// the iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( f_A.data() + f_A.size() );
  }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the place of the first key not smaller than \f$ k \f$, by binary search
 [[nodiscard]] std::size_t place( const K & k ) const;

 /// the keys are strictly increasing, \f$ O( n ) \f$
 [[nodiscard]] bool sorted( void ) const;

 std::vector< std::pair< K , V > > f_A;   ///< the pairs, sorted by key

 };  // end( class SortedArrayDictionary )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/SortedArrayDictionary.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SortedArrayDictionary.h included */

/*--------------------------------------------------------------------------*/
/*-------------------- End File SortedArrayDictionary.h --------------------*/
/*--------------------------------------------------------------------------*/
