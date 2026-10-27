/*--------------------------------------------------------------------------*/
/*-------------------------- File LearnedSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The learned sort of LearnedSort.h: the model of the CDF, its test of
 * fit on the sample, the distribution and the sorts of the buckets.
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

/// the items, \f$ b \f$, that the buckets of `learnedSortIf()` aim at
inline constexpr std::size_t kLearnedBucket = 4096;

/// the cells of the grid of `CdfModel` per bucket, as a logarithm, at the
/// least and at the most: from \f$ 2^2 \f$ to \f$ 2^6 \f$
inline constexpr int kCellBits = 2 , kCellBitsMax = 6;

/// the splitters of a cell of `CdfModel` at the most, \f$ c_{\max} \f$,
/// which the search compares with no branch
inline constexpr std::size_t kCellSplitters = 4;

/// the model of the CDF of the keys `U` of a sample: \f$ k - 1 \f$ splitters
/// at the quantiles \f$ i / k \f$ of the sorted sample, kept once each, and
/// a grid of cells on the range of the sample, each cell the place of its
/// first splitter; the bucket of a key is the number of the splitters less
/// than it, i.e., the place of the first splitter of its cell plus the
/// splitters of the cell less than it, \f$ c_{\max} \f$ comparisons with
/// no branch (the splitters of the cells after it are larger than the key,
/// and the splitters end with \f$ c_{\max} \f$ copies of the largest key);
/// if the sample repeats a splitter, each splitter has the bucket of the
/// keys equal to it. The grid is the finest of \f$ 4 \f$ to \f$ 64 \f$
/// cells per bucket that holds at most \f$ c_{\max} \f$ splitters per
/// cell; with none, the model does not fit
template< class U >
class CdfModel {
 public:

 /// the model of the sorted keys \f$ S[ 0 .. s ) \f$, \f$ s \ge 2 \f$,
 /// for \f$ k \f$ buckets, \f$ k \ge 2 \f$ a power of 2
 CdfModel( const U * S , std::size_t s , std::size_t k ) {
  f_sp.reserve( k - 1 + kCellSplitters );
  for( std::size_t i = 1 ; i < k ; ++i ) {
   const U x = S[ i * s / k ];
   if( f_sp.empty() || ( f_sp.back() < x ) )
    f_sp.push_back( x );
   else
    f_eq = true;
   }
  f_ns = f_sp.size();
  f_lo = S[ 0 ];
  const U range = U( S[ s - 1 ] - S[ 0 ] );
  for( int g = std::countr_zero( k ) + kCellBits ;
       g <= std::countr_zero( k ) + kCellBitsMax ; ++g ) {
   f_shift = std::max( 0 , int( std::bit_width( range ) ) - g );
   f_last = std::size_t( range >> f_shift );
   f_first.assign( f_last + 2 , 0 );
   for( const U x : f_sp )
    ++f_first[ cell( x ) + 1 ];
   std::size_t most = 0;
   for( std::size_t c = 0 ; c <= f_last ; ++c ) {
    most = std::max< std::size_t >( most , f_first[ c + 1 ] );
    f_first[ c + 1 ] += f_first[ c ];
    }
   if( most <= kCellSplitters ) {
    f_fit = true;
    break;
    }
   if( f_shift == 0 )
    break;
   }
  f_sp.resize( f_ns + kCellSplitters , U( ~U( 0 ) ) );
  }

 /// whether the grid holds at most \f$ c_{\max} \f$ splitters per cell
 [[nodiscard]] bool fits( void ) const { return( f_fit ); }

 /// the number of the splitters, distinct
 [[nodiscard]] std::size_t splitters( void ) const { return( f_ns ); }

 /// whether the buckets of equality are there
 [[nodiscard]] bool equality( void ) const { return( f_eq ); }

 /// the number of the buckets, \f$ 2 k' + 1 \f$ with the buckets of
 /// equality and \f$ k' + 1 \f$ without, \f$ k' \f$ the splitters
 [[nodiscard]] std::size_t buckets( void ) const {
  return( f_eq ? 2 * f_ns + 1 : f_ns + 1 );
  }

 /// whether the bucket \f$ b \f$ holds equal keys only
 [[nodiscard]] bool equal( std::size_t b ) const {
  return( f_eq && ( b & 1 ) );
  }

 /// the cell of the key \f$ u \f$, i.e.,
 /// \f$ \lfloor ( u - S_{\min} ) / w \rfloor \f$, the keys out of the
 /// range of the sample in the cells at its ends
 [[nodiscard]] std::size_t cell( U u ) const {
  const std::size_t c = std::size_t( U( u - f_lo ) >> f_shift );
  return( u < f_lo ? 0 : std::min( c , f_last ) );
  }

 /// the bucket of the key \f$ u \f$
 [[nodiscard]] std::size_t operator()( U u ) const {
  const std::size_t f = f_first[ cell( u ) ];
  const U * const p = f_sp.data() + f;
  std::size_t b = f;
  for( std::size_t t = 0 ; t < kCellSplitters ; ++t )
   b += std::size_t( p[ t ] < u );
  if( ! f_eq )
   return( b );
  return( 2 * b + std::size_t( ( b < f_ns ) & ( f_sp[ b ] == u ) ) );
  }

 private:

 std::vector< U > f_sp;                    ///< the splitters, padded
 std::vector< std::uint32_t > f_first;     ///< the first splitter of each
                                           ///< cell, and their end
 U f_lo = 0;                               ///< \f$ S_{\min} \f$
 int f_shift = 0;                          ///< \f$ \log_2 w \f$
 std::size_t f_last = 0;                   ///< the last cell
 std::size_t f_ns = 0;                     ///< the splitters, not padded
 bool f_eq = false;                        ///< a splitter repeated
 bool f_fit = false;                       ///< whether the model fits
 };

/// the keys of `radixKey()` of a sample of \f$ s \f$ items of \f$ A \f$ at
/// random places, sorted
template< VectorKey T >
auto learnedSample( const std::vector< T > & A , std::size_t s )
{
 using U = decltype( radixKey( T() ) );
 const std::size_t n = A.size();
 std::vector< U > S( s );
 std::uint64_t r = 0x9E3779B97F4A7C15ull ^ n;
 for( std::size_t i = 0 ; i < s ; ++i ) {
  r ^= r << 13;
  r ^= r >> 7;
  r ^= r << 17;
  S[ i ] = radixKey( A[ r % n ] );
  }
 vectorQuickSort( S );
 return( S );
 }

template< VectorKey T >
bool learnedSortIf( std::vector< T > & A )
{
 using U = decltype( radixKey( T() ) );
 const std::size_t n = A.size();
 if( n < 4 * kLearnedBucket )
  return( false );
 const std::size_t k = std::clamp< std::size_t >(
                        std::bit_floor( n / kLearnedBucket ) , 64 , 2048 );
 const auto S = learnedSample( A , std::min( n , 16 * k ) );
 const CdfModel< U > M( S.data() , S.size() , k );
 if( ( M.splitters() < k / 2 ) || ! M.fits() )
  return( false );
 const auto before = auditCopy( A );
 const std::size_t K = M.buckets();
 T * const a = A.data();
 // the bucket of each item, and the counts
 const auto id = std::make_unique_for_overwrite< std::uint16_t[] >( n );
 std::vector< std::size_t > start( K + 1 , 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t b = M( radixKey( a[ i ] ) );
  id[ i ] = std::uint16_t( b );
  ++start[ b + 1 ];
  }
 for( std::size_t b = 0 ; b < K ; ++b )
  start[ b + 1 ] += start[ b ];
 // the items into a buffer by buckets, through a line of the cache per
 // bucket, written out whole
 const auto buf = std::make_unique_for_overwrite< T[] >( n );
 T * const B = buf.get();
 {
  constexpr std::size_t L = 64 / sizeof( T );
  std::vector< std::size_t > next( start.begin() , start.end() - 1 );
  std::vector< std::array< T , L > > line( K );
  std::vector< std::uint8_t > fill( K , 0 );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::size_t b = id[ i ];
   line[ b ][ fill[ b ] ] = a[ i ];
   if( ++fill[ b ] == L ) {
    std::memcpy( B + next[ b ] , line[ b ].data() , 64 );
    next[ b ] += L;
    fill[ b ] = 0;
    }
   }
  for( std::size_t b = 0 ; b < K ; ++b )
   std::memcpy( B + next[ b ] , line[ b ].data() , fill[ b ] * sizeof( T ) );
 }
 // each bucket back to its place, and sorted there
 for( std::size_t b = 0 ; b < K ; ++b ) {
  const std::size_t s = start[ b ] , m = start[ b + 1 ] - s;
  std::memcpy( a + s , B + s , m * sizeof( T ) );
  if( ( m > 1 ) && ! M.equal( b ) )
   vectorQuickSort( a + s , m );
  }
 ADS_AUDIT( sortedPermutation( A , before , []( T x , T y ) {
  return( radixKey( x ) < radixKey( y ) ); } ) );
 return( true );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< VectorKey T >
void learnedSort( std::vector< T > & A )
{
 if( ! detail::learnedSortIf( A ) )
  sampleSort( A , []( T x , T y ) {        // the total order of the
   return( radixKey( x ) < radixKey( y ) ); } );   // keys, as the model's
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File LearnedSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
