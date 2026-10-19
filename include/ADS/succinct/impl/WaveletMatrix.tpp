/*--------------------------------------------------------------------------*/
/*------------------------- File WaveletMatrix.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of WaveletMatrix.h, with their contracts.
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

inline WaveletMatrix::WaveletMatrix( std::span< const std::uint64_t > x ,
                                     int w ) : f_n( x.size() )
{
 ADS_PRE( ( w >= 0 ) && ( w <= 64 ) );
 if( w == 0 ) {
  std::uint64_t m = 0;
  for( const auto v : x ) m |= v;
  w = std::max( 1 , int( std::bit_width( m ) ) );
  }
 ADS_PRE( ( w == 64 ) || std::all_of( x.begin() , x.end() ,
                                      [ w ]( std::uint64_t v ) {
                                       return( ( v >> w ) == 0 ); } ) );
 f_lev.reserve( std::size_t( w ) );
 f_zeros.reserve( std::size_t( w ) );
 std::vector< std::uint64_t > cur( x.begin() , x.end() ) , next( f_n );
 for( int l = 0 ; l < w ; ++l ) {
  const int sh = w - 1 - l;
  BitVector B( f_n );
  std::size_t z = 0;
  for( std::size_t i = 0 ; i < f_n ; ++i )
   if( ( cur[ i ] >> sh ) & 1 ) B.set( i );
   else ++z;
  // the stable partition: the 0s first, then the 1s
  std::size_t p0 = 0 , p1 = z;
  for( std::size_t i = 0 ; i < f_n ; ++i )
   next[ ( ( cur[ i ] >> sh ) & 1 ) ? p1++ : p0++ ] = cur[ i ];
  cur.swap( next );
  f_lev.emplace_back( std::move( B ) );
  f_zeros.push_back( z );
  }
 ADS_AUDIT( std::equal( x.begin() , x.end() , begin() ) );
 }

inline std::uint64_t WaveletMatrix::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 std::uint64_t c = 0;
 for( std::size_t l = 0 ; l < f_lev.size() ; ++l ) {
  const RankSelect & B = f_lev[ l ];
  const bool b = B[ i ];
  c = ( c << 1 ) | b;
  i = b ? f_zeros[ l ] + B.rank1( i ) : B.rank0( i );
  }
 return( c );
 }

inline std::size_t WaveletMatrix::start( std::uint64_t c ) const
{
 std::size_t s = 0;
 for( std::size_t l = 0 ; l < f_lev.size() ; ++l )
  s = bit( c , l ) ? f_zeros[ l ] + f_lev[ l ].rank1( s )
                   : f_lev[ l ].rank0( s );
 return( s );
 }

inline std::size_t WaveletMatrix::rank( std::uint64_t c ,
                                        std::size_t i ) const
{
 ADS_PRE( i <= f_n );
 if( ( f_lev.size() < 64 ) && ( c >> f_lev.size() ) ) return( 0 );
 std::size_t s = 0;
 for( std::size_t l = 0 ; l < f_lev.size() ; ++l ) {
  const RankSelect & B = f_lev[ l ];
  if( bit( c , l ) ) {
   s = f_zeros[ l ] + B.rank1( s );
   i = f_zeros[ l ] + B.rank1( i );
   }
  else {
   s = B.rank0( s );
   i = B.rank0( i );
   }
  }
 return( i - s );
 }

inline std::size_t WaveletMatrix::select( std::uint64_t c ,
                                          std::size_t k ) const
{
 ADS_PRE( k >= 1 );
 ADS_PRE( k <= rank( c , f_n ) );
 std::size_t p = start( c ) + k - 1;        // at the last level, then up
 for( std::size_t l = f_lev.size() ; l-- > 0 ; ) {
  const RankSelect & B = f_lev[ l ];
  p = bit( c , l ) ? B.select1( p - f_zeros[ l ] + 1 ) : B.select0( p + 1 );
  }
 ADS_POST( ( access( p ) == c ) && ( rank( c , p ) == k - 1 ) );
 return( p );
 }

inline std::uint64_t WaveletMatrix::quantile( std::size_t i , std::size_t j ,
                                              std::size_t k ) const
{
 ADS_PRE( ( i <= j ) && ( j <= f_n ) && ( k < j - i ) );
 std::uint64_t c = 0;
 for( std::size_t l = 0 ; l < f_lev.size() ; ++l ) {
  const RankSelect & B = f_lev[ l ];
  const std::size_t i1 = B.rank1( i ) , j1 = B.rank1( j );
  const std::size_t z = ( j - j1 ) - ( i - i1 );   // the 0s of the range
  if( k < z ) {
   c <<= 1;
   i -= i1;
   j -= j1;
   }
  else {
   c = ( c << 1 ) | 1;
   k -= z;
   i = f_zeros[ l ] + i1;
   j = f_zeros[ l ] + j1;
   }
  }
 return( c );
 }

inline std::size_t WaveletMatrix::countLess( std::size_t i , std::size_t j ,
                                             std::uint64_t c ) const
{
 ADS_PRE( ( i <= j ) && ( j <= f_n ) );
 if( ( f_lev.size() < 64 ) && ( c >> f_lev.size() ) ) return( j - i );
 std::size_t r = 0;
 for( std::size_t l = 0 ; ( l < f_lev.size() ) && ( i < j ) ; ++l ) {
  const RankSelect & B = f_lev[ l ];
  const std::size_t i1 = B.rank1( i ) , j1 = B.rank1( j );
  if( bit( c , l ) ) {                      // the 0s are below c
   r += ( j - j1 ) - ( i - i1 );
   i = f_zeros[ l ] + i1;
   j = f_zeros[ l ] + j1;
   }
  else {
   i -= i1;
   j -= j1;
   }
  }
 return( r );
 }

inline std::size_t WaveletMatrix::sizeInBits( void ) const
{
 std::size_t s = 64 * f_zeros.size();
 for( const auto & B : f_lev ) s += B.size() + B.directoryBits();
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File WaveletMatrix.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
