/*--------------------------------------------------------------------------*/
/*-------------------------- File IntegerCodes.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of IntegerCodes.h, with their contracts.
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

/// the bits of an integer in [ 0 , m ), ceil( log2 m ); 64 for m = 0,
/// which stands for 2^64
constexpr int bitsFor( std::uint64_t m )
{
 return( m == 0 ? 64 : m <= 1 ? 0 : lambda( m - 1 ) + 1 );
 }

/// the interpolative code of x[ l , r ), within [ low , hi ]
inline void interpolative( BitWriter & w ,
                           const std::vector< std::uint64_t > & x ,
                           std::size_t l , std::size_t r ,
                           std::uint64_t low , std::uint64_t hi )
{
 if( l >= r ) return;
 const std::size_t m = ( l + r - 1 ) / 2;
 encodeBounded( w , x[ m ] , low + ( m - l ) , hi - ( r - 1 - m ) );
 if( l < m ) interpolative( w , x , l , m , low , x[ m ] - 1 );
 if( m + 1 < r ) interpolative( w , x , m + 1 , r , x[ m ] + 1 , hi );
 }

/// the items x[ l , r ) decoded within [ low , hi ]
inline void interpolative( BitReader & rd , std::vector< std::uint64_t > & x ,
                           std::size_t l , std::size_t r ,
                           std::uint64_t low , std::uint64_t hi )
{
 if( l >= r ) return;
 const std::size_t m = ( l + r - 1 ) / 2;
 x[ m ] = decodeBounded( rd , low + ( m - l ) , hi - ( r - 1 - m ) );
 if( l < m ) interpolative( rd , x , l , m , low , x[ m ] - 1 );
 if( m + 1 < r ) interpolative( rd , x , m + 1 , r , x[ m ] + 1 , hi );
 }

}  // end( namespace detail )

/*---------------------------- PREPROCESSING -------------------------------*/

constexpr int binaryLength( std::uint64_t x )
{
 ADS_PRE( x >= 1 );
 return( int( std::bit_width( x ) ) );     // lambda( x ) + 1
 }

constexpr std::uint64_t signedToNatural( std::int64_t x )
{
 ADS_PRE( ( x < ( std::int64_t( 1 ) << 62 ) ) &&
          ( x > -( std::int64_t( 1 ) << 62 ) ) );
 return( x >= 0 ? 2 * std::uint64_t( x ) : 2 * std::uint64_t( -x ) + 1 );
 }

constexpr std::int64_t naturalToSigned( std::uint64_t y )
{
 return( y % 2 ? -std::int64_t( y / 2 ) : std::int64_t( y / 2 ) );
 }

inline std::vector< std::uint64_t > dGaps(
                                      const std::vector< std::uint64_t > & x )
{
 ADS_PRE( std::is_sorted( x.begin() , x.end() ) );
 std::vector< std::uint64_t > g( x.size() );
 for( std::size_t i = 0 ; i < x.size() ; ++i )
  g[ i ] = i ? x[ i ] - x[ i - 1 ] : x[ i ];
 return( g );
 }

inline std::vector< std::uint64_t > prefixSums(
                                      const std::vector< std::uint64_t > & g )
{
 std::vector< std::uint64_t > x( g.size() );
 for( std::size_t i = 0 ; i < g.size() ; ++i )
  x[ i ] = i ? x[ i - 1 ] + g[ i ] : g[ i ];
 return( x );
 }

/*------------------------------ UNARY, GAMMA, DELTA -----------------------*/

inline void encodeUnary( BitWriter & w , std::uint64_t x )
{
 ADS_PRE( x >= 1 );
 [[maybe_unused]] const std::size_t n = w.size();
 w.writeUnary( x - 1 );
 ADS_POST( w.size() - n == unaryLength( x ) );
 }

inline std::uint64_t decodeUnary( BitReader & r )
{
 return( r.readUnary() + 1 );
 }

constexpr std::uint64_t unaryLength( std::uint64_t x )
{
 return( x );
 }

inline void encodeGamma( BitWriter & w , std::uint64_t x )
{
 ADS_PRE( x >= 1 );
 [[maybe_unused]] const std::size_t n = w.size();
 const int L = binaryLength( x ) - 1;       // the bits after the leading 1
 if( L <= 31 )                              // U( L + 1 ) B( x ) in one field
  w.write( ( Word( 1 ) << L ) |
           ( detail::reverseField( x & detail::lowMask( L ) , L ) <<
             ( L + 1 ) ) , 2 * L + 1 );
 else {
  w.writeUnary( std::uint64_t( L ) );       // the 1 is that of B( x )
  w.writeBinary( x & detail::lowMask( L ) , L );
  }
 ADS_POST( w.size() - n == std::size_t( gammaLength( x ) ) );
 }

inline std::uint64_t decodeGamma( BitReader & r )
{
 const std::uint64_t L = r.readUnary();
 ADS_PRE( L < 64 );
 return( ( std::uint64_t( 1 ) << L ) | r.readBinary( int( L ) ) );
 }

constexpr int gammaLength( std::uint64_t x )
{
 return( 2 * lambda( x ) + 1 );
 }

inline void encodeDelta( BitWriter & w , std::uint64_t x )
{
 ADS_PRE( x >= 1 );
 [[maybe_unused]] const std::size_t n = w.size();
 const int L = binaryLength( x );
 encodeGamma( w , std::uint64_t( L ) );
 w.writeBinary( x , L );
 ADS_POST( w.size() - n == std::size_t( deltaLength( x ) ) );
 }

inline std::uint64_t decodeDelta( BitReader & r )
{
 const std::uint64_t L = decodeGamma( r );
 ADS_PRE( L <= 64 );
 return( r.readBinary( int( L ) ) );
 }

constexpr int deltaLength( std::uint64_t x )
{
 const int L = binaryLength( x );
 return( 1 + 2 * lambda( std::uint64_t( L ) ) + L );
 }

/*------------------------------ RICE, GOLOMB ------------------------------*/

inline void encodeRice( BitWriter & w , std::uint64_t x , int k )
{
 ADS_PRE( ( x >= 1 ) && ( k >= 0 ) && ( k < 64 ) );
 [[maybe_unused]] const std::size_t n = w.size();
 w.writeUnary( ( x - 1 ) >> k );
 w.writeBinary( ( x - 1 ) & detail::lowMask( k ) , k );
 ADS_POST( w.size() - n == riceLength( x , k ) );
 }

inline std::uint64_t decodeRice( BitReader & r , int k )
{
 ADS_PRE( ( k >= 0 ) && ( k < 64 ) );
 const std::uint64_t q = r.readUnary();
 return( ( q << k ) + r.readBinary( k ) + 1 );
 }

constexpr std::uint64_t riceLength( std::uint64_t x , int k )
{
 return( ( ( x - 1 ) >> k ) + 1 + std::uint64_t( k ) );
 }

inline void encodeGolomb( BitWriter & w , std::uint64_t x , std::uint64_t b )
{
 ADS_PRE( ( x >= 1 ) && ( b >= 1 ) && ( b <= ( Word( 1 ) << 63 ) ) );
 [[maybe_unused]] const std::size_t n = w.size();
 const std::uint64_t q = ( x - 1 ) / b , r = ( x - 1 ) % b;
 w.writeUnary( q );
 if( b > 1 ) {                              // the truncated binary code
  const int c = detail::bitsFor( b );
  const std::uint64_t t = ( std::uint64_t( 1 ) << c ) - b;
  if( r < t ) w.writeBinary( r , c - 1 );
  else w.writeBinary( r + t , c );
  }
 ADS_POST( w.size() - n == golombLength( x , b ) );
 }

inline std::uint64_t decodeGolomb( BitReader & r , std::uint64_t b )
{
 ADS_PRE( ( b >= 1 ) && ( b <= ( Word( 1 ) << 63 ) ) );
 const std::uint64_t q = r.readUnary();
 std::uint64_t v = 0;
 if( b > 1 ) {
  const int c = detail::bitsFor( b );
  const std::uint64_t t = ( std::uint64_t( 1 ) << c ) - b;
  v = r.readBinary( c - 1 );
  if( v >= t ) v = ( ( v << 1 ) | std::uint64_t( r.readBit() ) ) - t;
  }
 return( q * b + v + 1 );
 }

constexpr std::uint64_t golombLength( std::uint64_t x , std::uint64_t b )
{
 const std::uint64_t q = ( x - 1 ) / b , r = ( x - 1 ) % b;
 if( b == 1 ) return( q + 1 );
 const int c = detail::bitsFor( b );
 const std::uint64_t t = ( std::uint64_t( 1 ) << c ) - b;
 return( q + 1 + std::uint64_t( r < t ? c - 1 : c ) );
 }

/*-------------------------------- PFORDELTA -------------------------------*/

inline std::size_t PForDeltaBlock::size( void ) const
{
 return( slots.size() / std::size_t( b ) );
 }

inline std::size_t PForDeltaBlock::codeBits( void ) const
{
 return( slots.size() + 64 * exceptions.size() );
 }

inline int pforDeltaWidth( const std::vector< std::uint64_t > & x ,
                           std::uint64_t base , double f )
{
 ADS_PRE( ( f > 0 ) && ( f <= 1 ) );
 const double need = f * double( x.size() );
 for( int b = 1 ; b < 63 ; ++b ) {
  const std::uint64_t esc = ( std::uint64_t( 1 ) << b ) - 1;
  std::size_t in = 0;
  for( const auto v : x )
   if( ( v >= base ) && ( v - base < esc ) ) ++in;
  if( double( in ) >= need ) return( b );
  }
 return( 63 );
 }

inline PForDeltaBlock encodePForDelta( const std::vector< std::uint64_t > & x ,
                                       int b , std::uint64_t base )
{
 ADS_PRE( ( b >= 1 ) && ( b <= 63 ) );
 PForDeltaBlock p;
 p.b = b;
 p.base = base;
 p.slots.reserve( x.size() * std::size_t( b ) );
 const std::uint64_t esc = ( std::uint64_t( 1 ) << b ) - 1;
 for( const auto v : x )
  if( ( v >= base ) && ( v - base < esc ) ) p.slots.append( v - base , b );
  else {                                    // an exception
   p.slots.append( esc , b );
   p.exceptions.push_back( v );
   }
 ADS_AUDIT( decodePForDelta( p ) == x );
 return( p );
 }

inline std::vector< std::uint64_t > decodePForDelta( const PForDeltaBlock & p )
{
 const std::size_t n = p.size();
 const std::uint64_t esc = ( std::uint64_t( 1 ) << p.b ) - 1;
 std::vector< std::uint64_t > x( n );
 std::size_t e = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint64_t s = p.slots.get( i * std::size_t( p.b ) , p.b );
  x[ i ] = s == esc ? p.exceptions[ e++ ] : s + p.base;
  }
 return( x );
 }

/*---------------------- VARIABLE BYTE, (s,c)-DENSE ------------------------*/

inline void encodeVByte( std::vector< std::uint8_t > & out , std::uint64_t x )
{
 for( int i = vbyteLength( x ) - 1 ; i >= 0 ; --i )
  out.push_back( std::uint8_t( ( ( x >> ( 7 * i ) ) & 0x7F ) |
                               ( i > 0 ? 0x80 : 0 ) ) );
 }

inline std::uint64_t decodeVByte( const std::vector< std::uint8_t > & in ,
                                  std::size_t & p )
{
 std::uint64_t x = 0;
 for( ; ; ) {
  ADS_PRE( p < in.size() );
  const std::uint8_t b = in[ p++ ];
  x = ( x << 7 ) | ( b & 0x7F );
  if( b < 0x80 ) return( x );               // a stopper
  }
 }

constexpr int vbyteLength( std::uint64_t x )
{
 return( x == 0 ? 1 : ( binaryLength( x ) + 6 ) / 7 );
 }

inline void encodeDense( std::vector< std::uint32_t > & out , std::uint64_t x ,
                         std::uint32_t s , std::uint32_t c )
{
 ADS_PRE( ( s >= 1 ) && ( c >= 1 ) );
 const std::size_t at = out.size();
 out.push_back( std::uint32_t( x % s ) );   // the stopper, last
 for( std::uint64_t y = x / s ; y > 0 ; y /= c ) {
  --y;
  out.push_back( s + std::uint32_t( y % c ) );
  }
 std::reverse( out.begin() + std::ptrdiff_t( at ) , out.end() );
 ADS_POST( out.size() - at == std::size_t( denseLength( x , s , c ) ) );
 }

inline std::uint64_t decodeDense( const std::vector< std::uint32_t > & in ,
                                  std::size_t & p , std::uint32_t s ,
                                  std::uint32_t c )
{
 ADS_PRE( ( s >= 1 ) && ( c >= 1 ) );
 std::uint64_t y = 0;
 for( ; ; ) {
  ADS_PRE( ( p < in.size() ) && ( in[ p ] < s + c ) );
  const std::uint32_t d = in[ p++ ];
  if( d < s ) return( y * s + d );          // the stopper
  y = y * c + ( d - s ) + 1;
  }
 }

constexpr int denseLength( std::uint64_t x , std::uint32_t s ,
                           std::uint32_t c )
{
 int k = 1;
 for( std::uint64_t y = x / s ; y > 0 ; y = ( y - 1 ) / c ) ++k;
 return( k );
 }

inline std::uint32_t denseOptimalS( const std::vector< std::uint64_t > & f ,
                                    int bits )
{
 ADS_PRE( ( bits >= 1 ) && ( bits <= 16 ) );
 const std::uint32_t W = std::uint32_t( 1 ) << bits;
 std::uint32_t best = 1;
 auto cost = [ & ]( std::uint32_t s ) {
  std::uint64_t t = 0;
  for( std::size_t x = 0 ; x < f.size() ; ++x )
   t += f[ x ] * std::uint64_t( denseLength( x , s , W - s ) );
  return( t );
  };
 std::uint64_t bc = cost( 1 );
 for( std::uint32_t s = 2 ; s < W ; ++s )
  if( const std::uint64_t t = cost( s ) ; t < bc ) {
   bc = t;
   best = s;
   }
 return( best );
 }

/*------------------------------ INTERPOLATIVE -----------------------------*/

inline void encodeBounded( BitWriter & w , std::uint64_t x ,
                           std::uint64_t a , std::uint64_t b )
{
 ADS_PRE( ( a <= x ) && ( x <= b ) );
 w.writeBinary( x - a , detail::bitsFor( b - a + 1 ) );
 }

inline std::uint64_t decodeBounded( BitReader & r , std::uint64_t a ,
                                    std::uint64_t b )
{
 ADS_PRE( a <= b );
 return( a + r.readBinary( detail::bitsFor( b - a + 1 ) ) );
 }

inline void encodeInterpolative( BitWriter & w ,
                                 const std::vector< std::uint64_t > & x ,
                                 std::uint64_t low , std::uint64_t hi )
{
 ADS_PRE( x.empty() || ( ( low <= x.front() ) && ( x.back() <= hi ) ) );
 ADS_AUDIT( std::adjacent_find( x.begin() , x.end() ,
                                []( std::uint64_t a , std::uint64_t b ) {
                                 return( a >= b ); } ) == x.end() );
 detail::interpolative( w , x , 0 , x.size() , low , hi );
 }

inline std::vector< std::uint64_t > decodeInterpolative( BitReader & r ,
                                                         std::size_t n ,
                                                         std::uint64_t low ,
                                                         std::uint64_t hi )
{
 std::vector< std::uint64_t > x( n );
 detail::interpolative( r , x , 0 , n , low , hi );
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File IntegerCodes.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
