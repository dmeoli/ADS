/*--------------------------------------------------------------------------*/
/*-------------------------- File FastBitwise.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of FastBitwise.h, with their contracts.
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

namespace ADS
{

namespace detail
{

/// the 1 bits of the words get( 0 ) , ... , get( n - 1 ), in four sums
template< class G >
inline std::uint64_t popcountScalar( G && get , std::size_t n )
{
 std::uint64_t s0 = 0 , s1 = 0 , s2 = 0 , s3 = 0;
 const std::size_t q = n / 4;
 for( std::size_t k = 0 ; k < q ; ++k ) {
  s0 += std::uint64_t( std::popcount( get( 4 * k ) ) );
  s1 += std::uint64_t( std::popcount( get( 4 * k + 1 ) ) );
  s2 += std::uint64_t( std::popcount( get( 4 * k + 2 ) ) );
  s3 += std::uint64_t( std::popcount( get( 4 * k + 3 ) ) );
  }
 for( std::size_t i = 4 * q ; i < n ; ++i )
  s0 += std::uint64_t( std::popcount( get( i ) ) );
 return( s0 + s1 + s2 + s3 );
 }

/// the words from which popcount() and popcountAnd() call the kernels of
/// Highway, whose call through a table costs as much as a few words
inline constexpr std::size_t kVectorWords = 16;

}  // end( namespace detail )

inline std::uint64_t popcount( const Word * w , std::size_t n )
{
#ifdef ADS_WITH_HIGHWAY
 if( n >= detail::kVectorWords )  // the kernel of Highway [see Simd.h]
  return( detail::simd::popcount( w , n ) );
#endif
 return( detail::popcountScalar(
   [ w ]( std::size_t i ) { return( w[ i ] ); } , n ) );
 }

inline std::uint64_t popcountAnd( const Word * a , const Word * b ,
				  std::size_t n )
{
#ifdef ADS_WITH_HIGHWAY
 if( n >= detail::kVectorWords )
  return( detail::simd::popcountAnd( a , b , n ) );
#endif
 return( detail::popcountScalar(
   [ a , b ]( std::size_t i ) { return( a[ i ] & b[ i ] ); } , n ) );
 }

inline Word pdep( Word x , Word mask )
{
 Word r = 0;
 for( Word b = 1 ; mask ; b += b ) {  // the 1 bits of mask in order
  if( x & b )
   r |= mask & ( ~mask + 1 );
  mask &= mask - 1;
  }
 return( r );
 }

inline Word pext( Word x , Word mask )
{
 Word r = 0;
 for( Word b = 1 ; mask ; b += b ) {
  if( x & mask & ( ~mask + 1 ) )
   r |= b;
  mask &= mask - 1;
  }
 return( r );
 }

inline int select( Word x , int r )
{
 ADS_PRE( ( r >= 0 ) && ( r < std::popcount( x ) ) );
 constexpr Word L8 = 0x0101010101010101ULL , H8 = 0x8080808080808080ULL;
 Word s = x - ( ( x >> 1 ) & 0x5555555555555555ULL );
 s = ( s & 0x3333333333333333ULL ) + ( ( s >> 2 ) & 0x3333333333333333ULL );
 s = ( ( s + ( s >> 4 ) ) & 0x0F0F0F0F0F0F0F0FULL ) * L8;  // prefix sums
 const Word le = ( ( Word( r ) * L8 | H8 ) - s ) & H8;     // bytes <= r
 const int k = std::popcount( le ) * 8;  // the place of the byte
 int rr = r - int( ( ( s << 8 ) >> k ) & 0xFF );
 Word b = ( x >> k ) & 0xFF;
 while( rr-- > 0 )
  b &= b - 1;
 return( k + std::countr_zero( b ) );
 }

inline Word mortonEncode( std::uint32_t x , std::uint32_t y )
{
 return( interleave( x , y ) );
 }

inline std::pair< std::uint32_t , std::uint32_t > mortonDecode( Word z )
{
 return( deinterleave( z ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File FastBitwise.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
