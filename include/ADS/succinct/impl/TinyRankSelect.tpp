/*--------------------------------------------------------------------------*/
/*------------------------- File TinyRankSelect.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of TinyRankSelect.h, with their contracts.
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

template< unsigned L0 >
BasicTinyRankSelect< L0 >::BasicTinyRankSelect( BitVector b )
    : f_b( std::move( b ) )
{
 const std::size_t nr = f_b.size() / kSuper + 1;  // the one of the size
 f_e.assign( 8 * ( nr + 1 ) , 0 );                 // and one more
 for( std::size_t s = 0 ; s <= nr ; ++s )
  f_e[ 8 * s ] = Word( fill( s ) );
 finish();
 }

template< unsigned L0 >
template< Executor E >
BasicTinyRankSelect< L0 >::BasicTinyRankSelect( E & ex , BitVector b )
    : f_b( std::move( b ) )
{
 const std::size_t nr = f_b.size() / kSuper + 1;
 f_e.assign( 8 * ( nr + 1 ) , 0 );
 // each superblock writes its own line of the entries
 parallelFor(
   ex , 0 , nr + 1 ,
   [ & ]( std::size_t s ) { f_e[ 8 * s ] = Word( fill( s ) ); } ,
   kChunk );
 finish();
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::fill( std::size_t s )
{
 constexpr std::size_t kb = L0 / 64;  // the words of a block
 const Word * const P = f_b.words().data();
 const std::size_t nw = f_b.words().size();
 // the 1s before each block of the superblock, the words past the vector
 // being 0s
 std::size_t c[ 33 ];
 c[ 0 ] = 0;
 for( std::size_t j = 0 ; j < 32 ; ++j ) {
  const std::size_t lo = std::min( nw , kWords * s + kb * j ) ,
		    hi = std::min( nw , lo + kb );
  std::size_t in = 0;
  for( std::size_t i = lo ; i < hi ; ++i )
   in += std::size_t( std::popcount( P[ i ] ) );
  c[ j + 1 ] = c[ j ] + in;
  }
 // the field of each group of 4 blocks at the bit 64 + 56 g of the entry
 Word * const e = f_e.data() + 8 * s;
 for( unsigned g = 0 ; g < 8 ; ++g ) {
  const std::size_t b = c[ 4 * g ];
  const Word f = Word( b ) | ( Word( c[ 4 * g + 1 ] - b ) << 16 ) |
		 ( Word( c[ 4 * g + 2 ] - b ) << 29 ) |
		 ( Word( c[ 4 * g + 3 ] - b ) << 42 );
  const unsigned at = 64 + 56 * g , q = at / 64 , o = at % 64;
  e[ q ] |= f << o;
  if( o > 8 )
   e[ q + 1 ] |= f >> ( 64 - o );
  ADS_POST( field( e , g ) == f );
  }
 return( c[ 32 ] );
 }

template< unsigned L0 >
void BasicTinyRankSelect< L0 >::finish( void )
{
 // the 1s of each superblock, in its first word, become those before it
 std::size_t c = 0;
 for( std::size_t s = 0 ; 8 * s < f_e.size() ; ++s ) {
  const std::size_t in = std::size_t( f_e[ 8 * s ] );
  f_e[ 8 * s ] = Word( c );
  c += in;
  }
 f_ones = c;
 tree( false );
 tree( true );
 ADS_AUDIT( consistent() );
 }

template< unsigned L0 >
void BasicTinyRankSelect< L0 >::tree( bool b )
{
 Tree & T = f_t[ b ];
 BitVector &top = f_top[ b ] , &mid = f_mid[ b ] , &bot = f_bot[ b ];
 auto & head = f_head[ b ];
 T = Tree();
 top.clear();
 mid.clear();
 bot.clear();
 head.clear();
 const std::size_t c = b ? f_ones : zeros();
 if( c == 0 )
  return;
 const std::size_t nr = f_b.size() / kSuper + 1;
 // a about twice the bits b of a superblock, b' about its square root
 const std::size_t mu = std::max< std::size_t >( 1 , c / nr );
 const std::size_t a =
   std::bit_floor( std::max< std::size_t >( 1 , kAlpha * mu / 8 ) );
 T.A = unsigned( std::countr_zero( a ) );
 T.Bb = ( T.A + 1 ) / 2;
 const std::size_t bb = std::size_t( 1 ) << T.Bb , m = a >> T.Bb;
 // sigma( x ) on increasing ranks x, a cursor on the superblocks for each
 // level
 auto cursor = [ & , s = std::size_t( 0 ) ]( std::size_t x ) mutable {
  x = std::min( x , c - 1 );
  while( ( s + 1 < nr ) && ( before( s + 1 , b ) <= x ) )
   ++s;
  return( s );
  };
 auto sTop = cursor , sMid = cursor , sBot = cursor;
 const std::size_t t = ( c - 1 ) / a + 1;  // the groups of a ranks
 std::vector< std::size_t > o( t + 1 );
 std::size_t G = 0;  // the groups of M
 for( std::size_t j = 0 ; j <= t ; ++j )
  o[ j ] = sTop( j * a );
 for( std::size_t j = 0 ; j < t ; ++j )
  G += o[ j + 1 ] - o[ j ] >= kAlpha;
 T.wo = unsigned( std::max( 1 , int( std::bit_width( nr - 1 ) ) ) );
 T.wg = G ? unsigned( std::bit_width( G - 1 ) ) : 0;
 const int w = int( T.wo + T.wg );
 top.reserve( std::size_t( w ) * ( t + 1 ) );
 std::vector< std::size_t > M( m + 1 );
 std::size_t g = 0;
 for( std::size_t j = 0 ; j <= t ; ++j ) {
  const bool large = ( j < t ) && ( o[ j + 1 ] - o[ j ] >= kAlpha );
  top.append( Word( o[ j ] ) | ( Word( large ? g : 0 ) << T.wo ) , w );
  if( ! large )
   continue;
  // the group of M of j, then the groups of D of its long ranges
  const unsigned rho = unsigned( std::bit_width( o[ j + 1 ] - o[ j ] ) );
  std::size_t longs = 0;
  unsigned hat = 0;
  for( std::size_t i = 0 ; i <= m ; ++i )
   M[ i ] = sMid( j * a + i * bb ) - o[ j ];
  for( std::size_t i = 0 ; i < m ; ++i )
   if( M[ i + 1 ] - M[ i ] >= kAlpha ) {
    ++longs;
    hat = std::max( hat , unsigned( std::bit_width( M[ i + 1 ] - M[ i ] ) ) );
    }
  const unsigned kappa = unsigned( std::bit_width( longs ) );
  head.push_back( mid.size() );
  head.push_back( bot.size() );
  head.push_back( rho | ( kappa << 8 ) | ( hat << 16 ) );
  std::size_t prior = 0;  // the long ranges before i
  for( std::size_t i = 0 ; i <= m ; ++i ) {
   mid.append( Word( M[ i ] ) | ( Word( prior ) << rho ) ,
	       int( rho + kappa ) );
   if( ( i < m ) && ( M[ i + 1 ] - M[ i ] >= kAlpha ) ) {
    ++prior;
    for( std::size_t q = 0 ; q < bb ; ++q )
     bot.append( Word( sBot( j * a + i * bb + q ) - o[ j ] - M[ i ] ) ,
		 int( hat ) );
    }
   }
  ++g;
  }
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_b.size() );
 const Word * const e = f_e.data() + 8 * ( i / kSuper );
 // the 1s of the lines of the block before i [see detail::lineRank()]
 return( std::size_t( e[ 0 ] ) + inSuper( e , unsigned( ( i / L0 ) % 32 ) ) +
	  detail::lineRank< kScalar , kLines >( f_b.words().data() +
						 ( L0 / 64 ) * ( i / L0 ) ,
					       unsigned( i % L0 ) ) );
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::super( std::size_t x , bool b ) const
{
 const Tree & T = f_t[ b ];
 // two fields of w bits from the bit p of v, at once where they fit a word
 auto pair = []( const BitVector & v , std::size_t p , int w ) {
  if( 2 * w <= 64 ) {
   const Word y = v.get( p , 2 * w );
   return( std::pair< Word , Word >( y & detail::lowMask( w ) , y >> w ) );
   }
  return( std::pair< Word , Word >( v.get( p , w ) , v.get( p + w , w ) ) );
  };
 // the range of superblocks of x at the top, and below it if long
 const int w = int( T.wo + T.wg );
 const auto [ e0 , e1 ] =
   pair( f_top[ b ] , std::size_t( w ) * ( x >> T.A ) , w );
 const Word mo = detail::lowMask( int( T.wo ) );
 std::size_t lo = std::size_t( e0 & mo ) , hi = std::size_t( e1 & mo );
 if( hi - lo >= kAlpha ) {
  const std::uint64_t * h = f_head[ b ].data() + 3 * ( e0 >> T.wo );
  const unsigned rho = unsigned( h[ 2 ] & 0xFF ) ,
		 kappa = unsigned( ( h[ 2 ] >> 8 ) & 0xFF ) ,
		 hat = unsigned( ( h[ 2 ] >> 16 ) & 0xFF );
  const int u = int( rho + kappa );
  const std::size_t i = ( x & ( ( std::size_t( 1 ) << T.A ) - 1 ) ) >> T.Bb;
  const auto [ m0 , m1 ] = pair( f_mid[ b ] , h[ 0 ] + std::size_t( u ) * i ,
				 u );
  const Word mr = detail::lowMask( int( rho ) );
  const std::size_t base = lo;
  lo = base + std::size_t( m0 & mr );
  hi = base + std::size_t( m1 & mr );
  if( hi - lo >= kAlpha ) {  // the superblock of x itself
   const std::size_t q = ( std::size_t( m0 >> rho ) << T.Bb ) +
			 ( x & ( ( std::size_t( 1 ) << T.Bb ) - 1 ) );
   lo += std::size_t(
     f_bot[ b ].get( h[ 1 ] + std::size_t( hat ) * q , int( hat ) ) );
   hi = lo;
   }
  }
 while( lo < hi ) {  // the last superblock with at most x before it
  const std::size_t s = lo + ( hi - lo + 1 ) / 2;
  if( before( s , b ) <= x )
   lo = s;
  else
   hi = s - 1;
  }
 return( lo );
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::select( std::size_t k , bool b ) const
{
 const std::size_t x = k - 1 , lo = super( x , b );
 std::size_t r = x - before( lo , b );  // the r-th of the superblock, from 0
 // the group, by the counts before the groups 1 to 7, and in it the block,
 // by the counts of its blocks 1 to 3, with no branch
 const Word * const e = f_e.data() + 8 * lo;
 std::uint64_t f = field( e , 0 );
 std::size_t g = 0 , in = 0;
 for( unsigned h = 1 ; h < 8 ; ++h ) {
  const std::uint64_t fh = field( e , h );
  const std::size_t c1 = std::size_t( fh & 0xFFFF ) ,
		    ch = b ? c1 : 4 * h * L0 - c1;
  const bool le = ch <= r;
  g = le ? h : g;
  in = le ? ch : in;
  f = le ? fh : f;
  }
 r -= in;
 std::size_t t = 0;
 in = 0;
 for( unsigned h = 1 ; h < 4 ; ++h ) {
  const std::size_t d1 = std::size_t( ( f >> ( 3 + 13 * h ) ) & 0x1FFF ) ,
		    dh = b ? d1 : h * L0 - d1;
  const bool le = dh <= r;
  t = le ? h : t;
  in = le ? dh : in;
  }
 r -= in;
 // the line of the block, by the counts of its lines before the last one
 // (each one past the words of the vector read as the last line)
 const Word * const P = f_b.words().data();
 std::size_t W = kWords * lo + ( L0 / 64 ) * ( 4 * g + t );  // its word
 if constexpr( kLines > 1 ) {
  const std::size_t last = f_b.words().size() / 8 - 1;
  std::size_t q = 0 , cum = 0 , sq = 0;
  for( unsigned l = 0 ; l + 1 < kLines ; ++l ) {
   const std::size_t ones =
     detail::lineCount< kScalar >( P + 8 * std::min( W / 8 + l , last ) );
   cum += b ? ones : 512 - ones;
   const bool le = cum <= r;
   q += le;
   sq = le ? cum : sq;
   }
  W += 8 * q;
  r -= sq;
  }
 // the word, by the sums of the 8 words of the line
 const Word * const L = P + W;
 std::size_t p = 0 , sum = 0 , sp = 0;  // sp: the bits b before word p
 for( std::size_t h = 0 ; h < 7 ; ++h ) {
  sum += std::size_t( std::popcount( b ? L[ h ] : ~L[ h ] ) );
  const bool le = sum <= r;
  p += le;
  sp = le ? sum : sp;
  }
 return( 64 * ( W + p ) +
	  std::size_t( selectInWord( b ? L[ p ] : ~L[ p ] ,
				     int( r - sp ) ) ) );
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 return( detail::selected( *this , k , true , select( k , true ) ) );
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 return( detail::selected( *this , k , false , select( k , false ) ) );
 }

template< unsigned L0 >
std::size_t BasicTinyRankSelect< L0 >::directoryBits( void ) const
{
 std::size_t z = 64 * f_e.size();
 for( const bool b : { false , true } )
  z += f_top[ b ].size() + f_mid[ b ].size() + f_bot[ b ].size() +
       64 * f_head[ b ].size() + 4 * 8;  // and the shape of the tree
 return( z );
 }

template< unsigned L0 >
bool BasicTinyRankSelect< L0 >::consistent( void ) const
{
 // each entry against the counts of the words of its blocks
 const Word * const P = f_b.words().data();
 const std::size_t nw = f_b.words().size() , nr = f_b.size() / kSuper + 1;
 std::size_t c = 0;
 for( std::size_t s = 0 ; s <= nr ; ++s ) {
  const Word * const e = f_e.data() + 8 * s;
  if( std::size_t( e[ 0 ] ) != c )
   return( false );
  std::size_t in = 0;
  for( unsigned j = 0 ; j < 32 ; ++j ) {
   if( inSuper( e , j ) != in )
    return( false );
   for( std::size_t i = kWords * s + ( L0 / 64 ) * j ;
	i < std::min( nw , kWords * s + ( L0 / 64 ) * ( j + 1 ) ) ; ++i )
    in += std::size_t( std::popcount( P[ i ] ) );
   }
  c += in;
  }
 if( c != f_ones )
  return( false );
 // the superblock that the tree gives for every rank below 2^20 and for
 // the ranks multiple of b' above, against a scan of the entries
 for( const bool b : { false , true } ) {
  const std::size_t n1 = b ? f_ones : zeros();
  const std::size_t step =
    n1 < ( std::size_t( 1 ) << 20 ) ? 1 : std::size_t( 1 ) << f_t[ b ].Bb;
  std::size_t s = 0;
  for( std::size_t x = 0 ; x < n1 ; x += step ) {
   while( ( s + 1 < nr ) && ( before( s + 1 , b ) <= x ) )
    ++s;
   if( super( x , b ) != s )
    return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< unsigned L0 >
template< class A >
void BasicTinyRankSelect< L0 >::persist( A & a )
{
 a( f_b , f_e , f_top , f_mid , f_bot , f_head , f_ones );
 for( auto & T : f_t )
  a( T.A , T.Bb , T.wo , T.wg );
 const std::size_t n = f_b.size() , nr = n / kSuper + 1;
 bool ok = ( f_e.size() == 8 * ( nr + 1 ) ) && ( f_ones <= n );
 for( const bool b : { false , true } ) {
  const Tree & T = f_t[ b ];
  const std::size_t c = b ? f_ones : n - f_ones;
  ok = ok && ( T.A < 64 ) && ( T.Bb <= T.A ) && ( T.wo >= 1 ) &&
       ( T.wo + T.wg <= 64 ) && ( f_head[ b ].size() % 3 == 0 );
  if( ok && ( c > 0 ) )
   ok = f_top[ b ].size() == std::size_t( T.wo + T.wg ) *
			       ( ( c - 1 ) / ( std::size_t( 1 ) << T.A ) + 2 );
  // each group of M and its groups of D inside their vectors
  const std::size_t m = std::size_t( 1 ) << ( T.A - T.Bb );
  for( std::size_t g = 0 ; ok && 3 * g < f_head[ b ].size() ; ++g ) {
   const std::uint64_t * h = f_head[ b ].data() + 3 * g;
   const std::size_t rho = h[ 2 ] & 0xFF , kappa = ( h[ 2 ] >> 8 ) & 0xFF ,
		     hat = ( h[ 2 ] >> 16 ) & 0xFF;
   ok = ( rho + kappa <= 64 ) && ( hat <= 64 ) &&
	( h[ 0 ] <= f_mid[ b ].size() ) &&
	( ( m + 1 ) * ( rho + kappa ) <= f_mid[ b ].size() - h[ 0 ] ) &&
	( h[ 1 ] <= f_bot[ b ].size() );
   }
  }
 a.require( ok , "the directory of a TinyRankSelect" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File TinyRankSelect.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
