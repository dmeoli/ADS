/*--------------------------------------------------------------------------*/
/*------------------------- File GrafiteFilter.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of GrafiteFilter and BucketingFilter [see
 * GrafiteFilter.h], with their contracts.
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

namespace detail {

/// the word of the key x, in the order of the keys
template< RadixKey T >
std::uint64_t rangeWord( const T & x )
{
 return( std::uint64_t( radixKey( x ) ) );
 }

/// the words of the keys, sorted and once each
template< RadixKey T , class F >
std::vector< std::uint64_t > rangeWords( const std::vector< T > & keys ,
                                         F && f )
{
 std::vector< std::uint64_t > W;
 W.reserve( keys.size() );
 for( const T & k : keys )
  W.push_back( f( rangeWord( k ) ) );
 radixSort( W );
 W.erase( std::unique( W.begin() , W.end() ) , W.end() );
 return( W );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS GrafiteFilter ---------------------------*/
/*--------------------------------------------------------------------------*/

template< RadixKey T >
GrafiteFilter< T >::GrafiteFilter( const std::vector< T > & keys , int B ,
                                   std::uint64_t seed )
 : f_B( B )
{
 ADS_PRE( ( B >= 2 ) && ( B <= 40 ) );
 ADS_PRE( std::bit_width( std::uint64_t( keys.size() ) ) + B - 2 <= 63 );
 f_alo = hashWord( seed , 1 );
 f_ahi = hashWord( seed , 2 );
 f_blo = hashWord( seed , 3 );
 f_bhi = hashWord( seed , 4 );
 {
  std::vector< std::uint64_t > K;           // the distinct keys
  K.reserve( keys.size() );
  for( const T & k : keys )
   K.push_back( detail::rangeWord( k ) );
  radixSort( K );
  K.erase( std::unique( K.begin() , K.end() ) , K.end() );
  f_n = K.size();
 }
 f_r = std::max< std::uint64_t >( 1 , std::uint64_t( f_n ) << ( B - 2 ) );
 const auto C = detail::rangeWords( keys , [ this ]( std::uint64_t x ) {
  return( code( x ) ); } );
 f_codes = EliasFano( C , f_r );
 f_first = C.empty() ? 0 : C.front();
 ADS_AUDIT( std::all_of( keys.begin() , keys.end() ,
                         [ this ]( const T & k ) { return( maybe( k ) ); } ) );
 }

/*--------------------------------------------------------------------------*/

template< RadixKey T >
std::uint64_t GrafiteFilter< T >::code( std::uint64_t x ) const
{
 // q'( j ), the high word of alpha j + beta modulo 2^128, onto [ 0 , r )
 const std::uint64_t j = x / f_r;
 const Product128 p = product128( f_alo , j );
 const std::uint64_t lo = p.lo + f_blo;
 const std::uint64_t hi = p.hi + f_ahi * j + f_bhi +
                          std::uint64_t( lo < f_blo );
 const std::uint64_t q = product128( hi , f_r ).hi;
 const std::uint64_t c = q + x % f_r;      // < 2 r < 2^64
 return( c >= f_r ? c - f_r : c );
 }

template< RadixKey T >
bool GrafiteFilter< T >::meets( std::uint64_t x , std::uint64_t y ) const
{
 const std::size_t p = f_codes.nextGEQ( x );
 if( x <= y )
  return( ( p < f_codes.size() ) && ( f_codes.access( p ) <= y ) );
 return( ( p < f_codes.size() ) || ( f_first <= y ) );
 }

/*--------------------------------------------------------------------------*/

template< RadixKey T >
bool GrafiteFilter< T >::mayContain( const T & a , const T & b ) const
{
 const std::uint64_t x = detail::rangeWord( a ) ,
                     y = detail::rangeWord( b );
 ADS_PRE( x <= y );
 if( f_n == 0 )
  return( false );
 if( y - x >= f_r )                         // the bound is 1
  return( true );
 const std::uint64_t ja = x / f_r , jb = y / f_r;
 if( ja == jb )
  return( meets( code( x ) , code( y ) ) );
 const std::uint64_t c = jb * f_r;          // the first key of y's block
 return( meets( code( x ) , code( c - 1 ) ) ||
         meets( code( c ) , code( y ) ) );
 }

template< RadixKey T >
bool GrafiteFilter< T >::maybe( const T & x ) const
{
 return( ( f_n > 0 ) && f_codes.contains( code( detail::rangeWord( x ) ) ) );
 }

template< RadixKey T >
double GrafiteFilter< T >::falsePositiveBound( double l ) const
{
 return( std::min( 1.0 , l / double( std::uint64_t( 1 ) << ( f_B - 2 ) ) ) );
 }

template< RadixKey T >
std::size_t GrafiteFilter< T >::size( void ) const
{
 return( f_n );
 }

template< RadixKey T >
bool GrafiteFilter< T >::empty( void ) const
{
 return( f_n == 0 );
 }

template< RadixKey T >
std::size_t GrafiteFilter< T >::codes( void ) const
{
 return( f_codes.size() );
 }

template< RadixKey T >
std::uint64_t GrafiteFilter< T >::reduced( void ) const
{
 return( f_r );
 }

template< RadixKey T >
std::size_t GrafiteFilter< T >::sizeInBits( void ) const
{
 return( f_codes.sizeInBits() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS BucketingFilter --------------------------*/
/*--------------------------------------------------------------------------*/

template< RadixKey T >
BucketingFilter< T >::BucketingFilter( const std::vector< T > & keys ,
                                       int B )
{
 ADS_PRE( ( B >= 2 ) && ( B <= 40 ) );
 ADS_PRE( std::bit_width( std::uint64_t( keys.size() ) ) + B - 2 <= 63 );
 if( keys.empty() )
  return;
 // s = ceil( 2^64 / ( n 2^( B - 2 ) ) ), for the n distinct keys
 std::vector< std::uint64_t > K;
 K.reserve( keys.size() );
 for( const T & k : keys )
  K.push_back( detail::rangeWord( k ) );
 radixSort( K );
 K.erase( std::unique( K.begin() , K.end() ) , K.end() );
 const std::uint64_t d = std::uint64_t( K.size() ) << ( B - 2 );
 f_s = ~std::uint64_t( 0 ) / d + 1;
 for( auto & x : K )
  x /= f_s;
 K.erase( std::unique( K.begin() , K.end() ) , K.end() );
 f_buckets = EliasFano( K , ~std::uint64_t( 0 ) / f_s + 1 );
 ADS_AUDIT( std::all_of( keys.begin() , keys.end() ,
                         [ this ]( const T & k ) { return( maybe( k ) ); } ) );
 }

template< RadixKey T >
bool BucketingFilter< T >::mayContain( const T & a , const T & b ) const
{
 const std::uint64_t x = detail::rangeWord( a ) ,
                     y = detail::rangeWord( b );
 ADS_PRE( x <= y );
 const std::size_t p = f_buckets.nextGEQ( x / f_s );
 return( ( p < f_buckets.size() ) && ( f_buckets.access( p ) <= y / f_s ) );
 }

template< RadixKey T >
bool BucketingFilter< T >::maybe( const T & x ) const
{
 return( f_buckets.contains( detail::rangeWord( x ) / f_s ) );
 }

template< RadixKey T >
std::uint64_t BucketingFilter< T >::bucketWidth( void ) const
{
 return( f_s );
 }

template< RadixKey T >
std::size_t BucketingFilter< T >::buckets( void ) const
{
 return( f_buckets.size() );
 }

template< RadixKey T >
std::size_t BucketingFilter< T >::sizeInBits( void ) const
{
 return( f_buckets.sizeInBits() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File GrafiteFilter.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
