/*--------------------------------------------------------------------------*/
/*--------------------------- File LempelZiv.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of LempelZiv.h, with their contracts.
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

namespace detail {

/// the closest among the longest prefixes of s[ p .. ] that start in the
/// window of the w symbols before p, as ( distance , length ); ( 0 , 0 )
/// if none
inline std::pair< std::size_t , std::size_t > longestCopy(
                       std::string_view s , std::size_t p , std::size_t w )
{
 const std::size_t lo = p - std::min( p , w );
 std::size_t best = 0 , dist = 0;
 for( std::size_t j = p ; j-- > lo ; ) {
  std::size_t l = 0;
  while( ( p + l < s.size() ) && ( s[ j + l ] == s[ p + l ] ) ) ++l;
  if( l > best ) {
   best = l;
   dist = p - j;
   }
  }
 return( std::pair( dist , best ) );
 }

}  // end( namespace detail )

inline std::vector< LZ77Phrase > lz77Parse( std::string_view s ,
                                            std::size_t w )
{
 ADS_PRE( w > 0 );
 std::vector< LZ77Phrase > P;
 for( std::size_t p = 0 ; p < s.size() ; ) {
  const auto [ d , l ] = detail::longestCopy( s , p , w );
  std::optional< char > next;
  if( p + l < s.size() ) next = s[ p + l ];
  P.push_back( LZ77Phrase{ d , l , next } );
  p += l + 1;
  }
 ADS_AUDIT( lz77Decode( P ) == s );
 return( P );
 }

inline std::string lz77Decode( const std::vector< LZ77Phrase > & P )
{
 std::string s;
 for( const auto & f : P ) {
  ADS_PRE( ( f.distance <= s.size() ) &&
           ( ( f.distance > 0 ) == ( f.length > 0 ) ) );
  const std::size_t start = s.size() - f.distance;
  for( std::size_t i = 0 ; i < f.length ; ++i )   // the overlap included
   s += s[ start + i ];
  if( f.next ) s += *f.next;
  }
 return( s );
 }

inline std::vector< LZSSPhrase > lzssParse( std::string_view s ,
                                            std::size_t w ,
                                            std::size_t minLength )
{
 ADS_PRE( ( w > 0 ) && ( minLength > 0 ) );
 std::vector< LZSSPhrase > P;
 for( std::size_t p = 0 ; p < s.size() ; ) {
  const auto [ d , l ] = detail::longestCopy( s , p , w );
  if( l >= minLength ) {
   P.push_back( LZSSPhrase{ d , l , '\0' } );
   p += l;
   }
  else P.push_back( LZSSPhrase{ 0 , 1 , s[ p++ ] } );
  }
 ADS_AUDIT( lzssDecode( P ) == s );
 return( P );
 }

inline std::vector< LZSSPhrase > lzssParseHashed( std::string_view s ,
                                                  std::size_t w ,
                                                  std::size_t minLength ,
                                                  std::size_t maxChain )
{
 ADS_PRE( ( w > 0 ) && ( minLength >= 3 ) && ( maxChain > 0 ) );
 constexpr std::size_t H = std::size_t( 1 ) << 15;
 constexpr std::size_t none = std::numeric_limits< std::size_t >::max();
 const std::size_t n = s.size();
 std::vector< std::size_t > head( H , none ) , prev( n , none );
 auto hash = [ & ]( std::size_t i ) {
  const auto u = []( char c ) {
   return( std::size_t( static_cast< unsigned char >( c ) ) ); };
  return( ( ( u( s[ i ] ) << 10 ) ^ ( u( s[ i + 1 ] ) << 5 ) ^
            u( s[ i + 2 ] ) ) & ( H - 1 ) );
  };
 std::vector< LZSSPhrase > P;
 std::size_t inserted = 0;                 // the positions in the chains
 for( std::size_t p = 0 ; p < n ; ) {
  for( ; ( inserted < p ) && ( inserted + 3 <= n ) ; ++inserted ) {
   const std::size_t h = hash( inserted );
   prev[ inserted ] = head[ h ];
   head[ h ] = inserted;
   }
  std::size_t best = 0 , dist = 0;
  if( p + 3 <= n ) {
   std::size_t chain = 0;
   for( std::size_t j = head[ hash( p ) ] ;
        ( j != none ) && ( p - j <= w ) && ( chain < maxChain ) ;
        j = prev[ j ] , ++chain ) {
    std::size_t l = 0;
    while( ( p + l < n ) && ( s[ j + l ] == s[ p + l ] ) ) ++l;
    if( l > best ) {
     best = l;
     dist = p - j;
     }
    }
   }
  if( best >= minLength ) {
   P.push_back( LZSSPhrase{ dist , best , '\0' } );
   p += best;
   }
  else P.push_back( LZSSPhrase{ 0 , 1 , s[ p++ ] } );
  }
 ADS_AUDIT( lzssDecode( P ) == s );
 ADS_AUDIT( ( maxChain != std::numeric_limits< std::size_t >::max() ) ||
            ( P == lzssParse( s , w , minLength ) ) );
 return( P );
 }

inline std::string lzssDecode( const std::vector< LZSSPhrase > & P )
{
 std::string s;
 for( const auto & f : P ) {
  if( f.distance == 0 ) {
   s += f.literal;
   continue;
   }
  ADS_PRE( f.distance <= s.size() );
  const std::size_t start = s.size() - f.distance;
  for( std::size_t i = 0 ; i < f.length ; ++i ) s += s[ start + i ];
  }
 return( s );
 }

inline std::vector< LZ78Phrase > lz78Parse( std::string_view s ,
                                            std::size_t maxPhrases )
{
 ADS_PRE( maxPhrases > 0 );
 std::vector< std::map< char , std::size_t > > child( 1 );   // the trie
 std::vector< std::size_t > parent( 1 , 0 );
 std::vector< char > last( 1 , '\0' );     // the symbol into each node
 std::vector< LZ78Phrase > P;
 for( std::size_t p = 0 ; p < s.size() ; ) {
  std::size_t x = 0;
  for( ; p < s.size() ; ++p ) {
   const auto it = child[ x ].find( s[ p ] );
   if( it == child[ x ].end() ) break;
   x = it->second;
   }
  if( p == s.size() ) {                    // the text ends inside x
   P.push_back( LZ78Phrase{ parent[ x ] , last[ x ] } );
   break;
   }
  P.push_back( LZ78Phrase{ x , s[ p ] } );
  child[ x ][ s[ p ] ] = child.size();
  child.emplace_back();
  parent.push_back( x );
  last.push_back( s[ p++ ] );
  if( child.size() - 1 == maxPhrases ) {   // the dictionary restarts
   child.assign( 1 , std::map< char , std::size_t >() );
   parent.assign( 1 , 0 );
   last.assign( 1 , '\0' );
   }
  }
 ADS_AUDIT( lz78Decode( P , maxPhrases ) == s );
 return( P );
 }

inline std::string lz78Decode( const std::vector< LZ78Phrase > & P ,
                               std::size_t maxPhrases )
{
 ADS_PRE( maxPhrases > 0 );
 std::vector< std::string > D( 1 );
 std::string s;
 for( const auto & f : P ) {
  ADS_PRE( f.id < D.size() );
  D.push_back( D[ f.id ] + f.next );
  s += D.back();
  if( D.size() - 1 == maxPhrases ) D.assign( 1 , std::string() );
  }
 return( s );
 }

inline std::vector< std::uint32_t > lzwEncode( std::string_view s )
{
 std::vector< std::uint32_t > I;
 if( s.empty() ) return( I );
 std::unordered_map< std::uint64_t , std::uint32_t > D;   // ( f , c ) -> fc
 std::uint32_t next = 256;
 std::uint32_t f = static_cast< unsigned char >( s[ 0 ] );
 for( std::size_t i = 1 ; i < s.size() ; ++i ) {
  const std::uint32_t c = static_cast< unsigned char >( s[ i ] );
  const std::uint64_t key = ( std::uint64_t( f ) << 8 ) | c;
  if( const auto it = D.find( key ) ; it != D.end() ) f = it->second;
  else {
   I.push_back( f );
   D[ key ] = next++;
   f = c;
   }
  }
 I.push_back( f );
 ADS_AUDIT( lzwDecode( I ) == s );
 return( I );
 }

inline std::string lzwDecode( const std::vector< std::uint32_t > & I )
{
 std::string s;
 if( I.empty() ) return( s );
 std::vector< std::string > D( 256 );
 for( std::size_t c = 0 ; c < 256 ; ++c )
  D[ c ] = std::string( 1 , static_cast< char >( c ) );
 ADS_PRE( I[ 0 ] < 256 );
 std::string f = D[ I[ 0 ] ];
 s = f;
 for( std::size_t k = 1 ; k < I.size() ; ++k ) {
  ADS_PRE( I[ k ] <= D.size() );
  const std::string g = I[ k ] < D.size() ? D[ I[ k ] ]
                                          : f + f[ 0 ];   // the late phrase
  s += g;
  D.push_back( f + g[ 0 ] );
  f = g;
  }
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File LempelZiv.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
