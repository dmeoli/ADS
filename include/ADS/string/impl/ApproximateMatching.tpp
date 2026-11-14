/*--------------------------------------------------------------------------*/
/*---------------------- File ApproximateMatching.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ApproximateMatching [see ApproximateMatching.h], with
 * their contracts.
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

namespace ADS {

template< class S >
Table matchingTable( const S & P , const S & T , bool edit )
{
 const std::size_t m = P.size() , n = T.size();
 Table DP( m + 1 , std::vector< int >( n + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= m ; ++i ) DP[ i ][ 0 ] = int( i );
 for( std::size_t j = 0 ; j <= n ; ++j ) DP[ 0 ][ j ] = edit ? int( j ) : 0;
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ i ][ j ] = std::min( { DP[ i - 1 ][ j - 1 ] +
                              ( P[ i - 1 ] == T[ j - 1 ] ? 0 : 1 ) ,
                              DP[ i - 1 ][ j ] + 1 , DP[ i ][ j - 1 ] + 1 } );
 return( DP );
 }

template< class S >
std::pair< long , int > stringMatching( const S & P , const S & T )
{
 const auto DP = matchingTable( P , T );
 const std::size_t m = P.size();
 if( T.empty() )
  return( std::pair( -1L , int( m ) ) );
 std::size_t pos = 1;
 for( std::size_t j = 2 ; j <= T.size() ; ++j )
  if( DP[ m ][ j ] < DP[ m ][ pos ] )
   pos = j;
 ADS_POST( ( DP[ m ][ pos ] >= 0 ) && ( DP[ m ][ pos ] <= int( m ) ) );
 return( std::pair( long( pos ) - 1 , DP[ m ][ pos ] ) );
 }

template< class S >
std::size_t occurrence( const Table & DP , const S & P , const S & T ,
                        std::size_t j )
{
 ADS_PRE( ( DP.size() == P.size() + 1 ) && ( j <= T.size() ) &&
          ( DP[ 0 ].size() == T.size() + 1 ) );
 std::size_t i = P.size();
 while( i > 0 ) {
  if( ( j > 0 ) && ( DP[ i ][ j ] == DP[ i - 1 ][ j - 1 ] +
                     ( P[ i - 1 ] == T[ j - 1 ] ? 0 : 1 ) ) ) {
   --i; --j;                                 // a match or a substitution
   }
  else if( DP[ i ][ j ] == DP[ i - 1 ][ j ] + 1 )
   --i;                                      // P[ i - 1 ] not in T
  else
   --j;                                      // T[ j - 1 ] not in P
  }
 ADS_POST( j <= T.size() );
 return( j );
 }

template< class S >
int stringMatchingSingleRow( const S & P , const S & T )
{
 const std::size_t m = P.size() , n = T.size();
 std::vector< int > prev( n + 1 ) , DP( n + 1 , 0 );
 for( std::size_t i = 1 ; i <= m ; ++i ) {
  std::swap( prev , DP );
  DP[ 0 ] = int( i );
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ j ] = std::min( { prev[ j - 1 ] + ( P[ i - 1 ] == T[ j - 1 ] ? 0 : 1 ) ,
                         prev[ j ] + 1 , DP[ j - 1 ] + 1 } );
  }
 return( n ? *std::min_element( DP.begin() + 1 , DP.end() ) : int( m ) );
 }

template< class S >
int editDistance( const S & P , const S & T )
{
 if( T.size() > P.size() )
  return( editDistance( T , P ) );
 const std::size_t m = P.size() , n = T.size();
 std::vector< int > prev( n + 1 ) , DP( n + 1 );
 for( std::size_t j = 0 ; j <= n ; ++j ) DP[ j ] = int( j );
 for( std::size_t i = 1 ; i <= m ; ++i ) {
  std::swap( prev , DP );
  DP[ 0 ] = int( i );
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ j ] = std::min( { prev[ j - 1 ] + ( P[ i - 1 ] == T[ j - 1 ] ? 0 : 1 ) ,
                         prev[ j ] + 1 , DP[ j - 1 ] + 1 } );
  }
 ADS_POST( ( DP[ n ] >= int( m - n ) ) && ( DP[ n ] <= int( m ) ) );
 return( DP[ n ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File ApproximateMatching.tpp --------------------*/
/*--------------------------------------------------------------------------*/
