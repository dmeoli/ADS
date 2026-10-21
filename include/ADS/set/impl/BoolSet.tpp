/*--------------------------------------------------------------------------*/
/*---------------------------- File BoolSet.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BoolSet [see BoolSet.h], with their contracts.
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

inline BoolSet::BoolSet( long N )
{
 ADS_PRE( N >= 0 );
 f_N = std::size_t( N );
 f_W.assign( ( f_N + 63 ) / 64 , 0 );
 }

inline BoolSet::BoolSet( long N , Raw ) : f_N( std::size_t( N ) )
{
 f_W.resize( ( f_N + 63 ) / 64 );
 }

inline std::size_t BoolSet::size( void ) const
{
 return( f_size );
 }

inline bool BoolSet::empty( void ) const
{
 return( f_size == 0 );
 }

inline long BoolSet::dim( void ) const
{
 return( long( f_N ) );
 }

inline bool BoolSet::contains( long x ) const
{
 return( ( std::size_t( x ) < f_N ) &&     // a negative x is past N
         ( ( f_W[ std::size_t( x ) / 64 ] >> ( x % 64 ) ) & 1 ) );
 }

inline void BoolSet::insert( long x )
{
 ADS_PRE( ( x >= 0 ) && ( x < dim() ) );
 std::uint64_t & w = f_W[ std::size_t( x ) / 64 ];
 const std::uint64_t b = std::uint64_t( 1 ) << ( x % 64 );
 f_size += ( w & b ) ? 0 : 1;
 w |= b;
 ADS_POST( contains( x ) );
 ADS_INVARIANT( counted() );
 }

inline void BoolSet::remove( long x )
{
 if( contains( x ) ) {
  --f_size;
  f_W[ std::size_t( x ) / 64 ] &= ~( std::uint64_t( 1 ) << ( x % 64 ) );
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( counted() );
 }

inline bool BoolSet::counted( void ) const
{
 std::size_t c = 0;
 for( const std::uint64_t w : f_W )
  c += std::size_t( std::popcount( w ) );
 return( c == f_size );
 }

namespace detail {

/// the bits set in w[ 0 .. n ): with AVX2, the count of the bits of each
/// half byte looked up by a shuffle of bytes in a table of 16, and the
/// counts of the bytes summed by a sum of absolute differences, 4 words
/// at a time; elsewhere the count of the bits of each word
inline std::size_t countBits( const std::uint64_t * w , std::size_t n )
{
 std::size_t k = 0 , c = 0;
#if defined( __AVX2__ )
 const __m256i table = _mm256_setr_epi8( 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                         1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 ,
                                         0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                         1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 );
 const __m256i low = _mm256_set1_epi8( 0x0F );
 __m256i acc = _mm256_setzero_si256();
 for( ; k + 4 <= n ; k += 4 ) {
  const __m256i v = _mm256_loadu_si256(
                     reinterpret_cast< const __m256i * >( w + k ) );
  const __m256i b = _mm256_add_epi8(
   _mm256_shuffle_epi8( table , _mm256_and_si256( v , low ) ) ,
   _mm256_shuffle_epi8( table , _mm256_and_si256(
                                 _mm256_srli_epi16( v , 4 ) , low ) ) );
  acc = _mm256_add_epi64( acc , _mm256_sad_epu8( b ,
                                                 _mm256_setzero_si256() ) );
  }
 alignas( 32 ) std::uint64_t s[ 4 ];
 _mm256_store_si256( reinterpret_cast< __m256i * >( s ) , acc );
 c = std::size_t( s[ 0 ] + s[ 1 ] + s[ 2 ] + s[ 3 ] );
#endif
 for( ; k < n ; ++k )
  c += std::size_t( std::popcount( w[ k ] ) );
 return( c );
 }

/// C is A op B on every integer of the larger dimension, op one of the
/// characters | & - ^
inline bool boolSetIs( const BoolSet & C , const BoolSet & A ,
                       const BoolSet & B , char op )
{
 const long N = std::max( { A.dim() , B.dim() , C.dim() } );
 for( long i = 0 ; i < N ; ++i ) {
  const bool a = A.contains( i ) , b = B.contains( i );
  const bool c = op == '|' ? ( a || b ) : op == '&' ? ( a && b ) :
                 op == '-' ? ( a && ! b ) : ( a != b );
  if( C.contains( i ) != c )
   return( false );
  }
 return( true );
 }

}  // end( namespace detail )

inline std::optional< long > BoolSet::min( void ) const
{
 for( std::size_t k = 0 ; k < f_W.size() ; ++k )
  if( f_W[ k ] )
   return( long( k * 64 + std::size_t( std::countr_zero( f_W[ k ] ) ) ) );
 return( std::nullopt );
 }

inline std::optional< long > BoolSet::max( void ) const
{
 for( std::size_t k = f_W.size() ; k-- > 0 ; )
  if( f_W[ k ] )
   return( long( k * 64 + 63 - std::size_t( std::countl_zero( f_W[ k ] ) ) ) );
 return( std::nullopt );
 }

template< class F >
void BoolSet::forEach( F && f ) const
{
 for( std::size_t k = 0 ; k < f_W.size() ; ++k )
  for( std::uint64_t w = f_W[ k ] ; w ; w &= w - 1 )
   f( long( k * 64 + std::size_t( std::countr_zero( w ) ) ) );
 }

template< class Op >
inline void BoolSet::combine( BoolSet & C , const BoolSet & A ,
                              const BoolSet & B , Op op )
{
 const std::size_t na = A.f_W.size() , nb = B.f_W.size();
 const std::size_t w = C.f_W.size() , both = std::min( { na , nb , w } );
 const std::uint64_t * a = A.f_W.data() , * b = B.f_W.data();
 std::uint64_t * c = C.f_W.data();
 std::size_t k = 0 , n = 0;
#if defined( __AVX2__ )
 // the words of both, 4 at a time, their bits counted in the same pass as
 // in countBits()
 const __m256i table = _mm256_setr_epi8( 0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                         1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 ,
                                         0 , 1 , 1 , 2 , 1 , 2 , 2 , 3 ,
                                         1 , 2 , 2 , 3 , 2 , 3 , 3 , 4 );
 const __m256i low = _mm256_set1_epi8( 0x0F );
 __m256i acc = _mm256_setzero_si256();
 for( ; k + 4 <= both ; k += 4 ) {
  const __m256i v = op(
   _mm256_loadu_si256( reinterpret_cast< const __m256i * >( a + k ) ) ,
   _mm256_loadu_si256( reinterpret_cast< const __m256i * >( b + k ) ) );
  _mm256_storeu_si256( reinterpret_cast< __m256i * >( c + k ) , v );
  const __m256i t = _mm256_add_epi8(
   _mm256_shuffle_epi8( table , _mm256_and_si256( v , low ) ) ,
   _mm256_shuffle_epi8( table , _mm256_and_si256(
                                 _mm256_srli_epi16( v , 4 ) , low ) ) );
  acc = _mm256_add_epi64( acc , _mm256_sad_epu8( t ,
                                                 _mm256_setzero_si256() ) );
  }
 alignas( 32 ) std::uint64_t s[ 4 ];
 _mm256_store_si256( reinterpret_cast< __m256i * >( s ) , acc );
 n = std::size_t( s[ 0 ] + s[ 1 ] + s[ 2 ] + s[ 3 ] );
#endif
 for( ; k < w ; ++k ) {                     // the rest, one word at a time
  std::uint64_t x = op( k < na ? a[ k ] : 0 , k < nb ? b[ k ] : 0 );
  if( ( k + 1 == w ) && ( C.f_N % 64 ) )    // the bits past the dimension
   x &= ( std::uint64_t( 1 ) << ( C.f_N % 64 ) ) - 1;
  c[ k ] = x;
  n += std::size_t( std::popcount( x ) );
  }
 if( ( both == w ) && ( w > 0 ) && ( k == w ) && ( C.f_N % 64 ) &&
     ( w % 4 == 0 ) ) {                     // the last word was in a vector
  const std::uint64_t x = c[ w - 1 ];
  c[ w - 1 ] = x & ( ( std::uint64_t( 1 ) << ( C.f_N % 64 ) ) - 1 );
  n -= std::size_t( std::popcount( x ) - std::popcount( c[ w - 1 ] ) );
  }
 C.f_size = n;
 }

inline BoolSet setUnion( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) , BoolSet::Raw() );
 BoolSet::combine( C , A , B , []( auto a , auto b ) {
                    return( a | b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '|' ) );
 return( C );
 }

inline BoolSet setIntersection( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::min( A.dim() , B.dim() ) , BoolSet::Raw() );
 BoolSet::combine( C , A , B , []( auto a , auto b ) {
                    return( a & b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '&' ) );
 return( C );
 }

inline BoolSet setDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( A.dim() , BoolSet::Raw() );
 BoolSet::combine( C , A , B , []( auto a , auto b ) {
                    return( a & ~b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '-' ) );
 return( C );
 }

inline BoolSet setSymmetricDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) , BoolSet::Raw() );
 BoolSet::combine( C , A , B , []( auto a , auto b ) {
                    return( a ^ b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '^' ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BoolSet.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
