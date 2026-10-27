/*--------------------------------------------------------------------------*/
/*--------------------------- File GridPaths.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of GridPaths [see GridPaths.h], with their contracts.
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

template< class T >
GridSquare largestSquare( const std::vector< std::vector< T > > & A )
{
 GridSquare best;
 if( A.empty() )
  return( best );
 const std::size_t m = A.size() , n = A[ 0 ].size();
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       [ n ]( const std::vector< T > & r ) {
                        return( r.size() == n ); } ) );
 std::vector< std::size_t > S( n + 1 , 0 );     // S[ j + 1 ]: row i, col j
 for( std::size_t i = 0 ; i < m ; ++i ) {
  std::size_t diag = 0;                         // S[ i - 1 ][ j - 1 ]
  for( std::size_t j = 0 ; j < n ; ++j ) {
   const std::size_t up = S[ j + 1 ];
   S[ j + 1 ] = ( A[ i ][ j ] != T( 0 ) )
                ? 1 + std::min( { up , diag , S[ j ] } ) : 0;
   diag = up;
   if( S[ j + 1 ] > best.side ) {
    best.side = S[ j + 1 ];
    best.row = i + 1 - best.side;
    best.col = j + 1 - best.side;
    }
   }
  }
 ADS_AUDIT( [ & ] {
             for( std::size_t i = 0 ; i < best.side ; ++i )
              for( std::size_t j = 0 ; j < best.side ; ++j )
               if( A[ best.row + i ][ best.col + j ] == T( 0 ) )
                return( false );
             return( true );
             }() );
 return( best );
 }

/*--------------------------------------------------------------------------*/

template< class C , class T >
C countLatticePaths( const std::vector< std::vector< T > > & B )
{
 if( B.empty() )
  return( C( 0 ) );
 const std::size_t m = B.size() , n = B[ 0 ].size();
 ADS_PRE( std::all_of( B.begin() , B.end() ,       // before the empty
                       [ n ]( const std::vector< T > & r ) {   // first row
                        return( r.size() == n ); } ) );
 if( n == 0 )
  return( C( 0 ) );
 std::vector< C > N( n , C( 0 ) );              // N[ j ]: row i, col j
 N[ 0 ] = C( 1 );
 for( std::size_t i = 0 ; i < m ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j )
   if( B[ i ][ j ] == T( 0 ) )
    N[ j ] = C( 0 );
   else if( j > 0 )
    N[ j ] += N[ j - 1 ];
 return( N[ n - 1 ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File GridPaths.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
