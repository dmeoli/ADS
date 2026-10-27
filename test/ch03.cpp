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
 *         Università di Pisa \n
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

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <stdexcept>
#include <tuple>
#include <utility>
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

/// Exercises 3.1 and 3.2: the complex number, specified by its operations
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
 /// \f$ z / w = z \bar{w} / |w|^2 \f$, w not 0, by the method of Smith:
 /// w is first divided by its larger part, so that |w|^2 neither
 /// underflows to 0 for a tiny w nor overflows for a huge one
 Complex operator/( const Complex & w ) const {
  if( ( w.f_re == 0 ) && ( w.f_im == 0 ) )
   throw( std::domain_error( "Complex::operator/: division by 0" ) );
  if( std::abs( w.f_re ) >= std::abs( w.f_im ) ) {
   const double r = w.f_im / w.f_re , d = w.f_re + w.f_im * r;
   return( Complex( ( f_re + f_im * r ) / d , ( f_im - f_re * r ) / d ) );
   }
  const double r = w.f_re / w.f_im , d = w.f_re * r + w.f_im;
  return( Complex( ( f_re * r + f_im ) / d , ( f_im * r - f_re ) / d ) );
  }
 };

}  // end( namespace )

// Exercises 3.1 and 3.2: the operations of the complex numbers; the edges:
// 0 (modulus and argument 0, the division by 0, 0 divided by w), the
// argument on the four half-axes (pi on the negative reals), rho = 0, the
// division by a purely imaginary w, by a tiny w (|w|^2 underflows) and by
// a huge one (|w|^2 overflows), the modulus of huge parts (no overflow)
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
 CHECK_THROWS( Complex::cartesian( 0 , 0 ) / Complex::cartesian( -0.0 , 0 ) ,
               std::domain_error );
 const auto o = Complex::cartesian( 0 , 0 );
 CHECK( o.modulus() == 0 && o.argument() == 0 );
 const auto oz = o / z;
 CHECK( oz.re() == 0 && oz.im() == 0 );
 const double pi = std::numbers::pi;
 CHECK( near( Complex::cartesian( 3 , 0 ).argument() , 0 ) );
 CHECK( near( Complex::cartesian( -3 , 0 ).argument() , pi ) );
 CHECK( near( Complex::cartesian( 0 , 3 ).argument() , pi / 2 ) );
 CHECK( near( Complex::cartesian( 0 , -3 ).argument() , - pi / 2 ) );
 const auto t = Complex::trigonometric( 0 , 1.234 );
 CHECK( t.re() == 0 && t.im() == 0 );
 const auto v = Complex::trigonometric( 3 , - pi );     // -3
 CHECK( near( v.re() , -3 ) && near( v.modulus() , 3 ) );
 const auto i2 = z / Complex::cartesian( 0 , 2 );       // ( 1 + 2i ) / 2i
 CHECK( near( i2.re() , 1 ) && near( i2.im() , -0.5 ) );
 const auto tiny = z / Complex::cartesian( 1e-200 , 1e-200 );
 CHECK( std::abs( tiny.re() / 1.5e200 - 1 ) < 1e-12 &&
        std::abs( tiny.im() / 0.5e200 - 1 ) < 1e-12 );
 const auto huge = Complex::cartesian( 1e200 , 0 ) /
                   Complex::cartesian( 1e200 , 1e200 );  // ( 1 - i ) / 2
 CHECK( near( huge.re() , 0.5 ) && near( huge.im() , -0.5 ) );
 CHECK( std::abs( Complex::cartesian( 3e300 , 4e300 ).modulus() / 5e300 - 1 )
        < 1e-12 );
 const auto c = Complex::cartesian( 7 , 0 ).conjugate();
 CHECK( c.re() == 7 && c.im() == 0 );
 }

// Solution of exercise 3.3: w[ i , j , h ] of a n x m x k matrix stored by
// columns, the first index varying first, is v[ p ] with p = i + j n + h n m;
// the edges: the extents of 1 (1 x 1 x 1, a row, a column, a single layer),
// the first and the last place, and every place hit once
ADS_TEST( "ex3.3" )
{
 for( const auto & [ n , m , k ] : { std::tuple( 3 , 4 , 2 ) ,
        std::tuple( 1 , 1 , 1 ) , std::tuple( 1 , 5 , 1 ) ,
        std::tuple( 5 , 1 , 1 ) , std::tuple( 1 , 1 , 5 ) ,
        std::tuple( 2 , 3 , 1 ) } ) {
  std::vector< int > v;
  for( int h = 0 ; h < k ; ++h )    // by columns: i varies first
   for( int j = 0 ; j < m ; ++j )
    for( int i = 0 ; i < n ; ++i )
     v.push_back( 100 * i + 10 * j + h );
  std::vector< int > hit( v.size() , 0 );
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < m ; ++j )
    for( int h = 0 ; h < k ; ++h ) {
     const int p = i + j * n + h * n * m;
     CHECK( p >= 0 && p < int( v.size() ) );
     CHECK( v[ p ] == 100 * i + 10 * j + h );
     ++hit[ p ];
     }
  CHECK( std::count( hit.begin() , hit.end() , 1 ) == int( hit.size() ) );
  CHECK( v.front() == 0 &&
         v.back() == 100 * ( n - 1 ) + 10 * ( m - 1 ) + k - 1 );
  }
 }

// Exercise 3.4: w[ i_1 , ... , i_k ] of a matrix with min_j <= i_j <= max_j
// stored by rows, the last index varying first, is v[ p ] with
// p = sum_j ( i_j - min_j ) prod_{l > j} ( max_l - min_l + 1 ); the edges:
// k = 1, extents of 1 (min_j = max_j), all the bounds negative, bounds far
// from 0, and the first and the last place, against the visit by rows
ADS_TEST( "ex3.4" )
{
 using V = std::vector< int >;
 for( const auto & b : { std::pair( V{ -1 , 2 , 0 } , V{ 1 , 4 , 3 } ) ,
        std::pair( V{ 5 } , V{ 9 } ) , std::pair( V{ 7 } , V{ 7 } ) ,
        std::pair( V{ -4 , -9 , 3 } , V{ -2 , -9 , 3 } ) ,
        std::pair( V{ -3 , -3 , -3 , -3 } , V{ -2 , -1 , -3 , -2 } ) ,
        std::pair( V{ 1000000 , -1000000 } , V{ 1000002 , -999999 } ) } ) {
  const V & lo = b.first , & hi = b.second;
  auto p = [ & ]( const V & idx ) {
   long pos = 0;
   for( std::size_t j = 0 ; j < idx.size() ; ++j )
    pos = pos * ( hi[ j ] - lo[ j ] + 1 ) + ( idx[ j ] - lo[ j ] );
   return( pos );
   };
  long expected = 0;   // by rows: the visit in this order fills v[ 0 , ... ]
  V idx = lo;
  CHECK( p( lo ) == 0 );
  for( bool more = true ; more ; ) {      // the last index varies first
   CHECK( p( idx ) == expected++ );
   more = false;
   for( std::size_t j = idx.size() ; j-- > 0 ; )
    if( idx[ j ] < hi[ j ] ) { ++idx[ j ]; more = true; break; }
    else idx[ j ] = lo[ j ];
   }
  long size = 1;
  for( std::size_t j = 0 ; j < lo.size() ; ++j )
   size *= hi[ j ] - lo[ j ] + 1;
  CHECK( expected == size && p( hi ) == size - 1 );
  }
 }

namespace {

/// Exercise 3.5: the polynomial of degree at most k, with its coefficients
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

// Exercise 3.5: ( 1 + x ) ( 1 - x ) = 1 - x^2, and the other operations;
// the edges: the polynomial 0 (a constant 0 of degree 0), the product by 0
// and by a constant, operands of different degrees, a difference whose
// leading coefficients cancel (the degree is a bound, the coefficients 0),
// a coefficient beyond the degree (0), the value at x = 0 (a_0) and at a
// negative x, against the powers of x
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
 const Polynomial z( 0 );                       // 0
 CHECK( z.degree() == 0 && z( 5 ) == 0 && z.coefficient( 3 ) == 0 );
 const auto pz = p * z;
 CHECK( pz.degree() == 1 && pz( 7 ) == 0 && pz( -7 ) == 0 );
 const auto pc = p * c;                         // 4 + 4 x
 CHECK( near( pc.coefficient( 0 ) , 4 ) && near( pc.coefficient( 1 ) , 4 ) );
 const auto pp = p - p;                         // 0 x + 0, degree 1
 CHECK( pp.degree() == 1 && pp.coefficient( 0 ) == 0 &&
        pp.coefficient( 1 ) == 0 );
 Polynomial h( 4 );                             // x^4 - 2 x + 3
 h.set( 4 , 1 ); h.set( 1 , -2 ); h.set( 0 , 3 );
 CHECK( h.coefficient( 3 ) == 0 && h.coefficient( 5 ) == 0 );
 CHECK( near( h( 0 ) , 3 ) );
 for( double x : { -3.0 , -1.0 , -0.5 , 0.25 , 2.0 } ) {
  const double hx = x * x * x * x - 2 * x + 3;
  CHECK( near( h( x ) , hx ) );
  CHECK( near( ( h + p )( x ) , hx + 1 + x ) );  // degrees 4 and 1
  CHECK( near( ( p + h )( x ) , hx + 1 + x ) );
  CHECK( near( ( p - h )( x ) , 1 + x - hx ) );
  CHECK( std::abs( ( h * r )( x ) - hx * ( 1 - x * x ) ) <= 1e-9 );
  }
 CHECK( ( h + p ).degree() == 4 && ( p + h ).degree() == 4 &&
        ( h * r ).degree() == 6 );
 }

namespace {

// Solution of exercise 3.6: stampaDifferenze(), the items equal to the
// difference of the one before and the one after; three positions are
// moved together along the sequence; a sequence of less than three items
// prints nothing, and its end is checked before each position is advanced
template< Sequence S >
std::vector< typename S::Item > stampaDifferenze( const S & L )
{
 std::vector< typename S::Item > out;
 auto p = L.head();
 if( L.finished( p ) )
  return( out );
 auto q = L.next( p );
 if( L.finished( q ) )
  return( out );
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

// Exercise 3.7: the odd items before the even ones, each group in its first
// order, in O( n ) time: the even ones are moved at the end as they come,
// up to the last item of the original sequence. The positions must not
// change when items are inserted or removed elsewhere, as in List; on
// ArraySequence they do, and insert() and remove() take O( n ) time anyway
template< Sequence S >
void oddBeforeEven( S & L )
{
 if( L.empty() )
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

// Exercise 3.8: recursively, each item gets its rank, the sum of its value
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

// Exercise 3.6: L = 5, 2, 3, 1, 2 prints 2, 1; the edges: 0, 1 and 2 items
// (nothing, the first and the last item have no two neighbours; with one
// item the list of the book read its sentinel), exactly 3 items, every
// inner item printed, negative and zero items, against the definition
ADS_TEST( "ex3.6" )
{
 using V = std::vector< int >;
 CHECK( stampaDifferenze( List< int >{ 5 , 2 , 3 , 1 , 2 } ) ==
        V( { 2 , 1 } ) );
 CHECK( stampaDifferenze( ArraySequence< int >{ 5 , 2 , 3 , 1 , 2 } ) ==
        V( { 2 , 1 } ) );
 CHECK( stampaDifferenze( List< int >{ 1 , 1 , 1 } ).empty() );
 CHECK( stampaDifferenze( List< int >() ).empty() );
 CHECK( stampaDifferenze( ArraySequence< int >() ).empty() );
 CHECK( stampaDifferenze( List< int >{ 0 } ).empty() );
 CHECK( stampaDifferenze( ArraySequence< int >{ 0 } ).empty() );
 CHECK( stampaDifferenze( List< int >{ 0 , 0 } ).empty() );
 CHECK( stampaDifferenze( ArraySequence< int >{ 0 , 0 } ).empty() );
 CHECK( stampaDifferenze( List< int >{ 3 , 1 , 2 } ) == V( { 1 } ) );
 CHECK( stampaDifferenze( List< int >{ 0 , 0 , 0 , 0 } ) == V( { 0 , 0 } ) );
 CHECK( stampaDifferenze( List< int >{ -1 , -3 , 2 } ) == V( { -3 } ) );
 std::mt19937 g( 9 );
 for( int t = 0 ; t < 100 ; ++t ) {
  V v( g() % 8 );
  for( auto & x : v ) x = int( g() % 5 ) - 2;
  V want;
  for( std::size_t i = 1 ; i + 1 < v.size() ; ++i )
   if( v[ i ] == v[ i - 1 ] - v[ i + 1 ] ) want.push_back( v[ i ] );
  List< int > L;
  ArraySequence< int > A;
  for( int x : v ) {
   L.insert( L.next( L.tail() ) , x );
   A.insert( A.next( A.tail() ) , x );
   }
  CHECK( stampaDifferenze( L ) == want && stampaDifferenze( A ) == want );
  }
 }

// Exercise 3.7: L = 3, 7, 8, 1, 4 becomes 3, 7, 1, 8, 4; the edges: empty,
// one odd or one even item, all odd, all even, an even item first or last,
// negative items and 0, duplicates, against std::stable_partition
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
 CHECK( Z.empty() );
 for( const V & v : { V{ 1 } , V{ 2 } , V{ 1 , 3 , 5 } , V{ 2 , 1 } ,
                      V{ 1 , 2 } , V{ 4 , 3 , 2 , 1 } , V{ 0 , -1 , -2 , -3 } ,
                      V{ 2 , 2 , 1 , 1 , 2 } } ) {
  List< int > L2;                         // against a stable partition
  for( int x : v ) L2.insert( L2.next( L2.tail() ) , x );
  V want = v;
  std::stable_partition( want.begin() , want.end() ,
                         []( int x ) { return( x % 2 != 0 ); } );
  oddBeforeEven( L2 );
  CHECK( items( L2 ) == want );
  }
 }

// Exercise 3.8: L = 3, 2, 5 becomes 10, 7, 5; the edges: empty (the sum
// 0), one item, negative items and sums that cross 0, all zeros, and a
// list of 10^4 items (the depth of the recursion), against suffix sums
ADS_TEST( "ex3.8" )
{
 using V = std::vector< int >;
 List< int > L{ 3 , 2 , 5 };
 CHECK( rango( L , L.head() ) == 10 );
 CHECK( items( L ) == V( { 10 , 7 , 5 } ) );
 ArraySequence< int > A{ 1 };
 rango( A , A.head() );
 CHECK( items( A ) == V( { 1 } ) );
 List< int > E;
 ArraySequence< int > F;
 CHECK( rango( E , E.head() ) == 0 && E.empty() );
 CHECK( rango( F , F.head() ) == 0 && F.empty() );
 List< int > N{ -4 , 7 , -3 , 0 };
 CHECK( rango( N , N.head() ) == 0 );
 CHECK( items( N ) == V( { 0 , 4 , -3 , 0 } ) );
 List< int > Z{ 0 , 0 , 0 };
 CHECK( rango( Z , Z.head() ) == 0 && items( Z ) == V( 3 , 0 ) );
 List< long > B;
 for( long i = 0 ; i < 10000 ; ++i ) B.insert( B.next( B.tail() ) , i );
 CHECK( rango( B , B.head() ) == 10000L * 9999 / 2 );
 long i = 0;
 for( auto p = B.head() ; ! B.finished( p ) ; p = B.next( p ) , ++i )
  CHECK( B.read( p ) == ( 10000L * 9999 - i * ( i - 1 ) ) / 2 );
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
