/*--------------------------------------------------------------------------*/
/*----------------------- File WordRangeMinimum.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of WordRangeMinimum [see WordRangeMinimum.h], with their
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

#include <algorithm>
#include <type_traits>

namespace ADS {

template< class T , class Compare >
void WordRangeMinimum< T , Compare >::stacks( std::size_t b , std::size_t n )
{
 constexpr bool SIMD = std::is_arithmetic_v< T > &&
  ( std::is_same_v< Compare , std::less< T > > ||
    std::is_same_v< Compare , std::less<> > );
 std::uint64_t s = 0;
 if constexpr( SIMD )
  if( n == 64 ) {                           // the larger items at once
   for( std::size_t t = 0 ; t < 64 ; ++t ) {
    const std::uint64_t g = detail::greaterBits< 64 >( f_A.data() + b ,
                                                       f_A[ b + t ] );
    s = ( s & ~g ) | ( std::uint64_t( 1 ) << t );
    f_M[ b + t ] = s;
    }
   return;
   }
 for( std::size_t t = b ; t < b + n ; ++t ) {
  while( s ) {                              // pop the larger items
   const std::size_t top = b + 63 - std::size_t( std::countl_zero( s ) );
   if( ! f_less( f_A[ t ] , f_A[ top ] ) )
    break;
   s &= ~( std::uint64_t( 1 ) << ( top - b ) );
   }
  s |= std::uint64_t( 1 ) << ( t - b );
  f_M[ t ] = s;
  }
 }

template< class T , class Compare >
WordRangeMinimum< T , Compare >::WordRangeMinimum( std::vector< T > A ,
                                                   Compare less )
 : f_A( std::move( A ) ) , f_less( std::move( less ) )
{
 const std::size_t n = f_A.size();
 ADS_PRE( n < ( std::size_t( 1 ) << 32 ) );
 f_M.resize( n );
 f_P.resize( n );
 f_blocks = ( n + 63 ) / 64;
 const std::size_t levels = f_blocks ? std::size_t(
                             std::bit_width( f_blocks ) ) : 0;
 f_S.resize( levels * f_blocks );
 for( std::size_t b = 0 ; b < n ; b += 64 ) {    // each block
  const std::size_t e = std::min( n , b + 64 );
  stacks( b , e - b );
  std::size_t m = b;                        // the minimum of the prefix
  for( std::size_t t = b ; t < e ; ++t ) {
   if( f_less( f_A[ t ] , f_A[ m ] ) )
    m = t;
   f_P[ t ].pre = f_A[ m ];
   f_P[ t ].pi = std::uint8_t( m - b );
   }
  m = e - 1;                                // the minimum of the suffix,
  for( std::size_t t = e ; t-- > b ; ) {    // the leftmost one
   if( ! f_less( f_A[ m ] , f_A[ t ] ) )
    m = t;
   f_P[ t ].suf = f_A[ m ];
   f_P[ t ].si = std::uint8_t( m - b );
   }
  f_S[ b / 64 ] = Min{ f_P[ b ].suf , std::uint32_t( b + f_P[ b ].si ) };
  }
 for( std::size_t l = 1 ; l < levels ; ++l ) {   // the table of the blocks
  const std::size_t h = std::size_t( 1 ) << ( l - 1 );
  const Min * p = f_S.data() + ( l - 1 ) * f_blocks;
  Min * q = f_S.data() + l * f_blocks;
  for( std::size_t b = 0 ; b + 2 * h <= f_blocks ; ++b )
   q[ b ] = better( p[ b ] , p[ b + h ] );
  }
 }

template< class T , class Compare >
inline std::size_t WordRangeMinimum< T , Compare >::query(
                                       std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j < f_A.size() ) );
 // both answers, in one block and across blocks, are computed, and one is
 // chosen by a conditional move: the case of a random range would be
 // mispredicted half of the times; the blocks in between, when there are,
 // are taken on a branch, predicted on ranges of similar lengths
 const std::size_t bi = i / 64 , bj = j / 64;
 const Place & a = f_P[ i ];
 const Place & c = f_P[ j ];
 const std::size_t in = bi * 64 + std::size_t( std::countr_zero(
                         f_M[ j ] & ( ~std::uint64_t( 0 ) << ( i % 64 ) ) ) );
 Min m{ a.suf , std::uint32_t( bi * 64 + a.si ) };
 if( bj > bi + 1 ) {                        // the blocks in between
  const std::size_t f = bi + 1 , k = bj - f;
  const std::size_t l = std::size_t( std::bit_width( k ) ) - 1;
  const Min * p = f_S.data() + l * f_blocks;
  m = better( m , better( p[ f ] , p[ bj - ( std::size_t( 1 ) << l ) ] ) );
  }
 m = better( m , Min{ c.pre , std::uint32_t( bj * 64 + c.pi ) } );
 const std::size_t r = bi == bj ? in : std::size_t( m.i );
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t t = i ; t <= j ; ++t )
   if( f_less( f_A[ t ] , f_A[ r ] ) ||
       ( ( t < r ) && ! f_less( f_A[ r ] , f_A[ t ] ) ) )
    return( false );
  return( true ); }() );
 return( r );
 }

template< class T , class Compare >
const T & WordRangeMinimum< T , Compare >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_A.size() );
 return( f_A[ i ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File WordRangeMinimum.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
