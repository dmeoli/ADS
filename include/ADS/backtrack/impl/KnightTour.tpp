/*--------------------------------------------------------------------------*/
/*-------------------------- File KnightTour.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of KnightTour [see KnightTour.h], with their contracts.
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

inline std::optional< std::vector< std::vector< int > > > knightTour(
                                    int n , int x , int y ,
                                    bool warnsdorff )
{
 ADS_PRE( ( n < 1 ) || ( ( x >= 0 ) && ( x < n ) && ( y >= 0 ) &&
                         ( y < n ) ) );
 static constexpr std::array< int , 8 > X = { -1 , 1 , 2 , 2 , 1 , -1 , -2 ,
                                              -2 };
 static constexpr std::array< int , 8 > Y = { -2 , -2 , -1 , 1 , 2 , 2 , 1 ,
                                              -1 };
 if( n < 1 )                                   // no board, no tour
  return( std::nullopt );
 std::vector< std::vector< int > > A( n , std::vector< int >( n , 0 ) );
 auto ok = [ & ]( int a , int b ) {
  return( a >= 0 && a < n && b >= 0 && b < n && A[ a ][ b ] == 0 ); };
 auto degree = [ & ]( int a , int b ) {
  int d = 0;
  for( int i = 0 ; i < 8 ; ++i ) d += ok( a + X[ i ] , b + Y[ i ] );
  return( d );
  };
 auto rec = [ & ]( auto & self , int k , int a , int b ) -> bool {
  A[ a ][ b ] = k + 1;
  if( k == n * n - 1 )
   return( true );
  std::vector< std::pair< int , int > > moves;      // ( degree , move )
  for( int i = 0 ; i < 8 ; ++i )
   if( ok( a + X[ i ] , b + Y[ i ] ) )
    moves.emplace_back( warnsdorff ? degree( a + X[ i ] , b + Y[ i ] ) : 0 ,
                        i );
  std::stable_sort( moves.begin() , moves.end() , []( auto & p , auto & q ) {
   return( p.first < q.first ); } );
  for( auto [ d , i ] : moves )
   if( self( self , k + 1 , a + X[ i ] , b + Y[ i ] ) )
    return( true );
  A[ a ][ b ] = 0;
  return( false );
  };
 if( ( n < 1 ) || ! rec( rec , 0 , x , y ) )
  return( std::nullopt );
 ADS_AUDIT( ( [ & ] {                     // k and k + 1 a knight move apart
  std::vector< std::pair< int , int > > at( n * n );
  for( int a = 0 ; a < n ; ++a )
   for( int b = 0 ; b < n ; ++b ) at[ A[ a ][ b ] - 1 ] = { a , b };
  for( int k = 0 ; k + 1 < n * n ; ++k ) {
   const int da = std::abs( at[ k ].first - at[ k + 1 ].first ) ,
             db = std::abs( at[ k ].second - at[ k + 1 ].second );
   if( da * db != 2 ) return( false );
   }
  return( at[ 0 ] == std::pair( x , y ) ); }() ) );
 return( A );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File KnightTour.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
