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
 // the four streams at once, each in a buffer of its own, 4 codes of 11
 // bits at most between two writes of its bytes
 const std::size_t n = in.size() , q = ( n + 3 ) / 4;
 std::array< std::size_t , 5 > at;
 for( std::size_t k = 0 ; k < 5 ; ++k ) at[ k ] = std::min( n , k * q );
 const std::size_t room = ( q * kHuffMax ) / 8 + 24;   // a stream, at most
 const auto buf = std::make_unique_for_overwrite< std::uint8_t[] >( 4 * room );
 std::array< HuffWriter , 4 > W;
 for( std::size_t k = 0 ; k < 4 ; ++k ) W[ k ].p = buf.get() + k * room;
 const std::size_t last = at[ 4 ] - at[ 3 ];   // the shortest stream
 std::size_t i = 0;
 for( ; i + 4 <= last ; i += 4 )
  for( std::size_t k = 0 ; k < 4 ; ++k ) {
   const std::uint8_t * x = in.data() + at[ k ] + i;
   W[ k ].put( E[ x[ 0 ] ] );
   W[ k ].put( E[ x[ 1 ] ] );
   W[ k ].put( E[ x[ 2 ] ] );
   W[ k ].put( E[ x[ 3 ] ] );
   W[ k ].flush();
   }
 for( std::size_t k = 0 ; k < 4 ; ++k ) {
  for( std::size_t j = at[ k ] + i ; j < at[ k + 1 ] ; ++j ) {
   W[ k ].put( E[ in[ j ] ] );
   W[ k ].flush();
   }
  W[ k ].bits += 7;                         // the last byte, if partial
  W[ k ].flush();
  const std::uint8_t * b0 = buf.get() + k * room;
  const std::size_t size = std::size_t( W[ k ].p - b0 );
  if( k < 3 )
   storeLE< std::uint32_t >( out.data() + 137 + 4 * k ,
                             std::uint32_t( size ) );
  out.insert( out.end() , b0 , b0 + size );
  }
 out.resize( out.size() + 8 , 0 );          // room for the loads
 ADS_AUDIT( std::ranges::equal( huffmanDecompress( out ) , in ) );
 return( out );
 }

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
 // the tables of the values of L bits: the symbol and its length, and
 // the two symbols that fit in them (one, if the second does not), their
 // number and their bits
 std::array< std::uint16_t , 1 << kHuffMax > T{};
 const auto code = huffmanCodes( len );
 for( int s = 0 ; s < 256 ; ++s )
  for( std::uint32_t h = 0 ;
       ( len[ s ] > 0 ) && ( h < ( std::uint32_t( 1 ) << ( L - len[ s ] ) ) ) ;
       ++h )
   T[ code[ s ] | ( h << len[ s ] ) ] = std::uint16_t( ( s << 8 ) | len[ s ] );
 std::array< std::uint32_t , 1 << kHuffMax > T2;
 for( std::uint32_t v = 0 ; v < ( 1u << L ) ; ++v ) {
  const std::uint32_t l1 = T[ v ] & 0xFF , e2 = T[ v >> l1 ] ,
                      l2 = e2 & 0xFF;
  T2[ v ] = ( l2 > 0 ) && ( l1 + l2 <= std::uint32_t( L ) )
            ? ( T[ v ] >> 8 ) | ( e2 & 0xFF00 ) | ( 2u << 16 ) |
              ( ( l1 + l2 ) << 24 )
            : ( T[ v ] >> 8 ) | ( 1u << 16 ) | ( l1 << 24 );
  }
 // the four streams
 std::array< const std::uint8_t * , 4 > p;
 p[ 0 ] = in.data() + kHuffHead;
 for( std::size_t k = 1 ; k < 4 ; ++k ) {
  const auto sz = loadLE< std::uint32_t >( in.data() + 137 + 4 * ( k - 1 ) );
  ADS_PRE( std::size_t( p[ k - 1 ] - in.data() ) + sz + 8 <= in.size() );
  p[ k ] = p[ k - 1 ] + sz;
  }
 std::vector< std::uint8_t > out( n + 8 );  // room for the pairs
 const std::size_t q = ( n + 3 ) / 4;
 std::array< std::uint8_t * , 4 > o , end;
 for( std::size_t k = 0 ; k < 4 ; ++k ) {
  o[ k ] = out.data() + std::min( n , k * q );
  end[ k ] = out.data() + std::min( n , ( k + 1 ) * q );
  }
 const std::uint64_t mask = ( std::uint64_t( 1 ) << L ) - 1;
 std::array< std::uint64_t , 4 > w;        // the word of each stream
 std::array< unsigned , 4 > used{};         // its bits read
 for( std::size_t k = 0 ; k < 4 ; ++k )
  w[ k ] = loadLE< std::uint64_t >( p[ k ] );
 // all four at once, 4 lookups of each between two loads of its word, as
 // long as the 8 symbols that they may give stay in the stream (the second
 // byte of a lookup that gives one is overwritten by the next)
 auto room = [ & ]( void ) {
  return( ( end[ 0 ] - o[ 0 ] >= 8 ) && ( end[ 1 ] - o[ 1 ] >= 8 ) &&
          ( end[ 2 ] - o[ 2 ] >= 8 ) && ( end[ 3 ] - o[ 3 ] >= 8 ) );
  };
 while( room() ) {
  for( std::size_t j = 0 ; j < 4 ; ++j )
   for( std::size_t k = 0 ; k < 4 ; ++k ) {
    const std::uint32_t e = T2[ ( w[ k ] >> used[ k ] ) & mask ];
    storeLE< std::uint16_t >( o[ k ] , std::uint16_t( e ) );
    o[ k ] += ( e >> 16 ) & 0xFF;
    used[ k ] += e >> 24;
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
 out.resize( n );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HuffmanCoder.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
