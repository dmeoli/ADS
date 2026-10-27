/*--------------------------------------------------------------------------*/
/*--------------------------- File LoserTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tree of losers of a tournament among \f$ k \f$ players, the structure
 * of the merge of \f$ k \f$ sorted sequences (Knuth 5.4.1; the notes,
 * section 5.1.3, use a heap of \f$ k \f$ pairs for the same purpose).
 *
 * The \f$ k \f$ players are the leaves of a complete binary tree with
 * \f$ k - 1 \f$ internal nodes, laid out as a heap (the leaf of the player
 * \f$ i \f$ is the node \f$ k + i \f$, the parent of the node \f$ v \f$ is
 * \f$ \lfloor v / 2 \rfloor \f$); each internal node keeps the loser of the
 * match played there, and the node 0 keeps the overall winner, the player
 * with the smallest key, the lower index on equal keys, so that a merge
 * through the tree is stable. When the winner gets a new key (the next item
 * of its sequence) or runs out (an exhausted player loses every match),
 * `replace()` replays only the matches on the path from its leaf to the
 * root, against the losers stored there: exactly
 * \f$ \lceil \log_2 k \rceil \f$ or \f$ \lfloor \log_2 k \rfloor \f$
 * comparisons (a match on equal keys is decided by the indices, with the
 * same single call of `less`), no more, while a binary heap [see
 * `DaryHeap.h`] pays up to \f$ 2 \log_2 k \f$ for its pop and push, since
 * each level of its sift down compares the two children and then the item
 * moved down. The construction plays the \f$ k - 1 \f$ matches once,
 * \f$ O( k ) \f$.
 *
 * `kWayMerge()` merges \f$ k \f$ sorted vectors through the tree,
 * \f$ n \lceil \log_2 k \rceil + k \f$ comparisons at most for \f$ n \f$
 * items in all; in the external memory the same merge, fed by one block per
 * sequence, is the step of the multiway mergesort [see
 * `ExternalSorting.h`].
 *
 * The contracts [see `Contract.h`]: \f$ k \ge 1 \f$; `top()` and
 * `replace()` on a tree whose players are not all exhausted; the invariant,
 * that the winner has a key not larger than that of every player, is
 * checked after each `replace()`; `kWayMerge()` needs sorted sequences and
 * gives their items sorted, an audit. The functions are defined in
 * `LoserTree.tpp`.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 5, Sorting atomic items,
 *      doi:10.1017/9781009128933
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

/// the tree of losers of \f$ k \f$ players, the winner the smallest key by
/// `less`
template< class T , class C = std::less<> >
class LoserTree {

/*--------------------------------------------------------------------------*/
 public:

 /// the tree of the players with their first keys, an empty key for an
 /// exhausted one, its \f$ k - 1 \f$ matches played in \f$ O( k ) \f$ time
 ///
 /// In pseudocode (Knuth 5.4.1, beyond the book), \f$ L[ v ] \f$ the loser
 /// at the node \f$ v \f$ and \f$ W[ v ] \f$ the winner below it, the leaf
 /// \f$ k + i \f$ the player \f$ i \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{build}( \mathit{key}[ 0 .. k - 1 ] )
 ///     \cr
 /// 2 & \quad \textbf{for}\ v \gets k - 1\ \textbf{downto}\ 1
 ///     \ \textbf{do} \cr
 /// 3 & \qquad a \gets \text{the player of the node } 2 v;\ b \gets
 ///     \text{the player of the node } 2 v + 1 \cr
 /// 4 & \qquad \textbf{if}\ \mathrm{beats}( a , b )\ \textbf{then}\ W[ v
 ///     ] \gets a;\ L[ v ] \gets b\ \textbf{else}\ W[ v ] \gets b;\ L[ v ]
 ///     \gets a \cr
 /// 5 & \quad L[ 0 ] \gets W[ 1 ]\ \text{if } k > 1 \text{, else } 0 \cr
 /// 6 & \textbf{function}\ \mathrm{beats}( i , j ) \cr
 /// 7 & \quad \textbf{if}\ i \text{ is exhausted}\ \textbf{then}
 ///     \textbf{return false} \cr
 /// 8 & \quad \textbf{if}\ j \text{ is exhausted}\ \textbf{then}
 ///     \textbf{return true} \cr
 /// 9 & \quad \textbf{return}\ \mathit{key}[ i ] < \mathit{key}[ j ]
 ///     \ \textbf{or}\ ( \mathit{key}[ i ] = \mathit{key}[ j ]
 ///     \ \textbf{and}\ i < j )
 /// \end{array}}
 /// \f]
 /// \pre at least one player
 explicit LoserTree( std::vector< std::optional< T > > keys ,
                     C less = C() );

 /// the number of players, \f$ k \f$
 [[nodiscard]] std::size_t players( void ) const;

 /// whether all the players are exhausted
 [[nodiscard]] bool empty( void ) const;

 /// the player with the smallest key
 /// \pre not `empty()`
 [[nodiscard]] std::size_t winner( void ) const;

 /// the key of the winner
 /// \pre not `empty()`
 [[nodiscard]] const T & top( void ) const;

 /// the winner takes the key, or is exhausted if it is empty, and the
 /// matches on its path are replayed, \f$ \lceil \log_2 k \rceil \f$
 /// comparisons at most
 ///
 /// In pseudocode (Knuth 5.4.1, beyond the book), with
 /// \f$ \mathrm{beats}() \f$ of the constructor:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{replace}( x ) \cr
 /// 2 & \quad w \gets L[ 0 ];\ \mathit{key}[ w ] \gets x \cr
 /// 3 & \quad v \gets \lfloor ( w + k ) / 2 \rfloor \cr
 /// 4 & \quad \textbf{while}\ v \ge 1\ \textbf{do} \cr
 /// 5 & \qquad \textbf{if}\ \mathrm{beats}( L[ v ] , w )\ \textbf{then}
 ///     \ \text{swap } w \text{ and } L[ v ] \cr
 /// 6 & \qquad v \gets \lfloor v / 2 \rfloor \cr
 /// 7 & \quad L[ 0 ] \gets w
 /// \end{array}}
 /// \f]
 /// \pre not `empty()`
 void replace( std::optional< T > key );

/*--------------------------------------------------------------------------*/
 private:

 /// i beats j: a key against none, or a smaller key, or the lower index
 [[nodiscard]] bool beats( std::size_t i , std::size_t j ) const;

 /// the winner is not beaten by any player, \f$ O( k ) \f$
 [[nodiscard]] bool winnerIsBest( void ) const;

 std::vector< std::optional< T > > f_key;   ///< the keys of the players
 std::vector< std::size_t > f_node;         ///< f_node[ 0 ] the winner,
                                            ///< the others the losers
 C f_less;

 };  // end( class LoserTree )

/// the items of the sorted vectors of \f$ R \f$, merged through a tree of
/// losers
///
/// In pseudocode (Knuth 5.4.1, beyond the book), \f$ R_0 , \ldots ,
/// R_{k-1} \f$ the vectors:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{kWayMerge}( R ) \cr
/// 2 & \quad T \gets \text{the tree of losers of the first items of }
///     R_0 , \ldots , R_{k-1} \text{, none for an empty one} \cr
/// 3 & \quad \mathit{out} \gets \text{an empty vector} \cr
/// 4 & \quad \textbf{while not}\ \mathrm{empty}( T )\ \textbf{do} \cr
/// 5 & \qquad w \gets \mathrm{winner}( T );\ \text{append }
///     \mathrm{top}( T ) \text{ to } \mathit{out} \cr
/// 6 & \qquad \mathrm{replace}( T , \text{the next item of } R_w
///     \text{, none if it is over} ) \cr
/// 7 & \quad \textbf{return}\ \mathit{out}
/// \end{array}}
/// \f]
/// \pre each vector of \f$ R \f$ sorted
/// \post the items of \f$ R \f$, sorted (an audit)
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
