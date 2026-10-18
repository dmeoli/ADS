/*--------------------------------------------------------------------------*/
/*----------------------------- File BVGraph.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The compression of the adjacency lists of Boldi and Vigna (section 15.3.1
 * of the notes), designed for the Web graphs, whose nodes are the pages
 * sorted by URL: locality (most links stay in the host, so that the
 * successors of x are close to x and to each other) and similarity (the
 * pages of a host link to the same pages). The list of x is written as:
 *
 * - its outdegree, in \f$ \gamma \f$ [see IntegerCodes.h];
 * - the reference r: the list of x - r, among the previous W lists (the
 *   window), that shares the most successors with that of x, 0 for none;
 * - the copy list of the reference, a bit per successor of x - r telling
 *   whether it is a successor of x too, written as copy blocks: its first
 *   bit, then the lengths less 1 of its runs, the last one omitted since the
 *   outdegree of the reference tells it (Tables 15.5 and 15.6);
 * - the extra nodes, the successors not copied: the runs of at least
 *   \f$ L_{min} \f$ consecutive integers as intervals, the left extreme as
 *   a gap from the previous right extreme less 2 (the first one from x, by
 *   signedToNatural()) and the length less \f$ L_{min} \f$, then the
 *   residuals as gaps less 1 (the first one from x, by signedToNatural()),
 *   in \f$ \delta \f$ (Table 15.7).
 *
 * A list is decoded from its place in the stream, kept for every node in an
 * Elias-Fano sequence [see EliasFano.h], after the list of its reference;
 * the chains of references are at most R long, so that a list is decoded in
 * \f$ O( R d ) \f$ time for outdegrees up to d. With the window W = 7, the
 * chains R = 3 and \f$ L_{min} = 4 \f$ (the defaults of the WebGraph
 * framework) the Web graphs take about 3 bits per edge, against the
 * \f$ \log_2 n \f$ bits of a plain list and the \f$ 2 + \log ( n^2 / m ) \f$
 * of Elias-Fano [see EliasFanoGraph.h]; the \f$ k^2 \f$-tree [see
 * K2Tree.h] is comparable in space and also gives the predecessors. The
 * WebGraph framework codes the gaps by the \f$ \zeta \f$ codes, tuned to
 * their power-law distribution, and reorders the nodes (e.g., by layered
 * label propagation) to make locality stronger; neither is in the notes.
 *
 * copyList(), copyBlocks(), intervals() and residualGaps() are the steps of
 * the coding, given for the examples of the notes.
 *
 * The contracts [see Contract.h]: each list is strictly increasing (an
 * audit) with its nodes in [ 0 , n ), the window and the chains are at
 * least 1 and \f$ L_{min} \ge 2 \f$; the audit of the construction is that
 * each list is decoded back. The functions are defined in BVGraph.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
 * \see P. Boldi, S. Vigna, The WebGraph framework I: compression
 *      techniques, Proc. 13th WWW, 595-602, 2004,
 *      doi:10.1145/988672.988752
 * \see P. Boldi, M. Rosa, M. Santini, S. Vigna, Layered label propagation:
 *      a multiresolution coordinate-free ordering for compressing social
 *      networks, Proc. 20th WWW, 587-596, 2011,
 *      doi:10.1145/1963405.1963488
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

#ifndef __ADS_BVGraph
 #define __ADS_BVGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/compression/IntegerCodes.h"
#include "ADS/succinct/BitStream.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------------- CLASS BVGraph ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a static directed graph with the lists compressed by reference, copy
/// blocks, intervals and gaps
class BVGraph {

/*--------------------------------------------------------------------------*/
 public:

 using List = std::vector< std::size_t >;

 /// an interval of extra nodes, its left extreme and its length
 using Interval = std::pair< std::size_t , std::size_t >;

 BVGraph( void ) = default;

 /// the graph of the adjacency lists a, with the window w, chains of
 /// references at most r long and intervals at least lmin long
 /// \pre each a[ u ] strictly increasing in [ 0 , n ), w , r >= 1 and
 ///      lmin >= 2
 explicit BVGraph( const std::vector< List > & a , std::size_t w = 7 ,
                   std::size_t r = 3 , std::size_t lmin = 4 );

 /// the number of nodes, n
 [[nodiscard]] std::size_t order( void ) const;

 /// the number of edges, m
 [[nodiscard]] std::size_t edges( void ) const;

 /// \pre x < order()
 [[nodiscard]] std::size_t outdegree( std::size_t x ) const;

 /// the successors of x, in increasing order
 /// \pre x < order()
 [[nodiscard]] List successors( std::size_t x ) const;

 /// the reference of x, 0 for none
 /// \pre x < order()
 [[nodiscard]] std::size_t reference( std::size_t x ) const;

 /// the bits of the lists
 [[nodiscard]] std::size_t codeBits( void ) const;

 /// the bits of the lists and of their places
 [[nodiscard]] std::size_t sizeInBits( void ) const;

 /// the copy list of s against the reference list ref: a bit per item of
 /// ref, 1 if it is in s
 [[nodiscard]] static BitVector copyList( const List & s , const List & ref );

 /// the copy blocks of the copy list c: its first bit and the lengths less
 /// 1 of its runs but the last
 [[nodiscard]] static std::pair< bool , std::vector< std::size_t > >
 copyBlocks( const BitVector & c );

 /// the runs of at least lmin consecutive integers of the strictly
 /// increasing e, and the residuals
 [[nodiscard]] static std::pair< std::vector< Interval > , List >
 intervals( const List & e , std::size_t lmin );

 /// the residuals res of x as written: the first as signedToNatural() of
 /// its difference from x, the others as the gaps less 1
 [[nodiscard]] static std::vector< std::uint64_t > residualGaps(
                                         const List & res , std::size_t x );

/*--------------------------------------------------------------------------*/
 private:

 /// the list of x, decoded from the stream after that of its reference
 [[nodiscard]] List decode( std::size_t x ) const;

 std::size_t f_n = 0;
 std::size_t f_m = 0;
 std::size_t f_lmin = 4;
 BitVector f_bits;           ///< the lists, one after the other
 EliasFano f_start;          ///< the place of each list, and the end

 };  // end( class BVGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/BVGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BVGraph.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File BVGraph.h --------------------------*/
/*--------------------------------------------------------------------------*/
