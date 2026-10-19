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
                    ( t < w ? users( s ) * 1.035 : 0.0 ) + C[ t ];
   if( ( C[ s ] < 0 ) || ( c < C[ s ] ) ) {
    C[ s ] = c;
    cut[ s ] = t;
    }
   }
  }
 // the levels, the integers that continue moved to the next one in order
 std::vector< std::uint64_t > cur( x.begin() , x.end() ) , next;
 for( int s = 0 ; s < w ; s = cut[ s ] ) {
  Level L;
  L.shift = s;
  L.width = cut[ s ] - s;
  const bool last = cut[ s ] == w;
  L.chunks = BitVector( cur.size() * std::size_t( L.width ) );
  BitVector B( last ? 0 : cur.size() );
  next.clear();
  for( std::size_t i = 0 ; i < cur.size() ; ++i ) {
   const std::uint64_t v = cur[ i ] >> s;
   L.chunks.put( i * std::size_t( L.width ) ,
                 v & detail::lowMask( L.width ) , L.width );
   if( ! last && ( v >> L.width ) ) {
    B.set( i );
    next.push_back( cur[ i ] );
    }
   }
  if( ! last ) L.more = CompactRankSelect( std::move( B ) );
  f_lev.push_back( std::move( L ) );
  cur.swap( next );
  }
 ADS_AUDIT( std::equal( x.begin() , x.end() , begin() ) );
 }

inline std::uint64_t DirectAccessCodes::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::uint64_t v = 0;
 for( std::size_t l = 0 ; ; ++l ) {
  const Level & L = f_lev[ l ];
  v |= L.chunks.get( i * std::size_t( L.width ) , L.width ) << L.shift;
  if( ( l + 1 == f_lev.size() ) || ! L.more[ i ] ) break;
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
