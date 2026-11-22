/*--------------------------------------------------------------------------*/
/*----------------------- File CompactRankSelect.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of CompactRankSelect.h, with their contracts.
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

inline CompactRankSelect::CompactRankSelect( BitVector b )
    : f_b( std::move( b ) )
{
 directory( nullptr );
 }

inline CompactRankSelect::CompactRankSelect( std::span< const Word > w ,
					     std::size_t n )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 if( n < kFuse ) {  // a copy, then a count in cache
  f_b = BitVector( w , n );
  directory( nullptr );
  return;
  }
 f_b.f_n = n;
 f_b.f_w.clear();
 f_b.f_w.resize( BitVector::wordsFor( n ) );  // written by directory()
 directory( w.data() );
 }

inline void CompactRankSelect::directory( const Word * w )
{
 const std::size_t n = f_b.size() , ns = n / kSuper + 1;  // and one more
 Word * const P = f_b.f_w.data();
 const std::size_t nw = f_b.f_w.size();  // a multiple of 8
 f_e.resize( ns + 1 );
 const std::uint64_t bits = std::uint64_t( ns + 1 ) * kSuper;
 f_top.assign( std::size_t( bits >> kTop ) + 1 , 0 );
 for( auto & S : f_s ) {  // at most n / kSample + 2 each
  S.clear();
  S.reserve( n / kSample + 2 );
  }
 // at most the largest std::size_t, where it has fewer than kTop bits
 constexpr std::size_t kTopSupers = std::size_t(
   std::min< std::uint64_t >( ( std::uint64_t( 1 ) << kTop ) / kSuper ,
			      std::numeric_limits< std::size_t >::max() ) );
 constexpr std::size_t kW = kSuper / 64;  // the words of a superblock
 std::uint64_t c = 0 , top = 0;
 std::size_t next1 = 1 , next0 = 1;  // the next samples
 // the entry of the superblock s from its words Q (copied to the bits if
 // Copy): the 1s before the superblock since its top region in 32 bits
 // and those before each block since the superblock in 12 each; the
 // samples whose bit falls in it (the 0s only those before n)
 auto super = [ & ]< bool Copy >( std::size_t s , const Word * Q ) {
  if( s % kTopSupers == 0 )
   f_top[ s / kTopSupers ] = top = c;
  std::uint64_t v[ 9 ];  // the 1s before each block
  v[ 0 ] = 0;
  for( std::size_t k = 0 ; k < 8 ; ++k ) {
   std::uint64_t in = 0;
   for( std::size_t j = 0 ; j < 8 ; ++j ) {
    const Word x = Q[ 8 * k + j ];
    if constexpr( Copy )
     P[ kW * s + 8 * k + j ] = x;
    in += std::uint64_t( std::popcount( x ) );
    }
   v[ k + 1 ] = v[ k ] + in;
   }
  f_e[ s ] = entry( v , c - top );
  c += v[ 8 ];
  if( s < ns ) {
   const std::size_t end1 = std::size_t( c ) ,
		     end0 = std::min( n , ( s + 1 ) * kSuper ) - end1;
   for( ; next1 <= end1 ; next1 += kSample )
    f_s[ 1 ].push_back( std::uint32_t( s ) );
   for( ; next0 <= end0 ; next0 += kSample )
    f_s[ 0 ].push_back( std::uint32_t( s ) );
   }
  };
 std::size_t s = 0;
 if( w ) {
  // the superblocks of whole words of w copied as they are counted, then
  // the rest of w, its last word cut at n, and 0s up to the end
  const std::size_t sw = ( n + 63 ) / 64;
  for( ; kW * ( s + 1 ) <= n / 64 ; ++s )
   super.template operator()< true >( s , w + kW * s );
  for( std::size_t i = kW * s ; i < nw ; ++i )
   P[ i ] = i < sw ? w[ i ] : 0;
  if( n % 64 )
   P[ n / 64 ] &= detail::lowMask( int( n % 64 ) );
  }
 // the superblocks whose words exist, then the rest from a line of 0s
 for( ; ( s <= ns ) && ( kW * ( s + 1 ) <= nw ) ; ++s )
  super.template operator()< false >( s , P + kW * s );
 for( ; s <= ns ; ++s ) {
  std::array< Word , kW > Z{};
  for( std::size_t i = kW * s ; i < nw ; ++i )
   Z[ i - kW * s ] = P[ i ];
  super.template operator()< false >( s , Z.data() );
  }
 f_ones = std::size_t( c );
 closeSamples();
 ADS_INVARIANT( f_b.clean() );
 ADS_AUDIT( consistent() );
 }

inline void CompactRankSelect::closeSamples( void )
{
 // the superblock of the last bit b closes the samples
 const std::size_t ns = f_b.size() / kSuper + 1;
 for( const bool v : { false , true } )
  if( ! f_s[ v ].empty() ) {
   const std::size_t total = v ? f_ones : zeros();
   std::size_t t = f_s[ v ].back();
   while( ( t + 1 < ns ) && ( before( t + 1 , v ) < total ) )
    ++t;
   f_s[ v ].push_back( std::uint32_t( t ) );
   }
 }

template< Executor E >
CompactRankSelect::CompactRankSelect( E & ex , BitVector b )
    : f_b( std::move( b ) )
{
 directory( ex , nullptr );
 }

template< Executor E >
CompactRankSelect::CompactRankSelect( E & ex , std::span< const Word > w ,
				      std::size_t n )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 f_b.f_n = n;
 f_b.f_w.clear();
 f_b.f_w.resize( BitVector::wordsFor( n ) );  // written by directory()
 directory( ex , w.data() );
 }

template< Executor E >
void CompactRankSelect::directory( E & ex , const Word * w )
{
 const std::size_t n = f_b.size() , ns = n / kSuper + 1;  // and one more
 // at most the largest std::size_t, where it has fewer than kTop bits
 constexpr std::size_t kTopSupers = std::size_t(
   std::min< std::uint64_t >( ( std::uint64_t( 1 ) << kTop ) / kSuper ,
			      std::numeric_limits< std::size_t >::max() ) );
 constexpr std::size_t kW = kSuper / 64;  // the words of a superblock
 // the chunks of superblocks [see detail::chunkUnits()]
 const std::size_t cs =
   detail::chunkUnits( ns + 1 , kChunk , kBlocks , kTopSupers );
 const std::size_t q = ( ns + 1 + cs - 1 ) / cs;  // the chunks
 if( ( ex.concurrency() == 1 ) || ( q == 1 ) ) {
  directory( w );  // the sequential pass
  return;
  }
 Word * const P = f_b.f_w.data();
 const std::size_t nw = f_b.f_w.size();  // a multiple of 8
 f_e.resize( ns + 1 );
 const std::uint64_t bits = std::uint64_t( ns + 1 ) * kSuper;
 f_top.assign( std::size_t( bits >> kTop ) + 1 , 0 );
 // the 1s of the words of each chunk, copied from w first if given
 std::vector< std::uint64_t > c( q + 1 , 0 );
 parallelFor(
   ex , 0 , q ,
   [ & ]( std::size_t k ) {
    c[ k + 1 ] = detail::copyCount(
      P , w , n , std::min( nw , kW * k * cs ) ,
      k + 1 == q ? nw : std::min( nw , kW * ( k + 1 ) * cs ) );
    } ,
   1 );
 for( std::size_t k = 0 ; k < q ; ++k )
  c[ k + 1 ] += c[ k ];
 f_ones = std::size_t( c[ q ] );
 for( std::size_t t = 0 ; t <= ns ; t += kTopSupers )
  if( t % cs == 0 )
   f_top[ t / kTopSupers ] = c[ t / cs ];
 // the samples of rank 1 , 1 + kSample , ... of each value
 const std::size_t z = n - f_ones;
 f_s[ 1 ].assign( f_ones == 0 ? 0 : ( f_ones - 1 ) / kSample + 1 , 0 );
 f_s[ 0 ].assign( z == 0 ? 0 : ( z - 1 ) / kSample + 1 , 0 );
 // the entries and the samples of each chunk from the 1s before it
 parallelFor(
   ex , 0 , q ,
   [ & ]( std::size_t k ) {
    const std::size_t s0 = k * cs , s1 = std::min( ns + 1 , ( k + 1 ) * cs );
    std::uint64_t cc = c[ k ];
    const std::size_t z0 = std::min( n , s0 * kSuper ) - std::size_t( cc );
    std::size_t j1 = cc == 0 ? 0 : ( std::size_t( cc ) - 1 ) / kSample + 1 ,
		j0 = z0 == 0 ? 0 : ( z0 - 1 ) / kSample + 1;
    for( std::size_t s = s0 ; s < s1 ; ++s ) {
     if( ( s % kTopSupers == 0 ) && ( s % cs != 0 ) )
      f_top[ s / kTopSupers ] = cc;  // a top region inside the chunk
     const std::uint64_t top = f_top[ s / kTopSupers ];
     std::uint64_t v[ 9 ];  // the 1s before each block
     v[ 0 ] = 0;
     for( std::size_t b = 0 ; b < 8 ; ++b ) {
      std::uint64_t in = 0;
      for( std::size_t j = 0 ; j < 8 ; ++j ) {
       const std::size_t i = kW * s + 8 * b + j;
       if( i < nw )
	in += std::uint64_t( std::popcount( P[ i ] ) );
       }
      v[ b + 1 ] = v[ b ] + in;
      }
     f_e[ s ] = entry( v , cc - top );
     cc += v[ 8 ];
     if( s < ns ) {
      const std::size_t end1 = std::size_t( cc ) ,
			end0 = std::min( n , ( s + 1 ) * kSuper ) - end1;
      for( ; 1 + j1 * kSample <= end1 ; ++j1 )
       f_s[ 1 ][ j1 ] = std::uint32_t( s );
      for( ; 1 + j0 * kSample <= end0 ; ++j0 )
       f_s[ 0 ][ j0 ] = std::uint32_t( s );
      }
     }
    } ,
   1 );
 closeSamples();
 ADS_INVARIANT( f_b.clean() );
 ADS_AUDIT( consistent() );
 }

inline CompactRankSelect::Entry
CompactRankSelect::entry( const std::uint64_t ( &v )[ 9 ] , std::uint64_t c )
{
 // the count of block k at the bit 32 + 12 k of ( hi , lo )
 return( Entry{ c | ( v[ 1 ] << 44 ) | ( v[ 2 ] << 56 ) ,
		 ( v[ 2 ] >> 8 ) | ( v[ 3 ] << 4 ) | ( v[ 4 ] << 16 ) |
		   ( v[ 5 ] << 28 ) | ( v[ 6 ] << 40 ) | ( v[ 7 ] << 52 ) } );
 }

inline std::size_t CompactRankSelect::before( std::size_t s , bool b ) const
{
 const std::size_t o = std::size_t(
   f_top[ std::size_t( ( std::uint64_t( s ) * kSuper ) >> kTop ) ] +
   ( f_e[ s ].lo & 0xFFFFFFFFULL ) );
 return( b ? o : s * kSuper - o );
 }

inline std::size_t CompactRankSelect::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_b.size() );
 const Entry & e = f_e[ i / kSuper ];
 std::size_t r =
   std::size_t( f_top[ std::size_t( std::uint64_t( i ) >> kTop ) ] +
		( e.lo & 0xFFFFFFFFULL ) ) +
   inSuper( e , ( i / kBlock ) % 8 );
 // the 1s of the line of the block before i [see detail::lineRank()]
 return( r +
	  detail::lineRank< kScalar >( f_b.words().data() + 8 * ( i / kBlock ) ,
				       unsigned( i % kBlock ) ) );
 }

inline std::size_t CompactRankSelect::select( std::size_t k , bool b ) const
{
 const auto & S = f_s[ b ];
 const std::size_t j = ( k - 1 ) / kSample;
 std::size_t lo = S[ j ] , hi = S[ j + 1 ];
 while( lo < hi ) {  // the last superblock with fewer than k
  const std::size_t mid = lo + ( hi - lo + 1 ) / 2;
  if( before( mid , b ) < k )
   lo = mid;
  else
   hi = mid - 1;
  }
 std::size_t r = k - before( lo , b );  // the r-th of the superblock
 // the block, by the counts of the blocks 1 to 7, and its count
 const Entry & e = f_e[ lo ];
 std::size_t q = 0 , in = 0;
 for( std::size_t h = 1 ; h < 8 ; ++h ) {
  const std::size_t c1 = inSuper( e , h );
  const std::size_t c = b ? c1 : h * kBlock - c1;
  q = c < r ? h : q;
  in = c < r ? c : in;
  }
 r -= in;
 // the word, by the sums of the 8 words of the line
 const Word * w = f_b.words().data() + ( kSuper / 64 ) * lo + 8 * q;
 std::size_t p = 0 , sum = 0 , sp = 0;  // sp: the bits b before word p
 for( std::size_t h = 0 ; h < 7 ; ++h ) {
  sum += std::size_t( std::popcount( b ? w[ h ] : ~w[ h ] ) );
  const bool past = sum < r;
  p += past;
  sp = past ? sum : sp;
  }
 return(
   64 * ( ( kSuper / 64 ) * lo + 8 * q + p ) +
   std::size_t( selectInWord( b ? w[ p ] : ~w[ p ] , int( r - sp - 1 ) ) ) );
 }

inline std::size_t CompactRankSelect::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 return( detail::selected( *this , k , true , select( k , true ) ) );
 }

inline std::size_t CompactRankSelect::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 return( detail::selected( *this , k , false , select( k , false ) ) );
 }

inline std::size_t CompactRankSelect::directoryBits( void ) const
{
 return( 128 * f_e.size() + 64 * f_top.size() +
	  32 * ( f_s[ 0 ].size() + f_s[ 1 ].size() ) );
 }

inline bool CompactRankSelect::consistent( void ) const
{
 return( detail::rankConsistent( *this ) );
 }

/*--------------------------------------------------------------------------*/

template< class A >
inline void CompactRankSelect::persist( A & a )
{
 a( f_b , f_e , f_top , f_s , f_ones );
 const std::size_t n = f_b.size() , ns = n / kSuper + 1;
 a.require(
   ( f_e.size() == ns + 1 ) &&
     ( f_top.size() ==
       std::size_t( ( std::uint64_t( ns + 1 ) * kSuper ) >> kTop ) + 1 ) &&
     ( f_ones <= n ) && ( f_s[ 0 ].size() <= n / kSample + 2 ) &&
     ( f_s[ 1 ].size() <= n / kSample + 2 ) ,
   "the directory of a CompactRankSelect" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File CompactRankSelect.tpp --------------------*/
/*--------------------------------------------------------------------------*/
