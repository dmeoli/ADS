/*--------------------------------------------------------------------------*/
/*-------------------- File CountingQuotientFilter.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of QuotientTable and CountingQuotientFilter [see
 * CountingQuotientFilter.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS QuotientTable ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class S >
QuotientTable< S >::QuotientTable( int l ) : f_l( l )
{
 ADS_PRE( ( l >= 6 ) && ( l < 40 ) );
 const std::size_t m = std::size_t( 1 ) << l;
 const auto spare = std::size_t(
   std::ceil( 10 * std::sqrt( double( m ) ) / 64 ) );  // 10 sqrt( m ) slots
 f_nb = m / 64 + spare;
 f_T.assign( f_nb * kBlock , std::uint8_t( 0 ) );
 }

/*--------------------------------------------------------------------------*/

template< class S >
void QuotientTable< S >::setEnd( std::size_t i , bool v )
{
 const std::uint64_t w = end( i / 64 ) , b = std::uint64_t( 1 ) << ( i % 64 );
 setWord( i / 64 , 1 , v ? ( w | b ) : ( w & ~b ) );
 }

template< class S >
void QuotientTable< S >::setOcc( std::size_t q , bool v )
{
 const std::uint64_t w = occ( q / 64 ) , b = std::uint64_t( 1 ) << ( q % 64 );
 setWord( q / 64 , 0 , v ? ( w | b ) : ( w & ~b ) );
 }

template< class S >
std::size_t QuotientTable< S >::offset( std::size_t b ) const
{
 if( block( b )[ 0 ] < kSat )
  return( block( b )[ 0 ] );
 // saturated: from the last block before it with its offset, forward, the
 // offset of each block the end of the run of the last slot before it
 std::size_t a = b;
 while( block( a )[ 0 ] == kSat )
  --a;
 std::size_t o = block( a )[ 0 ];
 for( std::size_t c = a + 1 ; c <= b ; ++c )
  o = runEnd( 64 * c - 1 , o ) - ( 64 * c - 1 );
 return( o );
 }

template< class S >
std::size_t QuotientTable< S >::runEnd( std::size_t q , std::size_t ob ) const
{
 const std::size_t b = q / 64 , o = q % 64;
 const std::size_t j = std::size_t( std::popcount( occ( b ) & low( o + 1 ) ) );
 if( j == 0 )
  return( ob <= o ? q : 64 * b + ob - 1 );
 // the end bit of rank j - 1 from the slot 64 b + ob on
 std::size_t c = b + ob / 64 , k = j - 1;
 std::uint64_t w = end( c ) & ~low( ob % 64 );
 int s = select( w , k );
 while( s == 64 ) {
  k -= std::size_t( std::popcount( w ) );
  w = end( ++c );
  s = select( w , k );
  }
 const std::size_t e = 64 * c + std::size_t( s );
 return( e < q ? q : e );
 }

template< class S >
std::size_t QuotientTable< S >::busy( std::size_t i ) const
{
 const std::size_t b = i / 64 , o = i % 64 , ob = block( b )[ 0 ];
 const std::size_t r = std::size_t( std::popcount( occ( b ) & low( o + 1 ) ) );
 if( ob <= o )  // the runs begun up to i less those ended before it
  return( r - std::size_t( std::popcount( ( end( b ) & low( o ) ) >> ob ) ) );
 return( ob - o + r );
 }

template< class S >
std::size_t QuotientTable< S >::firstFree( std::size_t i ) const
{
 while( i < slots() ) {
  const std::size_t t = busy( i );
  if( t == 0 )
   return( i );
  i += t;
  }
 return( slots() );
 }

template< class S >
void QuotientTable< S >::open( std::size_t q , std::size_t p )
{
 const std::size_t E = firstFree( p );
 constexpr std::size_t s = sizeof( S );
 for( std::size_t b = E / 64 ; ; --b ) {  // [ p , E ) to [ p + 1 , E ]
  const std::size_t base = 64 * b;
  const std::size_t t0 = std::max( p + 1 , base ) ,
		    t1 = std::min( E , base + 63 );
  if( t0 <= t1 ) {
   std::uint8_t * B = block( b ) + 17;
   if( t0 > base )
    std::memmove( B + ( t0 - base ) * s , B + ( t0 - 1 - base ) * s ,
		  ( t1 - t0 + 1 ) * s );
   else {  // the first slot from the last of the block before
    std::memmove( B + s , B , ( t1 - base ) * s );
    std::memcpy( B , slotAt( base - 1 ) , s );
    }
   const std::uint64_t M = low( t1 - base + 1 ) & ~low( t0 - base );
   const std::uint64_t w = end( b );
   std::uint64_t u = ( w << 1 ) & M;
   if( t0 == base )
    u |= end( b - 1 ) >> 63;
   setWord( b , 1 , ( w & ~M ) | u );
   }
  if( b == p / 64 )
   break;
  }
 setEnd( p , false );
 for( std::size_t b = q / 64 + 1 ; b <= E / 64 ; ++b )
  if( block( b )[ 0 ] < kSat )
   ++block( b )[ 0 ];
 ++f_used;
 }

template< class S >
void QuotientTable< S >::close( std::size_t q , std::size_t s , std::size_t p ,
				std::size_t d )
{
 const std::size_t e = runEnd( q );
 const bool whole = ( p == s ) && ( p + d == e + 1 );
 if( ( p + d == e + 1 ) && ( p > s ) )  // the slot before ends the run
  setEnd( p - 1 , true );
 // the runs after them moved back, each no further than its quotient
 std::size_t bq = q , cur = p , dist = d;
 while( dist > 0 ) {
  if( isEnd( cur + dist - 1 ) )
   do
    ++bq;
   while( ( bq < cur + dist ) && ! occupied( bq ) );
  if( bq <= cur ) {
   set( cur , get( cur + dist ) );
   setEnd( cur , isEnd( cur + dist ) );
   ++cur;
   }
  else if( bq <= cur + dist ) {
   for( std::size_t i = cur ; i < cur + dist ; ++i ) {
    set( i , S( 0 ) );
    setEnd( i , false );
    }
   dist = cur + dist - bq;
   cur = bq;
   }
  else
   dist = 0;
  }
 if( whole )
  setOcc( q , false );
 f_used -= d;
 // the offsets after the block of q, until one does not change
 for( std::size_t b = q / 64 ; b + 1 < f_nb ; ++b ) {
  const std::size_t last = 64 * b + 63 , o = runEnd( last ) - last;
  const unsigned v = o < kSat ? unsigned( o ) : kSat ,
		 old = block( b + 1 )[ 0 ];
  block( b + 1 )[ 0 ] = std::uint8_t( v );
  if( ( old == v ) && ( v < kSat ) )
   break;
  }
 }

template< class S >
bool QuotientTable< S >::replace( std::size_t q , std::size_t p ,
				  std::size_t o ,
				  const S * v , std::size_t k )
{
 ADS_PRE( q < quotients() );
 if( k > o ) {
  const std::size_t d = k - o;
  if( f_used + d > quotients() - ( quotients() + 19 ) / 20 )  // 0.95 m
   return( false );
  std::size_t x = p + o;  // the d-th free slot from p + o in the table
  for( std::size_t i = 0 ; i < d ; ++i ) {
   x = firstFree( x );
   if( x >= slots() )
    return( false );
   ++x;
   }
  if( ! occupied( q ) ) {  // a run made at p, then grown at its end
   open( q , p );
   setOcc( q , true );
   setEnd( p , true );
   for( std::size_t i = 1 ; i < d ; ++i ) {
    open( q , p + i );
    setEnd( p + i - 1 , false );
    setEnd( p + i , true );
    }
   }
  else {
   const bool atEnd = ( p + o == runEnd( q ) + 1 );
   for( std::size_t i = 0 ; i < d ; ++i ) {
    open( q , p + o + i );
    if( atEnd ) {
     setEnd( p + o + i - 1 , false );
     setEnd( p + o + i , true );
     }
    }
   }
  for( std::size_t i = 0 ; i < k ; ++i )
   set( p + i , v[ i ] );
  return( true );
  }
 for( std::size_t i = 0 ; i < k ; ++i )
  set( p + i , v[ i ] );
 if( k < o )
  close( q , runStart( q ) , p + k , o - k );
 return( true );
 }

template< class S >
template< class Fn >
void QuotientTable< S >::forEachRun( Fn && fn ) const
{
 std::size_t pos = 0;
 for( std::size_t b = 0 ; b < quotients() / 64 ; ++b )
  for( std::uint64_t w = occ( b ) ; w != 0 ; w &= w - 1 ) {
   const std::size_t q = 64 * b + std::size_t( std::countr_zero( w ) ) ,
		     s = std::max( q , pos );
   std::size_t c = s / 64;
   std::uint64_t u = end( c ) & ~low( s % 64 );
   while( u == 0 )
    u = end( ++c );
   const std::size_t e = 64 * c + std::size_t( std::countr_zero( u ) );
   fn( q , s , e );
   pos = e + 1;
   }
 }

template< class S >
void QuotientTable< S >::clear( void )
{
 std::fill( f_T.begin() , f_T.end() , std::uint8_t( 0 ) );
 f_used = 0;
 }

template< class S >
bool QuotientTable< S >::consistent( void ) const
{
 std::size_t used = 0 , runs = 0 , ends = 0 , last = 0;
 bool ok = true , any = false;
 std::vector< std::size_t > E;  // the end of the runs, by quotient
 forEachRun( [ & ]( std::size_t q , std::size_t s , std::size_t e ) {
  ok &= ( s >= q ) && ( ! any || ( s > last ) );
  used += e - s + 1;
  ++runs;
  any = true;
  last = e;
  E.push_back( q );
  E.push_back( e );
  } );
 for( std::size_t b = 0 ; b < f_nb ; ++b )
  ends += std::size_t( std::popcount( end( b ) ) );
 ok &= ( used == f_used ) && ( runs == ends );
 // the offset of each block from the runs of the quotients before it
 std::size_t r = 0 , reach = 0;  // the end of the runs before the block
 for( std::size_t b = 1 ; b < f_nb ; ++b ) {
  while( ( r < E.size() ) && ( E[ r ] < 64 * b ) ) {
   reach = E[ r + 1 ] + 1;
   r += 2;
   }
  const std::size_t o = reach > 64 * b ? reach - 64 * b : 0;
  ok &= ( block( b )[ 0 ] == std::min< std::size_t >( o , kSat ) );
  }
 return( ok );
 }

template< class S >
template< class A >
inline void QuotientTable< S >::persist( A & a )
{
 a( f_T , f_nb , f_used , f_l );
 a.require( ( f_l >= 6 ) && ( f_l < 40 ) &&
	      ( f_nb >= ( std::size_t( 1 ) << f_l ) / 64 ) &&
	      ( f_T.size() == f_nb * kBlock ) ,
	    "the blocks of a QuotientTable" );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------- CLASS CountingQuotientFilter ----------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
CountingQuotientFilter< T , F , H >::CountingQuotientFilter( std::size_t n ,
							     H h )
    : f_h( h )
{
 const std::size_t m = std::max< std::size_t >(
   64 , std::bit_ceil( std::size_t( std::ceil( double( n ) / 0.95 ) ) ) );
 ADS_PRE( m < ( std::size_t( 1 ) << 40 ) );
 f_Q = detail::QuotientTable< F >( std::countr_zero( m ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
std::uint64_t CountingQuotientFilter< T , F , H >::hash( const T & x ) const
{
 if constexpr( detail::kAvalanching64< H , T > )
  return( detail::hash64( f_h , x ) );
 else
  return( hashWord( detail::hash64( f_h , x ) ) );
 }

template< class T , class F , class H >
std::uint64_t
CountingQuotientFilter< T , F , H >::fingerprint( std::uint64_t h ) const
{
 return( h >> ( 64 - fingerprintBits() ) );
 }

template< class T , class F , class H >
std::size_t CountingQuotientFilter< T , F , H >::code( std::uint64_t x ,
						     std::uint64_t c ,
						     Code & C ) const
{
 std::size_t k = 0;
 if( c == 0 )
  return( 0 );
 C[ k++ ] = F( x );
 if( c == 1 )
  return( k );
 if( c == 2 ) {
  C[ k++ ] = F( x );
  return( k );
  }
 if( c == 3 ) {  // x 0 x, and 0 0 0
  C[ k++ ] = F( 0 );
  C[ k++ ] = F( x );
  return( k );
  }
 // the digits of c - 3 (of c - 4 for x = 0) in base 2^r - 2 (2^r - 1),
 // none 0 and none x, from the least significant
 const std::uint64_t base = ( std::uint64_t( 1 ) << f_r ) - ( x ? 2 : 1 );
 std::uint64_t v = x ? c - 3 : c - 4 , d = 0;
 std::array< F , 66 > D;
 std::size_t nd = 0;
 do {
  d = v % base + 1;
  if( x && ( d >= x ) )
   ++d;
  D[ nd++ ] = F( d );
  v /= base;
  } while( v );
 if( x && ( d >= x ) )  // a first digit not below x: a 0 before it
  C[ k++ ] = F( 0 );
 while( nd > 0 )
  C[ k++ ] = D[ --nd ];
 if( x == 0 )
  C[ k++ ] = F( 0 );
 C[ k++ ] = F( x );
 return( k );
 }

template< class T , class F , class H >
void CountingQuotientFilter< T , F , H >::decode( std::size_t i ,
						std::uint64_t & x ,
						std::uint64_t & c ,
						std::size_t & j ) const
{
 x = f_Q.get( i );
 if( f_Q.isEnd( i ) ) {
  c = 1;
  j = i;
  return;
  }
 std::uint64_t d = f_Q.get( i + 1 );
 if( f_Q.isEnd( i + 1 ) || ( x && ( d >= x ) ) ) {  // x, or x x
  c = d == x ? 2 : 1;
  j = i + ( d == x );
  return;
  }
 if( x && ( d == 0 ) && ( f_Q.get( i + 2 ) == x ) ) {  // x 0 x
  c = 3;
  j = i + 2;
  return;
  }
 if( ( x == 0 ) && ( d == 0 ) ) {  // 0 0 0, or 0 0 before another
  c = f_Q.get( i + 2 ) == 0 ? 3 : 2;
  j = i + c - 1;
  return;
  }
 const std::uint64_t base = ( std::uint64_t( 1 ) << f_r ) - ( x ? 2 : 1 );
 std::uint64_t v = 0;
 std::size_t k = i + 1;
 while( ( d != x ) && ! f_Q.isEnd( k ) ) {
  if( d > x )
   --d;
  if( d && x )
   --d;
  v = v * base + d;
  d = f_Q.get( ++k );
  }
 if( x ) {
  c = v + 3;
  j = k;
  return;
  }
 if( f_Q.isEnd( k ) || ( f_Q.get( k + 1 ) != 0 ) ) {  // a 0 alone
  c = 1;
  j = i;
  return;
  }
 c = v + 4;
 j = k + 1;
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::find( std::uint64_t y ,
						std::size_t & i ,
					      std::size_t & j ,
					      std::uint64_t & c ) const
{
 const std::size_t q = std::size_t( y >> f_r );
 const std::uint64_t x = y & ( ( std::uint64_t( 1 ) << f_r ) - 1 );
 i = f_Q.runStart( q );
 if( ! f_Q.occupied( q ) )
  return( false );
 const std::size_t e = f_Q.runEnd( q );
 while( i <= e ) {
  std::uint64_t x1 , c1;
  std::size_t j1;
  decode( i , x1 , c1 , j1 );
  if( x1 >= x ) {
   j = j1;
   c = c1;
   return( x1 == x );
   }
  i = j1 + 1;
  }
 return( false );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::add( std::uint64_t y ,
					     std::uint64_t c )
{
 std::size_t i , j = 0;
 std::uint64_t c0 = 0;
 const bool f = find( y , i , j , c0 );
 Code C;
 const std::size_t k =
   code( y & ( ( std::uint64_t( 1 ) << f_r ) - 1 ) , ( f ? c0 : 0 ) + c , C );
 if( ! f_Q.replace( std::size_t( y >> f_r ) , i , f ? j - i + 1 : 0 ,
		    C.data() ,
		    k ) )
  return( false );
 f_n += c;
 f_d += ! f;
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::insertHash( std::uint64_t h ,
						    std::uint64_t c )
{
 ADS_PRE( c >= 1 );
 return( add( fingerprint( h ) , c ) );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::removeHash( std::uint64_t h ,
						    std::uint64_t c )
{
 const std::uint64_t y = fingerprint( h );
 std::size_t i , j = 0;
 std::uint64_t c0 = 0;
 if( ! find( y , i , j , c0 ) || ( c0 < c ) )
  return( false );
 Code C;
 const std::size_t k =
   code( y & ( ( std::uint64_t( 1 ) << f_r ) - 1 ) , c0 - c , C );
 f_Q.replace( std::size_t( y >> f_r ) , i , j - i + 1 , C.data() , k );
 f_n -= c;
 f_d -= ( c0 == c );
 return( true );
 }

template< class T , class F , class H >
std::uint64_t
CountingQuotientFilter< T , F , H >::countHash( std::uint64_t h ) const
{
 std::size_t i , j;
 std::uint64_t c = 0;
 return( find( fingerprint( h ) , i , j , c ) ? c : 0 );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool
CountingQuotientFilter< T , F , H >::maybeHash( std::uint64_t h ) const
{
 const std::uint64_t y = fingerprint( h );
 const std::size_t q = std::size_t( y >> f_r );
 if( ! f_Q.occupied( q ) )  // most of the absent keys, one bit
  return( false );
 std::size_t i , j;
 std::uint64_t c;
 return( find( y , i , j , c ) );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::insert( const T & x ,
						std::uint64_t c )
{
 const bool in = insertHash( hash( x ) , c );
 ADS_POST( ! in || ( count( x ) >= c ) );
 return( in );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::remove( const T & x ,
						std::uint64_t c )
{
 return( removeHash( hash( x ) , c ) );
 }

template< class T , class F , class H >
std::uint64_t CountingQuotientFilter< T , F , H >::count( const T & x ) const
{
 return( countHash( hash( x ) ) );
 }

template< class T , class F , class H >
ADS_FILTER_INLINE bool
CountingQuotientFilter< T , F , H >::maybe( const T & x ) const
{
 return( maybeHash( hash( x ) ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
template< class Fn >
void CountingQuotientFilter< T , F , H >::forEach( Fn && fn ) const
{
 f_Q.forEachRun( [ & ]( std::size_t q , std::size_t s , std::size_t e ) {
  for( std::size_t i = s ; i <= e ; ) {
   std::uint64_t x , c;
   std::size_t j;
   decode( i , x , c , j );
   fn( ( std::uint64_t( q ) << f_r ) | x , c );
   i = j + 1;
   }
  } );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::rebuild(
  int l , const std::vector< std::uint64_t > & Y ,
  const std::vector< std::uint64_t > & C )
{
 CountingQuotientFilter t( 1 , f_h );
 t.f_r = fingerprintBits() - l;
 t.f_Q = detail::QuotientTable< F >( l );
 for( std::size_t i = 0 ; i < Y.size() ; ++i )
  if( ! t.add( Y[ i ] , C[ i ] ) )
   return( false );
 *this = std::move( t );
 return( true );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::resize( std::size_t m )
{
 ADS_PRE( ( m >= 64 ) && std::has_single_bit( m ) );
 const int l = std::countr_zero( m );
 ADS_PRE( ( fingerprintBits() - l >= 2 ) &&
	  ( fingerprintBits() - l <= int( 8 * sizeof( F ) ) ) );
 std::vector< std::uint64_t > Y , C;
 Y.reserve( f_d );
 C.reserve( f_d );
 forEach( [ & ]( std::uint64_t y , std::uint64_t c ) {
  Y.push_back( y );
  C.push_back( c );
  } );
 return( rebuild( l , Y , C ) );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::merge(
  const CountingQuotientFilter & o )
{
 ADS_PRE( o.fingerprintBits() == fingerprintBits() );
 std::vector< std::uint64_t > YA , CA , Y , C;
 forEach( [ & ]( std::uint64_t y , std::uint64_t c ) {
  YA.push_back( y );
  CA.push_back( c );
  } );
 std::size_t a = 0;
 o.forEach( [ & ]( std::uint64_t y , std::uint64_t c ) {
  for( ; ( a < YA.size() ) && ( YA[ a ] < y ) ; ++a ) {
   Y.push_back( YA[ a ] );
   C.push_back( CA[ a ] );
   }
  if( ( a < YA.size() ) && ( YA[ a ] == y ) )
   c += CA[ a++ ];
  Y.push_back( y );
  C.push_back( c );
  } );
 for( ; a < YA.size() ; ++a ) {
  Y.push_back( YA[ a ] );
  C.push_back( CA[ a ] );
  }
 int l = f_Q.quotientBits();
 const double need = double( slotsUsed() + o.slotsUsed() );
 while( ( need > 0.95 * std::ldexp( 1.0 , l ) ) &&
	( fingerprintBits() - l > 2 ) && ( l < 39 ) )
  ++l;
 return( rebuild( l , Y , C ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
void CountingQuotientFilter< T , F , H >::clear( void )
{
 f_Q.clear();
 f_n = 0;
 f_d = 0;
 }

template< class T , class F , class H >
std::uint64_t CountingQuotientFilter< T , F , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class F , class H >
std::size_t CountingQuotientFilter< T , F , H >::distinct( void ) const
{
 return( f_d );
 }

template< class T , class F , class H >
bool CountingQuotientFilter< T , F , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class F , class H >
std::size_t CountingQuotientFilter< T , F , H >::capacity( void ) const
{
 return( f_Q.quotients() );
 }

template< class T , class F , class H >
std::size_t CountingQuotientFilter< T , F , H >::slotsUsed( void ) const
{
 return( f_Q.used() );
 }

template< class T , class F , class H >
double CountingQuotientFilter< T , F , H >::load( void ) const
{
 return( double( slotsUsed() ) / double( capacity() ) );
 }

template< class T , class F , class H >
int CountingQuotientFilter< T , F , H >::remainderBits( void ) const
{
 return( f_r );
 }

template< class T , class F , class H >
int CountingQuotientFilter< T , F , H >::fingerprintBits( void ) const
{
 return( f_Q.quotientBits() + f_r );
 }

template< class T , class F , class H >
double CountingQuotientFilter< T , F , H >::falsePositiveBound( void ) const
{
 return( std::ldexp( double( f_d ) , -fingerprintBits() ) );
 }

template< class T , class F , class H >
std::size_t CountingQuotientFilter< T , F , H >::bits( void ) const
{
 return( f_Q.bits() );
 }

/*--------------------------------------------------------------------------*/

template< class T , class F , class H >
template< class A >
inline void CountingQuotientFilter< T , F , H >::persist( A & a )
{
 a( f_Q , f_n , f_d , f_r );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( ( f_r >= 2 ) && ( f_r <= int( 8 * sizeof( F ) ) ) ,
	    "the remainders of a CountingQuotientFilter" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File CountingQuotientFilter.tpp -----------------*/
/*--------------------------------------------------------------------------*/
