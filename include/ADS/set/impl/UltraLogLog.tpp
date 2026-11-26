/*--------------------------------------------------------------------------*/
/*------------------------- File UltraLogLog.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of UltraLogLog [see UltraLogLog.h], with their contracts.
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

/// the word of the levels of a register r: 0 for r = 0, else the bit
/// floor( r / 4 ) and, below it, the two bits r mod 4
inline std::uint64_t ullUnpack( std::uint8_t r )
{
 if( r == 0 )
  return( 0 );
 return( ( std::uint64_t( 4 ) | ( r & 3U ) ) << ( ( r >> 2 ) - 2 ) );
 }

/// the register of a word x != 0 whose top bit is at least 2: four times
/// the top bit, plus the two bits below it
inline std::uint8_t ullPack( std::uint64_t x )
{
 const unsigned k = unsigned( std::bit_width( x ) ) - 1;
 return( std::uint8_t( ( k << 2 ) | ( ( x >> ( k - 2 ) ) & 3U ) ) );
 }

/// the constants of the FGRA estimator: tau and eta_0 .. eta_3 of Ertl,
/// the mean xi and the relative variance v of g( u , b ) lambda^tau in
/// the model of Poisson with levels of every integer, and the table of
/// eta_{ r mod 4 } 2^{ -tau floor( r / 4 ) } over the registers r
struct UllConstants
{
 double tau = 0.8194911375910897;
 double eta[ 4 ] = { 4.663135422063788 , 2.1378502137958524 ,
		     2.781144650979996 ,
		     0.9824082545153715 };
 double xi , v;
 double g[ 256 ];

 /// for each precision p from 3 to 26, the class of each register r for
 /// the tally (0 if empty, 1 of level 1, 2 and 3 of level 2 without and
 /// with the bit of the level 1, 4 else) and its term g[ r ] in the class
 /// 4, 0 in the others
 std::uint8_t cls[ 27 ][ 256 ];
 double gp[ 27 ][ 256 ];

 UllConstants( void ) {
  // E[ g ] lambda^tau = Gamma( tau ) / ln 2 sum_c a_c ( ( 1 + c )^-tau -
  // ( 2 + c )^-tau ), the coefficients a_c of the expansion of the
  // probabilities of the two bits in powers of e^{ -lambda 2^-u }
  const auto mean = [ & ]( const double * e , double t ) {
   const double a[ 4 ] = { e[ 3 ] , e[ 1 ] - e[ 3 ] , e[ 2 ] - e[ 3 ] ,
			   e[ 0 ] - e[ 1 ] - e[ 2 ] + e[ 3 ] };
   double s = 0;
   for( int j = 0 ; j < 4 ; ++j )
    s +=
      a[ j ] * ( std::pow( 1.0 + 2 * j , -t ) - std::pow( 2.0 + 2 * j , -t ) );
   return( std::tgamma( t ) / std::log( 2.0 ) * s );
   };
  const double e2[ 4 ] = { eta[ 0 ] * eta[ 0 ] , eta[ 1 ] * eta[ 1 ] ,
			   eta[ 2 ] * eta[ 2 ] , eta[ 3 ] * eta[ 3 ] };
  xi = mean( eta , tau );
  v = mean( e2 , 2 * tau ) / ( xi * xi ) - 1;
  for( int r = 0 ; r < 256 ; ++r )
   g[ r ] = eta[ r & 3 ] * std::exp2( -tau * double( r >> 2 ) );
  for( int p = 3 ; p <= 26 ; ++p )
   for( int r = 0 ; r < 256 ; ++r ) {
    const int one = 4 * ( p - 1 ) , two = 4 * p;
    cls[ p ][ r ] = std::uint8_t( r == 0         ? 0
				  : r == one     ? 1
				  : r == two     ? 2
				  : r == two + 2 ? 3
						 : 4 );
    gp[ p ][ r ] = cls[ p ][ r ] == 4 ? g[ r ] : 0;
    }
  }

 /// the constants, made once
 static const UllConstants & get( void ) {
  static const UllConstants c;
  return( c );
  }

 /// e_0( z ) z: the sum over the levels u = 0 , -1 , -2 , ... of
 /// g( u , b ) times their probability, q = z^{ 2^j } at u = -j
 double empty( double z ) const {
  double s = 0 , w = 1;
  for( double q = z ; q > 1e-300 ; q *= q , w *= std::exp2( tau ) ) {
   const double q2 = q * q , q4 = q2 * q2;
   const double h = eta[ 0 ] * q2 * q4 + eta[ 1 ] * q2 * ( 1 - q4 ) +
		    eta[ 2 ] * q4 * ( 1 - q2 ) +
		    eta[ 3 ] * ( 1 - q2 ) * ( 1 - q4 );
   s += w * q * ( 1 - q ) * h;
   if( q < 1e-18 )
    break;
   }
  return( s );
  }

 /// e_1( z ): a register of level 1, whose levels 0 and -1 are seen with
 /// probabilities 1 - z and 1 - z^2
 double one( double z ) const {
  return( std::exp2( -tau ) *
	   ( eta[ 0 ] * z * z * z + eta[ 1 ] * z * ( 1 - z * z ) +
	     eta[ 2 ] * z * z * ( 1 - z ) +
	     eta[ 3 ] * ( 1 - z ) * ( 1 - z * z ) ) );
  }

 /// e_2( b , z ): a register of level 2 with the bit b of the level 1,
 /// whose level 0 is seen with probability 1 - z
 double two( unsigned b , double z ) const {
  return( std::exp2( -2 * tau ) *
	   ( eta[ 2 * b ] * z + eta[ 2 * b + 1 ] * ( 1 - z ) ) );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , class H >
UltraLogLog< T , H >::UltraLogLog( unsigned p , H h ) : f_p( p ) , f_h( h )
{
 ADS_PRE( p >= 3 && p <= 26 );
 f_R.assign( std::size_t( 1 ) << p , 0 );
 retally();
 }

template< class T , class H >
void UltraLogLog< T , H >::retally( void )
{
 const auto & c = detail::UllConstants::get();
 f_t = Tally();
 f_t.cls = c.cls[ f_p ];
 f_t.g = c.gp[ f_p ];
 for( const std::uint8_t r : f_R ) {
  ++f_t.c[ f_t.cls[ r ] ];
  f_t.s += f_t.g[ r ];
  }
 }

template< class T , class H >
std::uint64_t UltraLogLog< T , H >::hash( const T & x ) const
{
 return( detail::sketchHash( f_h , x ) );
 }

template< class T , class H >
void UltraLogLog< T , H >::insertHash( std::uint64_t h )
{
 const std::size_t i = std::size_t( h >> ( 64 - f_p ) );
 const unsigned z = unsigned(
   std::countl_zero( ( h << f_p ) | ( ( std::uint64_t( 1 ) << f_p ) - 1 ) ) );
 const std::uint8_t r = f_R[ i ] ,
		    s = detail::ullPack(
		      detail::ullUnpack( r ) |
		      ( std::uint64_t( 1 ) << ( z + f_p - 1 ) ) );
 // the tally changed with no branch, by nothing when s = r, since a test
 // of s != r would be mispredicted on the first items of each register
 f_R[ i ] = s;
 --f_t.c[ f_t.cls[ r ] ];
 ++f_t.c[ f_t.cls[ s ] ];
 f_t.s += f_t.g[ s ] - f_t.g[ r ];
 }

template< class T , class H >
void UltraLogLog< T , H >::insert( const T & x )
{
 insertHash( hash( x ) );
 }

template< class T , class H >
std::uint64_t UltraLogLog< T , H >::block( const std::uint8_t * R ,
					   unsigned d ,
					  unsigned q )
{
 // the register k > 0 of the block sets the level of the leading zeros of
 // k in d bits, the same for all the k of [ 2^t , 2^{ t + 1 } )
 std::uint64_t x = detail::ullUnpack( R[ 0 ] );
 for( unsigned t = 0 ; t < d ; ++t )
  for( std::size_t k = std::size_t( 1 ) << t ; k < std::size_t( 2 ) << t ;
       ++k )
   if( R[ k ] != 0 ) {
    x |= std::uint64_t( 1 ) << ( q + d - 2 - t );
    break;
    }
 return( x );
 }

template< class T , class H >
void UltraLogLog< T , H >::merge( const UltraLogLog & o )
{
 ADS_PRE( o.f_p >= f_p );
 const unsigned d = o.f_p - f_p;
 if( d == 0 )  // register by register
  for( std::size_t i = 0 ; i < f_R.size() ; ++i ) {
   const std::uint64_t x =
     detail::ullUnpack( f_R[ i ] ) | detail::ullUnpack( o.f_R[ i ] );
   if( x != 0 )
    f_R[ i ] = detail::ullPack( x );
   }
 else
  for( std::size_t i = 0 ; i < f_R.size() ; ++i ) {
   const std::uint64_t x = detail::ullUnpack( f_R[ i ] ) |
			   block( o.f_R.data() + ( i << d ) , d , f_p );
   if( x != 0 )
    f_R[ i ] = detail::ullPack( x );
   }
 retally();
 }

template< class T , class H >
UltraLogLog< T , H > UltraLogLog< T , H >::downsize( unsigned q ) const
{
 ADS_PRE( q >= 3 && q <= f_p );
 UltraLogLog s( q , f_h );
 s.merge( *this );
 return( s );
 }

template< class T , class H >
double UltraLogLog< T , H >::estimate( void ) const
{
 const auto & c = detail::UllConstants::get();
 const double m = double( f_R.size() );
 const std::size_t c0 = f_t.c[ 0 ] , c1 = f_t.c[ 1 ] , c20 = f_t.c[ 2 ] ,
		   c21 = f_t.c[ 3 ];
 // the sum may have drifted below 0 by the roundings of its changes
 double s = std::max( 0.0 , f_t.s );
 if( c0 == f_R.size() )
  return( 0 );
 // g( u , b ) = eta_b 2^{ -tau u } with u = floor( r / 4 ) - p + 2
 s *= std::exp2( c.tau * double( f_p - 2 ) );
 const double z = double( c0 ) / m;
 s += m * c.empty( z ) + double( c1 ) * c.one( z ) +
      double( c20 ) * c.two( 0 , z ) + double( c21 ) * c.two( 1 , z );
 const double it = 1 / c.tau;
 return( m * std::pow( m * c.xi / s , it ) /
	  ( 1 + it * ( it + 1 ) * c.v / ( 2 * m ) ) );
 }

template< class T , class H >
double UltraLogLog< T , H >::estimateML( void ) const
{
 // beta[ l ] the registers that have seen the level l, a[ j ] those that
 // know they have not seen levels of total rate 2^-j
 const unsigned L = 65 - f_p;
 double beta[ 66 ] = {} , a[ 66 ] = {};
 for( const std::uint8_t r : f_R ) {
  if( r == 0 ) {
   ++a[ 0 ];
   continue;
   }
  const unsigned u = ( r >> 2 ) - f_p + 2;
  ++beta[ u ];
  if( u < L )
   ++a[ u ];
  if( r & 2 )
   ++beta[ u - 1 ];
  else if( u >= 2 )
   ++a[ u - 1 ];
  if( r & 1 )
   ++beta[ u - 2 ];
  else if( u >= 3 )
   ++a[ u - 2 ];
  }
 double alpha = 0 , B = 0 , W = 0;
 const auto w = [ & ]( unsigned l ) {
  return( std::exp2( -double( l < L ? l : L - 1 ) ) );
  };
 for( unsigned j = 0 ; j <= L ; ++j )
  alpha += a[ j ] * std::exp2( -double( j ) );
 for( unsigned l = 1 ; l <= L ; ++l ) {
  B += beta[ l ];
  W += beta[ l ] * w( l );
  }
 if( B == 0 )
  return( 0 );
 if( alpha == 0 )
  return( std::numeric_limits< double >::infinity() );
 // Newton on the decreasing and convex f from its lower bound, which
 // increases to the root
 double lambda = B / ( alpha + W / 2 );
 for( int it = 0 ; it < 200 ; ++it ) {
  double f = -alpha , df = 0;
  for( unsigned l = 1 ; l <= L ; ++l ) {
   if( beta[ l ] == 0 )
    continue;
   const double y = lambda * w( l );
   if( y > 700 )
    continue;
   const double t = std::expm1( y );
   f += beta[ l ] * w( l ) / t;
   df -= beta[ l ] * w( l ) * w( l ) * ( t + 1 ) / ( t * t );
   }
  const double step = -f / df;
  lambda += step;
  if( ! ( step > 1e-12 * lambda ) )
   break;
  }
 return( double( f_R.size() ) * lambda );
 }

template< class T , class H >
double UltraLogLog< T , H >::relativeStandardError( bool ml ) const
{
 const auto & c = detail::UllConstants::get();
 const double V = ml ? 4.631 / 8 : c.v / ( c.tau * c.tau );
 return( std::sqrt( V / double( f_R.size() ) ) );
 }

template< class T , class H >
void UltraLogLog< T , H >::clear( void )
{
 std::fill( f_R.begin() , f_R.end() , std::uint8_t( 0 ) );
 retally();
 }

template< class T , class H >
bool UltraLogLog< T , H >::empty( void ) const
{
 return( f_t.c[ 0 ] == f_R.size() );
 }

template< class T , class H >
unsigned UltraLogLog< T , H >::precision( void ) const
{
 return( f_p );
 }

template< class T , class H >
const std::vector< std::uint8_t > & UltraLogLog< T , H >::state( void ) const
{
 return( f_R );
 }

template< class T , class H >
bool UltraLogLog< T , H >::operator==( const UltraLogLog & o ) const
{
 return( f_p == o.f_p && f_R == o.f_R );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
template< class A >
inline void UltraLogLog< T , H >::persist( A & a )
{
 a( f_R , f_p );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 const std::size_t m = std::size_t( 1 ) << ( f_p & 31 );
 a.require( f_p >= 3 && f_p <= 26 && f_R.size() == m ,
	    "the precision of an UltraLogLog" );
 // a register is 0, or of a level of at least 1 whose bits below it are
 // of levels of at least 1
 bool ok = true;
 for( const std::uint8_t r : f_R )
  if( r != 0 ) {
   const unsigned k = r >> 2;
   ok = ok && ( k + 1 >= f_p ) && ( k != f_p - 1 || ( r & 3 ) == 0 ) &&
	( k != f_p || ( r & 1 ) == 0 );
   }
 a.require( ok , "the registers of an UltraLogLog" );
 if constexpr( A::loading )
  retally();
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File UltraLogLog.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
