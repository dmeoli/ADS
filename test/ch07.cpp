/*--------------------------------------------------------------------------*/
/*----------------------------- File ch07.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 7, Tabelle hash: the table with direct access of the
 * examples, the hash functions of section 7.1 on the keys of the book and
 * by the chi-square test, the deleted places of Figura 7.6, every table
 * against std::map on random operations, the expected numbers of accesses
 * of section 7.3, the restructuring of section 7.4, and the exercises 7.1
 * to 7.10, each a test named after it.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 7,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADSTest.h"

#include "ADS/ADT.h"
#include "ADS/dictionary/ChainedHashTable.h"
#include "ADS/dictionary/DirectTable.h"
#include "ADS/dictionary/HashFunctions.h"
#include "ADS/dictionary/OpenHashTable.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/// the identity, the h( k ) = k of the exercises
struct Id {
 std::size_t operator()( long k ) const { return( k ); }
 };

/// a hash that mixes the bits (splitmix64), close to uniform hashing
struct Mix {
 std::size_t operator()( std::uint64_t x ) const {
  x += 0x9E3779B97F4A7C15ull;
  x = ( x ^ ( x >> 30 ) ) * 0xBF58476D1CE4E5B9ull;
  x = ( x ^ ( x >> 27 ) ) * 0x94D049BB133111EBull;
  return( x ^ ( x >> 31 ) );
  }
 };

using Front = ChainedHashTable< long , long , Chain::FRONT , Id >;
using Open = OpenHashTable< long , long , Id >;

static_assert( Dictionary< ChainedHashTable< std::string , int > > );
static_assert( Dictionary< ChainedHashTable< long , long ,
                                             Chain::SORTED > > );
static_assert( Dictionary< OpenHashTable< std::string , int > > );
static_assert( Dictionary< DirectTable< int > > );

/*--------------------------------------------------------------------------*/
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the places of an open table as a string: '.' nil, '-' deleted, else the
/// key
template< class T >
std::string picture( const T & t )
{
 std::string s;
 for( std::size_t i = 0 ; i < t.capacity() ; ++i )
  switch( t.slot( i ) ) {
   case( T::Slot::NIL ): s += '.'; break;
   case( T::Slot::DELETED ): s += '-'; break;
   default: s += char( t.keyAt( i ) );
   }
 return( s );
 }

/// the chi-square statistic of the counts c against the uniform
double chi2( const std::vector< long > & c , long n )
{
 const double e = double( n ) / c.size();
 double x = 0;
 for( auto v : c )
  x += ( v - e ) * ( v - e ) / e;
 return( x );
 }

/// a dictionary against std::map on random operations on the keys
/// [ 0 , K )
template< class D >
void against_map( D & d , long K , int ops , unsigned seed )
{
 std::mt19937 g( seed );
 std::uniform_int_distribution< long > key( 0 , K - 1 );
 std::map< long , long > ref;
 for( int o = 0 ; o < ops ; ++o ) {
  const long k = key( g );
  switch( g() % 3 ) {
   case( 0 ):
    d.insert( k , o );
    ref[ k ] = o;
    break;
   case( 1 ):
    d.remove( k );
    ref.erase( k );
    break;
   default: {
    const auto it = ref.find( k );
    const auto v = d.lookup( k );
    CHECK( v.has_value() == ( it != ref.end() ) );
    if( v )
     CHECK( *v == it->second );
    }
   }
  CHECK( d.size() == ref.size() );
  }
 for( long k = 0 ; k < K ; ++k )
  CHECK( d.contains( k ) == ( ref.count( k ) == 1 ) );
 }

/// the averages of the accesses of a successful and of an unsuccessful
/// search after the insertion of n random keys
template< class D >
std::pair< double , double > accesses( D & d , std::size_t n ,
                                       unsigned seed )
{
 std::mt19937_64 g( seed );
 std::set< std::uint64_t > in;
 while( in.size() < n )
  in.insert( g() >> 1 );
 for( auto k : in )
  d.insert( k , 0 );
 double S = 0 , I = 0;
 for( auto k : in ) {
  (void) d.lookup( k );
  S += d.probes();
  }
 const int q = 20000;
 for( int t = 0 ; t < q ; ) {
  const auto k = g() >> 1;
  if( in.count( k ) )
   continue;
  (void) d.lookup( k );
  I += d.probes();
  ++t;
  }
 return( std::pair( S / n , I / q ) );
 }

/// true if x is within the relative tolerance r of y
bool near( double x , double y , double r ) {
 return( std::abs( x - y ) <= r * y );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- TESTS -----------------------------------*/
/*--------------------------------------------------------------------------*/

// the trains numbered from 1 to 36799 and the phone extensions in
// [ 280000 , 289999 ], both with a perfect hash
ADS_TEST( "sec7.direct-access" )
{
 DirectTable< std::string > trains( 1 , 36799 );
 CHECK( trains.capacity() == 36799 );
 trains.insert( 1 , "Trento" );
 trains.insert( 36799 , "Roma" );
 CHECK( trains.lookup( 36799 ) == "Roma" );
 CHECK( ! trains.lookup( 2 ) );
 CHECK_THROWS( (void) trains.lookup( 0 ) , std::out_of_range );
 CHECK_THROWS( trains.insert( 36800 , "" ) , std::out_of_range );

 DirectTable< int > phones( 280000 , 289999 );
 CHECK( phones.capacity() == 10000 );
 for( long k = 280000 ; k <= 289999 ; k += 7 )
  phones.insert( k , int( k % 1000 ) );
 CHECK( phones.size() == 1429 );
 phones.remove( 280007 );
 phones.remove( 280008 );
 CHECK( phones.size() == 1428 );
 CHECK( phones.lookup( 289996 ) == 996 );
 }

/*--------------------------------------------------------------------------*/

// the examples of section 7.1
ADS_TEST( "sec7.1.functions" )
{
 // extraction: the 16 least significant bits, then 16 bits from the 5th
 CHECK( hash_extraction( "Alberto" , 16 , 40 ) == 29807 );
 CHECK( hash_extraction( "Roberto" , 16 , 40 ) == 29807 );
 CHECK( hash_extraction( "Alberto" , 16 , 4 ) == 5830 );
 CHECK( hash_extraction( "Alessio" , 16 , 4 ) == 5830 );

 // XOR: 5 blocks of 16 bits, 8 zeros of padding, anagrams collide
 CHECK( hash_xor( "montresor" , 16 ) == 28689 );
 CHECK( hash_xor( "sontremor" , 16 ) == 28689 );

 // division by m = 383, by Horner as by the whole integer
 CHECK( int_bin( "Alberto" ) == 18415043350787183ull );
 CHECK( int_bin( "Alessio" ) == 18415056470632815ull );
 CHECK( int_bin( "Cristian" ) == 4860062892481405294ull );
 for( const char * k : { "Alberto" , "Alessio" , "Cristian" } )
  CHECK( hash_division( k , 383 ) == int_bin( k ) % 383 );
 CHECK( hash_division( "Alberto" , 383 ) == 221 );
 CHECK( hash_division( "Alessio" , 383 ) == 77 );
 CHECK( hash_division( "Cristian" , 383 ) == 130 );

 // multiplication, m = 2^16: the values of the book (51598, 33762,
 // 47280) do not follow from i = int( bin( k ) ), whose exact
 // floor( m frac( C i ) ) are 51576, 33740 and 47003; the version with
 // integers is off by at most m i / 2^64
 const struct { const char * k; std::uint64_t exact , fib; } M[] = {
  { "Alberto" , 51576 , 51514 } , { "Alessio" , 33740 , 33678 } ,
  { "Cristian" , 47003 , 30572 } };
 for( const auto & e : M ) {
  const auto i = int_bin( e.k );
  CHECK( hash_fibonacci( i , 16 ) == e.fib );
  const double off = 65536.0 * double( i ) / 18446744073709551616.0;
  CHECK( std::abs( double( e.fib ) - double( e.exact ) ) <= off + 1 );
  }
 // ... and agrees with the floating point one on small integers
 for( std::uint64_t i = 0 ; i < 4096 ; ++i )
  CHECK( hash_multiplication( i , 65536 ) == hash_fibonacci( i , 16 ) );

 // the middle square
 CHECK( hash_midsquare( 123456789 , 16 ) == 8855 );
 for( std::uint64_t i = 0 ; i < 1000 ; ++i )
  CHECK( hash_midsquare( i * 7919 , 10 ) < 1024 );

 CHECK_THROWS( (void) hash_extraction( "a" , 0 , 0 ) ,
               std::invalid_argument );
 CHECK_THROWS( (void) hash_division( "a" , 0 ) , std::invalid_argument );
 CHECK_THROWS( (void) int_bin( "123456789" ) , std::invalid_argument );
 }

// the simple uniformity by the chi-square test: the division by a prime
// spreads random strings as a uniform distribution would (the statistic
// has mean m - 1 and deviation sqrt( 2 ( m - 1 ) )), the extraction of the
// first bits sends keys with a common prefix to one place
ADS_TEST( "sec7.1.uniformity" )
{
 const long n = 20000 , m = 383;
 std::mt19937 g( 7 );
 std::vector< long > div( m ) , ext( m );
 for( long t = 0 ; t < n ; ++t ) {
  std::string k = "key";
  for( int c = 0 ; c < 8 ; ++c )
   k += char( 'a' + g() % 26 );
  ++div[ hash_division( k , m ) ];
  ++ext[ hash_extraction( k , 16 , 0 ) % m ];
  }
 CHECK( chi2( div , n ) < ( m - 1 ) + 6 * std::sqrt( 2.0 * ( m - 1 ) ) );
 CHECK( std::count( ext.begin() , ext.end() , n ) == 1 );
 }

/*--------------------------------------------------------------------------*/

// Figura 7.6: a deleted place does not stop the inspection, and an
// insertion reuses it
ADS_TEST( "sec7.2.deleted" )
{
 Open t( 7 , Probing::linear() );
 for( long k : { 0 , 7 , 14 } )        // all with H0( k ) = 0
  t.insert( k , k );
 t.remove( 7 );
 CHECK( t.slot( 1 ) == Open::Slot::DELETED );
 CHECK( t.lookup( 14 ) == 14 );
 CHECK( t.probes() == 3 );
 CHECK( ! t.lookup( 28 ) );
 CHECK( t.probes() == 4 );             // 0, 1 (deleted), 2, the nil 3
 t.insert( 21 , 21 );                  // into the first deleted place
 CHECK( t.slot( 1 ) == Open::Slot::FULL && t.keyAt( 1 ) == 21 );
 t.insert( 14 , 15 );                  // a key that is there stays
 CHECK( t.size() == 3 && t.lookup( 14 ) == 15 );

 // no nil left: an unsuccessful search stops after m inspections
 Open u( 5 , Probing::linear() );
 for( long k = 0 ; k < 5 ; ++k )
  u.insert( k , k );
 CHECK_THROWS( u.insert( 5 , 5 ) , std::overflow_error );
 for( long k = 0 ; k < 5 ; ++k )
  u.remove( k );
 CHECK( u.isEmpty() && ! u.lookup( 5 ) && u.probes() == 5 );
 u.insert( 5 , 5 );
 CHECK( u.lookup( 5 ) == 5 );
 }

// every table against std::map, with and without restructuring
ADS_TEST( "sec7.2.against-map" )
{
 unsigned seed = 1;
 for( bool r : { false , true } ) {
  ChainedHashTable< long , long , Chain::FRONT , Mix > a( 7 , r );
  ChainedHashTable< long , long , Chain::MOVE_TO_FRONT , Mix > b( 7 , r );
  ChainedHashTable< long , long , Chain::SORTED , Mix > c( 7 , r );
  against_map( a , 1000 , 20000 , seed++ );
  against_map( b , 1000 , 20000 , seed++ );
  against_map( c , 1000 , 20000 , seed++ );
  const std::size_t m = r ? 7 : 4099;
  for( auto p : { Probing::linear() , Probing::linear( 3 ) ,
                  Probing::quadratic( 1 , 1 ) , Probing::double_hashing() } )
   {
   OpenHashTable< long , long , Mix > d( m , p , r );
   against_map( d , 1000 , 20000 , seed++ );
   }
  OpenHashTable< long , long , Mix > e( r ? 8 : 4096 ,
                                         Probing::double_hashing() , r );
  against_map( e , 1000 , 20000 , seed++ );
  }
 CHECK_THROWS( ( Open( 8 , Probing::linear( 2 ) ) ) , std::invalid_argument );
 CHECK_THROWS( ( Open( 8 , Probing::quadratic( 0 , 1 ) ) ) ,
               std::invalid_argument );
 CHECK_THROWS( ( Open( 12 , Probing::double_hashing() ) ) ,
               std::invalid_argument );
 CHECK_THROWS( ( Front( 0 ) ) , std::invalid_argument );
 }

/*--------------------------------------------------------------------------*/

// the table of section 7.3: the averages of the accesses of the successful
// and of the unsuccessful searches against S( alpha ) and I( alpha )
ADS_TEST( "sec7.3.complexity" )
{
 const std::size_t m = 100003;
 unsigned seed = 11;
 for( double a : { 0.25 , 0.5 , 0.75 } ) {
  OpenHashTable< std::uint64_t , int , Mix > t( m , Probing::linear() );
  const auto [ S , I ] = accesses( t , std::size_t( a * m ) , seed++ );
  CHECK( near( S , ( 1 + 1 / ( 1 - a ) ) / 2 , 0.05 ) );
  CHECK( near( I , ( 1 + 1 / ( ( 1 - a ) * ( 1 - a ) ) ) / 2 , 0.05 ) );
  }
 for( double a : { 0.25 , 0.5 , 0.75 , 0.9 } ) {
  OpenHashTable< std::uint64_t , int , Mix > t( m ,
                                                Probing::double_hashing() );
  const auto [ S , I ] = accesses( t , std::size_t( a * m ) , seed++ );
  CHECK( near( S , - std::log( 1 - a ) / a , 0.05 ) );
  CHECK( near( I , 1 / ( 1 - a ) , 0.05 ) );
  }
 for( double a : { 0.5 , 1.0 , 2.0 , 4.0 } ) {
  ChainedHashTable< std::uint64_t , int , Chain::FRONT , Mix > t( m / 10 );
  const auto [ S , I ] = accesses( t , std::size_t( a * m / 10 ) , seed++ );
  CHECK( near( S , 1 + a / 2 , 0.05 ) );
  CHECK( near( I , 1 + a , 0.05 ) );
  }
 }

/*--------------------------------------------------------------------------*/

// section 7.4: the load factor stays in its band, n insertions move at most
// 2 n pairs, so they cost at most 3 n, and a sequence that alternates an
// insertion and a removal at the threshold restructures only once
ADS_TEST( "sec7.4.restructure" )
{
 const long n = 100000;
 OpenHashTable< long , long , Mix > t( 7 , Probing::double_hashing() , true );
 for( long k = 0 ; k < n ; ++k ) {
  t.insert( k , k );
  CHECK( t.load() <= 0.5 );
  }
 CHECK( t.size() + t.moved() <= std::size_t( 3 * n ) );
 CHECK( t.restructures() <= 1 + std::log2( double( n ) ) );
 for( long k = 0 ; k < n ; ++k )
  CHECK( t.lookup( k ) == k );
 for( long k = 0 ; k < n ; ++k ) {
  t.remove( k );
  CHECK( t.capacity() == 7 || t.load() >= 0.125 );
  }
 CHECK( t.isEmpty() && t.capacity() == 7 );

 ChainedHashTable< long , long , Chain::FRONT , Mix > c( 7 , true );
 for( long k = 0 ; k < n ; ++k ) {
  c.insert( k , k );
  CHECK( c.load() <= 2 );
  }
 CHECK( c.size() + c.moved() <= std::size_t( 3 * n ) );
 for( long k = 0 ; k < n ; ++k ) {
  c.remove( k );
  CHECK( c.capacity() == 7 || c.load() >= 0.5 );
  }

 // just after a doubling, removal and insertion of one key at the edge
 OpenHashTable< long , long , Mix > u( 7 , Probing::double_hashing() , true );
 long k = 0;
 for( ; u.restructures() < 6 ; ++k )
  u.insert( k , k );
 const auto r = u.restructures();
 for( int i = 0 ; i < 1000 ; ++i ) {
  u.remove( k - 1 );
  u.insert( k - 1 , 0 );
  }
 CHECK( u.restructures() <= r + 1 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ EXERCISES ---------------------------------*/
/*--------------------------------------------------------------------------*/

// the letters by their place in the Italian alphabet; the solution of the
// book counts it from 1, not from 0 as the text says
struct Letter {
 std::size_t operator()( long c ) const {
  return( std::string( "ABCDEFGHILMNOPQRSTUVZ" ).find( char( c ) ) + 1 );
  }
 };

ADS_TEST( "ex7.1" )
{
 OpenHashTable< long , long , Letter > t( 11 , Probing::linear() );
 for( char c : std::string( "CALCIOPULITO" ) )
  t.insert( c , 0 );
 CHECK( picture( t ) == ".AOCP..TUIL" );
 for( char c : std::string( "GALLIANI" ) )
  t.remove( c );
 CHECK( picture( t ) == ".-OCP..TU--" );
 for( char c : std::string( "RIVERA" ) )
  t.insert( c , 0 );
 CHECK( picture( t ) == ".AOCPRETUIV" );
 }

// the inspection ( k mod 11 + 3 i + i^2 ) mod 11 is not a permutation:
// the key 7 visits only the places 7, 0, 6, 3, 2 and 5, all full, while
// the place 8 is free, so the insertion of 7 fails; the other 10 keys need
// 12 accesses, 1.2 on average
ADS_TEST( "ex7.2" )
{
 Open t( 11 , Probing::quadratic( 3 , 1 ) );
 for( long k : { 33 , 10 , 24 , 14 , 16 , 13 , 23 , 31 , 18 , 11 } )
  t.insert( k , k );
 CHECK_THROWS( t.insert( 7 , 7 ) , std::overflow_error );
 CHECK( t.slot( 8 ) == Open::Slot::NIL );
 const std::vector< long > T = { 33 , 23 , 24 , 14 , 11 , 16 , 13 , 18 ,
                                 -1 , 31 , 10 };
 std::size_t sum = 0;
 for( std::size_t i = 0 ; i < 11 ; ++i )
  if( T[ i ] >= 0 ) {
   CHECK( t.keyAt( i ) == T[ i ] );
   (void) t.lookup( T[ i ] );
   sum += t.probes();
   }
 CHECK( sum == 12 );
 }

// keys of 8 bits: a table with direct access of 256 places, every
// operation one access
ADS_TEST( "ex7.3" )
{
 DirectTable< int > t( 0 , 255 );
 for( long k = 0 ; k < 256 ; ++k )
  t.insert( k , int( 255 - k ) );
 CHECK( t.size() == 256 && t.capacity() == 256 );
 for( long k = 0 ; k < 256 ; ++k )
  CHECK( t.lookup( k ) == 255 - k );
 }

// the procedures of the linear and of the quadratic inspection
ADS_TEST( "ex7.4" )
{
 auto visited = []( const Probing & p , std::size_t h , std::size_t m ) {
  std::set< std::size_t > s;
  for( std::size_t i = 0 ; i < m ; ++i )
   s.insert( p.at( h , i , m ) );
  return( s.size() );
  };
 for( long d : { 1 , 3 , 5 , 7 } )
  CHECK( visited( Probing::linear( d ) , 5 , 12 ) == ( d == 3 ? 4u : 12u ) );
 CHECK( visited( Probing::linear( -1 ) , 5 , 12 ) == 12 );
 // i^2 mod p takes ( p + 1 ) / 2 values for an odd prime p
 for( std::size_t p : { 7 , 11 , 13 , 101 } )
  CHECK( visited( Probing::quadratic( 0 , 1 ) , 3 , p ) == ( p + 1 ) / 2 );
 // the step of the double hashing is coprime with a prime or a power of 2
 for( std::size_t m : { 7 , 101 , 8 , 1024 } )
  for( std::size_t h = 0 ; h < 500 ; ++h )
   CHECK( visited( Probing::double_hashing() , h * 2654435761u , m ) == m );
 }

// the keys looked up often are brought to the head of their list
ADS_TEST( "ex7.5" )
{
 ChainedHashTable< long , long , Chain::MOVE_TO_FRONT , Id > t( 1 );
 for( long k = 1 ; k <= 5 ; ++k )
  t.insert( k , k );
 CHECK( t.chain( 0 ) == std::vector< long >( { 5 , 4 , 3 , 2 , 1 } ) );
 CHECK( t.lookup( 1 ) == 1 );
 CHECK( t.chain( 0 ) == std::vector< long >( { 1 , 5 , 4 , 3 , 2 } ) );

 // 1000 keys in 10 lists, 9 lookups out of 10 on 10 keys
 ChainedHashTable< long , long , Chain::FRONT , Mix > f( 10 );
 ChainedHashTable< long , long , Chain::MOVE_TO_FRONT , Mix > g( 10 );
 for( long k = 0 ; k < 1000 ; ++k ) {
  f.insert( k , k );
  g.insert( k , k );
  }
 std::mt19937 r( 5 );
 double pf = 0 , pg = 0;
 for( int q = 0 ; q < 20000 ; ++q ) {
  const long k = ( r() % 10 ) ? long( r() % 10 ) * 97 : long( r() % 1000 );
  (void) f.lookup( k );
  (void) g.lookup( k );
  pf += f.probes();
  pg += g.probes();
  }
 CHECK( pg < pf / 4 );
 }

// the replicated files: a table from the content to the paths, O( n ) time
// expected (the content stands for the content and the length)
ADS_TEST( "ex7.6" )
{
 const std::vector< std::pair< std::string , std::string > > files = {
  { "/a/x" , "alpha" } , { "/b/y" , "beta" } , { "/c/x" , "alpha" } ,
  { "/d" , "gamma" } , { "/e/f/g" , "beta" } , { "/h" , "alpha" } ,
  { "/i" , "alph" } };
 ChainedHashTable< std::string , std::vector< std::string > > t( 7 , true );
 std::vector< std::string > order;
 for( const auto & [ path , content ] : files ) {
  auto v = t.lookup( content );
  if( ! v ) {
   v.emplace();
   order.push_back( content );
   }
  v->push_back( path );
  t.insert( content , *v );
  }
 std::vector< std::vector< std::string > > replicated;
 for( const auto & c : order )
  if( auto v = t.lookup( c ) ; v->size() > 1 )
   replicated.push_back( *v );
 CHECK( replicated == std::vector< std::vector< std::string > >(
         { { "/a/x" , "/c/x" , "/h" } , { "/b/y" , "/e/f/g" } } ) );
 }

// a list of long keys with their hash: the hashes are compared first, and
// a key only when its hash is equal
ADS_TEST( "ex7.7" )
{
 const std::string prefix( 1000 , 'x' );
 std::vector< std::pair< std::string , std::size_t > > L;
 for( int i = 0 ; i < 500 ; ++i ) {
  const auto k = prefix + std::to_string( i );
  L.emplace_back( k , std::hash< std::string >()( k ) );
  }
 auto search = [ & ]( const std::string & k , bool by_hash , int & cmp ) {
  const auto h = std::hash< std::string >()( k );
  cmp = 0;
  for( std::size_t i = 0 ; i < L.size() ; ++i ) {
   if( by_hash && ( L[ i ].second != h ) )
    continue;
   ++cmp;
   if( L[ i ].first == k )
    return( long( i ) );
   }
  return( -1L );
  };
 int a , b;
 CHECK( search( prefix + "321" , false , a ) == 321 && a == 322 );
 CHECK( search( prefix + "321" , true , b ) == 321 && b == 1 );
 CHECK( search( prefix + "999" , true , b ) == -1 && b == 0 );
 }

// with an alphabet of 2^p characters and m = 2^p - 1, 2^p = 1 mod m and so
// int( bin( k ) ) mod m is the sum of the characters mod m: swapping two of
// them, hence any permutation, does not change the hash
ADS_TEST( "ex7.8" )
{
 std::string k = "emnoorrst";               // the letters of montresor
 const auto h = hash_division( k , 255 );
 long sum = 0;
 for( unsigned char c : k )
  sum += c;
 CHECK( h == std::uint64_t( sum % 255 ) );
 long n = 0;
 do {
  CHECK( hash_division( k , 255 ) == h );
  ++n;
  } while( std::next_permutation( k.begin() , k.end() ) );
 CHECK( n == 90720 );                        // 9! / ( 2! 2! )
 CHECK( hash_division( "montresor" , 383 ) !=
        hash_division( "sontremor" , 383 ) );
 }

// the duplicates of an unsorted vector, removed in O( n ) expected time by
// a table of the elements already seen
ADS_TEST( "ex7.9" )
{
 std::mt19937 g( 9 );
 std::vector< long > A( 50000 );
 for( auto & a : A )
  a = g() % 20000;
 OpenHashTable< long , bool , Mix > seen( 7 , Probing::double_hashing() ,
                                          true );
 std::vector< long > B;
 for( auto a : A )
  if( ! seen.contains( a ) ) {
   seen.insert( a , true );
   B.push_back( a );
   }
 std::set< long > s( A.begin() , A.end() );
 CHECK( B.size() == s.size() );
 CHECK( std::set< long >( B.begin() , B.end() ) == s );
 for( std::size_t i = 1 ; i < B.size() ; ++i )   // in order of first sight
  CHECK( std::find( A.begin() , A.end() , B[ i - 1 ] ) <
         std::find( A.begin() , A.end() , B[ i ] ) );
 }

// sorted lists: an unsuccessful search stops at the first larger key, half
// the accesses, and so does the insertion of keys in random order that
// looks for the key first (in increasing order each one would go to the
// end of its list); the successful search costs the same
ADS_TEST( "ex7.10" )
{
 const long n = 1000;
 ChainedHashTable< long , long , Chain::FRONT , Mix > f( 10 );
 ChainedHashTable< long , long , Chain::SORTED , Mix > s( 10 );
 std::vector< long > K( n );
 for( long k = 0 ; k < n ; ++k )
  K[ k ] = 2 * k;
 std::shuffle( K.begin() , K.end() , std::mt19937( 10 ) );
 double fi = 0 , si = 0;
 for( long k : K ) {
  f.insert( k , k );
  fi += f.probes();
  s.insert( k , k );
  si += s.probes();
  }
 CHECK( si < 0.6 * fi );
 for( std::size_t i = 0 ; i < s.capacity() ; ++i ) {
  const auto c = s.chain( i );
  CHECK( std::is_sorted( c.begin() , c.end() ) );
  }
 double fu = 0 , su = 0 , fs = 0 , ss = 0;
 for( long k = 0 ; k < n ; ++k ) {
  (void) f.lookup( 2 * k + 1 );
  (void) s.lookup( 2 * k + 1 );
  fu += f.probes();
  su += s.probes();
  (void) f.lookup( 2 * k );
  (void) s.lookup( 2 * k );
  fs += f.probes();
  ss += s.probes();
  }
 CHECK( near( fu / n , 1 + 100.0 , 0.05 ) );
 CHECK( near( su / n , 1 + 50.0 , 0.1 ) );
 CHECK( near( ss , fs , 0.1 ) );
 }

}  // end( namespace )

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch07.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
