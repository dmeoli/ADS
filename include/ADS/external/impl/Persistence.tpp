/*--------------------------------------------------------------------------*/
/*-------------------------- File Persistence.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The archives PersistWriter and PersistReader, and the functions store(),
 * restore() and restoreMapped() [see Persistence.h].
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <cstring>
#include <fstream>
#include <istream>
#include <limits>
#include <ostream>

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/*--------------------------------------------------------------------------*/

template< class T >
inline constexpr bool persistFalse = false;

/// an item of an array, to be read
template< class E >
[[nodiscard]] E persistBlankItem( void )
{
 if constexpr( requires { E::persistEmpty(); } )
  return( E::persistEmpty() );
 else
  return( E{} );
 }

template< class T >
struct IsVector : std::false_type
{ };
template< class E , class A >
struct IsVector< std::vector< E , A > > : std::true_type
{ };

template< class T >
struct IsStdArray : std::false_type
{ };
template< class E , std::size_t N >
struct IsStdArray< std::array< E , N > > : std::true_type
{ };

template< class T >
struct IsPair : std::false_type
{ };
template< class U , class V >
struct IsPair< std::pair< U , V > > : std::true_type
{ };

template< class T >
struct IsString : std::false_type
{ };
template< class A >
struct IsString< std::basic_string< char , std::char_traits< char > , A > >
    : std::true_type
{ };

/// the numbers of the format: those written in their own width in an
/// array (an enumeration is a number of its width)
template< class E >
concept PersistNumber =
  ( std::is_arithmetic_v< E > || std::is_enum_v< E > ) &&
  ( sizeof( E ) <= 8 ) && ( ! std::is_same_v< E , long double > ) &&
  ( ! std::is_same_v< E , bool > );

/// the items of an array written as their bytes: the numbers, and the
/// blocks of numbers of one type, which name it `persist_unit`
template< class E >
concept PersistBlock =
  PersistNumber< E > ||
  ( requires { typename E::persist_unit; } &&
    std::is_trivially_copyable_v< E > &&
    PersistNumber< typename E::persist_unit > &&
    ( sizeof( E ) % sizeof( typename E::persist_unit ) == 0 ) );

/// the number whose bytes are swapped in an item of an array
template< class E >
struct PersistUnitOf
{
 using type = E;
 };
template< class E >
 requires requires { typename E::persist_unit; }
struct PersistUnitOf< E >
{
 using type = typename E::persist_unit;
 };

/*--------------------------------------------------------------------------*/

/// the word of 8 bytes at \f$ p \f$, the first one the least significant
[[nodiscard]] inline std::uint64_t loadLittle( const std::byte * p )
{
 std::uint64_t w;
 std::memcpy( &w , p , 8 );
 if constexpr( std::endian::native == std::endian::big )
  w = byteSwap( w );
 return( w );
 }

/// the word \f$ w \f$ at \f$ p \f$, its least significant byte first
inline void storeLittle( std::byte * p , std::uint64_t w )
{
 if constexpr( std::endian::native == std::endian::big )
  w = byteSwap( w );
 std::memcpy( p , &w , 8 );
 }

/// the number \f$ x \f$ with its bytes reversed
template< class E >
[[nodiscard]] E reversed( E x )
{
 if constexpr( sizeof( E ) == 1 )
  return( x );
 else {
  using U = std::conditional_t<
    sizeof( E ) == 2 , std::uint16_t ,
    std::conditional_t< sizeof( E ) == 4 , std::uint32_t , std::uint64_t > >;
  U u;
  std::memcpy( &u , &x , sizeof( E ) );
  u = U( byteSwap( std::uint64_t( u ) << ( 64 - 8 * sizeof( E ) ) ) );
  std::memcpy( &x , &u , sizeof( E ) );
  return( x );
  }
 }

template< class E >
inline void persistSwap( E * p , std::size_t n )
{
 if constexpr( sizeof( E ) > 1 )
  for( std::size_t i = 0 ; i < n ; ++i )
   p[ i ] = reversed( p[ i ] );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS PersistChecksum ---------------------------*/
/*--------------------------------------------------------------------------*/

inline void PersistChecksum::round( std::uint64_t w )
{
 std::uint64_t & a = f_acc[ f_lane ];
 a = std::rotl( a + w * P2 , 31 ) * P1;
 f_lane = ( f_lane + 1 ) & 3;
 }

/*--------------------------------------------------------------------------*/

inline void PersistChecksum::update( const std::byte * p , std::size_t m )
{
 f_bytes += m;
 while( ( f_ntail > 0 ) && ( m > 0 ) ) {  // the word begun
  f_tail[ f_ntail++ ] = *p++;
  --m;
  if( f_ntail == 8 ) {
   round( loadLittle( f_tail.data() ) );
   f_ntail = 0;
   }
  }
 while( ( f_lane != 0 ) && ( m >= 8 ) ) {  // up to the lane 0
  round( loadLittle( p ) );
  p += 8;
  m -= 8;
  }
 for( ; m >= 32 ; p += 32 , m -= 32 )  // four lanes, independent
  for( unsigned l = 0 ; l < 4 ; ++l )
   f_acc[ l ] =
     std::rotl( f_acc[ l ] + loadLittle( p + 8 * l ) * P2 , 31 ) * P1;
 for( ; m >= 8 ; p += 8 , m -= 8 )
  round( loadLittle( p ) );
 for( ; m > 0 ; --m )
  f_tail[ f_ntail++ ] = *p++;
 }

/*--------------------------------------------------------------------------*/

inline std::uint64_t PersistChecksum::value( void ) const
{
 PersistChecksum c = *this;
 if( c.f_ntail > 0 ) {
  std::fill( c.f_tail.begin() + c.f_ntail , c.f_tail.end() , std::byte( 0 ) );
  c.round( loadLittle( c.f_tail.data() ) );
  }
 std::uint64_t h =
   std::rotl( c.f_acc[ 0 ] , 1 ) + std::rotl( c.f_acc[ 1 ] , 7 ) +
   std::rotl( c.f_acc[ 2 ] , 12 ) + std::rotl( c.f_acc[ 3 ] , 18 );
 h ^= f_bytes * P4;
 h ^= h >> 33;
 h *= P2;
 h ^= h >> 29;
 h *= P3;
 h ^= h >> 32;
 return( h );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- STRUCT PersistIO ----------------------------*/
/*--------------------------------------------------------------------------*/

struct PersistIO
{
 /// \f$ x \f$ written by \f$ w \f$: header, payload, trailer
 template< class T >
 static void write( const T & x , PersistWriter & w ) {
  w.template header< T >();
  const_cast< T & >( x ).persist( w );  // which only reads it
  w.trailer();
  }

 /// a \f$ T \f$ read by \f$ r \f$ from bytes: header, trailer, payload
 template< class T >
 static T readBytes( PersistReader & r , bool verify ) {
  r.f_sumOn = false;  // summed at once by trailerFirst()
  r.template header< T >();
  r.trailerFirst( verify );
  T x = persistBlank< T >();
  x.persist( r );
  r.ended();
  return( x );
  }

 /// a \f$ T \f$ read by \f$ r \f$ from a stream: header, payload, trailer
 template< class T >
 static T readStream( PersistReader & r ) {
  r.template header< T >();
  T x = persistBlank< T >();
  x.persist( r );
  r.trailerLast();
  return( x );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PersistWriter ---------------------------*/
/*--------------------------------------------------------------------------*/

inline PersistWriter::PersistWriter( std::ostream & s , ByteOrder o )
    : f_s( &s ) , f_o( o )
{ }

/*--------------------------------------------------------------------------*/

inline PersistWriter::PersistWriter( std::vector< std::byte > & b ,
				     ByteOrder o )
    : f_b( &b ) , f_o( o )
{ }

/*--------------------------------------------------------------------------*/

inline void PersistWriter::bytes( const void * p , std::size_t m , bool sum )
{
 if( m == 0 )
  return;
 const auto * q = static_cast< const std::byte * >( p );
 if( sum )
  f_sum.update( q , m );
 if( f_s ) {
  if( ! f_s->write( reinterpret_cast< const char * >( q ) ,
		    std::streamsize( m ) ) )
   throw( PersistError( "ADS: the stream refused the bytes written" ) );
  }
 else
  f_b->insert( f_b->end() , q , q + m );
 f_off += m;
 }

/*--------------------------------------------------------------------------*/

inline void PersistWriter::pad( std::size_t a )
{
 static constexpr std::byte z[ 64 ] = {};
 if( const std::size_t r = f_off % a )
  bytes( z , a - r );
 }

/*--------------------------------------------------------------------------*/

inline void PersistWriter::word( std::uint64_t w )
{
 if( f_o != ByteOrder::native )
  w = detail::reversed( w );
 bytes( &w , 8 );
 }

/*--------------------------------------------------------------------------*/

template< class E >
inline void PersistWriter::numbers( const E * p , std::size_t n )
{
 if( f_o == ByteOrder::native ) {
  bytes( p , n * sizeof( E ) );
  return;
  }
 using U = typename detail::PersistUnitOf< E >::type;
 constexpr std::size_t B = 4096 / sizeof( U );
 U buf[ B ];  // swapped a block at a time
 const std::size_t m = n * ( sizeof( E ) / sizeof( U ) );
 for( std::size_t i = 0 ; i < m ; i += B ) {
  const std::size_t k = std::min( B , m - i );
  std::memcpy( buf ,
	       reinterpret_cast< const std::byte * >( p ) + i * sizeof( U ) ,
	       k * sizeof( U ) );
  detail::persistSwap( buf , k );
  bytes( buf , k * sizeof( U ) );
  }
 }

/*--------------------------------------------------------------------------*/

template< class T >
inline void PersistWriter::header( void )
{
 using F = detail::PersistFormat;
 const std::string_view name = T::persistName;
 if( name.size() > F::name )
  throw( PersistError( "ADS: the name of a structure is too long" ) );
 std::array< std::byte , F::header > h{};
 std::memcpy( h.data() , F::magic.data() , 8 );
 h[ 8 ] = std::byte( F::version & 0xFF );
 h[ 9 ] = std::byte( F::version >> 8 );
 h[ 10 ] = std::byte( f_o );
 const std::uint32_t v = detail::persistVersion< T >();
 for( int i = 0 ; i < 4 ; ++i )
  h[ 12 + i ] = std::byte( ( v >> ( 8 * i ) ) & 0xFF );
 detail::storeLittle( h.data() + 16 , detail::persistTag< T >() );
 const auto len = std::uint32_t( name.size() );
 for( int i = 0 ; i < 4 ; ++i )
  h[ 24 + i ] = std::byte( ( len >> ( 8 * i ) ) & 0xFF );
 std::memcpy( h.data() + 28 , name.data() , name.size() );
 bytes( h.data() , h.size() );
 }

/*--------------------------------------------------------------------------*/

inline void PersistWriter::trailer( void )
{
 using F = detail::PersistFormat;
 pad( 8 );
 std::array< std::byte , F::trailer > t{};
 detail::storeLittle( t.data() , f_off - F::header );
 detail::storeLittle( t.data() + 8 , f_sum.value() );
 std::memcpy( t.data() + 24 , F::endMagic.data() , 8 );
 bytes( t.data() , t.size() , false );
 if( f_s && ! f_s->flush() )
  throw( PersistError( "ADS: the stream refused the bytes written" ) );
 }

/*--------------------------------------------------------------------------*/

template< class T >
inline void PersistWriter::item( T & x )
{
 using U = std::remove_cv_t< T >;
 if constexpr( std::is_same_v< U , bool > )
  word( x ? 1 : 0 );
 else if constexpr( std::is_enum_v< U > )
  word( std::uint64_t( std::underlying_type_t< U >( x ) ) );
 else if constexpr( std::is_integral_v< U > ) {
  static_assert( sizeof( U ) <= 8 , "ADS: an integer wider than 64 bits" );
  if constexpr( std::is_signed_v< U > )
   word( std::uint64_t( std::int64_t( x ) ) );
  else
   word( std::uint64_t( x ) );
  }
 else if constexpr( std::is_floating_point_v< U > ) {
  static_assert( sizeof( U ) <= 8 , "ADS: a long double is not persisted" );
  word( std::bit_cast< std::uint64_t >( double( x ) ) );
  }
 else if constexpr( detail::IsVector< U >::value ) {
  using E = typename U::value_type;
  word( x.size() );
  if constexpr( detail::PersistBlock< E > ) {
   word( sizeof( E ) );
   pad( detail::PersistFormat::align );
   numbers( x.data() , x.size() );
   pad( 8 );
   }
  else if constexpr( std::is_same_v< E , bool > ) {  // packed in words
   std::vector< std::uint64_t > w( ( x.size() + 63 ) / 64 , 0 );
   for( std::size_t i = 0 ; i < x.size() ; ++i )
    if( x[ i ] )
     w[ i / 64 ] |= std::uint64_t( 1 ) << ( i % 64 );
   item( w );
   }
  else
   for( auto & e : x )
    item( e );
  }
 else if constexpr( detail::IsString< U >::value ) {
  word( x.size() );
  bytes( x.data() , x.size() );
  pad( 8 );
  }
 else if constexpr( detail::IsStdArray< U >::value ) {
  for( auto & e : x )
   item( e );
  }
 else if constexpr( std::is_array_v< U > ) {
  for( auto & e : x )
   item( e );
  }
 else if constexpr( detail::IsPair< U >::value ) {
  item( x.first );
  item( x.second );
  }
 else if constexpr( Persistent< U > ) {
  word( detail::persistVersion< U >() );
  const_cast< U & >( x ).persist( *this );
  }
 else if constexpr( detail::HasPersist< U > )
  const_cast< U & >( x ).persist( *this );
 else
  static_assert( detail::persistFalse< U > ,
		 "ADS: a member that cannot be persisted" );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS PersistReader ---------------------------*/
/*--------------------------------------------------------------------------*/

inline PersistReader::PersistReader( std::istream & s , std::string w )
    : f_s( &s ) , f_where( std::move( w ) )
{ }

/*--------------------------------------------------------------------------*/

inline PersistReader::PersistReader( std::span< const std::byte > b ,
				     std::string w , MappedFile m )
    : f_b( b ) , f_map( std::move( m ) ) , f_where( std::move( w ) )
{ }

/*--------------------------------------------------------------------------*/

inline void PersistReader::fail( const std::string & m ) const
{
 throw( PersistError( "ADS: " + f_where + ": " + m ) );
 }

/*--------------------------------------------------------------------------*/

inline std::uint64_t PersistReader::left( void ) const
{
 if( f_s )
  return( f_end - f_off );  // the most, unknown
 return( std::min< std::uint64_t >( f_end , f_b.size() ) - f_off );
 }

/*--------------------------------------------------------------------------*/

inline void PersistReader::bytes( void * p , std::size_t m , bool sum )
{
 if( m == 0 )
  return;
 if( m > left() )
  fail( "truncated: " + std::to_string( m ) +
	" more bytes expected at the byte " + std::to_string( f_off ) );
 if( f_s ) {
  if( ! f_s->read( static_cast< char * >( p ) , std::streamsize( m ) ) )
   fail( "truncated: the stream ended at the byte " +
	 std::to_string( f_off + std::uint64_t( f_s->gcount() ) ) );
  }
 else
  std::memcpy( p , f_b.data() + f_off , m );
 if( sum && f_sumOn )
  f_sum.update( static_cast< const std::byte * >( p ) , m );
 f_off += m;
 }

/*--------------------------------------------------------------------------*/

inline void PersistReader::skip( std::size_t a )
{
 std::byte z[ 64 ];
 if( const std::size_t r = f_off % a )
  bytes( z , a - r );
 }

/*--------------------------------------------------------------------------*/

inline std::uint64_t PersistReader::word( void )
{
 std::uint64_t w;
 bytes( &w , 8 );
 return( f_swap ? detail::reversed( w ) : w );
 }

/*--------------------------------------------------------------------------*/

template< class T >
inline void PersistReader::header( void )
{
 using F = detail::PersistFormat;
 std::array< std::byte , F::header > h;
 bytes( h.data() , h.size() );
 if( std::memcmp( h.data() , F::magic.data() , 8 ) != 0 )
  fail( "not a file of ADS (its first bytes are not those of one)" );
 const unsigned fv = unsigned( h[ 8 ] ) | ( unsigned( h[ 9 ] ) << 8 );
 if( fv != F::version )
  fail( "version " + std::to_string( fv ) + " of the format, this build " +
	"reads the version " + std::to_string( F::version ) );
 const auto o = ByteOrder( h[ 10 ] );
 if( ( o != ByteOrder::little ) && ( o != ByteOrder::big ) )
  fail( "corrupted: the byte order of the header" );
 f_swap = ( o != ByteOrder::native );
 std::uint32_t len = 0 , v = 0;
 for( int i = 0 ; i < 4 ; ++i ) {
  v |= std::uint32_t( h[ 12 + i ] ) << ( 8 * i );
  len |= std::uint32_t( h[ 24 + i ] ) << ( 8 * i );
  }
 if( len > F::name )
  fail( "corrupted: the length of the name of the structure" );
 const std::string name( reinterpret_cast< const char * >( h.data() + 28 ) ,
			 len );
 if( name != T::persistName )
  fail( "it holds a " + name + ", not a " + std::string( T::persistName ) );
 if( v != detail::persistVersion< T >() )
  fail( "version " + std::to_string( v ) + " of " + name +
	", this build reads the version " +
	std::to_string( detail::persistVersion< T >() ) );
 if( detail::loadLittle( h.data() + 16 ) != detail::persistTag< T >() )
  fail( "it holds a " + name + " of other parameters" );
 }

/*--------------------------------------------------------------------------*/

inline void PersistReader::trailerFirst( bool verify )
{
 using F = detail::PersistFormat;
 const std::size_t n = f_b.size();
 if( n < F::header + F::trailer )
  fail( "truncated: " + std::to_string( n ) + " bytes, fewer than a " +
	"header and a trailer" );
 const std::byte * t = f_b.data() + n - F::trailer;
 if( std::memcmp( t + 24 , F::endMagic.data() , 8 ) != 0 )
  fail( "truncated: it does not end with the trailer of a file of ADS" );
 const std::uint64_t pay = detail::loadLittle( t );
 if( pay != n - F::header - F::trailer )
  fail( "truncated: " + std::to_string( n ) + " bytes, the trailer " +
	"gives a payload of " + std::to_string( pay ) );
 f_end = F::header + pay;
 if( verify ) {
  detail::PersistChecksum c;
  c.update( f_b.data() , std::size_t( f_end ) );
  if( c.value() != detail::loadLittle( t + 8 ) )
   fail( "corrupted: its checksum differs from that of its trailer" );
  }
 }

/*--------------------------------------------------------------------------*/

inline void PersistReader::trailerLast( void )
{
 using F = detail::PersistFormat;
 std::array< std::byte , F::trailer > t;
 const std::uint64_t pay = f_off - F::header;
 bytes( t.data() , t.size() , false );
 if( std::memcmp( t.data() + 24 , F::endMagic.data() , 8 ) != 0 )
  fail( "corrupted: no trailer after the payload" );
 if( detail::loadLittle( t.data() ) != pay )
  fail( "corrupted: the trailer gives a payload of " +
	std::to_string( detail::loadLittle( t.data() ) ) + " bytes, " +
	std::to_string( pay ) + " were read" );
 if( detail::loadLittle( t.data() + 8 ) != f_sum.value() )
  fail( "corrupted: its checksum differs from that of its trailer" );
 }

/*--------------------------------------------------------------------------*/

inline void PersistReader::ended( void ) const
{
 if( f_off != f_end )
  fail( "corrupted: " + std::to_string( f_end - f_off ) + " bytes of the " +
	"payload were not read" );
 }

/*--------------------------------------------------------------------------*/

template< class E , class A >
inline void PersistReader::numbers( std::vector< E , A > & v )
{
 const std::uint64_t n = word() , w = word();
 if( w != sizeof( E ) )
  fail( "corrupted: an array of items of " + std::to_string( w ) + " bytes, " +
	std::to_string( sizeof( E ) ) + " expected" );
 skip( detail::PersistFormat::align );
 if( n > left() / sizeof( E ) )
  fail( "truncated: an array of " + std::to_string( n ) + " items of " +
	std::to_string( sizeof( E ) ) + " bytes at the byte " +
	std::to_string( f_off ) );
 const std::size_t m = std::size_t( n ) * sizeof( E );
 if constexpr( detail::AdoptsMapping< A > ) {
  if( f_map && ( ! f_swap ) && ( n > 0 ) ) {  // a view of the mapping
   auto * p = const_cast< std::byte * >( f_b.data() + f_off );
   if( f_map.holds( p ) &&
       ( reinterpret_cast< std::uintptr_t >( p ) % alignof( E ) == 0 ) ) {
    f_map.offer( p , m );
    std::vector< E , A > u{ A( f_map ) };
    u.reserve( std::size_t( n ) );  // the bytes offered, taken
    if( static_cast< void * >( u.data() ) == p ) {
     u.resize( std::size_t( n ) );  // left as they are
     v = std::move( u );
     f_off += m;
     skip( 8 );
     return;
     }
    ( void )f_map.take( m );  // not taken: the offer withdrawn
    }
   }
  }
 // by blocks of 4 KB through a buffer, so that the size in a corrupted
 // stream allocates no more than the stream holds
 static_assert( sizeof( E ) <= 4096 && alignof( E ) <= 64 ,
		"ADS: an item of an array of more than 4 KB" );
 using U = typename detail::PersistUnitOf< E >::type;
 constexpr std::size_t B = 4096 / sizeof( E );
 alignas( 64 ) std::byte buf[ 4096 ];
 v.clear();
 if( ! f_s )
  v.reserve( std::size_t( n ) );
 for( std::size_t i = 0 ; i < n ; ) {
  const std::size_t k = std::min< std::size_t >( B , std::size_t( n ) - i );
  bytes( buf , k * sizeof( E ) );
  if( f_swap )
   detail::persistSwap( reinterpret_cast< U * >( buf ) ,
			k * ( sizeof( E ) / sizeof( U ) ) );
  const E * const b = reinterpret_cast< const E * >( buf );
  v.insert( v.end() , b , b + k );
  i += k;
  }
 skip( 8 );
 }

/*--------------------------------------------------------------------------*/

template< class T >
inline void PersistReader::item( T & x )
{
 using U = std::remove_cv_t< T >;
 if constexpr( std::is_same_v< U , bool > ) {
  const std::uint64_t w = word();
  require( w <= 1 , "a bool other than 0 and 1" );
  x = ( w == 1 );
  }
 else if constexpr( std::is_enum_v< U > ) {
  using I = std::underlying_type_t< U >;
  I i;
  item( i );
  x = U( i );
  }
 else if constexpr( std::is_integral_v< U > ) {
  const std::uint64_t w = word();
  if constexpr( std::is_signed_v< U > ) {
   const auto s = std::int64_t( w );
   require( ( s >= std::int64_t( std::numeric_limits< U >::min() ) ) &&
	      ( s <= std::int64_t( std::numeric_limits< U >::max() ) ) ,
	    "an integer out of the range of its type" );
   x = U( s );
   }
  else {
   require( w <= std::uint64_t( std::numeric_limits< U >::max() ) ,
	    "an integer out of the range of its type" );
   x = U( w );
   }
  }
 else if constexpr( std::is_floating_point_v< U > )
  x = U( std::bit_cast< double >( word() ) );
 else if constexpr( detail::IsVector< U >::value ) {
  using E = typename U::value_type;
  if constexpr( detail::PersistBlock< E > )
   numbers( x );
  else if constexpr( std::is_same_v< E , bool > ) {
   const std::uint64_t n = word();
   std::vector< std::uint64_t > w;
   item( w );
   require( w.size() == ( n + 63 ) / 64 , "the words of the bits" );
   x.assign( std::size_t( n ) , false );
   for( std::size_t i = 0 ; i < n ; ++i )
    x[ i ] = ( w[ i / 64 ] >> ( i % 64 ) ) & 1;
   }
  else {
   const std::uint64_t n = word();
   require( n <= left() / 8 , "the size of an array" );  // 8 bytes an item
   x.clear();
   for( std::uint64_t i = 0 ; i < n ; ++i ) {
    E e{};
    item( e );
    x.push_back( std::move( e ) );
    }
   }
  }
 else if constexpr( detail::IsString< U >::value ) {
  const std::uint64_t n = word();
  require( n <= left() , "the size of a string" );
  x.resize( std::size_t( n ) );
  bytes( x.data() , x.size() );
  skip( 8 );
  }
 else if constexpr( detail::IsStdArray< U >::value || std::is_array_v< U > ) {
  for( auto & e : x )
   item( e );
  }
 else if constexpr( detail::IsPair< U >::value ) {
  item( x.first );
  item( x.second );
  }
 else if constexpr( Persistent< U > ) {
  const std::uint64_t v = word();
  if( v != detail::persistVersion< U >() )
   fail( "version " + std::to_string( v ) + " of the " +
	 std::string( U::persistName ) + " in it, this build reads the " +
	 "version " + std::to_string( detail::persistVersion< U >() ) );
  x.persist( *this );
  }
 else if constexpr( detail::HasPersist< U > )
  x.persist( *this );
 else
  static_assert( detail::persistFalse< U > ,
		 "ADS: a member that cannot be persisted" );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< Persistent T >
inline void store( const T & x , std::ostream & s , ByteOrder o )
{
 PersistWriter w( s , o );
 detail::PersistIO::write( x , w );
 }

/*--------------------------------------------------------------------------*/

template< Persistent T >
inline void store( const T & x , const std::filesystem::path & p ,
		   ByteOrder o )
{
 std::ofstream s( p , std::ios::binary | std::ios::trunc );
 if( ! s )
  throw( PersistError( "ADS: " + p.string() + ": cannot write the file" ) );
 try {
  store( x , s , o );
  }
 catch( const PersistError & ) {
  throw( PersistError( "ADS: " + p.string() + ": cannot write the file" ) );
  }
 }

/*--------------------------------------------------------------------------*/

template< Persistent T >
inline std::vector< std::byte > store( const T & x , ByteOrder o )
{
 std::vector< std::byte > b;
 PersistWriter w( b , o );
 detail::PersistIO::write( x , w );
 return( b );
 }

/*--------------------------------------------------------------------------*/

template< Persistent T >
inline T restore( std::istream & s )
{
 PersistReader r( s , "the stream" );
 return( detail::PersistIO::readStream< T >( r ) );
 }

/*--------------------------------------------------------------------------*/

template< Persistent T >
inline T restore( std::span< const std::byte > b )
{
 PersistReader r( b , "the buffer" );
 return( detail::PersistIO::readBytes< T >( r , true ) );
 }

/*--------------------------------------------------------------------------*/

template< Persistent T >
inline T restore( const std::filesystem::path & p )
{
 std::ifstream s( p , std::ios::binary );
 if( ! s )
  throw( PersistError( "ADS: " + p.string() + ": cannot open the file" ) );
 PersistReader r( s , p.string() );
 T x = detail::PersistIO::readStream< T >( r );
 if( s.peek() != std::ifstream::traits_type::eof() )
  throw( PersistError( "ADS: " + p.string() +
		       ": corrupted: bytes after the trailer" ) );
 return( x );
 }

/*--------------------------------------------------------------------------*/

template< Persistent T >
inline T restoreMapped( const std::filesystem::path & p , bool verify )
{
 MappedFile m( p );
 const std::span< const std::byte > b = m.bytes();
 PersistReader r( b , p.string() , std::move( m ) );
 return( detail::PersistIO::readBytes< T >( r , verify ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Persistence.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
