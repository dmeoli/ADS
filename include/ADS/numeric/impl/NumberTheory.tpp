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
  r = U64( ( unsigned __int128 )( a ) * b % m );
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

inline bool isPrime( U64 n )
{
 if( n < 2 )
  return( false );
 for( U64 p : { 2 , 3 , 5 , 7 , 11 , 13 , 17 , 19 , 23 , 29 , 31 , 37 } ) {
  if( n == p )
   return( true );
  if( n % p == 0 )
   return( false );
  }
 for( U64 b : { 2 , 3 , 5 , 7 , 11 , 13 , 17 , 19 , 23 , 29 , 31 , 37 } )
  if( witness( n , b ) )
   return( false );
 return( true );
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
