/*--------------------------------------------------------------------------*/
/*----------------------------- File Catalan.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The numbers of Catalan, 1, 1, 2, 5, 14, 42, ...: \f$ C_n \f$ counts the
 * binary trees of n nodes (so the binary search trees on n keys, the trees
 * whose symmetric visit is 1 .. n), the strings of n pairs of balanced
 * parentheses, the full binary trees of n + 1 leaves (the
 * parenthesizations of a product of n + 1 factors), the triangulations of a
 * convex polygon of n + 2 sides, and the monotone lattice paths that never
 * cross the diagonal.
 *
 * catalan() fills the table of the recurrence on the size of the left
 * subtree, \f$ C_0 = 1 \f$ and
 * \f$ C_n = \sum_{k=0}^{n-1} C_k C_{n-1-k} \f$, with
 * \f$ \Theta( n^2 ) \f$ operations on numbers of \f$ \Theta( n ) \f$ bits,
 * and with T = BigInteger for any n [see BigInteger.h]. The closed form
 * \f$ C_n = \binom{2n}{n} / ( n + 1 ) \f$ and the recurrence
 * \f$ C_{n+1} = 2 ( 2 n + 1 ) C_n / ( n + 2 ) \f$ take O( n ) operations,
 * but their intermediate values exceed the result, so that in 64 bits they
 * stop before the table does, which reaches \f$ C_{36} \f$;
 * asymptotically \f$ C_n \sim 4^n / ( n^{3/2} \sqrt{\pi} ) \f$.
 *
 * The contracts [see Contract.h]: n >= 0, and \f$ C_n \f$ representable in
 * T (n <= 36 for 64 bits unsigned). The functions are defined in
 * Catalan.tpp.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 13,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf
 * \see R. P. Stanley, Catalan Numbers, Cambridge University Press, 2015,
 *      doi:10.1017/CBO9781139871495
 * \see D. E. Knuth, The Art of Computer Programming, vol. 1, 3rd ed.,
 *      Addison-Wesley, 1997, section 2.3.4.4
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

#ifndef __ADS_Catalan
 #define __ADS_Catalan
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the n-th number of Catalan, the binary trees of n nodes
/// \pre n >= 0 and C_n representable in T
template< class T = std::uint64_t >
T catalan( int n );

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/Catalan.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Catalan.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Catalan.h ---------------------------*/
/*--------------------------------------------------------------------------*/
