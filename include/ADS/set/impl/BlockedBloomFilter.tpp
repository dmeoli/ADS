/*--------------------------------------------------------------------------*/
/*---------------------- File BlockedBloomFilter.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BlockedBloomFilter [see BlockedBloomFilter.h], with
 * their contracts.
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

/// the 8 odd constants of the bits of a block: the high halves of the
/// first 8 words of SplitMix64 from 0, made odd
alignas( 32 ) inline constexpr std::uint32_t kBlockSalt[ 8 ] = {
 0xE220A839U , 0x6E789E6BU , 0x06C45D19U , 0xF88BB8A9U ,
 0x1B39896BU , 0x53CB9F0DU , 0x2C829ABFU , 0xC584133BU
 };

/// the 8 words of the bits of the low half h of a hash, one bit per word
struct BlockMask
{
 std::uint32_t m[ 8 ];

#if ADS_SIMD
 // the 8 words at once [see SimdInline.h]
 explicit BlockMask( std::uint32_t h ) {
  simd::here::blockBits8( h , kBlockSalt , m );
  }

 /// true if all the bits are in the 8 words from w
 bool in( const std::uint32_t * w ) const {
  return( simd::here::blockHas8( w , m ) );
  }

 /// sets the bits in the 8 words from w
 void set( std::uint32_t * w ) const { simd::here::blockSet8( w , m ); }
#else
 explicit BlockMask( std::uint32_t h ) {
  for( int i = 0 ; i < 8 ; ++i )
   m[ i ] = std::uint32_t( 1 ) << ( ( h * kBlockSalt[ i ] ) >> 27 );
  }

 bool in( const std::uint32_t * w ) const {
  std::uint32_t miss = 0;
  for( int i = 0 ; i < 8 ; ++i )
   miss |= m[ i ] & ~w[ i ];
  return( miss == 0 );
  }

 void set( std::uint32_t * w ) const {
  for( int i = 0 ; i < 8 ; ++i )
   w[ i ] |= m[ i ];
  }
#endif
 };

/// the hash of x by h, mixed by hashWord() [see FastHash.h] unless H
/// declares it avalanching: the hash of the filters on blocks
template< class H , class T >
inline std::uint64_t filterHash( const H & h , const T & x )
{
 if constexpr( detail::kAvalanching64< H , T > )
  return( detail::hash64( h , x ) );
 else
  return( hashWord( detail::hash64( h , x ) ) );
 }

/// the block of the hash h among n, by its high 32 bits, a product and a
/// shift (the reduction of Lemire, with no division)
inline std::size_t highRange( std::uint64_t h , std::size_t n )
{
 return( std::size_t( ( ( h >> 32 ) * std::uint64_t( n ) ) >> 32 ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , class H >
BlockedBloomFilter< T , H >::BlockedBloomFilter( std::size_t n ,
						double bitsPerItem , H h )
    : f_h( h )
{
 ADS_PRE( bitsPerItem > 0 );
 const double b = std::ceil( double( n ) * bitsPerItem / 256 );
 f_B.assign( std::size_t( std::max( 1.0 , b ) ) , Block{} );
 }

template< class T , class H >
std::uint64_t BlockedBloomFilter< T , H >::hash( const T & x ) const
{
 return( detail::filterHash( f_h , x ) );
 }

template< class T , class H >
std::size_t BlockedBloomFilter< T , H >::block( std::uint64_t h ) const
{
 return( detail::highRange( h , f_B.size() ) );
 }

template< class T , class H >
void BlockedBloomFilter< T , H >::insertHash( std::uint64_t h )
{
 detail::BlockMask( std::uint32_t( h ) ).set( f_B[ block( h ) ].w );
 ++f_n;
 }

template< class T , class H >
bool BlockedBloomFilter< T , H >::maybeHash( std::uint64_t h ) const
{
 return( detail::BlockMask( std::uint32_t( h ) ).in( f_B[ block( h ) ].w ) );
 }

template< class T , class H >
void BlockedBloomFilter< T , H >::insert( const T & x )
{
 insertHash( hash( x ) );
 ADS_POST( maybe( x ) );
 }

template< class T , class H >
bool BlockedBloomFilter< T , H >::maybe( const T & x ) const
{
 return( maybeHash( hash( x ) ) );
 }

template< class T , class H >
void BlockedBloomFilter< T , H >::unite( const BlockedBloomFilter & o )
{
 ADS_PRE( o.f_B.size() == f_B.size() );
 for( std::size_t b = 0 ; b < f_B.size() ; ++b )
  for( int i = 0 ; i < 8 ; ++i )
   f_B[ b ].w[ i ] |= o.f_B[ b ].w[ i ];
 f_n += o.f_n;
 }

template< class T , class H >
void BlockedBloomFilter< T , H >::clear( void )
{
 std::fill( f_B.begin() , f_B.end() , Block{} );
 f_n = 0;
 }

template< class T , class H >
std::size_t BlockedBloomFilter< T , H >::size( void ) const
{
 return( f_n );
 }

template< class T , class H >
bool BlockedBloomFilter< T , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , class H >
std::size_t BlockedBloomFilter< T , H >::bits( void ) const
{
 return( 256 * f_B.size() );
 }

template< class T , class H >
std::size_t BlockedBloomFilter< T , H >::blocks( void ) const
{
 return( f_B.size() );
 }

template< class T , class H >
double BlockedBloomFilter< T , H >::falsePositiveRate( void ) const
{
 // the mean over j of Poisson( j ; lambda ) ( 1 - ( 31 / 32 )^j )^8, the
 // terms by the recurrence of the Poisson probabilities from the mode
 const double lambda = double( f_n ) / double( f_B.size() );
 if( lambda == 0 )
  return( 0 );
 auto term = []( double j ) {
  return( std::pow( 1 - std::pow( 31.0 / 32 , j ) , 8 ) );
  };
 const double j0 = std::floor( lambda );
 const double p0 =
   std::exp( j0 * std::log( lambda ) - lambda - std::lgamma( j0 + 1 ) );
 double f = p0 * term( j0 );
 double p = p0;
 for( double j = j0 + 1 ; p > 1e-18 * p0 || j < j0 + 8 ; ++j ) {
  p *= lambda / j;
  f += p * term( j );
  }
 p = p0;
 for( double j = j0 ; j > 0 && ( p > 1e-18 * p0 || j > j0 - 8 ) ; --j ) {
  p *= j / lambda;
  f += p * term( j - 1 );
  }
 return( f );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
template< class A >
inline void BlockedBloomFilter< T , H >::persist( A & a )
{
 a( f_B , f_n );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( ! f_B.empty() , "the blocks of a BlockedBloomFilter" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File BlockedBloomFilter.tpp --------------------*/
/*--------------------------------------------------------------------------*/
