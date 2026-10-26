/*--------------------------------------------------------------------------*/
/*--------------------------- File LoserTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree of losers of a tournament among k players, the structure of the
 * merge of k sorted sequences (Knuth 5.4.1; the notes, section 5.1.3, use a
 * heap of k pairs for the same purpose).
 *
 * The k players are the leaves of a complete binary tree with k - 1
 * internal nodes, laid out as a heap (the leaf of player i is the node
 * k + i, the parent of node v is v / 2); each internal node keeps the
 * loser of the match played there, and the node 0 keeps the overall
 * winner, the player with the smallest key, the lower index on equal keys,
 * so that a merge through the tree is stable. When the winner gets a new
 * key (the next item of its sequence) or runs out (an exhausted player
 * loses every match), replace() replays only the matches on the path from
 * its leaf to the root, against the losers stored there: exactly
 * \f$ \lceil \log_2 k \rceil \f$ or \f$ \lfloor \log_2 k \rfloor \f$
 * comparisons (a match on equal keys is decided by the indices, with the
 * same single call of less), no more, while a binary heap [see
 * DaryHeap.h] pays up to \f$ 2 \log_2 k \f$ for its pop and push, since
 * each level of its sift down compares the two children and then the item
 * moved down. The construction plays the k - 1 matches once, O( k ).
 *
 * kWayMerge() merges k sorted vectors through the tree, \f$ n \lceil \log_2
 * k \rceil + k \f$ comparisons at most for n items in all; in the external
 * memory the same merge, fed by one block per sequence, is the step of the
 * multiway mergesort [see ExternalSorting.h].
 *
 * The contracts [see Contract.h]: k >= 1; top() and replace() on a tree
 * whose players are not all exhausted; the invariant, that the winner has a
 * key not larger than that of every player, is checked after each
 * replace(); kWayMerge() needs sorted sequences and gives their items
 * sorted, an audit. The functions are defined in LoserTree.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 5
 * \see D. E. Knuth, The Art of Computer Programming, vol. 3, 2nd ed.,
 *      Addison-Wesley, 1998, section 5.4.1
 * \see P. Sanders, Fast priority queues for cached memory, ACM Journal of
 *      Experimental Algorithmics 5, 7, 2000, doi:10.1145/351827.384249
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

#ifndef __ADS_LoserTree
 #define __ADS_LoserTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS LoserTree -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the tree of losers of k players, the winner the smallest key by less
template< class T , class C = std::less<> >
class LoserTree {

/*--------------------------------------------------------------------------*/
 public:

 /// the players with their first keys, an empty key for an exhausted one
 /// \pre at least one player
 explicit LoserTree( std::vector< std::optional< T > > keys ,
                     C less = C() );

 /// k, the players
 [[nodiscard]] std::size_t players( void ) const;

 /// all the players exhausted
 [[nodiscard]] bool empty( void ) const;

 /// the player with the smallest key
 /// \pre ! empty()
 [[nodiscard]] std::size_t winner( void ) const;

 /// its key
 /// \pre ! empty()
 [[nodiscard]] const T & top( void ) const;

 /// the winner takes the key, or is exhausted if it is empty, and the
 /// matches on its path are replayed
 /// \pre ! empty()
 void replace( std::optional< T > key );

/*--------------------------------------------------------------------------*/
 private:

 /// i beats j: a key against none, or a smaller key, or the lower index
 [[nodiscard]] bool beats( std::size_t i , std::size_t j ) const;

 /// the winner is not beaten by any player, O( k )
 [[nodiscard]] bool winnerIsBest( void ) const;

 std::vector< std::optional< T > > f_key;   ///< the keys of the players
 std::vector< std::size_t > f_node;         ///< f_node[ 0 ] the winner,
                                            ///< the others the losers
 C f_less;

 };  // end( class LoserTree )

/// the items of the sorted vectors R, merged through a tree of losers
/// \pre each vector of R sorted
/// \post the items of R, sorted (an audit)
template< class T , class C = std::less<> >
std::vector< T > kWayMerge( const std::vector< std::vector< T > > & R ,
                            C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/priority/impl/LoserTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* LoserTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File LoserTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
