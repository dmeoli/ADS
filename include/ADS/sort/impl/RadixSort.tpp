/*--------------------------------------------------------------------------*/
/*--------------------------- File RadixSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of RadixSort.h, with their contracts.
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

template< RadixKey T >
auto radixKey( T x )
{
 using U = std::conditional_t< sizeof( T ) == 1 , std::uint8_t ,
           std::conditional_t< sizeof( T ) == 2 , std::uint16_t ,
           std::conditional_t< sizeof( T ) <= 4 , std::uint32_t ,
                                                  std::uint64_t > > >;
 constexpr U sign = U( U( 1 ) << ( 8 * sizeof( T ) - 1 ) );
 if constexpr( std::is_floating_point_v< T > ) {
  const U u = std::bit_cast< U >( x );
  return( U( ( u & sign ) ? ~u : ( u | sign ) ) );
  }
 else if constexpr( std::is_signed_v< T > )
  return( U( U( x ) ^ sign ) );
 else
  return( U( x ) );
 }

namespace detail {

/// the LSD sort of A by the unsigned key( x ) of w bytes, through B
template< class T , class K >
void lsd( std::vector< T > & A , K & key , std::size_t w )
{
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 std::vector< std::array< std::size_t , 256 > > H( w );
 for( auto & h : H ) h.fill( 0 );
 for( const auto & x : A ) {                // all the histograms at once
  auto k = std::uint64_t( key( x ) );      // wide, so that >> 8 is defined
  for( std::size_t d = 0 ; d < w ; ++d , k >>= 8 )
   ++H[ d ][ std::size_t( k & 0xFF ) ];
  }
 std::vector< T > B( n );
 for( std::size_t d = 0 ; d < w ; ++d ) {
  auto & h = H[ d ];
  if( std::find( h.begin() , h.end() , n ) != h.end() )
   continue;                                // one bucket: nothing to do
  std::size_t sum = 0;                      // the starts of the buckets
  for( auto & c : h ) {
   const std::size_t t = c;
   c = sum;
   sum += t;
   }
  for( auto & x : A )
   B[ h[ std::size_t( ( key( x ) >> ( 8 * d ) ) & 0xFF ) ]++ ] =
    std::move( x );
  A.swap( B );
  }
 }

/// the MSD sort in place of A[ lo .. hi ) by the byte d of radixKey(),
/// counted from the least significant
template< RadixKey T >
void flag( std::vector< T > & A , std::size_t lo , std::size_t hi , int d )
{
 if( hi - lo <= 32 ) {
  insertionSort( A , lo , hi , []( T a , T b ) {
   return( radixKey( a ) < radixKey( b ) ); } );
  return;
  }
 auto byte = [ d ]( T x ) {
  return( std::size_t( ( radixKey( x ) >> ( 8 * d ) ) & 0xFF ) ); };
 std::array< std::size_t , 256 > count{} , start , next;
 for( std::size_t i = lo ; i < hi ; ++i ) ++count[ byte( A[ i ] ) ];
 std::size_t sum = lo;
 for( std::size_t b = 0 ; b < 256 ; ++b ) {
  start[ b ] = next[ b ] = sum;
  sum += count[ b ];
  }
 for( std::size_t b = 0 ; b < 256 ; ++b )   // the cycles into the stripes
  while( next[ b ] < start[ b ] + count[ b ] ) {
   std::size_t c = byte( A[ next[ b ] ] );
   while( c != b ) {
    std::swap( A[ next[ b ] ] , A[ next[ c ]++ ] );
    c = byte( A[ next[ b ] ] );
    }
   ++next[ b ];
   }
 if( d > 0 )
  for( std::size_t b = 0 ; b < 256 ; ++b )
   if( count[ b ] > 1 )
    flag( A , start[ b ] , start[ b ] + count[ b ] , d - 1 );
 }

/// the character d of s, 0 at its end, then the characters plus 1
inline int charAt( const std::string & s , std::size_t d )
{
 return( d < s.size() ? int( static_cast< unsigned char >( s[ d ] ) ) + 1
                      : 0 );
 }

/// the MSD sort of A[ lo .. hi ) by the characters from d on, through B
inline void msd( std::vector< std::string > & A ,
                 std::vector< std::string > & B , std::size_t lo ,
                 std::size_t hi , std::size_t d )
{
 if( hi - lo <= 16 ) {                      // the rest of the strings
  for( std::size_t i = lo + 1 ; i < hi ; ++i )
   for( std::size_t j = i ; ( j > lo ) &&
        ( A[ j ].compare( d , std::string::npos , A[ j - 1 ] , d ,
                          std::string::npos ) < 0 ) ; --j )
    std::swap( A[ j ] , A[ j - 1 ] );
  return;
  }
 std::array< std::size_t , 258 > c{};
 for( std::size_t i = lo ; i < hi ; ++i ) ++c[ charAt( A[ i ] , d ) + 1 ];
 for( std::size_t r = 0 ; r + 1 < c.size() ; ++r ) c[ r + 1 ] += c[ r ];
 for( std::size_t i = lo ; i < hi ; ++i )
  B[ lo + c[ charAt( A[ i ] , d ) ]++ ] = std::move( A[ i ] );
 for( std::size_t i = lo ; i < hi ; ++i ) A[ i ] = std::move( B[ i ] );
 for( std::size_t r = 1 ; r < 257 ; ++r )  // the ends ( r = 0 ) are done
  if( c[ r ] - c[ r - 1 ] > 1 )
   msd( A , B , lo + c[ r - 1 ] , lo + c[ r ] , d + 1 );
 }

}  // end( namespace detail )

template< RadixKey T >
void radixSort( std::vector< T > & A )
{
 const auto before = detail::auditCopy( A );
 auto key = []( T x ) { return( radixKey( x ) ); };
 detail::lsd( A , key , sizeof( T ) );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T a , T b ) {
  return( radixKey( a ) < radixKey( b ) ); } ) );
 }

template< class T , class K >
void radixSortBy( std::vector< T > & A , K key )
{
 using U = std::decay_t< decltype( key( A[ 0 ] ) ) >;
 static_assert( std::is_unsigned_v< U > , "the key must be unsigned" );
 detail::lsd( A , key , sizeof( U ) );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  for( std::size_t i = 1 ; i < A.size() ; ++i )
   if( key( A[ i ] ) < key( A[ i - 1 ] ) ) return( false );
  return( true );
  } ) );
 }

template< RadixKey T >
void americanFlagSort( std::vector< T > & A )
{
 const auto before = detail::auditCopy( A );
 if( A.size() > 1 )
  detail::flag( A , 0 , A.size() , int( sizeof( T ) ) - 1 );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T a , T b ) {
  return( radixKey( a ) < radixKey( b ) ); } ) );
 }

inline void msdRadixSort( std::vector< std::string > & A )
{
 const auto before = detail::auditCopy( A );
 std::vector< std::string > B( A.size() );
 if( A.size() > 1 )
  detail::msd( A , B , 0 , A.size() , 0 );
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

template< std::floating_point T >
void bucketSort( std::vector< T > & A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() , []( T x ) {
  return( ( x >= 0 ) && ( x < 1 ) ); } ) );
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 std::vector< std::vector< T > > B( n );
 for( T x : A )
  B[ std::size_t( x * T( n ) ) ].push_back( x );
 std::size_t i = 0;
 for( auto & b : B ) {
  insertionSort( b );
  for( T x : b ) A[ i++ ] = x;
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RadixSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
