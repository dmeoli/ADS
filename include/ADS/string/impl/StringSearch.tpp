/*--------------------------------------------------------------------------*/
/*------------------------- File StringSearch.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StringSearcher [see StringSearch.h], with their
 * contracts.
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

#include <algorithm>
#include <bit>
#include <cstring>

namespace ADS {

namespace detail {

/// the rank of a byte by its frequency in text, larger when more common
constexpr unsigned char searchRankOf( unsigned char c )
{
 constexpr const char * lower = " etaoinsrhldcumfpgwybvkxjqz";
 constexpr const char * upper = "ETAOINSRHLDCUMFPGWYBVKXJQZ";
 if( ( c == ' ' ) || ( c == '\n' ) ) return( 255 );
 for( unsigned i = 0 ; lower[ i ] ; ++i )
  if( lower[ i ] == char( c ) ) return( (unsigned char)( 254 - i ) );
 for( unsigned i = 0 ; upper[ i ] ; ++i )
  if( upper[ i ] == char( c ) ) return( (unsigned char)( 180 - i ) );
 if( ( c >= '0' ) && ( c <= '9' ) ) return( 150 );
 if( ( c == ',' ) || ( c == '.' ) ) return( 220 );
 if( c == 0 ) return( 140 );
 if( ( c > 32 ) && ( c < 127 ) ) return( 120 );   // the other signs
 return( 20 );                       // the control and the high bytes
 }

/// the ranks of the 256 bytes
struct SearchRanks {
 unsigned char r[ 256 ];
 constexpr SearchRanks( void ) : r() {
  for( unsigned c = 0 ; c < 256 ; ++c ) r[ c ] = searchRankOf(
                                                        (unsigned char)( c ) );
  }
 };

inline constexpr SearchRanks searchRanks{};

/// prefetches the line of p for a read
inline void searchPrefetch( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p , 0 , 3 );
#else
 (void) p;
#endif
 }

/// the vector comparisons of W bytes: equal( p , b ) has bit i set if the
/// byte p[ i ] is b
template< int W >
struct SearchVector;

#if ADS_SEARCH_SIMD >= 1
template<>
struct SearchVector< 16 > {
 using V = __m128i;
 static V splat( unsigned char b ) { return( _mm_set1_epi8( char( b ) ) ); }
 static std::uint64_t equal( const unsigned char * p , V b ) {
  return( std::uint32_t( _mm_movemask_epi8( _mm_cmpeq_epi8(
           _mm_loadu_si128( reinterpret_cast< const V * >( p ) ) , b ) ) ) );
  }
 };
#endif

#if ADS_SEARCH_SIMD >= 2
template<>
struct SearchVector< 32 > {
 using V = __m256i;
 static V splat( unsigned char b ) {
  return( _mm256_set1_epi8( char( b ) ) );
  }
 static std::uint64_t equal( const unsigned char * p , V b ) {
  return( std::uint32_t( _mm256_movemask_epi8( _mm256_cmpeq_epi8(
           _mm256_loadu_si256( reinterpret_cast< const V * >( p ) ) ,
           b ) ) ) );
  }
 };
#endif

#if ADS_SEARCH_SIMD >= 3
template<>
struct SearchVector< 64 > {
 using V = __m512i;
 static V splat( unsigned char b ) {
  return( _mm512_set1_epi8( char( b ) ) );
  }
 static std::uint64_t equal( const unsigned char * p , V b ) {
  return( _mm512_cmpeq_epi8_mask( _mm512_loadu_si512( p ) , b ) );
  }
 };
#endif

/// the width of the vectors of the filter
inline constexpr int searchWidth = ADS_SEARCH_SIMD == 3 ? 64 :
                                   ADS_SEARCH_SIMD == 2 ? 32 :
                                   ADS_SEARCH_SIMD == 1 ? 16 : 0;

/// the maximal suffix of P under the order of the bytes (or the reverse
/// one if reverse): the place before it, and its period in *period
inline long maximalSuffix( const unsigned char * P , long m , bool reverse ,
                           std::size_t * period )
{
 long ms = -1 , j = 0 , k = 1 , p = 1;
 while( j + k < m ) {
  const unsigned char a = P[ j + k ] , b = P[ ms + k ];
  if( a == b ) {
   if( k != p ) ++k;
   else {
    j += p;
    k = 1;
    }
   }
  else if( ( a < b ) != reverse ) {   // the suffix at j + k is smaller
   j += k;
   k = 1;
   p = j - ms;
   }
  else {                               // a larger suffix starts at j
   ms = j;
   j = ms + 1;
   k = p = 1;
   }
  }
 *period = std::size_t( p );
 return( ms );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- StringSearcher ------------------------------*/
/*--------------------------------------------------------------------------*/

inline StringSearcher::StringSearcher( std::string_view P ) : f_P( P )
{
 const std::size_t m = f_P.size();
 const auto * p = reinterpret_cast< const unsigned char * >( f_P.data() );
 if( m < 2 ) return;

 // the probes: the rarest bytes, the later place on a tie, distinct bytes
 // first, then distinct places
 bool seen[ 256 ] = {};
 int distinct = 0;
 for( std::size_t i = 0 ; i < m ; ++i )
  if( ! seen[ p[ i ] ] ) {
   seen[ p[ i ] ] = true;
   ++distinct;
   }
 f_probes = ( distinct < 8 ) && ( m >= 4 ) ? 4 : 2;
 int t = 0;
 for( int pass = 0 ; ( pass < 2 ) && ( t < f_probes ) ; ++pass )
  while( t < f_probes ) {
   std::size_t best = m;
   for( std::size_t i = m ; i-- > 0 ; ) {
    bool fresh = true , taken = false;
    for( int q = 0 ; q < t ; ++q ) {
     fresh = fresh && ( f_byte[ q ] != p[ i ] );
     taken = taken || ( f_off[ q ] == i );
     }
    if( taken || ( ( pass == 0 ) && ! fresh ) ) continue;
    if( ( best == m ) || ( detail::searchRanks.r[ p[ i ] ] <
                           detail::searchRanks.r[ p[ best ] ] ) )
     best = i;
    }
   if( best == m ) break;
   f_off[ t ] = best;
   f_byte[ t ] = p[ best ];
   ++t;
   }
 for( ; t < f_probes ; ++t ) {        // m = 2 or 3 with 4 probes: repeat
  f_off[ t ] = f_off[ 0 ];
  f_byte[ t ] = f_byte[ 0 ];
  }

 // the shifts of the q-grams, q the least with distinct^q >= 16 m, at
 // most 8, each at most 255
 if( m < kSkip ) return;
 f_q = 2;
 for( double c = double( distinct ) * distinct ;
      ( c < 16.0 * double( m ) ) && ( f_q < 8 ) ; c *= distinct ) ++f_q;
 f_qmask = f_q == 8 ? ~std::uint64_t( 0 ) :
                      ( std::uint64_t( 1 ) << ( 8 * f_q ) ) - 1;
 const std::size_t none = std::min< std::size_t >( m - f_q + 1 , 255 );
 f_shift.fill( std::uint8_t( none ) );
 // P with room for the loads of 8 bytes, on the stack if short
 unsigned char small[ 512 + 8 ] = {};
 std::vector< unsigned char > large;
 unsigned char * q = small;
 if( m > 512 ) {
  large.assign( m + 8 , 0 );
  q = large.data();
  }
 std::memcpy( q , p , m );
 for( std::size_t i = 0 ; i + f_q < m ; ++i )
  f_shift[ gram( q + i ) ] = std::uint8_t( std::min< std::size_t >(
                                                 m - f_q - i , 255 ) );
 const std::size_t h = gram( q + m - f_q );
 f_shift0 = f_shift[ h ];
 f_shift[ h ] = 0;
 }

inline std::size_t StringSearcher::gram( const unsigned char * p ) const
{
 std::uint64_t x;
 std::memcpy( &x , p , 8 );
 if constexpr( std::endian::native == std::endian::big )
  x >>= 64 - 8 * f_q;                // the first q bytes, at the bottom
 return( std::size_t( ( ( x & f_qmask ) * 0x9E3779B97F4A7C15ull ) >> 52 ) );
 }

template< class F >
std::size_t StringSearcher::skip( const unsigned char * T , std::size_t n ,
                                  std::size_t from , F && report ) const
{
 const std::size_t m = f_P.size() , last = n - m , end = m - f_q;
 const std::uint8_t * shift = f_shift.data();
 std::size_t s = from , work = 0;
 // the windows whose q-gram at the end can be loaded as 8 bytes, while 64
 // steps shift by m / 4 on average at least; the filter does the rest
 std::size_t steps = 0 , mark = s;
 while( ( s <= last ) && ( s + end + 8 <= n ) ) {
  if( ++steps == 64 ) {
   if( s - mark < 16 * m ) break;
   steps = 0;
   mark = s;
   }
  if( s + end + 1024 < n )            // the text a few windows ahead
   detail::searchPrefetch( T + s + end + 1024 );
  const std::size_t d = shift[ gram( T + s + end ) ];
  if( d ) {
   s += d;
   continue;
   }
  if( ( T[ s + f_off[ 0 ] ] == f_byte[ 0 ] ) &&
      ( std::memcmp( T + s , f_P.data() , m ) == 0 ) ) {
   if( report( s ) ) return( s );
   }
  else if( ( work += m ) > 16 * ( s - from ) + 4096 )
   return( twoWay( T , n , s + 1 , report ) );
  s += f_shift0;
  }
 if( s > last ) return( npos );
 constexpr int W = detail::searchWidth;
 if constexpr( W == 0 )
  return( filterBytes( T , n , s , report ) );
 else
  return( f_probes == 4 ? filter< 4 , W >( T , n , s , report )
                        : filter< 2 , W >( T , n , s , report ) );
 }

template< class F >
std::size_t StringSearcher::skipPair( const unsigned char * T ,
                                      std::size_t n , std::size_t from ,
                                      F && report ) const
{
 const std::size_t m = f_P.size() , last = n - m , end = m - f_q;
 if( last - from < ( std::size_t( 1 ) << 14 ) )
  return( skip( T , n , from , report ) );
 const std::uint8_t * shift = f_shift.data();
 // the windows [ from , mid ) by a, those from mid on by b, whose
 // occurrences wait in later until a is done
 const std::size_t mid = from + ( last - from ) / 2;
 std::size_t a = from , b = mid , work = 0 , steps = 0 , ma = a , mb = b;
 bool alive = true;
 std::vector< std::size_t > later;
 // a step of the window s: false when the search must go on otherwise
 auto step = [ & ]( std::size_t & s , bool first , std::size_t & out ) {
  if( s + end + 1024 < n ) detail::searchPrefetch( T + s + end + 1024 );
  const std::size_t d = shift[ gram( T + s + end ) ];
  if( d ) {
   s += d;
   return( true );
   }
  if( ( T[ s + f_off[ 0 ] ] == f_byte[ 0 ] ) &&
      ( std::memcmp( T + s , f_P.data() , m ) == 0 ) ) {
   if( ! first ) later.push_back( s );
   else if( report( s ) ) {
    out = s;
    return( false );
    }
   }
  else if( ( work += m ) > 16 * ( ( a - from ) + ( b - mid ) ) + 4096 ) {
   out = twoWay( T , n , first ? s + 1 : a , report );
   return( false );
   }
  s += f_shift0;
  return( true );
  };
 std::size_t out;
 while( a < mid ) {
  if( ++steps == 64 ) {                // too short shifts: one window
   if( ( a - ma < 16 * m ) || ( alive && ( b - mb < 16 * m ) ) )
    return( skip( T , n , a , report ) );
   steps = 0;
   ma = a;
   mb = b;
   }
  if( ! step( a , true , out ) ) return( out );
  if( alive ) {
   if( ( b <= last ) && ( b + end + 8 <= n ) ) {
    if( ! step( b , false , out ) ) return( out );
    }
   else
    alive = false;
   }
  }
 for( const std::size_t s : later )
  if( report( s ) ) return( s );
 return( skip( T , n , b , report ) );
 }

template< class F >
std::size_t StringSearcher::twoWay( const unsigned char * T ,
                                    std::size_t n , std::size_t from ,
                                    F && report ) const
{
 const auto * P = reinterpret_cast< const unsigned char * >( f_P.data() );
 const long m = long( f_P.size() );
 if( n < std::size_t( m ) ) return( npos );
 const std::size_t last = n - std::size_t( m );
 // the critical factorization of P = u v, u = P[ 0 .. ell ], made here,
 // since most searches never come to the Two-Way algorithm
 std::size_t p1 , p2;
 const long i1 = detail::maximalSuffix( P , m , false , &p1 );
 const long i2 = detail::maximalSuffix( P , m , true , &p2 );
 const long ell = std::max( i1 , i2 );
 std::size_t period = i1 > i2 ? p1 : p2;
 const bool periodic = std::memcmp( P , P + period ,
                                    std::size_t( ell + 1 ) ) == 0;
 if( ! periodic )
  period = std::size_t( std::max( ell + 1 , m - ell - 1 ) + 1 );
 if( periodic ) {
  long memory = -1;               // P[ 0 .. memory ] known to match
  for( std::size_t j = from ; j <= last ; ) {
   long i = std::max( ell , memory ) + 1;
   while( ( i < m ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) ++i;
   if( i >= m ) {
    i = ell;
    while( ( i > memory ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) --i;
    if( ( i <= memory ) && report( j ) ) return( j );
    j += period;
    memory = m - long( period ) - 1;
    }
   else {
    j += std::size_t( i - ell );
    memory = -1;
    }
   }
  }
 else
  for( std::size_t j = from ; j <= last ; ) {
   long i = ell + 1;
   while( ( i < m ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) ++i;
   if( i >= m ) {
    i = ell;
    while( ( i >= 0 ) && ( P[ i ] == T[ std::size_t( i ) + j ] ) ) --i;
    if( ( i < 0 ) && report( j ) ) return( j );
    j += period;
    }
   else
    j += std::size_t( i - ell );
   }
 return( npos );
 }

template< class F >
std::size_t StringSearcher::filterBytes( const unsigned char * T ,
                                         std::size_t n , std::size_t from ,
                                         F && report ) const
{
 const std::size_t m = f_P.size() , last = n - m , o = f_off[ 0 ];
 std::size_t budget = 4096 , work = 0;
 for( std::size_t s = from ; s <= last ; ++s ) {
  const void * q = std::memchr( T + s + o , f_byte[ 0 ] , last - s + 1 );
  if( ! q ) return( npos );
  s = std::size_t( static_cast< const unsigned char * >( q ) - T ) - o;
  bool pass = true;
  for( int k = 1 ; k < f_probes ; ++k )
   pass = pass && ( T[ s + f_off[ k ] ] == f_byte[ k ] );
  if( ! pass ) continue;
  if( std::memcmp( T + s , f_P.data() , m ) == 0 ) {
   if( report( s ) ) return( s );
   }
  else if( ( work += m ) > 16 * ( s - from ) + budget )
   return( twoWay( T , n , s + 1 , report ) );
  }
 return( npos );
 }

template< int k , int W , class F >
std::size_t StringSearcher::filter( const unsigned char * T ,
                                    std::size_t n , std::size_t from ,
                                    F && report ) const
{
 using SV = detail::SearchVector< W >;
 const std::size_t m = f_P.size() , last = n - m;
 if( last - from + 1 < std::size_t( W ) )
  return( filterBytes( T , n , from , report ) );
 typename SV::V B[ k ];
 const unsigned char * Q[ k ];
 for( int q = 0 ; q < k ; ++q ) {
  B[ q ] = SV::splat( f_byte[ q ] );
  Q[ q ] = T + f_off[ q ];
  }
 const std::size_t budget = 4096;
 std::size_t work = 0 , result = npos;
 // the block of the W starts from s, those of the bits off in mask
 // skipped: true when the scan ends, at result
 auto block = [ & ]( std::size_t s , std::uint64_t mask ) {
  for( int q = 0 ; q < k ; ++q ) mask &= SV::equal( Q[ q ] + s , B[ q ] );
  for( ; mask ; mask &= mask - 1 ) {
   const std::size_t c = s + std::size_t( std::countr_zero( mask ) );
   if( std::memcmp( T + c , f_P.data() , m ) == 0 ) {
    if( report( c ) ) {
     result = c;
     return( true );
     }
    }
   else if( ( work += m ) > 16 * ( c - from ) + budget ) {
    result = twoWay( T , n , c + 1 , report );
    return( true );
    }
   }
  return( false );
  };
 constexpr std::uint64_t all = ~std::uint64_t( 0 ) >> ( 64 - W );
 std::size_t s = from;
 for( ; s + W - 1 <= last ; s += W )
  if( block( s , all ) ) return( result );
 if( s <= last ) {                   // the last W starts, overlapping
  const std::size_t b = last - W + 1;
  if( block( b , all << ( s - b ) ) ) return( result );
  }
 return( npos );
 }

template< class F >
std::size_t StringSearcher::scan( std::string_view T , std::size_t from ,
                                  F && report ) const
{
 const std::size_t n = T.size() , m = f_P.size();
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 if( ( from > n ) || ( n - from < m ) ) return( npos );
 if( m == 0 ) {
  for( std::size_t s = from ; s <= n ; ++s )
   if( report( s ) ) return( s );
  return( npos );
  }
 if( m == 1 ) {
  for( std::size_t s = from ; s < n ; ++s ) {
   const void * q = std::memchr( t + s , f_P[ 0 ] , n - s );
   if( ! q ) return( npos );
   s = std::size_t( static_cast< const unsigned char * >( q ) - t );
   if( report( s ) ) return( s );
   }
  return( npos );
  }
 if( f_q > 0 ) return( skipPair( t , n , from , report ) );
 constexpr int W = detail::searchWidth;
 if constexpr( W == 0 )
  return( filterBytes( t , n , from , report ) );
 else
  return( f_probes == 4 ? filter< 4 , W >( t , n , from , report )
                        : filter< 2 , W >( t , n , from , report ) );
 }

inline std::size_t StringSearcher::find( std::string_view T ,
                                         std::size_t from ) const
{
 const std::size_t s = scan( T , from , []( std::size_t ) {
  return( true ); } );
 ADS_POST( ( s == npos ) || ( ( s >= from ) && ( s + f_P.size() <=
                              T.size() ) && ( T.substr( s , f_P.size() ) ==
                                              f_P ) ) );
 return( s );
 }

inline std::vector< std::size_t > StringSearcher::findAll(
                                                  std::string_view T ) const
{
 std::vector< std::size_t > R;
 (void) scan( T , 0 , [ & ]( std::size_t s ) {
  R.push_back( s );
  return( false ); } );
 ADS_AUDIT( R == naiveMatching( T , f_P ) );
 return( R );
 }

inline std::size_t StringSearcher::count( std::string_view T ) const
{
 std::size_t c = 0;
 (void) scan( T , 0 , [ & ]( std::size_t ) {
  ++c;
  return( false ); } );
 return( c );
 }

/*--------------------------------------------------------------------------*/

inline std::size_t findFirst( std::string_view T , std::string_view P ,
                              std::size_t from )
{
 return( StringSearcher( P ).find( T , from ) );
 }

inline std::vector< std::size_t > findAll( std::string_view T ,
                                           std::string_view P )
{
 return( StringSearcher( P ).findAll( T ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File StringSearch.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
