/*--------------------------------------------------------------------------*/
/*----------------------- File CompactRankSelect.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of CompactRankSelect.h, with their contracts.
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

inline CompactRankSelect::CompactRankSelect( BitVector b )
 : f_b( std::move( b ) )
{
 directory( nullptr );
 }

inline CompactRankSelect::CompactRankSelect( std::span< const Word > w ,
                                             std::size_t n )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 if( n < kFuse ) {                         // a copy, then a count in cache
  f_b = BitVector( w , n );
  directory( nullptr );
  return;
  }
 f_b.f_n = n;
 f_b.f_w.clear();
 f_b.f_w.resize( BitVector::wordsFor( n ) );   // written by directory()
 directory( w.data() );
 }

inline void CompactRankSelect::directory( const Word * w )
{
 const std::size_t n = f_b.size() , ns = n / kSuper + 1;   // and one more
 Word * const P = f_b.f_w.data();
 const std::size_t nw = f_b.f_w.size();     // a multiple of 8
 f_e.resize( ns + 1 );
 f_top.assign( ( ( ( ns + 1 ) * kSuper ) >> kTop ) + 1 , 0 );
 for( auto & S : f_s ) {                    // at most n / kSample + 2 each
  S.clear();
  S.reserve( n / kSample + 2 );
  }
 constexpr std::size_t kTopSupers = ( std::size_t( 1 ) << kTop ) / kSuper;
 constexpr std::size_t kW = kSuper / 64;    // the words of a superblock
 std::uint64_t c = 0 , top = 0;
 std::size_t next1 = 1 , next0 = 1;         // the next samples
 // the entry of the superblock s from its words Q (copied to the bits if
 // Copy): the 1s before the superblock since its top region in 32 bits
 // and those before each block since the superblock in 12 each; the
 // samples whose bit falls in it (the 0s only those before n)
 auto super = [ & ]< bool Copy >( std::size_t s , const Word * Q ) {
  if( s % kTopSupers == 0 ) f_top[ s / kTopSupers ] = top = c;
  std::uint64_t v[ 9 ];                     // the 1s before each block
  v[ 0 ] = 0;
  for( std::size_t k = 0 ; k < 8 ; ++k ) {
   std::uint64_t in = 0;
   for( std::size_t j = 0 ; j < 8 ; ++j ) {
    const Word x = Q[ 8 * k + j ];
    if constexpr( Copy ) P[ kW * s + 8 * k + j ] = x;
    in += std::uint64_t( std::popcount( x ) );
    }
   v[ k + 1 ] = v[ k ] + in;
   }
  // the count of block k at the bit 32 + 12 k of ( hi , lo )
  f_e[ s ] = Entry{ ( c - top ) | ( v[ 1 ] << 44 ) | ( v[ 2 ] << 56 ) ,
                    ( v[ 2 ] >> 8 ) | ( v[ 3 ] << 4 ) | ( v[ 4 ] << 16 ) |
                    ( v[ 5 ] << 28 ) | ( v[ 6 ] << 40 ) | ( v[ 7 ] << 52 ) };
  c += v[ 8 ];
  if( s < ns ) {
   const std::size_t end1 = std::size_t( c ) ,
                     end0 = std::min( n , ( s + 1 ) * kSuper ) - end1;
   for( ; next1 <= end1 ; next1 += kSample )
    f_s[ 1 ].push_back( std::uint32_t( s ) );
   for( ; next0 <= end0 ; next0 += kSample )
    f_s[ 0 ].push_back( std::uint32_t( s ) );
   }
  };
 std::size_t s = 0;
 if( w ) {
  // the superblocks of whole words of w copied as they are counted, then
  // the rest of w, its last word cut at n, and 0s up to the end
  const std::size_t sw = ( n + 63 ) / 64;
  for( ; kW * ( s + 1 ) <= n / 64 ; ++s )
   super.template operator()< true >( s , w + kW * s );
  for( std::size_t i = kW * s ; i < nw ; ++i ) P[ i ] = i < sw ? w[ i ] : 0;
  if( n % 64 ) P[ n / 64 ] &= detail::lowMask( int( n % 64 ) );
  }
 // the superblocks whose words exist, then the rest from a line of 0s
 for( ; ( s <= ns ) && ( kW * ( s + 1 ) <= nw ) ; ++s )
  super.template operator()< false >( s , P + kW * s );
 for( ; s <= ns ; ++s ) {
  std::array< Word , kW > Z{};
  for( std::size_t i = kW * s ; i < nw ; ++i ) Z[ i - kW * s ] = P[ i ];
  super.template operator()< false >( s , Z.data() );
  }
 f_ones = std::size_t( c );
 // the superblock of the last bit b closes the samples
 for( const bool v : { false , true } )
  if( ! f_s[ v ].empty() ) {
   const std::size_t total = v ? f_ones : zeros();
   std::size_t t = f_s[ v ].back();
   while( ( t + 1 < ns ) && ( before( t + 1 , v ) < total ) ) ++t;
   f_s[ v ].push_back( std::uint32_t( t ) );
   }
 ADS_INVARIANT( f_b.clean() );
 ADS_AUDIT( consistent() );
 }

inline std::size_t CompactRankSelect::before( std::size_t s , bool b ) const
{
 const std::size_t o = std::size_t( f_top[ ( s * kSuper ) >> kTop ] +
                                    ( f_e[ s ].lo & 0xFFFFFFFFULL ) );
 return( b ? o : s * kSuper - o );
 }

inline std::size_t CompactRankSelect::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_b.size() );
 const Entry & e = f_e[ i / kSuper ];
 std::size_t r = std::size_t( f_top[ i >> kTop ] +
                              ( e.lo & 0xFFFFFFFFULL ) ) +
                 inSuper( e , ( i / kBlock ) % 8 );
 // the 8 words of the line of the block, each cut at i, with no branch
 // (the words exist past the size [see BitVector.h])
 const Word * w = f_b.words().data() + 8 * ( i / kBlock );
#if defined( __AVX2__ ) && ! defined( ADS_COMPACT_SCALAR )
 // the bytes of the line before the one of i, counted 32 at a time by a
 // table of the 16 nibbles and summed by the absolute differences with 0,
 // and the bits of the byte of i before it
 const unsigned o = unsigned( i % kBlock );
 const __m256i lut = _mm256_setr_epi8( 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 , 1 , 2 ,
                                       2 , 3 , 2 , 3 , 3 , 4 , 0 , 1 , 1 , 2 ,
                                       1 , 2 , 2 , 3 , 1 , 2 , 2 , 3 , 2 , 3 ,
                                       3 , 4 );
 const __m256i nib = _mm256_set1_epi8( 0x0F );
 const __m256i id = _mm256_setr_epi8( 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 ,
                                      10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 ,
                                      18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 ,
                                      26 , 27 , 28 , 29 , 30 , 31 );
 const __m256i nb = _mm256_set1_epi8( char( o / 8 ) );
 auto bytes = [ & ]( const Word * p , __m256i ids ) {
  const __m256i v = _mm256_load_si256( reinterpret_cast< const __m256i * >(
                                        p ) );
  const __m256i hi = _mm256_and_si256( _mm256_srli_epi16( v , 4 ) , nib );
  const __m256i c = _mm256_add_epi8(
                     _mm256_shuffle_epi8( lut , _mm256_and_si256( v , nib ) ) ,
                     _mm256_shuffle_epi8( lut , hi ) );
  return( _mm256_and_si256( c , _mm256_cmpgt_epi8( nb , ids ) ) );
  };
 const __m256i s = _mm256_sad_epu8( _mm256_add_epi8(
                     bytes( w , id ) ,
                     bytes( w + 4 , _mm256_add_epi8( id ,
                                      _mm256_set1_epi8( 32 ) ) ) ) ,
                                    _mm256_setzero_si256() );
 const __m128i h = _mm_add_epi64( _mm256_castsi256_si128( s ) ,
                                  _mm256_extracti128_si256( s , 1 ) );
 r += std::size_t( _mm_cvtsi128_si64( h ) + _mm_extract_epi64( h , 1 ) );
 const unsigned char last = reinterpret_cast< const unsigned char * >( w )[
                             o / 8 ];
 return( r + std::size_t( std::popcount( unsigned(
                           last & ( ( 1u << ( o % 8 ) ) - 1 ) ) ) ) );
#else
 const std::int64_t o = std::int64_t( i % kBlock );
 for( std::size_t j = 0 ; j < 8 ; ++j ) {
  const auto c = unsigned( std::clamp< std::int64_t >(
                            o - std::int64_t( 64 * j ) , 0 , 64 ) );
#ifdef __BMI2__
  r += std::size_t( std::popcount( _bzhi_u64( w[ j ] , c ) ) );
#else
  r += std::size_t( std::popcount( c == 64 ? w[ j ] :
                                   w[ j ] & detail::lowMask( int( c ) ) ) );
#endif
  }
 return( r );
#endif
 }

inline std::size_t CompactRankSelect::select( std::size_t k , bool b ) const
{
 const auto & S = f_s[ b ];
 const std::size_t j = ( k - 1 ) / kSample;
 std::size_t lo = S[ j ] , hi = S[ j + 1 ];
 while( lo < hi ) {              // the last superblock with fewer than k
  const std::size_t mid = lo + ( hi - lo + 1 ) / 2;
  if( before( mid , b ) < k ) lo = mid;
  else hi = mid - 1;
  }
 std::size_t r = k - before( lo , b );      // the r-th of the superblock
 // the block, by the counts of the blocks 1 to 7, and its count
 const Entry & e = f_e[ lo ];
 std::size_t q = 0 , in = 0;
 for( std::size_t h = 1 ; h < 8 ; ++h ) {
  const std::size_t c1 = inSuper( e , h );
  const std::size_t c = b ? c1 : h * kBlock - c1;
  q = c < r ? h : q;
  in = c < r ? c : in;
  }
 r -= in;
 // the word, by the sums of the 8 words of the line
 const Word * w = f_b.words().data() + ( kSuper / 64 ) * lo + 8 * q;
 std::size_t p = 0 , sum = 0 , sp = 0;    // sp: the bits b before word p
 for( std::size_t h = 0 ; h < 7 ; ++h ) {
  sum += std::size_t( std::popcount( b ? w[ h ] : ~w[ h ] ) );
  const bool past = sum < r;
  p += past;
  sp = past ? sum : sp;
  }
 return( 64 * ( ( kSuper / 64 ) * lo + 8 * q + p ) +
         std::size_t( selectInWord( b ? w[ p ] : ~w[ p ] ,
                                    int( r - sp - 1 ) ) ) );
 }

inline std::size_t CompactRankSelect::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 const std::size_t p = select( k , true );
 ADS_POST( f_b[ p ] && ( rank1( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t CompactRankSelect::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 const std::size_t p = select( k , false );
 ADS_POST( ! f_b[ p ] && ( rank0( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t CompactRankSelect::directoryBits( void ) const
{
 return( 128 * f_e.size() + 64 * f_top.size() +
         32 * ( f_s[ 0 ].size() + f_s[ 1 ].size() ) );
 }

inline bool CompactRankSelect::consistent( void ) const
{
 std::size_t c = 0;
 for( std::size_t i = 0 ; i <= f_b.size() ; ++i ) {
  if( rank1( i ) != c ) return( false );
  if( i < f_b.size() && f_b[ i ] ) ++c;
  }
 return( c == f_ones );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File CompactRankSelect.tpp --------------------*/
/*--------------------------------------------------------------------------*/
