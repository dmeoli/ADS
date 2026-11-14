/*--------------------------------------------------------------------------*/
/*------------------------- File FrontCoding.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of FrontCoding.h, with their contracts.
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

inline std::vector< std::string > frontDecode(
                                        const std::vector< FrontPair > & F )
{
 std::vector< std::string > D;
 D.reserve( F.size() );
 for( std::size_t i = 0 ; i < F.size() ; ++i ) {
  ADS_PRE( F[ i ].first <= ( i ? D[ i - 1 ].size() : 0 ) );
  std::string s = i ? D[ i - 1 ].substr( 0 , F[ i ].first ) : std::string();
  s += F[ i ].second;
  D.push_back( std::move( s ) );
  }
 return( D );
 }

inline std::vector< FrontPair > frontCode(
                                        const std::vector< std::string > & D )
{
 std::vector< FrontPair > F;
 F.reserve( D.size() );
 for( std::size_t i = 0 ; i < D.size() ; ++i ) {
  const std::size_t l = i ? commonPrefixLength( D[ i - 1 ] , D[ i ] ) : 0;
  F.emplace_back( l , D[ i ].substr( l ) );
  }
 ADS_AUDIT( frontDecode( F ) == D );
 return( F );
 }

inline void encodeLength( std::uint32_t l , std::string & out )
{
 ADS_PRE( l < ( std::uint32_t( 1 ) << 30 ) );
 const std::size_t start = out.size();
 int k = 1;                                 // the bytes: l < 2^( 8 k - 2 )
 while( ( k < 4 ) && ( l >= ( std::uint32_t( 1 ) << ( 8 * k - 2 ) ) ) ) ++k;
 const std::uint32_t v = ( std::uint32_t( k - 1 ) << ( 8 * k - 2 ) ) | l;
 for( int j = k - 1 ; j >= 0 ; --j )        // the most significant first
  out.push_back( char( ( v >> ( 8 * j ) ) & 0xFF ) );
 ADS_AUDIT( [ & ]( void ) {
  std::size_t pos = start;
  return( ( decodeLength( out , pos ) == l ) && ( pos == out.size() ) ); }() );
 }

inline std::uint32_t decodeLength( std::string_view in , std::size_t & pos )
{
 ADS_PRE( pos < in.size() );
 const auto first = std::uint32_t( ( unsigned char )( in[ pos ] ) );
 const std::size_t k = ( first >> 6 ) + 1;
 ADS_PRE( pos + k <= in.size() );
 std::uint32_t v = first & 0x3F;
 for( std::size_t j = 1 ; j < k ; ++j )
  v = ( v << 8 ) | std::uint32_t( ( unsigned char )( in[ pos + j ] ) );
 pos += k;
 return( v );
 }

/*--------------------------------------------------------------------------*/

template< class A >
FrontCodedDictionary FrontCodedDictionary::build(
                       const std::vector< std::string > & D , A anchor )
{
 ADS_AUDIT( std::is_sorted( D.begin() , D.end() ) );
 FrontCodedDictionary F;
 const std::size_t n = D.size();
 F.f_l.resize( n );
 F.f_off.resize( n + 1 );
 std::size_t last = 0;                      // where the last anchor begins
 std::string code;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  std::size_t l = i ? commonPrefixLength( D[ i - 1 ] , D[ i ] ) : 0;
  if( anchor( i , l , F.f_chars.size() - last , D[ i ].size() ) ) {
   l = 0;
   F.f_a.push_back( i );
   last = F.f_chars.size();
   }
  F.f_l[ i ] = l;
  F.f_off[ i ] = F.f_chars.size();
  F.f_chars.append( D[ i ] , l , std::string::npos );
  code.clear();
  encodeLength( std::uint32_t( l ) , code );
  F.f_bytes += code.size() + D[ i ].size() - l;
  }
 F.f_off[ n ] = F.f_chars.size();
 ADS_AUDIT( std::equal( F.begin() , F.end() , D.begin() , D.end() ) );
 return( F );
 }

inline FrontCodedDictionary FrontCodedDictionary::bucketed(
                       const std::vector< std::string > & D , std::size_t b )
{
 ADS_PRE( b >= 1 );
 return( build( D , [ b ]( std::size_t i , std::size_t , std::size_t ,
                           std::size_t ) { return( i % b == 0 ); } ) );
 }

inline FrontCodedDictionary FrontCodedDictionary::localityPreserving(
                       const std::vector< std::string > & D , double c )
{
 ADS_PRE( c > 0 );
 FrontCodedDictionary F = build( D , [ c ]( std::size_t i , std::size_t l ,
                                            std::size_t chars ,
                                            std::size_t len ) {
  return( ( i == 0 ) || ( l == 0 ) ||       // the anchor too far behind
          ( double( chars ) > c * double( len ) ) ); } );
 ADS_POST( ( c <= 2 ) || [ & ]( void ) {
  double fc = 0;                            // FC( D )
  for( std::size_t i = 0 ; i < D.size() ; ++i )
   fc += double( D[ i ].size() -
                 ( i ? commonPrefixLength( D[ i - 1 ] , D[ i ] ) : 0 ) );
  if( double( F.characters() ) > ( 1 + 2 / ( c - 2 ) ) * fc + 1e-9 )
   return( false );
  for( std::size_t i = 0 ; i < D.size() ; ++i )
   if( double( F.decodeCost( i ) ) > ( c + 1 ) * double( D[ i ].size() ) )
    return( false );
  return( true ); }() );
 return( F );
 }

inline std::string_view FrontCodedDictionary::rest( std::size_t i ) const
{
 return( std::string_view( f_chars ).substr( f_off[ i ] ,
                                              f_off[ i + 1 ] - f_off[ i ] ) );
 }

inline std::size_t FrontCodedDictionary::anchorBefore( std::size_t i ) const
{
 return( *( std::upper_bound( f_a.begin() , f_a.end() , i ) - 1 ) );
 }

inline std::string FrontCodedDictionary::access( std::size_t i ) const
{
 ADS_PRE( i < size() );
 const std::size_t a = anchorBefore( i );
 std::string s( rest( a ) );
 for( std::size_t j = a + 1 ; j <= i ; ++j ) {   // the l characters only
  s.resize( f_l[ j ] );
  s.append( rest( j ) );
  }
 return( s );
 }

inline std::size_t FrontCodedDictionary::decodeCost( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( f_off[ i + 1 ] - f_off[ anchorBefore( i ) ] );
 }

inline std::size_t FrontCodedDictionary::anchorRank( std::string_view P ,
                                                     bool upper ) const
{
 std::size_t lo = 0 , hi = f_a.size();
 while( lo < hi ) {                         // the binary search on anchors
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( detail::precedes( anchorString( m ) , P , upper ) )
   lo = m + 1;
  else
   hi = m;
  }
 return( lo );
 }

inline std::size_t FrontCodedDictionary::rankInBucket( std::size_t k ,
                                                       std::string_view P ,
                                                       bool upper ) const
{
 ADS_PRE( k <= anchors() );
 if( k == 0 )
  return( 0 );
 const std::size_t a = f_a[ k - 1 ] ,
                   e = k < f_a.size() ? f_a[ k ] : size();
 std::string s( rest( a ) );
 std::size_t i = a;
 while( ( i < e ) && detail::precedes( s , P , upper ) )
  if( ++i < e ) {                           // decoded on the fly
   s.resize( f_l[ i ] );
   s.append( rest( i ) );
   }
 return( i );
 }

inline std::size_t FrontCodedDictionary::lexicographicPosition(
                                                  std::string_view Q ) const
{
 const std::size_t r = rankInBucket( anchorRank( Q , false ) , Q , false );
 ADS_POST( ( ( r == 0 ) || ( access( r - 1 ) < Q ) ) &&
           ( ( r == size() ) || ( Q <= access( r ) ) ) );
 return( r );
 }

inline std::size_t FrontCodedDictionary::prefixEnd( std::string_view P ) const
{
 const std::size_t r = rankInBucket( anchorRank( P , true ) , P , true );
 ADS_POST( ( ( r == 0 ) || detail::precedes( access( r - 1 ) , P , true ) ) &&
           ( ( r == size() ) ||
             ! detail::precedes( access( r ) , P , true ) ) );
 return( r );
 }

inline std::pair< std::size_t , std::size_t >
 FrontCodedDictionary::prefixRange( std::string_view P ) const
{
 return( std::pair( lexicographicPosition( P ) , prefixEnd( P ) ) );
 }

inline std::size_t FrontCodedDictionary::size( void ) const
{
 return( f_l.size() );
 }

inline std::size_t FrontCodedDictionary::anchors( void ) const
{
 return( f_a.size() );
 }

inline std::size_t FrontCodedDictionary::anchor( std::size_t k ) const
{
 ADS_PRE( k < anchors() );
 return( f_a[ k ] );
 }

inline std::string_view FrontCodedDictionary::anchorString(
                                                     std::size_t k ) const
{
 ADS_PRE( k < anchors() );
 return( rest( f_a[ k ] ) );                // an anchor is whole
 }

inline std::size_t FrontCodedDictionary::characters( void ) const
{
 return( f_chars.size() );
 }

inline std::size_t FrontCodedDictionary::bytes( void ) const
{
 return( f_bytes );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File FrontCoding.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
