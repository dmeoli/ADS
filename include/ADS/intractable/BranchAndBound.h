/*--------------------------------------------------------------------------*/
/*------------------------- File BranchAndBound.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The branch-&-bound algorithms of section 19.3 and of the exercises 19.9
 * to 19.12: the enumeration of chapter 16 that skips the subtree of a
 * sequence of choices S whose lower bound lb( S ) is not below the cost
 * minCost of the best solution found so far. The worst case stays
 * superpolynomial; the time in practice depends on the starting solution,
 * on the rule of branch, on the bound and on the order of visit.
 *
 * bbTsp() chooses the cities of the circuit one after the other from
 * S[ 0 ] = 0, in increasing order, depth first. The bound of the text is
 * \f[
 *   lb( S ) = C_i + \left\lceil \Big( A + B + \sum_{h \notin S} D_h \Big)
 *             / 2 \right\rceil ,
 * \f]
 * \f$ C_i \f$ the length of the path S[ 0 .. i ], A and B the least
 * distances to S[ 0 ] and from S[ i ] of a city out of S (the text has
 * from S[ 0 ], the same for symmetric distances), \f$ D_h \f$ the
 * least distance into and out of h over all the cities, as in the
 * example of the text (lb = 18 for S = 1 , 3 of Figura 19.5). Two values
 * of Figura 19.6 do not follow from this formula, 21 for 1 2 3 and for 1 5
 * (it gives 19 and 20), and no rule on p and q that I tried gives all the
 * values of the figure; with the formula the tree has 25 nodes instead of
 * 23 and ends with the same circuit of cost 19. The bound of the 1-tree
 * (exercise
 * 19.10) replaces the sum by the weight of a minimum spanning tree of the
 * cities out of S, for the symmetric distances. littleTsp() (exercise
 * 19.11, the algorithm of Little, Murty, Sweeney and Karel) branches on
 * including or excluding an edge, the one of reduced cost 0 whose
 * exclusion raises the bound the most, visits by least bound, and bounds
 * by reducing the rows and the columns of the matrix of the distances to
 * a least entry 0. maxWeightClique() (exercise 19.9) bounds by the weight
 * of the candidates, knapsackBB() (exercise 19.12) by the fractional
 * knapsack of chapter 14.
 *
 * The contracts [see Contract.h]: the distances are square, symmetric for the
 * bound of the 1-tree, the weights and the profits not negative and the
 * volumes positive; each optimum returned is checked as feasible, with its
 * cost. The operations are defined in BranchAndBound.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.3 and
 *      exercises 19.9-19.12,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see J. D. C. Little, K. G. Murty, D. W. Sweeney, C. Karel, An algorithm
 *      for the traveling salesman problem, Operations Research 11(6),
 *      972-989, 1963, doi:10.1287/opre.11.6.972
 * \see M. Held, R. M. Karp, The traveling-salesman problem and minimum
 *      spanning trees: part II, Mathematical Programming 1, 6-25, 1971,
 *      doi:10.1007/BF01584070
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

#ifndef __ADS_BranchAndBound
 #define __ADS_BranchAndBound
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"
#include "ADS/graph/MinimumSpanningTree.h"
#include "ADS/intractable/Approximation.h"
#include "ADS/priority/FibonacciHeapPriorityQueue.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ BOUNDS ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// the lower bound of a circuit that starts with S
enum class TspBound { BOOK , ONE_TREE };

/// the weight of a minimum spanning tree of the nodes in V, by the
/// symmetric distances D
/// \pre D is square and symmetric, and V has distinct cities
template< class T >
T spanningTreeWeight( const std::vector< std::vector< T > > & D ,
                      const Nodes & V );

/// the weight of a minimum 1-tree of r: a minimum spanning tree of the
/// other nodes and the two least edges of r; each circuit is a 1-tree, so
/// that the weight bounds the optimum from below
/// \pre D is square and symmetric, and r is a city
template< class T >
T oneTree( const std::vector< std::vector< T > > & D , int r );

/// lb( S ): a lower bound of the circuits that start with the path S,
/// the cost of the circuit if S has all the cities
/// \pre D is square, S has distinct cities and is not empty, D symmetric for
///      the 1-tree
template< class T >
T tspLowerBound( const std::vector< std::vector< T > > & D , const Nodes & S ,
                 TspBound how = TspBound::BOOK );

/*--------------------------------------------------------------------------*/
/*------------------------------- BBTSP ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// bbTsp(): the cost and a minimum circuit from 0, starting from the
/// circuit start if not empty; the number of nodes of the tree of the
/// choices generated, the root included, goes to *generated, if given
/// \pre D is square, start empty or a permutation, D symmetric for the 1-tree
/// \post a permutation and its cost, if n >= 2
template< class T >
std::pair< T , Nodes > bbTsp( const std::vector< std::vector< T > > & D ,
                              const Nodes & start = {} ,
                              TspBound how = TspBound::BOOK ,
                              long * generated = nullptr );

/*--------------------------------------------------------------------------*/
/*------------------------------ LITTLE ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// littleTsp(): the cost and a minimum circuit from 0, by the reduction
/// of the matrix; the number of nodes of the tree generated goes to
/// *generated, if given
/// \pre D is square
/// \post a permutation and its cost, if n >= 2
template< class T >
std::pair< T , Nodes > littleTsp( const std::vector< std::vector< T > > & D ,
                                  long * generated = nullptr );

/*--------------------------------------------------------------------------*/
/*----------------------- CLIQUE AND KNAPSACK ------------------------------*/
/*--------------------------------------------------------------------------*/

/// CRICCA DI PESO MASSIMO: the weight and the nodes of a clique of G of
/// largest weight, the weights w not negative; a set is extended by the
/// candidates adjacent to all of it, and pruned when its weight and that of
/// all the candidates is not above the best
/// \pre G is unoriented, i.e., square and symmetric, w of size n and not
///      negative
/// \post a clique and its weight
inline std::pair< long , Nodes > maxWeightClique(
                                            const Adjacency & G ,
                                            const std::vector< long > & w ,
                                            long * generated = nullptr );

/// ZAINO by branch-&-bound: the best profit and the objects, by places;
/// the objects by decreasing profit per volume, each in and then out,
/// pruned when the fractional knapsack of the rest does not beat the best
/// \pre p and v of the same size, p not negative, v positive
/// \post objects within C and their profit
inline std::pair< long , Nodes > knapsackBB( const std::vector< long > & p ,
                                             const std::vector< long > & v ,
                                             long C ,
                                             long * generated = nullptr );

/// ZAINO by backtracking with the simple bound: the objects in their
/// order, each in and then out, an object that does not fit skipped, and a
/// node pruned when its profit plus the profits of all the objects still
/// to decide does not beat the best; the nodes generated go to *generated
/// \pre p and v of the same size, p not negative, v positive
/// \post objects within C and their profit
inline std::pair< long , Nodes > knapsackBacktrack(
                                  const std::vector< long > & p ,
                                  const std::vector< long > & v , long C ,
                                  long * generated = nullptr );

/// the greedy heuristic of ZAINO: the objects by decreasing profit per
/// volume, each taken if it still fits; not optimal in general
/// \pre p and v of the same size, p not negative, v positive
/// \post objects within C and their profit
inline std::pair< long , Nodes > knapsackGreedy(
                                  const std::vector< long > & p ,
                                  const std::vector< long > & v , long C );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/impl/BranchAndBound.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BranchAndBound.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File BranchAndBound.h ------------------------*/
/*--------------------------------------------------------------------------*/
