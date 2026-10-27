/*--------------------------------------------------------------------------*/
/*-------------------------- File SequenceStack.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * `SequenceStack` is the STACK [see `ADT.h`] simulated with the operations
 * of any SEQUENCE, the top being its first item:
 * \f[
 *   \mathrm{top}() = \mathrm{read}( \mathrm{head}() ) \qquad
 *   \mathrm{pop}() = \mathrm{remove}( \mathrm{head}() ) \qquad
 *   \mathrm{push}( v ) = \mathrm{insert}( \mathrm{head}() , v ) \; ,
 * \f]
 * which takes \f$ O( 1 ) \f$ time on a list with pointers, even a singly
 * linked one (exercise 4.6), and has no capacity.
 *
 * The contracts [see `Contract.h`]: `top()` and `pop()` need a stack that
 * is not empty, and `push()` ensures one that is not. The operations are
 * defined in `SequenceStack.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 4.3.1,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf
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

#ifndef __ADS_SequenceStack
 #define __ADS_SequenceStack
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/ADT.h"
#include "ADS/Contract.h"

#include <ranges>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS SequenceStack ---------------------------*/
/*--------------------------------------------------------------------------*/

/// a STACK simulated with the operations of a SEQUENCE `S`, the top first
template< Sequence S >
class SequenceStack {

/*--------------------------------------------------------------------------*/
 public:

 using Item = typename S::Item;   ///< the type of the items

 /// true if there is no item, `empty()` of the sequence
 [[nodiscard]] bool empty( void ) const;

 /// the item on top, `read( head() )`
 /// \pre the stack is not empty
 [[nodiscard]] const Item & top( void ) const;

 /// removes the item on top and returns it, `remove( head() )`
 /// \pre the stack is not empty
 Item pop( void );

 /// puts \f$ v \f$ on top, `insert( head() , v )`
 /// \post the stack is not empty, \f$ v \f$ on top
 void push( const Item & v );

 /// the same items in the same order, those of the sequences
 [[nodiscard]] bool operator==( const SequenceStack & o ) const = default;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the items from the top to the bottom, with the
 // iterators of the sequence, when it has them

 /// the iterator of the sequence on the item on top
 [[nodiscard]] auto begin( void ) const
  requires std::ranges::range< const S > {
  return( std::ranges::cbegin( f_s ) );
  }

 /// the iterator of the sequence past the item at the bottom
 [[nodiscard]] auto end( void ) const
  requires std::ranges::range< const S > {
  return( std::ranges::cend( f_s ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 S f_s;   ///< the sequence, the top as its first item

 };  // end( class SequenceStack )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/stack/impl/SequenceStack.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SequenceStack.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File SequenceStack.h ------------------------*/
/*--------------------------------------------------------------------------*/
