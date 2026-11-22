/*--------------------------------------------------------------------------*/
/*--------------------------- File RankSelect.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RankSelect.h, with their contracts.
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

template< bool Lazy >
inline BasicRankSelect< Lazy >::BasicRankSelect( void )
    : BasicRankSelect( BitVector() )
{ }

template< bool Lazy >
inline BasicRankSelect< Lazy >::BasicRankSelect( BitVector b , Selects s )
    : f_b( std::move( b ) )
{
 directory( s , nullptr );
 }

template< bool Lazy >
inline BasicRankSelect< Lazy >::BasicRankSelect( std::span< const Word > w ,
						 std::size_t n , Selects s )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 if( n < kFuse ) {  // a copy, then a count in cache
  f_b = BitVector( w , n );
  directory( s , nullptr );
  return;
  }
 f_b.f_n = n;
 f_b.f_w.clear();
 f_b.f_w.resize( BitVector::wordsFor( n ) );  // written by directory()
 directory( s , w.data() );
 }

template< bool Lazy >
inline void BasicRankSelect< Lazy >::directory( Selects s , const Word * w )
{
 f_has[ 1 ] = ( s == Selects::ONES ) || ( s == Selects::BOTH );
 f_has[ 0 ] = ( s == Selects::ZEROS ) || ( s == Selects::BOTH );
 const std::size_t n = f_b.size();
 const std::size_t nb = n / kBlock + 1;  // and one more
 f_big = ( ( std::uint64_t( nb ) * kBlock ) >> kTop ) >
	 0;               // the last block counts too
 f_dir.resize( nb + 1 );  // each one written below
 if( f_big )
  f_top.assign( std::size_t( std::uint64_t( n ) >> kTop ) + 2 , 0 );
 else
  f_top.clear();
 Word * const P = f_b.f_w.data();
 const std::size_t nw = f_b.f_w.size();  // a multiple of 8
 std::uint64_t c = 0;
 // at most the largest std::size_t, where it has fewer than kTop bits
 constexpr std::size_t kTopBlocks = std::size_t(
   std::min< std::uint64_t >( ( std::uint64_t( 1 ) << kTop ) / kBlock ,
			      std::numeric_limits< std::size_t >::max() ) );
 // the entries of the blocks [ from , to ) from the words Q (copied to
 // the bits if Copy), a top block at a time: the sideways sums of the 4
 // words of a block in a byte each, their prefix sums by one product, the
 // 1s before the block since its top block above them
 auto run = [ & ]< bool Copy >( std::size_t from , std::size_t to ,
				const Word * Q ) {
  Word * const D = f_dir.data();
  for( std::size_t t = from ; t < to ; ) {
   const std::size_t end = std::min( to ,
				     ( t / kTopBlocks + 1 ) * kTopBlocks );
   if( f_big && ( t % kTopBlocks == 0 ) )
    f_top[ t / kTopBlocks ] = c;
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
    D[ t ] = entry( x0 , x1 , x2 , x3 , cc , top );
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
  for( std::size_t i = 4 * t ; i < nw ; ++i )
   P[ i ] = i < sw ? w[ i ] : 0;
  if( n % 64 )
   P[ n / 64 ] &= detail::lowMask( int( n % 64 ) );
  }
 // the blocks whose words exist, then those past them, with no bit (the
 // words read past the vector are 0)
 const std::size_t full = std::min( nb + 1 , nw / 4 );
 run.template operator()< false >( t , full , P );
 for( t = full ; t <= nb ; ++t ) {
  if( f_big && ( t % kTopBlocks == 0 ) )
   f_top[ t / kTopBlocks ] = c;
  f_dir[ t ] = ( c - ( f_big ? f_top[ t / kTopBlocks ] : 0 ) ) << ( 3 * kRel );
  }
 f_ones = std::size_t( c );
 if constexpr( ! Lazy )  // the inventories at once
  for( const bool v : { false , true } )
   if( f_has[ v ] )
    build( v );
 ADS_INVARIANT( f_b.clean() );
 ADS_AUDIT( consistent() );
 }

template< bool Lazy >
template< Executor E >
BasicRankSelect< Lazy >::BasicRankSelect( E & ex , BitVector b , Selects s )
    : f_b( std::move( b ) )
{
 directory( ex , s , nullptr );
 }

template< bool Lazy >
template< Executor E >
BasicRankSelect< Lazy >::BasicRankSelect( E & ex , std::span< const Word > w ,
					  std::size_t n , Selects s )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 f_b.f_n = n;
 f_b.f_w.clear();
 f_b.f_w.resize( BitVector::wordsFor( n ) );  // written by directory()
 directory( ex , s , w.data() );
 }

template< bool Lazy >
template< Executor E >
void BasicRankSelect< Lazy >::directory( E & ex , Selects s , const Word * w )
{
 const std::size_t n = f_b.size();
 const std::size_t nb = n / kBlock + 1;  // and one more
 // at most the largest std::size_t, where it has fewer than kTop bits
 constexpr std::size_t kTopBlocks = std::size_t(
   std::min< std::uint64_t >( ( std::uint64_t( 1 ) << kTop ) / kBlock ,
			      std::numeric_limits< std::size_t >::max() ) );
 // the chunks of blocks [see detail::chunkUnits()]
 const std::size_t cs =
   detail::chunkUnits( nb + 1 , kChunk , kBlocks , kTopBlocks );
 const std::size_t q = ( nb + 1 + cs - 1 ) / cs;  // the chunks
 if( ( ex.concurrency() == 1 ) || ( q == 1 ) ) {
  directory( s , w );  // the sequential pass
  return;
  }
 f_has[ 1 ] = ( s == Selects::ONES ) || ( s == Selects::BOTH );
 f_has[ 0 ] = ( s == Selects::ZEROS ) || ( s == Selects::BOTH );
 f_big = ( ( std::uint64_t( nb ) * kBlock ) >> kTop ) >
	 0;  // the last block counts too
 f_dir.resize( nb + 1 );
 if( f_big )
  f_top.assign( std::size_t( std::uint64_t( n ) >> kTop ) + 2 , 0 );
 else
  f_top.clear();
 Word * const P = f_b.f_w.data();
 const std::size_t nw = f_b.f_w.size();  // a multiple of 8
 // the 1s of the words of each chunk, copied from w first if given
 std::vector< std::uint64_t > c( q + 1 , 0 );
 parallelFor(
   ex , 0 , q ,
   [ & ]( std::size_t k ) {
    c[ k + 1 ] = detail::copyCount(
      P , w , n , std::min( nw , 4 * k * cs ) ,
      k + 1 == q ? nw : std::min( nw , 4 * ( k + 1 ) * cs ) );
    } ,
   1 );
 for( std::size_t k = 0 ; k < q ; ++k )
  c[ k + 1 ] += c[ k ];
 if( f_big )  // the top blocks that start one
  for( std::size_t t = 0 ; t <= nb ; t += kTopBlocks )
   if( t % cs == 0 )
    f_top[ t / kTopBlocks ] = c[ t / cs ];
 // the entries of each chunk from the 1s before it
 parallelFor(
   ex , 0 , q ,
   [ & ]( std::size_t k ) {
    Word * const D = f_dir.data();
    std::uint64_t cc = c[ k ];
    const std::size_t t1 = std::min( nb + 1 , ( k + 1 ) * cs );
    for( std::size_t t = k * cs ; t < t1 ; ++t ) {
     if( f_big && ( t % kTopBlocks == 0 ) && ( t % cs != 0 ) )
      f_top[ t / kTopBlocks ] = cc;  // a top block inside the chunk
     const std::uint64_t top = f_big ? f_top[ t / kTopBlocks ] : 0;
     if( 4 * t + 3 >= nw ) {  // no word: no bit
      D[ t ] = ( cc - top ) << ( 3 * kRel );
      continue;
      }
     D[ t ] = entry( P[ 4 * t ] , P[ 4 * t + 1 ] , P[ 4 * t + 2 ] ,
		     P[ 4 * t + 3 ] , cc , top );
     }
    } ,
   1 );
 f_ones = std::size_t( c[ q ] );
 if constexpr( ! Lazy )  // the inventories at once
  for( const bool v : { false , true } )
   if( f_has[ v ] )
    build( ex , v );
 ADS_INVARIANT( f_b.clean() );
 ADS_AUDIT( consistent() );
 }

template< bool Lazy >
template< class F >
void BasicRankSelect< Lazy >::samples( bool b , std::size_t t0 ,
				       std::size_t t1 ,
				       int e , std::size_t total ,
				       F && f ) const
{
 const auto & W = f_b.words();
 const std::size_t G = std::size_t( 1 ) << e;
 std::size_t lo = std::min( before( t0 , b ) , total );
 std::size_t next = ( lo + G - 1 ) & ~( G - 1 );
 // the bits b before the block and before its words 1, 2 and 3 once a
 // block, then the word of each sample by three comparisons and its place
 // by select
 for( std::size_t t = t0 ; ( t < t1 ) && ( next < total ) ; ++t ) {
  const std::size_t hi = std::min( before( t + 1 , b ) , total );
  if( next >= hi ) {
   lo = hi;
   continue;
   }
  const std::uint64_t en = f_dir[ t ];
  std::size_t c1 = std::size_t( en & kRelMask ) ,
	      c2 = std::size_t( ( en >> kRel ) & kRelMask ) ,
	      c3 = std::size_t( ( en >> ( 2 * kRel ) ) & kRelMask );
  if( ! b ) {
   c1 = kQuarter - c1;
   c2 = 2 * kQuarter - c2;
   c3 = 3 * kQuarter - c3;
   }
  for( ; next < hi ; next += G ) {
   const std::size_t r = next - lo;
   const std::size_t qq =
     std::size_t( r >= c1 ) + std::size_t( r >= c2 ) + std::size_t( r >= c3 );
   const std::size_t in = qq == 0 ? 0 : qq == 1 ? c1 : qq == 2 ? c2 : c3;
   const Word x = W[ 4 * t + qq ];
   f( next , 64 * ( 4 * t + qq ) +
	      std::size_t( selectInWord( b ? x : ~x , int( r - in ) ) ) );
   }
  lo = hi;
  }
 }

template< bool Lazy >
template< Executor E >
void BasicRankSelect< Lazy >::build( E & ex , bool b ) const
{
 Inventory & I = f_inv[ b ];
 const std::size_t total = b ? f_ones : zeros() , n = f_b.size();
 const std::size_t nb = f_dir.size() - 1;  // the blocks with their entry
 std::size_t groups;
 shape( b , I , groups );
 if( total == 0 )
  return;
 I.pos.assign( groups + 1 , 0 );
 I.off.assign( groups << I.s , 0 );
 // the place of the first bit b of each group, then the offsets of the
 // bits b of rank multiple of 2^g from the place of their group, each
 // pass on chunks of blocks
 parallelForBlocks(
   ex , 0 , nb ,
   [ & ]( std::size_t t0 , std::size_t t1 ) {
    samples( b , t0 , t1 , I.l , total ,
	     [ & ]( std::size_t c , std::size_t p ) {
     I.pos[ c >> I.l ] = p;
     } );
    } ,
   kChunk );
 parallelForBlocks(
   ex , 0 , nb ,
   [ & ]( std::size_t t0 , std::size_t t1 ) {
    samples( b , t0 , t1 , I.g , total ,
	     [ & ]( std::size_t c , std::size_t p ) {
     I.off[ c >> I.g ] = std::uint16_t( p - I.pos[ c >> I.l ] );
     } );
    } ,
   kChunk );
 I.pos[ groups ] = n;
 // the sparse groups, marked once all the places are read
 std::vector< std::uint8_t > sparse( groups , 0 );
 parallelFor(
   ex , 0 , groups ,
   [ & ]( std::size_t j ) {
    sparse[ j ] = I.pos[ j + 1 ] - I.pos[ j ] >= kSparse;
    } ,
   kChunk );
 parallelFor(
   ex , 0 , groups ,
   [ & ]( std::size_t j ) {
    if( sparse[ j ] )
     I.pos[ j ] |= Word( 1 ) << 63;
    } ,
   kChunk );
 }

template< bool Lazy >
inline void BasicRankSelect< Lazy >::adopt( const BasicRankSelect & o ,
					    bool move )
{
 for( const bool v : { false , true } )
  if constexpr( ! Lazy ) {  // all of them, built at once
   if( move )
    f_inv[ v ] = std::move( o.f_inv[ v ] );
   else
    f_inv[ v ] = o.f_inv[ v ];
   }
  else if( o.f_once.ready[ v ].load( std::memory_order_acquire ) ) {
   if( move )
    f_inv[ v ] = std::move( o.f_inv[ v ] );
   else
    f_inv[ v ] = o.f_inv[ v ];
   std::call_once( f_once.flag[ v ] , []() { } );
   f_once.ready[ v ].store( true , std::memory_order_release );
   }
 }

template< bool Lazy >
inline BasicRankSelect< Lazy >::BasicRankSelect( const BasicRankSelect & o )
    : f_b( o.f_b ) , f_dir( o.f_dir ) , f_top( o.f_top ) , f_big( o.f_big ) ,
      f_has
{ o.f_has[ 0 ] , o.f_has[ 1 ] } , f_ones( o.f_ones )
{
 adopt( o , false );
 }

template< bool Lazy >
inline BasicRankSelect< Lazy >::BasicRankSelect(
  BasicRankSelect && o ) noexcept
    : f_b( std::move( o.f_b ) ) , f_dir( std::move( o.f_dir ) ) ,
      f_top( std::move( o.f_top ) ) , f_big( o.f_big ) ,
      f_has
{ o.f_has[ 0 ] , o.f_has[ 1 ] } , f_ones( o.f_ones )
{
 adopt( o , true );
 }

template< bool Lazy >
inline BasicRankSelect< Lazy > &
BasicRankSelect< Lazy >::operator=( const BasicRankSelect & o )
{
 if( this != &o ) {
  BasicRankSelect c( o );
  *this = std::move( c );
  }
 return( *this );
 }

template< bool Lazy >
inline BasicRankSelect< Lazy > &
BasicRankSelect< Lazy >::operator=( BasicRankSelect && o ) noexcept
{
 if( this != &o ) {
  // the flags cannot be reset: the object is rebuilt in place
  this->~BasicRankSelect();
  ::new( static_cast< void * >( this ) ) BasicRankSelect( std::move( o ) );
  }
 return( *this );
 }

template< bool Lazy >
inline const BitVector & BasicRankSelect< Lazy >::bits( void ) const
{
 return( f_b );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::size( void ) const
{
 return( f_b.size() );
 }

template< bool Lazy >
inline bool BasicRankSelect< Lazy >::operator[]( std::size_t i ) const
{
 return( f_b[ i ] );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::ones( void ) const
{
 return( f_ones );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::zeros( void ) const
{
 return( f_b.size() - f_ones );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::before( std::size_t t ,
						    bool b ) const
{
 std::size_t o = std::size_t( f_dir[ t ] >> ( 3 * kRel ) );
 if( f_big )
  o += std::size_t(
    f_top[ std::size_t( ( std::uint64_t( t ) * kBlock ) >> kTop ) ] );
 return( b ? o : t * kBlock - o );
 }

template< bool Lazy >
inline Word BasicRankSelect< Lazy >::entry( Word x0 , Word x1 , Word x2 ,
					    Word x3 ,
					    std::uint64_t & c ,
					    std::uint64_t top )
{
 // the sideways sums of the words 0 to 2 in a byte each, their prefix
 // sums by one product
 const std::uint64_t r = ( ( std::uint64_t( std::popcount( x0 ) ) |
			     ( std::uint64_t( std::popcount( x1 ) ) << 8 ) |
			     ( std::uint64_t( std::popcount( x2 ) ) << 16 ) ) *
			   0x010101ULL ) &
			 0xFFFFFF;
 const Word e = ( ( c - top ) << ( 3 * kRel ) ) | r;
 c += ( r >> 16 ) + std::uint64_t( std::popcount( x3 ) );
 return( e );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_b.size() );
 const std::uint64_t e = f_dir[ i / kBlock ];
 const std::size_t q = ( i / kQuarter ) % 4;
 std::size_t r = std::size_t(
   ( e >> ( 3 * kRel ) ) + ( ( ( e << kRel ) >> ( kRel * q ) ) & kRelMask ) );
 if( f_big )
  r += std::size_t( f_top[ std::size_t( std::uint64_t( i ) >> kTop ) ] );
 // the 1s of the word up to i (the words exist past the size [see
 // BitVector.h])
 return( r + std::size_t( std::popcount(
		f_b.words()[ i / 64 ] &
		( ( std::uint64_t( 1 ) << ( i % 64 ) ) - 1 ) ) ) );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::rank0( std::size_t i ) const
{
 return( i - rank1( i ) );
 }

template< bool Lazy >
inline void BasicRankSelect< Lazy >::shape( bool b , Inventory & I ,
					    std::size_t & groups ) const
{
 const std::size_t total = b ? f_ones : zeros() , n = f_b.size();
 groups = 0;
 if( total == 0 )
  return;
 // 2^l bits b in kGroup places on average, within a factor 1.5
 const double per = 1.5 * double( total ) * double( kGroup ) / double( n );
 I.l = per < 2 ? 0 : std::bit_width( std::uint64_t( per ) ) - 1;
 I.g = std::max( I.l - 5 , 0 );
 I.s = I.l - I.g;
 groups = ( ( total - 1 ) >> I.l ) + 1;
 }

template< bool Lazy >
ADS_ALWAYS_INLINE const typename BasicRankSelect< Lazy >::Inventory &
BasicRankSelect< Lazy >::inventory( bool b ) const
{
 if constexpr( Lazy )
  if( ! f_once.ready[ b ].load( std::memory_order_acquire ) ) [[unlikely]]
   first( b );
 return( f_inv[ b ] );
 }

template< bool Lazy >
ADS_COLD_PATH inline void BasicRankSelect< Lazy >::first( bool b ) const
{
 std::call_once( f_once.flag[ b ] , [ this , b ]() { build( b ); } );
 f_once.ready[ b ].store( true , std::memory_order_release );
 }

template< bool Lazy >
inline void BasicRankSelect< Lazy >::build( bool b ) const
{
 Inventory & I = f_inv[ b ];
 const std::size_t total = b ? f_ones : zeros() , n = f_b.size();
 std::size_t groups;
 shape( b , I , groups );
 if( total == 0 )
  return;
 I.pos.assign( groups + 1 , 0 );
 I.off.assign( groups << I.s , 0 );
 const auto & W = f_b.words();
 const std::size_t M = ( std::size_t( 1 ) << I.s ) - 1;
 std::size_t base = 0;                               // the place of the group
 auto put = [ & ]( std::size_t c , std::size_t p ) {  // the bit b of rank c
  if( ( ( c >> I.g ) & M ) == 0 )
   I.pos[ c >> I.l ] = base = p;
  I.off[ c >> I.g ] = std::uint16_t( p - base );
  };
 if( I.g == 0 ) {
  // every bit b has its place: the bits of each word, from the lowest
  const std::size_t nw = ( n + 63 ) / 64;
  std::size_t c = 0;
  for( std::size_t w = 0 ; w < nw ; ++w ) {
   Word x = b ? W[ w ] : ~W[ w ];
   if( w + 1 == nw )
    x &= detail::lowMask( int( n - 64 * w ) );
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
    const std::size_t q =
      std::size_t( r >= c1 ) + std::size_t( r >= c2 ) + std::size_t( r >= c3 );
    const std::size_t in = q == 0 ? 0 : q == 1 ? c1 : q == 2 ? c2 : c3;
    const Word x = W[ 4 * t + q ];
    put( next , 64 * ( 4 * t + q ) +
		 std::size_t( selectInWord( b ? x : ~x , int( r - in ) ) ) );
    }
   lo = hi;
   }
  }
 I.pos[ groups ] = n;
 for( std::size_t j = 0 ; j < groups ; ++j )  // the sparse groups
  if( I.pos[ j + 1 ] - ( I.pos[ j ] & ( ~Word( 0 ) >> 1 ) ) >= kSparse )
   I.pos[ j ] |= Word( 1 ) << 63;
 }

template< bool Lazy >
ADS_ALWAYS_INLINE std::size_t BasicRankSelect< Lazy >::select( std::size_t k ,
							       bool b ) const
{
 const Inventory & I = inventory( b );
 const std::size_t r = k - 1;  // the bits b before it
 const std::size_t j = r >> I.l;
 const std::uint64_t h = I.pos[ j ];
 if( h >> 63 )
  return(
    selectSparse( k , b , std::size_t( h & ( ~Word( 0 ) >> 1 ) ) ,
		  std::size_t( I.pos[ j + 1 ] & ( ~Word( 0 ) >> 1 ) ) ) );
 // the place of the bit b of rank r rounded down to 2^g, and the bits b
 // to skip from there, word by word
 const std::size_t p =
   std::size_t( h ) +
   I.off[ ( j << I.s ) +
	  ( ( r >> I.g ) & ( ( std::size_t( 1 ) << I.s ) - 1 ) ) ];
 if( I.g == 0 )
  return( p );  // every bit b has its offset
 std::size_t t = r & ( ( std::size_t( 1 ) << I.g ) - 1 );
 const Word * W = f_b.words().data();
 std::size_t w = p / 64;
 Word x = ( b ? W[ w ] : ~W[ w ] ) & ( ~Word( 0 ) << ( p % 64 ) );
 for( ; ; ) {
  const std::size_t c = std::size_t( std::popcount( x ) );
  if( t < c )
   break;
  t -= c;
  x = b ? W[ ++w ] : ~W[ ++w ];
  }
 return( 64 * w + std::size_t( selectInWord( x , int( t ) ) ) );
 }

template< bool Lazy >
inline std::size_t
BasicRankSelect< Lazy >::selectSparse( std::size_t k , bool b , std::size_t p ,
				       std::size_t q ) const
{
 // the last block with fewer than k bits b before it, between the blocks
 // of p and of q
 std::size_t lo = p / kBlock , hi = std::min( q / kBlock , f_dir.size() - 2 );
 while( lo < hi ) {
  const std::size_t mid = lo + ( hi - lo + 1 ) / 2;
  if( before( mid , b ) < k )
   lo = mid;
  else
   hi = mid - 1;
  }
 std::size_t r = k - before( lo , b );  // the r-th one of the block
 // the word, by the counts before the words 1, 2 and 3
 const std::uint64_t e = f_dir[ lo ];
 std::size_t q4 = 0 , in = 0;
 for( std::size_t h = 1 ; h < 4 ; ++h ) {
  const std::size_t c1 =
    std::size_t( ( e >> ( kRel * ( h - 1 ) ) ) & kRelMask );
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

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) && f_has[ 1 ] );
 return( detail::selected( *this , k , true , select( k , true ) ) );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) && f_has[ 0 ] );
 return( detail::selected( *this , k , false , select( k , false ) ) );
 }

template< bool Lazy >
inline std::size_t BasicRankSelect< Lazy >::directoryBits( void ) const
{
 std::size_t s = 64 * ( f_dir.size() + f_top.size() );
 for( const bool v : { false , true } )
  if( f_has[ v ] ) {
   Inventory I;
   std::size_t groups;
   shape( v , I , groups );
   if( groups > 0 )
    s += 64 * ( groups + 1 ) + ( 16 * groups << I.s );
   }
 return( s );
 }

template< bool Lazy >
inline bool BasicRankSelect< Lazy >::consistent( void ) const
{
 return( detail::rankConsistent( *this ) );
 }

/*--------------------------------------------------------------------------*/

template< bool Lazy >
inline bool BasicRankSelect< Lazy >::fits( void ) const
{
 const std::size_t n = f_b.size() , nb = n / kBlock + 1;
 return( ( f_dir.size() == nb + 1 ) && ( f_ones <= n ) &&
	  ( f_big == ( ( ( std::uint64_t( nb ) * kBlock ) >> kTop ) > 0 ) ) &&
	  ( f_top.size() ==
	    ( f_big ? std::size_t( std::uint64_t( n ) >> kTop ) + 2 : 0 ) ) );
 }

/*--------------------------------------------------------------------------*/

template< bool Lazy >
inline bool BasicRankSelect< Lazy >::fits( bool b ) const
{
 Inventory S;
 std::size_t groups;
 shape( b , S , groups );
 const Inventory & I = f_inv[ b ];
 if( groups == 0 )
  return( I.pos.empty() && I.off.empty() );
 return( ( I.l == S.l ) && ( I.g == S.g ) && ( I.s == S.s ) &&
	  ( I.pos.size() == groups + 1 ) &&
	  ( I.off.size() == groups << I.s ) );
 }

/*--------------------------------------------------------------------------*/

template< bool Lazy >
template< class A >
inline void BasicRankSelect< Lazy >::persist( A & a )
{
 a( f_b , f_dir , f_top , f_big , f_has , f_ones );
 a.require( fits() , "the directory of a RankSelect" );
 for( const bool v : { false , true } ) {
  bool built = f_has[ v ];  // the inventories built, of a Lazy one
  if constexpr( Lazy ) {
   if constexpr( ! A::loading )
    built = f_once.ready[ v ].load( std::memory_order_acquire );
   a( built );
   }
  if( ! built )
   continue;
  a( f_inv[ v ] );
  a.require( f_has[ v ] && fits( v ) , "an inventory of a RankSelect" );
  if constexpr( Lazy && A::loading ) {
   std::call_once( f_once.flag[ v ] , []() { } );
   f_once.ready[ v ].store( true , std::memory_order_release );
   }
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RankSelect.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
