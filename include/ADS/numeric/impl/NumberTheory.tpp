/*--------------------------------------------------------------------------*/
/*------------------------- File NumberTheory.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of NumberTheory.h [see NumberTheory.h].
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

inline U64 gcd( U64 s , U64 m )
{
 while( m != 0 ) {
  const U64 r = s % m;
  s = m;
  m = r;
  }
 return( s );
 }

constexpr U64 binaryGcd( U64 a , U64 b )
{
 if( a == 0 ) return( b );
 if( b == 0 ) return( a );
 const int k = std::countr_zero( a | b );   // the common power of 2
 a >>= std::countr_zero( a );
#if defined( __x86_64__ ) && ( defined( __GNUC__ ) || defined( __clang__ ) )
 if( ! std::is_constant_evaluated() ) {
  // the shift by the trailing zeros of b - a, which are those of a - b,
  // starts with the difference, the minimum and | a - b | by two
  // conditional moves next to it: 5 cycles a step (sub, tzcnt, shrx)
  b >>= std::countr_zero( b );
  for( U64 d = b - a ; d != 0 ; d = b - a ) {   // a , b odd
   const U64 e = a - b;
   const int s = std::countr_zero( d );
   asm( "cmpq %[a], %[b]\n\t"
        "cmovbq %[e], %[d]\n\t"
        "cmovbq %[b], %[a]"
        : [d] "+&r"( d ) , [a] "+&r"( a ) : [b] "r"( b ) , [e] "r"( e )
        : "cc" );
   b = d >> s;
   }
  return( a << k );
  }
#endif
 do {                                       // a odd, b not 0
  b >>= std::countr_zero( b );
  const U64 lo = a < b ? a : b , hi = a < b ? b : a;
  a = lo;                                   // min and | a - b |, both by
  b = hi - lo;                              // conditional moves
  } while( b != 0 );
 return( a << k );
 }

namespace detail {

/// the product of 128 bits of a and b, its high half in hi
constexpr U64 mul128( U64 a , U64 b , U64 & hi )
{
#ifdef __SIZEOF_INT128__
 const unsigned __int128 p = ( unsigned __int128 )( a ) * b;
 hi = U64( p >> 64 );
 return( U64( p ) );
#else
 const U64 a0 = a & 0xFFFFFFFFULL , a1 = a >> 32 ,
           b0 = b & 0xFFFFFFFFULL , b1 = b >> 32;
 const U64 p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 , p11 = a1 * b1;
 const U64 mid = ( p00 >> 32 ) + ( p01 & 0xFFFFFFFFULL ) +
                 ( p10 & 0xFFFFFFFFULL );
 hi = p11 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 );
 return( ( mid << 32 ) | ( p00 & 0xFFFFFFFFULL ) );
#endif
 }

/// ( hi 2^64 + lo ) mod m, by one division of 128 bits by 64 where the
/// processor has it, instead of the call that divides two of 128 bits
/// \pre hi < m
constexpr U64 mod128( U64 hi , U64 lo , U64 m )
{
#if defined( __x86_64__ ) && ( defined( __GNUC__ ) || defined( __clang__ ) )
 if( ! std::is_constant_evaluated() ) {
  U64 q , r;
  asm( "divq %4" : "=a"( q ) , "=d"( r ) : "a"( lo ) , "d"( hi ) , "rm"( m ) );
  return( r );
  }
#endif
#ifdef __SIZEOF_INT128__
 return( U64( ( ( unsigned __int128 )( hi ) << 64 | lo ) % m ) );
#else
 for( int i = 0 ; i < 64 ; ++i ) {          // the bits of lo one by one
  const bool over = hi >> 63;
  hi = ( hi << 1 ) | ( lo >> 63 );
  lo <<= 1;
  if( over || ( hi >= m ) ) hi -= m;
  }
 return( hi );
#endif
 }

}  // end( namespace detail )

constexpr Montgomery64::Montgomery64( U64 n )
 : f_n( n ) , f_inv( 0 ) , f_one( 0 ) , f_r2( 0 )
{
 ADS_PRE( ( n & 1 ) == 1 );
 U64 x = ( 3 * n ) ^ 2;                     // n^-1 mod 2^5; each step of
 for( int i = 0 ; i < 4 ; ++i )             // Newton doubles the bits
  x *= 2 - n * x;
 f_inv = x;
 f_one = n > 1 ? detail::mod128( 1 , 0 , n ) : 0;   // 2^64 mod n
 f_r2 = n > 1 ? detail::mod128( f_one , 0 , n ) : 0;
 ADS_POST( f_inv * n == 1 );
 }

constexpr U64 Montgomery64::reduce( U64 hi , U64 lo ) const
{
 // q = lo n^-1 makes hi 2^64 + lo - q n a multiple of 2^64, and its
 // quotient hi - ( q n ) / 2^64 lies in ( -n , n )
 U64 qnHi;
 detail::mul128( lo * f_inv , f_n , qnHi );
 const U64 r = hi - qnHi;
 return( hi < qnHi ? r + f_n : r );
 }

constexpr U64 Montgomery64::mul( U64 a , U64 b ) const
{
 ADS_PRE( ( a < f_n ) && ( b < f_n ) );
 U64 hi;
 const U64 lo = detail::mul128( a , b , hi );
 return( reduce( hi , lo ) );
 }

constexpr U64 Montgomery64::to( U64 x ) const
{
 return( mul( x % f_n , f_r2 ) );
 }

constexpr U64 Montgomery64::pow( U64 x , U64 v ) const
{
 ADS_PRE( x < f_n );
 U64 r = f_one;                             // right to left: the squares
 for( ; v ; v >>= 1 ) {                     // and the products are two
  const U64 t = mul( r , x );               // chains that overlap, and the
  r ^= ( r ^ t ) & ( 0 - ( v & 1 ) );       // bit selects with no branch
  x = mul( x , x );
  }
 return( r );
 }

namespace detail {

/// the Montgomery arithmetic of the odd n < 2^62 with lazy reduction: the
/// forms stay in [ 0 , 2 n ), so that a product, below 4 n^2 < n 2^64,
/// needs no final comparison, which shortens the chain of dependent
/// operations of a power by one comparison and one move in each product
struct MontgomeryLazy {
 U64 n , inv , r1 , r2;
 explicit MontgomeryLazy( U64 m ) : n( m ) , inv( 0 ) , r1( 0 ) , r2( 0 ) {
  U64 x = ( 3 * n ) ^ 2;
  for( int i = 0 ; i < 4 ; ++i ) x *= 2 - n * x;
  inv = x;
  r1 = mod128( 1 , 0 , n );
  r2 = mod128( r1 , 0 , n );
  }
 [[nodiscard]] U64 modulus( void ) const { return( n ); }
 [[nodiscard]] U64 one( void ) const { return( r1 ); }
 /// a b R^-1 mod n in ( 0 , 2 n ), a , b < 2 n
 [[nodiscard]] U64 mul( U64 a , U64 b ) const {
  U64 hi , qnHi;
  const U64 lo = mul128( a , b , hi );
  mul128( lo * inv , n , qnHi );
  return( hi - qnHi + n );
  }
 [[nodiscard]] U64 to( U64 x ) const {
  return( mul( x < n ? x : x % n , r2 ) );
  }
 /// the form in [ 0 , n )
 [[nodiscard]] U64 canon( U64 x ) const { return( x >= n ? x - n : x ); }
 /// the number of the form x < 2 n
 [[nodiscard]] U64 from( U64 x ) const {
  U64 qnHi;
  mul128( x * inv , n , qnHi );
  return( qnHi ? n - qnHi : 0 );           // ( x - q n ) / 2^64
  }
 };

/// the Montgomery arithmetic of the odd n < 2^32 with R = 2^32, whose
/// constants cost divisions of 32 and 64 bits instead of 128
struct Montgomery32 {
 U64 n;
 std::uint32_t inv , r1 , r2;
 explicit Montgomery32( U64 m ) : n( m ) , inv( 0 ) , r1( 0 ) , r2( 0 ) {
  const std::uint32_t n32 = std::uint32_t( n );
  std::uint32_t x = ( 3 * n32 ) ^ 2;
  for( int i = 0 ; i < 3 ; ++i ) x *= 2 - n32 * x;
  inv = x;
  r1 = std::uint32_t( ( 0 - n32 ) % n32 );  // 2^32 mod n
  const U64 sq = U64( r1 ) * r1;           // its square, whose quotient by
#if defined( __x86_64__ ) && ( defined( __GNUC__ ) || defined( __clang__ ) )
  std::uint32_t q , r;                      // n fits 32 bits: one division
  asm( "divl %4" : "=a"( q ) , "=d"( r )    // of 64 by 32 bits
       : "a"( std::uint32_t( sq ) ) , "d"( std::uint32_t( sq >> 32 ) ) ,
         "rm"( n32 ) );
  r2 = r;
#else
  r2 = std::uint32_t( sq % n );
#endif
  }
 [[nodiscard]] U64 modulus( void ) const { return( n ); }
 [[nodiscard]] U64 one( void ) const { return( r1 ); }
 /// a b R^-1 mod n, a , b < n
 [[nodiscard]] U64 mul( U64 a , U64 b ) const {
  const U64 t = a * b , mn = U64( std::uint32_t( t ) * inv ) * n;
  const U64 th = t >> 32 , mh = mn >> 32;   // the low halves are equal
  return( th < mh ? th - mh + n : th - mh );
  }
 [[nodiscard]] U64 to( U64 x ) const {
  return( mul( x < n ? x : x % n , r2 ) );
  }
 [[nodiscard]] U64 canon( U64 x ) const { return( x ); }
 };

/// the Montgomery arithmetic of R = 2^32 modulo the odd n < 2^32 with
/// none of the constants of Montgomery32 but n^-1: the form of x by one
/// division of 64 by 32 bits, the powers from the lowest bit of the
/// exponent that is 1, so that they need no form of 1
struct Montgomery32Lite {
 std::uint32_t n , inv;
 explicit Montgomery32Lite( std::uint32_t m ) : n( m ) ,
                                                 inv( ( 3 * m ) ^ 2 ) {
  for( int i = 0 ; i < 3 ; ++i ) inv *= 2 - n * inv;   // Newton
  }
 /// a c 2^-32 mod n, a , c < n
 [[nodiscard]] U64 mul( U64 a , U64 c ) const {
  const U64 t = a * c , mn = U64( std::uint32_t( t ) * inv ) * n;
  const U64 th = t >> 32 , mh = mn >> 32;   // the low halves are equal
  return( th < mh ? th - mh + n : th - mh );
  }
 /// x 2^32 mod n, x < n
 [[nodiscard]] U64 to( std::uint32_t x ) const {
  const U64 sq = U64( x ) << 32;
#if defined( __x86_64__ ) && ( defined( __GNUC__ ) || defined( __clang__ ) )
  std::uint32_t q , r;
  asm( "divl %4" : "=a"( q ) , "=d"( r )
       : "a"( std::uint32_t( sq ) ) , "d"( std::uint32_t( sq >> 32 ) ) ,
         "rm"( n ) );
  return( r );
#else
  return( sq % n );
#endif
  }
 /// the form of b^v, v odd, from the form b of b
 [[nodiscard]] U64 powOdd( U64 b , U64 v ) const {
  U64 r = b;
  for( v >>= 1 ; v ; v >>= 1 ) {
   b = mul( b , b );
   const U64 t = mul( r , b );
   r = ( v & 1 ) ? t : r;
   }
  return( r );
  }
 };

/// the strong probable primality of the odd n < 2^32 to the base b < n,
/// with n - 1 = 2^s d, the comparisons on the numbers after one reduction
inline bool strongProbablePrime32( std::uint32_t n , std::uint32_t b ,
                                   std::uint32_t d , int s )
{
 const Montgomery32Lite M( n );
 U64 r = M.powOdd( M.to( b ) , d ) , y = M.mul( r , 1 );   // b^d mod n
 if( ( y == 1 ) || ( y == n - 1 ) )
  return( true );
 for( int i = 1 ; i < s ; ++i ) {
  r = M.mul( r , r );
  y = M.mul( r , 1 );
  if( y == n - 1 ) return( true );
  if( y == 1 ) return( false );
  }
 return( false );
 }

}  // end( namespace detail )

inline std::tuple< long long , long long , long long > extendedEuclid(
                                                   long long s , long long m )
{
 ADS_PRE( ( s >= 0 ) && ( m >= 0 ) );
 if( m == 0 )
  return( std::tuple( s , 1LL , 0LL ) );
 const auto [ d , p , q ] = extendedEuclid( m , s % m );
 const long long pp = q , qq = p - ( s / m ) * q;
 // p s + q m may overflow even if d does not: the identity is checked
 // modulo 2^64, where the unsigned arithmetic is exact
 ADS_POST( U64( d ) == U64( pp ) * U64( s ) + U64( qq ) * U64( m ) );
 return( std::tuple( d , pp , qq ) );
 }

inline long long modInverse( long long s , long long m )
{
 ADS_PRE( ( m >= 1 ) && ( s >= 0 ) );
 const auto [ d , p , q ] = extendedEuclid( s , m );
 ADS_PRE( d == 1 );
 const long long r = ( ( p % m ) + m ) % m;
 ADS_POST( mulMod( U64( s % m ) , U64( r ) , U64( m ) ) == U64( 1 % m ) );
 return( r );
 }

constexpr U64 mulMod61( U64 a , U64 b )
{
 ADS_PRE( ( a < kMersenne61 ) && ( b < kMersenne61 ) );
 const U64 a1 = a >> 32 , a0 = a & 0xFFFFFFFFULL ,
           b1 = b >> 32 , b0 = b & 0xFFFFFFFFULL;
 const U64 mid = a1 * b0 + a0 * b1;             // below 2^62
 const U64 lo = a0 * b0;
 U64 s = ( ( a1 * b1 ) << 3 )                   // 2^64 = 8
         + ( mid >> 29 )                        // 2^61 = 1
         + ( ( mid & ( ( U64( 1 ) << 29 ) - 1 ) ) << 32 )
         + ( lo & kMersenne61 ) + ( lo >> 61 );
 s = ( s & kMersenne61 ) + ( s >> 61 );
 if( s >= kMersenne61 ) s -= kMersenne61;
 ADS_POST( s < kMersenne61 );
 return( s );
 }

constexpr U64 mulMod( U64 a , U64 b , U64 m )
{
 ADS_PRE( m >= 1 );
 if( a >= m ) a %= m;
 if( b >= m ) b %= m;
 U64 r = 0;
 if( m <= ( U64( 1 ) << 32 ) )               // a b below 2^64
  r = a * b % m;
 else if( m == kMersenne61 )
  r = mulMod61( a , b );
 else {
#ifdef __SIZEOF_INT128__
  U64 hi;
  const U64 lo = detail::mul128( a , b , hi );   // hi < m, as a , b < m
  r = detail::mod128( hi , lo , m );
#else
  for( ; b ; b >>= 1 ) {                     // the doubling of Ahmes
   if( b & 1 ) r = ( r >= m - a ) ? r - ( m - a ) : r + a;
   a = ( a >= m - a ) ? a - ( m - a ) : a + a;
   }
#endif
  }
 ADS_POST( r < m );
 return( r );
 }

inline U64 powMod( U64 x , U64 v , U64 m )
{
 ADS_PRE( m >= 1 );
 if( ( m & 1 ) && ( m > 1 ) ) {
  if( m < ( U64( 1 ) << 32 ) ) {           // the powers of 2 of v by
   if( v == 0 )                             // squares, then the odd part
    return( 1 );
   const detail::Montgomery32Lite M( ( std::uint32_t( m ) ) );
   const int z = std::countr_zero( v );
   U64 b = M.to( std::uint32_t( x % m ) );
   for( int i = 0 ; i < z ; ++i ) b = M.mul( b , b );
   return( M.mul( M.powOdd( b , v >> z ) , 1 ) );
   }
  if( m >= ( U64( 1 ) << 62 ) ) {
   const Montgomery64 M( m );
   return( M.from( M.pow( M.to( x ) , v ) ) );
   }
  const detail::MontgomeryLazy M( m );     // the same with no comparison
  U64 r = M.one() , b = M.to( x );
  for( ; v ; v >>= 1 ) {
   const U64 t = M.mul( r , b );
   r ^= ( r ^ t ) & ( 0 - ( v & 1 ) );
   b = M.mul( b , b );
   }
  return( M.from( r ) );
  }
 U64 r = 1 % m;
 for( x %= m ; v ; v >>= 1 , x = mulMod( x , x , m ) )
  if( v & 1 )
   r = mulMod( r , x , m );
 return( r );
 }

inline bool isPrimeTrial( U64 n )
{
 if( n < 2 )
  return( false );
 for( U64 j = 2 ; j <= n / j ; ++j )
  if( n % j == 0 )
   return( false );
 return( true );
 }

inline bool witness( U64 n , U64 b )
{
 ADS_PRE( ( n >= 3 ) && ( n % 2 == 1 ) && ( b >= 1 ) && ( b < n ) );
 if( gcd( n , b ) != 1 )
  return( true );
 U64 m = n - 1;
 int v = 0;
 for( ; ( m & 1 ) == 0 ; m >>= 1 ) ++v;
 U64 x = powMod( b , m , n );
 if( ( x == 1 ) || ( x == n - 1 ) )
  return( false );
 for( int i = 1 ; i < v ; ++i ) {
  x = mulMod( x , x , n );
  if( x == n - 1 )
   return( false );
  }
 return( true );
 }

template< class R >
bool millerRabin( U64 n , int K , R & rng )
{
 ADS_PRE( K >= 0 );
 if( n < 4 )
  return( n == 2 || n == 3 );
 if( n % 2 == 0 )
  return( false );
 std::uniform_int_distribution< U64 > base( 2 , n - 2 );
 for( int j = 0 ; j < K ; ++j )
  if( witness( n , base( rng ) ) )
   return( false );
 return( true );
 }

namespace detail {

/// the 46 odd primes from 3 to 211 and two that never divide, their
/// inverses modulo 2^64 and the largest quotients ( 2^64 - 1 ) / p, apart
/// so that the tests of the 48 vectorize: n is a multiple of p iff
/// n p^-1 mod 2^64 is at most the quotient
struct SmallPrimes {
 U64 inv[ 48 ] , lim[ 48 ];
 std::uint32_t inv32[ 48 ] , lim32[ 48 ];  ///< the same modulo 2^32
 };

constexpr SmallPrimes smallPrimes( void )
{
 SmallPrimes S{};
 constexpr U64 P[ 46 ] = {
  3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67,
  71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139,
  149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211 };
 for( std::size_t i = 0 ; i < 48 ; ++i ) {
  if( i >= 46 ) {                           // n 1 > 0 never holds
   S.inv[ i ] = S.inv32[ i ] = 1;
   S.lim[ i ] = S.lim32[ i ] = 0;
   continue;
   }
  U64 x = ( 3 * P[ i ] ) ^ 2;
  for( int j = 0 ; j < 4 ; ++j ) x *= 2 - P[ i ] * x;
  S.inv[ i ] = x;
  S.lim[ i ] = ~U64( 0 ) / P[ i ];
  S.inv32[ i ] = std::uint32_t( x );
  S.lim32[ i ] = std::uint32_t( UINT32_MAX / P[ i ] );
  }
 return( S );
 }

inline constexpr SmallPrimes kSmallPrimes = smallPrimes();

/// the strong probable primality of the odd n > 2 to the k bases a, in
/// the arithmetic M modulo n, with n - 1 = 2^s d: the k powers are computed
/// in lockstep, k independent chains of products that the processor
/// overlaps, so that k bases cost little more than one
/// x in [ 0 , n ), x a form of the arithmetic M
template< class M_ >
U64 canonical( const M_ & M , U64 x )
{
 if constexpr( requires { M.canon( x ); } )
  return( M.canon( x ) );
 else
  return( x );
 }

template< int k , class M_ >
bool strongProbablePrimes( const M_ & M , const U64 * a , U64 d , int s )
{
 const U64 n = M.modulus() , one = M.one() , minusOne = n - one;
 U64 x[ k ];
 bool done[ k ];                            // the bases already passed
 if constexpr( k <= 2 ) {
  // right to left with no branch, as Montgomery64::pow(): few chains
  // need their overlap
  U64 b[ k ];
  for( int i = 0 ; i < k ; ++i ) {
   b[ i ] = M.to( a[ i ] );
   done[ i ] = canonical( M , b[ i ] ) == 0;   // a multiple of n
   x[ i ] = M.one();
   }
  for( U64 v = d ; v ; v >>= 1 ) {
   const U64 m = 0 - ( v & 1 );
   for( int i = 0 ; i < k ; ++i ) {
    const U64 t = M.mul( x[ i ] , b[ i ] );
    x[ i ] ^= ( x[ i ] ^ t ) & m;
    b[ i ] = M.mul( b[ i ] , b[ i ] );
    }
   }
  }
 else {
  // many chains fill the multiplier: the fewest products, by windows of
  // 4 bits from the top, each 4 squares and a product by a power of the
  // table P[ w ] = b^w, with no branch
  U64 P[ 16 ][ k ];
  for( int i = 0 ; i < k ; ++i ) {
   P[ 0 ][ i ] = M.one();
   P[ 1 ][ i ] = M.to( a[ i ] );
   done[ i ] = canonical( M , P[ 1 ][ i ] ) == 0;   // a multiple of n
   }
  for( int w = 2 ; w < 16 ; ++w )
   for( int i = 0 ; i < k ; ++i )
    P[ w ][ i ] = M.mul( P[ w - 1 ][ i ] , P[ 1 ][ i ] );
  int t = ( ( std::bit_width( d ) + 3 ) / 4 - 1 ) * 4;   // the top window
  for( int i = 0 ; i < k ; ++i ) x[ i ] = P[ ( d >> t ) & 15 ][ i ];
  for( t -= 4 ; t >= 0 ; t -= 4 ) {
   for( int q = 0 ; q < 4 ; ++q )
    for( int i = 0 ; i < k ; ++i ) x[ i ] = M.mul( x[ i ] , x[ i ] );
   const unsigned w = unsigned( d >> t ) & 15;
   for( int i = 0 ; i < k ; ++i ) x[ i ] = M.mul( x[ i ] , P[ w ][ i ] );
   }
  }
 int left = 0;
 for( int i = 0 ; i < k ; ++i ) {
  x[ i ] = canonical( M , x[ i ] );
  done[ i ] = done[ i ] || ( x[ i ] == one ) || ( x[ i ] == minusOne );
  left += ! done[ i ];
  }
 for( int r = 1 ; ( r < s ) && left ; ++r )
  for( int i = 0 ; i < k ; ++i )
   if( ! done[ i ] ) {
    x[ i ] = canonical( M , M.mul( x[ i ] , x[ i ] ) );
    if( x[ i ] == minusOne ) {
     done[ i ] = true;
     --left;
     }
    else if( x[ i ] == one )                // a square root of 1 not +-1
     return( false );
    }
 return( left == 0 );
 }

}  // end( namespace detail )

inline bool isPrime( U64 n )
{
 static constexpr U64 small[ 4 ] = {      // the primes below 256
  0x28208A20A08A28ACULL , 0x800228A202088288ULL ,
  0x8028208820A00A08ULL , 0x08028228800800A2ULL };
 if( n < 256 )
  return( ( small[ n >> 6 ] >> ( n & 63 ) ) & 1 );
 if( ( n & 1 ) == 0 )
  return( false );
 bool multiple = false;                     // of a prime up to 211, by
 if( n < ( U64( 1 ) << 32 ) ) {             // twice the lanes below 2^32
  const std::uint32_t n32 = std::uint32_t( n );
  for( std::size_t i = 0 ; i < 48 ; ++i )
   multiple |= std::uint32_t( n32 * detail::kSmallPrimes.inv32[ i ] ) <=
               detail::kSmallPrimes.lim32[ i ];
  }
 else
  for( std::size_t i = 0 ; i < 48 ; ++i )
   multiple |= n * detail::kSmallPrimes.inv[ i ] <=
               detail::kSmallPrimes.lim[ i ];
 if( multiple )
  return( false );
 if( n < 223 * 223 )                        // 223 the next prime
  return( true );
 // the fewest bases with no strong pseudoprime below the bound
 static constexpr U64 B3[] = { 4230279247111683200ULL ,
                               14694767155120705706ULL ,
                               16641139526367750375ULL } ,
  B4[] = { 2 , 141889084524735ULL , 1199124725622454117ULL ,
           11096072698276303650ULL } ,
  B5[] = { 2 , 4130806001517ULL , 149795463772692060ULL ,
           186635894390467037ULL , 3967304179347715805ULL } ,
  B6[] = { 2 , 123635709730000ULL , 9233062284813009ULL ,
           43835965440333360ULL , 761179012939631437ULL ,
           1263739024124850375ULL } ,
  B7[] = { 2 , 325 , 9375 , 28178 , 450775 , 9780504 , 1795265022 };
 const int s = std::countr_zero( n - 1 );
 const U64 d = ( n - 1 ) >> s;
 using detail::strongProbablePrimes;
 if( n < ( U64( 1 ) << 32 ) ) {
  // one base, read in a table of 1024 at the hash of n: for each bucket
  // the least base for which no odd composite of the bucket with no prime
  // factor up to 211 (those that come here) is a strong probable prime,
  // found by the exhaustive search over the 2^31 odd numbers below 2^32
  static constexpr unsigned char H[ 1024 ] = {
  5 , 19 , 3 , 12 , 2 , 18 , 2 , 10 , 3 , 14 , 14 , 2 , 10 , 6 , 3 , 5 , 11 ,
  7 , 7 , 5 , 11 , 5 , 10 , 3 , 15 , 5 , 17 , 3 , 5 , 11 , 5 , 11 , 7 , 14 ,
  18 , 5 , 3 , 6 , 7 , 2 , 3 , 6 , 21 , 3 , 14 , 6 , 17 , 6 , 57 , 6 , 7 ,
  6 , 2 , 13 , 2 , 2 , 17 , 20 , 7 , 2 , 12 , 15 , 6 , 2 , 2 , 6 , 10 , 5 ,
  2 , 28 , 10 , 7 , 11 , 7 , 24 , 3 , 26 , 17 , 6 , 14 , 3 , 7 , 24 , 5 ,
  10 , 3 , 6 , 6 , 7 , 2 , 6 , 2 , 2 , 14 , 7 , 5 , 6 , 3 , 3 , 10 , 19 , 5 ,
  11 , 2 , 13 , 3 , 5 , 5 , 11 , 6 , 11 , 13 , 6 , 7 , 22 , 28 , 3 , 3 , 3 ,
  11 , 19 , 20 , 2 , 5 , 20 , 13 , 6 , 5 , 13 , 7 , 3 , 2 , 6 , 7 , 18 , 10 ,
  17 , 2 , 2 , 6 , 6 , 5 , 3 , 19 , 12 , 10 , 11 , 11 , 19 , 11 , 13 , 2 ,
  13 , 5 , 10 , 18 , 22 , 5 , 10 , 3 , 3 , 7 , 11 , 6 , 33 , 2 , 21 , 5 ,
  15 , 7 , 2 , 12 , 13 , 11 , 6 , 6 , 13 , 5 , 13 , 13 , 3 , 7 , 31 , 3 , 7 ,
  3 , 10 , 11 , 6 , 19 , 6 , 5 , 13 , 3 , 67 , 2 , 2 , 6 , 3 , 29 , 10 , 11 ,
  7 , 3 , 2 , 2 , 14 , 17 , 2 , 6 , 5 , 3 , 33 , 6 , 15 , 2 , 10 , 14 , 2 ,
  3 , 2 , 11 , 2 , 19 , 2 , 2 , 21 , 2 , 2 , 41 , 2 , 10 , 7 , 12 , 14 , 7 ,
  2 , 10 , 11 , 6 , 15 , 20 , 5 , 6 , 20 , 13 , 10 , 11 , 63 , 3 , 13 , 14 ,
  2 , 13 , 6 , 10 , 2 , 3 , 29 , 11 , 2 , 15 , 10 , 2 , 3 , 5 , 6 , 7 , 19 ,
  11 , 21 , 13 , 15 , 2 , 2 , 10 , 11 , 5 , 2 , 21 , 19 , 10 , 6 , 2 , 7 ,
  20 , 2 , 3 , 14 , 18 , 6 , 7 , 10 , 6 , 6 , 6 , 2 , 19 , 3 , 37 , 3 , 6 ,
  3 , 5 , 21 , 6 , 5 , 10 , 2 , 3 , 2 , 17 , 5 , 19 , 28 , 11 , 3 , 11 , 5 ,
  5 , 12 , 6 , 10 , 14 , 14 , 23 , 13 , 2 , 17 , 14 , 3 , 7 , 58 , 7 , 19 ,
  5 , 2 , 2 , 7 , 5 , 37 , 10 , 17 , 10 , 7 , 7 , 7 , 19 , 3 , 2 , 30 , 15 ,
  6 , 2 , 5 , 10 , 21 , 12 , 13 , 19 , 5 , 5 , 2 , 10 , 11 , 11 , 12 , 2 ,
  5 , 11 , 5 , 10 , 10 , 2 , 7 , 46 , 15 , 2 , 7 , 15 , 2 , 11 , 6 , 22 ,
  13 , 2 , 3 , 5 , 3 , 26 , 15 , 6 , 5 , 13 , 6 , 6 , 2 , 2 , 24 , 7 , 34 ,
  19 , 2 , 7 , 5 , 3 , 6 , 2 , 24 , 13 , 11 , 5 , 2 , 5 , 11 , 13 , 6 , 3 ,
  5 , 17 , 7 , 15 , 2 , 5 , 6 , 5 , 6 , 3 , 11 , 6 , 11 , 7 , 3 , 6 , 11 ,
  2 , 6 , 7 , 2 , 2 , 6 , 10 , 13 , 3 , 5 , 14 , 5 , 11 , 2 , 15 , 3 , 3 ,
  6 , 7 , 3 , 7 , 2 , 6 , 5 , 10 , 2 , 7 , 3 , 3 , 10 , 11 , 11 , 5 , 17 ,
  23 , 11 , 6 , 13 , 2 , 6 , 5 , 5 , 11 , 7 , 18 , 17 , 10 , 11 , 11 , 6 ,
  17 , 3 , 15 , 2 , 3 , 14 , 12 , 12 , 10 , 7 , 13 , 2 , 6 , 6 , 2 , 5 , 6 ,
  26 , 15 , 31 , 2 , 38 , 11 , 7 , 7 , 19 , 2 , 6 , 5 , 3 , 40 , 7 , 10 , 2 ,
  11 , 13 , 10 , 10 , 10 , 10 , 17 , 6 , 11 , 7 , 6 , 5 , 11 , 2 , 10 , 7 ,
  3 , 13 , 13 , 11 , 11 , 5 , 10 , 23 , 2 , 7 , 2 , 52 , 26 , 10 , 3 , 28 ,
  22 , 3 , 2 , 2 , 15 , 10 , 7 , 3 , 11 , 2 , 6 , 3 , 6 , 2 , 15 , 2 , 6 ,
  5 , 7 , 10 , 2 , 2 , 15 , 2 , 5 , 12 , 5 , 5 , 6 , 3 , 2 , 2 , 3 , 6 , 3 ,
  7 , 6 , 10 , 15 , 10 , 7 , 15 , 13 , 2 , 7 , 7 , 2 , 3 , 3 , 11 , 26 , 5 ,
  7 , 10 , 12 , 18 , 3 , 2 , 19 , 26 , 15 , 22 , 2 , 11 , 11 , 2 , 2 , 2 ,
  7 , 10 , 2 , 20 , 5 , 6 , 10 , 7 , 23 , 6 , 6 , 7 , 10 , 17 , 14 , 7 , 6 ,
  5 , 6 , 2 , 2 , 6 , 3 , 2 , 15 , 6 , 11 , 17 , 5 , 6 , 5 , 15 , 10 , 5 ,
  3 , 13 , 26 , 2 , 5 , 29 , 13 , 2 , 2 , 15 , 12 , 7 , 2 , 2 , 2 , 3 , 11 ,
  13 , 5 , 2 , 7 , 3 , 5 , 2 , 14 , 7 , 5 , 6 , 6 , 2 , 18 , 13 , 31 , 2 ,
  2 , 15 , 5 , 6 , 3 , 23 , 2 , 15 , 2 , 13 , 2 , 5 , 7 , 3 , 6 , 6 , 11 ,
  5 , 12 , 2 , 13 , 5 , 28 , 24 , 3 , 13 , 5 , 3 , 2 , 6 , 10 , 11 , 2 , 2 ,
  3 , 11 , 7 , 14 , 12 , 2 , 2 , 2 , 10 , 17 , 17 , 11 , 2 , 17 , 5 , 37 ,
  21 , 12 , 6 , 20 , 10 , 7 , 12 , 3 , 33 , 35 , 6 , 19 , 3 , 18 , 17 , 6 ,
  10 , 15 , 6 , 14 , 5 , 6 , 5 , 6 , 38 , 6 , 3 , 3 , 5 , 3 , 3 , 15 , 10 ,
  23 , 17 , 6 , 37 , 15 , 10 , 10 , 2 , 6 , 17 , 19 , 7 , 14 , 10 , 6 , 5 ,
  15 , 6 , 18 , 6 , 5 , 11 , 14 , 3 , 2 , 6 , 13 , 7 , 10 , 11 , 3 , 13 , 2 ,
  6 , 15 , 7 , 6 , 10 , 5 , 10 , 2 , 5 , 10 , 6 , 10 , 17 , 2 , 2 , 24 , 2 ,
  6 , 10 , 3 , 3 , 2 , 2 , 2 , 3 , 11 , 11 , 3 , 17 , 2 , 17 , 7 , 5 , 12 ,
  23 , 6 , 52 , 15 , 6 , 7 , 20 , 13 , 24 , 5 , 2 , 2 , 2 , 7 , 7 , 6 , 18 ,
  23 , 2 , 10 , 6 , 10 , 13 , 2 , 11 , 3 , 5 , 5 , 3 , 12 , 12 , 10 , 13 ,
  11 , 15 , 11 , 59 , 11 , 21 , 7 , 23 , 37 , 10 , 7 , 13 , 2 , 2 , 14 , 7 ,
  5 , 5 , 6 , 13 , 13 , 2 , 2 , 6 , 18 , 11 , 11 , 3 , 14 , 12 , 12 , 11 ,
  7 , 12 , 2 , 2 , 17 , 7 , 18 , 5 , 15 , 38 , 13 , 10 , 2 , 5 , 2 , 14 , 3 ,
  17 , 12 , 3 , 3 , 7 , 6 , 13 , 15 , 2 , 17 , 12 , 2 , 11 , 30 , 6 , 2 ,
  17 , 7 , 2 , 6 , 17 , 23 , 5 , 19 , 2 , 5 , 2 , 10 , 2 , 5 , 13 , 7 , 14 ,
  7 , 13 , 6 , 11 , 2 , 5 , 2 , 13 , 11 , 11 , 5 , 7 , 3 , 7 , 2 , 15 , 13 ,
  2 , 10 , 6 , 14 , 2 , 13 , 22 , 17 , 5 , 17 , 6 , 7 , 12 , 5 , 6 , 3 , 6 ,
  5 , 2 , 2 , 3 , 6 , 6 , 11 , 2 , 6 , 22 , 14 , 5 , 3 , 22 , 5 , 10 , 3 ,
  40 , 6 , 11 , 3 , 11 , 3 , 2 , 6 , 37 , 5 , 7 , 13 , 5 , 13 , 6 , 7 , 23 ,
  13 , 15 , 15 , 14  };
  return( detail::strongProbablePrime32( std::uint32_t( n ) ,
           H[ ( std::uint32_t( n ) * 0x9E3779B1u ) >> 22 ] ,
           std::uint32_t( d ) , s ) );
  }
 // two bases first, as fast as one, which most composites fail; then the
 // others at once; with no comparison in the products below 2^62
 auto test = [ & ]( const auto & M ) {
  if( n < 350269456337ULL )
   return( strongProbablePrimes< 3 >( M , B3 , d , s ) );
  if( ! strongProbablePrimes< 2 >( M , n < 55245642489451ULL ? B4 :
                                   n < 7999252175582851ULL ? B5 :
                                   n < 585226005592931977ULL ? B6 : B7 ,
                                   d , s ) )
   return( false );
  if( n < 55245642489451ULL )
   return( strongProbablePrimes< 2 >( M , B4 + 2 , d , s ) );
  if( n < 7999252175582851ULL )
   return( strongProbablePrimes< 3 >( M , B5 + 2 , d , s ) );
  if( n < 585226005592931977ULL )
   return( strongProbablePrimes< 4 >( M , B6 + 2 , d , s ) );
  return( strongProbablePrimes< 5 >( M , B7 + 2 , d , s ) );
  };
 if( n < ( U64( 1 ) << 62 ) )
  return( test( detail::MontgomeryLazy( n ) ) );
 return( test( Montgomery64( n ) ) );
 }

namespace detail {

/// a factor of the odd composite m, by the rho of Pollard and Brent with
/// the constant c in the arithmetic M of m; possibly m itself
template< class M_ >
U64 rhoBrent( const M_ & M , U64 c )
{
 const U64 m = M.modulus() , cb = canonical( M , M.to( c ) );
 auto f = [ & ]( U64 v ) {                  // v^2 + c, in [ 0 , m )
  U64 t = canonical( M , M.mul( v , v ) ) + cb;
  return( t >= m ? t - m : t ); };
 auto dist = []( U64 a , U64 b ) { return( a > b ? a - b : b - a ); };
 constexpr U64 batch = 128;
 U64 y = canonical( M , M.to( 2 ) ) , x = y , ys = y ,
     q = canonical( M , M.one() ) , g = 1;
 for( U64 r = 1 ; g == 1 ; r *= 2 ) {
  x = y;
  for( U64 i = 0 ; i < r ; ++i ) y = f( y );
  for( U64 k = 0 ; ( k < r ) && ( g == 1 ) ; k += batch ) {
   ys = y;                                  // the batch from here
   for( U64 i = 0 ; ( i < batch ) && ( i < r - k ) ; ++i ) {
    y = f( y );
    q = canonical( M , M.mul( q , dist( x , y ) ) );
    }
   g = binaryGcd( q , m );
   }
  }
 if( g == m )                               // the batch one by one
  do {
   ys = f( ys );
   g = binaryGcd( dist( x , ys ) , m );
   } while( g == 1 );
 return( g );
 }

/// a factor of the odd composite m, neither 1 nor m
inline U64 factorOf( U64 m )
{
 for( U64 c = 1 ; ; ++c ) {
  U64 g;
  if( m < ( U64( 1 ) << 32 ) ) g = rhoBrent( Montgomery32( m ) , c );
  else if( m < ( U64( 1 ) << 62 ) ) g = rhoBrent( MontgomeryLazy( m ) , c );
  else g = rhoBrent( Montgomery64( m ) , c );
  if( ( g != 1 ) && ( g != m ) )
   return( g );
  }
 }

}  // end( namespace detail )

inline std::vector< U64 > factorize( U64 n )
{
 ADS_PRE( n >= 1 );
 [[maybe_unused]] const U64 n0 = n;
 std::vector< U64 > F;
 for( ; ( n & 1 ) == 0 ; n >>= 1 ) F.push_back( 2 );
 for( U64 p = 3 ; ( p < 256 ) && ( p * p <= n ) ; p += 2 )
  for( ; n % p == 0 ; n /= p ) F.push_back( p );
 std::vector< U64 > todo;                   // the cofactors to split
 if( n > 1 ) todo.push_back( n );
 while( ! todo.empty() ) {
  const U64 m = todo.back();
  todo.pop_back();
  if( ( m < 256 * 256 ) || isPrime( m ) )   // no factor below 256 left
   F.push_back( m );
  else {
   const U64 d = detail::factorOf( m );
   todo.push_back( d );
   todo.push_back( m / d );
   }
  }
 std::sort( F.begin() , F.end() );
 ADS_AUDIT( [ & ]( void ) {
  U64 prod = 1;
  for( const U64 p : F )
   if( ! isPrime( p ) ) return( false );
   else prod *= p;
  return( prod == n0 );
  }() );
 return( F );
 }

inline std::vector< U64 > sieve( U64 n )
{
 std::vector< U64 > primes;
 if( n < 2 )
  return( primes );
 BoolSet S( long( n ) + 1 );                // the integers 2 .. n
 for( U64 x = 2 ; x <= n ; ++x )
  S.insert( long( x ) );
 for( U64 p = 2 ; p <= n ; ++p ) {
  if( ! S.contains( long( p ) ) )           // p is the least item left
   continue;
  primes.push_back( p );
  if( p <= n / p )
   for( U64 m = p * p ; m <= n ; m += p )
    S.remove( long( m ) );
  }
 ADS_AUDIT( [ & ]( void ) {
  std::size_t i = 0;
  for( U64 x = 2 ; x <= n ; ++x )
   if( isPrimeTrial( x ) &&
       ( ( i >= primes.size() ) || ( primes[ i++ ] != x ) ) )
    return( false );
  return( i == primes.size() );
  }() );
 return( primes );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File NumberTheory.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
