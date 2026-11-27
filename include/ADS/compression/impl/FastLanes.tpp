/*--------------------------------------------------------------------------*/
/*--------------------------- File FastLanes.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of FastLanes.h, with their contracts.
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

constexpr std::size_t fastLanesTransposed( std::size_t p ) noexcept
{
 const std::size_t o = ( p / 16 ) % 8;
 const std::size_t rho = ( ( o & 1 ) << 2 ) | ( o & 2 ) | ( o >> 2 );
 return( 64 * ( p % 16 ) + 8 * rho + p / 128 );
 }

namespace detail::fl
{

inline constexpr std::size_t kVector = fastLanesVector;

/// the bits of T
template< class T >
inline constexpr unsigned kW = unsigned( 8 * sizeof( T ) );

/// the lanes of a vector of T
template< class T >
inline constexpr std::size_t kL = kVector / kW< T >;

/// the reversal of the 3 bits of o
constexpr std::size_t rho( std::size_t o )
{
 return( ( ( o & 1 ) << 2 ) | ( o & 2 ) | ( o >> 2 ) );
 }

/// the 2^b - 1 of T, all its bits for b = W
template< class T , unsigned B >
inline constexpr T kMask =
  B >= kW< T > ? T( ~T( 0 ) ) : T( ( std::uint64_t( 1 ) << B ) - 1 );

/// the table of the places of the transposed layout, tau( p )
inline constexpr auto kTau = []()
{
 std::array< std::uint16_t , kVector > t{};
 for( std::size_t p = 0 ; p < kVector ; ++p )
  t[ p ] = std::uint16_t( fastLanesTransposed( p ) );
 return( t );
 }();

/// the row R of the lane l of in, less base, into the word of the lane
/// that a holds, written out once full
template< class T , unsigned B , std::size_t R >
inline void packRow( const T * in , std::uint8_t * out , T base ,
		     std::size_t l ,
		     T & a )
{
 constexpr unsigned W = kW< T >;
 constexpr std::size_t L = kL< T > , bit = R * B , k = bit / W , t = bit % W;
 const T v = T( in[ R * L + l ] - base );
 if constexpr( t == 0 )
  a = v;
 else
  a = T( a | T( v << t ) );
 if constexpr( t + B >= W ) {
  storeLE< T >( out + ( k * L + l ) * sizeof( T ) , a );
  if constexpr( ( t + B > W ) && ( t > 0 ) )
   a = T( v >> ( W - t ) );
  }
 }

/// the row R of the lane l of the B-bit packing at in
template< class T , unsigned B , std::size_t R >
inline T unpackRow( const std::uint8_t * in , std::size_t l )
{
 constexpr unsigned W = kW< T >;
 constexpr std::size_t L = kL< T > , bit = R * B , k = bit / W , t = bit % W;
 if constexpr( B == 0 )
  return( T( 0 ) );
 else {
  T v = T( loadLE< T >( in + ( k * L + l ) * sizeof( T ) ) >> t );
  if constexpr( ( t + B > W ) && ( t > 0 ) )
   v = T( v | T( loadLE< T >( in + ( ( k + 1 ) * L + l ) * sizeof( T ) )
		 << ( W - t ) ) );
  return( T( v & kMask< T , B > ) );
  }
 }

template< class T , unsigned B , std::size_t... R >
inline void packLanes( const T * in , std::uint8_t * out , T base ,
		       std::index_sequence< R... > )
{
 for( std::size_t l = 0 ; l < kL< T > ; ++l ) {
  T a = 0;
  ( packRow< T , B , R >( in , out , base , l , a ) , ... );
  }
 }

template< class T , unsigned B , std::size_t... R >
inline void unpackLanes( const std::uint8_t * in , T * out , T base ,
			 std::index_sequence< R... > )
{
 for( std::size_t l = 0 ; l < kL< T > ; ++l )
  ( ( out[ R * kL< T > + l ] = T( unpackRow< T , B , R >( in , l ) + base ) ) ,
    ... );
 }

/// the row R = 8 o + s of the chain of the lane l, at the place 16 rho( o )
/// + 128 s + l, i.e., the row of that place in the packing, added to a
template< class T , unsigned B , std::size_t R >
inline void undeltaRow( const std::uint8_t * in , T * t , std::size_t l ,
			T & a )
{
 constexpr std::size_t p = 16 * rho( R / 8 ) + 128 * ( R % 8 );
 a = T( a + unpackRow< T , B , p / kL< T > >( in , l ) );
 t[ p + l ] = a;
 }

template< class T , unsigned B , std::size_t... R >
inline void undeltaLanes( const std::uint8_t * in , T * t , const T * base ,
			  std::index_sequence< R... > )
{
 for( std::size_t l = 0 ; l < kL< T > ; ++l ) {
  T a = base[ l ];
  ( undeltaRow< T , B , R >( in , t , l , a ) , ... );
  }
 }

// the kernels of the width B, on copies of the packed words in an array of
// their own, which the compiler knows to be apart from the values, so that
// it vectorizes the loop over the lanes with no test of overlap

template< class T , unsigned B >
void packWidth( const T * in , std::uint8_t * out , T base )
{
 alignas( 64 ) std::uint8_t w[ 128 * ( B > 0 ? B : 1 ) ];
 packLanes< T , B >( in , w , base , std::make_index_sequence< kW< T > >{} );
 std::memcpy( out , w , 128 * B );
 }

template< class T , unsigned B >
void unpackWidth( const std::uint8_t * in , T * out , T base )
{
 alignas( 64 ) std::uint8_t w[ 128 * ( B > 0 ? B : 1 ) ];
 std::memcpy( w , in , 128 * B );
 unpackLanes< T , B >( w , out , base ,
		       std::make_index_sequence< kW< T > >{} );
 }

template< class T , unsigned B >
void undeltaWidth( const std::uint8_t * in , T * t , const T * base )
{
 alignas( 64 ) std::uint8_t w[ 128 * ( B > 0 ? B : 1 ) ];
 alignas( 64 ) T a[ kL< T > ];
 std::memcpy( w , in , 128 * B );
 std::memcpy( a , base , sizeof( a ) );
 undeltaLanes< T , B >( w , t , a , std::make_index_sequence< kW< T > >{} );
 }

template< class T , std::size_t... B >
constexpr auto packTable( std::index_sequence< B... > )
{
 return(
   std::array< void ( * )( const T * , std::uint8_t * , T ) , sizeof...( B ) >{
     &packWidth< T , unsigned( B ) >... } );
 }

template< class T , std::size_t... B >
constexpr auto unpackTable( std::index_sequence< B... > )
{
 return(
   std::array< void ( * )( const std::uint8_t * , T * , T ) , sizeof...( B ) >{
     &unpackWidth< T , unsigned( B ) >... } );
 }

template< class T , std::size_t... B >
constexpr auto undeltaTable( std::index_sequence< B... > )
{
 return(
   std::array< void ( * )( const std::uint8_t * , T * , const T * ) ,
	       sizeof...( B ) >{ &undeltaWidth< T , unsigned( B ) >... } );
 }

/// the kernels of the widths 0 to W
template< class T >
inline constexpr auto kPack =
  packTable< T >( std::make_index_sequence< kW< T > + 1 >{} );
template< class T >
inline constexpr auto kUnpack =
  unpackTable< T >( std::make_index_sequence< kW< T > + 1 >{} );
template< class T >
inline constexpr auto kUndelta =
  undeltaTable< T >( std::make_index_sequence< kW< T > + 1 >{} );

/// the code DELTA of the 1024 values of v appended to out
template< class T >
void encodeDelta( const T * v , std::vector< std::uint8_t > & out )
{
 constexpr std::size_t L = kL< T >;
 alignas( 64 ) T e[ kVector ] , d[ kVector ];
 // the differences in the order of v, 0 at the head of a chain, and then
 // d_p = e_tau( p ) by the places of the decoder, from indices the compiler
 // knows
 T r = 0;
 for( std::size_t i = 0 ; i < kVector ; ++i ) {
  e[ i ] = i % kW< T > == 0 ? T( 0 ) : T( v[ i ] - v[ i - 1 ] );
  r |= e[ i ];
  }
 for( std::size_t c = 0 ; c < 16 ; ++c )
  for( std::size_t q = 0 ; q < 8 ; ++q )
   for( std::size_t a = 0 ; a < 8 ; ++a )
    d[ 128 * a + 16 * rho( q ) + c ] = e[ 64 * c + 8 * q + a ];
 const unsigned b = unsigned( std::bit_width( r ) );
 const std::size_t at = out.size();
 out.resize( at + 1 + L * sizeof( T ) + 128 * std::size_t( b ) );
 std::uint8_t * o = out.data() + at;
 *o++ = std::uint8_t( b );
 for( std::size_t l = 0 ; l < L ; ++l , o += sizeof( T ) )
  storeLE< T >( o , v[ kTau[ l ] ] );
 kPack< T >[ b ]( d , o , T( 0 ) );
 }

/// the 1024 values of the code DELTA at in[ at .. ) into v, at past it
template< class T >
void decodeDelta( std::span< const std::uint8_t > in , std::size_t & at ,
		  T * v )
{
 constexpr std::size_t L = kL< T >;
 ADS_PRE( at < in.size() );
 const unsigned b = in[ at ];
 ADS_PRE( ( b <= kW< T > ) &&
	  ( at + 1 + L * sizeof( T ) + 128 * std::size_t( b ) <= in.size() ) );
 alignas( 64 ) T base[ L ] , t[ kVector ];
 const std::uint8_t * p = in.data() + at + 1;
 for( std::size_t l = 0 ; l < L ; ++l , p += sizeof( T ) )
  base[ l ] = loadLE< T >( p );
 kUndelta< T >[ b ]( p , t , base );
 // v_tau( p ) = t_p by the places 64 c + 8 r + a = tau( 128 a + 16 rho( r )
 // + c ), the 8 values of a row stored one after the other, from indices
 // the compiler knows, which it turns into vector operations
 for( std::size_t c = 0 ; c < 16 ; ++c )
  for( std::size_t r = 0 ; r < 8 ; ++r )
   for( std::size_t a = 0 ; a < 8 ; ++a )
    v[ 64 * c + 8 * r + a ] = t[ 128 * a + 16 * rho( r ) + c ];
 at += 1 + L * sizeof( T ) + 128 * std::size_t( b );
 }

}  // end( namespace detail::fl )

template< FastLanesWord T >
void fastLanesPack( const T * in , std::uint8_t * out , unsigned b , T base )
{
 ADS_PRE( b <= detail::fl::kW< T > );
 ADS_AUDIT( std::all_of( in , in + fastLanesVector , [ & ]( T v ) {
  return( b >= detail::fl::kW< T > || T( v - base ) >> b == 0 );
  } ) );
 detail::fl::kPack< T >[ b ]( in , out , base );
 }

template< FastLanesWord T >
void fastLanesUnpack( const std::uint8_t * in , T * out , unsigned b , T base )
{
 ADS_PRE( b <= detail::fl::kW< T > );
 detail::fl::kUnpack< T >[ b ]( in , out , base );
 }

template< std::ranges::contiguous_range R >
 requires FastLanesWord< std::ranges::range_value_t< R > >
void fastLanesEncode( const R & x , std::vector< std::uint8_t > & out ,
		      FastLanesScheme s )
{
 using T = std::ranges::range_value_t< R >;
 using namespace detail::fl;
 const std::span< const T > v( std::ranges::data( x ) ,
			       std::size_t( std::ranges::size( x ) ) );
 const std::size_t n = v.size();
 out.clear();
 alignas( 64 ) T buf[ kVector ];
 for( std::size_t a = 0 ; a < n ; a += kVector ) {
  // the vector, the last one filled with its last value
  const std::size_t m = std::min( kVector , n - a );
  const T * w = v.data() + a;
  if( m < kVector ) {
   std::copy( w , w + m , buf );
   std::fill( buf + m , buf + kVector , w[ m - 1 ] );
   w = buf;
   }
  if( s == FastLanesScheme::FOR ) {
   const auto [ lo , hi ] = std::minmax_element( w , w + kVector );
   const T base = *lo;
   const unsigned b = unsigned( std::bit_width( T( *hi - base ) ) );
   const std::size_t at = out.size();
   out.resize( at + 1 + sizeof( T ) + 128 * std::size_t( b ) );
   out[ at ] = std::uint8_t( b );
   detail::storeLE< T >( out.data() + at + 1 , base );
   kPack< T >[ b ]( w , out.data() + at + 1 + sizeof( T ) , base );
   }
  else if( s == FastLanesScheme::DELTA )
   encodeDelta< T >( w , out );
  else {
   // the values of the runs, and the number of the run of each value
   alignas( 64 ) std::uint16_t run[ kVector ];
   std::size_t k = 0;
   const std::size_t at = out.size();
   out.resize( at + 2 + kVector * sizeof( T ) );
   std::uint8_t * o = out.data() + at + 2;
   for( std::size_t i = 0 ; i < kVector ; ++i ) {
    if( ( i == 0 ) || ( w[ i ] != w[ i - 1 ] ) )
     detail::storeLE< T >( o + sizeof( T ) * k++ , w[ i ] );
    run[ i ] = std::uint16_t( k - 1 );
    }
   detail::storeLE< std::uint16_t >( out.data() + at , std::uint16_t( k ) );
   out.resize( at + 2 + k * sizeof( T ) );
   encodeDelta< std::uint16_t >( run , out );
   }
  }
 ADS_AUDIT( fastLanesDecode< T >( out , n , s ) ==
	    std::vector< T >( v.begin() , v.end() ) );
 }

template< std::ranges::contiguous_range R >
 requires FastLanesWord< std::ranges::range_value_t< R > >
std::vector< std::uint8_t > fastLanesEncode( const R & x , FastLanesScheme s )
{
 std::vector< std::uint8_t > out;
 fastLanesEncode( x , out , s );
 return( out );
 }

template< FastLanesWord T >
void fastLanesDecode( std::span< const std::uint8_t > code ,
		      std::span< T > out ,
		      FastLanesScheme s )
{
 using namespace detail::fl;
 const std::size_t n = out.size();
 alignas( 64 ) T buf[ kVector ];
 std::size_t at = 0;
 for( std::size_t a = 0 ; a < n ; a += kVector ) {
  // the last vector, if not full, into a buffer of its own
  T * const v = a + kVector <= n ? out.data() + a : buf;
  if( s == FastLanesScheme::FOR ) {
   ADS_PRE( at < code.size() );
   const unsigned b = code[ at ];
   ADS_PRE( ( b <= kW< T > ) &&
	    ( at + 1 + sizeof( T ) + 128 * std::size_t( b ) <= code.size() ) );
   const T base = detail::loadLE< T >( code.data() + at + 1 );
   kUnpack< T >[ b ]( code.data() + at + 1 + sizeof( T ) , v , base );
   at += 1 + sizeof( T ) + 128 * std::size_t( b );
   }
  else if( s == FastLanesScheme::DELTA )
   decodeDelta< T >( code , at , v );
  else {
   ADS_PRE( at + 2 <= code.size() );
   const std::size_t runs =
     detail::loadLE< std::uint16_t >( code.data() + at );
   ADS_PRE( ( runs >= 1 ) && ( runs <= kVector ) &&
	    ( at + 2 + runs * sizeof( T ) <= code.size() ) );
   const std::uint8_t * const vals = code.data() + at + 2;
   at += 2 + runs * sizeof( T );
   alignas( 64 ) std::uint16_t run[ kVector ];
   decodeDelta< std::uint16_t >( code , at , run );
   ADS_PRE( std::all_of( run , run + kVector ,
			 [ & ]( std::uint16_t j ) { return( j < runs ); } ) );
   for( std::size_t i = 0 ; i < kVector ; ++i )
    v[ i ] = detail::loadLE< T >( vals + sizeof( T ) * run[ i ] );
   }
  if( v == buf )
   std::copy( buf , buf + ( n - a ) , out.data() + a );
  }
 }

template< FastLanesWord T >
std::vector< T > fastLanesDecode( std::span< const std::uint8_t > code ,
				  std::size_t n , FastLanesScheme s )
{
 std::vector< T > out( n );
 fastLanesDecode< T >( code , std::span< T >( out ) , s );
 return( out );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File FastLanes.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
