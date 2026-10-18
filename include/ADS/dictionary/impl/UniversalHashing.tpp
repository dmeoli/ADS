/*--------------------------------------------------------------------------*/
/*----------------------- File UniversalHashing.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of UniversalHashing.h, with their contracts.
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

/*--------------------------------------------------------------------------*/

inline CarterWegmanHash::CarterWegmanHash( std::uint64_t m ,
                                           std::uint64_t seed )
 : f_p( kMersenne61 ) , f_m( m )
{
 ADS_PRE( m >= 1 );
 SplitMix64 g( seed );
 f_a = 1 + g() % ( f_p - 1 );
 f_b = g() % f_p;
 }

inline CarterWegmanHash::CarterWegmanHash( std::uint64_t p ,
                                           std::uint64_t a ,
                                           std::uint64_t b ,
                                           std::uint64_t m )
 : f_p( p ) , f_a( a ) , f_b( b ) , f_m( m )
{
 ADS_PRE( ( p >= 2 ) && ( p < ( std::uint64_t( 1 ) << 63 ) ) &&
          ( a >= 1 ) && ( a < p ) && ( b < p ) && ( m >= 1 ) );
 }

inline std::uint64_t CarterWegmanHash::operator()( std::uint64_t k ) const
{
 ADS_PRE( k < f_p );
 std::uint64_t x = mulMod( f_a , k , f_p ) + f_b;   // below 2^64
 if( x >= f_p ) x -= f_p;
 const std::uint64_t r = x % f_m;
 ADS_POST( r < f_m );
 return( r );
 }

/*--------------------------------------------------------------------------*/

inline DotProductHash::DotProductHash( std::uint64_t m , int r ,
                                       std::uint64_t seed )
 : f_m( m )
{
 ADS_PRE( ( m >= 2 ) && ( m <= ( std::uint64_t( 1 ) << 32 ) ) && ( r >= 1 ) );
 SplitMix64 g( seed );
 f_a.resize( std::size_t( r ) );
 for( auto & d : f_a )
  d = g() % m;
 }

inline DotProductHash::DotProductHash( std::uint64_t m ,
                                       std::vector< std::uint64_t > a )
 : f_m( m ) , f_a( std::move( a ) )
{
 ADS_PRE( ( m >= 2 ) && ( m <= ( std::uint64_t( 1 ) << 32 ) ) &&
          ! f_a.empty() &&
          std::all_of( f_a.begin() , f_a.end() ,
                       [ m ]( std::uint64_t d ) { return( d < m ); } ) );
 }

inline std::uint64_t DotProductHash::universe( void ) const
{
 std::uint64_t u = 1;
 for( std::size_t i = 0 ; i < f_a.size() ; ++i ) {
  if( u > ~std::uint64_t( 0 ) / f_m ) return( 0 );   // past 2^64 - 1
  u *= f_m;
  }
 return( u );
 }

inline std::uint64_t DotProductHash::operator()( std::uint64_t k ) const
{
 ADS_PRE( ( universe() == 0 ) || ( k < universe() ) );
 std::uint64_t s = 0;
 for( const auto a : f_a ) {                // the digits, the least first
  s = ( s + mulMod( a , k % f_m , f_m ) ) % f_m;
  k /= f_m;
  }
 ADS_POST( s < f_m );
 return( s );
 }

/*--------------------------------------------------------------------------*/

inline MultiplyShiftHash::MultiplyShiftHash( int h , int l ,
                                             std::uint64_t seed )
 : f_h( h ) , f_l( l )
{
 ADS_PRE( ( l >= 1 ) && ( l < h ) && ( h <= 64 ) );
 f_mask = h == 64 ? ~std::uint64_t( 0 ) : ( std::uint64_t( 1 ) << h ) - 1;
 SplitMix64 g( seed );
 f_a = ( g() & f_mask ) | 1;
 }

inline MultiplyShiftHash MultiplyShiftHash::ofMultiplier( int h , int l ,
                                                         std::uint64_t a )
{
 MultiplyShiftHash f( h , l , std::uint64_t( 1 ) );
 ADS_PRE( ( a & 1 ) && ( a <= f.f_mask ) );
 f.f_a = a;
 return( f );
 }

inline std::uint64_t MultiplyShiftHash::operator()( std::uint64_t k ) const
{
 ADS_PRE( k <= f_mask );
 const std::uint64_t r = ( ( f_a * k ) & f_mask ) >> ( f_h - f_l );
 ADS_POST( r < ( std::uint64_t( 1 ) << f_l ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

inline StringHash::StringHash( std::uint64_t seed )
{
 SplitMix64 g( seed );
 f_x = 1 + g() % ( kMersenne61 - 1 );
 }

inline std::uint64_t StringHash::operator()( std::string_view s ) const
{
 std::uint64_t h = 0;
 for( auto i = s.size() ; i-- > 0 ; ) {     // Horner, from the last one
  h = mulMod61( h , f_x ) + std::uint64_t( ( unsigned char )( s[ i ] ) ) + 1;
  if( h >= kMersenne61 ) h -= kMersenne61;  // the character 0 counts as 1
  }
 ADS_POST( h < kMersenne61 );
 return( h );
 }

/*--------------------------------------------------------------------------*/

template< class K , class V , class H >
DLeftHashTable< K , V , H >::DLeftHashTable( std::size_t m , int d ,
                                             std::uint64_t seed , H h )
 : f_h( h )
{
 ADS_PRE( ( m >= 1 ) && ( d >= 1 ) );
 f_w = std::max< std::size_t >( 1 , m / std::size_t( d ) );
 f_T.resize( f_w * std::size_t( d ) );
 for( int i = 0 ; i < d ; ++i )
  f_g.emplace_back( splitmix64( seed ) + std::uint64_t( i ) );
 }

template< class K , class V , class H >
std::size_t DLeftHashTable< K , V , H >::list( const K & k , int i ) const
{
 return( std::size_t( i ) * f_w +
         std::size_t( f_g[ std::size_t( i ) ](
                       std::uint64_t( f_h( k ) ) ) % f_w ) );
 }

template< class K , class V , class H >
auto DLeftHashTable< K , V , H >::find( const K & k ) const
 -> std::optional< std::pair< std::size_t , std::size_t > >
{
 for( int i = 0 ; i < tables() ; ++i ) {
  const std::size_t b = list( k , i );
  for( std::size_t j = 0 ; j < f_T[ b ].size() ; ++j )
   if( f_T[ b ][ j ].first == k )
    return( std::pair( b , j ) );
  }
 return( std::nullopt );
 }

template< class K , class V , class H >
std::optional< V > DLeftHashTable< K , V , H >::lookup( const K & k ) const
{
 const auto f = find( k );
 return( f ? std::optional< V >( f_T[ f->first ][ f->second ].second )
           : std::nullopt );
 }

template< class K , class V , class H >
void DLeftHashTable< K , V , H >::insert( const K & k , const V & v )
{
 if( const auto f = find( k ) )
  f_T[ f->first ][ f->second ].second = v;
 else {
  std::size_t best = list( k , 0 );         // the shortest, the leftmost
  for( int i = 1 ; i < tables() ; ++i ) {
   const std::size_t b = list( k , i );
   if( f_T[ b ].size() < f_T[ best ].size() ) best = b;
   }
  f_T[ best ].emplace_back( k , v );
  ++f_n;
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( placed( k ) );
 }

template< class K , class V , class H >
void DLeftHashTable< K , V , H >::remove( const K & k )
{
 if( const auto f = find( k ) ) {
  auto & B = f_T[ f->first ];
  B[ f->second ] = std::move( B.back() );
  B.pop_back();
  --f_n;
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( placed( k ) );
 }

template< class K , class V , class H >
bool DLeftHashTable< K , V , H >::contains( const K & k ) const
{
 return( find( k ).has_value() );
 }

template< class K , class V , class H >
std::size_t DLeftHashTable< K , V , H >::size( void ) const
{
 return( f_n );
 }

template< class K , class V , class H >
std::size_t DLeftHashTable< K , V , H >::maxLoad( void ) const
{
 std::size_t l = 0;
 for( const auto & B : f_T )
  l = std::max( l , B.size() );
 return( l );
 }

template< class K , class V , class H >
int DLeftHashTable< K , V , H >::tables( void ) const
{
 return( int( f_g.size() ) );
 }

template< class K , class V , class H >
bool DLeftHashTable< K , V , H >::placed( const K & k ) const
{
 std::size_t c = 0;                         // k at most once in its lists
 for( int i = 0 ; i < tables() ; ++i )
  for( const auto & e : f_T[ list( k , i ) ] )
   c += e.first == k;
 if( c > 1 )
  return( false );
 if( f_n > 1024 )                           // a long sequence stays linear
  return( true );
 std::size_t n = 0;
 for( std::size_t b = 0 ; b < f_T.size() ; ++b )
  for( std::size_t j = 0 ; j < f_T[ b ].size() ; ++j ) {
   const auto f = find( f_T[ b ][ j ].first );
   if( ! f || ( f->first != b ) || ( f->second != j ) )
    return( false );
   ++n;
   }
 return( n == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File UniversalHashing.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
