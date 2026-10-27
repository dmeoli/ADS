/*--------------------------------------------------------------------------*/
/*--------------------------- File SatSolvers.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Two solvers of the satisfiability of a formula in conjunctive normal form
 * [see `Problems.h` for the formula and the backtrack of the reductions],
 * after Knuth (section 7.2.2.2 of The Art of Computer Programming), beyond the
 * book.
 *
 * -  `dpll()` is the procedure of Davis, Putnam, Logemann and Loveland: unit
 *   propagation (a clause with all its literals false but one forces it), then
 *   a branch on a literal of a shortest open clause, and back on a
 *   contradiction; the propagation scans the clauses, simple and slow.
 * -  `Cdcl` is conflict-driven clause learning (Knuth's Algorithm C of section
 *   7.2.2.2): each clause watches two of its literals, which need attention
 *   only when one becomes false, so that the propagation touches few clauses;
 *   a conflict is analyzed back to its first unique implication point, which
 *   gives a learned clause implied by the formula, and the search jumps back
 *   to the level where the clause forces its literal; the variables in the
 *   conflicts gain activity (VSIDS), and the decisions take the most active
 *   one, with the value it had last (phase saving); restarts follow the
 *   sequence of Luby, Sinclair and Zuckerman. The learned clauses are kept (no
 *   deletion), which suits formulae of moderate size. The heap of the
 *   variables is a parameter of `BasicCdcl` (`Cdcl` is the one on
 *   `std::priority_queue`), so that the same algorithm runs on the heaps of
 *   std, Boost or ADS with the same decisions and conflicts.
 *
 * The contracts [see `Contract.h`]: the literals are \f$ \pm ( v + 1 ) \f$
 * with \f$ v \in [ 0 , n ) \f$; a model returned passes `satCertificate()`, a
 * postcondition. A formula declared unsatisfiable has no certificate here (a
 * proof of the learned clauses, as DRAT, would be one). The functions are
 * defined in `SatSolvers.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 4B, Addison-Wesley,
 *      2022, section 7.2.2.2
 * \see M. Davis, H. Putnam, A computing procedure for quantification
 *      theory, Journal of the ACM 7(3), 201-215, 1960,
 *      doi:10.1145/321033.321034
 * \see M. Davis, G. Logemann, D. Loveland, A machine program for
 *      theorem-proving, Communications of the ACM 5(7), 394-397, 1962,
 *      doi:10.1145/368273.368557
 * \see J. P. Marques-Silva, K. A. Sakallah, GRASP: a search algorithm for
 *      propositional satisfiability, IEEE Transactions on Computers 48(5),
 *      506-521, 1999, doi:10.1109/12.769433
 * \see M. W. Moskewicz, C. F. Madigan, Y. Zhao, L. Zhang, S. Malik,
 *      Chaff: engineering an efficient SAT solver, DAC 2001, 530-535,
 *      doi:10.1145/378239.379017
 * \see M. Luby, A. Sinclair, D. Zuckerman, Optimal speedup of Las Vegas
 *      algorithms, Information Processing Letters 47(4), 173-180, 1993,
 *      doi:10.1016/0020-0190(93)90029-9
 * \see K. Pipatsrisawat, A. Darwiche, A lightweight component caching
 *      scheme for satisfiability solvers, SAT 2007, LNCS 4501, 294-299,
 *      doi:10.1007/978-3-540-72788-0_28 (phase saving)
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

#ifndef __ADS_SatSolvers
 #define __ADS_SatSolvers
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// a model of the formula \f$ F \f$ of \f$ n \f$ variables by DPLL, the
/// variables left free false, or nothing if \f$ F \f$ is unsatisfiable
///
/// In pseudocode (the procedure of Davis, Putnam, Logemann and Loveland,
/// beyond the book), \f$ a \f$ the partial assignment, all free at the start:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{dpll}( F , a ) \cr
/// 2 & \quad \textbf{repeat}\quad \triangleright\ \text{the unit}
///     \text{ propagation} \cr
/// 3 & \qquad \textbf{foreach}\ C \in F \text{ not satisfied by } a
///     \ \textbf{do} \cr
/// 4 & \qquad\quad \textbf{if}\ \text{every literal of } C \text{ is}
///     \text{ false }\ \textbf{then}\ \text{undo the forced ones};
///     \ \textbf{return false} \cr
/// 5 & \qquad\quad \textbf{if}\ \text{one literal } \ell \text{ of } C
///     \text{ alone is free }\ \textbf{then}\ \text{make } \ell
///     \text{ true, a forced one} \cr
/// 6 & \quad \textbf{until}\ \text{no literal was forced} \cr
/// 7 & \quad \textbf{if}\ \text{every clause is satisfied }
///     \ \textbf{then return true} \cr
/// 8 & \quad \ell \gets \text{the first free literal of a shortest open}
///     \text{ clause} \cr
/// 9 & \quad \textbf{foreach}\ \text{value of } \ell \text{, true}
///     \text{ first}\ \textbf{do} \cr
/// 10 & \qquad \text{give it to } \ell;\ \textbf{if}\ \mathrm{dpll}( F ,
///      a )\ \textbf{then return true} \cr
/// 11 & \quad \text{free } \ell \text{ and undo the forced ones};
///      \ \textbf{return false}
/// \end{array}}
/// \f]
/// \pre \f$ n \ge 0 \f$ and the literals of \f$ F \f$ are variables of
///      \f$ [ 0 , n ) \f$
/// \post a model returned passes `satCertificate()`
inline std::optional< std::vector< bool > > dpll( const Cnf & F , int n );

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS BasicCdcl -----------------------------*/
/*--------------------------------------------------------------------------*/

/// conflict-driven clause learning with two watched literals, the free
/// variables by activity in a `Heap` of pairs
/// \f$ ( \mathit{activity} , v ) \f$, the largest on top:
/// `std::priority_queue` by default, or a heap of Boost or of ADS [see
/// `DaryHeap.h`], with the same decisions and conflicts, since the pairs are
/// ordered totally
template< class Heap = std::priority_queue< std::pair< double , int > > >
class BasicCdcl {

/*--------------------------------------------------------------------------*/
 public:

 /// the outcome of `solve()`
 enum class Result {
  SAT ,      ///< a model found, in `model()`
  UNSAT ,    ///< the formula is unsatisfiable
  UNKNOWN    ///< the conflicts allowed are over before an answer
  };

 /// the solver of the formula \f$ F \f$ of \f$ n \f$ variables, its clauses
 /// with the repeated literals merged and the tautologies dropped, and its
 /// unit clauses assigned at the level 0
 /// \pre \f$ n \ge 0 \f$ and the literals of \f$ F \f$ are variables of
 ///      \f$ [ 0 , n ) \f$
 BasicCdcl( const Cnf & F , int n );

 /// the search, `UNKNOWN` after `maxConflicts` conflicts if that is
 /// \f$ \ge 0 \f$
 ///
 /// In pseudocode (conflict-driven clause learning, beyond the book, after
 /// Marques-Silva and Sakallah and after Chaff), the trail the literals
 /// assigned in order, each with its level and its reason, the clause that
 /// forced it:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{solve}( \mathit{maxConflicts} ) \cr
 /// 2 & \quad \textbf{if}\ F \text{ has an empty clause }\ \textbf{then
 ///     return}\ \text{UNSAT} \cr
 /// 3 & \quad \textbf{loop} \cr
 /// 4 & \qquad \kappa \gets \mathrm{propagate}() \cr
 /// 5 & \qquad \textbf{if}\ \kappa \ne \textbf{nil}\ \textbf{then}\quad
 ///     \triangleright\ \text{a conflict} \cr
 /// 6 & \qquad\quad \textbf{if}\ \text{the level is } 0\ \textbf{then
 ///     return}\ \text{UNSAT} \cr
 /// 7 & \qquad\quad ( C , b ) \gets \mathrm{analyze}( \kappa );
 ///     \ \text{cancel the levels above } b \cr
 /// 8 & \qquad\quad \text{assign the first literal of } C \text{, with}
 ///     \text{ the reason } C \text{ if } |C| > 1 \cr
 /// 9 & \qquad\quad \textbf{if}\ \text{the conflicts are }
 ///     \mathit{maxConflicts}\ \textbf{then return}\ \text{UNKNOWN} \cr
 /// 10 & \qquad\quad \textbf{if}\ \text{the budget is over }
 ///      \ \textbf{then}\ \text{cancel the levels above } 0;
 ///      \ \text{the budget } 100\, \mathrm{luby}( k ) \text{ for the }
 ///      k \text{-th restart} \cr
 /// 11 & \qquad \textbf{else} \cr
 /// 12 & \qquad\quad v \gets \text{the free variable of largest activity}
 ///      \cr
 /// 13 & \qquad\quad \textbf{if}\ v = \textbf{nil}\ \textbf{then return}
 ///      \ \text{SAT} \cr
 /// 14 & \qquad\quad \text{open a level and give } v \text{ its saved}
 ///      \text{ phase} \cr
 /// 15 & \textbf{function}\ \mathrm{propagate}() \cr
 /// 16 & \quad \textbf{foreach}\ \ell \text{ of the trail not}
 ///      \text{ propagated yet}\ \textbf{do} \cr
 /// 17 & \qquad \textbf{foreach}\ c \text{ watching } \lnot \ell
 ///      \ \textbf{do}\quad \triangleright\ \lnot \ell \text{ now false} \cr
 /// 18 & \qquad\quad \textbf{if}\ \text{the other watch of } c \text{ is}
 ///      \text{ true }\ \textbf{then continue} \cr
 /// 19 & \qquad\quad \textbf{if}\ \text{a literal of } c \text{ not}
 ///      \text{ watched is not false }\ \textbf{then}\ \text{watch it}
 ///      \text{ instead};\ \textbf{continue} \cr
 /// 20 & \qquad\quad \textbf{if}\ \text{the other watch is false }
 ///      \ \textbf{then return}\ c \cr
 /// 21 & \qquad\quad \text{assign the other watch, with the reason } c \cr
 /// 22 & \quad \textbf{return nil} \cr
 /// 23 & \textbf{function}\ \mathrm{analyze}( \kappa ) \cr
 /// 24 & \quad C \gets ();\ P \gets 0 \cr
 /// 25 & \quad \textbf{repeat} \cr
 /// 26 & \qquad \textbf{foreach}\ q \in \kappa \text{, but the literal}
 ///      \text{ resolved on, of a variable } v \text{ not seen, of a level}
 ///      \text{ above } 0\ \textbf{do} \cr
 /// 27 & \qquad\quad \text{mark } v \text{ as seen and raise its}
 ///      \text{ activity} \cr
 /// 28 & \qquad\quad \textbf{if}\ v \text{ is of the current level }
 ///      \ \textbf{then}\ P \gets P + 1\ \textbf{else}\ \text{append } q
 ///      \text{ to } C \cr
 /// 29 & \qquad p \gets \text{the last literal of the trail whose variable}
 ///      \text{ is seen};\ \kappa \gets \text{its reason};\ P \gets P - 1
 ///      \cr
 /// 30 & \quad \textbf{until}\ P = 0\quad \triangleright\ p \text{ is}
 ///      \text{ the first unique implication point} \cr
 /// 31 & \quad \textbf{return}\ ( \lnot p \cdot C , \text{the largest}
 ///      \text{ level of } C \text{, } 0 \text{ if } C = () )
 /// \end{array}}
 /// \f]
 /// \post `SAT` means that `model()` passes `satCertificate()`
 Result solve( long maxConflicts = -1 );

 /// the model found by `solve()`
 [[nodiscard]] const std::vector< bool > & model( void ) const;

 /// the conflicts met so far
 [[nodiscard]] long conflicts( void ) const { return( f_conflicts ); }

 /// the decisions taken so far
 [[nodiscard]] long decisions( void ) const { return( f_decisions ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the literal ±( v + 1 ) as 2 v + ( negative )
 static int code( int lit ) {
  return( 2 * ( std::abs( lit ) - 1 ) + ( lit < 0 ) );
  }

 /// -1 false, 0 free, 1 true
 [[nodiscard]] int value( int c ) const;

 void enqueue( int c , int reason );
 [[nodiscard]] int propagate( void );      ///< a conflict clause, or -1
 void analyze( int confl , std::vector< int > & learnt , int & back );
 void cancelUntil( int level );
 void bump( int v );
 [[nodiscard]] int pickBranch( void );     ///< a free variable, or -1
 [[nodiscard]] int level( void ) const { return( int( f_trailLim.size() ) ); }
 int addClause( std::vector< int > c );

 Cnf f_F;
 int f_n;
 bool f_empty = false;                      ///< an empty clause or a
                                            ///< conflict at level 0
 std::vector< std::vector< int > > f_C;     ///< the clauses, as codes
 std::vector< std::vector< int > > f_watch; ///< the clauses by literal
 std::vector< int > f_assign , f_level , f_reason;
 std::vector< bool > f_phase , f_seen , f_model;
 std::vector< int > f_trail , f_trailLim;
 std::size_t f_qhead = 0;
 std::vector< double > f_act;
 double f_inc = 1;
 Heap f_heap;
 long f_conflicts = 0 , f_decisions = 0;

 };  // end( class BasicCdcl )

/// the clause learning on `std::priority_queue`
using Cdcl = BasicCdcl<>;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/backtrack/impl/SatSolvers.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* SatSolvers.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File SatSolvers.h --------------------------*/
/*--------------------------------------------------------------------------*/
