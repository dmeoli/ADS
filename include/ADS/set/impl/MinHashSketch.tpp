/*--------------------------------------------------------------------------*/
/*------------------------- File MinHashSketch.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MinHashSketch [see MinHashSketch.h], with their
 * contracts.
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

template< class T , class H >
MinHashSketch< T , H >::MinHashSketch( std::size_t K , std::uint64_t seed ,
				       H h )
    : f_seed( seed ) , f_h( h )
{
 ADS_PRE( K >= 1 && std::uint64_t( K ) <= ( std::uint64_t( 1 ) << 32 ) );
 f_B.assign( K , kEmpty );
 }

template< class T , class H >
void MinHashSketch< T , H >::insertHash( std::uint64_t h )
{
 const Product128 q = product128( h , std::uint64_t( f_B.size() ) );
 std::uint64_t & b = f_B[ std::size_t( q.hi ) ];
 b = std::min( b , q.lo >> 1 );
 }

template< class T , class H >
void MinHashSketch< T , H >::insert( const T & x )
{
 insertHash( detail::sketchHash( f_h , x ) );
 }

template< class T , class H >
void MinHashSketch< T , H >::merge( const MinHashSketch & o )
{
 ADS_PRE( o.f_B.size() == f_B.size() && o.f_seed == f_seed );
 for( std::size_t i = 0 ; i < f_B.size() ; ++i )
  f_B[ i ] = std::min( f_B[ i ] , o.f_B[ i ] );
 }

template< class T , class H >
std::vector< std::uint64_t > MinHashSketch< T , H >::signature( void ) const
{
 std::vector< std::uint64_t > S = f_B;
 if( empty() )
  return( S );
 const std::uint64_t K = f_B.size();
 for( std::uint64_t i = 0 ; i < K ; ++i ) {
  if( f_B[ i ] != kEmpty )
   continue;
  // the bins pi( i , 1 ) , pi( i , 2 ) , ... of the hashes of the pairs,
  // the same for every set of this seed
  for( std::uint64_t a = 1 ; ; ++a ) {
   const std::uint64_t j =
     product128( hashWord( ( i << 32 ) | ( a & 0xFFFFFFFFULL ) , f_seed ) , K )
       .hi;
   if( f_B[ j ] != kEmpty ) {
    S[ i ] = f_B[ j ];
    break;
    }
   }
  }
 return( S );
 }

template< class T , class H >
double
MinHashSketch< T , H >::jaccard( const std::vector< std::uint64_t > & a ,
				const std::vector< std::uint64_t > & b )
{
 ADS_PRE( a.size() == b.size() && ! a.empty() );
 std::size_t e = 0;
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  e += a[ i ] == b[ i ];
 return( double( e ) / double( a.size() ) );
 }

template< class T , class H >
double MinHashSketch< T , H >::jaccard( const MinHashSketch & o ) const
{
 ADS_PRE( o.f_B.size() == f_B.size() && o.f_seed == f_seed );
 const bool e = empty() , f = o.empty();
 if( e || f )
  return( e && f ? 1.0 : 0.0 );
 return( jaccard( signature() , o.signature() ) );
 }

template< class T , class H >
std::size_t MinHashSketch< T , H >::bins( void ) const
{
 return( f_B.size() );
 }

template< class T , class H >
std::size_t MinHashSketch< T , H >::filled( void ) const
{
 return( std::size_t( f_B.size() - std::size_t( std::count(
				      f_B.begin() , f_B.end() , kEmpty ) ) ) );
 }

template< class T , class H >
bool MinHashSketch< T , H >::empty( void ) const
{
 return( std::all_of( f_B.begin() , f_B.end() ,
		       []( std::uint64_t b ) { return( b == kEmpty ); } ) );
 }

template< class T , class H >
const std::vector< std::uint64_t > &
MinHashSketch< T , H >::state( void ) const
{
 return( f_B );
 }

template< class T , class H >
void MinHashSketch< T , H >::clear( void )
{
 std::fill( f_B.begin() , f_B.end() , kEmpty );
 }

template< class T , class H >
bool MinHashSketch< T , H >::operator==( const MinHashSketch & o ) const
{
 return( f_seed == o.f_seed && f_B == o.f_B );
 }

/*--------------------------------------------------------------------------*/

template< class T , class H >
template< class A >
inline void MinHashSketch< T , H >::persist( A & a )
{
 a( f_B , f_seed );
 if constexpr( ! std::is_empty_v< H > )  // a hash function with a state
  a( f_h );
 a.require( ! f_B.empty() &&
	      std::uint64_t( f_B.size() ) <= ( std::uint64_t( 1 ) << 32 ) ,
	    "the bins of a MinHashSketch" );
 // a bin holds 63 bits, or is empty
 bool ok = true;
 for( const std::uint64_t b : f_B )
  ok = ok && ( b == kEmpty || ( b >> 63 ) == 0 );
 a.require( ok , "the values of a MinHashSketch" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File MinHashSketch.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
