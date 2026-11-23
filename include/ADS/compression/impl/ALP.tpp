/*--------------------------------------------------------------------------*/
/*------------------------------ File ALP.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ALP.h, with their contracts.
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

#include "ADS/compression/impl/Unaligned.h"

namespace ADS
{

namespace detail::alp
{

inline constexpr std::size_t kVector = fastLanesVector;
inline constexpr std::size_t kGroup = 100;   ///< the vectors of a row group
inline constexpr std::size_t kSampled = 8;   ///< the vectors sampled
inline constexpr std::size_t kSamples = 32;  ///< the values of a sample
inline constexpr std::size_t kPairs = 5;     ///< the pairs kept
inline constexpr std::size_t kDict = 8;      ///< the entries of ALP-RD
inline constexpr unsigned kMaxCut = 16;      ///< the largest cut of ALP-RD

inline constexpr double kPow10d[] = { 1e0 ,  1e1 ,  1e2 ,  1e3 ,  1e4 ,  1e5 ,
				      1e6 ,
				      1e7 ,  1e8 ,  1e9 ,  1e10 , 1e11 , 1e12 ,
				      1e13 ,
				      1e14 , 1e15 , 1e16 , 1e17 , 1e18 };
inline constexpr double kInvPow10d[] = { 1e0 ,   1e-1 ,  1e-2 ,  1e-3 ,  1e-4 ,
					 1e-5 ,  1e-6 ,  1e-7 ,  1e-8 ,  1e-9 ,
					 1e-10 , 1e-11 , 1e-12 , 1e-13 ,
					 1e-14 ,
					 1e-15 , 1e-16 , 1e-17 , 1e-18 };
inline constexpr float kPow10f[] = { 1e0f , 1e1f , 1e2f , 1e3f , 1e4f , 1e5f ,
				     1e6f , 1e7f , 1e8f , 1e9f , 1e10f };
inline constexpr float kInvPow10f[] = { 1e0f ,  1e-1f , 1e-2f , 1e-3f ,
					1e-4f , 1e-5f , 1e-6f , 1e-7f ,
					1e-8f , 1e-9f , 1e-10f };

/// the integers, the largest exponent, the constant of the rounding and
/// the powers of 10 of a type
template< class F >
struct Traits;

template<>
struct Traits< double >
{
 using U = std::uint64_t;
 using I = std::int64_t;
 static constexpr int kMaxE = 18;
 static constexpr double kSweet = 6755399441055744.0;  // 2^52 + 2^51
 static constexpr double kLimit = 2251799813685248.0;  // 2^51
 static constexpr const double * kPow = kPow10d;
 static constexpr const double * kInv = kInvPow10d;
 };

template<>
struct Traits< float >
{
 using U = std::uint32_t;
 using I = std::int32_t;
 static constexpr int kMaxE = 10;
 static constexpr float kSweet = 12582912.0f;  // 2^23 + 2^22
 static constexpr float kLimit = 4194304.0f;   // 2^22
 static constexpr const float * kPow = kPow10f;
 static constexpr const float * kInv = kInvPow10f;
 };

/// the bits of x
template< class F >
inline typename Traits< F >::U bitsOf( F x )
{
 typename Traits< F >::U u;
 std::memcpy( &u , &x , sizeof( F ) );
 return( u );
 }

/// the value of the bits u
template< class F >
inline F valueOf( typename Traits< F >::U u )
{
 F x;
 std::memcpy( &x , &u , sizeof( F ) );
 return( x );
 }

/// d of x by ( e , f ), true if its decoding gives back the bits of x
template< class F >
inline bool encode( F x , int e , int f , typename Traits< F >::I & d )
{
 using T = Traits< F >;
 const F t = x * T::kPow[ e ] * T::kInv[ f ];
 if( ! ( ( t < T::kLimit ) && ( t > -T::kLimit ) ) )  // a NaN too
  return( false );
 d = typename T::I( ( t + T::kSweet ) - T::kSweet );
 return( bitsOf( F( d ) * T::kPow[ f ] * T::kInv[ e ] ) == bitsOf( x ) );
 }

/// the cost of ( e , f ) on the values s, in bits
template< class F >
std::size_t cost( std::span< const F > s , int e , int f )
{
 using I = typename Traits< F >::I;
 using U = typename Traits< F >::U;
 I lo = 0 , hi = 0 , d = 0;
 std::size_t c = 0;
 bool any = false;
 for( const F x : s )
  if( encode( x , e , f , d ) ) {
   lo = any ? std::min( lo , d ) : d;
   hi = any ? std::max( hi , d ) : d;
   any = true;
   }
  else
   ++c;
 const std::size_t b =
   any ? std::size_t( std::bit_width( U( U( hi ) - U( lo ) ) ) ) : 0;
 return( s.size() * b + c * ( 8 * sizeof( F ) + 16 ) );
 }

/// the cut, the dictionary and the cost of ALP-RD on the values s
template< class F >
struct Cut
{
 unsigned lambda = 1;
 std::vector< std::uint16_t > dict;
 std::size_t cost = ~std::size_t( 0 );
 };

template< class F >
Cut< F > bestCut( std::span< const F > s )
{
 constexpr unsigned W = unsigned( 8 * sizeof( F ) );
 Cut< F > best;
 std::vector< std::uint16_t > hi( s.size() );
 for( unsigned lambda = 1 ; lambda <= kMaxCut ; ++lambda ) {
  // the high parts sorted, and their distinct values by count
  for( std::size_t i = 0 ; i < s.size() ; ++i )
   hi[ i ] = std::uint16_t( bitsOf( s[ i ] ) >> ( W - lambda ) );
  std::sort( hi.begin() , hi.end() );
  std::vector< std::pair< std::size_t , std::uint16_t > > run;
  for( std::size_t i = 0 ; i < hi.size() ; ) {
   std::size_t j = i;
   while( ( j < hi.size() ) && ( hi[ j ] == hi[ i ] ) )
    ++j;
   run.emplace_back( j - i , hi[ i ] );
   i = j;
   }
  std::sort( run.begin() , run.end() , []( const auto & a , const auto & b ) {
   return( a.first != b.first ? a.first > b.first : a.second < b.second );
   } );
  const std::size_t q = std::min( kDict , run.size() );
  std::size_t covered = 0;
  for( std::size_t k = 0 ; k < q ; ++k )
   covered += run[ k ].first;
  const std::size_t ib = q > 1 ? std::size_t( std::bit_width( q - 1 ) ) : 0;
  const std::size_t c =
    s.size() * ( W - lambda + ib ) + ( s.size() - covered ) * 32 + 16 * q;
  if( c < best.cost ) {
   best.lambda = lambda;
   best.cost = c;
   best.dict.clear();
   for( std::size_t k = 0 ; k < q ; ++k )
    best.dict.push_back( run[ k ].second );
   }
  }
 return( best );
 }

/// the vector x of m values by ALP with ( e , f ), appended to out
template< class F >
void encodeVector( const F * x , std::size_t m , int e , int f ,
		   std::vector< std::uint8_t > & out )
{
 using I = typename Traits< F >::I;
 using U = typename Traits< F >::U;
 alignas( 64 ) U d[ kVector ];
 std::uint16_t pos[ kVector ];
 std::size_t c = 0 , first = m;
 for( std::size_t i = 0 ; i < m ; ++i ) {
  I v = 0;
  if( encode( x[ i ] , e , f , v ) ) {
   d[ i ] = U( v );
   first = std::min( first , i );
   }
  else
   pos[ c++ ] = std::uint16_t( i );
  }
 const U fill = first < m ? d[ first ] : U( 0 );
 for( std::size_t k = 0 ; k < c ; ++k )
  d[ pos[ k ] ] = fill;
 std::fill( d + m , d + kVector , fill );
 I lo = I( d[ 0 ] ) , hi = I( d[ 0 ] );
 for( std::size_t i = 1 ; i < kVector ; ++i ) {
  lo = std::min( lo , I( d[ i ] ) );
  hi = std::max( hi , I( d[ i ] ) );
  }
 const unsigned b = unsigned( std::bit_width( U( U( hi ) - U( lo ) ) ) );
 const std::size_t at = out.size();
 out.resize( at + 5 + sizeof( U ) + 128 * std::size_t( b ) +
	     c * ( 2 + sizeof( U ) ) );
 std::uint8_t * o = out.data() + at;
 o[ 0 ] = std::uint8_t( e );
 o[ 1 ] = std::uint8_t( f );
 o[ 2 ] = std::uint8_t( b );
 storeLE< std::uint16_t >( o + 3 , std::uint16_t( c ) );
 storeLE< U >( o + 5 , U( lo ) );
 o += 5 + sizeof( U );
 fastLanesPack< U >( d , o , b , U( lo ) );
 o += 128 * std::size_t( b );
 for( std::size_t k = 0 ; k < c ; ++k , o += 2 )
  storeLE< std::uint16_t >( o , pos[ k ] );
 for( std::size_t k = 0 ; k < c ; ++k , o += sizeof( U ) )
  storeLE< U >( o , bitsOf( x[ pos[ k ] ] ) );
 }

/// the vector x of m values by ALP-RD with the cut, appended to out
template< class F >
void encodeVectorRD( const F * x , std::size_t m , const Cut< F > & cut ,
		     std::vector< std::uint8_t > & out )
{
 using U = typename Traits< F >::U;
 constexpr unsigned W = unsigned( 8 * sizeof( F ) );
 const unsigned r = W - cut.lambda;
 const std::size_t q = cut.dict.size();
 const unsigned ib = q > 1 ? unsigned( std::bit_width( q - 1 ) ) : 0;
 alignas( 64 ) U low[ kVector ];
 alignas( 64 ) std::uint16_t idx[ kVector ];
 std::uint16_t pos[ kVector ] , exc[ kVector ];
 std::size_t c = 0;
 const U mask = ( U( 1 ) << r ) - 1;
 for( std::size_t i = 0 ; i < m ; ++i ) {
  const U u = bitsOf( x[ i ] );
  const std::uint16_t h = std::uint16_t( u >> r );
  low[ i ] = u & mask;
  std::size_t k = 0;
  while( ( k < q ) && ( cut.dict[ k ] != h ) )
   ++k;
  if( k == q ) {
   pos[ c ] = std::uint16_t( i );
   exc[ c++ ] = h;
   k = 0;
   }
  idx[ i ] = std::uint16_t( k );
  }
 std::fill( low + m , low + kVector , U( 0 ) );
 std::fill( idx + m , idx + kVector , std::uint16_t( 0 ) );
 const std::size_t at = out.size();
 out.resize( at + 2 + 128 * std::size_t( ib + r ) + 4 * c );
 std::uint8_t * o = out.data() + at;
 storeLE< std::uint16_t >( o , std::uint16_t( c ) );
 o += 2;
 fastLanesPack< std::uint16_t >( idx , o , ib );
 o += 128 * std::size_t( ib );
 fastLanesPack< U >( low , o , r );
 o += 128 * std::size_t( r );
 for( std::size_t k = 0 ; k < c ; ++k , o += 2 )
  storeLE< std::uint16_t >( o , pos[ k ] );
 for( std::size_t k = 0 ; k < c ; ++k , o += 2 )
  storeLE< std::uint16_t >( o , exc[ k ] );
 }

/// the values of the row group of nv vectors from x, n values, by ALP or
/// ALP-RD, appended to out
template< class F >
void encodeGroup( const F * x , std::size_t n ,
		  std::vector< std::uint8_t > & out )
{
 using T = Traits< F >;
 const std::size_t nv = ( n + kVector - 1 ) / kVector;
 // the first level: 32 values of each of up to 8 vectors, the best pair
 // of each, and the 5 pairs best for most of them
 std::vector< F > all;
 std::vector< std::pair< int , int > > best;
 std::size_t alpCost = 0;
 const std::size_t sv = std::min( kSampled , nv );
 for( std::size_t j = 0 ; j < sv ; ++j ) {
  const std::size_t v = j * nv / sv , a = v * kVector;
  const std::size_t m = std::min( kVector , n - a );
  const std::size_t step = std::max< std::size_t >( 1 , m / kSamples );
  std::vector< F > s;
  for( std::size_t i = 0 ; ( i < m ) && ( s.size() < kSamples ) ; i += step )
   s.push_back( x[ a + i ] );
  all.insert( all.end() , s.begin() , s.end() );
  std::size_t c0 = ~std::size_t( 0 );
  std::pair< int , int > p0( 0 , 0 );
  for( int e = T::kMaxE ; e >= 0 ; --e )
   for( int f = e ; f >= 0 ; --f ) {
    const std::size_t c = cost< F >( s , e , f );
    if( c < c0 ) {
     c0 = c;
     p0 = { e , f };
     }
    }
  best.push_back( p0 );
  alpCost += c0;
  }
 std::sort( best.begin() , best.end() );
 std::vector< std::pair< std::size_t , std::pair< int , int > > > freq;
 for( std::size_t i = 0 ; i < best.size() ; ) {
  std::size_t j = i;
  while( ( j < best.size() ) && ( best[ j ] == best[ i ] ) )
   ++j;
  freq.emplace_back( j - i , best[ i ] );
  i = j;
  }
 std::sort( freq.begin() , freq.end() , []( const auto & a , const auto & b ) {
  return( a.first != b.first ? a.first > b.first : a.second > b.second );
  } );
 if( freq.size() > kPairs )
  freq.resize( kPairs );
 // ALP-RD if it costs less on the same values
 const Cut< F > cut = bestCut< F >( all );
 if( cut.cost < alpCost ) {
  out.push_back( 1 );
  out.push_back( std::uint8_t( cut.lambda ) );
  out.push_back( std::uint8_t( cut.dict.size() ) );
  for( const auto h : cut.dict ) {
   out.resize( out.size() + 2 );
   storeLE< std::uint16_t >( out.data() + out.size() - 2 , h );
   }
  for( std::size_t a = 0 ; a < n ; a += kVector )
   encodeVectorRD< F >( x + a , std::min( kVector , n - a ) , cut , out );
  return;
  }
 out.push_back( 0 );
 for( std::size_t a = 0 ; a < n ; a += kVector ) {
  // the second level: the pairs kept on 32 values of the vector, stopped
  // by two in a row that do not improve
  const std::size_t m = std::min( kVector , n - a );
  std::pair< int , int > p = freq[ 0 ].second;
  if( freq.size() > 1 ) {
   const std::size_t step = std::max< std::size_t >( 1 , m / kSamples );
   std::vector< F > s;
   for( std::size_t i = 0 ; ( i < m ) && ( s.size() < kSamples ) ; i += step )
    s.push_back( x[ a + i ] );
   std::size_t c0 = ~std::size_t( 0 ) , worse = 0;
   for( const auto & fp : freq ) {
    const std::pair< int , int > pf = fp.second;
    const std::size_t c = cost< F >( s , pf.first , pf.second );
    if( c < c0 ) {
     c0 = c;
     p = pf;
     worse = 0;
     }
    else if( ++worse == 2 )
     break;
    }
   }
  encodeVector< F >( x + a , m , p.first , p.second , out );
  }
 }

}  // end( namespace detail::alp )

template< std::ranges::contiguous_range R >
 requires ALPFloat< std::ranges::range_value_t< R > >
std::vector< std::uint8_t > alpCompress( const R & x )
{
 using F = std::ranges::range_value_t< R >;
 using namespace detail::alp;
 const F * const p = std::ranges::data( x );
 const std::size_t n = std::size_t( std::ranges::size( x ) );
 std::vector< std::uint8_t > out;
 for( std::size_t a = 0 ; a < n ; a += kGroup * kVector )
  encodeGroup< F >( p + a , std::min( kGroup * kVector , n - a ) , out );
 ADS_AUDIT( [ & ]() {
  const auto y = alpDecompress< F >( out , n );
  for( std::size_t i = 0 ; i < n ; ++i )
   if( bitsOf( y[ i ] ) != bitsOf( p[ i ] ) )
    return( false );
  return( true );
  }() );
 return( out );
 }

template< ALPFloat F >
std::vector< F > alpDecompress( std::span< const std::uint8_t > code ,
				std::size_t n )
{
 using namespace detail::alp;
 using T = Traits< F >;
 using U = typename T::U;
 using I = typename T::I;
 constexpr unsigned W = unsigned( 8 * sizeof( F ) );
 const std::size_t vectors = ( n + kVector - 1 ) / kVector;
 std::vector< F > out( vectors * kVector );
 std::size_t at = 0;
 alignas( 64 ) U d[ kVector ];
 alignas( 64 ) std::uint16_t idx[ kVector ];
 for( std::size_t g = 0 ; g < vectors ; g += kGroup ) {
  ADS_PRE( at < code.size() );
  const std::uint8_t scheme = code[ at++ ];
  ADS_PRE( scheme <= 1 );
  unsigned r = 0 , ib = 0;
  std::size_t q = 0;
  std::array< U , kDict > dict{};
  if( scheme == 1 ) {
   ADS_PRE( at + 2 <= code.size() );
   const unsigned lambda = code[ at ];
   q = code[ at + 1 ];
   ADS_PRE( ( lambda >= 1 ) && ( lambda <= kMaxCut ) && ( q >= 1 ) &&
	    ( q <= kDict ) && ( at + 2 + 2 * q <= code.size() ) );
   r = W - lambda;
   ib = q > 1 ? unsigned( std::bit_width( q - 1 ) ) : 0;
   for( std::size_t k = 0 ; k < q ; ++k )
    dict[ k ] =
      U( detail::loadLE< std::uint16_t >( code.data() + at + 2 + 2 * k ) )
      << r;
   at += 2 + 2 * q;
   }
  for( std::size_t k = g ; k < std::min( vectors , g + kGroup ) ; ++k ) {
   F * const v = out.data() + k * kVector;
   if( scheme == 0 ) {
    ADS_PRE( at + 5 + sizeof( U ) <= code.size() );
    const int e = code[ at ] , f = code[ at + 1 ];
    const unsigned b = code[ at + 2 ];
    const std::size_t c =
      detail::loadLE< std::uint16_t >( code.data() + at + 3 );
    ADS_PRE( ( e <= T::kMaxE ) && ( f <= e ) && ( b <= W ) &&
	     ( c <= kVector ) &&
	     ( at + 5 + sizeof( U ) + 128 * std::size_t( b ) +
		 c * ( 2 + sizeof( U ) ) <=
	       code.size() ) );
    const U base = detail::loadLE< U >( code.data() + at + 5 );
    const std::uint8_t * p = code.data() + at + 5 + sizeof( U );
    fastLanesUnpack< U >( p , d , b , base );
    const F up = T::kPow[ f ] , down = T::kInv[ e ];
    for( std::size_t i = 0 ; i < kVector ; ++i )
     v[ i ] = F( I( d[ i ] ) ) * up * down;
    p += 128 * std::size_t( b );
    for( std::size_t j = 0 ; j < c ; ++j ) {
     const std::size_t i = detail::loadLE< std::uint16_t >( p + 2 * j );
     ADS_PRE( i < kVector );
     v[ i ] =
       valueOf< F >( detail::loadLE< U >( p + 2 * c + sizeof( U ) * j ) );
     }
    at += 5 + sizeof( U ) + 128 * std::size_t( b ) + c * ( 2 + sizeof( U ) );
    }
   else {
    ADS_PRE( at + 2 <= code.size() );
    const std::size_t c = detail::loadLE< std::uint16_t >( code.data() + at );
    ADS_PRE( ( c <= kVector ) &&
	     ( at + 2 + 128 * std::size_t( ib + r ) + 4 * c <= code.size() ) );
    const std::uint8_t * p = code.data() + at + 2;
    fastLanesUnpack< std::uint16_t >( p , idx , ib );
    p += 128 * std::size_t( ib );
    fastLanesUnpack< U >( p , d , r );
    p += 128 * std::size_t( r );
    for( std::size_t i = 0 ; i < kVector ; ++i ) {
     ADS_PRE( idx[ i ] < q );
     v[ i ] = valueOf< F >( dict[ idx[ i ] ] | d[ i ] );
     }
    for( std::size_t j = 0 ; j < c ; ++j ) {
     const std::size_t i = detail::loadLE< std::uint16_t >( p + 2 * j );
     ADS_PRE( i < kVector );
     v[ i ] = valueOf< F >(
       ( U( detail::loadLE< std::uint16_t >( p + 2 * c + 2 * j ) ) << r ) |
       d[ i ] );
     }
    at += 2 + 128 * std::size_t( ib + r ) + 4 * c;
    }
   }
  }
 out.resize( n );
 return( out );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File ALP.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
