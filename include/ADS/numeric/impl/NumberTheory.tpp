/*--------------------------------------------------------------------------*/
/*------------------------- File NumberTheory.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of NumberTheory.h [see NumberTheory.h].
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
  const Montgomery64 M( m );
  return( M.from( M.pow( M.to( x ) , v ) ) );
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
   S.inv[ i ] = 1;
   S.lim[ i ] = 0;
   continue;
   }
  U64 x = ( 3 * P[ i ] ) ^ 2;
  for( int j = 0 ; j < 4 ; ++j ) x *= 2 - P[ i ] * x;
  S.inv[ i ] = x;
  S.lim[ i ] = ~U64( 0 ) / P[ i ];
  }
 return( S );
 }

inline constexpr SmallPrimes kSmallPrimes = smallPrimes();

/// the strong probable primality of the odd n > 2 to the k bases a, in
/// the arithmetic M modulo n, with n - 1 = 2^s d: the k powers are computed
/// in lockstep, k independent chains of products that the processor
/// overlaps, so that k bases cost little more than one
template< int k >
bool strongProbablePrimes( const Montgomery64 & M , const U64 * a , U64 d ,
                           int s )
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
   done[ i ] = b[ i ] == 0;                 // the base is a multiple of n
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
   done[ i ] = P[ 1 ][ i ] == 0;            // the base is a multiple of n
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
  done[ i ] = done[ i ] || ( x[ i ] == one ) || ( x[ i ] == minusOne );
  left += ! done[ i ];
  }
 for( int r = 1 ; ( r < s ) && left ; ++r )
  for( int i = 0 ; i < k ; ++i )
   if( ! done[ i ] ) {
    x[ i ] = M.mul( x[ i ] , x[ i ] );
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
 bool multiple = false;                     // of a prime up to 211
 for( std::size_t i = 0 ; i < 48 ; ++i )
  multiple |= n * detail::kSmallPrimes.inv[ i ] <=
              detail::kSmallPrimes.lim[ i ];
 if( multiple )
  return( false );
 if( n < 223 * 223 )                        // 223 the next prime
  return( true );
 // the fewest bases with no strong pseudoprime below the bound
 static constexpr U64 B1[] = { 9345883071009581737ULL } ,
  B3[] = { 4230279247111683200ULL , 14694767155120705706ULL ,
           16641139526367750375ULL } ,
  B4[] = { 2 , 141889084524735ULL , 1199124725622454117ULL ,
           11096072698276303650ULL } ,
  B5[] = { 2 , 4130806001517ULL , 149795463772692060ULL ,
           186635894390467037ULL , 3967304179347715805ULL } ,
  B6[] = { 2 , 123635709730000ULL , 9233062284813009ULL ,
           43835965440333360ULL , 761179012939631437ULL ,
           1263739024124850375ULL } ,
  B7[] = { 2 , 325 , 9375 , 28178 , 450775 , 9780504 , 1795265022 };
 const Montgomery64 M( n );
 const int s = std::countr_zero( n - 1 );
 const U64 d = ( n - 1 ) >> s;
 using detail::strongProbablePrimes;
 // two bases first, as fast as one, which most composites fail; then the
 // others at once
 if( n < 341531ULL ) return( strongProbablePrimes< 1 >( M , B1 , d , s ) );
 if( n < ( U64( 1 ) << 32 ) ) {
  // the base 2 and a second one chosen by a hash of n among 32, each a
  // witness for every strong pseudoprime to the base 2 below 2^32 of its
  // bucket (the 2314 of them), so that no composite passes both
  static constexpr unsigned char H[ 32 ] = {
   34 , 33 , 17 , 15 , 13 , 45 , 59 , 163 , 15 , 41 , 7 , 33 , 21 , 53 , 7 ,
   59 , 83 , 15 , 11 , 35 , 15 , 106 , 15 , 39 , 110 , 51 , 38 , 30 , 30 ,
   38 , 7 , 17 };
  const U64 b[ 2 ] = { 2 , H[ ( std::uint32_t( n ) * 0x9E3779B1u ) >> 27 ] };
  return( strongProbablePrimes< 2 >( M , b , d , s ) );
  }
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
