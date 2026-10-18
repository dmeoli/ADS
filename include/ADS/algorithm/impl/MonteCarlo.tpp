/*--------------------------------------------------------------------------*/
/*-------------------------- File MonteCarlo.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of MonteCarlo.h [see MonteCarlo.h].
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

template< class F , class R >
bool zeroPolynomial( F && p , std::size_t n , long d , int K , R & rng )
{
 ADS_PRE( K >= 0 );
 std::uniform_int_distribution< long > v( 1 , 2 * ( d < 1 ? 1 : d ) );
 std::vector< long > x( n );
 for( int k = 0 ; k < K ; ++k ) {
  for( auto & xi : x ) xi = v( rng );
  if( p( x ) != 0 )
   return( false );                        // surely not zero
  }
 return( true );
 }

template< class T , class R >
bool freivalds( const std::vector< std::vector< T > > & A ,
                const std::vector< std::vector< T > > & B ,
                const std::vector< std::vector< T > > & C , int K , R & rng )
{
 const std::size_t n = A.size();
 const auto square = [ n ]( const std::vector< std::vector< T > > & M ) {
  return( ( M.size() == n ) &&
          std::all_of( M.begin() , M.end() , [ n ]( const auto & r ) {
           return( r.size() == n ); } ) ); };
 ADS_PRE( square( A ) && square( B ) && square( C ) && ( K >= 0 ) );
 std::vector< T > x( n ) , y( n ) , z( n ) , w( n );
 auto mul = [ n ]( const auto & M , const std::vector< T > & v ,
                   std::vector< T > & r ) {
  for( std::size_t i = 0 ; i < n ; ++i ) {
   r[ i ] = T( 0 );
   for( std::size_t j = 0 ; j < n ; ++j ) r[ i ] += M[ i ][ j ] * v[ j ];
   }
  };
 for( int k = 0 ; k < K ; ++k ) {
  for( auto & xi : x ) xi = ( rng() & 1 ) ? T( 1 ) : T( -1 );
  mul( B , x , y );
  mul( A , y , z );
  mul( C , x , w );
  if( z != w )
   return( false );                        // surely A B != C
  }
 return( true );
 }

template< class F , class R >
bool halfSatisfiable( F && f , std::size_t n , int K , R & rng )
{
 ADS_PRE( K >= 0 );
 std::vector< bool > x( n );
 int yes = 0;
 for( int k = 0 ; k < K ; ++k ) {
  for( std::size_t i = 0 ; i < n ; ++i ) x[ i ] = rng() & 1;
  yes += f( x ) ? 1 : 0;
  }
 return( 2 * yes > K );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File MonteCarlo.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
