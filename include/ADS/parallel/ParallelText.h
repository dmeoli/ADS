/*--------------------------------------------------------------------------*/
/*--------------------------- File ParallelText.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel LCP array, beyond the book: lcpArray( ex , T , SA ) is the
 * array of lcpArray( T , SA ) [see SuffixArray.h], the longest common
 * prefixes of the suffixes adjacent in the suffix array, on the threads of
 * an executor [see Executor.h], by the permuted LCP of Karkkainen, Manzini
 * and Puglisi: \f$ \Phi[ SA[ r ] ] = SA[ r - 1 ] \f$, the suffix before
 * each one in the order of the suffix array, then the permuted array
 * \f$ PLCP[ i ] = lcp( i , \Phi[ i ] ) \f$ in the order of the text, and
 * last \f$ LCP[ r - 1 ] = PLCP[ SA[ r ] ] \f$. The first and the last
 * steps are parallel loops of n scattered moves. The second is the scan of
 * Kasai et al. along the text, which reads T in its order and uses
 * \f$ PLCP[ i ] \geq PLCP[ i - 1 ] - 1 \f$ to resume each comparison where
 * the previous one stopped, \f$ O( n ) \f$ comparisons in all; the text is
 * cut into 4 p parts, and each part starts its scan from h = 0, which
 * costs at most the largest lcp more per part, so that the result is the
 * same and the work \f$ O( n + p \max_i PLCP[ i ] ) \f$ (on a text of
 * one repeated symbol the parts gain nothing, since then the lcp are as
 * long as the text). The scan by the text order reads \f$ \Phi[ i ] \f$
 * and \f$ T[ \Phi[ i ] + h ] \f$ at random once per suffix, where the
 * algorithm of Kasai reads the rank and the suffix array at random.
 *
 * The contracts [see Contract.h]: SA is the suffix array of T (an audit);
 * the result is the LCP array (an audit).
 * \see J. Karkkainen, G. Manzini, S. J. Puglisi, Permuted longest-common-
 *      prefix array, Annual Symposium on Combinatorial Pattern Matching
 *      (CPM), LNCS 5577, 181-192, 2009, doi:10.1007/978-3-642-02441-2_17
 * \see T. Kasai, G. Lee, H. Arimura, S. Arikawa, K. Park, Linear-time
 *      longest-common-prefix computation in suffix arrays and its
 *      applications, Annual Symposium on Combinatorial Pattern Matching
 *      (CPM), LNCS 2089, 181-192, 2001, doi:10.1007/3-540-48194-X_17
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_ParallelText
 #define __ADS_ParallelText
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"
#include "ADS/string/SuffixArray.h"

#include <cstddef>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/// the LCP array of T and its suffix array SA on the threads of ex, n - 1
/// entries (none if n < 2)
/// \pre SA is a suffix array of T (an audit)
template< Executor E , class S >
[[nodiscard]] std::vector< std::size_t > lcpArray( E & ex , const S & T ,
                                       const std::vector< std::size_t > & SA );

/*--------------------------------------------------------------------------*/

template< Executor E , class S >
std::vector< std::size_t > lcpArray( E & ex , const S & T ,
                                     const std::vector< std::size_t > & SA )
{
 const std::size_t n = SA.size();
 ADS_PRE( std::size_t( std::size( T ) ) == n );
 if( ( ex.concurrency() == 1 ) || ( n < ( std::size_t( 1 ) << 15 ) ) )
  return( lcpArray( T , SA ) );
 // Phi, then PLCP in its place
 std::vector< std::size_t > P( n );
 parallelFor( ex , 0 , n , [ & ]( std::size_t r ) {
  P[ SA[ r ] ] = r == 0 ? n : SA[ r - 1 ]; } , 4096 );
 const std::size_t B = std::min( n , 4 * ex.concurrency() );
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) {
  const std::size_t i0 = n * b / B , i1 = n * ( b + 1 ) / B;
  std::size_t h = 0;
  for( std::size_t i = i0 ; i < i1 ; ++i ) {
   const std::size_t j = P[ i ];
   if( j == n ) {                           // the least suffix
    h = 0;
    P[ i ] = 0;
    continue;
    }
   while( ( i + h < n ) && ( j + h < n ) && ( T[ i + h ] == T[ j + h ] ) )
    ++h;
   P[ i ] = h;
   if( h > 0 ) --h;
   }
  } , 1 );
 std::vector< std::size_t > LCP( n - 1 );
 parallelFor( ex , 0 , n - 1 , [ & ]( std::size_t r ) {
  LCP[ r ] = P[ SA[ r + 1 ] ]; } , 4096 );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* ParallelText.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ParallelText.h ------------------------*/
/*--------------------------------------------------------------------------*/
