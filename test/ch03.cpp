/*--------------------------------------------------------------------------*/
/*----------------------------- File ch03.cpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Tests of chapter 3, Tipi di dato e strutture di dati: the exercises 3.1 to
 * 3.8, each a test named after it, which cites the place of the book it
 * comes from. The procedures on sequences are written once, on any
 * realization of the SEQUENCE, and checked on List.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, chapter 3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf
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
#include "ADS/sequence/ArraySequence.h"
#include "ADS/sequence/List.h"

#include <cmath>
#include <numbers>
#include <vector>

/*--------------------------------------------------------------------------*/
/*------------------------------- USING ------------------------------------*/
/*--------------------------------------------------------------------------*/

using namespace ADS;

namespace {

/*--------------------------------------------------------------------------*/
/*-------------------------------- HELPERS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< Sequence S >
std::vector< typename S::Item > items( const S & s )
{
 std::vector< typename S::Item > v;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  v.push_back( s.read( p ) );
 return( v );
 }

bool near( double a , double b ) { return( std::abs( a - b ) <= 1e-12 ); }

/*--------------------------------------------------------------------------*/
/*------------------------- ESERCIZI 3.1 E 3.2 -----------------------------*/
/*--------------------------------------------------------------------------*/

/// Esercizi 3.1 and 3.2: the complex number, specified by its operations
/// (real and imaginary part, argument, modulus, construction from the
/// Cartesian and from the trigonometric form, sum, difference, product,
/// conjugate, quotient) and realized with a record
class Complex {
 double f_re , f_im;
 Complex( double re , double im ) : f_re( re ) , f_im( im ) {}

 public:

 static Complex cartesian( double re , double im ) {
  return( Complex( re , im ) );
  }
 /// \f$ \rho ( \cos \theta + i \sin \theta ) \f$
 static Complex trigonometric( double rho , double theta ) {
  return( Complex( rho * std::cos( theta ) , rho * std::sin( theta ) ) );
  }
 double re( void ) const { return( f_re ); }
 double im( void ) const { return( f_im ); }
 double modulus( void ) const { return( std::hypot( f_re , f_im ) ); }
 double argument( void ) const { return( std::atan2( f_im , f_re ) ); }
 Complex conjugate( void ) const { return( Complex( f_re , - f_im ) ); }
 Complex operator+( const Complex & z ) const {
  return( Complex( f_re + z.f_re , f_im + z.f_im ) );
  }
 Complex operator-( const Complex & z ) const {
  return( Complex( f_re - z.f_re , f_im - z.f_im ) );
  }
 Complex operator*( const Complex & z ) const {
  return( Complex( f_re * z.f_re - f_im * z.f_im ,
                   f_re * z.f_im + f_im * z.f_re ) );
  }
 /// \f$ z / w = z \bar{w} / |w|^2 \f$, w not 0
 Complex operator/( const Complex & w ) const {
  const double d = w.f_re * w.f_re + w.f_im * w.f_im;
  if( d == 0 )
   throw( std::domain_error( "Complex::operator/: division by 0" ) );
  const Complex n = *this * w.conjugate();
  return( Complex( n.f_re / d , n.f_im / d ) );
  }
 };

}  // end( namespace )

// Esercizi 3.1 and 3.2: the operations of the complex numbers
ADS_TEST( "ex3.1-3.2" )
{
 const auto z = Complex::cartesian( 1 , 2 );
 const auto w = Complex::cartesian( 3 , -1 );
 const auto p = z * w;   // ( 1 + 2i )( 3 - i ) = 5 + 5i
 CHECK( near( p.re() , 5 ) && near( p.im() , 5 ) );
 const auto q = p / w;   // back to z
 CHECK( near( q.re() , 1 ) && near( q.im() , 2 ) );
 const auto s = z + w , d = z - w;
 CHECK( near( s.re() , 4 ) && near( s.im() , 1 ) );
 CHECK( near( d.re() , -2 ) && near( d.im() , 3 ) );
 CHECK( near( z.conjugate().im() , -2 ) );
 const auto u = Complex::trigonometric( 2 , std::numbers::pi / 2 );   // 2i
 CHECK( near( u.re() , 0 ) && near( u.im() , 2 ) );
 CHECK( near( u.modulus() , 2 ) );
 CHECK( near( u.argument() , std::numbers::pi / 2 ) );
 CHECK_THROWS( z / Complex::cartesian( 0 , 0 ) , std::domain_error );
 }

// Soluzione Esercizio 3.3: w[ i , j , h ] of a n x m x k matrix stored by
// columns, the first index varying first, is v[ p ] with p = i + j n + h n m
ADS_TEST( "ex3.3" )
{
 const int n = 3 , m = 4 , k = 2;
 std::vector< int > v;
 for( int h = 0 ; h < k ; ++h )    // by columns: i varies first
  for( int j = 0 ; j < m ; ++j )
   for( int i = 0 ; i < n ; ++i )
    v.push_back( 100 * i + 10 * j + h );
 for( int i = 0 ; i < n ; ++i )
  for( int j = 0 ; j < m ; ++j )
   for( int h = 0 ; h < k ; ++h )
    CHECK( v[ i + j * n + h * n * m ] == 100 * i + 10 * j + h );
 }

// Esercizio 3.4: w[ i_1 , ... , i_k ] of a matrix with min_j <= i_j <= max_j
// stored by rows, the last index varying first, is v[ p ] with
// p = sum_j ( i_j - min_j ) prod_{l > j} ( max_l - min_l + 1 )
ADS_TEST( "ex3.4" )
{
 const std::vector< int > lo = { -1 , 2 , 0 } , hi = { 1 , 4 , 3 };
 auto p = [ & ]( const std::vector< int > & idx ) {
  long pos = 0;
  for( std::size_t j = 0 ; j < idx.size() ; ++j )
   pos = pos * ( hi[ j ] - lo[ j ] + 1 ) + ( idx[ j ] - lo[ j ] );
  return( pos );
  };
 long expected = 0;   // by rows: the visit in this order fills v[ 0 , ... ]
 for( int a = lo[ 0 ] ; a <= hi[ 0 ] ; ++a )
  for( int b = lo[ 1 ] ; b <= hi[ 1 ] ; ++b )
   for( int c = lo[ 2 ] ; c <= hi[ 2 ] ; ++c )
    CHECK( p( { a , b , c } ) == expected++ );
 }

namespace {

/// Esercizio 3.5: the polynomial of degree at most k, with its coefficients
/// a_0 , ... , a_k, evaluated by the rule of Horner
class Polynomial {
 std::vector< double > f_a;
 public:
 explicit Polynomial( int k ) : f_a( k + 1 , 0.0 ) {}
 int degree( void ) const { return( int( f_a.size() ) - 1 ); }
 void set( int i , double a ) { f_a[ i ] = a; }
 double coefficient( int i ) const {
  return( i < int( f_a.size() ) ? f_a[ i ] : 0.0 );
  }
 /// \f$ a_0 + x ( a_1 + x ( a_2 + \cdots ) ) \f$
 double operator()( double x ) const {
  double v = 0;
  for( int i = degree() ; i >= 0 ; --i )
   v = v * x + f_a[ i ];
  return( v );
  }
 Polynomial operator+( const Polynomial & q ) const {
  Polynomial r( std::max( degree() , q.degree() ) );
  for( int i = 0 ; i <= r.degree() ; ++i )
   r.f_a[ i ] = coefficient( i ) + q.coefficient( i );
  return( r );
  }
 Polynomial operator-( const Polynomial & q ) const {
  Polynomial r( std::max( degree() , q.degree() ) );
  for( int i = 0 ; i <= r.degree() ; ++i )
   r.f_a[ i ] = coefficient( i ) - q.coefficient( i );
  return( r );
  }
 Polynomial operator*( const Polynomial & q ) const {
  Polynomial r( degree() + q.degree() );
  for( int i = 0 ; i <= degree() ; ++i )
   for( int j = 0 ; j <= q.degree() ; ++j )
    r.f_a[ i + j ] += f_a[ i ] * q.f_a[ j ];
  return( r );
  }
 };

}  // end( namespace )

// Esercizio 3.5: ( 1 + x ) ( 1 - x ) = 1 - x^2, and the other operations
ADS_TEST( "ex3.5" )
{
 Polynomial p( 1 ) , q( 1 );
 p.set( 0 , 1 ); p.set( 1 , 1 );    // 1 + x
 q.set( 0 , 1 ); q.set( 1 , -1 );   // 1 - x
 const auto r = p * q;
 CHECK( r.degree() == 2 );
 CHECK( near( r.coefficient( 0 ) , 1 ) && near( r.coefficient( 1 ) , 0 ) &&
        near( r.coefficient( 2 ) , -1 ) );
 for( double x : { -2.0 , 0.0 , 0.5 , 3.0 } ) {
  CHECK( near( r( x ) , 1 - x * x ) );
  CHECK( near( ( p + q )( x ) , 2 ) );
  CHECK( near( ( p - q )( x ) , 2 * x ) );
  }
 Polynomial c( 0 );   // a constant
 c.set( 0 , 4 );
 CHECK( near( c( 10 ) , 4 ) );
 }

namespace {

// Soluzione Esercizio 3.6: stampaDifferenze(), the items equal to the
// difference of the one before and the one after; three positions are
// moved together along the sequence
template< Sequence S >
std::vector< typename S::Item > stampaDifferenze( const S & L )
{
 std::vector< typename S::Item > out;
 auto p = L.head();
 auto q = L.next( p );
 auto r = L.next( q );
 while( ! L.finished( r ) ) {
  if( L.read( q ) == L.read( p ) - L.read( r ) )
   out.push_back( L.read( q ) );
  p = q;
  q = r;
  r = L.next( r );
  }
 return( out );
 }

// Esercizio 3.7: the odd items before the even ones, each group in its first
// order, in O( n ) time: the even ones are moved at the end as they come,
// up to the last item of the original sequence. The positions must not
// change when items are inserted or removed elsewhere, as in List; on
// ArraySequence they do, and insert() and remove() take O( n ) time anyway
template< Sequence S >
void oddBeforeEven( S & L )
{
 if( L.isEmpty() )
  return;
 const auto last = L.tail();
 auto p = L.head();
 bool done = false;
 while( ! done ) {
  done = ( p == last );
  const auto v = L.read( p );
  if( v % 2 == 0 ) {
   L.insert( L.next( L.tail() ) , v );
   p = L.remove( p );
   }
  else
   p = L.next( p );
  }
 }

// Esercizio 3.8: recursively, each item gets its rank, the sum of its value
// and of the values of the items after it; O( n ) time
template< Sequence S >
typename S::Item rango( S & L , typename S::Pos p )
{
 if( L.finished( p ) )
  return( 0 );
 const auto r = L.read( p ) + rango( L , L.next( p ) );
 L.write( p , r );
 return( r );
 }

}  // end( namespace )

// Esercizio 3.6: L = 5, 2, 3, 1, 2 prints 2, 1
ADS_TEST( "ex3.6" )
{
 using V = std::vector< int >;
 CHECK( stampaDifferenze( List< int >{ 5 , 2 , 3 , 1 , 2 } ) ==
        V( { 2 , 1 } ) );
 CHECK( stampaDifferenze( ArraySequence< int >{ 5 , 2 , 3 , 1 , 2 } ) ==
        V( { 2 , 1 } ) );
 CHECK( stampaDifferenze( List< int >{ 1 , 1 , 1 } ).empty() );
 }

// Esercizio 3.7: L = 3, 7, 8, 1, 4 becomes 3, 7, 1, 8, 4
ADS_TEST( "ex3.7" )
{
 using V = std::vector< int >;
 List< int > L{ 3 , 7 , 8 , 1 , 4 };
 oddBeforeEven( L );
 CHECK( items( L ) == V( { 3 , 7 , 1 , 8 , 4 } ) );
 List< int > E{ 2 , 4 , 6 };   // all even: unchanged
 oddBeforeEven( E );
 CHECK( items( E ) == V( { 2 , 4 , 6 } ) );
 List< int > Z;
 oddBeforeEven( Z );
 CHECK( Z.isEmpty() );
 }

// Esercizio 3.8: L = 3, 2, 5 becomes 10, 7, 5
ADS_TEST( "ex3.8" )
{
 using V = std::vector< int >;
 List< int > L{ 3 , 2 , 5 };
 CHECK( rango( L , L.head() ) == 10 );
 CHECK( items( L ) == V( { 10 , 7 , 5 } ) );
 ArraySequence< int > A{ 1 };
 rango( A , A.head() );
 CHECK( items( A ) == V( { 1 } ) );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- MAIN ------------------------------------*/
/*--------------------------------------------------------------------------*/

int main( int argc , char ** argv )
{
 return( ADS::test::main( argc , argv ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- End File ch03.cpp ---------------------------*/
/*--------------------------------------------------------------------------*/
