/*--------------------------------------------------------------------------*/
/*----------------------- File OrderMaintenanceList.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The order maintenance of exercise 8.11 of Morin (his `Sequence`), beyond
 * the book: a list of elements with `addAfter( e , v )`, which adds a new
 * element right after \f$ e \f$ (at the front if \f$ e \f$ is nil), in
 * \f$ O( \log n ) \f$ amortized time, `remove( e )`, in \f$ O( 1 ) \f$
 * amortized time, and
 * `testBefore( a , b )`, whether \f$ a \f$ comes before \f$ b \f$, in
 * \f$ O( 1 ) \f$ time.
 *
 * The elements are the nodes of a scapegoat tree [see `ScapegoatTree.h`]
 * in the order of the list, and each node carries a label of 64 bits that
 * encodes its path from the root: the root has the label \f$ 2^{63} \f$,
 * and a node of label \f$ \ell \f$ whose lowest bit set is \f$ 2^j \f$ gives
 * its children \f$ \ell \mp 2^{j-1} \f$, so that the labels increase in
 * symmetric order and `testBefore()` compares two words. A node of depth
 * \f$ d \f$ has its lowest bit at \f$ 2^{63-d} \f$, hence the labels hold
 * any tree of height at most 63; a scapegoat tree with \f$ b = 3 / 2 \f$ has
 * height at most \f$ \log_{3/2} q \f$, below 63 up to
 * \f$ q \approx 1.2 \cdot 10^{11} \f$ nodes. An insertion links a leaf
 * next to \f$ e \f$ and labels it from its parent, unless it is too deep,
 * in which case the scapegoat is found by climbing the parents and its
 * subtree is rebuilt and labeled again, \f$ O( s ) \f$ for its \f$ s \f$
 * nodes; a removal marks the node as dead, since splicing it would move a
 * subtree up and change all its labels, and the whole tree is rebuilt
 * without the dead nodes when they are as many as the living ones. Both
 * costs are paid as in the scapegoat tree, the relabeling being part of the
 * rebuild that a rotation-based tree could not afford (the discussion of
 * chapter 8 of Morin): \f$ O( \log n ) \f$ amortized.
 *
 * Dietz and Sleator, and more simply Bender, Cole, Demaine, Farach-Colton
 * and Zito, give \f$ O( 1 ) \f$ amortized insertions with the same
 * \f$ O( 1 ) \f$ comparison, by labels from a universe of \f$ O( n^2 ) \f$
 * relabeled in ranges of density bounded level by level; this class is the
 * \f$ O( \log n ) \f$ solution that the exercise asks for. An element stays
 * valid until it is removed.
 *
 * The contracts [see `Contract.h`]: the elements given are living elements
 * of this list and the list has fewer than \f$ 10^{11} \f$ nodes; the
 * invariant, an \f$ O( n ) \f$ check at the level 3, is that the labels
 * follow the paths, the parents and the bound on the height. The operations
 * are defined in `OrderMaintenanceList.tpp`.
 *
 * \see P. F. Dietz, D. D. Sleator, Two algorithms for maintaining order in a
 *      list, Proceedings of the 19th ACM Symposium on Theory of Computing
 *      (STOC), 365-372, 1987, doi:10.1145/28395.28434
 * \see M. A. Bender, R. Cole, E. D. Demaine, M. Farach-Colton, J. Zito, Two
 *      simplified algorithms for maintaining order in a list, Algorithms -
 *      ESA 2002, Lecture Notes in Computer Science 2461, 152-164, 2002,
 *      doi:10.1007/3-540-45749-6_17
 * \see I. Galperin, R. L. Rivest, Scapegoat trees, Proceedings of the 4th
 *      ACM-SIAM Symposium on Discrete Algorithms (SODA), 165-174, 1993
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, exercise 8.11, doi:10.15215/aupress/9781927356388.01
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

#ifndef __ADS_OrderMaintenanceList
 #define __ADS_OrderMaintenanceList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/tree/ScapegoatTree.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS OrderMaintenanceList -----------------------*/
/*--------------------------------------------------------------------------*/

/// a list of items of type `T` whose order between two elements is
/// tested in \f$ O( 1 ) \f$ by labels on a scapegoat tree
template< class T >
class OrderMaintenanceList {

 struct Node;

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;    ///< the type of the items

 /// an element of the list, valid until it is removed
 using Element = const Node *;

 /// the empty list
 OrderMaintenanceList( void ) = default;

 /// no copy, since the elements are pointers
 OrderMaintenanceList( const OrderMaintenanceList & ) = delete;

 /// no copy, since the elements are pointers
 OrderMaintenanceList & operator=( const OrderMaintenanceList & ) = delete;

 /// frees the elements
 ~OrderMaintenanceList();

 /// the number \f$ n \f$ of elements
 [[nodiscard]] std::size_t size( void ) const { return( f_live ); }

 /// a new element of \f$ v \f$ right after \f$ e \f$, at the front if
 /// \f$ e \f$ is nil, in \f$ O( \log n ) \f$ amortized time
 ///
 /// In pseudocode (exercise 8.11 of Morin, beyond the book), \f$ \ell \f$
 /// the labels and \f$ \mathrm{low}( x ) \f$ the lowest bit set of
 /// \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{addAfter}( e , v ) \cr
 /// 2 & \quad u \gets \text{a node of } v \text{, the left child of the }
 ///     \text{leftmost node below } e.\mathit{right} \cr
 /// & \qquad \text{or the right child of } e \text{ if it has none} \cr
 /// 3 & \quad q \gets q + 1;\ d \gets \text{the depth of } u \cr
 /// 4 & \quad \textbf{if}\ d > \log_b q\ \textbf{then}\ \mathrm{rebuild}(
 ///     \text{the scapegoat above } u ) \text{, labeling its subtree} \cr
 /// 5 & \quad \textbf{else}\ \ell( u ) \gets \ell( p ) \mp \mathrm{low}(
 ///     \ell( p ) ) / 2 \text{, } p \text{ the parent, } - \text{ for a left}
 ///     \text{ child} \cr
 /// 6 & \quad \textbf{return}\ u
 /// \end{array}}
 /// \f]
 /// \pre \f$ e \f$ is nil or an element of this list
 Element addAfter( Element e , const T & v );

 /// \f$ e \f$ out of the list, in \f$ O( 1 ) \f$ amortized time: marked as
 /// dead, the whole tree rebuilt without the dead nodes when they are as
 /// many as the others
 /// \pre \f$ e \f$ is an element of this list
 void remove( Element e );

 /// whether \f$ a \f$ comes before \f$ b \f$, by their labels, in
 /// \f$ O( 1 ) \f$ time
 /// \pre \f$ a \f$ and \f$ b \f$ are elements of this list
 [[nodiscard]] bool testBefore( Element a , Element b ) const {
  ADS_PRE( a && b && ! a->dead && ! b->dead );
  return( a->label < b->label );
  }

 /// the item of \f$ e \f$
 [[nodiscard]] static const T & value( Element e ) { return( e->item ); }

 /// the first element, nil if the list is empty
 [[nodiscard]] Element first( void ) const;

 /// the element after \f$ e \f$, nil after the last
 [[nodiscard]] Element next( Element e ) const;

 /// the number of rebuilds so far, and the nodes they took
 [[nodiscard]] std::pair< std::size_t , std::size_t > rebuildWork( void )
  const {
  return( std::pair< std::size_t , std::size_t >( f_rebuilds , f_work ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// a node of the tree, an element of the list
 struct Node {
  T item;                       ///< the item
  Node * left = nullptr;        ///< the left child
  Node * right = nullptr;       ///< the right child
  Node * parent = nullptr;      ///< the parent
  std::uint64_t label = 0;      ///< the path from the root
  bool dead = false;            ///< removed, waiting for a rebuild
  };

 static constexpr std::uint64_t kRoot = std::uint64_t( 1 ) << 63;

 /// the depth of a node by its label
 static int depth( const Node * u ) { return( std::countr_zero( kRoot ) -
                                              std::countr_zero( u->label ) ); }

 /// the subtree of \f$ u \f$ rebuilt perfectly balanced without its dead
 /// nodes, labeled from the label of \f$ u \f$
 void rebuild( Node * u );

 /// the labels, the parents and the bound on the height
 [[nodiscard]] bool valid( void ) const;

 Node * f_root = nullptr;          ///< the root
 std::size_t f_live = 0;           ///< the living nodes
 std::size_t f_q = 0;              ///< all the nodes, dead ones included
 std::vector< double > f_pow = { 1 };   ///< \f$ ( 3 / 2 )^d \f$
 std::vector< Node * > f_buf;      ///< the buffer of the rebuilds
 std::size_t f_rebuilds = 0;       ///< the rebuilds
 std::size_t f_work = 0;           ///< the nodes rebuilt

 };  // end( class OrderMaintenanceList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/OrderMaintenanceList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* OrderMaintenanceList.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File OrderMaintenanceList.h --------------------*/
/*--------------------------------------------------------------------------*/
