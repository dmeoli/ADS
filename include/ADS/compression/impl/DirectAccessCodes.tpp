/*--------------------------------------------------------------------------*/
/*----------------------- File DirectAccessCodes.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of DirectAccessCodes.h, with their contracts.
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

inline DirectAccessCodes::DirectAccessCodes(
                       std::span< const std::uint64_t > x ) : f_n( x.size() )
{
 if( f_n == 0 ) return;
 // n_s, the integers of more than s bits ( n_0 = n , each one has a chunk
 // at the first level ), and w, the bits of the largest one
 std::array< std::size_t , 66 > more{};
 for( const auto v : x ) ++more[ std::size_t( std::bit_width( v ) ) ];
 int w = 64;
 while( ( w > 1 ) && ( more[ std::size_t( w ) ] == 0 ) ) --w;
 for( int s = 63 ; s >= 0 ; --s ) more[ s ] += more[ s + 1 ];
 std::array< double , 65 > C{};
 std::array< int , 65 > cut{};
 auto users = [ & ]( int s ) {
  return( double( s == 0 ? f_n : more[ std::size_t( s ) + 1 ] ) );
  };
 for( int s = w - 1 ; s >= 0 ; --s ) {     // the program of the widths
  C[ s ] = -1;
  for( int t = s + 1 ; t <= w ; ++t ) {
   const double c = users( s ) * ( t - s ) +
                    ( t < w ? users( s ) * 1.25 : 0.0 ) + C[ t ];
   if( ( C[ s ] < 0 ) || ( c < C[ s ] ) ) {
    C[ s ] = c;
    cut[ s ] = t;
    }
   }
  }
 build( x , cut , w );
 }

inline DirectAccessCodes::DirectAccessCodes(
                       std::span< const std::uint64_t > x , int b )
 : f_n( x.size() )
{
 ADS_PRE( ( b >= 1 ) && ( b <= 64 ) );
 if( f_n == 0 ) return;
 std::uint64_t m = 0;
 for( const auto v : x ) m |= v;
 const int w = std::max( 1 , int( std::bit_width( m ) ) );
 std::array< int , 65 > cut{};
 for( int s = 0 ; s < w ; s += b ) cut[ s ] = std::min( w , s + b );
 build( x , cut , w );
 }

inline void DirectAccessCodes::build( std::span< const std::uint64_t > x ,
                                      const std::array< int , 65 > & cut ,
                                      int w )
{
 // the levels: the chunks of the integers that reach a level and the bits
 // of those that continue, each packed in order into words by a field of
 // 64 bits, then the integers that continue moved to the next level
 std::vector< std::uint64_t > cur , next;
 std::vector< Word > C , B;
 std::span< const std::uint64_t > in = x;
 for( int s = 0 ; s < w ; s = cut[ s ] ) {
  Level L;
  L.shift = s;
  L.width = cut[ s ] - s;
  const bool last = cut[ s ] == w;
  const std::size_t m = in.size();
  const int b = L.width;
  const std::uint64_t mask = detail::lowMask( b );
  C.assign( ( m * std::size_t( b ) + 63 ) / 64 + 1 , 0 );
  B.assign( last ? 0 : m / 64 + 1 , 0 );
  next.clear();
  next.reserve( last ? 0 : m );
  std::size_t at = 0;                       // the bit of the next chunk
  for( std::size_t i = 0 ; i < m ; ++i , at += std::size_t( b ) ) {
   const std::uint64_t v = in[ i ] >> s , c = v & mask;
   const std::size_t q = at / 64;
   const unsigned o = unsigned( at % 64 );
   C[ q ] |= c << o;
   if( o + unsigned( b ) > 64 ) C[ q + 1 ] |= c >> ( 64 - o );
   if( ! last ) {
    const bool more = ( v >> b ) != 0;
    B[ i / 64 ] |= Word( more ) << ( i % 64 );
    if( more ) next.push_back( in[ i ] );
    }
   }
  L.chunks = BitVector( std::span< const Word >( C ) , m * std::size_t( b ) );
  if( ! last ) L.more = RankSelect( std::span< const Word >( B ) , m ,
                                    RankSelect::Selects::NONE );
  f_lev.push_back( std::move( L ) );
  cur.swap( next );
  in = cur;
  }
 ADS_AUDIT( std::equal( x.begin() , x.end() , begin() ) );
 }

inline std::uint64_t DirectAccessCodes::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::uint64_t v = 0;
 const std::size_t last = f_lev.size() - 1;
 for( std::size_t l = 0 ; ; ++l ) {
  const Level & L = f_lev[ l ];
  // a chunk of 8 or 16 bits is its byte or its pair of bytes (the chunks
  // from the bit 0 of the words, little-endian)
  std::uint64_t c;
  if( ( std::endian::native == std::endian::little ) && ( L.width == 8 ) )
   c = reinterpret_cast< const std::uint8_t * >(
        L.chunks.words().data() )[ i ];
  else if( ( std::endian::native == std::endian::little ) &&
           ( L.width == 16 ) ) {
   std::uint16_t h;
   std::memcpy( &h , reinterpret_cast< const std::uint8_t * >(
                      L.chunks.words().data() ) + 2 * i , 2 );
   c = h;
   }
  else c = L.chunks.get( i * std::size_t( L.width ) , L.width );
  v |= c << L.shift;
  if( ( l == last ) || ! L.more[ i ] ) break;
  i = L.more.rank1( i );
  }
 return( v );
 }

inline std::vector< int > DirectAccessCodes::widths( void ) const
{
 std::vector< int > w;
 for( const auto & L : f_lev ) w.push_back( L.width );
 return( w );
 }

inline std::size_t DirectAccessCodes::sizeInBits( void ) const
{
 std::size_t s = 0;
 for( const auto & L : f_lev )
  s += L.chunks.size() + L.more.size() + L.more.directoryBits();
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File DirectAccessCodes.tpp --------------------*/
/*--------------------------------------------------------------------------*/
