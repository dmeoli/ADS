/*--------------------------------------------------------------------------*/
/*-------------------- File engineered_persistence.cpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of the persistence of Persistence.h, beyond the book: the format
 * (its header, its trailer, its checksum), the rejection of a file that
 * is not of ADS, of another structure, truncated or corrupted, with the
 * message that says which; the payload in either byte order, read on a
 * machine of the other one by swapping its numbers; the mapped files,
 * whose arrays are views over the mapping, opened with no copy, copied on
 * a copy of the structure and never written back to the file; and the
 * round trip, by a buffer, a stream, a file and a mapping, of every
 * persistent structure: the succinct ones, the indexes of texts and the
 * learned ones, the filters, the perfect hash functions, and the graphs
 * and the matrices in CSR form. With `ADS_WITH_CEREAL`, the same
 * structures in the archives of cereal, and a `BitVector` in the layout of
 * the `bit_vector` of sdsl-lite. At the level 3 of the contracts.
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

#include "ADSTest.h"

#include "ADS/compression/FMIndex.h"
#include "ADS/compression/HuffmanFMIndex.h"
#include "ADS/compression/MoveRIndex.h"
#include "ADS/compression/RIndex.h"
#include "ADS/external/Persistence.h"
#include "ADS/external/RecordFile.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/numeric/CsrMatrix.h"
#include "ADS/search/CompressedPGMIndex.h"
#include "ADS/search/PGMIndex.h"
#include "ADS/succinct/BPTree.h"
#include "ADS/succinct/BVGraph.h"
#include "ADS/succinct/BalancedParentheses.h"
#include "ADS/succinct/BitVector.h"
#include "ADS/succinct/CompactRankSelect.h"
#include "ADS/succinct/EliasFano.h"
#include "ADS/succinct/EliasFanoGraph.h"
#include "ADS/succinct/HuffmanWaveletTree.h"
#include "ADS/succinct/InterleavedRankSelect.h"
#include "ADS/succinct/JacobsonTree.h"
#include "ADS/succinct/K2Tree.h"
#include "ADS/succinct/LAVector.h"
#include "ADS/succinct/LoudsTree.h"
#include "ADS/succinct/MoveStructure.h"
#include "ADS/succinct/PackedArray.h"
#include "ADS/succinct/PartitionedEliasFano.h"
#include "ADS/succinct/RRRBitVector.h"
#include "ADS/succinct/RankSelect.h"
#include "ADS/succinct/WaveletMatrix.h"
#include "ADS/dictionary/FingerprintPerfectHash.h"
#include "ADS/dictionary/MinimalPerfectHash.h"
#include "ADS/dictionary/PerfectHashing.h"
#include "ADS/dictionary/RecSplit.h"
#include "ADS/set/BinaryFuseFilter.h"
#include "ADS/set/BlockedBloomFilter.h"
#include "ADS/set/BloomFilter.h"
#include "ADS/set/BloomVariants.h"
#include "ADS/set/CuckooFilter.h"
#include "ADS/set/GrafiteFilter.h"
#include "ADS/set/PrefixFilter.h"
#include "ADS/set/RibbonFilter.h"

#if defined( ADS_WITH_CEREAL )
 #include "ADS/interop/cereal/Cereal.h"

 #include <cereal/archives/binary.hpp>
 #include <cereal/archives/json.hpp>
 #include <cereal/archives/portable_binary.hpp>
 #if defined( ADS_WITH_SDSL )
  #include <sdsl/int_vector.hpp>
 #endif
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

static_assert( ADS_CONTRACTS == 3 , "the tests check all the contracts" );

namespace
{

/// the bytes \f$ b \f$ written to the file \f$ p \f$
void spill( const std::filesystem::path & p , std::span< const std::byte > b )
{
 std::ofstream out( p , std::ios::binary | std::ios::trunc );
 out.write( reinterpret_cast< const char * >( b.data() ) ,
	    std::streamsize( b.size() ) );
 }

/// the message of the PersistError that \f$ f \f$ throws, empty if none
template< class F >
std::string message( F && f )
{
 try {
  f();
  }
 catch( const PersistError & e ) {
  return( e.what() );
  }
 return( "" );
 }

/// \f$ x \f$ written and read back by a buffer, a stream, a file and a
/// mapping, in both byte orders, each copy the same as \f$ x \f$ by
/// \f$ \mathit{same} \f$; its bytes corrupted, truncated, or given to
/// another structure \f$ O \f$, rejected
template< class T , class O = BitVector , class S >
void roundTrip( const T & x , S && same )
{
 for( const ByteOrder o : { ByteOrder::little , ByteOrder::big } ) {
  const auto b = store( x , o );
  CHECK( same( x , restore< T >( std::span< const std::byte >( b ) ) ) );
  std::stringstream s;
  store( x , s , o );
  CHECK( same( x , restore< T >( s ) ) );
  }
 TempDirectory d;
 const auto p = d.path() / "x.ads";
 store( x , p );
 CHECK( same( x , restore< T >( p ) ) );
 {
  const T m = restoreMapped< T >( p );
  CHECK( same( x , m ) );
  const T c = m;  // a copy, on new memory
  CHECK( same( x , c ) );
  }
 CHECK( same( x , restoreMapped< T >( p , true ) ) );
 store( x , p ,
	ByteOrder::big == ByteOrder::native ? ByteOrder::little
					    : ByteOrder::big );
 CHECK( same( x , restoreMapped< T >( p , true ) ) );  // copied, swapped
 // a byte of the payload changed, the last byte cut, half of the file cut
 const auto b = store( x );
 auto bad = b;
 bad[ 128 + ( b.size() - 160 ) / 2 ] ^= std::byte( 0x10 );
 CHECK( message( [ & ] {
	 ( void )restore< T >( std::span< const std::byte >( bad ) );
	 } )
	  .find( "checksum" ) != std::string::npos );
 spill( p , bad );  // a stream finds the checksum at its end, or before
		   // a member that the change makes wrong
 CHECK( ! message( [ & ] { ( void )restore< T >( p ); } ).empty() );
 CHECK( message( [ & ] { ( void )restoreMapped< T >( p , true ); } )
	  .find( "checksum" ) != std::string::npos );
 for( const std::size_t cut :
      { b.size() - 1 , b.size() / 2 , std::size_t( 100 ) } ) {
  const std::span< const std::byte > t( b.data() , cut );
  CHECK( message( [ & ] { ( void )restore< T >( t ); } ).find( "truncated" ) !=
	 std::string::npos );
  spill( p , t );
  CHECK( message( [ & ] { ( void )restoreMapped< T >( p ); } )
	   .find( "truncated" ) != std::string::npos );
  CHECK( ! message( [ & ] { ( void )restore< T >( p ); } ).empty() );
  std::stringstream s(
    std::string( reinterpret_cast< const char * >( t.data() ) , t.size() ) );
  CHECK( ! message( [ & ] { ( void )restore< T >( s ); } ).empty() );
  }
 if constexpr( ! std::is_same_v< T , O > )
  CHECK( message( [ & ] {
	  ( void )restore< O >( std::span< const std::byte >( b ) );
	  } )
	   .find( "it holds a " ) != std::string::npos );
 }

/// \f$ n \f$ random bits, each 1 with probability \f$ 1 / k \f$
BitVector randomBits( std::size_t n , unsigned k , std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 BitVector b;
 for( std::size_t i = 0 ; i < n ; ++i )
  b.push_back( g() % k == 0 );
 return( b );
 }

/// \f$ n \f$ sorted random integers below \f$ u \f$
std::vector< std::uint64_t > sortedRandom( std::size_t n , std::uint64_t u ,
					   std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::uint64_t > x( n );
 for( auto & v : x )
  v = g() % u;
 std::sort( x.begin() , x.end() );
 return( x );
 }

/// whether two structures of bits answer the same ranks and selects
template< class R >
bool sameRanks( const R & a , const R & b )
{
 if( ( a.size() != b.size() ) || ( a.ones() != b.ones() ) )
  return( false );
 for( std::size_t i = 0 ; i <= a.size() ; i += 37 )
  if( a.rank1( i ) != b.rank1( i ) )
   return( false );
 for( std::size_t k = 1 ; k <= a.ones() ; k += 29 )
  if( a.select1( k ) != b.select1( k ) )
   return( false );
 for( std::size_t k = 1 ; k <= a.size() - a.ones() ; k += 29 )
  if( a.select0( k ) != b.select0( k ) )
   return( false );
 return( true );
 }

/// whether two sequences have the same items by `[]`
const auto sameItems = []( const auto & a , const auto & b )
{
 if( a.size() != b.size() )
  return( false );
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  if( a[ i ] != b[ i ] )
   return( false );
 return( true );
 };

/// a random text of \f$ n \f$ characters of an alphabet of \f$ s \f$
std::string randomText( std::size_t n , unsigned s , std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 std::string t( n , 'a' );
 for( auto & c : t )
  c = char( 'a' + g() % s );
 return( t );
 }

/// a random tree of \f$ n \f$ nodes, by the lists of the children
std::vector< std::vector< std::size_t > > randomTree( std::size_t n ,
						      std::uint64_t seed )
{
 std::mt19937_64 g( seed );
 std::vector< std::vector< std::size_t > > c( n );
 for( std::size_t v = 1 ; v < n ; ++v )
  c[ g() % v ].push_back( v );
 return( c );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- THE FORMAT ------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-format" )
{
 const BitVector b( "1011001110001111" );
 const auto f = store( b );
 // the header, the payload and the trailer
 CHECK( f.size() % 8 == 0 );
 CHECK( std::memcmp( f.data() ,
		     "\x89"
		     "ADS\r\n\x1A\n" ,
		     8 ) == 0 );
 CHECK( std::memcmp( f.data() + f.size() - 8 , "ADS-END\n" , 8 ) == 0 );
 CHECK( std::memcmp( f.data() + 28 , "BitVector" , 9 ) == 0 );
 CHECK( unsigned( f[ 10 ] ) == unsigned( ByteOrder::native ) );
 CHECK( restore< BitVector >( std::span< const std::byte >( f ) ) == b );
 // an empty structure, and one more payload than one structure reads
 CHECK( restore< BitVector >( std::span< const std::byte >(
	  store( BitVector() ) ) ) == BitVector() );
 // not a file of ADS, another version of the format, another structure
 auto g = f;
 g[ 1 ] = std::byte( 'X' );
 CHECK( message( [ & ] {
	 ( void )restore< BitVector >( std::span< const std::byte >( g ) );
	 } )
	  .find( "not a file of ADS" ) != std::string::npos );
 g = f;
 g[ 8 ] = std::byte( 2 );
 CHECK( message( [ & ] {
	 ( void )restore< BitVector >( std::span< const std::byte >( g ) );
	 } )
	  .find( "version 2 of the format" ) != std::string::npos );
 CHECK( message( [ & ] {
	 ( void )restore< RankSelect >( std::span< const std::byte >( f ) );
	 } )
	  .find( "it holds a BitVector, not a RankSelect" ) !=
	std::string::npos );
 // a payload that breaks an invariant (a bit past the size), with a
 // checksum made right: rejected as corrupted
 g = f;
 g[ 192 + 3 ] = std::byte( 0xFF );  // the words start at the byte 192
 detail::PersistChecksum c;
 c.update( g.data() , g.size() - 32 );
 detail::storeLittle( g.data() + g.size() - 24 , c.value() );
 CHECK( message( [ & ] {
	 ( void )restore< BitVector >( std::span< const std::byte >( g ) );
	 } )
	  .find( "corrupted: the words of a BitVector" ) !=
	std::string::npos );
 // a file that cannot be opened
 CHECK( message( [ & ] {
	 ( void )restoreMapped< BitVector >( "/nonexistent/x.ads" );
	 } )
	  .find( "/nonexistent/x.ads" ) != std::string::npos );
 // the checksum of a buffer does not depend on how it is cut
 std::vector< std::byte > z( 1000 );
 for( std::size_t i = 0 ; i < z.size() ; ++i )
  z[ i ] = std::byte( i * 7 + 3 );
 detail::PersistChecksum all , parts;
 all.update( z.data() , z.size() );
 for( std::size_t i = 0 , k = 1 ; i < z.size() ; i += k , k = k % 13 + 1 )
  parts.update( z.data() + i , std::min( k , z.size() - i ) );
 CHECK( all.value() == parts.value() );
 roundTrip< BitVector , RankSelect >( randomBits( 5000 , 3 , 1 ) ,
				     std::equal_to<>() );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-byte-order" )
{
 // the same structure in the two orders: the headers differ in the order
 // alone, and each number of the payload has its bytes reversed
 BitVector b;
 b.append( 0x0102030405060708ULL , 64 );
 const auto L = store( b , ByteOrder::little ) ,
   B = store( b , ByteOrder::big );
 CHECK( L.size() == B.size() );
 CHECK( unsigned( L[ 10 ] ) == 1 && unsigned( B[ 10 ] ) == 2 );
 CHECK( unsigned( L[ 128 ] ) == 64 && unsigned( B[ 128 + 7 ] ) == 64 );
 for( std::size_t i = 0 ; i < 8 ; ++i )  // the first word of the bits
  CHECK( L[ 192 + i ] == std::byte( 8 - i ) &&
	 B[ 192 + i ] == std::byte( i + 1 ) );
 CHECK( restore< BitVector >( std::span< const std::byte >( L ) ) == b );
 CHECK( restore< BitVector >( std::span< const std::byte >( B ) ) == b );
 // numbers of every width, swapped back
 const PackedArray< 2 > p( 1000 , 3 );
 const TritArray t( 999 , 2 );
 const auto P = store( p , ByteOrder::big );
 const auto T = store( t , ByteOrder::big );
 CHECK( restore< PackedArray< 2 > >( std::span< const std::byte >( P ) ) ==
	p );
 CHECK( restore< TritArray >( std::span< const std::byte >( T ) ) == t );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-mapped" )
{
 TempDirectory d;
 const auto p = d.path() / "bits.ads";
 // 2^24 bits, 2 MB of words: a copy of them would lie on a huge page,
 // their view lies where the file has them, at its byte 192
 const BitVector b = randomBits( std::size_t( 1 ) << 24 , 2 , 7 );
 store( b , p );
 {
  BitVector m = restoreMapped< BitVector >( p );
  const auto at = reinterpret_cast< std::uintptr_t >( m.words().data() );
  CHECK( at % 64 == 0 && at % 4096 == 192 );
  CHECK( m == b );
  // a write lands in the memory of the process, never in the file
  m.set( 0 , ! m[ 0 ] );
  CHECK( m[ 0 ] != b[ 0 ] );
  CHECK( restore< BitVector >( p ) == b );
  // a copy is on new memory, a move keeps the view
  const BitVector c = m;
  CHECK( c.words().data() != m.words().data() && c == m );
  const auto * w = m.words().data();
  BitVector v = std::move( m );
  CHECK( v.words().data() == w );
  // the view outlives the name of the file
  std::filesystem::remove( p );
  CHECK( v.size() == b.size() && v[ 1 ] == b[ 1 ] );
  // the vector grows past the mapping, onto new memory
  for( int i = 0 ; i < 1000 ; ++i )
   v.push_back( true );
  CHECK( v.size() == b.size() + 1000 && v[ b.size() ] && v[ 1 ] == b[ 1 ] );
  }
 // a directory on top of the bits, each array a view
 const RankSelect r( randomBits( std::size_t( 1 ) << 22 , 5 , 8 ) );
 store( r , p );
 const RankSelect m = restoreMapped< RankSelect >( p );
 CHECK( reinterpret_cast< std::uintptr_t >( m.bits().words().data() ) % 4096 !=
	0 );
 CHECK( sameRanks( m , r ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE SUCCINCT STRUCTURES ------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-succinct" )
{
 for( const std::size_t n : { std::size_t( 0 ) , std::size_t( 1 ) ,
			      std::size_t( 777 ) , std::size_t( 70000 ) } ) {
  const BitVector b = randomBits( n , 3 , n + 1 );
  roundTrip< BitVector , RankSelect >( b , std::equal_to<>() );
  roundTrip< RankSelect , LazyRankSelect >( RankSelect( b ) ,
					   sameRanks< RankSelect > );
  // a lazy directory with one inventory built, and with none
  LazyRankSelect l( b );
  roundTrip< LazyRankSelect , RankSelect >( l , sameRanks< LazyRankSelect > );
  if( b.count() > 0 )
   ( void )l.select1( 1 );
  roundTrip< LazyRankSelect , RankSelect >( l , sameRanks< LazyRankSelect > );
  roundTrip< RankSelect >( RankSelect( b , RankSelect::Selects::ONES ) ,
			   []( const RankSelect & x , const RankSelect & y ) {
			    for( std::size_t k = 1 ; k <= x.ones() ; k += 7 )
			     if( x.select1( k ) != y.select1( k ) )
			      return( false );
			    return( x.size() == y.size() );
			    } );
  roundTrip< CompactRankSelect >( CompactRankSelect( b ) ,
				  sameRanks< CompactRankSelect > );
  roundTrip< InterleavedRankSelect >( InterleavedRankSelect( b ) ,
				      sameRanks< InterleavedRankSelect > );
  roundTrip< RRRBitVector >( RRRBitVector( b ) , sameRanks< RRRBitVector > );
  }
 for( const std::size_t n :
      { std::size_t( 0 ) , std::size_t( 1 ) , std::size_t( 3000 ) } ) {
  auto x = sortedRandom( n , 1000000 , n + 3 );
  x.erase( std::unique( x.begin() , x.end() ) , x.end() );
  roundTrip< EliasFano , LazyEliasFano >(
    EliasFano( x ) , []( const EliasFano & a , const EliasFano & b ) {
     return( a.decode() == b.decode() && a.universe() == b.universe() );
     } );
  roundTrip< LazyEliasFano >( LazyEliasFano( x ) , sameItems );
  roundTrip< PartitionedEliasFano >( PartitionedEliasFano( x ) , sameItems );
  roundTrip< LAVector >( LAVector( x ) , sameItems );
  }
 roundTrip< PackedArray< 4 > >( PackedArray< 4 >( 3000 , 9 ) ,
				std::equal_to<>() );
 roundTrip< TritArray >( TritArray( 3001 , 1 ) , std::equal_to<>() );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-wavelet" )
{
 std::mt19937_64 g( 11 );
 std::vector< std::uint64_t > x( 5000 );
 std::vector< std::uint32_t > y( 5000 );
 for( std::size_t i = 0 ; i < x.size() ; ++i )
  y[ i ] = std::uint32_t( x[ i ] = g() % ( i % 7 == 0 ? 300 : 9 ) );
 const auto same = []( const auto & a , const auto & b ) {
  if( a.size() != b.size() )
   return( false );
  for( std::size_t i = 0 ; i < a.size() ; i += 3 )
   if( ( a.access( i ) != b.access( i ) ) ||
       ( a.rank( a.access( i ) , i ) != b.rank( b.access( i ) , i ) ) )
    return( false );
  return( true );
  };
 roundTrip< WaveletMatrix >( WaveletMatrix( x ) , same );
 roundTrip< WaveletMatrix >( WaveletMatrix() , same );
 roundTrip< HuffmanWaveletTree<> >( HuffmanWaveletTree<>( y ) , same );
 roundTrip< HuffmanWaveletTree< CompactRankSelect > , HuffmanWaveletTree<> >(
   HuffmanWaveletTree< CompactRankSelect >( y ) , same );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-trees" )
{
 const auto c = randomTree( 3000 , 5 );
 roundTrip< LoudsTree >(
   LoudsTree( c ) , []( const LoudsTree & a , const LoudsTree & b ) {
    return( a.bits() == b.bits() && a.size() == b.size() );
    } );
 roundTrip< BPTree >( BPTree( c ) , []( const BPTree & a , const BPTree & b ) {
  if( a.size() != b.size() )
   return( false );
  return( a.parentheses().bits().bits() == b.parentheses().bits().bits() );
  } );
 roundTrip< BalancedParentheses >(
   BalancedParentheses( BitVector( "1101100100" ) ) ,
   []( const BalancedParentheses & a , const BalancedParentheses & b ) {
    for( std::size_t i = 0 ; i < a.size() ; ++i )
     if( a.excess( i ) != b.excess( i ) )
      return( false );
    return( a.findClose( 0 ) == b.findClose( 0 ) );
    } );
 roundTrip< JacobsonTree >(
   JacobsonTree( BitVector( "11011000100" ) ) ,
   []( const JacobsonTree & a , const JacobsonTree & b ) {
    return( a.bits() == b.bits() );
    } );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-text-indexes" )
{
 const std::string t = randomText( 4000 , 4 , 13 );
 const std::string P[] = { "ab" , "cab" , "dddd" , "a" , "zz" };
 const auto same = [ & ]( const auto & a , const auto & b ) {
  for( const auto & p : P )
   if( ( a.count( p ) != b.count( p ) ) || ( a.locate( p ) != b.locate( p ) ) )
    return( false );
  return( a.size() == b.size() );
  };
 roundTrip< FMIndex >( FMIndex( t , 32 , 8 ) , same );
 roundTrip< HuffmanFMIndex<> >( HuffmanFMIndex<>( t , 16 ) , same );
 roundTrip< RIndex >( RIndex( t ) , same );
 roundTrip< MoveRIndex >( MoveRIndex( t ) , same );
 roundTrip< RIndex >( RIndex() , same );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-learned" )
{
 const auto x = sortedRandom( 20000 , std::uint64_t( 1 ) << 40 , 17 );
 const auto same = [ & ]( const auto & a , const auto & b ) {
  for( std::size_t i = 0 ; i < x.size() ; i += 13 )
   if( a.lowerBound( x[ i ] + 1 ) != b.lowerBound( x[ i ] + 1 ) )
    return( false );
  return( a.size() == b.size() );
  };
 roundTrip< PGMIndex< std::uint64_t > >( PGMIndex< std::uint64_t >( x ) ,
					 same );
 roundTrip< CompressedPGMIndex< std::uint64_t > >(
   CompressedPGMIndex< std::uint64_t >( x ) , same );
 // the parameters are in the file
 const auto f = store( PGMIndex< std::uint64_t >( x ) );
 CHECK( message( [ & ] {
	 ( void )restore< PGMIndex< std::uint64_t , 32 > >(
	   std::span< const std::byte >( f ) );
	 } )
	  .find( "of other parameters" ) != std::string::npos );
 std::vector< double > d( 1000 );
 for( std::size_t i = 0 ; i < d.size() ; ++i )
  d[ i ] = double( i ) * 0.5;
 roundTrip< PGMIndex< double > >(
   PGMIndex< double >( d ) ,
   []( const PGMIndex< double > & a , const PGMIndex< double > & b ) {
    return( a.lowerBound( 100.25 ) == b.lowerBound( 100.25 ) );
    } );
 }

/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-succinct-graphs" )
{
 const auto c = randomTree( 500 , 19 );
 std::vector< std::pair< std::size_t , std::size_t > > e;
 for( std::size_t v = 0 ; v < c.size() ; ++v )
  for( const auto w : c[ v ] )
   e.emplace_back( v , w );
 roundTrip< K2Tree >( K2Tree( c.size() , e ) ,
		      []( const K2Tree & a , const K2Tree & b ) {
		       for( std::size_t v = 0 ; v < a.order() ; v += 3 )
			if( a.neighbors( v ) != b.neighbors( v ) )
			 return( false );
		       return( a.edges() == b.edges() );
		       } );
 const auto same = []( const auto & a , const auto & b ) {
  for( std::size_t v = 0 ; v < a.order() ; ++v )
   if( a.successors( v ) != b.successors( v ) )
    return( false );
  return( a.edges() == b.edges() );
  };
 roundTrip< BVGraph >( BVGraph( c ) , same );
 roundTrip< EliasFanoGraph >( EliasFanoGraph( c ) , same );
 const std::vector< std::uint64_t > p{ 0 , 3 , 5 , 9 } , q{ 4 , 0 , 7 , 2 };
 const std::vector< std::uint32_t > tg{ 1 , 2 , 3 , 4 };
 roundTrip< MoveStructure >(
   MoveStructure( p , q , tg , 12 ) ,
   []( const MoveStructure & a , const MoveStructure & b ) {
    for( std::size_t i = 0 ; i < a.intervals() ; ++i )
     if( ( a.start( i ) != b.start( i ) ) || ( a.tag( i ) != b.tag( i ) ) )
      return( false );
    return( a.size() == b.size() );
    } );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- THE FILTERS ----------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-filters" )
{
 std::mt19937_64 g( 23 );
 std::vector< std::uint64_t > keys( 3000 ) , other( 3000 );
 for( auto & k : keys )
  k = g() >> 20;
 for( auto & k : other )
  k = g() >> 20;
 // the same answers on the keys and on other items
 const auto same = [ & ]( const auto & a , const auto & b ) {
  for( std::size_t i = 0 ; i < keys.size() ; ++i )
   if( ( a.maybe( keys[ i ] ) != b.maybe( keys[ i ] ) ) ||
       ( a.maybe( other[ i ] ) != b.maybe( other[ i ] ) ) )
    return( false );
  return( true );
  };
 const auto fill = [ & ]( auto f ) {
  for( const auto k : keys )
   f.insert( k );
  return( f );
  };
 roundTrip< BloomFilter< std::uint64_t > >(
   fill( BloomFilter< std::uint64_t >( 30000 , 5 ) ) , same );
 roundTrip< BlockedBloomFilter< std::uint64_t > >(
   fill( BlockedBloomFilter< std::uint64_t >( keys.size() ) ) , same );
 roundTrip< SpectralBloomFilter< std::uint64_t > >(
   fill( SpectralBloomFilter< std::uint64_t >( 30000 , 4 ) ) , same );
 roundTrip< PartitionedBloomFilter< std::uint64_t > >(
   fill( PartitionedBloomFilter< std::uint64_t >( 30000 , 4 ) ) , same );
 auto C = CuckooFilter< std::uint64_t >( keys.size() );
 for( const auto k : keys )
  ( void )C.insert( k );
 roundTrip< CuckooFilter< std::uint64_t > >( C , same );
 roundTrip< PrefixFilter< std::uint64_t > >(
   fill( PrefixFilter< std::uint64_t >( keys.size() ) ) , same );
 roundTrip< RibbonFilter< std::uint64_t > >(
   RibbonFilter< std::uint64_t >( keys ) , same );
 roundTrip< BinaryFuseFilter< std::uint64_t > >(
   BinaryFuseFilter< std::uint64_t >( keys ) , same );
 roundTrip< BinaryFuseFilter< std::uint64_t , std::uint16_t > >(
   BinaryFuseFilter< std::uint64_t , std::uint16_t >( keys ) , same );
 roundTrip< BinaryFuseFilter< std::uint64_t > >(
   BinaryFuseFilter< std::uint64_t >() , same );
 // the range filters, on ranges around the keys
 const auto ranges = [ & ]( const auto & a , const auto & b ) {
  for( std::size_t i = 0 ; i < keys.size() ; ++i )
   if( a.mayContain( other[ i ] , other[ i ] + 100 ) !=
       b.mayContain( other[ i ] , other[ i ] + 100 ) )
    return( false );
  return( same( a , b ) );
  };
 roundTrip< GrafiteFilter<> >( GrafiteFilter<>( keys ) , ranges );
 roundTrip< BucketingFilter<> >( BucketingFilter<>( keys ) , ranges );
 // a filter of 16 bits of fingerprint is not one of 8
 const auto f = store( BinaryFuseFilter< std::uint64_t >( keys ) );
 CHECK( message( [ & ] {
	 ( void )restore< BinaryFuseFilter< std::uint64_t , std::uint16_t > >(
	   std::span< const std::byte >( f ) );
	 } )
	  .find( "of other parameters" ) != std::string::npos );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- THE PERFECT HASHING --------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-perfect-hash" )
{
 std::mt19937_64 g( 29 );
 std::vector< std::uint64_t > keys( 5000 );
 for( auto & k : keys )
  k = g() >> 4;
 std::sort( keys.begin() , keys.end() );
 keys.erase( std::unique( keys.begin() , keys.end() ) , keys.end() );
 // the same place for each key
 const auto same = [ & ]( const auto & a , const auto & b ) {
  for( const auto k : keys )
   if( a( k ) != b( k ) )
    return( false );
  return( a.size() == b.size() );
  };
 roundTrip< MinimalPerfectHash< std::uint64_t > >(
   MinimalPerfectHash< std::uint64_t >( keys ) , same );
 roundTrip< RecSplit< std::uint64_t > >( RecSplit< std::uint64_t >( keys ) ,
					 same );
 roundTrip< RecSplit< std::uint64_t , 5 > >(
   RecSplit< std::uint64_t , 5 >( keys ) , same );
 roundTrip< FingerprintPerfectHash< std::uint64_t > >(
   FingerprintPerfectHash< std::uint64_t >( keys ) , same );
 roundTrip< OrderedMinimalPerfectHash< std::uint64_t > >(
   OrderedMinimalPerfectHash< std::uint64_t >( keys ) , same );
 std::vector< std::uint64_t > small( keys.begin() , keys.begin() + 500 );
 for( auto & k : small )
  k %= ( std::uint64_t( 1 ) << 60 );
 roundTrip< PerfectHashTable >(
   PerfectHashTable( small ) ,
   [ & ]( const PerfectHashTable & a , const PerfectHashTable & b ) {
    for( std::size_t i = 0 ; i < small.size() ; ++i )
     if( ( a.contains( small[ i ] ) != b.contains( small[ i ] ) ) ||
	 ( a.contains( small[ i ] + 1 ) != b.contains( small[ i ] + 1 ) ) )
      return( false );
    return( a.size() == b.size() );
    } );
 // a RecSplit of leaves of 8 keys is not one of 5
 const auto f = store( RecSplit< std::uint64_t >( keys ) );
 CHECK( message( [ & ] {
	 ( void )restore< RecSplit< std::uint64_t , 5 > >(
	   std::span< const std::byte >( f ) );
	 } )
	  .find( "of other parameters" ) != std::string::npos );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- THE GRAPHS AND THE MATRICES -----------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-graphs" )
{
 std::mt19937_64 g( 41 );
 std::vector< std::pair< int , int > > E;
 std::vector< std::tuple< int , int , double > > W;
 for( int k = 0 ; k < 5000 ; ++k ) {
  const int u = int( g() % 700 ) , v = int( g() % 700 );
  if( u == v )
   continue;  // no loop
  E.emplace_back( u , v );
  W.emplace_back( u , v , double( g() % 1000 ) / 8 );
  }
 // the same rows, heads and weights
 const auto same = []( const auto & a , const auto & b ) {
  if( ( a.n() != b.n() ) || ( a.m() != b.m() ) || ( a.arcs() != b.arcs() ) ||
      ( a.directed() != b.directed() ) )
   return( false );
  if( ! std::equal( a.offsets() , a.offsets() + a.n() + 1 , b.offsets() ) )
   return( false );
  for( int k = 0 ; k < a.arcs() ; ++k )
   if( a.arcHeads()[ k ] != b.arcHeads()[ k ] )
    return( false );
  return( true );
  };
 roundTrip< StaticGraph<> >( StaticGraph<>( 700 , E ) , same );
 roundTrip< StaticGraph<> >( StaticGraph<>( 700 , E , false ) , same );
 roundTrip< StaticGraph<> >( StaticGraph<>() , same );
 const StaticGraph< double > G( 700 , W );
 roundTrip< StaticGraph< double > , StaticGraph<> >(
   G ,
   [ & ]( const StaticGraph< double > & a , const StaticGraph< double > & b ) {
    return( same( a , b ) &&
	     std::equal( a.weightData() , a.weightData() + a.arcs() ,
			 b.weightData() ) );
    } );
 // the three arrays of a mapped graph are views of the file
 TempDirectory d;
 store( G , d.path() / "g.ads" );
 const auto M = restoreMapped< StaticGraph< double > >( d.path() / "g.ads" );
 CHECK( reinterpret_cast< std::uintptr_t >( M.offsets() ) % 64 == 0 &&
	reinterpret_cast< std::uintptr_t >( M.heads() ) % 64 == 0 &&
	reinterpret_cast< std::uintptr_t >( M.weightData() ) % 64 == 0 );
 // the interleaved layout, in the file of the separate one: its round
 // trips, and the file of each layout read into the other
 using P = StaticGraph< double , ArcLayout::Interleaved >;
 const P GI( 700 , W , false );
 const auto sameW = [ & ]( const auto & a , const auto & b ) {
  if( ! same( a , b ) )
   return( false );
  for( int k = 0 ; k < a.arcs() ; ++k )
   if( a.arcWeights()[ k ] != b.arcWeights()[ k ] )
    return( false );
  return( true );
  };
 roundTrip< P >( GI , sameW );
 roundTrip< P >( P() , sameW );
 CHECK( sameW( G , restoreMapped< P >( d.path() / "g.ads" ) ) );
 store( GI , d.path() / "gi.ads" );
 CHECK( sameW( GI ,
	       restore< StaticGraph< double > >( d.path() / "gi.ads" ) ) );
 // the matrices, of each type of index
 std::vector< std::tuple< std::size_t , std::size_t , double > > T;
 for( int k = 0 ; k < 4000 ; ++k )
  T.emplace_back( g() % 300 , g() % 200 , double( k ) - 1000.5 );
 std::vector< double > x( 200 );
 for( std::size_t j = 0 ; j < x.size() ; ++j )
  x[ j ] = double( j ) / 3;
 const auto product = [ & ]( const auto & a , const auto & b ) {
  const std::vector< double > z(
    x.begin() ,
    x.begin() + std::ptrdiff_t( std::min( x.size() , a.cols() ) ) );
  return( a == b &&
	   ( z.size() < a.cols() || a.multiply( z ) == b.multiply( z ) ) );
  };
 roundTrip< CsrMatrix >( CsrMatrix( 300 , 200 , T ) , product );
 roundTrip< CsrMatrix >( CsrMatrix() , product );
 roundTrip< BasicCsrMatrix< double , int > >(
   BasicCsrMatrix< double , int >(
     300 , 200 ,
     std::vector< std::tuple< std::size_t , std::size_t , double > >( T ) ) ,
   product );
 const auto f = store( CsrMatrix( 300 , 200 , T ) );
 CHECK( message( [ & ] {
	 ( void )restore< BasicCsrMatrix< double , int > >(
	   std::span< const std::byte >( f ) );
	 } )
	  .find( "of other parameters" ) != std::string::npos );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- CEREAL -----------------------------------*/
/*--------------------------------------------------------------------------*/

ADS_TEST( "persistence-cereal" )
{
#if defined( ADS_WITH_CEREAL )
 // the structures in an archive of each kind, next to other data
 const BitVector b = randomBits( 9000 , 3 , 31 );
 const RankSelect r( b );
 const auto x = sortedRandom( 2000 , 1000000 , 37 );
 const EliasFano e( x );
 std::vector< std::uint64_t > keys( x.begin() , x.end() );
 keys.erase( std::unique( keys.begin() , keys.end() ) , keys.end() );
 const BinaryFuseFilter< std::uint64_t > f( keys );
 const RecSplit< std::uint64_t > h( keys );
 const auto check = [ & ]( auto out , auto in ) {
  std::stringstream s;
  {
   decltype( out( s ) ) a = out( s );
   const int before = 17;
   a( before , b , r , e , f , h );
   }
  decltype( in( s ) ) a = in( s );
  int before = 0;
  BitVector b2;
  RankSelect r2;
  EliasFano e2;
  BinaryFuseFilter< std::uint64_t > f2;
  RecSplit< std::uint64_t > h2;
  a( before , b2 , r2 , e2 , f2 , h2 );
  CHECK( before == 17 && b2 == b && sameRanks( r2 , r ) && e2.decode() == x );
  for( const auto k : keys )
   CHECK( f2.maybe( k ) && h2( k ) == h( k ) );
  };
 check(
   []( std::ostream & s ) { return( cereal::BinaryOutputArchive( s ) ); } ,
   []( std::istream & s ) { return( cereal::BinaryInputArchive( s ) ); } );
 check(
   []( std::ostream & s ) {
    return( cereal::PortableBinaryOutputArchive( s ) );
    } ,
   []( std::istream & s ) {
    return( cereal::PortableBinaryInputArchive( s ) );
    } );
 // the vector of bits in the layout of sdsl-lite: the width 1 in a byte,
 // the factor 1.5 in a float, the bits in 8 bytes, then the words
 {
  std::stringstream s;
  {
   cereal::BinaryOutputArchive a( s );
   a( b );
   }
  const std::string w = s.str();
  CHECK( w.size() == 1 + 4 + 8 + 8 * ( ( b.size() + 63 ) / 64 ) );
  float g;
  std::uint64_t n;
  std::memcpy( &g , w.data() + 1 , 4 );
  std::memcpy( &n , w.data() + 5 , 8 );
  CHECK( w[ 0 ] == 1 && g == 1.5f && n == b.size() );
  CHECK( std::memcmp( w.data() + 13 , b.words().data() ,
		      w.size() - 13 ) == 0 );
  }
 // a JSON archive, by the names of the members
 {
  std::stringstream s;
  {
   cereal::JSONOutputArchive a( s );
   a( cereal::make_nvp( "bits" , BitVector( "10110" ) ) ,
      cereal::make_nvp( "rs" , r ) );
   }
  CHECK( s.str().find( "\"m_size\": 5" ) != std::string::npos );
  cereal::JSONInputArchive a( s );
  BitVector b3;
  RankSelect r3;
  a( b3 , r3 );
  CHECK( b3 == BitVector( "10110" ) && sameRanks( r3 , r ) );
  }
 // an invariant broken: a bit past the size
 {
  std::stringstream s;
  {
   cereal::BinaryOutputArchive a( s );
   a( b );
   }
  std::string w = s.str();
  w.back() = char( 0xFF );
  std::stringstream t( w );
  cereal::BinaryInputArchive a( t );
  BitVector z;
  CHECK_THROWS( a( z ) , PersistError );
  }
 #if defined( ADS_WITH_SDSL )
 // the vectors of bits of sdsl-lite read by ADS, and the other way
 {
  sdsl::bit_vector v( 1000 , 0 );
  for( std::size_t i = 0 ; i < v.size() ; i += 3 )
   v[ i ] = 1;
  std::stringstream s;
  {
   cereal::BinaryOutputArchive a( s );
   a( v );
   a( b );
   }
  cereal::BinaryInputArchive a( s );
  BitVector u;
  sdsl::bit_vector c;
  a( u , c );
  CHECK( u.size() == v.size() && c.size() == b.size() );
  for( std::size_t i = 0 ; i < v.size() ; ++i )
   CHECK( u[ i ] == bool( v[ i ] ) );
  for( std::size_t i = 0 ; i < b.size() ; ++i )
   CHECK( bool( c[ i ] ) == b[ i ] );
  }
 #endif
#endif
 }

//@MORETESTS@

/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------- End File engineered_persistence.cpp ------------------*/
/*--------------------------------------------------------------------------*/
