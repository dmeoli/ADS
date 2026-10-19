/*--------------------------------------------------------------------------*/
/*----------------------- File RandomGenerators.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the generators and of the functions of
 * RandomGenerators.h [see RandomGenerators.h].
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

template< bool StarStar >
constexpr Xoshiro256T< StarStar >::Xoshiro256T( std::uint64_t seed )
 : f_s{}
{
 for( auto & w : f_s ) {                    // the words of SplitMix64
  w = splitmix64( seed );
  seed += 0x9e3779b97f4a7c15ULL;
  }
 if( ( f_s[ 0 ] | f_s[ 1 ] | f_s[ 2 ] | f_s[ 3 ] ) == 0 )
  f_s[ 0 ] = 1;                             // the only state to avoid
 }

template< bool StarStar >
constexpr std::uint64_t Xoshiro256T< StarStar >::operator()( void )
{
 const std::uint64_t r = StarStar ? std::rotl( f_s[ 1 ] * 5 , 7 ) * 9
                         : std::rotl( f_s[ 0 ] + f_s[ 3 ] , 23 ) + f_s[ 0 ] ,
                     t = f_s[ 1 ] << 17;
 f_s[ 2 ] ^= f_s[ 0 ];
 f_s[ 3 ] ^= f_s[ 1 ];
 f_s[ 1 ] ^= f_s[ 2 ];
 f_s[ 0 ] ^= f_s[ 3 ];
 f_s[ 2 ] ^= t;
 f_s[ 3 ] = std::rotl( f_s[ 3 ] , 45 );
 return( r );
 }

template< bool StarStar >
constexpr void Xoshiro256T< StarStar >::jump( void )
{
 constexpr std::uint64_t J[] = { 0x180ec6d33cfd0abaULL ,
                                 0xd5a61266f0c9392cULL ,
                                 0xa9582618e03fc9aaULL ,
                                 0x39abdc4529b1661cULL };
 std::uint64_t s[ 4 ] = { 0 , 0 , 0 , 0 };
 for( std::uint64_t j : J )                 // the polynomial of the jump
  for( int b = 0 ; b < 64 ; ++b ) {
   if( ( j >> b ) & 1 )
    for( int i = 0 ; i < 4 ; ++i ) s[ i ] ^= f_s[ i ];
   ( *this )();
   }
 for( int i = 0 ; i < 4 ; ++i ) f_s[ i ] = s[ i ];
 }

constexpr std::uint64_t WyRand::operator()( void )
{
 f_s += 0xa0761d6478bd642fULL;
 std::uint64_t hi;
 const std::uint64_t lo = detail::mul128( f_s , f_s ^ 0xe7037ed1a0b428dbULL ,
                                          hi );
 return( hi ^ lo );
 }

template< FullWordGenerator G >
std::uint64_t uniformBelow( G && g , std::uint64_t s )
{
 ADS_PRE( s >= 1 );
 std::uint64_t hi , lo = detail::mul128( g() , s , hi );
 if( lo < s ) {                             // a rejection is possible
  const std::uint64_t t = ( 0 - s ) % s;    // 2^64 mod s
  while( lo < t )
   lo = detail::mul128( g() , s , hi );
  }
 ADS_POST( hi < s );
 return( hi );
 }

inline AliasTable::AliasTable( const std::vector< double > & w )
 : f_T( w.size() )
{
 ADS_PRE( ! w.empty() && ( w.size() <= std::size_t( UINT32_MAX ) ) );
 const std::size_t n = w.size();
 double sum = 0;
 for( const double x : w ) {
  ADS_PRE( std::isfinite( x ) && ( x >= 0 ) );
  sum += x;
  }
 ADS_PRE( sum > 0 );
 std::vector< double > p( n );
 std::vector< std::uint32_t > small , large;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  p[ i ] = w[ i ] * double( n ) / sum;
  ( p[ i ] < 1 ? small : large ).push_back( std::uint32_t( i ) );
  }
 auto keep = []( double q ) {               // floor( q 2^64 ), q in [ 0 , 1 ]
  return( q >= 1 ? ~std::uint64_t( 0 )
                 : std::uint64_t( q * 0x1.0p64 ) ); };
 while( ! small.empty() && ! large.empty() ) {
  const std::uint32_t s = small.back() , l = large.back();
  small.pop_back();
  f_T[ s ] = Entry{ keep( p[ s ] ) , l };   // s keeps p_s, the rest is l's
  p[ l ] = ( p[ l ] + p[ s ] ) - 1;
  if( p[ l ] < 1 ) {
   large.pop_back();
   small.push_back( l );
   }
  }
 for( const std::uint32_t i : large )       // 1 up to the rounding
  f_T[ i ] = Entry{ ~std::uint64_t( 0 ) , i };
 for( const std::uint32_t i : small )
  f_T[ i ] = Entry{ ~std::uint64_t( 0 ) , i };
 }

template< FullWordGenerator G >
std::size_t AliasTable::operator()( G && g ) const
{
 std::uint64_t h;
 const std::uint64_t l = detail::mul128( g() , f_T.size() , h );
 const Entry & e = f_T[ h ];
 return( l < e.keep ? h : e.alias );
 }

inline double AliasTable::probability( std::size_t i ) const
{
 ADS_PRE( i < f_T.size() );
 const double n = double( f_T.size() );
 double q = 0;
 for( std::size_t j = 0 ; j < f_T.size() ; ++j ) {
  const double k = double( f_T[ j ].keep ) * 0x1.0p-64;
  if( j == i ) q += k;
  if( f_T[ j ].alias == i ) q += 1 - k;
  }
 return( q / n );
 }

template< FullWordGenerator G >
double uniformReal( G && g )
{
 return( double( g() >> 11 ) * 0x1.0p-53 );
 }

namespace detail {

/// the indices of the k steps of the shuffle of bounds n , n - 1 , ... ,
/// n - k + 1, whose product is p <= 2^64 (0 for 2^64), swapped at once
template< int k , class I , class G >
void shuffleBatch( I first , std::uint64_t n , std::uint64_t p , G & g )
{
 std::uint64_t idx[ k ];
 for( ; ; ) {
  std::uint64_t lo = g();
  for( int j = 0 ; j < k ; ++j )
   lo = mul128( lo , n - std::uint64_t( j ) , idx[ j ] );
  if( lo >= p )                             // no rejection is possible
   break;
  if( lo >= ( 0 - p ) % p )                 // above 2^64 mod p
   break;
  }
 for( int j = 0 ; j < k ; ++j )             // the steps n - 1 , n - 2 , ...
  std::iter_swap( first + std::ptrdiff_t( n - 1 - std::uint64_t( j ) ) ,
                  first + std::ptrdiff_t( idx[ j ] ) );
 }

}  // end( namespace detail )

template< std::random_access_iterator I , FullWordGenerator G >
void shuffleFast( I first , I last , G && g )
{
 std::uint64_t n = std::uint64_t( last - first );   // items still to place
 for( ; n > ( std::uint64_t( 1 ) << 32 ) ; --n )
  std::iter_swap( first + std::ptrdiff_t( n - 1 ) ,
                  first + std::ptrdiff_t( uniformBelow( g , n ) ) );
 for( ; n > ( std::uint64_t( 1 ) << 10 ) ; n -= 2 )  // 2 bounds below 2^32
  detail::shuffleBatch< 2 >( first , n , n * ( n - 1 ) , g );
 for( ; n > 6 ; n -= 6 )                    // 6 bounds below 2^10
  detail::shuffleBatch< 6 >( first , n , n * ( n - 1 ) * ( n - 2 ) *
                             ( n - 3 ) * ( n - 4 ) * ( n - 5 ) , g );
 for( ; n > 1 ; --n )
  std::iter_swap( first + std::ptrdiff_t( n - 1 ) ,
                  first + std::ptrdiff_t( uniformBelow( g , n ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File RandomGenerators.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
