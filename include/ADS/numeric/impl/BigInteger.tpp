/*--------------------------------------------------------------------------*/
/*-------------------------- File BigInteger.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BigInteger [see BigInteger.h], with their contracts.
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

inline BigInteger::BigInteger( long long v ) : f_neg( v < 0 )
{
 auto m = v < 0 ? std::uint64_t( 0 ) - std::uint64_t( v ) : std::uint64_t( v );
 for( ; m ; m >>= 32 ) f_d.push_back( Limb( m ) );
 }

inline BigInteger::BigInteger( bool neg , Mag d ) : f_d( std::move( d ) )
{
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 }

inline void BigInteger::trim( Mag & d )
{
 while( ! d.empty() && ( d.back() == 0 ) ) d.pop_back();
 }

inline bool BigInteger::normal( void ) const
{
 return( ( f_d.empty() || ( f_d.back() != 0 ) ) &&
         ! ( f_neg && f_d.empty() ) );
 }

inline BigInteger::Limb BigInteger::divSmall( Mag & a , Limb d )
{
 std::uint64_t r = 0;
 for( std::size_t i = a.size() ; i-- > 0 ; ) {
  const std::uint64_t cur = ( r << 32 ) | a[ i ];
  a[ i ] = Limb( cur / d );
  r = cur % d;
  }
 trim( a );
 return( Limb( r ) );
 }

inline void BigInteger::mulAddSmall( Mag & a , Limb m , Limb c )
{
 std::uint64_t carry = c;
 for( auto & x : a ) {
  const std::uint64_t cur = std::uint64_t( x ) * m + carry;
  x = Limb( cur );
  carry = cur >> 32;
  }
 if( carry ) a.push_back( Limb( carry ) );
 }

namespace detail {

/// the value of the digit c in the bases up to 36, -1 if none
inline int digitValue( char c )
{
 if( ( c >= '0' ) && ( c <= '9' ) ) return( c - '0' );
 if( ( c >= 'a' ) && ( c <= 'z' ) ) return( c - 'a' + 10 );
 if( ( c >= 'A' ) && ( c <= 'Z' ) ) return( c - 'A' + 10 );
 return( -1 );
 }

/// the largest power of the base in a digit of 32 bits, and its exponent
inline std::pair< std::uint32_t , int > bigBase( int base )
{
 std::uint64_t p = std::uint64_t( base );
 int k = 1;
 while( p * std::uint64_t( base ) <= 0xFFFFFFFFULL ) {
  p *= std::uint64_t( base );
  ++k;
  }
 return( std::pair( std::uint32_t( p ) , k ) );
 }

}  // end( namespace detail )

inline BigInteger::BigInteger( const std::string & s , int base )
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 std::size_t i = 0;
 bool neg = false;
 if( ( i < s.size() ) && ( ( s[ i ] == '-' ) || ( s[ i ] == '+' ) ) )
  neg = s[ i++ ] == '-';
 ADS_PRE( i < s.size() );                   // at least one digit
 const auto [ P , k ] = detail::bigBase( base );
 while( i < s.size() ) {                    // k digits at a time
  Limb chunk = 0 , scale = 1;
  for( int t = 0 ; ( t < k ) && ( i < s.size() ) ; ++t , ++i ) {
   const int v = detail::digitValue( s[ i ] );
   ADS_PRE( ( v >= 0 ) && ( v < base ) );
   chunk = chunk * Limb( base ) + Limb( v );
   scale *= Limb( base );
   }
  mulAddSmall( f_d , scale , chunk );
  }
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 ADS_INVARIANT( normal() );
 }

inline std::string BigInteger::toString( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 const auto [ P , k ] = detail::bigBase( base );
 Mag a = f_d;
 std::string out;                           // reversed
 while( ! a.empty() ) {
  Limb r = divSmall( a , P );               // k digits of the output
  for( int t = 0 ; ( t < k ) && ( ! a.empty() || r ) ; ++t ) {
   out.push_back( "0123456789abcdefghijklmnopqrstuvwxyz"[ r % Limb( base ) ] );
   r /= Limb( base );
   }
  }
 if( f_neg ) out.push_back( '-' );
 std::reverse( out.begin() , out.end() );
 return( out );
 }

inline int BigInteger::sign( void ) const
{
 return( f_d.empty() ? 0 : f_neg ? -1 : 1 );
 }

inline BigInteger BigInteger::abs( void ) const
{
 return( BigInteger( false , f_d ) );
 }

inline std::size_t BigInteger::limbs( void ) const
{
 return( f_d.size() );
 }

inline BigInteger BigInteger::operator-( void ) const
{
 return( BigInteger( ! f_neg , f_d ) );
 }

inline int BigInteger::compareMag( const Mag & a , const Mag & b )
{
 if( a.size() != b.size() ) return( a.size() < b.size() ? -1 : 1 );
 for( std::size_t i = a.size() ; i-- > 0 ; )
  if( a[ i ] != b[ i ] ) return( a[ i ] < b[ i ] ? -1 : 1 );
 return( 0 );
 }

inline BigInteger::Mag BigInteger::addMag( const Mag & a , const Mag & b )
{
 const Mag & x = a.size() >= b.size() ? a : b;   // Algorithm A
 const Mag & y = a.size() >= b.size() ? b : a;
 Mag r( x.size() + 1 );
 std::uint64_t carry = 0;
 for( std::size_t i = 0 ; i < x.size() ; ++i ) {
  const std::uint64_t s = std::uint64_t( x[ i ] ) +
                          ( i < y.size() ? y[ i ] : 0 ) + carry;
  r[ i ] = Limb( s );
  carry = s >> 32;
  }
 r[ x.size() ] = Limb( carry );
 trim( r );
 return( r );
 }

inline BigInteger::Mag BigInteger::subMag( const Mag & a , const Mag & b )
{
 Mag r( a.size() );                         // Algorithm S
 std::int64_t borrow = 0;
 for( std::size_t i = 0 ; i < a.size() ; ++i ) {
  std::int64_t s = std::int64_t( a[ i ] ) -
                   std::int64_t( i < b.size() ? b[ i ] : 0 ) - borrow;
  borrow = s < 0;
  if( s < 0 ) s += std::int64_t( 1 ) << 32;
  r[ i ] = Limb( s );
  }
 trim( r );
 return( r );
 }

inline BigInteger operator+( const BigInteger & a , const BigInteger & b )
{
 if( a.f_neg == b.f_neg )
  return( BigInteger( a.f_neg , BigInteger::addMag( a.f_d , b.f_d ) ) );
 const int c = BigInteger::compareMag( a.f_d , b.f_d );
 if( c == 0 ) return( BigInteger() );
 return( c > 0 ? BigInteger( a.f_neg , BigInteger::subMag( a.f_d , b.f_d ) )
               : BigInteger( b.f_neg , BigInteger::subMag( b.f_d , a.f_d ) ) );
 }

inline BigInteger operator-( const BigInteger & a , const BigInteger & b )
{
 return( a + ( -b ) );
 }

inline BigInteger::Mag BigInteger::mulSchool( const Mag & a , const Mag & b )
{
 if( a.empty() || b.empty() ) return( Mag() );
 Mag w( a.size() + b.size() , 0 );          // Algorithm M
 for( std::size_t j = 0 ; j < b.size() ; ++j ) {
  std::uint64_t k = 0;
  for( std::size_t i = 0 ; i < a.size() ; ++i ) {
   const std::uint64_t t = std::uint64_t( a[ i ] ) * b[ j ] + w[ i + j ] + k;
   w[ i + j ] = Limb( t );
   k = t >> 32;
   }
  w[ j + a.size() ] = Limb( k );
  }
 trim( w );
 return( w );
 }

inline void BigInteger::addShifted( Mag & r , const Mag & x ,
                                     std::size_t off )
{
 if( r.size() < off + x.size() + 1 ) r.resize( off + x.size() + 1 , 0 );
 std::uint64_t carry = 0;
 std::size_t i = 0;
 for( ; i < x.size() ; ++i ) {
  const std::uint64_t s = std::uint64_t( r[ off + i ] ) + x[ i ] + carry;
  r[ off + i ] = Limb( s );
  carry = s >> 32;
  }
 for( ; carry ; ++i ) {
  if( off + i == r.size() ) r.push_back( 0 );
  const std::uint64_t s = std::uint64_t( r[ off + i ] ) + carry;
  r[ off + i ] = Limb( s );
  carry = s >> 32;
  }
 }

inline BigInteger::Mag BigInteger::mulKaratsuba( const Mag & a ,
                                                 const Mag & b )
{
 if( a.size() < b.size() ) return( mulKaratsuba( b , a ) );
 const std::size_t na = a.size() , nb = b.size();
 if( nb < kKaratsuba ) return( mulSchool( a , b ) );
 if( na >= 2 * nb ) {                       // unbalanced: a in pieces of nb
  Mag r( na + nb , 0 );
  for( std::size_t off = 0 ; off < na ; off += nb ) {
   Mag piece( a.begin() + long( off ) ,
              a.begin() + long( std::min( na , off + nb ) ) );
   trim( piece );
   addShifted( r , mulKaratsuba( piece , b ) , off );
   }
  trim( r );
  return( r );
  }
 const std::size_t m = na / 2;              // a = a1 b^m + a0, the same b
 Mag a0( a.begin() , a.begin() + long( m ) ) ,
     a1( a.begin() + long( m ) , a.end() ) ,
     b0( b.begin() , b.begin() + long( std::min( m , nb ) ) ) ,
     b1( b.begin() + long( std::min( m , nb ) ) , b.end() );
 trim( a0 );
 trim( b0 );
 const Mag z0 = mulKaratsuba( a0 , b0 ) , z2 = mulKaratsuba( a1 , b1 );
 Mag z1 = mulKaratsuba( addMag( a0 , a1 ) , addMag( b0 , b1 ) );
 z1 = subMag( subMag( z1 , z0 ) , z2 );     // ( a0 + a1 ) ( b0 + b1 ) - ...
 Mag r = z0;
 addShifted( r , z1 , m );
 addShifted( r , z2 , 2 * m );
 trim( r );
 return( r );
 }

inline BigInteger::Mag BigInteger::mulFFT( const Mag & a , const Mag & b )
{
 if( a.empty() || b.empty() ) return( Mag() );
 auto halves = []( const Mag & x ) {        // digits of 16 bits
  std::vector< double > h( 2 * x.size() );
  for( std::size_t i = 0 ; i < x.size() ; ++i ) {
   h[ 2 * i ] = double( x[ i ] & 0xFFFF );
   h[ 2 * i + 1 ] = double( x[ i ] >> 16 );
   }
  return( h );
  };
 const auto c = convolve( halves( a ) , halves( b ) );
 Mag w( a.size() + b.size() , 0 );
 std::uint64_t carry = 0;
 for( std::size_t i = 0 ; i < 2 * w.size() ; ++i ) {   // the carries
  if( i < c.size() ) carry += std::uint64_t( std::llround( c[ i ] ) );
  w[ i / 2 ] |= Limb( carry & 0xFFFF ) << ( 16 * ( i % 2 ) );
  carry >>= 16;
  }
 trim( w );
 return( w );
 }

inline BigInteger BigInteger::multiplySchool( const BigInteger & a ,
                                              const BigInteger & b )
{
 return( BigInteger( a.f_neg != b.f_neg , mulSchool( a.f_d , b.f_d ) ) );
 }

inline BigInteger BigInteger::multiplyKaratsuba( const BigInteger & a ,
                                                 const BigInteger & b )
{
 BigInteger r( a.f_neg != b.f_neg , mulKaratsuba( a.f_d , b.f_d ) );
 ADS_AUDIT( r == multiplySchool( a , b ) );
 return( r );
 }

inline BigInteger BigInteger::multiplyFFT( const BigInteger & a ,
                                           const BigInteger & b )
{
 BigInteger r( a.f_neg != b.f_neg , mulFFT( a.f_d , b.f_d ) );
 ADS_AUDIT( r == multiplySchool( a , b ) );
 return( r );
 }

inline BigInteger operator*( const BigInteger & a , const BigInteger & b )
{
 const std::size_t k = std::min( a.f_d.size() , b.f_d.size() );
 if( k >= BigInteger::kFFT )
  return( BigInteger::multiplyFFT( a , b ) );
 if( k >= BigInteger::kKaratsuba )
  return( BigInteger::multiplyKaratsuba( a , b ) );
 return( BigInteger::multiplySchool( a , b ) );
 }

inline std::pair< BigInteger::Mag , BigInteger::Mag > BigInteger::divModMag(
                                                const Mag & u , const Mag & v )
{
 if( compareMag( u , v ) < 0 ) return( std::pair( Mag() , u ) );
 if( v.size() == 1 ) {                      // the short division
  Mag q = u;
  const Limb r = divSmall( q , v[ 0 ] );
  return( std::pair( q , r ? Mag{ r } : Mag() ) );
  }
 const std::size_t n = v.size() , m = u.size() - n;
 const int s = std::countl_zero( v.back() );   // D1: normalize
 Mag vn( n ) , un( u.size() + 1 );
 for( std::size_t i = n ; i-- > 1 ; )
  vn[ i ] = ( v[ i ] << s ) | ( s ? Limb( std::uint64_t( v[ i - 1 ] ) >>
                                          ( 32 - s ) ) : 0 );
 vn[ 0 ] = v[ 0 ] << s;
 un[ u.size() ] = s ? Limb( std::uint64_t( u.back() ) >> ( 32 - s ) ) : 0;
 for( std::size_t i = u.size() ; i-- > 1 ; )
  un[ i ] = ( u[ i ] << s ) | ( s ? Limb( std::uint64_t( u[ i - 1 ] ) >>
                                          ( 32 - s ) ) : 0 );
 un[ 0 ] = u[ 0 ] << s;
 const std::uint64_t b = std::uint64_t( 1 ) << 32;
 Mag q( m + 1 );
 for( std::size_t j = m + 1 ; j-- > 0 ; ) {   // D2-D7
  const std::uint64_t num = ( std::uint64_t( un[ j + n ] ) << 32 ) |
                            un[ j + n - 1 ];
  std::uint64_t qhat = num / vn[ n - 1 ] , rhat = num % vn[ n - 1 ];
  while( ( qhat >= b ) ||                   // D3: at most 2 too large
         ( qhat * vn[ n - 2 ] > ( rhat << 32 ) + un[ j + n - 2 ] ) ) {
   --qhat;
   rhat += vn[ n - 1 ];
   if( rhat >= b ) break;
   }
  std::int64_t k = 0 , t;                   // D4: multiply and subtract
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::uint64_t p = qhat * vn[ i ];
   t = std::int64_t( un[ i + j ] ) - k - std::int64_t( p & 0xFFFFFFFFULL );
   un[ i + j ] = Limb( t );
   k = std::int64_t( p >> 32 ) - ( t >> 32 );
   }
  t = std::int64_t( un[ j + n ] ) - k;
  un[ j + n ] = Limb( t );
  q[ j ] = Limb( qhat );
  if( t < 0 ) {                             // D6: add back, rarely
   --q[ j ];
   std::uint64_t c = 0;
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::uint64_t z = std::uint64_t( un[ i + j ] ) + vn[ i ] + c;
    un[ i + j ] = Limb( z );
    c = z >> 32;
    }
   un[ j + n ] = Limb( std::uint64_t( un[ j + n ] ) + c );
   }
  }
 Mag r( n );                                // D8: unnormalize
 for( std::size_t i = 0 ; i < n ; ++i )
  r[ i ] = ( un[ i ] >> s ) | ( s ? Limb( std::uint64_t( un[ i + 1 ] ) <<
                                          ( 32 - s ) ) : 0 );
 trim( q );
 trim( r );
 return( std::pair( q , r ) );
 }

inline std::pair< BigInteger , BigInteger > BigInteger::divMod(
                                const BigInteger & a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 auto [ q , r ] = divModMag( a.f_d , b.f_d );
 std::pair< BigInteger , BigInteger > out( BigInteger( a.f_neg != b.f_neg ,
                                                       std::move( q ) ) ,
                                           BigInteger( a.f_neg ,
                                                       std::move( r ) ) );
 ADS_AUDIT( ( out.first * b + out.second == a ) &&
            ( compareMag( out.second.f_d , b.f_d ) < 0 ) &&
            ( ( out.second.sign() == 0 ) ||
              ( out.second.sign() == a.sign() ) ) );
 return( out );
 }

inline BigInteger operator/( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::divMod( a , b ).first );
 }

inline BigInteger operator%( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::divMod( a , b ).second );
 }

inline std::strong_ordering operator<=>( const BigInteger & a ,
                                         const BigInteger & b )
{
 if( a.sign() != b.sign() ) return( a.sign() <=> b.sign() );
 const int c = BigInteger::compareMag( a.f_d , b.f_d );
 return( a.f_neg ? 0 <=> c : c <=> 0 );
 }

inline BigInteger & BigInteger::operator+=( const BigInteger & b )
{
 return( *this = *this + b );
 }

inline BigInteger & BigInteger::operator-=( const BigInteger & b )
{
 return( *this = *this - b );
 }

inline BigInteger & BigInteger::operator*=( const BigInteger & b )
{
 return( *this = *this * b );
 }

inline BigInteger BigInteger::pow( BigInteger x , unsigned long n )
{
 BigInteger y = 1;                          // y x^n is invariant
 for( ; n ; n >>= 1 ) {
  if( n & 1 ) y *= x;
  if( n > 1 ) x *= x;
  }
 return( y );
 }

inline BigInteger BigInteger::gcd( BigInteger a , BigInteger b )
{
 a = a.abs();
 b = b.abs();
 while( b.sign() != 0 ) {
  BigInteger r = a % b;
  a = std::move( b );
  b = std::move( r );
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BigInteger.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
