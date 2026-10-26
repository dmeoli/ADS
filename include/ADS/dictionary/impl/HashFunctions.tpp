/*--------------------------------------------------------------------------*/
/*------------------------- File HashFunctions.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The hash functions of HashFunctions.h, with their contracts.
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

inline unsigned bin_bit( std::string_view k , std::size_t j )
{
 if( j >= 8 * k.size() )
  return( 0 );
 return( ( static_cast< unsigned char >( k[ j / 8 ] ) >> ( 7 - j % 8 ) ) &
         1u );
 }

inline std::uint64_t int_bin( std::string_view k )
{
 ADS_PRE( k.size() <= 8 );
 std::uint64_t i = 0;
 for( unsigned char c : k )
  i = ( i << 8 ) | c;
 return( i );
 }

inline std::uint64_t hash_extraction( std::string_view k , int p ,
                                      std::size_t s )
{
 ADS_PRE( ( p >= 1 ) && ( p <= 63 ) );
 std::uint64_t b = 0;
 for( int j = 0 ; j < p ; ++j )
  b = ( b << 1 ) | bin_bit( k , s + j );
 ADS_POST( b < ( 1ull << p ) );
 return( b );
 }

inline std::uint64_t hash_xor( std::string_view k , int p )
{
 ADS_PRE( ( p >= 1 ) && ( p <= 63 ) );
 std::uint64_t b = 0;
 for( std::size_t s = 0 ; s < 8 * k.size() ; s += p )
  b ^= hash_extraction( k , p , s );
 ADS_POST( b < ( 1ull << p ) );
 return( b );
 }

inline std::uint64_t hash_division( std::string_view k , std::uint64_t m ,
                                    std::uint64_t base , std::uint64_t seed )
{
 ADS_PRE( ( m > 0 ) && ( base > 0 ) &&
          ( m - 1 <= ( std::numeric_limits< std::uint64_t >::max() - 255 ) /
                     base ) );
 std::uint64_t b = seed % m;
 for( unsigned char c : k )
  b = ( b * base + c ) % m;
 ADS_POST( b < m );
 return( b );
 }

inline std::uint64_t hash_multiplication( std::uint64_t i , std::uint64_t m ,
                                          long double C )
{
 ADS_PRE( ( C >= 0 ) && ( C < 1 ) );
 long double ip;
 const long double f = std::modf( C * static_cast< long double >( i ) , &ip );
 const auto h = static_cast< std::uint64_t >( m * f );
 ADS_POST( ( m == 0 ) || ( h < m ) );
 return( h );
 }

inline std::uint64_t hash_fibonacci( std::uint64_t i , int p )
{
 ADS_PRE( ( p >= 1 ) && ( p <= 63 ) );
 constexpr std::uint64_t s = 0x9E3779B97F4A7C15ull;   // floor( C 2^64 )
 const std::uint64_t h = ( i * s ) >> ( 64 - p );
 ADS_POST( h < ( 1ull << p ) );
 return( h );
 }

inline std::uint64_t hash_midsquare( std::uint64_t i , int p )
{
 ADS_PRE( ( p >= 1 ) && ( p <= 32 ) );
 const std::uint64_t h = ( ( i * i ) >> ( 32 - p / 2 ) ) &
                         ( ( 1ull << p ) - 1 );
 ADS_POST( h < ( 1ull << p ) );
 return( h );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HashFunctions.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
