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

/// the LSD sort of A by the unsigned key( x ), through B: a first sweep
/// finds the bytes in which some keys differ, and only those are counted
/// and make a pass
template< class T , class K >
void lsd( std::vector< T > & A , K & key )
{
 using U = std::decay_t< decltype( key( A[ 0 ] ) ) >;
 constexpr std::size_t W = sizeof( U );
 const std::size_t n = A.size();
 if( n < 2 )
  return;
 U any = 0 , all = U( ~U( 0 ) );            // the bits set in some key
 for( const auto & x : A ) {                // and those set in all
  const U k = key( x );
  any |= k;
  all &= k;
  }
 std::array< std::size_t , W > D;          // the bytes that differ
 std::size_t m = 0;
 for( std::size_t d = 0 ; d < W ; ++d )
  if( ( ( any ^ all ) >> ( 8 * d ) ) & 0xFF )
   D[ m++ ] = d;
 if( m == 0 )
  return;                                   // all the keys are equal
 std::array< std::array< std::size_t , 256 > , W > H{};
 if( m == W )                               // all the histograms at once
  for( const auto & x : A ) {
   const U k = key( x );
   for( std::size_t d = 0 ; d < W ; ++d )
    ++H[ d ][ std::size_t( ( k >> ( 8 * d ) ) & 0xFF ) ];
   }
 else
  for( const auto & x : A ) {
   const U k = key( x );
   for( std::size_t t = 0 ; t < m ; ++t )
    ++H[ t ][ std::size_t( ( k >> ( 8 * D[ t ] ) ) & 0xFF ) ];
   }
 std::vector< T > B( n );
 for( std::size_t t = 0 ; t < m ; ++t ) {
  const std::size_t d = D[ t ];
  std::size_t * const h = H[ t ].data();
  std::size_t sum = 0;                      // the starts of the buckets
  for( std::size_t c = 0 ; c < 256 ; ++c ) {
   const std::size_t t = h[ c ];
   h[ c ] = sum;
   sum += t;
   }
  T * const b = B.data();
  const int s = int( 8 * d );
  for( auto & x : A )
   b[ h[ std::size_t( ( key( x ) >> s ) & 0xFF ) ]++ ] = std::move( x );
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
inline std::uint16_t charAt( const std::string & s , std::size_t d )
{
 return( std::uint16_t( d < s.size() ? static_cast< unsigned char >( s[ d ] )
                                       + 1 : 0 ) );
 }

/// the MSD sort of the strings of S[ lo .. hi ) by their characters from d
/// on, through T, with the character d of each kept in K, so that it is
/// read once per pass; the strings of the part share their first d
/// characters. The strings go back and forth between S and T, one move per
/// pass, and end in the vector sorted, S if home and T otherwise
inline void msd( std::vector< std::string > & S ,
                 std::vector< std::string > & T ,
                 std::vector< std::uint16_t > & K , std::size_t lo ,
                 std::size_t hi , std::size_t d , bool home )
{
 std::string * const s = S.data() , * const t = T.data();
 if( hi - lo <= 16 ) {                      // the rest of the strings
  for( std::size_t i = lo + 1 ; i < hi ; ++i )
   for( std::size_t j = i ; ( j > lo ) &&
        ( s[ j ].compare( d , std::string::npos , s[ j - 1 ] , d ,
                          std::string::npos ) < 0 ) ; --j )
    std::swap( s[ j ] , s[ j - 1 ] );
  if( ! home )
   std::move( s + lo , s + hi , t + lo );
  return;
  }
 // the prefix that all the strings share from d, skipped with one
 // comparison per character: equal keys and long shared prefixes make no
 // pass of their own; a string that keeps the whole prefix costs one call
 // of memcmp(), and only one that cuts it is compared character by
 // character
 const char * f = s[ lo ].data() + d;
 const std::size_t n0 = s[ lo ].size();
 std::size_t l = n0 - d;
 bool same = true;                          // all as long as s[ lo ]
 std::size_t i = lo + 1;
 for( ; ( i < hi ) && ( l > 0 ) ; ++i ) {
  const char * x = s[ i ].data() + d;
  const std::size_t m = std::min( l , s[ i ].size() - d );
  if( ( m < l ) || ( std::memcmp( x , f , m ) != 0 ) )
   l = std::size_t( std::mismatch( x , x + m , f ).first - x );
  same = same && ( s[ i ].size() == n0 );
  }
 if( ( i == hi ) && same && ( d + l == n0 ) ) {   // all equal
  if( ! home )
   std::move( s + lo , s + hi , t + lo );
  return;
  }
 d += l;
 std::uint16_t * const k = K.data();
 std::array< std::size_t , 258 > c{};
 for( i = lo ; i < hi ; ++i ) {
  k[ i ] = charAt( s[ i ] , d );
  ++c[ k[ i ] + 1 ];
  }
 if( c[ 1 ] == hi - lo ) {                  // all equal, all ended
  if( ! home )
   std::move( s + lo , s + hi , t + lo );
  return;
  }
 for( std::size_t r = 0 ; r + 1 < c.size() ; ++r ) c[ r + 1 ] += c[ r ];
 for( i = lo ; i < hi ; ++i )
  t[ lo + c[ k[ i ] ]++ ] = std::move( s[ i ] );
 for( std::size_t r = 0 ; r < 257 ; ++r ) {   // bucket r: [ b , e )
  const std::size_t b = lo + ( r == 0 ? 0 : c[ r - 1 ] ) , e = lo + c[ r ];
  if( ( r == 0 ) || ( e - b < 2 ) ) {       // sorted: back home
   if( home )
    std::move( t + b , t + e , s + b );
   }
  else
   msd( T , S , K , b , e , d + 1 , ! home );
  }
 }

}  // end( namespace detail )

template< RadixKey T >
void radixSort( std::vector< T > & A )
{
 const auto before = detail::auditCopy( A );
 auto key = []( T x ) { return( radixKey( x ) ); };
 detail::lsd( A , key );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T a , T b ) {
  return( radixKey( a ) < radixKey( b ) ); } ) );
 }

template< class T , class K >
void radixSortBy( std::vector< T > & A , K key )
{
 using U = std::decay_t< decltype( key( A[ 0 ] ) ) >;
 static_assert( std::is_unsigned_v< U > , "the key must be unsigned" );
 detail::lsd( A , key );
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
 const std::size_t n = A.size();
 if( n > 1 ) {
  std::vector< std::string > B( n );
  std::vector< std::uint16_t > K( n );
  detail::msd( A , B , K , 0 , n , 0 , true );
  }
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
