/*--------------------------------------------------------------------------*/
/*------------------------- File WaveletMatrix.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of WaveletMatrix.h, with their contracts.
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

inline WaveletMatrix::WaveletMatrix( std::span< const std::uint64_t > x ,
                                     int w ) : f_n( x.size() )
{
 Sequential s;
 init( s , x , w );
 }

template< Executor E >
WaveletMatrix::WaveletMatrix( E & ex , std::span< const std::uint64_t > x ,
                              int w ) : f_n( x.size() )
{
 init( ex , x , w );
 }

template< class U >
requires( std::is_same_v< U , std::uint8_t > ||
          std::is_same_v< U , std::uint16_t > ||
          std::is_same_v< U , std::uint32_t > )
WaveletMatrix::WaveletMatrix( std::span< const U > x , int w )
 : f_n( x.size() )
{
 Sequential s;
 init( s , x , w );
 }

template< Executor E , class U >
requires( std::is_same_v< U , std::uint8_t > ||
          std::is_same_v< U , std::uint16_t > ||
          std::is_same_v< U , std::uint32_t > )
WaveletMatrix::WaveletMatrix( E & ex , std::span< const U > x , int w )
 : f_n( x.size() )
{
 init( ex , x , w );
 }

template< class U , Executor E >
void WaveletMatrix::init( E & ex , std::span< const U > x , int w )
{
 ADS_PRE( ( w >= 0 ) && ( w <= 64 ) );
 if( w == 0 ) {
  const std::uint64_t m = parallelReduce( ex , 0 , f_n , std::uint64_t( 0 ) ,
   [ & ]( std::size_t lo , std::size_t hi ) {
    std::uint64_t o = 0;
    for( std::size_t i = lo ; i < hi ; ++i ) o |= x[ i ];
    return( o ); } , std::bit_or<>() , kSlice );
  w = std::max( 1 , int( std::bit_width( m ) ) );
  }
 ADS_PRE( ( w == 64 ) || std::all_of( x.begin() , x.end() ,
                                      [ w ]( std::uint64_t v ) {
                                       return( ( v >> w ) == 0 ); } ) );
 if( w <= 16 ) build< std::uint16_t >( ex , x , w );
 else if( w <= 32 ) build< std::uint32_t >( ex , x , w );
 else build< std::uint64_t >( ex , x , w );
 ADS_AUDIT( std::equal( x.begin() , x.end() , begin() ) );
 }

template< class T , class U , Executor E >
void WaveletMatrix::build( E & ex , std::span< const U > x , int w )
{
 const std::size_t n = f_n;
 std::vector< BitVector > B;
 B.reserve( std::size_t( w ) );
 // the words of the levels, 0, cleared on the threads of ex (the words
 // of a BitVector are left as they are by its allocator when resized)
 for( int l = 0 ; l < w ; ++l ) {
  B.emplace_back();
  B.back().f_n = n;
  B.back().f_w.resize( BitVector::wordsFor( n ) );
  }
 parallelForBlocks( ex , 0 , B.empty() ? 0 : B[ 0 ].f_w.size() ,
                    [ & ]( std::size_t lo , std::size_t hi ) {
  for( auto & b : B ) std::fill( b.f_w.begin() + std::ptrdiff_t( lo ) ,
                                 b.f_w.begin() + std::ptrdiff_t( hi ) ,
                                 Word( 0 ) ); } , 1 << 14 );
 f_zeros.assign( std::size_t( w ) , 0 );
 // the slices: about 4 per thread, from kSlice to kSliceMax items, so
 // that the levels of a slice stay in the second level of the cache
 const std::size_t p = std::max< std::size_t >( ex.concurrency() , 1 );
 const auto cut = blocksOf( 0 , n , std::clamp( ( n + 4 * p - 1 ) /
                                                ( 4 * p ) , kSlice ,
                                                kSliceMax ) );
 const std::size_t S = cut.first , size = cut.second;
 std::vector< T > C , D;                  // the items of the level, moved
 const int passes = ( w + 7 ) / 8;
 for( int pass = 0 , l = 0 ; pass < passes ; ++pass ) {
  const int k = ( w - l + ( passes - pass ) - 1 ) / ( passes - pass );
  const int sh = w - l - k;                 // the field of the k bits
  const std::size_t K = std::size_t( 1 ) << k;
  const bool last = pass + 1 == passes;
  // the key of an item: its k bits reversed, the bit of the level l the
  // lowest, so that the key of the level l + j is the key mod 2^j
  std::uint8_t rev[ 256 ];
  for( std::size_t a = 0 ; a < K ; ++a ) {
   std::size_t r = 0;
   for( int j = 0 ; j < k ; ++j ) r |= ( ( a >> ( k - 1 - j ) ) & 1 ) << j;
   rev[ a ] = std::uint8_t( r );
   }
  // the keys of each slice, counted in 4 tables by turns, so that two
  // equal keys in a row do not wait for each other
  std::vector< std::size_t > h( S * K , 0 );
  parallelFor( ex , 0 , S , [ & ]( std::size_t s ) {
   std::size_t H4[ 4 ][ 256 ] = {};
   const std::size_t i0 = s * size , i1 = std::min( n , i0 + size );
   auto count = [ & ]< bool FROM_X >() {
    const T * const Cp = C.data();
    const U * const xp = x.data();
    std::size_t i = i0;
    for( ; i + 4 <= i1 ; i += 4 )
     for( std::size_t u = 0 ; u < 4 ; ++u ) {
      const T c = FROM_X ? T( xp[ i + u ] ) : Cp[ i + u ];
      ++H4[ u ][ std::size_t( ( c >> sh ) & T( K - 1 ) ) ];
      }
    for( ; i < i1 ; ++i ) {
     const T c = FROM_X ? T( xp[ i ] ) : Cp[ i ];
     ++H4[ 0 ][ std::size_t( ( c >> sh ) & T( K - 1 ) ) ];
     }
    };
   if( pass == 0 ) count.template operator()< true >();
   else count.template operator()< false >();
   // the counts by the keys, the fields reversed
   std::size_t * H = h.data() + s * K;
   for( std::size_t f = 0 ; f < K ; ++f )
    H[ rev[ f ] ] = H4[ 0 ][ f ] + H4[ 1 ][ f ] + H4[ 2 ][ f ] + H4[ 3 ][ f ];
   } , 1 );
  // the first place of each slice in each stream: the stream ( j , a ) of
  // the level l + j holds the items of key mod 2^j = a, the streams of
  // smaller a first and, in a stream, the slices in order; the stream
  // ( k , a ) is the next order of the items
  const std::size_t R = 2 * K - 1;          // the streams of j = 0 .. k
  std::vector< std::size_t > first( S * R ) , end( S * R );
  for( int j = 0 ; j <= k ; ++j ) {
   const std::size_t J = std::size_t( 1 ) << j , base = J - 1;
   std::size_t at = 0;
   for( std::size_t a = 0 ; a < J ; ++a )
    for( std::size_t s = 0 ; s < S ; ++s ) {
     std::size_t c = 0;
     for( std::size_t b = a ; b < K ; b += J ) c += h[ s * K + b ];
     first[ s * R + base + a ] = at;
     at += c;
     end[ s * R + base + a ] = at;
     }
   }
  // the 0s of each level: the items whose bit j is 0
  for( int j = 0 ; j < k ; ++j ) {
   std::size_t z = 0;
   for( std::size_t s = 0 ; s < S ; ++s )
    for( std::size_t a = 0 ; a < K ; ++a )
     if( ! ( ( a >> j ) & 1 ) ) z += h[ s * K + a ];
   f_zeros[ std::size_t( l + j ) ] = z;
   }
  if( ! last ) D.resize( n );
  // the place of each stream of a slice within the slice, the streams in
  // the order of the level: with one slice the places are those of the
  // levels, and the slice writes the levels themselves
  std::vector< std::size_t > loc( S * R );
  for( std::size_t q = 0 ; q < S ; ++q )
   for( int j = 0 ; j < k ; ++j ) {
    const std::size_t J = std::size_t( 1 ) << j , base = J - 1;
    std::size_t at = 0;
    for( std::size_t a = 0 ; a < J ; ++a ) {
     loc[ q * R + base + a ] = at;
     at += end[ q * R + base + a ] - first[ q * R + base + a ];
     }
    }
  // the bits of the k levels of each slice, in the levels if one slice,
  // else in words of its own, at the places of loc
  const std::size_t sw = size / 64 + 2;     // the words of a slice level
  std::unique_ptr< Word[] > Lw( S > 1 ? new Word[ S * std::size_t( k ) * sw ]
                                      : nullptr );
  parallelFor( ex , 0 , S , [ & ]( std::size_t q ) {
   const std::size_t i0 = q * size , i1 = std::min( n , i0 + size );
   if( S > 1 )                              // the words of the slice, 0
    std::fill_n( Lw.get() + q * std::size_t( k ) * sw ,
                 std::size_t( k ) * sw , Word( 0 ) );
   Word * Wl[ 8 ];
   std::size_t cur[ 255 ];
   for( int j = 0 ; j < k ; ++j ) {
    const std::size_t J = std::size_t( 1 ) << j , base = J - 1;
    Wl[ j ] = S > 1 ? Lw.get() + ( q * std::size_t( k ) + std::size_t( j ) )
                                 * sw
                    : B[ std::size_t( l + j ) ].f_w.data();
    for( std::size_t a = 0 ; a < J ; ++a )
     cur[ base + a ] = S > 1 ? loc[ q * R + base + a ]
                             : first[ q * R + base + a ];
    }
   std::size_t next[ 256 ];
   for( std::size_t a = 0 ; a < K ; ++a )
    next[ a ] = first[ q * R + K - 1 + a ];
   // the level l in the order of the slice, 64 bits in a word at a time;
   // the others by an or at the next place of their stream; the levels
   // a constant, so that their loop unrolls, and the items read from x or
   // from C, out of the loop
   auto scan = [ & ]< int KK , bool FROM_X >( bool move ) {
    const T * const Cp = C.data();
    for( std::size_t i = i0 ; i < i1 ; i += 64 ) {
     const std::size_t e = std::min( i1 , i + 64 );
     Word w0 = 0;
     for( std::size_t t = i ; t < e ; ++t ) {
      const T c = FROM_X ? T( x[ t ] ) : Cp[ t ];
      const std::size_t a = rev[ std::size_t( ( c >> sh ) & T( K - 1 ) ) ];
      w0 |= Word( a & 1 ) << ( t - i );
      for( std::size_t j = 1 ; j < std::size_t( KK ) ; ++j ) {
       const std::size_t m = ( std::size_t( 1 ) << j ) - 1;
       const std::size_t at = cur[ m + ( a & m ) ]++;
       Wl[ j ][ at >> 6 ] |= Word( ( a >> j ) & 1 ) << ( at & 63 );
       }
      if( move ) D[ next[ a ]++ ] = c;
      }
     Wl[ 0 ][ ( i - ( S > 1 ? i0 : 0 ) ) >> 6 ] = w0;
     }
    };
   auto levels = [ & ]< int KK >() {
    if( pass == 0 ) scan.template operator()< KK , true >( ! last );
    else scan.template operator()< KK , false >( ! last );
    };
   switch( k ) {
    case( 1 ): levels.template operator()< 1 >(); break;
    case( 2 ): levels.template operator()< 2 >(); break;
    case( 3 ): levels.template operator()< 3 >(); break;
    case( 4 ): levels.template operator()< 4 >(); break;
    case( 5 ): levels.template operator()< 5 >(); break;
    case( 6 ): levels.template operator()< 6 >(); break;
    case( 7 ): levels.template operator()< 7 >(); break;
    default: levels.template operator()< 8 >(); break;
    }
   } , 1 );
  // the levels from the slices: each block of words of a level gathers
  // the runs of the slices that fall in it, the runs in the order of the
  // level, by copies of bits
  if( S > 1 )
   for( int j = 0 ; j < k ; ++j ) {
    const std::size_t J = std::size_t( 1 ) << j , base = J - 1;
    // the runs ( a , q ) in the order of their places in the level
    std::vector< std::size_t > rs;
    rs.reserve( J * S );
    for( std::size_t a = 0 ; a < J ; ++a )
     for( std::size_t q = 0 ; q < S ; ++q )
      if( end[ q * R + base + a ] > first[ q * R + base + a ] )
       rs.push_back( q * R + base + a );
    Word * const out = B[ std::size_t( l + j ) ].f_w.data();
    const std::size_t nw = ( n + 63 ) / 64;
    parallelForBlocks( ex , 0 , nw , [ & ]( std::size_t w0 ,
                                             std::size_t w1 ) {
     const std::size_t b0 = 64 * w0 , b1 = std::min( n , 64 * w1 );
     // the first run that ends past b0
     std::size_t r = std::size_t( std::partition_point( rs.begin() ,
                                                        rs.end() ,
      [ & ]( std::size_t x ) { return( end[ x ] <= b0 ); } ) - rs.begin() );
     for( ; ( r < rs.size() ) && ( first[ rs[ r ] ] < b1 ) ; ++r ) {
      const std::size_t x = rs[ r ] , q = x / R;
      const std::size_t g0 = std::max( first[ x ] , b0 ) ,
                        g1 = std::min( end[ x ] , b1 );
      detail::copyBits( out , g0 ,
                        Lw.get() + ( q * std::size_t( k ) +
                                     std::size_t( j ) ) * sw ,
                        loc[ x ] + ( g0 - first[ x ] ) , g1 - g0 );
      }
     } , 1024 );
    }
  C.swap( D );
  l += k;
  }
 f_lev.clear();
 f_lev.reserve( std::size_t( w ) );
 for( auto & b : B ) f_lev.emplace_back( ex , std::move( b ) );
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
