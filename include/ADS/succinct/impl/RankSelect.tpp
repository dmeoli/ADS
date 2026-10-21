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
 directory( s , nullptr );
 }

inline RankSelect::RankSelect( std::span< const Word > w , std::size_t n ,
                               Selects s )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 if( n < kFuse ) {                         // a copy, then a count in cache
  f_b = BitVector( w , n );
  directory( s , nullptr );
  return;
  }
 f_b.f_n = n;
 f_b.f_w.clear();
 f_b.f_w.resize( BitVector::wordsFor( n ) );   // written by directory()
 directory( s , w.data() );
 }

inline void RankSelect::directory( Selects s , const Word * w )
{
 f_has[ 1 ] = ( s == Selects::ONES ) || ( s == Selects::BOTH );
 f_has[ 0 ] = ( s == Selects::ZEROS ) || ( s == Selects::BOTH );
 const std::size_t n = f_b.size();
 const std::size_t nb = n / kBlock + 1;    // and one more
 f_big = ( ( nb * kBlock ) >> kTop ) > 0;   // the last block counts too
 f_dir.resize( nb + 1 );                   // each one written below
 if( f_big ) f_top.assign( ( n >> kTop ) + 2 , 0 );
 else f_top.clear();
 Word * const P = f_b.f_w.data();
 const std::size_t nw = f_b.f_w.size();     // a multiple of 8
 std::uint64_t c = 0;
 constexpr std::size_t kTopBlocks = ( std::size_t( 1 ) << kTop ) / kBlock;
 // the entries of the blocks [ from , to ) from the words Q (copied to
 // the bits if Copy), a top block at a time: the sideways sums of the 4
 // words of a block in a byte each, their prefix sums by one product, the
 // 1s before the block since its top block above them
 auto run = [ & ]< bool Copy >( std::size_t from , std::size_t to ,
                                const Word * Q ) {
  Word * const D = f_dir.data();
  for( std::size_t t = from ; t < to ; ) {
   const std::size_t end = std::min( to , ( t / kTopBlocks + 1 ) *
                                          kTopBlocks );
   if( f_big && ( t % kTopBlocks == 0 ) ) f_top[ t / kTopBlocks ] = c;
   const std::uint64_t top = f_big ? f_top[ t / kTopBlocks ] : 0;
   std::uint64_t cc = c;
   for( ; t < end ; ++t ) {
    const Word x0 = Q[ 4 * t ] , x1 = Q[ 4 * t + 1 ] , x2 = Q[ 4 * t + 2 ] ,
               x3 = Q[ 4 * t + 3 ];
    if constexpr( Copy ) {
     P[ 4 * t ] = x0;
     P[ 4 * t + 1 ] = x1;
     P[ 4 * t + 2 ] = x2;
     P[ 4 * t + 3 ] = x3;
     }
    const std::uint64_t r =
     ( ( std::uint64_t( std::popcount( x0 ) ) |
         ( std::uint64_t( std::popcount( x1 ) ) << 8 ) |
         ( std::uint64_t( std::popcount( x2 ) ) << 16 ) ) * 0x010101ULL ) &
     0xFFFFFF;
    D[ t ] = ( ( cc - top ) << ( 3 * kRel ) ) | r;
    cc += ( r >> 16 ) + std::uint64_t( std::popcount( x3 ) );
    }
   c = cc;
   }
  };
 std::size_t t = 0;
 if( w ) {
  // the blocks of 4 whole words of w copied as they are counted, then
  // the rest of w, its last word cut at n, and 0s up to the end
  const std::size_t sw = ( n + 63 ) / 64;
  t = ( n / 64 ) / 4;
  run.template operator()< true >( 0 , t , w );
  for( std::size_t i = 4 * t ; i < nw ; ++i ) P[ i ] = i < sw ? w[ i ] : 0;
  if( n % 64 ) P[ n / 64 ] &= detail::lowMask( int( n % 64 ) );
  }
 // the blocks whose words exist, then those past them, with no bit (the
 // words read past the vector are 0)
 const std::size_t full = std::min( nb + 1 , nw / 4 );
 run.template operator()< false >( t , full , P );
 for( t = full ; t <= nb ; ++t ) {
  if( f_big && ( t % kTopBlocks == 0 ) ) f_top[ t / kTopBlocks ] = c;
  f_dir[ t ] = ( c - ( f_big ? f_top[ t / kTopBlocks ] : 0 ) ) <<
               ( 3 * kRel );
  }
 f_ones = std::size_t( c );
 ADS_INVARIANT( f_b.clean() );
 ADS_AUDIT( consistent() );
 }

inline void RankSelect::adopt( const RankSelect & o , bool move )
{
 for( const bool v : { false , true } )
  if( o.f_ready[ v ].load( std::memory_order_acquire ) ) {
   if( move ) f_inv[ v ] = std::move( o.f_inv[ v ] );
   else f_inv[ v ] = o.f_inv[ v ];
   std::call_once( f_flag[ v ] , [](){} );
   f_ready[ v ].store( true , std::memory_order_release );
   }
 }

inline RankSelect::RankSelect( const RankSelect & o )
 : f_b( o.f_b ) , f_dir( o.f_dir ) , f_top( o.f_top ) , f_big( o.f_big ) ,
   f_has{ o.f_has[ 0 ] , o.f_has[ 1 ] } , f_ones( o.f_ones )
{
 adopt( o , false );
 }

inline RankSelect::RankSelect( RankSelect && o ) noexcept
 : f_b( std::move( o.f_b ) ) , f_dir( std::move( o.f_dir ) ) ,
   f_top( std::move( o.f_top ) ) , f_big( o.f_big ) ,
   f_has{ o.f_has[ 0 ] , o.f_has[ 1 ] } , f_ones( o.f_ones )
{
 adopt( o , true );
 }

inline RankSelect & RankSelect::operator=( const RankSelect & o )
{
 if( this != &o ) {
  RankSelect c( o );
  *this = std::move( c );
  }
 return( *this );
 }

inline RankSelect & RankSelect::operator=( RankSelect && o ) noexcept
{
 if( this != &o ) {
  // the flags cannot be reset: the object is rebuilt in place
  this->~RankSelect();
  ::new( static_cast< void * >( this ) ) RankSelect( std::move( o ) );
  }
 return( *this );
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

inline void RankSelect::shape( bool b , Inventory & I ,
                               std::size_t & groups ) const
{
 const std::size_t total = b ? f_ones : zeros() , n = f_b.size();
 groups = 0;
 if( total == 0 ) return;
 // 2^l bits b in kGroup places on average, within a factor 1.5
 const double per = 1.5 * double( total ) * double( kGroup ) / double( n );
 I.l = per < 2 ? 0 : std::bit_width( std::uint64_t( per ) ) - 1;
 I.g = std::max( I.l - 5 , 0 );
 I.s = I.l - I.g;
 groups = ( ( total - 1 ) >> I.l ) + 1;
 }

inline const RankSelect::Inventory & RankSelect::inventory( bool b ) const
{
 if( ! f_ready[ b ].load( std::memory_order_acquire ) ) [[unlikely]]
  first( b );
 return( f_inv[ b ] );
 }

#if defined( __GNUC__ )
__attribute__(( noinline , cold ))
#elif defined( _MSC_VER )
__declspec( noinline )
#endif
inline void RankSelect::first( bool b ) const
{
 std::call_once( f_flag[ b ] , [ this , b ]() { build( b ); } );
 f_ready[ b ].store( true , std::memory_order_release );
 }

inline void RankSelect::build( bool b ) const
{
 Inventory & I = f_inv[ b ];
 const std::size_t total = b ? f_ones : zeros() , n = f_b.size();
 std::size_t groups;
 shape( b , I , groups );
 if( total == 0 ) return;
 I.pos.assign( groups + 1 , 0 );
 I.off.assign( groups << I.s , 0 );
 const auto & W = f_b.words();
 const std::size_t M = ( std::size_t( 1 ) << I.s ) - 1;
 std::size_t base = 0;                     // the place of the group
 auto put = [ & ]( std::size_t c , std::size_t p ) {   // the bit b of rank c
  if( ( ( c >> I.g ) & M ) == 0 ) I.pos[ c >> I.l ] = base = p;
  I.off[ c >> I.g ] = std::uint16_t( p - base );
  };
 if( I.g == 0 ) {
  // every bit b has its place: the bits of each word, from the lowest
  const std::size_t nw = ( n + 63 ) / 64;
  std::size_t c = 0;
  for( std::size_t w = 0 ; w < nw ; ++w ) {
   Word x = b ? W[ w ] : ~W[ w ];
   if( w + 1 == nw ) x &= detail::lowMask( int( n - 64 * w ) );
   for( ; x ; x &= x - 1 , ++c )
    put( c , 64 * w + std::size_t( std::countr_zero( x ) ) );
   }
  }
 else {
  // the bits b of rank 0, 2^g, 2 2^g, ..., block by block: the bits b
  // before the block and before its words 1, 2 and 3 once a block, then
  // the word of each one by three comparisons and its place by select
  const std::size_t G = std::size_t( 1 ) << I.g;
  std::size_t next = 0 , lo = 0;
  for( std::size_t t = 0 ; next < total ; ++t ) {
   // (the 0s past the size, in the last block, are not counted)
   const std::size_t hi = std::min( before( t + 1 , b ) , total );
   if( next >= hi ) {
    lo = hi;
    continue;
    }
   const std::uint64_t e = f_dir[ t ];
   std::size_t c1 = std::size_t( e & kRelMask ) ,
               c2 = std::size_t( ( e >> kRel ) & kRelMask ) ,
               c3 = std::size_t( ( e >> ( 2 * kRel ) ) & kRelMask );
   if( ! b ) {
    c1 = kQuarter - c1;
    c2 = 2 * kQuarter - c2;
    c3 = 3 * kQuarter - c3;
    }
   for( ; next < hi ; next += G ) {
    const std::size_t r = next - lo;
    const std::size_t q = std::size_t( r >= c1 ) + std::size_t( r >= c2 ) +
                          std::size_t( r >= c3 );
    const std::size_t in = q == 0 ? 0 : q == 1 ? c1 : q == 2 ? c2 : c3;
    const Word x = W[ 4 * t + q ];
    put( next , 64 * ( 4 * t + q ) +
                std::size_t( selectInWord( b ? x : ~x , int( r - in ) ) ) );
    }
   lo = hi;
   }
  }
 I.pos[ groups ] = n;
 for( std::size_t j = 0 ; j < groups ; ++j )   // the sparse groups
  if( I.pos[ j + 1 ] - ( I.pos[ j ] & ( ~Word( 0 ) >> 1 ) ) >= kSparse )
   I.pos[ j ] |= Word( 1 ) << 63;
 }

inline std::size_t RankSelect::select( std::size_t k , bool b ) const
{
 const Inventory & I = inventory( b );
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
 if( I.g == 0 ) return( p );             // every bit b has its offset
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
 for( const bool v : { false , true } )
  if( f_has[ v ] ) {
   Inventory I;
   std::size_t groups;
   shape( v , I , groups );
   if( groups > 0 ) s += 64 * ( groups + 1 ) + ( 16 * groups << I.s );
   }
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
