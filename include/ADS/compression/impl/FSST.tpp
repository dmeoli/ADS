/*--------------------------------------------------------------------------*/
/*----------------------------- File FSST.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of FSST.h, with their contracts.
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

#include "ADS/compression/impl/Unaligned.h"

#include <random>

namespace ADS
{

namespace detail::fsst
{

inline constexpr std::size_t kSample = 16384;  ///< the bytes of the sample
inline constexpr std::size_t kChunk = 512;     ///< the bytes of a piece
inline constexpr std::size_t kIds = 511;       ///< the 255 codes and 256 bytes

/// the mask of the first len bytes of a word
constexpr std::uint64_t mask( unsigned len )
{
 return( len >= 8 ? ~std::uint64_t( 0 )
		   : ( std::uint64_t( 1 ) << ( 8 * len ) ) - 1 );
 }

/// the word of the next min( r , 8 ) bytes from p, the first in the low
/// byte, padded with 0
inline std::uint64_t load( const char * p , std::size_t r )
{
 if( r >= 8 )
  return(
    loadLE< std::uint64_t >( reinterpret_cast< const std::uint8_t * >( p ) ) );
 std::uint8_t b[ 8 ] = {};
 std::memcpy( b , p , r );
 return( loadLE< std::uint64_t >( b ) );
 }

/// whether one of the 4 bytes of w is 255, the escape
constexpr bool hasEscape( std::uint32_t w )
{
 const std::uint32_t v = ~w;
 return( ( ( v - 0x01010101u ) & ~v & 0x80808080u ) != 0 );
 }

}  // end( namespace detail::fsst )

inline FSSTTable::FSSTTable( std::nullptr_t )
    : f_hash( kHash ) , f_pair( std::size_t( 1 ) << 16 , 0 )
{
 f_byte.fill( std::uint16_t( ( 1 << 8 ) | ESCAPE ) );
 }

inline FSSTTable::FSSTTable( void ) : FSSTTable( nullptr )
{
 finish();
 }

template< std::ranges::forward_range R >
 requires std::convertible_to< std::ranges::range_reference_t< R > ,
			       std::string_view >
FSSTTable::FSSTTable( const R & S ) : FSSTTable()
{
 using namespace detail::fsst;
 std::vector< std::string_view > all;
 std::size_t total = 0;
 for( auto && s : S ) {
  all.emplace_back( std::string_view( s ) );
  total += all.back().size();
  }
 // the sample: all the strings if they are small, else pieces of 512
 // bytes of strings drawn at random up to 16 KB
 std::vector< std::string_view > Z;
 if( total <= kSample ) {
  for( const auto s : all )
   if( ! s.empty() )
    Z.push_back( s );
  }
 else {
  std::mt19937_64 g( 0x5eed );
  for( std::size_t got = 0 ; got < kSample ; ) {
   std::string_view s = all[ std::size_t( g() % all.size() ) ];
   if( s.size() > kChunk )
    s = s.substr( std::size_t( g() % ( s.size() - kChunk + 1 ) ) , kChunk );
   if( s.empty() )
    continue;
   Z.push_back( s );
   got += s.size();
   }
  }
 build( Z );
 }

inline std::string FSSTTable::symbol( std::size_t c ) const
{
 ADS_PRE( c < f_n );
 std::string s( f_len[ c ] , '\0' );
 for( std::size_t k = 0 ; k < s.size() ; ++k )
  s[ k ] = char( std::uint8_t( f_sym[ c ] >> ( 8 * k ) ) );
 return( s );
 }

template< bool Done >
inline std::uint16_t FSSTTable::match( std::uint64_t w , std::size_t r ) const
{
 if( r >= 3 ) {
  const Slot & e = f_hash[ slot( w ) ];
  if( ( e.len >= 3 ) && ( e.len <= r ) &&
      ( ( w & detail::fsst::mask( e.len ) ) == e.sym ) )
   return( std::uint16_t( ( e.len << 8 ) | e.code ) );
  }
 if( r >= 2 ) {
  const std::uint16_t p = f_pair[ w & 0xFFFF ];
  if( Done || ( ( p >> 8 ) == 2 ) )
   return( p );
  }
 return( f_byte[ w & 0xFF ] );
 }

inline bool FSSTTable::add( std::uint64_t sym , unsigned len )
{
 if( f_n == 255 )
  return( false );
 const auto code = std::uint8_t( f_n );
 if( len >= 3 ) {
  Slot & e = f_hash[ slot( sym ) ];
  if( e.len != 0 )
   return( false );
  e = Slot{ sym , std::uint8_t( len ) , code };
  }
 else if( len == 2 ) {
  if( ( f_pair[ sym ] >> 8 ) == 2 )
   return( false );
  f_pair[ sym ] = std::uint16_t( ( 2 << 8 ) | code );
  }
 else {
  if( ( f_byte[ sym ] & 0xFF ) != ESCAPE )
   return( false );
  f_byte[ sym ] = std::uint16_t( ( 1 << 8 ) | code );
  }
 f_sym[ f_n ] = sym;
 f_len[ f_n ] = std::uint8_t( len );
 ++f_n;
 return( true );
 }

inline void FSSTTable::finish( void )
{
 for( std::size_t w = 0 ; w < f_pair.size() ; ++w )
  if( ( f_pair[ w ] >> 8 ) != 2 )
   f_pair[ w ] = f_byte[ w & 0xFF ];
 }

inline void FSSTTable::build( const std::vector< std::string_view > & Z )
{
 using namespace detail::fsst;
 FSSTTable T( nullptr );
 // the counts of the pairs, of at most the bytes of the sample each, and
 // the pairs counted, so that neither a clear nor a scan visits the pairs
 // that do not occur
 std::vector< std::uint32_t > c1( kIds );
 std::vector< std::uint16_t > c2( kIds * kIds , 0 );
 std::vector< std::uint32_t > seen;
 struct Cand
 {
  std::uint64_t sym , gain;
  unsigned len;
  };
 std::vector< Cand > K;
 for( const unsigned frac : { 8u , 38u , 68u , 98u , 128u } ) {
  // the codes and the pairs of codes of the fraction of the sample, a
  // byte escaped as the id 255 + the byte
  std::fill( c1.begin() , c1.end() , 0u );
  for( const auto k : seen )
   c2[ k ] = 0;
  seen.clear();
  auto pair = [ & ]( std::size_t k ) {
   if( c2[ k ]++ == 0 )
    seen.push_back( std::uint32_t( k ) );
   };
  auto byteId = [ & ]( std::uint8_t b ) {
   const std::size_t c = T.f_byte[ b ] & 0xFF;
   return( c != ESCAPE ? c : 255 + std::size_t( b ) );
   };
  for( std::size_t j = 0 ; j < Z.size() ; ++j ) {
   if( ( frac < 128 ) && ( ( hashWord( j ) & 127 ) >= frac ) )
    continue;
   const std::string_view s = Z[ j ];
   std::size_t prev = kIds;
   for( std::size_t i = 0 ; i < s.size() ; ) {
    const std::uint64_t w = load( s.data() + i , s.size() - i );
    const std::uint16_t m = T.match< false >( w , s.size() - i );
    const std::size_t c = m & 0xFF , l = m >> 8;
    const std::uint8_t b0 = std::uint8_t( w );
    const std::size_t id = c != ESCAPE ? c : 255 + std::size_t( b0 );
    ++c1[ id ];
    if( l > 1 )
     ++c1[ byteId( b0 ) ];
    if( prev < kIds ) {
     pair( prev * kIds + id );
     if( l > 1 )
      pair( prev * kIds + byteId( b0 ) );
     }
    prev = id;
    i += l;
    }
   }
  // the candidates, their gains added up per symbol, by decreasing gain
  auto text = [ & ]( std::size_t id ) {
   return( id < 255 ? std::pair< std::uint64_t , unsigned >( T.f_sym[ id ] ,
							     T.f_len[ id ] )
		     : std::pair< std::uint64_t , unsigned >( id - 255 , 1 ) );
   };
  K.clear();
  for( std::size_t a = 0 ; a < kIds ; ++a )
   if( c1[ a ] > 0 ) {
    const auto [ s , l ] = text( a );
    K.push_back( { s , std::uint64_t( c1[ a ] ) * l * ( l == 1 ? 8 : 1 ) ,
		   l } );
    }
  for( const auto k : seen ) {
   const auto [ s , l ] = text( k / kIds );
   if( l >= 8 )
    continue;
   const auto [ t , m ] = text( k % kIds );
   const unsigned L = std::min( 8u , l + m );
   K.push_back( { ( s | ( t << ( 8 * l ) ) ) & mask( L ) ,
		  std::uint64_t( c2[ k ] ) * L , L } );
   }
  std::sort( K.begin() , K.end() , []( const Cand & x , const Cand & y ) {
   return( x.len != y.len ? x.len < y.len : x.sym < y.sym );
   } );
  std::size_t k = 0;
  for( std::size_t i = 0 ; i < K.size() ; ++i )
   if( ( k > 0 ) && ( K[ k - 1 ].len == K[ i ].len ) &&
       ( K[ k - 1 ].sym == K[ i ].sym ) )
    K[ k - 1 ].gain += K[ i ].gain;
   else
    K[ k++ ] = K[ i ];
  K.resize( k );
  // the 1024 best first, which the 255 symbols seldom pass, and the
  // others in order only if they do
  auto better = []( const Cand & x , const Cand & y ) {
   if( x.gain != y.gain )
    return( x.gain > y.gain );
   return( x.len != y.len ? x.len > y.len : x.sym < y.sym );
   };
  const auto head =
    K.begin() + std::ptrdiff_t( std::min< std::size_t >( K.size() , 1024 ) );
  std::nth_element( K.begin() , head , K.end() , better );
  std::sort( K.begin() , head , better );
  FSSTTable N( nullptr );
  for( std::size_t i = 0 ; ( i < K.size() ) && ( N.f_n < 255 ) ; ++i ) {
   if( i == 1024 )
    std::sort( K.begin() + 1024 , K.end() , better );
   N.add( K[ i ].sym , K[ i ].len );
   }
  T = std::move( N );
  }
 T.finish();
 *this = std::move( T );
 }

inline void FSSTTable::encode( std::string_view s ,
			       std::vector< std::uint8_t > & out ) const
{
 [[maybe_unused]] const std::size_t at = out.size();
 out.reserve( at + 2 * s.size() );
 for( std::size_t i = 0 ; i < s.size() ; ) {
  const std::uint64_t w = detail::fsst::load( s.data() + i , s.size() - i );
  const std::uint16_t m = match( w , s.size() - i );
  out.push_back( std::uint8_t( m ) );
  if( std::uint8_t( m ) == ESCAPE )
   out.push_back( std::uint8_t( w ) );
  i += std::size_t( m >> 8 );
  }
 ADS_AUDIT( decode( std::span( out ).subspan( at ) ) == s );
 }

inline std::vector< std::uint8_t >
FSSTTable::encode( std::string_view s ) const
{
 std::vector< std::uint8_t > out;
 encode( s , out );
 return( out );
 }

inline std::size_t FSSTTable::decode( std::span< const std::uint8_t > code ,
				      char * out ) const
{
 const std::uint8_t * y = code.data();
 const std::size_t m = code.size();
 std::uint8_t * o = reinterpret_cast< std::uint8_t * >( out );
 std::size_t i = 0 , j = 0;
 // 4 codes at a time while none of them is an escape
 while( i + 4 <= m ) {
  const std::uint32_t w = detail::loadLE< std::uint32_t >( y + i );
  if( detail::fsst::hasEscape( w ) )
   break;
  for( unsigned k = 0 ; k < 4 ; ++k ) {
   const std::uint8_t c = std::uint8_t( w >> ( 8 * k ) );
   ADS_PRE( c < f_n );
   detail::storeLE< std::uint64_t >( o + j , f_sym[ c ] );
   j += f_len[ c ];
   }
  i += 4;
  }
 while( i < m ) {
  const std::uint8_t c = y[ i++ ];
  if( c != ESCAPE ) {
   ADS_PRE( c < f_n );
   detail::storeLE< std::uint64_t >( o + j , f_sym[ c ] );
   j += f_len[ c ];
   }
  else {
   ADS_PRE( i < m );
   o[ j++ ] = y[ i++ ];
   }
  // past an escape, 4 codes at a time again
  while( i + 4 <= m ) {
   const std::uint32_t w = detail::loadLE< std::uint32_t >( y + i );
   if( detail::fsst::hasEscape( w ) )
    break;
   for( unsigned k = 0 ; k < 4 ; ++k ) {
    const std::uint8_t d = std::uint8_t( w >> ( 8 * k ) );
    ADS_PRE( d < f_n );
    detail::storeLE< std::uint64_t >( o + j , f_sym[ d ] );
    j += f_len[ d ];
    }
   i += 4;
   }
  }
 return( j );
 }

inline std::string
FSSTTable::decode( std::span< const std::uint8_t > code ) const
{
 std::string s( 8 * code.size() , '\0' );
 s.resize( decode( code , s.data() ) );
 return( s );
 }

inline std::vector< std::uint8_t > FSSTTable::serialize( void ) const
{
 std::vector< std::uint8_t > out( 1 + f_n );
 out[ 0 ] = std::uint8_t( f_n );
 for( std::size_t c = 0 ; c < f_n ; ++c ) {
  out[ 1 + c ] = f_len[ c ];
  for( unsigned k = 0 ; k < f_len[ c ] ; ++k )
   out.push_back( std::uint8_t( f_sym[ c ] >> ( 8 * k ) ) );
  }
 return( out );
 }

inline FSSTTable
FSSTTable::deserialize( std::span< const std::uint8_t > bytes )
{
 ADS_PRE( ! bytes.empty() );
 const std::size_t n = bytes[ 0 ];
 ADS_PRE( ( n <= 255 ) && ( bytes.size() >= 1 + n ) );
 FSSTTable T( nullptr );
 std::size_t at = 1 + n;
 for( std::size_t c = 0 ; c < n ; ++c ) {
  const unsigned len = bytes[ 1 + c ];
  ADS_PRE( ( len >= 1 ) && ( len <= 8 ) && ( at + len <= bytes.size() ) );
  std::uint64_t sym = 0;
  for( unsigned k = 0 ; k < len ; ++k )
   sym |= std::uint64_t( bytes[ at + k ] ) << ( 8 * k );
  at += len;
  [[maybe_unused]] const bool added = T.add( sym , len );
  ADS_PRE( added );
  }
 T.finish();
 return( T );
 }

template< std::ranges::forward_range R >
 requires std::convertible_to< std::ranges::range_reference_t< R > ,
			       std::string_view >
FSSTColumn::FSSTColumn( const R & S ) : f_table( S )
{
 for( auto && s : S ) {
  f_at.push_back( f_code.size() );
  f_table.encode( std::string_view( s ) , f_code );
  }
 f_at.push_back( f_code.size() );
 ADS_AUDIT( [ & ]() {
  std::size_t i = 0;
  for( auto && s : S )
   if( ( *this )[ i++ ] != std::string_view( s ) )
    return( false );
  return( true );
  }() );
 }

inline std::span< const std::uint8_t >
FSSTColumn::code( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( std::span< const std::uint8_t >( f_code ).subspan(
   f_at[ i ] , f_at[ i + 1 ] - f_at[ i ] ) );
 }

inline std::size_t FSSTColumn::sizeInBytes( void ) const
{
 std::size_t t = 1 + f_table.symbols();
 for( std::size_t c = 0 ; c < f_table.symbols() ; ++c )
  t += f_table.symbol( c ).size();
 return( f_code.size() + f_at.size() * sizeof( std::size_t ) + t );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File FSST.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
