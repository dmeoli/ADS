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
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <utility>
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

/// the worst hash, every key in the same place
struct Zero {
 std::size_t operator()( long ) const { return( 0 ); }
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
// [ 280000 , 289999 ], both with a perfect hash; the edges: a table of one
// place, negative keys, a key inserted twice or removed when absent, the
// order of the view, the keys at the ends of long checked without
// overflow, a > b refused, and the whole range of long refused where its
// size does not fit in std::size_t (not where long has 32 bits, as on
// MSVC, where it fits)
ADS_TEST( "sec7.direct-access" )
{
 using LL = std::numeric_limits< long >;
 DirectTable< int > one( 5 , 5 );
 CHECK( one.capacity() == 1 && one.size() == 0 && ! one.lookup( 5 ) );
 one.insert( 5 , 1 );
 one.insert( 5 , 2 );                      // overwritten, still one key
 CHECK( one.size() == 1 && one.lookup( 5 ) == 2 );
 one.remove( 5 );
 one.remove( 5 );                          // absent: nothing changes
 CHECK( one.size() == 0 && ! one.lookup( 5 ) );
 CHECK_THROWS( one.insert( 4 , 0 ) , contract_violation );
 CHECK_THROWS( one.remove( 6 ) , contract_violation );

 DirectTable< int > neg( -3 , 3 );
 for( long k : { 3 , -3 , 0 } )
  neg.insert( k , int( 10 * k ) );
 std::vector< std::pair< long , int > > seen;
 for( const auto & [ k , v ] : neg )
  seen.emplace_back( k , v );
 CHECK( seen == ( std::vector< std::pair< long , int > >(
                  { { -3 , -30 } , { 0 , 0 } , { 3 , 30 } } ) ) );
 for( long k : { LL::min() , LL::max() , -4L , 4L } )
  CHECK_THROWS( (void) neg.lookup( k ) , contract_violation );

 DirectTable< int > low( LL::min() , LL::min() + 2 );
 DirectTable< int > high( LL::max() - 2 , LL::max() );
 low.insert( LL::min() , 1 );
 high.insert( LL::max() , 2 );
 CHECK( low.lookup( LL::min() ) == 1 && high.lookup( LL::max() ) == 2 );
 CHECK_THROWS( (void) low.lookup( LL::max() ) , contract_violation );
 CHECK_THROWS( (void) high.lookup( LL::min() ) , contract_violation );
 CHECK_THROWS( ( DirectTable< int >( 1 , 0 ) ) , contract_violation );
 if constexpr( sizeof( long ) == sizeof( std::size_t ) )   // not on MSVC
  CHECK_THROWS( ( DirectTable< int >( LL::min() , LL::max() ) ) ,
                contract_violation );

 DirectTable< std::string > trains( 1 , 36799 );
 CHECK( trains.capacity() == 36799 );
 trains.insert( 1 , "Trento" );
 trains.insert( 36799 , "Roma" );
 CHECK( trains.lookup( 36799 ) == "Roma" );
 CHECK( ! trains.lookup( 2 ) );
 CHECK_THROWS( (void) trains.lookup( 0 ) , contract_violation );
 CHECK_THROWS( trains.insert( 36800 , "" ) , contract_violation );

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

// the examples of section 7.1; the edges: the empty key, the bits past its
// end, the bytes 0xFF, the widths p = 1 and the largest, m = 1, m = 0 and
// C = 0 for the multiplication, the square that wraps modulo 2^64, Horner
// at the largest m of the contract (no overflow with a byte above the
// base, against the exact arithmetic on 128 bits), every precondition
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

 // the edges
 using U64 = std::uint64_t;
 const std::string ff( 8 , char( 0xFF ) );
 CHECK( bin_bit( "A" , 0 ) == 0 && bin_bit( "A" , 1 ) == 1 &&
        bin_bit( "A" , 7 ) == 1 && bin_bit( "A" , 8 ) == 0 &&
        bin_bit( "" , 0 ) == 0 );
 CHECK( int_bin( "" ) == 0 && int_bin( "A" ) == 65 &&
        int_bin( ff ) == std::numeric_limits< U64 >::max() );
 CHECK( hash_extraction( "" , 63 , 0 ) == 0 );
 CHECK( hash_extraction( "Alberto" , 16 , 56 ) == 0 );   // past the end
 CHECK( hash_extraction( ff , 63 , 0 ) == ( 1ull << 63 ) - 1 );
 CHECK( hash_extraction( ff , 1 , 63 ) == 1 &&
        hash_extraction( ff , 1 , 64 ) == 0 );
 CHECK( hash_xor( "" , 16 ) == 0 && hash_xor( "A" , 63 ) == 65ull << 55 );
 CHECK( hash_xor( "AA" , 8 ) == 0 && hash_xor( ff , 1 ) == 0 );
 CHECK( hash_division( "Alberto" , 1 ) == 0 &&
        hash_division( "" , 383 , 256 , 1000 ) == 1000 % 383 );
 // a byte above the base, with m at the largest the precondition allows
 // and the leading digit m - 1: Horner must not overflow, against the
 // exact arithmetic modulo m (m < 2^63, so that a + a never overflows; no
 // integers of 128 bits, which MSVC lacks); m = ( 2^64 - 1 ) / 3 in base
 // 3, which overflowed with ( m - 1 ) 3 + 255, is refused
 auto addmod = []( U64 a , U64 b , U64 m ) {     // a , b < m
  return( a >= m - b ? a - ( m - b ) : a + b ); };
 auto mulmod = [ & ]( U64 a , U64 b , U64 m ) {  // a < m
  U64 r = 0;
  for( ; b ; b >>= 1 , a = addmod( a , a , m ) )
   if( b & 1 ) r = addmod( r , a , m );
  return( r ); };
 for( U64 base : { 3ull , 256ull } ) {
  const U64 m = ( std::numeric_limits< U64 >::max() - 255 ) / base + 1;
  U64 e = m - 1;
  for( unsigned char c : ff )
   e = addmod( mulmod( e , base , m ) , U64( c ) % m , m );
  CHECK( hash_division( ff , m , base , m - 1 ) == U64( e ) );
  CHECK_THROWS( (void) hash_division( ff , m + 1 , base ) ,
                contract_violation );
  }
 CHECK_THROWS( (void) hash_division( ff ,
                                     std::numeric_limits< U64 >::max() / 3 ,
                                     3 ) , contract_violation );
 CHECK( hash_multiplication( 12345 , 0 ) == 0 &&
        hash_multiplication( 0 , 1000 ) == 0 &&
        hash_multiplication( 12345 , 1000 , 0 ) == 0 &&
        hash_multiplication( 12345 , 1 ) == 0 );
 CHECK( hash_fibonacci( 0 , 63 ) == 0 && hash_fibonacci( 1 , 1 ) == 1 &&
        hash_fibonacci( 1 , 63 ) == 0x9E3779B97F4A7C15ull >> 1 );
 CHECK( hash_fibonacci( std::numeric_limits< U64 >::max() , 63 ) ==
        ( 0ull - 0x9E3779B97F4A7C15ull ) >> 1 );
 // i^2 wraps modulo 2^64: ( 2^64 - 1 )^2 = 1, 2^32 squared is 0
 CHECK( hash_midsquare( std::numeric_limits< U64 >::max() , 16 ) == 0 &&
        hash_midsquare( 1ull << 32 , 32 ) == 0 );
 CHECK( hash_midsquare( 1ull << 16 , 32 ) == 1ull << 16 &&
        hash_midsquare( 1ull << 16 , 1 ) == 1 );

 CHECK_THROWS( (void) hash_extraction( "a" , 0 , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) hash_extraction( "a" , 64 , 0 ) ,
               contract_violation );
 CHECK_THROWS( (void) hash_xor( "a" , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_xor( "a" , 64 ) , contract_violation );
 CHECK_THROWS( (void) hash_division( "a" , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_division( "a" , 7 , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_multiplication( 1 , 7 , 1 ) ,
               contract_violation );
 CHECK_THROWS( (void) hash_multiplication( 1 , 7 , -0.5L ) ,
               contract_violation );
 CHECK_THROWS( (void) hash_fibonacci( 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_fibonacci( 1 , 64 ) , contract_violation );
 CHECK_THROWS( (void) hash_midsquare( 1 , 0 ) , contract_violation );
 CHECK_THROWS( (void) hash_midsquare( 1 , 33 ) , contract_violation );
 CHECK_THROWS( (void) int_bin( "123456789" ) , contract_violation );
 }

// the simple uniformity by the chi-square test: the division by a prime
// spreads random strings as a uniform distribution would (the statistic
// has mean m - 1 and deviation sqrt( 2 ( m - 1 ) )), the extraction of the
// first bits sends keys with a common prefix to one place; the degenerate
// ends of the statistic: all the keys in one place give its largest value
// n ( m - 1 ), the exactly uniform counts give 0
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
 // the degenerate ends
 std::vector< long > one( m ) , flat( m , n / m );
 one[ hash_division( "key" , m ) ] = n;
 CHECK( std::abs( chi2( one , n ) - double( n ) * ( m - 1 ) ) < 1e-6 * n * m );
 CHECK( chi2( flat , ( n / m ) * m ) == 0 );
 }

/*--------------------------------------------------------------------------*/

// Figura 7.6: a deleted place does not stop the inspection, and an
// insertion reuses it; the edges: the full table (a failed insertion
// changes nothing), the table of deleted places only, a key updated past
// a deleted place, the empty table, the table of one place
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
 CHECK( u.size() == 5 && ! u.contains( 5 ) );   // nothing changed
 for( long k = 0 ; k < 5 ; ++k )
  u.remove( k );
 CHECK( u.empty() && ! u.lookup( 5 ) && u.probes() == 5 );
 u.insert( 5 , 5 );
 CHECK( u.lookup( 5 ) == 5 );

 // a key inserted again while a deleted place precedes it on its
 // inspection is updated where it is, the deleted place stays deleted
 Open w( 7 , Probing::linear() );
 for( long k : { 0 , 7 , 14 } )
  w.insert( k , k );
 w.remove( 7 );
 w.insert( 14 , 99 );
 CHECK( w.size() == 2 && w.lookup( 14 ) == 99 );
 CHECK( w.slot( 1 ) == Open::Slot::DELETED && w.keyAt( 2 ) == 14 );
 w.remove( 7 );                        // absent twice: nothing changes
 w.remove( 42 );
 CHECK( w.size() == 2 && w.slot( 1 ) == Open::Slot::DELETED );

 // the empty table: an unsuccessful search examines the one nil place
 Open e( 7 , Probing::linear() );
 CHECK( ! e.lookup( 3 ) && e.probes() == 1 && ! e.contains( 3 ) );
 e.remove( 3 );
 CHECK( e.empty() && e.begin() == e.end() );

 // one place: full after one key, a failed insertion leaves the table as
 // it was, a deleted place is reused
 Open one( 1 , Probing::linear() );
 one.insert( 8 , 8 );
 CHECK_THROWS( one.insert( 9 , 9 ) , std::overflow_error );
 CHECK( one.size() == 1 && one.lookup( 8 ) == 8 && ! one.contains( 9 ) );
 one.insert( 8 , 80 );                 // the key that is there: updated
 CHECK( one.lookup( 8 ) == 80 && one.probes() == 1 );
 one.remove( 8 );
 CHECK( ! one.lookup( 9 ) && one.probes() == 1 );
 one.insert( 9 , 9 );
 CHECK( one.size() == 1 && one.lookup( 9 ) == 9 );
 }

// every table against std::map, with and without restructuring; the
// edges: a single key, every key in the same place, a linear step of -1
// (the scan wraps around the start of the vector), the restructured
// tables of one place, a linear step that is a multiple of the primes 17
// and 37 that the doublings of 7 reach (the restructuring skips the sizes
// the inspection does not work with, and stays on the primes), and the
// preconditions of the constructors (m = 0, an inspection that does not
// work with m)
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
                  Probing::linear( -1 ) , Probing::quadratic( 1 , 1 ) ,
                  Probing::double_hashing() } )
   {
   OpenHashTable< long , long , Mix > d( m , p , r );
   against_map( d , 1000 , 20000 , seed++ );
   }
  OpenHashTable< long , long , Mix > e( r ? 8 : 4096 ,
                                         Probing::double_hashing() , r );
  against_map( e , 1000 , 20000 , seed++ );
  OpenHashTable< long , long , Mix > f( r ? 8 : 4096 ,
                                         Probing::triangular() , r );
  against_map( f , 1000 , 20000 , seed++ );
  OpenHashTable< long , long , Mix > g( r ? 10 : 4000 ,
                                         Probing::pseudo_random( 3 ) , r );
  against_map( g , 1000 , 20000 , seed++ );

  // the edges
  ChainedHashTable< long , long , Chain::SORTED , Mix > h( 7 , r );
  against_map( h , 1 , 2000 , seed++ );
  OpenHashTable< long , long , Mix > i( 7 , Probing::linear() , r );
  against_map( i , 1 , 2000 , seed++ );
  ChainedHashTable< long , long , Chain::MOVE_TO_FRONT , Zero > z( 7 , r );
  against_map( z , 200 , 5000 , seed++ );
  OpenHashTable< long , long , Zero > y( r ? 7 : 401 ,
                                          Probing::double_hashing() , r );
  against_map( y , 200 , 5000 , seed++ );
  }
 // the restructured tables of one place, which double to 2
 ChainedHashTable< long , long , Chain::FRONT , Mix > c1( 1 , true );
 against_map( c1 , 1000 , 20000 , seed++ );
 for( auto p : { Probing::linear() , Probing::double_hashing() ,
                 Probing::pseudo_random() } ) {
  OpenHashTable< long , long , Mix > o1( 1 , p , true );
  against_map( o1 , 1000 , 20000 , seed++ );
  CHECK( o1.capacity() >= 1 );
  }
 // a linear step that is a multiple of 17 and 37
 OpenHashTable< long , long , Mix > s( 7 , Probing::linear( 17 * 37 ) ,
                                        true );
 for( long k = 0 ; k < 100 ; ++k )
  s.insert( k , k );
 CHECK( s.size() == 100 && is_prime( s.capacity() ) );
 CHECK( s.capacity() % 17 != 0 && s.capacity() % 37 != 0 );
 for( long k = 0 ; k < 100 ; ++k )
  CHECK( s.lookup( k ) == k );
 for( long k = 0 ; k < 100 ; ++k )
  s.remove( k );
 CHECK( s.empty() && s.capacity() == 7 );
 against_map( s , 1000 , 20000 , seed++ );

 CHECK_THROWS( ( Open( 0 , Probing::linear() ) ) , contract_violation );
 CHECK_THROWS( ( Open( 1 , Probing::quadratic( 1 , 1 ) ) ) ,
               contract_violation );
 CHECK_THROWS( ( Open( 12 , Probing::triangular() ) ) ,
               contract_violation );
 CHECK_THROWS( ( Open( 8 , Probing::linear( 0 ) ) ) , contract_violation );
 CHECK_THROWS( ( Open( 8 , Probing::linear( 2 ) ) ) , contract_violation );
 CHECK_THROWS( ( Open( 8 , Probing::quadratic( 0 , 1 ) ) ) ,
               contract_violation );
 CHECK_THROWS( ( Open( 12 , Probing::double_hashing() ) ) ,
               contract_violation );
 CHECK_THROWS( ( Front( 0 ) ) , contract_violation );
 }

/*--------------------------------------------------------------------------*/

// the table of section 7.3: the averages of the accesses of the successful
// and of the unsuccessful searches against S( alpha ) and I( alpha ); the
// edge alpha = 0: an unsuccessful search accesses only the nil, as
// I( 0 ) = 1 says for all three, and a successful one in a table of one
// key accesses one place
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
 // the edge alpha = 0
 OpenHashTable< std::uint64_t , int , Mix > l( 101 , Probing::linear() );
 OpenHashTable< std::uint64_t , int , Mix > d( 101 ,
                                               Probing::double_hashing() );
 ChainedHashTable< std::uint64_t , int , Chain::FRONT , Mix > c( 101 );
 for( std::uint64_t k : { 0ull , 1ull , ~0ull } ) {
  CHECK( ! l.lookup( k ) && l.probes() == 1 );
  CHECK( ! d.lookup( k ) && d.probes() == 1 );
  CHECK( ! c.lookup( k ) && c.probes() == 1 );
  }
 // and a successful search in a table of one key accesses one place
 l.insert( 5 , 0 );
 c.insert( 5 , 0 );
 CHECK( l.lookup( 5 ) && l.probes() == 1 && c.lookup( 5 ) &&
        c.probes() == 1 );
 }

/*--------------------------------------------------------------------------*/

// section 7.4: the load factor stays in its band, n insertions move at most
// 2 n pairs, so they cost at most 3 n, and a sequence that alternates an
// insertion and a removal at the threshold restructures only once; the
// edges: the initial size 1, which doubles at the first insertion of the
// open table and at the third of the chained one, and never goes below 1;
// a removal of an absent key does not shrink the table; with one halving
// per removal at most, the open table emptied (it halves when
// alpha < 1 / 8) stays at 4, the chained one (alpha < 1 / 2) is back to 1
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
 CHECK( t.empty() && t.capacity() == 7 );

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

 // the edges
 OpenHashTable< long , long , Mix > o( 1 , Probing::double_hashing() , true );
 ChainedHashTable< long , long , Chain::FRONT , Mix > l( 1 , true );
 o.insert( 0 , 0 );
 CHECK( o.capacity() == 2 && o.restructures() == 1 );
 for( long x = 0 ; x < 3 ; ++x ) {
  CHECK( l.capacity() == 1 );
  l.insert( x , x );
  }
 CHECK( l.capacity() == 2 && l.restructures() == 1 );
 for( long x = 1 ; x < 1000 ; ++x ) {
  o.insert( x , x );
  l.insert( x , x );
  }
 const auto oc = o.capacity() , lc = l.capacity();
 for( long x = 1000 ; x < 2000 ; ++x ) {
  o.remove( x );
  l.remove( x );
  }
 CHECK( o.capacity() == oc && l.capacity() == lc );
 for( long x = 0 ; x < 1000 ; ++x ) {
  o.remove( x );
  l.remove( x );
  CHECK( o.capacity() >= 1 && l.capacity() >= 1 );
  }
 CHECK( o.empty() && o.capacity() == 4 && l.empty() &&
        l.capacity() == 1 );
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

// the insertions, the removals and the insertions of the exercise; the
// edge of the full table: Z (21 mod 11 = 10) wraps around to the last nil
// place 0, then no new letter finds a place, an old one does
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
 // the edge of the full table
 CHECK( t.size() == 10 );
 t.insert( 'Z' , 0 );
 CHECK( picture( t ) == "ZAOCPRETUIV" && t.probes() == 2 );
 CHECK_THROWS( t.insert( 'B' , 0 ) , std::overflow_error );
 CHECK( t.size() == 11 && ! t.contains( 'B' ) );
 t.insert( 'C' , 1 );
 CHECK( t.lookup( 'C' ) == 1 && ! t.lookup( 'B' ) && t.probes() == 11 );
 }

// the inspection ( k mod 11 + 3 i + i^2 ) mod 11 is not a permutation:
// the key 7 visits only the places 7, 0, 6, 3, 2 and 5, all full, while
// the place 8 is free, so the insertion of 7 fails; the other 10 keys need
// 12 accesses, 1.2 on average; the failed insertion changes nothing, and
// the search of 7, which never meets the nil 8, stops after m inspections
ADS_TEST( "ex7.2" )
{
 Open t( 11 , Probing::quadratic( 3 , 1 ) );
 for( long k : { 33 , 10 , 24 , 14 , 16 , 13 , 23 , 31 , 18 , 11 } )
  t.insert( k , k );
 CHECK_THROWS( t.insert( 7 , 7 ) , std::overflow_error );
 CHECK( t.slot( 8 ) == Open::Slot::NIL && t.size() == 10 );
 CHECK( ! t.lookup( 7 ) && t.probes() == 11 );
 t.remove( 7 );
 CHECK( t.size() == 10 );
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
// operation one access; the edges: the keys 0 and 255 at the ends, -1 and
// 256 out of the range
ADS_TEST( "ex7.3" )
{
 DirectTable< int > t( 0 , 255 );
 for( long k = 0 ; k < 256 ; ++k )
  t.insert( k , int( 255 - k ) );
 CHECK( t.size() == 256 && t.capacity() == 256 );
 for( long k = 0 ; k < 256 ; ++k )
  CHECK( t.lookup( k ) == 255 - k );
 // the edges
 t.remove( 0 );
 t.remove( 255 );
 CHECK( t.size() == 254 && ! t.lookup( 0 ) && ! t.lookup( 255 ) );
 CHECK_THROWS( (void) t.lookup( -1 ) , contract_violation );
 CHECK_THROWS( t.insert( 256 , 0 ) , contract_violation );
 }

// the procedures of the linear and of the quadratic inspection; the edges:
// the step 0 or a multiple of m stays on one place, a step congruent to -1
// visits all, LONG_MIN = 4 mod 12 visits 12 / 4; the inspection of the
// index i is that of i mod m; m = 1 has one place, every inspection works
// with it but the quadratic one (1 is not prime), and none with m = 0
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

 // the edges
 CHECK( visited( Probing::linear( 0 ) , 5 , 12 ) == 1 );
 CHECK( visited( Probing::linear( 24 ) , 5 , 12 ) == 1 );
 CHECK( visited( Probing::linear( -13 ) , 5 , 12 ) == 12 );
 CHECK( visited( Probing::linear( std::numeric_limits< long >::min() ) ,
                 5 , 12 ) == 3 );
 for( auto p : { Probing::linear( 5 ) , Probing::quadratic( 3 , 1 ) ,
                 Probing::double_hashing() } )
  for( std::size_t i = 0 ; i < 30 ; ++i )
   CHECK( p.at( 9 , i , 11 ) == p.at( 9 , i % 11 , 11 ) );
 for( auto p : { Probing::linear() , Probing::linear( 0 ) ,
                 Probing::double_hashing() , Probing::triangular() ,
                 Probing::pseudo_random() } ) {
  CHECK( p.works( 1 ) && ! p.works( 0 ) );
  for( std::size_t i = 0 ; i < 5 ; ++i )
   CHECK( p.at( 12345 , i , 1 ) == 0 );
  CHECK_THROWS( (void) p.at( 1 , 0 , 0 ) , contract_violation );
  }
 CHECK( ! Probing::quadratic( 1 , 1 ).works( 1 ) );
 CHECK( Probing::step( 77 , 1 ) == 1 && Probing::step( 0 , 7 ) == 1 &&
        Probing::step( 0 , 8 ) == 1 );
 CHECK_THROWS( (void) Probing::step( 1 , 0 ) , contract_violation );
 CHECK_THROWS( Probing::linear( 2 ).check( 8 ) , contract_violation );
 }

// the keys looked up often are brought to the head of their list; the
// edges: the head stays, an absent key and a failed removal move nothing,
// the last key goes to the head, a list of one key
ADS_TEST( "ex7.5" )
{
 ChainedHashTable< long , long , Chain::MOVE_TO_FRONT , Id > t( 1 );
 for( long k = 1 ; k <= 5 ; ++k )
  t.insert( k , k );
 CHECK( t.chain( 0 ) == std::vector< long >( { 5 , 4 , 3 , 2 , 1 } ) );
 CHECK( t.lookup( 1 ) == 1 );
 CHECK( t.chain( 0 ) == std::vector< long >( { 1 , 5 , 4 , 3 , 2 } ) );
 // the edges
 CHECK( t.lookup( 1 ) == 1 && t.probes() == 1 );
 CHECK( ! t.lookup( 9 ) && t.probes() == 6 );
 t.remove( 9 );
 CHECK( t.chain( 0 ) == std::vector< long >( { 1 , 5 , 4 , 3 , 2 } ) );
 CHECK( t.lookup( 2 ) == 2 && t.probes() == 5 );
 CHECK( t.chain( 0 ) == std::vector< long >( { 2 , 1 , 5 , 4 , 3 } ) );
 ChainedHashTable< long , long , Chain::MOVE_TO_FRONT , Id > one( 1 );
 CHECK( ! one.lookup( 0 ) && one.chain( 0 ).empty() );
 one.insert( 7 , 7 );
 CHECK( one.lookup( 7 ) == 7 &&
        one.chain( 0 ) == std::vector< long >( { 7 } ) );
 CHECK_THROWS( (void) one.chain( 1 ) , contract_violation );

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
// expected (the content stands for the content and the length); the
// edges: no file, no replica, all the files equal, the empty content
ADS_TEST( "ex7.6" )
{
 using Files = std::vector< std::pair< std::string , std::string > >;
 using Groups = std::vector< std::vector< std::string > >;
 auto replicas = []( const Files & files ) {
  ChainedHashTable< std::string , std::vector< std::string > > t( 7 ,
                                                                 true );
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
  Groups replicated;
  for( const auto & c : order )
   if( auto v = t.lookup( c ) ; v->size() > 1 )
    replicated.push_back( *v );
  return( replicated );
  };
 const Files files = {
  { "/a/x" , "alpha" } , { "/b/y" , "beta" } , { "/c/x" , "alpha" } ,
  { "/d" , "gamma" } , { "/e/f/g" , "beta" } , { "/h" , "alpha" } ,
  { "/i" , "alph" } };
 CHECK( replicas( files ) == Groups(
         { { "/a/x" , "/c/x" , "/h" } , { "/b/y" , "/e/f/g" } } ) );
 CHECK( replicas( Files() ).empty() );
 CHECK( replicas( { { "/a" , "x" } } ).empty() );
 CHECK( replicas( { { "/a" , "x" } , { "/b" , "y" } } ).empty() );
 Files same;
 Groups all( 1 );
 for( int i = 0 ; i < 100 ; ++i ) {
  same.emplace_back( "/" + std::to_string( i ) , "z" );
  all[ 0 ].push_back( "/" + std::to_string( i ) );
  }
 CHECK( replicas( same ) == all );
 CHECK( replicas( { { "/e" , "" } , { "/f" , "" } , { "/g" , "e" } } ) ==
        Groups( { { "/e" , "/f" } } ) );
 }

// a list of long keys with their hash: the hashes are compared first, and
// a key only when its hash is equal; the edges: the first and the last
// key, a key equal to the prefix of all, the empty key, the empty list,
// and a pair whose hash is equal but whose key is not, which costs one
// comparison of the keys more
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
 // the edges
 CHECK( search( prefix + "0" , false , a ) == 0 && a == 1 );
 CHECK( search( prefix + "499" , true , b ) == 499 && b == 1 );
 CHECK( search( prefix , false , a ) == -1 && a == 500 );
 CHECK( search( "" , true , b ) == -1 && b == 0 );
 const auto h = std::hash< std::string >()( prefix + "321" );
 L.insert( L.begin() , std::pair< std::string , std::size_t >( "y" , h ) );
 CHECK( search( prefix + "321" , true , b ) == 322 && b == 2 );
 L.clear();
 CHECK( search( prefix + "321" , true , b ) == -1 && b == 0 );
 CHECK( search( prefix + "321" , false , a ) == -1 && a == 0 );
 }

// with an alphabet of 2^p characters and m = 2^p - 1, 2^p = 1 mod m and so
// int( bin( k ) ) mod m is the sum of the characters mod m: swapping two of
// them, hence any permutation, does not change the hash; the edges: the
// empty key and a character 255 = 0 mod 255 count nothing, a single
// character is its own sum, a run of one character is a multiple of it
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
 // the edges
 const std::string ff( 7 , char( 0xFF ) );
 CHECK( hash_division( "" , 255 ) == 0 && hash_division( ff , 255 ) == 0 );
 CHECK( hash_division( "a" + ff , 255 ) == 'a' &&
        hash_division( ff + "a" , 255 ) == 'a' );
 CHECK( hash_division( std::string( 100 , 'b' ) , 255 ) ==
        100u * 'b' % 255 );
 }

// the duplicates of an unsorted vector, removed in O( n ) expected time by
// a table of the elements already seen; the edges: the empty vector, all
// the elements equal, all distinct, the extremes of long
ADS_TEST( "ex7.9" )
{
 auto dedup = []( const std::vector< long > & A ) {
  OpenHashTable< long , bool , Mix > seen( 7 , Probing::double_hashing() ,
                                           true );
  std::vector< long > B;
  for( auto a : A )
   if( ! seen.contains( a ) ) {
    seen.insert( a , true );
    B.push_back( a );
    }
  return( B );
  };
 using LL = std::numeric_limits< long >;
 CHECK( dedup( {} ).empty() );
 CHECK( dedup( std::vector< long >( 1000 , 4 ) ) ==
        std::vector< long >( { 4 } ) );
 std::vector< long > D( 1000 );
 std::iota( D.begin() , D.end() , -500 );
 CHECK( dedup( D ) == D );
 CHECK( dedup( { LL::max() , LL::min() , 0 , LL::min() , LL::max() } ) ==
        std::vector< long >( { LL::max() , LL::min() , 0 } ) );

 std::mt19937 g( 9 );
 std::vector< long > A( 50000 );
 for( auto & a : A )
  a = g() % 20000;
 const auto B = dedup( A );
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
// end of its list); the successful search costs the same; the edges, on
// one list: keys in decreasing and in increasing order and the extremes
// of long stay sorted, a key below all stops at the head, one above all
// goes through the list to the nil, removing the head and the tail keeps
// the order, a key inserted again is not duplicated
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

 // the edges, on one list
 using LL = std::numeric_limits< long >;
 ChainedHashTable< long , long , Chain::SORTED , Id > o( 1 );
 CHECK( ! o.lookup( 0 ) && o.probes() == 1 );
 for( long k : { 9 , 7 , 5 , 3 , 1 , 2 , 4 , 6 , 8 , 10 } )
  o.insert( k , k );
 o.insert( LL::max() , 0 );
 o.insert( LL::min() , 0 );
 o.insert( 5 , 50 );
 std::vector< long > c = { LL::min() , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9 ,
                           10 , LL::max() };
 CHECK( o.chain( 0 ) == c && o.size() == 12 && o.lookup( 5 ) == 50 );
 CHECK( ! o.lookup( -7 ) && o.probes() == 2 );       // stops at 1
 CHECK( o.lookup( LL::min() ) == 0 && o.probes() == 1 );
 CHECK( ! o.lookup( 11 ) && o.probes() == 12 );      // stops at max
 o.remove( LL::min() );
 o.remove( LL::max() );
 o.remove( 0 );
 c.erase( c.begin() );
 c.pop_back();
 CHECK( o.chain( 0 ) == c && o.size() == 10 );
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
