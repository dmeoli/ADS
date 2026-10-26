/*--------------------------------------------------------------------------*/
/*-------------------------- File IntervalTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The interval tree as an augmented map, beyond the book: a multiset of
 * closed intervals \f$ [ a , b ] \f$ of a totally ordered type, kept in a
 * map [see JoinTree.h] whose keys are the pairs \f$ ( a , b ) \f$ in
 * lexicographic order, whose values are their multiplicities, and whose
 * augmentation is the largest right end of the subtree,
 * \f$ \mathit{aug}( u ) = \max \{ b : ( a , b ) \in T_u \} \f$, the
 * monoid of the maximum on the right ends (the identity being "no
 * interval"). The map is balanced by weight, and the augmentation is kept
 * by its join alone, as for every augmented map.
 *
 * A stabbing query asks for the intervals that contain a point x. The
 * intervals with \f$ a \le x \f$ are the keys up to \f$ ( x , \infty ) \f$,
 * a prefix of the order, and x is in one of them if and only if the
 * largest right end of the prefix is \f$ \ge x \f$; stabbed( x ) reads that
 * maximum on the path that separates the prefix from the rest, combining
 * the augmented values of the left subtrees that the path leaves behind,
 * in \f$ O( \log n ) \f$ time. stab( x , f ) reports them all by a visit
 * that enters a subtree only if its augmented value is \f$ \ge x \f$ and,
 * for a node with \f$ a > x \f$, does not enter its right subtree: each
 * node visited either is reported, or lies on one of the paths that lead
 * to the reported ones, so that the visit costs
 * \f$ O( \min( n , ( k + 1 ) \log n ) ) \f$ for k intervals reported.
 * The intervals are inserted and removed one at a time in
 * \f$ O( \log n ) \f$, and the constructor takes them all by multiInsert()
 * [see JoinAlgorithms.h], on the threads of an executor if given one.
 *
 * The contracts [see Contract.h]: \f$ a \le b \f$ for each interval; the
 * invariant is that of the map, the largest right ends included. The
 * operations are defined in IntervalTree.tpp.
 *
 * \see H. Edelsbrunner, Dynamic data structures for orthogonal
 *      intersection queries, Report F59, Institut für
 *      Informationsverarbeitung, Technische Universität Graz, 1980
 * \see E. M. McCreight, Priority search trees, SIAM Journal on Computing
 *      14(2), 257-276, 1985, doi:10.1137/0214021
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, section 14.3 (the interval
 *      tree as an augmented search tree)
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

#ifndef __ADS_IntervalTree
 #define __ADS_IntervalTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/tree/JoinTree.h"

#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS IntervalTree ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a multiset of closed intervals of T with stabbing queries
template< std::totally_ordered T >
class IntervalTree {

/*--------------------------------------------------------------------------*/
 public:

 using Interval = std::pair< T , T >;   ///< \f$ [ a , b ] \f$

 /// the augmentation: the largest right end, nothing for no interval
 struct MaxEnd {
  using Value = std::optional< T >;
  static Value identity( void ) { return( std::nullopt ); }
  static Value from( const Interval & i , const std::size_t & ) {
   return( i.second );
   }
  static Value combine( const Value & x , const Value & y ) {
   if( ! x ) return( y );
   if( ! y ) return( x );
   return( *x < *y ? y : x );
   }
  };

 /// no interval
 IntervalTree( void ) = default;

 /// the intervals of I, sorted and inserted by multiInsert()
 /// \pre \f$ a \le b \f$ for each one
 explicit IntervalTree( std::vector< Interval > I );

 /// the intervals of I, on the threads of ex
 /// \pre \f$ a \le b \f$ for each one
 template< Executor E >
 IntervalTree( E & ex , std::vector< Interval > I );

 /// one more copy of \f$ [ a , b ] \f$, \f$ O( \log n ) \f$
 /// \pre \f$ a \le b \f$
 void insert( const T & a , const T & b );

 /// one copy less of \f$ [ a , b ] \f$, if there, \f$ O( \log n ) \f$
 void remove( const T & a , const T & b );

 /// the intervals, with their copies
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether an interval contains x, \f$ O( \log n ) \f$
 [[nodiscard]] bool stabbed( const T & x ) const;

 /// f( a , b ) once per copy of each interval that contains x, by
 /// increasing ( a , b ), \f$ O( \min( n , ( k + 1 ) \log n ) ) \f$ for k
 /// distinct intervals reported
 template< class F >
 void stab( const T & x , F && f ) const;

 /// the intervals that contain x, with their copies
 [[nodiscard]] std::size_t count( const T & x ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Map = JoinTree< Interval , std::size_t , WeightBalance , MaxEnd >;

 /// the sorted intervals with their multiplicities
 static std::vector< std::pair< Interval , std::size_t > > counted(
                                             std::vector< Interval > & I );

 Map f_map;            ///< ( a , b ) -> its copies
 std::size_t f_n = 0;  ///< the intervals, with their copies

 };  // end( class IntervalTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/geometry/impl/IntervalTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* IntervalTree.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File IntervalTree.h ------------------------*/
/*--------------------------------------------------------------------------*/
