/*--------------------------------------------------------------------------*/
/*------------------------- File BloomVariants.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BloomVariants.h, with their contracts.
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

namespace ADS {

inline double bloomFalsePositive( double m , double n , int r )
{
 ADS_PRE( ( m >= 1 ) && ( n >= 0 ) && ( r >= 1 ) );
 if( n == 0 ) return( 0 );
 if( m == 1 ) return( 1 );
 const double zero = std::exp( double( r ) * n * std::log1p( -1 / m ) );
 return( std::pow( 1 - zero , double( r ) ) );
 }

inline int bloomBestHashes( double m , double n )
{
 ADS_PRE( ( m >= 1 ) && ( n >= 1 ) );
 const double x = std::min( m / n * std::log( 2.0 ) ,     // an int floor
                    double( std::numeric_limits< int >::max() - 1 ) );
 const int lo = std::max( 1 , int( std::floor( x ) ) ) , hi = lo + 1;
 return( bloomFalsePositive( m , n , hi ) < bloomFalsePositive( m , n , lo )
         ? hi : lo );
 }

inline double bloomLowerBound( double n , double epsilon )
{
 ADS_PRE( ( n >= 0 ) && ( epsilon > 0 ) && ( epsilon < 1 ) );
 return( n * std::log2( 1 / epsilon ) );
 }

inline double bloomBits( double n , double epsilon )
{
 return( bloomLowerBound( n , epsilon ) / std::log( 2.0 ) );
 }

inline std::pair< double , double > compressedBloom( double bitsPerKey ,
                                                     int r )
{
 ADS_PRE( ( bitsPerKey > 0 ) && ( r >= 1 ) );
 const double p = std::exp( - double( r ) / bitsPerKey );   // a bit is 0
 const double entropy = ( p <= 0 ) || ( p >= 1 ) ? 0 :
                        - p * std::log2( p ) - ( 1 - p ) * std::log2( 1 - p );
 return( std::pair( std::pow( 1 - p , double( r ) ) , bitsPerKey * entropy ) );
 }

template< class T , class H >
std::pair< std::vector< T > , std::vector< T > > bloomIntersection(
                                                 const std::vector< T > & A ,
                                                 const std::vector< T > & B ,
                                                 std::size_t mA )
{
 ADS_PRE( ( mA >= 1 ) && ! A.empty() );
 BloomFilter< T , H > F( mA , bloomBestHashes( double( mA ) ,
                                               double( A.size() ) ) );
 for( const auto & a : A ) F.insert( a );   // the machine of A sends F
 std::vector< T > Q , I;
 for( const auto & b : B )                  // the machine of B sends Q
  if( F.maybe( b ) ) Q.push_back( b );
 using Set = std::unordered_set< T , H >;
 const Set S( A.begin() , A.end() );
 for( const auto & q : Q )                  // the machine of A filters Q
  if( S.count( q ) ) I.push_back( q );
 ADS_POST( [ & ]( void ) {
  const Set R( I.begin() , I.end() );
  for( const auto & b : B )
   if( ( S.count( b ) > 0 ) != ( R.count( b ) > 0 ) ) return( false );
  return( true ); }() );
 return( std::pair( std::move( Q ) , std::move( I ) ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , class C , class H >
SpectralBloomFilter< T , C , H >::SpectralBloomFilter( std::size_t m , int r ,
                                                       H h )
 : f_C( m , 0 ) , f_r( r ) , f_h( h )
{
 ADS_PRE( ( m >= 1 ) && ( r >= 1 ) );
 }

template< class T , class C , class H >
std::vector< std::size_t > SpectralBloomFilter< T , C , H >::places(
                                                       const T & x ) const
{
 const std::uint64_t a = splitmix64( std::uint64_t( f_h( x ) ) ) ,
                     b = splitmix64( a ) | 1;
 std::vector< std::size_t > P( std::size_t( f_r ) , 0 );
 detail::BloomProbe Q( a , b , f_C.size() );
 for( int i = 0 ; i < f_r ; ++i )
  P[ std::size_t( i ) ] = Q.next();
 return( P );
 }

template< class T , class C , class H >
void SpectralBloomFilter< T , C , H >::insert( const T & x , C f )
{
 constexpr C top = std::numeric_limits< C >::max();
 for( const auto j : places( x ) )          // a counter at top sticks
  f_C[ j ] = top - f_C[ j ] < f ? top : C( f_C[ j ] + f );
 ADS_POST( ( count( x ) >= f ) && ( ( f == 0 ) || maybe( x ) ) );
 }

template< class T , class C , class H >
void SpectralBloomFilter< T , C , H >::remove( const T & x , C f )
{
 ADS_PRE( count( x ) >= f );
 constexpr C top = std::numeric_limits< C >::max();
 for( const auto j : places( x ) )
  if( f_C[ j ] != top )
   f_C[ j ] = C( f_C[ j ] - f );
 }

template< class T , class C , class H >
C SpectralBloomFilter< T , C , H >::count( const T & x ) const
{
 C c = std::numeric_limits< C >::max();
 for( const auto j : places( x ) )          // the minimum selection
  c = std::min( c , f_C[ j ] );
 return( c );
 }

template< class T , class C , class H >
bool SpectralBloomFilter< T , C , H >::maybe( const T & x ) const
{
 return( count( x ) > 0 );
 }

template< class T , class C , class H >
std::size_t SpectralBloomFilter< T , C , H >::stuck( void ) const
{
 return( std::size_t( std::count( f_C.begin() , f_C.end() ,
                                  std::numeric_limits< C >::max() ) ) );
 }

template< class T , class C , class H >
std::size_t SpectralBloomFilter< T , C , H >::counters( void ) const
{
 return( f_C.size() );
 }

template< class T , class C , class H >
int SpectralBloomFilter< T , C , H >::hashes( void ) const
{
 return( f_r );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
PartitionedBloomFilter< T , H >::PartitionedBloomFilter( std::size_t m ,
                                                         int r , H h )
 : f_r( r ) , f_h( h )
{
 ADS_PRE( ( r >= 1 ) && ( m >= std::size_t( r ) ) );
 f_w = m / std::size_t( r );
 f_B.assign( f_w * std::size_t( r ) , false );
 }

template< class T , class H >
detail::BloomProbe PartitionedBloomFilter< T , H >::probe( const T & x )
 const
{
 const std::uint64_t a = splitmix64( std::uint64_t( f_h( x ) ) ) ,
                     b = splitmix64( a ) | 1;
 return( detail::BloomProbe( a , b , f_w ) );
 }

template< class T , class H >
void PartitionedBloomFilter< T , H >::insert( const T & x )
{
 detail::BloomProbe P = probe( x );
 for( int i = 0 ; i < f_r ; ++i )
  f_B[ std::size_t( i ) * f_w + P.next() ] = true;
 ADS_POST( maybe( x ) );
 }

template< class T , class H >
bool PartitionedBloomFilter< T , H >::maybe( const T & x ) const
{
 detail::BloomProbe P = probe( x );
 for( int i = 0 ; i < f_r ; ++i )
  if( ! f_B[ std::size_t( i ) * f_w + P.next() ] )
   return( false );
 return( true );
 }

template< class T , class H >
std::size_t PartitionedBloomFilter< T , H >::ones( void ) const
{
 return( std::size_t( std::count( f_B.begin() , f_B.end() , true ) ) );
 }

template< class T , class H >
std::size_t PartitionedBloomFilter< T , H >::bits( void ) const
{
 return( f_B.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BloomVariants.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
