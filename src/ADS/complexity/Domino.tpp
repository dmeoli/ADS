/*--------------------------------------------------------------------------*/
/*---------------------------- File Domino.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Domino [see Domino.h], with their contracts.
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

inline bool legalTiling( const std::vector< Tile > & D , int n ,
                         const Tiling & T )
{
 if( ( int( T.size() ) != n ) || ( n < 1 ) || ( T[ 0 ][ 0 ] != 0 ) )
  return( false );
 for( int i = 0 ; i < n ; ++i ) {
  if( int( T[ i ].size() ) != n ) return( false );
  for( int j = 0 ; j < n ; ++j ) {
   const int t = T[ i ][ j ];
   if( ( t < 0 ) || ( t >= int( D.size() ) ) ) return( false );
   if( ( i + 1 < n ) && ( D[ t ].right != D[ T[ i + 1 ][ j ] ].left ) )
    return( false );
   if( ( j + 1 < n ) && ( D[ t ].up != D[ T[ i ][ j + 1 ] ].down ) )
    return( false );
   }
  }
 return( true );
 }

inline std::optional< Tiling > squareTiling( const std::vector< Tile > & D ,
                                             int n )
{
 const auto result = [ & ]( void ) -> std::optional< Tiling > {
  if( ( n < 1 ) || D.empty() )
   return( std::nullopt );
  Tiling T( n , std::vector< int >( n , -1 ) );
  auto rec = [ & ]( auto & self , int s ) -> bool {
   if( s == n * n )
    return( true );
   const int i = s % n , j = s / n;                 // by rows from the bottom
   for( int t = 0 ; t < int( D.size() ) ; ++t ) {
    if( ( s == 0 ) && ( t != 0 ) ) break;
    if( ( i > 0 ) && ( D[ T[ i - 1 ][ j ] ].right != D[ t ].left ) ) continue;
    if( ( j > 0 ) && ( D[ T[ i ][ j - 1 ] ].up != D[ t ].down ) ) continue;
    T[ i ][ j ] = t;
    if( self( self , s + 1 ) )
     return( true );
    }
   T[ i ][ j ] = -1;
   return( false );
   };
  if( ! rec( rec , 0 ) )
   return( std::nullopt );
  return( T );
  }();
 ADS_POST( ! result || legalTiling( D , n , *result ) );
 return( result );
 }

inline bool linearTiling( const std::vector< Tile > & D , long n )
{
 const int m = int( D.size() );
 if( ( n < 1 ) || ( m == 0 ) )
  return( false );
 // the nodes reachable from d, and a cycle among them, by a dfs
 std::vector< int > state( m , 0 );               // 0 new, 1 open, 2 done
 std::vector< long > longest( m , 0 );            // in the acyclic part
 bool cycle = false;
 auto dfs = [ & ]( auto & self , int u ) -> void {
  state[ u ] = 1;
  for( int v = 0 ; v < m ; ++v )
   if( D[ u ].right == D[ v ].left ) {
    if( state[ v ] == 1 ) cycle = true;
    else if( state[ v ] == 0 ) self( self , v );
    longest[ u ] = std::max( longest[ u ] , 1 + longest[ v ] );
    }
  state[ u ] = 2;
  };
 dfs( dfs , 0 );
 return( cycle || ( longest[ 0 ] >= n - 1 ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Domino.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
