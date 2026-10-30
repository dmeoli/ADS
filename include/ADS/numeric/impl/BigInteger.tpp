/*--------------------------------------------------------------------------*/
/*-------------------------- File BigInteger.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BigInteger [see BigInteger.h], with their contracts:
 * the backend of GMP, and the algorithms of the book without it.
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
 std::uint64_t lo = mulWide( a , b , hi );
 lo += c;
 hi += lo < c;
 lo += d;
 hi += lo < d;
 return( lo );
 }

/// a + b + c, c in { 0 , 1 }, the carry out in c
inline std::uint64_t addCarry( std::uint64_t a , std::uint64_t b ,
                               std::uint64_t & c )
{
 const std::uint64_t s = a + b , t = s + c;
 c = std::uint64_t( s < a ) | std::uint64_t( t < s );
 return( t );
 }

/// a - b - c, c in { 0 , 1 }, the borrow out in c
inline std::uint64_t subBorrow( std::uint64_t a , std::uint64_t b ,
                                std::uint64_t & c )
{
 const std::uint64_t s = a - b , t = s - c;
 c = std::uint64_t( a < b ) | std::uint64_t( s < c );
 return( t );
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

/*--------------------------------------------------------------------------*/
/*------------------------------ THE NTT -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the number theoretic transform modulo a prime P < 2^31 with 2^26 | P - 1,
/// whose multiplicative group G generates
template< std::uint32_t P , std::uint32_t G >
struct Ntt {

 /// a b modulo P
 static std::uint32_t mul( std::uint32_t a , std::uint32_t b ) {
  return( std::uint32_t( std::uint64_t( a ) * b % P ) );
  }

 /// x^e modulo P, by the binary method
 static std::uint32_t power( std::uint32_t x , std::uint64_t e ) {
  std::uint32_t y = 1;
  for( ; e ; e >>= 1 ) {
   if( e & 1 ) y = mul( y , x );
   x = mul( x , x );
   }
  return( y );
  }

 /// the transform of a, of a length n that is a power of 2, in place: the
 /// items in the order of the reversed bits of their places, then the
 /// rounds of the butterflies of Cooley and Tukey on the blocks of length
 /// 2, 4, ..., n, by the root of unity of each length or by its inverse;
 /// the inverse times 1 / n
 static void transform( std::vector< std::uint32_t > & a , bool inverse ) {
  const std::size_t n = a.size();
  for( std::size_t i = 1 , j = 0 ; i < n ; ++i ) {
   std::size_t bit = n >> 1;
   for( ; j & bit ; bit >>= 1 ) j ^= bit;
   j ^= bit;
   if( i < j ) std::swap( a[ i ] , a[ j ] );
   }
  for( std::size_t len = 2 ; len <= n ; len <<= 1 ) {
   std::uint32_t w = power( G , ( P - 1 ) / len );
   if( inverse ) w = power( w , P - 2 );
   for( std::size_t i = 0 ; i < n ; i += len ) {
    std::uint32_t x = 1;
    for( std::size_t k = 0 ; k < len / 2 ; ++k ) {
     const std::uint32_t u = a[ i + k ] ,
                         v = mul( a[ i + k + len / 2 ] , x );
     a[ i + k ] = u + v >= P ? u + v - P : u + v;
     a[ i + k + len / 2 ] = u >= v ? u - v : u + P - v;
     x = mul( x , w );
     }
    }
   }
  if( inverse ) {
   const std::uint32_t ninv = power( std::uint32_t( n % P ) , P - 2 );
   for( auto & x : a ) x = mul( x , ninv );
   }
  }

 /// the cyclic convolution modulo P of x and y, both of length n
 static std::vector< std::uint32_t > convolve(
                    const std::vector< std::uint32_t > & x ,
                    const std::vector< std::uint32_t > & y ) {
  std::vector< std::uint32_t > a( x.size() ) , b( y.size() );
  for( std::size_t i = 0 ; i < x.size() ; ++i ) a[ i ] = x[ i ] % P;
  for( std::size_t i = 0 ; i < y.size() ; ++i ) b[ i ] = y[ i ] % P;
  transform( a , false );
  transform( b , false );
  for( std::size_t i = 0 ; i < a.size() ; ++i ) a[ i ] = mul( a[ i ] ,
                                                               b[ i ] );
  transform( a , true );
  return( a );
  }

 };  // end( struct Ntt )

/// the three primes of the NTT and their generators
using Ntt1 = Ntt< 469762049u , 3u >;        // 7 2^26 + 1
using Ntt2 = Ntt< 1811939329u , 13u >;      // 27 2^26 + 1
using Ntt3 = Ntt< 2013265921u , 31u >;      // 15 2^27 + 1

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------- THE FORM OF THE NUMBERS ------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::BigInteger( long long v ) : f_neg( v < 0 )
{
 auto m = v < 0 ? std::uint64_t( 0 ) - std::uint64_t( v ) : std::uint64_t( v );
 if( m ) f_d = { Limb( m ) };
 }

inline BigInteger::BigInteger( bool neg , Mag d ) : f_d( std::move( d ) )
{
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 ADS_INVARIANT( normal() );
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

inline int BigInteger::sign( void ) const
{
 return( f_d.empty() ? 0 : ( f_neg ? -1 : 1 ) );
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

/*--------------------------------------------------------------------------*/
/*------------------------- THE OPERATIONS ON SPANS ------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::Limb BigInteger::addN( Limb * r , const Limb * a ,
                                          std::size_t na , const Limb * b ,
                                          std::size_t nb )
{
 Limb c = 0;
 std::size_t i = 0;
 for( ; i < nb ; ++i ) r[ i ] = detail::addCarry( a[ i ] , b[ i ] , c );
 for( ; i < na ; ++i ) r[ i ] = detail::addCarry( a[ i ] , 0 , c );
 return( c );
 }

inline BigInteger::Limb BigInteger::subN( Limb * r , const Limb * a ,
                                          std::size_t na , const Limb * b ,
                                          std::size_t nb )
{
 Limb c = 0;
 std::size_t i = 0;
 for( ; i < nb ; ++i ) r[ i ] = detail::subBorrow( a[ i ] , b[ i ] , c );
 for( ; i < na ; ++i ) r[ i ] = detail::subBorrow( a[ i ] , 0 , c );
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

/*--------------------------------------------------------------------------*/
/*------------------------- THE SUM AND THE ORDER --------------------------*/
/*--------------------------------------------------------------------------*/

inline int BigInteger::compareMag( const Mag & a , const Mag & b )
{
 if( a.size() != b.size() ) return( a.size() < b.size() ? -1 : 1 );
#if ADS_BIGINT_GMP
 if( a.empty() ) return( 0 );
 const int c = mpn_cmp( reinterpret_cast< const mp_limb_t * >( a.data() ) ,
                        reinterpret_cast< const mp_limb_t * >( b.data() ) ,
                        mp_size_t( a.size() ) );
 return( ( c > 0 ) - ( c < 0 ) );
#else
 for( std::size_t i = a.size() ; i-- > 0 ; )
  if( a[ i ] != b[ i ] ) return( a[ i ] < b[ i ] ? -1 : 1 );
 return( 0 );
#endif
 }

inline BigInteger::Mag BigInteger::addMag( const Mag & a , const Mag & b )
{
 if( a.size() < b.size() ) return( addMag( b , a ) );
 if( b.empty() ) return( a );
 Mag r( a.size() + 1 );
#if ADS_BIGINT_GMP
 r.back() = mpn_add( reinterpret_cast< mp_limb_t * >( r.data() ) ,
                     reinterpret_cast< const mp_limb_t * >( a.data() ) ,
                     mp_size_t( a.size() ) ,
                     reinterpret_cast< const mp_limb_t * >( b.data() ) ,
                     mp_size_t( b.size() ) );
#else
 r.back() = addN( r.data() , a.data() , a.size() , b.data() , b.size() );
#endif
 trim( r );
 return( r );
 }

inline BigInteger::Mag BigInteger::subMag( const Mag & a , const Mag & b )
{
 if( b.empty() ) return( a );
 Mag r( a.size() );
#if ADS_BIGINT_GMP
 mpn_sub( reinterpret_cast< mp_limb_t * >( r.data() ) ,
          reinterpret_cast< const mp_limb_t * >( a.data() ) ,
          mp_size_t( a.size() ) ,
          reinterpret_cast< const mp_limb_t * >( b.data() ) ,
          mp_size_t( b.size() ) );
#else
 subN( r.data() , a.data() , a.size() , b.data() , b.size() );
#endif
 trim( r );
 return( r );
 }

inline BigInteger BigInteger::addSigned( const BigInteger & a ,
                                         const BigInteger & b , bool bneg )
{
 if( a.f_neg == bneg ) return( BigInteger( bneg , addMag( a.f_d , b.f_d ) ) );
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
 return( BigInteger::addSigned( a , b , ! b.f_neg && ! b.f_d.empty() ) );
 }

inline BigInteger & BigInteger::operator+=( const BigInteger & b )
{
 return( *this = *this + b );
 }

inline BigInteger & BigInteger::operator-=( const BigInteger & b )
{
 return( *this = *this - b );
 }

inline std::strong_ordering operator<=>( const BigInteger & a ,
                                         const BigInteger & b )
{
 if( a.sign() != b.sign() ) return( a.sign() <=> b.sign() );
 const int c = BigInteger::compareMag( a.f_d , b.f_d );
 return( ( a.f_neg ? -c : c ) <=> 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PRODUCT -------------------------------*/
/*--------------------------------------------------------------------------*/

inline void BigInteger::mulSchool( Limb * r , const Limb * a , std::size_t na ,
                                   const Limb * b , std::size_t nb )
{
 std::fill( r , r + na + nb , Limb( 0 ) );
 for( std::size_t j = 0 ; j < nb ; ++j )    // a row per digit of b
  r[ j + na ] = addMul1( r + j , a , na , b[ j ] );
 }

inline BigInteger::Mag BigInteger::mulNTT( const Limb * a , std::size_t na ,
                                           const Limb * b , std::size_t nb )
{
 // the pieces of 32 bits, padded to the length n of the convolution, a
 // power of 2 at least the 2 na + 2 nb - 1 coefficients of the product
 const std::size_t n = std::bit_ceil( 2 * ( na + nb ) );
 auto pieces = [ n ]( const Limb * x , std::size_t m ) {
  std::vector< std::uint32_t > f( n , 0 );
  for( std::size_t i = 0 ; i < m ; ++i ) {
   f[ 2 * i ] = std::uint32_t( x[ i ] );
   f[ 2 * i + 1 ] = std::uint32_t( x[ i ] >> 32 );
   }
  return( f );
  };
 const std::vector< std::uint32_t > fa = pieces( a , na ) ,
                                    fb = pieces( b , nb );
 const auto c1 = detail::Ntt1::convolve( fa , fb ) ,
            c2 = detail::Ntt2::convolve( fa , fb ) ,
            c3 = detail::Ntt3::convolve( fa , fb );
 // each coefficient c = x1 + p1 t2 + p1 p2 t3 by Garner, below 2^89 and so
 // below p1 p2 p3, added at its place 32 k
 constexpr std::uint64_t p1 = 469762049u , p2 = 1811939329u ,
                         p3 = 2013265921u , p12 = p1 * p2;
 const std::uint32_t i12 = detail::Ntt2::power( std::uint32_t( p1 % p2 ) ,
                                                p2 - 2 ) ,
                     i123 = detail::Ntt3::power( std::uint32_t( p12 % p3 ) ,
                                                 p3 - 2 );
 Mag r( na + nb + 2 , 0 );
 for( std::size_t k = 0 ; k + 1 < 2 * ( na + nb ) ; ++k ) {
  const std::uint64_t x1 = c1[ k ] ,
                      t2 = detail::Ntt2::mul( std::uint32_t(
                                ( c2[ k ] + p2 - x1 % p2 ) % p2 ) , i12 ) ,
                      y = ( x1 + p1 * t2 ) % p3 ,
                      t3 = detail::Ntt3::mul( std::uint32_t(
                                ( c3[ k ] + p3 - y ) % p3 ) , i123 );
  Limb hi , lo = detail::mulWide( p12 , t3 , hi );
  const Limb s = x1 + p1 * t2;              // below 2^63
  lo += s;
  hi += lo < s;
  Limb v[ 3 ];
  std::size_t nv;
  if( k % 2 == 0 ) {
   v[ 0 ] = lo;
   v[ 1 ] = hi;
   nv = 2;
   }
  else {
   v[ 0 ] = lo << 32;
   v[ 1 ] = ( hi << 32 ) | ( lo >> 32 );
   v[ 2 ] = hi >> 32;
   nv = 3;
   }
  const std::size_t at = k / 2;
  addN( r.data() + at , r.data() + at , r.size() - at , v , nv );
  }
 trim( r );
 return( r );
 }

inline std::size_t BigInteger::mulScratch( std::size_t n )
{
 // a level of n digits takes at most 4 ( n / 2 + 2 ) digits and leaves
 // factors of at most n / 2 + 2 to the next one, down to 4 digits
 std::size_t s = 0;
 for( ; n >= 4 ; n = n / 2 + 2 ) {
  s += 4 * ( n / 2 + 2 );
  if( n == 4 ) break;
  }
 return( s );
 }

inline void BigInteger::mulRec( Limb * r , const Limb * a , std::size_t na ,
                                const Limb * b , std::size_t nb , Method how ,
                                Limb * w )
{
 if( na < nb ) {
  std::swap( a , b );
  std::swap( na , nb );
  }
 if( ( nb < 4 ) || ( how == Method::SCHOOL ) ||
     ( ( how == Method::AUTO ) && ( nb < kKaratsuba ) ) ) {
  mulSchool( r , a , na , b , nb );
  return;
  }
 if( ( ( how == Method::FFT ) ||
       ( ( how == Method::AUTO ) && ( nb >= kFFT ) ) ) &&
     ( na + nb <= kNTTMax ) ) {
  const Mag p = mulNTT( a , na , b , nb );
  std::copy( p.begin() , p.end() , r );
  std::fill( r + p.size() , r + na + nb , Limb( 0 ) );
  return;
  }
 // Karatsuba, a = a1 b^l + a0 and b = b1 b^l + b0
 const std::size_t l = ( na + 1 ) / 2 , h = na - l , n = na + nb;
 if( nb <= l ) {                            // b alone: a1 b b^l + a0 b
  mulRec( r , a , l , b , nb , how , w );
  Limb * t = w;                             // a1 b, h + nb digits
  mulRec( t , a + l , h , b , nb , how , w + h + nb );
  std::fill( r + l + nb , r + n , Limb( 0 ) );
  addN( r + l , r + l , n - l , t , h + nb );
  return;
  }
 mulRec( r , a , l , b , l , how , w );     // z0, 2 l digits
 mulRec( r + 2 * l , a + l , h , b + l , nb - l , how , w );   // z2
 Limb * sa = w , * sb = w + l + 1 , * z1 = w + 2 * ( l + 1 );
 sa[ l ] = addN( sa , a , l , a + l , h );  // a0 + a1 and b0 + b1, l + 1
 sb[ l ] = addN( sb , b , l , b + l , nb - l );   // digits each
 mulRec( z1 , sa , l + 1 , sb , l + 1 , how , w + 4 * ( l + 1 ) );
 subN( z1 , z1 , 2 * ( l + 1 ) , r , 2 * l );   // z1 - z0 - z2 >= 0,
 subN( z1 , z1 , 2 * ( l + 1 ) , r + 2 * l , n - 2 * l );   // below b^{n-l}
 addN( r + l , r + l , n - l , z1 , std::min( 2 * ( l + 1 ) , n - l ) );
 }

inline BigInteger::Mag BigInteger::mulMag( const Mag & a , const Mag & b ,
                                           Method how )
{
 if( a.empty() || b.empty() ) return( Mag() );
#if ADS_BIGINT_GMP
 if( how == Method::AUTO ) {                // mpn_mul() or mpn_sqr()
  Mag r( a.size() + b.size() );
  mp_limb_t * const p = reinterpret_cast< mp_limb_t * >( r.data() );
  const mp_limb_t * const x = reinterpret_cast< const mp_limb_t * >(
                                                                 a.data() ) ,
                  * const y = reinterpret_cast< const mp_limb_t * >(
                                                                 b.data() );
  if( ( &a == &b ) || ( a == b ) ) mpn_sqr( p , x , mp_size_t( a.size() ) );
  else if( a.size() >= b.size() )
   mpn_mul( p , x , mp_size_t( a.size() ) , y , mp_size_t( b.size() ) );
  else mpn_mul( p , y , mp_size_t( b.size() ) , x , mp_size_t( a.size() ) );
  trim( r );
  return( r );
  }
#endif
 // the scratch space of Karatsuba allocated once, for all its levels
 Mag r( a.size() + b.size() ) ,
     w( mulScratch( std::max( a.size() , b.size() ) ) );
 mulRec( r.data() , a.data() , a.size() , b.data() , b.size() , how ,
         w.data() );
 trim( r );
 return( r );
 }

inline BigInteger BigInteger::multiplySchool( const BigInteger & a ,
                                              const BigInteger & b )
{
 return( BigInteger( a.f_neg != b.f_neg ,
                     mulMag( a.f_d , b.f_d , Method::SCHOOL ) ) );
 }

inline BigInteger BigInteger::multiplyKaratsuba( const BigInteger & a ,
                                                 const BigInteger & b )
{
 BigInteger p( a.f_neg != b.f_neg ,
               mulMag( a.f_d , b.f_d , Method::KARATSUBA ) );
 ADS_AUDIT( p == multiplySchool( a , b ) );
 return( p );
 }

inline BigInteger BigInteger::multiplyFFT( const BigInteger & a ,
                                           const BigInteger & b )
{
 BigInteger p( a.f_neg != b.f_neg , mulMag( a.f_d , b.f_d , Method::FFT ) );
 ADS_AUDIT( p == multiplySchool( a , b ) );
 return( p );
 }

inline BigInteger operator*( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger( a.f_neg != b.f_neg ,
                     BigInteger::mulMag( a.f_d , b.f_d ) ) );
 }

inline BigInteger & BigInteger::operator*=( const BigInteger & b )
{
 return( *this = *this * b );
 }

inline void BigInteger::addMul( BigInteger & r , const BigInteger & a ,
                                const BigInteger & b )
{
 r = r + a * b;
 }

inline BigInteger BigInteger::pow( const BigInteger & x , unsigned long n )
{
#if ADS_BIGINT_GMP
 mpz_t X , Y;
 mpz_init( Y );
 mpz_pow_ui( Y , gmpView( X , x ) , n );
 return( gmpTake( Y ) );
#else
 if( n == 0 ) return( BigInteger( 1 ) );
 BigInteger y = x;                          // from the leftmost bit of n
 for( int i = int( std::bit_width( n ) ) - 2 ; i >= 0 ; --i ) {
  y = y * y;
  if( ( n >> i ) & 1 ) y = y * x;
  }
 return( y );
#endif
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE DIVISION ------------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::Limb BigInteger::divSmall( Mag & a , Limb d )
{
 // a and d shifted by s bits, so that the top bit of d is set: the
 // quotient is the same, the remainder shifted
 const int s = std::countl_zero( d );
 const Limb dn = d << s;
 const std::size_t n = a.size();
 if( n == 0 ) return( 0 );
 Limb r = s ? a[ n - 1 ] >> ( 64 - s ) : 0;
 for( std::size_t i = n ; i-- > 0 ; ) {
  const Limb u = s ? ( a[ i ] << s ) | ( i ? a[ i - 1 ] >> ( 64 - s ) : 0 )
                   : a[ i ];
  const Limb q = detail::divWide( r , u , dn );
  r = u - q * dn;                           // below dn, modulo 2^64
  a[ i ] = q;
  }
 trim( a );
 return( r >> s );
 }

inline void BigInteger::mulAddSmall( Mag & a , Limb m , Limb c )
{
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  a[ i ] = detail::mulAdd( a[ i ] , m , c , 0 , c );
 if( c ) a.push_back( c );
 }

inline std::pair< BigInteger::Mag , BigInteger::Mag > BigInteger::divModMag(
                                  const Mag & u , const Mag & v , bool school )
{
 if( compareMag( u , v ) < 0 ) return( std::pair( Mag() , u ) );
 const std::size_t n = v.size() , m = u.size() - n;
#if ADS_BIGINT_GMP
 if( ! school ) {                           // mpn_tdiv_qr(), in place
  Mag q( m + 1 ) , r( n );
  mpn_tdiv_qr( reinterpret_cast< mp_limb_t * >( q.data() ) ,
               reinterpret_cast< mp_limb_t * >( r.data() ) , 0 ,
               reinterpret_cast< const mp_limb_t * >( u.data() ) ,
               mp_size_t( u.size() ) ,
               reinterpret_cast< const mp_limb_t * >( v.data() ) ,
               mp_size_t( n ) );
  trim( q );
  trim( r );
  return( std::pair( std::move( q ) , std::move( r ) ) );
  }
#else
 (void) school;
#endif
 if( n == 1 ) {                             // the short division
  Mag q = u;
  const Limb r = divSmall( q , v[ 0 ] );
  return( std::pair( std::move( q ) , r ? Mag{ r } : Mag() ) );
  }
 // Algorithm D: D1, the shift of u and v by s bits, u one digit longer
 const int s = std::countl_zero( v.back() );
 auto shifted = [ s ]( const Mag & x , std::size_t len ) {
  Mag y( len , 0 );
  for( std::size_t i = 0 ; i < x.size() ; ++i ) {
   y[ i ] |= x[ i ] << s;
   if( s && ( i + 1 < len ) ) y[ i + 1 ] = x[ i ] >> ( 64 - s );
   }
  return( y );
  };
 const Mag w = shifted( v , n );
 Mag x = shifted( u , u.size() + 1 ) , q( m + 1 );
 const Limb v1 = w[ n - 1 ] , v2 = w[ n - 2 ];
 for( std::size_t j = m + 1 ; j-- > 0 ; ) {
  // D3: the trial quotient from the two leading digits, and its test
  // on the third, while the remainder rh is below b
  Limb qh , rh;
  bool big;
  if( x[ j + n ] >= v1 ) {                  // then x[ j + n ] == v1
   qh = ~Limb( 0 );
   rh = x[ j + n - 1 ] + v1;
   big = rh < v1;
   }
  else {
   qh = detail::divWide( x[ j + n ] , x[ j + n - 1 ] , v1 );
   rh = x[ j + n - 1 ] - qh * v1;
   big = false;
   }
  while( ! big ) {
   Limb hi;
   const Limb lo = detail::mulWide( qh , v2 , hi );
   if( ( hi < rh ) || ( ( hi == rh ) && ( lo <= x[ j + n - 2 ] ) ) ) break;
   --qh;
   rh += v1;
   big = rh < v1;
   }
  // D4, D5, D6: x[ j .. j + n ] -= qh w, and w added back if negative
  const Limb c = subMul1( x.data() + j , w.data() , n , qh ) ,
             t = x[ j + n ];
  x[ j + n ] = t - c;
  if( t < c ) {
   --qh;
   x[ j + n ] += addN( x.data() + j , x.data() + j , n , w.data() , n );
   }
  q[ j ] = qh;
  }
 // D8: the remainder, shifted back
 Mag r( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  r[ i ] = s ? ( x[ i ] >> s ) | ( x[ i + 1 ] << ( 64 - s ) ) : x[ i ];
 trim( q );
 trim( r );
 return( std::pair( std::move( q ) , std::move( r ) ) );
 }

inline std::pair< BigInteger , BigInteger > BigInteger::divMod(
                                const BigInteger & a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 auto [ q , r ] = divModMag( a.f_d , b.f_d , false );
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
 ADS_PRE( b.sign() != 0 );
#if ADS_BIGINT_GMP
 mpz_t A , B , Q;                           // mpz_tdiv_q(), no remainder
 mpz_init( Q );
 mpz_tdiv_q( Q , BigInteger::gmpView( A , a ) , BigInteger::gmpView( B , b ) );
 BigInteger q = BigInteger::gmpTake( Q );
#else
 BigInteger q = BigInteger::divMod( a , b ).first;
#endif
 ADS_AUDIT( q == BigInteger::divModSchool( a , b ).first );
 return( q );
 }

inline BigInteger operator%( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::divMod( a , b ).second );
 }

inline BigInteger BigInteger::divExact( BigInteger a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
#if ADS_BIGINT_GMP
 mpz_t A , B , Q;                           // mpz_divexact()
 mpz_init( Q );
 mpz_divexact( Q , gmpView( A , a ) , gmpView( B , b ) );
 BigInteger q = gmpTake( Q );
#else
 BigInteger q = a / b;
#endif
 ADS_AUDIT( q * b == a );
 return( q );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONVERSIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

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
#if ADS_BIGINT_GMP
 if( s[ i ] != '0' ) {
  // mpn_set_str() on the values of the figures, the most significant
  // first, into room for the bits of L figures and one digit more
  std::vector< unsigned char > f( L );
  for( std::size_t j = 0 ; j < L ; ++j )
   f[ j ] = static_cast< unsigned char >( detail::digitValue( s[ i + j ] ) );
  f_d.resize( ( L * std::size_t( std::bit_width( unsigned( base - 1 ) ) )
                + 63 ) / 64 + 1 );
  f_d.resize( std::size_t( mpn_set_str(
                            reinterpret_cast< mp_limb_t * >( f_d.data() ) ,
                            f.data() , L , base ) ) );
  }
#else
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
 else f_d = fromStringSchool( s.substr( i ) , base ).f_d;
#endif
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 ADS_INVARIANT( normal() );
 ADS_AUDIT( *this == fromStringSchool( s , base ) );
 }

inline void BigInteger::figures( Mag a , int base , std::string & out )
{
 // the chunks of k figures, the least significant first, by the short
 // divisions by P, then reversed with the zeros on the left removed
 const auto [ P , k ] = detail::bigBase( base );
 std::string t;
 while( ! a.empty() ) {
  Limb r = divSmall( a , P );
  for( int j = 0 ; j < k ; ++j , r /= Limb( base ) )
   t.push_back( detail::kFigures[ r % Limb( base ) ] );
  }
 while( ( t.size() > 1 ) && ( t.back() == '0' ) ) t.pop_back();
 out.append( t.rbegin() , t.rend() );
 }

inline std::string BigInteger::toString( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 std::string out;
 if( f_neg ) out.push_back( '-' );
 const std::size_t n = f_d.size();
#if ADS_BIGINT_GMP
 mpz_t X;                                   // mpz_get_str(), in place
 mpz_srcptr x = gmpView( X , f_d.data() , n );
 const std::size_t o = out.size();
 out.resize( o + mpz_sizeinbase( x , base ) + 1 );
 mpz_get_str( out.data() + o , base , x );
 out.resize( o + std::strlen( out.data() + o ) );
#else
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
 else figures( f_d , base , out );
#endif
 ADS_AUDIT( out == toStringSchool( base ) );
 return( out );
 }

inline std::string BigInteger::toStringSchool( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 std::string out;
 if( f_neg ) out.push_back( '-' );
 figures( f_d , base , out );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SQUARE ROOT -----------------------------*/
/*--------------------------------------------------------------------------*/

inline std::pair< BigInteger , BigInteger > BigInteger::sqrtRem(
                                                       const BigInteger & a )
{
 ADS_PRE( a.sign() >= 0 );
#if ADS_BIGINT_GMP
 const std::size_t n = a.f_d.size();
 Mag s( ( n + 1 ) / 2 ) , r( n );
 if( n > 0 )                                // mpn_sqrtrem(), in place
  r.resize( std::size_t( mpn_sqrtrem(
                          reinterpret_cast< mp_limb_t * >( s.data() ) ,
                          reinterpret_cast< mp_limb_t * >( r.data() ) ,
                          reinterpret_cast< const mp_limb_t * >(
                                                         a.f_d.data() ) ,
                          mp_size_t( n ) ) ) );
 std::pair< BigInteger , BigInteger > out( BigInteger( false ,
                                                       std::move( s ) ) ,
                                           BigInteger( false ,
                                                       std::move( r ) ) );
#else
 BigInteger s = sqrtNewton( a );
 std::pair< BigInteger , BigInteger > out( s , a - s * s );
#endif
 ADS_AUDIT( ( out.first * out.first + out.second == a ) &&
            ( out.second.sign() >= 0 ) &&
            ( out.second <= out.first + out.first ) );
 ADS_AUDIT( out.first == sqrtNewton( a ) );
 return( out );
 }

inline BigInteger BigInteger::sqrt( const BigInteger & a )
{
 ADS_PRE( a.sign() >= 0 );
#if ADS_BIGINT_GMP
 const std::size_t n = a.f_d.size();
 Mag s( ( n + 1 ) / 2 );
 if( n > 0 )                                // mpn_sqrtrem(), no remainder
  mpn_sqrtrem( reinterpret_cast< mp_limb_t * >( s.data() ) , nullptr ,
               reinterpret_cast< const mp_limb_t * >( a.f_d.data() ) ,
               mp_size_t( n ) );
 BigInteger r( false , std::move( s ) );
#else
 BigInteger r = sqrtNewton( a );
#endif
 ADS_AUDIT( r == sqrtNewton( a ) );
 return( r );
 }

inline BigInteger BigInteger::sqrtNewton( const BigInteger & a )
{
 ADS_PRE( a.sign() >= 0 );
 if( a.sign() == 0 ) return( a );
 // from 2^ceil( bits / 2 ), above the root, x = ( x + a / x ) / 2 while
 // it decreases
 const std::size_t bits = 64 * a.f_d.size() -
                          std::size_t( std::countl_zero( a.f_d.back() ) ) ,
                   e = ( bits + 1 ) / 2;
 Mag p( e / 64 + 1 , 0 );
 p.back() = Limb( 1 ) << ( e % 64 );
 BigInteger x( false , std::move( p ) );
 for( ; ; ) {
  BigInteger y = x + a / x;
  divSmall( y.f_d , 2 );
  if( y >= x ) return( x );
  x = std::move( y );
  }
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

inline BigInteger::Mag BigInteger::gcdLehmer( Mag u , Mag v )
{
 if( compareMag( u , v ) < 0 ) std::swap( u , v );
 // p x - q y for the digits x and y, by the rows of Algorithm M; the
 // result is not negative
 auto lin = []( const Mag & p , Limb x , const Mag & q , Limb y ) {
  Mag t( std::max( p.size() + 1 , q.size() ) , 0 );
  t[ p.size() ] = addMul1( t.data() , p.data() , p.size() , x );
  Limb c = subMul1( t.data() , q.data() , q.size() , y );
  for( std::size_t i = q.size() ; c && ( i < t.size() ) ; ++i ) {
   const Limb z = t[ i ];
   t[ i ] = z - c;
   c = z < c;
   }
  trim( t );
  return( t );
  };
 while( v.size() >= 2 ) {
  // the leading 60 bits of u and the bits of v at the same place
  const std::size_t sh = 64 * u.size() -
                         std::size_t( std::countl_zero( u.back() ) ) - 60;
  auto top = [ sh ]( const Mag & x ) {
   const std::size_t w = sh / 64 , o = sh % 64;
   if( w >= x.size() ) return( std::int64_t( 0 ) );
   Limb y = x[ w ] >> o;
   if( o && ( w + 1 < x.size() ) ) y |= x[ w + 1 ] << ( 64 - o );
   return( std::int64_t( y & ( ( Limb( 1 ) << 60 ) - 1 ) ) );
   };
  std::int64_t uh = top( u ) , vh = top( v ) , A = 1 , B = 0 , C = 0 ,
               D = 1;
  // L2, L3: the steps of Euclid while the quotients of the two ends of
  // the intervals agree, and so are those of u and v
  while( ( vh + C > 0 ) && ( vh + D > 0 ) && ( uh + A >= 0 ) &&
         ( uh + B >= 0 ) ) {
   const std::int64_t q = ( uh + A ) / ( vh + C );
   if( q != ( uh + B ) / ( vh + D ) ) break;
   std::int64_t t = A - q * C;
   A = C;
   C = t;
   t = B - q * D;
   B = D;
   D = t;
   t = uh - q * vh;
   uh = vh;
   vh = t;
   }
  if( B == 0 ) {                            // L4: a division
   Mag r = divModMag( u , v , true ).second;
   u = std::move( v );
   v = std::move( r );
   }
  else {
   // L4: ( A u + B v , C u + D v ), A and B of opposite signs, as C and D
   // are (one of them 0 after the first step), the positive one first
   Mag x = B > 0 ? lin( v , Limb( B ) , u , Limb( -A ) )
                 : lin( u , Limb( A ) , v , Limb( -B ) ) ,
       y = D > 0 ? lin( v , Limb( D ) , u , Limb( -C ) )
                 : lin( u , Limb( C ) , v , Limb( -D ) );
   u = std::move( x );
   v = std::move( y );
   }
  }
 if( v.empty() ) return( u );
 const Limb d = v[ 0 ] , r = divSmall( u , d );
 return( Mag{ gcdWord( d , r ) } );
 }

inline BigInteger BigInteger::gcd( const BigInteger & a ,
                                   const BigInteger & b )
{
#if ADS_BIGINT_GMP
 mpz_t A , B , G;                           // mpz_gcd()
 mpz_init( G );
 mpz_gcd( G , gmpView( A , a.f_d.data() , a.f_d.size() ) ,
          gmpView( B , b.f_d.data() , b.f_d.size() ) );
 BigInteger g = gmpTake( G );
#else
 BigInteger g( false , gcdLehmer( a.f_d , b.f_d ) );
#endif
 ADS_AUDIT( g == gcdEuclid( a , b ) );
 return( g );
 }

inline BigInteger BigInteger::gcdEuclid( BigInteger a , BigInteger b )
{
 a = a.abs();
 b = b.abs();
 while( b.sign() != 0 ) {
  BigInteger r = BigInteger( false ,
                             divModMag( a.f_d , b.f_d , true ).second );
  a = std::move( b );
  b = std::move( r );
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE BACKEND OF GMP --------------------------*/
/*--------------------------------------------------------------------------*/

#if ADS_BIGINT_GMP

static_assert( sizeof( mp_limb_t ) == sizeof( BigInteger::Limb ) ,
               "the limbs of GMP are the digits of BigInteger" );

inline mpz_srcptr BigInteger::gmpView( mpz_t z , const Limb * d ,
                                       std::size_t n )
{
 return( mpz_roinit_n( z , reinterpret_cast< const mp_limb_t * >( d ) ,
                       mp_size_t( n ) ) );
 }

inline mpz_srcptr BigInteger::gmpView( mpz_t z , const BigInteger & x )
{
 const mp_size_t n = mp_size_t( x.f_d.size() );
 return( mpz_roinit_n( z , reinterpret_cast< const mp_limb_t * >(
                                                         x.f_d.data() ) ,
                       x.f_neg ? -n : n ) );
 }

inline BigInteger BigInteger::gmpNumber( mpz_srcptr z )
{
 const Limb * d = reinterpret_cast< const Limb * >( mpz_limbs_read( z ) );
 return( BigInteger( mpz_sgn( z ) < 0 , Mag( d , d + mpz_size( z ) ) ) );
 }

inline BigInteger BigInteger::gmpTake( mpz_ptr z )
{
 BigInteger x = gmpNumber( z );
 mpz_clear( z );
 return( x );
 }

#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BigInteger.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
