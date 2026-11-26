/*--------------------------------------------------------------------------*/
/*----------------------- File ConsensusRecSplit.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConsensusRecSplit [see ConsensusRecSplit.h], with their
 * contracts.
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

/*--------------------------------------------------------------------------*/

template< class T , std::size_t K , class H >
ConsensusRecSplit< T , K ,
		   H >::ConsensusRecSplit( const std::vector< T > & keys ,
						 double epsilon ,
						 std::uint64_t seed , H h )
    : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 Sequential ex;
 make( ex , keys , epsilon );
 }

template< class T , std::size_t K , class H >
template< Executor E >
ConsensusRecSplit< T , K , H >::ConsensusRecSplit( E & ex ,
						 const std::vector< T > & keys ,
						 double epsilon ,
						 std::uint64_t seed , H h )
    : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 make( ex , keys , epsilon );
 }

template< class T , std::size_t K , class H >
template< Executor E >
void ConsensusRecSplit< T , K , H >::make( E & ex ,
					   const std::vector< T > & keys ,
					 double epsilon )
{
 ADS_PRE( ( std::uint64_t( keys.size() ) < ( std::uint64_t( 1 ) << 32 ) ) &&
	  ( epsilon > 0 ) && ( epsilon <= 1 ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > S( keys );
  std::sort( S.begin() , S.end() );
  return( std::adjacent_find( S.begin() , S.end() ) == S.end() );
  }() );
 // the bits of a split of s keys, log2( 2^s / binom( s , s / 2 ) ) plus
 // the overhead, larger for the larger nodes
 for( std::size_t l = 0 ; l < kLevels ; ++l ) {
  const double s = double( K >> l ) ,
	       b = s -
		   ( std::lgamma( s + 1 ) - 2 * std::lgamma( s / 2 + 1 ) ) /
		     std::log( 2.0 ) +
		   epsilon / 3.4 * std::pow( s , 0.75 );
  f_micro[ l ] = std::uint64_t( std::ceil( b * double( 1 << 20 ) ) );
  }
 if( f_n == 0 )
  return;
 std::vector< std::uint64_t > Y( f_n );  // the hashes of the keys to place
 parallelFor( ex , 0 , f_n ,
	      [ & ]( std::size_t i ) { Y[ i ] = hash( keys[ i ] ); } );
 if constexpr( ! ( std::is_integral_v< T > &&
		   std::is_same_v< H , FastHash< T > > )) {
  // two keys of one hash would never split; an integer is its own base
  std::vector< std::uint64_t > D( Y );
  radixSort( D );
  if( std::adjacent_find( D.begin() , D.end() ) != D.end() )
   throw( std::runtime_error( "ConsensusRecSplit: two keys have the same "
			      "hash" ) );
  }
 const std::size_t nb = f_nb = f_n / K , last = f_n - nb * K;
 // the K-perfect function: two layers of thresholds, the hash of a key
 // in the layer 1 splitmix64() of its hash
 std::vector< std::uint64_t > B( nb * K );  // the keys by bucket
 std::vector< std::uint32_t > fill( nb , 0 );
 std::vector< std::uint64_t > freeAt , Z;  // the bucket of each free place
 std::vector< std::uint32_t > start;
 f_thr = BitVector( nb * kThresholdBits , false );
 std::size_t L = std::size_t( std::ceil( kOverload * double( nb ) ) );
 for( std::size_t y = 0 ; ( y < 2 ) && ! Y.empty() ; ++y ) {
  const std::size_t lb = f_lbase[ y ];
  if( y == 1 ) {
   if( nb <= lb )
    break;
   L = nb - lb;
   }
  if( L == 0 )
   break;
  f_expect[ y ] = std::uint64_t(
    4294967295.0 *
    std::min( 1.0 , double( L * K ) / double( Y.size() ) / kOverload ) );
  f_lbase[ y + 1 ] = lb + L;
  f_layers = y + 1;
  auto layer = [ & ]( std::uint64_t x ) {
   return( y == 0 ? x : splitmix64( x ) );
   };
  auto at = [ & ]( std::uint64_t x ) {
   return( std::size_t( ( ( x & 0xFFFFFFFFULL ) * L ) >> 32 ) );
   };
  auto th = [ & ]( std::uint64_t x ) { return( layer( x ) >> 32 ); };
  // the keys by bucket, by a counting sort
  start.assign( L + 1 , 0 );
  for( const std::uint64_t x : Y )
   ++start[ at( layer( x ) ) + 1 ];
  for( std::size_t b = 0 ; b < L ; ++b )
   start[ b + 1 ] += start[ b ];
  Z.resize( Y.size() );
  {
   std::vector< std::uint32_t > next( start.begin() , start.end() - 1 );
   for( const std::uint64_t x : Y )
    Z[ next[ at( layer( x ) ) ]++ ] = x;
   }
  Y.clear();  // the keys bumped
  for( std::size_t b = 0 ; b < L ; ++b ) {
   // the keys of the scaled thresholds up to t are kept, at most K of
   // them: t is that of the K-th least threshold, less 1 if the next one
   // has the same
   const auto i = Z.begin() + start[ b ] , j = Z.begin() + start[ b + 1 ];
   std::uint64_t t = kRange - 1;
   if( std::size_t( j - i ) > K ) {
    auto byTh = [ & ]( std::uint64_t u , std::uint64_t v ) {
     return( th( u ) < th( v ) );
     };
    std::nth_element( i , i + ( K - 1 ) , j , byTh );
    t = scale( th( i[ K - 1 ] ) , y );
    if( scale( th( *std::min_element( i + K , j , byTh ) ) , y ) == t )
     --t;  // the K-th and the next one cannot be told apart
    }
   f_thr.put( ( lb + b ) * kThresholdBits , t , int( kThresholdBits ) );
   std::size_t kept = 0;
   for( auto e = i ; e != j ; ++e )
    if( scale( th( *e ) , y ) <= t )
     B[ ( lb + b ) * K + kept++ ] = *e;
    else
     Y.push_back( *e );
   fill[ lb + b ] = std::uint32_t( kept );
   freeAt.insert( freeAt.end() , K - kept , lb + b );
   }
  }
 // the keys bumped by both layers: the fallback sends each one to a free
 // place of a bucket, or to one of the last values
 for( std::size_t j = 0 ; j < last ; ++j )
  freeAt.push_back( nb + j );
 if( freeAt.size() != Y.size() )
  throw( std::logic_error( "ConsensusRecSplit: the free places are not "
			   "the keys bumped" ) );
 f_fallback = FingerprintPerfectHash< std::uint64_t >( Y );
 if( ! freeAt.empty() )
  f_free = EliasFano( freeAt );
 for( const std::uint64_t x : Y )
  if( const std::size_t b = std::size_t( freeAt[ f_fallback( x ) ] ); b < nb )
   B[ b * K + fill[ b ]++ ] = x;
 // the seeds of the levels, from the root down, by chunks of buckets,
 // each chunk after its root of 64 bits and on whole words
 if( nb > 0 ) {
  const std::size_t g = std::min( nb , std::size_t( 1 ) << kChunkLog ) ,
		    chunks = ( nb + ( std::size_t( 1 ) << kChunkLog ) - 1 ) >>
			     kChunkLog;
  for( std::size_t l = 0 ; l < kLevels ; ++l ) {
   f_chunk[ l ] = end( l , ( g << l ) - 1 ) + 64;
   if( chunks > 1 )  // on whole words, written by one thread each
    f_chunk[ l ] = ( f_chunk[ l ] + 63 ) / 64 * 64;
   f_off[ l + 1 ] = f_off[ l ] + chunks * f_chunk[ l ];
   }
  f_seeds = BitVector( f_off[ kLevels ] , false );
  [ & ]< std::size_t... l >( std::index_sequence< l... > ) {
   ( level< l >( ex , B ) , ... );
   }( std::make_index_sequence< kLevels >() );
  }
 ADS_AUDIT( [ & ]( void ) {
  std::vector< char > seen( f_n , 0 );
  for( const T & k : keys ) {
   const std::size_t v = ( *this )( k );
   if( ( v >= f_n ) || seen[ v ] )
    return( false );
   seen[ v ] = 1;
   }
  return( true );
  }() );
 }

/*--------------------------------------------------------------------------*/

template< class T , std::size_t K , class H >
std::uint64_t ConsensusRecSplit< T , K , H >::base( const T & k ) const
{
 if constexpr( std::is_integral_v< T > && std::is_same_v< H , FastHash< T > > )
  return( std::uint64_t( k ) );  // mixed by splitmix64()
 else if constexpr( detail::kAvalanching64< H , T > )
  return( detail::hash64( f_h , k ) );
 else
  return( hashWord( detail::hash64( f_h , k ) ) );
 }

template< class T , std::size_t K , class H >
std::uint64_t ConsensusRecSplit< T , K , H >::hash( const T & k ) const
{
 return( splitmix64( base( k ) ^ f_seed ) );
 }

template< class T , std::size_t K , class H >
bool ConsensusRecSplit< T , K , H >::left( std::uint64_t h ,
					 std::uint64_t sigma )
{
 return( ( splitmix64( h + sigma ) >> 63 ) != 0 );
 }

template< class T , std::size_t K , class H >
std::uint64_t ConsensusRecSplit< T , K , H >::end( std::size_t l ,
						 std::size_t i ) const
{
 return( ( f_micro[ l ] * std::uint64_t( i + 1 ) ) >> 20 );
 }

template< class T , std::size_t K , class H >
std::uint64_t ConsensusRecSplit< T , K , H >::scale( std::uint64_t t ,
						   std::size_t y ) const
{
 // a scale of kRange - 1 steps on the last 1 / kTrim of the expected
 // threshold, 0 kept for a bucket that keeps no key
 const std::uint64_t e = f_expect[ y ] ,
		     r = std::max< std::uint64_t >( 1 , e / kTrim ) ,
		     lo = e - std::min( e , r );
 if( t < lo )
  return( 1 );
 return( std::min( kRange - 1 , 1 + ( kRange - 1 ) * ( t - lo ) / r ) );
 }

template< class T , std::size_t K , class H >
std::size_t ConsensusRecSplit< T , K , H >::bucket( std::uint64_t h ) const
{
 std::uint64_t x = h;
 for( std::size_t y = 0 ; y < f_layers ; ++y ) {
  if( y > 0 )
   x = splitmix64( x );
  const std::size_t lb = f_lbase[ y ] ,
		    b = lb + std::size_t( ( ( x & 0xFFFFFFFFULL ) *
					    ( f_lbase[ y + 1 ] - lb ) ) >>
					  32 );
  if( scale( x >> 32 , y ) <=
      f_thr.get( b * kThresholdBits , int( kThresholdBits ) ) )
   return( b );
  }
 if( f_fallback.empty() )
  return( 0 );  // a key out of S, when no key of S was bumped twice
 return( std::size_t( f_free.access( f_fallback( h ) ) ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , std::size_t K , class H >
template< std::size_t l , Executor E >
void ConsensusRecSplit< T , K , H >::level( E & ex ,
					  std::vector< std::uint64_t > & X )
{
 const std::size_t chunks =
   ( f_nb + ( std::size_t( 1 ) << kChunkLog ) - 1 ) >> kChunkLog;
 parallelFor(
   ex , 0 , chunks ,
   [ & ]( std::size_t c ) {
    const std::size_t first = c << kChunkLog ,
		      g = std::min( f_nb - first , std::size_t( 1 )
						    << kChunkLog );
    search< l >( X.data() + first * K , g << l ,
		 f_off[ l ] + c * f_chunk[ l ] );
    } ,
   1 );
 }

template< class T , std::size_t K , class H >
template< std::size_t l >
void ConsensusRecSplit< T , K , H >::search( std::uint64_t * X ,
					   std::size_t tasks ,
					   std::uint64_t o )
{
 // the seed of the task i is the word of the 64 bits of the level from
 // E_i, the bit E_i in its bit 0, so that its fragment is in the top
 // ell_i bits and the root in the low bits of the seed of the task 0
 constexpr std::size_t s = K >> l , half = s / 2;
 // in a local, since the writes of the seeds could alias the members
 const std::uint64_t micro = f_micro[ l ];
 auto end = [ micro ]( std::size_t i ) {
  return( ( micro * std::uint64_t( i + 1 ) ) >> 20 );
  };
 // the keys counted with no branch, which the compiler unrolls and
 // vectorizes, by blocks of 64 above them with the test of the two halves
 // after each block
 auto splits = [ & ]( std::size_t i , std::uint64_t sigma ) {
  const std::uint64_t * P = X + i * s;
  std::size_t a = 0;
  if constexpr( s <= 64 ) {
   for( std::size_t q = 0 ; q < s ; ++q )
    a += std::size_t( splitmix64( P[ q ] + sigma ) >> 63 );
   return( a == half );
   }
  for( std::size_t j = 0 ; j < s ; j += 64 ) {
   const std::size_t e = std::min< std::size_t >( s , j + 64 );
   for( std::size_t q = j ; q < e ; ++q )
    a += std::size_t( splitmix64( P[ q ] + sigma ) >> 63 );
   if( ( a > half ) || ( e - a > half ) )
    return( false );
   }
  return( true );
  };
 // E_{i-1} and E_i, the fragment of the task i in the bits of `top` and
 // its least step
 std::size_t i = 0;
 std::uint64_t e0 = 0 , e1 = end( 0 ) , sigma = 0;
 auto widths = [ & ]( std::uint64_t & top , std::uint64_t & step ) {
  top = ~std::uint64_t( 0 ) << ( 64 - ( e1 - e0 ) );
  step = std::uint64_t( 1 ) << ( 64 - ( e1 - e0 ) );
  };
 std::uint64_t top , step;
 widths( top , step );
 for( ; ; ) {
  if( splits( i , sigma ) ) {
   f_seeds.put( o + e1 , sigma , 64 );
   if( ++i == tasks )
    break;
   e0 = e1;
   e1 = end( i );
   widths( top , step );
   sigma >>= e1 - e0;  // the next fragment, 0
   continue;
   }
  if( sigma < top ) {  // the fragment is not all 1s
   sigma += step;
   continue;
   }
  while( ( sigma >= top ) && ( i > 0 ) ) {
   --i;  // back to the task before, as it was written
   e1 = e0;
   e0 = i == 0 ? 0 : end( i - 1 );
   widths( top , step );
   sigma = f_seeds.get( o + e1 , 64 );
   }
  if( sigma >= top )
   sigma = ( sigma & ~top ) + 1;  // the root, the fragment 0
  else
   sigma += step;
  }
 if( s > 2 )  // the halves of each task, the left one first
  for( std::size_t t = 0 ; t < tasks ; ++t ) {
   const std::uint64_t sg = f_seeds.get( o + end( t ) , 64 );
   std::partition( X + t * s , X + ( t + 1 ) * s ,
		   [ & ]( std::uint64_t x ) { return( left( x , sg ) ); } );
   }
 }

/*--------------------------------------------------------------------------*/

template< class T , std::size_t K , class H >
std::size_t ConsensusRecSplit< T , K , H >::operator()( const T & k ) const
{
 ADS_PRE( f_n >= 1 );
 const std::uint64_t h = hash( k );
 std::size_t t = bucket( h );
 if( t >= f_nb )
  return( t - f_nb + K * f_nb );  // one of the last n mod K values
 for( std::size_t l = 0 ; l < kLevels ; ++l ) {
  // the chunk of the node t and its place in the chunk
  const std::size_t c = t >> ( kChunkLog + l );
  const std::uint64_t at =
    f_off[ l ] + c * f_chunk[ l ] + end( l , t - ( c << ( kChunkLog + l ) ) );
  t = 2 * t + ( left( h , f_seeds.get( at , 64 ) ) ? 0 : 1 );
  }
 return( t );
 }

template< class T , std::size_t K , class H >
std::size_t ConsensusRecSplit< T , K , H >::size( void ) const
{
 return( f_n );
 }

template< class T , std::size_t K , class H >
bool ConsensusRecSplit< T , K , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , std::size_t K , class H >
std::size_t ConsensusRecSplit< T , K , H >::bits( void ) const
{
 return( 64 * ( f_seeds.words().size() + f_thr.words().size() ) +
	  f_fallback.bits() + f_free.sizeInBits() +
	  8 * ( sizeof( *this ) - sizeof( f_fallback ) ) );
 }

template< class T , std::size_t K , class H >
std::size_t ConsensusRecSplit< T , K , H >::buckets( void ) const
{
 return( f_nb );
 }

template< class T , std::size_t K , class H >
std::size_t ConsensusRecSplit< T , K , H >::bumped( void ) const
{
 return( f_fallback.size() );
 }

/*--------------------------------------------------------------------------*/

template< class T , std::size_t K , class H >
template< class A >
inline void ConsensusRecSplit< T , K , H >::persist( A & a )
{
 a( f_seed , f_n , f_nb , f_layers , f_lbase , f_expect , f_thr , f_fallback ,
    f_free ,
    f_micro , f_off , f_seeds );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( ( f_nb == f_n / K ) && ( f_layers <= 2 ) &&
	      ( f_seeds.size() == f_off[ kLevels ] ) &&
	      ( f_thr.size() == f_nb * kThresholdBits ) ,
	    "the seeds of a ConsensusRecSplit" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File ConsensusRecSplit.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
