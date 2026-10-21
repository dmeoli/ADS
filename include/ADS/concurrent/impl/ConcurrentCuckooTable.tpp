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

/// the line of cache of p fetched ahead, for reading
inline void prefetch( const void * p )
{
#if defined( __GNUC__ ) || defined( __clang__ )
 __builtin_prefetch( p );
#else
 (void) p;
#endif
 }

/// the object x read atomically and relaxed if A, plainly otherwise
template< bool A , class T >
[[nodiscard]] inline T loadRelaxed( const T & x )
{
 if constexpr( A )
  return( std::atomic_ref< T >( const_cast< T & >( x ) ).load(
           std::memory_order_relaxed ) );
 else
  return( x );
 }

/// the object x set to v, atomically and relaxed if A, plainly otherwise
template< bool A , class T , class U >
inline void storeRelaxed( T & x , U && v )
{
 if constexpr( A )
  std::atomic_ref< T >( x ).store( T( std::forward< U >( v ) ) ,
                                   std::memory_order_relaxed );
 else
  x = std::forward< U >( v );
 }

}  // end( namespace detail )

/*------------------------------- THE LOCKS --------------------------------*/

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::lock( Stripe & s )
{
 for( int n = 0 ; ; ) {
  std::uint64_t v = s.version.load( std::memory_order_relaxed );
  if( ! ( v & 1 ) &&
      s.version.compare_exchange_weak( v , v + 1 ,
                                       std::memory_order_acquire ,
                                       std::memory_order_relaxed ) ) {
   // the odd version visible before any store of the holder, to a reader
   // that sees one of those stores
   if constexpr( optimistic )
    std::atomic_thread_fence( std::memory_order_release );
   return;
   }
  if( ++n < 64 )
   detail::spinPause();
  else
   std::this_thread::yield();
  }
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::unlock( Stripe & s )
{
 s.version.store( s.version.load( std::memory_order_relaxed ) + 1 ,
                  std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::HoldAll(
                                       const ConcurrentCuckooTable & t )
 : f_L( t.f_L.get() )
{
 // the stripe 0 first: the table changes only under all the locks in use,
 // which include it
 lock( f_L[ 0 ] );
 f_n = 1;
 extend( t.f_lg.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::extend( int lg )
{
 const std::size_t m = std::size_t( 1 ) << ( lg - regionLg( lg ) );
 for( ; f_n < m ; ++f_n )
  lock( f_L[ f_n ] );                     // in increasing order
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::~HoldAll()
{
 while( f_n > 0 )
  unlock( f_L[ --f_n ] );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentCuckooTable< K , V , H , Eq >::lockRegion( std::uint64_t h )
 const -> Table *
{
 // the region of the size seen, and the table checked under its lock: a
 // doubling holds all the locks, so that it is over or not begun
 for( ; ; ) {
  const int lg = f_lg.load( std::memory_order_acquire );
  Stripe & s = f_L[ first( h , lg ) >> regionLg( lg ) ];
  lock( s );
  Table * T = f_table.load( std::memory_order_relaxed );
  if( T->lg == lg )
   return( T );
  unlock( s );
  }
 }

/*------------------------------ THE BUCKETS -------------------------------*/

template< class K , class V , class H , class Eq >
int ConcurrentCuckooTable< K , V , H , Eq >::regionLg( int lg )
{
 return( lg - std::clamp( lg - kLgRegion , 0 , kLgStripes ) );
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::Table::Table( int l )
 : lg( l ) , rg( regionLg( l ) ) ,
   T( HugePageAllocator< std::uint32_t >().allocate( buckets() ) ) ,
   B( nullptr )
{
 // on huge pages when large, so that a lookup misses the TLB once per 2 MB
 std::uninitialized_value_construct_n( T , buckets() );
 try {
  B = HugePageAllocator< Bucket >().allocate( buckets() );
  std::uninitialized_value_construct_n( B , buckets() );
  }
 catch( ... ) {
  if( B )
   HugePageAllocator< Bucket >().deallocate( B , buckets() );
  HugePageAllocator< std::uint32_t >().deallocate( T , buckets() );
  throw;
  }
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::Table::~Table()
{
 std::destroy_n( B , buckets() );
 HugePageAllocator< Bucket >().deallocate( B , buckets() );
 HugePageAllocator< std::uint32_t >().deallocate( T , buckets() );
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::ConcurrentCuckooTable(
                                          std::size_t n , H h , Eq eq )
 : f_L( std::make_unique< Stripe[] >( kStripes ) ) , f_h( h ) , f_eq( eq )
{
 const std::size_t b = std::bit_ceil( std::max< std::size_t >(
                        2 , ( n * 10 + 35 ) / 36 ) );   // at load 0.9
 f_cur = std::make_unique< Table >( std::countr_zero( b ) );
 f_lg.store( f_cur->lg , std::memory_order_relaxed );
 f_table.store( f_cur.get() , std::memory_order_release );
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
                                                            int rg )
{
 // an odd offset below the buckets of a region, so that the two differ
 // and stay in the region, by the top bits of the product of the tag by
 // the golden ratio
 const auto d = std::size_t( ( std::uint64_t( t ) * 0x9E3779B97F4A7C15ULL )
                             >> ( 64 - rg ) );
 return( b ^ ( d | 1 ) );
 }

template< class K , class V , class H , class Eq >
std::uint32_t ConcurrentCuckooTable< K , V , H , Eq >::tags( const Table & T ,
                                                             std::size_t b )
{
 return( detail::loadRelaxed< optimistic >( T.T[ b ] ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::setTags( Table & T ,
                                                       std::size_t b ,
                                                       std::uint32_t w )
{
 detail::storeRelaxed< optimistic >( T.T[ b ] , w );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::locate(
                                     const Table & T , const K & k ,
                                     std::uint32_t t , std::size_t b1 ,
                                     std::size_t b2 ) const
{
 for( std::size_t b : { b1 , b2 } ) {
  std::uint32_t m = detail::zeroBytes( T.T[ b ] ^ ( t * 0x01010101U ) );
  for( ; m ; m &= m - 1 ) {
   const std::size_t i = 4 * b + std::size_t( std::countr_zero( m ) >> 3 );
   if( f_eq( T.S( i ).key , k ) )
    return( i );
   }
  }
 return( ~std::size_t( 0 ) );
 }

template< class K , class V , class H , class Eq >
int ConcurrentCuckooTable< K , V , H , Eq >::freePlace( const Table & T ,
                                                        std::size_t b )
{
 const std::uint32_t z = detail::zeroBytes( T.T[ b ] );
 return( z ? std::countr_zero( z ) >> 3 : 4 );
 }

template< class K , class V , class H , class Eq >
template< class KK , class VV >
void ConcurrentCuckooTable< K , V , H , Eq >::put( Table & T ,
                                                   std::size_t b , int i ,
                                                   std::uint32_t t ,
                                                   KK && k , VV && v )
{
 Slot & s = T.S( 4 * b + std::size_t( i ) );
 detail::storeRelaxed< optimistic >( s.key , std::forward< KK >( k ) );
 detail::storeRelaxed< optimistic >( s.value , std::forward< VV >( v ) );
 setTags( T , b , T.T[ b ] | ( t << ( 8 * i ) ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::assign( Table & T ,
                                                      std::size_t i ,
                                                      const V & v )
{
 detail::storeRelaxed< optimistic >( T.S( i ).value , v );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::clearPlace( Table & T ,
                                                          std::size_t i )
{
 setTags( T , i >> 2 , T.T[ i >> 2 ] & ~( 0xFFU << ( 8 * ( i & 3 ) ) ) );
 if constexpr( ! optimistic )
  T.S( i ) = Slot{};                      // what the pair holds goes now
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::move( Table & T ,
                                                    std::size_t a , int i ,
                                                    std::size_t d , int j )
{
 // first copied, then cleared: the key is always in one of its buckets
 Slot & s = T.S( 4 * a + std::size_t( i ) );
 put( T , d , j , ( T.T[ a ] >> ( 8 * i ) ) & 0xFF , std::move( s.key ) ,
      std::move( s.value ) );
 setTags( T , a , T.T[ a ] & ~( 0xFFU << ( 8 * i ) ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::inBucket( const Table & T ,
                                                        std::size_t b ) const
{
 for( int i = 0 ; i < 4 ; ++i ) {
  const std::uint32_t t = ( T.T[ b ] >> ( 8 * i ) ) & 0xFF;
  if( t == 0 )
   continue;
  const std::uint64_t h = hash( T.S( 4 * b + std::size_t( i ) ).key );
  const std::size_t b1 = first( h , T.lg );
  if( ( tag( h ) != t ) || ( ( b != b1 ) && ( b != other( b1 , t , T.rg ) ) ) )
   return( false );
  }
 return( true );
 }

/*------------------------------ THE LOOKUPS -------------------------------*/

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::fetch( const Table & T ,
                                                     std::size_t b1 ,
                                                     std::size_t b2 )
{
 // the tags of both buckets and the places of the first at once, so that
 // their misses overlap; the places of the second, where fewer keys are,
 // only when a tag matches there
 detail::prefetch( &T.T[ b1 ] );
 detail::prefetch( &T.T[ b2 ] );
 detail::prefetch( &T.B[ b1 ] );
 }

template< class K , class V , class H , class Eq >
int ConcurrentCuckooTable< K , V , H , Eq >::peek( const K & k ,
                                                   std::uint64_t h ,
                                                   V * out ) const
{
 // the seqlock: the version of the region even before the reads and the
 // same after them, and the table read after the version, so that it is
 // the one of that version; the tables replaced stay, so that a read of
 // one is wasted but never invalid
 if constexpr( optimistic ) {
  const std::uint32_t t = tag( h );
  for( int a = 0 ; a < 4 ; ++a ) {
   const Table * T = f_table.load( std::memory_order_acquire );
   const std::size_t b1 = first( h , T->lg ) , b2 = other( b1 , t , T->rg );
   fetch( *T , b1 , b2 );
   const Stripe & s = f_L[ b1 >> T->rg ];
   const std::uint64_t v = s.version.load( std::memory_order_acquire );
   if( ( v & 1 ) || ( f_table.load( std::memory_order_acquire ) != T ) ) {
    detail::spinPause();
    continue;
    }
   int found = 0;
   V x{};
   for( std::size_t b : { b1 , b2 } ) {
    std::uint32_t m = detail::zeroBytes( tags( *T , b ) ^
                                         ( t * 0x01010101U ) );
    for( ; m ; m &= m - 1 ) {
     const std::size_t i = 4 * b +
                           std::size_t( std::countr_zero( m ) >> 3 );
     if( f_eq( detail::loadRelaxed< true >( T->S( i ).key ) , k ) ) {
      found = 1;
      if( out )
       x = detail::loadRelaxed< true >( T->S( i ).value );
      break;
      }
     }
    if( found )
     break;
    }
   std::atomic_thread_fence( std::memory_order_acquire );
   if( s.version.load( std::memory_order_relaxed ) == v ) {
    if( found && out )
     *out = x;
    return( found );
    }
   }
  }
 (void) k;
 (void) h;
 (void) out;
 return( 2 );
 }

template< class K , class V , class H , class Eq >
std::optional< V > ConcurrentCuckooTable< K , V , H , Eq >::find(
                                                   const K & k ) const
{
 const std::uint64_t h = hash( k );
 if constexpr( optimistic ) {
  V x{};
  const int r = peek( k , h , &x );
  if( r < 2 )
   return( r ? std::optional< V >( x ) : std::nullopt );
  }
 Table * T = lockRegion( h );
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , T->lg );
 Hold g( f_L[ b1 >> T->rg ] );
 const std::size_t i = locate( *T , k , t , b1 , other( b1 , t , T->rg ) );
 if( i == ~std::size_t( 0 ) )
  return( std::nullopt );
 return( std::optional< V >( T->S( i ).value ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::contains( const K & k ) const
{
 const std::uint64_t h = hash( k );
 if constexpr( optimistic ) {
  const int r = peek( k , h , nullptr );
  if( r < 2 )
   return( r == 1 );
  }
 Table * T = lockRegion( h );
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , T->lg );
 Hold g( f_L[ b1 >> T->rg ] );
 return( locate( *T , k , t , b1 , other( b1 , t , T->rg ) ) !=
         ~std::size_t( 0 ) );
 }

/*------------------------------ THE INSERTION -----------------------------*/

template< class K , class V , class H , class Eq >
int ConcurrentCuckooTable< K , V , H , Eq >::search( const Table & T ,
                                                     std::uint64_t h ,
                                                     Node * N ) const
{
 int nn = 0;
 const std::size_t b1 = first( h , T.lg );
 N[ nn++ ] = Node{ b1 , -1 , -1 , 0 , 0 };
 N[ nn++ ] = Node{ other( b1 , tag( h ) , T.rg ) , -1 , -1 , 0 , 0 };
 auto onPath = [ & ]( int x , std::size_t b ) {
  for( ; x >= 0 ; x = N[ x ].parent )
   if( N[ x ].b == b )
    return( true );
  return( false ); };
 for( int q = 0 ; q < nn ; ++q ) {
  const std::uint32_t w = T.T[ N[ q ].b ];
  if( detail::zeroBytes( w ) )
   return( q );
  if( N[ q ].depth == kDepth )
   continue;
  for( int i = 0 ; ( i < 4 ) && ( nn < kNodes ) ; ++i ) {
   const std::uint32_t t = ( w >> ( 8 * i ) ) & 0xFF;
   const std::size_t a = other( N[ q ].b , t , T.rg );
   if( ! onPath( q , a ) )
    N[ nn++ ] = Node{ a , q , i , N[ q ].depth + 1 , t };
   }
  }
 return( -1 );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::makeRoom(
                                         Table & T , std::uint64_t h ) const
{
 Node N[ kNodes ];
 const int q = search( T , h , N );
 if( q < 0 )
  return( ~std::size_t( 0 ) );
 int x = q;                               // the moves, from the end
 for( ; N[ x ].parent >= 0 ; x = N[ x ].parent )
  move( T , N[ N[ x ].parent ].b , N[ x ].from , N[ x ].b ,
        freePlace( T , N[ x ].b ) );
 return( 4 * N[ x ].b + std::size_t( freePlace( T , N[ x ].b ) ) );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::room(
                                         Table & T , std::uint64_t h ,
                                         std::int64_t n ) const
{
 const std::size_t b1 = first( h , T.lg ) ,
                   b2 = other( b1 , tag( h ) , T.rg );
 if( int j = freePlace( T , b1 ) ; j < 4 )
  return( 4 * b1 + std::size_t( j ) );
 if( int j = freePlace( T , b2 ) ; j < 4 )
  return( 4 * b2 + std::size_t( j ) );
 if( 50 * ( n + 1 ) > 49 * ( std::int64_t( 4 ) << T.rg ) )
  return( ~std::size_t( 0 ) );            // past the load 0.98
 return( makeRoom( T , h ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::add( const K & k ,
                                                   const V & v ,
                                                   bool assign )
{
 const std::uint64_t h = hash( k );
 const std::uint32_t t = tag( h );
 for( ; ; ) {
  Table * T = lockRegion( h );
  const int lg = T->lg;
  {
   const std::size_t b1 = first( h , T->lg ) , b2 = other( b1 , t , T->rg );
   fetch( *T , b1 , b2 );
   Stripe & s = f_L[ b1 >> T->rg ];
   Hold g( s );
   const std::size_t i = locate( *T , k , t , b1 , b2 );
   if( i != ~std::size_t( 0 ) ) {
    if( assign )
     this->assign( *T , i , v );
    return( false );
    }
   const std::int64_t n = s.count.load( std::memory_order_relaxed );
   const std::size_t r = room( *T , h , n );
   if( r != ~std::size_t( 0 ) ) {
    put( *T , r >> 2 , int( r & 3 ) , t , k , v );
    s.count.store( n + 1 , std::memory_order_relaxed );
    ADS_INVARIANT( inBucket( *T , b1 ) && inBucket( *T , b2 ) );
    return( true );
    }
   }
  grow( lg );
  }
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::placeInto( Table & N ,
                                                         std::uint64_t h ,
                                                         Slot && x ) const
{
 const std::size_t r = room( N , h , 0 );
 if( r == ~std::size_t( 0 ) )
  return( false );
 N.S( r ) = std::move( x );               // N is not yet seen by others
 N.T[ r >> 2 ] |= tag( h ) << ( 8 * ( r & 3 ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::rehash( int lg , HoldAll & g )
{
 // the new table filled before it replaces the old one, which an
 // optimistic reader may still read, and which is therefore only copied;
 // the pairs that find no place (rare), with those placed, go to a table
 // twice as large
 Table & O = *f_cur;
 auto N = std::make_unique< Table >( lg );
 std::vector< Slot > P;
 for( std::size_t i = 0 ; i < ( O.buckets() << 2 ) ; ++i )
  if( ( ( O.T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF ) &&
      ( ! P.empty() || ! placeInto( *N , hash( O.S( i ).key ) ,
                                    std::move( O.S( i ) ) ) ) )
   P.push_back( std::move( O.S( i ) ) );
 while( ! P.empty() ) {
  for( std::size_t j = 0 ; j < ( N->buckets() << 2 ) ; ++j )
   if( ( N->T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF )
    P.push_back( std::move( N->S( j ) ) );
  N = std::make_unique< Table >( ++lg );
  std::vector< Slot > R;
  for( Slot & x : P )
   if( ! R.empty() || ! placeInto( *N , hash( x.key ) , std::move( x ) ) )
    R.push_back( std::move( x ) );
  P.swap( R );
  }
 g.extend( lg );                          // before the table is seen
 for( std::size_t s = 0 ; s < g.held() ; ++s )   // the pairs per region
  f_L[ s ].count.store( 0 , std::memory_order_relaxed );
 for( std::size_t b = 0 ; b < N->buckets() ; ++b )
  if( const std::uint32_t w = N->T[ b ] ) {
   Stripe & s = f_L[ b >> N->rg ];
   s.count.store( s.count.load( std::memory_order_relaxed ) + 4 -
                  std::popcount( detail::zeroBytes( w ) ) ,
                  std::memory_order_relaxed );
   }
 f_table.store( N.get() , std::memory_order_release );
 f_lg.store( lg , std::memory_order_release );
 if constexpr( optimistic )
  f_old.push_back( std::move( f_cur ) );  // a reader may be on it
 f_cur = std::move( N );
 f_rehashes.fetch_add( 1 , std::memory_order_relaxed );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::grow( int lg )
{
 HoldAll g( *this );
 if( f_cur->lg != lg )
  return;                                 // another thread doubled it
 std::int64_t n = 0;
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  n += f_L[ s ].count.load( std::memory_order_relaxed );
 const std::size_t places = f_cur->buckets() << 2;
 if( ( places >= 256 ) && ( 16 * std::size_t( n ) < places ) )
  throw( std::length_error( "ConcurrentCuckooTable::grow: more than 8 "
                            "keys share both their buckets" ) );
 rehash( lg + 1 , g );
 }

/*------------------------------ THE INTERFACE -----------------------------*/

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
 Table * T = lockRegion( h );
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , T->lg );
 Hold g( f_L[ b1 >> T->rg ] );
 const std::size_t i = locate( *T , k , t , b1 , other( b1 , t , T->rg ) );
 if( i == ~std::size_t( 0 ) )
  return( false );
 if constexpr( optimistic ) {             // a copy, stored atomically
  V x = T->S( i ).value;
  std::invoke( std::forward< F >( f ) , x );
  assign( *T , i , x );
  }
 else
  std::invoke( std::forward< F >( f ) , T->S( i ).value );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::erase( const K & k )
{
 const std::uint64_t h = hash( k );
 Table * T = lockRegion( h );
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , T->lg ) , b2 = other( b1 , t , T->rg );
 fetch( *T , b1 , b2 );
 Stripe & s = f_L[ b1 >> T->rg ];
 Hold g( s );
 const std::size_t i = locate( *T , k , t , b1 , b2 );
 if( i == ~std::size_t( 0 ) )
  return( false );
 clearPlace( *T , i );
 s.count.store( s.count.load( std::memory_order_relaxed ) - 1 ,
                std::memory_order_relaxed );
 ADS_INVARIANT( inBucket( *T , b1 ) && inBucket( *T , b2 ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::size( void ) const
{
 // the counters of the regions in use, the only ones not 0
 const int lg = f_lg.load( std::memory_order_relaxed );
 const std::size_t m = std::size_t( 1 ) << ( lg - regionLg( lg ) );
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
 if( b > f_cur->buckets() )
  rehash( std::countr_zero( b ) , g );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::clear( void )
{
 HoldAll g( *this );
 Table & T = *f_cur;
 for( std::size_t b = 0 ; b < T.buckets() ; ++b )
  setTags( T , b , 0 );
 if constexpr( ! optimistic )
  for( std::size_t i = 0 ; i < ( T.buckets() << 2 ) ; ++i )
   T.S( i ) = Slot{};
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  f_L[ s ].count.store( 0 , std::memory_order_relaxed );
 }

template< class K , class V , class H , class Eq >
template< class F >
void ConcurrentCuckooTable< K , V , H , Eq >::forEach( F && f )
{
 HoldAll g( *this );
 Table & T = *f_cur;
 for( std::size_t i = 0 ; i < ( T.buckets() << 2 ) ; ++i )
  if( ( T.T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF ) {
   if constexpr( optimistic ) {           // a copy, stored atomically
    V x = T.S( i ).value;
    f( std::as_const( T.S( i ).key ) , x );
    assign( T , i , x );
    }
   else
    f( std::as_const( T.S( i ).key ) , T.S( i ).value );
   }
 }

template< class K , class V , class H , class Eq >
template< class F >
void ConcurrentCuckooTable< K , V , H , Eq >::forEach( F && f ) const
{
 HoldAll g( *this );
 const Table & T = *f_cur;
 for( std::size_t i = 0 ; i < ( T.buckets() << 2 ) ; ++i )
  if( ( T.T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF )
   f( T.S( i ).key , T.S( i ).value );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::consistent( void ) const
{
 HoldAll g( *this );
 const Table & T = *f_cur;
 if( ( f_table.load( std::memory_order_relaxed ) != &T ) ||
     ( f_lg.load( std::memory_order_relaxed ) != T.lg ) ||
     ( g.held() != ( std::size_t( 1 ) << ( T.lg - T.rg ) ) ) )
  return( false );
 std::vector< std::int64_t > n( g.held() , 0 );
 for( std::size_t i = 0 ; i < ( T.buckets() << 2 ) ; ++i ) {
  const std::uint32_t t = ( T.T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF;
  if( t == 0 )
   continue;
  ++n[ ( i >> 2 ) >> T.rg ];
  const K & k = T.S( i ).key;
  const std::uint64_t h = hash( k );
  const std::size_t b1 = first( h , T.lg ) , b2 = other( b1 , t , T.rg );
  if( ( tag( h ) != t ) || ( ( ( i >> 2 ) != b1 ) && ( ( i >> 2 ) != b2 ) ) )
   return( false );
  int c = 0;                              // once in its two buckets
  for( std::size_t b : { b1 , b2 } )
   for( std::size_t j = 4 * b ; j < 4 * b + 4 ; ++j )
    if( ( ( ( T.T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF ) == t ) &&
        f_eq( T.S( j ).key , k ) )
     ++c;
  if( c != 1 )
   return( false );
  }
 for( std::size_t s = 0 ; s < g.held() ; ++s )   // each region counted
  if( n[ s ] != f_L[ s ].count.load( std::memory_order_relaxed ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File ConcurrentCuckooTable.tpp -------------------*/
/*--------------------------------------------------------------------------*/
