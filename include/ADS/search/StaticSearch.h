/*--------------------------------------------------------------------------*/
/*-------------------------- File StaticSearch.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered searches of a sorted array that does not change, beyond
 * the book [see `BinarySearch.h` for the binary search of section 1.2]: the
 * place of the first item not smaller than a key, the lower bound, in the
 * fewest misses of the cache and with no branch that the processor may
 * mispredict.
 *
 * - `branchlessLowerBound()` is the binary search with no branch of Khuong
 *   and Morin: the part still to examine is a base and a length, the
 *   length halves at each step whatever the comparison, which only decides
 *   whether the base moves (a conditional move rather than a jump), and the
 *   two items that the next step may compare are prefetched, so that the
 *   misses of the cache of two steps overlap;
 *   \f$ \lceil \log_2 n \rceil + 1 \f$ comparisons on an array of
 *   \f$ n > 0 \f$ items (the length goes from \f$ n \f$ to
 *   \f$ \lceil n / 2 \rceil \f$ at each step, down to 1, and a last
 *   comparison decides), none of them mispredicted;
 * - `SPlusTree` is the static B+-tree of Slotin (the S+ tree), the sorted
 *   items themselves being its leaves: the items are cut into blocks of
 *   \f$ B \f$ items, \f$ B \f$ the items of 64 bytes, a line of cache (16 of
 *   32 bits, 8 of 64 bits), and each internal node has \f$ B \f$ keys and
 *   \f$ B + 1 \f$ children, its key \f$ j \f$ being the first item below its
 *   child \f$ j + 1 \f$; the nodes of a level are stored one after the
 *   other, the children of the node \f$ k \f$ being the nodes
 *   \f$ k ( B + 1 ) , \ldots , k ( B + 1 ) + B \f$ of the next level, so
 *   that no pointer is stored, and the levels one after the other, the
 *   leaves last. The level of the leaves has
 *   \f$ s_{H-1} = \lceil n / B \rceil \f$ nodes and each level above
 *   \f$ s_\ell = \lceil s_{\ell+1} / ( B + 1 ) \rceil \f$, up to the root,
 *   so that there are
 *   \f[
 *     H = 1 + \Big\lceil \log_{B+1} \Big\lceil \frac{n}{B}
 *     \Big\rceil \Big\rceil
 *   \f]
 *   levels, and a search reads one line per level, \f$ H \f$ in all (3 for
 *   a million keys of 32 bits, against the 20 probes of the binary search,
 *   of which the last 4 share a line). In a node the search counts the keys
 *   smaller than the key looked for, which is the child to take: on the
 *   arithmetic keys under `std::less` the count runs on the whole line with
 *   no branch, the places past the last item holding the largest value of
 *   the type, as one or two comparisons of vectors [see `CountLess.h`]; on
 *   the others it is a scan of the keys in use. The space is
 *   \f[
 *     B \sum_\ell s_\ell \approx n \Big( 1 + \frac{1}{B+1}
 *     + \frac{1}{(B+1)^2} + \cdots \Big) = n \frac{B+1}{B}
 *   \f]
 *   items, on huge pages when they are at least 2 MB [see
 *   `HugePageAllocator.h`], so that a search misses the TLB at most once,
 *   and the building takes \f$ O( n ) \f$ time.
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
 * instead of searching in a node; the FAST trees of Kim et al. lay out a
 * binary tree in blocks of lines and pages and search a block with SIMD
 * comparisons, and the comparative study of Bellomo et al. finds the
 * B-trees with SIMD searches the fastest in a lookup among the classic,
 * the compressed and the learned indexes [see `PGMIndex.h`] of integers.
 *
 * The contracts [see `Contract.h`]: the array sorted, an audit; each result
 * \f$ r \f$ is such that the items before \f$ r \f$ are smaller than the key
 * and those from \f$ r \f$ on are not, a postcondition checked as an audit.
 * The functions are defined in `StaticSearch.tpp`. The items of an
 * `SPlusTree` are trivially copyable.
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
 * \see C. Kim, J. Chhugani, N. Satish, E. Sedlar, A. D. Nguyen, T.
 *      Kaldewey, V. W. Lee, S. A. Brandt, P. Dubey, FAST: fast architecture
 *      sensitive tree search on modern CPUs and GPUs, Proceedings of SIGMOD
 *      2010, 339-350, 2010, doi:10.1145/1807167.1807206
 * \see L. Bellomo, G. Cianci, L. de Rosa, P. Ferragina, M. Odorisio, A
 *      comparative study of compressed, learned, and traditional indexing
 *      methods for integer data, Proceedings of SEA 2025, LIPIcs 338,
 *      5:1-5:23, 2025, doi:10.4230/LIPIcs.SEA.2025.5
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

/// the first place of an item not smaller than \f$ v \f$ in the sorted
/// \f$ A \f$, \f$ n \f$ if none, by the binary search with no branch
///
/// In pseudocode (Khuong and Morin, beyond the book), the part still to
/// examine being \f$ A[ b .. b + \ell - 1 ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{branchlessLowerBound}( A , v ) \cr
/// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ 0 \cr
/// 3 & \quad b \gets 0;\ \ell \gets n \cr
/// 4 & \quad \textbf{while}\ \ell > 1\ \textbf{do} \cr
/// 5 & \qquad h \gets \lfloor \ell / 2 \rfloor;\ \text{prefetch the two }
///     \text{middles of the next step} \cr
/// 6 & \qquad b \gets b + h \cdot [ A[ b + h ] < v ]\quad \triangleright
///     \ \text{a conditional move} \cr
/// 7 & \qquad \ell \gets \ell - h \cr
/// 8 & \quad \textbf{return}\ b + [ A[ b ] < v ]
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is sorted (audit)
/// \post \f$ A[ r - 1 ] < v \le A[ r ] \f$
template< class T , class C = std::less<> >
std::size_t branchlessLowerBound( const std::vector< T > & A , const T & v ,
                                  C less = C() );

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS SPlusTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the static B+-tree of Slotin on a sorted array, by the order `less()`
template< class T , class Less = std::less< T > , std::size_t Bytes = 64 >
 requires std::is_trivially_copyable_v< T >
class SPlusTree {

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;   ///< the type of the items

 /// the items of a node, a line of cache
 static constexpr std::size_t B = sizeof( T ) <= Bytes ?
  std::max< std::size_t >( Bytes / sizeof( T ) , 2 ) : 2;

 /// an empty tree
 SPlusTree( void ) = default;

 /// the tree on the sorted items of \f$ A \f$, in \f$ O( n ) \f$
 ///
 /// In pseudocode (the layout of Slotin, beyond the book), the levels
 /// numbered from the root, \f$ 0 \f$, to the leaves, \f$ H - 1 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{layout}( A ) \cr
 /// 2 & \quad s_{H-1} \gets \lceil n / B \rceil;\ \textbf{while}\ s_\ell > 1
 ///     \ \textbf{do}\ s_{\ell-1} \gets \lceil s_\ell / ( B + 1 ) \rceil \cr
 /// 3 & \quad \text{fill all the nodes with the largest value};\ \text{copy }
 ///     A \text{ into the leaves} \cr
 /// 4 & \quad \textbf{foreach}\ \text{level } h < H - 1 \text{, node } k
 ///     \text{ and key } j\ \textbf{do} \cr
 /// 5 & \qquad c \gets k ( B + 1 ) + j + 1\quad \triangleright\ \text{the }
 ///     \text{child after the key} \cr
 /// 6 & \qquad \textbf{if}\ c < s_{h+1}\ \textbf{then}\ \text{key } j
 ///     \text{ of } k \gets A[ c ( B + 1 )^{H - h - 2} B ]
 /// \end{array}}
 /// \f]
 /// \pre \f$ A \f$ is sorted (audit)
 explicit SPlusTree( const std::vector< T > & A , Less less = Less() );

 /// a copy of the tree \f$ o \f$
 SPlusTree( const SPlusTree & o );

 /// the tree \f$ o \f$, moved
 SPlusTree( SPlusTree && o ) noexcept = default;

 /// the tree \f$ o \f$, assigned by a swap
 SPlusTree & operator=( SPlusTree o ) noexcept;

 /// the first place of an item not smaller than \f$ v \f$, `size()` if none
 ///
 /// In pseudocode (Slotin, beyond the book), \f$ \mathrm{count}( k , v ) \f$
 /// the keys of the node \f$ k \f$ smaller than \f$ v \f$, with no branch on
 /// the arithmetic keys [see `CountLess.h`]:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{lowerBound}( v ) \cr
 /// 2 & \quad k \gets 0 \cr
 /// 3 & \quad \textbf{for}\ h \gets 0\ \textbf{to}\ H - 2\ \textbf{do}\ k
 ///     \gets k ( B + 1 ) + \mathrm{count}( \text{node } k \text{ of level }
 ///     h , v ) \cr
 /// 4 & \quad \textbf{return}\ \min\{ n , k B + \mathrm{count}( \text{leaf }
 ///     k , v ) \}
 /// \end{array}}
 /// \f]
 /// \post \f$ A[ r - 1 ] < v \le A[ r ] \f$
 [[nodiscard]] std::size_t lowerBound( const T & v ) const;

 /// the first place of an item larger than \f$ v \f$, `size()` if none
 ///
 /// It is `lowerBound()` above with the keys not larger than \f$ v \f$
 /// counted, \f$ n \f$ at once if \f$ v \f$ is not smaller than the padding.
 /// \post \f$ A[ r - 1 ] \le v < A[ r ] \f$
 [[nodiscard]] std::size_t upperBound( const T & v ) const;

 /// true if \f$ v \f$ is an item, by `lowerBound( v )`
 [[nodiscard]] bool contains( const T & v ) const;

 /// the item of place \f$ i \f$, in the sorted order
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const;

 /// the number \f$ n \f$ of the items
 [[nodiscard]] std::size_t size( void ) const;

 /// true if there are no items
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

 /// the memory of \f$ n \f$ items, aligned to a line, on huge pages if at
 /// least one [see `HugePageAllocator.h`]
 [[nodiscard]] static T * allocate( std::size_t n );

 struct Free {
  std::size_t n = 0;                    ///< the items
  void operator()( T * p ) const noexcept {
   if( n * sizeof( T ) >= HUGE_BYTES )
    HugePageAllocator< T >().deallocate( p , n );
   else
    ::operator delete( p , std::align_val_t( 64 ) );
   }
  };

 static constexpr std::size_t HUGE_BYTES = std::size_t( 1 ) << 21;

 /// the keys of the node at \f$ a \f$ smaller than \f$ v \f$, or not larger
 /// if `Up`
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
