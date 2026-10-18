/*--------------------------------------------------------------------------*/
/*-------------------------- File ListRanking.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The ranking of a list, the distance of each item from the tail, in the
 * memory and in the two-level memory [see TwoLevelMemory.h], following
 * chapter 4 of the notes. A list of n items is the vector succ of their
 * successors, the items being 0 , ... , n - 1 and the tail t the only one
 * with succ[ t ] = t.
 *
 * - listRank() is the sequential algorithm: the predecessors, then a walk
 *   back from the tail, O( n ) time, but up to one I/O per item on the
 *   disk, since the links jump anywhere.
 * - listRankPointerJumping() is the parallel algorithm of Wyllie: every
 *   item at once adds the rank of its successor to its own and jumps to the
 *   successor of its successor, so that the rank of i is the distance
 *   between i and its current successor; \f$ \lceil \log_2 n \rceil \f$
 *   rounds, \f$ O( n \log n ) \f$ operations (Lemma 4.1).
 * - parallelStep() simulates one parallel step \f$ A[ a_i ] \leftarrow
 *   op( A[ a_i ] , A[ b_i ] ) \f$ on the disk by two sorts and three scans
 *   (Theorem 4.1): the triples \f$ \langle a_i , b_i , \cdot \rangle \f$
 *   sorted by \f$ b_i \f$, filled in by a scan of A, sorted back by
 *   \f$ a_i \f$ and applied by a second scan of A; any parallel algorithm of
 *   T steps on n processors becomes \f$ \tilde O( ( n / B ) T ) \f$ I/Os
 *   (Theorem 4.3, of Chiang et al.). externalPointerJumping() is the
 *   algorithm of Wyllie so simulated, the rank and the successor carried in
 *   one record, \f$ \tilde O( ( n / B ) \log n ) \f$ I/Os (Theorem 4.2).
 * - The divide and conquer of section 4.3 removes an independent set I
 *   (no item of I is the successor of another one of I), letting the
 *   predecessors of its items jump over them, ranks the shorter list and
 *   puts the items of I back, each one adding the rank of its successor.
 *   randomIndependentSet() tosses a coin per item and takes the heads whose
 *   successor shows tails, n / 4 items on average; deterministicCoinTossing()
 *   is the technique of Cole and Vishkin: the colors start as the distinct
 *   indices and each round replaces the color of i by \f$ 2 \pi + z \f$,
 *   \f$ \pi \f$ the lowest bit where it differs from the color of the
 *   successor and z that bit of i (the tail compares with its bit 0), so
 *   that adjacent colors stay different while b bits become \f$ \lceil
 *   \log_2 b \rceil + 1 \f$, down to six colors in \f$ \log^* n \f$ rounds,
 *   and three by recoloring 5, 4 and 3; independentSetOfColors() takes the
 *   local minima, at least about n / 4 items. listRankIndependentSet() is
 *   the recursion in the memory with either set, and externalListRank()
 *   is the same recursion on the disk with the random coins, each step a
 *   join of records by sorts and scans, \f$ T( n ) = \tilde O( n / B ) + T(
 *   7 n / 8 ) = \tilde O( n / B ) \f$ I/Os (Theorem 4.5; the coins are
 *   tossed again until \f$ | I | \geq n / 8 \f$, which happens after O( 1 )
 *   attempts on average), the list in memory once it fits.
 *
 * Compared with the literature: the optimal parallel rankings of Cole and
 * Vishkin and of Anderson and Miller reach O( n ) operations in O( log n )
 * time by the same independent sets; on the disk the bound \f$ \Theta(
 * \mathrm{sort}( n ) ) \f$ of Chiang et al. is optimal, and is what
 * externalListRank() attains, while the deterministic recursion on the
 * disk would repeat the joins of the coins \f$ \log^* n \f$ times per
 * level, and is here only in the memory. The records of the disk are
 * atomic items of the model.
 *
 * The contracts [see Contract.h]: succ is a list, one tail and every item
 * reached from the only head, an audit; the ops of a parallel step have
 * distinct destinations and the indices are in A, an audit; the colors of
 * adjacent items differ and the independent sets are independent, audits;
 * the files share the memory, which is at least 3 B, and the outputs are
 * empty. The functions are defined in ListRanking.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 4
 * \see J. C. Wyllie, The complexity of parallel computations, PhD thesis,
 *      Cornell University, 1979
 * \see R. Cole, U. Vishkin, Deterministic coin tossing with applications
 *      to optimal parallel list ranking, Information and Control 70(1),
 *      32-53, 1986, doi:10.1016/S0019-9958(86)80023-7
 * \see R. J. Anderson, G. L. Miller, Deterministic parallel list ranking,
 *      Algorithmica 6, 859-868, 1991, doi:10.1007/BF01759076
 * \see Y.-J. Chiang, M. T. Goodrich, E. F. Grove, R. Tamassia, D. E.
 *      Vengroff, J. S. Vitter, External-memory graph algorithms,
 *      Proceedings of the 6th ACM-SIAM Symposium on Discrete Algorithms
 *      (SODA), 139-149, 1995
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

#ifndef __ADS_ListRanking
 #define __ADS_ListRanking
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/external/ExternalSorting.h"
#include "ADS/external/TwoLevelMemory.h"
#include "ADS/search/BinarySearch.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// succ is a list: one tail t with succ[ t ] = t, and every item reached
/// from the only item that is no successor
[[nodiscard]] inline bool isList( const std::vector< std::size_t > & succ );

/// the distances from the tail, by the predecessors and a walk back
/// \pre succ is a list (an audit)
[[nodiscard]] inline std::vector< std::size_t > listRank(
                                    const std::vector< std::size_t > & succ );

/// the distances from the tail, by pointer jumping; the rounds go to
/// *rounds, if given
/// \pre succ is a list (an audit)
[[nodiscard]] inline std::vector< std::size_t > listRankPointerJumping(
                                    const std::vector< std::size_t > & succ ,
                                    std::size_t * rounds = nullptr );

/// the heads whose successor shows tails, a coin per item drawn by g
/// \pre succ is a list (an audit)
/// \post an independent set, without the tail (an audit)
template< class R >
std::vector< bool > randomIndependentSet(
                          const std::vector< std::size_t > & succ , R & g );

/// three colors in { 0 , 1 , 2 }, by deterministic coin tossing; the
/// rounds down to six colors go to *rounds, if given
/// \pre succ is a list (an audit)
/// \post adjacent items have different colors (an audit)
[[nodiscard]] inline std::vector< int > deterministicCoinTossing(
                                    const std::vector< std::size_t > & succ ,
                                    std::size_t * rounds = nullptr );

/// the items whose color is smaller than those of their neighbours, the
/// tail excluded and not counted as a neighbour, so that a list of two
/// items or more gives at least one
/// \pre adjacent items have different colors
/// \post an independent set (an audit)
[[nodiscard]] inline std::vector< bool > independentSetOfColors(
                                    const std::vector< std::size_t > & succ ,
                                    const std::vector< int > & color );

/// the distances from the tail, by removing independent sets, random ones
/// drawn by g or, if deterministic, the local minima of the three colors;
/// the levels of the recursion go to *levels, if given
/// \pre succ is a list (an audit)
/// \post the ranks of listRank() (an audit)
template< class R >
std::vector< std::size_t > listRankIndependentSet(
                                    const std::vector< std::size_t > & succ ,
                                    R & g , bool deterministic = false ,
                                    std::size_t * levels = nullptr );

/// out[ i ] = op( A[ i ] , A[ b ] ) for each pair ( i , b ) of ops, out[ i ]
/// = A[ i ] for the i of no pair, the values of A read before any change;
/// two sorts and three scans
/// \pre the first components of ops distinct (an audit), all indices
///      below the size of A, out empty, the files on the same memory
template< class T , class Op >
void parallelStep( const DiskFile< T > & A ,
                   const DiskFile< std::pair< std::size_t , std::size_t > > &
                                                                         ops ,
                   Op op , DiskFile< T > & out );

/// the ranks of the list succ into rank, by pointer jumping, each round a
/// parallelStep(); the rounds go to *rounds, if given
/// \pre succ is a list (an audit), M >= 3 B, rank empty, the files on the
///      same memory
/// \post the ranks of listRank() (an audit)
inline void externalPointerJumping( const DiskFile< std::size_t > & succ ,
                                    DiskFile< std::size_t > & rank ,
                                    std::size_t * rounds = nullptr );

/// the ranks of the list succ into rank, by removing random independent
/// sets drawn by g; the levels of the recursion go to *levels, if given
/// \pre succ is a list (an audit), M >= 3 B, rank empty, the files on the
///      same memory
/// \post the ranks of listRank() (an audit)
template< class R >
void externalListRank( const DiskFile< std::size_t > & succ ,
                       DiskFile< std::size_t > & rank , R & g ,
                       std::size_t * levels = nullptr );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/ListRanking.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* ListRanking.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ListRanking.h -------------------------*/
/*--------------------------------------------------------------------------*/
