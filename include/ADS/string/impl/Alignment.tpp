/*--------------------------------------------------------------------------*/
/*--------------------------- File Alignment.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Alignment [see Alignment.h], with their contracts.
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
int alignmentScore( const S & P , const S & T , Alignment kind )
{
 const std::size_t m = P.size() , n = T.size();
 const bool global = kind == Alignment::GLOBAL;
 std::vector< std::vector< int > > DP( m + 1 , std::vector< int >( n + 1 ) );
 for( std::size_t i = 0 ; i <= m ; ++i )
  DP[ i ][ 0 ] = global ? -2 * int( i ) : 0;
 for( std::size_t j = 0 ; j <= n ; ++j )
  DP[ 0 ][ j ] = global ? -2 * int( j ) : 0;
 int best = 0;
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j ) {
   const int d = P[ i - 1 ] == T[ j - 1 ] ? 1 : -1;
   int v = std::max( { DP[ i ][ j - 1 ] - 2 , DP[ i - 1 ][ j ] - 2 ,
                       DP[ i - 1 ][ j - 1 ] + d } );
   if( kind == Alignment::LOCAL )
    v = std::max( v , 0 );
   DP[ i ][ j ] = v;
   best = std::max( best , v );
   }
 if( global )
  return( DP[ m ][ n ] );
 if( kind == Alignment::LOCAL )
  return( best );
 int s = DP[ m ][ n ];                      // the last row and column
 for( std::size_t j = 0 ; j <= n ; ++j ) s = std::max( s , DP[ m ][ j ] );
 for( std::size_t i = 0 ; i <= m ; ++i ) s = std::max( s , DP[ i ][ n ] );
 return( s );
 }

template< class S >
std::pair< S , S > globalAlignment( const S & P , const S & T ,
                                    typename S::value_type gap )
{
 const std::size_t m = P.size() , n = T.size();
 std::vector< std::vector< int > > DP( m + 1 , std::vector< int >( n + 1 ) );
 for( std::size_t i = 0 ; i <= m ; ++i ) DP[ i ][ 0 ] = -2 * int( i );
 for( std::size_t j = 0 ; j <= n ; ++j ) DP[ 0 ][ j ] = -2 * int( j );
 for( std::size_t i = 1 ; i <= m ; ++i )
  for( std::size_t j = 1 ; j <= n ; ++j )
   DP[ i ][ j ] = std::max( { DP[ i ][ j - 1 ] - 2 , DP[ i - 1 ][ j ] - 2 ,
                              DP[ i - 1 ][ j - 1 ] +
                              ( P[ i - 1 ] == T[ j - 1 ] ? 1 : -1 ) } );
 S a , b;
 for( std::size_t i = m , j = n ; ( i > 0 ) || ( j > 0 ) ; )
  if( ( i > 0 ) && ( j > 0 ) && ( DP[ i ][ j ] == DP[ i - 1 ][ j - 1 ] +
                                  ( P[ i - 1 ] == T[ j - 1 ] ? 1 : -1 ) ) ) {
   a.push_back( P[ --i ] );
   b.push_back( T[ --j ] );
   }
  else if( ( i > 0 ) && ( DP[ i ][ j ] == DP[ i - 1 ][ j ] - 2 ) ) {
   a.push_back( P[ --i ] );
   b.push_back( gap );
   }
  else {
   a.push_back( gap );
   b.push_back( T[ --j ] );
   }
 std::reverse( a.begin() , a.end() );
 std::reverse( b.begin() , b.end() );
 ADS_POST( ( a.size() == b.size() ) &&
           ( a.size() >= std::max( P.size() , T.size() ) ) );
 return( std::pair( a , b ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Alignment.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
