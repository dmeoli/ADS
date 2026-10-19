/*--------------------------------------------------------------------------*/
/*--------------------------- File BitStream.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BitStream.h, with their contracts.
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

namespace detail {

/// the len bits of v in the reverse order: the bits of each byte turned
/// around in 3 rounds of masks, then the bytes by a swap of the processor
/// (the 5 rounds and the swap of the halves of reverseBits() [see
/// Bitwise.h] in a constant expression)
constexpr Word reverseField( Word v , int len )
{
 if( len == 0 ) return( 0 );
#if defined( __GNUC__ ) || defined( __clang__ )
 if( ! std::is_constant_evaluated() ) {
  v = ( ( v >> 1 ) & 0x5555555555555555ULL ) |
      ( ( v & 0x5555555555555555ULL ) << 1 );
  v = ( ( v >> 2 ) & 0x3333333333333333ULL ) |
      ( ( v & 0x3333333333333333ULL ) << 2 );
  v = ( ( v >> 4 ) & 0x0F0F0F0F0F0F0F0FULL ) |
      ( ( v & 0x0F0F0F0F0F0F0F0FULL ) << 4 );
  return( __builtin_bswap64( v ) >> ( 64 - len ) );
  }
#endif
 return( reverseBits( v ) >> ( 64 - len ) );
 }

}  // end( namespace detail )

inline std::size_t BitWriter::size( void ) const
{
 return( f_b.size() );
 }

inline void BitWriter::writeBit( bool b )
{
 f_b.push_back( b );
 }

inline void BitWriter::write( Word v , int len )
{
 f_b.append( v , len );
 }

inline void BitWriter::writeBinary( Word v , int len )
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( ( len == 64 ) || ( ( v >> len ) == 0 ) );
 f_b.append( detail::reverseField( v , len ) , len );
 }

inline void BitWriter::writeUnary( std::uint64_t q )
{
 for( ; q >= 64 ; q -= 64 ) f_b.append( 0 , 64 );
 f_b.append( Word( 1 ) << q , int( q ) + 1 );
 }

inline const BitVector & BitWriter::bits( void ) const
{
 return( f_b );
 }

inline BitVector BitWriter::release( void )
{
 BitVector b = std::move( f_b );
 f_b.clear();
 return( b );
 }

inline BitReader::BitReader( const BitVector & b , std::size_t p )
 : f_b( &b ) , f_p( p )
{
 ADS_PRE( p <= b.size() );
 }

inline std::size_t BitReader::position( void ) const
{
 return( f_p );
 }

inline void BitReader::seek( std::size_t p )
{
 ADS_PRE( p <= f_b->size() );
 f_p = p;
 }

inline bool BitReader::atEnd( void ) const
{
 return( f_p == f_b->size() );
 }

inline bool BitReader::readBit( void )
{
 ADS_PRE( ! atEnd() );
 return( ( *f_b )[ f_p++ ] );
 }

inline Word BitReader::read( int len )
{
 ADS_PRE( ( len >= 0 ) && ( len <= 64 ) );
 ADS_PRE( f_p + std::size_t( len ) <= f_b->size() );
 const Word v = f_b->get( f_p , len );
 f_p += std::size_t( len );
 return( v );
 }

inline Word BitReader::readBinary( int len )
{
 return( detail::reverseField( read( len ) , len ) );
 }

inline std::uint64_t BitReader::readUnary( void )
{
 std::uint64_t q = 0;
 for( ; ; ) {
  ADS_PRE( ! atEnd() );
  const std::size_t left = f_b->size() - f_p;
  const int len = left < 64 ? int( left ) : 64;
  const Word w = f_b->get( f_p , len );
  if( w ) {                                 // the 1 is in this word
   const int z = std::countr_zero( w );
   f_p += std::size_t( z ) + 1;
   return( q + std::uint64_t( z ) );
   }
  q += std::uint64_t( len );
  f_p += std::size_t( len );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File BitStream.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
