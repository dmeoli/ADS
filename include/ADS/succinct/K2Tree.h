/*--------------------------------------------------------------------------*/
/*----------------------------- File K2Tree.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The \f$ k^2 \f$-tree of Brisaboa, Ladra and Navarro (section 15.3.3 of the
 * notes): the adjacency matrix of a graph of n nodes, padded to the side
 * \f$ N = k^h \ge n \f$, is split into \f$ k^2 \f$ submatrices of side
 * N / k, each one a child of the root marked 1 if it has an edge and 0
 * otherwise, and each child marked 1 is split again, down to the cells. The
 * marks are written by levels: T has those of all the levels but the last,
 * L those of the cells, and with rank on T [see RankSelect.h] the children
 * of the node marked at the place p of T start at the place
 * \f$ rank_1( p + 1 ) k^2 \f$ of T followed by L, as in LOUDS [see
 * LoudsTree.h]: no pointer is stored.
 *
 * neighbors( u ) walks down the children that cross the row u (k of the
 * \f$ k^2 \f$ at each level) and reverseNeighbors( v ) those that cross the
 * column v, so that the same bits give the successors and the predecessors,
 * which the lists give only by doubling them; hasEdge( u , v ) follows one
 * path, in \f$ O( h ) \f$ time. A row is read in \f$ O( \sqrt{m} ) \f$ time on
 * average for m edges spread uniformly, \f$ O( n ) \f$ in the worst case
 * (Theorem 15.8), and the tree takes \f$ k^2 m ( \log_{k^2} ( n^2 / m ) + O( 1
 * ) ) \f$ bits in the worst case (Theorem 15.9), far fewer when the edges are
 * clustered, as in the Web graphs ordered by URL, where the submatrices of
 * zeros are large. A larger k gives fewer levels and faster queries, a smaller
 * one fewer bits; the authors use a larger k at the top levels and a smaller
 * one below, and the construction here works from the edges, by levels, in
 * \f$ O( m h ) \f$ time, without the matrix that the recursive construction of
 * the notes (Algorithm 15.3) visits whole.
 *
 * The contracts [see Contract.h]: \f$ k \ge 2 \f$, the edges join nodes in
 * [ 0 , n ), and the nodes asked for exist; the postcondition of the
 * construction is that the tree has as many cells marked as distinct edges.
 * The functions are defined in K2Tree.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
 * \see N. R. Brisaboa, S. Ladra, G. Navarro, k2-trees for compact web graph
 *      representation, Proc. 16th SPIRE, LNCS 5721, 18-30, 2009,
 *      doi:10.1007/978-3-642-03784-9_3
 * \see N. R. Brisaboa, S. Ladra, G. Navarro, Compact representation of Web
 *      graphs with extended functionality, Information Systems 39, 152-174,
 *      2014, doi:10.1016/j.is.2013.08.003
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

#ifndef __ADS_K2Tree
 #define __ADS_K2Tree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/RankSelect.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS K2Tree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the adjacency matrix of a static directed graph as a k^2-tree
class K2Tree {

/*--------------------------------------------------------------------------*/
 public:

 using Edge = std::pair< std::size_t , std::size_t >;

 K2Tree( void ) = default;

 /// the graph of n nodes and the edges e (repeated ones counted once)
 /// \pre k >= 2 and the nodes of the edges in [ 0 , n )
 /// \post the cells marked are the distinct edges
 K2Tree( std::size_t n , const std::vector< Edge > & e , std::size_t k = 2 );

 /// the number of nodes, n
 [[nodiscard]] std::size_t order( void ) const;

 /// the number of distinct edges
 [[nodiscard]] std::size_t edges( void ) const;

 /// the successors of u, in increasing order
 /// \pre u < order()
 [[nodiscard]] std::vector< std::size_t > neighbors( std::size_t u ) const;

 /// the predecessors of v, in increasing order
 /// \pre v < order()
 [[nodiscard]] std::vector< std::size_t > reverseNeighbors(
                                                       std::size_t v ) const;

 /// whether ( u , v ) is an edge
 /// \pre u , v < order()
 [[nodiscard]] bool hasEdge( std::size_t u , std::size_t v ) const;

 /// T, the marks of the levels but the last
 [[nodiscard]] const BitVector & levels( void ) const;

 /// L, the marks of the cells
 [[nodiscard]] const BitVector & leaves( void ) const;

 /// the bits of T, L and the rank directory of T
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the mark at the place p of T followed by L
 [[nodiscard]] bool mark( std::size_t p ) const;

 /// the place of the first child of the node marked at p, 0 for the root
 [[nodiscard]] std::size_t children( std::size_t p ) const;

 /// the cells of the row ( or the column, if ! row ) r of the submatrix
 /// of side s whose children start at base, offset by o
 void walk( std::size_t s , std::size_t r , std::size_t o ,
            std::size_t base , bool row ,
            std::vector< std::size_t > & out ) const;

 std::size_t f_n = 0;
 std::size_t f_k = 2;
 std::size_t f_side = 1;      ///< N = k^h, the side of the padded matrix
 std::size_t f_m = 0;
 RankSelect f_t;              ///< T
 BitVector f_l;               ///< L

 };  // end( class K2Tree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/K2Tree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* K2Tree.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File K2Tree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
