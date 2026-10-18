/*--------------------------------------------------------------------------*/
/*-------------------------- File EliasFanoGraph.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A static directed graph whose adjacency lists are coded by Elias-Fano
 * [see EliasFano.h] (section 15.3.2 of the notes), the representation of
 * the adjacency lists of large graphs in Facebook's Unicorn and in the
 * quasi-succinct indices of Vigna. The edge \f$ ( u , v ) \f$ of a graph of
 * n nodes is the integer \f$ u n + v \f$, so that the lists, each one
 * sorted, are one increasing sequence in \f$ [ 0 , n^2 ) \f$, and the
 * prefix sums of the outdegrees are a second sequence that tells where the
 * list of u starts; with m edges the graph takes about
 * \f$ m ( 2 + \lceil \log_2 ( n^2 / m ) \rceil ) + n ( 2 + \lceil \log_2 ( m
 * / n ) \rceil ) \f$ bits, plus the directories, and answers in O( 1 ) time
 * the outdegree and the i-th successor of a node, and the existence of an
 * edge by nextGEQ() in O( 1 ) expected time.
 *
 * The notes code each list on its own, with its own universe: since
 * \f$ \sum_u d_u \log ( n / d_u ) \le m \log ( n^2 / m ) \f$ by the concavity
 * of the logarithm, the lists alone can take fewer bits when the degrees are
 * skewed, at the price of a directory and an offset per list; the single
 * sequence here keeps one directory and the same O( 1 ) access. Neither
 * exploits the locality and the similarity of the lists of the Web, which
 * the codes of Boldi and Vigna do [see BVGraph.h], nor the clustering of the
 * adjacency matrix, which the \f$ k^2 \f$-tree does [see K2Tree.h].
 *
 * The contracts [see Contract.h]: each list is strictly increasing (an
 * audit) with its nodes in [ 0 , n ), \f$ n^2 \f$ fits in 64 bits, and the
 * nodes and the places asked for exist. The functions are defined in
 * EliasFanoGraph.tpp.
 *
 * \see P. Ferragina, Pearls of Algorithm Engineering, Cambridge University
 *      Press, 2023, chapter 15
 * \see S. Vigna, Quasi-succinct indices, Proc. 6th WSDM, 83-92, 2013,
 *      doi:10.1145/2433396.2433409
 * \see M. Curtiss et al., Unicorn: a system for searching the social graph,
 *      Proc. VLDB Endowment 6(11), 1150-1161, 2013,
 *      doi:10.14778/2536222.2536239
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

#ifndef __ADS_EliasFanoGraph
 #define __ADS_EliasFanoGraph
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/succinct/EliasFano.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS EliasFanoGraph --------------------------*/
/*--------------------------------------------------------------------------*/

/// a static directed graph with its adjacency lists in Elias-Fano
class EliasFanoGraph {

/*--------------------------------------------------------------------------*/
 public:

 EliasFanoGraph( void ) = default;

 /// the graph of the adjacency lists a, node u having the successors a[ u ]
 /// \pre each a[ u ] is strictly increasing in [ 0 , n ), n^2 < 2^64
 explicit EliasFanoGraph(
                       const std::vector< std::vector< std::size_t > > & a );

 /// the number of nodes, n
 [[nodiscard]] std::size_t order( void ) const;

 /// the number of edges, m
 [[nodiscard]] std::size_t edges( void ) const;

 /// \pre u < order()
 [[nodiscard]] std::size_t outdegree( std::size_t u ) const;

 /// the i-th successor of u, from 0
 /// \pre u < order() and i < outdegree( u )
 [[nodiscard]] std::size_t successor( std::size_t u , std::size_t i ) const;

 /// the successors of u, in increasing order
 /// \pre u < order()
 [[nodiscard]] std::vector< std::size_t > successors( std::size_t u ) const;

 /// whether ( u , v ) is an edge
 /// \pre u , v < order()
 [[nodiscard]] bool hasEdge( std::size_t u , std::size_t v ) const;

 /// the bits of the two sequences, with their directories
 [[nodiscard]] std::size_t sizeInBits( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 std::size_t f_n = 0;
 EliasFano f_edges;          ///< u n + v for each edge ( u , v )
 EliasFano f_start;          ///< where each list starts, and m at the end

 };  // end( class EliasFanoGraph )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/succinct/impl/EliasFanoGraph.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* EliasFanoGraph.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File EliasFanoGraph.h -----------------------*/
/*--------------------------------------------------------------------------*/
