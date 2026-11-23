/*--------------------------------------------------------------------------*/
/*--------------------------- File ParallelText.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel LCP and suffix arrays, beyond the book:
 * `lcpArray( ex , T , SA )` is the array of `lcpArray( T , SA )` [see
 * `SuffixArray.h`], the longest common prefixes of the suffixes adjacent in
 * the suffix array, on the threads of an executor [see `Executor.h`], by
 * the permuted LCP of Kärkkäinen, Manzini and Puglisi:
 * \f$ \Phi[ \mathit{SA}[ r ] ] = \mathit{SA}[ r - 1 ] \f$, the
 * suffix before each one in the order of the suffix array, then the permuted
 * array \f$ \mathit{PLCP}[ i ] = \mathrm{lcp}( i , \Phi[ i ] ) \f$ in the
 * order of the text, and last
 * \f$ \mathit{LCP}[ r - 1 ] = \mathit{PLCP}[ \mathit{SA}[ r ] ] \f$. The
 * first and the last steps are parallel loops of \f$ n \f$ scattered moves.
 * The second is the scan of Kasai et al. along the text, which reads
 * \f$ T \f$ in its order and uses
 * \f$ \mathit{PLCP}[ i ] \ge \mathit{PLCP}[ i - 1 ] - 1 \f$ to resume each
 * comparison where the previous one stopped, \f$ O( n ) \f$ comparisons in
 * all; the text is cut into \f$ 4 p \f$ parts, and each part starts its scan
 * from \f$ h = 0 \f$, which costs at most the largest lcp more per part, so
 * that the result is the same and the work
 * \f$ O( n + p \max_i \mathit{PLCP}[ i ] ) \f$ (on a text of one repeated
 * symbol the parts gain nothing, since then the lcp are as long as the
 * text). The scan by the text order reads \f$ \Phi[ i ] \f$ and
 * \f$ T[ \Phi[ i ] + h ] \f$ at random once per suffix, where the algorithm
 * of Kasai reads the rank and the suffix array at random.
 *
 * `lcpArrayPhi( ex , T , SA )` does the same on a text of bytes and a suffix
 * array of indices of the type `I` (e.g., 32 bits), the result of
 * `lcpArrayPhi( T , SA )` [see `SuffixSorting.h`]: its scans compare the
 * first 4 symbols one by one and then 8 at a time by a load of 8 bytes, the
 * first different byte found by the trailing zeros of the exclusive or, and
 * fetch the text at \f$ \Phi[ i + 16 ] \f$ ahead of its use.
 *
 * `suffixArray( ex , T )` is the suffix array of a text of bytes, the result
 * of `suffixArray< I >( T )` [see `SuffixSorting.h`], built by the prefix
 * doubling of Manber and Myers with the discarding of the suffixes already
 * sorted, as in the suffix array of ParlayLib and PBBS. After the round
 * \f$ h \f$ the suffixes are sorted by their first \f$ 2 h \f$ symbols,
 * and only the segments of suffixes still tied are sorted again, by the
 * rank of the suffix \f$ h \f$ places on (the first place of its segment);
 * a segment of one suffix is dropped. The first round sorts the words of
 * \f$ w = \lfloor 64 / b \rfloor \f$ codes of
 * \f$ b = \lceil \log_2 ( \sigma + 1 ) \rceil \f$ bits of the
 * \f$ \sigma \f$ bytes of \f$ T \f$ (the packing of the alphabet of the
 * distributed constructions of Kurpicz et al. and of Haag et al.), and every
 * sort is the stable `integerSort()` [see `ParallelPrimitives.h`], the
 * small segments each on one thread and the large ones on all the threads.
 * The work is \f$ O( n \log n ) \f$ in the worst case (a text of one
 * repeated symbol) and close to linear when the suffixes drop out early, as
 * on most texts, the depth that of \f$ O( \log n ) \f$ rounds of sorts and
 * scans, and the memory at most \f$ 36 n \f$ bytes with indices of 32 bits
 * and \f$ 48 n \f$ with 64.
 *
 * The contracts [see `Contract.h`]: \f$ \mathit{SA} \f$ is the suffix array
 * of \f$ T \f$ (an audit); the result is the LCP array (an audit), and the
 * suffix array built is one (an audit).
 *
 * \see J. Kärkkäinen, G. Manzini, S. J. Puglisi, Permuted longest-common-
 *      prefix array, Annual Symposium on Combinatorial Pattern Matching
 *      (CPM), LNCS 5577, 181-192, 2009, doi:10.1007/978-3-642-02441-2_17
 * \see T. Kasai, G. Lee, H. Arimura, S. Arikawa, K. Park, Linear-time
 *      longest-common-prefix computation in suffix arrays and its
 *      applications, Annual Symposium on Combinatorial Pattern Matching
 *      (CPM), LNCS 2089, 181-192, 2001, doi:10.1007/3-540-48194-X_17
 * \see U. Manber, G. Myers, Suffix arrays: a new method for on-line string
 *      searches, SIAM Journal on Computing 22(5), 935-948, 1993,
 *      doi:10.1137/0222058
 * \see R. Dementiev, J. Kärkkäinen, J. Mehnert, P. Sanders, Better external
 *      memory suffix array construction, ACM Journal of Experimental
 *      Algorithmics 12, 3.4:1-24, 2008, doi:10.1145/1227161.1402296
 * \see J. Shun, G. E. Blelloch, J. T. Fineman, P. B. Gibbons, A. Kyrola,
 *      H. V. Simhadri, K. Tangwongsan, Brief announcement: the Problem Based
 *      Benchmark Suite, Proceedings of the 24th ACM Symposium on Parallelism
 *      in Algorithms and Architectures (SPAA), 68-70, 2012,
 *      doi:10.1145/2312005.2312018
 * \see F. Kurpicz, P. Mehnert, P. Sanders, M. Schimek, Scalable distributed
 *      string sorting, 32nd Annual European Symposium on Algorithms (ESA),
 *      LIPIcs 308, 83:1-83:17, 2024, doi:10.4230/LIPIcs.ESA.2024.83
 * \see M. Haag, F. Kurpicz, P. Sanders, M. Schimek, Fast and lightweight
 *      distributed suffix array construction, 33rd Annual European
 *      Symposium on Algorithms (ESA), LIPIcs 351, 47, 2025,
 *      doi:10.4230/LIPIcs.ESA.2025.47
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

#ifndef __ADS_ParallelText
 #define __ADS_ParallelText
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/memory/Prefetch.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelPrimitives.h"
#include "ADS/string/SuffixArray.h"
#include "ADS/string/SuffixSorting.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <span>
#include <type_traits>
#include <string_view>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/// the LCP array of \f$ T \f$ and of its suffix array \f$ \mathit{SA} \f$
/// on the threads of `ex`, \f$ n - 1 \f$ entries (none if \f$ n < 2 \f$)
///
/// In pseudocode (the permuted LCP of Kärkkäinen, Manzini and Puglisi on
/// the scan of Kasai et al., beyond the book), \f$ T[ 0 .. n - 1 ] \f$ and
/// \f$ \mathit{SA}[ 0 .. n - 1 ] \f$:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{lcpArray}( \mathit{ex} , T , \mathit{SA} )
///     \cr
/// 2 & \quad \textbf{if}\ p = 1\ \textbf{or}\ n < 2^{15}
///     \ \textbf{then return}\ \mathrm{lcpArray}( T , \mathit{SA} ) \cr
/// 3 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ n - 1
///     \ \textbf{do in parallel}\ \Phi[ \mathit{SA}[ r ] ] \gets \mathit{SA}[
///     r
///     - 1 ]\quad \triangleright\ \textbf{nil}\ \text{for } r = 0 \cr
/// 4 & \quad B \gets \min\{ n , 4 p \} \cr
/// 5 & \quad \textbf{for}\ b \gets 0\ \textbf{to}\ B - 1
///     \ \textbf{do in parallel} \cr
/// 6 & \qquad h \gets 0 \cr
/// 7 & \qquad \textbf{for}\ i \gets \lfloor n b / B \rfloor\ \textbf{to}
///     \ \lfloor n ( b + 1 ) / B \rfloor - 1\ \textbf{do} \cr
/// 8 & \qquad\quad j \gets \Phi[ i ] \cr
/// 9 & \qquad\quad \textbf{if}\ j = \textbf{nil}\ \textbf{then}\ h \gets 0;
///     \ \mathit{PLCP}[ i ] \gets 0;\ \textbf{continue} \cr
/// 10 & \qquad\quad \textbf{while}\ i + h < n\ \textbf{and}\ j + h < n
///     \ \textbf{and}\ T[ i + h ] = T[ j + h ]\ \textbf{do}\ h \gets h + 1 \cr
/// 11 & \qquad\quad \mathit{PLCP}[ i ] \gets h;\ h \gets \max\{ h - 1 , 0
///     \} \cr
/// 12 & \quad \textbf{for}\ r \gets 0\ \textbf{to}\ n - 2
///     \ \textbf{do in parallel}\ \mathit{LCP}[ r ] \gets \mathit{PLCP}[
///     \mathit{SA}[ r + 1 ] ] \cr
/// 13 & \quad \textbf{return}\ \mathit{LCP}
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{SA} \f$ is a suffix array of \f$ T \f$ (an audit)
template< Executor E , class S >
[[nodiscard]] std::vector< std::size_t >
lcpArray( E & ex , const S & T , const std::vector< std::size_t > & SA );

/// the LCP array of the text \f$ T \f$ of bytes and of its suffix array
/// \f$ \mathit{SA} \f$ of indices of the type `I`, on the threads of `ex`,
/// the result of `lcpArrayPhi( T , SA )` [see `SuffixSorting.h`]
///
/// In pseudocode (beyond the book), that of `lcpArray()` above with its line
/// 10 comparing 8 bytes at a time:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 10 & \qquad\quad h \gets \mathrm{lcp}( i , j , h )\quad \triangleright
///     \ \text{4 bytes one by one, then words of 8 bytes} \cr
/// 11 & \textbf{function}\ \mathrm{lcp}( i , j , h ) \cr
/// 12 & \quad \mathit{far} \gets n - \max\{ i , j \} \cr
/// 13 & \quad \textbf{while}\ h < \min\{ \mathit{far} , h_0 + 4 \}
///     \ \textbf{and}\ T[ i + h ] = T[ j + h ]\ \textbf{do}\ h \gets h +
///     1\quad
///     \triangleright\ h_0 \text{ the } h \text{ given} \cr
/// 14 & \quad \textbf{if}\ \text{all 4 matched}\ \textbf{then} \cr
/// 15 & \qquad \textbf{while}\ h + 8 \le \mathit{far}\ \textbf{and}\ T[ i + h
///     .. i + h + 7 ] = T[ j + h .. j + h + 7 ]\ \textbf{do}\ h \gets h + 8
///     \cr
/// 16 & \qquad \textbf{if}\ h + 8 \le \mathit{far}\ \textbf{then}\ h \gets h
///     + \text{the first byte where they differ} \cr
/// 17 & \qquad \textbf{else while}\ h < \mathit{far}\ \textbf{and}\ T[ i + h
///     ] = T[ j + h ]\ \textbf{do}\ h \gets h + 1 \cr
/// 18 & \quad \textbf{return}\ h
/// \end{array}}
/// \f]
/// \pre \f$ \mathit{SA} \f$ is a suffix array of \f$ T \f$
template< Executor E , class I >
[[nodiscard]] std::vector< I > lcpArrayPhi( E & ex , std::string_view T ,
					    const std::vector< I > & SA );

/// the suffix array of the bytes of \f$ T \f$ on the threads of `ex`, on
/// integers `I` (32 or 64 bits, unsigned): the result of
/// `suffixArray< I >( T )` [see `SuffixSorting.h`], which it is when
/// \f$ p = 1 \f$ or \f$ n < 2^{15} \f$
///
/// In pseudocode (the prefix doubling of Manber and Myers with the discarding
/// of the sorted suffixes, as engineered in ParlayLib, beyond the book),
/// \f$ c( x ) \in [ 1 , \sigma ] \f$ the rank of the byte \f$ x \f$ among
/// the \f$ \sigma \f$ bytes of \f$ T \f$ and \f$ c( x ) = 0 \f$ past its
/// end, \f$ b = \lceil \log_2 ( \sigma + 1 ) \rceil \f$, \f$ w = \lfloor 64
/// / b \rfloor \f$, a segment a range of \f$ \mathit{SA} \f$ of at least two
/// suffixes still tied:
/// \f[
/// \class{pseudocode}{\begin{array}{r@{\quad}l}
/// 1 & \textbf{function}\ \mathrm{suffixArray}( \mathit{ex} , T ) \cr
/// 2 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do in }
///     \textbf{parallel}\ P[ i ] \gets ( c( T[ i ] ) \cdots c( T[ i + w - 1 ]
///     ) , i )\quad \triangleright\ w \text{ codes of } b \text{ bits} \cr
/// 3 & \quad \mathrm{integerSort}( \mathit{ex} , P , w b );\ \mathit{SA}[ j ]
///     \gets P[ j ].i \cr
/// 4 & \quad S \gets \mathrm{refine}( [ 0 , n ) , j \mapsto P[ j ].
///     \mathit{key} );\ h \gets w \cr
/// 5 & \quad \textbf{while}\ S \ne \emptyset\ \textbf{do} \cr
/// 6 & \qquad \textbf{for each}\ [ l , r ) \in S\ \textbf{do in parallel}
///     \quad \triangleright\ \text{the large ones on all the threads} \cr
/// 7 & \qquad\quad \textbf{for}\ j \gets l\ \textbf{to}\ r - 1\ \textbf{do}
///     \ k \gets \mathit{SA}[ j ];\ K[ j ] \gets ( R[ k + h ] + 1 , k )
///     \quad \triangleright\ 0 \text{ if } k + h \ge n \cr
/// 8 & \qquad\quad \text{sort } K[ l .. r ) \text{ by the first item};
///     \ \mathit{SA}[ l .. r ) \gets \text{the second items} \cr
/// 9 & \qquad S \gets \bigcup_{[ l , r ) \in S} \mathrm{refine}( [ l , r ) ,
///     j \mapsto K[ j ].\mathit{key} )\quad \triangleright\ \text{by }
///     \mathrm{flatten} \cr
/// 10 & \qquad h \gets 2 h \cr
/// 11 & \quad \textbf{return}\ \mathit{SA} \cr
/// 12 & \textbf{function}\ \mathrm{refine}( [ l , r ) , \kappa ) \cr
/// 13 & \quad \textbf{for}\ j \gets l\ \textbf{to}\ r - 1\ \textbf{do}
///     \ R[ \mathit{SA}[ j ] ] \gets \max\{ j' \le j : j' = l\ \textbf{or}
///     \ \kappa( j' ) \ne \kappa( j' - 1 ) \}\quad \triangleright\ \text{a}
///     \text{ scan by } \max \cr
/// 14 & \quad \textbf{return}\ \text{the runs of equal } \kappa \text{ of }
///     [ l , r ) \text{ with at least two places}
/// \end{array}}
/// \f]
/// \pre \f$ n < 2^{32} - 1 \f$ if `I` has 32 bits
/// \post a suffix array of \f$ T \f$ (an audit)
template< class I = std::uint32_t , Executor E >
 requires( std::same_as< I , std::uint32_t > ||
	   std::same_as< I , std::uint64_t > )
[[nodiscard]] std::vector< I > suffixArray( E & ex , std::string_view T );

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
 parallelFor(
   ex , 0 , n ,
   [ & ]( std::size_t r ) { P[ SA[ r ] ] = r == 0 ? n : SA[ r - 1 ]; } ,
   4096 );
 const std::size_t B = std::min( n , 4 * ex.concurrency() );
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    const std::size_t i0 = n * b / B , i1 = n * ( b + 1 ) / B;
    std::size_t h = 0;
    for( std::size_t i = i0 ; i < i1 ; ++i ) {
     const std::size_t j = P[ i ];
     if( j == n ) {  // the least suffix
      h = 0;
      P[ i ] = 0;
      continue;
      }
     while( ( i + h < n ) && ( j + h < n ) && ( T[ i + h ] == T[ j + h ] ) )
      ++h;
     P[ i ] = h;
     if( h > 0 )
      --h;
     }
    } ,
   1 );
 std::vector< std::size_t > LCP( n - 1 );
 parallelFor(
   ex , 0 , n - 1 , [ & ]( std::size_t r ) { LCP[ r ] = P[ SA[ r + 1 ] ]; } ,
   4096 );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class I >
std::vector< I > lcpArrayPhi( E & ex , std::string_view T ,
			      const std::vector< I > & SA )
{
 const std::size_t n = SA.size();
 ADS_PRE( T.size() == n );
 if( ( ex.concurrency() == 1 ) || ( n < ( std::size_t( 1 ) << 15 ) ) )
  return( lcpArrayPhi( T , SA ) );
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 const auto P = std::make_unique_for_overwrite< I[] >( n );
 parallelFor(
   ex , 0 , n ,
   [ & ]( std::size_t r ) { P[ SA[ r ] ] = r == 0 ? I( n ) : SA[ r - 1 ]; } ,
   4096 );
 // the lcp of the suffixes i and Phi[ i ] from h on: 8 symbols at a time
 // after the first 4, which are compared one by one (most lcp are short)
 auto lcp = [ & ]( std::size_t i , std::size_t j , std::size_t h ) {
  const std::size_t far = n - std::max( i , j ) ,
  near = std::min( far , h + 4 );
  while( ( h < near ) && ( t[ i + h ] == t[ j + h ] ) )
   ++h;
  if( h == near )
   for( ; ; ) {
    if( h + 8 > far ) {
     while( ( h < far ) && ( t[ i + h ] == t[ j + h ] ) )
      ++h;
     break;
     }
    std::uint64_t a , b;
    std::memcpy( &a , t + i + h , 8 );
    std::memcpy( &b , t + j + h , 8 );
    if( a != b ) {
     if constexpr( std::endian::native == std::endian::little )
      h += std::size_t( std::countr_zero( a ^ b ) ) / 8;
     else
      h += std::size_t( std::countl_zero( a ^ b ) ) / 8;
     break;
     }
    h += 8;
    }
  return( h );
  };
 const std::size_t B = std::min( n , 4 * ex.concurrency() );
 parallelFor(
   ex , 0 , B ,
   [ & ]( std::size_t b ) {
    const std::size_t i0 = n * b / B , i1 = n * ( b + 1 ) / B;
    constexpr std::size_t D = 16;  // the text at Phi, ahead
    std::size_t h = 0;
    for( std::size_t i = i0 ; i < i1 ; ++i ) {
     if( i + D < i1 )
      ADS::detail::prefetchLine(
	t + std::min< std::size_t >( std::size_t( P[ i + D ] ) + h , n - 1 ) );
     const std::size_t j = std::size_t( P[ i ] );
     h = j == n ? 0 : lcp( i , j , h );
     P[ i ] = I( h );
     h -= ( h != 0 );
     }
    } ,
   1 );
 std::vector< I > LCP( n - 1 );
 parallelFor(
   ex , 0 , n - 1 , [ & ]( std::size_t r ) { LCP[ r ] = P[ SA[ r + 1 ] ]; } ,
   4096 );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

namespace detail
{

/// the prefix doubling of `suffixArray( ex , T )` on any number of threads
/// and at any size
template< class I , class E >
std::vector< I > suffixDoubling( E & ex , std::string_view T )
{
 const std::size_t n = T.size();
 if constexpr( sizeof( I ) < 8 )
  ADS_PRE( n < std::size_t( std::numeric_limits< I >::max() ) );
 std::vector< I > SA( n );
 if( n < 2 ) {
  if( n == 1 )
   SA[ 0 ] = 0;
  return( SA );
  }
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 // the codes of the bytes, 0 past the end, and the first w of each suffix
 // in a word of w b bits
 const auto H = histogram(
   ex , n , [ & ]( std::size_t i ) { return( std::size_t( t[ i ] ) ); } ,
   256 );
 std::array< std::uint64_t , 256 > code{};
 std::uint64_t sigma = 0;
 for( std::size_t c = 0 ; c < 256 ; ++c )
  if( H[ c ] != 0 )
   code[ c ] = ++sigma;
 const int b = int( std::bit_width( sigma ) );
 const std::size_t w = std::size_t( 64 / b );
 const int bits = int( w ) * b;
 const std::uint64_t mask =
   bits == 64 ? ~std::uint64_t( 0 ) : ( std::uint64_t( 1 ) << bits ) - 1;
 auto at = [ & ]( std::size_t i ) -> std::uint64_t {
  return( i < n ? code[ t[ i ] ] : 0 );
  };
 struct Item
 {
  std::uint64_t key;  // the first w codes
  I i;                // the suffix
  };
 std::vector< Item > P( n );
 parallelForBlocks(
   ex , 0 , n ,
   [ & ]( std::size_t lo , std::size_t hi ) {
    std::uint64_t k = 0;
    for( std::size_t j = 0 ; j < w ; ++j )
     k = ( k << b ) | at( lo + j );
    for( std::size_t i = lo ; i < hi ; ++i ) {
     P[ i ] = Item{ k & mask , I( i ) };
     k = ( k << b ) | at( i + w );
     }
    } ,
   4096 );
 integerSort( ex , P , []( const Item & x ) { return( x.key ); } , bits );
 parallelFor( ex , 0 , n , [ & ]( std::size_t j ) { SA[ j ] = P[ j ].i; } );
 // R[ k ], the rank of the suffix k: the first place of its segment
 std::vector< I > R( n );
 struct Seg
 {
  I l , r;  // the places [ l , r ) of the segment in SA
  };
 // the runs of equal kappa in [ l , r ): the ranks set, the runs of two or
 // more returned, on one thread
 auto refineSeq = [ & ]( std::size_t l , std::size_t r , auto && kappa ,
			 std::vector< Seg > & out ) {
  std::size_t s = l;
  for( std::size_t j = l ; j < r ; ++j ) {
   if( ( j > l ) && ( kappa( j ) != kappa( j - 1 ) ) ) {
    if( j - s >= 2 )
     out.push_back( Seg{ I( s ) , I( j ) } );
    s = j;
    }
   R[ SA[ j ] ] = I( s );
   }
  if( r - s >= 2 )
   out.push_back( Seg{ I( s ) , I( r ) } );
  };
 // the same on all the threads: the first place of the run of each place by
 // a scan by max, then the starts of the runs by a pack
 auto refinePar = [ & ]( std::size_t l , std::size_t r , auto && kappa ) {
  const std::size_t m = r - l;
  std::vector< I > st( m );
  parallelFor( ex , 0 , m , [ & ]( std::size_t q ) {
   const std::size_t j = l + q;
   st[ q ] = ( q == 0 ) || ( kappa( j ) != kappa( j - 1 ) ) ? I( j ) : I( 0 );
   } );
  parallelInclusiveScan( ex , st.begin() , m , st.begin() , I( 0 ) ,
			 []( I a , I c ) { return( std::max( a , c ) ); } );
  parallelFor( ex , 0 , m ,
	       [ & ]( std::size_t q ) { R[ SA[ l + q ] ] = st[ q ]; } );
  const auto S = packIndex( ex , m , [ & ]( std::size_t q ) {
   return( std::size_t( st[ q ] ) == l + q );
   } );
  auto all = tabulate( ex , S.size() , [ & ]( std::size_t k ) {
   return(
     Seg{ I( l + S[ k ] ) , I( k + 1 < S.size() ? l + S[ k + 1 ] : r ) } );
   } );
  return(
    filter( ex , all , []( const Seg & g ) { return( g.r - g.l >= 2 ); } ) );
  };
 std::vector< Seg > segs;
 {
  auto kappa = [ & ]( std::size_t j ) { return( P[ j ].key ); };
  if( n < kPrimitiveSmall )
   refineSeq( 0 , n , kappa , segs );
  else
   segs = refinePar( 0 , n , kappa );
  }
 std::vector< Item >().swap( P );
 const std::size_t p = std::max< std::size_t >( ex.concurrency() , 1 );
 const std::size_t big = std::max( kPrimitiveSmall , n / ( 4 * p ) );
 const int kbits = int( std::bit_width( n ) );
 struct Key
 {
  I key;  // the rank h places on, plus 1, or 0
  I i;    // the suffix
  };
 std::vector< Key > K( n );
 auto key = [ & ]( const Key & x ) { return( std::uint64_t( x.key ) ); };
 for( std::size_t h = w ; ! segs.empty() ; h *= 2 ) {
  auto keyOf = [ & ]( std::size_t j ) {
   const std::size_t k = std::size_t( SA[ j ] );
   return( Key{ k + h < n ? I( R[ k + h ] + 1 ) : I( 0 ) , I( k ) } );
   };
  auto kappa = [ & ]( std::size_t j ) { return( K[ j ].key ); };
  const std::size_t S = segs.size();
  const auto cut = blocksOf( 0 , S , 1 , std::min< std::size_t >( S , 1024 ) );
  // the sorts of the segments: the small ones each on one thread, then the
  // large ones each on all the threads
  parallelForBlocks(
    ex , 0 , S ,
    [ & ]( std::size_t lo , std::size_t hi ) {
     for( std::size_t q = lo ; q < hi ; ++q ) {
      const std::size_t l = segs[ q ].l , r = segs[ q ].r;
      if( r - l >= big )
       continue;
      for( std::size_t j = l ; j < r ; ++j )
       K[ j ] = keyOf( j );
      if( r - l <= 16 )
       for( std::size_t j = l + 1 ; j < r ; ++j ) {
	const Key x = K[ j ];
	std::size_t i = j;
	for( ; ( i > l ) && ( K[ i - 1 ].key > x.key ) ; --i )
	 K[ i ] = K[ i - 1 ];
	K[ i ] = x;
        }
      else
       std::sort(
	 K.begin() + std::ptrdiff_t( l ) , K.begin() + std::ptrdiff_t( r ) ,
	 []( const Key & a , const Key & c ) { return( a.key < c.key ); } );
      for( std::size_t j = l ; j < r ; ++j )
       SA[ j ] = K[ j ].i;
      }
     } ,
    1 , cut.first );
  for( const Seg & g : segs )
   if( std::size_t( g.r - g.l ) >= big ) {
    const std::size_t l = g.l , r = g.r;
    parallelFor( ex , l , r ,
		 [ & ]( std::size_t j ) { K[ j ] = keyOf( j ); } );
    integerSort( ex , std::span< Key >( K.data() + l , r - l ) , key , kbits );
    parallelFor( ex , l , r , [ & ]( std::size_t j ) { SA[ j ] = K[ j ].i; } );
    }
  // the ranks and the segments still tied, once all the sorts have read R
  std::vector< std::vector< Seg > > L( cut.first );
  parallelForBlocks(
    ex , 0 , S ,
    [ & ]( std::size_t lo , std::size_t hi ) {
     auto & out = L[ lo / cut.second ];
     for( std::size_t q = lo ; q < hi ; ++q )
      if( std::size_t( segs[ q ].r - segs[ q ].l ) < big )
       refineSeq( segs[ q ].l , segs[ q ].r , kappa , out );
     } ,
    1 , cut.first );
  for( const Seg & g : segs )
   if( std::size_t( g.r - g.l ) >= big )
    L.push_back( refinePar( g.l , g.r , kappa ) );
  segs = flatten( ex , L );
  }
 ADS_AUDIT( detail::isSuffixArrayOf( t , n , SA.data() ) );
 return( SA );
 }

}  // end( namespace detail )

template< class I , Executor E >
 requires( std::same_as< I , std::uint32_t > ||
	   std::same_as< I , std::uint64_t > )
std::vector< I > suffixArray( E & ex , std::string_view T )
{
 const std::size_t n = T.size();
 if( ( ( ex.concurrency() == 1 ) || ( n < ( std::size_t( 1 ) << 15 ) ) ) &&
     ( n <
       std::size_t( std::numeric_limits< std::make_signed_t< I > >::max() ) ) )
  return( suffixArray< I >( T ) );
 return( detail::suffixDoubling< I >( ex , T ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif /* ParallelText.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File ParallelText.h ------------------------*/
/*--------------------------------------------------------------------------*/
