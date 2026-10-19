/*--------------------------------------------------------------------------*/
/*--------------------- File ConcurrentCuckooTable.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConcurrentCuckooTable [see ConcurrentCuckooTable.h],
 * with their contracts.
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

#include <algorithm>
#include <bit>
#include <stdexcept>
#include <thread>
#include <utility>

#if defined( _MSC_VER ) && ( defined( _M_X64 ) || defined( _M_IX86 ) )
 #include <intrin.h>
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the hint to the processor that the thread spins on a lock
inline void spinPause( void )
{
#if defined( __x86_64__ ) || defined( __i386__ )
 __builtin_ia32_pause();
#elif defined( _MSC_VER ) && ( defined( _M_X64 ) || defined( _M_IX86 ) )
 _mm_pause();
#elif defined( __aarch64__ )
 asm volatile( "yield" );
#endif
 }

}  // end( namespace detail )

/*------------------------------- THE LOCKS --------------------------------*/

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::lock( Stripe & s )
{
 for( int n = 0 ; ; ) {
  if( ! s.held.exchange( true , std::memory_order_acquire ) )
   return;
  while( s.held.load( std::memory_order_relaxed ) )
   if( ++n < 64 )
    detail::spinPause();
   else
    std::this_thread::yield();
  }
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::tryLock( Stripe & s )
{
 return( ! s.held.load( std::memory_order_relaxed ) &&
         ! s.held.exchange( true , std::memory_order_acquire ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::unlock( Stripe & s )
{
 s.held.store( false , std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::Hold::Hold( Hold && o ) noexcept
 : f_a( std::exchange( o.f_a , nullptr ) ) ,
   f_b( std::exchange( o.f_b , nullptr ) )
{
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::Hold::~Hold()
{
 if( f_b )
  unlock( *f_b );
 if( f_a )
  unlock( *f_a );
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::HoldAll(
                                       const ConcurrentCuckooTable & t )
 : f_L( t.f_L.get() )
{
 // the stripe 0 first: the buckets change only under all the locks in
 // use, which include it
 lock( f_L[ 0 ] );
 f_n = 1;
 extend( std::size_t( 1 ) << t.f_lg.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::extend(
                                                         std::size_t b )
{
 for( const std::size_t m = std::min( b , kStripes ) ; f_n < m ; ++f_n )
  lock( f_L[ f_n ] );                     // in increasing order
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::~HoldAll()
{
 while( f_n > 0 )
  unlock( f_L[ --f_n ] );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentCuckooTable< K , V , H , Eq >::lockBuckets( std::size_t a ,
                                                           std::size_t b )
 const -> Hold
{
 std::size_t sa = a & ( kStripes - 1 ) , sb = b & ( kStripes - 1 );
 if( sa > sb )
  std::swap( sa , sb );
 lock( f_L[ sa ] );
 if( sb == sa )
  return( Hold( &f_L[ sa ] , nullptr ) );
 lock( f_L[ sb ] );
 return( Hold( &f_L[ sa ] , &f_L[ sb ] ) );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentCuckooTable< K , V , H , Eq >::lockKey( std::uint64_t h ,
                                                       std::size_t & b1 ,
                                                       std::size_t & b2 )
 const -> Hold
{
 // the buckets of the size seen, and the size seen again under their
 // locks: a doubling holds all the locks, so that it is over or not begun
 for( ; ; ) {
  const int lg = f_lg.load( std::memory_order_acquire );
  b1 = first( h , lg );
  b2 = other( b1 , tag( h ) , lg );
  Hold g = lockBuckets( b1 , b2 );
  if( f_lg.load( std::memory_order_relaxed ) == lg )
   return( g );
  }
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::count( std::size_t b ,
                                                     std::int64_t c ) const
{
 // only the holder of the lock writes the counter: no atomic addition
 Stripe & s = f_L[ b & ( kStripes - 1 ) ];
 s.count.store( s.count.load( std::memory_order_relaxed ) + c ,
                std::memory_order_relaxed );
 }

/*------------------------------ THE BUCKETS -------------------------------*/

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::ConcurrentCuckooTable(
                                          std::size_t n , H h , Eq eq )
 : f_L( std::make_unique< Stripe[] >( kStripes ) ) , f_h( h ) , f_eq( eq )
{
 const std::size_t b = std::bit_ceil( std::max< std::size_t >(
                        2 , ( n * 10 + 35 ) / 36 ) );   // at load 0.9
 f_lg.store( std::countr_zero( b ) , std::memory_order_relaxed );
 f_T.assign( b , 0 );
 f_S.resize( 4 * b );
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentCuckooTable< K , V , H , Eq >::hash( const K & k )
 const
{
 if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( hashWord( std::uint64_t( f_h( k ) ) ) );
 }

template< class K , class V , class H , class Eq >
std::uint32_t ConcurrentCuckooTable< K , V , H , Eq >::tag( std::uint64_t h )
{
 const auto t = std::uint32_t( h & 0xFF );
 return( t ? t : 1 );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::first( std::uint64_t h ,
                                                            int lg )
{
 return( std::size_t( h >> ( 64 - lg ) ) );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::other( std::size_t b ,
                                                            std::uint32_t t ,
                                                            int lg )
{
 // an odd offset below the buckets, so that the two differ, by the top
 // bits of the product of the tag by the golden ratio
 const auto d = std::size_t( ( std::uint64_t( t ) * 0x9E3779B97F4A7C15ULL )
                             >> ( 64 - lg ) );
 return( b ^ ( d | 1 ) );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::locate( const K & k ,
                                                             std::uint32_t t ,
                                                             std::size_t b1 ,
                                                             std::size_t b2 )
 const
{
 for( std::size_t b : { b1 , b2 } ) {
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
int ConcurrentCuckooTable< K , V , H , Eq >::freePlace( std::size_t b ) const
{
 const std::uint32_t z = detail::zeroBytes( f_T[ b ] );
 return( z ? std::countr_zero( z ) >> 3 : 4 );
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
void ConcurrentCuckooTable< K , V , H , Eq >::put( std::size_t b , int i ,
                                                   std::uint32_t t , KK && k ,
                                                   VV && v )
{
 Slot & s = f_S[ 4 * b + std::size_t( i ) ];
 s.key = std::forward< KK >( k );
 s.value = std::forward< VV >( v );
 f_T[ b ] |= t << ( 8 * i );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::move( std::size_t a , int i ,
                                                    std::size_t d , int j )
{
 Slot & s = f_S[ 4 * a + std::size_t( i ) ];
 put( d , j , ( f_T[ a ] >> ( 8 * i ) ) & 0xFF , std::move( s.key ) ,
      std::move( s.value ) );
 f_T[ a ] &= ~( 0xFFU << ( 8 * i ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::inBucket( std::size_t b ) const
{
 const int lg = f_lg.load( std::memory_order_relaxed );
 for( int i = 0 ; i < 4 ; ++i ) {
  const std::uint32_t t = ( f_T[ b ] >> ( 8 * i ) ) & 0xFF;
  if( t == 0 )
   continue;
  const std::uint64_t h = hash( f_S[ 4 * b + std::size_t( i ) ].key );
  const std::size_t b1 = first( h , lg );
  if( ( tag( h ) != t ) || ( ( b != b1 ) && ( b != other( b1 , t , lg ) ) ) )
   return( false );
  }
 return( true );
 }

/*------------------------------ THE INSERTION -----------------------------*/

template< class K , class V , class H , class Eq >
template< class R >
int ConcurrentCuckooTable< K , V , H , Eq >::search( std::uint64_t h ,
                                                     int lg , Node * N ,
                                                     R && read ) const
{
 int nn = 0;
 const std::size_t b1 = first( h , lg );
 N[ nn++ ] = Node{ b1 , -1 , -1 , 0 , 0 };
 N[ nn++ ] = Node{ other( b1 , tag( h ) , lg ) , -1 , -1 , 0 , 0 };
 auto onPath = [ & ]( int x , std::size_t b ) {
  for( ; x >= 0 ; x = N[ x ].parent )
   if( N[ x ].b == b )
    return( true );
  return( false ); };
 for( int q = 0 ; q < nn ; ++q ) {
  std::uint32_t w = 0;
  if( ! read( N[ q ].b , w ) )
   return( -2 );
  if( detail::zeroBytes( w ) )
   return( q );
  if( N[ q ].depth == kDepth )
   continue;
  for( int i = 0 ; ( i < 4 ) && ( nn < kNodes ) ; ++i ) {
   const std::uint32_t t = ( w >> ( 8 * i ) ) & 0xFF;
   const std::size_t a = other( N[ q ].b , t , lg );
   if( ! onPath( q , a ) )
    N[ nn++ ] = Node{ a , q , i , N[ q ].depth + 1 , t };
   }
  }
 return( -1 );
 }

template< class K , class V , class H , class Eq >
int ConcurrentCuckooTable< K , V , H , Eq >::cuckoo( std::uint64_t h ,
                                                     int lg ,
                                                     std::uint32_t w1 ,
                                                     std::uint32_t w2 )
{
 // the search reads each bucket under its lock alone (but the two of the
 // key, read last under their locks), the moves lock the two buckets of
 // each; no lock is held from one step to the next
 Node N[ kNodes ];
 const std::size_t b1 = first( h , lg ) , b2 = other( b1 , tag( h ) , lg );
 const int q = search( h , lg , N , [ & ]( std::size_t b ,
                                           std::uint32_t & w ) {
  if( ( b == b1 ) || ( b == b2 ) ) {
   w = ( b == b1 ) ? w1 : w2;
   return( true );
   }
  Stripe & s = f_L[ b & ( kStripes - 1 ) ];
  lock( s );
  const bool same = f_lg.load( std::memory_order_relaxed ) == lg;
  if( same )
   w = f_T[ b ];
  unlock( s );
  return( same ); } );
 if( q == -2 )
  return( 1 );
 if( q == -1 )
  return( 2 );
 for( int x = q ; N[ x ].parent >= 0 ; x = N[ x ].parent ) {
  const std::size_t a = N[ N[ x ].parent ].b , d = N[ x ].b;
  const int i = N[ x ].from;
  Hold g = lockBuckets( a , d );
  if( ( f_lg.load( std::memory_order_relaxed ) != lg ) ||
      ( ( ( f_T[ a ] >> ( 8 * i ) ) & 0xFF ) != N[ x ].tag ) )
   return( 1 );
  const int j = freePlace( d );
  if( j == 4 )
   return( 1 );
  move( a , i , d , j );
  ADS_INVARIANT( inBucket( a ) && inBucket( d ) );
  }
 return( 0 );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::shift( std::size_t b1 ,
                                                            std::size_t b2 )
{
 // the other bucket is locked by a try, which never waits, so that the
 // order of the locks does not matter
 const int lg = f_lg.load( std::memory_order_relaxed );
 const std::size_t s1 = b1 & ( kStripes - 1 ) , s2 = b2 & ( kStripes - 1 );
 for( std::size_t b : { b1 , b2 } )
  for( int i = 0 ; i < 4 ; ++i ) {
   const std::size_t a = other( b , ( f_T[ b ] >> ( 8 * i ) ) & 0xFF , lg );
   const std::size_t sa = a & ( kStripes - 1 );
   const bool held = ( sa == s1 ) || ( sa == s2 );
   if( ! held && ! tryLock( f_L[ sa ] ) )
    continue;
   const int j = freePlace( a );
   if( j < 4 ) {
    move( b , i , a , j );
    ADS_INVARIANT( inBucket( a ) );
    }
   if( ! held )
    unlock( f_L[ sa ] );
   if( j < 4 )
    return( 4 * b + std::size_t( i ) );
   }
 return( ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::add( const K & k ,
                                                   const V & v ,
                                                   bool assign )
{
 const std::uint64_t h = hash( k );
 const std::uint32_t t = tag( h );
 for( ; ; ) {
  int lg;
  std::uint32_t w1 , w2;
  {
   std::size_t b1 , b2;
   Hold g = lockKey( h , b1 , b2 );
   const std::size_t i = locate( k , t , b1 , b2 );
   if( i != ~std::size_t( 0 ) ) {
    if( assign )
     f_S[ i ].value = v;
    return( false );
    }
   std::size_t b = b1;
   int j = freePlace( b1 );
   if( j == 4 ) {
    b = b2;
    j = freePlace( b2 );
    }
   std::size_t r = 4 * b + std::size_t( j );
   if( ( j < 4 ) || ( ( r = shift( b1 , b2 ) ) != ~std::size_t( 0 ) ) ) {
    put( r >> 2 , int( r & 3 ) , t , k , v );
    count( b1 , 1 );
    ADS_INVARIANT( inBucket( b1 ) && inBucket( b2 ) );
    return( true );
    }
   lg = f_lg.load( std::memory_order_relaxed );
   w1 = f_T[ b1 ];
   w2 = f_T[ b2 ];
   }
  if( cuckoo( h , lg , w1 , w2 ) == 2 )
   grow( lg );
  }
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
bool ConcurrentCuckooTable< K , V , H , Eq >::place( std::uint64_t h ,
                                                     KK && k , VV && v )
{
 const int lg = f_lg.load( std::memory_order_relaxed );
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , lg ) , b2 = other( b1 , t , lg );
 int i = freePlace( b1 );
 if( i < 4 ) {
  put( b1 , i , t , std::forward< KK >( k ) , std::forward< VV >( v ) );
  return( true );
  }
 if( ( i = freePlace( b2 ) ) < 4 ) {
  put( b2 , i , t , std::forward< KK >( k ) , std::forward< VV >( v ) );
  return( true );
  }
 Node N[ kNodes ];
 const int q = search( h , lg , N , [ & ]( std::size_t b ,
                                           std::uint32_t & w ) {
  w = f_T[ b ];
  return( true ); } );
 if( q < 0 )
  return( false );
 int x = q;
 for( ; N[ x ].parent >= 0 ; x = N[ x ].parent )
  move( N[ N[ x ].parent ].b , N[ x ].from , N[ x ].b ,
        freePlace( N[ x ].b ) );
 put( N[ x ].b , freePlace( N[ x ].b ) , t , std::forward< KK >( k ) ,
      std::forward< VV >( v ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::rehash( int lg ,
                                                      HoldAll & g )
{
 // the new table built before the old one is given up; the pairs moved
 // into it, and if one finds no place, the ones placed and the ones left
 // gathered and a larger table tried
 std::vector< std::uint32_t > T( std::size_t( 1 ) << lg , 0 );
 std::vector< Slot > S( std::size_t( 4 ) << lg );
 g.extend( T.size() );
 T.swap( f_T );
 S.swap( f_S );
 f_lg.store( lg , std::memory_order_release );
 f_rehashes.fetch_add( 1 , std::memory_order_relaxed );
 std::vector< Slot > P;                   // the pairs not placed
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  if( ( ( T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF ) &&
      ( ! P.empty() ||
        ! place( hash( S[ i ].key ) , std::move( S[ i ].key ) ,
                 std::move( S[ i ].value ) ) ) )
   P.push_back( std::move( S[ i ] ) );
 while( ! P.empty() ) {                   // the rare case
  for( std::size_t j = 0 ; j < f_S.size() ; ++j )
   if( ( f_T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF )
    P.push_back( std::move( f_S[ j ] ) );
  ++lg;
  g.extend( std::size_t( 1 ) << lg );
  f_T.assign( std::size_t( 1 ) << lg , 0 );
  f_S.assign( std::size_t( 4 ) << lg , Slot{} );
  f_lg.store( lg , std::memory_order_release );
  f_rehashes.fetch_add( 1 , std::memory_order_relaxed );
  std::vector< Slot > R;
  for( Slot & s : P )
   if( ! R.empty() ||
       ! place( hash( s.key ) , std::move( s.key ) , std::move( s.value ) ) )
    R.push_back( std::move( s ) );
  P.swap( R );
  }
 std::int64_t n = 0;                      // the counters, from 0
 for( std::size_t s = 0 ; s < g.held() ; ++s ) {
  n += f_L[ s ].count.load( std::memory_order_relaxed );
  f_L[ s ].count.store( 0 , std::memory_order_relaxed );
  }
 f_L[ 0 ].count.store( n , std::memory_order_relaxed );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::grow( int lg )
{
 HoldAll g( *this );
 if( f_lg.load( std::memory_order_relaxed ) != lg )
  return;                                 // another thread doubled it
 std::int64_t n = 0;
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  n += f_L[ s ].count.load( std::memory_order_relaxed );
 const std::size_t places = std::size_t( 4 ) << lg;
 if( ( places >= 256 ) && ( 16 * std::size_t( n ) < places ) )
  throw( std::length_error( "ConcurrentCuckooTable::grow: more than 8 "
                            "keys share both their buckets" ) );
 rehash( lg + 1 , g );
 }

/*------------------------------ THE INTERFACE -----------------------------*/

template< class K , class V , class H , class Eq >
std::optional< V > ConcurrentCuckooTable< K , V , H , Eq >::find(
                                                   const K & k ) const
{
 const std::uint64_t h = hash( k );
 std::size_t b1 , b2;
 Hold g = lockKey( h , b1 , b2 );
 const std::size_t i = locate( k , tag( h ) , b1 , b2 );
 if( i == ~std::size_t( 0 ) )
  return( std::nullopt );
 return( std::optional< V >( f_S[ i ].value ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::contains( const K & k ) const
{
 const std::uint64_t h = hash( k );
 std::size_t b1 , b2;
 Hold g = lockKey( h , b1 , b2 );
 return( locate( k , tag( h ) , b1 , b2 ) != ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::insert( const K & k ,
                                                      const V & v )
{
 return( add( k , v , false ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::insertOrAssign( const K & k ,
                                                              const V & v )
{
 return( add( k , v , true ) );
 }

template< class K , class V , class H , class Eq >
template< class F >
bool ConcurrentCuckooTable< K , V , H , Eq >::update( const K & k , F && f )
{
 const std::uint64_t h = hash( k );
 std::size_t b1 , b2;
 Hold g = lockKey( h , b1 , b2 );
 const std::size_t i = locate( k , tag( h ) , b1 , b2 );
 if( i == ~std::size_t( 0 ) )
  return( false );
 std::invoke( std::forward< F >( f ) , f_S[ i ].value );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::erase( const K & k )
{
 const std::uint64_t h = hash( k );
 std::size_t b1 , b2;
 Hold g = lockKey( h , b1 , b2 );
 const std::size_t i = locate( k , tag( h ) , b1 , b2 );
 if( i == ~std::size_t( 0 ) )
  return( false );
 f_T[ i >> 2 ] &= ~( 0xFFU << ( 8 * ( i & 3 ) ) );
 f_S[ i ] = Slot{};                       // what the pair holds goes now
 count( b1 , -1 );
 ADS_INVARIANT( inBucket( b1 ) && inBucket( b2 ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::size( void ) const
{
 // the counters of the stripes in use, the only ones not 0
 const std::size_t m = std::min( kStripes , std::size_t( 1 ) <<
                                 f_lg.load( std::memory_order_relaxed ) );
 std::int64_t n = 0;
 for( std::size_t s = 0 ; s < m ; ++s )
  n += f_L[ s ].count.load( std::memory_order_relaxed );
 return( n > 0 ? std::size_t( n ) : 0 );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::empty( void ) const
{
 return( size() == 0 );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::capacity( void ) const
{
 return( std::size_t( 4 ) << f_lg.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::rehashes( void ) const
{
 return( f_rehashes.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::reserve( std::size_t n )
{
 const std::size_t b = std::bit_ceil( std::max< std::size_t >(
                        2 , ( n * 10 + 35 ) / 36 ) );
 HoldAll g( *this );
 if( b > f_T.size() )
  rehash( std::countr_zero( b ) , g );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::clear( void )
{
 HoldAll g( *this );
 std::fill( f_T.begin() , f_T.end() , 0 );
 for( Slot & s : f_S )
  s = Slot{};
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  f_L[ s ].count.store( 0 , std::memory_order_relaxed );
 }

template< class K , class V , class H , class Eq >
template< class F >
void ConcurrentCuckooTable< K , V , H , Eq >::forEach( F && f )
{
 HoldAll g( *this );
 for( std::size_t i = 0 ; i < f_S.size() ; ++i )
  if( ( f_T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF )
   f( std::as_const( f_S[ i ].key ) , f_S[ i ].value );
 }

template< class K , class V , class H , class Eq >
template< class F >
void ConcurrentCuckooTable< K , V , H , Eq >::forEach( F && f ) const
{
 HoldAll g( *this );
 for( std::size_t i = 0 ; i < f_S.size() ; ++i )
  if( ( f_T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF )
   f( f_S[ i ].key , f_S[ i ].value );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::consistent( void ) const
{
 HoldAll g( *this );
 const int lg = f_lg.load( std::memory_order_relaxed );
 if( ( f_T.size() != ( std::size_t( 1 ) << lg ) ) ||
     ( f_S.size() != 4 * f_T.size() ) )
  return( false );
 std::int64_t n = 0;
 for( std::size_t i = 0 ; i < f_S.size() ; ++i ) {
  const std::uint32_t t = ( f_T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF;
  if( t == 0 )
   continue;
  ++n;
  const K & k = f_S[ i ].key;
  const std::uint64_t h = hash( k );
  const std::size_t b1 = first( h , lg ) , b2 = other( b1 , t , lg );
  if( ( tag( h ) != t ) || ( ( ( i >> 2 ) != b1 ) && ( ( i >> 2 ) != b2 ) ) )
   return( false );
  int c = 0;                              // once in its two buckets
  for( std::size_t b : { b1 , b2 } )
   for( std::size_t j = 4 * b ; j < 4 * b + 4 ; ++j )
    if( ( ( ( f_T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF ) == t ) &&
        f_eq( f_S[ j ].key , k ) )
     ++c;
  if( c != 1 )
   return( false );
  }
 std::int64_t m = 0;
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  m += f_L[ s ].count.load( std::memory_order_relaxed );
 return( n == m );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File ConcurrentCuckooTable.tpp -------------------*/
/*--------------------------------------------------------------------------*/
