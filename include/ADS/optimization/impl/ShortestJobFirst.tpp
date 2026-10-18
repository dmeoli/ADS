/*--------------------------------------------------------------------------*/
/*----------------------- File ShortestJobFirst.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ShortestJobFirst [see ShortestJobFirst.h], with their
 * contracts.
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

template< class T >
std::vector< std::size_t > shortestJobFirst( const std::vector< T > & t )
{
 ADS_PRE( std::all_of( t.begin() , t.end() ,
                       []( const T & x ) { return( !( x < T( 0 ) ) ); } ) );
 std::vector< std::size_t > A( t.size() );
 std::iota( A.begin() , A.end() , 0 );
 std::stable_sort( A.begin() , A.end() , [ & ]( std::size_t a ,
                   std::size_t b ) { return( t[ a ] < t[ b ] ); } );
 ADS_POST( std::is_sorted( A.begin() , A.end() , [ & ]( std::size_t a ,
            std::size_t b ) { return( t[ a ] < t[ b ] ); } ) );
 return( A );
 }

template< class T >
T totalCompletion( const std::vector< T > & t ,
                   const std::vector< std::size_t > & A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       [ & ]( std::size_t i ) { return( i < t.size() ); } ) );
 T now = T( 0 ) , sum = T( 0 );
 for( std::size_t i : A )
  sum += ( now += t[ i ] );
 return( sum );
 }

template< class T , class W >
std::vector< std::size_t > smithOrder( const std::vector< T > & t ,
                                       const std::vector< W > & w )
{
 ADS_PRE( t.size() == w.size() );
 for( std::size_t i = 0 ; i < t.size() ; ++i )
  ADS_PRE( ! ( t[ i ] < T( 0 ) ) && ! ( w[ i ] < W( 0 ) ) &&
           ( ( T( 0 ) < t[ i ] ) || ( W( 0 ) < w[ i ] ) ) );
 auto before = [ & ]( std::size_t a , std::size_t b ) {
  return( t[ a ] * w[ b ] < t[ b ] * w[ a ] );
  };
 std::vector< std::size_t > A( t.size() );
 std::iota( A.begin() , A.end() , 0 );
 std::stable_sort( A.begin() , A.end() , before );
 ADS_POST( std::is_sorted( A.begin() , A.end() , before ) );
 return( A );
 }

template< class T , class W >
auto weightedCompletion( const std::vector< T > & t ,
                         const std::vector< W > & w ,
                         const std::vector< std::size_t > & A )
{
 ADS_PRE( t.size() == w.size() );
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       [ & ]( std::size_t i ) { return( i < t.size() ); } ) );
 using R = decltype( t[ 0 ] * w[ 0 ] );
 T now = T( 0 );
 R sum = R( 0 );
 for( std::size_t i : A )
  sum += w[ i ] * ( now += t[ i ] );
 return( sum );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File ShortestJobFirst.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
