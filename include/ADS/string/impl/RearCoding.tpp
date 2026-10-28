/*--------------------------------------------------------------------------*/
/*--------------------------- File RearCoding.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RearCoding.h, with their contracts.
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

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>

namespace ADS {

inline std::vector< RearPair > rearCode( const std::vector< std::string > & D )
{
 std::vector< RearPair > R;
 R.reserve( D.size() );
 for( std::size_t i = 0 ; i < D.size() ; ++i ) {
  const std::size_t l = i ? commonPrefixLength( D[ i - 1 ] , D[ i ] ) : 0;
  R.emplace_back( i ? D[ i - 1 ].size() - l : 0 , D[ i ].substr( l ) );
  }
 ADS_AUDIT( rearDecode( R ) == D );
 return( R );
 }

inline std::vector< std::string > rearDecode(
                                           const std::vector< RearPair > & R )
{
 std::vector< std::string > D;
 D.reserve( R.size() );
 std::string s;
 for( const RearPair & p : R ) {
  ADS_PRE( p.first <= s.size() );
  s.resize( s.size() - p.first );
  s += p.second;
  D.push_back( s );
  }
 return( D );
 }

inline void RearCodedDictionary::Code::build(
                                 const std::array< std::uint64_t , 256 > & f )
{
 len = limitedHuffmanLengths( f , kMax );
 for( int c = 0 ; c < 256 ; ++c )
  if( ( f[ c ] > 0 ) && ( len[ c ] == 0 ) )
   len[ c ] = 1;                            // a lone byte takes one bit
 // the canonical code: by length, then by byte; each codeword is turned
 // around, so that its first bit is the lowest, as the decoder reads
 std::array< int , 256 > by;
 for( int c = 0 ; c < 256 ; ++c ) by[ c ] = c;
 std::sort( by.begin() , by.end() , [ & ]( int a , int b ) {
  return( len[ a ] < len[ b ] || ( len[ a ] == len[ b ] && a < b ) ); } );
 std::uint32_t code = 0;
 int last = 0;
 table.assign( std::size_t( 1 ) << kMax , 0 );
 for( const int c : by ) {
  if( len[ c ] == 0 )
   continue;
  code <<= ( len[ c ] - last );
  last = len[ c ];
  std::uint32_t w = 0;
  for( int b = 0 ; b < len[ c ] ; ++b )
   w |= ( ( code >> b ) & 1 ) << ( len[ c ] - 1 - b );
  word[ c ] = w;
  const std::uint32_t ext = std::uint32_t( 1 ) << ( kMax - len[ c ] );
  for( std::uint32_t e = 0 ; e < ext ; ++e )
   table[ w | ( e << len[ c ] ) ] = std::uint16_t( c | ( len[ c ] << 8 ) );
  ++code;
  }
 // the symbols whose codewords fit whole in each window of kWide bits, up
 // to 3, with their number and their bits
 multi.assign( std::size_t( 1 ) << kWide , 0 );
 for( std::uint32_t v = 0 ; v < ( std::uint32_t( 1 ) << kWide ) ; ++v ) {
  std::uint32_t e = 0 , used = 0 , count = 0;
  while( count < 3 ) {
   const std::uint16_t t = table[ ( v >> used ) &
                                  ( ( std::uint32_t( 1 ) << kMax ) - 1 ) ];
   const std::uint32_t l = t >> 8;
   if( ( l == 0 ) || ( used + l > std::uint32_t( kWide ) ) )
    break;
   e |= std::uint32_t( t & 255 ) << ( 8 * count );
   used += l;
   ++count;
   }
  multi[ v ] = e | ( count << 24 ) | ( used << 26 );
  }
 }

inline std::uint64_t RearCodedDictionary::peek( std::size_t pos ) const
{
 if constexpr( std::endian::native == std::endian::little ) {
  std::uint64_t x;
  std::memcpy( &x , reinterpret_cast< const unsigned char * >(
                      f_C.words().data() ) + pos / 8 , 8 );
  return( x >> ( pos % 8 ) );
  }
 else
  return( f_C.get( pos , 56 ) );
 }

inline unsigned RearCodedDictionary::decode( const Code & c ,
                                             std::size_t & pos ) const
{
 const std::uint16_t e = c.table[ std::size_t( peek( pos ) ) &
                                  ( ( std::size_t( 1 ) << kMax ) - 1 ) ];
 pos += std::size_t( e >> 8 );
 return( e & 255 );
 }

inline std::size_t RearCodedDictionary::getLength( const Code & c ,
                                                   std::size_t & pos ) const
{
 const unsigned v = decode( c , pos );
 if( v < 255 )
  return( v );
 const std::size_t w = std::size_t( peek( pos ) &             // an escape
                                   ( ( std::uint64_t( 1 ) << 30 ) - 1 ) );
 pos += 30;
 return( w );
 }

inline void RearCodedDictionary::putLength( const Code & c , std::size_t v )
{
 if( v < 255 ) {
  f_C.append( c.word[ v ] , c.len[ v ] );
  return;
  }
 f_C.append( c.word[ 255 ] , c.len[ 255 ] );
 f_C.append( v , 30 );
 }

inline RearCodedDictionary::RearCodedDictionary(
                    const std::vector< std::string > & D , std::size_t b )
 : f_n( D.size() ) , f_b( b )
{
 ADS_PRE( b >= 1 );
 ADS_AUDIT( std::is_sorted( D.begin() , D.end() ) );
 // the prefix that all the strings share: the first string of a bucket
 // keeps its kHead bytes after it in f_head, and its code begins after
 // them
 if( f_n > 0 )
  f_common = D[ 0 ].substr( 0 , commonPrefixLength( D[ 0 ] , D.back() ) );
 const std::size_t g = f_common.size();
 auto from = [ & ]( std::size_t i ) {
  return( i % b ? commonPrefixLength( D[ i - 1 ] , D[ i ] )
                : std::min( D[ i ].size() , g + kHead ) );
  };
 // the frequencies of the bytes of the characters, of the rears and of
 // the lengths (255 an escape for the larger ones): of the rest for the
 // strings in a bucket, whole for the first
 std::array< std::uint64_t , 256 > fc{} , fr{} , fs{};
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  const std::string & s = D[ i ];
  ADS_PRE( s.size() < ( std::size_t( 1 ) << 30 ) );
  const std::size_t l = from( i );
  if( i % b ) ++fr[ std::min< std::size_t >( D[ i - 1 ].size() - l , 255 ) ];
  ++fs[ std::min< std::size_t >( i % b ? s.size() - l : s.size() , 255 ) ];
  for( std::size_t k = l ; k < s.size() ; ++k )
   ++fc[ (unsigned char) s[ k ] ];
  }
 f_char.build( fc );
 f_rear.build( fr );
 f_size.build( fs );
 std::vector< std::uint64_t > P;
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  const std::string & s = D[ i ];
  const std::size_t l = from( i );
  if( i % b == 0 ) {                        // the first of a bucket
   P.push_back( f_C.size() );
   for( std::size_t j = g ; j < g + kHead ; j += 8 )
    f_head.push_back( prefix8( std::string_view( s ).substr(
                                 std::min( j , s.size() ) ) ) );
   putLength( f_size , s.size() );
   }
  else {
   putLength( f_rear , D[ i - 1 ].size() - l );
   putLength( f_size , s.size() - l );
   }
  for( std::size_t k = l ; k < s.size() ; ++k )
   f_C.append( f_char.word[ (unsigned char) s[ k ] ] ,
               f_char.len[ (unsigned char) s[ k ] ] );
  }
 f_P = EliasFano( P , f_C.size() + 1 );
 f_C.append( 0 , 64 );                      // so that a peek never ends
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t i = 0 ; i < f_n ; ++i )
   if( access( i ) != D[ i ] ) return( false );
  return( true ); }() );
 }

inline std::uint64_t RearCodedDictionary::prefix8( std::string_view s )
{
 std::uint64_t v = 0;
 for( std::size_t k = 0 ; k < 8 ; ++k )
  v = ( v << 8 ) | ( k < s.size() ? (unsigned char) s[ k ] : 0 );
 return( v );
 }

inline unsigned RearCodedDictionary::byte( std::size_t k ,
                                           std::size_t j ) const
{
 return( unsigned( f_head[ kHead / 8 * k + j / 8 ] >>
                   ( 56 - 8 * ( j % 8 ) ) ) & 255 );
 }

inline std::size_t RearCodedDictionary::size( void ) const
{
 return( f_n );
 }

inline bool RearCodedDictionary::empty( void ) const
{
 return( f_n == 0 );
 }

inline std::size_t RearCodedDictionary::bucketSize( void ) const
{
 return( f_b );
 }

inline void RearCodedDictionary::readRest( std::string & s ,
                                           std::size_t len ,
                                           std::size_t & pos ) const
{
 // up to 3 characters a lookup of kWide bits, while 3 are left at least
 for( ; len >= 3 ; ) {
  const std::uint32_t e = f_char.multi[ std::size_t( peek( pos ) ) &
                                    ( ( std::size_t( 1 ) << kWide ) - 1 ) ];
  const std::size_t count = ( e >> 24 ) & 3;
  for( std::size_t k = 0 ; k < count ; ++k )
   s.push_back( char( ( e >> ( 8 * k ) ) & 255 ) );
  pos += e >> 26;
  len -= count;
  }
 for( ; len > 0 ; --len )
  s.push_back( char( decode( f_char , pos ) ) );
 }

template< bool S >
inline unsigned RearCodedDictionary::follow( std::string_view Q ,
                                             std::size_t & l ,
                                             std::size_t a , std::size_t n ,
                                             std::size_t & pos ) const
{
 bool live = ( l == a );
 std::size_t k = a;                       // the place of the next character
 const std::size_t e = a + n;
 unsigned d = 256;
 // whether the character c at k ends the comparison
 auto stops = [ & ]( unsigned c ) {
  if( ( k < Q.size() ) && ( c == (unsigned char) Q[ k ] ) ) {
   l = k + 1;
   return( false );
   }
  if( k < Q.size() ) d = c;
  live = false;
  return( true );
  };
 // up to 3 characters a lookup of kWide bits, while 3 are left at least
 for( ; k + 3 <= e ; ) {
  const std::uint32_t w = f_char.multi[ std::size_t( peek( pos ) ) &
                                    ( ( std::size_t( 1 ) << kWide ) - 1 ) ];
  const std::size_t count = ( w >> 24 ) & 3;
  if( live ) {
   for( std::size_t t = 0 ; t < count ; ++t , ++k )
    if( stops( ( w >> ( 8 * t ) ) & 255 ) ) {
     if constexpr( ! S )
      return( d );
     k += count - t;
     break;
     }
   }
  else
   k += count;
  pos += w >> 26;
  }
 for( ; k < e ; ++k ) {
  const unsigned c = decode( f_char , pos );
  if( live && stops( c ) && ! S )
   return( d );
  }
 return( d );
 }

inline std::string RearCodedDictionary::head( std::size_t k ,
                                              std::size_t & pos ) const
{
 pos = std::size_t( f_P.access( k ) );
 const std::size_t len = getLength( f_size , pos );
 std::string s;
 s.reserve( len );
 s = f_common;
 for( std::size_t j = 0 ; ( j < kHead ) && ( s.size() < len ) ; ++j )
  s.push_back( char( byte( k , j ) ) );
 readRest( s , len - s.size() , pos );
 return( s );
 }

inline std::string RearCodedDictionary::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::size_t pos;
 std::string s = head( i / f_b , pos );
 for( std::size_t j = i - i % f_b ; j < i ; ++j ) {
  const std::size_t r = getLength( f_rear , pos );
  s.resize( s.size() - r );
  readRest( s , getLength( f_size , pos ) , pos );
  }
 return( s );
 }

inline std::size_t RearCodedDictionary::rank( std::string_view Q , bool upper ,
                                              bool * eq ) const
{
 // all the strings begin with f_common: if Q leaves it, all of them are
 // on one side of Q
 const std::size_t g = f_common.size();
 const std::size_t lq = commonPrefixLength( f_common , Q );
 if( lq < g ) {
  if( eq ) *eq = false;
  if( ( lq == Q.size() ) ? upper
                         : (unsigned char) f_common[ lq ] <
                           (unsigned char) Q[ lq ] )
   return( f_n );
  return( 0 );
  }
 // whether the first string of the bucket k precedes Q, by its kHead
 // bytes after the common prefix when they decide (a byte that differs,
 // past the bytes of Q as well, decides both orders, but for Q# whose
 // bytes past Q are masked), and by the rest of the string decoded, up to
 // where it differs from Q, otherwise
 const std::string_view R = Q.substr( g );
 constexpr std::size_t W = kHead / 8;
 std::array< std::uint64_t , W > q , m;
 for( std::size_t j = 0 ; j < W ; ++j ) {
  const std::size_t r = R.size() > 8 * j ? R.size() - 8 * j : 0;
  m[ j ] = ( ! upper ) || ( r >= 8 ) ? ~std::uint64_t( 0 )
           : r == 0 ? 0 : ~std::uint64_t( 0 ) << ( 64 - 8 * r );
  q[ j ] = prefix8( R.substr( std::min( 8 * j , R.size() ) ) ) & m[ j ];
  }
 auto before = [ & ]( std::size_t k ) {
  for( std::size_t j = 0 ; j < W ; ++j ) {
   const std::uint64_t h = f_head[ W * k + j ] & m[ j ];
   if( h != q[ j ] ) return( h < q[ j ] );
   }
  // the bytes in f_head that both strings have are equal: the code
  // goes on after them
  std::size_t pos = std::size_t( f_P.access( k ) );
  const std::size_t len = getLength( f_size , pos );
  std::size_t l = std::min( len , g + kHead );
  if( l <= Q.size() ) {
   const unsigned d = follow< false >( Q , l , l , len - l , pos );
   if( d < 256 )
    return( d < (unsigned char) Q[ l ] );
   }
  return( upper || ( len < Q.size() ) );
  };
 const std::size_t B = f_head.size() / W;
 // the heads that precede Q, by a binary search whose two next probes
 // are prefetched while one is compared
 std::size_t lo = 0 , pos;
 if( B > 0 ) {
  for( std::size_t n = B ; n > 1 ; ) {
   const std::size_t half = n / 2;
#if defined( __GNUC__ ) || defined( __clang__ )
   __builtin_prefetch( f_head.data() + W * ( lo + half / 2 ) );
   __builtin_prefetch( f_head.data() + W * ( lo + half + half / 2 ) );
#endif
   lo = before( lo + half ) ? lo + half : lo;
   n -= half;
   }
  lo += before( lo ) ? 1 : 0;
  }
 // the head of the bucket lo, the first that does not precede Q
 auto atHead = [ & ]( std::size_t r ) {
  if( eq )
   *eq = ( lo < B ) && ( head( lo , pos ) == Q );
  return( r );
  };
 if( lo == 0 )
  return( atHead( 0 ) );
 const std::size_t end = std::min( lo * f_b , f_n );
 std::size_t i = ( lo - 1 ) * f_b;
 // the first string of the bucket lo - 1, of length len, shares l
 // characters with Q: f_common, those in f_head, and those of its code
 pos = std::size_t( f_P.access( lo - 1 ) );
 std::size_t len = getLength( f_size , pos ) , l = g;
 const std::size_t a = std::min( len , g + kHead );
 while( ( l < a ) && ( l < Q.size() ) &&
        ( byte( lo - 1 , l - g ) == (unsigned char) Q[ l ] ) )
  ++l;
 (void) follow< true >( Q , l , a , len - a , pos );
 // the string precedes Q here; a next string that keeps more than l of
 // its characters has its character at l, and precedes Q as it does;
 // one that keeps fewer is compared with Q from where it goes on
 for( ; i + 1 < end ; ++i ) {
  const std::size_t keep = len - getLength( f_rear , pos );
  const std::size_t n = getLength( f_size , pos );
  len = keep + n;
  const bool live = ( keep <= l );
  if( live )
   l = keep;
  const unsigned d = follow< true >( Q , l , keep , n , pos );
  if( live && ! ( d < 256 ? d < (unsigned char) Q[ l ]
                          : upper || ( len < Q.size() ) ) ) {
   if( eq )
    *eq = ( l == Q.size() ) && ( len == Q.size() );
   return( i + 1 );
   }
  }
 return( atHead( end ) );
 }

inline std::size_t RearCodedDictionary::lexicographicPosition(
                                                   std::string_view Q ) const
{
 return( rank( Q , false ) );
 }

inline std::size_t RearCodedDictionary::lookup( std::string_view Q ) const
{
 bool eq = false;
 const std::size_t r = rank( Q , false , &eq );
 return( eq ? r : f_n );
 }

inline std::size_t RearCodedDictionary::prefixEnd( std::string_view P ) const
{
 return( rank( P , true ) );
 }

inline std::pair< std::size_t , std::size_t >
 RearCodedDictionary::prefixRange( std::string_view P ) const
{
 return( std::make_pair( rank( P , false ) , rank( P , true ) ) );
 }

inline std::size_t RearCodedDictionary::codeBytes( void ) const
{
 return( ( f_C.size() + 7 ) / 8 );
 }

inline std::size_t RearCodedDictionary::bytes( void ) const
{
 const std::size_t code = 3 * ( 256 * ( 1 + 4 ) +
                                ( std::size_t( 1 ) << kMax ) * 2 +
                                ( std::size_t( 1 ) << kWide ) * 4 );
 return( codeBytes() + ( f_P.sizeInBits() + 7 ) / 8 +
         f_head.size() * sizeof( std::uint64_t ) + f_common.size() + code );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RearCoding.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
