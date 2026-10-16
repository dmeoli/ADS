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
inline bool subsetSumDP( const std::vector< long > & A , long k ,
                         std::vector< std::vector< bool > > * rows = nullptr )
{
 std::vector< bool > M( k + 1 , false );
 M[ 0 ] = true;
 if( rows ) rows->push_back( M );
 for( long a : A ) {
  for( long j = k ; j >= a ; --j )      // downwards, each a used once
   M[ j ] = M[ j - a ] || M[ j ];
  if( rows ) rows->push_back( M );
  }
 return( M[ k ] );
 }

/// the places of a subset of A of sum k, by the table of all the rows,
/// O( n k ) time and space, or nothing
inline std::optional< Nodes > subsetSumSet( const std::vector< long > & A ,
                                            long k )
{
 if( k < 0 ) return( std::nullopt );
 const std::size_t n = A.size();
 std::vector< std::vector< bool > > M( n + 1 ,
                                       std::vector< bool >( k + 1 , false ) );
 M[ 0 ][ 0 ] = true;
 for( std::size_t i = 0 ; i < n ; ++i )
  for( long j = 0 ; j <= k ; ++j )
   M[ i + 1 ][ j ] = M[ i ][ j ] ||
                     ( ( j >= A[ i ] ) && M[ i ][ j - A[ i ] ] );
 if( ! M[ n ][ k ] ) return( std::nullopt );
 Nodes S;
 for( std::size_t i = n ; i > 0 ; --i )
  if( ! M[ i - 1 ][ k ] ) {             // A[ i - 1 ] is needed
   S.push_back( int( i - 1 ) );
   k -= A[ i - 1 ];
   }
 std::reverse( S.begin() , S.end() );
 return( S );
 }

/// PARTIZIONE by dynamic programming: the places of a half of A of half
/// the sum, or nothing (exercise 19.1)
inline std::optional< Nodes > partitionDP( const std::vector< long > & A )
{
 const long s = std::accumulate( A.begin() , A.end() , 0L );
 if( s % 2 ) return( std::nullopt );
 return( subsetSumSet( A , s / 2 ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- SCHEDULING ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// SCHEDULING MULTIPROCESSORE by dynamic programming on the vectors of
/// the loads of the m processors, each at most d: the processor of each
/// program, or nothing; O( n ( d + 1 )^m ) time (exercise 19.2)
inline std::optional< std::vector< int > > schedulingDP(
                        const std::vector< long > & t , int m , long d )
{
 using Loads = std::vector< long >;
 // the loads reached after i programs, each with the loads before the
 // last one and its processor
 std::vector< std::map< Loads , std::pair< Loads , int > > > L( t.size() + 1 );
 L[ 0 ][ Loads( m , 0 ) ] = { Loads() , -1 };
 for( std::size_t i = 0 ; i < t.size() ; ++i )
  for( const auto & [ x , from ] : L[ i ] )
   for( int q = 0 ; q < m ; ++q )
    if( x[ q ] + t[ i ] <= d ) {
     Loads y = x;
     y[ q ] += t[ i ];
     L[ i + 1 ].try_emplace( y , x , q );
     }
 if( L.back().empty() ) return( std::nullopt );
 std::vector< int > a( t.size() );
 Loads x = L.back().begin()->first;
 for( std::size_t i = t.size() ; i > 0 ; --i ) {
  const auto & [ y , q ] = L[ i ].at( x );
  a[ i - 1 ] = q;
  x = y;
  }
 return( a );
 }

/// an interval [ start , end ) of the program on a processor
struct Piece {
 int program;
 double start , end;
 };

/// the rule of McNaughton: the length d of the schedule and the pieces
/// of each processor; the programs fill a processor up to d and go on at
/// time 0 of the next one, and the two pieces of a program do not overlap
/// because t[ i ] <= d; O( n + m ) time (exercise 19.3)
inline std::pair< double , std::vector< std::vector< Piece > > > mcNaughton(
                        const std::vector< double > & t , int m )
{
 double d = std::accumulate( t.begin() , t.end() , 0.0 ) / m;
 for( double x : t ) d = std::max( d , x );
 std::vector< std::vector< Piece > > P( m );
 const double eps = 1e-12 * std::max( d , 1.0 );
 int q = 0;
 double now = 0;
 for( std::size_t i = 0 ; i < t.size() ; ++i )
  for( double r = t[ i ] ; ( r > eps ) && ( q < m ) ; ) {
   const double s = std::min( r , d - now );
   P[ q ].push_back( { int( i ) , now , now + s } );
   now += s;
   r -= s;
   if( d - now <= eps ) {               // the processor is full
    ++q;
    now = 0;
    }
   }
 return( std::pair( d , P ) );
 }

/// REAL-TIME SCHEDULING without priority rule: the tasks of time C[ i ]
/// every T[ i ] run on m processors if and only if C[ i ] <= T[ i ] and
/// \f$ \sum_i C_i / T_i \leq m \f$; the schedule of a period of length 1,
/// task i for the fraction C[ i ] / T[ i ] of it, or nothing (exercise
/// 19.4)
inline std::optional< std::vector< std::vector< Piece > > > realTimeScheduling(
                        const std::vector< double > & C ,
                        const std::vector< double > & T , int m )
{
 std::vector< double > u( C.size() );
 for( std::size_t i = 0 ; i < C.size() ; ++i ) u[ i ] = C[ i ] / T[ i ];
 auto [ d , P ] = mcNaughton( u , m );
 if( d > 1 + 1e-12 ) return( std::nullopt );
 return( P );
 }

/// bottino(): the thief of each jewel of price P[ i ], so that the three
/// thieves take m / 3 each, or nothing; S[ i ][ j ][ h ] tells whether the
/// first i jewels give j to the first and h to the second thief, and the
/// prices are positive, so that j and h range in [ 0 , m / 3 ];
/// O( n m^2 ) time (exercise 19.5)
inline std::optional< std::vector< int > > bottino(
                        const std::vector< long > & P )
{
 const long m = std::accumulate( P.begin() , P.end() , 0L );
 if( m % 3 ) return( std::nullopt );
 const long t = m / 3;
 const std::size_t n = P.size();
 using Table = std::vector< std::vector< bool > >;
 const Table empty( t + 1 , std::vector< bool >( t + 1 , false ) );
 std::vector< Table > S( n + 1 , empty );
 S[ 0 ][ 0 ][ 0 ] = true;
 for( std::size_t i = 0 ; i < n ; ++i )
  for( long j = 0 ; j <= t ; ++j )
   for( long h = 0 ; h <= t ; ++h )
    S[ i + 1 ][ j ][ h ] = S[ i ][ j ][ h ] ||
                           ( ( j >= P[ i ] ) && S[ i ][ j - P[ i ] ][ h ] ) ||
                           ( ( h >= P[ i ] ) && S[ i ][ j ][ h - P[ i ] ] );
 if( ! S[ n ][ t ][ t ] ) return( std::nullopt );
 std::vector< int > a( n );
 for( long i = long( n ) , j = t , h = t ; i > 0 ; --i ) {   // backwards
  const long p = P[ i - 1 ];
  if( S[ i - 1 ][ j ][ h ] ) a[ i - 1 ] = 2;
  else if( ( j >= p ) && S[ i - 1 ][ j - p ][ h ] ) {
   a[ i - 1 ] = 0;
   j -= p;
   }
  else {
   a[ i - 1 ] = 1;
   h -= p;
   }
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* Pseudopolynomial.h included */

/*--------------------------------------------------------------------------*/
/*---------------------- End File Pseudopolynomial.h -----------------------*/
/*--------------------------------------------------------------------------*/
