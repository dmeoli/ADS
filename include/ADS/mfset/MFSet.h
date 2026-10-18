/*--------------------------------------------------------------------------*/
/*------------------------------ File MFSet.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The MFSET of section 10.2, the union of disjoint sets: Mfset( n ) makes
 * the n components {0} , ... , {n - 1}, find( x ) gives the representative
 * of the component of x, merge( x , y ) joins the components of x and y,
 * if they are distinct.
 *
 * MFSet keeps the components as a forest, a vector of parents p, the roots
 * with p[ x ] = x being the representatives (section 10.2.2): find() goes
 * up to the root, merge() makes a root a child of the other. Two heuristics
 * may be switched off, to study them:
 *
 * - the union by rank (section 10.2.3): rank[ r ] is the height of the
 *   tree of the root r, the lower tree goes under the higher one and the
 *   rank grows by one only when they are equal; by the Teorema 10.1 a tree
 *   of root r has at least \f$ 2^{rank[ r ]} \f$ nodes, so find() and
 *   merge() take O( log n );
 * - the union by size, in place of the rank: size[ r ] is the number of
 *   nodes of the tree of the root r, and the smaller tree goes under the
 *   larger one (on a tie, the root of y under that of x). A node goes one
 *   level down only when its tree at least doubles, hence a tree of root r
 *   has height at most \f$ \log_2 size[ r ] \f$, and the bound O( log n )
 *   is the same;
 * - the compression of the paths (section 10.2.4): find() makes each node
 *   on its way a child of the root. With both, a sequence of m operations,
 *   \f$ m \geq n \f$, takes \f$ O( n + m \alpha( m , n ) ) \f$, where
 *   \f$ \alpha \f$, the inverse of the function of Ackermann, is at most 4
 *   for every n and m that fit in a computer. Then rank[ r ] is an upper
 *   bound to the height (exercise 10.7).
 *
 * find() is the iterative version in two passes, the root first and then
 * the compression, with no stack (exercise 10.8). ListMFSet.h has
 * the realization on lists of section 10.2.1. components() gives
 * the MFSET of the connected components of an unoriented graph, which can
 * then follow new edges (section 10.2.5).
 *
 * The contracts [see Contract.h]: the elements are in [ 0 , n ), find()
 * gives a root and after merge() x and y have the same root; the invariant
 * is that the parents form a forest of count() trees, the rank growing
 * towards the roots with the union by rank, and size[ r ] the number of
 * nodes of the tree of each root r with the union by size. merge() of two
 * elements already in the same component returns false and changes
 * nothing, a relaxation of the precondition that they are in distinct
 * components which some specifications state. The operations are defined
 * in MFSet.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 10.2,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf
 * \see B. A. Galler, M. J. Fischer, An improved equivalence algorithm,
 *      Communications of the ACM 7(5), 301-303, 1964,
 *      doi:10.1145/364099.364331
 * \see R. E. Tarjan, Efficiency of a good but not linear set union
 *      algorithm, Journal of the ACM 22(2), 215-225, 1975,
 *      doi:10.1145/321879.321884
 * \see R. E. Tarjan, J. van Leeuwen, Worst-case analysis of set union
 *      algorithms, Journal of the ACM 31(2), 245-281, 1984,
 *      doi:10.1145/62.2160
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

#ifndef __ADS_MFSet
 #define __ADS_MFSet
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/Graph.h"

#include <stdexcept>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS MFSet -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the union of disjoint sets on a forest
class MFSet {

/*--------------------------------------------------------------------------*/
 public:

 /// how merge() chooses the root that goes under the other one
 enum class Union {
  NONE ,   ///< the root of y under that of x
  RANK ,   ///< the lower tree under the higher one
  SIZE     ///< the smaller tree under the larger one
  };

 /// Mfset( n ): the n components {0} , ... , {n - 1}, with the given union
 /// and with the compression of the paths or not
 /// \pre n >= 0
 explicit MFSet( int n , Union u = Union::RANK , bool compress = true );

 [[nodiscard]] int n( void ) const;

 /// the number of components
 [[nodiscard]] int count( void ) const;

 /// the representative of the component of x, the root of its tree
 /// \pre x in [ 0 , n )
 /// \post the result is a root
 int find( int x );

 /// joins the components of x and y; true if they were distinct
 /// \pre x , y in [ 0 , n )
 /// \post x and y are in the same component
 bool merge( int x , int y );

 /// true if x and y are in the same component, find( x ) == find( y )
 /// \pre x , y in [ 0 , n )
 bool same( int x , int y );

 /// the parent of x, x itself if a root
 /// \pre x in [ 0 , n )
 [[nodiscard]] int parent( int x ) const;

 /// the rank of x
 /// \pre x in [ 0 , n )
 [[nodiscard]] int rank( int x ) const;

 /// the number of nodes of the tree of x, if x is a root and the union is
 /// by size
 /// \pre x in [ 0 , n )
 [[nodiscard]] int size( int x ) const;

 /// the steps up the trees made by find() so far
 [[nodiscard]] long steps( void ) const;

/*--------------------------------------------------------------------------*/
 private:

 /// the root of x, without compressing the path
 [[nodiscard]] int root( int x ) const;

 /// the parents form a forest of count() trees, with the rank growing
 /// upwards if the heuristic is on, O( n )
 [[nodiscard]] bool forest( void ) const;

 std::vector< int > f_p;      ///< the parents
 std::vector< int > f_rank;   ///< the ranks
 std::vector< int > f_size;   ///< the sizes of the trees, at the roots
 Union f_union;               ///< the heuristic of the union
 bool f_compress;             ///< the compression of the paths
 int f_count;                 ///< the number of components
 long f_steps = 0;            ///< see steps()

 };  // end( class MFSet )

/*--------------------------------------------------------------------------*/

/// the MFSET of the connected components of an unoriented graph, built by
/// merging the ends of each edge (the cc() of section 10.2.5); its
/// elements are the names [ 0 , g.bound() ), so that a name that is not a
/// node of g, a deleted one, is a component of its own, counted by count()
template< Graph G >
MFSet components( const G & g );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/mfset/impl/MFSet.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* MFSet.h included */

/*--------------------------------------------------------------------------*/
/*---------------------------- End File MFSet.h ----------------------------*/
/*--------------------------------------------------------------------------*/
