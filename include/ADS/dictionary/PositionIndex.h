/*--------------------------------------------------------------------------*/
/*-------------------------- File PositionIndex.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The index of the positions of objects in a vector, beyond the book: a
 * hash table from the address of an object to its position, of 32 bits,
 * in a vector that holds it, e.g., the constraints or the variables of a
 * model, whose positions change when some of them are erased. Besides the
 * operations of a dictionary, `renumber()` takes the sorted positions just
 * erased from the vector and, in one pass over the table, drops the
 * objects that were there and moves every other position \f$ p \f$ to
 * \f$ p - |\{ e \in E : e < p \}| \f$, its place in the vector after the
 * erasure of \f$ E \f$.
 *
 * The table is `FlatHashTable` [see `FlatHashTable.h`], from the address to
 * the position, with the hash of ADS, so that a search, an insertion and a
 * removal cost what they cost there: in the common case one group of 16
 * control bytes and one line of the pairs, compared all at once.
 *
 * `renumber()` walks all the pairs in the order of the places, which reads
 * the memory of the table once and in order, and gives each key its new
 * position: \f$ \mathrm{rank}( p ) \f$, the erased positions below \f$ p \f$,
 * is the count of the erased positions before the word of 64 bits of
 * \f$ p \f$ in a bit vector of them plus the population count of the bits of
 * that word below \f$ p \f$, in \f$ O( 1 ) \f$ time and with no branch
 * [Jacobson], and a position past the largest erased one, \f$ e \f$, moves by
 * \f$ m \f$, the number of the erased ones; the word and its count sit side
 * by side, so that a rank costs one access to the memory, which for
 * \f$ e \le 2^{22} \f$ is a table of at most 1 MB. The keys of the erased
 * positions are gathered on the way and removed afterwards. The whole of it
 * takes \f$ O( N + m + e / 64 ) \f$ time for \f$ N \f$ places and
 * \f$ e / 4 \f$ bytes of scratch, which stays with the index for the next
 * call.
 *
 * The contracts [see `Contract.h`]: no key is nil, the positions are below
 * \f$ 2^{32} - 1 \f$, and the positions given to `renumber()` are strictly
 * increasing. The operations are defined in `PositionIndex.tpp`.
 *
 * \see G. Jacobson, Space-efficient static trees and graphs, 30th Annual
 *      Symposium on Foundations of Computer Science (FOCS 1989), 549-554,
 *      1989, doi:10.1109/SFCS.1989.63533 (rank by the counts of blocks)
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

#ifndef __ADS_PositionIndex
 #define __ADS_PositionIndex
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/dictionary/FastHash.h"
#include "ADS/dictionary/FlatHashTable.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PositionIndex ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the positions of objects of type `T`, by their addresses
template< class T , class H = FastHash< const T * > >
class PositionIndex {

/*--------------------------------------------------------------------------*/
 public:

 using Position = std::uint32_t;     ///< the type of the positions

 /// the position of a key absent
 static constexpr Position NONE = std::numeric_limits< Position >::max();

 /// an empty index with room for \f$ n \f$ keys
 explicit PositionIndex( std::size_t n = 0 , H h = H() )
  : f_table( n , std::move( h ) ) {}

/*--------------------------------------------------------------------------*/

 /// the position of \f$ p \f$, `NONE` if absent
 [[nodiscard]] Position find( const T * p ) const;

 /// true if \f$ p \f$ is in the index
 [[nodiscard]] bool contains( const T * p ) const;

 /// gives \f$ p \f$ the position \f$ i \f$; true if \f$ p \f$ was absent,
 /// false if its position was replaced
 /// \pre \f$ p \f$ is not nil and \f$ i < \f$ `NONE`
 bool insert( const T * p , Position i );

 /// removes \f$ p \f$; false if it was absent
 bool erase( const T * p );

 /// the index of the vector \f$ v \f$ of pointers (any range with
 /// \f$ v[ i ] \f$): \f$ v[ i ] \f$ at the position \f$ i \f$, all the other
 /// keys gone
 /// \pre no \f$ v[ i ] \f$ is nil, and \f$ v \f$ has fewer than `NONE` items
 template< std::ranges::random_access_range R >
 void assign( const R & v );

 /// the erasure of the positions \f$ E \f$ from the vector: the keys at a
 /// position in \f$ E \f$ go away, and each other position \f$ p \f$ becomes
 /// \f$ p - |\{ e \in E : e < p \}| \f$
 ///
 /// In pseudocode (beyond the book, the rank by the counts of the words of
 /// Jacobson), \f$ B \f$ the bit vector of \f$ E \f$, \f$ w_j \f$ its words of
 /// 64 bits and \f$ r_j \f$ the number of 1s in
 /// \f$ w_0 , \ldots , w_{j-1} \f$, \f$ N = \max E + 1 \f$ and
 /// \f$ m = |E| \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{renumber}( E ) \cr
 /// 2 & \quad \text{build } B \text{ and the counts } r_j \cr
 /// 3 & \quad D \gets \emptyset \cr
 /// 4 & \quad \textbf{foreach}\ ( k , p ) \text{ in the order of the places}
 ///     \ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ p \ge N\ \textbf{then}\ p \gets p - m \cr
 /// 6 & \qquad \textbf{else} \cr
 /// 7 & \qquad\quad \textbf{if}\ B[ p ] = 1\ \textbf{then}\ D \gets D \cup
 ///     \{ k \} \cr
 /// 8 & \qquad\quad p \gets p - r_{\lfloor p / 64 \rfloor} -
 ///     \mathrm{popcount}( w_{\lfloor p / 64 \rfloor} \bmod 2^{p \bmod 64} )
 ///     \cr
 /// 9 & \quad \textbf{foreach}\ k \in D\ \textbf{do}\ \mathrm{remove}( k )
 /// \end{array}}
 /// \f]
 /// \pre \f$ E \f$ is strictly increasing
 void renumber( std::span< const Position > E );

 /// the number of keys
 [[nodiscard]] std::size_t size( void ) const { return( f_table.size() ); }

 /// true if there are no keys
[[nodiscard]] bool empty( void ) const { return( f_table.empty() ); }

 /// the number of places of the table
 [[nodiscard]] std::size_t capacity( void ) const {
  return( f_table.capacity() );
  }

 /// room for \f$ n \f$ keys with no rebuild
 void reserve( std::size_t n ) { f_table.reserve( n ); }

 /// removes all the keys, keeping the places
 void clear( void ) { f_table.clear(); }

 /// `f( p , i )` for each key \f$ p \f$ at the position \f$ i \f$, in the
 /// order of the places
 template< class F >
 void forEach( F && f ) const { f_table.forEach( f ); }

 /// exchanges the contents with `o` in \f$ O( 1 ) \f$
void swap( PositionIndex & o ) noexcept {
  f_table.swap( o.f_table );
  f_words.swap( o.f_words );
  f_drop.swap( o.f_drop );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a word of the positions erased and the count of those before it
 struct Word {
  std::uint64_t bits;             ///< the positions erased in the word
  Position rank;                  ///< the positions erased before it
  };

 FlatHashTable< const T * , Position , H > f_table;   ///< the positions
 std::vector< Word > f_words;      ///< the positions erased
 std::vector< const T * > f_drop;  ///< the keys of the positions erased

 };  // end( class PositionIndex )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/dictionary/impl/PositionIndex.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* PositionIndex.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File PositionIndex.h ------------------------*/
/*--------------------------------------------------------------------------*/
