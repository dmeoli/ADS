/*--------------------------------------------------------------------------*/
/*------------------------ File WordRangeMinimum.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered range minimum query, beyond the book [see
 * RangeMinimum.h for the sparse table and the solution of Bender and
 * Farach-Colton]: the place of the leftmost minimum of A[ i .. j ], in O( 1
 * ) time with a few accesses to the memory and O( n ) words of space.
 *
 * The array is cut into blocks of 64 items, the bits of a word. Inside a
 * block that starts at b, the stack of the minima from the right is kept
 * as a mask for every place j:
 * \f[
 *   \mathit{mask}[ j ] = \sum \big\{ 2^{t - b} \;:\; b \le t \le j , \;
 *   A[ t ] \le A[ u ] \mbox{ for all } t < u \le j \big\} ,
 * \f]
 * i.e., the places on the stack of the scan of the block from b up to j,
 * which pops the items larger than the new one and pushes it. The leftmost
 * minimum of A[ i .. j ] in a block is then the lowest bit of mask[ j ] at
 * or above i, \f$ b + \mathrm{ctz}( \mathit{mask}[ j ] \wedge \neg ( 2^{i
 * - b} - 1 ) ) \f$, one count of the trailing zeros: the places of the stack
 * from i on are the minima from the right of A[ i .. j ], increasing, and the
 * first of them is the minimum, the leftmost one since the equal items stay
 * on the stack. A query across blocks takes the minimum of the suffix of
 * the block of i, that of the prefix of the block of j and, if there are
 * blocks in between, that of the two entries of a sparse table on the
 * minima of the blocks that cover them [see SparseTable in RangeMinimum.h],
 * the leftmost of the smallest of three; the minima of the suffixes and of
 * the prefixes are kept for every place, the value and its offset in the
 * block side by side, and so are the minima in the table, so that a query
 * makes at most five reads of the memory, independent of one another, and
 * never reads the array; the minima are chosen by conditional moves, which
 * a random range would mispredict as branches half of the times.
 *
 * The masks are built by the stacks in O( n ) time, or on the integers and
 * the floating types under std::less by one comparison of vectors of the
 * block per item [see greaterBits() in CountLess.h], the larger items
 * cleared from the mask of the item before all at once, \f$ \mathit{mask}[
 * j ] = ( \mathit{mask}[ j - 1 ] \wedge \neg G_j ) \vee 2^{j - b} \f$ with
 * \f$ G_j \f$ the bits of the items of the block larger than A[ j ] (the
 * stack is increasing, so the items it pops are exactly those of it larger
 * than A[ j ]), with no branch. The space is
 * \f[
 *   n \big( 8 + 2 \, \mathit{sizeof}( T ) + 2 \big) +
 *   \Big\lceil \frac{n}{64} \Big\rceil \Big\lceil \log_2
 *   \Big\lceil \frac{n}{64} \Big\rceil \Big\rceil
 *   \big( \mathit{sizeof}( T ) + 4 \big)
 * \f]
 * bytes besides the array (the masks, the minima of the suffixes and of the
 * prefixes with their offsets, and the table), on huge pages [see
 * HugePageAllocator.h]: about 20 bytes per item of 32 bits, against the
 * \f$ 4 \lceil \log_2 n \rceil \f$ bytes of a sparse table on all the
 * items (80 for a million).
 *
 * Compared with the literature: the masks are the in-word stacks of the
 * practical solutions that came after the succinct one of Fischer and Heun
 * (who encode the Cartesian tree of each block in 2 n + o( n ) bits and
 * answer through a table of the shapes of the trees), e.g., the structure
 * of Baumstark, Gog, Heuer and Labeit, which trade those bits for a query
 * that touches one or two words; the decomposition into blocks with a
 * sparse table on their minima is that of Bender and Farach-Colton.

 * The contracts [see Contract.h]: fewer than \f$ 2^{32} \f$ items; a query
 * asks for i <= j < n, and its answer is a leftmost minimum of the range,
 * an audit. The functions are defined in WordRangeMinimum.tpp.
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

/// the leftmost minima of the ranges of an array, by the order less()
template< class T , class Compare = std::less< T > >
class WordRangeMinimum {

/*--------------------------------------------------------------------------*/
 public:

 WordRangeMinimum( void ) = default;

 /// the structure on A
 /// \pre A.size() < 2^32
 explicit WordRangeMinimum( std::vector< T > A , Compare less = {} );

 /// the place of the leftmost minimum of A[ i .. j ]
 /// \pre i <= j < size()
 [[nodiscard]] std::size_t query( std::size_t i , std::size_t j ) const;

 [[nodiscard]] std::size_t size( void ) const { return( f_A.size() ); }

 [[nodiscard]] const T & operator[]( std::size_t i ) const;

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 // a constant view of the array, random access

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using const_reference = const T &;
 using reference = const_reference;
 using const_iterator = typename std::vector< T >::const_iterator;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const { return( f_A.begin() ); }
 [[nodiscard]] const_iterator end( void ) const { return( f_A.end() ); }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// what is kept of a place besides its stack: the minima of the suffix
 /// and of the prefix of the block that end at it, with their offsets in
 /// the block
 struct Place {
  T suf;
  T pre;
  std::uint8_t si;
  std::uint8_t pi;
  };

 /// a minimum and its place
 struct Min {
  T v;
  std::uint32_t i;
  };

 /// of the minima a, then b, the one of the smaller item, a on a tie;
 /// the value and the place are chosen apart, by conditional moves
 /// rather than by a branch, which the random items would mispredict
 [[nodiscard]] Min better( const Min & a , const Min & b ) const {
  const bool t = f_less( b.v , a.v );
  return( Min{ t ? b.v : a.v , t ? b.i : a.i } );
  }

 /// the stacks of the block of n items from b
 void stacks( std::size_t b , std::size_t n );

 std::vector< T > f_A;
 std::vector< std::uint64_t , HugePageAllocator< std::uint64_t > > f_M;
                                         ///< the stack of each place
 std::vector< Place , HugePageAllocator< Place > > f_P;   ///< the places
 std::vector< Min , HugePageAllocator< Min > > f_S;       ///< the table
 std::size_t f_blocks = 0;               ///< the blocks, a row of f_S
 [[no_unique_address]] Compare f_less;

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
