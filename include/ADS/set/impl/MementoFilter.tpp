/*--------------------------------------------------------------------------*/
/*------------------------- File MementoFilter.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MementoFilter [see MementoFilter.h], with their
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

template< RadixKey T , class S >
MementoFilter< T , S >::MementoFilter( std::size_t n , int r , int f ,
				      std::uint64_t seed )
    : f_seed( seed ) , f_r( r ) ,
      f_f( f == 0 ? int( 8 * sizeof( S ) ) - r : f )
{
 constexpr int w =
   int( 8 * sizeof( decltype( radixKey( std::declval< T >() ) ) ) );
 ADS_PRE( ( r >= 2 ) && ( r < w ) && ( f_f >= 2 ) &&
	  ( f_f + r <= int( 8 * sizeof( S ) ) ) );
 const std::size_t m = std::max< std::size_t >(
   64 , std::bit_ceil( std::size_t( std::ceil( double( n ) / 0.95 ) ) ) );
 ADS_PRE( m < ( std::size_t( 1 ) << 40 ) );
 f_w = f_f + r;
 f_Q = detail::QuotientTable< S >( std::countr_zero( m ) );
 }

/*--------------------------------------------------------------------------*/

template< RadixKey T , class S >
std::uint64_t MementoFilter< T , S >::word( const T & x )
{
 return( std::uint64_t( radixKey( x ) ) );
 }

template< RadixKey T , class S >
void MementoFilter< T , S >::place( std::uint64_t p , std::size_t & q ,
				   std::uint64_t & fp ) const
{
 const std::uint64_t y =
   hashWord( p , f_seed ) >> ( 64 - f_Q.quotientBits() - f_f );
 q = std::size_t( y >> f_f );
 fp = y & ( ( std::uint64_t( 1 ) << f_f ) - 1 );
 fp += ( fp == 0 );
 }

template< RadixKey T , class S >
std::uint64_t MementoFilter< T , S >::chunk( std::size_t i ,
					    std::size_t t ) const
{
 const std::size_t w = std::size_t( f_w ) , o = t % w;
 i += t / w;
 std::uint64_t v = std::uint64_t( f_Q.get( i ) ) >> o;
 if( o + std::size_t( f_r ) > w )  // across two slots
  v |= std::uint64_t( f_Q.get( i + 1 ) ) << ( w - o );
 return( v & ( ( std::uint64_t( 1 ) << f_r ) - 1 ) );
 }

template< RadixKey T , class S >
auto MementoFilter< T , S >::box( std::size_t i , std::size_t e ) const -> Box
{
 const std::uint64_t mr = ( std::uint64_t( 1 ) << f_r ) - 1;
 Box B;
 const std::uint64_t s0 = f_Q.get( i );
 B.fp = s0 >> f_r;
 B.lo = B.hi = s0 & mr;
 B.k = 1;
 B.last = i;
 if( i == e )
  return( B );
 const std::uint64_t s1 = f_Q.get( i + 1 ) , f1 = s1 >> f_r;
 if( f1 > B.fp )  // another box
  return( B );
 B.hi = s1 & mr;
 B.last = i + 1;
 B.k = 2;
 if( f1 == B.fp )
  return( B );
 // the fingerprint 0: the length k - 2 and the list, packed from i + 2
 B.list = i + 2;
 std::size_t t = std::size_t( f_r );
 std::uint64_t v = chunk( B.list , 0 );
 if( v == mr ) {  // c chunks of 2^r - 1, then c + 1 digits
  std::size_t c = 1;
  while( ( v = chunk( B.list , t ) ) == mr ) {
   ++c;
   t += std::size_t( f_r );
   }
  t += std::size_t( f_r );
  for( ; c > 0 ; --c ) {
   v = v * mr + chunk( B.list , t );
   t += std::size_t( f_r );
   }
  }
 B.k = std::size_t( v ) + 2;
 B.at = t;
 t += std::size_t( f_r ) * ( B.k - 2 );
 B.last = B.list + ( t + std::size_t( f_w ) - 1 ) / std::size_t( f_w ) - 1;
 return( B );
 }

template< RadixKey T , class S >
std::vector< std::uint64_t >
MementoFilter< T , S >::mementos( const Box & B ) const
{
 std::vector< std::uint64_t > M;
 M.reserve( B.k );
 M.push_back( B.lo );
 for( std::size_t j = 0 ; j + 2 < B.k ; ++j )
  M.push_back( chunk( B.list , B.at + j * std::size_t( f_r ) ) );
 if( B.k > 1 )
  M.push_back( B.hi );
 return( M );
 }

template< RadixKey T , class S >
std::vector< S >
MementoFilter< T , S >::encode( std::uint64_t fp ,
			       const std::vector< std::uint64_t > & M ) const
{
 const std::size_t k = M.size();
 std::vector< S > V;
 if( k == 0 )
  return( V );
 V.push_back( S( ( fp << f_r ) | M.front() ) );
 if( k == 1 )
  return( V );
 V.push_back( S( ( ( k == 2 ? fp : 0 ) << f_r ) | M.back() ) );
 if( k == 2 )
  return( V );
 // the chunks: the length, then the mementos between the two ends
 const std::uint64_t mr = ( std::uint64_t( 1 ) << f_r ) - 1;
 std::vector< std::uint64_t > C;
 const std::uint64_t L = k - 2;
 if( L < mr )
  C.push_back( L );
 else {
  std::vector< std::uint64_t > D;  // the digits in base 2^r - 1
  for( std::uint64_t v = L ; v > 0 ; v /= mr )
   D.push_back( v % mr );
  C.assign( D.size() - 1 , mr );
  C.insert( C.end() , D.rbegin() , D.rend() );
  }
 C.insert( C.end() , M.begin() + 1 , M.end() - 1 );
 const std::size_t w = std::size_t( f_w ) ,
		   bits = C.size() * std::size_t( f_r );
 std::vector< std::uint64_t > W( ( bits + w - 1 ) / w , 0 );
 for( std::size_t j = 0 ; j < C.size() ; ++j ) {
  const std::size_t t = j * std::size_t( f_r ) , i = t / w , o = t % w;
  W[ i ] |= C[ j ] << o;
  if( o + std::size_t( f_r ) > w )
   W[ i + 1 ] |= C[ j ] >> ( w - o );
  }
 const std::uint64_t mw =
   w >= 64 ? ~std::uint64_t( 0 ) : ( std::uint64_t( 1 ) << w ) - 1;
 for( std::uint64_t x : W )
  V.push_back( S( x & mw ) );
 return( V );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::meets( const Box & B , std::uint64_t a ,
				   std::uint64_t b ) const
{
 if( ( B.lo > b ) || ( B.hi < a ) )
  return( false );
 if( ( B.lo >= a ) || ( B.hi <= b ) )
  return( true );
 // lo < a and hi > b: the first memento of the list not below a
 std::size_t l = 0 , u = B.k - 2;
 while( l < u ) {
  const std::size_t j = ( l + u ) / 2;
  if( chunk( B.list , B.at + j * std::size_t( f_r ) ) < a )
   l = j + 1;
  else
   u = j;
  }
 return( ( l < B.k - 2 ) &&
	  ( chunk( B.list , B.at + l * std::size_t( f_r ) ) <= b ) );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::find( std::size_t q , std::uint64_t fp , Box & B ,
				  std::size_t & i ) const
{
 i = f_Q.runStart( q );
 if( ! f_Q.occupied( q ) )
  return( false );
 const std::size_t e = f_Q.runEnd( q );
 while( i <= e ) {
  B = box( i , e );
  if( B.fp >= fp )
   return( B.fp == fp );
  i = B.last + 1;
  }
 return( false );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::boxOf( std::uint64_t p , Box & B ) const
{
 std::size_t q , i;
 std::uint64_t fp;
 place( p , q , fp );
 return( find( q , fp , B , i ) );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::add( std::size_t q , std::uint64_t fp ,
				 std::uint64_t u )
{
 Box B;
 std::size_t i;
 if( find( q , fp , B , i ) ) {
  auto M = mementos( B );
  M.insert( std::upper_bound( M.begin() , M.end() , u ) , u );
  const auto V = encode( fp , M );
  if( ! f_Q.replace( q , i , B.last - i + 1 , V.data() , V.size() ) )
   return( false );
  }
 else {
  const S v = S( ( fp << f_r ) | u );
  if( ! f_Q.replace( q , i , 0 , &v , 1 ) )
   return( false );
  ++f_boxes;
  }
 ++f_n;
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< RadixKey T , class S >
bool MementoFilter< T , S >::insert( const T & x )
{
 const std::uint64_t v = word( x );
 std::size_t q;
 std::uint64_t fp;
 place( v >> f_r , q , fp );
 const bool in = add( q , fp , v & ( ( std::uint64_t( 1 ) << f_r ) - 1 ) );
 ADS_POST( ! in || maybe( x ) );
 return( in );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::remove( const T & x )
{
 const std::uint64_t v = word( x ) ,
		     u = v & ( ( std::uint64_t( 1 ) << f_r ) - 1 );
 std::size_t q , i;
 std::uint64_t fp;
 place( v >> f_r , q , fp );
 Box B;
 if( ! find( q , fp , B , i ) )
  return( false );
 auto M = mementos( B );
 const auto it = std::lower_bound( M.begin() , M.end() , u );
 if( ( it == M.end() ) || ( *it != u ) )
  return( false );
 M.erase( it );
 const auto V = encode( fp , M );
 if( ! f_Q.replace( q , i , B.last - i + 1 , V.data() , V.size() ) )
  return( false );
 --f_n;
 f_boxes -= M.empty();
 return( true );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::mayContain( const T & a , const T & b ) const
{
 const std::uint64_t wa = word( a ) , wb = word( b ) ,
		     mr = ( std::uint64_t( 1 ) << f_r ) - 1;
 ADS_PRE( wa <= wb );
 const std::uint64_t pa = wa >> f_r , pb = wb >> f_r;
 Box B;
 if( pa == pb )
  return( boxOf( pa , B ) && meets( B , wa & mr , wb & mr ) );
 if( boxOf( pa , B ) && ( B.hi >= ( wa & mr ) ) )
  return( true );
 if( boxOf( pb , B ) && ( B.lo <= ( wb & mr ) ) )
  return( true );
 if( pb - pa - 1 > capacity() )  // too many partitions to ask
  return( ! empty() );
 for( std::uint64_t p = pa + 1 ; p < pb ; ++p )
  if( boxOf( p , B ) )
   return( true );
 return( false );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::maybe( const T & x ) const
{
 return( mayContain( x , x ) );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::resize( std::size_t m )
{
 ADS_PRE( ( m >= capacity() ) && std::has_single_bit( m ) );
 const int l = std::countr_zero( m ) , k = l - f_Q.quotientBits() ,
   f = f_f - k;
 ADS_PRE( f >= 2 );
 MementoFilter t( 1 , f_r , f , f_seed );
 t.f_w = f_w;
 t.f_Q = detail::QuotientTable< S >( l );
 bool ok = true;
 f_Q.forEachRun( [ & ]( std::size_t q , std::size_t s , std::size_t e ) {
  for( std::size_t i = s ; ok && ( i <= e ) ; ) {
   const Box B = box( i , e );
   const std::uint64_t y = ( std::uint64_t( q ) << f_f ) | B.fp;
   std::uint64_t fp = y & ( ( std::uint64_t( 1 ) << f ) - 1 );
   fp += ( fp == 0 );
   for( std::uint64_t u : mementos( B ) )
    ok = ok && t.add( std::size_t( y >> f ) , fp , u );
   i = B.last + 1;
   }
  } );
 if( ! ok )
  return( false );
 *this = std::move( t );
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< RadixKey T , class S >
double MementoFilter< T , S >::falsePositiveBound( void ) const
{
 const double rho =
   ( std::ldexp( 1.0 , f_f ) + 2 ) / std::ldexp( 1.0 , 2 * f_f );
 return( std::min( 1.0 , 2 * double( f_n ) * rho / double( capacity() ) ) );
 }

template< RadixKey T , class S >
void MementoFilter< T , S >::clear( void )
{
 f_Q.clear();
 f_n = 0;
 f_boxes = 0;
 }

template< RadixKey T , class S >
std::size_t MementoFilter< T , S >::size( void ) const
{
 return( f_n );
 }

template< RadixKey T , class S >
bool MementoFilter< T , S >::empty( void ) const
{
 return( f_n == 0 );
 }

template< RadixKey T , class S >
std::size_t MementoFilter< T , S >::boxes( void ) const
{
 return( f_boxes );
 }

template< RadixKey T , class S >
std::size_t MementoFilter< T , S >::capacity( void ) const
{
 return( f_Q.quotients() );
 }

template< RadixKey T , class S >
double MementoFilter< T , S >::load( void ) const
{
 return( double( f_Q.used() ) / double( capacity() ) );
 }

template< RadixKey T , class S >
int MementoFilter< T , S >::mementoBits( void ) const
{
 return( f_r );
 }

template< RadixKey T , class S >
int MementoFilter< T , S >::fingerprintBits( void ) const
{
 return( f_f );
 }

template< RadixKey T , class S >
std::size_t MementoFilter< T , S >::bits( void ) const
{
 return( f_Q.bits() );
 }

/*--------------------------------------------------------------------------*/

template< RadixKey T , class S >
template< class A >
inline void MementoFilter< T , S >::persist( A & a )
{
 a( f_Q , f_n , f_boxes , f_seed , f_r , f_f , f_w );
 a.require( ( f_r >= 2 ) && ( f_f >= 2 ) && ( f_w >= f_f + f_r ) &&
	      ( f_w <= int( 8 * sizeof( S ) ) ) ,
	    "the slots of a MementoFilter" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File MementoFilter.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
