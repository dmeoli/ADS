/*--------------------------------------------------------------------------*/
/*------------------------ File WordRangeMinimum.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered range minimum query, beyond the book [see `RangeMinimum.h`
 * for the sparse table and the solution of Bender and Farach-Colton]: the
 * place of the leftmost minimum of \f$ A[ i .. j ] \f$, in \f$ O( 1 ) \f$
 * time with a few accesses to the memory and \f$ O( n ) \f$ words of space.
 *
 * The array is cut into blocks of 64 items, the bits of a word. Inside a
 * block that starts at \f$ b \f$, the stack of the minima from the right is
 * kept as a mask for every place \f$ j \f$:
 * \f[
 *   \mathit{mask}[ j ] = \sum \big\{ 2^{t - b} \;:\; b \le t \le j , \;
 *   A[ t ] \le A[ u ] \mbox{ for all } t < u \le j \big\} ,
 * \f]
 * i.e., the places on the stack of the scan of the block from \f$ b \f$ up
 * to \f$ j \f$, which pops the items larger than the new one and pushes it.
 * The leftmost minimum of \f$ A[ i .. j ] \f$ in a block is then the lowest
 * bit of \f$ \mathit{mask}[ j ] \f$ at or above \f$ i \f$,
 * \f$ b + \mathrm{ctz}( \mathit{mask}[ j ] \wedge \neg ( 2^{i-b} - 1 ) ) \f$,
 * one count of the trailing zeros: the places of the stack from \f$ i \f$ on
 * are the minima from the right of \f$ A[ i .. j ] \f$, increasing, and the
 * first of them is the minimum, the leftmost one since the equal items stay
 * on the stack. A query across blocks takes the minimum of the suffix of
 * the block of \f$ i \f$, that of the prefix of the block of \f$ j \f$ and,
 * if there are blocks in between, that of the two entries of a sparse table
 * on the minima of the blocks that cover them [see `SparseTable` in
 * `RangeMinimum.h`], the leftmost of the smallest of three; the minima of
 * the suffixes and of the prefixes are kept for every place, the value and
 * its offset in the block side by side, and so are the minima in the table,
 * so that a query makes at most five reads of the memory, independent of
 * one another, and never reads the array; the minima are chosen by
 * conditional moves, which a random range would mispredict as branches half
 * of the times.
 *
 * The masks are built by the stacks in \f$ O( n ) \f$ time, or on the
 * integers and the floating types under `std::less` by one comparison of
 * vectors of the block per item [see `greaterBits()` in `CountLess.h`], the
 * larger items cleared from the mask of the item before all at once,
 * \f$ \mathit{mask}[ j ] = ( \mathit{mask}[ j-1 ] \wedge \neg G_j )
 * \vee 2^{j-b} \f$ with \f$ G_j \f$ the bits of the items of the block larger
 * than \f$ A[ j ] \f$ (the stack is increasing, so the items it pops are
 * exactly those of it larger than \f$ A[ j ] \f$), with no branch. The space
 * is
 * \f[
 *   n \big( 8 + 2 \, \mathit{sizeof}( T ) + 2 \big) +
 *   \Big\lceil \frac{n}{64} \Big\rceil \Big\lceil \log_2
 *   \Big\lceil \frac{n}{64} \Big\rceil \Big\rceil
 *   \big( \mathit{sizeof}( T ) + 4 \big)
 * \f]
 * bytes besides the array (the masks, the minima of the suffixes and of the
 * prefixes with their offsets, and the table), on huge pages [see
 * `HugePageAllocator.h`]: about 20 bytes per item of 32 bits, against the
 * \f$ 4 \lceil \log_2 n \rceil \f$ bytes of a sparse table on all the
 * items (80 for a million).
 *
 * Compared with the literature: the masks are the in-word stacks of the
 * practical solutions that came after the succinct one of Fischer and Heun
 * (who encode the Cartesian tree of each block in \f$ 2 n + o( n ) \f$ bits
 * and answer through a table of the shapes of the trees), e.g., the
 * structure of Baumstark, Gog, Heuer and Labeit, which trade those bits for
 * a query that touches one or two words; the decomposition into blocks with
 * a sparse table on their minima is that of Bender and Farach-Colton.
 *
 * The contracts [see `Contract.h`]: fewer than \f$ 2^{32} \f$ items; a query
 * asks for \f$ i \le j < n \f$, and its answer is a leftmost minimum of the
 * range, an audit. The functions are defined in `WordRangeMinimum.tpp`.
 *
 * \see J. Fischer, V. Heun, Space-efficient preprocessing schemes for range
 *      minimum queries on static arrays, SIAM Journal on Computing 40(2),
 *      465-492, 2011, doi:10.1137/090779759
 * \see M. A. Bender, M. Farach-Colton, The LCA problem revisited,
 *      Proceedings of LATIN 2000, LNCS 1776, 88-94, 2000,
 *      doi:10.1007/10719839_9
 * \see N. Baumstark, S. Gog, T. Heuer, J. Labeit, Practical range minimum
 *      queries revisited, Proceedings of SEA 2017, LIPIcs 75, 12:1-12:16,
 *      2017, doi:10.4230/LIPIcs.SEA.2017.12
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

#ifndef __ADS_WordRangeMinimum
 #define __ADS_WordRangeMinimum
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/search/CountLess.h"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS WordRangeMinimum --------------------------*/
/*--------------------------------------------------------------------------*/

/// the leftmost minima of the ranges of an array, by the order `less()`
template< class T , class Compare = std::less< T > >
class WordRangeMinimum {

/*--------------------------------------------------------------------------*/
 public:

 /// an empty structure
 WordRangeMinimum( void ) = default;

 /// the structure on \f$ A \f$, in \f$ O( n ) \f$
 ///
 /// In pseudocode (the in-word stacks, beyond the book), \f$ S \f$ the table
 /// of the minima of the blocks as in `SparseTable`:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( A ) \cr
 /// 2 & \quad \textbf{foreach}\ \text{block of 64 items from } b\ \textbf{do}
 ///     \cr
 /// 3 & \qquad s \gets 0 \cr
 /// 4 & \qquad \textbf{for}\ t \gets b\ \textbf{to}\ \text{the end of the }
 ///     \text{block}\ \textbf{do} \cr
 /// 5 & \qquad\quad s \gets s \text{ without the places of the items }
 ///     \text{larger than } A[ t ] \cr
 /// 6 & \qquad\quad s \gets s + 2^{t - b};\ \mathit{mask}[ t ] \gets s \cr
 /// 7 & \qquad \text{the minima of the prefixes and of the suffixes of the }
 ///     \text{block, by two scans} \cr
 /// 8 & \quad S \gets \text{the sparse table of the minima of the blocks}
 /// \end{array}}
 /// \f]
 /// \pre \f$ n < 2^{32} \f$
 explicit WordRangeMinimum( std::vector< T > A , Compare less = {} );

 /// the place of the leftmost minimum of \f$ A[ i .. j ] \f$,
 /// \f$ O( 1 ) \f$
 ///
 /// In pseudocode (the in-word stacks, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{query}( i , j ) \cr
 /// 2 & \quad b_i \gets \lfloor i / 64 \rfloor;\ b_j \gets \lfloor j / 64
 ///     \rfloor \cr
 /// 3 & \quad \textbf{if}\ b_i = b_j\ \textbf{then return}\ 64 b_i +
 ///     \mathrm{ctz}( \mathit{mask}[ j ] \wedge \neg ( 2^{i \bmod 64} - 1 )
 ///     ) \cr
 /// 4 & \quad m \gets \text{the minimum of the suffix of block } b_i
 ///     \text{ from } i \cr
 /// 5 & \quad \textbf{if}\ b_j > b_i + 1\ \textbf{then}\ m \gets \text{the }
 ///     \text{better of } m \text{ and } S.\mathrm{query}( b_i + 1 , b_j - 1
 ///     ) \cr
 /// 6 & \quad \textbf{return}\ \text{the better of } m \text{ and the }
 ///     \text{minimum of the prefix of } b_j \text{ to } j
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le j < n \f$
 /// \post a leftmost minimum of \f$ A[ i .. j ] \f$ (audit)
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 /// the item of place \f$ i \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the array, random access

 using value_type = T;                   ///< the type of the items
 using size_type = std::size_t;          ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using const_reference = const T &;      ///< a reference to an item
 using reference = const_reference;      ///< the same, constant
 using const_iterator = typename std::vector< T >::const_iterator;
                                         ///< an iterator on the items
 using iterator = const_iterator;        ///< the same, constant

 /// the first item
 [[nodiscard]] const_iterator begin( void ) const { return( f_A.begin() ); }
 /// past the last item
 [[nodiscard]] const_iterator end( void ) const { return( f_A.end() ); }
 /// the first item
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 /// past the last item
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// what is kept of a place besides its stack: the minima of the suffix
 /// and of the prefix of the block that end at it, with their offsets in
 /// the block
 struct Place {
  T suf;             ///< the minimum of the suffix
  T pre;             ///< the minimum of the prefix
  std::uint8_t si;   ///< the offset of the minimum of the suffix
  std::uint8_t pi;   ///< the offset of the minimum of the prefix
  };

 /// a minimum and its place
 struct Min {
  T v;               ///< the minimum
  std::uint32_t i;   ///< its place
  };

 /// of the minima \f$ a \f$, then \f$ b \f$, the one of the smaller item,
 /// \f$ a \f$ on a tie;
 /// the value and the place are chosen apart, by conditional moves
 /// rather than by a branch, which the random items would mispredict
 [[nodiscard]] Min better( const Min & a , const Min & b ) const {
  const bool t = f_less( b.v , a.v );
  return( Min{ t ? b.v : a.v , t ? b.i : a.i } );
  }

 /// the stacks of the block of \f$ n \f$ items from \f$ b \f$
 void stacks( std::size_t b , std::size_t n );

 std::vector< T > f_A;                   ///< the items
 std::vector< std::uint64_t , HugePageAllocator< std::uint64_t > > f_M;
                                         ///< the stack of each place
 std::vector< Place , HugePageAllocator< Place > > f_P;   ///< the places
 std::vector< Min , HugePageAllocator< Min > > f_S;       ///< the table
 std::size_t f_blocks = 0;               ///< the blocks, a row of `f_S`
 [[no_unique_address]] Compare f_less;   ///< the order

 };  // end( class WordRangeMinimum )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/WordRangeMinimum.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* WordRangeMinimum.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File WordRangeMinimum.h -----------------------*/
/*--------------------------------------------------------------------------*/
