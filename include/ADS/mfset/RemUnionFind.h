/*--------------------------------------------------------------------------*/
/*-------------------------- File RemUnionFind.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The engineered MFSET, beyond the book [see MFSet.h for the forest of
 * section 10.2 with the union by rank and the compression of the paths]:
 * the union of disjoint sets by the algorithm of Rem with splicing, which
 * the experiments of Patwary, Blair and Manne found the fastest of the
 * sequential algorithms of union and find.
 *
 * The parents are a vector of 32-bit integers, and a parent is never
 * smaller than its child, p[ x ] >= x, the roots having p[ x ] = x: the
 * union by index takes the place of the union by rank, with no array of
 * ranks. merge( x , y ) climbs the two paths from x and y together, always
 * from the one whose parent is smaller: if that one is a root it is linked
 * under the parent of the other one and the two sets are one; otherwise
 * its parent is replaced by the larger parent of the other one (the
 * splicing), which only shortens its path, and the climb goes on from its
 * old parent. When the two climbs reach the same parent, x and y were in
 * the same set. The climb stops as soon as the sets are joined, often well
 * below the roots, and each step both compresses and moves on, so that the
 * two finds and the link of the union by rank become one interleaved walk.
 * find( x ) climbs with the halving of the path, each node on the way made
 * a child of its grandparent, in one pass. The link by index is the
 * naive link, which with the splicing or the halving takes \f$ O( m
 * \log_{1 + m / n} n ) \f$ time for m operations in the worst case
 * (Tarjan and van Leeuwen), and \f$ O( m \alpha( m , n ) ) \f$ expected
 * time, the bound of the union by rank, when the elements are numbered in
 * a random order (Goel, Khanna, Larkin and Tarjan).
 *
 * The contracts [see Contract.h]: the elements are in [ 0 , n ), n <
 * \f$ 2^{32} \f$; find() gives a root and after merge() x and y have the
 * same root; the invariant is that every parent is not smaller than its
 * child and that the roots are count(), an O( n ) check at the level 3 on
 * the sets of up to 4096 elements. The operations are defined in
 * RemUnionFind.tpp.
 *
 * \see M. M. A. Patwary, J. R. S. Blair, F. Manne, Experiments on
 *      union-find algorithms for the disjoint-set data structure,
 *      Proceedings of SEA 2010, LNCS 6049, 411-423, 2010,
 *      doi:10.1007/978-3-642-13193-6_35
 * \see A. Goel, S. Khanna, D. H. Larkin, R. E. Tarjan, Disjoint set union
 *      with randomized linking, Proceedings of SODA 2014, 1005-1017, 2014,
 *      doi:10.1137/1.9781611973402.75
 * \see E. W. Dijkstra, A Discipline of Programming, Prentice-Hall, 1976,
 *      chapter 23 (the algorithm of Rem)
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
 * \see S. Dhulipala, C. Hong, J. Shun, ConnectIt: a framework for static
 *      and incremental parallel graph connectivity algorithms, Proceedings
 *      of the VLDB Endowment 14(4), 653-667, 2020,
 *      doi:10.14778/3436905.3436921
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

#ifndef __ADS_RemUnionFind
 #define __ADS_RemUnionFind
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS RemUnionFind -----------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets of the elements 0 , ... , n - 1
class RemUnionFind {

/*--------------------------------------------------------------------------*/
 public:

 /// the n sets {0} , ... , {n - 1}
 /// \pre 0 <= n < 2^32
 explicit RemUnionFind( std::size_t n = 0 );

 /// the number of the elements
 [[nodiscard]] std::size_t n( void ) const;

 /// the number of the sets
 [[nodiscard]] std::size_t count( void ) const;

 /// the representative of the set of x, the root of its tree
 /// \pre x < n()
 std::uint32_t find( std::uint32_t x );

 /// joins the sets of x and y; true if they were distinct
 /// \pre x < n() and y < n()
 bool merge( std::uint32_t x , std::uint32_t y );

 /// whether x and y are in the same set
 /// \pre x < n() and y < n()
 bool same( std::uint32_t x , std::uint32_t y );

 /// the parent of x, x itself if a root
 [[nodiscard]] std::uint32_t parent( std::uint32_t x ) const;

/*--------------------------------------------------------------------------*/
 private:

 [[nodiscard]] bool forest( void ) const;

 std::vector< std::uint32_t > f_p;   ///< the parents
 std::size_t f_count = 0;            ///< the number of the sets

 };  // end( class RemUnionFind )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/RemUnionFind.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RemUnionFind.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File RemUnionFind.h -------------------------*/
/*--------------------------------------------------------------------------*/
