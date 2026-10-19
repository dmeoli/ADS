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

/// the alphabets of more than saisSmall symbols have their buckets and
/// the places these point to prefetched too, and their LMS places are
/// set with a branch, which touches a bucket only for an LMS place
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
/// instead. Two entries a step, the text before the entries D ahead
/// prefetched, and on the large alphabets also their buckets (D / 2
/// ahead) and the places these point to (D / 4 ahead)
template< bool small , bool first , class C , class S >
void saisInduceL( const C * T , S * SA , S n , const S * size , S * B ,
                  S K )
{
 constexpr S D = S( saisAhead );
 saisBuckets( size , B , K , false );
 S j = n - 1;
 SA[ B[ T[ j ] ]++ ] = ( j > 0 ) && ( T[ j - 1 ] < T[ j ] ) ? ~j : j;
 // an entry: the suffix before it moved to the head of its bucket
 auto move = [ & ]( S i ) {
  S q = SA[ i ];
  SA[ i ] = first && ( q > 0 ) ? 0 : ~q;
  if( q > 0 ) {
   --q;
   const S c0 = S( T[ q ] );
   SA[ B[ c0 ]++ ] = ( q > 0 ) && ( S( T[ q - 1 ] ) < c0 ) ? ~q : q;
   }
  };
 S i = 0;
 for( ; i + D + 1 < n ; i += 2 ) {     // two at a time, no checks
  const S a = SA[ i + D ] , b = SA[ i + D + 1 ];
  saisPrefetch( T + ( a > 0 ? a - 1 : 0 ) );
  saisPrefetch( T + ( b > 0 ? b - 1 : 0 ) );
  if constexpr( ! small ) {
   const S e = SA[ i + D / 2 ] , f = SA[ i + D / 4 ];
   saisPrefetch( B + T[ e > 0 ? e - 1 : 0 ] );
   saisPrefetchW( SA + B[ T[ f > 0 ? f - 1 : 0 ] ] );
   }
  move( i );
  move( i + 1 );
  }
 for( ; i < n ; ++i ) move( i );
 }

/// the scan from right to left: the S-suffixes, each stored complemented
/// if the suffix before it is of type L (or it is the suffix 0), and the
/// complemented entries restored; in the first induction the entries
/// moved are cleared instead, and the complemented ones, the LMS-suffixes
/// (and the suffix 0), stay complemented. Two entries a step, prefetched
/// as in saisInduceL()
template< bool small , bool first , class C , class S >
void saisInduceS( const C * T , S * SA , S n , const S * size , S * B ,
                  S K )
{
 constexpr S D = S( saisAhead );
 saisBuckets( size , B , K , true );
 // an entry: the suffix before it moved to the tail of its bucket
 auto move = [ & ]( S i ) {
  const S q = SA[ i ];
  if( q > 0 ) {
   const S p = q - 1;
   const S c0 = S( T[ p ] );
   SA[ --B[ c0 ] ] = ( p == 0 ) || ( S( T[ p - 1 ] ) > c0 ) ? ~p : p;
   if constexpr( first ) SA[ i ] = 0;
   }
  else if constexpr( ! first )
   SA[ i ] = ~q;
  };
 S i = n - 1;
 for( ; i >= D + 1 ; i -= 2 ) {        // two at a time, no checks
  const S a = SA[ i - D ] , b = SA[ i - D - 1 ];
  saisPrefetch( T + ( a > 0 ? a - 1 : 0 ) );
  saisPrefetch( T + ( b > 0 ? b - 1 : 0 ) );
  if constexpr( ! small ) {
   const S e = SA[ i - D / 2 ] , f = SA[ i - D / 4 ];
   saisPrefetch( B + T[ e > 0 ? e - 1 : 0 ] );
   saisPrefetchW( SA + ( B[ T[ f > 0 ? f - 1 : 0 ] ] - 1 ) );
   }
  move( i );
  move( i - 1 );
  }
 for( ; i >= 0 ; --i ) move( i );
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

/// the first stage of SA-IS, the partial induction: the types and the LMS
/// places of T (n >= 2 symbols in [ 0 , K )), and the LMS-substrings
/// sorted and named by one scan of the L-suffixes whose predecessor is L
/// and one of those whose predecessor is S. The suffixes i >= 1 fall into
/// four kinds by the types of i and i - 1, LL, LS, LMS (S after L) and SS,
/// counted per first symbol in size4; the area A, an LL part and an LMS
/// part per symbol, holds the entries that the scan from left to right
/// moves, and the area C, an LS part and an SS part per symbol, those that
/// the scan from right to left moves, so that no scan visits an entry with
/// nothing to do. An entry is marked (its top bit) when it starts a new
/// name: a moved suffix is marked unless the last one moved into the same
/// part came from a suffix of the same name, the name being the number d
/// of the marks met by the scan. On return SA[ 0 .. m ) holds the LMS
/// places sorted by their substrings, SA[ m + p / 2 ] the name (from 1) of
/// p, 0 elsewhere in [ m , n ), runs the LMS-substrings of each name, lms
/// the bit vector of the LMS places and *first the type of the suffix 0
/// (1 for S); the number of the LMS places is returned
template< class C , class S >
S saisPartial( const C * T , S * SA , S n , S K ,
               std::vector< S > & size4 ,
               std::vector< std::uint64_t > & lms , S * first ,
               S * names , std::vector< S > & runs )
{
 constexpr S top = std::numeric_limits< S >::min() ,
             low = std::numeric_limits< S >::max();
 constexpr S D = S( saisAhead );
 const std::size_t k = std::size_t( K );
 // the kinds, right to left: 0 LL , 1 LS , 2 LMS , 3 SS, 2 t( i ) +
 // t( i - 1 ), 1 for S
 size4.assign( 4 * k , 0 );
 lms.assign( std::size_t( n ) / 64 + 1 , 0 );
 S m = 0 , s = 0;                    // the suffix n - 1 is of type L
 for( S i = n - 1 ; i >= 1 ; --i ) {
  const S a = S( T[ i - 1 ] ) , b = S( T[ i ] );
  const S u = S( a < b ) | ( S( a == b ) & s );
  const S kind = 2 * s + u;
  if( ( K > S( saisSmall ) ) && ( i > D ) )
   saisPrefetchW( size4.data() + 4 * std::size_t( T[ i - D ] ) );
  ++size4[ 4 * std::size_t( b ) + std::size_t( kind ) ];
  const S l = S( kind == 2 );
  lms[ std::size_t( i ) >> 6 ] |= std::uint64_t( l ) << ( i & 63 );
  m += l;
  s = u;
  }
 *first = s;
 if( m == 0 ) return( 0 );

 // the parts: A = LL , LMS per symbol; C = LS , SS per symbol; the LMS
 // places sorted at last in [ 0 , m ), per symbol
 // a part v = 2 c + t of the moves of a scan: where the next move goes,
 // and the name of the last one, side by side in one line of cache
 struct Part {
  S to , last;
  };
 std::vector< Part > L( 2 * k , Part{ 0 , 0 } ) , R( 2 * k , Part{ 0 , 0 } );
 std::vector< S > seed( k );
 S sum = 0;
 for( std::size_t c = 0 ; c < k ; ++c ) {
  L[ 2 * c ].to = sum;               // LL: the moves of the L-scan into A
  sum += size4[ 4 * c ];
  seed[ c ] = sum;
  sum += size4[ 4 * c + 2 ];
  }
 const S endA = sum;
 for( std::size_t c = 0 , o = 0 ; c < k ; ++c ) {
  L[ 2 * c + 1 ].to = sum;           // LS: the moves of the L-scan into C
  sum += size4[ 4 * c + 1 ] + size4[ 4 * c + 3 ];
  R[ 2 * c ].to = sum;               // SS: the moves of the S-scan
  o += std::size_t( size4[ 4 * c + 2 ] );
  R[ 2 * c + 1 ].to = S( o );        // the LMS places, from the end
  }
 const S endC = sum;                 // n - 1: the suffix 0 is in none
 const bool large = K > S( saisSmall );
 // the prefetches of the large alphabets read the symbols of entries not
 // written yet, which must then be 0
 if( large ) std::fill( SA , SA + n , S( 0 ) );

 // the seeds, the LMS places by their first symbol, the first one of each
 // symbol marked
 for( std::size_t w = 0 ; w < lms.size() ; ++w ) {
  if( large && ( w + 1 < lms.size() ) )
   for( std::uint64_t v = lms[ w + 1 ] ; v ; v &= v - 1 )
    saisPrefetchW( seed.data() + T[ ( w + 1 ) * 64 +
                                    std::size_t( std::countr_zero( v ) ) ] );
  for( std::uint64_t v = lms[ w ] ; v ; v &= v - 1 ) {
   const S p = S( w * 64 + std::size_t( std::countr_zero( v ) ) );
   SA[ seed[ std::size_t( T[ p ] ) ]++ ] = p;
   }
  }
 for( std::size_t c = 0 ; c < k ; ++c )
  if( size4[ 4 * c + 2 ] ) SA[ seed[ c ] - size4[ 4 * c + 2 ] ] |= top;

 // the scan of A from left to right: the suffix p - 1, of type L, to the
 // LL part of its symbol if p - 2 is of type L, to its LS part otherwise
 S d = 1;
 {
  const S j = n - 1;
  const std::size_t v = 2 * std::size_t( T[ j ] ) +
                        std::size_t( T[ j - 1 ] < T[ j ] );
  SA[ L[ v ].to++ ] = j | top;
  L[ v ].last = d;
  }
 // the part of an entry p: the symbol before it and the type of the one
 // before that, with no check (p > 1 , or 0 for an entry not yet there)
 auto part = [ & ]( S p , bool forS ) {
  const S j = p - ( p > 1 ? 1 : 0 );
  const S c = S( T[ j ] ) , b = S( T[ j - ( j > 0 ? 1 : 0 ) ] );
  return( 2 * std::size_t( c ) + std::size_t( forS ? b > c : b < c ) );
  };
 for( S i = 0 ; i < endA ; ++i ) {
  if( i + D < endA ) {
   const S a = SA[ i + D ] & low;
   saisPrefetch( T + ( a - ( a > 1 ? 2 : a ) ) );
   if( large ) {                      // its part, and where it writes
    saisPrefetchW( L.data() + part( SA[ i + D / 2 ] & low , false ) );
    saisPrefetchW( SA + L[ part( SA[ i + D / 4 ] & low , false ) ].to );
    }
   }
  S p = SA[ i ];
  d += S( p < 0 );
  p &= low;
  const S j = p - 1;
  if( j == 0 ) continue;             // the suffix 0 moves nothing
  const S c = S( T[ j ] );
  Part & q = L[ 2 * std::size_t( c ) + std::size_t( S( T[ j - 1 ] ) < c ) ];
  SA[ q.to++ ] = j | ( q.last != d ? top : 0 );
  q.last = d;
  }

 // the marks of the LS parts moved one place left, so that a mark tells
 // that an entry differs from the one after it, as the scan from right to
 // left meets them; the last entry of a part is always marked
 for( std::size_t c = 0 ; c < k ; ++c ) {
  const S e = L[ 2 * c + 1 ].to , b = e - size4[ 4 * c + 1 ];
  for( S i = e - 1 , carry = top ; i >= b ; --i ) {
   const S x = SA[ i ];
   SA[ i ] = ( x & low ) | carry;
   carry = x & top;
   }
  }

 // the scan of C from right to left: the suffix p - 1, of type S, to the
 // SS part of its symbol if p - 2 is of type S, among the sorted LMS
 // places otherwise
 for( S i = endC - 1 ; i >= endA ; --i ) {
  if( i - D >= endA ) {
   const S a = SA[ i - D ] & low;
   saisPrefetch( T + ( a - ( a > 1 ? 2 : a ) ) );
   if( large ) {
    saisPrefetchW( R.data() + part( SA[ i - D / 2 ] & low , true ) );
    saisPrefetchW( SA + ( R[ part( SA[ i - D / 4 ] & low , true ) ].to -
                          1 ) );
    }
   }
  S p = SA[ i ];
  d += S( p < 0 );
  p &= low;
  const S j = p - 1;
  if( j == 0 ) continue;
  const S c = S( T[ j ] );
  Part & q = R[ 2 * std::size_t( c ) + std::size_t( S( T[ j - 1 ] ) > c ) ];
  SA[ --q.to ] = j | ( q.last != d ? top : 0 );
  q.last = d;
  }

 // the names: SA[ i ] marked if it differs from SA[ i + 1 ]
 std::fill( SA + m , SA + n , S( 0 ) );
 S name = 1;
 runs.assign( 1 , 0 );
 for( S i = 0 ; i < m ; ++i ) {
  const S p = SA[ i ] & low;
  SA[ m + p / 2 ] = name;
  ++runs.back();
  if( ( SA[ i ] < 0 ) && ( i + 1 < m ) ) {
   ++name;
   runs.push_back( 0 );
   }
  SA[ i ] = p;
  }
 *names = name;
 return( m );
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
 constexpr S D = S( saisAhead );
 (void) counts;
 std::vector< S > size4 , runs;
 std::vector< std::uint64_t > lms;
 S first , names;
 const S m = saisPartial( T , SA , n , K , size4 , lms , &first , &names ,
                          runs );
 // the numbers of the symbols, the suffix 0 in none of the kinds
 std::vector< S > size( std::size_t( K ) , 0 ) , B( std::size_t( K ) , 0 );
 for( std::size_t c = 0 ; c < std::size_t( K ) ; ++c )
  size[ c ] = size4[ 4 * c ] + size4[ 4 * c + 1 ] + size4[ 4 * c + 2 ] +
              size4[ 4 * c + 3 ];
 ++size[ std::size_t( T[ 0 ] ) ];
 // the LMS places from the last to the first, by the bits set
 auto forLms = [ & ]( auto && f ) {
  for( std::size_t w = lms.size() ; w-- > 0 ; )
   for( std::uint64_t v = lms[ w ] ; v ; ) {
    const int h = 63 - std::countl_zero( v );
    v &= ~( std::uint64_t( 1 ) << h );
    f( S( w * 64 + std::size_t( h ) ) );
    }
  };

 // no LMS place: the suffixes of type S, if any, are a prefix of T; with
 // none (the suffix 0 is of type L) T never rises, each suffix is larger
 // than the next one and SA is n - 1 , ... , 0
 if( m == 0 ) {
  if( first == 0 )
   for( S r = 0 ; r < n ; ++r ) SA[ r ] = n - 1 - r;
  else {
   std::fill( SA , SA + n , S( 0 ) );
   saisInduce< false >( T , SA , n , size.data() , B.data() , K );
   }
  return;
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
 std::size_t r = 1;
 for( ; r + D + 4 <= n ; r += 4 ) {             // 4 at a time, no checks
  for( std::size_t q = 0 ; q < 4 ; ++q ) saisPrefetchW( P + SA[ r + D + q ] );
  for( std::size_t q = 0 ; q < 4 ; ++q ) P[ SA[ r + q ] ] = SA[ r + q - 1 ];
  }
 for( ; r < n ; ++r ) P[ SA[ r ] ] = SA[ r - 1 ];
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
 const I * const Q = P.get();
 std::size_t r = 0;
 for( ; r + D + 5 <= n ; r += 4 ) {             // 4 at a time, no checks
  for( std::size_t q = 0 ; q < 4 ; ++q )
   detail::saisPrefetch( Q + SA[ r + D + 1 + q ] );
  for( std::size_t q = 0 ; q < 4 ; ++q ) LCP[ r + q ] = Q[ SA[ r + 1 + q ] ];
  }
 for( ; r + 1 < n ; ++r ) LCP[ r ] = Q[ SA[ r + 1 ] ];
 ADS_AUDIT( detail::isLcpArrayOf( T , SA , LCP ) );
 return( LCP );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SuffixSorting.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
