/*--------------------------------------------------------------------------*/
/*------------------------ File Pseudopolynomial.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The pseudopolynomial algorithms of section 19.1 and of the exercises
 * 19.1 to 19.5. An algorithm is pseudopolynomial if its time is a
 * polynomial in the size \f$ d \f$ of the input and in the largest integer
 * \f$ \# \f$ in it: `subsetSumDP()` takes \f$ O( n k ) \f$, polynomial if
 * \f$ k \f$ is bounded by a polynomial in \f$ n \f$ and exponential in
 * \f$ d = O( n \log k ) \f$ otherwise. A problem is strongly NP-complete if
 * it stays NP-complete when \f$ \# \f$ is bounded by a polynomial in
 * \f$ d \f$ (CLIQUE, TRAVELING SALESMAN, 3-PARTITION); none of them has a
 * pseudopolynomial algorithm unless P = NP (Theorem 19.1), while SUBSET SUM,
 * PARTITION and KNAPSACK are weakly NP-complete and have one.
 *
 * The scheduling of \f$ n \f$ programs of times \f$ t_i \f$ on \f$ m \f$
 * processors within \f$ d \f$ is pseudopolynomial by dynamic programming on
 * the loads of the processors, \f$ O( n ( d + 1 )^m ) \f$; if a program may
 * be split in intervals, the rule of McNaughton fills the processors one
 * after the other up to
 * \f$ d = \max\{ \max_i t_i , \sum_i t_i / m \} \f$ in \f$ O( n ) \f$ time,
 * and the periodic tasks of REAL-TIME SCHEDULING, with
 * \f$ t_i = C_i / T_i \f$ and \f$ d = 1 \f$, are scheduled the same way when
 * no priority rule is required.
 *
 * The contracts [see `Contract.h`]: the numbers are not negative and the
 * processors at least one; each solution returned is checked as a
 * postcondition, the preemptive schedule of McNaughton included. The
 * operations are defined in `Pseudopolynomial.tpp`.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.1 and
 *      exercises 19.1-19.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see R. McNaughton, Scheduling with deadlines and loss functions,
 *      Management Science 6(1), 1-12, 1959, doi:10.1287/mnsc.6.1.1
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

#ifndef __ADS_Pseudopolynomial
 #define __ADS_Pseudopolynomial
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/complexity/Problems.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- SUBSET SUM ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// whether some elements of \f$ A \f$ sum to \f$ k \f$ (false if
/// \f$ k < 0 \f$), by the vector \f$ M \f$ with \f$ M[ j ] \f$ true if some
/// of \f$ A[ 0 .. i ] \f$ sum to \f$ j \f$; the vector \f$ M \f$ after each
/// element goes to `*rows`, if given, the first one before any;
/// \f$ O( n k ) \f$ time; \f$ M \f$ is a vector of `Bits`,
/// `std::vector< bool >` by default, or `boost::dynamic_bitset`, or
/// `BitVector` of ADS [see `BitVector.h`], with the same cells written
///
/// In pseudocode (section 19.1), one row of the table overwritten, from the
/// right so that each element is used once:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{subsetSumDP}( A , k )\quad \triangleright
///     \ M[ j ] \text{: some subset of the items seen sums to } j \cr
/// 2 & \quad \textbf{if}\ k < 0\ \textbf{then return false} \cr
/// 3 & \quad M \gets ( \textbf{true} , \textbf{false} , \ldots ,
///     \textbf{false} ) \text{, of } k + 1 \text{ entries} \cr
/// 4 & \quad \textbf{foreach}\ a \in A\ \textbf{do}\ M[ j ] \gets M[ j ]
///     \text{ or } M[ j - a ] \text{ for } j = k , k - 1 , \ldots , a\quad
///     \triangleright\ \text{downwards: } a \text{ once} \cr
/// 5 & \quad \textbf{return}\ M[ k ]
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not negative
/// \post `*rows`, if given, has \f$ n + 1 \f$ rows (none if \f$ k < 0 \f$)
template< class Bits = std::vector< bool > >
bool subsetSumDP( const std::vector< long > & A , long k ,
		  std::vector< std::vector< bool > > * rows = nullptr );

/// the places of a subset of \f$ A \f$ of sum \f$ k \f$, by the table of all
/// the rows, \f$ O( n k ) \f$ time and space, or nothing
///
/// In pseudocode (section 19.1), the subset read backwards from the table,
/// an element needed when the row above has not the sum:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{subsetSumSet}( A , k )\quad
///     \triangleright\ M[ i ][ j ] \text{ for the first } i \text{ items} \cr
/// 2 & \quad \textbf{if}\ k < 0\ \textbf{then return nil} \cr
/// 3 & \quad M[ 0 ][ j ] \gets ( j = 0 ) \text{ for every } j \cr
/// 4 & \quad M[ i + 1 ][ j ] \gets M[ i ][ j ] \text{ or } ( A[ i ] \le j
///     \text{ and } M[ i ][ j - A[ i ] ] ) \text{, for } i < n \text{ and }
///     j \le k \cr
/// 5 & \quad \textbf{if}\ \textbf{not}\ M[ n ][ k ]
///     \ \textbf{then return nil} \cr
/// 6 & \quad S \gets \emptyset;\ j \gets k \cr
/// 7 & \quad \textbf{for}\ i \gets n\ \textbf{downto}\ 1\ \textbf{do}\quad
///     \triangleright\ \text{the item } i - 1 \text{ is needed iff } j
///     \text{ was not reachable without it} \cr
/// 8 & \qquad \textbf{if}\ \textbf{not}\ M[ i - 1 ][ j ]\ \textbf{then}\ S
///     \gets S \cup \{ i - 1 \};\ j \gets j - A[ i - 1 ] \cr
/// 9 & \quad \textbf{return}\ S
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not negative
/// \post the places returned are distinct, of sum \f$ k \f$
inline std::optional< Nodes > subsetSumSet( const std::vector< long > & A ,
					    long k );

/// PARTITION by dynamic programming: the places of a half of \f$ A \f$ of
/// half the sum, or nothing (exercise 19.1)
///
/// In pseudocode (exercise 19.1):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{partitionDP}( A )\quad \triangleright
///     \ \text{two halves of equal sum} \cr
/// 2 & \quad s \gets \textstyle\sum_i A[ i ] \cr
/// 3 & \quad \textbf{return}\ \textbf{nil} \text{ if } s
///     \text{ is odd, else } \mathrm{subsetSumSet}( A , s / 2 )
/// \end{array}}
/// \f]
/// \pre \f$ A \f$ is not negative
/// \post the places returned are of half the sum
inline std::optional< Nodes > partitionDP( const std::vector< long > & A );

/*--------------------------------------------------------------------------*/
/*---------------------------- SCHEDULING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// MULTIPROCESSOR SCHEDULING by dynamic programming on the vectors of the
/// loads of the \f$ m \f$ processors, each at most \f$ d \f$: the processor
/// of each program, or nothing (also if \f$ d < 0 \f$, even with no
/// programs, since an idle processor is loaded 0);
/// \f$ O( n ( d + 1 )^m ) \f$ time (exercise 19.2)
///
/// In pseudocode (exercise 19.2), \f$ L_i \f$ the vectors of the loads
/// reachable with the first \f$ i \f$ programs, each with the vector it
/// comes from and the processor of the \f$ i \f$-th program:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{schedulingDP}( t , m , d )\quad
///     \triangleright\ L_i \text{ the loads of the } m
///     \text{ processors after } i \text{ jobs} \cr
/// 2 & \quad \textbf{if}\ d < 0\ \textbf{then return nil} \cr
/// 3 & \quad L_0 \gets \{ ( 0 , \ldots , 0 ) \} \cr
/// 4 & \quad L_{i+1} \gets \{ x + t_i e_q : x \in L_i \text{, } x_q + t_i
///     \le d \} \text{ for } i = 0 , \ldots , n - 1\quad \triangleright
///     \ \text{a set: no copies} \cr
/// 5 & \quad \textbf{return}\ \textbf{nil} \text{ if } L_n = \emptyset
///     \text{, else the processor of each job, read back from a vector of }
///     L_n
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$ and \f$ t \f$ is not negative
/// \post each processor returned is loaded at most \f$ d \f$
inline std::optional< std::vector< int > >
schedulingDP( const std::vector< long > & t , int m , long d );

/// an interval \f$ [ \mathit{start} , \mathit{end} ) \f$ of the program on a
/// processor
struct Piece
{
 int program;   ///< the program
 double start ,  ///< the time at which the piece starts
   end;         ///< the time at which the piece ends
 };

/// the rule of McNaughton: the length \f$ d \f$ of the schedule and the
/// pieces of each processor; the programs fill a processor up to \f$ d \f$
/// and go on at time 0 of the next one, and the two pieces of a program do
/// not overlap because \f$ t[ i ] \le d \f$; \f$ O( n + m ) \f$ time
/// (exercise 19.3)
///
/// In pseudocode (exercise 19.3), \f$ d \f$ the least length possible, since
/// no program is shorter than its time and the work cannot be done faster
/// than in \f$ \sum_i t_i / m \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{mcNaughton}( t , m )\quad \triangleright
///     \ d \text{ the optimum, a lower bound met} \cr
/// 2 & \quad d \gets \max\{ \max_i t_i , \textstyle\sum_i t_i / m \};\ q
///     \gets 0;\ \mathit{now} \gets 0\quad \triangleright
///     \ \text{the processor and the time filled} \cr
/// 3 & \quad \textbf{foreach}\ \text{job } i \text{, in order}\ \textbf{do}
///     \cr
/// 4 & \qquad r \gets t_i \cr
/// 5 & \qquad \textbf{while}\ r > 0 \text{ and } q < m\ \textbf{do}\quad
///     \triangleright\ \text{a job split ends on } q
///     \text{ and restarts on } q + 1 \cr
/// 6 & \qquad\quad s \gets \min\{ r , d - \mathit{now} \};\ \text{give } i
///     \text{ the time } [ \mathit{now} , \mathit{now} + s ) \text{ of } q
///     \cr
/// 7 & \qquad\quad r \gets r - s;\ \mathit{now} \gets \mathit{now} + s;
///     \ \textbf{if}\ \mathit{now} = d\ \textbf{then}\ ( q , \mathit{now} )
///     \gets ( q + 1 , 0 ) \cr
/// 8 & \quad \textbf{return}\ ( d , \text{the pieces} )
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$ and \f$ t \f$ is not negative
/// \post each program runs for its time within \f$ [ 0 , d ] \f$, never on
///       two processors at once
inline std::pair< double , std::vector< std::vector< Piece > > >
mcNaughton( const std::vector< double > & t , int m );

/// REAL-TIME SCHEDULING without priority rule: the tasks of time
/// \f$ C[ i ] \f$ every \f$ T[ i ] \f$ run on \f$ m \f$ processors if and
/// only if \f$ C[ i ] \le T[ i ] \f$ and \f$ \sum_i C_i / T_i \le m \f$; the
/// schedule of a period of length 1, task \f$ i \f$ for the fraction
/// \f$ C[ i ] / T[ i ] \f$ of it, or nothing (exercise 19.4)
///
/// In pseudocode (exercise 19.4):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{realTimeScheduling}( C , T , m )\quad
///     \triangleright\ \text{the utilizations as the lengths of the jobs} \cr
/// 2 & \quad ( d , P ) \gets \mathrm{mcNaughton}( ( C_i / T_i : i = 1 ,
///     \ldots , n ) , m ) \cr
/// 3 & \quad \textbf{return}\ P \text{ if } d \le 1 \text{, else }
///     \textbf{nil}
/// \end{array}}
/// \f]
/// \pre \f$ m \ge 1 \f$, \f$ C \f$ and \f$ T \f$ of the same size,
///      \f$ C \ge 0 \f$, \f$ T > 0 \f$
/// \post a schedule returned has \f$ m \f$ processors
inline std::optional< std::vector< std::vector< Piece > > >
realTimeScheduling( const std::vector< double > & C ,
		    const std::vector< double > & T , int m );

/// the thief of each jewel of price \f$ P[ i ] \f$, so that the three
/// thieves take \f$ m / 3 \f$ each, or nothing; \f$ S[ i ][ j ][ h ] \f$
/// tells whether the first \f$ i \f$ jewels give \f$ j \f$ to the first and
/// \f$ h \f$ to the second thief, and the prices are not negative, so that
/// \f$ j \f$ and \f$ h \f$ range in \f$ [ 0 , m / 3 ] \f$;
/// \f$ O( n m^2 ) \f$ time (exercise 19.5)
///
/// In pseudocode (exercise 19.5), the third thief taking what is left:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bottino}( P )\quad \triangleright\ S[ i ][
///     j ][ h ] \text{: the first } i \text{ items give } j
///     \text{ to one thief and } h \text{ to another} \cr
/// 2 & \quad m \gets \textstyle\sum_i P[ i ];\ \textbf{if}\ 3 \nmid m
///     \ \textbf{then return nil} \cr
/// 3 & \quad S[ 0 ][ j ][ h ] \gets ( j = h = 0 ) \cr
/// 4 & \quad \textbf{foreach}\ i \in [ 0 , n ) \text{ and } j , h \in [ 0 ,
///     m / 3 ]\ \textbf{do}\quad \triangleright\ \text{the item } i
///     \text{ to the third thief, the first or the second} \cr
/// 5 & \qquad S[ i + 1 ][ j ][ h ] \gets S[ i ][ j ][ h ] \text{ or } S[ i
///     ][ j - P[ i ] ][ h ] \text{ or } S[ i ][ j ][ h - P[ i ] ] \cr
/// 6 & \quad \textbf{return}\ \textbf{nil} \text{ if not } S[ n ][ m / 3 ][
///     m / 3 ] \text{, else the thief of each item, read back from it}
/// \end{array}}
/// \f]
/// \pre the prices are not negative
/// \post an assignment returned gives a third of the value to each thief
inline std::optional< std::vector< int > >
bottino( const std::vector< long > & P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/impl/Pseudopolynomial.tpp"

/*--------------------------------------------------------------------------*/

#endif /* Pseudopolynomial.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File Pseudopolynomial.h -----------------------*/
/*--------------------------------------------------------------------------*/
