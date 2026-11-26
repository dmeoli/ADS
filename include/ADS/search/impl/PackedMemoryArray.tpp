/*--------------------------------------------------------------------------*/
/*----------------------- File PackedMemoryArray.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PackedMemoryArray [see PackedMemoryArray.h], with their
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

#include <algorithm>
#include <iterator>
#include <tuple>

namespace ADS
{

#define ADS_PMA_T                                                             \
 template< class K, bool Z >                                                  \
  requires detail::PMAKey< K, Z >
#define ADS_PMA PackedMemoryArray< K, Z >

namespace detail
{

/// whether \f$ a \f$ and \f$ b \f$ are equal by `<` alone
template< class K >
inline bool pmaSame( const K & a , const K & b )
{
 return( ! ( a < b ) && ! ( b < a ) );
 }

}  // end( namespace detail )

/*------------------------------- THE LEAVES -------------------------------*/

ADS_PMA_T
inline std::size_t ADS_PMA::step( const K & x , const K & y )
{
 if constexpr( Z )
  return( std::size_t( vbyteLength( std::uint64_t( y - x ) ) ) );
 else {
  ( void )x;
  ( void )y;
  return( 1 );
  }
 }

ADS_PMA_T
std::size_t ADS_PMA::units( const K * x , std::size_t c )
{
 if( c == 0 )
  return( 0 );
 if constexpr( ! Z )
  return( c );
 else {
  std::size_t u = kHead;
  for( std::size_t i = 1 ; i < c ; ++i )
   u += step( x[ i - 1 ] , x[ i ] );
  return( u );
  }
 }

ADS_PMA_T
std::size_t ADS_PMA::leafOf( const K & x ) const
{
 const K * E = f_head.data();
 const std::size_t L = f_L;
 std::size_t j = 1;
 while( j <= L ) {  // a conditional move a level
  ADS::detail::prefetchLine( E + std::min( 16 * j , L ) );
  j = 2 * j + ( x < E[ j ] ? 0 : 1 );
  }
 j >>= std::countr_one( j ) + 1;  // the first head > x, 0 if none
 if( j == 0 )
  return( L - 1 );
 const std::size_t first = leafAt( j );
 return( first == 0 ? 0 : first - 1 );
 }

ADS_PMA_T
std::size_t ADS_PMA::decode( std::size_t l , K * out ) const
{
 const std::size_t u = f_units[ l ];
 if( u == 0 )
  return( 0 );
 K h = head( l );
 out[ 0 ] = h;
 if constexpr( ! Z ) {
  std::copy_n( tail( l ) , u - 1 , out + 1 );
  return( u );
  }
 else {
  const std::uint8_t * t = tail( l );
  const std::size_t end = u - kHead;
  std::size_t p = 0 , c = 1;
  while( p < end ) {
   h = K( h + K( decodeVByte( t , p ) ) );
   out[ c++ ] = h;
   }
  return( c );
  }
 }

ADS_PMA_T
void ADS_PMA::decodeTo( std::size_t l , std::vector< K > & out ) const
{
 const std::size_t at = out.size();
 out.resize( at + count( l ) );
 decode( l , out.data() + at );
 }

ADS_PMA_T
std::size_t ADS_PMA::count( std::size_t l ) const
{
 const std::size_t u = f_units[ l ];
 if constexpr( ! Z )
  return( u );
 else {
  if( u == 0 )
   return( 0 );
  const std::uint8_t * t = tail( l );
  std::size_t c = 1;
  for( std::size_t p = 0 ; p < u - kHead ; ++p )
   c += t[ p ] < 0x80 ? 1 : 0;  // a stopper ends a difference
  return( c );
  }
 }

ADS_PMA_T
void ADS_PMA::encode( std::size_t l , const K * x , std::size_t c , bool h )
{
 ADS_PRE( c >= 1 );
 ADS_AUDIT( units( x , c ) <= f_C );
 if( h )
  f_head[ pos( l ) ] = x[ 0 ];
 if constexpr( ! Z ) {
  std::copy_n( x + 1 , c - 1 , tail( l ) );
  f_units[ l ] = std::uint32_t( c );
  }
 else {
  std::uint8_t * t = tail( l );
  std::size_t p = 0;
  for( std::size_t i = 1 ; i < c ; ++i )
   p +=
     std::size_t( encodeVByte( t + p ,
			       std::uint64_t( x[ i ] - x[ i - 1 ] ) ) );
  f_units[ l ] = std::uint32_t( kHead + p );
  }
 }

/*----------------------------- THE THRESHOLDS -----------------------------*/

ADS_PMA_T
inline std::size_t ADS_PMA::upper( std::size_t h ) const
{
 if( f_H == 0 )
  return( f_C );
 return( ( f_C << h ) * ( 4 * f_H - h ) / ( 4 * f_H ) );
 }

ADS_PMA_T
inline std::size_t ADS_PMA::lower( std::size_t h ) const
{
 if( f_H == 0 )
  return( 0 );
 return( ( ( f_C << h ) * ( f_H + h ) + 8 * f_H - 1 ) / ( 8 * f_H ) );
 }

ADS_PMA_T
std::size_t ADS_PMA::unitsOf( std::size_t a , std::size_t r ) const
{
 std::size_t s = 0;
 for( std::size_t l = a ; l < a + r ; ++l )
  s += f_units[ l ];
 return( s );
 }

/*--------------------------- THE REDISTRIBUTION ---------------------------*/

ADS_PMA_T
template< Executor E >
bool ADS_PMA::spread( E & ex , std::size_t a , std::size_t r ,
		      const std::vector< K > & x )
{
 const std::size_t m = x.size();
 if( m < r )
  return( false );  // a leaf would be empty
 // P[ j ], the units of x_0 .. x_{j-1} one after the other
 std::vector< std::size_t > P;
 if constexpr( Z ) {
  P.assign( m + 1 , 0 );
  std::vector< std::size_t > c( m );
  parallelFor( ex , 0 , m , [ & ]( std::size_t i ) {
   c[ i ] = i == 0 ? kHead : step( x[ i - 1 ] , x[ i ] );
   } );
  ( void )parallelInclusiveScan( ex , c.begin() , m , P.begin() + 1 ,
				 std::size_t( 0 ) );
  }
 auto pre = [ & ]( std::size_t j ) {
  if constexpr( Z )
   return( P[ j ] );
  else
   return( j );
  };
 const std::size_t U = pre( m );
 std::vector< std::size_t > b( r + 1 );
 b[ 0 ] = 0;
 b[ r ] = m;
 parallelFor( ex , 1 , r , [ & ]( std::size_t i ) {
  const std::size_t t = i * ( U / r ) + i * ( U % r ) / r;
  if constexpr( Z )
   b[ i ] =
     std::size_t( std::lower_bound( P.begin() , P.end() , t ) - P.begin() );
  else
   b[ i ] = t;
  } );
 for( std::size_t i = 1 ; i < r ; ++i )  // each leaf a key, at least
  b[ i ] = std::min( std::max( b[ i ] , b[ i - 1 ] + 1 ) , m - ( r - i ) );
 for( std::size_t i = 0 ; i < r ; ++i )
  if( kHead + pre( b[ i + 1 ] ) - pre( b[ i ] + 1 ) > f_C )
   return( false );  // the share of the leaf a + i does not fit it
 parallelFor(
   ex , 0 , r ,
   [ & ]( std::size_t i ) {
    encode( a + i , x.data() + b[ i ] , b[ i + 1 ] - b[ i ] , true );
    } ,
   std::max< std::size_t >( 1 , 4096 / f_C ) );
 return( true );
 }

ADS_PMA_T
const typename ADS_PMA::Spill *
ADS_PMA::spillOf( const std::vector< Spill > & Out , std::size_t l )
{
 const auto it = std::lower_bound(
   Out.begin() , Out.end() , l ,
   []( const Spill & s , std::size_t v ) { return( s.leaf < v ); } );
 return( ( it != Out.end() ) && ( it->leaf == l ) && ! it->used ? &*it
								 : nullptr );
 }

ADS_PMA_T
template< Executor E >
void ADS_PMA::gather( E & ex , std::size_t a , std::size_t r ,
		      const std::vector< Spill > & Out ,
		      std::vector< K > & x ) const
{
 std::vector< std::size_t > off( r + 1 , 0 );
 parallelFor( ex , 0 , r , [ & ]( std::size_t i ) {
  const Spill * s = spillOf( Out , a + i );
  off[ i + 1 ] = s ? s->key.size() : count( a + i );
  } );
 for( std::size_t i = 0 ; i < r ; ++i )
  off[ i + 1 ] += off[ i ];
 x.resize( off[ r ] );
 parallelFor(
   ex , 0 , r ,
   [ & ]( std::size_t i ) {
    const Spill * s = spillOf( Out , a + i );
    if( s )
     std::copy( s->key.begin() , s->key.end() , x.begin() + off[ i ] );
    else
     decode( a + i , x.data() + off[ i ] );
    } ,
   std::max< std::size_t >( 1 , 4096 / f_C ) );
 }

ADS_PMA_T
template< Executor E >
void ADS_PMA::build( E & ex , std::vector< K > && x )
{
 const std::size_t n = x.size();
 if( n == 0 ) {
  clear();
  return;
  }
 const std::size_t b = std::size_t( std::bit_width( n ) );
 const std::size_t C =
   Z ? std::max< std::size_t >( 256 , std::bit_ceil( 16 * b ) )
     : std::max< std::size_t >( 16 , std::bit_ceil( 2 * b ) );
 std::size_t U = n;
 if constexpr( Z )
  U = kHead + parallelReduce(
		ex , 1 , n , std::size_t( 0 ) ,
		[ & ]( std::size_t lo , std::size_t hi ) {
		 std::size_t s = 0;
		 for( std::size_t i = lo ; i < hi ; ++i )
		  s += step( x[ i - 1 ] , x[ i ] );
		 return( s );
		 } ,
		std::plus<>() );
 std::size_t L = 1;
 while( U > L * C / 2 )  // at most half full
  L *= 2;
 f_n = n;
 f_L = L;
 f_H = std::size_t( std::countr_zero( L ) );
 f_C = C;
 f_head.assign( L + 1 , K() );
 f_units.assign( L , 0 );
 f_tail.assign( L * ( C - kHead ) , Cell() );
 const bool spread_ = spread( ex , 0 , L , x );
 ADS_POST( spread_ );
 ADS_INVARIANT( wellFormed() );
 }

ADS_PMA_T
bool ADS_PMA::climb( std::size_t l , const K & x , bool in , std::size_t lu )
{
 Sequential s;
 std::vector< K > buf;
 for( std::size_t h = 1 ; h <= f_H ; ++h ) {
  const std::size_t a = ( l >> h ) << h , r = std::size_t( 1 ) << h;
  const std::size_t u = unitsOf( a , r ) - f_units[ l ] + lu;
  if( in ? ( u > upper( h ) ) : ( u < lower( h ) ) )
   continue;
  gather( s , a , r , {} , buf );
  const auto at = std::lower_bound( buf.begin() , buf.end() , x );
  if( in )
   buf.insert( at , x );
  else
   buf.erase( at );
  if( spread( s , a , r , buf ) )
   return( true );
  }
 return( false );
 }

/*------------------------------ CONSTRUCTION ------------------------------*/

ADS_PMA_T
ADS_PMA::PackedMemoryArray( std::vector< K > S )
{
 std::sort( S.begin() , S.end() );
 S.erase( std::unique( S.begin() , S.end() ,
		       []( const K & a , const K & b ) {
			return( detail::pmaSame( a , b ) );
		        } ) ,
	  S.end() );
 Sequential s;
 build( s , std::move( S ) );
 }

ADS_PMA_T
void ADS_PMA::clear( void )
{
 f_n = f_L = f_H = f_C = 0;
 f_head = std::vector< K >();  // the memory given back
 f_units = std::vector< std::uint32_t >();
 f_tail = std::vector< Cell >();
 }

ADS_PMA_T
std::size_t ADS_PMA::bytes( void ) const
{
 return( sizeof( *this ) + f_head.capacity() * sizeof( K ) +
	  f_units.capacity() * sizeof( std::uint32_t ) +
	  f_tail.capacity() * sizeof( Cell ) );
 }

/*-------------------------------- QUERIES ---------------------------------*/

ADS_PMA_T
bool ADS_PMA::contains( const K & x ) const
{
 if( f_n == 0 )
  return( false );
 const std::size_t l = leafOf( x );
 K h = head( l );
 if( x < h )
  return( false );
 if( ! ( h < x ) )
  return( true );
 const std::size_t u = f_units[ l ];
 if constexpr( ! Z ) {
  const K *t = tail( l ) , *e = t + ( u - 1 );
  const K * p = std::lower_bound( t , e , x );
  return( ( p != e ) && ! ( x < *p ) );
  }
 else {
  const std::uint8_t * t = tail( l );
  for( std::size_t p = 0 ; p < u - kHead ; ) {
   h = K( h + K( decodeVByte( t , p ) ) );
   if( h >= x )
    return( h == x );
   }
  return( false );
  }
 }

ADS_PMA_T
std::optional< K > ADS_PMA::lowerBound( const K & x ) const
{
 std::optional< K > r;
 scan( x , [ & ]( const K & k ) {
  r = k;
  return( false );
  } );
 return( r );
 }

ADS_PMA_T
template< class F >
void ADS_PMA::scan( const K & lo , F && f ) const
{
 if( f_n == 0 )
  return;
 for( std::size_t l = leafOf( lo ) ; l < f_L ; ++l ) {
  K h = head( l );
  const std::size_t u = f_units[ l ];
  if( ! ( h < lo ) && ! f( h ) )
   return;
  if constexpr( ! Z ) {
   const K *t = tail( l ) , *e = t + ( u - 1 );
   for( const K * p = h < lo ? std::lower_bound( t , e , lo ) : t ; p != e ;
	++p )
    if( ! f( *p ) )
     return;
   }
  else {
   const std::uint8_t * t = tail( l );
   for( std::size_t p = 0 ; p < u - kHead ; ) {
    h = K( h + K( decodeVByte( t , p ) ) );
    if( ( h >= lo ) && ! f( h ) )
     return;
    }
   }
  }
 }

ADS_PMA_T
template< class F >
void ADS_PMA::forEach( const K & lo , const K & hi , F && f ) const
{
 scan( lo , [ & ]( const K & k ) {
  if( ! ( k < hi ) )
   return( false );
  f( k );
  return( true );
  } );
 }

ADS_PMA_T
template< class F >
void ADS_PMA::forEach( F && f ) const
{
 for( std::size_t l = 0 ; l < f_L ; ++l ) {
  K h = head( l );
  f( h );
  const std::size_t u = f_units[ l ];
  if constexpr( ! Z ) {
   const K * t = tail( l );
   for( std::size_t i = 0 ; i + 1 < u ; ++i )
    f( t[ i ] );
   }
  else {
   const std::uint8_t * t = tail( l );
   for( std::size_t p = 0 ; p < u - kHead ; ) {
    h = K( h + K( decodeVByte( t , p ) ) );
    f( h );
    }
   }
  }
 }

/*---------------------------- THE POINT UPDATES ---------------------------*/

ADS_PMA_T
bool ADS_PMA::insert( const K & x )
{
 if( f_n == 0 ) {
  Sequential s;
  build( s , std::vector< K >{ x } );
  return( true );
  }
 const std::size_t l = leafOf( x );
 const std::size_t u = f_units[ l ];
 const K h = head( l );
 Cell * const t = tail( l );
 if( ! ( x < h ) && ! ( h < x ) )
  return( false );  // the head already
 std::size_t lu;
 if constexpr( ! Z ) {  // the keys after x moved by one cell, in place
  lu = u + 1;
  K * const e = t + ( u - 1 );
  K * const p = x < h ? t : std::lower_bound( t , e , x );
  if( ( p != e ) && ! ( x < h ) && ! ( x < *p ) )
   return( false );  // there already
  if( lu <= f_C ) {
   std::copy_backward( p , e , e + 1 );
   if( x < h ) {  // a new head, only in the first leaf
    *p = h;
    f_head[ pos( l ) ] = x;
    }
   else
    *p = x;
   f_units[ l ] = std::uint32_t( lu );
   ++f_n;
   return( true );
   }
  }
 else {  // the bytes after x moved by the length of its code, in place
  const std::size_t e = u - kHead;
  std::size_t q = e , p = e;  // the code that x splits: [ q , p )
  K a = h , k = h;            // the keys before and after x
  // the differences of x from a and of k from x
  std::uint64_t d1 = 0 , d2 = 0;
  if( x < h ) {  // a new head, only in the first leaf
   q = p = 0;
   d1 = std::uint64_t( h - x );
   }
  else {
   for( std::size_t r = 0 ; r < e ; ) {
    const std::size_t at = r;
    k = K( a + K( decodeVByte( t , r ) ) );
    if( ! ( k < x ) ) {
     q = at;
     p = r;
     break;
     }
    a = k;
    }
   if( k == x )
    return( false );  // there already
   d1 = std::uint64_t( x - a );
   if( p > q )
    d2 = std::uint64_t( k - x );
   }
  const std::size_t n1 = std::size_t( vbyteLength( d1 ) );
  const std::size_t n2 = p > q ? std::size_t( vbyteLength( d2 ) ) : 0;
  lu = u - ( p - q ) + n1 + n2;
  if( lu <= f_C ) {
   std::copy_backward( t + p , t + e , t + ( lu - kHead ) );
   const std::size_t w = q + std::size_t( encodeVByte( t + q , d1 ) );
   if( n2 > 0 )
    encodeVByte( t + w , d2 );
   if( x < h )
    f_head[ pos( l ) ] = x;
   f_units[ l ] = std::uint32_t( lu );
   ++f_n;
   return( true );
   }
  }
 if( ! climb( l , x , true , lu ) ) {  // the root overflows: a building
  Sequential s;
  std::vector< K > all;
  gather( s , 0 , f_L , {} , all );
  all.insert( std::lower_bound( all.begin() , all.end() , x ) , x );
  build( s , std::move( all ) );
  return( true );
  }
 ++f_n;
 return( true );
 }

ADS_PMA_T
bool ADS_PMA::erase( const K & x )
{
 if( f_n == 0 )
  return( false );
 const std::size_t l = leafOf( x );
 const std::size_t u = f_units[ l ];
 const K h = head( l );
 Cell * const t = tail( l );
 if( x < h )
  return( false );  // not there
 std::size_t lu;     // the units of the leaf without x
 bool more;          // whether the leaf keeps a key
 // the keys after x moved back in place: the cells [ q + g , e ) to q
 std::size_t q = 0 , g = 1 , e = u - 1;
 std::uint64_t d = 0;    // the code written at q when compressed
 std::optional< K > nh;  // the new head, if x is the head
 if constexpr( ! Z ) {
  if( h < x ) {
   const K * const p = std::lower_bound( t , t + e , x );
   if( ( p == t + e ) || ( x < *p ) )
    return( false );  // not there
   q = std::size_t( p - t );
   }
  else
   nh = e > 0 ? t[ 0 ] : h;  // the first key of the tail the head
  lu = u - 1;
  more = u > 1;
  }
 else {
  e = u - kHead;
  more = e > 0;
  if( ! ( h < x ) ) {  // the head: the first key of the tail in its place
   std::size_t r = 0;
   nh = more ? K( h + K( decodeVByte( t , r ) ) ) : h;
   g = r;
   lu = more ? u - r : 0;
   }
  else {
   K a = h , k = h;  // the keys before x and x
   std::size_t r = 0;
   while( r < e ) {
    q = r;
    k = K( a + K( decodeVByte( t , r ) ) );
    if( ! ( k < x ) )
     break;
    a = k;
    }
   if( k != x )
    return( false );  // not there
   std::size_t w = r;  // past the code of the key after x
   if( r < e ) {       // the code of a key after x, from a
    d = std::uint64_t( K( x + K( decodeVByte( t , w ) ) ) - a );
    g = w - q - std::size_t( vbyteLength( d ) );
    }
   else
    g = r - q;
   lu = u - g;
   }
  }
 if( f_n == 1 ) {
  clear();
  return( true );
  }
 if( more && ( ( lu >= lower( 0 ) ) || ( f_L == 1 ) ) ) {  // in place
  std::size_t at = q;
  if constexpr( Z )
   if( d > 0 )
    at += std::size_t( encodeVByte( t + q , d ) );
  std::copy( t + at + g , t + e , t + at );
  if( nh )
   f_head[ pos( l ) ] = *nh;
  f_units[ l ] = std::uint32_t( lu );
  --f_n;
  return( true );
  }
 if( ! climb( l , x , false , lu ) ) {  // the root underflows: a building
  Sequential s;
  std::vector< K > all;
  gather( s , 0 , f_L , {} , all );
  all.erase( std::lower_bound( all.begin() , all.end() , x ) );
  build( s , std::move( all ) );
  return( true );
  }
 --f_n;
 return( true );
 }

/*------------------------------- THE BATCHES ------------------------------*/

ADS_PMA_T
template< Executor E >
std::size_t ADS_PMA::insertBatch( E & ex , std::vector< K > B )
{
 ADS::sort( ex , B );
 B.erase( std::unique( B.begin() , B.end() ,
		       []( const K & a , const K & b ) {
			return( detail::pmaSame( a , b ) );
		        } ) ,
	  B.end() );
 return( batch( ex , B , true ) );
 }

ADS_PMA_T
std::size_t ADS_PMA::insertBatch( std::vector< K > B )
{
 Sequential s;
 return( insertBatch( s , std::move( B ) ) );
 }

ADS_PMA_T
template< Executor E >
std::size_t ADS_PMA::eraseBatch( E & ex , std::vector< K > B )
{
 ADS::sort( ex , B );
 B.erase( std::unique( B.begin() , B.end() ,
		       []( const K & a , const K & b ) {
			return( detail::pmaSame( a , b ) );
		        } ) ,
	  B.end() );
 return( batch( ex , B , false ) );
 }

ADS_PMA_T
std::size_t ADS_PMA::eraseBatch( std::vector< K > B )
{
 Sequential s;
 return( eraseBatch( s , std::move( B ) ) );
 }

ADS_PMA_T
template< Executor E >
std::vector< K > ADS_PMA::setOp( E & ex , const std::vector< K > & A ,
				 const std::vector< K > & B , bool in )
{
 std::size_t nb , sz;
 std::tie( nb , sz ) = blocksOf( 0 , A.size() , 4096 );
 std::vector< std::vector< K > > part( nb );
 parallelFor(
   ex , 0 , nb ,
   [ & ]( std::size_t i ) {
    const std::size_t lo = i * sz , hi = std::min( A.size() , lo + sz );
    const auto b0 =
      i == 0 ? B.begin() : std::lower_bound( B.begin() , B.end() , A[ lo ] );
    const auto b1 = hi == A.size()
		      ? B.end()
		      : std::lower_bound( B.begin() , B.end() , A[ hi ] );
    auto & P = part[ i ];
    if( in )
     std::set_union( A.begin() + lo , A.begin() + hi , b0 , b1 ,
		     std::back_inserter( P ) );
    else
     std::set_difference( A.begin() + lo , A.begin() + hi , b0 , b1 ,
			  std::back_inserter( P ) );
    } ,
   1 , nb );
 std::vector< std::size_t > off( nb + 1 , 0 );
 for( std::size_t i = 0 ; i < nb ; ++i )
  off[ i + 1 ] = off[ i ] + part[ i ].size();
 std::vector< K > M( off[ nb ] );
 parallelFor(
   ex , 0 , nb ,
   [ & ]( std::size_t i ) {
    std::copy( part[ i ].begin() , part[ i ].end() , M.begin() + off[ i ] );
    } ,
   1 , nb );
 return( M );
 }

ADS_PMA_T
void ADS_PMA::mergeLeaves( const std::vector< K > & B , std::size_t j ,
			   std::size_t j1 , bool in , Part & part )
{
 std::vector< K > cur , merged;
 while( j < j1 ) {
  const std::size_t l = leafOf( B[ j ] );
  const std::size_t e =
    l + 1 < f_L ? std::size_t( std::lower_bound( B.begin() + long( j ) ,
						 B.begin() + long( j1 ) ,
						 head( l + 1 ) ) -
			       B.begin() )
		: j1;
  cur.clear();
  decodeTo( l , cur );
  merged.clear();
  if( in )
   std::set_union( cur.begin() , cur.end() , B.begin() + long( j ) ,
		   B.begin() + long( e ) , std::back_inserter( merged ) );
  else
   std::set_difference( cur.begin() , cur.end() , B.begin() + long( j ) ,
			B.begin() + long( e ) , std::back_inserter( merged ) );
  j = e;
  if( merged.size() == cur.size() )
   continue;  // the leaf does not change
  part.changed += in ? merged.size() - cur.size() : cur.size() - merged.size();
  part.touched.push_back( l );
  const std::size_t mu = units( merged.data() , merged.size() );
  if( ( mu <= f_C ) && ! merged.empty() &&
      detail::pmaSame( merged[ 0 ] , cur[ 0 ] ) )
   encode( l , merged.data() , merged.size() , false );  // in place
  else  // apart, its units recorded after the merge
   part.out.push_back( { l , std::move( merged ) , mu , false } );
  }
 }

ADS_PMA_T
template< Executor E >
std::size_t ADS_PMA::batch( E & ex , std::vector< K > & B , bool in )
{
 const std::size_t k = B.size();
 if( k == 0 )
  return( 0 );
 if( f_n == 0 ) {
  if( ! in )
   return( 0 );
  build( ex , std::move( B ) );
  return( k );
  }
 if( k < kPointBatch ) {  // a key at a time
  std::size_t c = 0;
  for( const K & x : B )
   c += ( in ? insert( x ) : erase( x ) ) ? 1 : 0;
  return( c );
  }
 const std::size_t n0 = f_n;
 if( k >= f_n / 8 ) {  // the whole set merged with the batch, and built
  std::vector< K > A;
  gather( ex , 0 , f_L , {} , A );
  std::vector< K > M = setOp( ex , A , B , in );
  build( ex , std::move( M ) );
  return( in ? f_n - n0 : n0 - f_n );
  }

 // the merge: blocks of the batch whose borders are those of the leaves,
 // each leaf written in place by the block that owns it if it fits with its
 // head, else kept apart, so that no block writes a head or a leaf that
 // another one reads
 std::size_t nb , sz;
 std::tie( nb , sz ) = blocksOf( 0 , k , kPointBatch , 4 * kBlocks );
 std::vector< std::size_t > start( nb + 1 , k );
 start[ 0 ] = 0;
 parallelFor( ex , 1 , nb , [ & ]( std::size_t i ) {
  const std::size_t s = i * sz , l = leafOf( B[ s ] );
  start[ i ] =
    l == 0 ? 0
	   : std::size_t( std::lower_bound(
			    B.begin() , B.begin() + long( s + 1 ) ,
			    head( l ) ) -
			  B.begin() );
  } );
 std::vector< Part > part( nb );
 parallelFor(
   ex , 0 , nb ,
   [ & ]( std::size_t i ) {
    mergeLeaves( B , start[ i ] , start[ i + 1 ] , in , part[ i ] );
    } ,
   1 , nb );
 std::vector< std::size_t > T;  // the leaves touched, increasing
 std::vector< Spill > Out;      // the leaves apart, increasing
 std::size_t changed = 0;
 for( auto & p : part ) {
  changed += p.changed;
  T.insert( T.end() , p.touched.begin() , p.touched.end() );
  for( auto & s : p.out )
   Out.push_back( std::move( s ) );
  }
 for( const Spill & s : Out )
  f_units[ s.leaf ] =
    std::uint32_t( std::min< std::size_t >( s.units , ~std::uint32_t( 0 ) ) );
 f_n = in ? f_n + changed : f_n - changed;
 if( T.empty() )
  return( 0 );

 // the counting, a level at a time from the leaves touched
 std::vector< Node > cur( T.size() );
 for( std::size_t i = 0 ; i < T.size() ; ++i )
  cur[ i ] = { T[ i ] , f_units[ T[ i ] ] };
 for( const Spill & s : Out )  // their units, unsaturated
  std::lower_bound(
    cur.begin() , cur.end() , s.leaf ,
    []( const Node & a , std::size_t v ) { return( a.index < v ); } )
    ->units = s.units;
 std::vector< std::pair< std::size_t , std::size_t > > R;  // ( a , r )
 bool root = false;
 for( std::size_t h = 0 ; ! cur.empty() ; ++h ) {
  std::vector< char > bad( cur.size() );
  parallelFor( ex , 0 , cur.size() , [ & ]( std::size_t i ) {
   const std::size_t u = cur[ i ].units;
   bad[ i ] = in ? u > upper( h ) : u < lower( h );
   } );
  std::vector< std::size_t > up;  // the parents of the nodes out of bounds
  for( std::size_t i = 0 ; i < cur.size() ; ++i )
   if( ! bad[ i ] ) {
    if( ( h > 0 ) || spillOf( Out , cur[ i ].index ) )
     R.emplace_back( cur[ i ].index << h , std::size_t( 1 ) << h );
    }
   else if( up.empty() || ( up.back() != cur[ i ].index / 2 ) )
    up.push_back( cur[ i ].index / 2 );
  if( ! up.empty() && ( h == f_H ) ) {
   root = true;
   break;
   }
  std::vector< Node > next( up.size() );
  parallelFor( ex , 0 , up.size() , [ & ]( std::size_t i ) {
   std::size_t u = 0;
   for( std::size_t c = 2 * up[ i ] ; c < 2 * up[ i ] + 2 ; ++c ) {
    const auto it = std::lower_bound(
      cur.begin() , cur.end() , c ,
      []( const Node & a , std::size_t v ) { return( a.index < v ); } );
    u += ( it != cur.end() ) && ( it->index == c )
	   ? it->units
	   : unitsOf( c << h , std::size_t( 1 ) << h );  // counted once
    }
   next[ i ] = { up[ i ] , u };
   } );
  cur = std::move( next );
  }
 if( root ) {  // the root out of its bounds: a building
  std::vector< K > A;
  gather( ex , 0 , f_L , Out , A );
  build( ex , std::move( A ) );
  return( changed );
  }

 // the redistribution of the largest regions, in parallel
 std::sort( R.begin() , R.end() , []( const auto & a , const auto & b ) {
  return( a.first < b.first ||
	   ( a.first == b.first && a.second > b.second ) );
  } );
 std::vector< std::pair< std::size_t , std::size_t > > S;
 for( const auto & z : R )  // the regions are nested or disjoint
  if( S.empty() || ( z.first >= S.back().first + S.back().second ) )
   S.push_back( z );
 auto use = [ & ]( std::size_t a , std::size_t r ) {
  for( auto it = std::lower_bound(
	 Out.begin() , Out.end() , a ,
	 []( const Spill & s , std::size_t v ) { return( s.leaf < v ); } ) ;
       ( it != Out.end() ) && ( it->leaf < a + r ) ; ++it )
   it->used = true;
  };
 std::vector< char > fail( S.size() , 0 );
 if( S.size() == 1 ) {
  std::vector< K > x;
  gather( ex , S[ 0 ].first , S[ 0 ].second , Out , x );
  fail[ 0 ] = ! spread( ex , S[ 0 ].first , S[ 0 ].second , x );
  }
 else
  parallelFor(
    ex , 0 , S.size() ,
    [ & ]( std::size_t i ) {
     Sequential s;
     std::vector< K > x;
     gather( s , S[ i ].first , S[ i ].second , Out , x );
     fail[ i ] = ! spread( s , S[ i ].first , S[ i ].second , x );
     } ,
    1 );
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  if( ! fail[ i ] )
   use( S[ i ].first , S[ i ].second );
 std::vector< std::pair< std::size_t , std::size_t > > done_;
 auto within = [ & ]( std::size_t a ) {  // in a region spread from above
  for( const auto & d : done_ )
   if( ( d.first <= a ) && ( a < d.first + d.second ) )
    return( true );
  return( false );
  };
 for( std::size_t i = 0 ; i < S.size() ; ++i ) {
  if( ! fail[ i ] || within( S[ i ].first ) )
   continue;
  // a share that does not fit, or too few keys: the regions above
  bool done = false;
  std::size_t a = S[ i ].first , r = S[ i ].second;
  while( ! done && ( r < f_L ) ) {
   r *= 2;
   a = a / r * r;
   std::vector< K > x;
   gather( ex , a , r , Out , x );
   done = spread( ex , a , r , x );
   }
  if( ! done ) {
   std::vector< K > A;
   gather( ex , 0 , f_L , Out , A );
   build( ex , std::move( A ) );
   return( changed );
   }
  use( a , r );
  done_.emplace_back( a , r );
  }
 ADS_INVARIANT( wellFormed() );
 return( changed );
 }

/*------------------------------- THE ORDER --------------------------------*/

ADS_PMA_T
bool ADS_PMA::wellFormed( void ) const
{
 if( f_n == 0 )
  return( f_L == 0 );
 if( ! std::has_single_bit( f_L ) ||
     ( f_H != std::size_t( std::countr_zero( f_L ) ) ) ||
     ( f_units.size() != f_L ) || ( f_tail.size() != f_L * tailCells() ) )
  return( false );
 std::size_t n = 0;
 std::vector< K > x;
 for( std::size_t l = 0 ; l < f_L ; ++l ) {
  if( ( f_units[ l ] == 0 ) || ( f_units[ l ] > f_C ) )
   return( false );
  x.clear();
  decodeTo( l , x );
  if( units( x.data() , x.size() ) != f_units[ l ] )
   return( false );
  for( std::size_t i = 1 ; i < x.size() ; ++i )
   if( ! ( x[ i - 1 ] < x[ i ] ) )
    return( false );
  if( ( l + 1 < f_L ) && ! ( x.back() < head( l + 1 ) ) )
   return( false );
  if( leafAt( pos( l ) ) != l )
   return( false );
  n += x.size();
  }
 return( n == f_n );
 }

#undef ADS_PMA
#undef ADS_PMA_T

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File PackedMemoryArray.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
