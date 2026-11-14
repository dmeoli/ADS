/*--------------------------------------------------------------------------*/
/*------------------------ File StringMatching.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StringMatching [see StringMatching.h], with their
 * contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

namespace ADS {

inline std::vector< std::size_t > naiveMatching( std::string_view T ,
                                                 std::string_view P )
{
 std::vector< std::size_t > R;
 for( std::size_t s = 0 ; s + P.size() <= T.size() ; ++s )
  if( T.compare( s , P.size() , P ) == 0 ) R.push_back( s );
 return( R );
 }

inline std::vector< long > failure( std::string_view P )
{
 std::vector< long > F( P.size() + 1 );
 F[ 0 ] = -1;
 long k = -1;
 for( std::size_t q = 0 ; q < P.size() ; ++q ) {
  while( ( k >= 0 ) && ( P[ k ] != P[ q ] ) ) k = F[ k ];
  F[ q + 1 ] = ++k;
  ADS_POST( F[ q + 1 ] <= long( q ) );    // a proper border
  }
 return( F );
 }

namespace detail {

/// the search of KMP, G on a mismatch and F[ m ] after an occurrence
inline std::vector< std::size_t > kmpSearch( std::string_view T ,
                                             std::string_view P ,
                                             const std::vector< long > & G ,
                                             const std::vector< long > & F ,
                                             long * comparisons )
{
 std::vector< std::size_t > R;
 const long m = long( P.size() );
 long q = 0;                            // the characters of P matched
 if( m == 0 ) {
  for( std::size_t i = 0 ; i <= T.size() ; ++i ) R.push_back( i );
  return( R );
  }
 for( std::size_t i = 0 ; i < T.size() ; ++i ) {
  for( ; ; ) {
   if( comparisons ) ++*comparisons;
   if( P[ q ] == T[ i ] ) break;
   q = G[ q ];
   if( q < 0 ) break;
   }
  if( ++q == m ) {
   R.push_back( i + 1 - m );
   q = F[ m ];
   }
  }
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

}  // end( namespace detail )

inline std::vector< std::size_t > kmp( std::string_view T ,
                                       std::string_view P ,
                                       long * comparisons )
{
 const auto F = failure( P );
 return( detail::kmpSearch( T , P , F , F , comparisons ) );
 }

inline std::vector< long > strongFailure( std::string_view P )
{
 const auto F = failure( P );
 const std::size_t m = P.size();
 std::vector< long > G( m + 1 );
 G[ 0 ] = -1;
 for( std::size_t q = 1 ; q <= m ; ++q ) {
  const long k = F[ q ];
  G[ q ] = ( q < m ) && ( k >= 0 ) && ( P[ k ] == P[ q ] ) ? G[ k ] : k;
  ADS_POST( G[ q ] <= F[ q ] );
  }
 return( G );
 }

inline std::vector< std::size_t > kmpStrong( std::string_view T ,
                                             std::string_view P ,
                                             long * comparisons )
{
 return( detail::kmpSearch( T , P , strongFailure( P ) , failure( P ) ,
                            comparisons ) );
 }


namespace detail {

/// the maximal suffix of P under the order of the bytes (or the reverse
/// one if reverse): the place before it, and its period in *period
inline long maximalSuffix( const unsigned char * P , long m , bool reverse ,
                           std::size_t * period )
{
 long ms = -1 , j = 0 , k = 1 , p = 1;
 while( j + k < m ) {
  const unsigned char a = P[ j + k ] , b = P[ ms + k ];
  if( a == b ) {
   if( k != p ) ++k;
   else {
    j += p;
    k = 1;
    }
   }
  else if( ( a < b ) != reverse ) {   // the suffix at j + k is smaller
   j += k;
   k = 1;
   p = j - ms;
   }
  else {                               // a larger suffix starts at j
   ms = j;
   j = ms + 1;
   k = p = 1;
   }
  }
 *period = std::size_t( p );
 return( ms );
 }

/// the Two-Way algorithm: the occurrences of P (m > 0 bytes) in T at or
/// after from, each given to report(), which returns true to stop; the
/// place where it stopped, or npos
template< class F >
std::size_t twoWayScan( const unsigned char * T , std::size_t n ,
                        const unsigned char * P , std::size_t mm ,
                        std::size_t from , F && report )
{
 constexpr std::size_t npos = std::string_view::npos;
 const long m = long( mm );
 if( n < mm ) return( npos );
 const std::size_t last = n - mm;
 // the critical factorization P = u v, u = P[ 0 .. ell ]
 std::size_t p1 , p2;
 const long i1 = maximalSuffix( P , m , false , &p1 );
 const long i2 = maximalSuffix( P , m , true , &p2 );
 const long ell = std::max( i1 , i2 );
 std::size_t period = i1 > i2 ? p1 : p2;
 const bool periodic = std::memcmp( P , P + period ,
                                    std::size_t( ell + 1 ) ) == 0;
 if( ! periodic )
  period = std::size_t( std::max( ell + 1 , m - ell - 1 ) + 1 );
 if( periodic ) {
  long memory = -1;               // P[ 0 .. memory ] known to match
  for( std::size_t j = from ; j <= last ; ) {
   long i = std::max( ell , memory ) + 1;
   while( ( i < m ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) ++i;
   if( i >= m ) {
    i = ell;
    while( ( i > memory ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) --i;
    if( ( i <= memory ) && report( j ) ) return( j );
    j += period;
    memory = m - long( period ) - 1;
    }
   else {
    j += std::size_t( i - ell );
    memory = -1;
    }
   }
  }
 else
  for( std::size_t j = from ; j <= last ; ) {
   long i = ell + 1;
   while( ( i < m ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) ++i;
   if( i >= m ) {
    i = ell;
    while( ( i >= 0 ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) --i;
    if( ( i < 0 ) && report( j ) ) return( j );
    j += period;
    }
   else
    j += std::size_t( i - ell );
   }
 return( npos );
 }

/// all the places 0 , ... , n, the occurrences of the empty pattern
inline std::vector< std::size_t > everyPlace( std::size_t n )
{
 std::vector< std::size_t > R( n + 1 );
 for( std::size_t i = 0 ; i <= n ; ++i ) R[ i ] = i;
 return( R );
 }

/// the bad character of Horspool: the distance from the last place of c
/// in P[ 0 .. m - 2 ] to m - 1, m if none
inline std::array< std::size_t , 256 > badCharacter( std::string_view P )
{
 std::array< std::size_t , 256 > bc;
 bc.fill( P.size() );
 for( std::size_t i = 0 ; i + 1 < P.size() ; ++i )
  bc[ static_cast< unsigned char >( P[ i ] ) ] = P.size() - 1 - i;
 return( bc );
 }

}  // end( namespace detail )

inline std::vector< std::size_t > boyerMoore( std::string_view T ,
                                              std::string_view P ,
                                              long * comparisons )
{
 const long n = long( T.size() ) , m = long( P.size() );
 if( m == 0 ) return( detail::everyPlace( T.size() ) );
 std::vector< std::size_t > R;
 if( m > n ) return( R );
 // suff[ i ]: the longest suffix of P[ 0 .. i ] that is a suffix of P
 std::vector< long > suff( std::size_t( m ) , 0 ) , gs( std::size_t( m ) , m );
 suff[ std::size_t( m - 1 ) ] = m;
 for( long i = m - 2 , g = m - 1 , f = m - 1 ; i >= 0 ; --i )
  if( ( i > g ) && ( suff[ std::size_t( i + m - 1 - f ) ] < i - g ) )
   suff[ std::size_t( i ) ] = suff[ std::size_t( i + m - 1 - f ) ];
  else {
   if( i < g ) g = i;
   f = i;
   while( ( g >= 0 ) && ( P[ std::size_t( g ) ] ==
                          P[ std::size_t( g + m - 1 - f ) ] ) ) --g;
   suff[ std::size_t( i ) ] = f - g;
   }
 // gs[ i ]: the shift of a mismatch at i, to the next occurrence of the
 // suffix matched, or to the longest prefix of P that is its suffix
 for( long i = m - 1 , j = 0 ; i >= -1 ; --i )
  if( ( i == -1 ) || ( suff[ std::size_t( i ) ] == i + 1 ) )
   for( ; j < m - 1 - i ; ++j )
    if( gs[ std::size_t( j ) ] == m ) gs[ std::size_t( j ) ] = m - 1 - i;
 for( long i = 0 ; i + 1 < m ; ++i )
  gs[ std::size_t( m - 1 - suff[ std::size_t( i ) ] ) ] = m - 1 - i;
 const auto bc = detail::badCharacter( P );
 for( long j = 0 ; j <= n - m ; ) {
  long i = m - 1;
  for( ; i >= 0 ; --i ) {
   if( comparisons ) ++*comparisons;
   if( P[ std::size_t( i ) ] != T[ std::size_t( i + j ) ] ) break;
   }
  if( i < 0 ) {
   R.push_back( std::size_t( j ) );
   j += gs[ 0 ];
   }
  else {
   const long b = long( bc[ static_cast< unsigned char >(
                                  T[ std::size_t( i + j ) ] ) ] ) - m + 1 + i;
   j += std::max( gs[ std::size_t( i ) ] , b );
   }
  }
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

inline std::vector< std::size_t > horspool( std::string_view T ,
                                            std::string_view P ,
                                            long * comparisons )
{
 const std::size_t n = T.size() , m = P.size();
 if( m == 0 ) return( detail::everyPlace( n ) );
 std::vector< std::size_t > R;
 if( m > n ) return( R );
 const auto bc = detail::badCharacter( P );
 for( std::size_t j = 0 ; j <= n - m ; ) {
  std::size_t i = m;               // compared from the last byte
  while( i > 0 ) {
   if( comparisons ) ++*comparisons;
   if( P[ i - 1 ] != T[ j + i - 1 ] ) break;
   --i;
   }
  if( i == 0 ) R.push_back( j );
  j += bc[ static_cast< unsigned char >( T[ j + m - 1 ] ) ];
  }
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

inline std::vector< std::size_t > shiftOr( std::string_view T ,
                                           std::string_view P )
{
 const std::size_t n = T.size() , m = P.size();
 ADS_PRE( m <= 64 );
 if( m == 0 ) return( detail::everyPlace( n ) );
 // B[ c ] has the bit i clear if P[ i ] is c; the bit i of D is clear if
 // P[ 0 .. i ] ends at the current byte of T
 std::array< std::uint64_t , 256 > B;
 B.fill( ~std::uint64_t( 0 ) );
 for( std::size_t i = 0 ; i < m ; ++i )
  B[ static_cast< unsigned char >( P[ i ] ) ] &= ~( std::uint64_t( 1 ) << i );
 const std::uint64_t top = std::uint64_t( 1 ) << ( m - 1 );
 std::uint64_t D = ~std::uint64_t( 0 );
 std::vector< std::size_t > R;
 for( std::size_t j = 0 ; j < n ; ++j ) {
  D = ( D << 1 ) | B[ static_cast< unsigned char >( T[ j ] ) ];
  if( ! ( D & top ) ) R.push_back( j + 1 - m );
  }
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

inline std::vector< std::size_t > zFunction( std::string_view S )
{
 const std::size_t n = S.size();
 std::vector< std::size_t > Z( n , 0 );
 if( n == 0 ) return( Z );
 Z[ 0 ] = n;
 // [ l , r ) is the Z-box that reaches farthest: S[ l .. r ) = S[ 0 .. r - l )
 for( std::size_t i = 1 , l = 0 , r = 0 ; i < n ; ++i ) {
  std::size_t k = i < r ? std::min( Z[ i - l ] , r - i ) : 0;
  while( ( i + k < n ) && ( S[ k ] == S[ i + k ] ) ) ++k;
  Z[ i ] = k;
  if( i + k > r ) {
   l = i;
   r = i + k;
   }
  ADS_POST( Z[ i ] <= n - i );
  }
 return( Z );
 }

inline std::vector< std::size_t > zMatching( std::string_view T ,
                                             std::string_view P )
{
 const std::size_t n = T.size() , m = P.size();
 if( m == 0 ) return( detail::everyPlace( n ) );
 const auto Z = zFunction( P );
 std::vector< std::size_t > R;
 // [ l , r ) the box of T that reaches farthest: T[ l .. r ) = P[ 0 .. r - l )
 for( std::size_t i = 0 , l = 0 , r = 0 ; i < n ; ++i ) {
  std::size_t k = i < r ? std::min( Z[ i - l ] , r - i ) : 0;
  while( ( i + k < n ) && ( k < m ) && ( T[ i + k ] == P[ k ] ) ) ++k;
  if( i + k > r ) {
   l = i;
   r = i + k;
   }
  if( k == m ) R.push_back( i );
  }
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

inline std::vector< std::size_t > twoWay( std::string_view T ,
                                          std::string_view P )
{
 if( P.empty() ) return( detail::everyPlace( T.size() ) );
 std::vector< std::size_t > R;
 (void) detail::twoWayScan( reinterpret_cast< const unsigned char * >(
                             T.data() ) , T.size() ,
                            reinterpret_cast< const unsigned char * >(
                             P.data() ) , P.size() , 0 ,
                            [ & ]( std::size_t s ) {
                             R.push_back( s );
                             return( false ); } );
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File StringMatching.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
