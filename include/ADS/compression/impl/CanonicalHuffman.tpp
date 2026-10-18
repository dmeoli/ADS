/*--------------------------------------------------------------------------*/
/*----------------------- File CanonicalHuffman.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CanonicalHuffman [see CanonicalHuffman.h], with their
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

template< class C , class F >
std::map< C , int > huffmanLengths( const std::map< C , F > & freq )
{
 std::map< C , int > len;
 for( const auto & [ c , s ] : huffman( freq ) )
  len[ c ] = int( s.size() );
 return( len );
 }

template< class C , class F >
std::map< C , int > limitedLengths( const std::map< C , F > & freq , int L )
{
 static_assert( std::is_integral_v< F > , "integer frequencies" );
 ADS_PRE( ! freq.empty() && ( L >= 1 ) && ( L <= 63 ) );
 ADS_PRE( ( L >= 62 ) ||
          ( freq.size() <= ( std::uint64_t( 1 ) << L ) ) );
 ADS_PRE( std::all_of( freq.begin() , freq.end() , []( const auto & e ) {
  return( e.second > 0 ); } ) );
 std::map< C , F > f = freq;
 for( ; ; ) {
  auto len = huffmanLengths( f );
  int max = 0;
  for( const auto & e : len ) max = std::max( max , e.second );
  if( max <= L ) return( len );
  for( auto & e : f ) e.second = e.second / 2 + e.second % 2;
  }
 }

template< class C >
CanonicalHuffman< C >::CanonicalHuffman( const std::map< C , int > & lengths )
 : f_len( lengths )
{
 ADS_PRE( ! lengths.empty() );
 const std::uint64_t one = std::uint64_t( 1 ) << 63;
 std::uint64_t kraft = 0;                  // sum of 2^( 63 - l )
 for( const auto & [ c , l ] : lengths ) {
  ADS_PRE( ( l >= 1 ) && ( l <= 63 ) );
  kraft += std::uint64_t( 1 ) << ( 63 - l );
  ADS_PRE( kraft <= one );
  f_max = std::max( f_max , l );
  }
 f_symb.assign( f_max + 1 , std::vector< C >() );
 for( const auto & [ c , l ] : lengths )   // in the order of the symbols
  f_symb[ l ].push_back( c );
 f_fc.assign( f_max + 2 , 0 );
 for( int l = f_max - 1 ; l >= 1 ; --l ) {
  const std::uint64_t next = f_fc[ l + 1 ] + f_symb[ l + 1 ].size();
  f_fc[ l ] = next / 2 + next % 2;
  }
 for( int l = 1 ; l <= f_max ; ++l )
  for( std::size_t i = 0 ; i < f_symb[ l ].size() ; ++i )
   f_code[ f_symb[ l ][ i ] ] = f_fc[ l ] + i;
 }

template< class C >
template< class F >
CanonicalHuffman< C > CanonicalHuffman< C >::fromFrequencies(
                                            const std::map< C , F > & freq )
{
 return( CanonicalHuffman( huffmanLengths( freq ) ) );
 }

template< class C >
std::uint64_t CanonicalHuffman< C >::firstCode( int l ) const
{
 ADS_PRE( ( l >= 1 ) && ( l <= f_max ) );
 return( f_fc[ l ] );
 }

template< class C >
std::string CanonicalHuffman< C >::code( const C & c ) const
{
 ADS_PRE( f_len.count( c ) > 0 );
 const int l = f_len.at( c );
 const std::uint64_t v = f_code.at( c );
 std::string s( std::size_t( l ) , '0' );
 for( int i = 0 ; i < l ; ++i )
  if( ( v >> ( l - 1 - i ) ) & 1u ) s[ i ] = '1';
 return( s );
 }

template< class C >
void CanonicalHuffman< C >::encode( const C & c , PackedBitWriter & w ) const
{
 const auto it = f_code.find( c );
 ADS_PRE( it != f_code.end() );
 w.put( it->second , f_len.at( c ) );
 }

template< class C >
std::vector< std::uint8_t > CanonicalHuffman< C >::encode(
                                          std::span< const C > text ) const
{
 PackedBitWriter w;
 for( const C & c : text ) encode( c , w );
 ADS_AUDIT( std::ranges::equal( decode( w.bytes() , text.size() ) ,
                                text ) );
 return( w.bytes() );
 }

template< class C >
C CanonicalHuffman< C >::decode( PackedBitReader & r ) const
{
 std::uint64_t v = std::uint64_t( r.get() );
 int l = 1;
 while( v < f_fc[ l ] ) {                 // a prefix of a longer code
  ADS_PRE( l < f_max );
  v = 2 * v + std::uint64_t( r.get() );
  ++l;
  }
 ADS_PRE( v - f_fc[ l ] < f_symb[ l ].size() );
 return( f_symb[ l ][ v - f_fc[ l ] ] );
 }

template< class C >
std::vector< C > CanonicalHuffman< C >::decode(
          const std::vector< std::uint8_t > & bits , std::size_t n ) const
{
 PackedBitReader r( bits );
 std::vector< C > text;
 text.reserve( n );
 for( std::size_t i = 0 ; i < n ; ++i ) text.push_back( decode( r ) );
 ADS_PRE( r.position() <= r.size() );       // no bit past the end
 return( text );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File CanonicalHuffman.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
