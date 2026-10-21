/*--------------------------------------------------------------------------*/
/*--------------------- File PartitionedEliasFano.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of PartitionedEliasFano.h, with their contracts.
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

inline PartitionedEliasFano::Code PartitionedEliasFano::code(
                                         std::uint64_t m , std::uint64_t u )
{
 if( u == m ) return( Code{ FULL , 0 , 0 } );
 const int ell = u > m ? std::bit_width( u / m ) - 1 : 0;
 const std::uint64_t ef = m * std::uint64_t( ell ) + m +
                          ( ( u - 1 ) >> ell ) + 1;
 if( u <= ef ) return( Code{ VECTOR , u , 0 } );
 return( Code{ ELIAS , ef , ell } );
 }

inline PartitionedEliasFano::PartitionedEliasFano(
                                     std::span< const std::uint64_t > x )
 : PartitionedEliasFano( x , x.empty() ? 0 : x.back() + 1 )
{
 }

inline PartitionedEliasFano::PartitionedEliasFano(
                std::span< const std::uint64_t > x , std::uint64_t u )
 : f_n( x.size() ) , f_u( u )
{
 ADS_PRE( x.empty() || ( x.back() < u ) );
 ADS_AUDIT( std::adjacent_find( x.begin() , x.end() ,
                                []( std::uint64_t a , std::uint64_t b ) {
                                 return( a >= b ); } ) == x.end() );
 if( f_n == 0 ) return;
 const std::size_t nb = ( f_n + kB - 1 ) / kB;
 // the places of the codes, then the codes
 std::vector< std::uint64_t > at( nb + 1 , 0 );
 auto block = [ & ]( std::size_t j , std::uint64_t & b , std::uint64_t & m ,
                     std::uint64_t & uj ) {
  b = j == 0 ? 0 : x[ j * kB - 1 ] + 1;
  m = std::min( kB , f_n - j * kB );
  uj = x[ j * kB + m - 1 ] - b + 1;
  };
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  at[ j + 1 ] = at[ j ] + code( m , uj ).bits;
  }
 f_codes = BitVector( std::size_t( at[ nb ] ) );
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  const Code c = code( m , uj );
  const std::size_t p = std::size_t( at[ j ] );
  for( std::size_t k = 0 ; k < m ; ++k ) {
   const std::uint64_t v = x[ j * kB + k ] - b;
   if( c.kind == VECTOR ) f_codes.set( p + std::size_t( v ) );
   else if( c.kind == ELIAS ) {
    if( c.ell > 0 )
     f_codes.put( p + k * std::size_t( c.ell ) ,
                  v & detail::lowMask( c.ell ) , c.ell );
    f_codes.set( p + std::size_t( m ) * std::size_t( c.ell ) +
                 std::size_t( v >> c.ell ) + k );
    }
   }
  }
 // the records: the last item and the place of the code of each block
 f_wl = std::max( 1 , int( std::bit_width( u - 1 ) ) );
 f_wo = std::max( 1 , int( std::bit_width( at[ nb ] ) ) );
 f_wr = f_wl + f_wo + 8;
 f_rec = BitVector( nb * std::size_t( f_wr ) );
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  std::uint64_t b , m , uj;
  block( j , b , m , uj );
  const Code c = code( m , uj );
  const std::uint64_t l = x[ std::min( f_n , ( j + 1 ) * kB ) - 1 ];
  const std::size_t r = j * std::size_t( f_wr );
  f_rec.put( r , l , f_wl );
  f_rec.put( r + std::size_t( f_wl ) , at[ j ] , f_wo );
  f_rec.put( r + std::size_t( f_wl + f_wo ) ,
             ( std::uint64_t( c.ell ) << 2 ) | std::uint64_t( c.kind ) , 8 );
  if( j % kApart == 0 ) f_apart.push_back( l );
  }
 // the items apart in the layout of Eytzinger, with the rank of each
 f_eyt.assign( f_apart.size() + 1 , 0 );
 f_rank.assign( f_apart.size() + 1 , 0 );
 std::size_t next = 0;
 auto fill = [ & ]( auto & self , std::size_t t ) -> void {
  if( t > f_apart.size() ) return;
  self( self , 2 * t );
  f_rank[ t ] = std::uint32_t( next );
  f_eyt[ t ] = f_apart[ next++ ];
  self( self , 2 * t + 1 );
  };
 fill( fill , 1 );
 f_apart.clear();
 f_apart.shrink_to_fit();
 ADS_AUDIT( std::equal( x.begin() , x.end() , begin() ) );
 }

inline std::size_t PartitionedEliasFano::select1( std::size_t p ,
                                                  std::size_t k ) const
{
 // the aligned words from the one of p, the bits before p cleared
 const Word * W = f_codes.words().data() + p / 64;
 Word w = W[ 0 ] & ( ~Word( 0 ) << ( p % 64 ) );
 for( std::size_t q = p - p % 64 ; ; q += 64 , w = *++W ) {
  const std::size_t c = std::size_t( std::popcount( w ) );
  if( k < c ) return( q + std::size_t( selectInWord( w , int( k ) ) ) );
  k -= c;
  }
 }

inline std::size_t PartitionedEliasFano::select0( std::size_t p ,
                                                  std::size_t k ) const
{
 const Word * W = f_codes.words().data() + p / 64;
 Word w = ~W[ 0 ] & ( ~Word( 0 ) << ( p % 64 ) );
 for( std::size_t q = p - p % 64 ; ; q += 64 , w = ~*++W ) {
  const std::size_t c = std::size_t( std::popcount( w ) );
  if( k < c ) return( q + std::size_t( selectInWord( w , int( k ) ) ) );
  k -= c;
  }
 }

inline std::size_t PartitionedEliasFano::ones( std::size_t p ,
                                               std::size_t q ) const
{
 if( p == q ) return( 0 );
 const Word * W = f_codes.words().data();
 const std::size_t a = p / 64 , b = ( q - 1 ) / 64;
 const Word first = ~Word( 0 ) << ( p % 64 ) ,
            lastm = ~Word( 0 ) >> ( 63 - ( q - 1 ) % 64 );
 if( a == b )
  return( std::size_t( std::popcount( W[ a ] & first & lastm ) ) );
 std::size_t c = std::size_t( std::popcount( W[ a ] & first ) ) +
                 std::size_t( std::popcount( W[ b ] & lastm ) );
 for( std::size_t i = a + 1 ; i < b ; ++i )
  c += std::size_t( std::popcount( W[ i ] ) );
 return( c );
 }

inline std::uint64_t PartitionedEliasFano::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 const std::size_t j = i / kB , k = i % kB;
 const std::uint64_t m = std::min( kB , f_n - j * kB );
 const std::uint64_t b = j == 0 ? 0 : last( j - 1 ) + 1;
 const unsigned f = form( j );
 const int ell = int( f >> 2 );
 const std::size_t p = where( j );
 if( ( f & 3 ) == FULL ) return( b + k );
 if( ( f & 3 ) == VECTOR ) return( b + ( select1( p , k ) - p ) );
 const std::uint64_t low = ell > 0 ? f_codes.get( p + k * std::size_t( ell ) ,
                                                  ell ) : 0;
 const std::size_t hp = p + std::size_t( m ) * std::size_t( ell );
 const std::uint64_t high = select1( hp , k ) - hp - k;
 return( b + ( ( high << ell ) | low ) );
 }

inline std::size_t PartitionedEliasFano::nextGEQ( std::uint64_t x ) const
{
 const std::size_t nb = ( f_n + kB - 1 ) / kB;
 if( ( f_n == 0 ) || ( x > last( nb - 1 ) ) ) return( f_n );
 // the first block whose last item is >= x: the records apart, then at
 // most kApart records
 // the first item apart >= x, by the descent of the layout of
 // Eytzinger (the node t has the children 2t and 2t + 1, its subtree
 // three levels down fetched ahead), its rank by the trailing 1s of the
 // path; the block is within kApart blocks before its one
 const std::uint64_t * E = f_eyt.data();
 const std::size_t na = f_eyt.size() - 1;
 std::size_t t = 1;
 while( t <= na ) {
#if defined( __GNUC__ )
  __builtin_prefetch( E + 8 * t );
#endif
  t = 2 * t + ( E[ t ] < x );
  }
 t >>= std::countr_one( t ) + 1;           // 0 if all are below x
 const std::size_t a = t == 0 ? na : f_rank[ t ];
 std::size_t j = a == 0 ? 0 : ( a - 1 ) * kApart + 1;
 while( last( j ) < x ) ++j;
 const std::uint64_t m = std::min( kB , f_n - j * kB );
 const std::uint64_t b = j == 0 ? 0 : last( j - 1 ) + 1;
 const unsigned f = form( j );
 const Code c{ Kind( f & 3 ) , 0 , int( f >> 2 ) };
 const std::size_t p = where( j );
 const std::uint64_t v = x < b ? 0 : x - b;
 std::size_t k;
 if( c.kind == FULL ) k = std::size_t( v );
 else if( c.kind == VECTOR ) k = ones( p , p + std::size_t( v ) );
 else {
  // the items of the bucket of v share their high bits: their low bits
  // decide, and the first item of a later bucket is above v
  const std::size_t hp = p + std::size_t( m ) * std::size_t( c.ell );
  const std::uint64_t hv = v >> c.ell , lv = v & detail::lowMask( c.ell );
  std::size_t pos = hv == 0 ? hp : select0( hp , std::size_t( hv - 1 ) ) + 1;
  k = pos - hp - std::size_t( hv );
  for( ; ( k < m ) && f_codes[ pos ] ; ++k , ++pos )
   if( ( c.ell > 0 ? f_codes.get( p + k * std::size_t( c.ell ) , c.ell )
                   : 0 ) >= lv ) break;
  }
 const std::size_t r = j * kB + k;
 ADS_POST( ( r == f_n ) || ( access( r ) >= x ) );
 ADS_POST( ( r == 0 ) || ( access( r - 1 ) < x ) );
 return( r );
 }

inline bool PartitionedEliasFano::contains( std::uint64_t x ) const
{
 const std::size_t p = nextGEQ( x );
 return( ( p < f_n ) && ( access( p ) == x ) );
 }

inline std::array< std::size_t , 3 > PartitionedEliasFano::kinds( void )
 const
{
 std::array< std::size_t , 3 > c{};
 for( std::size_t j = 0 ; j * kB < f_n ; ++j ) {
  const std::uint64_t m = std::min( kB , f_n - j * kB );
  const std::uint64_t b = j == 0 ? 0 : last( j - 1 ) + 1;
  ADS_INVARIANT( ( form( j ) & 3 ) == unsigned( code( m , last( j ) - b +
                                                1 ).kind ) );
  ++c[ form( j ) & 3 ];
  }
 return( c );
 }

inline std::size_t PartitionedEliasFano::sizeInBits( void ) const
{
 return( f_codes.size() + f_rec.size() + 96 * f_eyt.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File PartitionedEliasFano.tpp -------------------*/
/*--------------------------------------------------------------------------*/
