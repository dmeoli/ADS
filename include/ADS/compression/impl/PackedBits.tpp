/*--------------------------------------------------------------------------*/
/*-------------------------- File PackedBits.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PackedBitWriter and PackedBitReader [see
 * PackedBits.h], with their contracts.
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

inline void PackedBitWriter::put( bool b )
{
 if( f_size % 8 == 0 ) f_bytes.push_back( 0 );
 if( b ) f_bytes.back() |= std::uint8_t( 0x80u >> ( f_size % 8 ) );
 ++f_size;
 }

inline void PackedBitWriter::put( std::uint64_t v , int k )
{
 ADS_PRE( ( k >= 0 ) && ( k <= 64 ) );
 ADS_PRE( ( k == 64 ) || ( ( v >> k ) == 0 ) );
 for( int i = k ; i-- > 0 ; )
  put( ( ( v >> i ) & 1u ) != 0 );
 }

inline void PackedBitWriter::put( const std::string & s )
{
 for( char c : s ) {
  ADS_PRE( ( c == '0' ) || ( c == '1' ) );
  put( c == '1' );
  }
 }

inline bool PackedBitReader::get( void )
{
 const std::size_t i = f_pos++;
 if( i / 8 >= f_bytes.size() ) return( false );
 return( ( ( f_bytes[ i / 8 ] >> ( 7 - i % 8 ) ) & 1u ) != 0 );
 }

inline std::uint64_t PackedBitReader::get( int k )
{
 ADS_PRE( ( k >= 0 ) && ( k <= 64 ) );
 std::uint64_t v = 0;
 for( int i = 0 ; i < k ; ++i )
  v = ( v << 1 ) | std::uint64_t( get() );
 return( v );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File PackedBits.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
