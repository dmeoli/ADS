/*--------------------------------------------------------------------------*/
/*--------------------------- File RankSelect.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RankSelect.h, with their contracts.
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

inline RankSelect::RankSelect( void ) : RankSelect( BitVector() )
{
 }

inline RankSelect::RankSelect( BitVector b , Selects s )
 : f_b( std::move( b ) )
{
 f_has[ 1 ] = ( s == Selects::ONES ) || ( s == Selects::BOTH );
 f_has[ 0 ] = ( s == Selects::ZEROS ) || ( s == Selects::BOTH );
 const auto & W = f_b.words();
 const std::size_t nb = f_b.size() / kBlock + 1;   // and one more
 f_big = ( ( nb * kBlock ) >> kTop ) > 0;   // the last block counts too
 f_dir.assign( nb + 1 , 0 );
 f_top.assign( ( f_b.size() >> kTop ) + 2 , 0 );
 std::uint64_t c = 0;
 constexpr std::size_t kTopBlocks = ( std::size_t( 1 ) << kTop ) / kBlock;
 for( std::size_t t = 0 ; t <= nb ; ++t ) {
  if( t % kTopBlocks == 0 ) f_top[ t / kTopBlocks ] = c;
  // the words of the block, none past the words (a multiple of 4)
  const bool in = 4 * t < W.size();
  const Word w0 = in ? W[ 4 * t ] : 0 , w1 = in ? W[ 4 * t + 1 ] : 0 ,
             w2 = in ? W[ 4 * t + 2 ] : 0 , w3 = in ? W[ 4 * t + 3 ] : 0;
  const std::uint64_t c1 = std::uint64_t( std::popcount( w0 ) ) ,
                      c2 = c1 + std::uint64_t( std::popcount( w1 ) ) ,
                      c3 = c2 + std::uint64_t( std::popcount( w2 ) );
  f_dir[ t ] = ( ( c - f_top[ t / kTopBlocks ] ) << ( 3 * kRel ) ) | c1 |
               ( c2 << kRel ) | ( c3 << ( 2 * kRel ) );
  c += c3 + std::uint64_t( std::popcount( w3 ) );
  }
 f_ones = std::size_t( c );
 for( const bool v : { true , false } )
  if( f_has[ v ] ) inventory( v );
 ADS_AUDIT( consistent() );
 }

inline const BitVector & RankSelect::bits( void ) const
{
 return( f_b );
 }

inline std::size_t RankSelect::size( void ) const
{
 return( f_b.size() );
 }

inline bool RankSelect::operator[]( std::size_t i ) const
{
 return( f_b[ i ] );
 }

inline std::size_t RankSelect::ones( void ) const
{
 return( f_ones );
 }

inline std::size_t RankSelect::zeros( void ) const
{
 return( f_b.size() - f_ones );
 }

inline std::size_t RankSelect::before( std::size_t t , bool b ) const
{
 std::size_t o = std::size_t( f_dir[ t ] >> ( 3 * kRel ) );
 if( f_big ) o += std::size_t( f_top[ ( t * kBlock ) >> kTop ] );
 return( b ? o : t * kBlock - o );
 }

inline std::size_t RankSelect::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_b.size() );
 const std::uint64_t e = f_dir[ i / kBlock ];
 const std::size_t q = ( i / kQuarter ) % 4;
 std::size_t r = std::size_t( ( e >> ( 3 * kRel ) ) +
                              ( ( ( e << kRel ) >> ( kRel * q ) ) &
                                kRelMask ) );
 if( f_big ) r += std::size_t( f_top[ i >> kTop ] );
 // the 1s of the word up to i (the words exist past the size [see
 // BitVector.h])
#ifdef __BMI2__
 return( r + std::size_t( std::popcount( _bzhi_u64( f_b.words()[ i / 64 ] ,
                                                    unsigned( i % 64 ) ) ) ) );
#else
 return( r + std::size_t( std::popcount(
                            f_b.words()[ i / 64 ] &
                            detail::lowMask( int( i % 64 ) ) ) ) );
#endif
 }

inline std::size_t RankSelect::rank0( std::size_t i ) const
{
 return( i - rank1( i ) );
 }

inline void RankSelect::inventory( bool b )
{
 Inventory & I = f_inv[ b ];
 const std::size_t total = b ? f_ones : zeros() , n = f_b.size();
 if( total == 0 ) return;
 // 2^l bits b in kGroup places on average, within a factor 1.5
 const double per = 1.5 * double( total ) * double( kGroup ) / double( n );
 I.l = per < 2 ? 0 : std::bit_width( std::uint64_t( per ) ) - 1;
 I.g = std::max( I.l - 5 , 0 );
 I.s = I.l - I.g;
 const std::size_t groups = ( ( total - 1 ) >> I.l ) + 1;
 I.pos.assign( groups + 1 , 0 );
 I.off.assign( groups << I.s , 0 );
 // the places of the bits b of rank 0, 2^g, 2 2^g, ...: the block of
 // each by the directory, a block at a time, its word by the counts of the
 // words of the block, and the place in the word by select
 const auto & W = f_b.words();
 const std::size_t G = std::size_t( 1 ) << I.g;
 std::size_t t = 0;
 for( std::size_t next = 0 ; next < total ; next += G ) {
  while( before( t + 1 , b ) <= next ) ++t;
  std::size_t r = next - before( t , b );   // the bits b before it
  const std::uint64_t e = f_dir[ t ];
  std::size_t q = 0 , in = 0;
  for( std::size_t h = 1 ; h < 4 ; ++h ) {
   const std::size_t c1 = std::size_t( ( e >> ( kRel * ( h - 1 ) ) ) &
                                       kRelMask );
   const std::size_t c = b ? c1 : h * kQuarter - c1;
   q = c <= r ? h : q;
   in = c <= r ? c : in;
   }
  r -= in;
  const std::size_t w = 4 * t + q;
  const std::size_t p = 64 * w +
                        std::size_t( selectInWord( b ? W[ w ] : ~W[ w ] ,
                                                   int( r ) ) );
  const std::size_t j = next >> I.l;
  const std::size_t m = ( next >> I.g ) & ( ( std::size_t( 1 ) << I.s ) - 1 );
  if( m == 0 ) I.pos[ j ] = p;
  else I.off[ ( j << I.s ) + m ] = std::uint16_t( p - I.pos[ j ] );
  }
 I.pos[ groups ] = n;
 for( std::size_t j = 0 ; j < groups ; ++j )   // the sparse groups
  if( I.pos[ j + 1 ] - ( I.pos[ j ] & ( ~Word( 0 ) >> 1 ) ) >= kSparse )
   I.pos[ j ] |= Word( 1 ) << 63;
 }

inline std::size_t RankSelect::select( std::size_t k , bool b ) const
{
 const Inventory & I = f_inv[ b ];
 const std::size_t r = k - 1;               // the bits b before it
 const std::size_t j = r >> I.l;
 const std::uint64_t h = I.pos[ j ];
 if( h >> 63 )
  return( selectSparse( k , b , std::size_t( h & ( ~Word( 0 ) >> 1 ) ) ,
                        std::size_t( I.pos[ j + 1 ] &
                                     ( ~Word( 0 ) >> 1 ) ) ) );
 // the place of the bit b of rank r rounded down to 2^g, and the bits b
 // to skip from there, word by word
 const std::size_t p = std::size_t( h ) +
                       I.off[ ( j << I.s ) + ( ( r >> I.g ) &
                                               ( ( std::size_t( 1 ) << I.s )
                                                 - 1 ) ) ];
 std::size_t t = r & ( ( std::size_t( 1 ) << I.g ) - 1 );
 const Word * W = f_b.words().data();
 std::size_t w = p / 64;
 Word x = ( b ? W[ w ] : ~W[ w ] ) & ( ~Word( 0 ) << ( p % 64 ) );
 for( ; ; ) {
  const std::size_t c = std::size_t( std::popcount( x ) );
  if( t < c ) break;
  t -= c;
  x = b ? W[ ++w ] : ~W[ ++w ];
  }
 return( 64 * w + std::size_t( selectInWord( x , int( t ) ) ) );
 }

inline std::size_t RankSelect::selectSparse( std::size_t k , bool b ,
                                             std::size_t p ,
                                             std::size_t q ) const
{
 // the last block with fewer than k bits b before it, between the blocks
 // of p and of q
 std::size_t lo = p / kBlock , hi = std::min( q / kBlock , f_dir.size() - 2 );
 while( lo < hi ) {
  const std::size_t mid = lo + ( hi - lo + 1 ) / 2;
  if( before( mid , b ) < k ) lo = mid;
  else hi = mid - 1;
  }
 std::size_t r = k - before( lo , b );      // the r-th one of the block
 // the word, by the counts before the words 1, 2 and 3
 const std::uint64_t e = f_dir[ lo ];
 std::size_t q4 = 0 , in = 0;
 for( std::size_t h = 1 ; h < 4 ; ++h ) {
  const std::size_t c1 = std::size_t( ( e >> ( kRel * ( h - 1 ) ) ) &
                                      kRelMask );
  const std::size_t c = b ? c1 : h * kQuarter - c1;
  if( c < r ) {
   q4 = h;
   in = c;
   }
  }
 r -= in;
 const Word x = f_b.words()[ ( kBlock / 64 ) * lo + q4 ];
 return( 64 * ( ( kBlock / 64 ) * lo + q4 ) +
         std::size_t( selectInWord( b ? x : ~x , int( r - 1 ) ) ) );
 }

inline std::size_t RankSelect::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) && f_has[ 1 ] );
 const std::size_t p = select( k , true );
 ADS_POST( f_b[ p ] && ( rank1( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t RankSelect::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) && f_has[ 0 ] );
 const std::size_t p = select( k , false );
 ADS_POST( ! f_b[ p ] && ( rank0( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t RankSelect::directoryBits( void ) const
{
 std::size_t s = 64 * ( f_dir.size() + f_top.size() );
 for( const auto & I : f_inv ) s += 64 * I.pos.size() + 16 * I.off.size();
 return( s );
 }

inline bool RankSelect::consistent( void ) const
{
 std::size_t c = 0;
 for( std::size_t i = 0 ; i <= f_b.size() ; ++i ) {
  if( rank1( i ) != c ) return( false );
  if( i < f_b.size() && f_b[ i ] ) ++c;
  }
 return( c == f_ones );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RankSelect.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
