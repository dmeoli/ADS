/*--------------------------------------------------------------------------*/
/*---------------------------- File Assignment.h ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The assignment problem, beyond the book: given the costs
 * \f$ c( i , j ) \f$ of \f$ n \f$ rows and \f$ n \f$ columns (the workers
 * and the tasks, the left and the right side of a complete bipartite
 * graph), a perfect matching of the rows to the columns of least total
 * cost, a cost `infinity()` [see `ShortestPaths.h`] forbidding its pair.
 * `Assignment< W >` solves it by the Hungarian method of Kuhn in the form
 * of the shortest augmenting paths (Jonker and Volgenant, Crouse), in
 * \f$ O( n^3 ) \f$, and keeps it optimal after the changes of the costs
 * (see \ref theory_graph_assign): a change of a row or of a column frees
 * one pair, restores the feasibility of the dual by a single potential and
 * augments once, in \f$ O( n^2 ) \f$, which is the incremental Hungarian
 * algorithm of Toroslu and Üçoluk, and the dynamic one of Mills-Tettey,
 * Stentz and Dias for the costs that change. A change of a single cost
 * that leaves its reduced cost nonnegative off the matching, or that
 * raises a cost off it, costs \f$ O( 1 ) \f$.
 *
 * The changes are queued by `setCost()`, `setRow()` and `setColumn()`, and
 * worked at the next query, as the observers of `Reoptimization.h` do,
 * which hold the same counts of the outcomes [see `Reopt`]; the observer
 * `AssignmentObserver` of `Reoptimization.h` keeps an assignment on the
 * weights of a bipartite graph that changes. The libraries of graphs solve
 * the assignment from scratch (LEMON by its minimum cost flows, SciPy by
 * the algorithm of Crouse), and none keeps the duals for a warm start.
 *
 * The contracts [see `Contract.h`]: \f$ n \ge 0 \f$, the rows and the
 * columns in \f$ [ 0 , n ) \f$, the costs finite or `infinity()`; at the
 * level of the audits each `solve()` checks the duals: every reduced cost
 * \f$ c( i , j ) - u_i - v_j \f$ of a finite cost nonnegative, and zero on
 * the pairs of the assignment. The operations are defined in
 * `Assignment.tpp`.
 *
 * \see H. W. Kuhn, The Hungarian method for the assignment problem, Naval
 *      Research Logistics Quarterly 2(1-2), 83-97, 1955,
 *      doi:10.1002/nav.3800020109
 * \see J. Munkres, Algorithms for the assignment and transportation
 *      problems, Journal of the Society for Industrial and Applied
 *      Mathematics 5(1), 32-38, 1957, doi:10.1137/0105003
 * \see R. Jonker, A. Volgenant, A shortest augmenting path algorithm for
 *      dense and sparse linear assignment problems, Computing 38(4),
 *      325-340, 1987, doi:10.1007/BF02278710
 * \see D. F. Crouse, On implementing 2D rectangular assignment algorithms,
 *      IEEE Transactions on Aerospace and Electronic Systems 52(4),
 *      1679-1696, 2016, doi:10.1109/TAES.2016.140952
 * \see I. H. Toroslu, G. Üçoluk, Incremental assignment problem,
 *      Information Sciences 177(6), 1523-1529, 2007,
 *      doi:10.1016/j.ins.2006.05.004, and the addendum of A. Volgenant,
 *      178(23), 4583, 2008, doi:10.1016/j.ins.2008.07.020
 * \see G. A. Mills-Tettey, A. Stentz, M. B. Dias, The dynamic Hungarian
 *      algorithm for the assignment problem with changing costs, technical
 *      report CMU-RI-TR-07-27, Robotics Institute, Carnegie Mellon
 *      University, 2007
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

#ifndef __ADS_Assignment
 #define __ADS_Assignment
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/graph/ShortestPaths.h"

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Assignment -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an assignment of least cost of \f$ n \f$ rows to \f$ n \f$ columns, kept
/// optimal after the changes of the costs
template< class W = long long >
requires std::is_arithmetic_v< W >
class Assignment {

/*--------------------------------------------------------------------------*/
 public:

 using Weight = W;   ///< the type of the costs

 /// the assignment of the costs \f$ C \f$, the row \f$ i \f$ in
 /// \f$ C[ i n .. i n + n - 1 ] \f$, computed at the first query
 /// \pre \f$ n \ge 0 \f$, \f$ |C| = n^2 \f$
 Assignment( int n , std::vector< W > C );

 /// the number of rows, and of columns
 [[nodiscard]] int n( void ) const { return( f_n ); }

 /// the cost of the row \f$ i \f$ and the column \f$ j \f$
 /// \pre \f$ i , j \in [ 0 , n ) \f$
 [[nodiscard]] W cost( int i , int j ) const;

 /// the cost of \f$ ( i , j ) \f$ set to \f$ c \f$, worked at the next
 /// query
 /// \pre \f$ i , j \in [ 0 , n ) \f$
 void setCost( int i , int j , W c );

 /// the costs of the row \f$ i \f$ set to \f$ R \f$
 /// \pre \f$ i \in [ 0 , n ) \f$, \f$ |R| = n \f$
 void setRow( int i , const std::vector< W > & R );

 /// the costs of the column \f$ j \f$ set to \f$ K \f$
 /// \pre \f$ j \in [ 0 , n ) \f$, \f$ |K| = n \f$
 void setColumn( int j , const std::vector< W > & K );

 /// brings the assignment up to date
 ///
 /// In pseudocode (beyond the book; it is correct by Lemma 9 of
 /// \ref theory_graph_assign), with \f$ u \f$ and \f$ v \f$ the
 /// potentials of the rows and of the columns, \f$ D_r \f$ and \f$ D_c \f$
 /// the rows and the columns changed since the last query:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{solve}() \cr
 /// 2 & \quad \textbf{if}\ \text{there is no assignment yet}
 ///     \textbf{ or } 2 ( |D_r| + |D_c| ) > n\ \textbf{then} \cr
 /// 3 & \qquad v \gets 0;\ u_i \gets \min_j c( i , j ) \text{ for each }
 ///     i;\ \text{every row free} \cr
 /// 4 & \quad \textbf{else} \cr
 /// 5 & \qquad \textbf{foreach}\ i \in D_r\ \textbf{do}\ \text{free } i;
 ///     \ u_i \gets \min_j ( c( i , j ) - v_j ) \cr
 /// 6 & \qquad \textbf{foreach}\ j \in D_c\ \textbf{do}\ \text{free } j;
 ///     \ v_j \gets \min_i ( c( i , j ) - u_i ) \cr
 /// 7 & \quad \textbf{foreach}\ \text{free row } i\ \textbf{do}
 ///     \ \mathrm{augment}( i )
 /// \end{array}}
 /// \f]
 void solve( void );

 /// the column of the row \f$ i \f$, \f$ -1 \f$ if none (no perfect
 /// matching of finite costs)
 /// \pre \f$ i \in [ 0 , n ) \f$
 [[nodiscard]] int column( int i );

 /// the row of the column \f$ j \f$, \f$ -1 \f$ if none
 /// \pre \f$ j \in [ 0 , n ) \f$
 [[nodiscard]] int row( int j );

 /// whether every row has a column, i.e., a perfect matching of finite
 /// costs exists
 [[nodiscard]] bool perfect( void );

 /// the total cost of the pairs of the assignment
 [[nodiscard]] W total( void );

 /// the potential \f$ u_i \f$ of the row \f$ i \f$, a solution of the dual
 /// with those of the columns
 /// \pre \f$ i \in [ 0 , n ) \f$
 [[nodiscard]] W rowPotential( int i );

 /// the potential \f$ v_j \f$ of the column \f$ j \f$
 /// \pre \f$ j \in [ 0 , n ) \f$
 [[nodiscard]] W columnPotential( int j );

 /// how the last `solve()` with changes, or with no assignment yet, did
 /// it [see `Reopt` of `Reoptimization.h`]: 0 from scratch, 1 by the
 /// augmentations of the rows freed, 2 with no work
 [[nodiscard]] int outcome( void ) const { return( f_outcome ); }

 /// the solves of each outcome so far
 [[nodiscard]] long long count( int outcome ) const {
  return( f_count[ outcome ] );
  }

 /// the augmentations so far
 [[nodiscard]] long long augmentations( void ) const { return( f_aug ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the reduced cost of \f$ ( i , j ) \f$, for a finite cost
 [[nodiscard]] W reduced( int i , int j ) const {
  return( f_C[ std::size_t( i ) * std::size_t( f_n ) + std::size_t( j ) ]
          - f_u[ i ] - f_v[ j ] );
  }

 /// whether the cost of \f$ ( i , j ) \f$ is finite
 [[nodiscard]] bool finite( int i , int j ) const {
  return( f_C[ std::size_t( i ) * std::size_t( f_n ) + std::size_t( j ) ]
          != infinity< W >() );
  }

 /// the row \f$ i \f$ and its column freed
 void freeRow( int i );

 /// the shortest augmenting path from the free row \f$ i_0 \f$ in the
 /// reduced costs, and the potentials that keep them nonnegative; false if
 /// no free column is reached
 ///
 /// In pseudocode (Jonker and Volgenant, in the form of Crouse, beyond the
 /// book), \f$ \delta_j \f$ the length of the shortest path found so far to
 /// the column \f$ j \f$, \f$ S \f$ the columns scanned:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{augment}( i_0 ) \cr
 /// 2 & \quad \delta \gets \infty;\ S \gets \emptyset;\ i \gets i_0;
 ///     \ \mu \gets 0 \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \textbf{foreach}\ j \notin S\ \textbf{with}\ \mu + c( i , j
 ///     ) - u_i - v_j < \delta_j\ \textbf{do}\ \delta_j \gets \mu + c( i ,
 ///     j ) - u_i - v_j;\ \pi_j \gets i \cr
 /// 5 & \qquad j \gets \text{the } j \notin S \text{ of least } \delta_j
 ///     \text{, a free one on a tie};\ \textbf{if}\ \delta_j = \infty
 ///     \ \textbf{then return false} \cr
 /// 6 & \qquad \mu \gets \delta_j;\ S \gets S \cup \{ j \} \cr
 /// 7 & \qquad \textbf{if}\ j \text{ is free}\ \textbf{then break}
 ///     \ \textbf{else}\ i \gets \text{the row of } j \cr
 /// 8 & \quad u_{i_0} \gets u_{i_0} + \mu;\ \textbf{foreach}\ j \in S
 ///     \ \textbf{do}\ v_j \gets v_j - ( \mu - \delta_j ) \cr
 /// 9 & \quad \textbf{foreach}\ \text{row } i \ne i_0 \text{ of a } j \in S
 ///     \ \textbf{do}\ u_i \gets u_i + \mu - \delta_j \cr
 /// 10 & \quad \text{flip the pairs along } \pi \text{ from } j \text{ back}
 ///     \text{ to } i_0;\ \textbf{return true}
 /// \end{array}}
 /// \f]
 bool augment( int i0 );

 /// whether the duals are feasible and tight on the pairs (audit)
 [[nodiscard]] bool optimal( void ) const;

 int f_n;                                  ///< the rows
 std::vector< W > f_C;                     ///< the costs, by rows
 std::vector< W > f_u;                     ///< the potentials of the rows
 std::vector< W > f_v;                     ///< those of the columns
 std::vector< int > f_col;                 ///< the column of each row
 std::vector< int > f_row;                 ///< the row of each column
 std::vector< char > f_dirtyR;             ///< the rows changed
 std::vector< char > f_dirtyC;             ///< the columns changed
 std::vector< int > f_DR;                  ///< the rows changed, listed
 std::vector< int > f_DC;                  ///< the columns changed
 std::vector< std::pair< int , int > > f_cells;   ///< the single costs
                                                 ///< changed
 std::vector< W > f_dist;                  ///< \f$ \delta \f$ of augment()
 std::vector< int > f_pred;                ///< \f$ \pi \f$ of augment()
 std::vector< char > f_scanned;            ///< \f$ S \f$ of augment()
 std::vector< int > f_order;               ///< the columns of \f$ S \f$
 bool f_ready = false;                     ///< whether there is one
 bool f_pending = false;                   ///< whether changes wait
 int f_outcome = 0;                        ///< the last outcome
 long long f_count[ 3 ] = { 0 , 0 , 0 };   ///< the solves by outcome
 long long f_aug = 0;                      ///< the augmentations

 };  // end( class Assignment )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/graph/impl/Assignment.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Assignment.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File Assignment.h --------------------------*/
/*--------------------------------------------------------------------------*/
