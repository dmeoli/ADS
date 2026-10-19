/*--------------------------------------------------------------------------*/
/*-------------------------- File StaticSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered searches of a sorted array that does not change, beyond
 * the book [see BinarySearch.h for the binary search of section 1.2]: the
 * place of the first item not smaller than a key, the lower bound, in the
 * fewest misses of the cache and with no branch that the processor may
 * mispredict.
 *
 * - branchlessLowerBound() is the binary search with no branch of Khuong
 *   and Morin: the part still to examine is a base and a length, the
 *   length halves at each step whatever the comparison, which only decides
 *   whether the base moves (a conditional move rather than a jump), and the
 *   two items that the next step may compare are prefetched, so that the
 *   misses of the cache of two steps overlap; \f$ \lceil \log_2 n \rceil
 *   \f$ comparisons, none of them mispredicted.
 * - SPlusTree is the static B+-tree of Slotin (the S+ tree), the sorted
 *   items themselves being its leaves: the items are cut into blocks of B
 *   items, B the items of 64 bytes, a line of cache (16 of 32 bits, 8 of
 *   64 bits), and each internal node has B keys and B + 1 children, its key
 *   j being the first item below its child j + 1; the nodes of a level are
 *   stored one after the other, the children of the node k being the nodes
 *   k ( B + 1 ) , ... , k ( B + 1 ) + B of the next level, so that no
 *   pointer is stored, and the levels one after the other, the leaves last.
 *   A search reads one line per level, \f$ \lceil \log_{B+1} ( n / B )
 *   \rceil + 1 \f$ lines in all (3 for a million keys of 32 bits, against
 *   the 20 of the binary search, of which the last 4 share a line), and in
 *   a node it counts the keys smaller than the key looked for, which is the
 *   child to take: on the arithmetic keys under std::less the count runs on
 *   the whole line with no branch, the places past the last item holding
 *   the largest value of the type, and the compiler turns it into one or
 *   two comparisons of vectors; on the others it is a scan of the line. The
 *   space is \f$ n ( 1 + 1 / B + 1 / B^2 + \dots ) \f$ items, and the
 *   building O( n ) time.
 *
 * Compared with the literature: the layout of Eytzinger (a heap order of
 * the items) also reads one line for the last 4 levels of the binary
 * search, and with the prefetch of the 16 descendants 4 levels down it
 * hides the latency of the misses rather than avoiding them, at the price
 * of the bandwidth; Khuong and Morin found it the fastest of the layouts of
 * the binary search, and the B-tree layout, with a node per line, faster on
 * the arrays that do not fit in the cache. The S+ tree removes from the
 * B-tree of Khuong and Morin the comparison with the key at every level
 * (the leaves are the array, and the internal keys only route), and counts
 * instead of searching in a node.
 *
 * The contracts [see Contract.h]: the array sorted, an audit; each result
 * r is such that the items before r are smaller than the key and those
 * from r on are not, a postcondition checked as an audit. The functions
 * are defined in StaticSearch.tpp. The items of an SPlusTree are
 * trivially copyable.
 *
 * \see P.-V. Khuong, P. Morin, Array layouts for comparison-based
 *      searching, ACM Journal of Experimental Algorithmics 22, 1.3:1-1.3:39,
 *      2017, doi:10.1145/3053370
 * \see S. Slotin, Algorithms for Modern Hardware, 2022, chapter 13.8
 *      (Static B-trees, the S+ tree),
 *      https://en.algorithmica.org/hpc/data-structures/s-tree/
 * \see J. Rao, K. A. Ross, Cache conscious indexing for decision-support
 *      in main memory, Proceedings of VLDB 1999, 78-89, 1999 (the CSS
 *      trees, B-trees with no pointers)
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

#ifndef __ADS_StaticSearch
 #define __ADS_StaticSearch
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/HugePageAllocator.h"
#include "ADS/search/CountLess.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>


/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the first place of an item not smaller than v in the sorted A, n if
/// none, by the binary search with no branch
/// \pre A is sorted (an audit)
/// \post A[ r - 1 ] < v <= A[ r ]
template< class T , class C = std::less<> >
std::size_t branchlessLowerBound( const std::vector< T > & A , const T & v ,
                                  C less = C() );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS SPlusTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the static B+-tree of Slotin on a sorted array, by the order less()
template< class T , class Less = std::less< T > , std::size_t Bytes = 64 >
 requires std::is_trivially_copyable_v< T >
class SPlusTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the items of a node, a line of cache
 static constexpr std::size_t B = sizeof( T ) <= Bytes ?
  std::max< std::size_t >( Bytes / sizeof( T ) , 2 ) : 2;

 SPlusTree( void ) = default;

 /// the tree on the sorted items of A
 /// \pre A is sorted (an audit)
 explicit SPlusTree( const std::vector< T > & A , Less less = Less() );

 SPlusTree( const SPlusTree & o );

 SPlusTree( SPlusTree && o ) noexcept = default;

 SPlusTree & operator=( SPlusTree o ) noexcept;

 /// the first place of an item not smaller than v, size() if none
 [[nodiscard]] std::size_t lowerBound( const T & v ) const;

 /// the first place of an item larger than v, size() if none
 [[nodiscard]] std::size_t upperBound( const T & v ) const;

 [[nodiscard]] bool contains( const T & v ) const;

 /// the item of place i, in the sorted order
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// the levels, the leaves included
 [[nodiscard]] int height( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 static constexpr bool FAST = std::is_arithmetic_v< T > &&
  ( std::is_same_v< Less , std::less< T > > ||
    std::is_same_v< Less , std::less<> > );

 [[nodiscard]] static constexpr T pad( void ) {
  if constexpr( std::numeric_limits< T >::has_infinity )
   return( std::numeric_limits< T >::infinity() );
  else
   return( std::numeric_limits< T >::max() );
  }

 /// the memory of n items, aligned to a line, on huge pages if at least
 /// one [see HugePageAllocator.h]
 [[nodiscard]] static T * allocate( std::size_t n );

 struct Free {
  std::size_t n = 0;                    ///< the items
  void operator()( T * p ) const noexcept {
   if( n * sizeof( T ) >= HUGE )
    HugePageAllocator< T >().deallocate( p , n );
   else
    ::operator delete( p , std::align_val_t( 64 ) );
   }
  };

 static constexpr std::size_t HUGE = std::size_t( 1 ) << 21;

 /// the keys of the node at a smaller than v, or not larger if Up
 template< bool Up >
 [[nodiscard]] std::size_t count( const T * a , const T & v ,
                                  std::size_t m ) const;

 template< bool Up >
 [[nodiscard]] std::size_t search( const T & v ) const;

 void layout( const std::vector< T > & A );

 std::vector< std::size_t > f_size;     ///< the nodes of each level

 std::unique_ptr< T[] , Free > f_a;     ///< the levels, the leaves last
 std::vector< std::size_t > f_off;      ///< the first item of each level
 std::size_t f_items = 0;               ///< the items of the levels
 std::size_t f_n = 0;                   ///< the items
 [[no_unique_address]] Less f_less;     ///< the order of the items

 };  // end( class SPlusTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/search/impl/StaticSearch.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* StaticSearch.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File StaticSearch.h -------------------------*/
/*--------------------------------------------------------------------------*/
