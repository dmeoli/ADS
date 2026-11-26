/*--------------------------------------------------------------------------*/
/*---------------------------- File PtrHash.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PtrHash [see PtrHash.h], with their contracts.
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

namespace detail
{

/// the high half of the product a b on 128 bits, a place of [ 0 , b )
[[nodiscard]] constexpr std::uint64_t ptrHashHigh( std::uint64_t a ,
						   std::uint64_t b )
{
 return( product128( a , b ).hi );
 }

/// the cubic bucket function of PtrHash on a fraction r of 2^64,
/// 255 / 256 r^2 ( 1 + r ) / 2 + r / 256
[[nodiscard]] constexpr std::uint64_t ptrHashCubic( std::uint64_t r )
{
 const std::uint64_t r2 = ptrHashHigh( r , r ) ,
		     r3 = ptrHashHigh( r2 , ( r >> 1 ) |
					     ( std::uint64_t( 1 ) << 63 ) );
 return( r3 / 256 * 255 + r / 256 );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
PtrHash< T , F , R , H >::PtrHash( const std::vector< T > & keys ,
				   double lambda ,
				double alpha , std::uint64_t seed , H h )
    : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 Sequential ex;
 make( ex , keys , lambda , alpha );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
template< Executor E >
PtrHash< T , F , R , H >::PtrHash( E & ex , const std::vector< T > & keys ,
				double lambda , double alpha ,
				std::uint64_t seed , H h )
    : f_seed( seed ) , f_n( keys.size() ) , f_h( h )
{
 make( ex , keys , lambda , alpha );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
template< Executor E >
void PtrHash< T , F , R , H >::make( E & ex , const std::vector< T > & keys ,
				  double lambda , double alpha )
{
 ADS_PRE( ( std::uint64_t( keys.size() ) < ( std::uint64_t( 1 ) << 32 ) ) &&
	  ( lambda >= 1 ) && ( alpha >= 0.5 ) && ( alpha <= 1 ) );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > S( keys );
  std::sort( S.begin() , S.end() );
  return( std::adjacent_find( S.begin() , S.end() ) == S.end() );
  }() );
 if( f_n == 0 )
  return;
 // the parts of kappa keys, the bound of Vigna halved; their slots, odd and
 // not multiples of 3, and their buckets, odd
 const double eps = ( 1 - alpha ) / 2;
 if( eps > 0 ) {
  const double x = double( f_n ) * eps * eps / 2 ,
	       kappa = 4 / ( eps * eps ) * std::max( 1.0 , std::log( x ) );
  f_P = std::max< std::size_t >( 1 , std::size_t( double( f_n ) / kappa ) );
  }
 const std::size_t per = f_n / f_P;
 f_S = std::max< std::size_t >(
   1 , std::size_t( std::ceil( double( per ) / alpha ) ) );
 while( ( f_S % 2 == 0 ) || ( f_S % 3 == 0 ) )
  ++f_S;
 f_B = std::size_t( std::ceil( double( per ) / lambda ) ) + 8;
 if( f_B % 2 == 0 )
  ++f_B;
 f_mS = ~std::uint64_t( 0 ) / f_S + 1;  // 0 for S = 1, the place 0
 for( ; ; ) {
  if( ++f_trials > kMaxTrials )
   throw( std::runtime_error( "PtrHash: no seed separates the keys" ) );
  if( build( ex , keys ) )
   break;
  f_seed = hashWord( f_seed , f_trials );  // a new seed
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

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::uint64_t PtrHash< T , F , R , H >::base( const T & k ) const
{
 if constexpr( std::is_integral_v< T > && std::is_same_v< H , FastHash< T > > )
  return( std::uint64_t( k ) );  // mixed by splitmix64()
 else if constexpr( detail::kAvalanching64< H , T > )
  return( detail::hash64( f_h , k ) );
 else
  return( hashWord( detail::hash64( f_h , k ) ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::uint64_t PtrHash< T , F , R , H >::hash( const T & k ) const
{
 return( splitmix64( base( k ) ^ f_seed ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::bucket( std::uint64_t h ) const
{
 if constexpr( F == PtrHashBucket::Linear )
  // floor( P B h / 2^64 ) / B is the part floor( P h / 2^64 )
  return( std::size_t( detail::ptrHashHigh( h , f_P * f_B ) ) );
 else {
  // the part and what of h it leaves, then the bucket of the rest
  const Product128 p = product128( h , f_P );
  return(
    std::size_t( p.hi ) * f_B +
    std::size_t( detail::ptrHashHigh( detail::ptrHashCubic( p.lo ) , f_B ) ) );
  }
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::inPart( std::uint64_t h ,
					   std::uint64_t hp ) const
{
 // the fast modulo of Lemire, Kaser and Kurz by S
 return( std::size_t( detail::ptrHashHigh( f_mS * ( h ^ hp ) , f_S ) ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::uint64_t PtrHash< T , F , R , H >::pilotHash( std::uint64_t p ) const
{
 return( kC * ( p ^ f_seed ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::value( std::uint64_t h ,
					  std::size_t b ) const
{
 const std::size_t t = std::size_t( detail::ptrHashHigh( h , f_P ) ) * f_S +
		       inPart( h , pilotHash( f_pilot[ b ] ) );
 if( t < f_n )
  return( t );
 if constexpr( R == PtrHashRemap::EliasFano )
  return( std::size_t( f_remap.access( t - f_n ) ) );
 else
  return( std::size_t( f_remap[ t - f_n ] ) );
 }

/*--------------------------------------------------------------------------*/

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::ptrdiff_t PtrHash< T , F , R , H >::part( std::size_t q ,
					    std::span< const std::uint64_t > K ,
					    std::uint64_t * W )
{
 const std::size_t S = f_S , B = f_B;
 std::size_t evicted = 0;
 SplitMix64 g( f_seed ^ q );  // the same pilots on any thread
 std::uint8_t * pilot = f_pilot.data() + q * B;
 // the buckets of the part, consecutive in K since the hashes are sorted
 std::vector< std::uint32_t > start( B + 1 , 0 );
 for( const std::uint64_t h : K )
  ++start[ bucket( h ) - q * B + 1 ];
 std::size_t smax = 0;
 for( std::size_t b = 0 ; b < B ; ++b ) {
  smax = std::max< std::size_t >( smax , start[ b + 1 ] );
  start[ b + 1 ] += start[ b ];
  }
 auto len = [ & ]( std::size_t b ) {
  return( std::size_t( start[ b + 1 ] - start[ b ] ) );
  };
 // the buckets by decreasing size, by a counting sort of the sizes
 std::vector< std::uint32_t > bySize( smax + 2 , 0 ) , order( B );
 for( std::size_t b = 0 ; b < B ; ++b )
  ++bySize[ smax - len( b ) + 1 ];
 for( std::size_t s = 0 ; s <= smax ; ++s )
  bySize[ s + 1 ] += bySize[ s ];
 for( std::size_t b = 0 ; b < B ; ++b )
  order[ bySize[ smax - len( b ) ]++ ] = std::uint32_t( b );
 auto isTaken = [ & ]( std::size_t t ) {
  return( ( W[ t >> 6 ] >> ( t & 63 ) ) & 1 );
  };
 auto flip = [ & ]( std::size_t t ) {
  W[ t >> 6 ] ^= std::uint64_t( 1 ) << ( t & 63 );
  };
 auto keys = [ & ]( std::size_t b ) {
  return( K.subspan( start[ b ] , len( b ) ) );
  };
 // the first pilot that sends the keys of the bucket to free slots,
 // distinct among them, the slots then taken; kPilots if none
 std::vector< std::size_t > T0( smax );
 auto firstFree = [ & ]( std::span< const std::uint64_t > Kb ) {
  for( std::size_t p = 0 ; p < kPilots ; ++p ) {
   const std::uint64_t hp = pilotHash( p );
   bool hit = false;
   for( const std::uint64_t h : Kb )  // no early exit, fewer branches
    hit = hit | bool( isTaken( inPart( h , hp ) ) );
   if( hit )
    continue;
   std::size_t i = 0;
   for( ; i < Kb.size() ; ++i ) {
    T0[ i ] = inPart( Kb[ i ] , hp );
    if( isTaken( T0[ i ] ) )
     break;  // two keys of the bucket in one slot
    flip( T0[ i ] );
    }
   if( i == Kb.size() )
    return( p );
   while( i-- > 0 )
    flip( T0[ i ] );
   }
  return( kPilots );
  };
 std::vector< std::uint32_t > owner( S , kNone );
 const std::size_t rlen = std::min( kRecent , B / 4 );
 std::array< std::uint32_t , kRecent > recent{};
 std::priority_queue< std::pair< std::uint32_t , std::uint32_t > > Q;
 for( const std::uint32_t nb : order ) {
  if( len( nb ) == 0 )
   break;  // the empty buckets are last, their pilots 0
  Q.push( { std::uint32_t( len( nb ) ) , nb } );
  recent.fill( kNone );
  std::size_t ri = 0 , chain = 0;
  if( rlen > 0 )
   recent[ 0 ] = nb;
  while( ! Q.empty() ) {
   const std::uint32_t b = Q.top().second;
   Q.pop();
   const auto Kb = keys( b );
   if( const std::size_t p = firstFree( Kb ); p < kPilots ) {
    pilot[ b ] = std::uint8_t( p );
    const std::uint64_t hp = pilotHash( p );
    for( const std::uint64_t h : Kb )
     owner[ inPart( h , hp ) ] = b;
    continue;
    }
   // the pilot of the least sum of the squares of the sizes met, its
   // slots distinct and out of the buckets placed last, from a random one
   const std::size_t p0 = std::size_t( g() & ( kPilots - 1 ) );
   std::size_t best = ~std::size_t( 0 ) , bp = kPilots;
   for( std::size_t d = 0 ; d < kPilots ; ++d ) {
    const std::size_t p = ( p0 + d ) & ( kPilots - 1 );
    const std::uint64_t hp = pilotHash( p );
    std::size_t score = 0;
    bool ok = true;
    for( std::size_t i = 0 ; ok && ( i < Kb.size() ) ; ++i ) {
     T0[ i ] = inPart( Kb[ i ] , hp );
     const std::uint32_t o = owner[ T0[ i ] ];
     if( o == kNone )
      continue;
     ok = std::find( recent.begin() , recent.begin() + rlen , o ) ==
	  recent.begin() + rlen;
     score += len( o ) * len( o );
     ok = ok && ( score < best );
     }
    if( ! ok )
     continue;
    std::sort( T0.begin() , T0.begin() + Kb.size() );
    if( std::adjacent_find( T0.begin() , T0.begin() + Kb.size() ) !=
	T0.begin() + Kb.size() )
     continue;
    best = score;
    bp = p;
    if( score <= 1 )
     break;  // a free pilot was not found: 1 is the least
    }
   if( bp == kPilots )
    return( -1 );  // equal slots for every pilot, or a cycle
   pilot[ b ] = std::uint8_t( bp );
   const std::uint64_t hp = pilotHash( bp );
   for( const std::uint64_t h : Kb ) {
    const std::size_t t = inPart( h , hp );
    if( const std::uint32_t o = owner[ t ]; o != kNone ) {
     Q.push( { std::uint32_t( len( o ) ) , o } );  // evicted
     ++chain;
     ++evicted;
     const std::uint64_t ho = pilotHash( pilot[ o ] );
     for( const std::uint64_t h2 : keys( o ) ) {
      const std::size_t t2 = inPart( h2 , ho );
      owner[ t2 ] = kNone;
      flip( t2 );
      }
     }
    owner[ t ] = b;
    flip( t );
    }
   if( rlen > 0 ) {
    ri = ( ri + 1 ) % rlen;
    recent[ ri ] = b;
    }
   if( chain > 10 * S )
    return( -1 );
   }
  }
 return( std::ptrdiff_t( evicted ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
template< Executor E >
bool PtrHash< T , F , R , H >::build( E & ex , const std::vector< T > & keys )
{
 const std::size_t n = f_n , P = f_P , S = f_S , m = P * S - n ,
		   w = S / 64 + 1;  // the words of the slots of a part
 std::vector< std::uint64_t > K( n );
 parallelFor( ex , 0 , n ,
	      [ & ]( std::size_t i ) { K[ i ] = hash( keys[ i ] ); } );
 radixSort( K );  // by part, then by bucket in the part
 if( std::adjacent_find( K.begin() , K.end() ) != K.end() )
  throw( std::runtime_error( "PtrHash: two keys have the same hash" ) );
 std::vector< std::size_t > start( P + 1 , 0 );
 for( const std::uint64_t h : K )
  ++start[ std::size_t( detail::ptrHashHigh( h , P ) ) + 1 ];
 for( std::size_t q = 0 ; q < P ; ++q ) {
  if( start[ q + 1 ] > S )
   return( false );  // a part fuller than its slots
  start[ q + 1 ] += start[ q ];
  }
 // the parts, each on its own pilots and on its own words of the slots
 f_pilot.assign( P * f_B , 0 );
 std::vector< std::uint64_t > taken( P * w , 0 );
 std::vector< std::ptrdiff_t > evicted( P , 0 );
 parallelFor(
   ex , 0 , P ,
   [ & ]( std::size_t q ) {
    evicted[ q ] = part( q ,
			 std::span< const std::uint64_t >( K ).subspan(
			   start[ q ] , start[ q + 1 ] - start[ q ] ) ,
			 taken.data() + q * w );
    } ,
   1 );
 f_evictions = 0;
 for( const std::ptrdiff_t e : evicted ) {
  if( e < 0 )
   return( false );
  f_evictions += std::size_t( e );
  }
 // the i-th slot >= n taken to the i-th slot < n left free; the slots
 // >= n not taken to the last free one before, non-decreasing
 auto isTaken = [ & ]( std::size_t t ) {
  const std::size_t q = t / S , u = t - q * S;
  return( ( taken[ q * w + ( u >> 6 ) ] >> ( u & 63 ) ) & 1 );
  };
 std::vector< std::uint64_t > V;
 V.reserve( m );
 for( std::size_t t = 0 ; ( t < n ) && ( V.size() < m ) ; ++t )
  if( ! isTaken( t ) ) {
   while( ! isTaken( n + V.size() ) )
    V.push_back( t );
   V.push_back( t );
   }
 V.resize( m , n - 1 );  // the slots past the last one taken
 if constexpr( R == PtrHashRemap::EliasFano )
  f_remap = m > 0 ? EliasFano( V ) : EliasFano();
 else
  f_remap.assign( V.begin() , V.end() );
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::operator()( const T & k ) const
{
 ADS_PRE( f_n >= 1 );
 const std::uint64_t h = hash( k );
 return( value( h , bucket( h ) ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
void PtrHash< T , F , R , H >::stream( std::span< const T > K ,
				    std::span< std::size_t > V ) const
{
 ADS_PRE( ( V.size() >= K.size() ) && ( K.empty() || ( f_n >= 1 ) ) );
 std::array< std::uint64_t , kAhead > hs;
 std::array< std::size_t , kAhead > bs;
 const std::size_t n = K.size() , a = std::min( kAhead , n );
 for( std::size_t i = 0 ; i < a ; ++i ) {
  hs[ i ] = hash( K[ i ] );
  bs[ i ] = bucket( hs[ i ] );
  detail::prefetchLine( f_pilot.data() + bs[ i ] );
  }
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t j = i & ( kAhead - 1 );
  V[ i ] = value( hs[ j ] , bs[ j ] );
  if( i + kAhead < n ) {
   hs[ j ] = hash( K[ i + kAhead ] );
   bs[ j ] = bucket( hs[ j ] );
   detail::prefetchLine( f_pilot.data() + bs[ j ] );
   }
  }
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
void PtrHash< T , F , R , H >::prefetch( const T & k ) const
{
 ADS_PRE( f_n >= 1 );
 detail::prefetchLine( f_pilot.data() + bucket( hash( k ) ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::size( void ) const
{
 return( f_n );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
bool PtrHash< T , F , R , H >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::bits( void ) const
{
 std::size_t r = 0;
 if constexpr( R == PtrHashRemap::EliasFano )
  r = f_remap.sizeInBits();
 else
  r = 32 * f_remap.size();
 return( 8 * f_pilot.size() + r + 8 * sizeof( *this ) );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::parts( void ) const
{
 return( f_P );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::buckets( void ) const
{
 return( f_P * f_B );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::slots( void ) const
{
 return( f_P * f_S );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::trials( void ) const
{
 return( f_trials );
 }

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
std::size_t PtrHash< T , F , R , H >::evictions( void ) const
{
 return( f_evictions );
 }

/*--------------------------------------------------------------------------*/

template< class T , PtrHashBucket F , PtrHashRemap R , class H >
template< class A >
inline void PtrHash< T , F , R , H >::persist( A & a )
{
 a( f_seed , f_n , f_P , f_S , f_B , f_mS , f_pilot , f_remap , f_trials ,
    f_evictions );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( ( f_n == 0 ) ||
	      ( ( f_P >= 1 ) && ( f_S >= 1 ) && ( f_P * f_S >= f_n ) &&
		( f_pilot.size() == f_P * f_B ) &&
		( f_remap.size() == f_P * f_S - f_n ) ) ,
	    "the pilots of a PtrHash" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File PtrHash.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
