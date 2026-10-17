/*--------------------------------------------------------------------------*/
/*------------------------------ File Arena.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * An arena of objects allocated in blocks, beyond the book: make() builds
 * an object in the current block, a new block of n objects is taken when
 * it is full, and all the objects die with the arena. Allocating costs a
 * pointer bump instead of a call to the allocator, the objects made one
 * after another are next to each other in memory, and their addresses
 * never change, which the pointers among them (the parents of the nodes of
 * a search) rely on; the price is that an object cannot be freed by
 * itself. The blocks come from the allocator A, which may give, e.g., huge
 * pages.
 *
 * The contracts [see Contract.h]: a block holds at least one object; the
 * invariant is that size() objects have been made. The operations are
 * defined in Arena.tpp.
 *
 * \see D. R. Hanson, Fast allocation and deallocation of memory based on
 *      object lifetimes, Software: Practice and Experience 20(1), 5-12,
 *      1990, doi:10.1002/spe.4380200104
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

#ifndef __ADS_Arena
 #define __ADS_Arena
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS Arena --------------------------------*/
/*--------------------------------------------------------------------------*/

/// an arena of objects of type T in blocks of n
template< class T , class A = std::allocator< T > >
class Arena {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty arena whose blocks hold n objects
 /// \pre n >= 1
 explicit Arena( std::size_t n = std::size_t( 1 ) << 12 ,
                 const A & alloc = A() );

 Arena( const Arena & ) = delete;
 Arena & operator=( const Arena & ) = delete;

 /// destroys all the objects and frees the blocks
 ~Arena();

 /// a new object built from the arguments, whose address never changes
 template< class... Args >
 T * make( Args &&... args );

 /// the number of objects made
 [[nodiscard]] std::size_t size( void ) const;

 /// the memory of the blocks, in bytes
 [[nodiscard]] std::size_t bytes( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_n;                ///< the objects of a block
 std::size_t f_used = 0;         ///< the objects of the last block
 std::size_t f_size = 0;         ///< the objects made
 std::vector< T * > f_blocks;    ///< the blocks
 A f_alloc;                      ///< the allocator of the blocks

 };  // end( class Arena )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/memory/Arena.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Arena.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Arena.h ----------------------------*/
/*--------------------------------------------------------------------------*/
