/*--------------------------------------------------------------------------*/
/*------------------------ File Pseudopolynomial.h -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The pseudopolynomial algorithms of section 19.1 and of the exercises
 * 19.1 to 19.5. An algorithm is pseudopolynomial if its time is a
 * polynomial in the size d of the input and in the largest integer # in
 * it: subsetSumDP() takes \f$ O( n k ) \f$, polynomial if k is bounded by
 * a polynomial in n and exponential in \f$ d = O( n \log k ) \f$
 * otherwise. A problem is strongly NP-complete if it stays NP-complete
 * when # is bounded by a polynomial in d (CRICCA, COMMESSO VIAGGIATORE,
 * 3-PARTIZIONE); none of them has a pseudopolynomial algorithm unless
 * P = NP (Teorema 19.1), while SOMMA DI SOTTOINSIEME, PARTIZIONE and ZAINO
 * are weakly NP-complete and have one.
 *
 * The scheduling of n programs of times \f$ t_i \f$ on m processors within
 * d is pseudopolynomial by dynamic programming on the loads of the
 * processors, \f$ O( n ( d + 1 )^m ) \f$; if a program may be split in
 * intervals, the rule of McNaughton fills the processors one after the
 * other up to \f$ d = \max\{ \max_i t_i , \sum_i t_i / m \} \f$ in O( n )
 * time, and the periodic tasks of REAL-TIME SCHEDULING, with
 * \f$ t_i = C_i / T_i \f$ and d = 1, are scheduled the same way when no
 * priority rule is required.
 *
 * The contracts [see Contract.h]: the numbers are not negative and the
 * processors at least one; each solution returned is checked as a
 * postcondition, the preemptive schedule of McNaughton included. The
 * operations are defined in Pseudopolynomial.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 19.1 and
 *      exercises 19.1-19.5,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf
 * \see R. McNaughton, Scheduling with deadlines and loss functions,
 *      Management Science 6(1), 1-12, 1959, doi:10.1287/mnsc.6.1.1
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- SUBSET SUM ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// subsetSum(): M[ j ] tells whether some of A[ 0 .. i ] sum to j; the
/// vector M after each element goes to *rows, if given, the first one
/// before any; O( n k ) time
/// \pre k >= 0 and A is not negative
/// \post *rows, if given, has n + 1 rows
inline bool subsetSumDP( const std::vector< long > & A , long k ,
                         std::vector< std::vector< bool > > * rows = nullptr );

/// the places of a subset of A of sum k, by the table of all the rows,
/// O( n k ) time and space, or nothing
/// \pre A is not negative
/// \post the places returned are distinct, of sum k
inline std::optional< Nodes > subsetSumSet( const std::vector< long > & A ,
                                            long k );

/// PARTIZIONE by dynamic programming: the places of a half of A of half
/// the sum, or nothing (exercise 19.1)
/// \pre A is not negative
/// \post the places returned are of half the sum
inline std::optional< Nodes > partitionDP( const std::vector< long > & A );

/*--------------------------------------------------------------------------*/
/*---------------------------- SCHEDULING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// SCHEDULING MULTIPROCESSORE by dynamic programming on the vectors of
/// the loads of the m processors, each at most d: the processor of each
/// program, or nothing; O( n ( d + 1 )^m ) time (exercise 19.2)
/// \pre m >= 1 and t is not negative
/// \post each processor returned is loaded at most d
inline std::optional< std::vector< int > > schedulingDP(
                        const std::vector< long > & t , int m , long d );

/// an interval [ start , end ) of the program on a processor
struct Piece {
 int program;
 double start , end;
 };

/// the rule of McNaughton: the length d of the schedule and the pieces
/// of each processor; the programs fill a processor up to d and go on at
/// time 0 of the next one, and the two pieces of a program do not overlap
/// because t[ i ] <= d; O( n + m ) time (exercise 19.3)
/// \pre m >= 1 and t is not negative
/// \post each program runs for its time within [ 0 , d ], never on two
///       processors at once
inline std::pair< double , std::vector< std::vector< Piece > > > mcNaughton(
                        const std::vector< double > & t , int m );

/// REAL-TIME SCHEDULING without priority rule: the tasks of time C[ i ]
/// every T[ i ] run on m processors if and only if C[ i ] <= T[ i ] and
/// \f$ \sum_i C_i / T_i \leq m \f$; the schedule of a period of length 1,
/// task i for the fraction C[ i ] / T[ i ] of it, or nothing (exercise
/// 19.4)
/// \pre m >= 1, C and T of the same size, C >= 0, T > 0
/// \post a schedule returned has m processors
inline std::optional< std::vector< std::vector< Piece > > > realTimeScheduling(
                        const std::vector< double > & C ,
                        const std::vector< double > & T , int m );

/// bottino(): the thief of each jewel of price P[ i ], so that the three
/// thieves take m / 3 each, or nothing; S[ i ][ j ][ h ] tells whether the
/// first i jewels give j to the first and h to the second thief, and the
/// prices are positive, so that j and h range in [ 0 , m / 3 ];
/// O( n m^2 ) time (exercise 19.5)
/// \pre the prices are not negative
/// \post an assignment returned gives a third of the value to each thief
inline std::optional< std::vector< int > > bottino(
                        const std::vector< long > & P );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/intractable/Pseudopolynomial.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Pseudopolynomial.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File Pseudopolynomial.h -----------------------*/
/*--------------------------------------------------------------------------*/
