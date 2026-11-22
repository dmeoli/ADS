/*--------------------------------------------------------------------------*/
/*------------------------- File PrefixFilter.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PrefixFilter [see PrefixFilter.h], with their
 * contracts.
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

/*--------------------------------------------------------------------------*/

template< class T , class H >
std::size_t PrefixFilter< T , H >::binsFor( std::size_t n )
{
 return( std::max< std::size_t >( 1 , ( 4 * n + 94 ) / 95 ) );  // 23.75
 }

template< class T , class H >
double PrefixFilter< T , H >::spareMean( std::size_t n , std::size_t m )
{
 // n - m E[ min( B , k ) ], the binomial probabilities of B by their
 // recurrence from Pr[ B = 0 ] = ( 1 - 1 / m )^n
 if( m == 1 )
  return( double( n > kK ? n - kK : 0 ) );
 const double dn = double( n ) , dm = double( m );
 double p = std::exp( dn * std::log1p( -1 / dm ) );
 double low = 0 , mean = 0;  // Pr[ B < k ], E[ B ; B < k ]
 for( unsigned j = 0 ; j < kK && double( j ) <= dn ; ++j ) {
  low += p;
  mean += double( j ) * p;
  p *= ( dn - double( j ) ) / ( double( j + 1 ) * ( dm - 1 ) );
  }
 return( std::max( 0.0 , dn - dm * ( mean + kK * ( 1 - low ) ) ) );
 }

template< class T , class H >
PrefixFilter< T , H >::PrefixFilter( std::size_t n , H h )
    : f_B( binsFor( n ) ) , f_cap( n ) ,
      f_spare( std::size_t( std::ceil( 1.1 * spareMean( n ,
							binsFor( n ) ) ) ) ,
	       kSpareBits ) ,
      f_h( h )
{
 ADS_PRE( n <= std::size_t( 0xFFFFFFFFU ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
std::uint64_t PrefixFilter< T , H >::header( const Bin & B )
{
 if constexpr( std::endian::native == std::endian::little ) {
  std::uint64_t w;
  std::memcpy( &w , B.b , 8 );
  return( w & ( ( std::uint64_t( 1 ) << 56 ) - 1 ) );
  }
 else {
  std::uint64_t w = 0;
  for( unsigned i = 0 ; i < kBody ; ++i )
   w |= std::uint64_t( B.b[ i ] ) << ( 8 * i );
  return( w );
  }
 }

template< class T , class H >
void PrefixFilter< T , H >::setHeader( Bin & B , std::uint64_t w )
{
 if constexpr( std::endian::native == std::endian::little ) {
  std::uint64_t u;  // byte 7 kept, one store
  std::memcpy( &u , B.b , 8 );
  u = ( u & ~( ( std::uint64_t( 1 ) << 56 ) - 1 ) ) | w;
  std::memcpy( B.b , &u , 8 );
  }
 else
  for( unsigned i = 0 ; i < kBody ; ++i )
   B.b[ i ] = static_cast< unsigned char >( w >> ( 8 * i ) );
 }

template< class T , class H >
unsigned PrefixFilter< T , H >::select0( std::uint64_t w , unsigned j )
{
 return( unsigned( selectInWord( ~w , int( j ) ) ) );
 }

template< class T , class H >
void PrefixFilter< T , H >::run( std::uint64_t w , unsigned q , unsigned & s ,
				unsigned & e )
{
 // a zero put below the header, so that the zeros of ranks q and q + 1
 // are the places just before the run of q and at its end
 const std::uint64_t u = ( w & kHead ) << 1;
 const unsigned z = select0( u , q );  // the zero of rank q
 s = z - q;
 // the zero of rank q + 1, past the ones that follow that of rank q
 e = z + unsigned( std::countr_zero( ~u >> ( z + 1 ) ) ) - q;
 }

template< class T , class H >
std::uint32_t PrefixFilter< T , H >::equal( const Bin & B , unsigned r )
{
#ifdef ADS_WITH_HIGHWAY
 // the 32 bytes at once [see SimdInline.h]
 return( std::uint32_t(
   detail::simd::here::bytesEqual< 32 >( B.b ,
					 std::uint8_t( r ) ) >> kBody ) );
#else
 std::uint32_t v = 0;
 for( unsigned i = 0 ; i < kK ; ++i )
  v |= std::uint32_t( B.b[ kBody + i ] == r ) << i;
 return( v );
#endif
 }

template< class T , class H >
std::uint32_t PrefixFilter< T , H >::below( const Bin & B , unsigned r )
{
#ifdef ADS_WITH_HIGHWAY
 return( std::uint32_t(
   detail::simd::here::bytesBelow< 32 >( B.b ,
					 std::uint8_t( r ) ) >> kBody ) );
#else
 std::uint32_t v = 0;
 for( unsigned i = 0 ; i < kK ; ++i )
  v |= std::uint32_t( B.b[ kBody + i ] < r ) << i;
 return( v );
#endif
 }

template< class T , class H >
std::uint32_t PrefixFilter< T , H >::runMask( std::uint64_t w , unsigned q )
{
 unsigned s , e;
 run( w , q , s , e );
 return( ( std::uint32_t( 1 ) << e ) - ( std::uint32_t( 1 ) << s ) );
 }

template< class T , class H >
bool PrefixFilter< T , H >::find( const Bin & B , std::uint64_t w ,
				  unsigned q ,
				 unsigned r )
{
 const std::uint32_t v = equal( B , r );
 if( v == 0 )  // most of the misses
  return( false );
 if( std::has_single_bit( v ) ) {
  // the remainder i is in the run of q iff the header has its one, the
  // i-th, at the place i + q, i.e., with q zeros below it (Even, Even and
  // Morrison)
  const std::uint64_t h = w & kHead;
  const unsigned i = unsigned( std::countr_zero( v ) ) , p = i + q;
  return( ( ( h >> p ) & 1 ) != 0 &&
	   unsigned(
	     std::popcount( h & ( ( std::uint64_t( 1 ) << p ) - 1 ) ) ) == i );
  }
 return( ( v & runMask( w , q ) ) != 0 );
 }

template< class T , class H >
std::uint32_t PrefixFilter< T , H >::maximum( const Bin & B , std::uint64_t w )
{
 const unsigned last = unsigned( std::bit_width( w & kHead ) ) - kK;
 return( ( std::uint32_t( last ) << 8 ) | B.b[ kBody + kK - 1 ] );
 }

template< class T , class H >
void PrefixFilter< T , H >::put( Bin & B , std::uint64_t w , unsigned q ,
				unsigned r , unsigned c )
{
 unsigned s , e;
 run( w , q , s , e );
 const unsigned i =
   s +
   unsigned( std::popcount(
     below( B , r ) & ( ( std::uint32_t( 1 ) << e ) -
		       ( std::uint32_t( 1 ) << s ) ) ) );  // r in its order
 const unsigned p = e + q;  // a one at the end of the run
 const std::uint64_t lo = ( std::uint64_t( 1 ) << p ) - 1;
 const std::uint64_t h =
   ( w & ~kHead ) | ( w & lo ) | ( ( w & kHead & ~lo ) << 1 ) | ( lo + 1 );
 if constexpr( std::endian::native == std::endian::little ) {
  // the 32 bytes as 4 words, the bytes from kBody + i moved up by one at
  // once, r written at kBody + i and the header in the low 7 bytes
  std::uint64_t W[ 4 ];
  std::memcpy( W , B.b , 32 );
  const unsigned at = kBody + i;
  auto bytesBelow = []( unsigned t ) {  // the low t bytes, t <= 8
   return( t == 0 ? std::uint64_t( 0 )
		   : ~std::uint64_t( 0 ) >> ( 64 - 8 * t ) );
   };
  std::uint64_t carry = 0;
  for( unsigned k = 0 ; k < 4 ; ++k ) {
   const unsigned b0 = 8 * k;
   const std::uint64_t keep =
     bytesBelow( std::min( 8U , at > b0 ? at - b0 : 0U ) );
   const std::uint64_t upto =
     bytesBelow( std::min( 8U , at + 1 > b0 ? at + 1 - b0 : 0U ) );
   const std::uint64_t moved = ( W[ k ] << 8 ) | carry;
   carry = W[ k ] >> 56;
   W[ k ] = ( W[ k ] & keep ) | ( moved & ~upto ) |
	    ( 0x0101010101010101ULL * r & upto & ~keep );
   }
  W[ 0 ] = ( W[ 0 ] & ~( ( std::uint64_t( 1 ) << 56 ) - 1 ) ) | h;
  std::memcpy( B.b , W , 32 );
  }
 else {
  std::memmove( B.b + kBody + i + 1 , B.b + kBody + i , c - i );
  B.b[ kBody + i ] = static_cast< unsigned char >( r );
  setHeader( B , h );
  }
 ADS_INVARIANT( unsigned( std::popcount( header( B ) & kHead ) ) == c + 1 );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
std::uint64_t PrefixFilter< T , H >::hash( const T & x ) const
{
 return( detail::filterHash( f_h , x ) );  // [see BlockedBloomFilter.h]
 }

template< class T , class H >
std::size_t PrefixFilter< T , H >::bin( std::uint64_t h ) const
{
 return( detail::highRange( h , f_B.size() ) );
 }

template< class T , class H >
std::uint32_t PrefixFilter< T , H >::mini( std::uint64_t h )
{
 return( std::uint32_t( ( ( h & 0xFFFFFFFFU ) * kS ) >> 32 ) );
 }

template< class T , class H >
std::uint64_t PrefixFilter< T , H >::spareHash( std::size_t i ,
					       std::uint32_t x )
{
 return( hashWord( std::uint64_t( i ) * kS + x , 0x2545F4914F6CDD1DULL ) );
 }

template< class T , class H >
bool PrefixFilter< T , H >::insertHash( std::uint64_t h )
{
 if( f_n == f_cap )  // full
  return( false );
 ++f_n;
 const std::size_t i = bin( h );
 const std::uint32_t x = mini( h );
 Bin & B = f_B[ i ];
 std::uint64_t w = header( B );
 auto c = unsigned( std::popcount( w & kHead ) );
 if( c == kK ) [[unlikely]] {
  const std::uint32_t y = maximum( B , w );
  ++f_spareN;
  if( x >= y ) {  // x past the prefix
   setHeader( B , ( w & ~kTop ) | ( std::uint64_t( y >> 8 ) << 50 ) );
   f_spare.insertHash( spareHash( i , x ) );
   return( true );
   }
  // y evicted: the last one of the header, the last byte of the body;
  // the quotient of the new maximum written after the put
  w &= ( std::uint64_t( 1 ) << ( std::bit_width( w & kHead ) - 1 ) ) - 1;
  f_spare.insertHash( spareHash( i , y ) );
  put( B , w , x >> 8 , x & 255 , kK - 1 );
  w = header( B );
  setHeader( B ,
	     ( w & ~kTop ) | ( std::uint64_t( maximum( B ,
						       w ) >> 8 ) << 50 ) );
  return( true );
  }
 put( B , w , x >> 8 , x & 255 , c );
 return( true );
 }

template< class T , class H >
ADS_FILTER_INLINE bool
PrefixFilter< T , H >::maybeHash( std::uint64_t h ) const
{
 const std::size_t i = bin( h );
 const std::uint32_t x = mini( h );
 const Bin & B = f_B[ i ];
 const std::uint64_t w = header( B );
 if( find( B , w , x >> 8 , x & 255 ) )
  return( true );
 // not in the bin: in the spare if the bin overflowed and x is above its
 // maximum, from the 6 top bits of the header and the last byte (above
 // every x if it did not)
 const std::uint32_t lim =
   std::uint32_t( ( w >> 42 ) & 0x3F00 ) | B.b[ kBody + kK - 1 ];
 if( x > lim ) [[unlikely]]
  return( spare( i , x ) );
 return( false );
 }

template< class T , class H >
ADS_FILTER_NOINLINE bool PrefixFilter< T , H >::spare( std::size_t i ,
						      std::uint32_t x ) const
{
 return( f_spare.maybeHash( spareHash( i , x ) ) );
 }

template< class T , class H >
bool PrefixFilter< T , H >::insert( const T & x )
{
 const bool in = insertHash( hash( x ) );
 ADS_POST( ! in || maybe( x ) );
 return( in );
 }

template< class T , class H >
ADS_FILTER_INLINE bool PrefixFilter< T , H >::maybe( const T & x ) const
{
 return( maybeHash( hash( x ) ) );
 }

template< class T , class H >
void PrefixFilter< T , H >::clear( void )
{
 std::fill( f_B.begin() , f_B.end() , Bin{} );
 f_spare.clear();
 f_n = 0;
 f_spareN = 0;
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
std::size_t PrefixFilter< T , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class H >
bool PrefixFilter< T , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class H >
std::size_t PrefixFilter< T , H >::capacity( void ) const
{
 return( f_cap );
 }

template< class T , class H >
double PrefixFilter< T , H >::load( void ) const
{
 return( double( f_n ) / double( kK * f_B.size() ) );
 }

template< class T , class H >
std::size_t PrefixFilter< T , H >::bits( void ) const
{
 return( 8 * sizeof( Bin ) * f_B.size() + f_spare.bits() );
 }

template< class T , class H >
std::size_t PrefixFilter< T , H >::bins( void ) const
{
 return( f_B.size() );
 }

template< class T , class H >
std::size_t PrefixFilter< T , H >::spareSize( void ) const
{
 return( f_spareN );
 }

template< class T , class H >
double PrefixFilter< T , H >::expectedSpareSize( void ) const
{
 return( spareMean( f_cap , f_B.size() ) );
 }

template< class T , class H >
double PrefixFilter< T , H >::falsePositiveRate( void ) const
{
 // the sum over the loads j of a bin, Pr[ B = j ] by its recurrence
 const double es = f_spare.falsePositiveRate();
 auto term = [ & ]( double j ) {
  double t = -std::expm1( j * std::log1p( -1.0 / kS ) );
  if( j > kK )
   t += ( j + 1 - kK ) / ( j + 1 ) * es;
  return( t );
  };
 const double n = double( f_n ) , m = double( f_B.size() );
 if( f_B.size() == 1 )
  return( term( n ) );
 double p = std::exp( n * std::log1p( -1 / m ) ) , f = 0 , seen = 0;
 for( double j = 0 ; j <= n ; ++j ) {
  f += p * term( j );
  seen += p;
  if( seen > 1 - 1e-15 || ( j > n / m && p < 1e-18 ) )
   break;
  p *= ( n - j ) / ( ( j + 1 ) * ( m - 1 ) );
  }
 return( f );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
template< class A >
inline void PrefixFilter< T , H >::persist( A & a )
{
 a( f_B , f_cap , f_n , f_spareN , f_spare );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( f_B.size() == binsFor( f_cap ) , "the bins of a PrefixFilter" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PrefixFilter.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
