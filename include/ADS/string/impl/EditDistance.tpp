/*--------------------------------------------------------------------------*/
/*------------------------- File EditDistance.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of EditDistance [see EditDistance.h], with their
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

#include "ADS/SimdInline.h"
#include "ADS/string/ApproximateMatching.h"
#include "ADS/string/LongestCommonSubsequence.h"

#include <algorithm>
#include <bit>
#include <cstring>
#include <limits>
#include <memory>
#include <string>

namespace ADS
{

namespace detail
{

/// the bits of P: at( c )[ b ] has bit i set if P[ 64 b + i ] is c, for
/// the B blocks of 64 places; up to 4 blocks they lie in the object and
/// are cleared at once, beyond only the words of the bytes of P and of the
/// text T are cleared, so that a pair costs O( m + n ) and not 256 B words
class PeqBlocks
{
 public:
 PeqBlocks( std::string_view P , std::string_view T , std::size_t B )
     : f_B( B ) {
  std::uint64_t * w = f_small;
  if( 256 * B > sizeof( f_small ) / sizeof( f_small[ 0 ] ) ) {
   f_big = std::make_unique_for_overwrite< std::uint64_t[] >( 256 * B );
   w = f_big.get();
   }
  f_w = w;
  if( w == f_small )  // a few blocks: all the bytes
   std::memset( w , 0 , 256 * B * sizeof( std::uint64_t ) );
  else
   clearSeen( P , T , B );
  for( std::size_t i = 0 ; i < P.size() ; ++i )
   w[ std::size_t( static_cast< unsigned char >( P[ i ] ) ) * B + i / 64 ] |=
     std::uint64_t( 1 ) << ( i % 64 );
  }

 PeqBlocks( const PeqBlocks & ) = delete;
 PeqBlocks & operator=( const PeqBlocks & ) = delete;

 /// the B words of c, a byte of P or of T
 [[nodiscard]] const std::uint64_t * at( unsigned char c ) const {
  return( f_w + std::size_t( c ) * f_B );
  }

 private:
 /// the words of the bytes of P and T cleared, each byte once
 void clearSeen( std::string_view P , std::string_view T , std::size_t B ) {
  std::uint64_t * w = f_big.get();
  std::uint64_t seen[ 4 ] = { 0 , 0 , 0 , 0 };
  auto clear = [ & ]( std::string_view S ) {
   for( unsigned char c : S )
    if( ! ( ( seen[ c >> 6 ] >> ( c & 63 ) ) & 1 ) ) {
     seen[ c >> 6 ] |= std::uint64_t( 1 ) << ( c & 63 );
     std::fill_n( w + c * B , B , 0 );
     }
   };
  clear( P );
  clear( T );
  }

 std::size_t f_B;
 std::uint64_t f_small[ 256 * 4 ];
 std::unique_ptr< std::uint64_t[] > f_big;
 const std::uint64_t * f_w;
 };

/// the horizontal difference at the boundary of two blocks: +1 if p, -1
/// if n (one bit each)
struct Carry
{
 std::uint64_t p , n;
 };

/// a column of a block of 64 cells: the vertical differences +1 in Pv and
/// -1 in Mv, the matches in Eq, the horizontal difference c at its top,
/// replaced by the one at its bottom; no branch
inline void myersBlock( std::uint64_t & Pv , std::uint64_t & Mv ,
			std::uint64_t Eq , Carry & c )
{
 const std::uint64_t Xv = Eq | Mv;
 Eq |= c.n;
 const std::uint64_t Xh = ( ( ( Eq & Pv ) + Pv ) ^ Pv ) | Eq;
 std::uint64_t Ph = Mv | ~( Xh | Pv ) , Mh = Pv & Xh;
 const Carry out{ Ph >> 63 , Mh >> 63 };
 Ph = ( Ph << 1 ) | c.p;
 Mh = ( Mh << 1 ) | c.n;
 Pv = Mh | ~( Xv | Ph );
 Mv = Ph & Xv;
 c = out;
 }

/// a step of the K lanes of a strip: the lane q moves its block (Pv in
/// pv[ q ], Mv in mv[ q ], the matches eq[ q ], the carry in cp[ q ] and
/// cn[ q ]) by a column as myersBlock(), if Masked only where act[ q ] is
/// all ones; on the vectors of Highway [see SimdInline.h], else a lane at
/// a time, their chains of operations overlapping in the processor
template< std::size_t K , bool Masked >
inline void myersLanes( std::uint64_t * pv , std::uint64_t * mv ,
			const std::uint64_t * eq , std::uint64_t * cp ,
			std::uint64_t * cn , const std::uint64_t * act )
{
#ifdef ADS_WITH_HIGHWAY
 simd::here::myersLanes< K , Masked >( pv , mv , eq , cp , cn , act );
#else
 for( std::size_t q = 0 ; q < K ; ++q ) {
  std::uint64_t p = pv[ q ] , n = mv[ q ];
  Carry c{ cp[ q ] , cn[ q ] };
  myersBlock( p , n , eq[ q ] , c );
  if( ! Masked || act[ q ] ) {
   pv[ q ] = p;
   mv[ q ] = n;
   cp[ q ] = c.p;
   cn[ q ] = c.n;
   }
  }
#endif
 }

/// the edit distance of a pattern P of at most 64 bytes and a text T
inline std::size_t levenshteinWord( std::string_view P , std::string_view T )
{
 const PeqBlocks Peq( P , T , 1 );
 const std::size_t m = P.size();
 const std::uint64_t top = std::uint64_t( 1 ) << ( m - 1 );
 std::uint64_t Pv = ~std::uint64_t( 0 ) , Mv = 0;
 std::size_t d = m;
 for( unsigned char c : T ) {
  const std::uint64_t Eq = *Peq.at( c );
  const std::uint64_t Xv = Eq | Mv;
  const std::uint64_t Xh = ( ( ( Eq & Pv ) + Pv ) ^ Pv ) | Eq;
  std::uint64_t Ph = Mv | ~( Xh | Pv ) , Mh = Pv & Xh;
  d += ( Ph & top ) != 0;
  d -= ( Mh & top ) != 0;
  Ph = ( Ph << 1 ) | 1;  // the top row: D[ 0 ][ j ] = j
  Mh <<= 1;
  Pv = Mh | ~( Xv | Ph );
  Mv = Ph & Xv;
  }
 return( d );
 }

/// the edit distance of P (m > 64 bytes, the column) and T (n >= m
/// bytes), by all the blocks of every column, K = 4 columns at a time in a
/// strip: the block b of the column j needs only the block b - 1 of the
/// column j and the block b of the column j - 1, so that the blocks i - k
/// of the columns j + k, k < K, are independent; they are the lanes of a
/// step of the strip [see myersLanes()], which computes K blocks at once on
/// the vectors of Highway, and otherwise overlaps their chains of
/// operations in the processor. A
/// block passes from a column of the strip to the next in a register, and
/// a step loads and stores one block
inline std::size_t levenshteinFull( std::string_view P , std::string_view T )
{
 constexpr std::size_t K = 4;
 const std::size_t m = P.size() , n = T.size() , B = ( m + 63 ) / 64;
 const PeqBlocks Peq( P , T , B );
 std::vector< std::uint64_t > V( 2 * B );  // Pv and Mv of each block
 for( std::size_t b = 0 ; b < B ; ++b )
  V[ 2 * b ] = ~std::uint64_t( 0 );
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 std::size_t d = 64 * B;  // the bottom of the last
 // a column over the blocks [ from , to ), its carry in c
 auto column = [ & ]( const std::uint64_t * eq , std::size_t from ,
		      std::size_t to , Carry & c ) {
  for( std::size_t b = from ; b < to ; ++b )
   myersBlock( V[ 2 * b ] , V[ 2 * b + 1 ] , eq[ b ] , c );
  };
 std::size_t j = 0;
 if( B >= K )
  for( ; j + K <= n ; j += K ) {
   const std::uint64_t * eq[ K ];
   Carry c[ K ];
   for( std::size_t k = 0 ; k < K ; ++k ) {
    eq[ k ] = Peq.at( t[ j + k ] );
    c[ k ] = Carry{ 1 , 0 };
    }
   // the start: the column k over the blocks [ 0 , K - 1 - k )
   for( std::size_t k = 0 ; k + 1 < K ; ++k )
    column( eq[ k ] , 0 , K - 1 - k , c[ k ] );
   // the steps i: the column k at the block i - k, the lane k
   std::uint64_t pv[ K ] , mv[ K ] , cp[ K ] , cn[ K ] , e[ K ];
   for( std::size_t k = 0 ; k < K ; ++k ) {
    pv[ k ] = V[ 2 * ( K - 1 - k ) ];
    mv[ k ] = V[ 2 * ( K - 1 - k ) + 1 ];
    cp[ k ] = c[ k ].p;
    cn[ k ] = c[ k ].n;
    }
   for( std::size_t i = K - 1 ; ; ) {
    for( std::size_t k = 0 ; k < K ; ++k )
     e[ k ] = eq[ k ][ i - k ];
    myersLanes< K , false >( pv , mv , e , cp , cn , nullptr );
    if( ++i == B )
     break;
    V[ 2 * ( i - K ) ] = pv[ K - 1 ];  // done by the strip
    V[ 2 * ( i - K ) + 1 ] = mv[ K - 1 ];
    for( std::size_t k = K - 1 ; k > 0 ; --k ) {
     pv[ k ] = pv[ k - 1 ];
     mv[ k ] = mv[ k - 1 ];
     }
    pv[ 0 ] = V[ 2 * i ];
    mv[ 0 ] = V[ 2 * i + 1 ];
    }
   for( std::size_t k = 0 ; k < K ; ++k ) {
    V[ 2 * ( B - 1 - k ) ] = pv[ k ];
    V[ 2 * ( B - 1 - k ) + 1 ] = mv[ k ];
    c[ k ] = Carry{ cp[ k ] , cn[ k ] };
    }
   // the end: the column k over the blocks [ B - k , B )
   for( std::size_t k = 1 ; k < K ; ++k )
    column( eq[ k ] , B - k , B , c[ k ] );
   for( std::size_t k = 0 ; k < K ; ++k )
    d += c[ k ].p - c[ k ].n;
   }
 for( ; j < n ; ++j ) {
  Carry c{ 1 , 0 };
  column( Peq.at( t[ j ] ) , 0 , B , c );
  d += c.p - c.n;
  }
 const std::size_t pad = 64 * B - m;
 const std::uint64_t mask = pad ? ~std::uint64_t( 0 ) << ( 64 - pad ) : 0;
 return( d + std::size_t( std::popcount( V[ 2 * B - 1 ] & mask ) ) -
	  std::size_t( std::popcount( V[ 2 * B - 2 ] & mask ) ) );
 }

/// the edit distance of P (m > 64 bytes, the column) and T (n >= m bytes)
/// if at most k, k + 1 otherwise: only the blocks in the diagonal band of
/// k, and among them those that may lie on a path of cost at most k, as in
/// edlib: a cell ( i , j ) of value v costs at least v + | ( m - i ) - ( n
/// - j ) | to the end, and the cells of a block are at least its bottom v
/// minus their distance from it, so that the first and the last block go
/// when the least of these bounds exceeds k; the block below the last one
/// enters when the bottom of the last is at most k + 1, since the cells at
/// most k grow by one row per column. With Highway, a band of 8 blocks or
/// more goes 4 columns at a time in a strip as levenshteinFull(): the lane
/// of a column whose band does not hold the block of a step keeps its
/// state, a block may enter the band at each column of the strip (the cells
/// it starts with, 64 more than the bottom above and then +1 each, are
/// never below their values), and the blocks are dropped between two strips
/// \pre k >= n - m
/// \post *work is the number of blocks computed, and the search gives up
///       (k + 1) once it exceeds budget
inline std::size_t levenshteinBand( std::string_view P , std::string_view T ,
				    std::size_t k , std::size_t budget ,
				    std::size_t * work )
{
 *work = 0;
 const std::size_t m = P.size() , n = T.size() , B = ( m + 63 ) / 64 ,
   d = n - m;
 const PeqBlocks Peq( P , T , B );
 std::vector< std::uint64_t > V( 2 * B );  // Pv and Mv of each block
 std::vector< std::size_t > score( B );    // at the bottom of each
 for( std::size_t b = 0 ; b < B ; ++b ) {
  V[ 2 * b ] = ~std::uint64_t( 0 );
  score[ b ] = 64 * ( b + 1 );
  }
 // the least cost to the end of a cell of the block b in the column j
 // (the least of i + | c - i | on the rows i of the block, c = j - d the
 // row of the cell on the diagonal of the end, possibly negative)
 auto least = [ & ]( std::size_t b , std::size_t j ) {
  const std::ptrdiff_t r = std::ptrdiff_t( 64 * ( b + 1 ) ) , top = r - 63 ,
		       c = std::ptrdiff_t( j ) - std::ptrdiff_t( d ) ,
		       g = top <= c ? c : 2 * top - c ,
		       v = std::ptrdiff_t( score[ b ] ) + g - r;
  return( std::size_t( std::max< std::ptrdiff_t >( v , 0 ) ) );
  };
 // the rows i in [ j - k , j - d + k ] of the column j, 1 to m
 std::size_t first = 0 , last = std::min( B - 1 , ( k - d ) / 64 );
 const auto * t = reinterpret_cast< const unsigned char * >( T.data() );
 for( std::size_t j = 1 ; j <= n ; ) {
  const std::size_t lo = j > k ? j - k : 1 , hi = j + k - d;
  while( ( first < last ) &&
	 ( ( 64 * ( first + 1 ) < lo ) || ( least( first , j - 1 ) > k ) ) )
   ++first;
#ifdef ADS_WITH_HIGHWAY
  constexpr std::size_t K = 4;
  if( ( j + K - 1 <= n ) && ( last - first + 1 >= 2 * K ) ) {
   // the blocks [ first , L[ q ] ] of the column j + q, in the lane q
   std::size_t L[ K ];
   const std::uint64_t * eq[ K ];
   for( std::size_t q = 0 ; q < K ; ++q ) {
    L[ q ] = std::min( { B - 1 , last + q + 1 , ( hi + q - 1 ) / 64 } );
    eq[ q ] = Peq.at( t[ j - 1 + q ] );
    }
   const std::size_t F = first;
   const std::uint64_t ones = ~std::uint64_t( 0 );
   std::uint64_t pv[ K ] = { V[ 2 * F ] } , mv[ K ] = { V[ 2 * F + 1 ] } ,
		       sv[ K ] = { std::uint64_t( score[ F ] ) } ,
		       cp[ K ] = { 1 , 1 , 1 , 1 } , cn[ K ] = {} , act[ K ] ,
		       e[ K ];
   for( std::size_t i = F ; ; ++i ) {
    // the lane q at the block i - q: in its band, at its top, entering;
    // an entering block is all +1, its bottom 64 below that of the block
    // above in the previous column, which the lane q left one step ago
    // (now in the lane q + 1, or stored, for the lane K - 1) and whose
    // carry gives back its bottom in the previous column
    for( std::size_t q = 0 ; q < K ; ++q ) {
     const bool band = ( i >= F + q ) && ( i - q <= L[ q ] );
     act[ q ] = band ? ones : 0;
     if( ( i == L[ q ] + q ) && ( L[ q ] > ( q ? L[ q - 1 ] : last ) ) ) {
      pv[ q ] = ones;
      mv[ q ] = 0;
      const std::uint64_t above =
	q + 1 < K ? sv[ q + 1 ] : ( i >= K ? score[ i - K ] : 0 );
      sv[ q ] = above - ( cp[ q ] - cn[ q ] ) + 64;
      }
     if( i == F + q ) {
      cp[ q ] = 1;
      cn[ q ] = 0;
      }
     e[ q ] = ( i >= q ) && ( i - q < B ) ? eq[ q ][ i - q ] : 0;
     }
    myersLanes< K , true >( pv , mv , e , cp , cn , act );
    for( std::size_t q = 0 ; q < K ; ++q )
     sv[ q ] += ( cp[ q ] - cn[ q ] ) & act[ q ];
    if( ( i >= K - 1 ) && ( i - ( K - 1 ) < B ) ) {  // the lane K - 1, done
     V[ 2 * ( i - ( K - 1 ) ) ] = pv[ K - 1 ];
     V[ 2 * ( i - ( K - 1 ) ) + 1 ] = mv[ K - 1 ];
     score[ i - ( K - 1 ) ] = std::size_t( sv[ K - 1 ] );
     }
    if( i == L[ K - 1 ] + K - 1 )
     break;
    for( std::size_t q = K - 1 ; q > 0 ; --q ) {
     pv[ q ] = pv[ q - 1 ];
     mv[ q ] = mv[ q - 1 ];
     sv[ q ] = sv[ q - 1 ];
     }
    const bool in = i + 1 < B;
    pv[ 0 ] = in ? V[ 2 * ( i + 1 ) ] : 0;
    mv[ 0 ] = in ? V[ 2 * ( i + 1 ) + 1 ] : 0;
    sv[ 0 ] = in ? score[ i + 1 ] : 0;
    }
   *work += ( L[ K - 1 ] + K - F ) * K;
   last = L[ K - 1 ];
   j += K;
   while( ( last > first ) && ( least( last , j - 1 ) > k ) )
    --last;
   if( ( ( first == last ) && ( least( last , j - 1 ) > k ) ) ||
       ( *work > budget ) )
    return( k + 1 );  // no cell of the column leads to k
   continue;
   }
#endif
  const std::uint64_t * eq = Peq.at( t[ j - 1 ] );
  Carry h{ 1 , 0 };  // above the band, as the top row
  for( std::size_t b = first ; b <= last ; ++b ) {
   myersBlock( V[ 2 * b ] , V[ 2 * b + 1 ] , eq[ b ] , h );
   score[ b ] += h.p - h.n;
   }
  if( ( last + 1 < B ) && ( 64 * ( last + 1 ) < hi ) &&
      ( score[ last ] <= k + 1 ) ) {  // the next block enters, all +1
   ++last;
   V[ 2 * last ] = ~std::uint64_t( 0 );
   V[ 2 * last + 1 ] = 0;
   score[ last ] = score[ last - 1 ] + 64 - h.p + h.n;
   myersBlock( V[ 2 * last ] , V[ 2 * last + 1 ] , eq[ last ] , h );
   score[ last ] += h.p - h.n;
   }
  else
   while( ( last > first ) && ( least( last , j ) > k ) )
    --last;
  *work += last - first + 1;
  if( ( ( first == last ) && ( least( last ,
				      j ) > k ) ) || ( *work > budget ) )
   return( k + 1 );  // no cell of the column leads to k
  ++j;
  }
 if( last < B - 1 )
  return( k + 1 );
 // the rows of the last block past m
 const std::size_t pad = 64 * B - m;
 const std::uint64_t mask = pad ? ~std::uint64_t( 0 ) << ( 64 - pad ) : 0;
 const std::size_t r = score[ B - 1 ] +
		       std::size_t( std::popcount( V[ 2 * B - 1 ] & mask ) ) -
		       std::size_t( std::popcount( V[ 2 * B - 2 ] & mask ) );
 return( std::min( r , k + 1 ) );
 }

/// the edit distance of a and b if at most e, e + 1 otherwise, by the
/// diagonal transitions of Landau and Vishkin (the furthest reaching
/// paths of Myers): F[ k ] is the furthest row of the diagonal k = j - i
/// reached with h edits, from the rows of h - 1 edits on the diagonals k
/// - 1, k and k + 1, then slid along the matches, 8 bytes at a time;
/// \f$ O( n + h^2 ) \f$ expected time for distance h on random strings.
/// At h = 32, 64, 128, ... edits, the furthest row r reached gives the
/// guess h m / r of the distance, and the search gives up (e + 1) if the
/// guess exceeds far; the last guess goes to *guess
inline std::size_t levenshteinDiagonal( std::string_view a ,
					std::string_view b ,
					std::size_t e , std::size_t far ,
					std::size_t * guess )
{
 const std::ptrdiff_t m = std::ptrdiff_t( a.size() ) ,
		      n = std::ptrdiff_t( b.size() ) , goal = n - m;
 const auto * x = reinterpret_cast< const unsigned char * >( a.data() );
 const auto * y = reinterpret_cast< const unsigned char * >( b.data() );
 // the row reached from the row i of the diagonal k along the matches
 auto slide = [ & ]( std::ptrdiff_t i , std::ptrdiff_t k ) {
  const std::ptrdiff_t end = std::min( m , n - k );
  while( i + 8 <= end ) {
   std::uint64_t u , v;
   std::memcpy( &u , x + i , 8 );
   std::memcpy( &v , y + i + k , 8 );
   if( u != v ) {
    if constexpr( std::endian::native == std::endian::little )
     return( i + std::countr_zero( u ^ v ) / 8 );
    else
     return( i + std::countl_zero( u ^ v ) / 8 );
    }
   i += 8;
   }
  while( ( i < end ) && ( x[ i ] == y[ i + k ] ) )
   ++i;
  return( i );
  };
 constexpr std::ptrdiff_t none =
   std::numeric_limits< std::ptrdiff_t >::min() / 2;
 const std::ptrdiff_t E =
   std::ptrdiff_t( std::min< std::size_t >( e , std::size_t( m + n ) ) );
 // the rows of the diagonals -E - 1 to E + 1, at the place k + E + 1
 std::vector< std::ptrdiff_t > F( std::size_t( 2 * E + 3 ) , none ) ,
   G( std::size_t( 2 * E + 3 ) , none );
 const std::ptrdiff_t o = E + 1;
 F[ std::size_t( o ) ] = slide( 0 , 0 );
 *guess = e + 1;
 if( ( goal == 0 ) && ( F[ std::size_t( o ) ] == m ) )
  return( 0 );
 for( std::ptrdiff_t h = 1 ; h <= E ; ++h ) {
  const std::ptrdiff_t lo = std::max( -h , -m ) , hi = std::min( h , n );
  for( std::ptrdiff_t k = lo ; k <= hi ; ++k ) {
   const std::size_t p = std::size_t( k + o );
   // a substitution or a deletion (a row more), an insertion (a column)
   std::ptrdiff_t i = std::max( { F[ p ] + 1 , F[ p + 1 ] + 1 , F[ p - 1 ] } );
   i = std::min( { i , m , n - k } );
   if( i < 0 ) {
    G[ p ] = none;
    continue;
    }
   i = std::max< std::ptrdiff_t >( i , k < 0 ? -k : 0 );
   G[ p ] = slide( i , k );
   }
  std::swap( F , G );
  if( ( goal >= lo ) && ( goal <= hi ) &&
      ( F[ std::size_t( goal + o ) ] == m ) )
   return( std::size_t( h ) );
  if( ( h >= 32 ) && ! ( h & ( h - 1 ) ) ) {  // a power of 2
   const std::ptrdiff_t r = *std::max_element( F.begin() , F.end() );
   *guess = std::size_t( h ) * std::size_t( m ) /
	    std::size_t( std::max< std::ptrdiff_t >( r , 1 ) );
   if( *guess > far )
    return( e + 1 );
   }
  }
 return( e + 1 );
 }

/// a and b without their common prefix and suffix
inline void stripAffixes( std::string_view & a , std::string_view & b )
{
 std::size_t p = 0;
 const std::size_t l = std::min( a.size() , b.size() );
 while( ( p < l ) && ( a[ p ] == b[ p ] ) )
  ++p;
 a.remove_prefix( p );
 b.remove_prefix( p );
 std::size_t s = 0;
 const std::size_t r = std::min( a.size() , b.size() );
 while( ( s < r ) && ( a[ a.size() - 1 - s ] == b[ b.size() - 1 - s ] ) )
  ++s;
 a.remove_suffix( s );
 b.remove_suffix( s );
 }

/// the LCS of the column a, of m bytes in B blocks of bits Peq, and of
/// b, by the bit-parallel LCS; NB the number of blocks, if fixed (1 to 4,
/// kept in registers), 0 if not
template< std::size_t NB >
std::size_t lcsBlocks( const PeqBlocks & Peq , std::string_view b ,
		       std::size_t m , std::size_t B )
{
 std::uint64_t fixed[ NB ? NB : 1 ];
 std::vector< std::uint64_t > grown;
 std::uint64_t * V = fixed;
 if constexpr( NB == 0 ) {
  grown.assign( B , ~std::uint64_t( 0 ) );
  V = grown.data();
  }
 else
  for( std::size_t k = 0 ; k < NB ; ++k )
   V[ k ] = ~std::uint64_t( 0 );
 const std::size_t nb = NB ? NB : B;
 for( unsigned char c : b ) {
  const std::uint64_t * M = Peq.at( c );
  std::uint64_t carry = 0;
  for( std::size_t k = 0 ; k < nb ; ++k ) {
   const std::uint64_t v = V[ k ] , u = v & M[ k ];
   const std::uint64_t s = v + u , x = s + carry;
   carry = ( s < v ) | ( x < s );
   V[ k ] = x | ( v & ~M[ k ] );
   }
  }
 std::size_t r = 0;
 for( std::size_t k = 0 ; k < nb ; ++k ) {
  const std::size_t bits = std::min< std::size_t >( 64 , m - 64 * k );
  const std::uint64_t mask =
    bits == 64 ? ~std::uint64_t( 0 ) : ( std::uint64_t( 1 ) << bits ) - 1;
  r += std::size_t( std::popcount( ~V[ k ] & mask ) );
  }
 return( r );
 }

/// the edit distance of a and b if at most k, k + 1 otherwise; with
/// doubling, k doubled from max( 64 , n - m ) until the distance is found,
/// or all the blocks computed once the band of k spans half the column
inline std::size_t levenshteinUpTo( std::string_view a , std::string_view b ,
				    std::size_t k , bool doubling )
{
 stripAffixes( a , b );
 if( a.size() > b.size() )
  std::swap( a , b );  // a is the column
 const std::size_t m = a.size() , n = b.size();
 if( n - m > k )
  return( k + 1 );
 if( m == 0 )
  return( n );
 if( m <= 64 )
  return( std::min( levenshteinWord( a , b ) , k + 1 ) );
 auto wide = [ & ]( std::size_t t ) {  // the band spans half the column
  return( 2 * t - ( n - m ) >= m / 2 );
  };
 const std::size_t all = n * ( ( m + 63 ) / 64 );  // the full table
 std::size_t work , guess;
 // the diagonal transitions, \f$ O( h^2 ) \f$, beat the blocks, O( n h /
 // 64 ), below a distance near n / 48 + 128: they go up to it, unless
 // their guess of the distance exceeds twice as much (and up to k, if
 // less); not on two blocks or fewer, which cost little
 const std::size_t cheap = n / 48 + 128 , e0 = std::min( k , cheap );
 guess = 0;
 if( m > 128 ) {
  const std::size_t r0 =
    levenshteinDiagonal( a , b , e0 , e0 == k ? n + 1 : 2 * cheap , &guess );
  if( ( r0 <= e0 ) || ( e0 == k ) )
   return( std::min( r0 , k + 1 ) );
  guess = std::min( guess , n );
  guess = std::max( guess + guess / 4 , e0 + 1 );
  }
 if( ! doubling )
  return( wide( std::min( k , n ) )
	     ? std::min( levenshteinFull( a , b ) , k + 1 )
	     : levenshteinBand( a , b , k , all , &work ) );
 // the bands, from the guess and a quarter more, while their blocks in
 // all stay below a quarter of the table, which levenshteinFull()
 // computes some 2 to 3 times faster per block
 std::size_t spent = 0;
 for( std::size_t t = std::max( { n - m , std::size_t( 64 ) , guess } ) ; ;
      t *= 2 ) {
  if( wide( t ) || ( spent >= all / 4 ) )
   return( levenshteinFull( a , b ) );
  const std::size_t r = levenshteinBand( a , b , t , all / 4 - spent , &work );
  if( r <= t )
   return( r );
  spent += work;
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

inline std::size_t levenshtein( std::string_view a , std::string_view b )
{
 const std::size_t r = detail::levenshteinUpTo(
   a , b , std::numeric_limits< std::size_t >::max() - 1 , true );
 ADS_AUDIT(
   r == std::size_t( editDistance( std::string( a ) , std::string( b ) ) ) );
 return( r );
 }

inline std::size_t levenshtein( std::string_view a , std::string_view b ,
				std::size_t k )
{
 const std::size_t r = detail::levenshteinUpTo( a , b , k , false );
 ADS_AUDIT( r == std::min( std::size_t( editDistance( std::string( a ) ,
						      std::string( b ) ) ) ,
			   k + 1 ) );
 return( r );
 }

inline std::size_t lcsLength( std::string_view a , std::string_view b )
{
 const std::size_t full = std::min( a.size() , b.size() );
 detail::stripAffixes( a , b );
 const std::size_t common = full - std::min( a.size() , b.size() );
 if( a.size() > b.size() )
  std::swap( a , b );
 const std::size_t m = a.size() , B = ( m + 63 ) / 64;
 std::size_t r = common;
 if( m > 0 ) {
  const detail::PeqBlocks Peq( a , b , B );
  switch( B ) {
   case( 1 ):
    r += detail::lcsBlocks< 1 >( Peq , b , m , B );
    break;
   case( 2 ):
    r += detail::lcsBlocks< 2 >( Peq , b , m , B );
    break;
   case( 3 ):
    r += detail::lcsBlocks< 3 >( Peq , b , m , B );
    break;
   case( 4 ):
    r += detail::lcsBlocks< 4 >( Peq , b , m , B );
    break;
   default:
    r += detail::lcsBlocks< 0 >( Peq , b , m , B );
   }
  }
 ADS_AUDIT( r == lcs( std::string( a ) , std::string( b ) ).size() + common );
 return( r );
 }

inline std::vector< std::pair< std::size_t , std::size_t > >
approximateMatches( std::string_view T , std::string_view P , std::size_t k )
{
 std::vector< std::pair< std::size_t , std::size_t > > R;
 const std::size_t m = P.size() , n = T.size();
 if( m == 0 ) {  // the empty substring at every end
  for( std::size_t j = 1 ; j <= n ; ++j )
   R.emplace_back( j , 0 );
  return( R );
  }
 const std::size_t B = ( m + 63 ) / 64;
 const detail::PeqBlocks Peq( P , T , B );
 std::vector< std::uint64_t > Pv( B , ~std::uint64_t( 0 ) ) , Mv( B , 0 );
 std::vector< std::size_t > score( B );
 for( std::size_t b = 0 ; b < B ; ++b )
  score[ b ] = 64 * ( b + 1 );
 const std::size_t pad = 64 * B - m;
 const std::uint64_t mask = pad ? ~std::uint64_t( 0 ) << ( 64 - pad ) : 0;
 // the blocks 0 .. y are computed, the others are above k
 std::size_t y = std::min( B - 1 , k / 64 );
 for( std::size_t j = 1 ; j <= n ; ++j ) {
  const std::uint64_t * eq =
    Peq.at( static_cast< unsigned char >( T[ j - 1 ] ) );
  detail::Carry h{ 0 , 0 };  // the top row: D[ 0 ][ j ] = 0
  for( std::size_t b = 0 ; b <= y ; ++b ) {
   detail::myersBlock( Pv[ b ] , Mv[ b ] , eq[ b ] , h );
   score[ b ] += h.p - h.n;
   }
  // the next block enters when the bottom of y is at most k + 1, so
  // that the cells of the blocks below stay above k
  if( ( y + 1 < B ) && ( score[ y ] <= k + 1 ) ) {
   ++y;
   Pv[ y ] = ~std::uint64_t( 0 );
   Mv[ y ] = 0;
   score[ y ] = score[ y - 1 ] + 64 - h.p + h.n;
   detail::myersBlock( Pv[ y ] , Mv[ y ] , eq[ y ] , h );
   score[ y ] += h.p - h.n;
   }
  else
   while( ( y > 0 ) && ( score[ y ] > k + 64 ) )
    --y;
  if( y == B - 1 ) {
   const std::size_t d = score[ B - 1 ] +
			 std::size_t( std::popcount( Mv[ B - 1 ] & mask ) ) -
			 std::size_t( std::popcount( Pv[ B - 1 ] & mask ) );
   if( d <= k )
    R.emplace_back( j , d );
   }
  }
 ADS_AUDIT( [ & ]( void ) {
  const auto DP = matchingTable( std::string( P ) , std::string( T ) );
  std::size_t r = 0;
  for( std::size_t j = 1 ; j <= n ; ++j )
   if( std::size_t( DP[ m ][ j ] ) <= k ) {
    if( ( r == R.size() ) || ( R[ r ].first != j ) ||
	( R[ r ].second != std::size_t( DP[ m ][ j ] ) ) )
     return( false );
    ++r;
    }
  return( r == R.size() );
  }() );
 return( R );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File EditDistance.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
