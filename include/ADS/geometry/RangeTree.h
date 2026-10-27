/*--------------------------------------------------------------------------*/
/*---------------------------- File RangeTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The two-dimensional range tree of Bentley as an augmented map, beyond the
 * book: a multiset of points \f$ ( x , y ) \f$ of the plane, kept in a map
 * [see `JoinTree.h`] whose keys are the points in lexicographic order, whose
 * values are their multiplicities, and whose augmentation is the sorted
 * sequence of the ordinates of the subtree, with their copies: the monoid of
 * the sorted sequences under the merge, whose identity is the empty
 * sequence. The map is balanced by weight; the merge is not of constant
 * cost, and a subtree of \f$ s \f$ points keeps \f$ s \f$ ordinates, so that
 * the structure takes \f$ O( n \log n ) \f$ space, as the range tree does.
 *
 * `count( x1 , x2 , y1 , y2 )` counts the points of the rectangle
 * \f$ [ x_1 , x_2 ] \times [ y_1 , y_2 ] \f$: the points with
 * \f$ x_1 \le x \le x_2 \f$ are the keys between \f$ ( x_1 , -\infty ) \f$
 * and \f$ ( x_2 , +\infty ) \f$, a range of the order, which the map covers
 * by \f$ O( \log n ) \f$ pieces [see `JoinTree::augVisit()`], whole
 * subtrees and single points; in each piece two binary searches on its
 * sorted ordinates count those in \f$ [ y_1 , y_2 ] \f$, whence
 * \f$ O( \log^2 n ) \f$ time. The fractional cascading of Lueker and of
 * Willard would bring it to \f$ O( \log n ) \f$, at the price of the
 * pointers between the sequences, which a merge would have to rebuild.
 *
 * The points are given all at once to the constructor, which inserts them
 * by `multiInsert()` [see `JoinAlgorithms.h`], on the threads of an executor
 * if given one: the tree is built by joins of balanced halves, and each join
 * merges the sequences of its two sides, \f$ O( n \log n ) \f$ work in all.
 * A point inserted or removed afterwards costs \f$ O( n ) \f$, since the
 * sequences of all its ancestors change, the root's included: the dynamic
 * range tree that the augmented maps of Sun and Blelloch keep in
 * \f$ O( \log^2 n ) \f$ per update shares the inner trees between the
 * versions (persistence), which these maps, destructive, do not.
 *
 * The contracts [see `Contract.h`]: \f$ x_1 \le x_2 \f$ and
 * \f$ y_1 \le y_2 \f$ are not required (an empty rectangle counts 0); the
 * invariant is that of the map, the sorted ordinates included. The
 * operations are defined in `RangeTree.tpp`.
 *
 * \see J. L. Bentley, Decomposable searching problems, Information
 *      Processing Letters 8(5), 244-251, 1979,
 *      doi:10.1016/0020-0190(79)90117-0
 * \see J. L. Bentley, Multidimensional divide-and-conquer, Communications
 *      of the ACM 23(4), 214-229, 1980, doi:10.1145/358841.358850
 * \see G. S. Lueker, A data structure for orthogonal range queries, 19th
 *      Annual Symposium on Foundations of Computer Science (FOCS), 28-34,
 *      1978, doi:10.1109/SFCS.1978.1
 * \see Y. Sun, G. E. Blelloch, Parallel range, segment and rectangle
 *      queries with augmented maps, Proceedings of the 21st Workshop on
 *      Algorithm Engineering and Experiments (ALENEX), 159-173, 2019,
 *      doi:10.1137/1.9781611975499.13
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

#ifndef __ADS_RangeTree
 #define __ADS_RangeTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/tree/JoinTree.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RangeTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a multiset of points of \f$ T \times T \f$, with the count of the points
/// of a rectangle
template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
class RangeTree {

/*--------------------------------------------------------------------------*/
 public:

 using Point = std::pair< T , T >;   ///< \f$ ( x , y ) \f$

 /// the augmentation: the sorted ordinates, with their copies
 struct Ordinates {
  /// the ordinates of a subtree, sorted
  using Value = std::vector< T >;
  /// the empty sequence, the identity of the merge
  static Value identity( void ) { return( Value() ); }
  /// \f$ c \f$ copies of the ordinate \f$ y \f$ of the point
  /// \f$ ( x , y ) \f$
  static Value from( const Point & p , const std::size_t & c ) {
   return( Value( c , p.second ) );
   }
  /// the merge of the sorted sequences \f$ a \f$ and \f$ b \f$,
  /// \f$ O( |a| + |b| ) \f$
  static Value combine( const Value & a , const Value & b ) {
   Value m( a.size() + b.size() );
   std::merge( a.begin() , a.end() , b.begin() , b.end() , m.begin() );
   return( m );
   }
  };

 /// no point
 RangeTree( void ) = default;

 /// the points of \f$ P \f$, sorted, counted and inserted by `multiInsert()`,
 /// \f$ O( n \log n ) \f$
 ///
 /// In pseudocode (the construction of the augmented maps of Sun and
 /// Blelloch, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{RangeTree}( P ) \cr
 /// 2 & \quad \text{sort } P \text{ by } ( x , y ) \cr
 /// 3 & \quad B \gets \text{the distinct points of } P \text{, each with its }
 ///     \text{copies} \cr
 /// 4 & \quad \mathrm{multiInsert}( T , B )\quad \triangleright\ \text{each }
 ///     \text{join merges the ordinates of its sides} \cr
 /// 5 & \quad n \gets |P|
 /// \end{array}}
 /// \f]
 explicit RangeTree( std::vector< Point > P );

 /// the points of \f$ P \f$, as above, on the threads of `ex`
 template< Executor E >
 RangeTree( E & ex , std::vector< Point > P );

 /// one more copy of \f$ ( x , y ) \f$, \f$ O( n ) \f$, since the ordinates
 /// of all its ancestors are merged again
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( x , y ) \cr
 /// 2 & \quad c \gets \text{the copies of } ( x , y ) \text{ in } T
 ///     \text{, 0 if none} \cr
 /// 3 & \quad \mathrm{insert}( T , ( x , y ) , c + 1 );\ n \gets n + 1
 /// \end{array}}
 /// \f]
 void insert( const T & x , const T & y );

 /// one copy less of \f$ ( x , y ) \f$, if there, \f$ O( n ) \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( x , y ) \cr
 /// 2 & \quad c \gets \text{the copies of } ( x , y ) \text{ in } T \cr
 /// 3 & \quad \textbf{if}\ c = \textbf{nil}\ \textbf{then return} \cr
 /// 4 & \quad \textbf{if}\ c = 1\ \textbf{then}\ \mathrm{remove}( T , ( x , y
 ///     ) )\ \textbf{else}\ \mathrm{insert}( T , ( x , y ) , c - 1 ) \cr
 /// 5 & \quad n \gets n - 1
 /// \end{array}}
 /// \f]
 void remove( const T & x , const T & y );

 /// the points, with their copies
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the points of \f$ [ x_1 , x_2 ] \times [ y_1 , y_2 ] \f$, with their
 /// copies, \f$ O( \log^2 n ) \f$
 ///
 /// In pseudocode (the range tree of Bentley on an augmented map, beyond
 /// the book), \f$ Y \f$ the sorted ordinates of a piece:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{count}( x_1 , x_2 , y_1 , y_2 ) \cr
 /// 2 & \quad \textbf{if}\ x_2 < x_1\ \textbf{or}\ y_2 < y_1\ \textbf{then
 ///     return}\ 0 \cr
 /// 3 & \quad c \gets 0 \cr
 /// 4 & \quad \textbf{foreach}\ \text{piece of the keys from } ( x_1 ,
 ///     -\infty ) \text{ to } ( x_2 , +\infty )\ \textbf{do} \cr
 /// 5 & \qquad c \gets c + \mathrm{upperBound}( Y , y_2 ) -
 ///     \mathrm{lowerBound}( Y , y_1 ) \cr
 /// 6 & \quad \textbf{return}\ c
 /// \end{array}}
 /// \f]
 /// where the pieces, \f$ O( \log n ) \f$ subtrees and single points, are
 /// those of `JoinTree::augVisit()`.
 [[nodiscard]] std::size_t count( const T & x1 , const T & x2 ,
                                  const T & y1 , const T & y2 ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Map = JoinTree< Point , std::size_t , WeightBalance , Ordinates >;

 /// the sorted points with their multiplicities
 static std::vector< std::pair< Point , std::size_t > > counted(
                                                std::vector< Point > & P );

 Map f_map;            ///< \f$ ( x , y ) \mapsto \f$ its copies
 std::size_t f_n = 0;  ///< the points, with their copies

 };  // end( class RangeTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/geometry/impl/RangeTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* RangeTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File RangeTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
