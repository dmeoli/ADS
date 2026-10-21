/*--------------------------------------------------------------------------*/
/*------------------ File LongestCommonExtension.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of LceIndex [see LongestCommonExtension.h], with their
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
#include <bit>
#include <cstring>
#include <limits>

#if defined( __AVX2__ )
 #include <immintrin.h>
#endif

namespace ADS {

namespace detail {

/// prefetches the line of p for a read
inline void lcePrefetch( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p , 0 , 3 );
#else
 (void) p;
#endif
 }

/// the bytes that lce() compares at once, and the first of them where
/// p and q differ (lceChunk if none)
#if defined( __AVX2__ )
inline constexpr std::size_t lceChunk = 32;

inline std::size_t lceMismatch( const unsigned char * p ,
                                const unsigned char * q )
{
 using V = __m256i;
 const V a = _mm256_loadu_si256( reinterpret_cast< const V * >( p ) ) ,
         b = _mm256_loadu_si256( reinterpret_cast< const V * >( q ) );
 const unsigned e = unsigned( _mm256_movemask_epi8(
                     _mm256_cmpeq_epi8( a , b ) ) );
 return( std::size_t( std::countr_one( e ) ) );
 }
#else
inline constexpr std::size_t lceChunk = 8;

inline std::size_t lceMismatch( const unsigned char * p ,
                                const unsigned char * q )
{
 std::uint64_t a , b;
 std::memcpy( &a , p , 8 );
 std::memcpy( &b , q , 8 );
 if( a == b ) return( 8 );
 if constexpr( std::endian::native == std::endian::little )
  return( std::size_t( std::countr_zero( a ^ b ) ) / 8 );
 else
  return( std::size_t( std::countl_zero( a ^ b ) ) / 8 );
 }
#endif

/// the symbols that lce() compares directly before the range minimum
#ifndef ADS_LCE_DIRECT
 #define ADS_LCE_DIRECT 64
#endif
inline constexpr std::size_t lceDirect = ADS_LCE_DIRECT;

/// the lcp of the suffixes i and j of T (n bytes), symbol by symbol
inline std::size_t lceNaive( std::string_view T , std::size_t i ,
                             std::size_t j )
{
 std::size_t h = 0;
 while( ( i + h < T.size() ) && ( j + h < T.size() ) &&
        ( T[ i + h ] == T[ j + h ] ) )
  ++h;
 return( h );
 }

}  // end( namespace detail )

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
LceIndex< I >::LceIndex( std::string_view T ) : f_T( T ) , f_n( T.size() )
{
 f_T.append( detail::lceChunk , '\0' );   // room for the loads
 if( f_n < 2 ) return;
 const auto SA = suffixArray< I >( T );
 const auto lcp = lcpArrayPhi( T , SA );
 f_rank.resize( f_n );
 for( std::size_t r = 0 ; r < f_n ; ++r ) f_rank[ SA[ r ] ] = I( r );
 // LCP again, from a place of 64 bytes, so that a block of 16 entries
 // (of 32 bits) is one line of the cache
 const std::size_t N = lcp.size();
 constexpr std::size_t B = 16 , G = 64 , A = 64 / sizeof( I );
 f_line.assign( N + A , 0 );
 while( reinterpret_cast< std::uintptr_t >( f_line.data() + f_shift ) % 64 )
  ++f_shift;
 std::copy( lcp.begin() , lcp.end() , f_line.begin() + f_shift );
 const I * L0 = f_line.data() + f_shift;
 // the words of the blocks: the stack of the suffix minima of a block,
 // a pop clearing its highest bit
 f_blocks = ( N + B - 1 ) / B;
 f_word.resize( N );
 std::vector< I > least( f_blocks );
 for( std::size_t c = 0 ; c < f_blocks ; ++c ) {
  const std::size_t s = B * c , e = std::min( s + B , N );
  unsigned stack = 0;
  for( std::size_t r = s ; r < e ; ++r ) {
   while( stack ) {
    const int top = std::bit_width( stack ) - 1;
    if( L0[ s + std::size_t( top ) ] < L0[ r ] ) break;
    stack &= ~( 1u << top );
    }
   stack |= 1u << ( r - s );
   f_word[ r ] = std::uint16_t( stack );
   }
  least[ c ] = L0[ s + std::size_t( std::countr_zero( stack ) ) ];
  }
 // in each group of G blocks the sparse table of their minima, 7 levels
 // of G entries; the minima of the groups in a sparse table of their own
 f_groups = ( f_blocks + G - 1 ) / G;
 f_inner.assign( f_groups * 7 * G , 0 );
 std::vector< I > group( f_groups );
 for( std::size_t q = 0 ; q < f_groups ; ++q ) {
  I * row = f_inner.data() + q * 7 * G;
  const std::size_t s = q * G , e = std::min( s + G , f_blocks );
  for( std::size_t x = s ; x < e ; ++x ) row[ x - s ] = least[ x ];
  for( std::size_t L = 1 ; L < 7 ; ++L ) {
   const std::size_t half = std::size_t( 1 ) << ( L - 1 );
   for( std::size_t x = 0 ; x + 2 * half <= e - s ; ++x )
    row[ L * G + x ] = std::min( row[ ( L - 1 ) * G + x ] ,
                                 row[ ( L - 1 ) * G + x + half ] );
   }
  group[ q ] = *std::min_element( row , row + ( e - s ) );
  }
 const std::size_t levels = std::size_t( std::bit_width( f_groups ) );
 f_top.resize( levels * f_groups );
 std::copy( group.begin() , group.end() , f_top.begin() );
 for( std::size_t L = 1 ; L < levels ; ++L ) {
  const I * below = f_top.data() + ( L - 1 ) * f_groups;
  I * here = f_top.data() + L * f_groups;
  const std::size_t half = std::size_t( 1 ) << ( L - 1 );
  for( std::size_t c = 0 ; c + 2 * half <= f_groups ; ++c )
   here[ c ] = std::min( below[ c ] , below[ c + half ] );
  }
 }

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
I LceIndex< I >::minimum( std::size_t a , std::size_t c ) const
{
 constexpr std::size_t B = 16 , G = 64;
 const I * const lcp = f_line.data() + f_shift;
 const std::size_t ba = a / B , bc = c / B , oa = a % B;
 // the lines of the two blocks requested with their words, before the
 // words tell which entries of them to read
 detail::lcePrefetch( lcp + B * ba );
 detail::lcePrefetch( lcp + B * bc );
 if( ba == bc )
  return( lcp[ a + std::size_t( std::countr_zero( unsigned( f_word[ c ] ) >>
                                                  oa ) ) ] );
 // the end of the block of a, the start of that of c
 const std::size_t ea = std::min( B * ba + B - 1 , f_word.size() - 1 );
 const unsigned wa = unsigned( f_word[ ea ] ) >> oa ,
                wc = unsigned( f_word[ c ] );
 I v = std::min( lcp[ a + std::size_t( std::countr_zero( wa ) ) ] ,
                 lcp[ B * bc + std::size_t( std::countr_zero( wc ) ) ] );
 if( ba + 1 == bc ) return( v );
 // the blocks x .. y between them: in one group by its table, else the
 // two partial groups by theirs and the groups between by the top table
 const std::size_t x = ba + 1 , y = bc - 1 , gx = x / G , gy = y / G;
 auto inner = [ & ]( std::size_t q , std::size_t f , std::size_t l ) {
  const std::size_t L = std::size_t( std::bit_width( l - f + 1 ) ) - 1;
  const I * row = f_inner.data() + ( q * 7 + L ) * G;
  return( std::min( row[ f ] , row[ l + 1 - ( std::size_t( 1 ) << L ) ] ) );
  };
 if( gx == gy ) return( std::min( v , inner( gx , x % G , y % G ) ) );
 v = std::min( { v , inner( gx , x % G , G - 1 ) , inner( gy , 0 , y % G ) } );
 if( gx + 1 < gy ) {
  const std::size_t f = gx + 1 , len = gy - f ,
                    L = std::size_t( std::bit_width( len ) ) - 1;
  const I * row = f_top.data() + L * f_groups;
  v = std::min( { v , row[ f ] , row[ gy - ( std::size_t( 1 ) << L ) ] } );
  }
 return( v );
 }

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::size_t LceIndex< I >::lce( std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i <= f_n ) && ( j <= f_n ) );
 if( i == j ) return( f_n - i );
 const std::size_t far = f_n - std::max( i , j );
 // the first chunk of symbols directly (32 with AVX2, 8 else): the text
 // has a chunk of 0 after it, and a mismatch past far is cut to far; the
 // rest out of line, so that this part is inlined where it is called
 const auto * t = reinterpret_cast< const unsigned char * >( f_T.data() );
 const std::size_t d = detail::lceMismatch( t + i , t + j );
 if( d < detail::lceChunk ) {
  const std::size_t e = std::min( d , far );
  ADS_AUDIT( e == detail::lceNaive( std::string_view( f_T.data() , f_n ) ,
                                    i , j ) );
  return( e );
  }
 if( detail::lceChunk >= far ) return( far );
 return( longer( i , j , far ) );
 }

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::size_t LceIndex< I >::longer( std::size_t i , std::size_t j ,
                                   std::size_t far ) const
{
 // the next chunks directly up to lceDirect, then the range minimum
 const auto * t = reinterpret_cast< const unsigned char * >( f_T.data() );
 for( std::size_t h = detail::lceChunk ; h < detail::lceDirect ;
      h += detail::lceChunk ) {
  const std::size_t d = detail::lceMismatch( t + i + h , t + j + h );
  if( d < detail::lceChunk ) {
   const std::size_t e = std::min( h + d , far );
   ADS_AUDIT( e == detail::lceNaive( std::string_view( f_T.data() , f_n ) ,
                                     i , j ) );
   return( e );
   }
  if( h + detail::lceChunk >= far ) return( far );
  }
 std::size_t a = f_rank[ i ] , b = f_rank[ j ];
 if( a > b ) std::swap( a , b );
 const std::size_t v = minimum( a , b - 1 );
 ADS_AUDIT( v == detail::lceNaive( std::string_view( f_T.data() , f_n ) ,
                                   i , j ) );
 return( v );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------ End File LongestCommonExtension.tpp -------------------*/
/*--------------------------------------------------------------------------*/
