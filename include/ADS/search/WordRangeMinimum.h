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
 * block, the stack of the minima from the right is kept as a mask for every
 * place j: the bit t of mask[ j ] is set if A[ t ] is not larger than every
 * item of A[ t + 1 .. j ], i.e., if t is on the stack of the scan of the
 * block from its start up to j, which pops the items larger than the new
 * one and pushes it. The leftmost minimum of A[ i .. j ] in a block is then
 * the lowest bit of mask[ j ] at or above i, one count of the trailing zeros:
 * the items of the stack from i on are the minima from the right of A[ i ..
 * j ], increasing, and the first of them is the minimum, the leftmost one
 * since the equal items stay on the stack. A query across blocks takes the
 * minimum of the suffix of the block of i, that of the prefix of the block
 * of j, and that of the blocks in between from a sparse table on the minima
 * of the blocks, two entries that cover them [see SparseTable in
 * RangeMinimum.h]; the minima of the suffixes and of the prefixes are kept
 * for every place, the value and its offset in the block side by side, and
 * so are the minima in the table, so that such a query makes four reads of
 * the memory, independent of one another, and never reads the array. The
 * masks take 8 bytes per item, the minima of the suffixes and of the
 * prefixes two items and two bytes, and the table \f$ ( n / 64 ) \log_2( n
 * / 64 ) \f$ items and indices, less than the masks; all are built in O( n )
 * time.
 *
 * Compared with the literature: the masks are the in-word stacks of the
 * practical solutions that came after the succinct one of Fischer and Heun
 * (who encode the Cartesian tree of each block in 2 n + o( n ) bits and
 * answer through a table of the shapes of the trees), e.g., the structure
 * of Baumstark, Gog, Heuer and Labeit, and the "linear RMQ" of the
 * competitive programming, which trade those bits for a query that touches
 * one or two words; the sparse table alone answers with two accesses but
 * takes \f$ n \log_2 n \f$ words, too many past the cache.
 *
 * The contracts [see Contract.h]: fewer than \f$ 2^{32} \f$ items; a query
 * asks for i <= j < n, and its answer is a leftmost minimum of the range,
 * an audit. The functions are defined in WordRangeMinimum.tpp.
 *
 * \see J. Fischer, V. Heun, Space-efficient preprocessing schemes for range
 *      minimum queries on static arrays, SIAM Journal on Computing 40(2),
 *      465-492, 2011, doi:10.1137/090779759
 * \see N. Baumstark, S. Gog, T. Heuer, J. Labeit, Practical range minimum
 *      queries revisited, Proceedings of SEA 2017, LIPIcs 75, 12:1-12:16,
 *      2017, doi:10.4230/LIPIcs.SEA.2017.12
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
