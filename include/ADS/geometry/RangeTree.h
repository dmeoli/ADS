/*--------------------------------------------------------------------------*/
/*---------------------------- File RangeTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The two-dimensional range tree of Bentley as an augmented map, beyond the
 * book: a multiset of points \f$ ( x , y ) \f$ of the plane, kept in a map
 * [see JoinTree.h] whose keys are the points in lexicographic order, whose
 * values are their multiplicities, and whose augmentation is the sorted
 * sequence of the ordinates of the subtree, with their copies: the monoid
 * of the sorted sequences under the merge, whose identity is the empty
 * sequence. The map is balanced by weight; the merge is not of constant
 * cost, and a subtree of s points keeps s ordinates, so that the
 * structure takes \f$ O( n \log n ) \f$ space, as the range tree does.
 *
 * count( x1 , x2 , y1 , y2 ) counts the points of the rectangle
 * \f$ [ x_1 , x_2 ] \times [ y_1 , y_2 ] \f$: the points with
 * \f$ x_1 \le x \le x_2 \f$ are the keys between \f$ ( x_1 , -\infty ) \f$
 * and \f$ ( x_2 , +\infty ) \f$, a range of the order, which the map
 * covers by \f$ O( \log n ) \f$ pieces [see JoinTree::augVisit()], whole
 * subtrees and single points; in each piece two binary searches on its
 * sorted ordinates count those in \f$ [ y_1 , y_2 ] \f$, whence
 * \f$ O( \log^2 n ) \f$ time. The fractional cascading of Lueker and of
 * Willard would bring it to \f$ O( \log n ) \f$, at the price of the
 * pointers between the sequences, which a merge would have to rebuild.
 *
 * The points are given all at once to the constructor, which inserts them
 * by multiInsert() [see JoinAlgorithms.h], on the threads of an executor if
 * given one: the tree is built by joins of balanced halves, and each join
 * merges the sequences of its two sides, \f$ O( n \log n ) \f$ work in all.
 * A point inserted or removed afterwards costs \f$ O( n ) \f$, since the
 * sequences of all its ancestors change, the root's included: the dynamic
 * range tree that the augmented maps of Sun and Blelloch keep in
 * \f$ O( \log^2 n ) \f$ per update shares the inner trees between the
 * versions (persistence), which these maps, destructive, do not.
 *
 * The contracts [see Contract.h]: \f$ x_1 \le x_2 \f$ and
 * \f$ y_1 \le y_2 \f$ are not required (an empty rectangle counts 0); the
 * invariant is that of the map, the sorted ordinates included. The
 * operations are defined in RangeTree.tpp.
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

/// a multiset of points of T x T, with the count of the points of a
/// rectangle
template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
class RangeTree {

/*--------------------------------------------------------------------------*/
 public:

 using Point = std::pair< T , T >;   ///< \f$ ( x , y ) \f$

 /// the augmentation: the sorted ordinates, with their copies
 struct Ordinates {
  using Value = std::vector< T >;
  static Value identity( void ) { return( Value() ); }
  static Value from( const Point & p , const std::size_t & c ) {
   return( Value( c , p.second ) );
   }
  static Value combine( const Value & a , const Value & b ) {
   Value m( a.size() + b.size() );
   std::merge( a.begin() , a.end() , b.begin() , b.end() , m.begin() );
   return( m );
   }
  };

 /// no point
 RangeTree( void ) = default;

 /// the points of P, sorted and inserted by multiInsert()
 explicit RangeTree( std::vector< Point > P );

 /// the points of P, on the threads of ex
 template< Executor E >
 RangeTree( E & ex , std::vector< Point > P );

 /// one more copy of ( x , y ), \f$ O( n ) \f$
 void insert( const T & x , const T & y );

 /// one copy less of ( x , y ), if there, \f$ O( n ) \f$
 void remove( const T & x , const T & y );

 /// the points, with their copies
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the points of \f$ [ x_1 , x_2 ] \times [ y_1 , y_2 ] \f$, with their
 /// copies, \f$ O( \log^2 n ) \f$
 [[nodiscard]] std::size_t count( const T & x1 , const T & x2 ,
                                  const T & y1 , const T & y2 ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Map = JoinTree< Point , std::size_t , WeightBalance , Ordinates >;

 /// the sorted points with their multiplicities
 static std::vector< std::pair< Point , std::size_t > > counted(
                                                std::vector< Point > & P );

 Map f_map;            ///< ( x , y ) -> its copies
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
