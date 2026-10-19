/*--------------------------------------------------------------------------*/
/*------------------------- File SuffixSorting.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuffixSorting [see SuffixSorting.h], with their
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
#include <limits>
#include <memory>
#include <type_traits>

namespace ADS {

namespace detail {

/// how far ahead the scans of the induction prefetch the text; on the
/// large alphabets the buckets are prefetched half as far and the places
/// they point to a quarter
inline constexpr std::ptrdiff_t saisAhead = 64;

/// the alphabets of at most saisSmall symbols keep the bucket of the last
/// symbol in a register; on the larger ones the buckets and the places
/// they point to are prefetched
inline constexpr std::ptrdiff_t saisSmall = 1024;

/// prefetches the line of p for a read
inline void saisPrefetch( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p , 0 , 3 );
#else
 (void) p;
#endif
 }

/// prefetches the line of p for a write
inline void saisPrefetchW( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p , 1 , 3 );
#else
 (void) p;
#endif
 }

/// the first (heads) or the next past the last (tails) place of each of
/// the K buckets, from their sizes
template< class S >
void saisBuckets( const S * size , S * B , S K , bool tails )
{
 S sum = 0;
 for( S c = 0 ; c < K ; ++c ) {
  sum += size[ c ];
  B[ c ] = tails ? sum : sum - size[ c ];
  }
 }

/// the scan from left to right: the L-suffixes, each stored complemented
/// if the suffix before it is of type S, and every entry scanned
/// complemented; in the first induction the entries moved are cleared
/// instead. On the small alphabets no branch depends on the entries: an
/// entry with nothing to move writes to a dummy place
template< bool small , bool first , class C , class S >
void saisInduceL( const C * T , S * SA , S n , const S * size , S * B ,
                  S K )
{
 constexpr S D = S( saisAhead );
 saisBuckets( size , B , K , false );
 S j = n - 1;
 SA[ B[ T[ j ] ]++ ] = ( j > 0 ) && ( T[ j - 1 ] < T[ j ] ) ? ~j : j;
 S dummy = 0;
 for( S i = 0 ; i < n ; ++i ) {
  if( i + D < n ) {                   // the text, then bucket and place
   const S a = SA[ i + D ];
   if( a > 0 ) saisPrefetch( T + ( a - 1 ) );
   if constexpr( ! small ) {
    const S e = SA[ i + D / 2 ] , f = SA[ i + D / 4 ];
    if( e > 0 ) saisPrefetch( B + T[ e - 1 ] );
    if( f > 0 ) saisPrefetchW( SA + B[ T[ f - 1 ] ] );
    }
   }
  j = SA[ i ];
  if constexpr( small ) {
   const S v = S( j > 0 );
   SA[ i ] = first && v ? 0 : ~j;
   const S p = v ? j - 1 : 0;
   const S c0 = S( T[ p ] );
   const bool mark = ( p > 0 ) && ( S( T[ p - ( p > 0 ) ] ) < c0 );
   S * dst = v ? SA + B[ c0 ] : &dummy;
   *dst = mark ? ~p : p;
   B[ c0 ] += v;
   }
  else {
   SA[ i ] = first && ( j > 0 ) ? 0 : ~j;
   if( j > 0 ) {
    --j;
    const S c0 = S( T[ j ] );
    SA[ B[ c0 ]++ ] = ( j > 0 ) && ( S( T[ j - 1 ] ) < c0 ) ? ~j : j;
    }
   }
  }
 }

/// the scan from right to left: the S-suffixes, each stored complemented
/// if the suffix before it is of type L (or it is the suffix 0), and the
/// complemented entries restored; in the first induction the entries
/// moved are cleared instead, and the complemented ones, the LMS-suffixes
/// (and the suffix 0), stay complemented. On the small alphabets no
/// branch depends on the entries
template< bool small , bool first , class C , class S >
void saisInduceS( const C * T , S * SA , S n , const S * size , S * B ,
                  S K )
{
 constexpr S D = S( saisAhead );
 saisBuckets( size , B , K , true );
 S dummy = 0;
 for( S i = n - 1 ; i >= 0 ; --i ) {
  if( i >= D ) {
   const S a = SA[ i - D ];
   if( a > 0 ) saisPrefetch( T + ( a - 1 ) );
   if constexpr( ! small ) {
    const S e = SA[ i - D / 2 ] , f = SA[ i - D / 4 ];
    if( e > 0 ) saisPrefetch( B + T[ e - 1 ] );
    if( f > 0 ) saisPrefetchW( SA + ( B[ T[ f - 1 ] ] - 1 ) );
    }
   }
  const S j = SA[ i ];
  if constexpr( small ) {
   const S v = S( j > 0 );
   const S p = v ? j - 1 : 0;
   const S c0 = S( T[ p ] );
   const bool mark = ( p == 0 ) || ( S( T[ p - ( p > 0 ) ] ) > c0 );
   B[ c0 ] -= v;
   S * dst = v ? SA + B[ c0 ] : &dummy;
   *dst = mark ? ~p : p;
   if constexpr( first ) SA[ i ] = v ? 0 : j;
   else SA[ i ] = v ? j : ~j;
   }
  else if( j > 0 ) {
   const S p = j - 1;
   const S c0 = S( T[ p ] );
   SA[ --B[ c0 ] ] = ( p == 0 ) || ( S( T[ p - 1 ] ) > c0 ) ? ~p : p;
   if constexpr( first ) SA[ i ] = 0;
   }
  else if constexpr( ! first )
   SA[ i ] = ~j;
  }
 }

/// the two scans of the induction
template< bool first , class C , class S >
void saisInduce( const C * T , S * SA , S n , const S * size , S * B ,
                 S K )
{
 if( K <= S( saisSmall ) ) {
  saisInduceL< true , first >( T , SA , n , size , B , K );
  saisInduceS< true , first >( T , SA , n , size , B , K );
  }
 else {
  saisInduceL< false , first >( T , SA , n , size , B , K );
  saisInduceS< false , first >( T , SA , n , size , B , K );
  }
 }

/// the symbols T[ a .. a + l - 1 ] and T[ b .. b + l - 1 ] are equal
template< class C , class S >
bool saisEqual( const C * T , S a , S b , S l )
{
 if constexpr( sizeof( C ) == 1 )
  return( std::memcmp( T + a , T + b , std::size_t( l ) ) == 0 );
 else
  return( std::equal( T + a , T + a + l , T + b ) );
 }

/// the suffix array of R, m names in [ 0 , K ) of which few repeat, into
/// SA[ 0 .. m - 1 ], by the doubling of Larsson and Sadakane on the groups
/// of equal names alone: the suffixes are first sorted by their name (a
/// counting sort by the counts of the names), so that a name that occurs
/// once is already in place, and a group of equal ranks is sorted by the
/// rank of the suffix h places ahead, h = 1, 2, 4, ..., its rank the last
/// place of its group; false, with SA undefined, if more than limit
/// suffixes are still tied after a round, or after 16 rounds, which
/// leaves the work to the recursion
template< class S >
bool saisDoubling( const S * R , S * SA , S m ,
                   const std::vector< S > & count , S limit )
{
 const std::size_t K = count.size();
 S tied = 0;                          // in the groups of a repeated name
 for( std::size_t c = 0 ; c < K ; ++c )
  if( count[ c ] > 1 ) tied += count[ c ];
 if( tied > limit ) return( false );
 std::vector< S > V( std::size_t( m ) , 0 ) , start( K , 0 );
 for( std::size_t c = 0 , sum = 0 ; c < K ; ++c ) {
  start[ c ] = S( sum );
  sum += std::size_t( count[ c ] );
  }
 // the counting sort, the group of each name prefetched, then its place
 constexpr S D = S( saisAhead );
 for( S k = 0 ; k < m ; ++k ) {
  if( k + 2 * D < m ) saisPrefetch( start.data() + R[ k + 2 * D ] );
  if( k + D < m ) saisPrefetchW( SA + start[ std::size_t( R[ k + D ] ) ] );
  SA[ start[ std::size_t( R[ k ] ) ]++ ] = k;
  }
 // the rank of a suffix, the last place of its group: start[ c ] is now
 // one past it
 for( S k = 0 ; k < m ; ++k ) {
  if( k + D < m ) saisPrefetch( start.data() + R[ k + D ] );
  V[ std::size_t( k ) ] = start[ std::size_t( R[ k ] ) ] - 1;
  }
 // the groups of more than one suffix, as ( first place , size )
 std::vector< std::pair< S , S > > G , H;
 for( std::size_t c = 0 ; c < K ; ++c )
  if( count[ c ] > 1 ) G.emplace_back( start[ c ] - count[ c ] , count[ c ] );
 std::vector< std::pair< S , S > > key;    // ( rank ahead , suffix )
 for( S h = 1 ; ! G.empty() ; h *= 2 ) {
  tied = 0;
  for( const auto & [ f , g ] : G ) tied += g;
  if( ( tied > limit ) || ( h > ( S( 1 ) << 16 ) ) ) return( false );
  // the keys of all the groups first, then the new ranks
  key.clear();
  for( const auto & [ f , g ] : G )
   for( S i = f ; i < f + g ; ++i ) {
    const S k = SA[ i ];
    if( k + h < m ) saisPrefetch( V.data() + ( k + h ) );
    key.emplace_back( k + h < m ? V[ std::size_t( k + h ) ] : S( -1 ) , k );
    }
  H.clear();
  std::size_t at = 0;
  for( const auto & [ f , g ] : G ) {
   auto * a = key.data() + at;
   std::sort( a , a + g );
   for( S i = 0 ; i < g ; ) {
    S j = i + 1;
    while( ( j < g ) && ( a[ j ].first == a[ i ].first ) ) ++j;
    if( j - i > 1 ) H.emplace_back( f + i , j - i );
    i = j;
    }
   at += std::size_t( g );
   }
  at = 0;
  for( const auto & [ f , g ] : G ) {
   const auto * a = key.data() + at;
   for( S i = 0 ; i < g ; ) {
    S j = i + 1;
    while( ( j < g ) && ( a[ j ].first == a[ i ].first ) ) ++j;
    for( S q = i ; q < j ; ++q ) {
     SA[ f + q ] = a[ q ].second;
     V[ std::size_t( a[ q ].second ) ] = f + j - 1;
     }
    i = j;
    }
   at += std::size_t( g );
   }
  G.swap( H );
  }
 return( true );
 }

/// SA-IS on T[ 0 .. n - 1 ], symbols in [ 0 , K ), into SA[ 0 .. n - 1 ];
/// the number of each symbol from *counts if given (the names of the
/// level above, counted in their sorted order), else counted here
template< class C , class S >
void sais( const C * T , S * SA , S n , S K ,
           std::vector< S > * counts = nullptr )
{
 if( n <= 1 ) {
  if( n == 1 ) SA[ 0 ] = 0;
  return;
  }
 std::vector< S > size , B( std::size_t( K ) , 0 );
 if( counts ) size = std::move( *counts );
 else {
  size.assign( std::size_t( K ) , 0 );
  for( S i = 0 ; i < n ; ++i ) ++size[ std::size_t( T[ i ] ) ];
  }
 constexpr S D = S( saisAhead );

 // the LMS places at the ends of their buckets, and in a bit vector; on
 // the small alphabets with no branch on the types, a place that is not
 // LMS writing to x, on the large ones a bucket is touched only by an LMS
 // place
 std::vector< std::uint64_t > lms( std::size_t( n ) / 64 + 1 , 0 );
 std::fill( SA , SA + n , S( 0 ) );
 saisBuckets( size.data() , B.data() , K , true );
 S m = 0 , x = 0;
 S s = 0;                           // the type of i + 1: n - 1 is L
 for( S i = n - 2 ; i >= 0 ; --i ) {
  const S a = S( T[ i ] ) , b = S( T[ i + 1 ] );
  const S t = S( a < b ) | ( S( a == b ) & s );
  const S l = ( t ^ 1 ) & s;        // i + 1 is LMS
  if( K <= S( saisSmall ) ) {
   S & bucket = B[ std::size_t( b ) ];
   bucket -= l;
   *( l ? SA + bucket : &x ) = i + 1;
   }
  else {
   if( i >= D ) saisPrefetch( B.data() + T[ i - D ] );
   if( l ) SA[ --B[ std::size_t( b ) ] ] = i + 1;
   }
  lms[ std::size_t( i + 1 ) >> 6 ] |= std::uint64_t( l ) << ( ( i + 1 ) &
                                                             63 );
  m += l;
  s = t;
  }
 // the LMS places from the last to the first, by the bits set
 auto forLms = [ & ]( auto && f ) {
  for( std::size_t w = lms.size() ; w-- > 0 ; )
   for( std::uint64_t v = lms[ w ] ; v ; ) {
    const int h = 63 - std::countl_zero( v );
    v &= ~( std::uint64_t( 1 ) << h );
    f( S( w * 64 + std::size_t( h ) ) );
    }
  };

 // no LMS place: a text that never rises, all its suffixes of type L
 if( m == 0 ) {
  saisInduce< false >( T , SA , n , size.data() , B.data() , K );
  return;
  }

 // the LMS-substrings sorted by one induction, then compacted
 saisInduce< true >( T , SA , n , size.data() , B.data() , K );
 for( S i = 0 , k = 0 ; i < n ; ++i )
  if( SA[ i ] < -1 ) SA[ k++ ] = ~SA[ i ];

 // their lengths at m + p / 2, then their names, from 1
 std::fill( SA + m , SA + n , S( 0 ) );
 S p = n;
 forLms( [ & ]( S i ) {
  SA[ m + i / 2 ] = p - i + 1;       // the last one includes the end
  p = i; } );
 S names = 0;
 std::vector< S > runs;              // the LMS-substrings of each name
 for( S i = 0 , prev = -1 , plen = 0 ; i < m ; ++i ) {
  if( i + D < m ) {
   const S q = SA[ i + D ];
   saisPrefetch( SA + m + q / 2 );
   saisPrefetch( T + q );
   }
  const S p = SA[ i ] , len = SA[ m + p / 2 ];
  if( ( prev < 0 ) || ( len != plen ) || ( p + len > n ) ||
      ( prev + plen > n ) || ! saisEqual( T , p , prev , len ) ) {
   ++names;
   runs.push_back( 0 );
   }
  ++runs.back();
  prev = p;
  plen = len;
  SA[ m + p / 2 ] = names;
  }

 // the LMS-suffixes sorted by the string of the names at the back of SA,
 // unless the names are all distinct: by the doubling on the groups of
 // equal names, if few, or else by recursion
 if( names < m ) {
  for( S i = n - 1 , j = n - 1 ; i >= m ; --i ) {
   const S v = SA[ i ];
   SA[ j ] = v - 1;                   // a slot of no LMS place is 0
   j -= S( v != 0 );
   }
  S * R = SA + ( n - m );
  if( ! saisDoubling( static_cast< const S * >( R ) , SA , m , runs ,
                      m / 4 ) )
   sais( static_cast< const S * >( R ) , SA , m , names , &runs );
  S j = m;                            // the LMS places, in order
  forLms( [ & ]( S i ) { R[ --j ] = i; } );
  for( S i = 0 ; i < m ; ++i ) {
   if( i + D < m ) saisPrefetch( R + SA[ i + D ] );
   SA[ i ] = R[ SA[ i ] ];
   }
  }

 // the LMS-suffixes at the ends of their buckets, and the induction
 std::fill( SA + m , SA + n , S( 0 ) );
 saisBuckets( size.data() , B.data() , K , true );
 for( S i = m - 1 ; i >= 0 ; --i ) {
  if( i >= D ) saisPrefetch( T + SA[ i - D ] );
  const S p = SA[ i ];
  SA[ i ] = 0;
  SA[ --B[ std::size_t( T[ p ] ) ] ] = p;
  }
 saisInduce< false >( T , SA , n , size.data() , B.data() , K );
 }

/// SA is the suffix array of T (bytes or integers), O( n )
template< class C , class I >
bool isSuffixArrayOf( const C * T , std::size_t n , const I * SA )
{
 std::vector< std::size_t > R( n + 1 , 0 );
 for( std::size_t r = 0 ; r < n ; ++r ) {
  if( ( SA[ r ] >= n ) || R[ SA[ r ] ] ) return( false );
  R[ SA[ r ] ] = r + 1;              // 0 for the end
  }
 for( std::size_t r = 1 ; r < n ; ++r ) {
  const std::size_t a = SA[ r - 1 ] , b = SA[ r ];
  if( T[ a ] != T[ b ] ) {
   if( T[ a ] > T[ b ] ) return( false );
   }
  else if( R[ a + 1 ] >= R[ b + 1 ] ) return( false );
  }
 return( true );
 }

/// LCP[ r ] is the lcp of the suffixes SA[ r ] and SA[ r + 1 ] of T
template< class I >
bool isLcpArrayOf( std::string_view T , const std::vector< I > & SA ,
                   const std::vector< I > & LCP )
{
 const std::size_t n = T.size();
 if( LCP.size() != ( n < 2 ? 0 : n - 1 ) ) return( false );
 for( std::size_t r = 0 ; r + 1 < n ; ++r ) {
  const std::size_t a = SA[ r ] , b = SA[ r + 1 ] , h = LCP[ r ];
  if( ( a + h > n ) || ( b + h > n ) ||
      ( T.substr( a , h ) != T.substr( b , h ) ) ||
      ( ( a + h < n ) && ( b + h < n ) && ( T[ a + h ] == T[ b + h ] ) ) )
   return( false );
  }
 return( true );
 }

/// the permuted LCP of T and SA into P, by Phi
template< class I >
void plcp( std::string_view T , const I * SA , I * P )
{
 const std::size_t n = T.size();
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 if( n == 0 ) return;
 // Phi in P, n for the first suffix of SA
 constexpr std::size_t D = saisAhead;
 P[ SA[ 0 ] ] = I( n );
 for( std::size_t r = 1 ; r < n ; ++r ) {
  if( r + D < n ) saisPrefetchW( P + SA[ r + D ] );
  P[ SA[ r ] ] = SA[ r - 1 ];
  }
 // the text at Phi prefetched ahead, the place of the comparison but for
 // h; the first suffix of SA has Phi = n, and h is then already 0, since
 // the suffix before it in the text shares at most one symbol with its
 // predecessor in SA. The first 2 symbols are compared one by one, since
 // the lcp is short on the texts with few repeats, then 8 at a time
 std::size_t h = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  if( i + D < n ) saisPrefetch( t + std::min< std::size_t >( P[ i + D ] + h ,
                                                            n ) );
  const std::size_t j = P[ i ] , far = n - std::max( i , j );
  const std::size_t near = std::min( far , h + 2 );
  while( ( h < near ) && ( t[ i + h ] == t[ j + h ] ) ) ++h;
  if( h == near )
   for( ; ; ) {
    if( h + 8 > far ) {
     while( ( h < far ) && ( t[ i + h ] == t[ j + h ] ) ) ++h;
     break;
     }
    std::uint64_t a , b;
    std::memcpy( &a , t + i + h , 8 );
    std::memcpy( &b , t + j + h , 8 );
    if( a != b ) {
     if constexpr( std::endian::native == std::endian::little )
      h += std::size_t( std::countr_zero( a ^ b ) ) / 8;
     else
      h += std::size_t( std::countl_zero( a ^ b ) ) / 8;
     break;
     }
    h += 8;
    }
  P[ i ] = I( h );
  h -= ( h != 0 );
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::vector< I > suffixArray( std::string_view T )
{
 using S = std::make_signed_t< I >;
 ADS_PRE( T.size() < std::size_t( std::numeric_limits< S >::max() ) );
 std::vector< I > SA( T.size() );
 detail::sais( reinterpret_cast< const unsigned char * >( T.data() ) ,
               reinterpret_cast< S * >( SA.data() ) , S( T.size() ) ,
               S( 256 ) );
 ADS_AUDIT( detail::isSuffixArrayOf(
             reinterpret_cast< const unsigned char * >( T.data() ) ,
             T.size() , SA.data() ) );
 return( SA );
 }

template< class I >
 requires( std::same_as< I , std::uint32_t > ||
           std::same_as< I , std::uint64_t > )
std::vector< I > suffixArray( const std::vector< I > & T , std::size_t K )
{
 using S = std::make_signed_t< I >;
 ADS_PRE( T.size() < std::size_t( std::numeric_limits< S >::max() ) );
 ADS_PRE( K <= std::size_t( std::numeric_limits< S >::max() ) );
 ADS_PRE( std::all_of( T.begin() , T.end() , [ K ]( I c ) {
  return( c < K ); } ) );
 std::vector< I > SA( T.size() );
 detail::sais( reinterpret_cast< const S * >( T.data() ) ,
               reinterpret_cast< S * >( SA.data() ) , S( T.size() ) ,
               S( std::max< std::size_t >( K , 1 ) ) );
 ADS_AUDIT( detail::isSuffixArrayOf( T.data() , T.size() , SA.data() ) );
 return( SA );
 }

template< class I >
std::vector< I > plcpArray( std::string_view T , const std::vector< I > & SA )
{
 const std::size_t n = T.size();
 ADS_AUDIT( ( SA.size() == n ) && detail::isSuffixArrayOf(
             reinterpret_cast< const unsigned char * >( T.data() ) , n ,
             SA.data() ) );
 std::vector< I > P( n );
 detail::plcp( T , SA.data() , P.data() );
 return( P );
 }

template< class I >
std::vector< I > lcpArrayPhi( std::string_view T ,
                              const std::vector< I > & SA )
{
 const std::size_t n = T.size();
 ADS_AUDIT( ( SA.size() == n ) && detail::isSuffixArrayOf(
             reinterpret_cast< const unsigned char * >( T.data() ) , n ,
             SA.data() ) );
 const auto P = std::make_unique_for_overwrite< I[] >( n );
 detail::plcp( T , SA.data() , P.get() );
 std::vector< I > LCP( n < 2 ? 0 : n - 1 );
 constexpr std::size_t D = detail::saisAhead;
 for( std::size_t r = 0 ; r + 1 < n ; ++r ) {
  if( r + D + 1 < n ) detail::saisPrefetch( P.get() + SA[ r + D + 1 ] );
  LCP[ r ] = P[ SA[ r + 1 ] ];
  }
 ADS_AUDIT( detail::isLcpArrayOf( T , SA , LCP ) );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SuffixSorting.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
