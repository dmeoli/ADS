/*--------------------------------------------------------------------------*/
/*-------------------------- File DidacticSorts.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Three sorts that are slow on purpose, beyond the book, as examples of the
 * analysis of recurrences and of the expected time:
 *
 * - `stoogeSort()` sorts the first two thirds, the last two thirds and the
 *   first two thirds again, \f$ T( n ) = 3 T( 2 n / 3 ) + O( 1 ) \f$, i.e.,
 *   \f$ \Theta( n^{\log_{1.5} 3} ) = O( n^{2.71} ) \f$ by the master theorem
 *   [see `MasterTheorem.h`];
 * - `slowSort()` of Broder and Stolfi (multiply and surrender): the maximum
 *   of the two halves, sorted recursively, goes to the end and the rest is
 *   sorted again, \f$ T( n ) = 2 T( n / 2 ) + T( n - 1 ) + 1 \f$, which is not
 *   even polynomial;
 * - `bogoSort()` shuffles the vector until it is sorted, \f$ n! / s \f$
 *   shuffles expected for \f$ s \f$ sorted orders, each \f$ O( n ) \f$.
 *
 * The contracts [see `Contract.h`]: `bogoSort()` needs \f$ n \le 10 \f$, so
 * that it ends in a reasonable time; each sort ends with the same items
 * sorted, an audit. The functions are defined in `DidacticSorts.tpp`.
 *
 * \see T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, Introduction
 *      to Algorithms, 3rd ed., MIT Press, 2009, problem 7-3 (stooge sort)
 * \see A. Broder, J. Stolfi, Pessimal algorithms and simplexity analysis,
 *      ACM SIGACT News 16(3), 49-53, 1984, doi:10.1145/990534.990536
 * \see H. Gruber, M. Holzer, O. Ruepp, Sorting the slow way: an analysis
 *      of perversely awful randomized sorting algorithms, Proceedings of FUN
 *      2007, LNCS 4475, 183-197, 2007, doi:10.1007/978-3-540-72914-3_17
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

#ifndef __ADS_DidacticSorts
 #define __ADS_DidacticSorts
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/sort/Sorted.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <random>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the stooge sort
///
/// In pseudocode (problem 7-3 of Cormen, Leiserson, Rivest and Stein, beyond
/// the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{stooge}( A , i , j ) \cr
/// 2 & \quad \textbf{if}\ A[ j ] < A[ i ]\ \textbf{then}\ A[ i ]
///     \leftrightarrow A[ j ] \cr
/// 3 & \quad \textbf{if}\ j - i + 1 \ge 3\ \textbf{then} \cr
/// 4 & \qquad t \gets \lfloor ( j - i + 1 ) / 3 \rfloor \cr
/// 5 & \qquad \mathrm{stooge}( A , i , j - t );\ \mathrm{stooge}( A , i + t ,
///     j );\ \mathrm{stooge}( A , i , j - t ) \cr
/// 6 & \textbf{procedure}\ \mathrm{stoogeSort}( A , n ) \cr
/// 7 & \quad \textbf{if}\ n > 1\ \textbf{then}\ \mathrm{stooge}( A , 0 , n - 1
///     )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void stoogeSort( std::vector< T > & A , C less = C() );

/// the slow sort of Broder and Stolfi
///
/// In pseudocode (Broder and Stolfi, beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{procedure}\ \mathrm{slow}( A , i , j ) \cr
/// 2 & \quad \textbf{if}\ i \ge j\ \textbf{then return} \cr
/// 3 & \quad m \gets \lfloor ( i + j ) / 2 \rfloor \cr
/// 4 & \quad \mathrm{slow}( A , i , m );\ \mathrm{slow}( A , m + 1 , j ) \cr
/// 5 & \quad \textbf{if}\ A[ j ] < A[ m ]\ \textbf{then}\ A[ m ]
///     \leftrightarrow A[ j ]\quad \triangleright\ \text{the maximum to the }
///     \text{end} \cr
/// 6 & \quad \mathrm{slow}( A , i , j - 1 ) \cr
/// 7 & \textbf{procedure}\ \mathrm{slowSort}( A , n ) \cr
/// 8 & \quad \textbf{if}\ n > 1\ \textbf{then}\ \mathrm{slow}( A , 0 , n - 1 )
/// \end{array}}
/// \f]
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class C = std::less<> >
void slowSort( std::vector< T > & A , C less = C() );

/// the bogo sort with the generator \f$ g \f$; the number of shuffles
///
/// In pseudocode (beyond the book):
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{bogoSort}( A , g ) \cr
/// 2 & \quad s \gets 0 \cr
/// 3 & \quad \textbf{while}\ A \text{ is not sorted}\ \textbf{do}
///     \ \text{shuffle } A \text{ by } g;\ s \gets s + 1 \cr
/// 4 & \quad \textbf{return}\ s
/// \end{array}}
/// \f]
/// \pre \f$ n \le 10 \f$
/// \post \f$ A \f$ is sorted, with the same items (audit)
template< class T , class R , class C = std::less<> >
long bogoSort( std::vector< T > & A , R & g , C less = C() );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sort/impl/DidacticSorts.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* DidacticSorts.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File DidacticSorts.h ------------------------*/
/*--------------------------------------------------------------------------*/
