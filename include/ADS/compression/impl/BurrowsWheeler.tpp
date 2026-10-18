/*--------------------------------------------------------------------------*/
/*------------------------ File BurrowsWheeler.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BurrowsWheeler.h, with their contracts.
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

/// the suffix array of s$, $ smaller than every symbol, by prefix
/// doubling: SA[ 0 ] = n, the suffix $
inline std::vector< std::size_t > sentinelSuffixArray( std::string_view s )
{
 const std::size_t n1 = s.size() + 1;
 std::vector< std::size_t > sa( n1 ) , rank( n1 ) , tmp( n1 );
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  rank[ i ] = std::size_t( static_cast< unsigned char >( s[ i ] ) ) + 1;
 rank[ s.size() ] = 0;
 std::iota( sa.begin() , sa.end() , std::size_t( 0 ) );
 if( n1 == 1 ) return( sa );
 for( std::size_t k = 1 ; ; k *= 2 ) {
  // the key of a suffix: the ranks of its two halves of length k, the
  // second 0 past the end (only the $ can reach it, and it is unique)
  auto key = [ & ]( std::size_t i ) {
   return( std::pair( rank[ i ] , i + k < n1 ? rank[ i + k ] + 1 : 0 ) );
   };
  std::sort( sa.begin() , sa.end() , [ & ]( std::size_t a , std::size_t b ) {
   return( key( a ) < key( b ) ); } );
  tmp[ sa[ 0 ] ] = 0;
  for( std::size_t i = 1 ; i < n1 ; ++i )
   tmp[ sa[ i ] ] = tmp[ sa[ i - 1 ] ] +
                    ( key( sa[ i - 1 ] ) < key( sa[ i ] ) ? 1 : 0 );
  rank.swap( tmp );
  if( rank[ sa[ n1 - 1 ] ] == n1 - 1 ) break;   // all ranks distinct
  }
 return( sa );
 }

/// the 256 bytes in their order
inline std::string allBytes( void )
{
 std::string a( 256 , '\0' );
 for( std::size_t c = 0 ; c < 256 ; ++c ) a[ c ] = static_cast< char >( c );
 return( a );
 }

/// the alphabet has distinct symbols
inline bool distinctSymbols( std::string_view alphabet )
{
 std::vector< bool > seen( 256 , false );
 for( char c : alphabet ) {
  const auto u = static_cast< unsigned char >( c );
  if( seen[ u ] ) return( false );
  seen[ u ] = true;
  }
 return( true );
 }

}  // end( namespace detail )

inline BWT bwt( std::string_view s )
{
 const auto sa = detail::sentinelSuffixArray( s );
 BWT b{ std::string() , 0 };
 b.last.reserve( s.size() );
 for( std::size_t i = 0 ; i < sa.size() ; ++i )
  if( sa[ i ] == 0 ) b.primary = i;
  else b.last += s[ sa[ i ] - 1 ];
 ADS_AUDIT( inverseBwt( b ) == s );
 return( b );
 }

inline std::vector< std::size_t > lfMapping( const BWT & b )
{
 const std::size_t n = b.last.size();
 ADS_PRE( ( n == 0 ) ? ( b.primary == 0 )
                     : ( ( b.primary >= 1 ) && ( b.primary <= n ) ) );
 // the symbol of row i of L, 0 for the $ and the byte + 1 otherwise
 auto sym = [ & ]( std::size_t i ) -> std::size_t {
  if( i == b.primary ) return( 0 );
  return( std::size_t( static_cast< unsigned char >(
                         b.last[ i < b.primary ? i : i - 1 ] ) ) + 1 );
  };
 std::vector< std::size_t > C( 258 , 0 ) , LF( n + 1 );
 for( std::size_t i = 0 ; i <= n ; ++i ) ++C[ sym( i ) + 1 ];
 for( std::size_t c = 1 ; c < C.size() ; ++c ) C[ c ] += C[ c - 1 ];
 for( std::size_t i = 0 ; i <= n ; ++i ) LF[ i ] = C[ sym( i ) ]++;
 return( LF );
 }

inline std::string inverseBwt( const BWT & b )
{
 const auto LF = lfMapping( b );
 const std::size_t n = b.last.size();
 std::string s( n , '\0' );
 std::size_t k = 0;                        // the row $s
 for( std::size_t i = n ; i-- > 0 ; ) {
  ADS_PRE( k != b.primary );               // the $ only at the end
  s[ i ] = b.last[ k < b.primary ? k : k - 1 ];
  k = LF[ k ];
  }
 ADS_PRE( k == b.primary );
 return( s );
 }

inline std::vector< std::uint8_t > moveToFront( std::string_view s ,
                                                std::string_view alphabet )
{
 ADS_PRE( detail::distinctSymbols( alphabet ) );
 std::string L( alphabet );
 std::vector< std::uint8_t > x;
 x.reserve( s.size() );
 for( char c : s ) {
  const std::size_t p = L.find( c );
  ADS_PRE( p != std::string::npos );
  x.push_back( std::uint8_t( p ) );
  std::rotate( L.begin() , L.begin() + std::ptrdiff_t( p ) ,
               L.begin() + std::ptrdiff_t( p ) + 1 );
  }
 ADS_AUDIT( inverseMoveToFront( x , alphabet ) == s );
 return( x );
 }

inline std::vector< std::uint8_t > moveToFront( std::string_view s )
{
 return( moveToFront( s , detail::allBytes() ) );
 }

inline std::string inverseMoveToFront( const std::vector< std::uint8_t > & x ,
                                       std::string_view alphabet )
{
 ADS_PRE( detail::distinctSymbols( alphabet ) );
 std::string L( alphabet ) , s;
 s.reserve( x.size() );
 for( std::uint8_t p : x ) {
  ADS_PRE( p < L.size() );
  s += L[ p ];
  std::rotate( L.begin() , L.begin() + p , L.begin() + p + 1 );
  }
 return( s );
 }

inline std::string inverseMoveToFront( const std::vector< std::uint8_t > & x )
{
 return( inverseMoveToFront( x , detail::allBytes() ) );
 }

inline std::vector< std::pair< std::size_t , char > > runLengthEncode(
                                                          std::string_view s )
{
 std::vector< std::pair< std::size_t , char > > R;
 for( std::size_t i = 0 ; i < s.size() ; ) {
  std::size_t j = i;
  while( ( j < s.size() ) && ( s[ j ] == s[ i ] ) ) ++j;
  R.push_back( std::pair( j - i , s[ i ] ) );
  i = j;
  }
 ADS_AUDIT( runLengthDecode( R ) == s );
 return( R );
 }

inline std::string runLengthDecode(
                     const std::vector< std::pair< std::size_t , char > > & R )
{
 std::string s;
 for( const auto & [ l , c ] : R ) {
  ADS_PRE( l > 0 );
  s.append( l , c );
  }
 return( s );
 }

inline std::vector< std::uint32_t > zeroRunEncode(
                                        const std::vector< std::uint8_t > & x )
{
 std::vector< std::uint32_t > y;
 for( std::size_t i = 0 ; i < x.size() ; ) {
  if( x[ i ] != 0 ) {
   y.push_back( std::uint32_t( x[ i++ ] ) + 1 );
   continue;
   }
  std::size_t j = i;
  while( ( j < x.size() ) && ( x[ j ] == 0 ) ) ++j;
  const std::uint64_t v = std::uint64_t( j - i ) + 1;   // the code of Wheeler
  for( int b = int( std::bit_width( v ) ) - 2 ; b >= 0 ; --b )
   y.push_back( std::uint32_t( ( v >> b ) & 1u ) );
  i = j;
  }
 ADS_AUDIT( zeroRunDecode( y ) == x );
 return( y );
 }

inline std::vector< std::uint8_t > zeroRunDecode(
                                       const std::vector< std::uint32_t > & y )
{
 std::vector< std::uint8_t > x;
 for( std::size_t i = 0 ; i < y.size() ; ) {
  ADS_PRE( y[ i ] <= 256 );
  if( y[ i ] >= 2 ) {
   x.push_back( std::uint8_t( y[ i++ ] - 1 ) );
   continue;
   }
  std::uint64_t v = 1;                      // the leading 1
  for( ; ( i < y.size() ) && ( y[ i ] < 2 ) ; ++i ) v = 2 * v + y[ i ];
  x.insert( x.end() , std::size_t( v - 1 ) , std::uint8_t( 0 ) );
  }
 return( x );
 }

inline std::vector< std::uint8_t > bzipCompress( std::string_view s ,
                                                 std::size_t block )
{
 ADS_PRE( block > 0 );
 constexpr int maxLength = 20 , lengthBits = 5;
 PackedBitWriter W;
 W.put( std::uint64_t( s.size() ) , 64 );
 for( std::size_t start = 0 ; start < s.size() ; start += block ) {
  const auto t = s.substr( start , block );
  const BWT b = bwt( t );
  const auto y = zeroRunEncode( moveToFront( b.last ) );
  std::map< std::uint32_t , std::uint64_t > freq;
  for( std::uint32_t v : y ) ++freq[ v ];
  const CanonicalHuffman< std::uint32_t > H( limitedLengths( freq ,
                                                             maxLength ) );
  W.put( std::uint64_t( t.size() ) , 64 );
  W.put( std::uint64_t( b.primary ) , 64 );
  W.put( std::uint64_t( y.size() ) , 64 );
  for( std::uint32_t c = 0 ; c <= 256 ; ++c ) {   // the preamble
   const auto it = H.lengths().find( c );
   W.put( std::uint64_t( it == H.lengths().end() ? 0 : it->second ) ,
          lengthBits );
   }
  for( std::uint32_t v : y ) H.encode( v , W );
  }
 ADS_AUDIT( bzipDecompress( W.bytes() ) == s );
 return( W.bytes() );
 }

inline std::string bzipDecompress( std::span< const std::uint8_t > code )
{
 constexpr int lengthBits = 5;
 PackedBitReader R( code );
 const std::uint64_t n = R.get( 64 );
 std::string s;
 while( s.size() < n ) {
  const std::size_t m = std::size_t( R.get( 64 ) );
  const std::size_t primary = std::size_t( R.get( 64 ) );
  const std::size_t k = std::size_t( R.get( 64 ) );
  std::map< std::uint32_t , int > len;
  for( std::uint32_t c = 0 ; c <= 256 ; ++c )
   if( const int l = int( R.get( lengthBits ) ) ; l > 0 ) len[ c ] = l;
  ADS_PRE( ( m > 0 ) && ( m <= n - s.size() ) && ! len.empty() );
  const CanonicalHuffman< std::uint32_t > H( len );
  std::vector< std::uint32_t > y;
  y.reserve( k );
  for( std::size_t i = 0 ; i < k ; ++i ) y.push_back( H.decode( R ) );
  const BWT b{ inverseMoveToFront( zeroRunDecode( y ) ) , primary };
  ADS_PRE( b.last.size() == m );
  s += inverseBwt( b );
  }
 ADS_PRE( R.position() <= R.size() );
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File BurrowsWheeler.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
