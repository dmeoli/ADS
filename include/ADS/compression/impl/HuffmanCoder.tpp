/*--------------------------------------------------------------------------*/
/*------------------------- File HuffmanCoder.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of HuffmanCoder.h, with their contracts.
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

#include "ADS/compression/impl/Unaligned.h"

#include <memory>

namespace ADS {

namespace detail {

/// the longest codeword of HuffmanCoder.h, and the bytes of its header
constexpr int kHuffMax = 11;
constexpr std::size_t kHuffHead = 8 + 1 + 128 + 12;
/// the bytes from which the decoder reads a window of 12 bits
constexpr std::size_t kHuffWide = std::size_t( 1 ) << 18;

/// the l low bits of c turned around
constexpr std::uint32_t reverseLow( std::uint32_t c , int l )
{
 std::uint32_t r = 0;
 for( int i = 0 ; i < l ; ++i , c >>= 1 ) r = ( r << 1 ) | ( c & 1 );
 return( r );
 }

/// the canonical codes of the lengths len, turned around for a reading
/// from the least significant bit
inline std::array< std::uint32_t , 256 > huffmanCodes(
                                const std::array< std::uint8_t , 256 > & len )
{
 std::array< std::uint32_t , 256 > code{};
 std::uint32_t c = 0;
 for( int l = 1 ; l <= kHuffMax ; ++l , c <<= 1 )
  for( int s = 0 ; s < 256 ; ++s )
   if( len[ s ] == l ) code[ s ] = reverseLow( c++ , l );
 return( code );
 }

/// the writer of a stream: the bits not yet written, from the least
/// significant one, and the place of the next byte
struct HuffWriter {
 std::uint64_t acc = 0;
 unsigned bits = 0;
 std::uint8_t * p = nullptr;

 /// appends the code e (its value, its length in the bits 16 up)
 void put( std::uint32_t e ) {
  acc |= std::uint64_t( e & 0xFFFF ) << bits;
  bits += e >> 16;
  }

 /// appends the len bits of v
 void put( std::uint64_t v , unsigned len ) {
  acc |= v << bits;
  bits += len;
  }

 /// writes the whole bytes, 8 at a time, with 8 bytes of room after p
 void flush( void ) {
  storeLE< std::uint64_t >( p , acc );
  p += bits / 8;
  acc = ( bits >= 64 ) ? 0 : acc >> ( 8 * ( bits / 8 ) );
  bits %= 8;
  }
 };

/// the Kraft sum of the lengths, in units of 2^-kHuffMax
inline std::uint64_t huffmanKraft(
                                const std::array< std::uint8_t , 256 > & len )
{
 std::uint64_t k = 0;
 for( const auto l : len )
  if( l > 0 ) k += std::uint64_t( 1 ) << ( kHuffMax - l );
 return( k );
 }

}  // end( namespace detail )

inline std::array< std::uint8_t , 256 > limitedHuffmanLengths(
                             const std::array< std::uint64_t , 256 > & f ,
                             int L )
{
 std::array< std::uint8_t , 256 > len{};
 std::vector< int > sym;                   // the bytes used, by frequency
 for( int s = 0 ; s < 256 ; ++s )
  if( f[ s ] > 0 ) sym.push_back( s );
 ADS_PRE( ( L >= 1 ) && ( L <= 64 ) &&
          ( sym.size() <= ( std::size_t( 1 ) << std::min( L , 16 ) ) ) );
 if( sym.size() == 1 ) len[ sym[ 0 ] ] = 1;
 if( sym.size() <= 1 ) return( len );
 std::stable_sort( sym.begin() , sym.end() , [ & ]( int a , int b ) {
  return( f[ a ] < f[ b ] ); } );
 // package-merge: an item is a byte or a package of two items of the list
 // of the level below, each with its weight; the 2m - 2 lightest items of
 // the last list hold each byte as many times as its length
 struct Item { std::uint64_t w; int a , b; };   // a < 0: the byte -a - 1
 std::vector< Item > pool;
 const std::size_t m = sym.size();
 std::vector< int > list , next;
 for( const int s : sym ) {
  list.push_back( int( pool.size() ) );
  pool.push_back( { f[ s ] , -s - 1 , 0 } );
  }
 for( int l = 1 ; l < L ; ++l ) {
  next.clear();
  std::size_t i = 0 , j = 0;
  while( ( i < m ) || ( j + 1 < list.size() ) ) {
   const bool pack = ( j + 1 < list.size() ) &&
                     ( ( i == m ) || ( pool[ list[ j ] ].w +
                                       pool[ list[ j + 1 ] ].w <
                                       f[ sym[ i ] ] ) );
   if( pack ) {
    next.push_back( int( pool.size() ) );
    pool.push_back( { pool[ list[ j ] ].w + pool[ list[ j + 1 ] ].w ,
                      list[ j ] , list[ j + 1 ] } );
    j += 2;
    }
   else next.push_back( int( i++ ) );       // the byte sym[ i ] at pool[ i ]
   }
  list.swap( next );
  }
 std::vector< int > stack( list.begin() ,
                           list.begin() + std::ptrdiff_t( 2 * m - 2 ) );
 while( ! stack.empty() ) {
  const Item it = pool[ stack.back() ];
  stack.pop_back();
  if( it.a < 0 ) ++len[ -it.a - 1 ];
  else {
   stack.push_back( it.a );
   stack.push_back( it.b );
   }
  }
 return( len );
 }

inline std::vector< std::uint8_t > huffmanCompress(
                                          std::span< const std::uint8_t > in )
{
 using namespace detail;
 std::vector< std::uint8_t > out( kHuffHead , 0 );
 storeLE< std::uint64_t >( out.data() , in.size() );
 const auto f = byteHistogram( in );
 const auto len = limitedHuffmanLengths( f , kHuffMax );
 const int distinct = int( std::count_if( f.begin() , f.end() ,
                                          []( auto x ) {
                                           return( x > 0 ); } ) );
 if( distinct <= 1 ) {                      // the byte, if any
  out.resize( 9 );
  out[ 8 ] = 0;
  if( distinct == 1 ) out.push_back( in[ 0 ] );
  return( out );
  }
 out[ 8 ] = std::uint8_t( *std::max_element( len.begin() , len.end() ) );
 for( int s = 0 ; s < 256 ; s += 2 )
  out[ 9 + s / 2 ] = std::uint8_t( len[ s ] | ( len[ s + 1 ] << 4 ) );
 const auto code = huffmanCodes( len );
 std::array< std::uint32_t , 256 > E;       // the code and its length
 for( int c = 0 ; c < 256 ; ++c )
  E[ c ] = code[ c ] | ( std::uint32_t( len[ c ] ) << 16 );
 // the four streams one after the other, each straight after the one
 // before in the output, 4 codes of 11 bits at most joined and written at
 // once
 const std::size_t n = in.size() , q = ( n + 3 ) / 4;
 out.resize( kHuffHead + 4 * ( ( q * kHuffMax ) / 8 + 16 ) );
 HuffWriter W;
 W.p = out.data() + kHuffHead;
 for( std::size_t k = 0 ; k < 4 ; ++k ) {
  const std::uint8_t * start = W.p;
  const std::size_t a = std::min( n , k * q ) , b = std::min( n , a + q );
  std::size_t j = a;
  for( ; j + 4 <= b ; j += 4 ) {
   const std::uint8_t * x = in.data() + j;
   const std::uint32_t e0 = E[ x[ 0 ] ] , e1 = E[ x[ 1 ] ] ,
                       e2 = E[ x[ 2 ] ] , e3 = E[ x[ 3 ] ];
   const unsigned l0 = e0 >> 16 , l01 = l0 + ( e1 >> 16 ) ,
                  l012 = l01 + ( e2 >> 16 );
   const std::uint64_t v = std::uint64_t( e0 & 0xFFFF ) |
                           ( std::uint64_t( e1 & 0xFFFF ) << l0 ) |
                           ( std::uint64_t( e2 & 0xFFFF ) << l01 ) |
                           ( std::uint64_t( e3 & 0xFFFF ) << l012 );
   W.put( v , l012 + ( e3 >> 16 ) );
   W.flush();
   }
  for( ; j < b ; ++j ) {
   W.put( E[ in[ j ] ] );
   W.flush();
   }
  W.bits += 7;                              // the last byte, if partial
  W.flush();
  W.acc = 0;
  W.bits = 0;
  if( k < 3 )
   storeLE< std::uint32_t >( out.data() + 137 + 4 * k ,
                             std::uint32_t( W.p - start ) );
  }
 out.resize( std::size_t( W.p - out.data() ) );
 out.resize( out.size() + 8 , 0 );          // room for the loads
 ADS_AUDIT( std::ranges::equal( huffmanDecompress( out ) , in ) );
 return( out );
 }

namespace detail {

/// the four streams of the code in decoded into out, by a window of W
/// bits: the table of the single symbols of each value of W bits, and the
/// table of up to 3 symbols a lookup, an entry of 32 bits each, the
/// symbols in the low 24, then the bits read in 4 and the symbols in 2
template< int W >
inline void huffmanDecodeStreams( std::span< const std::uint8_t > in ,
                                  const std::array< std::uint8_t , 256 > &
                                   len , int L , std::size_t n ,
                                  std::uint8_t * out )
{
 ( void )L;
 std::array< std::uint16_t , 1 << W > T{};
 const auto code = huffmanCodes( len );
 for( int s = 0 ; s < 256 ; ++s )
  for( std::uint32_t h = 0 ;
       ( len[ s ] > 0 ) && ( h < ( std::uint32_t( 1 ) << ( W - len[ s ] ) ) ) ;
       ++h )
   T[ code[ s ] | ( h << len[ s ] ) ] = std::uint16_t( ( s << 8 ) | len[ s ] );
 std::array< std::uint32_t , 1 << W > T3;
 for( std::uint32_t v = 0 ; v < ( 1u << W ) ; ++v ) {
  std::uint32_t syms = 0 , bits = 0 , cnt = 0;
  for( ; cnt < 3 ; ++cnt ) {
   const std::uint16_t e = T[ ( v >> bits ) & ( ( 1u << W ) - 1 ) ];
   const std::uint32_t l = e & 0xFF;
   if( ( l == 0 ) || ( bits + l > std::uint32_t( W ) ) ) break;
   syms |= std::uint32_t( e >> 8 ) << ( 8 * cnt );
   bits += l;
   }
  if( cnt == 0 ) {                          // a code of no symbol: none
   cnt = 1;
   bits = T[ v ] & 0xFF;
   }
  T3[ v ] = syms | ( bits << 24 ) | ( cnt << 28 );
  }
 // the four streams
 std::array< const std::uint8_t * , 4 > p;
 p[ 0 ] = in.data() + kHuffHead;
 for( std::size_t k = 1 ; k < 4 ; ++k ) {
  const auto sz = loadLE< std::uint32_t >( in.data() + 137 + 4 * ( k - 1 ) );
  ADS_PRE( std::size_t( p[ k - 1 ] - in.data() ) + sz + 8 <= in.size() );
  p[ k ] = p[ k - 1 ] + sz;
  }
 const std::size_t q = ( n + 3 ) / 4;
 std::array< std::uint8_t * , 4 > o , end;
 for( std::size_t k = 0 ; k < 4 ; ++k ) {
  o[ k ] = out + std::min( n , k * q );
  end[ k ] = out + std::min( n , ( k + 1 ) * q );
  }
 constexpr std::uint64_t mask = ( std::uint64_t( 1 ) << W ) - 1;
 std::array< std::uint64_t , 4 > w;        // the word of each stream
 std::array< unsigned , 4 > used{};         // its bits read
 for( std::size_t k = 0 ; k < 4 ; ++k )
  w[ k ] = loadLE< std::uint64_t >( p[ k ] );
 // all four at once, 4 lookups of each between two loads of its word
 // (48 bits at most), as long as the 12 symbols that they may give, and
 // the 4 bytes that each lookup stores, stay in the stream (the bytes past
 // its symbols are overwritten by the next lookup); the rounds that
 // surely fit, counted again when they are over
 auto rounds = [ & ]( void ) {
  std::ptrdiff_t m = end[ 0 ] - o[ 0 ];
  for( std::size_t k = 1 ; k < 4 ; ++k ) m = std::min( m , end[ k ] - o[ k ] );
  return( m >= 3 ? std::size_t( ( m - 3 ) / 12 ) : std::size_t( 0 ) );
  };
 for( std::size_t R = rounds() ; R > 0 ; R = rounds() )
  for( ; R > 0 ; --R ) {
  for( std::size_t j = 0 ; j < 4 ; ++j )
   for( std::size_t k = 0 ; k < 4 ; ++k ) {
    const std::uint32_t e = T3[ ( w[ k ] >> used[ k ] ) & mask ];
    storeLE< std::uint32_t >( o[ k ] , e );
    o[ k ] += e >> 28;
    used[ k ] += ( e >> 24 ) & 15;
    }
  for( std::size_t k = 0 ; k < 4 ; ++k ) {
   p[ k ] += used[ k ] >> 3;
   used[ k ] &= 7;
   w[ k ] = loadLE< std::uint64_t >( p[ k ] );
   }
  }
 for( std::size_t k = 0 ; k < 4 ; ++k )     // the rest, a symbol a time
  for( ; o[ k ] < end[ k ] ; ++o[ k ] ) {
   const std::uint16_t e = T[ ( w[ k ] >> used[ k ] ) & mask ];
   *o[ k ] = std::uint8_t( e >> 8 );
   used[ k ] += e & 0xFF;
   p[ k ] += used[ k ] >> 3;
   used[ k ] &= 7;
   w[ k ] = loadLE< std::uint64_t >( p[ k ] );
   }
 }

}  // end( namespace detail )

inline std::vector< std::uint8_t > huffmanDecompress(
                                          std::span< const std::uint8_t > in )
{
 using namespace detail;
 ADS_PRE( in.size() >= 9 );
 const std::size_t n = std::size_t( loadLE< std::uint64_t >( in.data() ) );
 const int L = in[ 8 ];
 if( L == 0 ) {                             // one distinct byte, or none
  ADS_PRE( in.size() == ( n > 0 ? 10u : 9u ) );
  return( std::vector< std::uint8_t >( n , n > 0 ? in[ 9 ] : 0 ) );
  }
 ADS_PRE( ( L <= kHuffMax ) && ( in.size() >= kHuffHead + 8 ) );
 std::array< std::uint8_t , 256 > len{};
 for( int s = 0 ; s < 256 ; ++s )
  len[ s ] = std::uint8_t( ( in[ 9 + s / 2 ] >> ( 4 * ( s % 2 ) ) ) & 15 );
 ADS_PRE( ( huffmanKraft( len ) <= ( std::uint64_t( 1 ) << kHuffMax ) ) &&
          ( *std::max_element( len.begin() , len.end() ) == L ) );
 // a window of 12 bits on a long input, where the table of 16 KB pays
 // for itself (more symbols a lookup), of 11 bits on a short one
 std::vector< std::uint8_t > out( n + 8 );  // room for the triples
 if( n >= kHuffWide )
  huffmanDecodeStreams< 12 >( in , len , L , n , out.data() );
 else huffmanDecodeStreams< kHuffMax >( in , len , L , n , out.data() );
 out.resize( n );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HuffmanCoder.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
