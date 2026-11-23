/*--------------------------------------------------------------------------*/
/*------------------------ File CountMinSketch.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CountMinSketch and CountSketch [see CountMinSketch.h],
 * with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// the rows of a hash h of 64 bits: g_i = h + i h', h' an odd hash of h;
/// the counter of the row i is the high half of g_i w on 128 bits, and its
/// sign the top bit of the low half
struct SketchRows
{
 std::uint64_t g , step;

 explicit SketchRows( std::uint64_t h ) : g( h ) ,
   step( hashWord( h ) | 1 ) { }

 /// the counter of the current row among w, its sign in s, then the next
 /// row
 std::size_t next( std::size_t w , bool & s ) {
  const Product128 q = product128( g , std::uint64_t( w ) );
  g += step;
  s = ( q.lo >> 63 ) != 0;
  return( std::size_t( q.hi ) );
  }

 /// the counter of the current row among w, then the next row
 std::size_t next( std::size_t w ) {
  const std::size_t j = std::size_t( product128( g , std::uint64_t( w ) ).hi );
  g += step;
  return( j );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- CountMinSketch ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class C , class H >
CountMinSketch< T , C , H >::CountMinSketch( std::size_t w , std::size_t d ,
					   CountMinUpdate u , H h )
    : f_w( w ) , f_d( d ) , f_u( u ) , f_h( h )
{
 ADS_PRE( w >= 1 && d >= 1 );
 f_C.assign( w * d , C{} );
 }

template< class T , class C , class H >
CountMinSketch< T , C , H >
CountMinSketch< T , C , H >::forError( double epsilon , double delta ,
				     CountMinUpdate u , H h )
{
 ADS_PRE( epsilon > 0 && delta > 0 && delta < 1 );
 const double w = std::ceil( std::exp( 1.0 ) / epsilon ) ,
	      d = std::ceil( std::log( 1 / delta ) );
 return( CountMinSketch( std::size_t( w ) ,
			 std::size_t( std::max( 1.0 , d ) ) ,
			  u , h ) );
 }

template< class T , class C , class H >
void CountMinSketch< T , C , H >::insertHash( std::uint64_t h , C c )
{
 f_N += c;
 detail::SketchRows r( h );
 if( f_u == CountMinUpdate::plain ) {
  for( std::size_t i = 0 ; i < f_d ; ++i )
   f_C[ i * f_w + r.next( f_w ) ] += c;
  return;
  }
 // the conservative update: the counters up to the new estimate; the d
 // places are found twice, the hash being cheaper than a buffer of them
 detail::SketchRows q( h );
 C e = f_C[ q.next( f_w ) ];
 for( std::size_t i = 1 ; i < f_d ; ++i )
  e = std::min( e , f_C[ i * f_w + q.next( f_w ) ] );
 e += c;
 for( std::size_t i = 0 ; i < f_d ; ++i ) {
  C & x = f_C[ i * f_w + r.next( f_w ) ];
  x = std::max( x , e );
  }
 }

template< class T , class C , class H >
void CountMinSketch< T , C , H >::insert( const T & x , C c )
{
 insertHash( detail::sketchHash( f_h , x ) , c );
 }

template< class T , class C , class H >
C CountMinSketch< T , C , H >::estimateHash( std::uint64_t h ) const
{
 detail::SketchRows r( h );
 C e = f_C[ r.next( f_w ) ];
 for( std::size_t i = 1 ; i < f_d ; ++i )
  e = std::min( e , f_C[ i * f_w + r.next( f_w ) ] );
 return( e );
 }

template< class T , class C , class H >
C CountMinSketch< T , C , H >::estimate( const T & x ) const
{
 return( estimateHash( detail::sketchHash( f_h , x ) ) );
 }

template< class T , class C , class H >
void CountMinSketch< T , C , H >::merge( const CountMinSketch & o )
{
 ADS_PRE( o.f_w == f_w && o.f_d == f_d && o.f_u == f_u );
 for( std::size_t i = 0 ; i < f_C.size() ; ++i )
  f_C[ i ] += o.f_C[ i ];
 f_N += o.f_N;
 }

template< class T , class C , class H >
C CountMinSketch< T , C , H >::total( void ) const
{
 return( f_N );
 }

template< class T , class C , class H >
std::size_t CountMinSketch< T , C , H >::width( void ) const
{
 return( f_w );
 }

template< class T , class C , class H >
std::size_t CountMinSketch< T , C , H >::depth( void ) const
{
 return( f_d );
 }

template< class T , class C , class H >
CountMinUpdate CountMinSketch< T , C , H >::update( void ) const
{
 return( f_u );
 }

template< class T , class C , class H >
double CountMinSketch< T , C , H >::errorBound( void ) const
{
 return( std::exp( 1.0 ) / double( f_w ) * double( f_N ) );
 }

template< class T , class C , class H >
void CountMinSketch< T , C , H >::clear( void )
{
 std::fill( f_C.begin() , f_C.end() , C{} );
 f_N = C{};
 }

template< class T , class C , class H >
bool CountMinSketch< T , C , H >::empty( void ) const
{
 return( f_N == C{} );
 }

template< class T , class C , class H >
bool CountMinSketch< T , C , H >::operator==( const CountMinSketch & o ) const
{
 return( f_w == o.f_w && f_d == o.f_d && f_u == o.f_u && f_N == o.f_N &&
	  f_C == o.f_C );
 }

template< class T , class C , class H >
template< class A >
inline void CountMinSketch< T , C , H >::persist( A & a )
{
 a( f_C , f_w , f_d , f_N , f_u );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( f_w >= 1 && f_d >= 1 && f_C.size() / f_d == f_w &&
	      f_C.size() % f_d == 0 ,
	    "the shape of a CountMinSketch" );
 a.require( f_u == CountMinUpdate::plain ||
	      f_u == CountMinUpdate::conservative ,
	    "the update of a CountMinSketch" );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ CountSketch -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class C , class H >
CountSketch< T , C , H >::CountSketch( std::size_t w , std::size_t d , H h )
    : f_w( w ) , f_d( d ) , f_h( h )
{
 ADS_PRE( w >= 1 && d >= 1 );
 f_C.assign( w * d , C{} );
 }

template< class T , class C , class H >
void CountSketch< T , C , H >::insertHash( std::uint64_t h , C c )
{
 detail::SketchRows r( h );
 for( std::size_t i = 0 ; i < f_d ; ++i ) {
  bool s;
  const std::size_t j = r.next( f_w , s );
  f_C[ i * f_w + j ] += s ? C( -c ) : c;
  }
 }

template< class T , class C , class H >
void CountSketch< T , C , H >::insert( const T & x , C c )
{
 insertHash( detail::sketchHash( f_h , x ) , c );
 }

template< class T , class C , class H >
double CountSketch< T , C , H >::median( double * v , std::size_t d )
{
 const std::size_t k = d / 2;
 std::nth_element( v , v + k , v + d );
 if( d % 2 == 1 )
  return( v[ k ] );
 return( ( *std::max_element( v , v + k ) + v[ k ] ) / 2 );
 }

template< class T , class C , class H >
double CountSketch< T , C , H >::estimateHash( std::uint64_t h ) const
{
 // the d values on the stack up to 32 rows, on the heap beyond
 double b[ 32 ];
 std::vector< double > big( f_d > 32 ? f_d : 0 );
 double * v = f_d > 32 ? big.data() : b;
 detail::SketchRows r( h );
 for( std::size_t i = 0 ; i < f_d ; ++i ) {
  bool s;
  const std::size_t j = r.next( f_w , s );
  const double c = double( f_C[ i * f_w + j ] );
  v[ i ] = s ? -c : c;
  }
 return( median( v , f_d ) );
 }

template< class T , class C , class H >
double CountSketch< T , C , H >::estimate( const T & x ) const
{
 return( estimateHash( detail::sketchHash( f_h , x ) ) );
 }

template< class T , class C , class H >
double CountSketch< T , C , H >::secondMoment( void ) const
{
 std::vector< double > v( f_d , 0.0 );
 for( std::size_t i = 0 ; i < f_d ; ++i )
  for( std::size_t j = 0 ; j < f_w ; ++j ) {
   const double c = double( f_C[ i * f_w + j ] );
   v[ i ] += c * c;
   }
 return( median( v.data() , f_d ) );
 }

template< class T , class C , class H >
void CountSketch< T , C , H >::merge( const CountSketch & o )
{
 ADS_PRE( o.f_w == f_w && o.f_d == f_d );
 for( std::size_t i = 0 ; i < f_C.size() ; ++i )
  f_C[ i ] += o.f_C[ i ];
 }

template< class T , class C , class H >
std::size_t CountSketch< T , C , H >::width( void ) const
{
 return( f_w );
 }

template< class T , class C , class H >
std::size_t CountSketch< T , C , H >::depth( void ) const
{
 return( f_d );
 }

template< class T , class C , class H >
void CountSketch< T , C , H >::clear( void )
{
 std::fill( f_C.begin() , f_C.end() , C{} );
 }

template< class T , class C , class H >
bool CountSketch< T , C , H >::operator==( const CountSketch & o ) const
{
 return( f_w == o.f_w && f_d == o.f_d && f_C == o.f_C );
 }

template< class T , class C , class H >
template< class A >
inline void CountSketch< T , C , H >::persist( A & a )
{
 a( f_C , f_w , f_d );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( f_w >= 1 && f_d >= 1 && f_C.size() / f_d == f_w &&
	      f_C.size() % f_d == 0 ,
	    "the shape of a CountSketch" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File CountMinSketch.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
