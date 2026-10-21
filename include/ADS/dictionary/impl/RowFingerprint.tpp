/*--------------------------------------------------------------------------*/
/*------------------------ File RowFingerprint.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RowFingerprint [see RowFingerprint.h], with their
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

namespace detail {

/// the words of a block of a row
inline constexpr std::size_t kRowBlock = 64;

/// the hash of the n values of x, by blocks, from the seed h
[[nodiscard]] inline std::uint64_t hashValues( const double * x ,
                                               std::size_t n ,
                                               std::uint64_t h )
{
 alignas( 64 ) std::uint64_t b[ kRowBlock ];
 for( std::size_t i = 0 ; i < n ; i += kRowBlock ) {
  const std::size_t k = n - i < kRowBlock ? n - i : kRowBlock;
  for( std::size_t j = 0 ; j < k ; ++j )
   b[ j ] = canonicalWord( x[ i + j ] );
  h = hashBytes( b , 8 * k , h );
  }
 return( h );
 }

/// true if the n values of a and b are equal as numbers, every NaN equal
[[nodiscard]] inline bool sameValues( const double * a , const double * b ,
                                      std::size_t n )
{
 if( ( n == 0 ) || ( std::memcmp( a , b , n * sizeof( double ) ) == 0 ) )
  return( true );                           // the same bits, the common case
 std::size_t i = 0;
 for( ; i + 8 <= n ; i += 8 ) {             // a block at a time
  bool d = false;
  for( std::size_t j = 0 ; j < 8 ; ++j ) {
   const double x = a[ i + j ] , y = b[ i + j ];
   d |= ! ( ( x == y ) | ( ( x != x ) & ( y != y ) ) );
   }
  if( d )
   return( false );
  }
 for( ; i < n ; ++i ) {
  const double x = a[ i ] , y = b[ i ];
  if( ! ( ( x == y ) || ( ( x != x ) && ( y != y ) ) ) )
   return( false );
  }
 return( true );
 }

}  // end( namespace detail )

inline std::uint64_t canonicalWord( double x )
{
 std::uint64_t w = std::bit_cast< std::uint64_t >( x );
 w = x == 0.0 ? 0 : w;                      // -0 as +0
 w = x != x ? 0x7FF8000000000000ULL : w;    // one NaN
 return( w );
 }

inline std::uint64_t rowFingerprint( std::span< const double > x ,
                                     std::uint64_t seed )
{
 if( x.empty() )
  return( hashBytes( nullptr , 0 , seed ) );
 return( detail::hashValues( x.data() , x.size() , seed ) );
 }

template< class I >
std::uint64_t rowFingerprint( std::size_t n , const I * ind ,
                              const double * val , std::uint64_t seed )
{
 static_assert( std::is_integral_v< I > );
 ADS_PRE( ( n == 0 ) || ( ( ind != nullptr ) && ( val != nullptr ) ) );
 if( n == 0 )
  return( hashBytes( nullptr , 0 , seed ) );
 return( detail::hashValues( val , n ,
                             hashBytes( ind , n * sizeof( I ) , seed ) ) );
 }

inline bool sameRow( std::span< const double > a ,
                     std::span< const double > b )
{
 return( ( a.size() == b.size() ) &&
         detail::sameValues( a.data() , b.data() , a.size() ) );
 }

template< class I >
bool sameRow( std::size_t na , const I * ia , const double * va ,
              std::size_t nb , const I * ib , const double * vb )
{
 ADS_PRE( ( na == 0 ) || ( ( ia != nullptr ) && ( va != nullptr ) ) );
 ADS_PRE( ( nb == 0 ) || ( ( ib != nullptr ) && ( vb != nullptr ) ) );
 if( na != nb )
  return( false );
 if( na == 0 )
  return( true );
 return( ( std::memcmp( ia , ib , na * sizeof( I ) ) == 0 ) &&
         detail::sameValues( va , vb , na ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File RowFingerprint.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
