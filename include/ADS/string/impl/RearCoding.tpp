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

inline RearCodedDictionary::RearCodedDictionary(
                    const std::vector< std::string > & D , std::size_t b )
 : f_n( D.size() ) , f_b( b )
{
 ADS_PRE( b >= 1 );
 ADS_AUDIT( std::is_sorted( D.begin() , D.end() ) );
 std::vector< std::uint64_t > P;
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  const std::string & s = D[ i ];
  ADS_PRE( s.size() < ( std::size_t( 1 ) << 30 ) );
  if( i % b == 0 ) {                        // the first of a bucket, whole
   P.push_back( f_C.size() );
   encodeLength( std::uint32_t( s.size() ) , f_C );
   f_C += s;
   }
  else {
   const std::size_t l = commonPrefixLength( D[ i - 1 ] , s );
   encodeLength( std::uint32_t( D[ i - 1 ].size() - l ) , f_C );
   encodeLength( std::uint32_t( s.size() - l ) , f_C );
   f_C.append( s , l , std::string::npos );
   }
  }
 f_P = EliasFano( P , f_C.size() + 1 );
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t i = 0 ; i < f_n ; ++i )
   if( access( i ) != D[ i ] ) return( false );
  return( true ); }() );
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

inline std::string_view RearCodedDictionary::head( std::size_t k ,
                                                   std::size_t & pos ) const
{
 pos = std::size_t( f_P.access( k ) );
 const std::size_t len = decodeLength( f_C , pos );
 const std::string_view h( f_C.data() + pos , len );
 pos += len;
 return( h );
 }

inline std::string RearCodedDictionary::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::size_t pos;
 std::string s( head( i / f_b , pos ) );
 for( std::size_t j = i - i % f_b ; j < i ; ++j ) {
  const std::size_t r = decodeLength( f_C , pos );
  const std::size_t len = decodeLength( f_C , pos );
  s.resize( s.size() - r );
  s.append( f_C , pos , len );
  pos += len;
  }
 return( s );
 }

inline std::size_t RearCodedDictionary::rank( std::string_view Q , bool upper ,
                                              bool * eq ) const
{
 // whether s precedes Q (or Q#), from the prefix l that they share
 auto precedes = [ & ]( std::string_view s , std::size_t l ) {
  if( ( l < s.size() ) && ( l < Q.size() ) )
   return( (unsigned char) s[ l ] < (unsigned char) Q[ l ] );
  return( upper || ( s.size() < Q.size() ) );
  };
 const std::size_t B = ( f_n + f_b - 1 ) / f_b;
 std::size_t lo = 0 , hi = B , pos;          // the heads that precede Q
 while( lo < hi ) {
  const std::size_t mid = lo + ( hi - lo ) / 2;
  const std::string_view h = head( mid , pos );
  if( precedes( h , commonPrefixLength( h , Q ) ) ) lo = mid + 1;
  else hi = mid;
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
 std::string s( head( lo - 1 , pos ) );
 std::size_t l = commonPrefixLength( s , Q );
 // s precedes Q here; a next string that keeps more than l characters of
 // s has its character at l, and precedes Q as s does
 for( ; i + 1 < end ; ++i ) {
  const std::size_t r = decodeLength( f_C , pos );
  const std::size_t len = decodeLength( f_C , pos );
  const std::size_t keep = s.size() - r;
  const std::string_view rest( f_C.data() + pos , len );
  pos += len;
  s.resize( keep );
  s += rest;
  if( keep <= l ) {
   l = keep + commonPrefixLength( rest , Q.substr( std::min( keep ,
                                                             Q.size() ) ) );
   if( ! precedes( s , l ) ) {
    if( eq )
     *eq = ( l == Q.size() ) && ( s.size() == Q.size() );
    return( i + 1 );
    }
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
 return( f_C.size() );
 }

inline std::size_t RearCodedDictionary::bytes( void ) const
{
 return( f_C.size() + ( f_P.sizeInBits() + 7 ) / 8 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RearCoding.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
