/*--------------------------------------------------------------------------*/
/*---------------------------- File bignum.cpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The benchmark of the integers of any length and of the rationals:
 * BigInteger and Rational of ADS against boost::multiprecision::cpp_int and
 * boost::rational and, if GMP is found (ADS_HAS_GMP), against mpz_class and
 * mpq_class, the reference of the field. The cases are the product of two
 * numbers of n digits of 32 bits (ADS by Algorithm M, by Karatsuba and by
 * the FFT), the
 * quotient of 2n digits by n, the conversion of n digits to decimal, and
 * the harmonic number \f$ H_n \f$ summed as a rational. Built with
 * ADS_WITH_GMP, it times BigInteger and Rational alone, on the backend of
 * GMP [see BigInteger.h], as "ADS::BigInteger GMP" and "ADS::Rational GMP",
 * the others coming from the build without it.
 *
 * Each cell is the median of r runs, in nanoseconds per operation (one
 * product, one quotient, one conversion, one sum of n fractions); the
 * contracts are off (ADS_CONTRACTS=0).
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

#ifdef ADS_WITH_GMP                         // the backend alone
 #undef ADS_HAS_BOOST
 #undef ADS_HAS_GMP
 #define ADS_WHO " GMP"
#else
 #define ADS_WHO ""
#endif

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

/// the four cases on the number type N, made from a hexadecimal string by
/// make( s ) and written in decimal by dec( x )
template< class N , class M , class D , class Mul >
void run( const std::string & who , std::size_t n , const std::string & a ,
          const std::string & b , const std::string & c , int r , M && make ,
          D && dec , Mul && mul )
{
 const N x = make( a ) , y = make( b ) , z = make( c );   // c has 2n digits
 print( "bignum" , "product" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { const N p = mul( x , y ); sink = sink + ( p != 0 ); } ,
         r , 1 ) );
 print( "bignum" , "quotient 2n/n" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { const N q = z / y; sink = sink + ( q != 0 ); } , r ,
         1 ) );
 print( "bignum" , "to decimal" , n , who , median( [](){ return( 0 ); } ,
         [ & ]( int ) { sink = sink + dec( x ).size(); } , r , 1 ) );
 }

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
 const Options o( argc , argv , { 16 , 256 , 4096 , 32768 } );
 for( std::size_t n : o.sizes ) {
  std::mt19937_64 g( o.seed );
  const std::string a = hex( n , g ) , b = hex( n , g ) , c = hex( 2 * n , g );
  const int r = o.runs;
  using ADS::BigInteger;
  auto adsMake = []( const std::string & s ) {
   return( BigInteger( s , 16 ) ); };
  auto adsDec = []( const BigInteger & x ) { return( x.toString() ); };
  run< BigInteger >( "ADS::BigInteger" ADS_WHO , n , a , b , c , r ,
                     adsMake , adsDec , []( const auto & x , const auto & y ) {
                      return( x * y ); } );
  run< BigInteger >( "ADS::BigInteger" ADS_WHO " A/A" , n , a , b , c , r ,
                     adsMake , adsDec , []( const auto & x , const auto & y ) {
                      return( x * y ); } );
#ifndef ADS_WITH_GMP
  run< BigInteger >( "ADS::BigInteger school" , n , a , b , c , r , adsMake ,
                     adsDec , []( const auto & x , const auto & y ) {
                      return( BigInteger::multiplySchool( x , y ) ); } );
  run< BigInteger >( "ADS::BigInteger Karatsuba" , n , a , b , c , r ,
                     adsMake , adsDec , []( const auto & x , const auto & y ) {
                      return( BigInteger::multiplyKaratsuba( x , y ) ); } );
  run< BigInteger >( "ADS::BigInteger FFT" , n , a , b , c , r , adsMake ,
                     adsDec , []( const auto & x , const auto & y ) {
                      return( BigInteger::multiplyFFT( x , y ) ); } );
#endif
#ifdef ADS_HAS_BOOST
  using boost::multiprecision::cpp_int;
  run< cpp_int >( "boost::multiprecision::cpp_int" , n , a , b , c , r ,
                  []( const std::string & s ) {
                   return( cpp_int( "0x" + s ) ); } ,
                  []( const cpp_int & x ) { return( x.str() ); } ,
                  []( const auto & x , const auto & y ) {
                   return( cpp_int( x * y ) ); } );
#endif
#ifdef ADS_HAS_GMP
  run< mpz_class >( "GMP mpz_class" , n , a , b , c , r ,
                    []( const std::string & s ) {
                     return( mpz_class( s , 16 ) ); } ,
                    []( const mpz_class & x ) { return( x.get_str( 10 ) ); } ,
                    []( const auto & x , const auto & y ) {
                     return( mpz_class( x * y ) ); } );
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
