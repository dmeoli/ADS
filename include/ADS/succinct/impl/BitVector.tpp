/*--------------------------------------------------------------------------*/
/*--------------------------- File BitVector.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BitVector.h, with their contracts.
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

namespace ADS
{

namespace detail
{

/// the mask of the len lowest bits
constexpr Word lowMask( int len )
{
 return( len >= 64 ? ~Word( 0 ) : ( Word( 1 ) << len ) - 1 );
 }

/// the place of the 1 of rank r in the byte v at 8 v + r, 8 if v has no
/// such 1 (the table of Vigna, 2 KB)
inline constexpr std::array< std::uint8_t , 2048 > kSelectInByte = [] {
 std::array< std::uint8_t , 2048 > t{};
 for( unsigned v = 0 ; v < 256 ; ++v ) {
  unsigned r = 0;
  for( unsigned i = 0 ; i < 8 ; ++i )
   if( ( v >> i ) & 1 )
    t[ 8 * v + r++ ] = std::uint8_t( i );
  for( ; r < 8 ; ++r )
   t[ 8 * v + r ] = 8;
  }
 return( t );
 }();

}  // end( namespace detail )

constexpr int selectInWord( Word x , int k )
{
 ADS_PRE( ( k >= 0 ) && ( k < sidewaysSum( x ) ) );
 constexpr Word L8 = 0x0101010101010101ULL;
 constexpr Word H8 = 0x8080808080808080ULL;
 Word s = x - ( ( x >> 1 ) & 0x5555555555555555ULL );
 s = ( s & 0x3333333333333333ULL ) + ( ( s >> 2 ) & 0x3333333333333333ULL );
 s = ( ( s + ( s >> 4 ) ) & 0x0F0F0F0F0F0F0F0FULL ) * L8;  // byte sums
 // the bytes whose sum is above k get their high bit, in parallel
 const Word b = ( s + ( Word( 127 - k ) * L8 ) ) & H8;
 const int byte = ruler( b ) / 8;
 const int r = k - int( ( ( s << 8 ) >> ( 8 * byte ) ) & 0xFF );  // 1s before
 const unsigned v = unsigned( ( x >> ( 8 * byte ) ) & 0xFF );
 return( 8 * byte + detail::kSelectInByte[ 8 * v + unsigned( r ) ] );
 }

namespace detail
{

template< bool Scalar >
inline std::size_t lineRank( const Word * w , unsigned p )
{
 ADS_PRE( p < 512 );
#ifdef ADS_WITH_HIGHWAY
 if constexpr( ! Scalar )  // the 8 words at once [see SimdInline.h]
  return( simd::here::lineRank512( w , p ) );
#endif
 std::size_t r = 0;
 for( unsigned j = 0 ; j < 8 ; ++j ) {
  const auto c = unsigned( std::clamp< std::int64_t >(
    std::int64_t( p ) - 64 * std::int64_t( j ) , 0 , 64 ) );
  r += std::size_t( std::popcount( w[ j ] & lowMask( int( c ) ) ) );
  }
 return( r );
 }

inline std::uint64_t copyCount( Word * P , const Word * w , std::size_t n ,
				std::size_t w0 , std::size_t w1 )
{
 if( w ) {
  const std::size_t sw = ( n + 63 ) / 64;
  for( std::size_t i = w0 ; i < w1 ; ++i )
   P[ i ] = i < sw ? w[ i ] : 0;
  if( ( n % 64 ) && ( n / 64 >= w0 ) && ( n / 64 < w1 ) )
   P[ n / 64 ] &= lowMask( int( n % 64 ) );
  }
 std::uint64_t a = 0;
 for( std::size_t i = w0 ; i < w1 ; ++i )
  a += std::uint64_t( std::popcount( P[ i ] ) );
 return( a );
 }

inline std::size_t chunkUnits( std::size_t u , std::size_t chunk ,
			       std::size_t chunks , std::size_t top )
{
 const std::size_t cs = std::max( chunk , u / chunks + 1 );
 return( cs < top ? std::bit_ceil( cs ) : ( cs + top - 1 ) / top * top );
 }

template< RankSelectBits R >
inline std::size_t selected( [[maybe_unused]] const R & r ,
			     [[maybe_unused]] std::size_t k ,
			     [[maybe_unused]] bool b , std::size_t p )
{
 ADS_POST( ( r[ p ] == b ) &&
	   ( ( b ? r.rank1( p ) : p - r.rank1( p ) ) == k - 1 ) );
 return( p );
 }

template< RankSelectBits R >
inline bool rankConsistent( const R & r )
{
 std::size_t c = 0;
 for( std::size_t i = 0 ; i <= r.size() ; ++i ) {
  if( r.rank1( i ) != c )
   return( false );
  if( ( i < r.size() ) && r[ i ] )
   ++c;
  }
 return( c == r.ones() );
 }

}  // end( namespace detail )

inline BitVector::BitVector( std::size_t n , bool b )
    : f_w( wordsFor( n ) , 0 ) , f_n( n )
{
 if( b ) {
  std::fill( f_w.begin() , f_w.begin() + std::ptrdiff_t( n / 64 ) ,
	     ~Word( 0 ) );
  if( n % 64 )
   f_w[ n / 64 ] = detail::lowMask( int( n % 64 ) );
  }
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

inline BitVector::BitVector( std::span< const Word > w , std::size_t n )
    : f_w() , f_n( n )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 f_w.reserve( wordsFor( n ) );  // the words copied once
 f_w.assign( w.begin() , w.begin() + std::ptrdiff_t( ( n + 63 ) / 64 ) );
 f_w.resize( wordsFor( n ) , 0 );
 if( n % 64 )
  f_w[ n / 64 ] &= detail::lowMask( int( n % 64 ) );
 ADS_INVARIANT( clean() );
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
 if( b )
  f_w[ i / 64 ] |= m;
 else
  f_w[ i / 64 ] &= ~m;
 ADS_POST( ( *this )[ i ] == b );
 }

inline void BitVector::push_back( bool b )
{
 if( ( f_n + 1 ) / 64 + 2 > f_w.size() )
  f_w.resize( f_w.size() + 8 , 0 );
 if( b )
  f_w[ f_n / 64 ] |= Word( 1 ) << ( f_n % 64 );
 ++f_n;
 ADS_POST( ( *this )[ f_n - 1 ] == b );
 }

inline void BitVector::append( Word v , int len )
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( ( len == 64 ) || ( ( v >> len ) == 0 ) );
 if( len == 0 )
  return;
 if( ( f_n + std::size_t( len ) ) / 64 + 2 > f_w.size() )
  f_w.resize( f_w.size() + 8 , 0 );
 const std::size_t w = f_n / 64;
 const int o = int( f_n % 64 );
 f_w[ w ] |= v << o;
 if( o + len > 64 )
  f_w[ w + 1 ] = v >> ( 64 - o );
 f_n += std::size_t( len );
 ADS_POST( get( f_n - std::size_t( len ) , len ) == v );
 ADS_INVARIANT( clean() );
 }

inline Word BitVector::get( std::size_t i , int len ) const
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( i + std::size_t( len ) <= f_n );
 const std::size_t w = i / 64;  // a word past the last bit, always
 const int o = int( i % 64 );
 const Word r = ( f_w[ w ] >> o ) | ( ( f_w[ w + 1 ] << 1 ) << ( 63 - o ) );
 return( r & detail::lowMask( len ) );
 }

inline void BitVector::put( std::size_t i , Word v , int len )
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( ( len == 64 ) || ( ( v >> len ) == 0 ) );
 ADS_PRE( i + std::size_t( len ) <= f_n );
 if( len == 0 )
  return;
 const std::size_t w = i / 64;
 const int o = int( i % 64 );
 const Word m = detail::lowMask( len );
 f_w[ w ] = ( f_w[ w ] & ~( m << o ) ) | ( v << o );
 if( o + len > 64 ) {
  const int h = 64 - o;  // the bits in the first word
  f_w[ w + 1 ] = ( f_w[ w + 1 ] & ~( m >> h ) ) | ( v >> h );
  }
 ADS_POST( get( i , len ) == v );
 }

inline std::size_t BitVector::count( void ) const
{
 std::size_t c = 0;
 for( const Word w : f_w )
  c += std::size_t( sidewaysSum( w ) );
 return( c );
 }

inline const Words & BitVector::words( void ) const
{
 return( f_w );
 }

inline std::string BitVector::toString( void ) const
{
 std::string s( f_n , '0' );
 for( std::size_t i = 0 ; i < f_n ; ++i )
  if( ( *this )[ i ] )
   s[ i ] = '1';
 return( s );
 }

inline void BitVector::reserve( std::size_t n )
{
 f_w.reserve( wordsFor( n ) );
 }

inline void BitVector::clear( void )
{
 f_w.assign( wordsFor( 0 ) , 0 );
 f_n = 0;
 }

inline bool BitVector::clean( void ) const
{
 if( ( f_w.size() < f_n / 64 + 2 ) || ( f_w.size() % 8 ) )
  return( false );
 if( f_w[ f_n / 64 ] & ~detail::lowMask( int( f_n % 64 ) ) )
  return( false );
 return( std::all_of( f_w.begin() + std::ptrdiff_t( f_n / 64 + 1 ) ,
		      f_w.end() ,
		       []( Word w ) { return( w == 0 ); } ) );
 }

/*--------------------------------------------------------------------------*/

template< class A >
inline void BitVector::persist( A & a )
{
 a( f_n , f_w );
 a.require( clean() , "the words of a BitVector" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File BitVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
