/*--------------------------------------------------------------------------*/
/*----------------------- File BucketCuckooTable.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BucketCuckooTable [see BucketCuckooTable.h], with their
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

namespace detail
{

/// the mask of the bytes equal to 0 in the word w, their bits 7 set,
/// exactly (no false byte after a borrow)
[[nodiscard]] constexpr std::uint32_t zeroBytes( std::uint32_t w )
{
 return( ~( ( ( w & 0x7F7F7F7FU ) + 0x7F7F7F7FU ) | w | 0x7F7F7F7FU ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A >
BucketCuckooTable< K , V , H , Eq , A >::BucketCuckooTable( std::size_t n ,
							    H h ,
							Eq eq ,
							const A & alloc )
    : f_T( alloc ) , f_S( alloc ) , f_X( alloc ) , f_XT( alloc ) , f_h( h ) ,
      f_eq( eq )
{
 const std::size_t b = std::bit_ceil(
   std::max< std::size_t >( 2 , ( n * 10 + 35 ) / 36 ) );  // at load 0.9
 f_lg = std::countr_zero( b );
 f_T.assign( b , 0 );
 f_S.resize( 4 * b );
 }

template< class K , class V , class H , class Eq , class A >
std::uint64_t BucketCuckooTable< K , V , H , Eq ,
				 A >::hash( const K & k ) const
{
 if constexpr( detail::kAvalanching64< H , K > )
  return( detail::hash64( f_h , k ) );
 else
  return( hashWord( detail::hash64( f_h , k ) ) );
 }

template< class K , class V , class H , class Eq , class A >
std::uint32_t BucketCuckooTable< K , V , H , Eq , A >::tag( std::uint64_t h )
{
 const auto t = std::uint32_t( h & 0xFF );
 return( t ? t : 1 );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t
BucketCuckooTable< K , V , H , Eq , A >::first( std::uint64_t h ) const
{
 return( std::size_t( h >> ( 64 - f_lg ) ) );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t
BucketCuckooTable< K , V , H , Eq , A >::other( std::size_t b ,
					    std::uint32_t t ) const
{
 // an odd offset below the buckets, so that the two differ, by the top
 // bits of the product of the tag by the golden ratio
 const auto d = std::size_t( ( std::uint64_t( t ) * 0x9E3779B97F4A7C15ULL ) >>
			     ( 64 - f_lg ) );
 return( b ^ ( d | 1 ) );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t
BucketCuckooTable< K , V , H , Eq , A >::locate( const K & k ,
					     std::uint64_t h ) const
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
 if( f_xn == 0 ) [[likely]]  // the stash, empty but for degenerate hashes
  return( ~std::size_t( 0 ) );
 for( std::size_t j = 0 ; j < f_X.size() ; ++j )
  if( f_XT[ j ] && f_eq( f_X[ j ].key , k ) )
   return( f_S.size() + j );
 return( ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq , class A >
auto BucketCuckooTable< K , V , H , Eq , A >::at( std::size_t i ) const
  -> const Slot &
{
 return( i < f_S.size() ? f_S[ i ] : f_X[ i - f_S.size() ] );
 }

template< class K , class V , class H , class Eq , class A >
auto BucketCuckooTable< K , V , H , Eq , A >::at( std::size_t i ) -> Slot &
{
 return( i < f_S.size() ? f_S[ i ] : f_X[ i - f_S.size() ] );
 }

template< class K , class V , class H , class Eq , class A >
template< class KK , class VV >
void BucketCuckooTable< K , V , H , Eq , A >::stash( KK && k , VV && v )
{
 // the first empty place of the stash, else a new one at its end
 std::size_t j = 0;
 if( f_xn < f_X.size() )
  for( ; f_XT[ j ] ; ++j )
   ;
 else {
  j = f_X.size();
  f_X.emplace_back();
  f_XT.push_back( 0 );
  }
 f_X[ j ].key = std::forward< KK >( k );
 f_X[ j ].value = std::forward< VV >( v );
 f_XT[ j ] = 1;
 ++f_xn;
 }

template< class K , class V , class H , class Eq , class A >
int BucketCuckooTable< K , V , H , Eq , A >::freePlace( std::size_t b ) const
{
 const std::uint32_t z = detail::zeroBytes( f_T[ b ] );
 return( z ? std::countr_zero( z ) >> 3 : 4 );
 }

template< class K , class V , class H , class Eq , class A >
template< class KK , class VV >
void BucketCuckooTable< K , V , H , Eq , A >::put( std::size_t b , int i ,
					       std::uint32_t t , KK && k ,
					       VV && v )
{
 Slot & s = f_S[ 4 * b + std::size_t( i ) ];
 s.key = std::forward< KK >( k );
 s.value = std::forward< VV >( v );
 f_T[ b ] |= t << ( 8 * i );
 }

/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A >
std::size_t BucketCuckooTable< K , V , H , Eq ,
			       A >::makeRoom( std::uint64_t h )
{
 // the nodes of the search: a bucket, the node it comes from and the place
 // of that bucket whose key moves into it
 struct Node
 {
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
  return( false );
  };
 for( int q = 0 ; q < nn ; ++q ) {
  const std::size_t b = N[ q ].b;
  for( int i = 0 ; i < 4 ; ++i ) {
   const std::uint32_t t = ( f_T[ b ] >> ( 8 * i ) ) & 0xFF;
   const std::size_t a = other( b , t );
   const int j = freePlace( a );
   if( j < 4 ) {  // the moves, from the end
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

template< class K , class V , class H , class Eq , class A >
template< class KK , class VV >
bool BucketCuckooTable< K , V , H , Eq , A >::place( std::uint64_t h ,
						     KK && k ,
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
 if( 50 * ( f_n + 1 ) > 49 * f_S.size() )  // past the load 0.98
  return( false );
 const std::size_t r = makeRoom( h );
 if( r == ~std::size_t( 0 ) )
  return( false );
 put( r >> 2 , int( r & 3 ) , t , std::forward< KK >( k ) ,
      std::forward< VV >( v ) );
 return( true );
 }

template< class K , class V , class H , class Eq , class A >
void BucketCuckooTable< K , V , H , Eq , A >::grow( void )
{
 // the pairs moved from the old places and from the stash into a table
 // twice as large; one that finds no place goes to the new stash, so that
 // a doubling allocates the table once
 Tags T( f_T.get_allocator() ) , XT( f_XT.get_allocator() );
 Slots S( f_S.get_allocator() ) , X( f_X.get_allocator() );
 T.swap( f_T );
 S.swap( f_S );
 X.swap( f_X );
 XT.swap( f_XT );
 f_xn = 0;
 ++f_lg;
 f_T.assign( std::size_t( 1 ) << f_lg , 0 );
 f_S.assign( std::size_t( 4 ) << f_lg , Slot{} );
 ++f_rehashes;
 // two passes, so that the misses overlap: the hashes of the keys, the
 // memory of the key AHEAD places later prefetched if its hash reads it
 // [see detail::kIndirectKey]; then the placements, the first bucket of
 // the key AHEAD places later prefetched
 constexpr std::size_t AHEAD = 8;
 auto full = [ & ]( std::size_t j ) {
  return( ( ( T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF ) != 0 );
  };
 detail::AllocVector< A , std::uint64_t > Y( S.size() , S.get_allocator() );
 for( std::size_t j = 0 ; j < S.size() ; ++j ) {
  if( j + AHEAD < S.size() )
   detail::prefetchKey( S[ j + AHEAD ].key );
  if( full( j ) )
   Y[ j ] = hash( S[ j ].key );
  }
 for( std::size_t i = 0 ; i < S.size() ; ++i ) {
  if( ( i + AHEAD < S.size() ) && full( i + AHEAD ) ) {
   const std::size_t b = first( Y[ i + AHEAD ] );
   ADS::detail::prefetchLine( &f_T[ b ] );
   ADS::detail::prefetchLine( &f_S[ 4 * b ] );
   }
  if( full( i ) &&
      ! place( Y[ i ] , std::move( S[ i ].key ) , std::move( S[ i ].value ) ) )
   stash( std::move( S[ i ].key ) , std::move( S[ i ].value ) );
  }
 for( std::size_t j = 0 ; j < X.size() ; ++j )
  if( XT[ j ] && ! place( hash( X[ j ].key ) , std::move( X[ j ].key ) ,
			  std::move( X[ j ].value ) ) )
   stash( std::move( X[ j ].key ) , std::move( X[ j ].value ) );
 }

/*--------------------------------------------------------------------------*/

template< class K , class V , class H , class Eq , class A >
std::optional< V >
BucketCuckooTable< K , V , H , Eq , A >::lookup( const K & k ) const
{
 const V * v = find( k );
 return( v ? std::optional< V >( *v ) : std::nullopt );
 }

template< class K , class V , class H , class Eq , class A >
const V * BucketCuckooTable< K , V , H , Eq , A >::find( const K & k ) const
{
 const std::size_t i = locate( k , hash( k ) );
 return( i == ~std::size_t( 0 ) ? nullptr : &at( i ).value );
 }

template< class K , class V , class H , class Eq , class A >
V * BucketCuckooTable< K , V , H , Eq , A >::find( const K & k )
{
 const std::size_t i = locate( k , hash( k ) );
 return( i == ~std::size_t( 0 ) ? nullptr : &at( i ).value );
 }

template< class K , class V , class H , class Eq , class A >
bool BucketCuckooTable< K , V , H , Eq , A >::insert( const K & k ,
						      const V & v )
{
 const std::uint64_t h = hash( k );
 const std::size_t i = locate( k , h );
 if( i != ~std::size_t( 0 ) ) {
  at( i ).value = v;
  ADS_INVARIANT( placed( k ) );
  return( false );
  }
 // at most kGrowths doublings, and only while the table would be more
 // than half full: equal hashes find no place at any size
 for( int g = 0 ; ! place( h , k , v ) ; ++g ) {
  if( ( g == kGrowths ) || ( 2 * ( f_n + 1 ) <= f_S.size() ) ) {
   stash( k , v );
   break;
   }
  grow();
  }
 ++f_n;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( placed( k ) );
 return( true );
 }

template< class K , class V , class H , class Eq , class A >
bool BucketCuckooTable< K , V , H , Eq , A >::remove( const K & k )
{
 const std::size_t i = locate( k , hash( k ) );
 if( i == ~std::size_t( 0 ) )
  return( false );
 if( i < f_S.size() ) {
  f_T[ i >> 2 ] &= ~( 0xFFU << ( 8 * ( i & 3 ) ) );
  f_S[ i ] = Slot{};  // what the pair holds goes now
  }
 else {  // a hole of the stash, which keeps the iterators on the others
  f_XT[ i - f_S.size() ] = 0;
  f_X[ i - f_S.size() ] = Slot{};
  --f_xn;
  }
 --f_n;
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( placed( k ) );
 return( true );
 }

template< class K , class V , class H , class Eq , class A >
bool BucketCuckooTable< K , V , H , Eq , A >::contains( const K & k ) const
{
 return( locate( k , hash( k ) ) != ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t BucketCuckooTable< K , V , H , Eq , A >::size( void ) const
{
 return( f_n );
 }

template< class K , class V , class H , class Eq , class A >
bool BucketCuckooTable< K , V , H , Eq , A >::empty( void ) const
{
 return( f_n == 0 );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t BucketCuckooTable< K , V , H , Eq , A >::capacity( void ) const
{
 return( f_S.size() );
 }

template< class K , class V , class H , class Eq , class A >
double BucketCuckooTable< K , V , H , Eq , A >::load( void ) const
{
 return( double( f_n ) / double( f_S.size() ) );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t BucketCuckooTable< K , V , H , Eq , A >::rehashes( void ) const
{
 return( f_rehashes );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t BucketCuckooTable< K , V , H , Eq , A >::moves( void ) const
{
 return( f_moves );
 }

template< class K , class V , class H , class Eq , class A >
std::size_t BucketCuckooTable< K , V , H , Eq , A >::stashed( void ) const
{
 return( f_xn );
 }

template< class K , class V , class H , class Eq , class A >
bool BucketCuckooTable< K , V , H , Eq , A >::placed( const K & k ) const
{
 auto ok = [ & ]( const K & x ) {  // in one bucket, once
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
  for( std::size_t j = 0 ; j < f_X.size() ; ++j )
   c += f_XT[ j ] && f_eq( f_X[ j ].key , x );
  return( c );
  };
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
 std::size_t x = 0;
 for( std::size_t j = 0 ; j < f_X.size() ; ++j )
  if( f_XT[ j ] ) {
   ++x;
   if( ok( f_X[ j ].key ) != 1 )
    return( false );
   }
 return( ( x == f_xn ) && ( n + x == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File BucketCuckooTable.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
