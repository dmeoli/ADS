/*--------------------------------------------------------------------------*/
/*-------------------------- File IntervalTree.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The interval tree as an augmented map, beyond the book: a multiset of
 * closed intervals \f$ [ a , b ] \f$ of a totally ordered type, kept in a
 * map [see `JoinTree.h`] whose keys are the pairs \f$ ( a , b ) \f$ in
 * lexicographic order, whose values are their multiplicities, and whose
 * augmentation is the largest right end of the subtree,
 * \f$ \mathit{aug}( u ) = \max \{ b : ( a , b ) \in T_u \} \f$, the monoid
 * of the maximum on the right ends, whose identity is no interval. The map
 * is balanced by weight, and the augmentation is kept by its join alone, as
 * for every augmented map.
 *
 * A stabbing query asks for the intervals that contain a point \f$ x \f$.
 * The intervals with \f$ a \le x \f$ are the keys up to
 * \f$ ( x , \infty ) \f$, a prefix of the order, and \f$ x \f$ is in one of
 * them if and only if the largest right end of the prefix is at least
 * \f$ x \f$; `stabbed( x )` reads that maximum on the path that separates
 * the prefix from the rest, combining the augmented values of the left
 * subtrees that the path leaves behind, in \f$ O( \log n ) \f$ time.
 * `stab( x , f )` reports them all by a visit that enters a subtree only if
 * its augmented value is at least \f$ x \f$ and, for a node with
 * \f$ a > x \f$, does not enter its right subtree: each node visited either
 * is reported, or lies on one of the paths that lead to the reported ones,
 * so that the visit costs \f$ O( \min( n , ( k + 1 ) \log n ) ) \f$ for
 * \f$ k \f$ intervals reported. The intervals are inserted and removed one
 * at a time in \f$ O( \log n ) \f$, and the constructor takes them all by
 * `multiInsert()` [see `JoinAlgorithms.h`], on the threads of an executor
 * if given one.
 *
 * The contracts [see `Contract.h`]: \f$ a \le b \f$ for each interval; the
 * invariant is that of the map, the largest right ends included. The
 * operations are defined in `IntervalTree.tpp`.
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

/// a multiset of closed intervals of `T` with stabbing queries
template< std::totally_ordered T >
class IntervalTree {

/*--------------------------------------------------------------------------*/
 public:

 using Interval = std::pair< T , T >;   ///< \f$ [ a , b ] \f$

 /// the augmentation: the largest right end, nothing for no interval
 struct MaxEnd {
  /// the largest right end, or nothing
  using Value = std::optional< T >;
  /// no interval, the identity of the maximum
  static Value identity( void ) { return( std::nullopt ); }
  /// the right end \f$ b \f$ of the interval \f$ [ a , b ] \f$
  static Value from( const Interval & i , const std::size_t & ) {
   return( i.second );
   }
  /// \f$ \max( x , y ) \f$, nothing being smaller than any end
  static Value combine( const Value & x , const Value & y ) {
   if( ! x ) return( y );
   if( ! y ) return( x );
   return( *x < *y ? y : x );
   }
  };

 /// no interval
 IntervalTree( void ) = default;

 /// the intervals of \f$ I \f$, sorted, counted and inserted by
 /// `multiInsert()`
 ///
 /// In pseudocode (the construction of the augmented maps of Sun and
 /// Blelloch, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{IntervalTree}( I ) \cr
 /// 2 & \quad \text{sort } I \text{ by } ( a , b ) \cr
 /// 3 & \quad B \gets \text{the distinct intervals of } I \text{, each with }
 ///     \text{its copies} \cr
 /// 4 & \quad \mathrm{multiInsert}( T , B )\quad \triangleright\ \text{joins }
 ///     \text{of balanced halves, } \mathit{aug} \text{ kept by the join} \cr
 /// 5 & \quad n \gets |I|
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \le b \f$ for each one
 explicit IntervalTree( std::vector< Interval > I );

 /// the intervals of \f$ I \f$, as above, on the threads of `ex`
 /// \pre \f$ a \le b \f$ for each one
 template< Executor E >
 IntervalTree( E & ex , std::vector< Interval > I );

 /// one more copy of \f$ [ a , b ] \f$, \f$ O( \log n ) \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( a , b ) \cr
 /// 2 & \quad c \gets \text{the copies of } ( a , b ) \text{ in } T
 ///     \text{, 0 if none} \cr
 /// 3 & \quad \mathrm{insert}( T , ( a , b ) , c + 1 );\ n \gets n + 1
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \le b \f$
 void insert( const T & a , const T & b );

 /// one copy less of \f$ [ a , b ] \f$, if there, \f$ O( \log n ) \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{remove}( a , b ) \cr
 /// 2 & \quad c \gets \text{the copies of } ( a , b ) \text{ in } T \cr
 /// 3 & \quad \textbf{if}\ c = \textbf{nil}\ \textbf{then return} \cr
 /// 4 & \quad \textbf{if}\ c = 1\ \textbf{then}\ \mathrm{remove}( T , ( a , b
 ///     ) )\ \textbf{else}\ \mathrm{insert}( T , ( a , b ) , c - 1 ) \cr
 /// 5 & \quad n \gets n - 1
 /// \end{array}}
 /// \f]
 void remove( const T & a , const T & b );

 /// the intervals, with their copies
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// whether an interval contains \f$ x \f$, \f$ O( \log n ) \f$
 ///
 /// In pseudocode (Sun and Blelloch, beyond the book), \f$ m \f$ the largest
 /// right end of the intervals with \f$ a \le x \f$ met so far:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{stabbed}( x ) \cr
 /// 2 & \quad m \gets \textbf{nil};\ u \gets \text{the root of } T \cr
 /// 3 & \quad \textbf{while}\ u \ne \textbf{nil}\ \textbf{do} \cr
 /// 4 & \qquad \textbf{if}\ x < a( u )\ \textbf{then}\ u \gets u.\mathit{left}
 ///     \cr
 /// 5 & \qquad \textbf{else} \cr
 /// 6 & \qquad\quad m \gets \max( m , \mathit{aug}( u.\mathit{left} ) , b( u )
 ///     );\ u \gets u.\mathit{right} \cr
 /// 7 & \quad \textbf{return}\ m \ne \textbf{nil}\ \textbf{and}\ m \ge x
 /// \end{array}}
 /// \f]
 [[nodiscard]] bool stabbed( const T & x ) const;

 /// `f( a , b )` once per copy of each interval that contains \f$ x \f$, by
 /// increasing \f$ ( a , b ) \f$; \f$ O( \min( n , ( k + 1 ) \log n ) ) \f$
 /// for \f$ k \f$ distinct intervals reported
 ///
 /// In pseudocode (Sun and Blelloch, beyond the book), a visit in symmetric
 /// order cut where no interval can contain \f$ x \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{visit}( u , x , f ) \cr
 /// 2 & \quad \textbf{if}\ u = \textbf{nil}\ \textbf{or}\ \mathit{aug}( u ) <
 ///     x\ \textbf{then return} \cr
 /// 3 & \quad \mathrm{visit}( u.\mathit{left} , x , f ) \cr
 /// 4 & \quad \textbf{if}\ x < a( u )\ \textbf{then return}\quad
 ///     \triangleright\ \text{no } a \le x \text{ to the right} \cr
 /// 5 & \quad \textbf{if}\ b( u ) \ge x\ \textbf{then}\ \text{call }
 ///     f( a( u ) , b( u ) ) \text{ once per copy} \cr
 /// 6 & \quad \mathrm{visit}( u.\mathit{right} , x , f ) \cr
 /// 7 & \textbf{procedure}\ \mathrm{stab}( x , f ) \cr
 /// 8 & \quad \mathrm{visit}( \text{the root of } T , x , f )
 /// \end{array}}
 /// \f]
 template< class F >
 void stab( const T & x , F && f ) const;

 /// the intervals that contain \f$ x \f$, with their copies, by `stab()`;
 /// \f$ O( \min( n , ( k + 1 ) \log n ) ) \f$
 ///
 /// In pseudocode:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{count}( x ) \cr
 /// 2 & \quad \textbf{return}\ \text{the calls of } f \text{ in }
 ///     \mathrm{stab}( x , f )
 /// \end{array}}
 /// \f]
 [[nodiscard]] std::size_t count( const T & x ) const;

/*--------------------------------------------------------------------------*/
 private:

 using Map = JoinTree< Interval , std::size_t , WeightBalance , MaxEnd >;

 /// the sorted intervals with their multiplicities
 static std::vector< std::pair< Interval , std::size_t > > counted(
                                             std::vector< Interval > & I );

 Map f_map;            ///< \f$ ( a , b ) \mapsto \f$ its copies
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
