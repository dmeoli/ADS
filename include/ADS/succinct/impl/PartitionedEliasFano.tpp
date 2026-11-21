/*--------------------------------------------------------------------------*/
/*--------------------- File PartitionedEliasFano.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of PartitionedEliasFano.h, with their contracts.
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

inline PartitionedEliasFano::Code
PartitionedEliasFano::code( std::uint64_t m , std::uint64_t u )
{
 if( u == m )
  return( Code{ FULL , 0 , 0 } );
 const int ell = u > m ? std::bit_width( u / m ) - 1 : 0;
 const std::uint64_t ef =
   m * std::uint64_t( ell ) + m + ( ( u - 1 ) >> ell ) + 1;
 if( u <= ef )
  return( Code{ VECTOR , u , 0 } );
 return( Code{ ELIAS , ef , ell } );
 }

inline PartitionedEliasFano::PartitionedEliasFano(
  std::span< const std::uint64_t > x )
    : PartitionedEliasFano( x , x.empty() ? 0 : x.back() + 1 )
{ }

inline PartitionedEliasFano::PartitionedEliasFano(
  std::span< const std::uint64_t > x , std::uint64_t u )
    : f_n( x.size() ) , f_u( u )
{
 ADS_PRE( x.empty() || ( x.back() < u ) );
 ADS_AUDIT( std::adjacent_find( x.begin() , x.end() ,
				[]( std::uint64_t a , std::uint64_t b ) {
				 return( a >= b );
				 } ) == x.end() );
 if( f_n == 0 )
  return;
 f_last = x.back();
 const std::size_t nb = ( f_n + kB - 1 ) / kB , ns = ( nb + kS - 1 ) / kS;
 ADS_PRE( ns <= std::size_t( UINT32_MAX ) );
 // the places of the codes, then the codes, with 128 0s after them for
 // the selects that read 4 words from any place in a code (the words of
 // a BitVector go a word past its last bit)
 std::vector< std::uint64_t > at( nb + 1 , 0 );
 auto block = [ & ]( std::size_t j , std::uint64_t & b , std::uint64_t & m ,
		     std::uint64_t & uj ) {
  b = j == 0 ? 0 : x[ j * kB - 1 ] + 1;
  m = std::min( kB , f_n - j * kB );
  uj = x[ j * kB + m - 1 ] - b + 1;
  };
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  at[ j + 1 ] = at[ j ] + code( m , uj ).bits;
  }
 f_codes = BitVector( std::size_t( at[ nb ] ) + 128 );
 std::vector< std::uint64_t > sample( nb , 0 );
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  const Code c = code( m , uj );
  const std::size_t p = std::size_t( at[ j ] );
  for( std::size_t k = 0 ; k < m ; ++k ) {
   const std::uint64_t v = x[ j * kB + k ] - b;
   if( c.kind == VECTOR )
    f_codes.set( p + std::size_t( v ) );
   else if( c.kind == ELIAS ) {
    if( c.ell > 0 )
     f_codes.put( p + k * std::size_t( c.ell ) , v & detail::lowMask( c.ell ) ,
		  c.ell );
    f_codes.set( p + std::size_t( m ) * std::size_t( c.ell ) +
		 std::size_t( v >> c.ell ) + k );
    }
   if( ( k == kSample ) && ( c.kind != FULL ) )
    sample[ j ] = c.kind == VECTOR ? v : ( v >> c.ell ) + k;
   }
  }
 // the superblocks, and the widths of the fields of the records: b and
 // the place relative to the superblock
 f_sup.assign( 2 * ns , 0 );
 std::uint64_t mb = 0 , mo = 0 , ms = 0;
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  if( j % kS == 0 ) {
   f_sup[ 2 * ( j / kS ) ] = b;
   f_sup[ 2 * ( j / kS ) + 1 ] = at[ j ];
   }
  mb = std::max( mb , b - f_sup[ 2 * ( j / kS ) ] );
  mo = std::max( mo , at[ j ] - f_sup[ 2 * ( j / kS ) + 1 ] );
  ms = std::max( ms , sample[ j ] );
  }
 f_wb = std::max( 1 , int( std::bit_width( mb ) ) );
 f_wo = std::max( 1 , int( std::bit_width( mo ) ) );
 f_ws = std::max( 1 , int( std::bit_width( ms ) ) );
 f_s = f_wb + f_wo + f_ws + 8 <= 64 ? 1 : 2;
 f_sh = f_s == 1 ? f_wb : 0;
 f_mb = detail::lowMask( f_wb );
 f_mo = detail::lowMask( f_wo );
 f_ms = detail::lowMask( f_ws );
 ADS_INVARIANT( f_wo + f_ws + 8 <= 64 );
 f_rec.assign( nb * f_s , 0 );
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  const Code c = code( m , uj );
  f_rec[ j * f_s ] = b - f_sup[ 2 * ( j / kS ) ];
  f_rec[ j * f_s + f_s - 1 ] |=
    ( ( at[ j ] - f_sup[ 2 * ( j / kS ) + 1 ] ) | ( sample[ j ] << f_wo ) |
      ( ( ( std::uint64_t( c.ell ) << 2 ) | std::uint64_t( c.kind ) )
	<< ( f_wo + f_ws ) ) )
    << f_sh;
  }
 // the buckets: H[ t ] the last superblock whose b is below t 2^s, with
 // a bucket per superblock or so
 while( ( f_shift < 63 ) && ( ( f_last >> f_shift ) >= ns ) )
  ++f_shift;
 f_bkt.assign( std::size_t( f_last >> f_shift ) + 2 , 0 );
 for( std::size_t t = 0 , q = 0 ; t < f_bkt.size() ; ++t ) {
  while( ( q + 1 < ns ) && ( ( f_sup[ 2 * ( q + 1 ) ] >> f_shift ) < t ) )
   ++q;
  f_bkt[ t ] = std::uint32_t( q );
  }
 ADS_AUDIT( std::equal( x.begin() , x.end() , begin() ) );
 }

inline PartitionedEliasFano::Record
PartitionedEliasFano::record( std::size_t j ) const
{
 const std::uint64_t * r = f_rec.data() + j * f_s;
 const std::uint64_t * c = f_sup.data() + 2 * ( j / kS );
 const std::uint64_t hi = r[ f_s - 1 ] >> f_sh;
 return( Record{ c[ 0 ] + ( r[ 0 ] & f_mb ) ,
		  std::size_t( c[ 1 ] + ( hi & f_mo ) ) ,
		  std::size_t( ( hi >> f_wo ) & f_ms ) ,
		  unsigned( ( hi >> ( f_wo + f_ws ) ) & 0xFF ) } );
 }

template< bool One >
inline std::size_t PartitionedEliasFano::select( std::size_t p ,
						 std::size_t k ) const
{
 // the 4 words from the one of p, the bits before p cleared, and the
 // word of the k-th bit chosen by comparisons of the prefix sums, with
 // no branch; a scan of the words after them if the bit is not there
 const Word * W = f_codes.words().data() + p / 64;
 const Word f = One ? 0 : ~Word( 0 );
 std::array< Word , 4 > w;
 for( int i = 0 ; i < 4 ; ++i )
  w[ i ] = W[ i ] ^ f;
 w[ 0 ] &= ~Word( 0 ) << ( p % 64 );
 std::array< std::size_t , 4 > s;
 s[ 0 ] = 0;
 for( int i = 1 ; i < 4 ; ++i )
  s[ i ] = s[ i - 1 ] + std::size_t( std::popcount( w[ i - 1 ] ) );
 const std::size_t q = p - p % 64;
 if( const std::size_t t = s[ 3 ] + std::size_t( std::popcount( w[ 3 ] ) );
     k >= t ) [[unlikely]] {
  k -= t;
  for( std::size_t i = 4 ; ; ++i ) {
   const Word v = W[ i ] ^ f;
   const std::size_t c = std::size_t( std::popcount( v ) );
   if( k < c )
    return( q + 64 * i + std::size_t( selectInWord( v , int( k ) ) ) );
   k -= c;
   }
  }
 const std::size_t i = std::size_t( k >= s[ 1 ] ) +
		       std::size_t( k >= s[ 2 ] ) + std::size_t( k >= s[ 3 ] );
 return( q + 64 * i +
	  std::size_t( selectInWord( w[ i ] , int( k - s[ i ] ) ) ) );
 }

inline std::size_t PartitionedEliasFano::ones( std::size_t p ,
					       std::size_t q ) const
{
 if( p == q )
  return( 0 );
 const Word * W = f_codes.words().data();
 const std::size_t a = p / 64 , b = ( q - 1 ) / 64;
 const Word first = ~Word( 0 ) << ( p % 64 ) ,
	    lastm = ~Word( 0 ) >> ( 63 - ( q - 1 ) % 64 );
 if( a == b )
  return( std::size_t( std::popcount( W[ a ] & first & lastm ) ) );
 std::size_t c = std::size_t( std::popcount( W[ a ] & first ) ) +
		 std::size_t( std::popcount( W[ b ] & lastm ) );
 for( std::size_t i = a + 1 ; i < b ; ++i )
  c += std::size_t( std::popcount( W[ i ] ) );
 return( c );
 }

inline std::uint64_t PartitionedEliasFano::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 const std::size_t j = i / kB , k = i % kB;
 const std::uint64_t m = std::min( kB , f_n - j * kB );
 const Record r = record( j );
 if( ( r.form & 3 ) == FULL )
  return( r.b + k );
 // a vector is the high part of a code with no low bits whose k-th 1 is
 // not shifted by k, so the two kinds share the select
 const int ell = int( r.form >> 2 );
 const std::size_t e = ( r.form & 3 ) == ELIAS ? 1 : 0;
 const std::size_t hp = r.p + std::size_t( m ) * std::size_t( ell );
 const std::size_t h = k / kSample;  // 0 or 1, from the 1 sampled if 1
 const std::size_t at =
   select< true >( hp + ( r.sample & ( 0 - h ) ) , k - kSample * h );
 const std::uint64_t low = f_codes.get( r.p + k * std::size_t( ell ) , ell );
 return( r.b + ( ( std::uint64_t( at - hp - e * k ) << ell ) | low ) );
 }

inline std::size_t PartitionedEliasFano::nextGEQ( std::uint64_t x ) const
{
 if( ( f_n == 0 ) || ( x > f_last ) )
  return( f_n );
 // the last superblock q with b <= x, in the range of the bucket of x,
 // then the last block j of q with b_j <= x by a count with no branch:
 // b_{j+1} > x, so the last item of j is >= x
 const std::size_t t = std::size_t( x >> f_shift );
 std::size_t q = f_bkt[ t ] , len = f_bkt[ t + 1 ] - q + 1;
 while( len > 1 ) {
  const std::size_t half = len / 2;
  q = f_sup[ 2 * ( q + half ) ] <= x ? q + half : q;
  len -= half;
  }
 const std::uint64_t xr = x - f_sup[ 2 * q ];
 std::size_t j = q * kS;
 const std::size_t nj = std::min( kS , ( f_n - 1 ) / kB + 1 - j );
 std::size_t c = 0;
 if( ( f_s == 1 ) && ( nj == kS ) ) [[likely]] {
  // a whole superblock of records of one word: a loop of fixed length
  const std::uint64_t * R = f_rec.data() + j;
  for( std::size_t d = 1 ; d < kS ; ++d )
   c += std::size_t( ( R[ d ] & f_mb ) <= xr );
  }
 else
  for( std::size_t d = 1 ; d < nj ; ++d )
   c += std::size_t( relBase( j + d ) <= xr );
 j += c;
 const std::uint64_t m = std::min( kB , f_n - j * kB );
 const Record r = record( j );
 const std::uint64_t v = x - r.b;
 const bool late = ( m > kSample ) && ( r.form & 3 ) != FULL;
 std::size_t k;
 if( ( r.form & 3 ) == FULL )
  k = std::size_t( v );
 else if( ( r.form & 3 ) == VECTOR )
  k = late && ( v >= r.sample )
	? kSample + ones( r.p + r.sample , r.p + std::size_t( v ) )
	: ones( r.p , r.p + std::size_t( v ) );
 else {
  // the items of the bucket of v share their high bits: their low bits
  // decide, and the first item of a later bucket is above v; the 0 of
  // rank hv - 1 from the 1 sampled if the 0s before that are fewer
  const int ell = int( r.form >> 2 );
  const std::size_t hp = r.p + std::size_t( m ) * std::size_t( ell );
  const std::uint64_t hv = v >> ell , lv = v & detail::lowMask( ell );
  std::size_t pos = hp;
  if( hv > 0 ) {
   const std::size_t z = late ? r.sample - kSample : SIZE_MAX;
   const std::size_t g = std::size_t( hv - 1 ) >= z ? 1 : 0;
   pos = select< false >( hp + ( r.sample & ( 0 - g ) ) ,
			  std::size_t( hv - 1 ) - ( z & ( 0 - g ) ) ) +
	 1;
   }
  k = pos - hp - std::size_t( hv );
  for( ; ( k < m ) && f_codes[ pos ] ; ++k , ++pos )
   if( f_codes.get( r.p + k * std::size_t( ell ) , ell ) >= lv )
    break;
  }
 const std::size_t res = j * kB + k;
 ADS_POST( ( res == f_n ) || ( access( res ) >= x ) );
 ADS_POST( ( res == 0 ) || ( access( res - 1 ) < x ) );
 return( res );
 }

inline bool PartitionedEliasFano::contains( std::uint64_t x ) const
{
 const std::size_t p = nextGEQ( x );
 return( ( p < f_n ) && ( access( p ) == x ) );
 }

inline std::array< std::size_t , 3 > PartitionedEliasFano::kinds( void ) const
{
 std::array< std::size_t , 3 > c{};
 const std::size_t nb = ( f_n + kB - 1 ) / kB;
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  const std::uint64_t m = std::min( kB , f_n - j * kB );
  const Record r = record( j );
  const std::uint64_t l = j + 1 < nb ? record( j + 1 ).b - 1 : f_last;
  ADS_INVARIANT( ( r.form & 3 ) == unsigned( code( m , l - r.b + 1 ).kind ) );
  ++c[ r.form & 3 ];
  }
 return( c );
 }

inline std::size_t PartitionedEliasFano::sizeInBits( void ) const
{
 return( f_codes.size() + 64 * ( f_rec.size() + f_sup.size() ) +
	  32 * f_bkt.size() );
 }

/*--------------------------------------------------------------------------*/

template< class A >
inline void PartitionedEliasFano::persist( A & a )
{
 a( f_n , f_u , f_last , f_codes , f_rec , f_sup , f_bkt , f_shift , f_s ,
    f_wb , f_wo ,
    f_ws , f_sh );
 const std::size_t nb = ( f_n + kB - 1 ) / kB , ns = ( nb + kS - 1 ) / kS;
 if constexpr( A::loading ) {
  f_mb = detail::lowMask( f_wb );
  f_mo = detail::lowMask( f_wo );
  f_ms = detail::lowMask( f_ws );
  }
 a.require( ( f_wb >= 1 ) && ( f_wb <= 64 ) && ( f_wo >= 1 ) &&
	      ( f_wo <= 64 ) && ( f_ws >= 1 ) && ( f_ws <= 16 ) &&
	      ( f_wo + f_ws + 8 <= 64 ) &&
	      ( f_s == ( f_wb + f_wo + f_ws + 8 <= 64 ? 1u : 2u ) ) &&
	      ( f_sh == ( f_s == 1 ? f_wb : 0 ) ) &&
	      ( f_rec.size() == nb * f_s ) && ( f_sup.size() == 2 * ns ) &&
	      ( ( f_n == 0 ) ||
		( ( f_last < f_u ) && ( f_shift >= 0 ) && ( f_shift < 64 ) &&
		  ( f_codes.size() >= 128 ) &&
		  ( f_bkt.size() == std::size_t( f_last >> f_shift ) + 2 ) &&
		  ( f_bkt.back() < ns ) ) ) ,
	    "the records of a PartitionedEliasFano" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File PartitionedEliasFano.tpp -------------------*/
/*--------------------------------------------------------------------------*/
