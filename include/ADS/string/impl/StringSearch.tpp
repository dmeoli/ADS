/*--------------------------------------------------------------------------*/
/*------------------------- File StringSearch.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StringSearcher [see StringSearch.h], with their
 * contracts.
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

/// the first place i in [ i , m ) where P[ i ] != T[ i ], m if none,
/// 8 bytes at a time
inline long searchForward( const unsigned char * P , const unsigned char * T ,
                           long i , long m )
{
 for( ; i + 8 <= m ; i += 8 ) {
  std::uint64_t a , b;
  std::memcpy( &a , P + i , 8 );
  std::memcpy( &b , T + i , 8 );
  if( a != b ) {
   if constexpr( std::endian::native == std::endian::little )
    return( i + long( std::countr_zero( a ^ b ) / 8 ) );
   else
    return( i + long( std::countl_zero( a ^ b ) / 8 ) );
   }
  }
 while( ( i < m ) && ( P[ i ] == T[ i ] ) ) ++i;
 return( i );
 }

/// the last place i in ( low , i ] where P[ i ] != T[ i ], at most low
/// if none, 8 bytes at a time
inline long searchBackward( const unsigned char * P ,
                            const unsigned char * T , long i , long low )
{
 for( ; i - 8 >= low ; i -= 8 ) {
  std::uint64_t a , b;
  std::memcpy( &a , P + i - 7 , 8 );
  std::memcpy( &b , T + i - 7 , 8 );
  if( a != b ) {
   if constexpr( std::endian::native == std::endian::little )
    return( i - long( std::countl_zero( a ^ b ) / 8 ) );
   else
    return( i - long( std::countr_zero( a ^ b ) / 8 ) );
   }
  }
 while( ( i > low ) && ( P[ i ] == T[ i ] ) ) --i;
 return( i );
 }

/// the Two-Way algorithm of Crochemore and Perrin as detail::twoWayScan()
/// [see StringMatching.h], the two scans of each window 8 bytes at a time
template< class F >
std::size_t searchTwoWay( const unsigned char * T , std::size_t n ,
                          const unsigned char * P , std::size_t mm ,
                          std::size_t from , F && report )
{
 constexpr std::size_t npos = std::string_view::npos;
 const long m = long( mm );
 if( n < mm ) return( npos );
 const std::size_t last = n - mm;
 std::size_t p1 , p2;
 const long i1 = maximalSuffix( P , m , false , &p1 );
 const long i2 = maximalSuffix( P , m , true , &p2 );
 const long ell = std::max( i1 , i2 );
 std::size_t period = i1 > i2 ? p1 : p2;
 const bool periodic = std::memcmp( P , P + period ,
                                    std::size_t( ell + 1 ) ) == 0;
 if( ! periodic )
  period = std::size_t( std::max( ell + 1 , m - ell - 1 ) + 1 );
 long memory = -1;                    // P[ 0 .. memory ] known to match
 for( std::size_t j = from ; j <= last ; ) {
  const unsigned char * W = T + j;
  const long i = searchForward( P , W , std::max( ell , memory ) + 1 , m );
  if( i >= m ) {
   const long low = periodic ? memory : -1;
   if( ( searchBackward( P , W , ell , low ) <= low ) && report( j ) )
    return( j );
   j += period;
   if( periodic ) memory = m - long( period ) - 1;
   }
  else {
   j += std::size_t( i - ell );
   memory = -1;
   }
  }
 return( npos );
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
 // first, then distinct places; the last place of each byte (m if none),
 // and the number of bytes
 std::size_t last[ 256 ];
 std::fill( last , last + 256 , m );
 for( std::size_t i = 0 ; i < m ; ++i ) last[ p[ i ] ] = i;
 int distinct = 0;
 for( unsigned c = 0 ; c < 256 ; ++c ) distinct += int( last[ c ] < m );
 // 4 probes on the patterns of few bytes (a small alphabet, likely), 2
 // else: a pattern shorter than 8 with no repeated byte is no sign of one
 f_probes = ( m >= 4 ) && ( ( ( distinct < 8 ) && ( m >= 8 ) ) ||
                            ( std::size_t( distinct ) < m ) ) ? 4 : 2;
 // the distinct bytes: those of least rank, the later last place on a
 // tie, by one look at each of the 256 bytes
 int t = 0;
 for( unsigned c = 0 ; c < 256 ; ++c ) {
  if( last[ c ] == m ) continue;
  auto before = [ & ]( unsigned a , unsigned b ) {
   const unsigned ra = detail::searchRanks.r[ a ] ,
                  rb = detail::searchRanks.r[ b ];
   return( ( ra < rb ) || ( ( ra == rb ) && ( last[ a ] > last[ b ] ) ) );
   };
  int z = t < f_probes ? t++ : f_probes;
  if( ( z == f_probes ) && ! before( c , f_byte[ f_probes - 1 ] ) ) continue;
  if( z == f_probes ) --z;
  for( ; ( z > 0 ) && before( c , f_byte[ z - 1 ] ) ; --z )
   f_byte[ z ] = f_byte[ z - 1 ];
  f_byte[ z ] = (unsigned char)( c );
  }
 for( int q = 0 ; q < t ; ++q ) f_off[ q ] = last[ f_byte[ q ] ];
 // then distinct places, if fewer bytes than probes
 while( t < f_probes ) {
  std::size_t best = m;
  for( std::size_t i = m ; i-- > 0 ; ) {
   bool taken = false;
   for( int q = 0 ; q < t ; ++q ) taken = taken || ( f_off[ q ] == i );
   if( taken ) continue;
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
      ( std::memcmp( T + s , f_P.data() , m ) == 0 ) && report( s ) )
   return( s );
  if( ( work += m ) > 16 * ( s - from ) + 4096 )
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
 if( ( last - from < ( std::size_t( 1 ) << 14 ) ) || ( n < end + 8 ) )
  return( skip( T , n , from , report ) );
 const std::uint8_t * shift = f_shift.data();
 // the windows [ from , top ) in C segments, each scanned by its chain of
 // shifts, the C chains a step at a time so that their loads overlap;
 // the windows from top on, whose q-gram cannot be loaded as 8 bytes,
 // and those of a search gone wrong, by skip()
 constexpr int C = 4;
 const std::size_t top = std::min( last , n - end - 8 ) + 1;
 std::size_t s[ C ] , lim[ C ];
 for( int k = 0 ; k < C ; ++k )
  s[ k ] = from + ( top - from ) / C * std::size_t( k );
 for( int k = 0 ; k + 1 < C ; ++k ) lim[ k ] = s[ k + 1 ];
 lim[ C - 1 ] = top;
 // the occurrences of the chains after the first, reported when the
 // chains before them are done
 std::vector< std::size_t > later[ C ];
 std::size_t work = 0 , steps = 0 , mark = s[ 0 ];
 // the window of chain k whose q-gram has shift 0: false if the search
 // must go on otherwise (out then its result)
 auto check = [ & ]( int k , bool direct , std::size_t & out ) {
  const std::size_t w = s[ k ];
  if( ( T[ w + f_off[ 0 ] ] == f_byte[ 0 ] ) &&
      ( std::memcmp( T + w , f_P.data() , m ) == 0 ) ) {
   if( ! direct ) later[ k ].push_back( w );
   else if( report( w ) ) {
    out = w;
    return( false );
    }
   }
  std::size_t done = 0;
  for( int z = 0 ; z < C ; ++z )
   done += s[ z ] - ( from + ( top - from ) / C * std::size_t( z ) );
  if( ( work += m ) > 16 * done + 4096 ) {
   out = twoWay( T , n , direct ? w + 1 : s[ 0 ] , report );
   return( false );
   }
  s[ k ] = w + f_shift0;
  return( true );
  };
 std::size_t out;
 // all the chains at once, while none is done; every 64 steps the first
 // one must have moved by 16 m, or the filter does better
 for( bool on = true ; on ; ) {
  std::size_t x[ C ];
  for( int k = 0 ; k < C ; ++k ) x[ k ] = s[ k ];
  for( ; ; ) {
   bool in = true;
   for( int k = 0 ; k < C ; ++k ) in = in && ( x[ k ] < lim[ k ] );
   if( ! in ) {
    on = false;
    break;
    }
   if( ++steps == 64 ) {
    if( x[ 0 ] - mark < 16 * m ) return( skip( T , n , x[ 0 ] , report ) );
    steps = 0;
    mark = x[ 0 ];
    }
   std::size_t d[ C ];
   bool zero = false;
   for( int k = 0 ; k < C ; ++k ) {
    detail::searchPrefetch( T + std::min( x[ k ] + end + 1024 , n - 1 ) );
    d[ k ] = shift[ gram( T + x[ k ] + end ) ];
    zero = zero || ( d[ k ] == 0 );
    }
   if( zero ) break;
   for( int k = 0 ; k < C ; ++k ) x[ k ] += d[ k ];
   }
  for( int k = 0 ; k < C ; ++k ) s[ k ] = x[ k ];
  if( ! on ) break;
  for( int k = 0 ; k < C ; ++k )
   if( ( s[ k ] < lim[ k ] ) && ( shift[ gram( T + s[ k ] + end ) ] == 0 ) &&
       ! check( k , k == 0 , out ) )
    return( out );
  }
 // each chain to the end of its segment, in order, its occurrences found
 // so far reported first
 for( int k = 0 ; k < C ; ++k ) {
  for( const std::size_t w : later[ k ] )
   if( report( w ) ) return( w );
  later[ k ].clear();
  while( s[ k ] < lim[ k ] ) {
   const std::size_t d = shift[ gram( T + s[ k ] + end ) ];
   if( d ) {
    s[ k ] += d;
    continue;
    }
   if( ! check( k , true , out ) ) return( out );
   }
  }
 return( skip( T , n , top , report ) );
 }

template< class F >
std::size_t StringSearcher::twoWay( const unsigned char * T ,
                                    std::size_t n , std::size_t from ,
                                    F && report ) const
{
 const auto * p = reinterpret_cast< const unsigned char * >( f_P.data() );
 return( detail::searchTwoWay( T , n , p , f_P.size() , from , report ) );
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
  if( ( std::memcmp( T + s , f_P.data() , m ) == 0 ) && report( s ) )
   return( s );
  if( ( work += m ) > 16 * ( s - from ) + budget )
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
 // the starts from s whose bits are on in mask, which passed the probes:
 // true when the scan ends, at result
 auto verify = [ & ]( std::size_t s , std::uint64_t mask ) {
  for( ; mask ; mask &= mask - 1 ) {
   const std::size_t c = s + std::size_t( std::countr_zero( mask ) );
   if( ( std::memcmp( T + c , f_P.data() , m ) == 0 ) && report( c ) ) {
    result = c;
    return( true );
    }
   if( ( work += m ) > 16 * ( c - from ) + budget ) {
    result = twoWay( T , n , c + 1 , report );
    return( true );
    }
   }
  return( false );
  };
 // the block of the W starts from s, those of the bits off in mask
 // skipped
 auto probe = [ & ]( std::size_t s , std::uint64_t mask ) {
  for( int q = 0 ; q < k ; ++q ) mask &= SV::equal( Q[ q ] + s , B[ q ] );
  return( mask );
  };
 auto block = [ & ]( std::size_t s , std::uint64_t mask ) {
  return( verify( s , probe( s , mask ) ) );
  };
 constexpr std::uint64_t all = ~std::uint64_t( 0 ) >> ( 64 - W );
 std::size_t s = from;
 // 4 blocks a step, the verifications out of the way of the probes
 for( ; s + 4 * W - 1 <= last ; s += 4 * W ) {
  const std::uint64_t a = probe( s , all ) , b = probe( s + W , all ) ,
                      c = probe( s + 2 * W , all ) ,
                      d = probe( s + 3 * W , all );
  if( ( a | b | c | d ) == 0 ) [[likely]] continue;
  if( verify( s , a ) || verify( s + W , b ) || verify( s + 2 * W , c ) ||
      verify( s + 3 * W , d ) )
   return( result );
  }
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
