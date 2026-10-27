/*--------------------------------------------------------------------------*/
/*---------------------- File SuccinctPatriciaTrie.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuccinctPatriciaTrie.h, with their contracts.
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
#include <bit>
#include <cstdint>

namespace ADS {

inline SuccinctPatriciaTrie::SuccinctPatriciaTrie( void )
 : SuccinctPatriciaTrie( std::vector< std::string >() )
{
 }

inline SuccinctPatriciaTrie::SuccinctPatriciaTrie(
                                       const std::vector< std::string > & D )
 : f_n( D.size() )
{
 ADS_AUDIT( std::adjacent_find( D.begin() , D.end() ,
                                []( const std::string & a ,
                                    const std::string & b ) {
                                 return( !( a < b ) ); } ) == D.end() );
 // a node: its strings [ l , r ), the depth h of its end, and the rest of
 // its label, s_l[ a , a + len )
 struct Item {
  std::size_t l , r , h , a , len;
  };
 BitVector bp , end , hasRest;
 std::vector< std::uint64_t > at;
 bp.push_back( true );
 std::vector< Item > S{ Item{ 0 , f_n , 0 , 0 , 0 } } , kids;
 while( ! S.empty() ) {
  const Item it = S.back();
  S.pop_back();
  hasRest.push_back( it.len > 0 );
  if( it.len > 0 ) {                        // its length, 7 bits a byte
   ADS_PRE( it.len < ( std::size_t( 1 ) << 30 ) );
   at.push_back( f_rest.size() );
   for( std::size_t x = it.len ; ; x >>= 7 )
    if( x < 128 ) {
     f_rest.push_back( char( x ) );
     break;
     }
    else f_rest.push_back( char( 128 | ( x & 127 ) ) );
   f_rest.append( D[ it.l ] , it.a , it.len );
   }
  const bool t = ( it.l < it.r ) && ( D[ it.l ].size() == it.h );
  end.push_back( t );
  // the runs of the same character at h, sorted since the strings share
  // their first h characters
  kids.clear();
  for( std::size_t i = it.l + ( t ? 1 : 0 ) ; i < it.r ; ) {
   const unsigned char c = (unsigned char) D[ i ][ it.h ];
   const std::size_t j = std::size_t( std::upper_bound(
    D.begin() + long( i ) , D.begin() + long( it.r ) , c ,
    [ & ]( unsigned char x , const std::string & s ) {
     return( x < (unsigned char) s[ it.h ] ); } ) - D.begin() );
   const std::size_t h = j == i + 1 ? D[ i ].size()
                         : it.h + commonPrefixLength(
                            std::string_view( D[ i ] ).substr( it.h ) ,
                            std::string_view( D[ j - 1 ] ).substr( it.h ) );
   kids.push_back( Item{ i , j , h , it.h + 1 , h - it.h - 1 } );
   f_first.push_back( char( c ) );
   bp.push_back( true );
   i = j;
   }
  bp.push_back( false );
  for( std::size_t k = kids.size() ; k-- > 0 ; )
   S.push_back( kids[ k ] );
  }
 f_bp = BalancedParentheses( std::move( bp ) );
 f_end = CompactRankSelect( std::move( end ) );
 f_hasRest = CompactRankSelect( std::move( hasRest ) );
 f_restAt = EliasFano( at , f_rest.size() + 1 );
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t i = 0 ; i < f_n ; ++i )
   if( ( access( i ) != D[ i ] ) || ( lookup( D[ i ] ) != i ) )
    return( false );
  return( true ); }() );
 }

inline std::size_t SuccinctPatriciaTrie::size( void ) const
{
 return( f_n );
 }

inline bool SuccinctPatriciaTrie::empty( void ) const
{
 return( f_n == 0 );
 }

inline std::size_t SuccinctPatriciaTrie::nodes( void ) const
{
 return( f_end.size() );
 }

inline std::size_t SuccinctPatriciaTrie::node( std::size_t p ) const
{
 return( p - f_bp.bits().rank1( p ) );
 }

inline std::size_t SuccinctPatriciaTrie::place( std::size_t v ) const
{
 return( v == 0 ? 1 : f_bp.bits().select0( v ) + 1 );
 }

inline std::string_view SuccinctPatriciaTrie::rest( std::size_t v ) const
{
 if( ! f_hasRest[ v ] )
  return( std::string_view() );
 std::size_t a = std::size_t( f_restAt.access( f_hasRest.rank1( v ) ) );
 std::size_t len = 0;
 for( int sh = 0 ; ; sh += 7 ) {            // its length, 7 bits a byte
  const unsigned char b = (unsigned char) f_rest[ a++ ];
  len |= std::size_t( b & 127 ) << sh;
  if( b < 128 ) break;
  }
 return( std::string_view( f_rest ).substr( a , len ) );
 }

inline std::size_t SuccinctPatriciaTrie::close( std::size_t p ) const
{
 const auto & W = f_bp.bits().bits().words();
 std::size_t w = p / 64;
 std::uint64_t x = ~W[ w ] >> ( p % 64 );
 if( x )
  return( p + std::size_t( std::countr_zero( x ) ) );
 while( ! ( x = ~W[ ++w ] ) ) ;
 return( 64 * w + std::size_t( std::countr_zero( x ) ) );
 }

inline std::size_t SuccinctPatriciaTrie::child( std::size_t c ,
                                                std::size_t i ) const
{
 return( i == 0 ? c + 1 : f_bp.findClose( c - 1 - i ) + 1 );
 }

inline std::size_t SuccinctPatriciaTrie::subtreeEnd( std::size_t p ) const
{
 return( node( f_bp.forwardSearch( p - 1 , -1 ) + 1 ) );
 }

inline std::size_t SuccinctPatriciaTrie::lookup( std::string_view Q ) const
{
 std::size_t p = 1 , r1 = 1 , j = 0;      // r1 the open parentheses before p
 for( ; ; ) {
  const std::size_t v = p - r1;
  if( v > 0 ) {
   const std::string_view r = rest( v );
   if( ( Q.size() - j < r.size() ) || ( Q.compare( j , r.size() , r ) != 0 ) )
    return( f_n );
   j += r.size();
   }
  if( j == Q.size() )
   return( f_end[ v ] ? f_end.rank1( v ) : f_n );
  // the children of v: its open parentheses up to its closed one c, and
  // their first characters from the open parentheses before p on
  const std::size_t c = close( p );
  const unsigned char x = (unsigned char) Q[ j ];
  const char * F = f_first.data() + ( r1 - 1 );
  const std::size_t i = std::size_t( std::lower_bound(
   F , F + ( c - p ) , x , []( char a , unsigned char b ) {
    return( (unsigned char) a < b ); } ) - F );
  if( ( i == c - p ) || ( (unsigned char) F[ i ] != x ) )
   return( f_n );
  p = child( c , i );
  r1 = f_bp.bits().rank1( p );
  ++j;
  }
 }

inline bool SuccinctPatriciaTrie::contains( std::string_view Q ) const
{
 return( lookup( Q ) != f_n );
 }

inline std::string SuccinctPatriciaTrie::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::size_t v = f_end.select1( i + 1 ) , p = place( v );
 std::string s;                             // backwards
 while( v > 0 ) {
  const std::string_view r = rest( v );
  s.append( r.rbegin() , r.rend() );
  const std::size_t q = f_bp.findOpen( p - 1 );   // in the parent
  const std::size_t u = node( q ) , pu = place( u );
  const std::size_t c = close( pu );
  s.push_back( f_first[ f_bp.bits().rank1( pu ) - 1 + ( c - 1 - q ) ] );
  v = u;
  p = pu;
  }
 std::reverse( s.begin() , s.end() );
 return( s );
 }

inline std::pair< std::size_t , std::size_t >
 SuccinctPatriciaTrie::walk( std::string_view P ) const
{
 std::size_t p = 1 , r1 = 1 , j = 0;      // r1 the open parentheses before p
 for( ; ; ) {
  const std::size_t v = p - r1;
  if( v > 0 ) {
   const std::string_view r = rest( v );
   const std::size_t m = commonPrefixLength( r , P.substr( j ) );
   if( j + m == P.size() )                  // P ends on the edge into v
    return( std::make_pair( f_end.rank1( v ) ,
                            f_end.rank1( subtreeEnd( p ) ) ) );
   if( m < r.size() ) {                     // a mismatch on the edge
    const std::size_t x = (unsigned char) P[ j + m ] <
                          (unsigned char) r[ m ] ? f_end.rank1( v )
                          : f_end.rank1( subtreeEnd( p ) );
    return( std::make_pair( x , x ) );
    }
   j += r.size();
   }
  if( j == P.size() )
   return( std::make_pair( f_end.rank1( v ) ,
                           f_end.rank1( subtreeEnd( p ) ) ) );
  const std::size_t c = close( p );
  const unsigned char x = (unsigned char) P[ j ];
  const char * F = f_first.data() + ( r1 - 1 );
  const std::size_t i = std::size_t( std::lower_bound(
   F , F + ( c - p ) , x , []( char a , unsigned char b ) {
    return( (unsigned char) a < b ); } ) - F );
  if( ( i == c - p ) || ( (unsigned char) F[ i ] != x ) ) {
   // no child of the character: the strings before the subtree of the
   // first child with a larger one, or past the subtree of v
   const std::size_t e = i < c - p ? node( child( c , i ) ) : subtreeEnd( p );
   return( std::make_pair( f_end.rank1( e ) , f_end.rank1( e ) ) );
   }
  p = child( c , i );
  r1 = f_bp.bits().rank1( p );
  ++j;
  }
 }

inline std::size_t SuccinctPatriciaTrie::lexicographicPosition(
                                                   std::string_view Q ) const
{
 return( walk( Q ).first );
 }

inline std::pair< std::size_t , std::size_t >
 SuccinctPatriciaTrie::prefixRange( std::string_view P ) const
{
 return( walk( P ) );
 }

inline std::size_t SuccinctPatriciaTrie::bytes( void ) const
{
 const std::size_t bits = f_bp.size() + f_bp.bits().directoryBits() +
                          f_bp.treeBits() + f_end.size() +
                          f_end.directoryBits() + f_hasRest.size() +
                          f_hasRest.directoryBits() + f_restAt.sizeInBits();
 return( ( bits + 7 ) / 8 + f_first.size() + f_rest.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File SuccinctPatriciaTrie.tpp ------------------*/
/*--------------------------------------------------------------------------*/
