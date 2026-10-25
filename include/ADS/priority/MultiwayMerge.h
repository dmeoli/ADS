/*--------------------------------------------------------------------------*/
/*------------------------- File MultiwayMerge.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered merge of k sorted sequences, beyond the book [see
 * LoserTree.h for the tree of losers of the notes]: FlatLoserTree, the
 * tree of losers with the keys in its nodes, for the merges whose
 * sequences arrive a block at a time, and multiwayMerge(), the merge of k
 * sorted ranges in memory, by a funnel of binary mergers.
 *
 * - FlatLoserTree keeps in each of its \f$ K - 1 \f$ internal nodes, K the
 *   power of two not below k, the key of the loser of the match played
 *   there and its source, instead of the index of the player alone: a
 *   replay walks the \f$ \log_2 K \f$ nodes from the leaf of the winner to
 *   the root, and each match reads the key in the node, which lies in the
 *   array of the tree, never in the sequence of the loser. A key of at
 *   most 16 bytes copied as bytes is kept by value, a larger one by its
 *   address, which must stay valid while it is in the tree. On the
 *   integers in their natural order a match compares the pairs \f$ ( x ,
 *   s ) \f$ of key and source as one integer of 128 bits, and an exhausted
 *   source has the largest key and its index with the highest bit set, so
 *   that the matches are made of flags and the swaps of the winner with
 *   the loser of a node of masks, with no jump: the matches of a random
 *   merge are won by either side with probability 1/2, which a branch
 *   mispredicts half of the time. Equal keys are won by the lower source,
 *   so that a merge is stable; a replay makes \f$ \lceil \log_2 k \rceil
 *   \f$ matches.
 * - multiwayMerge() on contiguous ranges of items copied as bytes builds a
 *   tree of binary mergers, the halves of the sources below each node,
 *   with a buffer of 4 KiB at each internal node, filled lazily (the lazy
 *   funnel of Brodal, Fagerberg and Vinther with binary mergers): a node
 *   merges its two children into its buffer when the buffer runs out, a
 *   child being refilled when it runs out in turn. A merge goes by
 *   stretches as long as the least of the room left in its two inputs and
 *   its output, \f$ m = \min\{ | X | , | Y | , | O | \} \f$, since each
 *   step takes one item from one input, so that its m steps check no end;
 *   each step is a comparison, a store of the item picked by a mask and
 *   the advance of the input by the flag, with no jump. An item climbs
 *   \f$ \lceil \log_2 k \rceil \f$ mergers, \f$ O( n \log k ) \f$ time
 *   with the constant of a merge of two sequences, the buffers taking
 *   \f$ 4 ( k - 1 ) \f$ KiB, in the cache for the fan-ins of the external
 *   sorts. Items of more than 16 bytes in 3 or 4 ranges are merged by
 *   computing at each step the least head of each pair and the least of
 *   the two, with no jump, over stretches as long as the shortest range,
 *   so that an item is copied once. Two ranges are merged by the same
 *   step with no jump, one range is copied, and the other iterators and
 *   types go through FlatLoserTree, \f$ O( n \log k + k ) \f$ time.
 *
 * Compared with the literature: the tree of losers with the keys in the
 * nodes is that of Knuth (5.4.1), whose cache behaviour Sanders analysed
 * for his sequence heaps; the multiway merge of the MCSTL of Singler,
 * Sanders and Putze, in the parallel mode of libstdc++ and in tlx, has the
 * same tree, with branches on the matches, and special merges of 3 and 4
 * sequences. The funnel is the k-merger of the cache-oblivious funnelsort
 * of Frigo, Leiserson, Prokop and Ramachandran, made of binary mergers
 * as Brodal, Fagerberg and Vinther engineered it; with no jump in its
 * steps it beats the tree of losers by about 1.7 times on random keys,
 * where every match of the tree is a coin toss for the predictor.
 *
 * The contracts [see Contract.h]: k >= 1 sources; top(), winner() and the
 * replays on a tree that is not empty; the ranges of multiwayMerge()
 * sorted by less, an audit, and the output their items sorted, a stable
 * merge, an audit. The functions are defined in MultiwayMerge.tpp.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.4.1
 * \see P. Sanders, Fast priority queues for cached memory, ACM Journal of
 *      Experimental Algorithmics 5, 7, 2000, doi:10.1145/351827.384249
 * \see J. Singler, P. Sanders, F. Putze, MCSTL: the multi-core standard
 *      template library, Euro-Par 2007, LNCS 4641, 682-694, 2007,
 *      doi:10.1007/978-3-540-74466-5_72
 * \see M. Frigo, C. E. Leiserson, H. Prokop, S. Ramachandran,
 *      Cache-oblivious algorithms, ACM Transactions on Algorithms 8(1),
 *      1-22, 2012, doi:10.1145/2071379.2071383
 * \see G. S. Brodal, R. Fagerberg, K. Vinther, Engineering a
 *      cache-oblivious sorting algorithm, ACM Journal of Experimental
 *      Algorithmics 12, 1-23, 2008, doi:10.1145/1227161.1227164
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

#ifndef __ADS_MultiwayMerge
 #define __ADS_MultiwayMerge
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the natural order of an arithmetic type: std::less or std::greater
template< class T , class C >
inline constexpr bool kNaturalOrder =
 std::is_arithmetic_v< T > &&
 ( std::is_same_v< C , std::less<> > || std::is_same_v< C , std::less< T > >
   || std::is_same_v< C , std::greater<> > ||
   std::is_same_v< C , std::greater< T > > );

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS FlatLoserTree ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the tree of losers of k sources with the keys in its nodes, the winner
/// the smallest key by less, the lower source on equal keys
template< class T , class C = std::less<> >
class FlatLoserTree {

/*--------------------------------------------------------------------------*/
 public:

 /// the tree of k sources, all exhausted
 /// \pre k >= 1 and k < 2^31
 explicit FlatLoserTree( std::size_t k , C less = C() );

 /// k, the sources
 [[nodiscard]] std::size_t sources( void ) const;

 /// before build(): the source s starts with the key x, whose address is
 /// kept if T is large
 /// \pre s < k
 void start( std::size_t s , const T & x );

 /// plays the matches of the keys given by start()
 void build( void );

 /// all the sources exhausted
 [[nodiscard]] bool empty( void ) const;

 /// the source with the smallest key
 /// \pre ! empty()
 [[nodiscard]] std::size_t winner( void ) const;

 /// its key
 /// \pre ! empty()
 [[nodiscard]] const T & top( void ) const;

 /// the winner takes the key x, and the matches on its path are replayed
 /// \pre ! empty()
 void replaceTop( const T & x );

 /// the winner is exhausted, and the matches on its path are replayed
 /// \pre ! empty()
 void exhaustTop( void );

/*--------------------------------------------------------------------------*/
 private:

 /// the arithmetic types in their natural order
 static constexpr bool kNatural = detail::kNaturalOrder< T , C >;

 /// the integers of at most 64 bits in their natural order, compared with
 /// their sources as one integer of 128 bits where the compiler has it
 static constexpr bool kInteger = kNatural && std::is_integral_v< T > &&
                                  ! std::is_same_v< T , bool > &&
                                  ( sizeof( T ) <= 8 );

 /// the natural order is std::less
 static constexpr bool kUp = std::is_same_v< C , std::less<> > ||
                             std::is_same_v< C , std::less< T > >;

 /// a key by value if small and copied as bytes, else by address
 static constexpr bool kByValue = std::is_trivially_copyable_v< T > &&
                                  ( sizeof( T ) <= 16 ) &&
                                  std::is_default_constructible_v< T >;

 using Key = std::conditional_t< kByValue , T , const T * >;

 /// the flag of an exhausted source
 static constexpr std::uint32_t kSup = std::uint32_t( 1 ) << 31;

 /// a node: the key and the source of the loser there
 struct Node {
  Key key;
  std::uint32_t src;
  };

 /// the key of a node
 static const T & keyOf( const Node & a );

 /// the largest key of the natural order
 static T supKey( void );

 /// a beats b: a smaller key, or the same key from a lower source
 [[nodiscard]] bool beats( const Node & a , const Node & b ) const;

 /// replays the matches of the leaf of cur up to the root
 void replay( Node cur );

 /// the winner is not beaten by any source, O( K )
 [[nodiscard]] bool winnerIsBest( void ) const;

 std::size_t f_k;                           ///< the sources
 std::size_t f_K;                           ///< the leaves, a power of two
 std::vector< Node > f_node;                ///< 0 the winner, the losers
 std::vector< Node > f_leaf;                ///< the keys of build()
 C f_less;

 };  // end( class FlatLoserTree )

/*--------------------------------------------------------------------------*/

/// the items of the sorted ranges R[ i ] = [ first , last ) merged into
/// out, the range of lower index first on equal items; the end of the
/// output
/// \pre each range sorted by less (an audit)
/// \post the output sorted, with the items of the ranges (an audit)
template< class I , class O , class C = std::less<> >
O multiwayMerge( const std::vector< std::pair< I , I > > & R , O out ,
                 C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/MultiwayMerge.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MultiwayMerge.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File MultiwayMerge.h ------------------------*/
/*--------------------------------------------------------------------------*/
