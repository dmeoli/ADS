/*--------------------------------------------------------------------------*/
/*------------------------- File exam_complexity.cpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the exercises on complexity of the written exams, 2011-2026: each
 * recurrence is evaluated numerically, exactly on the integers with the
 * floors of its statement, and its claimed order is checked by the ratios
 * T( n ) / g( n ) over growing n, by the witnesses c and m of the proofs by
 * substitution, and by the solutions of MasterTheorem.h on the exact powers
 * of b; each fragment of code is run with a counter of its steps, which is
 * compared with the recurrence written for it and with its order.
 *
 * A step is one iteration of a loop of the fragment or one call on a base
 * case, so that the count of a fragment is its recurrence evaluated exactly
 * (with the floors of the code where the solution writes n / 2).
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 2.9,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf
 * \see M. Akra, L. Bazzi, On the solution of linear recurrence equations,
 *      Computational Optimization and Applications 10(2), 195-210, 1998,
 *      doi:10.1023/A:1018373005182
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/algorithm/AkraBazzi.h"
#include "ADS/algorithm/MasterTheorem.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <numbers>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

namespace complexity {

using Int = std::int64_t;
using Ints = std::vector< Int >;
using Matrix = std::vector< Ints >;

/*--------------------------------------------------------------------------*/
/*------------------------- RECURRENCES AND ORDERS -------------------------*/
/*--------------------------------------------------------------------------*/

/// a function of the integers n >= 0 computed by memoized recursion: rhs(
/// T , n ) gives the value at n, calling T( m ) for the values it needs
class Memo {
 public:
  using Rhs = std::function< double( Memo & , Int ) >;

  explicit Memo( Rhs rhs ) : f_rhs( std::move( rhs ) ) {}

  /// the value at n
  /// \pre n >= 0
  double operator()( Int n ) {
   ADS_PRE( n >= 0 );
   const auto it = f_memo.find( n );
   if( it != f_memo.end() )
    return( it->second );
   const double v = f_rhs( *this , n );
   f_memo.emplace( n , v );
   return( v );
   }

 private:
  Rhs f_rhs;
  std::unordered_map< Int , double > f_memo;
 };

/// floor( n / x ) for a real x > 0 (an irrational divisor of a statement)
/// \pre x > 0
Int down( Int n , double x )
{
 ADS_PRE( x > 0 );
 return( Int( std::floor( double( n ) / x ) ) );
 }

/// floor( sqrt( n ) )
/// \pre n >= 0
Int isqrt( Int n )
{
 ADS_PRE( n >= 0 );
 Int r = Int( std::sqrt( double( n ) ) );  // the products by division:
 while( ( r > 0 ) && ( r > n / r ) ) --r;   // no overflow up to INT64_MAX
 while( r + 1 <= n / ( r + 1 ) ) ++r;
 return( r );
 }

/// log_2 n
double lg( double n ) { return( std::log2( n ) ); }

/// the n where the orders are checked in [ m , N ]: all of them up to 4096,
/// a geometric progression of ratio 1.01 beyond, N, and 300 random ones
/// drawn with a fixed seed, in increasing order
/// \pre 0 <= m <= N
Ints sample( Int m , Int N )
{
 ADS_PRE( ( 0 <= m ) && ( m <= N ) );
 Ints s;
 for( Int n = m ; n <= std::min< Int >( N , 4096 ) ; ++n )
  s.push_back( n );
 for( double x = 4096 ; x < double( N ) ; x *= 1.01 )
  if( Int( x ) >= m )
   s.push_back( Int( x ) );
 s.push_back( N );
 std::mt19937_64 g( 1 );
 std::uniform_int_distribution< Int > u( m , N );
 for( int i = 0 ; i < 300 ; ++i )
  s.push_back( u( g ) );
 std::sort( s.begin() , s.end() );
 s.erase( std::unique( s.begin() , s.end() ) , s.end() );
 return( s );
 }

/// whether p( n ) holds for every n of sample( m , N )
template< class P >
bool holds( Int m , Int N , P && p )
{
 for( const Int n : sample( m , N ) )
  if( ! p( n ) )
   return( false );
 return( true );
 }

/// whether T( n ) <= c g( n ) for every n of sample( m , N )
template< class F , class G >
bool upper( F & T , G && g , double c , Int m , Int N = 10000000 )
{
 return( holds( m , N , [ & ]( Int n ) {
  return( T( n ) <= c * g( double( n ) ) * ( 1 + 1e-9 ) ); } ) );
 }

/// whether T( n ) >= c g( n ) for every n of sample( m , N )
template< class F , class G >
bool lower( F & T , G && g , double c , Int m , Int N = 10000000 )
{
 return( holds( m , N , [ & ]( Int n ) {
  return( T( n ) >= c * g( double( n ) ) * ( 1 - 1e-9 ) ); } ) );
 }

/// the largest T( i ) / g( i ) for i in [ lo , hi ], i.e., the least c of
/// the base cases of a proof by substitution
template< class F , class G >
double worst( F & T , G && g , Int lo , Int hi )
{
 double c = 0;
 for( Int i = lo ; i <= hi ; ++i )
  c = std::max( c , T( i ) / g( double( i ) ) );
 return( c );
 }

/// the least and the largest T( n ) / g( n ) on sample( m , N )
template< class F , class G >
std::pair< double , double > ratios( F & T , G && g , Int m , Int N )
{
 double lo = HUGE_VAL , hi = 0;
 for( const Int n : sample( m , N ) ) {
  const double r = T( n ) / g( double( n ) );
  lo = std::min( lo , r );
  hi = std::max( hi , r );
  }
 return( std::pair( lo , hi ) );
 }

/// whether T( n ) / g( n ) stays within a factor 8 on [ 100 , N ]: the
/// floors of a statement do not change the order found on the powers
template< class F , class G >
bool bounded( F & T , G && g , Int N = 1000000000000 )
{
 const auto [ lo , hi ] = ratios( T , g , 100 , N );
 return( ( lo > 0 ) && ( hi <= 8 * lo ) );
 }

/// whether T( b^k ) = a T( b^{k-1} ) + f( b^k ), T( 1 ) = 1, divided by
/// G( b^k ), changes by less than 5% over the last 5 steps of k <= 60 (or
/// of the k where T stays below 10^250)
template< class F >
bool settles( double a , double b , F && f , const Growth & G )
{
 std::vector< double > ratio;
 double n = 1 , T = 1;
 for( int k = 1 ; k <= 60 ; ++k ) {
  n *= b;
  T = a * T + f( n );
  if( std::log10( T ) > 250 )
   break;
  if( k >= 4 )
   ratio.push_back( T / G.at( n ) );
  }
 return( ( ratio.size() >= 8 ) &&
         ( std::abs( ratio.back() / ratio[ ratio.size() - 6 ] - 1 ) <
           0.05 ) );
 }

/// whether g is Theta( c^n n^p log^q n ( log log n )^r ), O() if upper
bool same( const Growth & g , double c , double p , double q = 0 ,
           double r = 0 , bool upper = false )
{
 return( ( std::abs( g.c - c ) < 1e-9 ) && ( std::abs( g.p - p ) < 1e-9 ) &&
         ( std::abs( g.q - q ) < 1e-9 ) && ( std::abs( g.r - r ) < 1e-9 ) &&
         ( g.upper == upper ) );
 }

/// T( n ) = a T( n / b ) + f( n ) by the master theorem in the form with
/// the three cases f = O( n^{alpha - eps} ), f = Theta( n^alpha ) and f =
/// Omega( n^{alpha + eps} ) with a f( n / b ) <= c f( n ), c < 1, for f(
/// n ) = n^beta log^k n: the order, or nothing when f falls between the
/// cases (beta = alpha and k != 0), where only Teorema 2.4 [see
/// MasterTheorem.h] applies
/// \pre a >= 1 and b > 1
std::optional< Growth > extendedMaster( double a , double b , double beta ,
                                        double k = 0 )
{
 ADS_PRE( ( a >= 1 ) && ( b > 1 ) );
 const double alpha = std::log( a ) / std::log( b );
 if( ( std::abs( beta - alpha ) < 1e-12 ) && ( k != 0 ) )
  return( std::nullopt );
 return( master( a , b , beta , k ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE FRAGMENTS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// 11 (2013-05-03): crazy( n ), a loop of floor( n / 2 ) iterations and the
/// calls on floor( n / 2 ) and floor( n / 3 ); its steps are T( n ) = T(
/// n / 2 ) + T( n / 3 ) + n / 2, T( n ) = 1 for n <= 1, Theta( n )
/// \pre n >= 0
Int crazyHalfThird( Int n , Int & steps )
{
 ADS_PRE( n >= 0 );
 if( n <= 1 ) {
  ++steps;
  return( n );
  }
 Int b = 1;
 for( Int i = 1 ; i <= n / 2 ; ++i ) {
  b = ( b * 2 ) % 1007;
  ++steps;
  }
 return( b + crazyHalfThird( n / 2 , steps ) +
         crazyHalfThird( n / 3 , steps ) );
 }

/// 12 (2013-06-17): fun( V , i , j ), the sum of V[ i .. j ] and the calls
/// on V[ i .. j - 1 ] and on the first floor( sqrt( n ) ) values; its steps
/// are T( n ) = T( n - 1 ) + T( floor( sqrt( n ) ) ) + n, T( 1 ) = 1,
/// Theta( n^2 ). The text calls fun( V , i , i + floor( sqrt( j - i + 1 )
/// ) ), on floor( sqrt( n ) ) + 1 values, which for n = 2, 3 is the call
/// itself and never ends; the second call here is on floor( sqrt( n ) )
/// values, as in the recurrence of the solution
/// \pre 0 <= i <= j < |V|
Int fun( const Ints & V , Int i , Int j , Int & steps )
{
 ADS_PRE( ( 0 <= i ) && ( i <= j ) && ( j < Int( V.size() ) ) );
 if( i == j ) {
  ++steps;
  return( 1 );
  }
 Int t = 0;
 for( Int k = i ; k <= j ; ++k ) {
  t += V[ k ];
  ++steps;
  }
 return( t + fun( V , i , j - 1 , steps ) +
         fun( V , i , i + isqrt( j - i + 1 ) - 1 , steps ) );
 }

/// 16 (2014-04-24): mistery( A , i , j ), 2^k random reads with k in [ 1 ,
/// n ] drawn by rnd( n ) + 1, rnd( n ) in [ 0 , n ), and the calls on the
/// two halves; at most W( n ) = W( ceil( n / 2 ) ) + W( floor( n / 2 ) ) +
/// 2^n steps, W( 1 ) = 1, which is O( 2^n ), reached when rnd( n ) = n - 1
/// \pre 0 <= i, j < |A|, n <= 40
template< class R >
Int mistery( const Ints & A , Int i , Int j , R & rnd , Int & steps )
{
 if( j < i )
  return( 0 );
 ADS_PRE( ( 0 <= i ) && ( j < Int( A.size() ) ) && ( j - i < 40 ) );
 if( i == j ) {
  ++steps;
  return( 2 * A[ i ] );
  }
 const Int n = j - i + 1;
 Int sum = 0;
 const Int k = rnd( n ) + 1;
 for( Int r = 1 ; r <= ( Int( 1 ) << k ) ; ++r ) {
  sum += A[ i + rnd( n ) ];
  ++steps;
  }
 const Int m = ( i + j ) / 2;
 return( sum + mistery( A , i , m , rnd , steps ) +
         mistery( A , m + 1 , j , rnd , steps ) );
 }

/// 31 (2017-01-24): crazy( n ), a double loop of n ( n + 1 ) / 2 iterations
/// and the calls on floor( n / 4 ) twice and on floor( n / 2 ); its steps
/// are T( n ) = 2 T( n / 4 ) + T( n / 2 ) + n ( n + 1 ) / 2, T( n ) = 1
/// for n <= 1, Theta( n^2 )
/// \pre n >= 0
Int crazyQuarters( Int n , Int & steps )
{
 ADS_PRE( n >= 0 );
 if( n <= 1 ) {
  ++steps;
  return( n * n );
  }
 Int b = 1;
 for( Int i = 1 ; i <= n ; ++i )
  for( Int j = i ; j <= n ; ++j ) {
   b = ( b + i * j ) % 1007;
   ++steps;
   }
 return( b + crazyQuarters( n / 4 , steps ) + crazyQuarters( n / 2 , steps )
         + crazyQuarters( n / 4 , steps ) );
 }

/// 59 (2023-01-17): R( A , i , j ), the sum of fewer than 4 values or the
/// calls on the first three quarters and on the last three quarters; its
/// calls are T( n ) = T( 3 d ) + T( n - d ) + 1, d = floor( n / 4 ), T( n )
/// = 1 for n < 4, Theta( n^{log_{4/3} 2} )
/// \pre 0 <= i <= j < |A|
Int threeQuarters( const Ints & A , Int i , Int j , Int & calls )
{
 ADS_PRE( ( 0 <= i ) && ( i <= j ) && ( j < Int( A.size() ) ) );
 ++calls;
 const Int n = j - i + 1;
 if( n < 4 ) {
  Int s = 0;
  for( Int k = i ; k <= j ; ++k )
   s += A[ k ];
  return( s );
  }
 const Int d = n / 4;
 const Int r1 = threeQuarters( A , i , i + 3 * d - 1 , calls );
 const Int r2 = threeQuarters( A , i + d , j , calls );
 return( r1 - r2 );
 }

/// 60 (2023-02-09): R( A , start , end ), a scan from both ends while the
/// values are equal and the calls on the two halves; its steps (calls and
/// iterations) are T( n ) = 2 T( n / 2 ) + 1, Theta( n ), when the values
/// are distinct and T( n ) = 2 T( n / 2 ) + ceil( n / 2 ) + 1, Theta( n
/// log n ), when they are all equal
/// \pre 0 <= s <= e < |A|
Int equalEnds( const Ints & A , Int s , Int e , Int & steps )
{
 ADS_PRE( ( 0 <= s ) && ( s <= e ) && ( e < Int( A.size() ) ) );
 ++steps;
 if( s == e )
  return( A[ s ] );
 Int tot = 0 , i = s , j = e;
 while( ( i <= j ) && ( A[ i ] == A[ j ] ) ) {
  tot += A[ i ] + A[ j ];
  ++i;
  --j;
  ++steps;
  }
 const Int mid = ( s + e ) / 2;
 tot += equalEnds( A , s , mid , steps );
 tot += equalEnds( A , mid + 1 , e , steps );
 return( tot );
 }

/// 61 (2023-06-20): R( A , x1 , y1 , x2 , y2 ), the sum of the border of a
/// square of side n and the calls on the two diagonal quadrants; its steps
/// (calls and iterations) are T( n ) = 2 T( n / 2 ) + 2 n - 1, T( 1 ) = 1,
/// Theta( n log n )
/// \pre the square is in A
Int border( const Matrix & A , Int x1 , Int y1 , Int x2 , Int y2 ,
            Int & steps )
{
 ADS_PRE( ( 0 <= x1 ) && ( x1 <= x2 ) && ( x2 < Int( A.size() ) ) &&
          ( 0 <= y1 ) && ( y1 <= y2 ) && ( y2 < Int( A.size() ) ) );
 ++steps;
 if( ( x1 == x2 ) && ( y1 == y2 ) )
  return( A[ x1 ][ y1 ] );
 Int tot = 0;
 for( Int x = x1 + 1 ; x <= x2 - 1 ; ++x ) {
  tot += A[ x ][ y1 ] + A[ x ][ y2 ];
  ++steps;
  }
 for( Int y = y1 ; y <= y2 ; ++y ) {
  tot += A[ x1 ][ y ] + A[ x2 ][ y ];
  ++steps;
  }
 const Int xm = ( x1 + x2 ) / 2 , ym = ( y1 + y2 ) / 2;
 tot += border( A , x1 , y1 , xm , ym , steps );
 tot += border( A , xm + 1 , ym + 1 , x2 , y2 , steps );
 return( tot );
 }

/// 63 (2023-08-23): R( A , x1 , y1 , x2 , y2 ), the sum of a square of side
/// n and the calls on the two diagonal quadrants; its iterations are T( n )
/// = 2 T( n / 2 ) + n^2, T( 1 ) = 1, that is, 2 n^2 - n, Theta( n^2 )
/// \pre the square is in A
Int square( const Matrix & A , Int x1 , Int y1 , Int x2 , Int y2 ,
            Int & steps )
{
 ADS_PRE( ( 0 <= x1 ) && ( x1 <= x2 ) && ( x2 < Int( A.size() ) ) &&
          ( 0 <= y1 ) && ( y1 <= y2 ) && ( y2 < Int( A.size() ) ) );
 Int tot = 0;
 for( Int x = x1 ; x <= x2 ; ++x )
  for( Int y = y1 ; y <= y2 ; ++y ) {
   tot += A[ x ][ y ];
   ++steps;
   }
 if( ( x1 != x2 ) || ( y1 != y2 ) ) {
  const Int xm = ( x1 + x2 ) / 2 , ym = ( y1 + y2 ) / 2;
  tot += square( A , x1 , y1 , xm , ym , steps );
  tot += square( A , xm + 1 , ym + 1 , x2 , y2 , steps );
  }
 return( tot );
 }

/// 64 (2024-01-09): frec( A , si , sj , ei , ej ), a walk up or left from
/// the bottom right corner of a square of side n, between n - 1 and 2 n - 2
/// steps, and the calls on the four quadrants; its steps (calls and walk)
/// are T( n ) = 4 T( n / 2 ) + Theta( n ), T( 1 ) = 1, Theta( n^2 )
/// \pre the square is in A
Int staircase( const Matrix & A , Int si , Int sj , Int ei , Int ej ,
               Int & steps )
{
 ADS_PRE( ( 0 <= si ) && ( si <= ei ) && ( ei < Int( A.size() ) ) &&
          ( 0 <= sj ) && ( sj <= ej ) && ( ej < Int( A.size() ) ) );
 ++steps;
 if( ( si == ei ) || ( sj == ej ) )
  return( 0 );
 Int sum = 0 , i = ei , j = ej;
 while( ( i > si ) && ( j > sj ) ) {
  sum += A[ i ][ j ];
  ++steps;
  if( A[ i ][ j ] < A[ i - 1 ][ j ] )
   --i;
  else
   --j;
  }
 const Int mi = ( si + ei ) / 2 , mj = ( sj + ej ) / 2;
 sum += staircase( A , si , sj , mi , mj , steps );
 sum += staircase( A , mi + 1 , sj , ei , mj , steps );
 sum += staircase( A , si , mj + 1 , mi , ej , steps );
 sum += staircase( A , mi + 1 , mj + 1 , ei , ej , steps );
 return( sum );
 }

/// a merge sort of A[ s .. e ] counting the values written by the merges,
/// n log_2 n when n is a power of 2
void countedMergeSort( Ints & A , Int s , Int e , Int & steps )
{
 if( s >= e )
  return;
 const Int m = ( s + e ) / 2;
 countedMergeSort( A , s , m , steps );
 countedMergeSort( A , m + 1 , e , steps );
 Ints B;
 B.reserve( std::size_t( e - s + 1 ) );
 Int i = s , j = m + 1;
 while( ( i <= m ) || ( j <= e ) ) {
  B.push_back( ( j > e ) || ( ( i <= m ) && ( A[ i ] <= A[ j ] ) ) ?
               A[ i++ ] : A[ j++ ] );
  ++steps;
  }
 std::copy( B.begin() , B.end() , A.begin() + s );
 }

/// 65 (2024-02-01): fr( A1 , A2 , s1 , e1 , s2 , e2 ), on two subarrays of
/// n values a shuffle of each (n steps), two calls, a merge sort of each (n
/// log_2 n steps), two more calls; its steps are T( n ) = 4 T( n / 2 ) + 2 n
/// + 2 n log_2 n, T( 1 ) = 1, Theta( n^2 ) by the extended master theorem
/// \pre the two subarrays have the same length, a power of 2
Int shuffleSort( Ints & A1 , Ints & A2 , Int s1 , Int e1 , Int s2 , Int e2 ,
                 std::mt19937 & g , Int & steps )
{
 ADS_PRE( ( e1 - s1 == e2 - s2 ) && ( std::has_single_bit(
            std::uint64_t( e1 - s1 + 1 ) ) ) );
 if( s1 == e1 ) {
  ++steps;
  return( A1[ s1 ] > A2[ s2 ] ? 1 : -1 );
  }
 Int sum = 0;
 const Int m1 = ( s1 + e1 ) / 2 , m2 = ( s2 + e2 ) / 2;
 auto shuffle = [ & ]( Ints & A , Int s , Int e ) {
  for( Int i = s ; i <= e ; ++i ) {
   std::swap( A[ i ] ,
              A[ std::uniform_int_distribution< Int >( s , i )( g ) ] );
   ++steps;
   }
  };
 shuffle( A1 , s1 , e1 );
 shuffle( A2 , s2 , e2 );
 sum += shuffleSort( A1 , A2 , s1 , m1 , s2 , m2 , g , steps );
 sum += shuffleSort( A1 , A2 , s1 , m1 , m2 + 1 , e2 , g , steps );
 countedMergeSort( A1 , s1 , e1 , steps );
 countedMergeSort( A2 , s2 , e2 , steps );
 sum += shuffleSort( A1 , A2 , m1 + 1 , e1 , s2 , m2 , g , steps );
 sum += shuffleSort( A1 , A2 , m1 + 1 , e1 , m2 + 1 , e2 , g , steps );
 return( sum );
 }

/// 66 (2024-06-14): super( n ), a loop of floor( n / 5 ) iterations and the
/// calls on floor( n / 2 ) and floor( n / 4 ); its steps are T( n ) = T( n /
/// 2 ) + T( n / 4 ) + floor( n / 5 ), T( n ) = 1 for n < 4, Theta( n )
/// \pre n >= 0
Int superRec( Int n , Int & steps )
{
 ADS_PRE( n >= 0 );
 if( n < 4 ) {
  ++steps;
  return( n );
  }
 Int b = 1;
 for( Int i = 1 ; i <= n / 5 ; ++i ) {
  b = ( b * 2 ) % 997;
  ++steps;
  }
 return( b + superRec( n / 2 , steps ) + superRec( n / 4 , steps ) );
 }

/// 74 (2026-01-12): mystery( A , start , end ), a loop halving i = n - 1
/// down to 0 (floor( log_2( n - 1 ) ) + 1 iterations) and the calls on the
/// two halves; its steps are T( n ) = T( ceil( n / 2 ) ) + T( floor( n / 2
/// ) ) + Theta( log n ), T( 1 ) = 1, Theta( n )
/// \pre 0 <= s <= e < |A|
Int halving( const Ints & A , Int s , Int e , Int & steps )
{
 ADS_PRE( ( 0 <= s ) && ( s <= e ) && ( e < Int( A.size() ) ) );
 if( e == s ) {
  ++steps;
  return( A[ s ] );
  }
 Int sum = 0;
 for( Int i = e - s ; i > 0 ; i /= 2 ) {
  sum += A[ s + i ];
  ++steps;
  }
 const Int mid = ( s + e ) / 2;
 sum += halving( A , s , mid , steps );
 sum += halving( A , mid + 1 , e , steps );
 return( sum );
 }

/// 75 (2026-02-02): mystery( A , start , end ), the sum of fewer than 10
/// values or the calls without the first 10 and without the last 10; its
/// calls are T( n ) = 2 T( n - 10 ) + 1, T( n ) = 1 for n < 10, that is,
/// 2^{floor( n / 10 ) + 1} - 1, Theta( 2^{n / 10} )
/// \pre 0 <= s, e < |A|, e >= s - 1
Int tens( const Ints & A , Int s , Int e , Int & calls )
{
 ADS_PRE( ( 0 <= s ) && ( e < Int( A.size() ) ) && ( e >= s - 1 ) );
 ++calls;
 if( e - s + 1 < 10 ) {
  Int sum = 0;
  for( Int i = s ; i <= e ; ++i )
   sum += A[ i ];
  return( sum );
  }
 return( tens( A , s + 10 , e , calls ) + tens( A , s , e - 10 , calls ) );
 }

/// 78 (2026-08-28): mystery( A , n ), for each i the length of the run of
/// values after A[ i ] smaller than A[ i ]; Theta( n ) in the best case (A
/// non-decreasing), Theta( n^2 ) in the worst one (A decreasing, n ( n - 1
/// ) / 2 iterations); returns the iterations
Int runs( const Ints & A )
{
 const Int n = Int( A.size() );
 Int s = 0;
 for( Int i = 0 ; i <= n - 2 ; ++i )
  for( Int j = i + 1 ; ( j < n ) && ( A[ j ] < A[ i ] ) ; ++j )
   ++s;
 return( s );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ GOSSIP ------------------------------------*/
/*--------------------------------------------------------------------------*/

/// 14 (2013-09-02): the secrets known by each of n people, as masks of bits
using Known = std::vector< std::uint64_t >;

/// everybody knows only his own secret
/// \pre 1 <= n <= 64
Known secrets( int n )
{
 ADS_PRE( ( 1 <= n ) && ( n <= 64 ) );
 Known K( static_cast< std::size_t >( n ) );
 for( int i = 0 ; i < n ; ++i )
  K[ std::size_t( i ) ] = std::uint64_t( 1 ) << i;
 return( K );
 }

/// whether everybody knows every secret
bool everybody( const Known & K )
{
 const std::uint64_t all = K.size() == 64 ? ~std::uint64_t( 0 ) :
                           ( std::uint64_t( 1 ) << K.size() ) - 1;
 return( std::all_of( K.begin() , K.end() ,
                      [ & ]( std::uint64_t k ) { return( k == all ); } ) );
 }

/// 14a: the chain: in round i person i sends to i + 1, then back from n - 1
/// to 0, one message per round: 2 n - 2 messages, the fewest possible since
/// each secret must leave its owner and each person must receive at least
/// once after the last of the first n - 1 messages; returns the messages
/// \pre n >= 1
Int chainGossip( Known & K )
{
 const Int n = Int( K.size() );
 ADS_PRE( n >= 1 );
 Int messages = 0;
 for( Int i = 0 ; i + 1 < n ; ++i , ++messages )
  K[ std::size_t( i + 1 ) ] |= K[ std::size_t( i ) ];
 for( Int i = n - 1 ; i > 0 ; --i , ++messages )
  K[ std::size_t( i - 1 ) ] |= K[ std::size_t( i ) ];
 return( messages );
 }

/// 14b: the exchange on the hypercube for n = 2^k: in round r each person
/// sends to the one whose index differs in bit r; after round r each group
/// of 2^{r+1} knows its secrets: k = log_2 n rounds, the fewest possible
/// since the holders of a secret at most double in a round, and n k
/// messages; returns the rounds
/// \pre n is a power of 2
int cubeGossip( Known & K , Int & messages )
{
 const std::size_t n = K.size();
 ADS_PRE( std::has_single_bit( n ) );
 int rounds = 0;
 for( std::size_t bit = 1 ; bit < n ; bit <<= 1 , ++rounds ) {
  const Known before = K;
  for( std::size_t i = 0 ; i < n ; ++i , ++messages )
   K[ i ^ bit ] |= before[ i ];
  }
 return( rounds );
 }

/// the state of n <= 4 people as an integer of n^2 bits
std::uint32_t encode( const Known & K )
{
 std::uint32_t s = 0;
 for( std::size_t i = 0 ; i < K.size() ; ++i )
  s |= std::uint32_t( K[ i ] ) << ( i * K.size() );
 return( s );
 }

/// the people of the state s of n people
Known decode( std::uint32_t s , int n )
{
 Known K( static_cast< std::size_t >( n ) );
 for( int i = 0 ; i < n ; ++i )
  K[ std::size_t( i ) ] = ( s >> ( i * n ) ) & ( ( 1u << n ) - 1 );
 return( K );
 }

/// the fewest messages that make everybody know every secret, by breadth
/// first search on the states, each message from one person to another
/// \pre 1 <= n <= 4
Int fewestMessages( int n )
{
 ADS_PRE( ( 1 <= n ) && ( n <= 4 ) );
 std::vector< int > d( std::size_t( 1 ) << ( n * n ) , -1 );
 std::queue< std::uint32_t > q;
 const std::uint32_t s0 = encode( secrets( n ) );
 d[ s0 ] = 0;
 q.push( s0 );
 while( ! q.empty() ) {
  const std::uint32_t s = q.front();
  q.pop();
  const Known K = decode( s , n );
  if( everybody( K ) )
   return( d[ s ] );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( i != j ) {
     Known L = K;
     L[ std::size_t( j ) ] |= L[ std::size_t( i ) ];
     const std::uint32_t t = encode( L );
     if( d[ t ] < 0 ) {
      d[ t ] = d[ s ] + 1;
      q.push( t );
      }
     }
  }
 return( -1 );
 }

/// the fewest rounds that make everybody know every secret, each person
/// sending at most one message per round, by breadth first search on the
/// states over all the choices of a round
/// \pre 1 <= n <= 4
int fewestRounds( int n )
{
 ADS_PRE( ( 1 <= n ) && ( n <= 4 ) );
 std::vector< int > d( std::size_t( 1 ) << ( n * n ) , -1 );
 std::queue< std::uint32_t > q;
 const std::uint32_t s0 = encode( secrets( n ) );
 d[ s0 ] = 0;
 q.push( s0 );
 int choices = 1;
 for( int i = 0 ; i < n ; ++i )
  choices *= n;                     // nobody, or one of the n - 1 others
 while( ! q.empty() ) {
  const std::uint32_t s = q.front();
  q.pop();
  const Known K = decode( s , n );
  if( everybody( K ) )
   return( d[ s ] );
  for( int c = 0 ; c < choices ; ++c ) {
   Known L = K;
   for( int i = 0 , x = c ; i < n ; ++i , x /= n ) {
    const int t = x % n;            // t == i: no message
    if( t != i )
     L[ std::size_t( t ) ] |= K[ std::size_t( i ) ];
    }
   const std::uint32_t t = encode( L );
   if( d[ t ] < 0 ) {
    d[ t ] = d[ s ] + 1;
    q.push( t );
    }
   }
  }
 return( -1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE FIVE ALGORITHMS ----------------------------*/
/*--------------------------------------------------------------------------*/

/// 20 (2014-10-31): the orders of the five recurrences 20/1 .. 20/5
std::array< Growth , 5 > fiveAlgorithms( void )
{
 return( std::array< Growth , 5 >{ master( 1 , 1.5 , 1 ) ,
                                   master( 4 , 2 , 2.5 ) ,
                                   master( 2 , 4 , 0.5 ) ,
                                   master( 3 , 2 , 1 , 1 ) ,
                                   linear( 1 , 5.0 / 6 ) } );
 }

/// the index of the least of the orders
std::size_t best( const std::array< Growth , 5 > & G )
{
 std::size_t b = 0;
 for( std::size_t i = 1 ; i < G.size() ; ++i )
  if( G[ i ] < G[ b ] )
   b = i;
 return( b );
 }

}  // end( namespace complexity )

using namespace complexity;

/// n, n^2, n^3 and the other orders of the claims
double one( double n ) { return( n ); }
double two( double n ) { return( n * n ); }
double three( double n ) { return( n * n * n ); }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// 1, 2011-04-11, Complessita: T( n ) = T( n / 2 ) + 1 on even n, T( n - 2 )
// + 1 on odd n, T( 1 ) = 1: O( n ), tight on odd n, and Omega( log n ),
// tight on powers of 2, so no Theta
ADS_TEST( "complexity-1" )
{
 const Int N = 1 << 20;
 std::vector< double > T( std::size_t( N + 1 ) , 0 );
 T[ 1 ] = 1;
 for( Int n = 2 ; n <= N ; ++n )
  T[ std::size_t( n ) ] = 1 + ( n % 2 == 0 ? T[ std::size_t( n / 2 ) ] :
                                             T[ std::size_t( n - 2 ) ] );
 for( Int n = 1 ; n <= N ; ++n ) {
  CHECK( T[ std::size_t( n ) ] <= double( n ) );
  CHECK( T[ std::size_t( n ) ] >= lg( double( n ) ) );
  if( n % 2 == 1 )
   CHECK( T[ std::size_t( n ) ] == double( ( n + 1 ) / 2 ) );
  }
 for( int k = 0 ; k <= 20 ; ++k )
  CHECK( T[ std::size_t( 1 ) << k ] == k + 1 );
 // neither bound is Theta: the ratios run away on the two families
 CHECK( T[ std::size_t( N ) ] / double( N ) < 1e-4 );
 CHECK( T[ std::size_t( N - 1 ) ] / lg( double( N - 1 ) ) > 1e4 );
 }

// 2, 2011-05-02, Complessita: T( n ) = 3 T( n / 4 ) + T( n / 5 ) + n,
// T( n ) = 1 for n <= 1: Theta( n ), T( n ) <= 20 max( 1 , n ); the first
// values by hand, and the contracts of the evaluation (n < 0, an empty or
// reversed sample)
ADS_TEST( "complexity-2" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 3 * T( n / 4 ) + T( n / 5 ) + double( n ) ); } );
 for( Int n = 0 ; n <= 4 ; ++n )
  CHECK( T( n ) <= 20.0 * double( std::max< Int >( 1 , n ) ) );
 CHECK( ( T( 0 ) == 1 ) && ( T( 1 ) == 1 ) );
 CHECK( ( T( 2 ) == 6 ) && ( T( 3 ) == 7 ) );   // 3 T( 0 ) + T( 0 ) + n
 CHECK( T( 4 ) == 3 * 1 + 1 + 4 );
 CHECK( T( 5 ) == 3 * 1 + 1 + 5 );
 CHECK_THROWS( (void) T( -1 ) , contract_violation );
 CHECK_THROWS( (void) sample( 5 , 4 ) , contract_violation );
 CHECK_THROWS( (void) sample( -1 , 4 ) , contract_violation );
 CHECK( sample( 7 , 7 ) == Ints{ 7 } );           // a single n
 CHECK( sample( 0 , 3 ) == ( Ints{ 0 , 1 , 2 , 3 } ) );
 CHECK( upper( T , one , 20 , 1 ) );
 CHECK( lower( T , one , 1 , 1 ) );
 }

// 3, 2011-06-06, Complessita: T( n ) = 11 n / 5 + T( n / 5 ) + T( 7 n / 10
// ), T( n ) = 1 for n <= 1, the recurrence of the selection in linear
// time: Theta( n ), c = 22
ADS_TEST( "complexity-3" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 11.0 * double( n ) / 5 + T( n / 5 ) +
                         T( 7 * n / 10 ) ); } );
 CHECK( std::abs( T( 2 ) - 32.0 / 5 ) < 1e-12 );
 CHECK( std::abs( T( 3 ) - 70.0 / 5 ) < 1e-12 );
 CHECK( std::abs( T( 4 ) - 81.0 / 5 ) < 1e-12 );
 CHECK( T( 0 ) > 22 * 0.0 );                // T( 0 ) fails c n: base 1 .. 4
 CHECK( upper( T , one , 22 , 1 ) );
 CHECK( lower( T , one , 1 , 1 ) );
 }

// 4, 2011-07-18, Complessita: T( n ) = sqrt( n ) T( sqrt( n ) ) + sqrt( n ):
// O( n log log n ) and Omega( n ), and Theta( n ) by T( n ) <= c n - b
// sqrt( n ), b = 1 / ( 2^{1/4} - 1 ), c = b + 1. The solution takes the
// base T( 1 ) = 1, which the real square roots of n > 1 never reach; the
// proof holds with T( n ) = 1 for 1 <= n <= 2, the base here, where the
// lower bound c n needs c <= 1 / 2
ADS_TEST( "complexity-4" )
{
 const double b = 1 / ( std::pow( 2.0 , 0.25 ) - 1 ) , c = b + 1;
 std::function< double( double ) > T = [ & ]( double n ) {
  return( n <= 2 ? 1.0 : std::sqrt( n ) * T( std::sqrt( n ) ) +
                         std::sqrt( n ) ); };
 for( double n = 1 ; n < 1e300 ; n *= 1.01 ) {
  CHECK( T( n ) <= c * n - b * std::sqrt( n ) + 1e-9 * n );
  CHECK( T( n ) >= n / 2 );
  }
 CHECK( T( 2 ) == 1 );
 }

// 5, 2011-09-07, Complessita: T( n ) = 4 T( sqrt( n ) ) + log_2 n; with n
// = 2^k, S( k ) = 4 S( k / 2 ) + k^2 = Theta( k^2 log k ), so T( n ) =
// Theta( log^2 n log log n )
ADS_TEST( "complexity-5" )
{
 const Growth S = master( 4 , 2 , 2 );
 CHECK( same( S , 1 , 2 , 1 ) );
 CHECK( settles( 4 , 2 , []( double k ) { return( k * k ); } , S ) );
 // T( 2^{2^j} ) = S( 2^j ) with T( 2 ) = 1, against log^2 n log log n
 std::vector< double > ratio;
 double s = 1;
 for( int j = 1 ; j <= 60 ; ++j ) {
  const double k = std::pow( 2.0 , j );
  s = 4 * s + k * k;
  ratio.push_back( s / ( k * k * lg( k ) ) );
  }
 CHECK( std::abs( ratio.back() / ratio[ ratio.size() - 6 ] - 1 ) < 0.05 );
 }

// 6, 2012-01-11, Complessita: T( n ) = T( n / c ) + 1, T( n ) = 1 for n <=
// 1: Theta( log n ), between floor( log_c n ) + 1 and floor( log_c n ) + 2
// (exactly floor( log_c n ) + 1 only for c = 2)
ADS_TEST( "complexity-6" )
{
 for( const Int c : { 2 , 3 , 7 } ) {
  Memo T( [ c ]( Memo & T , Int n ) {
   return( n <= 1 ? 1.0 : T( n / c ) + 1 ); } );
  CHECK( holds( 1 , 1000000000000 , [ & ]( Int n ) {
   Int l = 0;                               // floor( log_c n )
   for( Int m = n ; m >= c ; m /= c )
    ++l;
   return( ( T( n ) >= double( l + 1 ) ) &&
           ( T( n ) <= double( l + ( c == 2 ? 1 : 2 ) ) ) ); } ) );
  if( c > 2 )
   CHECK( T( 2 ) == 2 );                    // floor( log_c 2 ) + 1 = 1
  CHECK( T( 0 ) == 1 );
  CHECK( bounded( T , []( double n ) { return( lg( n ) ); } ) );
  }
 }

// 7, 2012-05-03, Complessita: T( n ) = 2 T( n / 4 ) + sqrt( n ): Theta(
// sqrt( n ) log n ) by the master theorem; by substitution the induction
// needs c >= 1 / 2 for O and c <= 1 / 2 for Omega, the base cases 2 .. 7
ADS_TEST( "complexity-7" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 2 * T( n / 4 ) + std::sqrt( double( n ) ) ); } );
 auto g = []( double n ) { return( std::sqrt( n ) * lg( n ) ); };
 const double c = std::max( 0.5 , worst( T , g , 2 , 7 ) );
 CHECK( upper( T , g , c , 2 ) );
 CHECK( lower( T , g , 0.5 , 2 ) );
 const Growth G = master( 2 , 4 , 0.5 );
 CHECK( same( G , 1 , 0.5 , 1 ) );
 CHECK( settles( 2 , 4 , []( double n ) { return( std::sqrt( n ) ); } ,
                 G ) );
 }

// 8, 2012-07-09, Complessita: T( n ) = T( m ) + T( n - m ) + 1, T( n ) = 1
// for n <= m: Theta( n ), n / m <= T( n ) <= 2 n; exactly 2 ceil( n / m )
// - 1
ADS_TEST( "complexity-8" )
{
 for( const Int m : { 1 , 2 , 5 } ) {
  const Int N = 100000;
  std::vector< double > T( std::size_t( N + 1 ) , 1 );
  for( Int n = m + 1 ; n <= N ; ++n )
   T[ std::size_t( n ) ] = T[ std::size_t( m ) ] +
                           T[ std::size_t( n - m ) ] + 1;
  for( Int n = 1 ; n <= N ; ++n ) {
   CHECK( T[ std::size_t( n ) ] == double( 2 * ( ( n + m - 1 ) / m ) - 1 ) );
   CHECK( double( n ) / double( m ) <= T[ std::size_t( n ) ] );
   CHECK( T[ std::size_t( n ) ] <= 2.0 * double( n ) );
   }
  }
 }

// 9, 2013-01-07, Complessita: T( n ) = T( n / 4 ) + n on even n, T( n - 4
// ) + n on odd n, T( 1 ) = 1: Omega( n ) and O( n^2 ). The solution works
// on n / 2 and n - 2 instead of the n / 4 and n - 4 of the statement,
// writes c ( n - 2 )^2 + 1 for c ( n - 2 )^2 + n and concludes T( n ) <=
// c n for T( n ) <= c n^2; the bounds hold in both readings, with T( n ) =
// 1 for n <= 1 (the statement reaches T( -1 ) from n = 3)
ADS_TEST( "complexity-9" )
{
 const Int N = 1 << 17;
 for( const Int d : { 4 , 2 } ) {
  std::vector< double > T( std::size_t( N + 1 ) , 1 );
  for( Int n = 2 ; n <= N ; ++n )
   T[ std::size_t( n ) ] = double( n ) +
    ( n % 2 == 0 ? T[ std::size_t( n / d ) ] :
                   T[ std::size_t( std::max< Int >( n - d , 0 ) ) ] );
  for( Int n = 1 ; n <= N ; ++n ) {
   CHECK( T[ std::size_t( n ) ] >= double( n ) );
   CHECK( T[ std::size_t( n ) ] <= two( double( n ) ) );
   }
  // odd n: about n^2 / ( 2 d ); powers of d: about n d / ( d - 1 )
  const double odd = T[ std::size_t( N - 1 ) ] / two( double( N - 1 ) );
  CHECK( std::abs( odd - 1.0 / double( 2 * d ) ) < 1e-3 );
  Int p = 1;
  while( p * d <= N ) p *= d;
  CHECK( std::abs( T[ std::size_t( p ) ] / double( p ) -
                   double( d ) / double( d - 1 ) ) < 1e-3 );
  if( d == 2 )
   CHECK( T[ 3 ] > 3 );                     // T( n ) <= n is false
  }
 }

// 10.1, 2013-01-28, Complessita: A( n ) = 4 A( n / 2 ) + n^2 log n:
// Theta( n^2 log^2 n ) by the generalized master theorem; the substitution
// proves the loose O( n^3 ) with c >= 2
ADS_TEST( "complexity-10-1" )
{
 const Growth G = master( 4 , 2 , 2 , 1 );
 CHECK( same( G , 1 , 2 , 2 ) );
 CHECK( ! extendedMaster( 4 , 2 , 2 , 1 ) );
 CHECK( settles( 4 , 2 , []( double n ) { return( n * n * std::log( n ) ); } ,
                 G ) );
 double A = 1;
 for( int k = 1 ; k <= 60 ; ++k ) {
  const double n = std::pow( 2.0 , k );
  A = 4 * A + n * n * k;
  CHECK( A <= 2 * three( n ) );
  }
 }

// 10.2, 2013-01-28, Complessita: B( n ) = 4 B( n / 2 ) + n^2: Theta( n^2
// log n )
ADS_TEST( "complexity-10-2" )
{
 const Growth G = master( 4 , 2 , 2 );
 CHECK( same( G , 1 , 2 , 1 ) );
 CHECK( settles( 4 , 2 , two , G ) );
 Memo B( []( Memo & B , Int n ) {
  return( n <= 1 ? 1.0 : 4 * B( n / 2 ) + two( double( n ) ) ); } );
 CHECK( bounded( B , []( double n ) { return( n * n * lg( n ) ); } ) );
 }

// 10.3, 2013-01-28, Complessita: C( n ) = n C( n - 1 ): Theta( n! ),
// exactly n! C( 1 )
ADS_TEST( "complexity-10-3" )
{
 std::uint64_t C = 1 , f = 1;
 for( std::uint64_t n = 2 ; n <= 20 ; ++n ) {
  C *= n;
  f *= n;
  CHECK( C == f );
  }
 CHECK( f == 2432902008176640000ull );
 double D = 3;                              // C( 1 ) = 3
 double g = 1;
 for( int n = 2 ; n <= 150 ; ++n ) {
  D *= n;
  g *= n;
  CHECK( std::abs( D / g - 3 ) < 1e-9 );
  }
 }

// 11, 2013-05-03, Complessita: crazy( n ) costs T( n ) = T( n / 2 ) + T( n
// / 3 ) + n / 2: Theta( n ), n / 2 <= T( n ) <= 3 n; the base cases n = 0,
// 1 cost one step, n = 2, 3 one iteration and two base calls, and n < 0 is
// a contract violation
ADS_TEST( "complexity-11" )
{
 const std::array< std::pair< Int , Int > , 4 > small{ {
  { 0 , 1 } , { 1 , 1 } , { 2 , 3 } , { 3 , 3 } } };
 for( const auto & [ n , s ] : small ) {
  Int steps = 0;
  crazyHalfThird( n , steps );
  CHECK( steps == s );
  }
 Int none = 0;
 CHECK_THROWS( crazyHalfThird( -1 , none ) , contract_violation );
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : double( n / 2 ) + T( n / 2 ) + T( n / 3 ) ); } );
 std::mt19937_64 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Int n = t < 100 ? t : Int( g() % 100000 );
  Int steps = 0;
  crazyHalfThird( n , steps );
  CHECK( double( steps ) == T( n ) );
  }
 CHECK( upper( T , one , 3 , 1 ) );
 CHECK( lower( T , one , 0.5 , 1 ) );
 }

// 12, 2013-06-17, Complessita: fun( V , 1 , n ) costs T( n ) = T( n - 1 ) +
// T( floor( sqrt( n ) ) ) + n, T( 1 ) = 1: Theta( n^2 ), T( n ) <= 2 n^2
// (the procedure is run on its corrected second call, see fun()); floor(
// sqrt( n ) ) exact at 0, 1, around the squares and at the largest square
// of int64, and the contracts of fun() (i > j, j out of V) and of isqrt()
ADS_TEST( "complexity-12" )
{
 CHECK( ( isqrt( 0 ) == 0 ) && ( isqrt( 1 ) == 1 ) && ( isqrt( 2 ) == 1 ) );
 for( Int r = 2 ; r <= 2000 ; ++r ) {
  CHECK( isqrt( r * r ) == r );
  CHECK( isqrt( r * r - 1 ) == r - 1 );
  }
 const Int big = 3037000499;                // floor( sqrt( 2^63 - 1 ) )
 CHECK( isqrt( big * big ) == big );
 CHECK( isqrt( big * big - 1 ) == big - 1 );
 CHECK( isqrt( INT64_MAX ) == big );
 CHECK_THROWS( (void) isqrt( -1 ) , contract_violation );
 {
  const Ints W{ 4 , 5 };
  Int steps = 0;
  CHECK( fun( W , 1 , 1 , steps ) == 1 );   // one value: the base case
  CHECK( steps == 1 );
  CHECK_THROWS( fun( W , 1 , 0 , steps ) , contract_violation );
  CHECK_THROWS( fun( W , 0 , 2 , steps ) , contract_violation );
  CHECK_THROWS( fun( W , -1 , 0 , steps ) , contract_violation );
  }
 const Int N = 100000;
 std::vector< double > T( std::size_t( N + 1 ) , 1 );
 for( Int n = 2 ; n <= N ; ++n )
  T[ std::size_t( n ) ] = T[ std::size_t( n - 1 ) ] +
                          T[ std::size_t( isqrt( n ) ) ] + double( n );
 std::mt19937 g( 1 );
 Ints V( 600 );
 for( Int & v : V )
  v = Int( g() % 10 );
 for( Int n = 1 ; n <= 600 ; n += ( n < 30 ? 1 : 37 ) ) {
  Int steps = 0;
  fun( V , 0 , n - 1 , steps );
  CHECK( double( steps ) == T[ std::size_t( n ) ] );
  }
 for( Int n = 1 ; n <= N ; ++n ) {
  CHECK( T[ std::size_t( n ) ] <= 2 * two( double( n ) ) );
  CHECK( T[ std::size_t( n ) ] >= two( double( n ) ) / 2 );
  }
 // the ratio converges, to 1 from below
 CHECK( T[ std::size_t( N ) ] / two( double( N ) ) > 0.95 );
 }

// 13, 2013-07-22, Complessita: T( n ) = 2 T( 2 n / 3 ) + n^2: Theta( n^2 ),
// c = 9, m = 1
ADS_TEST( "complexity-13" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 2 * T( 2 * n / 3 ) + two( double( n ) ) ); } );
 CHECK( upper( T , two , 9 , 1 ) );
 CHECK( lower( T , two , 1 , 1 ) );
 const Growth G = master( 2 , 1.5 , 2 );
 CHECK( same( G , 1 , 2 ) );
 CHECK( settles( 2 , 1.5 , two , G ) );
 }

// 14a, 2013-09-02, Complessita: n people, a secret each, one message per
// person per round: Theta( n ) messages, 2 n - 2 by the chain; n = 2 needs
// 1 round and 2 messages; one person needs nothing, 64 people fill the
// whole mask, and 0 or 65 people (or fewer than 1 or more than 4 for the
// search) are contract violations
ADS_TEST( "complexity-14-a" )
{
 CHECK( everybody( secrets( 1 ) ) );
 CHECK( ! everybody( secrets( 2 ) ) && ! everybody( secrets( 64 ) ) );
 CHECK( fewestRounds( 1 ) == 0 );
 CHECK_THROWS( (void) secrets( 0 ) , contract_violation );
 CHECK_THROWS( (void) secrets( 65 ) , contract_violation );
 CHECK_THROWS( (void) fewestMessages( 0 ) , contract_violation );
 CHECK_THROWS( (void) fewestMessages( 5 ) , contract_violation );
 CHECK_THROWS( (void) fewestRounds( 5 ) , contract_violation );
 {
  Known none;
  CHECK_THROWS( chainGossip( none ) , contract_violation );
  }
 CHECK( fewestMessages( 2 ) == 2 );
 CHECK( fewestRounds( 2 ) == 1 );
 CHECK( fewestMessages( 1 ) == 0 );
 for( int n = 1 ; n <= 4 ; ++n )
  CHECK( fewestMessages( n ) == 2 * n - 2 );
 for( int n = 1 ; n <= 64 ; ++n ) {
  Known K = secrets( n );
  CHECK( chainGossip( K ) == 2 * n - 2 );
  CHECK( everybody( K ) );
  }
 }

// 14b, 2013-09-02, Complessita: the same people: Theta( log n ) rounds,
// log_2 n by the exchange on the hypercube, since the holders of a secret
// at most double in a round; one person needs 0 rounds and 0 messages, and
// a number of people that is not a power of 2 is a contract violation
ADS_TEST( "complexity-14-b" )
{
 for( const int n : { 3 , 5 , 6 , 63 } ) {
  Known K = secrets( n );
  Int messages = 0;
  CHECK_THROWS( cubeGossip( K , messages ) , contract_violation );
  }
 {
  Known K;                                  // nobody: 0 is not a power of 2
  Int messages = 0;
  CHECK_THROWS( cubeGossip( K , messages ) , contract_violation );
  }
 for( int n = 1 ; n <= 4 ; ++n )
  CHECK( fewestRounds( n ) == int( std::bit_width( unsigned( n - 1 ) ) ) );
 for( int k = 0 ; k <= 6 ; ++k ) {
  const int n = 1 << k;
  Known K = secrets( n );
  Int messages = 0;
  CHECK( cubeGossip( K , messages ) == k );
  CHECK( everybody( K ) );
  CHECK( messages == Int( n ) * k );
  }
 // random protocols: the holders of each secret at most double per round
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const int n = 2 + int( g() % 30 );
  Known K = secrets( n );
  for( int r = 0 ; r < 8 ; ++r ) {
   const Known before = K;
   for( int i = 0 ; i < n ; ++i ) {
    const int j = int( g() % unsigned( n ) );
    if( ( j != i ) && ( g() % 4 != 0 ) )
     K[ std::size_t( j ) ] |= before[ std::size_t( i ) ];
    }
   for( int s = 0 ; s < n ; ++s ) {
    int h0 = 0 , h1 = 0;
    for( int i = 0 ; i < n ; ++i ) {
     h0 += int( ( before[ std::size_t( i ) ] >> s ) & 1 );
     h1 += int( ( K[ std::size_t( i ) ] >> s ) & 1 );
     }
    CHECK( h1 <= 2 * h0 );
    }
   }
  }
 }

// 15, 2014-02-03, Complessita: T( n ) = sum_{i=1}^{floor( log n )} T( n /
// 2^i ) + 1: O( n ) by T( n ) <= c n - b with b >= 1 and c >= b + 1. The
// solution writes c = 1, b = 1, for which the base T( 1 ) = 1 <= c - b = 0
// is false; c = 2, b = 1 works
ADS_TEST( "complexity-15" )
{
 Memo T( []( Memo & T , Int n ) {
  if( n <= 1 )
   return( 1.0 );
  double s = 1;
  for( int i = 1 ; i <= int( std::bit_width( std::uint64_t( n ) ) ) - 1 ;
       ++i )
   s += T( n >> i );
  return( s ); } );
 CHECK( T( 1 ) > 1 * 1.0 - 1 );
 CHECK( holds( 1 , 10000000 , [ & ]( Int n ) {
  return( T( n ) <= 2.0 * double( n ) - 1 ); } ) );
 CHECK( bounded( T , one , 10000000 ) );
 }

// 16, 2014-04-24, Complessita: mistery( A , i , j ) costs at most T( n ) =
// 2 T( n / 2 ) + 2^n: O( 2^n ) by the third case, with 2 2^{n/2} <= 2^n / 2
// for n >= 4; the empty range and one value cost 0 and 1 steps, and more
// than 40 values or a range out of A are contract violations
ADS_TEST( "complexity-16" )
{
 Memo W( []( Memo & W , Int n ) {
  return( n <= 1 ? 1.0 : W( ( n + 1 ) / 2 ) + W( n / 2 ) +
                         std::ldexp( 1.0 , int( n ) ) ); } );
 std::mt19937 g( 1 );
 Ints A( 20 );
 for( Int & a : A )
  a = Int( g() % 100 );
 auto worstRnd = []( Int n ) { return( n - 1 ); };
 auto rnd = [ & ]( Int n ) {
  return( std::uniform_int_distribution< Int >( 0 , n - 1 )( g ) ); };
 for( Int n = 1 ; n <= 20 ; ++n ) {
  Int steps = 0;
  mistery( A , 0 , n - 1 , worstRnd , steps );
  CHECK( double( steps ) == W( n ) );
  for( int t = 0 ; t < 20 ; ++t ) {
   Int s = 0;
   mistery( A , 0 , n - 1 , rnd , s );
   CHECK( s <= steps );
   }
  }
 for( Int n = 4 ; n <= 1000 ; ++n ) {
  CHECK( 2 * std::pow( 2.0 , double( n ) / 2 ) <=
         std::pow( 2.0 , double( n ) ) / 2 );
  CHECK( W( n ) <= 2 * std::ldexp( 1.0 , int( n ) ) );
  }
 CHECK( std::abs( W( 1000 ) / std::ldexp( 1.0 , 1000 ) - 1 ) < 1e-9 );
 Int steps = 0;
 CHECK( mistery( A , 3 , 2 , rnd , steps ) == 0 );   // empty
 CHECK( steps == 0 );
 CHECK( mistery( A , 7 , 7 , rnd , steps ) == 2 * A[ 7 ] );   // one value
 CHECK( steps == 1 );
 const Ints L( 50 , 1 );                     // n = 41: 2^41 reads refused
 CHECK_THROWS( mistery( L , 0 , 40 , worstRnd , steps ) , contract_violation );
 CHECK_THROWS( mistery( A , 0 , 20 , rnd , steps ) , contract_violation );
 }

// 17, 2014-06-16, 24, 2015-07-14, and 32, 2017-02-07, Complessita: T( n ) =
// 2 T( n / 8 ) + 3 n^{1/3}: Theta( n^{1/3} log n ); the upper bound by
// substitution needs c >= 1 / 3 and the base cases 2 .. 15, the lower one
// c <= 1 / 3
ADS_TEST( "complexity-17" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 2 * T( n / 8 ) + 3 * std::cbrt( double( n ) ) ); } );
 auto g = []( double n ) { return( std::cbrt( n ) * lg( n ) ); };
 const double c = std::max( 1.0 / 3 , worst( T , g , 2 , 15 ) );
 CHECK( upper( T , g , c , 2 ) );
 CHECK( lower( T , g , 1.0 / 3 , 2 ) );
 CHECK( T( 1 ) >= 0 );                      // base of the lower bound
 const Growth G = master( 2 , 8 , 1.0 / 3 );
 CHECK( same( G , 1 , 1.0 / 3 , 1 ) );
 CHECK( settles( 2 , 8 , []( double n ) { return( 3 * std::cbrt( n ) ); } ,
                 G ) );
 }

// 18, 2014-07-21, Complessita: T( n ) = min_{1 <= k < n} ( T( k ) + T( n -
// k ) ) + 1: O( n ) by T( n ) <= c n - b, b >= 1, c = 2; exactly 2 n - 1
ADS_TEST( "complexity-18" )
{
 const int N = 1000;
 std::vector< double > T( N + 1 , 1 );
 for( int n = 2 ; n <= N ; ++n ) {
  double m = HUGE_VAL;
  for( int k = 1 ; k < n ; ++k )
   m = std::min( m , T[ std::size_t( k ) ] + T[ std::size_t( n - k ) ] );
  T[ std::size_t( n ) ] = m + 1;
  }
 for( int n = 1 ; n <= N ; ++n )
  CHECK( T[ std::size_t( n ) ] == 2 * n - 1 );
 }

// 19, 2014-09-01, Complessita: T( n ) = 6 T( n / 8 ) + T( n / 4 ) + 1: O(
// n ) by T( n ) <= c n - b, b >= 1 / 6. The solution checks only the base
// T( 1 ) and takes c >= b + 1, but n = 2 .. 7 reach T( 0 ) and T( 1 ),
// which do not satisfy c n - b, and T( 2 ) = 8 > 2 c - b; b = 1 / 6 needs
// c >= 49 / 12
ADS_TEST( "complexity-19" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 6 * T( n / 8 ) + T( n / 4 ) + 1 ); } );
 const double b = 1.0 / 6;
 CHECK( T( 2 ) == 8 );
 CHECK( T( 2 ) > 2 * ( b + 1 ) - b );
 CHECK( holds( 1 , 10000000 , [ & ]( Int n ) {
  return( T( n ) <= 49.0 / 12 * double( n ) - b + 1e-9 ); } ) );
 CHECK( ! holds( 2 , 2 , [ & ]( Int n ) {
  return( T( n ) <= 4.08 * double( n ) - b ); } ) );
 CHECK( bounded( T , one ) );
 }

// 20.1, 2014-10-31, Complessita: T( n ) = T( 2 n / 3 ) + 2 n - 4: Theta(
// n ) by the third case; of the five algorithms 20.1 .. 20.5 the best is
// the third, Theta( sqrt( n ) log n )
ADS_TEST( "complexity-20-1" )
{
 const Growth G = master( 1 , 1.5 , 1 );
 CHECK( same( G , 1 , 1 ) );
 CHECK( settles( 1 , 1.5 , []( double n ) { return( 2 * n - 4 ); } , G ) );
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( 2 * n / 3 ) + 2.0 * double( n ) - 4 ); } );
 CHECK( bounded( T , one ) );
 CHECK( best( fiveAlgorithms() ) == 2 );
 }

// 20.2, 2014-10-31, Complessita: T( n ) = 4 T( n / 2 ) + n^2 sqrt( n ):
// Theta( n^2 sqrt( n ) ) by the third case, regularity with c = 1 /
// sqrt( 2 )
ADS_TEST( "complexity-20-2" )
{
 const Growth G = master( 4 , 2 , 2.5 );
 CHECK( same( G , 1 , 2.5 ) );
 auto f = []( double n ) { return( std::pow( n , 2.5 ) ); };
 CHECK( settles( 4 , 2 , f , G ) );
 for( double n = 2 ; n < 1e100 ; n *= 2 )
  CHECK( 4 * f( n / 2 ) <= f( n ) / std::sqrt( 2.0 ) * ( 1 + 1e-12 ) );
 Memo T( [ f ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 4 * T( n / 2 ) + f( double( n ) ) ); } );
 CHECK( bounded( T , f ) );
 CHECK( best( fiveAlgorithms() ) == 2 );
 }

// 20.3, 2014-10-31, Complessita: T( n ) = 2 T( n / 4 ) + sqrt( n ) + 10 log
// n: Theta( sqrt( n ) log n ) by the second case, the best of the five
ADS_TEST( "complexity-20-3" )
{
 const Growth G = master( 2 , 4 , 0.5 );
 CHECK( same( G , 1 , 0.5 , 1 ) );
 auto f = []( double n ) { return( std::sqrt( n ) + 10 * lg( n ) ); };
 CHECK( settles( 2 , 4 , f , G ) );
 Memo T( [ f ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 2 * T( n / 4 ) + f( double( n ) ) ); } );
 CHECK( bounded( T , []( double n ) { return( std::sqrt( n ) * lg( n ) ); }
                 ) );
 const auto five = fiveAlgorithms();
 CHECK( best( five ) == 2 );
 for( std::size_t i = 0 ; i < five.size() ; ++i )
  if( i != 2 )
   CHECK( five[ 2 ] < five[ i ] );
 }

// 20.4, 2014-10-31, Complessita: T( n ) = 3 T( n / 2 ) + 2 n log n + 10 n:
// Theta( n^{log_2 3} ) by the first case
ADS_TEST( "complexity-20-4" )
{
 const Growth G = master( 3 , 2 , 1 , 1 );
 CHECK( same( G , 1 , std::log2( 3.0 ) ) );
 auto f = []( double n ) { return( 2 * n * lg( n ) + 10 * n ); };
 CHECK( settles( 3 , 2 , f , G ) );
 Memo T( [ f ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 3 * T( n / 2 ) + f( double( n ) ) ); } );
 CHECK( bounded( T , []( double n ) { return( std::pow( n , lg( 3 ) ) ); }
                 ) );
 CHECK( best( fiveAlgorithms() ) == 2 );
 }

// 20.5, 2014-10-31, Complessita: T( n ) = T( n - 6 ) + n^{5/6}: Theta(
// n^{11/6} ) by the linear recurrences of constant order
ADS_TEST( "complexity-20-5" )
{
 const Growth G = linear( 1 , 5.0 / 6 );
 CHECK( same( G , 1 , 11.0 / 6 ) );
 const Int N = 1000000;
 std::vector< double > T( std::size_t( N + 1 ) , 1 );
 for( Int n = 2 ; n <= N ; ++n )
  T[ std::size_t( n ) ] = T[ std::size_t( std::max< Int >( n - 6 , 0 ) ) ] +
                          std::pow( double( n ) , 5.0 / 6 );
 double lo = HUGE_VAL , hi = 0;
 for( Int n = 100 ; n <= N ; ++n ) {
  const double r = T[ std::size_t( n ) ] / G.at( double( n ) );
  lo = std::min( lo , r );
  hi = std::max( hi , r );
  }
 CHECK( hi <= 2 * lo );
 // the ratio tends to 1 / ( 6 ( 11 / 6 ) ) = 1 / 11
 CHECK( std::abs( T[ std::size_t( N ) ] / G.at( double( N ) ) - 1.0 / 11 ) <
        1e-3 );
 CHECK( best( fiveAlgorithms() ) == 2 );
 }

// 21, 2015-01-12, Complessita: T( n ) = T( n / 2 ) + 2^n: Theta( 2^n ),
// 2^n <= T( n ) <= 2 2^n for n >= 2
ADS_TEST( "complexity-21" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( n / 2 ) + std::ldexp( 1.0 , int( n ) ) ); } );
 for( Int n = 2 ; n <= 1000 ; ++n ) {
  const double r = T( n ) / std::ldexp( 1.0 , int( n ) );
  CHECK( ( 1 <= r ) && ( r <= 2 ) );
  }
 CHECK( T( 1 ) == 1 );
 CHECK( std::abs( T( 1000 ) / std::ldexp( 1.0 , 1000 ) - 1 ) < 1e-9 );
 }

// 22, 2015-02-02, Complessita: T( n ) = T( n / sqrt( 2 ) ) + T( n / 2 ) + T(
// n / ( 2 sqrt( 2 ) ) ) + T( n / 4 ) + n^2: Theta( n^2 ), c = 16; T( 2 ) and
// T( 3 ) reach T( 0 ) and are base cases
ADS_TEST( "complexity-22" )
{
 const double r2 = std::sqrt( 2.0 );
 Memo T( [ r2 ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( down( n , r2 ) ) + T( n / 2 ) +
                         T( down( n , 2 * r2 ) ) + T( n / 4 ) +
                         two( double( n ) ) ); } );
 CHECK( T( 0 ) > 16 * 0.0 );
 CHECK( upper( T , two , 16 , 1 ) );
 CHECK( lower( T , two , 1 , 1 ) );
 }

// 23, 2015-06-08, Complessita: T( n ) = T( n / 2^{1/3} ) + T( n / 5^{1/3} )
// + T( n / 7^{1/3} ) + n^3: Theta( n^3 ), c = 70 / 11
ADS_TEST( "complexity-23" )
{
 const double a = std::cbrt( 2.0 ) , b = std::cbrt( 5.0 ) ,
              c = std::cbrt( 7.0 );
 Memo T( [ a , b , c ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( down( n , a ) ) + T( down( n , b ) ) +
                         T( down( n , c ) ) + three( double( n ) ) ); } );
 CHECK( upper( T , three , 70.0 / 11 , 1 ) );
 CHECK( lower( T , three , 1 , 1 ) );
 }

// 25.1, 2015-11-05, Complessita: T( n ) = T( beta n ) + n^beta, 0 < beta <
// 1: Theta( n^beta ) by the third case; the version T( n ) = T( beta n ) /
// beta + n^beta is Theta( n ) by the first case
ADS_TEST( "complexity-25-1" )
{
 for( const double beta : { 0.3 , 0.5 , 0.8 } ) {
  auto f = [ beta ]( double n ) { return( std::pow( n , beta ) ); };
  const Growth G = master( 1 , 1 / beta , beta ) ,
               H = master( 1 / beta , 1 / beta , beta );
  CHECK( same( G , 1 , beta ) );
  CHECK( same( H , 1 , 1 ) );
  CHECK( settles( 1 , 1 / beta , f , G ) );
  CHECK( settles( 1 / beta , 1 / beta , f , H ) );
  Memo T( [ beta , f ]( Memo & T , Int n ) {
   return( n <= 1 ? 1.0 : T( Int( beta * double( n ) ) ) + f( double( n ) ) );
   } );
  Memo U( [ beta , f ]( Memo & U , Int n ) {
   return( n <= 1 ? 1.0 : U( Int( beta * double( n ) ) ) / beta +
                          f( double( n ) ) ); } );
  CHECK( bounded( T , f ) );
  CHECK( bounded( U , one ) );
  }
 }

// 25.2, 2015-11-05, Complessita: T( n ) = T( n / 2 ) + T( 4 n / 5 ) + T( 3 n
// / 10 ) + n^2: Theta( n^2 ), c = 50
ADS_TEST( "complexity-25-2" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( n / 2 ) + T( 4 * n / 5 ) + T( 3 * n / 10 ) +
                         two( double( n ) ) ); } );
 CHECK( upper( T , two , 50 , 1 ) );
 CHECK( lower( T , two , 1 , 1 ) );
 }

// 26.1, 2016-01-26, Complessita: T( n ) = ( T( n - 1 ) + T( 3 n / 4 ) ) / 2
// + n: Theta( n ), c = 8
ADS_TEST( "complexity-26-1" )
{
 const Int N = 1000000;
 std::vector< double > T( std::size_t( N + 1 ) , 1 );
 for( Int n = 2 ; n <= N ; ++n )
  T[ std::size_t( n ) ] = ( T[ std::size_t( n - 1 ) ] +
                            T[ std::size_t( 3 * n / 4 ) ] ) / 2 + double( n );
 for( Int n = 1 ; n <= N ; ++n ) {
  CHECK( T[ std::size_t( n ) ] <= 8.0 * double( n ) );
  CHECK( T[ std::size_t( n ) ] >= double( n ) );
  }
 }

// 26.2, 2016-01-26, Complessita: T( n ) = T( n / 2 ) + T( n / 4 ) + T( n /
// 6 ) + T( n / 12 ) + 1: Theta( n ), Omega with c <= 1 and O by T( n ) <=
// c n - b, b >= 1 / 3. The solution checks only the base T( 1 ) and takes
// c >= b + 1, but n = 2 .. 11 reach T( 0 ), and T( 2 ) = 5 > 2 c - b;
// b = 1 / 3 needs c >= 8 / 3
ADS_TEST( "complexity-26-2" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( n / 2 ) + T( n / 4 ) + T( n / 6 ) + T( n / 12 ) +
                         1 ); } );
 const double b = 1.0 / 3;
 CHECK( T( 2 ) == 5 );
 CHECK( T( 2 ) > 2 * ( b + 1 ) - b );
 CHECK( holds( 1 , 10000000 , [ & ]( Int n ) {
  return( T( n ) <= 8.0 / 3 * double( n ) - b + 1e-9 ); } ) );
 CHECK( lower( T , one , 1 , 1 ) );
 }

// 27, 2016-06-06, Complessita: T( n ) = T( n / 10 ) + T( 5 n / 6 ) + T( n /
// 16 ) + n: Theta( n ), c = 240 (the fractions sum to 239 / 240, so that
// T( n ) / n is still below 30 at n = 10^7 and creeps up to its limit)
ADS_TEST( "complexity-27" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( n / 10 ) + T( 5 * n / 6 ) + T( n / 16 ) +
                         double( n ) ); } );
 CHECK( upper( T , one , 240 , 1 ) );
 CHECK( lower( T , one , 1 , 1 ) );
 CHECK( T( 10000000 ) < 30.0 * 10000000 );
 }

// 28, 2016-07-06, Complessita: T( n ) = 2 T( n / sqrt( 2 ) - 5 ) + n^{pi /
// 2}: O( n^2 ) by T( n ) <= 2 T( n / sqrt( 2 ) ) + n^{pi / 2} and the first
// case
ADS_TEST( "complexity-28" )
{
 const double r2 = std::sqrt( 2.0 ) , e = std::numbers::pi / 2;
 const Growth G = master( 2 , r2 , e );
 CHECK( same( G , 1 , 2 ) );
 CHECK( settles( 2 , r2 , [ e ]( double n ) { return( std::pow( n , e ) ); } ,
                 G ) );
 Memo T( [ r2 , e ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 2 * T( std::max< Int >( down( n , r2 ) - 5 , 0 ) ) +
                         std::pow( double( n ) , e ) ); } );
 CHECK( upper( T , two , 2 , 1 , 1000000000000 ) );
 CHECK( bounded( T , two ) );
 }

// 29, 2016-08-29, Complessita: T( n ) = T( n / 15 ) + T( n / 10 ) + 2 T( n /
// 6 ) + sqrt( n ): O( n ) by substitution, the base cases n < 15 reaching
// T( 0 ), and O( n^{log_6 4} ) by T( n ) <= 4 T( n / 6 ) + sqrt( n ); not
// O( sqrt( n ) ). The solution evaluates T( 12 ) = 6 + sqrt( 2 ) + 2 sqrt(
// 3 ), but T( 12 ) = T( 0 ) + T( 1 ) + 2 T( 2 ) + sqrt( 12 ) = 10 + 2 sqrt(
// 2 ) + 2 sqrt( 3 ). The exact order is Theta( n^p ), p = 0.6596... of
// Akra and Bazzi; one term of Akra and Bazzi is the master theorem (p
// negative when sum a_i < 1), and its contracts
ADS_TEST( "complexity-29" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( n / 15 ) + T( n / 10 ) + 2 * T( n / 6 ) +
                         std::sqrt( double( n ) ) ); } );
 const double r2 = std::sqrt( 2.0 ) , r3 = std::sqrt( 3.0 );
 CHECK( std::abs( T( 2 ) - ( 4 + r2 ) ) < 1e-12 );
 CHECK( std::abs( T( 12 ) - ( 10 + 2 * r2 + 2 * r3 ) ) < 1e-12 );
 CHECK( std::abs( T( 12 ) - ( 6 + r2 + 2 * r3 ) ) > 1 );
 const double c = std::max( 2.0 , worst( T , one , 1 , 14 ) );
 CHECK( upper( T , one , c , 1 ) );
 const double q = std::log( 4.0 ) / std::log( 6.0 );
 auto g = [ q ]( double n ) { return( std::pow( n , q ) ); };
 CHECK( upper( T , g , ratios( T , g , 1 , 1000 ).second , 1 ,
               1000000000000 ) );
 // not O( sqrt( n ) ): the ratio grows by more than 10 from 10^3 to 10^12
 CHECK( T( 1000000000000 ) / 1e6 > 10 * T( 1000 ) / std::sqrt( 1000.0 ) );
 const double p = akraBazziExponent( { { 1 , 1.0 / 15 } , { 1 , 0.1 } ,
                                       { 2 , 1.0 / 6 } } );
 CHECK( ( 0.6595 < p ) && ( p < 0.6597 ) );
 // one term is the master theorem; the terms are contracts (none, a_i <=
 // 0, b_i outside ( 0 , 1 ))
 CHECK( std::abs( akraBazziExponent( { { 2 , 0.5 } } ) - 1 ) < 1e-9 );
 CHECK( akraBazzi( { { 2 , 0.5 } } , 1 ) == master( 2 , 2 , 1 ) );
 CHECK( akraBazzi( { { 1 , 0.5 } } , 0 , -1 ) == master( 1 , 2 , 0 , -1 ) );
 CHECK( std::abs( akraBazziExponent( { { 1 , 0.5 } } ) ) < 1e-9 );
 CHECK( std::abs( akraBazziExponent( { { 0.5 , 0.5 } } ) + 1 ) < 1e-9 );
 using Terms = std::vector< std::pair< double , double > >;
 for( const Terms & bad : { Terms{} , Terms{ { 0 , 0.5 } } ,
                            Terms{ { -1 , 0.5 } } , Terms{ { 1 , 0 } } ,
                            Terms{ { 1 , 1 } } , Terms{ { 1 , 0.5 } ,
                                                        { 1 , 1.5 } } } ) {
  CHECK_THROWS( (void) akraBazziExponent( bad ) , contract_violation );
  CHECK_THROWS( (void) akraBazzi( bad , 1 ) , contract_violation );
  }
 CHECK( bounded( T , [ p ]( double n ) { return( std::pow( n , p ) ); } ) );
 }

// 30, 2016-11-03, Complessita: T( n ) = 27 T( n / 9 ) + n sqrt( n ): Theta(
// n^{3/2} log n ); the induction needs c >= 1 / log 9, the base cases 2 ..
// 17, m = 2
ADS_TEST( "complexity-30" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 27 * T( n / 9 ) + std::pow( double( n ) , 1.5 ) );
  } );
 auto g = []( double n ) { return( std::pow( n , 1.5 ) * lg( n ) ); };
 const double c = std::max( 1 / lg( 9 ) , worst( T , g , 2 , 17 ) );
 CHECK( upper( T , g , c , 2 ) );
 CHECK( bounded( T , g ) );
 const Growth G = master( 27 , 9 , 1.5 );
 CHECK( same( G , 1 , 1.5 , 1 ) );
 CHECK( settles( 27 , 9 , []( double n ) { return( std::pow( n , 1.5 ) ); } ,
                 G ) );
 }

// 31, 2017-01-24, Complessita: crazy( n ) costs T( n ) = 2 T( n / 4 ) + T(
// n / 2 ) + n^2: Theta( n^2 ), the induction with c >= 8 / 5. The solution
// checks only the base T( 1 ), but n = 2, 3 reach T( 0 ), and T( 2 ) = 7 >
// ( 8 / 5 ) 4; the witness is c = 7 / 4; crazy( 2 ) by hand, and n < 0 is
// a contract violation
ADS_TEST( "complexity-31" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : 2 * T( n / 4 ) + T( n / 2 ) + two( double( n ) ) );
  } );
 CHECK( T( 2 ) == 7 );
 CHECK( T( 2 ) > 8.0 / 5 * 4 );
 CHECK( upper( T , two , 7.0 / 4 , 1 ) );
 CHECK( lower( T , two , 1 , 1 ) );
 Memo C( []( Memo & C , Int n ) {
  return( n <= 1 ? 1.0 : 2 * C( n / 4 ) + C( n / 2 ) +
                         double( n * ( n + 1 ) / 2 ) ); } );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 200 ; ++t ) {
  const Int n = t < 50 ? t : Int( g() % 1000 );
  Int steps = 0;
  crazyQuarters( n , steps );
  CHECK( double( steps ) == C( n ) );
  }
 CHECK( upper( C , two , 7.0 / 4 , 1 ) );
 CHECK( lower( C , two , 0.5 , 1 ) );
 Int steps = 0;                             // n = 2: 3 iterations, 3 bases
 crazyQuarters( 2 , steps );
 CHECK( steps == 6 );
 CHECK_THROWS( crazyQuarters( -1 , steps ) , contract_violation );
 }

// 33, 2017-06-08, Complessita: T( n ) = T( n / 2 ) + T( n / 4 ) + n on the
// multiples of 4 that are not multiples of 81, T( n / 9 ) + T( n / 81 ) +
// sqrt( n ) on the multiples of 81, log n otherwise: O( n ), reached on the
// powers of 4, and Omega( log n ), reached on the other n
ADS_TEST( "complexity-33" )
{
 Memo T( []( Memo & T , Int n ) {
  if( n <= 1 )
   return( 1.0 );
  if( n % 81 == 0 )
   return( T( n / 9 ) + T( n / 81 ) + std::sqrt( double( n ) ) );
  if( n % 4 == 0 )
   return( T( n / 2 ) + T( n / 4 ) + double( n ) );
  return( lg( double( n ) ) ); } );
 CHECK( upper( T , one , 4 , 1 , 1000000000000 ) );
 CHECK( lower( T , []( double n ) { return( lg( n ) ); } , 1 , 2 ,
               1000000000000 ) );
 CHECK( holds( 2 , 1000000 , [ & ]( Int n ) {
  return( ( n % 2 == 0 ) || ( n % 81 == 0 ) ||
          ( T( n ) == lg( double( n ) ) ) ); } ) );
 // powers of 4: T( n ) / n tends to 4
 const double r = T( Int( 1 ) << 40 ) / std::ldexp( 1.0 , 40 );
 CHECK( ( 3.99 < r ) && ( r < 4 ) );
 }

// 34, 2017-06-29, Complessita: T( n ) = 2 T( n / 2 ) + 4 T( n / 4 ) + 15 T(
// n / 8 ) + n^2, T( n ) = 1 for n <= 8: Theta( n^2 ), c = 64
ADS_TEST( "complexity-34" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 8 ? 1.0 : 2 * T( n / 2 ) + 4 * T( n / 4 ) + 15 * T( n / 8 ) +
                         two( double( n ) ) ); } );
 CHECK( upper( T , two , 64 , 1 ) );
 CHECK( lower( T , two , 1 , 9 ) );
 }

// 35, 2017-09-04, Complessita: T( n ) = 3 T( n / 3 ) + 6 T( n / 6 ) + 54 T(
// n / 12 ) + n^2, T( n ) = 1 for n <= 12: Theta( n^2 ), c = 8 (the
// solution writes 6 / 16 for the 6 / 36 of the sum 7 / 8)
ADS_TEST( "complexity-35" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 12 ? 1.0 : 3 * T( n / 3 ) + 6 * T( n / 6 ) + 54 * T( n / 12 )
                          + two( double( n ) ) ); } );
 CHECK( std::abs( 3.0 / 9 + 6.0 / 36 + 54.0 / 144 - 7.0 / 8 ) < 1e-15 );
 CHECK( upper( T , two , 8 , 1 ) );
 CHECK( lower( T , two , 1 , 13 ) );
 }

// 36, 2018-01-08, Complessita: T( n ) = 16 T( n / 4 ) + 256 T( n / 16 ) +
// n^2 sqrt( n ), T( n ) = 1 for n < 16: Theta( n^2 sqrt( n ) ), c = 4
ADS_TEST( "complexity-36" )
{
 auto g = []( double n ) { return( std::pow( n , 2.5 ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n < 16 ? 1.0 : 16 * T( n / 4 ) + 256 * T( n / 16 ) +
                         g( double( n ) ) ); } );
 CHECK( upper( T , g , 4 , 1 ) );
 CHECK( lower( T , g , 1 , 16 ) );
 }

// 37, 2018-01-31, Complessita: T( n ) = T( n / 2 ) + T( n / 3 ) + n log n:
// Theta( n log n ), c = 6, m = 2, since T( 1 ) = 1 > c 1 log 1
ADS_TEST( "complexity-37" )
{
 auto g = []( double n ) { return( n * lg( n ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( n / 2 ) + T( n / 3 ) + g( double( n ) ) ); } );
 CHECK( T( 2 ) == 4 );
 CHECK( T( 1 ) > 6 * g( 1 ) );
 CHECK( upper( T , g , 6 , 2 ) );
 CHECK( lower( T , g , 1 , 2 ) );
 }

// 38, 2018-06-28, Complessita: T_a( n ) = 7 T_a( n / 3 ) + T_a( n / a ) +
// n^2 log n is O( n^2 log n ) for every integer a >= 3 and not for a = 2
// (7 / 9 + 1 / 4 > 1). The solution adds that the least real a is sqrt( 9 /
// 2 ); there 7 / 9 + 2 / 9 = 1 and T is Theta( n^2 log^2 n ), so the bound
// holds for every a > sqrt( 9 / 2 ) but not for a = sqrt( 9 / 2 ) itself
ADS_TEST( "complexity-38" )
{
 auto g = []( double n ) { return( n * n * lg( n ) ); };
 auto make = [ g ]( double a ) {
  return( Memo( [ a , g ]( Memo & T , Int n ) {
   return( n <= 1 ? 1.0 : 7 * T( n / 3 ) + T( down( n , a ) ) +
                          g( double( n ) ) ); } ) ); };
 Memo T3 = make( 3 );
 CHECK( same( master( 8 , 3 , 2 , 1 ) , 1 , 2 , 1 ) );
 for( double n = 3 ; n < 1e100 ; n *= 3 )            // regularity, c = 8 / 9
  CHECK( 8 * g( n / 3 ) <= 8.0 / 9 * g( n ) );
 for( const double a : { 3.0 , 4.0 , 5.0 , 6.0 } ) {
  Memo T = make( a );
  CHECK( bounded( T , g ) );
  CHECK( holds( 1 , 100000000 , [ & ]( Int n ) {
   return( T( n ) <= T3( n ) ); } ) );
  }
 CHECK( 7.0 / 9 + 1.0 / 4 > 1 );
 CHECK( akraBazziExponent( { { 7 , 1.0 / 3 } , { 1 , 0.5 } } ) > 2 );
 Memo T2 = make( 2 );
 CHECK( ratios( T2 , g , 100000000000 , 1000000000000 ).first >
        3 * ratios( T2 , g , 1000 , 10000 ).second );
 Memo Ts = make( std::sqrt( 4.5 ) );
 auto h = [ g ]( double n ) { return( g( n ) * lg( n ) ); };
 const double n3 = 1e3 , n15 = 1e15;
 CHECK( Ts( Int( n15 ) ) / g( n15 ) > 4 * Ts( Int( n3 ) ) / g( n3 ) );
 const double r3 = Ts( Int( n3 ) ) / h( n3 ) ,
              r15 = Ts( Int( n15 ) ) / h( n15 );
 CHECK( ( r15 < r3 ) && ( r15 > r3 / 2 ) );
 }

// 39, 2018-07-25, Complessita: T_k( n ) = k^2 T_k( n / k ) + n^{k / 2}:
// T_2 and T_3 are Theta( n^2 ), T_4 is Theta( n^2 log n ), to be discarded
ADS_TEST( "complexity-39" )
{
 std::vector< Growth > G;
 for( const int k : { 2 , 3 , 4 } ) {
  G.push_back( master( k * k , k , k / 2.0 ) );
  CHECK( settles( k * k , k , [ k ]( double n ) {
   return( std::pow( n , k / 2.0 ) ); } , G.back() ) );
  }
 CHECK( same( G[ 0 ] , 1 , 2 ) );
 CHECK( same( G[ 1 ] , 1 , 2 ) );
 CHECK( same( G[ 2 ] , 1 , 2 , 1 ) );
 CHECK( ( G[ 0 ] == G[ 1 ] ) && ( G[ 1 ] < G[ 2 ] ) );
 }

// 40, 2018-08-21, Complessita: T( n ) = 3 T( n / 3 ) + 4 T( n / 4 ) + 12 T(
// n / 6 ) + n^2, T( n ) = 1 for n <= 6: Theta( n^2 ), c = 12, m = 1
ADS_TEST( "complexity-40" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 6 ? 1.0 : 3 * T( n / 3 ) + 4 * T( n / 4 ) + 12 * T( n / 6 ) +
                         two( double( n ) ) ); } );
 CHECK( upper( T , two , 12 , 1 ) );
 CHECK( lower( T , two , 1 , 7 ) );
 }

// 41, 2019-01-21, Complessita: T( n ) = 4 T( n / 4 ) + 9 T( n / 9 ) + n
// sqrt( n ), T( n ) = 1 for n <= 9: Theta( n sqrt( n ) ), c = 6, m = 1
ADS_TEST( "complexity-41" )
{
 auto g = []( double n ) { return( std::pow( n , 1.5 ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n <= 9 ? 1.0 : 4 * T( n / 4 ) + 9 * T( n / 9 ) + g( double( n ) ) );
  } );
 CHECK( upper( T , g , 6 , 1 ) );
 CHECK( lower( T , g , 1 , 10 ) );
 }

// 42, 2019-02-07, Complessita: T( n ) = T( n / 2 ) + T( n / 4 ) + T( n / 8 )
// + 2 T( n / 16 ) + 1, T( n ) = 1 for n <= 16: O( n ) by T( n ) <= c n - b,
// b = 1 / 4, c = 5 / 4, and Theta( n ) since the fractions sum to 1
ADS_TEST( "complexity-42" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 16 ? 1.0 : T( n / 2 ) + T( n / 4 ) + T( n / 8 ) +
                          2 * T( n / 16 ) + 1 ); } );
 CHECK( holds( 1 , 10000000 , [ & ]( Int n ) {
  return( T( n ) <= 1.25 * double( n ) - 0.25 + 1e-9 ); } ) );
 CHECK( bounded( T , one ) );
 }

// 43, 2019-06-17, Complessita: T( n ) = 2 T( n / 3 ) + 3 T( n / 2 ) + n^2,
// T( n ) = 1 for n < 3: Theta( n^2 ), c = 36, m = 1
ADS_TEST( "complexity-43" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 3 ? 1.0 : 2 * T( n / 3 ) + 3 * T( n / 2 ) + two( double( n ) ) );
  } );
 CHECK( upper( T , two , 36 , 1 ) );
 CHECK( lower( T , two , 1 , 3 ) );
 }

// 44, 2019-07-18, Complessita: T( n ) = T( n / 2 ) + 2 T( n / 4 ) + 3 T( n /
// 6 ) + n^2, T( n ) = 1 for n < 6: Theta( n^2 ), c = 24 / 13, m = 1
ADS_TEST( "complexity-44" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 6 ? 1.0 : T( n / 2 ) + 2 * T( n / 4 ) + 3 * T( n / 6 ) +
                        two( double( n ) ) ); } );
 CHECK( upper( T , two , 24.0 / 13 , 1 ) );
 CHECK( lower( T , two , 1 , 6 ) );
 }

// 45, 2020-06-12, Complessita: T( n ) = T( n / 2 ) + 2 T( n / ( 2 sqrt( 2 )
// ) ) + 7 T( n / 4 ) + n^2, n >= 4: Theta( n^2 ), c = 16, m = 1, with the
// base T( n ) = 1 of the statement and with the T( n ) = n of the solution
ADS_TEST( "complexity-45" )
{
 const double d = 2 * std::sqrt( 2.0 );
 for( const bool linearBase : { false , true } ) {
  Memo T( [ d , linearBase ]( Memo & T , Int n ) {
   if( n < 4 )
    return( linearBase ? double( std::max< Int >( n , 1 ) ) : 1.0 );
   return( T( n / 2 ) + 2 * T( down( n , d ) ) + 7 * T( n / 4 ) +
           two( double( n ) ) ); } );
  CHECK( upper( T , two , 16 , 1 ) );
  CHECK( lower( T , two , 1 , 4 ) );
  }
 }

// 46, 2020-07-03, Complessita: T( n ) = 3 T( n / 3 ) + 4 T( n / 4 ) + 5 T( n
// / 5 ) + 6 T( n / 6 ) + n^2, T( n ) = 1 for n < 6: Theta( n^2 ), c = 20
ADS_TEST( "complexity-46" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 6 ? 1.0 : 3 * T( n / 3 ) + 4 * T( n / 4 ) + 5 * T( n / 5 ) +
                        6 * T( n / 6 ) + two( double( n ) ) ); } );
 CHECK( upper( T , two , 20 , 1 ) );
 CHECK( lower( T , two , 1 , 6 ) );
 }

// 47, 2020-07-24, Complessita: T( n ) = a T( n / 2 ) + n^{a - 1}: Theta(
// log n ) for a = 1, Theta( n log n ) for a = 2, Theta( n^{a - 1} ) for
// a >= 3; the extreme parameters of the master theorem: a = 1 with f
// constant (binary search, log n), with f = 1 / log n (log log n) and with
// f = 1 / log^2 n (constant); a < 1 or b <= 1 are contract violations
ADS_TEST( "complexity-47" )
{
 CHECK( same( master( 1 , 2 , 0 ) , 1 , 0 , 1 ) );
 CHECK( same( master( 1 , 2 , 0 , -1 ) , 1 , 0 , 0 , 1 ) );
 CHECK( same( master( 1 , 2 , 0 , -2 ) , 1 , 0 ) );
 CHECK( same( master( 1 , 2 , -1 ) , 1 , 0 ) );   // f falling: Theta( 1 )
 {                                           // T( 2^h ) = 1 + sum 1 / i
  double T = 1 , H = 0;
  for( int h = 1 ; h <= 1000000 ; ++h ) {
   T += 1.0 / h;
   H += 1.0 / ( double( h ) * h );
   }
  CHECK( std::abs( T - 1 - std::log( 1e6 ) - 0.5772156649 ) < 1e-5 );
  CHECK( H < 2 );                                       // bounded
  }
 for( const double b : { 1.0 , 0.5 , 0.0 } )
  CHECK_THROWS( (void) master( 2 , b , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 0.5 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) master( 0 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) extendedMaster( 0.5 , 2 , 1 ) , contract_violation );
 CHECK_THROWS( (void) extendedMaster( 2 , 1 , 1 ) , contract_violation );
 CHECK( same( master( 2 , 2 , 1 ) , 1 , 1 , 1 ) );
 CHECK( same( master( 3 , 2 , 2 ) , 1 , 2 ) );
 for( int a = 1 ; a <= 10 ; ++a ) {
  const Growth G = master( a , 2 , a - 1 );
  if( a >= 3 )
   CHECK( same( G , 1 , a - 1 ) );
  CHECK( settles( a , 2 , [ a ]( double n ) {
   return( std::pow( n , a - 1 ) ); } , G ) );
  Memo T( [ a ]( Memo & T , Int n ) {
   return( n < 2 ? 1.0 : a * T( n / 2 ) + std::pow( double( n ) , a - 1 ) );
   } );
  CHECK( bounded( T , [ & ]( double n ) { return( G.at( n ) ); } ) );
  }
 }

// 48, 2020-09-04, Complessita: T( n ) = 2 T( n / a ) + n^{ceil( a / 2 )}:
// Theta( n log n ) for a = 2, Theta( n^{ceil( a / 2 )} ) for a >= 3
ADS_TEST( "complexity-48" )
{
 CHECK( same( master( 2 , 2 , 1 ) , 1 , 1 , 1 ) );
 CHECK( same( master( 2 , 3 , 2 ) , 1 , 2 ) );
 CHECK( same( master( 2 , 4 , 2 ) , 1 , 2 ) );
 CHECK( same( master( 2 , 5 , 3 ) , 1 , 3 ) );
 for( int a = 2 ; a <= 12 ; ++a ) {
  const int e = ( a + 1 ) / 2;
  const Growth G = master( 2 , a , e );
  if( a >= 3 )
   CHECK( same( G , 1 , e ) );
  CHECK( settles( 2 , a , [ e ]( double n ) { return( std::pow( n , e ) ); } ,
                  G ) );
  Memo T( [ a , e ]( Memo & T , Int n ) {
   return( n < 2 ? 1.0 : 2 * T( n / a ) + std::pow( double( n ) , e ) ); } );
  CHECK( bounded( T , [ & ]( double n ) { return( G.at( n ) ); } ) );
  }
 }

// 49, 2021-01-18, Complessita: T( n ) = 7 T( n / 2 ) + 4 T( n / 4 ) + n^3
// log n, T( n ) = 1 for n < 8: Theta( n^3 log n ), c = 16, m = 2
ADS_TEST( "complexity-49" )
{
 auto g = []( double n ) { return( n * n * n * lg( n ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n < 8 ? 1.0 : 7 * T( n / 2 ) + 4 * T( n / 4 ) + g( double( n ) ) );
  } );
 CHECK( worst( T , g , 2 , 7 ) == 1.0 / 8 );
 CHECK( upper( T , g , 16 , 2 ) );
 CHECK( lower( T , g , 1 , 8 ) );
 }

// 50, 2021-02-08, Complessita: T( n ) = 3 T( n / 2 ) + 3 T( n / 4 ) + n^2
// log n, T( n ) = 1 for n < 8: Theta( n^2 log n ), c = 16, m = 2
ADS_TEST( "complexity-50" )
{
 auto g = []( double n ) { return( n * n * lg( n ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n < 8 ? 1.0 : 3 * T( n / 2 ) + 3 * T( n / 4 ) + g( double( n ) ) );
  } );
 CHECK( worst( T , g , 2 , 7 ) == 1.0 / 4 );
 CHECK( upper( T , g , 16 , 2 ) );
 CHECK( lower( T , g , 1 , 8 ) );
 }

// 51, 2021-02-19, Complessita: T( n ) = 6 T( n / 2 ) + T( n / ( 2 cbrt( 2 )
// ) ) + n^3 - n^2, T( n ) = 1 for n < 8: Theta( n^3 ), c = 16 / 3, m = 1
ADS_TEST( "complexity-51" )
{
 const double d = 2 * std::cbrt( 2.0 );
 Memo T( [ d ]( Memo & T , Int n ) {
  return( n < 8 ? 1.0 : 6 * T( n / 2 ) + T( down( n , d ) ) +
                        three( double( n ) ) - two( double( n ) ) ); } );
 CHECK( upper( T , three , 16.0 / 3 , 1 ) );
 CHECK( lower( T , three , 7.0 / 8 , 8 ) );
 }

// 52.1, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n: Theta( n^2 ) by
// the basic master theorem, first case
ADS_TEST( "complexity-52-1" )
{
 const auto G = extendedMaster( 16 , 4 , 1 );
 CHECK( G && same( *G , 1 , 2 ) );
 CHECK( settles( 16 , 4 , one , *G ) );
 }

// 52.2, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n log n: Theta(
// n^2 ) by the extended master theorem, first case
ADS_TEST( "complexity-52-2" )
{
 const auto G = extendedMaster( 16 , 4 , 1 , 1 );
 CHECK( G && same( *G , 1 , 2 ) );
 CHECK( settles( 16 , 4 , []( double n ) { return( n * lg( n ) ); } , *G ) );
 }

// 52.3, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n^2: Theta( n^2
// log n ) by the basic master theorem, second case
ADS_TEST( "complexity-52-3" )
{
 const auto G = extendedMaster( 16 , 4 , 2 );
 CHECK( G && same( *G , 1 , 2 , 1 ) );
 CHECK( settles( 16 , 4 , two , *G ) );
 }

// 52.4, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n^2 + log n:
// Theta( n^2 log n ) by the extended master theorem, f = Theta( n^2 )
ADS_TEST( "complexity-52-4" )
{
 const auto G = extendedMaster( 16 , 4 , 2 );
 CHECK( G && same( *G , 1 , 2 , 1 ) );
 CHECK( settles( 16 , 4 , []( double n ) { return( n * n + lg( n ) ); } ,
                 *G ) );
 }

// 52.5, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n^2 / log n: no
// case of the extended master theorem applies; Teorema 2.4 gives Theta(
// n^2 log log n )
ADS_TEST( "complexity-52-5" )
{
 CHECK( ! extendedMaster( 16 , 4 , 2 , -1 ) );
 const Growth G = master( 16 , 4 , 2 , -1 );
 CHECK( same( G , 1 , 2 , 0 , 1 ) );
 CHECK( settles( 16 , 4 , []( double n ) { return( n * n / lg( n ) ); } ,
                 G ) );
 }

// 52.6, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n^2 log n: no case
// of the extended master theorem applies; Teorema 2.4 gives Theta( n^2
// log^2 n )
ADS_TEST( "complexity-52-6" )
{
 CHECK( ! extendedMaster( 16 , 4 , 2 , 1 ) );
 const Growth G = master( 16 , 4 , 2 , 1 );
 CHECK( same( G , 1 , 2 , 2 ) );
 CHECK( settles( 16 , 4 , []( double n ) { return( n * n * lg( n ) ); } ,
                 G ) );
 }

// 52.7, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n^3: Theta( n^3 )
// by the basic master theorem, third case
ADS_TEST( "complexity-52-7" )
{
 const auto G = extendedMaster( 16 , 4 , 3 );
 CHECK( G && same( *G , 1 , 3 ) );
 CHECK( settles( 16 , 4 , three , *G ) );
 }

// 52.8, 2021-06-16, Complessita: T( n ) = 16 T( n / 4 ) + n^3 log n: Theta(
// n^3 log n ) by the extended master theorem, third case, with 16 f( n /
// 4 ) <= f( n ) / 4
ADS_TEST( "complexity-52-8" )
{
 auto f = []( double n ) { return( n * n * n * lg( n ) ); };
 const auto G = extendedMaster( 16 , 4 , 3 , 1 );
 CHECK( G && same( *G , 1 , 3 , 1 ) );
 CHECK( settles( 16 , 4 , f , *G ) );
 for( double n = 4 ; n < 1e100 ; n *= 4 )
  CHECK( 16 * f( n / 4 ) <= f( n ) / 4 );
 }

// 53, 2021-07-26, Complessita: T( n ) = 3 T( n / 2 ) + 3 T( n / 4 ) + n^2,
// T( n ) = 1 for n < 4: Theta( n^2 ), c = 16, m = 1
ADS_TEST( "complexity-53" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 4 ? 1.0 : 3 * T( n / 2 ) + 3 * T( n / 4 ) + two( double( n ) ) );
  } );
 CHECK( upper( T , two , 16 , 1 ) );
 CHECK( lower( T , two , 1 , 4 ) );
 }

// 54, 2021-08-31, Complessita: T( n ) = 3 T( n / 2 ) + 8 T( n / 6 ) + n^2,
// T( n ) = 1 for n < 6: Theta( n^2 ), c = 36, m = 1
ADS_TEST( "complexity-54" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 6 ? 1.0 : 3 * T( n / 2 ) + 8 * T( n / 6 ) + two( double( n ) ) );
  } );
 CHECK( upper( T , two , 36 , 1 ) );
 CHECK( lower( T , two , 1 , 6 ) );
 }

// 55, 2022-01-17, Complessita: T( n ) = 2 T( n / 2 ) + 31 T( n / 8 ) + n^2
// - n, T( n ) = 1 for n < 8: Theta( n^2 ), c = 64, m = 1
ADS_TEST( "complexity-55" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 8 ? 1.0 : 2 * T( n / 2 ) + 31 * T( n / 8 ) + two( double( n ) )
                        - double( n ) ); } );
 CHECK( upper( T , two , 64 , 1 ) );
 CHECK( lower( T , two , 1 , 8 ) );
 }

// 56, 2022-02-07, Complessita: T( n ) = 12 T( n / 8 ) + 18 T( n / 27 ) + n
// cbrt( n ), T( n ) = 1 for n < 27: Theta( n cbrt( n ) ), c = 36, m = 1
ADS_TEST( "complexity-56" )
{
 auto g = []( double n ) { return( n * std::cbrt( n ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n < 27 ? 1.0 : 12 * T( n / 8 ) + 18 * T( n / 27 ) +
                         g( double( n ) ) ); } );
 CHECK( upper( T , g , 36 , 1 ) );
 CHECK( lower( T , g , 1 , 27 ) );
 }

// 57, 2022-06-21, Complessita: T( n ) = T( n / sqrt( 2 ) ) + T( n / sqrt(
// 4 ) ) + T( n / sqrt( 5 ) ) + n^2, T( n ) = 1 for n <= 2: Theta( n^2 ), c
// = 20, m = 1
ADS_TEST( "complexity-57" )
{
 const double r2 = std::sqrt( 2.0 ) , r5 = std::sqrt( 5.0 );
 Memo T( [ r2 , r5 ]( Memo & T , Int n ) {
  return( n <= 2 ? 1.0 : T( down( n , r2 ) ) + T( n / 2 ) +
                         T( down( n , r5 ) ) + two( double( n ) ) ); } );
 CHECK( T( 3 ) == 12 );
 CHECK( upper( T , two , 20 , 1 ) );
 CHECK( lower( T , two , 1 , 3 ) );
 }

// 58, 2022-07-13, Complessita: T( n ) = T( 2 n / 9 ) + T( 5 n / 18 ) + n:
// Theta( n ), c = 2, m = 1, the small n reaching T( 0 ) checked one by one
ADS_TEST( "complexity-58" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( 2 * n / 9 ) + T( 5 * n / 18 ) + double( n ) ); } );
 CHECK( T( 2 ) == 4 );
 CHECK( T( 3 ) == 5 );
 CHECK( T( 4 ) == 6 );
 CHECK( upper( T , one , 2 , 1 ) );
 CHECK( lower( T , one , 1 , 1 ) );
 }

// 59, 2023-01-17, Complessita: R( A , i , j ) costs T( n ) = 2 T( 3 n / 4 )
// + 1: Theta( n^{log_{4/3} 2} ), about n^{2.41}; the first recursive n
// = 4 by hand, and the contracts of the fragment
ADS_TEST( "complexity-59" )
{
 Memo C( []( Memo & C , Int n ) {
  return( n < 4 ? 1.0 : C( 3 * ( n / 4 ) ) + C( n - n / 4 ) + 1 ); } );
 std::mt19937 g( 1 );
 Ints A( 600 );
 for( Int & a : A )
  a = Int( g() % 100 );
 for( Int n = 1 ; n <= 600 ; n += ( n < 40 ? 1 : 53 ) ) {
  Int calls = 0;
  threeQuarters( A , 0 , n - 1 , calls );
  CHECK( double( calls ) == C( n ) );
  }
 const Growth G = master( 2 , 4.0 / 3 , 0 );
 CHECK( same( G , 1 , std::log( 2.0 ) / std::log( 4.0 / 3 ) ) );
 CHECK( ( 2.40 < G.p ) && ( G.p < 2.41 ) );
 CHECK( settles( 2 , 4.0 / 3 , []( double ) { return( 1.0 ); } , G ) );
 CHECK( bounded( C , [ & ]( double n ) { return( G.at( n ) ); } ,
                 10000000 ) );
 // n = 4: d = 1, the calls on 3 and 3 values; a range out of A refused
 Int calls = 0;
 CHECK( threeQuarters( A , 0 , 3 , calls ) ==
        ( A[ 0 ] + A[ 1 ] + A[ 2 ] ) - ( A[ 1 ] + A[ 2 ] + A[ 3 ] ) );
 CHECK( calls == 3 );
 CHECK_THROWS( threeQuarters( A , 5 , 4 , calls ) , contract_violation );
 CHECK_THROWS( threeQuarters( A , 0 , 600 , calls ) , contract_violation );
 }

// 60, 2023-02-09, Complessita: R( A , start , end ) costs T( n ) = 2 T( n /
// 2 ) + 1, Theta( n ), on distinct values and T( n ) = 2 T( n / 2 ) + n,
// Theta( n log n ), on equal values; a palindrome of 3 values by hand, and
// the contracts of the fragment
ADS_TEST( "complexity-60" )
{
 {
  const Ints P{ 1 , 2 , 1 };                // the scan meets in the middle
  Int steps = 0;
  CHECK( equalEnds( P , 0 , 2 , steps ) == 6 + 3 + 1 );
  CHECK( steps == 1 + 2 + 3 + 1 );
  CHECK_THROWS( equalEnds( P , 2 , 1 , steps ) , contract_violation );
  CHECK_THROWS( equalEnds( P , 0 , 3 , steps ) , contract_violation );
  }
 for( int h = 0 ; h <= 18 ; ++h ) {
  const Int n = Int( 1 ) << h;
  Ints D( static_cast< std::size_t >( n ) ) , E( std::size_t( n ) , 1 );
  std::iota( D.begin() , D.end() , Int( 1 ) );
  Int best = 0 , worst = 0;
  CHECK( equalEnds( D , 0 , n - 1 , best ) == n * ( n + 1 ) / 2 );
  equalEnds( E , 0 , n - 1 , worst );
  CHECK( best == 2 * n - 1 );
  CHECK( worst == 2 * n - 1 + Int( h ) * n / 2 );
  }
 CHECK( same( master( 2 , 2 , 0 ) , 1 , 1 ) );
 CHECK( same( master( 2 , 2 , 1 ) , 1 , 1 , 1 ) );
 // n not a power of 2, random values: between the two cases
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Int n = 1 + Int( g() % 2000 );
  Ints A( static_cast< std::size_t >( n ) );
  for( Int & a : A )
   a = Int( g() % 3 );
  Int steps = 0;
  equalEnds( A , 0 , n - 1 , steps );
  CHECK( ( 2 * n - 1 <= steps ) &&
         ( double( steps ) <= 2.0 * double( n ) +
                              double( n ) * ( lg( double( n ) ) + 1 ) ) );
  }
 }

// 61, 2023-06-20, Complessita: R( A , x1 , y1 , x2 , y2 ) costs T( n ) = 2
// T( n / 2 ) + n: Theta( n log n ), from the single cell; a square out of
// A is a contract violation
ADS_TEST( "complexity-61" )
{
 Memo C( []( Memo & C , Int n ) {
  return( n <= 1 ? 1.0 : 2 * C( n / 2 ) + 2.0 * double( n ) - 1 ); } );
 for( int h = 0 ; h <= 11 ; ++h ) {
  const Int n = Int( 1 ) << h;
  const Matrix A( std::size_t( n ) , Ints( std::size_t( n ) , 1 ) );
  Int steps = 0;
  border( A , 0 , 0 , n - 1 , n - 1 , steps );
  CHECK( double( steps ) == C( n ) );
  if( h == 1 ) {
   Int none = 0;
   CHECK_THROWS( border( A , 0 , 0 , 2 , 2 , none ) , contract_violation );
   CHECK_THROWS( border( A , 1 , 1 , 0 , 0 , none ) , contract_violation );
   }
  }
 const Growth G = master( 2 , 2 , 1 );
 CHECK( same( G , 1 , 1 , 1 ) );
 CHECK( settles( 2 , 2 , []( double n ) { return( 2 * n - 1 ); } , G ) );
 }

// 62, 2023-07-11, Complessita: T( n ) = 9 T( n / 3 ) + 36 T( n / 6 ) + n^3,
// T( n ) = 1 for n < 6: Theta( n^3 ), c = 2, m = 1
ADS_TEST( "complexity-62" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 6 ? 1.0 : 9 * T( n / 3 ) + 36 * T( n / 6 ) +
                        three( double( n ) ) ); } );
 CHECK( upper( T , three , 2 , 1 ) );
 CHECK( lower( T , three , 1 , 6 ) );
 }

// 63, 2023-08-23, Complessita: R( A , x1 , y1 , x2 , y2 ) costs T( n ) = 2
// T( n / 2 ) + n^2: Theta( n^2 ), exactly 2 n^2 - n, from the single cell;
// a square out of A is a contract violation
ADS_TEST( "complexity-63" )
{
 for( int h = 0 ; h <= 11 ; ++h ) {
  const Int n = Int( 1 ) << h;
  const Matrix A( std::size_t( n ) , Ints( std::size_t( n ) , 1 ) );
  Int steps = 0;
  CHECK( square( A , 0 , 0 , n - 1 , n - 1 , steps ) == 2 * n * n - n );
  CHECK( steps == 2 * n * n - n );
  if( h == 1 ) {
   Int none = 0;
   CHECK_THROWS( square( A , 0 , 0 , 2 , 2 , none ) , contract_violation );
   CHECK_THROWS( square( A , -1 , 0 , 1 , 1 , none ) , contract_violation );
   }
  }
 const Growth G = master( 2 , 2 , 2 );
 CHECK( same( G , 1 , 2 ) );
 CHECK( settles( 2 , 2 , two , G ) );
 }

// 64, 2024-01-09, Complessita: frec( A , si , sj , ei , ej ) costs T( n ) =
// 4 T( n / 2 ) + O( n ), the four quadrants having side n / 2: Theta( n^2
// ); the single cell and the square of side 2 by hand, and a square out of
// A is a contract violation
ADS_TEST( "complexity-64" )
{
 {
  const Matrix A( 2 , Ints( 2 , 1 ) );
  Int steps = 0;
  CHECK( staircase( A , 1 , 1 , 1 , 1 , steps ) == 0 );
  CHECK( steps == 1 );
  steps = 0;                                // 1 call, 1 step left, 4 cells
  CHECK( staircase( A , 0 , 0 , 1 , 1 , steps ) == 1 );
  CHECK( steps == 6 );
  CHECK_THROWS( staircase( A , 0 , 0 , 2 , 2 , steps ) , contract_violation );
  CHECK_THROWS( staircase( A , 1 , 0 , 0 , 1 , steps ) , contract_violation );
  }
 Memo lo( []( Memo & L , Int n ) {
  return( n <= 1 ? 1.0 : 4 * L( n / 2 ) + double( n ) ); } );
 Memo hi( []( Memo & H , Int n ) {
  return( n <= 1 ? 1.0 : 4 * H( n / 2 ) + 2.0 * double( n ) - 1 ); } );
 std::mt19937 g( 1 );
 for( int h = 0 ; h <= 9 ; ++h ) {
  const Int n = Int( 1 ) << h;
  for( int t = 0 ; t < 3 ; ++t ) {
   Matrix A( static_cast< std::size_t >( n ) ,
             Ints( static_cast< std::size_t >( n ) ) );
   for( auto & row : A )
    for( Int & a : row )
     a = t == 0 ? 1 : Int( g() % 100 );
   Int steps = 0;
   staircase( A , 0 , 0 , n - 1 , n - 1 , steps );
   CHECK( ( lo( n ) <= double( steps ) ) && ( double( steps ) <= hi( n ) ) );
   }
  }
 const Growth G = master( 4 , 2 , 1 );
 CHECK( same( G , 1 , 2 ) );
 CHECK( settles( 4 , 2 , one , G ) );
 CHECK( settles( 4 , 2 , []( double n ) { return( 2 * n - 1 ); } , G ) );
 }

// 65, 2024-02-01, Complessita: fr( A1 , A2 , ... ) costs T( n ) = 4 T( n /
// 2 ) + n log n, the merge sorts dominating the shuffles: Theta( n^2 ) by
// the extended master theorem, first case; two subarrays of different
// length, or of a length that is not a power of 2, are contract violations
ADS_TEST( "complexity-65" )
{
 {
  Ints X( 8 , 0 ) , Y( 8 , 0 );
  std::mt19937 h( 1 );
  Int none = 0;
  CHECK_THROWS( shuffleSort( X , Y , 0 , 3 , 0 , 1 , h , none ) ,
                contract_violation );
  CHECK_THROWS( shuffleSort( X , Y , 0 , 2 , 0 , 2 , h , none ) ,
                contract_violation );
  CHECK( shuffleSort( X , Y , 5 , 5 , 2 , 2 , h , none ) == -1 );   // ties
  CHECK( none == 1 );
  }
 Memo C( []( Memo & C , Int n ) {
  return( n <= 1 ? 1.0 : 4 * C( n / 2 ) + 2.0 * double( n ) +
                         2.0 * double( n ) * lg( double( n ) ) ); } );
 std::mt19937 g( 1 );
 for( int h = 0 ; h <= 9 ; ++h ) {
  const Int n = Int( 1 ) << h;
  Ints A1( static_cast< std::size_t >( n ) ) ,
       A2( static_cast< std::size_t >( n ) );
  for( Int & a : A1 ) a = Int( g() % 1000 );
  for( Int & a : A2 ) a = Int( g() % 1000 );
  Int steps = 0;
  const Int s = shuffleSort( A1 , A2 , 0 , n - 1 , 0 , n - 1 , g , steps );
  CHECK( double( steps ) == C( n ) );
  CHECK( ( std::abs( s ) <= n * n ) && ( ( s - n * n ) % 2 == 0 ) );
  }
 const auto G = extendedMaster( 4 , 2 , 1 , 1 );
 CHECK( G && same( *G , 1 , 2 ) );
 CHECK( settles( 4 , 2 , []( double n ) { return( 2 * n + 2 * n * lg( n ) ); }
                 , *G ) );
 }

// 66, 2024-06-14, Complessita: super( n ) costs T( n ) = T( n / 2 ) + T( n /
// 4 ) + n / 5: Theta( n ), c = 1, m = 1; n = 4 with no iteration, and n <
// 0 is a contract violation
ADS_TEST( "complexity-66" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 4 ? 1.0 : T( n / 2 ) + T( n / 4 ) + double( n / 5 ) ); } );
 std::mt19937_64 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Int n = t < 100 ? t : Int( g() % 500000 );
  Int steps = 0;
  superRec( n , steps );
  CHECK( double( steps ) == T( n ) );
  }
 CHECK( upper( T , one , 1 , 1 ) );
 CHECK( lower( T , one , 0.2 , 1 ) );
 // n = 4: no iteration (floor( 4 / 5 ) = 0), the two base calls
 Int steps = 0;
 superRec( 4 , steps );
 CHECK( steps == 2 );
 CHECK_THROWS( superRec( -1 , steps ) , contract_violation );
 }

// 67, 2024-07-02, Complessita: T( n ) = 7 T( n / 4 ) + n sqrt( n ), T( n ) =
// 1 for n < 4: Theta( n sqrt( n ) ), c = 8 (and the master theorem)
ADS_TEST( "complexity-67" )
{
 auto g = []( double n ) { return( std::pow( n , 1.5 ) ); };
 Memo T( [ g ]( Memo & T , Int n ) {
  return( n < 4 ? 1.0 : 7 * T( n / 4 ) + g( double( n ) ) ); } );
 CHECK( upper( T , g , 8 , 1 ) );
 CHECK( lower( T , g , 1 , 4 ) );
 const Growth G = master( 7 , 4 , 1.5 );
 CHECK( same( G , 1 , 1.5 ) );
 CHECK( settles( 7 , 4 , g , G ) );
 }

// 68, 2024-08-26, Complessita: A = 5 T( n / 2 ) + n is Theta( n^{log_2 5}
// ), B = 2 T( n - 2 ) + 1 exponential, C = 9 T( n / 3 ) + n^2 is Theta( n^2
// log n ): C is the best. The solution writes Theta( 2^n ) for B, which
// is Theta( 2^{n / 2} ): O( 2^n ) but not Omega( 2^n )
ADS_TEST( "complexity-68" )
{
 const Growth A = master( 5 , 2 , 1 ) , B = linear( 2 , 0 ) ,
              C = master( 9 , 3 , 2 );
 CHECK( same( A , 1 , std::log2( 5.0 ) ) );
 CHECK( same( B , 2 , 0 , 0 , 0 , true ) );
 CHECK( same( C , 1 , 2 , 1 ) );
 CHECK( ( C < A ) && ( C < B ) );
 CHECK( settles( 5 , 2 , one , A ) );
 CHECK( settles( 9 , 3 , two , C ) );
 std::vector< double > T( 1001 , 1 );
 for( std::size_t n = 2 ; n <= 1000 ; ++n )
  T[ n ] = 2 * T[ n - 2 ] + 1;
 for( std::size_t n = 0 ; n <= 1000 ; ++n )
  CHECK( T[ n ] == std::ldexp( 1.0 , int( n / 2 ) + 1 ) - 1 );
 CHECK( T[ 1000 ] / std::ldexp( 1.0 , 1000 ) < 1e-100 );
 }

// 69, 2025-01-14, Complessita: T( n ) = 2 T( n / a ) + n^{a - 1}: Theta( n
// log n ) for a = 2, Theta( n^{a - 1} ) for a > 2
ADS_TEST( "complexity-69" )
{
 CHECK( same( master( 2 , 2 , 1 ) , 1 , 1 , 1 ) );
 CHECK( same( master( 2 , 3 , 2 ) , 1 , 2 ) );
 for( int a = 2 ; a <= 12 ; ++a ) {
  const Growth G = master( 2 , a , a - 1 );
  if( a > 2 )
   CHECK( same( G , 1 , a - 1 ) );
  CHECK( settles( 2 , a , [ a ]( double n ) {
   return( std::pow( n , a - 1 ) ); } , G ) );
  Memo T( [ a ]( Memo & T , Int n ) {
   return( n < 2 ? 1.0 : 2 * T( n / a ) + std::pow( double( n ) , a - 1 ) );
   } );
  CHECK( bounded( T , [ & ]( double n ) { return( G.at( n ) ); } ) );
  }
 }

// 70, 2025-02-03, Complessita: T( n ) = 2 a T( n / a ) + n^a: Theta( n^2
// log n ) for a = 2, Theta( n^a ) for a >= 3
ADS_TEST( "complexity-70" )
{
 CHECK( same( master( 4 , 2 , 2 ) , 1 , 2 , 1 ) );
 CHECK( same( master( 6 , 3 , 3 ) , 1 , 3 ) );
 for( int a = 2 ; a <= 12 ; ++a ) {
  const Growth G = master( 2 * a , a , a );
  if( a >= 3 )
   CHECK( same( G , 1 , a ) );
  CHECK( settles( 2 * a , a , [ a ]( double n ) {
   return( std::pow( n , a ) ); } , G ) );
  Memo T( [ a ]( Memo & T , Int n ) {
   return( n < 2 ? 1.0 : 2 * a * T( n / a ) + std::pow( double( n ) , a ) );
   } );
  CHECK( bounded( T , [ & ]( double n ) { return( G.at( n ) ); } ) );
  }
 }

// 71, 2025-06-13, Complessita: T( n ) = T( delta n ) + n^{delta + 1}, 0 <
// delta < 1: Theta( n^{delta + 1} ) by the third case
ADS_TEST( "complexity-71" )
{
 for( const double d : { 0.25 , 0.5 , 0.75 , 0.9 } ) {
  const Growth G = master( 1 , 1 / d , d + 1 );
  CHECK( same( G , 1 , d + 1 ) );
  auto f = [ d ]( double n ) { return( std::pow( n , d + 1 ) ); };
  CHECK( settles( 1 , 1 / d , f , G ) );
  Memo T( [ d , f ]( Memo & T , Int n ) {
   return( n <= 1 ? 1.0 : T( Int( d * double( n ) ) ) + f( double( n ) ) );
   } );
  CHECK( bounded( T , f ) );
  // T( n ) <= f( n ) / ( 1 - d^{d + 1} ) + 1
  CHECK( upper( T , f , 1 / ( 1 - std::pow( d , d + 1 ) ) + 1 , 1 ) );
  }
 }

// 72, 2025-07-14, Complessita: T( n ) = T( n / sqrt( 2 ) ) + 2 T( n / sqrt(
// 5 ) ) + n^2: Theta( n^2 ), c = 10, m = 1, with T( 2 ) = 7 reaching T( 0 )
ADS_TEST( "complexity-72" )
{
 const double r2 = std::sqrt( 2.0 ) , r5 = std::sqrt( 5.0 );
 Memo T( [ r2 , r5 ]( Memo & T , Int n ) {
  return( n <= 1 ? 1.0 : T( down( n , r2 ) ) + 2 * T( down( n , r5 ) ) +
                         two( double( n ) ) ); } );
 CHECK( T( 2 ) == 7 );
 CHECK( upper( T , two , 10 , 1 ) );
 CHECK( lower( T , two , 1 , 1 ) );
 }

// 73, 2025-08-25, Complessita: A = 5 T( n / 2 ) + n is Theta( n^{log_2 5}
// ), B = 3 T( n - 10 ) + 1 exponential, C = 16 T( n / 4 ) + n^2 is Theta(
// n^2 log n ): C is the best. The solution writes Theta( 3^n ) for B,
// which is Theta( 3^{n / 10} ): O( 3^n ) but not Omega( 3^n )
ADS_TEST( "complexity-73" )
{
 const Growth A = master( 5 , 2 , 1 ) , B = linear( 3 , 0 ) ,
              C = master( 16 , 4 , 2 );
 CHECK( same( A , 1 , std::log2( 5.0 ) ) );
 CHECK( same( B , 3 , 0 , 0 , 0 , true ) );
 CHECK( same( C , 1 , 2 , 1 ) );
 CHECK( ( C < A ) && ( C < B ) );
 CHECK( settles( 16 , 4 , two , C ) );
 std::vector< double > T( 1001 , 1 );
 for( std::size_t n = 10 ; n <= 1000 ; ++n )
  T[ n ] = 3 * T[ n - 10 ] + 1;
 for( std::size_t n = 0 ; n <= 1000 ; ++n )
  CHECK( std::abs( T[ n ] / ( ( std::pow( 3.0 , double( n / 10 + 1 ) ) - 1 )
                              / 2 ) - 1 ) < 1e-12 );
 CHECK( T[ 1000 ] / std::pow( 3.0 , 1000 ) < 1e-100 );
 }

// 74, 2026-01-12, Complessita: mystery( A , start , end ) costs T( n ) = 2
// T( n / 2 ) + Theta( log n ): Theta( n ) by the extended master theorem;
// n = 2 by hand, and the contracts of the fragment
ADS_TEST( "complexity-74" )
{
 Memo C( []( Memo & C , Int n ) {
  return( n <= 1 ? 1.0 : C( ( n + 1 ) / 2 ) + C( n / 2 ) +
          double( std::bit_width( std::uint64_t( n - 1 ) ) ) ); } );
 std::mt19937 g( 1 );
 for( int t = 0 ; t < 300 ; ++t ) {
  const Int n = 1 + ( t < 100 ? t : Int( g() % 100000 ) );
  const Ints A( std::size_t( n ) , 1 );
  Int steps = 0;
  const Int sum = halving( A , 0 , n - 1 , steps );
  CHECK( sum == steps );                    // ones: the steps
  CHECK( double( steps ) == C( n ) );
  }
 {                                          // n = 2: one iteration, 2 bases
  const Ints A{ 3 , 4 };
  Int steps = 0;
  CHECK( halving( A , 0 , 1 , steps ) == 4 + 3 + 4 );
  CHECK( steps == 3 );
  CHECK_THROWS( halving( A , 1 , 0 , steps ) , contract_violation );
  CHECK_THROWS( halving( A , 0 , 2 , steps ) , contract_violation );
  }
 const auto G = extendedMaster( 2 , 2 , 0 , 1 );
 CHECK( G && same( *G , 1 , 1 ) );
 CHECK( settles( 2 , 2 , []( double n ) { return( lg( n ) ); } , *G ) );
 CHECK( bounded( C , one ) );
 }

// 75, 2026-02-02, Complessita: mystery( A , start , end ) costs T( n ) = 2
// T( n - 10 ) + 1, T( n ) = 1 for n < 10: exponential, the solution writes
// Theta( 2^n ), and the calls are exactly 2^{floor( n / 10 ) + 1} - 1,
// Theta( 2^{n / 10} ): O( 2^n ) but not Omega( 2^n ); the empty range and
// the ranges of 9 and 10 values, the extremes of linear() (see there) and
// the contracts of the fragment
ADS_TEST( "complexity-75" )
{
 const Ints A( 250 , 1 );
 for( Int n = 0 ; n <= 220 ; ++n ) {
  Int calls = 0;
  const Int q = n / 10 , r = n % 10;
  CHECK( tens( A , 0 , n - 1 , calls ) == ( Int( 1 ) << q ) * r );
  CHECK( calls == ( Int( 1 ) << ( q + 1 ) ) - 1 );
  }
 CHECK( same( linear( 2 , 0 ) , 2 , 0 , 0 , 0 , true ) );
 // the extremes of linear(): a = 1 polynomial, a just above 1 already
 // exponential (T( n ) = 1.5 T( n - 1 ) + 1 is Theta( 1.5^n ), once given
 // as Theta( n )), a < 1 a contract violation
 CHECK( same( linear( 1 , 0 ) , 1 , 1 ) );
 CHECK( same( linear( 1.5 , 0 ) , 1.5 , 0 , 0 , 0 , true ) );
 CHECK( same( linear( 1.01 , 2 ) , 1.01 , 2 , 0 , 0 , true ) );
 {
  double T = 1;
  for( int n = 1 ; n <= 1000 ; ++n )
   T = 1.5 * T + 1;
  CHECK( std::abs( T / std::pow( 1.5 , 1000 ) - 3 ) < 1e-9 );
  }
 CHECK( linear( 1 , 0 ) < linear( 1.5 , 0 ) );
 CHECK_THROWS( (void) linear( 0.99 , 0 ) , contract_violation );
 CHECK_THROWS( (void) linear( 0 , 1 ) , contract_violation );
 {
  Int calls = 0;
  CHECK_THROWS( tens( A , 3 , 1 , calls ) , contract_violation );
  CHECK_THROWS( tens( A , 0 , 250 , calls ) , contract_violation );
  CHECK_THROWS( tens( A , -1 , 5 , calls ) , contract_violation );
  }
 }

// 76, 2026-06-22, Complessita: T( n ) = 2 T( n / 4 ) + 3 T( n / 9 ) + 7 T( n
// / 49 ) + n, T( n ) = 1 for n < 49: Theta( n ), c = 42, m = 1
ADS_TEST( "complexity-76" )
{
 Memo T( []( Memo & T , Int n ) {
  return( n < 49 ? 1.0 : 2 * T( n / 4 ) + 3 * T( n / 9 ) + 7 * T( n / 49 ) +
                         double( n ) ); } );
 CHECK( upper( T , one , 42 , 1 ) );
 CHECK( lower( T , one , 1 , 49 ) );
 }

// 77, 2026-07-20, Complessita: T( n ) = T( n / 2 ) + log n: neither the
// basic nor the extended master theorem applies; Theta( log^2 n ), exactly
// 1 + ( log n + 1 ) log n / 2 on the powers of 2, and the loose O( n ) of
// the substitution with c = 2
ADS_TEST( "complexity-77" )
{
 CHECK( ! extendedMaster( 1 , 2 , 0 , 1 ) );
 const Growth G = master( 1 , 2 , 0 , 1 );
 CHECK( same( G , 1 , 0 , 2 ) );
 double T = 1;
 for( int h = 1 ; h <= 1000 ; ++h ) {
  T += h;                                   // T( 2^h ) = T( 2^{h-1} ) + h
  CHECK( T == 1 + double( h ) * ( h + 1 ) / 2 );
  if( h < 1000 )
   CHECK( T <= 2 * std::ldexp( 1.0 , h ) );
  }
 CHECK( settles( 1 , 2 , []( double n ) { return( lg( n ) ); } , G ) );
 }

// 78, 2026-08-28, Complessita: mystery( A , n ): Theta( n ) in the best
// case, A non-decreasing, and Theta( n^2 ) in the worst one, A decreasing,
// with n ( n - 1 ) / 2 iterations; n = 0 and 1, all values equal (the
// ties stop each run), the extreme values of int64
ADS_TEST( "complexity-78" )
{
 std::mt19937 g( 1 );
 for( Int n = 0 ; n <= 150 ; ++n ) {
  Ints A( static_cast< std::size_t >( n ) );
  for( Int & a : A )
   a = Int( g() % 10 );
  // brute force: the pairs i < j with A[ i + 1 .. j ] all below A[ i ]
  Int pairs = 0;
  for( Int i = 0 ; i < n ; ++i )
   for( Int j = i + 1 ; j < n ; ++j ) {
    bool below = true;
    for( Int k = i + 1 ; k <= j ; ++k )
     below = below && ( A[ std::size_t( k ) ] < A[ std::size_t( i ) ] );
    pairs += below ? 1 : 0;
    }
  CHECK( runs( A ) == pairs );
  std::sort( A.begin() , A.end() );
  CHECK( runs( A ) == 0 );
  std::iota( A.rbegin() , A.rend() , Int( 0 ) );
  CHECK( runs( A ) == n * ( n - 1 ) / 2 );
  std::fill( A.begin() , A.end() , Int( 7 ) );   // ties stop the runs
  CHECK( runs( A ) == 0 );
  }
 // the extreme values of the type, and a run broken by an equal value
 CHECK( runs( { INT64_MAX , INT64_MIN , INT64_MIN } ) == 2 );
 CHECK( runs( { INT64_MIN , INT64_MAX } ) == 0 );
 CHECK( runs( { 5 , 1 , 5 , 1 } ) == 2 );
 CHECK( runs( { 3 , 2 , 1 , 3 , 0 } ) == 2 + 1 + 0 + 1 );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File exam_complexity.cpp --------------------*/
/*--------------------------------------------------------------------------*/
