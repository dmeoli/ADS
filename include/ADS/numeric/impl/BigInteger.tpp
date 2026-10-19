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

namespace detail {

/*--------------------------------------------------------------------------*/
/*------------------------- THE ARITHMETIC OF WORDS ------------------------*/
/*--------------------------------------------------------------------------*/

#if ADS_BIGINT_WIDE == 128
__extension__ typedef unsigned __int128 UInt128;
#endif

/// a b = hi 2^64 + lo, lo returned: one instruction where the compiler
/// has it, else the four products of the halves (Algorithm 4.3.1M)
inline std::uint64_t mulWide( std::uint64_t a , std::uint64_t b ,
                              std::uint64_t & hi )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 p = UInt128( a ) * b;
 hi = std::uint64_t( p >> 64 );
 return( std::uint64_t( p ) );
#elif ADS_BIGINT_WIDE == 64
 return( _umul128( a , b , &hi ) );
#else
 constexpr std::uint64_t M = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & M , a1 = a >> 32 , b0 = b & M , b1 = b >> 32 ,
                     p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
                     mid = ( p00 >> 32 ) + ( p01 & M ) + ( p10 & M );
 hi = a1 * b1 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 );
 return( ( mid << 32 ) | ( p00 & M ) );
#endif
 }

/// a b + c + d = hi 2^64 + lo, lo returned, which never overflows
inline std::uint64_t mulAdd( std::uint64_t a , std::uint64_t b ,
                             std::uint64_t c , std::uint64_t d ,
                             std::uint64_t & hi )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 p = UInt128( a ) * b + c + d;
 hi = std::uint64_t( p >> 64 );
 return( std::uint64_t( p ) );
#else
 std::uint64_t lo = mulWide( a , b , hi );
 lo += c;
 hi += lo < c;
 lo += d;
 hi += lo < d;
 return( lo );
#endif
 }

/// a + b + c, c in { 0 , 1 }, the carry out in c
inline std::uint64_t addCarry( std::uint64_t a , std::uint64_t b ,
                               std::uint64_t & c )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 s = UInt128( a ) + b + c;
 c = std::uint64_t( s >> 64 );
 return( std::uint64_t( s ) );
#elif ADS_BIGINT_WIDE == 64
 unsigned long long s;
 c = _addcarry_u64( (unsigned char)( c ) , a , b , &s );
 return( s );
#else
 const std::uint64_t s = a + b , t = s + c;
 c = std::uint64_t( s < a ) | std::uint64_t( t < s );
 return( t );
#endif
 }

/// a - b - c, c in { 0 , 1 }, the borrow out in c
inline std::uint64_t subBorrow( std::uint64_t a , std::uint64_t b ,
                                std::uint64_t & c )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 s = UInt128( a ) - b - c;
 c = std::uint64_t( s >> 64 ) & 1;
 return( std::uint64_t( s ) );
#elif ADS_BIGINT_WIDE == 64
 unsigned long long s;
 c = _subborrow_u64( (unsigned char)( c ) , a , b , &s );
 return( s );
#else
 const std::uint64_t s = a - b , t = s - c;
 c = std::uint64_t( a < b ) | std::uint64_t( s < c );
 return( t );
#endif
 }

/// floor( ( u1 2^64 + u0 ) / d ) for d with the top bit set and u1 < d;
/// in the portable form the division of two digits of 32 bits by one
/// twice (Hacker's Delight, figure 9-3), also for clang-cl, whose division
/// of unsigned __int128 needs a library that MSVC lacks
inline std::uint64_t divWide( std::uint64_t u1 , std::uint64_t u0 ,
                              std::uint64_t d )
{
#if ( ADS_BIGINT_WIDE == 128 ) && ! defined( _MSC_VER )
 return( std::uint64_t( ( ( UInt128( u1 ) << 64 ) | u0 ) / d ) );
#elif ADS_BIGINT_WIDE == 64
 unsigned long long r;
 return( _udiv128( u1 , u0 , d , &r ) );
#else
 constexpr std::uint64_t B = std::uint64_t( 1 ) << 32 , M = B - 1;
 const std::uint64_t d1 = d >> 32 , d0 = d & M , n1 = u0 >> 32 ,
                     n0 = u0 & M;
 std::uint64_t q1 = u1 / d1 , r = u1 - q1 * d1;
 while( ( q1 >= B ) || ( q1 * d0 > ( ( r << 32 ) | n1 ) ) ) {
  --q1;
  r += d1;
  if( r >= B ) break;
  }
 const std::uint64_t u21 = ( u1 << 32 ) + n1 - q1 * d;   // modulo 2^64
 std::uint64_t q0 = u21 / d1;
 r = u21 - q0 * d1;
 while( ( q0 >= B ) || ( q0 * d0 > ( ( r << 32 ) | n0 ) ) ) {
  --q0;
  r += d1;
  if( r >= B ) break;
  }
 return( ( q1 << 32 ) + q0 );
#endif
 }

/// the reciprocal of d with the top bit set, floor( ( b^2 - 1 ) / d ) - b
inline std::uint64_t reciprocal( std::uint64_t d )
{
 return( divWide( ~d , ~std::uint64_t( 0 ) , d ) );
 }

/// the quotient of u1 b + u0 by d with the top bit set, u1 < d, by the
/// reciprocal v of d (Moeller and Granlund, Algorithm 4), the remainder in
/// r; the sum is modulo b^2
inline std::uint64_t div2by1( std::uint64_t u1 , std::uint64_t u0 ,
                              std::uint64_t d , std::uint64_t v ,
                              std::uint64_t & r )
{
 std::uint64_t q1;
 const std::uint64_t q0 = mulAdd( v , u1 , u0 , 0 , q1 );
 q1 += u1 + 1;
 r = u0 - q1 * d;
 if( r > q0 ) {
  --q1;
  r += d;
  }
 if( r >= d ) {                             // rarely
  ++q1;
  r -= d;
  }
 return( q1 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE FIGURES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the value of the digit c in the bases up to 36, -1 if none
inline int digitValue( char c )
{
 if( ( c >= '0' ) && ( c <= '9' ) ) return( c - '0' );
 if( ( c >= 'a' ) && ( c <= 'z' ) ) return( c - 'a' + 10 );
 if( ( c >= 'A' ) && ( c <= 'Z' ) ) return( c - 'A' + 10 );
 return( -1 );
 }

/// the largest power of the base in a digit of 64 bits, and its exponent
inline std::pair< std::uint64_t , int > bigBase( int base )
{
 const std::uint64_t b = std::uint64_t( base );
 std::uint64_t p = b;
 int k = 1;
 while( p <= ~std::uint64_t( 0 ) / b ) {
  p *= b;
  ++k;
  }
 return( std::pair( p , k ) );
 }

/// the figures of the bases up to 36
inline constexpr const char * kFigures =
                                       "0123456789abcdefghijklmnopqrstuvwxyz";

/// the k figures of r in the base, the last one at end[ -1 ]; the base 10
/// by 9 figures at a time in 32 bits, each divided by a constant
inline void putFigures( char * end , std::uint64_t r , int k , int base )
{
 if( base == 10 ) {
  for( ; k > 9 ; k -= 9 , r /= 1000000000u ) {
   std::uint32_t x = std::uint32_t( r % 1000000000u );
   for( int t = 0 ; t < 9 ; ++t , x /= 10 ) *--end = char( '0' + x % 10 );
   }
  std::uint32_t x = std::uint32_t( r );
  for( int t = 0 ; t < k ; ++t , x /= 10 ) *--end = char( '0' + x % 10 );
  }
 else
  for( int t = 0 ; t < k ; ++t , r /= std::uint64_t( base ) )
   *--end = kFigures[ r % std::uint64_t( base ) ];
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE NTT -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the arithmetic of Montgomery modulo a prime P < 2^31, R = 2^32, and the
/// number theoretic transform modulo P of up to 2^26 terms, the roots of
/// unity from G, a quadratic nonresidue
template< std::uint32_t P , std::uint32_t G >
struct NTT {

 /// -1 / P modulo 2^32, by Newton from the 3 bits right in P
 static constexpr std::uint32_t inv( void ) {
  std::uint32_t x = P;
  for( int i = 0 ; i < 4 ; ++i ) x *= 2 - P * x;
  return( 0u - x );
  }

 static constexpr std::uint32_t kInv = inv();
 static constexpr std::uint32_t kR = std::uint32_t(   ///< R mod P
                                       ( std::uint64_t( 1 ) << 32 ) % P );
 static constexpr std::uint32_t kR2 = std::uint32_t(  ///< R^2 mod P
                                       std::uint64_t( kR ) * kR % P );

 /// a b / R mod P, for a < 2^32 and b < P
 static std::uint32_t mul( std::uint32_t a , std::uint32_t b ) {
  const std::uint64_t t = std::uint64_t( a ) * b;
  const std::uint32_t m = std::uint32_t( t ) * kInv;
  const std::uint32_t u = std::uint32_t( ( t + std::uint64_t( m ) * P ) >>
                                         32 );
  return( u >= P ? u - P : u );
  }

 static std::uint32_t add( std::uint32_t a , std::uint32_t b ) {
  const std::uint32_t s = a + b;
  return( s >= P ? s - P : s );
  }

 static std::uint32_t sub( std::uint32_t a , std::uint32_t b ) {
  return( a >= b ? a - b : a + P - b );
  }

 /// x^e, both x and the result in the form of Montgomery
 static std::uint32_t power( std::uint32_t x , std::uint64_t e ) {
  std::uint32_t y = kR;
  for( ; e ; e >>= 1 ) {
   if( e & 1 ) y = mul( y , x );
   x = mul( x , x );
   }
  return( y );
  }

 /// the powers of the roots of unity (inverse if inv), in the form of
 /// Montgomery, the round of length len at len / 2 , ... , len - 1, for
 /// the transforms up to n terms; exact, kept per thread and grown
 static const std::uint32_t * roots( std::size_t n , bool inverse ) {
  thread_local std::vector< std::uint32_t > T[ 2 ];
  auto & t = T[ inverse ];
  if( t.size() < n ) {
   t.assign( n , 0 );
   for( std::size_t len = 2 ; len <= n ; len *= 2 ) {
    std::uint32_t w = power( mul( G , kR2 ) , ( P - 1 ) / len );
    if( inverse ) w = power( w , P - 2 );
    std::uint32_t x = kR;
    for( std::size_t k = 0 ; k < len / 2 ; ++k ) {
     t[ len / 2 + k ] = x;
     x = mul( x , w );
     }
    }
   }
  return( t.data() );
  }

 /// the transform of a, n a power of 2, from the natural order to that of
 /// the reversed bits (Gentleman and Sande)
 static void forward( std::uint32_t * a , std::size_t n ) {
  const std::uint32_t * T = roots( n , false );
  for( std::size_t len = n ; len >= 2 ; len /= 2 ) {
   const std::size_t h = len / 2;
   const std::uint32_t * w = T + h;
   for( std::size_t i = 0 ; i < n ; i += len ) {
    std::uint32_t * x = a + i , * y = x + h;
    for( std::size_t k = 0 ; k < h ; ++k ) {
     const std::uint32_t u = x[ k ] , v = y[ k ];
     x[ k ] = add( u , v );
     y[ k ] = mul( sub( u , v ) , w[ k ] );
     }
    }
   }
  }

 /// the inverse transform times n, from the order of the reversed bits to
 /// the natural one (Cooley and Tukey)
 static void backward( std::uint32_t * a , std::size_t n ) {
  const std::uint32_t * T = roots( n , true );
  for( std::size_t len = 2 ; len <= n ; len *= 2 ) {
   const std::size_t h = len / 2;
   const std::uint32_t * w = T + h;
   for( std::size_t i = 0 ; i < n ; i += len ) {
    std::uint32_t * x = a + i , * y = x + h;
    for( std::size_t k = 0 ; k < h ; ++k ) {
     const std::uint32_t u = x[ k ] , v = mul( y[ k ] , w[ k ] );
     x[ k ] = add( u , v );
     y[ k ] = sub( u , v );
     }
    }
   }
  }

 /// x of na limbs of 64 bits as 2 na pieces of 32 bits in the form of
 /// Montgomery, the least significant first, and zeros up to n
 static void load( std::uint32_t * f , const std::uint64_t * x ,
                   std::size_t na , std::size_t n ) {
  for( std::size_t i = 0 ; i < na ; ++i ) {
   f[ 2 * i ] = mul( std::uint32_t( x[ i ] ) , kR );
   f[ 2 * i + 1 ] = mul( std::uint32_t( x[ i ] >> 32 ) , kR );
   }
  std::fill( f + 2 * na , f + n , 0 );
  }

 /// fa = the cyclic convolution modulo P of the pieces of 32 bits of a and
 /// b, na and nb limbs, n terms, with fb as scratch; a square if fb is null
 static void convolve( std::uint32_t * fa , std::uint32_t * fb ,
                       const std::uint64_t * a , std::size_t na ,
                       const std::uint64_t * b , std::size_t nb ,
                       std::size_t n ) {
  load( fa , a , na , n );
  forward( fa , n );
  // the factor 1 / n, and R^2 against the two products of Montgomery
  const std::uint32_t c = mul( mul( P - ( P - 1 ) / std::uint32_t( n ) ,
                                    kR2 ) , kR2 );
  if( ! fb )
   for( std::size_t i = 0 ; i < n ; ++i )
    fa[ i ] = mul( mul( fa[ i ] , fa[ i ] ) , c );
  else {
   load( fb , b , nb , n );
   forward( fb , n );
   for( std::size_t i = 0 ; i < n ; ++i )
    fa[ i ] = mul( mul( fa[ i ] , fb[ i ] ) , c );
   }
  backward( fa , n );
  }

 };  // end( struct NTT )

}  // end( namespace detail )

inline BigInteger::BigInteger( long long v ) : f_neg( v < 0 )
{
 auto m = v < 0 ? std::uint64_t( 0 ) - std::uint64_t( v ) : std::uint64_t( v );
 if( m ) f_d = { Limb( m ) };
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

/*--------------------------------------------------------------------------*/
/*------------------------- THE OPERATIONS ON SPANS ------------------------*/
/*--------------------------------------------------------------------------*/

inline int BigInteger::cmpN( const Limb * a , const Limb * b , std::size_t n )
{
 for( std::size_t i = n ; i-- > 0 ; )
  if( a[ i ] != b[ i ] ) return( a[ i ] < b[ i ] ? -1 : 1 );
 return( 0 );
 }

inline BigInteger::Limb BigInteger::addN( Limb * r , const Limb * a ,
                                          std::size_t na , const Limb * b ,
                                          std::size_t nb )
{
 Limb c = 0;
 std::size_t i = 0;
 for( ; i < nb ; ++i ) r[ i ] = detail::addCarry( a[ i ] , b[ i ] , c );
 for( ; c && ( i < na ) ; ++i ) c = ( ( r[ i ] = a[ i ] + 1 ) == 0 );
 if( r != a ) std::copy( a + i , a + na , r + i );
 return( c );
 }

inline BigInteger::Limb BigInteger::subN( Limb * r , const Limb * a ,
                                          std::size_t na , const Limb * b ,
                                          std::size_t nb )
{
 Limb c = 0;                                // the borrow, 0 or 1
 std::size_t i = 0;
 for( ; i < nb ; ++i ) r[ i ] = detail::subBorrow( a[ i ] , b[ i ] , c );
 for( ; c && ( i < na ) ; ++i ) c = ( ( r[ i ] = a[ i ] - 1 ) == ~Limb( 0 ) );
 if( r != a ) std::copy( a + i , a + na , r + i );
 return( c );
 }

inline BigInteger::Limb BigInteger::addTo( Limb * r , std::size_t nr ,
                                           const Limb * x , std::size_t nx )
{
 return( addN( r , r , nr , x , nx ) );
 }

inline BigInteger::Limb BigInteger::subFrom( Limb * r , std::size_t nr ,
                                             const Limb * x , std::size_t nx )
{
 return( subN( r , r , nr , x , nx ) );
 }

inline BigInteger::Limb BigInteger::mul1( Limb * r , const Limb * a ,
                                          std::size_t n , Limb m )
{
 Limb c = 0;
 for( std::size_t i = 0 ; i < n ; ++i )
  r[ i ] = detail::mulAdd( a[ i ] , m , c , 0 , c );
 return( c );
 }

inline BigInteger::Limb BigInteger::addMul1( Limb * r , const Limb * a ,
                                             std::size_t n , Limb m )
{
 Limb c = 0;                                // a m + r + c < 2^128
 for( std::size_t i = 0 ; i < n ; ++i )
  r[ i ] = detail::mulAdd( a[ i ] , m , r[ i ] , c , c );
 return( c );
 }

inline BigInteger::Limb BigInteger::subMul1( Limb * r , const Limb * a ,
                                             std::size_t n , Limb m )
{
 Limb c = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  Limb hi;
  const Limb lo = detail::mulAdd( a[ i ] , m , c , 0 , hi ) , x = r[ i ];
  r[ i ] = x - lo;
  c = hi + ( x < lo );
  }
 return( c );
 }

inline bool BigInteger::absDiff( Limb * r , const Limb * a , std::size_t na ,
                                 const Limb * b , std::size_t nb )
{
 std::size_t i = na;
 while( ( i > nb ) && ( a[ i - 1 ] == 0 ) ) --i;
 if( ( i > nb ) || ( cmpN( a , b , nb ) >= 0 ) ) {
  subN( r , a , na , b , nb );
  return( false );
  }
 subN( r , b , nb , a , nb );               // a has zeros above nb
 std::fill( r + nb , r + na , 0 );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONVERSIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::Limb BigInteger::divSmall( Mag & a , Limb d )
{
 const int s = std::countl_zero( d );       // d shifted, and a with it
 const Limb dn = d << s , v = detail::reciprocal( dn );
 const std::size_t n = a.size();
 Limb r = 0;
 if( n == 0 ) return( 0 );
 if( s == 0 )
  for( std::size_t i = n ; i-- > 0 ; )
   a[ i ] = detail::div2by1( r , a[ i ] , dn , v , r );
 else {
  r = a[ n - 1 ] >> ( 64 - s );
  for( std::size_t i = n ; i-- > 0 ; ) {
   const Limb u = ( a[ i ] << s ) | ( i ? a[ i - 1 ] >> ( 64 - s ) : 0 );
   a[ i ] = detail::div2by1( r , u , dn , v , r );
   }
  }
 trim( a );
 return( r >> s );
 }

inline BigInteger::Limb BigInteger::modSmall( const Limb * a , std::size_t n ,
                                              Limb d )
{
 const int s = std::countl_zero( d );
 const Limb dn = d << s , v = detail::reciprocal( dn );
 Limb r = 0;
 if( n == 0 ) return( 0 );
 if( ( n >= kModFold ) && ( s >= 4 ) && ( d > 1 ) ) {
  // R = rh b + rl, below 2^127, takes 4 digits at a time as R b^4 + ... ,
  // i.e., as the sum of their products by c[ k ] = b^k mod d < 2^60: the
  // products are independent, and one division by d is left at the end
  auto mod = [ & ]( Limb hi , Limb lo ) {   // ( hi b + lo ) mod d, hi < d
   Limb m;
   detail::div2by1( ( hi << s ) | ( lo >> ( 64 - s ) ) , lo << s , dn , v ,
                    m );
   return( m >> s );
   };
  Limb c[ 6 ] , rh = a[ n - 1 ] , rl = a[ n - 2 ] , H , L;
  c[ 1 ] = mod( 1 , 0 );
  for( int k = 2 ; k < 6 ; ++k ) {
   L = detail::mulWide( c[ k - 1 ] , c[ 1 ] , H );
   c[ k ] = mod( H , L );
   }
  auto acc = [ & ]( Limb x , Limb y ) {     // ( H , L ) += x y
   Limb ph;
   const Limb pl = detail::mulWide( x , y , ph );
   L += pl;
   H += ph + ( L < pl );
   };
  std::size_t i = n - 2;
  for( ; i >= 4 ; i -= 4 ) {
   L = detail::mulAdd( rl , c[ 4 ] , a[ i - 4 ] , 0 , H );
   acc( rh , c[ 5 ] );
   acc( a[ i - 1 ] , c[ 3 ] );
   acc( a[ i - 2 ] , c[ 2 ] );
   acc( a[ i - 3 ] , c[ 1 ] );
   rh = H;
   rl = L;
   }
  for( ; i > 0 ; --i ) {
   L = detail::mulAdd( rl , c[ 1 ] , a[ i - 1 ] , 0 , H );
   acc( rh , c[ 2 ] );
   rh = H;
   rl = L;
   }
  const Limb t[ 2 ] = { rl , rh };
  return( modSmall( t , 2 , d ) );
  }
 if( s == 0 )
  for( std::size_t i = n ; i-- > 0 ; )
   detail::div2by1( r , a[ i ] , dn , v , r );
 else {
  r = a[ n - 1 ] >> ( 64 - s );
  for( std::size_t i = n ; i-- > 0 ; )
   detail::div2by1( r , ( a[ i ] << s ) |
                        ( i ? a[ i - 1 ] >> ( 64 - s ) : 0 ) , dn , v , r );
  }
 return( r >> s );
 }

inline void BigInteger::mulAddSmall( Mag & a , Limb m , Limb c )
{
 for( auto & x : a ) x = detail::mulAdd( x , m , c , 0 , c );
 if( c ) a.push_back( c );
 }

inline BigInteger BigInteger::fromStringSchool( const std::string & s ,
                                                int base )
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 std::size_t i = 0;
 bool neg = false;
 if( ( i < s.size() ) && ( ( s[ i ] == '-' ) || ( s[ i ] == '+' ) ) )
  neg = s[ i++ ] == '-';
 ADS_PRE( i < s.size() );                   // at least one digit
 const int k = detail::bigBase( base ).second;
 Mag d;
 while( i < s.size() ) {                    // k digits at a time
  Limb chunk = 0 , scale = 1;
  for( int t = 0 ; ( t < k ) && ( i < s.size() ) ; ++t , ++i ) {
   const int v = detail::digitValue( s[ i ] );
   ADS_PRE( ( v >= 0 ) && ( v < base ) );
   chunk = chunk * Limb( base ) + Limb( v );
   scale *= Limb( base );
   }
  mulAddSmall( d , scale , chunk );
  }
 return( BigInteger( neg , std::move( d ) ) );
 }

inline BigInteger::Mag BigInteger::fromChunks( const Limb * c , std::size_t n ,
                                               const std::vector< Mag > & pw ,
                                               Limb P )
{
 Mag x;
 if( n < kConvert ) {                       // Horner, a product by P each
  x.reserve( n );
  for( std::size_t j = n ; j-- > 0 ; ) mulAddSmall( x , P , c[ j ] );
  return( x );
  }
 const std::size_t h = std::bit_floor( n - 1 );   // n / 2 <= h < n
 const Mag & p = pw[ std::size_t( std::countr_zero( h ) ) ];
 const Mag hi = fromChunks( c + h , n - h , pw , P ) ,
           lo = fromChunks( c , h , pw , P );
 if( hi.empty() ) return( lo );
 x.resize( hi.size() + p.size() );          // hi P^h + lo, lo < P^h
 mulP( x.data() , hi.data() , hi.size() , p.data() , p.size() );
 addTo( x.data() , x.size() , lo.data() , lo.size() );
 trim( x );
 return( x );
 }

inline BigInteger::BigInteger( const std::string & s , int base )
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 std::size_t i = 0;
 bool neg = false;
 if( ( i < s.size() ) && ( ( s[ i ] == '-' ) || ( s[ i ] == '+' ) ) )
  neg = s[ i++ ] == '-';
 ADS_PRE( i < s.size() );                   // at least one digit
 for( std::size_t j = i ; j < s.size() ; ++j )
  ADS_PRE( ( detail::digitValue( s[ j ] ) >= 0 ) &&
           ( detail::digitValue( s[ j ] ) < base ) );
 while( ( i + 1 < s.size() ) && ( s[ i ] == '0' ) ) ++i;
 const std::size_t L = s.size() - i;        // the figures
 if( ( base & ( base - 1 ) ) == 0 ) {       // b bits per figure
  const std::size_t b = std::size_t( std::countr_zero( unsigned( base ) ) );
  f_d.assign( ( L * b + 63 ) / 64 , 0 );
  std::size_t bit = 0;
  for( std::size_t j = s.size() ; j-- > i ; bit += b ) {
   const Limb v = Limb( detail::digitValue( s[ j ] ) );
   const std::size_t w = bit / 64 , o = bit % 64;
   f_d[ w ] |= v << o;
   if( o + b > 64 ) f_d[ w + 1 ] |= v >> ( 64 - o );
   }
  }
 else {                                     // chunks of k figures
  const auto [ P , k ] = detail::bigBase( base );
  const std::size_t K = std::size_t( k ) , C = ( L + K - 1 ) / K;
  Mag c( C );                               // the least significant first
  for( std::size_t j = 0 ; j < C ; ++j ) {
   const std::size_t hi = s.size() - j * K , lo = hi - std::min( K , hi - i );
   Limb x = 0;
   for( std::size_t t = lo ; t < hi ; ++t )
    x = x * Limb( base ) + Limb( detail::digitValue( s[ t ] ) );
   c[ j ] = x;
   }
  std::vector< Mag > pw( 1 , Mag{ P } );    // P^{2^i}, 2^i < C
  if( C >= kConvert )
   while( ( std::size_t( 1 ) << pw.size() ) < C )
    pw.push_back( mulMag( pw.back() , pw.back() ) );
  f_d = fromChunks( c.data() , C , pw , P );
  }
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 ADS_INVARIANT( normal() );
 ADS_AUDIT( *this == fromStringSchool( s , base ) );
 }

inline void BigInteger::figures( Mag a , std::size_t pad , int base ,
                                 std::string & out )
{
 const auto [ P , k ] = detail::bigBase( base );
 std::vector< Limb > c;                     // the least significant first
 c.reserve( a.size() + a.size() / 4 + 1 );
 while( ! a.empty() ) c.push_back( divSmall( a , P ) );
 std::size_t top = 0;                       // the figures of the top chunk
 if( ! c.empty() )
  for( Limb y = c.back() ; y ; y /= Limb( base ) ) ++top;
 const std::size_t len = c.empty() ? 0 : ( c.size() - 1 ) * std::size_t( k ) +
                                         top;
 if( pad > len ) out.append( pad - len , '0' );
 const std::size_t o = out.size();
 out.resize( o + len );
 char * end = out.data() + o + len;
 for( std::size_t j = 0 ; j + 1 < c.size() ; ++j , end -= k )
  detail::putFigures( end , c[ j ] , k , base );
 if( ! c.empty() ) detail::putFigures( end , c.back() , int( top ) , base );
 }

inline void BigInteger::toFigures( const Mag & x , std::size_t pad , int base ,
                                   const std::vector< Mag > & pw , int k ,
                                   std::string & out )
{
 if( x.size() < kConvert ) {
  figures( x , pad , base , out );
  return;
  }
 std::size_t i = 0;                         // the power nearest sqrt( x )
 while( ( i + 1 < pw.size() ) &&
        ( pw[ i + 1 ].size() <= ( x.size() + 1 ) / 2 ) )
  ++i;
 const auto [ q , r ] = divModMag( x , pw[ i ] );
 const std::size_t low = std::size_t( k ) << i;   // the figures of r
 toFigures( q , pad > low ? pad - low : 0 , base , pw , k , out );
 toFigures( r , low , base , pw , k , out );
 }

inline std::string BigInteger::toString( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 std::string out;
 if( f_neg ) out.push_back( '-' );
 const std::size_t n = f_d.size();
 if( ( base & ( base - 1 ) ) == 0 ) {       // b bits per figure
  const std::size_t b = std::size_t( std::countr_zero( unsigned( base ) ) ) ,
                    bits = 64 * n - std::size_t( std::countl_zero(
                                                              f_d.back() ) ) ,
                    D = ( bits + b - 1 ) / b , o = out.size();
  out.resize( o + D );
  for( std::size_t j = 0 ; j < D ; ++j ) {   // the figure j from the right
   const std::size_t w = j * b / 64 , sh = j * b % 64;
   Limb x = f_d[ w ] >> sh;
   if( ( sh + b > 64 ) && ( w + 1 < n ) ) x |= f_d[ w + 1 ] << ( 64 - sh );
   out[ o + D - 1 - j ] = detail::kFigures[ x & Limb( base - 1 ) ];
   }
  }
 else if( n < kConvert ) figures( f_d , 0 , base , out );
 else {
  const auto [ P , k ] = detail::bigBase( base );
  std::vector< Mag > pw( 1 , Mag{ P } );    // up to half the digits
  while( 2 * pw.back().size() - 1 <= ( n + 1 ) / 2 ) {
   Mag p = mulMag( pw.back() , pw.back() );
   if( p.size() > ( n + 1 ) / 2 ) break;
   pw.push_back( std::move( p ) );
   }
  toFigures( f_d , 0 , base , pw , k , out );
  }
 ADS_AUDIT( out == toStringSchool( base ) );
 return( out );
 }

inline std::string BigInteger::toStringSchool( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 std::string out;
 if( f_neg ) out.push_back( '-' );
 figures( f_d , 0 , base , out );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE SUM AND THE ORDER --------------------------*/
/*--------------------------------------------------------------------------*/

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
 return( cmpN( a.data() , b.data() , a.size() ) );
 }

inline BigInteger::Mag BigInteger::addMag( const Mag & a , const Mag & b )
{
 const Mag & x = a.size() >= b.size() ? a : b;   // Algorithm A
 const Mag & y = a.size() >= b.size() ? b : a;
 Mag r( x.size() + 1 );
 r[ x.size() ] = addN( r.data() , x.data() , x.size() , y.data() ,
                       y.size() );
 if( r.back() == 0 ) r.pop_back();
 return( r );
 }

inline BigInteger::Mag BigInteger::subMag( const Mag & a , const Mag & b )
{
 Mag r( a.size() );                         // Algorithm S
 subN( r.data() , a.data() , a.size() , b.data() , b.size() );
 trim( r );
 return( r );
 }

inline BigInteger BigInteger::addSigned( const BigInteger & a ,
                                         const BigInteger & b , bool bneg )
{
 if( a.f_neg == bneg ) return( BigInteger( a.f_neg , addMag( a.f_d ,
                                                             b.f_d ) ) );
 const int c = compareMag( a.f_d , b.f_d );
 if( c == 0 ) return( BigInteger() );
 return( c > 0 ? BigInteger( a.f_neg , subMag( a.f_d , b.f_d ) )
               : BigInteger( bneg , subMag( b.f_d , a.f_d ) ) );
 }

inline BigInteger operator+( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::addSigned( a , b , b.f_neg ) );
 }

inline BigInteger operator-( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::addSigned( a , b , ! b.f_neg ) );
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
 if( f_neg != b.f_neg ) return( *this = *this + b );
 if( f_d.size() < b.f_d.size() ) f_d.resize( b.f_d.size() , 0 );
 const Limb c = addTo( f_d.data() , f_d.size() , b.f_d.data() ,
                       b.f_d.size() );
 if( c ) f_d.push_back( c );
 return( *this );
 }

inline BigInteger & BigInteger::operator-=( const BigInteger & b )
{
 if( f_neg == b.f_neg ) return( *this = *this - b );
 if( f_d.size() < b.f_d.size() ) f_d.resize( b.f_d.size() , 0 );
 const Limb c = addTo( f_d.data() , f_d.size() , b.f_d.data() ,
                       b.f_d.size() );
 if( c ) f_d.push_back( c );
 return( *this );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PRODUCT -------------------------------*/
/*--------------------------------------------------------------------------*/

inline void BigInteger::mulSchool( Limb * r , const Limb * a , std::size_t na ,
                                   const Limb * b , std::size_t nb )
{
 if( na < nb ) {                            // the rows on the longer one
  std::swap( a , b );
  std::swap( na , nb );
  }
 if( nb == 0 ) {
  std::fill( r , r + na , 0 );
  return;
  }
 r[ na ] = mul1( r , a , na , b[ 0 ] );     // Algorithm M
 for( std::size_t j = 1 ; j < nb ; ++j )
  r[ na + j ] = addMul1( r + j , a , na , b[ j ] );
 }

inline void BigInteger::sqrSchool( Limb * r , const Limb * a , std::size_t n )
{
 if( n == 0 ) return;
 r[ 0 ] = 0;                                // the products a_i a_j, i < j
 r[ 2 * n - 1 ] = 0;
 r[ n ] = mul1( r + 1 , a + 1 , n - 1 , a[ 0 ] );
 for( std::size_t i = 1 ; i + 1 < n ; ++i )
  r[ n + i ] = addMul1( r + 2 * i + 1 , a + i + 1 , n - i - 1 , a[ i ] );
 Limb s = 0 , c = 0;                        // twice them, and the squares
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb lo = r[ 2 * i ] , hi = r[ 2 * i + 1 ];
  const Limb l2 = ( lo << 1 ) | s , h2 = ( hi << 1 ) | ( lo >> 63 );
  s = hi >> 63;
  Limb ph;
  const Limb pl = detail::mulWide( a[ i ] , a[ i ] , ph );
  r[ 2 * i ] = detail::addCarry( l2 , pl , c );
  r[ 2 * i + 1 ] = detail::addCarry( h2 , ph , c );
  }
 }

inline std::size_t BigInteger::karatsubaScratch( std::size_t n )
{
 std::size_t s = 0;
 for( ; n >= kKaratsuba ; n -= n / 2 ) s += 6 * ( n - n / 2 ) + 1;
 return( s );
 }

inline void BigInteger::karatsuba( Limb * r , const Limb * a , const Limb * b ,
                                   std::size_t n , Limb * w ,
                                   std::size_t fft )
{
 const bool sq = a == b;
 if( n < kKaratsuba ) {
  if( sq ) sqrSchool( r , a , n );
  else mulSchool( r , a , n , b , n );
  return;
  }
 if( ( n >= fft ) && ( 2 * n <= kNTTMax ) ) {
  mulNTT( r , a , n , b , n );
  return;
  }
 const std::size_t h = n / 2 , l = n - h;   // a = a1 b^l + a0, the same b
 karatsuba( r , a , b , l , w , fft );      // z0 = a0 b0
 karatsuba( r + 2 * l , a + l , b + l , h , w , fft );   // z2 = a1 b1
 Limb * da = w , * db = w + l , * t = w + 2 * l , * mid = w + 4 * l;
 bool neg = absDiff( da , a , l , a + l , h );   // t = |a0 - a1| |b0 - b1|
 if( sq ) neg = false;
 else neg = neg != absDiff( db , b , l , b + l , h );
 karatsuba( t , da , sq ? da : db , l , w + 6 * l + 1 , fft );
 // a0 b1 + a1 b0 = z0 + z2 - ( a0 - a1 ) ( b0 - b1 )
 mid[ 2 * l ] = addN( mid , r , 2 * l , r + 2 * l , 2 * h );
 if( neg ) mid[ 2 * l ] += addTo( mid , 2 * l , t , 2 * l );
 else mid[ 2 * l ] -= subFrom( mid , 2 * l , t , 2 * l );
 addTo( r + l , 2 * n - l , mid , 2 * l + 1 );
 }

inline void BigInteger::mulNTT( Limb * r , const Limb * a , std::size_t na ,
                                const Limb * b , std::size_t nb )
{
 using N1 = detail::NTT< 2013265921u , 31 >;   // 15 2^27 + 1
 using N2 = detail::NTT< 1811939329u , 13 >;   // 27 2^26 + 1
 using N3 = detail::NTT< 469762049u , 3 >;     // 7 2^26 + 1
 constexpr std::uint64_t p1 = 2013265921u , p2 = 1811939329u ,
                         p3 = 469762049u;
 // a coefficient, at most 2^25 products of two pieces below 2^32, stays
 // below p1 p2 p3 > 2^90: p1 p2 > 2^26 ( 2^63 / p3 ) says so in 64 bits
 static_assert( ( p1 * p2 ) >> 26 > ( std::uint64_t( 1 ) << 63 ) / p3 );
 if( ( na == 0 ) || ( nb == 0 ) ) {
  std::fill( r , r + na + nb , 0 );
  return;
  }
 const std::size_t m = 2 * ( na + nb ) - 1 , n = std::bit_ceil( m );
 ADS_PRE( na + nb <= kNTTMax );
 const bool sq = ( a == b ) && ( na == nb );
 std::vector< std::uint32_t , detail::NoInit< std::uint32_t > > r1( n ) ,
                                     r2( n ) , r3( n ) , w( sq ? 0 : n );
 std::uint32_t * fb = sq ? nullptr : w.data();
 N1::convolve( r1.data() , fb , a , na , b , nb , n );
 N2::convolve( r2.data() , fb , a , na , b , nb , n );
 N3::convolve( r3.data() , fb , a , na , b , nb , n );
 // Garner: x = y1 + p1 y2 + p1 p2 y3 < p1 p2 p3, with the inverses in the
 // form of Montgomery; the pieces of 32 bits of the product, two per limb
 const std::uint32_t i12 = N2::power( N2::mul( std::uint32_t( p1 - p2 ) ,
                                               N2::kR2 ) ,
                                      p2 - 2 ) ,
                     p1m3 = N3::mul( std::uint32_t( p1 % p3 ) , N3::kR2 ) ,
                     i123 = N3::power( N3::mul( std::uint32_t( p1 * p2 %
                                                               p3 ) ,
                                                N3::kR2 ) ,
                                       p3 - 2 );
 const std::uint64_t p12 = p1 * p2 , p12lo = p12 & 0xFFFFFFFFULL ,
                     p12hi = p12 >> 32;
 std::uint64_t c0 = 0 , c1 = 0 , c2 = 0;    // the carry, three pieces
 for( std::size_t i = 0 ; i < 2 * ( na + nb ) ; ++i ) {
  std::uint64_t x0 = 0 , x1 = 0 , x2 = 0;
  if( i < m ) {
   const std::uint32_t y1 = r1[ i ] ,
                       y2 = N2::mul( N2::sub( r2[ i ] , y1 >= p2 ?
                                              std::uint32_t( y1 - p2 ) : y1 ) ,
                                     i12 ) ,
                       y3 = N3::mul( N3::sub( r3[ i ] ,
                                     N3::add( N3::mul( y1 , N3::kR ) ,
                                              N3::mul( y2 , p1m3 ) ) ) ,
                                     i123 );
   const std::uint64_t lo = y1 + p1 * y2 , tl = p12lo * y3 ,
                       th = p12hi * y3 ,
                       s0 = ( lo & 0xFFFFFFFFULL ) + ( tl & 0xFFFFFFFFULL ) ,
                       s1 = ( lo >> 32 ) + ( tl >> 32 ) +
                            ( th & 0xFFFFFFFFULL ) + ( s0 >> 32 );
   x0 = s0 & 0xFFFFFFFFULL;
   x1 = s1 & 0xFFFFFFFFULL;
   x2 = ( th >> 32 ) + ( s1 >> 32 );
   }
  const std::uint64_t v0 = x0 + c0 , v1 = x1 + c1 + ( v0 >> 32 ) ,
                      v2 = x2 + c2 + ( v1 >> 32 );
  if( i & 1 ) r[ i / 2 ] |= ( v0 & 0xFFFFFFFFULL ) << 32;
  else r[ i / 2 ] = v0 & 0xFFFFFFFFULL;
  c0 = v1 & 0xFFFFFFFFULL;
  c1 = v2 & 0xFFFFFFFFULL;
  c2 = v2 >> 32;
  }
 }

inline void BigInteger::mulP( Limb * r , const Limb * a , std::size_t na ,
                              const Limb * b , std::size_t nb ,
                              std::size_t fft )
{
 if( na < nb ) {
  std::swap( a , b );
  std::swap( na , nb );
  }
 if( nb < kKaratsuba ) {
  if( ( a == b ) && ( na == nb ) ) sqrSchool( r , a , na );
  else mulSchool( r , a , na , b , nb );
  return;
  }
 if( ( nb >= fft ) && ( na + nb <= kNTTMax ) ) {
  mulNTT( r , a , na , b , nb );
  return;
  }
 Mag w( karatsubaScratch( nb ) );
 if( na == nb ) {
  karatsuba( r , a , b , nb , w.data() , fft );
  return;
  }
 Mag t( 2 * nb );                           // a in pieces of nb digits
 std::fill( r , r + na + nb , 0 );
 for( std::size_t off = 0 ; off < na ; off += nb ) {
  const std::size_t len = std::min( nb , na - off );
  if( len == nb ) karatsuba( t.data() , a + off , b , nb , w.data() , fft );
  else mulP( t.data() , b , nb , a + off , len , fft );
  addTo( r + off , na + nb - off , t.data() , len + nb );
  }
 }

inline BigInteger::Mag BigInteger::mulMag( const Mag & a , const Mag & b ,
                                           std::size_t fft )
{
 if( a.empty() || b.empty() ) return( Mag() );
 Mag r( a.size() + b.size() );
 mulP( r.data() , a.data() , a.size() , b.data() , b.size() , fft );
 trim( r );
 return( r );
 }

inline BigInteger BigInteger::multiplySchool( const BigInteger & a ,
                                              const BigInteger & b )
{
 Mag r( a.f_d.size() + b.f_d.size() );
 mulSchool( r.data() , a.f_d.data() , a.f_d.size() , b.f_d.data() ,
            b.f_d.size() );
 return( BigInteger( a.f_neg != b.f_neg , std::move( r ) ) );
 }

inline BigInteger BigInteger::multiplyKaratsuba( const BigInteger & a ,
                                                 const BigInteger & b )
{
 BigInteger r( a.f_neg != b.f_neg , mulMag( a.f_d , b.f_d ,
                                            std::size_t( -1 ) ) );
 ADS_AUDIT( r == multiplySchool( a , b ) );
 return( r );
 }

inline BigInteger BigInteger::multiplyFFT( const BigInteger & a ,
                                           const BigInteger & b )
{
 Mag d( a.f_d.size() + b.f_d.size() );
 if( d.size() <= kNTTMax )
  mulNTT( d.data() , a.f_d.data() , a.f_d.size() , b.f_d.data() ,
          b.f_d.size() );
 else
  mulP( d.data() , a.f_d.data() , a.f_d.size() , b.f_d.data() ,
        b.f_d.size() , 0 );
 BigInteger r( a.f_neg != b.f_neg , std::move( d ) );
 ADS_AUDIT( r == multiplySchool( a , b ) );
 return( r );
 }

inline BigInteger operator*( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger( a.f_neg != b.f_neg , BigInteger::mulMag( a.f_d ,
                                                              b.f_d ) ) );
 }

inline BigInteger & BigInteger::operator*=( const BigInteger & b )
{
 return( *this = *this * b );
 }

inline BigInteger BigInteger::pow( const BigInteger & x , unsigned long n )
{
 if( n == 0 ) return( 1 );
 BigInteger y = x;                          // x^( the leading bits of n )
 for( int i = int( std::bit_width( n ) ) - 1 ; i-- > 0 ; ) {
  y *= y;
  if( ( n >> i ) & 1 ) y *= x;
  }
 return( y );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE QUOTIENT ------------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::Limb BigInteger::divBase( Limb * a , std::size_t m ,
                                             const Limb * b , std::size_t n ,
                                             Limb * q )
{
 Limb top = 0;
 if( cmpN( a + m , b , n ) >= 0 ) {
  subFrom( a + m , n , b , n );
  top = 1;
  }
 const Limb d1 = b[ n - 1 ] , d0 = b[ n - 2 ] , v = detail::reciprocal( d1 );
 for( std::size_t j = m ; j-- > 0 ; ) {     // D2-D7
  const Limb u2 = a[ j + n ] , u1 = a[ j + n - 1 ] , u0 = a[ j + n - 2 ];
  Limb qhat , rhat;                         // D3: at most 2 too large
  bool big = false;                         // rhat >= b
  if( u2 >= d1 ) {                          // u2 = d1: qhat = b - 1
   qhat = ~Limb( 0 );
   rhat = u1 + d1;
   big = rhat < d1;
   }
  else qhat = detail::div2by1( u2 , u1 , d1 , v , rhat );
  while( ! big ) {                          // qhat d0 > rhat b + u0
   Limb ph;
   const Limb pl = detail::mulWide( qhat , d0 , ph );
   if( ( ph < rhat ) || ( ( ph == rhat ) && ( pl <= u0 ) ) ) break;
   --qhat;
   rhat += d1;
   big = rhat < d1;
   }
  const Limb borrow = subMul1( a + j , b , n , qhat );   // D4
  if( u2 < borrow ) {                       // D6: add back, rarely
   --qhat;
   addN( a + j , a + j , n , b , n );
   }
  a[ j + n ] = 0;
  q[ j ] = qhat;
  }
 return( top );
 }

inline BigInteger::Limb BigInteger::divRec( Limb * a , std::size_t m ,
                                            const Limb * b , std::size_t n ,
                                            Limb * q , Limb * t )
{
 if( m < kDivide ) return( divBase( a , m , b , n , q ) );
 const Limb one = 1;
 const std::size_t k = m / 2;
 // the upper half Q1 of a / b^{2k} by b / b^k, then A' = R1 b^{2k} +
 // ( a mod b^{2k} ) - Q1 B0 b^k in a[ 0 , n + k ), its borrow in hi
 int qh = int( divRec( a + 2 * k , m - k , b + k , n - k , q + k , t ) );
 mulP( t , q + k , m - k , b , k );
 std::int64_t hi = -std::int64_t( subFrom( a + k , n , t , m ) );
 if( qh ) hi -= std::int64_t( subFrom( a + m , n + k - m , b , k ) );
 while( hi < 0 ) {                          // at most twice
  qh -= int( subFrom( q + k , m - k , &one , 1 ) );
  hi += std::int64_t( addTo( a + k , n , b , n ) );
  }
 // the lower half Q0 of A' / b^k, then A'' = R0 b^k + ( A' mod b^k ) -
 // Q0 B0 in a[ 0 , n )
 int ql = int( divRec( a + k , k , b + k , n - k , q , t ) );
 mulP( t , q , k , b , k );
 hi = -std::int64_t( subFrom( a , n , t , 2 * k ) );
 if( ql ) hi -= std::int64_t( subFrom( a + k , n - k , b , k ) );
 while( hi < 0 ) {
  ql -= int( subFrom( q , k , &one , 1 ) );
  hi += std::int64_t( addTo( a , n , b , n ) );
  }
 if( ql ) qh += int( addTo( q + k , m - k , &one , 1 ) );
 return( Limb( qh ) );
 }

inline std::pair< BigInteger::Mag , BigInteger::Mag > BigInteger::divModMag(
                                const Mag & u , const Mag & v , bool school )
{
 if( compareMag( u , v ) < 0 ) return( std::pair( Mag() , u ) );
 if( v.size() == 1 ) {                      // the short division
  Mag q = u;
  const Limb r = divSmall( q , v[ 0 ] );
  return( std::pair( std::move( q ) , r ? Mag{ r } : Mag() ) );
  }
 const std::size_t n = v.size() , m = u.size() + 1 - n;
 const int s = std::countl_zero( v.back() );   // D1: normalize
 auto shift = [ s ]( Limb * r , const Limb * x , std::size_t nx ) {
  if( s == 0 ) {                            // the digit shifted out
   std::copy( x , x + nx , r );
   return( Limb( 0 ) );
   }
  const Limb out = x[ nx - 1 ] >> ( 64 - s );
  for( std::size_t i = nx ; i-- > 1 ; )
   r[ i ] = ( x[ i ] << s ) | ( x[ i - 1 ] >> ( 64 - s ) );
  r[ 0 ] = x[ 0 ] << s;
  return( out );
  };
 Mag vn( n ) , un( u.size() + 1 ) , q( m );
 shift( vn.data() , v.data() , n );
 un[ u.size() ] = shift( un.data() , u.data() , u.size() );
 if( school || ( m < kDivide ) )
  divBase( un.data() , m , vn.data() , n , q.data() );
 else {                                     // by blocks of n digits
  Mag t( n );
  std::size_t j = m;
  for( ; j > n ; j -= n )
   divRec( un.data() + j - n , n , vn.data() , n , q.data() + j - n ,
           t.data() );
  divRec( un.data() , j , vn.data() , n , q.data() , t.data() );
  }
 un.resize( n );                            // D8: unnormalize
 if( s )
  for( std::size_t i = 0 ; i < n ; ++i )
   un[ i ] = ( un[ i ] >> s ) | ( i + 1 < n ? un[ i + 1 ] << ( 64 - s ) : 0 );
 trim( q );
 trim( un );
 return( std::pair( std::move( q ) , std::move( un ) ) );
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
 ADS_AUDIT( out == divModSchool( a , b ) );
 return( out );
 }

inline std::pair< BigInteger , BigInteger > BigInteger::divModSchool(
                                const BigInteger & a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 auto [ q , r ] = divModMag( a.f_d , b.f_d , true );
 return( std::pair( BigInteger( a.f_neg != b.f_neg , std::move( q ) ) ,
                    BigInteger( a.f_neg , std::move( r ) ) ) );
 }

inline BigInteger operator/( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::divMod( a , b ).first );
 }

inline BigInteger operator%( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::divMod( a , b ).second );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- THE GREATEST COMMON DIVISOR ----------------------*/
/*--------------------------------------------------------------------------*/

inline std::uint64_t BigInteger::gcdWord( std::uint64_t a , std::uint64_t b )
{
 if( a == 0 ) return( b );
 if( b == 0 ) return( a );
 const int s = std::countr_zero( a | b );   // the binary gcd, 4.5.2B
 a >>= std::countr_zero( a );
 do {
  b >>= std::countr_zero( b );
  if( a > b ) std::swap( a , b );
  b -= a;
  } while( b );
 return( a << s );
 }

inline void BigInteger::linComb( Mag & u , Mag & v , std::int64_t A ,
                                 std::int64_t B , std::int64_t C ,
                                 std::int64_t D )
{
 // each row x X - y Y with x , y >= 0: the two products by one digit and
 // their difference, which never borrows past the top, as X is u or v
 const bool s1 = B > 0 , s2 = D > 0;
 const Limb x1 = Limb( s1 ? B : A ) , y1 = Limb( s1 ? -A : -B ) ,
            x2 = Limb( s2 ? D : C ) , y2 = Limb( s2 ? -C : -D );
 const std::size_t n = u.size();
 v.resize( n , 0 );
 Limb cx1 = 0 , cy1 = 0 , b1 = 0 , cx2 = 0 , cy2 = 0 , b2 = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb ui = u[ i ] , vi = v[ i ] ,
             p1 = detail::mulAdd( s1 ? vi : ui , x1 , cx1 , 0 , cx1 ) ,
             q1 = detail::mulAdd( s1 ? ui : vi , y1 , cy1 , 0 , cy1 ) ,
             p2 = detail::mulAdd( s2 ? vi : ui , x2 , cx2 , 0 , cx2 ) ,
             q2 = detail::mulAdd( s2 ? ui : vi , y2 , cy2 , 0 , cy2 );
  u[ i ] = detail::subBorrow( p1 , q1 , b1 );
  v[ i ] = detail::subBorrow( p2 , q2 , b2 );
  }
 trim( u );
 trim( v );
 }

inline BigInteger::Mag BigInteger::gcdMag( Mag u , Mag v )
{
 if( compareMag( u , v ) < 0 ) std::swap( u , v );
 constexpr std::int64_t kCap = std::int64_t( 1 ) << 30;
 while( v.size() > 1 ) {                    // L1: the leading 62 bits
  const std::size_t sh = 64 * u.size() -
                         std::size_t( std::countl_zero( u.back() ) ) - 62;
  auto top = [ sh ]( const Mag & x ) {
   auto at = [ & ]( std::size_t i ) {
    return( i < x.size() ? x[ i ] : Limb( 0 ) ); };
   const std::size_t w = sh / 64 , o = sh % 64;
   return( std::int64_t( o ? ( at( w ) >> o ) | ( at( w + 1 ) << ( 64 - o ) )
                           : at( w ) ) );
   };
  std::int64_t uh = top( u ) , vh = top( v ) , A = 1 , B = 0 , C = 0 , D = 1;
  for( ; ; ) {                              // L2, L3: Euclid on 62 bits
   const std::int64_t x = uh + A , y = vh + C , z = vh + D;
   if( ( y <= 0 ) || ( z <= 0 ) ) break;
   const std::int64_t q = x / y;
   if( q >= kCap ) break;
   // q = ( uh + B ) / z, q z = q y + q ( D - C ) < 2^62 + 2^61
   const std::int64_t e = uh + B - q * z;
   if( ( e < 0 ) || ( e >= z ) ) break;
   const std::int64_t T = A - q * C , S = B - q * D;
   if( ( T <= -kCap ) || ( T >= kCap ) || ( S <= -kCap ) || ( S >= kCap ) )
    break;
   A = C;
   C = T;
   B = D;
   D = S;
   const std::int64_t W = uh - q * vh;
   uh = vh;
   vh = W;
   }
  if( B == 0 ) {                            // L4: one long division
   Mag r = divModMag( u , v ).second;
   u = std::move( v );
   v = std::move( r );
   }
  else linComb( u , v , A , B , C , D );    // L4: the cofactors
  }
 if( v.empty() ) return( u );
 const Limb g = gcdWord( modSmall( u.data() , u.size() , v[ 0 ] ) , v[ 0 ] );
 return( Mag{ g } );
 }

inline BigInteger BigInteger::gcd( const BigInteger & a ,
                                   const BigInteger & b )
{
 const Mag & x = a.f_d.size() >= b.f_d.size() ? a.f_d : b.f_d;
 const Mag & y = a.f_d.size() >= b.f_d.size() ? b.f_d : a.f_d;
 BigInteger g;
 if( y.size() == 1 )                        // no copy of the longer one
  g.f_d = { gcdWord( modSmall( x.data() , x.size() , y[ 0 ] ) , y[ 0 ] ) };
 else g.f_d = gcdMag( x , y );
 ADS_AUDIT( g == gcdEuclid( a , b ) );
 return( g );
 }

inline BigInteger BigInteger::gcdEuclid( BigInteger a , BigInteger b )
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
