/*--------------------------------------------------------------------------*/
/*-------------------------- File BloomFilter.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BloomFilter [see BloomFilter.h], with their
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

template< class T , class H >
BloomFilter< T , H >::BloomFilter( std::size_t m , int k , H h )
 : f_B( ( m + 63 ) / 64 , 0 ) , f_m( m ) , f_k( k ) , f_h( h )
{
 ADS_PRE( ( m >= 1 ) && ( k >= 1 ) );
 }

template< class T , class H >
BloomFilter< T , H > BloomFilter< T , H >::forItems( std::size_t n ,
                                                     double p )
{
 ADS_PRE( ( n >= 1 ) && ( p > 0 ) && ( p < 1 ) );
 const double ln2 = std::log( 2.0 );
 const auto m = std::size_t( std::ceil( - double( n ) * std::log( p ) /
                                        ( ln2 * ln2 ) ) );
 const int k = std::max( 1 , int( std::lround( double( m ) / double( n ) *
                                               ln2 ) ) );
 return( BloomFilter( m , k ) );
 }

template< class T , class H >
void BloomFilter< T , H >::insert( const T & x )
{
 const std::uint64_t a = splitmix64( std::uint64_t( f_h( x ) ) ) ,
                     b = splitmix64( a ) | 1;
 for( int i = 0 ; i < f_k ; ++i ) {
  const std::size_t j = std::size_t( ( a + std::uint64_t( i ) * b ) % f_m );
  f_B[ j / 64 ] |= std::uint64_t( 1 ) << ( j % 64 );
  }
 ++f_n;
 ADS_POST( maybe( x ) );
 }

template< class T , class H >
bool BloomFilter< T , H >::maybe( const T & x ) const
{
 const std::uint64_t a = splitmix64( std::uint64_t( f_h( x ) ) ) ,
                     b = splitmix64( a ) | 1;
 for( int i = 0 ; i < f_k ; ++i ) {
  const std::size_t j = std::size_t( ( a + std::uint64_t( i ) * b ) % f_m );
  if( !( f_B[ j / 64 ] >> ( j % 64 ) & 1 ) )
   return( false );
  }
 return( true );
 }

template< class T , class H >
double BloomFilter< T , H >::falsePositiveRate( void ) const
{
 return( std::pow( 1 - std::exp( - double( f_k ) * double( f_n ) /
                                 double( f_m ) ) , double( f_k ) ) );
 }

template< class T , class H >
std::size_t BloomFilter< T , H >::bits( void ) const
{
 return( f_m );
 }

template< class T , class H >
int BloomFilter< T , H >::hashes( void ) const
{
 return( f_k );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BloomFilter.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
