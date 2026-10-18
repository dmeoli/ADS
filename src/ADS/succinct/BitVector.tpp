/*--------------------------------------------------------------------------*/
/*--------------------------- File BitVector.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BitVector.h, with their contracts.
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

/// the mask of the len lowest bits
constexpr Word lowMask( int len )
{
 return( len >= 64 ? ~Word( 0 ) : ( Word( 1 ) << len ) - 1 );
 }

}  // end( namespace detail )

constexpr int selectInWord( Word x , int k )
{
 ADS_PRE( ( k >= 0 ) && ( k < sidewaysSum( x ) ) );
 constexpr Word L8 = 0x0101010101010101ULL;
 constexpr Word H8 = 0x8080808080808080ULL;
 Word s = x - ( ( x >> 1 ) & 0x5555555555555555ULL );
 s = ( s & 0x3333333333333333ULL ) + ( ( s >> 2 ) & 0x3333333333333333ULL );
 s = ( ( s + ( s >> 4 ) ) & 0x0F0F0F0F0F0F0F0FULL ) * L8;   // byte sums
 // the bytes whose sum is above k get their high bit, in parallel
 const Word b = ( s + ( Word( 127 - k ) * L8 ) ) & H8;
 const int byte = ruler( b ) / 8;
 int r = k - int( ( ( s << 8 ) >> ( 8 * byte ) ) & 0xFF );   // 1s before it
 Word v = ( x >> ( 8 * byte ) ) & 0xFF;
 for( ; r > 0 ; --r ) v &= v - 1;           // the lower 1s cleared
 return( 8 * byte + ruler( v ) );
 }

inline BitVector::BitVector( std::size_t n , bool b )
 : f_w( ( n + 63 ) / 64 , b ? ~Word( 0 ) : 0 ) , f_n( n )
{
 if( b && ( n % 64 ) ) f_w.back() &= detail::lowMask( int( n % 64 ) );
 ADS_INVARIANT( clean() );
 }

inline BitVector::BitVector( std::string_view s )
{
 reserve( s.size() );
 for( const char c : s ) {
  ADS_PRE( ( c == '0' ) || ( c == '1' ) );
  push_back( c == '1' );
  }
 }

inline std::size_t BitVector::size( void ) const
{
 return( f_n );
 }

inline bool BitVector::empty( void ) const
{
 return( f_n == 0 );
 }

inline bool BitVector::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 return( ( f_w[ i / 64 ] >> ( i % 64 ) ) & 1 );
 }

inline void BitVector::set( std::size_t i , bool b )
{
 ADS_PRE( i < f_n );
 const Word m = Word( 1 ) << ( i % 64 );
 if( b ) f_w[ i / 64 ] |= m;
 else f_w[ i / 64 ] &= ~m;
 ADS_POST( ( *this )[ i ] == b );
 }

inline void BitVector::push_back( bool b )
{
 if( f_n % 64 == 0 ) f_w.push_back( 0 );
 if( b ) f_w.back() |= Word( 1 ) << ( f_n % 64 );
 ++f_n;
 ADS_POST( ( *this )[ f_n - 1 ] == b );
 }

inline void BitVector::append( Word v , int len )
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( ( len == 64 ) || ( ( v >> len ) == 0 ) );
 if( len == 0 ) return;
 const int o = int( f_n % 64 );
 if( o == 0 ) f_w.push_back( v );
 else {
  f_w.back() |= v << o;
  if( o + len > 64 ) f_w.push_back( v >> ( 64 - o ) );
  }
 f_n += std::size_t( len );
 ADS_POST( get( f_n - std::size_t( len ) , len ) == v );
 ADS_INVARIANT( clean() );
 }

inline Word BitVector::get( std::size_t i , int len ) const
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( i + std::size_t( len ) <= f_n );
 if( len == 0 ) return( 0 );
 const std::size_t w = i / 64;
 const int o = int( i % 64 );
 Word r = f_w[ w ] >> o;
 if( o + len > 64 ) r |= f_w[ w + 1 ] << ( 64 - o );
 return( r & detail::lowMask( len ) );
 }

inline void BitVector::put( std::size_t i , Word v , int len )
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( ( len == 64 ) || ( ( v >> len ) == 0 ) );
 ADS_PRE( i + std::size_t( len ) <= f_n );
 if( len == 0 ) return;
 const std::size_t w = i / 64;
 const int o = int( i % 64 );
 const Word m = detail::lowMask( len );
 f_w[ w ] = ( f_w[ w ] & ~( m << o ) ) | ( v << o );
 if( o + len > 64 ) {
  const int h = 64 - o;                     // the bits in the first word
  f_w[ w + 1 ] = ( f_w[ w + 1 ] & ~( m >> h ) ) | ( v >> h );
  }
 ADS_POST( get( i , len ) == v );
 }

inline std::size_t BitVector::count( void ) const
{
 std::size_t c = 0;
 for( const Word w : f_w ) c += std::size_t( sidewaysSum( w ) );
 return( c );
 }

inline const std::vector< Word > & BitVector::words( void ) const
{
 return( f_w );
 }

inline std::string BitVector::toString( void ) const
{
 std::string s( f_n , '0' );
 for( std::size_t i = 0 ; i < f_n ; ++i )
  if( ( *this )[ i ] ) s[ i ] = '1';
 return( s );
 }

inline void BitVector::reserve( std::size_t n )
{
 f_w.reserve( ( n + 63 ) / 64 );
 }

inline void BitVector::clear( void )
{
 f_w.clear();
 f_n = 0;
 }

inline bool BitVector::clean( void ) const
{
 if( f_w.size() != ( f_n + 63 ) / 64 ) return( false );
 return( ( f_n % 64 == 0 ) ||
         ( ( f_w.back() & ~detail::lowMask( int( f_n % 64 ) ) ) == 0 ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File BitVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
