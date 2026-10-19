/*--------------------------------------------------------------------------*/
/*----------------------- File BucketCuckooTable.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BucketCuckooTable [see BucketCuckooTable.h], with their
 * contracts.
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

namespace ADS {

namespace detail {

/// the mask of the bytes equal to 0 in the word w, their bits 7 set,
/// exactly (no false byte after a borrow)
[[nodiscard]] constexpr std::uint32_t zeroBytes( std::uint32_t w )
{
 return( ~( ( ( w & 0x7F7F7F7FU ) + 0x7F7F7F7FU ) | w | 0x7F7F7F7FU ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
BucketCuckooTable< K , V , H , Eq >::BucketCuckooTable( std::size_t n , H h ,
                                                        Eq eq )
 : f_h( h ) , f_eq( eq )
{
 const std::size_t b = std::bit_ceil( std::max< std::size_t >(
                        2 , ( n * 10 + 35 ) / 36 ) );   // at load 0.9
 f_lg = std::countr_zero( b );
 f_T.assign( b , 0 );
 f_S.resize( 4 * b );
 }

template< class K , class V , class H , class Eq >
std::uint64_t BucketCuckooTable< K , V , H , Eq >::hash( const K & k ) const
{
 if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( hashWord( std::uint64_t( f_h( k ) ) ) );
 }

template< class K , class V , class H , class Eq >
std::uint32_t BucketCuckooTable< K , V , H , Eq >::tag( std::uint64_t h )
{
 const auto t = std::uint32_t( h & 0xFF );
 return( t ? t : 1 );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::first( std::uint64_t h )
 const
{
 return( std::size_t( h >> ( 64 - f_lg ) ) );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::other( std::size_t b ,
                                                        std::uint32_t t )
 const
{
 // an odd offset below the buckets, so that the two differ, by the top
 // bits of the product of the tag by the golden ratio
 const auto d = std::size_t( ( std::uint64_t( t ) * 0x9E3779B97F4A7C15ULL )
                             >> ( 64 - f_lg ) );
 return( b ^ ( d | 1 ) );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::locate( const K & k ,
                                                         std::uint64_t h )
 const
{
 const std::uint32_t t = tag( h );
 std::size_t b = first( h );
 for( int j = 0 ; j < 2 ; ++j , b = other( b , t ) ) {
  std::uint32_t m = detail::zeroBytes( f_T[ b ] ^ ( t * 0x01010101U ) );
  for( ; m ; m &= m - 1 ) {
   const std::size_t i = 4 * b + std::size_t( std::countr_zero( m ) >> 3 );
   if( f_eq( f_S[ i ].key , k ) )
    return( i );
   }
  }
 return( ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq >
int BucketCuckooTable< K , V , H , Eq >::freePlace( std::size_t b ) const
{
 const std::uint32_t z = detail::zeroBytes( f_T[ b ] );
 return( z ? std::countr_zero( z ) >> 3 : 4 );
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
void BucketCuckooTable< K , V , H , Eq >::put( std::size_t b , int i ,
                                               std::uint32_t t , KK && k ,
                                               VV && v )
{
 Slot & s = f_S[ 4 * b + std::size_t( i ) ];
 s.key = std::forward< KK >( k );
 s.value = std::forward< VV >( v );
 f_T[ b ] |= t << ( 8 * i );
 }

/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::makeRoom( std::uint64_t h )
{
 // the nodes of the search: a bucket, the node it comes from and the place
 // of that bucket whose key moves into it
 struct Node {
  std::size_t b;
  int parent , from , depth;
  };
 Node N[ kNodes ];
 int nn = 0;
 const std::size_t b1 = first( h );
 N[ nn++ ] = Node{ b1 , -1 , -1 , 0 };
 N[ nn++ ] = Node{ other( b1 , tag( h ) ) , -1 , -1 , 0 };
 auto onPath = [ & ]( int x , std::size_t b ) {
  for( ; x >= 0 ; x = N[ x ].parent )
   if( N[ x ].b == b )
    return( true );
  return( false ); };
 for( int q = 0 ; q < nn ; ++q ) {
  const std::size_t b = N[ q ].b;
  for( int i = 0 ; i < 4 ; ++i ) {
   const std::uint32_t t = ( f_T[ b ] >> ( 8 * i ) ) & 0xFF;
   const std::size_t a = other( b , t );
   const int j = freePlace( a );
   if( j < 4 ) {                          // the moves, from the end
    Slot & s = f_S[ 4 * b + std::size_t( i ) ];
    put( a , j , t , std::move( s.key ) , std::move( s.value ) );
    f_T[ b ] &= ~( 0xFFU << ( 8 * i ) );
    ++f_moves;
    int x = q , free = i;
    for( ; N[ x ].parent >= 0 ; x = N[ x ].parent ) {
     const std::size_t p = N[ N[ x ].parent ].b;
     const int from = N[ x ].from;
     const std::uint32_t u = ( f_T[ p ] >> ( 8 * from ) ) & 0xFF;
     Slot & r = f_S[ 4 * p + std::size_t( from ) ];
     put( N[ x ].b , free , u , std::move( r.key ) , std::move( r.value ) );
     f_T[ p ] &= ~( 0xFFU << ( 8 * from ) );
     ++f_moves;
     free = from;
     }
    return( 4 * N[ x ].b + std::size_t( free ) );
    }
   if( ( N[ q ].depth + 1 < kDepth ) && ( nn < int( kNodes ) ) &&
       ! onPath( q , a ) )
    N[ nn++ ] = Node{ a , q , i , N[ q ].depth + 1 };
   }
  }
 return( ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
bool BucketCuckooTable< K , V , H , Eq >::place( std::uint64_t h , KK && k ,
                                                 VV && v )
{
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h ) , b2 = other( b1 , t );
 int i = freePlace( b1 );
 if( i < 4 ) {
  put( b1 , i , t , std::forward< KK >( k ) , std::forward< VV >( v ) );
  return( true );
  }
 if( ( i = freePlace( b2 ) ) < 4 ) {
  put( b2 , i , t , std::forward< KK >( k ) , std::forward< VV >( v ) );
  return( true );
  }
 if( 50 * ( f_n + 1 ) > 49 * f_S.size() ) // past the load 0.98
  return( false );
 const std::size_t r = makeRoom( h );
 if( r == ~std::size_t( 0 ) )
  return( false );
 put( r >> 2 , int( r & 3 ) , t , std::forward< KK >( k ) ,
      std::forward< VV >( v ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
void BucketCuckooTable< K , V , H , Eq >::grow( void )
{
 // the pairs moved from the old places into a table twice as large; if one
 // finds no place, the ones placed and the ones left are gathered and a
 // larger table is tried
 std::vector< std::uint32_t > T;
 std::vector< Slot > S;
 T.swap( f_T );
 S.swap( f_S );
 auto fresh = [ & ]( int lg ) {
  f_lg = lg;
  f_T.assign( std::size_t( 1 ) << lg , 0 );
  f_S.assign( std::size_t( 4 ) << lg , Slot{} );
  ++f_rehashes;
  };
 fresh( f_lg + 1 );
 std::size_t i = 0;
 for( ; i < S.size() ; ++i )
  if( ( ( T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF ) &&
      ! place( hash( S[ i ].key ) , std::move( S[ i ].key ) ,
               std::move( S[ i ].value ) ) )
   break;
 if( i == S.size() )
  return;
 std::vector< Slot > P;                   // the rare case
 P.reserve( f_n );
 auto gather = [ & ]( void ) {
  for( std::size_t j = 0 ; j < f_S.size() ; ++j )
   if( ( f_T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF )
    P.push_back( std::move( f_S[ j ] ) );
  };
 for( ; i < S.size() ; ++i )
  if( ( T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF )
   P.push_back( std::move( S[ i ] ) );
 gather();
 for( int lg = f_lg + 1 ; ; ++lg ) {
  fresh( lg );
  std::size_t j = 0;
  for( ; j < P.size() ; ++j )
   if( ! place( hash( P[ j ].key ) , std::move( P[ j ].key ) ,
                std::move( P[ j ].value ) ) )
    break;
  if( j == P.size() )
   return;
  std::vector< Slot > R( std::make_move_iterator( P.begin() + long( j ) ) ,
                         std::make_move_iterator( P.end() ) );
  P.swap( R );                            // the ones not placed,
  gather();                               // and the ones placed
  }
 }

/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq >
std::optional< V > BucketCuckooTable< K , V , H , Eq >::lookup( const K & k )
 const
{
 const V * v = find( k );
 return( v ? std::optional< V >( *v ) : std::nullopt );
 }

template< class K , class V , class H , class Eq >
const V * BucketCuckooTable< K , V , H , Eq >::find( const K & k ) const
{
 const std::size_t i = locate( k , hash( k ) );
 return( i == ~std::size_t( 0 ) ? nullptr : &f_S[ i ].value );
 }

template< class K , class V , class H , class Eq >
V * BucketCuckooTable< K , V , H , Eq >::find( const K & k )
{
 const std::size_t i = locate( k , hash( k ) );
 return( i == ~std::size_t( 0 ) ? nullptr : &f_S[ i ].value );
 }

template< class K , class V , class H , class Eq >
bool BucketCuckooTable< K , V , H , Eq >::insert( const K & k , const V & v )
{
 const std::uint64_t h = hash( k );
 const std::size_t i = locate( k , h );
 if( i != ~std::size_t( 0 ) ) {
  f_S[ i ].value = v;
  ADS_INVARIANT( placed( k ) );
  return( false );
  }
 while( ! place( h , k , v ) )
  grow();
 ++f_n;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( placed( k ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool BucketCuckooTable< K , V , H , Eq >::remove( const K & k )
{
 const std::size_t i = locate( k , hash( k ) );
 if( i == ~std::size_t( 0 ) )
  return( false );
 f_T[ i >> 2 ] &= ~( 0xFFU << ( 8 * ( i & 3 ) ) );
 f_S[ i ] = Slot{};                       // what the pair holds goes now
 --f_n;
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( placed( k ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool BucketCuckooTable< K , V , H , Eq >::contains( const K & k ) const
{
 return( locate( k , hash( k ) ) != ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::size( void ) const
{
 return( f_n );
 }

template< class K , class V , class H , class Eq >
bool BucketCuckooTable< K , V , H , Eq >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::capacity( void ) const
{
 return( f_S.size() );
 }

template< class K , class V , class H , class Eq >
double BucketCuckooTable< K , V , H , Eq >::load( void ) const
{
 return( double( f_n ) / double( f_S.size() ) );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::rehashes( void ) const
{
 return( f_rehashes );
 }

template< class K , class V , class H , class Eq >
std::size_t BucketCuckooTable< K , V , H , Eq >::moves( void ) const
{
 return( f_moves );
 }

template< class K , class V , class H , class Eq >
bool BucketCuckooTable< K , V , H , Eq >::placed( const K & k ) const
{
 auto ok = [ & ]( const K & x ) {             // in one bucket, once
  const std::uint64_t h = hash( x );
  const std::uint32_t t = tag( h );
  const std::size_t b1 = first( h ) , b2 = other( b1 , t );
  int c = 0;
  for( std::size_t b : { b1 , b2 } ) {
   for( int i = 0 ; i < 4 ; ++i )
    if( ( ( f_T[ b ] >> ( 8 * i ) ) & 0xFF ) == t &&
        f_eq( f_S[ 4 * b + std::size_t( i ) ].key , x ) )
     ++c;
   if( b2 == b1 )
    break;
   }
  return( c ); };
 if( ok( k ) > 1 )
  return( false );
 if( f_S.size() > 1024 )
  return( true );
 std::size_t n = 0;
 for( std::size_t i = 0 ; i < f_S.size() ; ++i ) {
  const std::uint32_t t = ( f_T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF;
  if( t == 0 )
   continue;
  ++n;
  const std::uint64_t h = hash( f_S[ i ].key );
  if( ( tag( h ) != t ) || ( ok( f_S[ i ].key ) != 1 ) )
   return( false );
  const std::size_t b1 = first( h );
  if( ( ( i >> 2 ) != b1 ) && ( ( i >> 2 ) != other( b1 , t ) ) )
   return( false );
  }
 return( n == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File BucketCuckooTable.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
