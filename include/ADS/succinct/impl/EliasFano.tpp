/*--------------------------------------------------------------------------*/
/*--------------------------- File EliasFano.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of EliasFano.h, with their contracts.
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

template< bool Lazy >
inline BasicEliasFano< Lazy >::BasicEliasFano(
                                       const std::vector< std::uint64_t > & x )
 : BasicEliasFano( x , x.empty() ? 0 : x.back() + 1 )
{
 }

template< bool Lazy >
inline BasicEliasFano< Lazy >::BasicEliasFano(
                     const std::vector< std::uint64_t > & x , std::uint64_t u )
 : f_n( x.size() ) , f_u( u )
{
 ADS_AUDIT( std::is_sorted( x.begin() , x.end() ) );
 ADS_PRE( x.empty() || ( x.back() < u ) );
 if( f_n > 0 ) {                            // ell = ceil( log2( u / n ) )
  const std::uint64_t c = u / f_n + ( u % f_n ? 1 : 0 );
  f_l = c <= 1 ? 0 : lambda( c - 1 ) + 1;
  }
 BitVector H( f_n == 0 ? 0 : f_n + std::size_t( bucket( u - 1 ) ) + 1 );
 // L in a local, which the writes of the loop cannot alias, so that its
 // fields stay in registers when the constructor is not inlined
 BitVector L;
 L.reserve( f_n * std::size_t( f_l ) );
 const int l = f_l;
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  L.append( x[ i ] & detail::lowMask( l ) , l );
  H.set( std::size_t( l >= 64 ? 0 : x[ i ] >> l ) + i );
  }
 f_low = std::move( L );
 f_high = BasicRankSelect< Lazy >( std::move( H ) );
 ADS_AUDIT( decode() == x );
 }

template< bool Lazy >
inline std::uint64_t BasicEliasFano< Lazy >::bucket( std::uint64_t x ) const
{
 return( f_l >= 64 ? 0 : x >> f_l );
 }

template< bool Lazy >
inline std::size_t BasicEliasFano< Lazy >::size( void ) const
{
 return( f_n );
 }

template< bool Lazy >
inline bool BasicEliasFano< Lazy >::empty( void ) const
{
 return( f_n == 0 );
 }

template< bool Lazy >
inline std::uint64_t BasicEliasFano< Lazy >::universe( void ) const
{
 return( f_u );
 }

template< bool Lazy >
inline std::uint64_t BasicEliasFano< Lazy >::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 const std::uint64_t h = f_high.select1( i + 1 ) - i;
 const std::uint64_t l = f_low.get( i * std::size_t( f_l ) , f_l );
 return( f_l >= 64 ? l : ( h << f_l ) | l );
 }

template< bool Lazy >
inline std::uint64_t BasicEliasFano< Lazy >::operator[]( std::size_t i ) const
{
 return( access( i ) );
 }

template< bool Lazy >
inline std::size_t BasicEliasFano< Lazy >::nextGEQ( std::uint64_t x ) const
{
 std::size_t p = f_n;
 if( ( f_n > 0 ) && ( x < f_u ) ) {
  const std::uint64_t h = bucket( x );
  // the first item of the bucket of x and the place of its 1 in H
  std::size_t pos = h == 0 ? 0 : f_high.select0( std::size_t( h ) ) + 1;
  p = pos - std::size_t( h );
  // the items of the bucket share the high bits of x: their low bits
  // decide, and the first item of a later bucket is above x
  const std::uint64_t xl = x & detail::lowMask( f_l );
  auto low = [ & ]( std::size_t i ) {
   return( f_low.get( i * std::size_t( f_l ) , f_l ) ); };
  int t = 0;                                // a few items one by one
  for( ; ( t < 8 ) && ( pos < f_high.size() ) && f_high[ pos ] ;
       ++t , ++pos , ++p )
   if( low( p ) >= xl ) break;
  if( ( t == 8 ) && ( pos < f_high.size() ) && f_high[ pos ] ) {
   // a long bucket: its end by the 0 that closes it, then a binary search
   // on the low bits, sorted within the bucket
   std::size_t q = f_high.select0( std::size_t( h ) + 1 ) - std::size_t( h );
   while( p < q ) {
    const std::size_t mid = p + ( q - p ) / 2;
    if( low( mid ) < xl ) p = mid + 1;
    else q = mid;
    }
   }
  }
 ADS_POST( ( p == f_n ) || ( access( p ) >= x ) );
 ADS_POST( ( p == 0 ) || ( access( p - 1 ) < x ) );
 return( p );
 }

template< bool Lazy >
inline bool BasicEliasFano< Lazy >::contains( std::uint64_t x ) const
{
 const std::size_t p = nextGEQ( x );
 return( ( p < f_n ) && ( access( p ) == x ) );
 }

template< bool Lazy >
inline int BasicEliasFano< Lazy >::lowWidth( void ) const
{
 return( f_l );
 }

template< bool Lazy >
inline const BitVector & BasicEliasFano< Lazy >::low( void ) const
{
 return( f_low );
 }

template< bool Lazy >
inline const BasicRankSelect< Lazy > & BasicEliasFano< Lazy >::high( void )
 const
{
 return( f_high );
 }

template< bool Lazy >
inline std::size_t BasicEliasFano< Lazy >::codeBits( void ) const
{
 return( f_low.size() + f_high.size() );
 }

template< bool Lazy >
inline std::size_t BasicEliasFano< Lazy >::sizeInBits( void ) const
{
 return( codeBits() + f_high.directoryBits() );
 }

template< bool Lazy >
inline std::vector< std::uint64_t > BasicEliasFano< Lazy >::decode( void )
 const
{
 std::vector< std::uint64_t > x;
 x.reserve( f_n );
 std::uint64_t h = 0;                       // a scan of H, bucket by bucket
 for( std::size_t pos = 0 ; x.size() < f_n ; ++pos )
  if( f_high[ pos ] ) {
   const std::size_t i = x.size();
   const std::uint64_t l = f_low.get( i * std::size_t( f_l ) , f_l );
   x.push_back( f_l >= 64 ? l : ( h << f_l ) | l );
   }
  else ++h;
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File EliasFano.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
