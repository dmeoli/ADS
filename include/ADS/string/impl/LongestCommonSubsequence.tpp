/*--------------------------------------------------------------------------*/
/*------------------- File LongestCommonSubsequence.tpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LongestCommonSubsequence [see LongestCommonSubsequence.h],
 * with their contracts.
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

/// whether a is a subsequence of b
template< class S >
bool isSubsequence( const S & a , const S & b )
{
 std::size_t i = 0;
 for( std::size_t j = 0 ; ( i < a.size() ) && ( j < b.size() ) ; ++j )
  if( a[ i ] == b[ j ] ) ++i;
 return( i == a.size() );
 }

}  // end( namespace detail )

template< class S >
std::vector< std::vector< int > > lcsTable( const S & P , const S & T )
{
 const std::size_t m = P.size() , n = T.size();
 std::vector< std::vector< int > > DP( m + 1 , std::vector< int >( n + 1 ) );
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ i ][ j ] = P[ i - 1 ] == T[ j - 1 ] ? DP[ i - 1 ][ j - 1 ] + 1 :
                  std::max( DP[ i - 1 ][ j ] , DP[ i ][ j - 1 ] );
 return( DP );
 }

template< class S >
int lcsMemo( const S & P , const S & T , long * solved )
{
 std::vector< std::vector< int > > DP(
                   P.size() + 1 , std::vector< int >( T.size() + 1 , -1 ) );
 auto rec = [ & ]( auto & self , std::size_t i , std::size_t j ) -> int {
  if( ( i == 0 ) || ( j == 0 ) )
   return( 0 );
  if( DP[ i ][ j ] < 0 ) {
   if( solved ) ++*solved;
   DP[ i ][ j ] = P[ i - 1 ] == T[ j - 1 ] ?
                  self( self , i - 1 , j - 1 ) + 1 :
                  std::max( self( self , i - 1 , j ) ,
                            self( self , i , j - 1 ) );
   }
  return( DP[ i ][ j ] );
  };
 return( rec( rec , P.size() , T.size() ) );
 }

template< class S >
S lcs( const S & P , const S & T )
{
 const auto DP = lcsTable( P , T );
 S out;
 for( std::size_t i = P.size() , j = T.size() ; ( i > 0 ) && ( j > 0 ) ; )
  if( P[ i - 1 ] == T[ j - 1 ] ) {
   out.push_back( P[ i - 1 ] );
   --i; --j;
   }
  else if( DP[ i - 1 ][ j ] >= DP[ i ][ j - 1 ] )
   --i;
  else
   --j;
 std::reverse( out.begin() , out.end() );
 ADS_POST( int( out.size() ) == DP[ P.size() ][ T.size() ] );
 ADS_AUDIT( detail::isSubsequence( out , P ) &&
            detail::isSubsequence( out , T ) );
 return( out );
 }

template< class S >
std::vector< std::pair< char , typename S::value_type > > diff(
                                                 const S & P , const S & T )
{
 const auto DP = lcsTable( P , T );
 std::vector< std::pair< char , typename S::value_type > > out;
 std::size_t i = P.size() , j = T.size();
 while( ( i > 0 ) || ( j > 0 ) )
  if( ( i > 0 ) && ( j > 0 ) && ( P[ i - 1 ] == T[ j - 1 ] ) ) {
   out.emplace_back( ' ' , P[ i - 1 ] );
   --i; --j;
   }
  else if( ( j > 0 ) &&
           ( ( i == 0 ) || ( DP[ i ][ j - 1 ] >= DP[ i - 1 ][ j ] ) ) )
   out.emplace_back( '+' , T[ --j ] );
  else
   out.emplace_back( '-' , P[ --i ] );
 std::reverse( out.begin() , out.end() );
 return( out );
 }

template< class S >
std::tuple< std::size_t , std::size_t , std::size_t >
longestCommonSubstring( const S & P , const S & T )
{
 std::vector< std::size_t > prev( T.size() + 1 , 0 ) , DP( T.size() + 1 , 0 );
 std::size_t best = 0 , ei = 0 , ej = 0;
 for( std::size_t i = 1 ; i <= P.size() ; ++i ) {
  std::swap( prev , DP );
  for( std::size_t j = 1 ; j <= T.size() ; ++j ) {
   DP[ j ] = P[ i - 1 ] == T[ j - 1 ] ? prev[ j - 1 ] + 1 : 0;
   if( DP[ j ] > best ) { best = DP[ j ]; ei = i; ej = j; }
   }
  }
 ADS_POST( ( best <= ei ) && ( best <= ej ) );
 ADS_AUDIT( std::equal( P.begin() + ( ei - best ) , P.begin() + ei ,
                        T.begin() + ( ej - best ) ) );
 return( std::tuple( best , ei , ej ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------- End File LongestCommonSubsequence.tpp ------------------*/
/*--------------------------------------------------------------------------*/
