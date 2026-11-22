/*--------------------------------------------------------------------------*/
/*--------------------- File ConcurrentCuckooTable.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConcurrentCuckooTable [see ConcurrentCuckooTable.h],
 * with their contracts.
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

#include <algorithm>
#include <bit>
#include <mutex>
#include <thread>
#include <utility>

#include "ADS/concurrent/impl/Spin.tpp"

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

namespace detail
{

/// the object x read atomically with acquire if A, plainly otherwise: the
/// reads of the seqlock, each ordered before the second read of the
/// version [see peek()]; on x86-64 the same plain load as a relaxed one
template< bool A , class T >
[[nodiscard]] inline T loadAcquire( const T & x )
{
 if constexpr( A )
  return( std::atomic_ref< T >( const_cast< T & >( x ) )
	     .load( std::memory_order_acquire ) );
 else
  return( x );
 }

/// the object x set to v, atomically with release if A, plainly otherwise:
/// the stores of a writer of the seqlock, each ordered after its
/// compare-and-swap of the version [see lock()]; on x86-64 the same plain
/// store as a relaxed one
template< bool A , class T , class U >
inline void storeRelease( T & x , U && v )
{
 if constexpr( A )
  std::atomic_ref< T >( x ).store( T( std::forward< U >( v ) ) ,
				   std::memory_order_release );
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
  // the odd version visible before any store of the holder, to a reader
  // that sees one of those stores: they are releases [see storeRelease()]
  if( ! ( v & 1 ) &&
      s.version.compare_exchange_weak( v , v + 1 , std::memory_order_acquire ,
				       std::memory_order_relaxed ) )
   return;
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
void ConcurrentCuckooTable< K , V , H , Eq >::lockHelping( Stripe & s ) const
{
 // as lock(), but a thread that waits, holding no lock, migrates the parts
 // of a doubling left, instead of spinning while the doubling holds all
 // the locks
 for( int n = 0 ; ; ) {
  std::uint64_t v = s.version.load( std::memory_order_relaxed );
  if( ! ( v & 1 ) &&
      s.version.compare_exchange_weak( v , v + 1 , std::memory_order_acquire ,
				       std::memory_order_relaxed ) )
   return;
  if( ( n & 15 ) == 15 )
   help();
  if( ++n < 64 )
   detail::spinPause();
  else
   std::this_thread::yield();
  }
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::HoldAll(
  const ConcurrentCuckooTable & t )
    : f_L( t.f_L.get() )
{
 // the stripe 0 first: the table changes only under all the locks in use,
 // which include it; while none is held, a migration can be helped
 t.lockHelping( f_L[ 0 ] );
 f_n = 1;
 extend( t.f_lg.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::extend( int lg )
{
 const std::size_t m = std::size_t( 1 ) << ( lg - regionLg( lg ) );
 for( ; f_n < m ; ++f_n )
  lock( f_L[ f_n ] );  // in increasing order
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::HoldAll::~HoldAll()
{
 while( f_n > 0 )
  unlock( f_L[ --f_n ] );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentCuckooTable< K , V , H ,
			    Eq >::lockRegion( std::uint64_t h ) const
  -> Table *
{
 // the region of the size seen, and the table checked under its lock: a
 // doubling holds all the locks, so that it is over or not begun; a lock
 // free at once here, the wait by a call in tail position, so that no
 // value stays live across a call
 const int lg = f_lg.load( std::memory_order_acquire );
 Stripe & s = f_L[ first( h , lg ) >> regionLg( lg ) ];
 std::uint64_t v = s.version.load( std::memory_order_relaxed );
 if( ! ( v & 1 ) &&
     s.version.compare_exchange_weak( v , v + 1 , std::memory_order_acquire ,
				      std::memory_order_relaxed ) ) {
  Table * T = f_table.load( std::memory_order_relaxed );
  if( T->lg == lg )
   return( T );
  unlock( s );
  }
 return( waitRegion( h ) );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentCuckooTable< K , V , H ,
			    Eq >::waitRegion( std::uint64_t h ) const
  -> Table *
{
 for( ; ; ) {
  const int lg = f_lg.load( std::memory_order_acquire );
  Stripe & s = f_L[ first( h , lg ) >> regionLg( lg ) ];
  lockHelping( s );
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
ConcurrentCuckooTable< K , V , H , Eq >::Table::Table( int l , bool init )
    : lg( l ) , rg( regionLg( l ) ) ,
      T( HugePageAllocator< std::uint32_t >().allocate( buckets() ) ) ,
      O( nullptr ) , B( nullptr )
{
 // on huge pages when large, so that a lookup misses the TLB once per 2 MB
 try {
  O = HugePageAllocator< std::uint64_t >().allocate( overflowWords() );
  B = HugePageAllocator< Bucket >().allocate( buckets() );
  if( init ) {
   std::uninitialized_value_construct_n( T , buckets() );
   std::uninitialized_value_construct_n( O , overflowWords() );
   std::uninitialized_value_construct_n( B , buckets() );
   }
  }
 catch( ... ) {
  if( B )
   HugePageAllocator< Bucket >().deallocate( B , buckets() );
  if( O )
   HugePageAllocator< std::uint64_t >().deallocate( O , overflowWords() );
  HugePageAllocator< std::uint32_t >().deallocate( T , buckets() );
  throw;
  }
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::Table::~Table()
{
 std::destroy_n( B , buckets() );
 HugePageAllocator< Bucket >().deallocate( B , buckets() );
 HugePageAllocator< std::uint64_t >().deallocate( O , overflowWords() );
 HugePageAllocator< std::uint32_t >().deallocate( T , buckets() );
 }

template< class K , class V , class H , class Eq >
ConcurrentCuckooTable< K , V , H , Eq >::ConcurrentCuckooTable( std::size_t n ,
							     H h , Eq eq )
    : f_L( std::make_unique< Stripe[] >( kStripes ) ) , f_h( h ) , f_eq( eq )
{
 const std::size_t b = std::bit_ceil(
   std::max< std::size_t >( 2 , ( n * 10 + 35 ) / 36 ) );  // at load 0.9
 f_cur = std::make_unique< Table >( std::countr_zero( b ) );
 f_lg.store( f_cur->lg , std::memory_order_relaxed );
 f_table.store( f_cur.get() , std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentCuckooTable< K , V , H ,
				     Eq >::hash( const K & k ) const
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
 const auto d = std::size_t( ( std::uint64_t( t ) * 0x9E3779B97F4A7C15ULL ) >>
			     ( 64 - rg ) );
 return( b ^ ( d | 1 ) );
 }

template< class K , class V , class H , class Eq >
std::uint32_t ConcurrentCuckooTable< K , V , H , Eq >::tags( const Table & T ,
							  std::size_t b )
{
 return( detail::loadAcquire< optimistic >( T.T[ b ] ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::setTags( Table & T ,
						       std::size_t b ,
						    std::uint32_t w )
{
 detail::storeRelease< optimistic >( T.T[ b ] , w );
 }

template< class K , class V , class H , class Eq >
std::size_t
ConcurrentCuckooTable< K , V , H , Eq >::overflowBit( std::size_t b ,
						   std::uint32_t t )
{
 // the top bits of the tag choose the bit of the bucket
 return( b * kOverflow +
	  ( t >> ( 8 - std::countr_zero( unsigned( kOverflow ) ) ) ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::overflowed( const Table & T ,
						       std::size_t b ,
						       std::uint32_t t )
{
 const std::size_t i = overflowBit( b , t );
 return(
   ( detail::loadAcquire< optimistic >( T.O[ i >> 6 ] ) >> ( i & 63 ) ) & 1 );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::markOverflow( Table & T ,
							 std::size_t b ,
							 std::uint32_t t )
{
 // the 16 buckets of a word are in one region, whose lock is held, but
 // in a table of fewer buckets, which has one region
 const std::size_t i = overflowBit( b , t );
 if( ! ( ( T.O[ i >> 6 ] >> ( i & 63 ) ) & 1 ) )
  detail::storeRelease< optimistic >(
    T.O[ i >> 6 ] , T.O[ i >> 6 ] | ( std::uint64_t( 1 ) << ( i & 63 ) ) );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::locate(
  const Table & T , const K & k , std::uint32_t t , std::size_t b1 ,
  std::size_t b2 , bool bit ) const
{
 // the second bucket only if the first has overflowed for the tag t, or
 // always, if not bit, for a key that is there (the bit is then set)
 for( std::size_t b : { b1 , b2 } ) {
  std::uint32_t m = detail::zeroBytes( T.T[ b ] ^ ( t * 0x01010101U ) );
  for( ; m ; m &= m - 1 ) {
   const std::size_t i = 4 * b + std::size_t( std::countr_zero( m ) >> 3 );
   if( f_eq( T.S( i ).key , k ) )
    return( i );
   }
  if( bit && ! overflowed( T , b1 , t ) )
   break;
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
void ConcurrentCuckooTable< K , V , H , Eq >::put( Table & T , std::size_t b ,
						int i , std::uint32_t t ,
						KK && k , VV && v )
{
 Slot & s = T.S( 4 * b + std::size_t( i ) );
 detail::storeRelease< optimistic >( s.key , std::forward< KK >( k ) );
 detail::storeRelease< optimistic >( s.value , std::forward< VV >( v ) );
 setTags( T , b , T.T[ b ] | ( t << ( 8 * i ) ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::assign( Table & T ,
						      std::size_t i ,
						   const V & v )
{
 detail::storeRelease< optimistic >( T.S( i ).value , v );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::clearPlace( Table & T ,
						       std::size_t i )
{
 // the bits of overflow stay: they may be set for a key that is not there
 setTags( T , i >> 2 , T.T[ i >> 2 ] & ~( 0xFFU << ( 8 * ( i & 3 ) ) ) );
 if constexpr( ! optimistic )
  T.S( i ) = Slot{};  // what the pair holds goes now
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::move( Table & T , std::size_t a ,
						 int i , std::size_t d ,
						 int j )
{
 // first copied, then the bit of overflow of a for its tag set, then
 // cleared, by a release that orders the bit before it: the key is always
 // in one of its buckets and, if a is its first bucket, a says that it may
 // be in d once it is not in a (if a is the other one, the bit is set for
 // nothing)
 Slot & s = T.S( 4 * a + std::size_t( i ) );
 const std::uint32_t t = ( T.T[ a ] >> ( 8 * i ) ) & 0xFF;
 put( T , d , j , t , std::move( s.key ) , std::move( s.value ) );
 markOverflow( T , a , t );
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
  if( ( tag( h ) != t ) || ( ( b != b1 ) && ( ( b != other( b1 , t ,
							    T.rg ) ) ||
					      ! overflowed( T , b1 , t ) ) ) )
   return( false );
  }
 return( true );
 }

/*------------------------------ THE LOOKUPS -------------------------------*/

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::fetch( const Table & T ,
						  std::size_t b )
{
 // both at once, so that their misses overlap
 detail::prefetch( &T.T[ b ] );
 detail::prefetch( &T.B[ b ] );
 }

template< class K , class V , class H , class Eq >
ADS_ALWAYS_INLINE int
ConcurrentCuckooTable< K , V , H , Eq >::peek( const K & k , std::uint64_t h ,
					    V * out ) const
{
 // the seqlock: the version of the region even before the reads and the
 // same after them, and the table read before the version and again after
 // it, so that it is the one of that version; the tables replaced stay,
 // so that a read of one is wasted but never invalid
 if constexpr( optimistic ) {
  const std::uint32_t t = tag( h );
  for( int a = 0 ; a < 4 ; ++a ) {
   const Table * T = f_table.load( std::memory_order_acquire );
   const std::size_t b1 = first( h , T->lg );
   fetch( *T , b1 );
   const Stripe & s = f_L[ b1 >> T->rg ];
   // a key absent from the tags of its first bucket, which has not
   // overflowed for its tag, is in neither bucket at the moment of the
   // read of those tags, if the table is still in use: no version is
   // checked, since a key enters its other bucket after the bit is set and
   // leaves its first one by a release after it [see move()], which the
   // read of the tags acquires
   std::uint32_t w = tags( *T , b1 );
   if( ! detail::zeroBytes( w ^ ( t * 0x01010101U ) ) &&
       ! overflowed( *T , b1 , t ) &&
       ( f_table.load( std::memory_order_acquire ) == T ) )
    return( 0 );
   const std::uint64_t v = s.version.load( std::memory_order_acquire );
   if( ( v & 1 ) || ( f_table.load( std::memory_order_acquire ) != T ) ) {
    detail::spinPause();
    continue;
    }
   int found = 0;
   V x{};
   for( std::size_t b = b1 ; ; ) {
    w = tags( *T , b );
    std::uint32_t m = detail::zeroBytes( w ^ ( t * 0x01010101U ) );
    for( ; m ; m &= m - 1 ) {
     const std::size_t i = 4 * b + std::size_t( std::countr_zero( m ) >> 3 );
     if( f_eq( detail::loadAcquire< true >( T->S( i ).key ) , k ) ) {
      found = 1;
      if( out )
       x = detail::loadAcquire< true >( T->S( i ).value );
      break;
      }
     }
    if( found || ( b != b1 ) || ! overflowed( *T , b1 , t ) )
     break;
    b = other( b1 , t , T->rg );
    }
   // a key of the stash has the bit of overflow of b1 set [see stashPut()]
   if( ! found && overflowed( *T , b1 , t ) &&
       ( f_xn.load( std::memory_order_acquire ) != 0 ) )
    found = fromStash( k , out ? &x : nullptr );
   // the reads above acquire [see loadAcquire()]: one that saw a store of
   // a writer makes its odd version seen here
   if( s.version.load( std::memory_order_relaxed ) == v ) {
    if( found && out )
     *out = x;
    return( found );
    }
   }
  }
 ( void )k;
 ( void )h;
 ( void )out;
 return( 2 );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::lockedFind( const K & k ,
						       std::uint64_t h ,
						       V * out ) const
{
 Table * T = lockRegion( h );
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , T->lg );
 Hold g( f_L[ b1 >> T->rg ] );
 const std::size_t i = locate( *T , k , t , b1 , other( b1 , t , T->rg ) );
 if( i == ~std::size_t( 0 ) )
  return( fromStash( k , out ) );
 if( out )
  *out = T->S( i ).value;
 return( true );
 }

/*------------------------------- THE STASH --------------------------------*/

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::fromStash( const K & k ,
						      V * out ) const
{
 // empty but for degenerate hashes; a pair enters it under all the locks,
 // that of the region of k among them
 if( f_xn.load( std::memory_order_acquire ) == 0 ) [[likely]]
  return( false );
 std::lock_guard< std::mutex > g( f_xm );
 for( const Slot & x : f_X )
  if( f_eq( x.key , k ) ) {
   if( out )
    *out = x.value;
   return( true );
   }
 return( false );
 }

template< class K , class V , class H , class Eq >
template< class F >
bool ConcurrentCuckooTable< K , V , H , Eq >::inStash( const K & k , F && f )
{
 if( f_xn.load( std::memory_order_acquire ) == 0 ) [[likely]]
  return( false );
 std::lock_guard< std::mutex > g( f_xm );
 for( std::size_t j = 0 ; j < f_X.size() ; ++j )
  if( f_eq( f_X[ j ].key , k ) ) {
   f( j );
   return( true );
   }
 return( false );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::stashPut( Table & T ,
						     std::uint64_t h ,
						     Slot && x )
{
 // the bit of overflow of the first bucket set before the pair enters the
 // stash, so that a lookup without locks reads it [see peek()]
 markOverflow( T , first( h , T.lg ) , tag( h ) );
 std::lock_guard< std::mutex > g( f_xm );
 f_X.push_back( std::move( x ) );
 f_xn.store( f_X.size() , std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
ADS_ALWAYS_INLINE std::optional< V >
ConcurrentCuckooTable< K , V , H , Eq >::find( const K & k ) const
{
 // the lookup without locks inlined at the call, the rest called
 const std::uint64_t h = hash( k );
 V x{};
 if constexpr( optimistic ) {
  const int r = peek( k , h , &x );
  if( r < 2 )
   return( r ? std::optional< V >( x ) : std::nullopt );
  }
 if( ! lockedFind( k , h , &x ) )
  return( std::nullopt );
 return( std::optional< V >( std::move( x ) ) );
 }

template< class K , class V , class H , class Eq >
ADS_ALWAYS_INLINE bool
ConcurrentCuckooTable< K , V , H , Eq >::contains( const K & k ) const
{
 const std::uint64_t h = hash( k );
 if constexpr( optimistic ) {
  const int r = peek( k , h , nullptr );
  if( r < 2 )
   return( r == 1 );
  }
 return( lockedFind( k , h , nullptr ) );
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
  return( false );
  };
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
std::size_t
ConcurrentCuckooTable< K , V , H , Eq >::makeRoom( Table & T ,
						std::uint64_t h ) const
{
 Node N[ kNodes ];
 const int q = search( T , h , N );
 if( q < 0 )
  return( ~std::size_t( 0 ) );
 int x = q;  // the moves, from the end
 for( ; N[ x ].parent >= 0 ; x = N[ x ].parent )
  move( T , N[ N[ x ].parent ].b , N[ x ].from , N[ x ].b ,
	freePlace( T , N[ x ].b ) );
 if( x == 1 )  // the place is in the other bucket of h
  markOverflow( T , N[ 0 ].b , tag( h ) );
 return( 4 * N[ x ].b + std::size_t( freePlace( T , N[ x ].b ) ) );
 }

template< class K , class V , class H , class Eq >
std::size_t
ConcurrentCuckooTable< K , V , H , Eq >::room( Table & T , std::uint64_t h ,
					    std::int64_t n ) const
{
 const std::size_t b1 = first( h , T.lg ) , b2 = other( b1 , tag( h ) , T.rg );
 if( int j = freePlace( T , b1 ); j < 4 )
  return( 4 * b1 + std::size_t( j ) );
 if( int j = freePlace( T , b2 ); j < 4 ) {
  // the overflow of b1 set before the key is put in b2 [see peek()]
  markOverflow( T , b1 , tag( h ) );
  return( 4 * b2 + std::size_t( j ) );
  }
 if( 50 * ( n + 1 ) > 49 * ( std::int64_t( 4 ) << T.rg ) )
  return( ~std::size_t( 0 ) );  // past the load 0.98
 return( makeRoom( T , h ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::add( const K & k , const V & v ,
						bool assign )
{
 const std::uint64_t h = hash( k );
 const std::uint32_t t = tag( h );
 for( ; ; ) {
  Table * T = lockRegion( h );
  const int lg = T->lg;
  {
   const std::size_t b1 = first( h , T->lg ) , b2 = other( b1 , t , T->rg );
   fetch( *T , b1 );
   detail::prefetch( &T->T[ b2 ] );  // where a new key goes if b1 is full
   Stripe & s = f_L[ b1 >> T->rg ];
   Hold g( s );
   const std::size_t i = locate( *T , k , t , b1 , b2 );
   if( i != ~std::size_t( 0 ) ) {
    if( assign )
     this->assign( *T , i , v );
    return( false );
    }
   if( inStash( k , [ & ]( std::size_t j ) {
	if( assign )
	 f_X[ j ].value = v;
        } ) )
    return( false );
   const std::int64_t n = s.count.load( std::memory_order_relaxed );
   const std::size_t r = room( *T , h , n );
   if( r != ~std::size_t( 0 ) ) {
    put( *T , r >> 2 , int( r & 3 ) , t , k , v );
    s.count.store( n + 1 , std::memory_order_relaxed );
    ADS_INVARIANT( inBucket( *T , b1 ) && inBucket( *T , b2 ) );
    return( true );
    }
   }
  if( const int r = grow( lg , h , k , v , assign ); r >= 0 )
   return( r == 1 );
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
 N.S( r ) = std::move( x );  // N is not yet seen by others
 N.T[ r >> 2 ] |= tag( h ) << ( 8 * ( r & 3 ) );
 return( true );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentCuckooTable< K , V , H , Eq >::claim( std::size_t & c ) const
{
 // the parts of a migration by a compare-and-swap of its own number, so
 // that a thread late on a migration over claims nothing of the next one
 std::uint64_t w = f_claim.load( std::memory_order_acquire );
 for( ; ; ) {
  const std::size_t next = w & 0xFFFF , parts = ( w >> 16 ) & 0xFFFF;
  if( next >= parts )
   return( false );
  if( f_claim.compare_exchange_weak( w , w + 1 , std::memory_order_acq_rel ,
				     std::memory_order_acquire ) ) {
   c = next;
   return( true );
   }
  }
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::help( void ) const
{
 for( std::size_t c ; claim( c ) ; )
  migrate( c );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::migrate( std::size_t c ) const
{
 // what the claim acquired, published before it, and not changed until
 // this part is counted as migrated
 Table & O = *f_from.load( std::memory_order_relaxed );
 Table & N = *f_to.load( std::memory_order_relaxed );
 std::int64_t * const C = f_count.load( std::memory_order_relaxed );
 const std::size_t S = f_part.load( std::memory_order_relaxed );
 const int d = N.lg - O.lg;
 // the old buckets [ lo , hi ) go to the new ones [ lo , hi ) << d, which
 // hold their new regions whole and receive no other key, since the first
 // bucket of a key is the top bits of its hash
 const std::size_t lo = c * S , hi = lo + S , nlo = lo << d , nhi = hi << d;
 std::uninitialized_value_construct_n( N.T + nlo , nhi - nlo );
 const std::size_t olo = ( nlo * kOverflow ) >> 6;
 const std::size_t ohi =
   ( hi == O.buckets() ) ? N.overflowWords() : ( ( nhi * kOverflow ) >> 6 );
 std::uninitialized_value_construct_n( N.O + olo , ohi - olo );
 std::uninitialized_value_construct_n( N.B + nlo , nhi - nlo );
 for( std::size_t i = lo << 2 ; i < ( hi << 2 ) ; ++i )
  if( ( ( O.T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF ) &&
      ! placeInto( N , hash( O.S( i ).key ) , std::move( O.S( i ) ) ) ) {
   // rare: kept apart, for the table twice as large
   while( f_leftLock.exchange( true , std::memory_order_acquire ) )
    detail::spinPause();
   f_left.push_back( std::move( O.S( i ) ) );
   f_leftLock.store( false , std::memory_order_release );
   }
 for( std::size_t b = nlo ; b < nhi ; ++b )  // the pairs of each region
  C[ b >> N.rg ] += 4 - std::popcount( detail::zeroBytes( N.T[ b ] ) );
 f_done.fetch_add( 1 , std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::rehash( int lg , HoldAll & g )
{
 // the new table filled before it replaces the old one, which an
 // optimistic reader may still read, and which is therefore only copied;
 // the old table is cut in parts of whole regions, at least 4096 buckets,
 // which the threads that wait for a lock claim and migrate with this one
 // (the cooperative migration of Maier et al.); the pairs that find no
 // place (rare) go to the stash
 Table & O = *f_cur;
 auto N = std::make_unique< Table >( lg , false );
 const std::size_t S = std::min(
   O.buckets() , std::max( std::size_t( 1 ) << O.rg , std::size_t( 4096 ) ) );
 const std::size_t parts = O.buckets() / S;
 std::vector< std::int64_t > C( N->buckets() >> N->rg , 0 );
 f_left.clear();
 f_from.store( &O , std::memory_order_relaxed );
 f_to.store( N.get() , std::memory_order_relaxed );
 f_count.store( C.data() , std::memory_order_relaxed );
 f_part.store( S , std::memory_order_relaxed );
 f_done.store( 0 , std::memory_order_relaxed );
 const std::uint64_t e =
   ( f_claim.load( std::memory_order_relaxed ) >> 32 ) + 1;
 f_claim.store( ( e << 32 ) | ( std::uint64_t( parts ) << 16 ) ,
		std::memory_order_release );
 help();
 for( int n = 0 ; f_done.load( std::memory_order_acquire ) < parts ; )
  if( ++n < 64 )
   detail::spinPause();
  else
   std::this_thread::yield();
 // the pairs left by the parts and those of the stash placed again; those
 // that find no place go to the new stash, with the bit of overflow of
 // their first bucket set
 std::vector< Slot > P , R;
 P.swap( f_left );
 {
  std::lock_guard< std::mutex > l( f_xm );
  for( Slot & x : f_X )
   P.push_back( std::move( x ) );
  f_X.clear();
  }
 const bool counted = P.empty();
 for( Slot & x : P ) {
  const std::uint64_t hx = hash( x.key );
  if( ! placeInto( *N , hx , std::move( x ) ) ) {
   markOverflow( *N , first( hx , N->lg ) , tag( hx ) );
   R.push_back( std::move( x ) );
   }
  }
 {
  std::lock_guard< std::mutex > l( f_xm );
  f_X.swap( R );
  f_xn.store( f_X.size() , std::memory_order_release );
  }
 g.extend( lg );                              // before the table is seen
 for( std::size_t s = 0 ; s < g.held() ; ++s )  // the pairs per region
  f_L[ s ].count.store( counted ? C[ s ] : 0 , std::memory_order_relaxed );
 if( ! counted )
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
int ConcurrentCuckooTable< K , V , H , Eq >::grow( int lg , std::uint64_t h ,
						const K & k , const V & v ,
						bool assign )
{
 HoldAll g( *this );
 if( f_cur->lg != lg )
  return( -1 );  // another thread doubled it
 std::int64_t n = 0;
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  n += f_L[ s ].count.load( std::memory_order_relaxed );
 const std::size_t places = f_cur->buckets() << 2;
 if( ( places < 256 ) || ( 16 * std::size_t( n ) >= places ) ) {
  rehash( lg + 1 , g );
  return( -1 );
  }
 // more than 8 keys share both buckets: k in the stash, unless another
 // thread put it in or made room for it since its lock was released
 Table & T = *f_cur;
 const std::uint32_t t = tag( h );
 const std::size_t b1 = first( h , T.lg ) , b2 = other( b1 , t , T.rg );
 if( const std::size_t i = locate( T , k , t , b1 , b2 , false );
     i != ~std::size_t( 0 ) ) {
  if( assign )
   this->assign( T , i , v );
  return( 0 );
  }
 if( inStash( k , [ & ]( std::size_t j ) {
      if( assign )
       f_X[ j ].value = v;
      } ) )
  return( 0 );
 Stripe & s = f_L[ b1 >> T.rg ];
 const std::int64_t m = s.count.load( std::memory_order_relaxed );
 if( const std::size_t r = room( T , h , m ); r != ~std::size_t( 0 ) ) {
  put( T , r >> 2 , int( r & 3 ) , t , k , v );
  s.count.store( m + 1 , std::memory_order_relaxed );
  return( 1 );
  }
 stashPut( T , h , Slot{ k , v } );
 return( 1 );
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
  return( inStash( k , [ & ]( std::size_t j ) {
   std::invoke( std::forward< F >( f ) , f_X[ j ].value );
   } ) );
 if constexpr( optimistic ) {  // a copy, stored atomically
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
 fetch( *T , b1 );
 detail::prefetch( &T->T[ b2 ] );  // where the key is if not in b1
 Stripe & s = f_L[ b1 >> T->rg ];
 Hold g( s );
 const std::size_t i = locate( *T , k , t , b1 , b2 , false );
 if( i == ~std::size_t( 0 ) )  // the bit of overflow of b1 stays
  return( inStash( k , [ & ]( std::size_t j ) {
   f_X[ j ] = std::move( f_X.back() );
   f_X.pop_back();
   f_xn.store( f_X.size() , std::memory_order_release );
   } ) );
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
 std::int64_t n = std::int64_t( f_xn.load( std::memory_order_relaxed ) );
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
std::size_t ConcurrentCuckooTable< K , V , H , Eq >::stashed( void ) const
{
 return( f_xn.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentCuckooTable< K , V , H , Eq >::reserve( std::size_t n )
{
 const std::size_t b =
   std::bit_ceil( std::max< std::size_t >( 2 , ( n * 10 + 35 ) / 36 ) );
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
 for( std::size_t i = 0 ; i < T.overflowWords() ; ++i )  // after the tags
  detail::storeRelease< optimistic >( T.O[ i ] , std::uint64_t( 0 ) );
 if constexpr( ! optimistic )
  for( std::size_t i = 0 ; i < ( T.buckets() << 2 ) ; ++i )
   T.S( i ) = Slot{};
 for( std::size_t s = 0 ; s < g.held() ; ++s )
  f_L[ s ].count.store( 0 , std::memory_order_relaxed );
 std::lock_guard< std::mutex > l( f_xm );
 f_X.clear();
 f_xn.store( 0 , std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
template< class F >
void ConcurrentCuckooTable< K , V , H , Eq >::forEach( F && f )
{
 HoldAll g( *this );
 Table & T = *f_cur;
 for( std::size_t i = 0 ; i < ( T.buckets() << 2 ) ; ++i )
  if( ( T.T[ i >> 2 ] >> ( 8 * ( i & 3 ) ) ) & 0xFF ) {
   if constexpr( optimistic ) {  // a copy, stored atomically
    V x = T.S( i ).value;
    f( std::as_const( T.S( i ).key ) , x );
    assign( T , i , x );
    }
   else
    f( std::as_const( T.S( i ).key ) , T.S( i ).value );
   }
 std::lock_guard< std::mutex > l( f_xm );  // a lookup may read the stash
 for( Slot & x : f_X )
  f( std::as_const( x.key ) , x.value );
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
 std::lock_guard< std::mutex > l( f_xm );
 for( const Slot & x : f_X )
  f( x.key , x.value );
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
  if( ( tag( h ) != t ) ||
      ( ( ( i >> 2 ) != b1 ) &&
	( ( ( i >> 2 ) != b2 ) || ! overflowed( T , b1 , t ) ) ) )
   return( false );  // in its other bucket only with the overflow of b1
  int c = 0;          // once in its two buckets
  for( std::size_t b : { b1 , b2 } )
   for( std::size_t j = 4 * b ; j < 4 * b + 4 ; ++j )
    if( ( ( ( T.T[ j >> 2 ] >> ( 8 * ( j & 3 ) ) ) & 0xFF ) == t ) &&
	f_eq( T.S( j ).key , k ) )
     ++c;
  if( c != 1 )
   return( false );
  }
 for( std::size_t s = 0 ; s < g.held() ; ++s )  // each region counted
  if( n[ s ] != f_L[ s ].count.load( std::memory_order_relaxed ) )
   return( false );
 // the stash: counted, each key once, in neither of its buckets, and with
 // the bit of overflow of the first set
 std::lock_guard< std::mutex > l( f_xm );
 if( f_xn.load( std::memory_order_relaxed ) != f_X.size() )
  return( false );
 for( std::size_t j = 0 ; j < f_X.size() ; ++j ) {
  const K & k = f_X[ j ].key;
  const std::uint64_t h = hash( k );
  const std::uint32_t t = tag( h );
  const std::size_t b1 = first( h , T.lg );
  if( ( locate( T , k , t , b1 , other( b1 , t , T.rg ) , false ) !=
	~std::size_t( 0 ) ) ||
      ! overflowed( T , b1 , t ) )
   return( false );
  for( std::size_t i = j + 1 ; i < f_X.size() ; ++i )
   if( f_eq( f_X[ i ].key , k ) )
    return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File ConcurrentCuckooTable.tpp -------------------*/
/*--------------------------------------------------------------------------*/
