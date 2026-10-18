/*--------------------------------------------------------------------------*/
/*-------------------------- File Palindromes.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Palindromes [see Palindromes.h], with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// whether s[ a .. b ) is a palindrome
template< class S > requires std::is_class_v< S >
bool isPalindromeRange( const S & s , std::size_t a , std::size_t b )
{
 for( ; a + 1 < b ; ++a , --b )
  if( !( s[ a ] == s[ b - 1 ] ) )
   return( false );
 return( true );
 }

/// whether the pieces are palindromes and joined give s
template< class S > requires std::is_class_v< S >
bool joinsPalindromes( const std::vector< S > & pieces , const S & s )
{
 std::vector< typename S::value_type > all;
 for( const auto & p : pieces ) {
  if( ! isPalindromeRange( p , 0 , p.size() ) )
   return( false );
  all.insert( all.end() , p.begin() , p.end() );
  }
 return( std::equal( all.begin() , all.end() , s.begin() , s.end() ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
std::pair< std::size_t , std::size_t > longestPalindrome( const S & s )
{
 const std::size_t n = s.size();
 std::size_t bestStart = 0 , bestLength = 0;
 auto keep = [ & ]( std::size_t start , std::size_t length ) {
  if( ( length > bestLength ) ||
      ( ( length == bestLength ) && ( start < bestStart ) ) ) {
   bestStart = start;
   bestLength = length;
   }
  };
 // d1[ i ]: the radius of the longest odd palindrome centred at i, the
 // centre included; [ l , r ) the rightmost palindrome found so far
 std::vector< std::size_t > d1( n ) , d2( n );
 for( std::size_t i = 0 , l = 0 , r = 0 ; i < n ; ++i ) {
  std::size_t k = i < r ? std::min( d1[ l + r - 1 - i ] , r - i ) : 1;
  while( ( i + k < n ) && ( i >= k ) && ( s[ i - k ] == s[ i + k ] ) )
   ++k;
  d1[ i ] = k;
  if( i + k > r ) {
   l = i + 1 - k;
   r = i + k;
   }
  keep( i + 1 - k , 2 * k - 1 );
  }
 // d2[ i ]: the half length of the longest even palindrome whose right
 // half starts at i
 for( std::size_t i = 0 , l = 0 , r = 0 ; i < n ; ++i ) {
  std::size_t k = i < r ? std::min( d2[ l + r - i ] , r - i ) : 0;
  while( ( i + k < n ) && ( i >= k + 1 ) && ( s[ i - k - 1 ] == s[ i + k ] ) )
   ++k;
  d2[ i ] = k;
  if( i + k > r ) {
   l = i - k;
   r = i + k;
   }
  if( k > 0 )
   keep( i - k , 2 * k );
  }
 ADS_POST( detail::isPalindromeRange( s , bestStart , bestStart + bestLength )
           && ( ( bestLength > 0 ) == ( n > 0 ) ) );
 return( std::pair( bestStart , bestLength ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
S longestPalindromicSubstring( const S & s )
{
 const auto [ start , length ] = longestPalindrome( s );
 return( S( s.begin() + start , s.begin() + start + length ) );
 }

/*--------------------------------------------------------------------------*/

inline std::string longestPalindromicSubstring( const std::string & s )
{
 return( longestPalindromicSubstring< std::string >( s ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
S longestPalindromicSubsequence( const S & s )
{
 const std::size_t n = s.size();
 if( n == 0 )
  return( S() );
 // DP[ i ][ j ], the length for s[ i .. j ], for i <= j
 std::vector< std::vector< std::size_t > > DP(
                                n , std::vector< std::size_t >( n , 0 ) );
 for( std::size_t i = n ; i-- > 0 ; ) {
  DP[ i ][ i ] = 1;
  for( std::size_t j = i + 1 ; j < n ; ++j )
   DP[ i ][ j ] = s[ i ] == s[ j ]
                  ? ( i + 1 <= j - 1 ? DP[ i + 1 ][ j - 1 ] : 0 ) + 2
                  : std::max( DP[ i + 1 ][ j ] , DP[ i ][ j - 1 ] );
  }
 std::vector< typename S::value_type > left;
 bool middle = false;
 std::size_t mid = 0;
 for( std::size_t i = 0 , j = n - 1 ; ; ) {
  if( i == j ) {
   middle = true;
   mid = i;
   break;
   }
  if( s[ i ] == s[ j ] ) {
   left.push_back( s[ i ] );
   if( i + 1 == j )
    break;
   ++i;
   --j;
   }
  else if( DP[ i + 1 ][ j ] >= DP[ i ][ j - 1 ] )
   ++i;
  else
   --j;
  }
 std::vector< typename S::value_type > all( left );
 if( middle )
  all.push_back( s[ mid ] );
 all.insert( all.end() , left.rbegin() , left.rend() );
 S r( all.begin() , all.end() );
 ADS_POST( ( r.size() == DP[ 0 ][ n - 1 ] ) &&
           detail::isPalindromeRange( r , 0 , r.size() ) );
 ADS_AUDIT( detail::isSubsequence( r , s ) &&
            ( r.size() == lcs( s , S( s.rbegin() , s.rend() ) ).size() ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

inline std::string longestPalindromicSubsequence( const std::string & s )
{
 return( longestPalindromicSubsequence< std::string >( s ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
std::vector< S > palindromicPartition( const S & s )
{
 const std::size_t n = s.size();
 std::vector< std::size_t > D( n + 1 , n + 1 ) , from( n + 1 , 0 );
 D[ 0 ] = 0;
 // the centres by increasing h = a + b - 1 for the palindrome s[ a .. b ):
 // every palindrome ending before a has a smaller h, so D[ a ] is final
 for( std::size_t h = 0 ; h + 1 < 2 * n ; ++h ) {
  std::size_t a = ( h + 1 ) / 2 , b = h / 2 + 1;        // s[ a .. b )
  if( a == b ) {                                     // an even centre
   if( !( s[ a - 1 ] == s[ a ] ) )
    continue;
   --a;
   ++b;
   }
  for( ; ; ) {
   if( D[ a ] + 1 < D[ b ] ) {
    D[ b ] = D[ a ] + 1;
    from[ b ] = a;
    }
   if( ( a == 0 ) || ( b == n ) || !( s[ a - 1 ] == s[ b ] ) )
    break;
   --a;
   ++b;
   }
  }
 std::vector< S > pieces;
 for( std::size_t j = n ; j > 0 ; j = from[ j ] )
  pieces.push_back( S( s.begin() + from[ j ] , s.begin() + j ) );
 std::reverse( pieces.begin() , pieces.end() );
 ADS_POST( pieces.size() == D[ n ] );
 ADS_AUDIT( detail::joinsPalindromes( pieces , s ) );
 return( pieces );
 }

/*--------------------------------------------------------------------------*/

inline std::vector< std::string > palindromicPartition( const std::string & s )
{
 return( palindromicPartition< std::string >( s ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Palindromes.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
