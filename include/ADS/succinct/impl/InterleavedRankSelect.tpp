/*--------------------------------------------------------------------------*/
/*-------------------- File InterleavedRankSelect.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of InterleavedRankSelect.h, with their contracts.
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

inline InterleavedRankSelect::InterleavedRankSelect( const BitVector & b )
{
 build( b.words().data() , b.size() );
 }

inline InterleavedRankSelect::InterleavedRankSelect(
                             std::span< const Word > w , std::size_t n )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 build( w.data() , n );
 }

inline void InterleavedRankSelect::build( const Word * w , std::size_t n )
{
 f_n = n;
 const std::size_t lines = n / kData + 1;  // the one of n too
 f_L.clear();
 f_L.resize( 8 * lines );                  // each word written below
 // the bits [ t , t + len ) of w, len <= 64 and t + len <= n
 auto read = [ & ]( std::size_t t , int len ) {
  const std::size_t q = t / 64;
  const unsigned o = unsigned( t % 64 );
  Word x = w[ q ] >> o;
  if( o + unsigned( len ) > 64 ) x |= w[ q + 1 ] << ( 64 - o );
  return( x & detail::lowMask( len ) );
  };
 // the bits of each line from its bit 16, then its count since its
 // superblock in its bits 0 to 15
 std::size_t c = 0 , next[ 2 ] = { 1 , 1 };
 const std::size_t nw = ( n + 63 ) / 64;
 for( std::size_t l = 0 ; l < lines ; ++l ) {
  Word * W = f_L.data() + 8 * l;
  const std::size_t q = l * kData / 64;
  const unsigned o = unsigned( l * kData % 64 );
  if( q + 8 < nw ) {
   // the 8 pieces from the words q to q + 8, the shift the same for all
   // (the bits past the 496 of the line are cleared below)
   Word x[ 8 ];
   for( std::size_t k = 0 ; k < 8 ; ++k )
    x[ k ] = ( w[ q + k ] >> o ) | ( ( w[ q + k + 1 ] << 1 ) << ( 63 - o ) );
   W[ 0 ] = x[ 0 ] << 16;
   for( std::size_t k = 1 ; k < 8 ; ++k )
    W[ k ] = ( x[ k ] << 16 ) | ( x[ k - 1 ] >> 48 );
   }
  else {
   std::fill( W , W + 8 , Word( 0 ) );
   for( std::size_t k = 0 ; k < 8 ; ++k ) {
    const std::size_t off = l * kData + 64 * k;
    if( off >= n ) break;
    const int len = int( std::min< std::size_t >( k < 7 ? 64 : 48 ,
                                                  n - off ) );
    const Word piece = read( off , len );
    W[ k ] |= piece << 16;
    if( k < 7 ) W[ k + 1 ] |= piece >> 48;
    }
   }
  if( l % kLines == 0 ) f_S.push_back( c );
  W[ 0 ] |= Word( c - std::size_t( f_S.back() ) );
  std::size_t in = std::size_t( std::popcount( W[ 0 ] >> 16 ) );
  for( std::size_t k = 1 ; k < 8 ; ++k )
   in += std::size_t( std::popcount( W[ k ] ) );
  // the samples: the line of every 8192-th bit b, with the bits b before
  // the line (the 0s only those before n)
  const std::size_t b1 = c , e1 = c + in , b0 = l * kData - c ,
                    e0 = std::min( n , ( l + 1 ) * kData ) - e1;
  for( ; next[ 1 ] <= e1 ; next[ 1 ] += kSample ) {
   f_sl[ 1 ].push_back( std::uint32_t( l ) );
   f_sc[ 1 ].push_back( b1 );
   }
  for( ; next[ 0 ] <= e0 ; next[ 0 ] += kSample ) {
   f_sl[ 0 ].push_back( std::uint32_t( l ) );
   f_sc[ 0 ].push_back( b0 );
   }
  c = e1;
  }
 f_ones = c;
 // the last line closes the samples
 const std::size_t last = lines - 1 , ob = before( last );
 for( const bool b : { false , true } ) {
  f_sl[ b ].push_back( std::uint32_t( last ) );
  f_sc[ b ].push_back( b ? ob : last * kData - ob );
  }
 ADS_AUDIT( consistent() );
 }

inline bool InterleavedRankSelect::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 const std::size_t p = i % kData + 16;
 return( ( f_L[ 8 * ( i / kData ) + p / 64 ] >> ( p % 64 ) ) & 1 );
 }

inline std::size_t InterleavedRankSelect::inLine( std::size_t l ,
                                                  unsigned p ) const
{
 const Word * W = f_L.data() + 8 * l;
#if defined( __AVX2__ )
 // the bytes of the line before the one of p, 32 at a time by a table of
 // the 16 nibbles, the bits of the byte of p before it, less the count
 const __m256i lut = _mm256_setr_epi8( 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 , 1 , 2 ,
                                       2 , 3 , 2 , 3 , 3 , 4 , 0 , 1 , 1 , 2 ,
                                       1 , 2 , 2 , 3 , 1 , 2 , 2 , 3 , 2 , 3 ,
                                       3 , 4 );
 const __m256i nib = _mm256_set1_epi8( 0x0F );
 const __m256i id = _mm256_setr_epi8( 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 ,
                                      10 , 11 , 12 , 13 , 14 , 15 , 16 , 17 ,
                                      18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 ,
                                      26 , 27 , 28 , 29 , 30 , 31 );
 const __m256i nb = _mm256_set1_epi8( char( p / 8 ) );
 auto bytes = [ & ]( const Word * q , __m256i ids ) {
  const __m256i v = _mm256_load_si256( reinterpret_cast< const __m256i * >(
                                        q ) );
  const __m256i hi = _mm256_and_si256( _mm256_srli_epi16( v , 4 ) , nib );
  const __m256i c = _mm256_add_epi8(
                     _mm256_shuffle_epi8( lut , _mm256_and_si256( v , nib ) ) ,
                     _mm256_shuffle_epi8( lut , hi ) );
  return( _mm256_and_si256( c , _mm256_cmpgt_epi8( nb , ids ) ) );
  };
 const __m256i s = _mm256_sad_epu8( _mm256_add_epi8(
                     bytes( W , id ) ,
                     bytes( W + 4 , _mm256_add_epi8( id ,
                                      _mm256_set1_epi8( 32 ) ) ) ) ,
                                    _mm256_setzero_si256() );
 const __m128i h = _mm_add_epi64( _mm256_castsi256_si128( s ) ,
                                  _mm256_extracti128_si256( s , 1 ) );
 std::size_t r = std::size_t( _mm_cvtsi128_si64( h ) +
                              _mm_extract_epi64( h , 1 ) );
 if( p < 512 ) {
  const unsigned char last = reinterpret_cast< const unsigned char * >( W )[
                              p / 8 ];
  r += std::size_t( std::popcount( unsigned( last & ( ( 1u << ( p % 8 ) ) -
                                                      1 ) ) ) );
  }
 return( r - std::size_t( std::popcount( W[ 0 ] & 0xFFFF ) ) );
#else
 std::size_t r = 0;
 for( unsigned j = 0 ; j < 8 ; ++j ) {
  const int c = int( std::clamp< std::int64_t >( std::int64_t( p ) -
                                                 64 * std::int64_t( j ) ,
                                                 0 , 64 ) );
  r += std::size_t( std::popcount( W[ j ] & detail::lowMask( c ) ) );
  }
 return( r - std::size_t( std::popcount( W[ 0 ] & 0xFFFF ) ) );
#endif
 }

inline std::size_t InterleavedRankSelect::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_n );
 const std::size_t l = i / kData;
 return( before( l ) + inLine( l , unsigned( i % kData + 16 ) ) );
 }

inline std::size_t InterleavedRankSelect::select( std::size_t k ,
                                                  bool b ) const
{
 auto beforeB = [ & ]( std::size_t l ) {
  const std::size_t o = before( l );
  return( b ? o : l * kData - o );
  };
 const std::size_t j = ( k - 1 ) / kSample;
 const std::size_t lo = f_sl[ b ][ j ] , hi = f_sl[ b ][ j + 1 ];
 const std::size_t clo = std::size_t( f_sc[ b ][ j ] ) ,
                   chi = std::size_t( f_sc[ b ][ j + 1 ] );
 // the guess by interpolation between the samples, then a line at a time
 std::size_t l = lo + ( chi > clo ? ( k - 1 - clo ) * ( hi - lo ) /
                                    ( chi - clo ) : 0 );
 l = std::min( l , hi );
 while( ( l > lo ) && ( beforeB( l ) >= k ) ) --l;
 while( ( l < hi ) && ( beforeB( l + 1 ) < k ) ) ++l;
 // the word of the line, and the bit in it
 std::size_t r = k - beforeB( l );
 const Word * W = f_L.data() + 8 * l;
 for( std::size_t q = 0 ; ; ++q ) {
  Word x = b ? W[ q ] : ~W[ q ];
  if( q == 0 ) x &= ~Word( 0xFFFF );
  const std::size_t c = std::size_t( std::popcount( x ) );
  if( r <= c )
   return( l * kData + 64 * q +
           std::size_t( selectInWord( x , int( r - 1 ) ) ) - 16 );
  r -= c;
  }
 }

inline std::size_t InterleavedRankSelect::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 const std::size_t p = select( k , true );
 ADS_POST( ( *this )[ p ] && ( rank1( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t InterleavedRankSelect::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 const std::size_t p = select( k , false );
 ADS_POST( ! ( *this )[ p ] && ( rank0( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t InterleavedRankSelect::directoryBits( void ) const
{
 return( 64 * f_L.size() - f_n + 64 * f_S.size() +
         96 * ( f_sl[ 0 ].size() + f_sl[ 1 ].size() ) );
 }

inline bool InterleavedRankSelect::consistent( void ) const
{
 std::size_t c = 0;
 for( std::size_t i = 0 ; i <= f_n ; ++i ) {
  if( rank1( i ) != c ) return( false );
  if( i < f_n && ( *this )[ i ] ) ++c;
  }
 return( c == f_ones );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File InterleavedRankSelect.tpp -------------------*/
/*--------------------------------------------------------------------------*/
