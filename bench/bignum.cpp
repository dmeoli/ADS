/*--------------------------------------------------------------------------*/
/*---------------------------- File bignum.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the integers of any length and of the rationals:
 * BigInteger and Rational of ADS against boost::multiprecision::cpp_int and
 * boost::rational and, if GMP is found (ADS_HAS_GMP), against mpz_class and
 * mpq_class, the reference of the field, and, if FLINT is found
 * (ADS_HAS_FLINT), against its fmpz_t and fmpq_t. The cases are the
 * product of two numbers of n digits of 32 bits (ADS by Algorithm M, by
 * Karatsuba and by the FFT), the square of one, the quotient of 2n digits
 * by n, the conversion of n digits to decimal, the greatest common
 * divisor of two numbers of n digits with a common factor of 3, the
 * integer square root of 2n digits, and the harmonic number \f$ H_n \f$
 * summed as a rational. Built with
 * ADS_WITH_GMP, it times BigInteger and Rational alone, on the backend of
 * GMP [see BigInteger.h], as "ADS::BigInteger GMP" and "ADS::Rational GMP",
 * the others coming from the build without it.
 *
 * Each cell is the median of r runs, in nanoseconds per operation (one
 * product, one quotient, one conversion, one sum of n fractions), a run
 * of the others repeating the operation 4096 / n times, so that the
 * short ones last many ticks of the clock; the contracts are off
 * (ADS_CONTRACTS=0).
 *
 *   bignum [ -r runs ] [ -s seed ] n1 n2 ... *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

#include "ADS/numeric/BigInteger.h"
#include "ADS/numeric/Rational.h"

#if __has_include( <boost/multiprecision/cpp_int.hpp> )
 #include <boost/multiprecision/cpp_int.hpp>
 #include <boost/rational.hpp>
 #define ADS_HAS_BOOST 1
#endif

#ifdef ADS_HAS_GMP
 #include <gmpxx.h>
#endif

#ifdef ADS_HAS_FLINT
 #include <flint/fmpq.h>
 #include <flint/fmpz.h>
#endif

#ifdef ADS_WITH_GMP                         // the backend alone
 #undef ADS_HAS_BOOST
 #undef ADS_HAS_GMP
 #undef ADS_HAS_FLINT
 #define ADS_WHO " GMP"
#else
 #define ADS_WHO ""
#endif

#include <algorithm>
#include <string>

using namespace ADS::bench;

namespace {

/// a random hexadecimal number of n digits of 32 bits
std::string hex( std::size_t n , std::mt19937_64 & g )
{
 std::string s( 8 * n , '0' );
 for( auto & c : s ) c = "0123456789abcdef"[ g() % 16 ];
 s[ 0 ] = '1';
 return( s );
 }

/// the cases on the number type N, made from a hexadecimal string by
/// make( s ) and written in decimal by dec( x ), the product by mul( x , y );
/// with all, also the square, the gcd by gcd( x , y ) and the square root
/// by root( x ); each run repeats the operation k times, k n about 2^12,
/// so that the short ones last many ticks of the clock
template< class N , class M , class D , class Mul , class G , class S >
void run( const std::string & who , std::size_t n , const std::string & a ,
          const std::string & b , const std::string & c , int r , M && make ,
          D && dec , Mul && mul , bool all , G && gcd , S && root )
{
 const N x = make( a ) , y = make( b ) , z = make( c );   // c has 2n digits
 const N x3 = mul( x , make( "3" ) ) , y3 = mul( y , make( "3" ) );
 const std::size_t k = std::max< std::size_t >( 1 , 4096 / n );
 auto cell = [ & ]( const char * name , auto && op ) {
  print( "bignum" , name , n , who , median( [](){ return( 0 ); } ,
          [ & ]( int ) {
           for( std::size_t i = 0 ; i < k ; ++i ) op(); } , r , k ) );
  };
 cell( "product" , [ & ] { const N p = mul( x , y );
                           sink = sink + ( p != 0 ); } );
 if( all ) cell( "square" , [ & ] { const N p = mul( x , x );
                                    sink = sink + ( p != 0 ); } );
 cell( "quotient 2n/n" , [ & ] { const N q = z / y;
                                 sink = sink + ( q != 0 ); } );
 cell( "to decimal" , [ & ] { sink = sink + dec( x ).size(); } );
 if( ! all ) return;
 cell( "gcd" , [ & ] { const N g = gcd( x3 , y3 );
                       sink = sink + ( g != 0 ); } );
 cell( "square root 2n" , [ & ] { const N s = root( z );
                                  sink = sink + ( s != 0 ); } );
 }

#ifdef ADS_HAS_FLINT
/// fmpz_t of FLINT with what run() asks of a number
struct Fmpz {
 fmpz_t v;
 Fmpz( void ) { fmpz_init( v ); }
 explicit Fmpz( const std::string & s , int base = 16 ) {
  fmpz_init( v );
  fmpz_set_str( v , s.c_str() , base );
  }
 Fmpz( const Fmpz & o ) { fmpz_init_set( v , o.v ); }
 Fmpz & operator=( const Fmpz & o ) {
  fmpz_set( v , o.v );
  return( *this );
  }
 ~Fmpz( void ) { fmpz_clear( v ); }
 friend Fmpz operator/( const Fmpz & a , const Fmpz & b ) {
  Fmpz q;
  fmpz_tdiv_q( q.v , a.v , b.v );
  return( q );
  }
 bool operator!=( int x ) const { return( ! fmpz_equal_si( v , x ) ); }
 };

/// fmpq_t of FLINT with what harmonic() asks of a fraction
struct Fmpq {
 fmpq_t v;
 Fmpq( long p , long q ) {
  fmpq_init( v );
  fmpq_set_si( v , p , ulong( q ) );
  }
 Fmpq( const Fmpq & o ) {
  fmpq_init( v );
  fmpq_set( v , o.v );
  }
 Fmpq & operator=( const Fmpq & o ) {
  fmpq_set( v , o.v );
  return( *this );
  }
 ~Fmpq( void ) { fmpq_clear( v ); }
 friend Fmpq operator+( const Fmpq & a , const Fmpq & b ) {
  Fmpq s( 0 , 1 );
  fmpq_add( s.v , a.v , b.v );
  return( s );
  }
 friend bool operator!=( const Fmpq & a , const Fmpq & b ) {
  return( ! fmpq_equal( a.v , b.v ) );
  }
 };
#endif

/// the harmonic number H_n summed as a fraction of the type R
template< class R , class Make >
void harmonic( const std::string & who , std::size_t n , int r ,
               Make && frac )
{
 print( "bignum" , "harmonic H_n" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) {
          R h = frac( 0 , 1 );
          for( std::size_t i = 1 ; i <= n ; ++i )
           h = h + frac( 1 , long( i ) );
          sink = sink + ( h != frac( 0 , 1 ) ); } , r , 1 ) );
 }

}  // end( namespace )

int main( int argc , char ** argv )
{
 const Options o( argc , argv , { 16 , 256 , 1024 , 4096 , 32768 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  const std::string a = hex( n , g ) , b = hex( n , g ) , c = hex( 2 * n , g );
  const int r = o.runs;
  using ADS::BigInteger;
  auto adsMake = []( const std::string & s ) {
   return( BigInteger( s , 16 ) ); };
  auto adsDec = []( const BigInteger & x ) { return( x.toString() ); };
  auto adsMul = []( const auto & x , const auto & y ) { return( x * y ); };
  auto adsGcd = []( const auto & x , const auto & y ) {
   return( BigInteger::gcd( x , y ) ); };
  auto adsRoot = []( const auto & x ) { return( BigInteger::sqrt( x ) ); };
  run< BigInteger >( "ADS::BigInteger" ADS_WHO , n , a , b , c , r ,
                     adsMake , adsDec , adsMul , true , adsGcd , adsRoot );
  run< BigInteger >( "ADS::BigInteger" ADS_WHO " A/A" , n , a , b , c , r ,
                     adsMake , adsDec , adsMul , true , adsGcd , adsRoot );
#ifndef ADS_WITH_GMP
  run< BigInteger >( "ADS::BigInteger school" , n , a , b , c , r , adsMake ,
                     adsDec , []( const auto & x , const auto & y ) {
                      return( BigInteger::multiplySchool( x , y ) ); } ,
                     false , adsGcd , adsRoot );
  run< BigInteger >( "ADS::BigInteger Karatsuba" , n , a , b , c , r ,
                     adsMake , adsDec , []( const auto & x , const auto & y ) {
                      return( BigInteger::multiplyKaratsuba( x , y ) ); } ,
                     false , adsGcd , adsRoot );
  run< BigInteger >( "ADS::BigInteger FFT" , n , a , b , c , r , adsMake ,
                     adsDec , []( const auto & x , const auto & y ) {
                      return( BigInteger::multiplyFFT( x , y ) ); } ,
                     false , adsGcd , adsRoot );
#endif
#ifdef ADS_HAS_BOOST
  using boost::multiprecision::cpp_int;
  run< cpp_int >( "boost::multiprecision::cpp_int" , n , a , b , c , r ,
                  []( const std::string & s ) {
                   return( cpp_int( "0x" + s ) ); } ,
                  []( const cpp_int & x ) { return( x.str() ); } ,
                  []( const auto & x , const auto & y ) {
                   return( cpp_int( x * y ) ); } , true ,
                  []( const auto & x , const auto & y ) {
                   return( cpp_int( boost::multiprecision::gcd( x , y ) ) ); } ,
                  []( const auto & x ) {
                   return( cpp_int( boost::multiprecision::sqrt( x ) ) ); } );
#endif
#ifdef ADS_HAS_GMP
  run< mpz_class >( "GMP mpz_class" , n , a , b , c , r ,
                    []( const std::string & s ) {
                     return( mpz_class( s , 16 ) ); } ,
                    []( const mpz_class & x ) { return( x.get_str( 10 ) ); } ,
                    []( const auto & x , const auto & y ) {
                     return( mpz_class( x * y ) ); } , true ,
                    []( const auto & x , const auto & y ) {
                     return( mpz_class( gcd( x , y ) ) ); } ,
                    []( const auto & x ) {
                     return( mpz_class( sqrt( x ) ) ); } );
#endif
#ifdef ADS_HAS_FLINT
  run< Fmpz >( "FLINT fmpz_t" , n , a , b , c , r ,
               []( const std::string & s ) { return( Fmpz( s ) ); } ,
               []( const Fmpz & x ) {
                char * t = fmpz_get_str( nullptr , 10 , x.v );
                std::string d( t );
                flint_free( t );
                return( d ); } ,
               []( const Fmpz & x , const Fmpz & y ) {
                Fmpz p;
                fmpz_mul( p.v , x.v , y.v );
                return( p ); } , true ,
               []( const Fmpz & x , const Fmpz & y ) {
                Fmpz g;
                fmpz_gcd( g.v , x.v , y.v );
                return( g ); } ,
               []( const Fmpz & x ) {
                Fmpz s;
                fmpz_sqrt( s.v , x.v );
                return( s ); } );
#endif
  const std::size_t h = 64 * n;             // the terms of H_n
  if( h > 300000 ) continue;
  harmonic< ADS::Rational >( "ADS::Rational" ADS_WHO , h , r ,
                             []( long p , long q ) {
                              return( ADS::Rational( p , q ) ); } );
#ifdef ADS_HAS_BOOST
  harmonic< boost::rational< cpp_int > >( "boost::rational<cpp_int>" , h , r ,
                                          []( long p , long q ) {
                                           return( boost::rational< cpp_int >(
                                                    p , q ) ); } );
#endif
#ifdef ADS_HAS_FLINT
  harmonic< Fmpq >( "FLINT fmpq_t" , h , r , []( long p , long q ) {
                     return( Fmpq( p , q ) ); } );
#endif
#ifdef ADS_HAS_GMP
  harmonic< mpq_class >( "GMP mpq_class" , h , r , []( long p , long q ) {
                          mpq_class x( p , q );
                          x.canonicalize();
                          return( x ); } );
#endif
  }
 return( 0 );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- End File bignum.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
