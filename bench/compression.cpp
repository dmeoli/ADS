/*--------------------------------------------------------------------------*/
/*-------------------------- File compression.cpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the compressors and of the codes: the coders of ADS
 * against, when they are found, huff0 and FSE of Collet (the entropy
 * coders of Zstandard, from FiniteStateEntropy), Stream VByte of Lemire,
 * the codes of Elias of sdsl-lite, libsais of Grebnov, the csa_wt of
 * sdsl-lite and LZ4:
 *
 * - entropy: the encoding and the decoding of n bytes of a text and of a
 *   skewed source (bytes of probability about 2^-k), by the code of
 *   Huffman (the canonical one of the notes, a bit at a time, and the
 *   engineered one), rANS, tANS and the arithmetic code of the notes;
 *   huff0 and FSE code blocks of 128 KB, as Zstandard does;
 * - integers: the encoding and the decoding of n integers of 32 bits of a
 *   geometric distribution (the gaps of a posting list) by the
 *   variable-byte code, by the binary packing and its patched form (PFor),
 *   against the SIMD-BP128 and the SIMD-FastPFor of FastPFor, and by the
 *   codes gamma and delta of Elias, and of their prefix sums by the
 *   packings of the differences at a distance of 4;
 * - bwt: the transform of Burrows and Wheeler of n bytes of the text;
 * - rindex: count() and locate() of patterns of 8 bytes of a repetitive
 *   text (copies of n / 100 bytes of the text, 1 byte in 1000 changed) by
 *   the r-index and by the r-index on move structures, against the
 *   FM-index of Huffman, the r-index of Prezza and the csa_wt of
 *   sdsl-lite;
 * - fmindex: count() and locate() of patterns of 8 bytes of the text, by
 *   the FM-index of the notes and by the one on the wavelet tree of
 *   Huffman (on RankSelect and on CompactRankSelect), and
 *   the building, with the sdsl index on a wavelet tree shaped by Huffman,
 *   both sampling one suffix in 32;
 * - lz: the parse of LZSS by the chains of gzip against the compression
 *   and the decompression of LZ4, a point of reference rather than a match,
 *   and the parse of LZ77 with no window in linear time;
 * - space: the bits per byte (per integer, per byte of the text for the
 *   index), in the column of the time, so that the ratios of
 *   bench/summary.py read the same way (above 1 when ADS is smaller).
 *
 * The rANS of Giesen (ryg_rans) is driven as in its example, with two
 * states and bytes of renormalization.
 *
 * The text is the file named by the variable ADS_BENCH_TEXT, repeated up to
 * n bytes (the book1 of the Calgary corpus in the results), and else words
 * drawn from a Zipf law. Each cell is the median of r runs, in nanoseconds
 * per byte (per integer, per pattern, per occurrence); the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   compression [ -r runs ] [ -s seed ] n1 n2 ...
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------------- INCLUDES --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef ADS_CONTRACTS
 #define ADS_CONTRACTS 0
#endif

#include "Bench.h"

#include "ADS/compression/ArithmeticCoding.h"
#include "ADS/compression/BurrowsWheeler.h"
#include "ADS/compression/CanonicalHuffman.h"
#include "ADS/compression/FMIndex.h"
#include "ADS/compression/IntegerCodes.h"
#include "ADS/compression/LempelZiv.h"
#if __has_include( "ADS/compression/RIndex.h" )
 #include "ADS/compression/RIndex.h"
 #define ADS_HAS_RINDEX 1
#endif
#if __has_include( "ADS/compression/MoveRIndex.h" )
 #include "ADS/compression/MoveRIndex.h"
 #define ADS_HAS_MOVER 1
#endif
#if __has_include( <r_index.hpp> ) && __has_include( <sdsl/csa_wt.hpp> )
 #include <r_index.hpp>
 #define ADS_HAS_PREZZA 1
#endif
#if __has_include( "ADS/compression/HuffmanFMIndex.h" )
 #include "ADS/compression/HuffmanFMIndex.h"
 #define ADS_HAS_HFM 1
#endif
#if __has_include( "ADS/compression/LZ77Linear.h" )
 #include "ADS/compression/LZ77Linear.h"
 #define ADS_HAS_LZLIN 1
#endif

#if __has_include( "ADS/compression/HuffmanCoder.h" )
 #include "ADS/compression/HuffmanCoder.h"
 #include "ADS/compression/Rans.h"
 #include "ADS/compression/StreamVByte.h"
 #define ADS_HAS_CODERS 1
#endif
#if __has_include( "ADS/compression/BinaryPacking.h" )
 #include "ADS/compression/BinaryPacking.h"
 #define ADS_HAS_BP 1
#endif
#if __has_include( "ADS/compression/Tans.h" )
 #include "ADS/compression/Tans.h"
 #define ADS_HAS_TANS 1
#endif

#if __has_include( <fse/fse.h> )
 #include <fse/fse.h>
 #include <fse/huf.h>
 #define ADS_HAS_FSE 1
#endif

#if __has_include( <fastpfor/codecfactory.h> )
 #include <fastpfor/codecfactory.h>
 #include <emmintrin.h>
 #define ADS_HAS_FASTPFOR 1
#endif

#if __has_include( <streamvbyte.h> )
 #include <streamvbyte.h>
 #define ADS_HAS_SVB 1
#endif

#if __has_include( <sdsl/coder_elias_gamma.hpp> )
 #include <sdsl/coder_elias_delta.hpp>
 #include <sdsl/coder_elias_gamma.hpp>
 #include <sdsl/csa_wt.hpp>
 #include <sdsl/suffix_arrays.hpp>
 #define ADS_HAS_SDSL 1
#endif

#if __has_include( <libsais.h> )
 #include <libsais.h>
 #define ADS_HAS_SAIS 1
#endif

#if __has_include( <rans_byte.h> )
 #include <rans_byte.h>
 #define ADS_HAS_RYG 1
#endif

#if __has_include( <lz4.h> )
 #include <lz4.h>
 #define ADS_HAS_LZ4 1
#endif

#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <sstream>

using namespace ADS::bench;

namespace {

using Bytes = std::vector< std::uint8_t >;

/*--------------------------------------------------------------------------*/
/*--------------------------------- INPUTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// n bytes of the text of ADS_BENCH_TEXT repeated, else words of a Zipf
/// law of 5000 words of 2 to 9 letters
Bytes text( std::size_t n , unsigned seed )
{
 Bytes t;
 if( const char * f = std::getenv( "ADS_BENCH_TEXT" ) ) {
  std::ifstream in( f , std::ios::binary );
  std::stringstream ss;
  ss << in.rdbuf();
  const std::string s = ss.str();
  while( ( t.size() < n ) && ! s.empty() )
   t.insert( t.end() , s.begin() , s.begin() + std::ptrdiff_t(
             std::min( s.size() , n - t.size() ) ) );
  if( t.size() == n ) return( t );
  t.clear();
  }
 std::mt19937_64 g( seed );
 std::vector< std::string > words( 5000 );
 for( auto & w : words ) {
  const std::size_t len = 2 + g() % 8;
  for( std::size_t i = 0 ; i < len ; ++i ) w += char( 'a' + g() % 26 );
  }
 std::vector< double > cum( words.size() );
 double s = 0;
 for( std::size_t i = 0 ; i < words.size() ; ++i )
  cum[ i ] = s += 1.0 / double( i + 1 );
 std::uniform_real_distribution< double > U( 0 , s );
 while( t.size() < n ) {
  const auto & w = words[ std::size_t( std::lower_bound( cum.begin() ,
                                                         cum.end() ,
                                                         U( g ) ) -
                                       cum.begin() ) ];
  t.insert( t.end() , w.begin() , w.end() );
  t.push_back( g() % 12 == 0 ? '\n' : ' ' );
  }
 t.resize( n );
 return( t );
 }

/// n bytes of probability about 2^-k for the byte k
Bytes skewed( std::size_t n , unsigned seed )
{
 std::mt19937_64 g( seed );
 Bytes t( n );
 for( auto & c : t )
  c = std::uint8_t( std::countr_zero( g() | ( 1ULL << 40 ) ) );
 return( t );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- ENTROPY --------------------------------*/
/*--------------------------------------------------------------------------*/

/// the cells of a coder of bytes: encode( in ) gives the code, decode(
/// code , n ) the bytes back
template< class E , class D >
void coder( const std::string & what , const Bytes & in ,
            const std::string & who , E encode , D decode , int r )
{
 if( skip( who ) )
  return;
 const std::size_t n = in.size();
 Bytes code = encode( in );
 if( decode( code , n ) != in ) {
  std::fprintf( stderr , "%s: the round trip fails\n" , who.c_str() );
  return;
  }
 print( "entropy" , "encode " + what , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          sink = sink + encode( in ).size(); } , r , n ) );
 print( "entropy" , "decode " + what , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          sink = sink + decode( code , n ).size(); } , r , n ) );
 print( "space" , "entropy " + what , n , who ,
        8 * double( code.size() ) / double( n ) );
 }

#ifdef ADS_HAS_FSE
/// the blocks of 128 KB of in by f( dst , cap , src , n ), each one after
/// its size in 4 bytes and a mark: 0 coded, 1 one byte repeated, 2 raw
template< class F >
Bytes blocks( const Bytes & in , F f )
{
 Bytes out;
 const std::size_t B = 128 * 1024;
 Bytes buf( HUF_compressBound( B ) + FSE_compressBound( B ) );
 for( std::size_t a = 0 ; a < in.size() ; a += B ) {
  const std::size_t len = std::min( B , in.size() - a );
  std::size_t c = f( buf.data() , buf.size() , in.data() + a , len );
  std::uint8_t mark = 0;
  if( FSE_isError( c ) || ( c == 0 ) ) {    // not compressible: raw
   mark = 2;
   std::copy( in.begin() + std::ptrdiff_t( a ) ,
              in.begin() + std::ptrdiff_t( a + len ) , buf.begin() );
   c = len;
   }
  else if( c == 1 ) mark = 1;               // one byte repeated
  const auto at = out.size();
  out.resize( at + 5 );
  std::memcpy( out.data() + at , &c , 4 );
  out[ at + 4 ] = mark;
  out.insert( out.end() , buf.begin() , buf.begin() + std::ptrdiff_t( c ) );
  }
 return( out );
 }

/// the n bytes of the blocks of code by g( dst , len , src , c )
template< class G >
Bytes unblocks( const Bytes & code , std::size_t n , G g )
{
 Bytes out( n );
 const std::size_t B = 128 * 1024;
 std::size_t p = 0;
 for( std::size_t a = 0 ; a < n ; a += B ) {
  const std::size_t len = std::min( B , n - a );
  std::uint32_t c;
  std::memcpy( &c , code.data() + p , 4 );
  const std::uint8_t mark = code[ p + 4 ];
  p += 5;
  if( mark == 2 ) std::memcpy( out.data() + a , code.data() + p , len );
  else if( mark == 1 ) std::memset( out.data() + a , code[ p ] , len );
  else g( out.data() + a , len , code.data() + p , c );
  p += c;
  }
 return( out );
 }
#endif

void entropyCoders( std::size_t n , int r , unsigned seed )
{
 for( const auto & [ what , in ] : { std::pair( std::string( "text" ) ,
                                                text( n , seed ) ) ,
                                     std::pair( std::string( "skewed" ) ,
                                                skewed( n , seed ) ) } ) {
#ifdef ADS_HAS_CODERS
  for( const char * who : { "ADS::huffmanCompress" ,
                            "ADS::huffmanCompress A/A" } )
   coder( what , in , who , []( const Bytes & s ) {
           return( ADS::huffmanCompress( s ) ); } ,
          []( const Bytes & c , std::size_t ) {
           return( ADS::huffmanDecompress( c ) ); } , r );
  coder( what , in , "ADS::ransCompress" , []( const Bytes & s ) {
          return( ADS::ransCompress( s ) ); } ,
         []( const Bytes & c , std::size_t ) {
          return( ADS::ransDecompress( c ) ); } , r );
#endif
#ifdef ADS_HAS_TANS
  coder( what , in , "ADS::tansCompress" , []( const Bytes & s ) {
          return( ADS::tansCompress( s ) ); } ,
         []( const Bytes & c , std::size_t ) {
          return( ADS::tansDecompress( c ) ); } , r );
#endif
  if( n <= 1000000 ) {                      // the coders of the notes
   std::map< std::uint8_t , std::uint64_t > freq;
   for( const auto c : in ) ++freq[ c ];
   const auto H = ADS::CanonicalHuffman< std::uint8_t >::fromFrequencies(
                   freq );
   coder( what , in , "ADS::CanonicalHuffman" , [ & ]( const Bytes & s ) {
           return( H.encode( std::span< const std::uint8_t >( s ) ) ); } ,
          [ & ]( const Bytes & c , std::size_t n ) {
           return( H.decode( c , n ) ); } , r );
   coder( what , in , "ADS::arithmeticEncode" , [ & ]( const Bytes & s ) {
           return( ADS::arithmeticEncode(
                    std::span< const std::uint8_t >( s ) , freq ) ); } ,
          [ & ]( const Bytes & c , std::size_t n ) {
           return( ADS::arithmeticDecode< std::uint8_t >( c , n ,
                                                          freq ) ); } , r );
   }
#if defined( ADS_HAS_RYG ) && defined( ADS_HAS_CODERS )
  // the rANS of Giesen with bytes of renormalization and two states, on
  // the frequencies scaled as those of ADS, as in its example
  coder( what , in , "ryg_rans" , []( const Bytes & s ) {
          std::array< std::uint64_t , 256 > h{};
          for( const auto c : s ) ++h[ c ];
          const auto f = ADS::scaleFrequencies( h , 12 );
          RansEncSymbol E[ 256 ];
          std::uint32_t cum = 0;
          for( int c = 0 ; c < 256 ; ++c ) {
           if( f[ c ] ) RansEncSymbolInit( &E[ c ] , cum , f[ c ] , 12 );
           cum += f[ c ];
           }
          Bytes buf( s.size() + 64 );
          std::uint8_t * p = buf.data() + buf.size();
          RansState r0 , r1;
          RansEncInit( &r0 );
          RansEncInit( &r1 );
          const std::size_t n = s.size();
          if( n & 1 ) RansEncPutSymbol( &r0 , &p , &E[ s[ n - 1 ] ] );
          for( std::size_t i = n & ~std::size_t( 1 ) ; i > 0 ; i -= 2 ) {
           RansEncPutSymbol( &r1 , &p , &E[ s[ i - 1 ] ] );
           RansEncPutSymbol( &r0 , &p , &E[ s[ i - 2 ] ] );
           }
          RansEncFlush( &r1 , &p );
          RansEncFlush( &r0 , &p );
          Bytes out( 512 );                 // the frequencies, then the code
          for( int c = 0 ; c < 256 ; ++c ) {
           out[ 2 * c ] = std::uint8_t( f[ c ] );
           out[ 2 * c + 1 ] = std::uint8_t( f[ c ] >> 8 );
           }
          out.insert( out.end() , p , buf.data() + buf.size() );
          return( out ); } ,
         []( const Bytes & c , std::size_t n ) {
          RansDecSymbol D[ 256 ];
          std::uint8_t cum2sym[ 1 << 12 ];
          std::uint32_t cum = 0;
          for( int s = 0 ; s < 256 ; ++s ) {
           const std::uint32_t f = c[ 2 * s ] | ( c[ 2 * s + 1 ] << 8 );
           RansDecSymbolInit( &D[ s ] , cum , f );
           for( std::uint32_t j = 0 ; j < f ; ++j )
            cum2sym[ cum + j ] = std::uint8_t( s );
           cum += f;
           }
          std::uint8_t * p = const_cast< std::uint8_t * >( c.data() ) + 512;
          RansState r0 , r1;
          RansDecInit( &r0 , &p );
          RansDecInit( &r1 , &p );
          Bytes out( n );
          std::size_t i = 0;
          for( ; i + 2 <= n ; i += 2 ) {
           const std::uint8_t s0 = cum2sym[ RansDecGet( &r0 , 12 ) ] ,
                              s1 = cum2sym[ RansDecGet( &r1 , 12 ) ];
           out[ i ] = s0;
           out[ i + 1 ] = s1;
           RansDecAdvanceSymbolStep( &r0 , &D[ s0 ] , 12 );
           RansDecAdvanceSymbolStep( &r1 , &D[ s1 ] , 12 );
           RansDecRenorm( &r0 , &p );
           RansDecRenorm( &r1 , &p );
           }
          if( i < n ) {
           const std::uint8_t s0 = cum2sym[ RansDecGet( &r0 , 12 ) ];
           out[ i ] = s0;
           RansDecAdvanceSymbol( &r0 , &p , &D[ s0 ] , 12 );
           }
          return( out ); } , r );
#endif
#ifdef ADS_HAS_FSE
  coder( what , in , "huff0" , []( const Bytes & s ) {
          return( blocks( s , []( void * d , std::size_t cap ,
                                  const void * src , std::size_t len ) {
           return( HUF_compress( d , cap , src , len ) ); } ) ); } ,
         []( const Bytes & c , std::size_t n ) {
          return( unblocks( c , n , []( void * d , std::size_t len ,
                                        const void * src , std::size_t cs ) {
           return( HUF_decompress( d , len , src , cs ) ); } ) ); } , r );
  coder( what , in , "FSE" , []( const Bytes & s ) {
          return( blocks( s , []( void * d , std::size_t cap ,
                                  const void * src , std::size_t len ) {
           return( FSE_compress( d , cap , src , len ) ); } ) ); } ,
         []( const Bytes & c , std::size_t n ) {
          return( unblocks( c , n , []( void * d , std::size_t len ,
                                        const void * src , std::size_t cs ) {
           return( FSE_decompress( d , len , src , cs ) ); } ) ); } , r );
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- INTEGERS -------------------------------*/
/*--------------------------------------------------------------------------*/

/// the bits of a code
template< class C >
std::size_t bitsOf( const C & c )
{
 return( 8 * c.size() * sizeof( c[ 0 ] ) );
 }

#ifdef ADS_HAS_SDSL
std::size_t bitsOf( const sdsl::int_vector<> & z )
{
 return( z.bit_size() );
 }
#endif

/// the cells of a code of integers: encode( x ) gives the code, decode(
/// code , n ) the integers back, bits( code ) its bits
template< class E , class D >
void integers( const std::string & what ,
               const std::vector< std::uint32_t > & x ,
               const std::string & who , E encode , D decode , int r )
{
 if( skip( who ) )
  return;
 const std::size_t n = x.size();
 auto code = encode( x );
 if( decode( code , n ) != x ) {
  std::fprintf( stderr , "%s: the round trip fails\n" , who.c_str() );
  return;
  }
 print( "integers" , "encode " + what , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          sink = sink + std::uint64_t( encode( x ).size() ); } , r , n ) );
 print( "integers" , "decode " + what , n , who , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          sink = sink + decode( code , n ).size(); } , r , n ) );
 print( "space" , "integers " + what , n , who ,
        double( bitsOf( code ) ) / double( n ) );
 }

void integerCodes( std::size_t n , int r , unsigned seed )
{
 std::mt19937_64 g( seed );
 std::geometric_distribution< std::uint32_t > G( 1.0 / 64 );
 std::vector< std::uint32_t > x( n );
 for( auto & v : x ) v = 1 + G( g );        // positive, for gamma and delta
 using V = std::vector< std::uint32_t >;
 for( const char * who : { "ADS::encodeVByte" , "ADS::encodeVByte A/A" } )
  integers( "vbyte" , x , who , []( const V & x ) {
             Bytes out;
             out.reserve( x.size() * 2 );
             for( const auto v : x ) ADS::encodeVByte( out , v );
             return( out ); } ,
            []( const Bytes & c , std::size_t n ) {
             V x( n );
             std::size_t p = 0;
             for( auto & v : x )
              v = std::uint32_t( ADS::decodeVByte( c , p ) );
             return( x ); } , r );
#ifdef ADS_HAS_CODERS
 integers( "vbyte" , x , "ADS::streamVByteEncode" , []( const V & x ) {
            return( ADS::streamVByteEncode( x ) ); } ,
           []( const Bytes & c , std::size_t n ) {
            return( ADS::streamVByteDecode( c , n ) ); } , r );
#endif
#ifdef ADS_HAS_SVB
 integers( "vbyte" , x , "streamvbyte" , []( const V & x ) {
            Bytes out( x.size() * 5 + 32 );
            out.resize( streamvbyte_encode( x.data() ,
                                            std::uint32_t( x.size() ) ,
                                            out.data() ) );
            return( out ); } ,
           []( const Bytes & c , std::size_t n ) {
            V x( n );
            streamvbyte_decode( c.data() , x.data() , std::uint32_t( n ) );
            return( x ); } , r );
#endif
 // the binary packing, of the integers and of their prefix sums
 V sx( x.size() );
 std::uint32_t acc = 0;
 for( std::size_t i = 0 ; i < x.size() ; ++i ) sx[ i ] = acc += x[ i ];
#ifdef ADS_HAS_BP
 for( const bool sorted : { false , true } )
  integers( sorted ? "sorted" : "packed" , sorted ? sx : x ,
            "ADS::binaryPack" , [ sorted ]( const V & x ) {
             return( ADS::binaryPack( x , sorted ) ); } ,
            [ sorted ]( const Bytes & c , std::size_t n ) {
             return( ADS::binaryUnpack( c , n , sorted ) ); } , r );
 for( const bool sorted : { false , true } )
  integers( sorted ? "sorted" : "packed" , sorted ? sx : x ,
            "ADS::patchedPack" , [ sorted ]( const V & x ) {
             return( ADS::patchedPack( x , sorted ) ); } ,
            [ sorted ]( const Bytes & c , std::size_t n ) {
             return( ADS::patchedUnpack( c , n , sorted ) ); } , r );
#endif
#ifdef ADS_HAS_FASTPFOR
 // the codecs of FastPFor, their last integers by variable bytes; the
 // sorted ones by the differences at a distance of 4 taken and summed 4
 // at a time by SSE2, as the library does
 auto d4 = []( std::uint32_t * p , std::size_t m ) {
  for( std::size_t i = m ; ( i-- > 4 ) && ( i >= m / 4 * 4 ) ; )
   p[ i ] -= p[ i - 4 ];
  for( std::size_t q = m / 4 ; q-- > 1 ; ) {
   auto * a = reinterpret_cast< __m128i * >( p ) + q;
   _mm_storeu_si128( a , _mm_sub_epi32( _mm_loadu_si128( a ) ,
                                        _mm_loadu_si128( a - 1 ) ) );
   }
  };
 auto i4 = []( std::uint32_t * p , std::size_t m ) {
  for( std::size_t q = 1 ; q < m / 4 ; ++q ) {
   auto * a = reinterpret_cast< __m128i * >( p ) + q;
   _mm_storeu_si128( a , _mm_add_epi32( _mm_loadu_si128( a ) ,
                                        _mm_loadu_si128( a - 1 ) ) );
   }
  for( std::size_t i = std::max< std::size_t >( m / 4 * 4 , 4 ) ; i < m ;
       ++i ) p[ i ] += p[ i - 4 ];
  };
 for( const char * name : { "simdbinarypacking" , "simdfastpfor128" } )
  for( const bool sorted : { false , true } ) {
   if( skip( std::string( "FastPFor::" ) + name ) )
    continue;
   FastPForLib::CODECFactory F;
   auto codec = F.getFromName( name );
   integers( sorted ? "sorted" : "packed" , sorted ? sx : x ,
             std::string( "FastPFor::" ) + name ,
             [ & , sorted ]( const V & x ) {
              V in( x ) , out( x.size() + 1024 );
              if( sorted ) d4( in.data() , in.size() );
              std::size_t m = out.size();
              codec->encodeArray( in.data() , in.size() , out.data() , m );
              out.resize( m );
              return( out ); } ,
             [ & , sorted ]( const V & c , std::size_t n ) {
              V y( n + 1024 );
              std::size_t m = y.size();
              codec->decodeArray( c.data() , c.size() , y.data() , m );
              y.resize( n );
              if( sorted ) i4( y.data() , n );
              return( y ); } , r );
   }
#endif
 for( const bool delta : { false , true } ) {
  const std::string what = delta ? "delta" : "gamma";
  integers( what , x , delta ? "ADS::encodeDelta" : "ADS::encodeGamma" ,
            [ delta ]( const V & x ) {
             ADS::BitWriter w;
             for( const auto v : x )
              if( delta ) ADS::encodeDelta( w , v );
              else ADS::encodeGamma( w , v );
             const auto & W = w.bits().words();
             return( std::vector< std::uint64_t >( W.begin() , W.end() ) ); } ,
            [ delta ]( const std::vector< std::uint64_t > & c ,
                       std::size_t n ) {
             ADS::BitVector b;
             for( const auto v : c ) b.append( v , 64 );
             ADS::BitReader rd( b );
             V x( n );
             for( auto & v : x )
              v = std::uint32_t( delta ? ADS::decodeDelta( rd )
                                       : ADS::decodeGamma( rd ) );
             return( x ); } , r );
#ifdef ADS_HAS_SDSL
  auto sdslCode = [ delta ]( const V & x ) {
   sdsl::int_vector<> v( x.size() , 0 , 32 ) , z;
   for( std::size_t i = 0 ; i < x.size() ; ++i ) v[ i ] = x[ i ];
   if( delta ) sdsl::coder::elias_delta<>::encode( v , z );
   else sdsl::coder::elias_gamma<>::encode( v , z );
   return( z ); };
  integers( what , x , delta ? "sdsl::elias_delta" : "sdsl::elias_gamma" ,
            sdslCode , [ delta ]( const sdsl::int_vector<> & z ,
                                  std::size_t n ) {
             sdsl::int_vector<> y;
             if( delta ) sdsl::coder::elias_delta<>::decode( z , y );
             else sdsl::coder::elias_gamma<>::decode( z , y );
             V x( n );
             for( std::size_t i = 0 ; i < n ; ++i )
              x[ i ] = std::uint32_t( y[ i ] );
             return( x ); } , r );
#endif
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ BWT AND INDEX -----------------------------*/
/*--------------------------------------------------------------------------*/

void transforms( std::size_t n , int r , unsigned seed )
{
 const Bytes t = text( n , seed );
 // the bytes 0 made 1, since sdsl-lite keeps the 0 for the end of the
 // text (book1 has one) and would index only the text before it
 std::string s( t.begin() , t.end() );
 std::replace( s.begin() , s.end() , '\0' , '\1' );
 for( const char * who : { "ADS::bwt" , "ADS::bwt A/A" } )
  if( ! skip( who ) )
   print( "bwt" , "text" , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) { sink = sink + ADS::bwt( s ).primary; } , r ,
           n ) );
#ifdef ADS_HAS_SAIS
 if( ! skip( "libsais" ) )
  print( "bwt" , "text" , n , "libsais" , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           Bytes U( n );
           std::vector< std::int32_t > A( n );
           sink = sink + std::uint64_t( libsais_bwt( t.data() , U.data() ,
                                                     A.data() ,
                                                     std::int32_t( n ) , 0 ,
                                                     nullptr ) ); } , r ,
          n ) );
#endif
 // the index: patterns of 8 bytes at random places of the text
 std::mt19937_64 g( seed );
 std::vector< std::string > P( 2000 );
 for( auto & p : P ) p = s.substr( g() % ( n - 8 ) , 8 );
 std::size_t occ = 0;
 {
 const ADS::FMIndex F( s , 64 , 32 );
 for( const auto & p : P ) occ += F.count( p );
 }
 auto cells = [ & ]( const std::string & who , auto build , auto count ,
                     auto locate , auto bits ) {
  if( skip( who ) )
   return;
  print( "fmindex" , "build" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) { sink = sink + bits( *build() ); } , r , n ) );
  const auto F = build();
  print( "fmindex" , "count" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::size_t c = 0;
           for( const auto & p : P ) c += count( *F , p );
           sink = sink + c; } , r , P.size() ) );
  const std::size_t few = std::min< std::size_t >( P.size() , 200 );
  print( "fmindex" , "locate" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::size_t c = 0;
           for( std::size_t i = 0 ; i < few ; ++i ) c += locate( *F , P[ i ] );
           sink = sink + c; } , r , std::max< std::size_t >( occ * few /
                                                            P.size() , 1 ) ) );
  print( "space" , "fmindex" , n , who , double( bits( *F ) ) / double( n ) );
  };
 for( const char * who : { "ADS::FMIndex" , "ADS::FMIndex A/A" } )
  cells( who , [ & ]() {
          return( std::make_unique< ADS::FMIndex >( s , 64 , 32 ) ); } ,
         []( const ADS::FMIndex & F , const std::string & p ) {
          return( F.count( p ) ); } ,
         []( const ADS::FMIndex & F , const std::string & p ) {
          return( F.locate( p ).size() ); } ,
         []( const auto & F ) {
          if constexpr( requires { F.sizeInBits(); } )
           return( std::size_t( F.sizeInBits() ) );
          else return( std::size_t( 0 ) ); } );
#ifdef ADS_HAS_HFM
 auto huffman = [ & ]< class RS >( const char * who , RS * ) {
  using H = ADS::HuffmanFMIndex< RS >;
  cells( who , [ & ]() { return( std::make_unique< H >( s , 32 ) ); } ,
         []( const H & F , const std::string & p ) {
          return( F.count( p ) ); } ,
         []( const H & F , const std::string & p ) {
          return( F.locate( p ).size() ); } ,
         []( const H & F ) { return( F.sizeInBits() ); } );
  };
 huffman( "ADS::HuffmanFMIndex" , ( ADS::RankSelect * )nullptr );
 huffman( "ADS::HuffmanFMIndex compact" ,
          ( ADS::CompactRankSelect * )nullptr );
#endif
#ifdef ADS_HAS_SDSL
 using CSA = sdsl::csa_wt< sdsl::wt_huff<> , 32 , 32 >;
 cells( "sdsl::csa_wt<wt_huff>" , [ & ]() {
         auto F = std::make_unique< CSA >();
         sdsl::construct_im( *F , s.c_str() , 1 );
         return( F ); } ,
        []( const CSA & F , const std::string & p ) {
         return( sdsl::count( F , p.begin() , p.end() ) ); } ,
        []( const CSA & F , const std::string & p ) {
         return( sdsl::locate( F , p.begin() , p.end() ).size() ); } ,
        []( const CSA & F ) {
         return( 8 * std::size_t( sdsl::size_in_bytes( F ) ) ); } );
#endif
 }

/*--------------------------------------------------------------------------*/
/*----------------------------------- LZ -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// a repetitive text of n bytes: copies of the first n / 100 bytes of the
/// text (1000 at least) with 1 byte in 1000 changed, its bytes 0 and 1
/// made 2 (the r-index of Prezza keeps them)
std::string repetitive( std::size_t n , unsigned seed )
{
 const Bytes t = text( std::max< std::size_t >( n / 100 , 1000 ) , seed );
 std::string base( t.begin() , t.end() ) , s;
 std::mt19937_64 g( seed );
 while( s.size() < n ) {
  std::string copy = base;
  for( auto & c : copy )
   if( g() % 1000 == 0 ) c = static_cast< char >( 'a' + g() % 26 );
  s += copy.substr( 0 , n - s.size() );
  }
 for( auto & c : s )
  if( static_cast< unsigned char >( c ) < 2 ) c = '\2';
 return( s );
 }

void rIndexes( std::size_t n , int r , unsigned seed )
{
 const std::string s = repetitive( n , seed );
 std::mt19937_64 g( seed );
 std::vector< std::string > P( 2000 );
 for( auto & p : P ) p = s.substr( g() % ( n - 8 ) , 8 );
 std::size_t occ = 0;
 {
 const ADS::HuffmanFMIndex<> F( s , 32 );
 for( std::size_t i = 0 ; i < 200 ; ++i ) occ += F.count( P[ i ] );
 }
 auto cells = [ & ]( const std::string & who , auto build , auto count ,
                     auto locate , auto bits ) {
  if( skip( who ) )
   return;
  print( "rindex" , "build" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) { sink = sink + bits( *build() ); } , r , n ) );
  const auto F = build();
  print( "rindex" , "count" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::size_t c = 0;
           for( const auto & p : P ) c += count( *F , p );
           sink = sink + c; } , r , P.size() ) );
  print( "rindex" , "locate" , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           std::size_t c = 0;
           for( std::size_t i = 0 ; i < 200 ; ++i ) c += locate( *F , P[ i ] );
           sink = sink + c; } , r , std::max< std::size_t >( occ , 1 ) ) );
  print( "space" , "rindex" , n , who , double( bits( *F ) ) / double( n ) );
  };
#ifdef ADS_HAS_RINDEX
 // the first structure, run again as "A/A" for the noise of the table
 for( const char * who : { "ADS::RIndex" , "ADS::RIndex A/A" } )
  cells( who , [ & ]() { return( std::make_unique< ADS::RIndex >( s ) ); } ,
         []( const ADS::RIndex & F , const std::string & p ) {
          return( F.count( p ) ); } ,
         []( const ADS::RIndex & F , const std::string & p ) {
          return( F.locate( p ).size() ); } ,
         []( const ADS::RIndex & F ) { return( F.sizeInBits() ); } );
#endif
#ifdef ADS_HAS_MOVER
 cells( "ADS::MoveRIndex" , [ & ]() {
         return( std::make_unique< ADS::MoveRIndex >( s ) ); } ,
        []( const ADS::MoveRIndex & F , const std::string & p ) {
         return( F.count( p ) ); } ,
        []( const ADS::MoveRIndex & F , const std::string & p ) {
         return( F.locate( p ).size() ); } ,
        []( const ADS::MoveRIndex & F ) { return( F.sizeInBits() ); } );
#endif
#ifdef ADS_HAS_HFM
 using H = ADS::HuffmanFMIndex<>;
 cells( "ADS::HuffmanFMIndex" , [ & ]() {
         return( std::make_unique< H >( s , 32 ) ); } ,
        []( const H & F , const std::string & p ) { return( F.count( p ) ); } ,
        []( const H & F , const std::string & p ) {
         return( F.locate( p ).size() ); } ,
        []( const H & F ) { return( F.sizeInBits() ); } );
#endif
#ifdef ADS_HAS_PREZZA
 // its building writes its steps on the standard output, silenced
 using RI = ri::r_index<>;
 cells( "ri::r_index" , [ & ]() {
         std::string t = s;
         std::ofstream null( "/dev/null" );
         auto * old = std::cout.rdbuf( null.rdbuf() );
         auto R = std::make_unique< RI >( t , true );
         std::cout.rdbuf( old );
         return( R ); } ,
        []( const RI & F , const std::string & p ) {
         std::string q = p;
         const auto [ a , b ] = const_cast< RI & >( F ).count( q );
         return( b >= a ? std::size_t( b - a + 1 ) : std::size_t( 0 ) ); } ,
        []( const RI & F , const std::string & p ) {
         std::string q = p;
         return( const_cast< RI & >( F ).locate_all( q ).size() ); } ,
        []( const RI & F ) {
         std::ofstream null( "/dev/null" );
         return( 8 * std::size_t( const_cast< RI & >( F ).serialize(
                                   null ) ) ); } );
#endif
#ifdef ADS_HAS_SDSL
 using CSA = sdsl::csa_wt< sdsl::wt_huff<> , 32 , 32 >;
 cells( "sdsl::csa_wt<wt_huff>" , [ & ]() {
         auto F = std::make_unique< CSA >();
         sdsl::construct_im( *F , s.c_str() , 1 );
         return( F ); } ,
        []( const CSA & F , const std::string & p ) {
         return( sdsl::count( F , p.begin() , p.end() ) ); } ,
        []( const CSA & F , const std::string & p ) {
         return( sdsl::locate( F , p.begin() , p.end() ).size() ); } ,
        []( const CSA & F ) {
         return( 8 * std::size_t( sdsl::size_in_bytes( F ) ) ); } );
#endif
 }

void lempelZiv( std::size_t n , int r , unsigned seed )
{
 const Bytes t = text( n , seed );
 const std::string s( t.begin() , t.end() );
 for( const char * who : { "ADS::lzssParseHashed" ,
                           "ADS::lzssParseHashed A/A" } )
  if( ! skip( who ) )
   print( "lz" , "parse" , n , who , median( [](){ return( 0 ); } ,
           [ & ]( int ) {
            sink = sink + ADS::lzssParseHashed( s , 65536 , 4 ,
                                                16 ).size(); } , r , n ) );
#ifdef ADS_HAS_LZLIN
 if( ! skip( "ADS::lz77ParseLinear" ) )
  print( "lz" , "parse unbounded" , n , "ADS::lz77ParseLinear" , median(
          [](){ return( 0 ); } , [ & ]( int ) {
           sink = sink + ADS::lz77ParseLinear( s ).size(); } , r , n ) );
#endif
 if( ! skip( "ADS::lzssDecode" ) ) {
  const auto Z = ADS::lzssParseHashed( s , 65536 , 4 , 16 );
  print( "lz" , "decode" , n , "ADS::lzssDecode" , median(
          [](){ return( 0 ); } , [ & ]( int ) {
           sink = sink + ADS::lzssDecode( Z ).size(); } , r , n ) );
  }
#ifdef ADS_HAS_LZ4
 // the decoding needs the code of the encoding
 const bool lz4c = ! skip( "LZ4_compress_default" ) ,
            lz4d = ! skip( "LZ4_decompress_safe" );
 if( ! lz4c && ! lz4d )
  return;
 std::string c( std::size_t( LZ4_compressBound( int( n ) ) ) , '\0' );
 if( lz4c )
  print( "lz" , "parse" , n , "LZ4_compress_default" , median(
          [](){ return( 0 ); } , [ & ]( int ) {
           sink = sink + std::uint64_t( LZ4_compress_default(
                    s.data() , c.data() , int( n ) , int( c.size() ) ) ); } ,
          r , n ) );
 if( ! lz4d )
  return;
 c.resize( std::size_t( LZ4_compress_default( s.data() , c.data() , int( n ) ,
                                              int( c.size() ) ) ) );
 std::string d( n , '\0' );
 print( "lz" , "decode" , n , "LZ4_decompress_safe" , median(
         [](){ return( 0 ); } , [ & ]( int ) {
          sink = sink + std::uint64_t( LZ4_decompress_safe(
                   c.data() , d.data() , int( c.size() ) , int( n ) ) ); } ,
         r , n ) );
#endif
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 100000 , 1000000 , 10000000 } );
 for( std::size_t n : o.sizes ) entropyCoders( n , o.runs , o.seed );
 for( std::size_t n : o.sizes ) integerCodes( n , o.runs , o.seed );
 for( std::size_t n : o.sizes )
  if( n <= 10000000 ) transforms( n , o.runs , o.seed );
 for( std::size_t n : o.sizes )
  if( n <= 10000000 ) rIndexes( n , o.runs , o.seed );
 for( std::size_t n : o.sizes )
  if( n <= 10000000 ) lempelZiv( n , o.runs , o.seed );
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------ End File compression.cpp ------------------------*/
/*--------------------------------------------------------------------------*/
