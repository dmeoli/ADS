/*--------------------------------------------------------------------------*/
/*------------------------- File RandomNumbers.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RandomNumbers.h, with their contracts.
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

inline LinearCongruential::LinearCongruential( std::uint64_t a ,
                                               std::uint64_t c ,
                                               std::uint64_t m ,
                                               std::uint64_t seed )
 : f_a( a ) , f_c( c ) , f_m( m ) , f_x( seed )
{
 ADS_PRE( ( m == 0 ) || ( ( a < m ) && ( c < m ) && ( seed < m ) ) );
 }

inline LinearCongruential LinearCongruential::minstd( std::uint64_t seed )
{
 return( LinearCongruential( 16807 , 0 , 2147483647 , seed ) );
 }

inline LinearCongruential LinearCongruential::mmix( std::uint64_t seed )
{
 return( LinearCongruential( 6364136223846793005ULL ,
                             1442695040888963407ULL , 0 , seed ) );
 }

inline std::uint64_t LinearCongruential::operator()( void )
{
 if( f_m == 0 )                             // modulo 2^64 by overflow
  f_x = f_a * f_x + f_c;
 else {
  f_x = mulMod( f_a , f_x , f_m );          // + c without overflow
  f_x = f_x >= f_m - f_c ? f_x - ( f_m - f_c ) : f_x + f_c;
  }
 return( f_x );
 }

inline std::uint64_t LinearCongruential::period( std::uint64_t limit ) const
{
 LinearCongruential g = *this;              // the sequence is eventually
 const std::uint64_t start = g();           // periodic: Brent's cycle
 std::uint64_t power = 1 , lambda = 1 , tort = start , hare = g();
 while( tort != hare ) {                    // detection
  if( power == lambda ) {
   tort = hare;
   power *= 2;
   lambda = 0;
   }
  hare = g();
  ++lambda;
  if( lambda > limit ) return( 0 );
  }
 return( lambda );
 }

inline double chiSquare( const std::vector< long > & Y ,
                         const std::vector< double > & p )
{
 ADS_PRE( Y.size() == p.size() );
 double n = 0;
 for( long y : Y ) n += double( y );
 double V = 0;
 for( std::size_t s = 0 ; s < Y.size() ; ++s ) {
  const double e = n * p[ s ];
  ADS_PRE( e > 0 );
  V += ( double( Y[ s ] ) - e ) * ( double( Y[ s ] ) - e ) / e;
  }
 return( V );
 }

inline std::pair< double , double > kolmogorovSmirnov(
                                             std::vector< double > U )
{
 ADS_PRE( ! U.empty() );
 std::sort( U.begin() , U.end() );
 const double n = double( U.size() );
 double plus = 0 , minus = 0;               // max( j / n - F( x_j ) ) and
 for( std::size_t j = 0 ; j < U.size() ; ++j ) {   // max( F( x_j ) -
  plus = std::max( plus , double( j + 1 ) / n - U[ j ] );   // ( j - 1 ) / n )
  minus = std::max( minus , U[ j ] - double( j ) / n );
  }
 return( std::pair( std::sqrt( n ) * plus , std::sqrt( n ) * minus ) );
 }

inline double serialTest( const std::vector< double > & U , int d )
{
 ADS_PRE( ( d >= 2 ) && ( U.size() >= 2 ) );
 std::vector< long > Y( std::size_t( d ) * std::size_t( d ) , 0 );
 for( std::size_t j = 0 ; j + 1 < U.size() ; j += 2 ) {
  const auto a = std::size_t( U[ j ] * d ) , b = std::size_t( U[ j + 1 ] * d );
  ++Y[ std::min( a , std::size_t( d - 1 ) ) * std::size_t( d ) +
       std::min( b , std::size_t( d - 1 ) ) ];
  }
 return( chiSquare( Y , std::vector< double >( Y.size() ,
                                               1.0 / double( Y.size() ) ) ) );
 }

inline double spectralTest2( std::uint64_t a , std::uint64_t m )
{
 ADS_PRE( ( m >= 2 ) && ( a > 0 ) && ( a < m ) );
 long double u1 = double( m ) , v1 = 0 ,   // the basis ( m , 0 ) ,
             u2 = - double( a ) , v2 = 1;   // ( -a , 1 ), reduced as Gauss
 auto norm = []( long double u , long double v ) { return( u * u + v * v ); };
 for( ; ; ) {
  if( norm( u1 , v1 ) < norm( u2 , v2 ) ) {
   std::swap( u1 , u2 );
   std::swap( v1 , v2 );
   }
  const long double q =
   std::round( ( u1 * u2 + v1 * v2 ) / norm( u2 , v2 ) );
  u1 -= q * u2;
  v1 -= q * v2;
  if( norm( u1 , v1 ) >= norm( u2 , v2 ) )
   break;                                   // ( u2 , v2 ) is the shortest
  }
 return( double( std::sqrt( norm( u2 , v2 ) ) ) );
 }

template< class T , class R >
void shuffle( std::vector< T > & A , R & g )
{
 for( std::size_t j = A.size() ; j > 1 ; --j ) {   // P2-P3
  const std::size_t k = std::size_t( g() % j );
  std::swap( A[ j - 1 ] , A[ k ] );
  }
 }

template< class R >
std::vector< std::size_t > selectionSampling( std::size_t n , std::size_t N ,
                                              R & g )
{
 ADS_PRE( n <= N );
 std::vector< std::size_t > out;
 for( std::size_t t = 0 ; out.size() < n ; ++t ) {   // t seen, m chosen
  const double U = double( g() >> 11 ) * 0x1.0p-53;
  if( double( N - t ) * U < double( n - out.size() ) )
   out.push_back( t );
  }
 return( out );
 }

template< class T , class R >
std::vector< T > reservoirSampling( const std::vector< T > & S ,
                                    std::size_t n , R & g )
{
 std::vector< T > res;
 for( std::size_t t = 0 ; t < S.size() ; ++t )
  if( t < n )
   res.push_back( S[ t ] );                 // the first n fill it
  else {
   const std::size_t M = std::size_t( g() % ( t + 1 ) );
   if( M < n ) res[ M ] = S[ t ];           // R4: replace with prob. n/t
   }
 return( res );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RandomNumbers.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
