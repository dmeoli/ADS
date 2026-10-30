/*--------------------------------------------------------------------------*/
/*------------------------- File RandomHashing.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RandomHashing.h, with their contracts.
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

constexpr std::uint64_t splitmix64( std::uint64_t x )
{
 x += 0x9e3779b97f4a7c15ULL;
 x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
 x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebULL;
 return( x ^ ( x >> 31 ) );
 }

constexpr std::uint64_t fibonacciPlace( std::uint64_t h , int b )
{
 ADS_PRE( ( b >= 1 ) && ( b <= 64 ) );
 return( ( h * 0x9e3779b97f4a7c15ULL ) >> ( 64 - b ) );
 }

inline ZobristTable::ZobristTable( std::size_t n , std::size_t m ,
                                   std::uint64_t seed )
 : f_n( n ) , f_m( m )
{
 ADS_PRE( ( n >= 1 ) && ( m >= 1 ) );
 SplitMix64 g( seed );
 f_R.resize( n * m );
 for( auto & r : f_R )
  r = g();
 }

inline std::uint64_t ZobristTable::operator()( std::size_t f ,
                                               std::size_t v ) const
{
 ADS_PRE( ( f < f_n ) && ( v < f_m ) );
 return( f_R[ f * f_m + v ] );
 }

template< std::ranges::random_access_range S >
 requires std::ranges::sized_range< S >
std::uint64_t ZobristTable::hash( const S & s ) const
{
 ADS_PRE( std::size_t( std::ranges::size( s ) ) == f_n );
 std::uint64_t h = 0;
 auto it = std::ranges::begin( s );
 for( std::size_t f = 0 ; f < f_n ; ++f )
  h ^= ( *this )( f , std::size_t( it[ std::ptrdiff_t( f ) ] ) );
 return( h );
 }

inline std::uint64_t ZobristTable::update( std::uint64_t h , std::size_t f ,
                                           std::size_t a ,
                                           std::size_t b ) const
{
 return( h ^ ( *this )( f , a ) ^ ( *this )( f , b ) );
 }

inline TabulationHash::TabulationHash( std::uint64_t seed )
{
 SplitMix64 g( seed );
 for( auto & t : f_T )
  for( auto & w : t )
   w = g();
 }

inline std::uint64_t TabulationHash::operator()( std::uint64_t x ) const
{
 std::uint64_t h = 0;
 for( int i = 0 ; i < 8 ; ++i , x >>= 8 )
  h ^= f_T[ i ][ x & 0xFF ];
 return( h );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RandomHashing.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
