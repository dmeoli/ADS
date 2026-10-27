/*--------------------------------------------------------------------------*/
/*---------------------- File ConcurrentFlatTable.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConcurrentFlatTable [see ConcurrentFlatTable.h], with
 * their contracts.
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
#include <cstring>
#include <new>
#include <stdexcept>
#include <thread>

#include "ADS/concurrent/impl/Spin.tpp"

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// the identities of the tables, never reused, so that a reservation of
/// nodes kept by a thread is never taken for one of another table
inline std::atomic< std::uint64_t > flatTableIds{ 0 };

/// the identities of the threads, which pick their counters
inline std::atomic< unsigned > flatThreadIds{ 0 };

}  // end( namespace detail )

/*------------------------------- THE WORDS --------------------------------*/

template< class K , class V , class H , class Eq >
template< class X >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::bits( const X & x ,
                                                           std::size_t at )
{
 std::uint64_t w = 0;
 std::memcpy( reinterpret_cast< unsigned char * >( &w ) + at , &x ,
              sizeof( X ) );
 return( w );
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::pack( const K & k ,
                                                           const V & v )
{
 if constexpr( valueBytes() == 0 ) {
  (void) v;
  return( bits( k , 0 ) );
  }
 else
  return( bits( k , 0 ) | bits( v , sizeof( K ) ) );
 }

template< class K , class V , class H , class Eq >
K ConcurrentFlatTable< K , V , H , Eq >::keyOf( std::uint64_t w )
{
 K k;
 std::memcpy( &k , &w , sizeof( K ) );
 return( k );
 }

template< class K , class V , class H , class Eq >
V ConcurrentFlatTable< K , V , H , Eq >::valueOf( std::uint64_t w )
{
 if constexpr( valueBytes() == 0 ) {
  (void) w;
  return( V{} );
  }
 else {
  V v;
  std::memcpy( &v , reinterpret_cast< const unsigned char * >( &w ) +
               sizeof( K ) , sizeof( V ) );
  return( v );
  }
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::mark( void )
{
 // a byte 1 in the place of the first byte of the key, which is 0
 return( bits( static_cast< unsigned char >( 1 ) , 0 ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::zeroKey( const K & k )
{
 if constexpr( packed )
  return( bits( k , 0 ) == 0 );
 else {
  (void) k;
  return( false );
  }
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentFlatTable< K , V , H , Eq >::home( std::uint64_t h ,
                                                         const Table & T )
{
 return( std::size_t( h >> ( 64 - T.lg ) ) );
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::print( std::uint64_t h )
{
 // the low bits, independent of the top ones that give the place
 return( h & 0xFFFFFF );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::node( std::uint64_t i ) const
 -> Node *
{
 // the segment j holds the nodes from 2^12 ( 2^j - 1 ), 2^( 12 + j ) of
 // them
 const int j = std::bit_width( ( i >> kLgSeg ) + 1 ) - 1;
 const std::uint64_t base = ( ( std::uint64_t( 1 ) << j ) - 1 ) << kLgSeg;
 return( f_seg[ j ].load( std::memory_order_acquire ) + ( i - base ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::holds( std::uint64_t w ,
                                                   const K & k ,
                                                   std::uint64_t h ) const
{
 if constexpr( packed ) {
  (void) h;
  return( f_eq( keyOf( w ) , k ) );
  }
 else {
  if( ( w >> kFp ) != print( h ) )
   return( false );
  const Node * p = node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) - 1 );
  return( ( p->h == h ) && f_eq( p->e.key , k ) );
  }
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::hashOf(
                                                   std::uint64_t w ) const
{
 if constexpr( packed )
  return( hash( keyOf( w ) ) );
 else
  return( node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) - 1 )->h );
 }

/*------------------------------- THE VECTORS ------------------------------*/

template< class K , class V , class H , class Eq >
ConcurrentFlatTable< K , V , H , Eq >::Table::Table( int l )
 : lg( l ) ,
   W( HugePageAllocator< std::atomic< std::uint64_t > >().allocate(
       std::size_t( 1 ) << l ) )
{
 // on huge pages when large, so that a lookup misses the TLB once per 2 MB
 std::uninitialized_value_construct_n( W , places() );
 }

template< class K , class V , class H , class Eq >
ConcurrentFlatTable< K , V , H , Eq >::Table::~Table()
{
 std::destroy_n( W , places() );
 HugePageAllocator< std::atomic< std::uint64_t > >().deallocate( W ,
                                                                 places() );
 }

template< class K , class V , class H , class Eq >
ConcurrentFlatTable< K , V , H , Eq >::ConcurrentFlatTable( std::size_t n ,
                                                            H h , Eq eq )
 : f_S( std::make_unique< Stripe[] >( kStripes ) ) ,
   f_id( detail::flatTableIds.fetch_add( 1 , std::memory_order_relaxed ) +
         1 ) ,
   f_h( h ) , f_eq( eq )
{
 for( auto & s : f_seg )
  s.store( nullptr , std::memory_order_relaxed );
 const int lg = std::max( kLeast , int( std::bit_width(
                                    std::max< std::size_t >( 2 * n , 1 ) -
                                    1 ) ) );
 f_cur = std::make_unique< Table >( lg );
 f_table.store( f_cur.get() , std::memory_order_release );
 nodes( n );
 }

template< class K , class V , class H , class Eq >
ConcurrentFlatTable< K , V , H , Eq >::~ConcurrentFlatTable()
{
 if constexpr( ! packed ) {
  const Table & T = *f_cur;
  for( std::size_t i = 0 ; i < T.places() ; ++i )
   if( const std::uint64_t w = T.W[ i ].load( std::memory_order_relaxed ) )
    node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) - 1 )->~Node();
  for( int j = 0 ; j < kSegs ; ++j )
   if( Node * p = f_seg[ j ].load( std::memory_order_relaxed ) )
    HugePageAllocator< Node >().deallocate( p , std::size_t( 1 ) <<
                                                ( kLgSeg + j ) );
  }
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::hash( const K & k ) const
{
 if constexpr( requires { typename H::is_avalanching; } )
  return( std::uint64_t( f_h( k ) ) );
 else
  return( hashWord( std::uint64_t( f_h( k ) ) ) );
 }

/*------------------------------- THE NODES --------------------------------*/

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::slabs( void ) -> Slab *
{
 thread_local Slab s[ 4 ];
 return( s );
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::segment( int j , bool touch )
{
 // on huge pages when large, so that the nodes are faulted in 2 MB at a
 // time, and faulted at once if asked; the thread that loses the race
 // frees its own
 if( f_seg[ j ].load( std::memory_order_acquire ) )
  return;
 const std::size_t m = std::size_t( 1 ) << ( kLgSeg + j );
 Node * p = HugePageAllocator< Node >().allocate( m );
 if( touch )
  std::memset( static_cast< void * >( p ) , 0 , m * sizeof( Node ) );
 Node * e = nullptr;
 if( ! f_seg[ j ].compare_exchange_strong( e , p ,
                                           std::memory_order_acq_rel ) )
  HugePageAllocator< Node >().deallocate( p , m );
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::claim( void )
{
 const std::uint64_t id = f_id.load( std::memory_order_relaxed );
 Slab * S = slabs();
 Slab * s = nullptr;
 for( int i = 0 ; i < 4 ; ++i )
  if( S[ i ].id == id ) {
   s = S + i;
   break;
   }
 if( s && ( s->next < s->end ) )
  return( s->next );
 if( ! s ) {                              // the least recently filled
  s = S;
  for( int i = 1 ; i < 4 ; ++i )
   if( S[ i ].end < s->end )
    s = S + i;
  }
 // 64 more nodes, all in one segment, since the segments start at
 // multiples of 2^12
 const std::uint64_t b = f_nodes.fetch_add( kSlab ,
                                            std::memory_order_relaxed );
 const int j = std::bit_width( ( b >> kLgSeg ) + 1 ) - 1;
 if( ( j >= kSegs ) || ( b + kSlab >= ( std::uint64_t( 1 ) << kFp ) ) )
  throw( std::length_error( "ConcurrentFlatTable::claim: more than 2^40 "
                            "nodes" ) );
 segment( j , false );
 *s = Slab{ id , b , b + kSlab };
 return( b );
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::consume( std::uint64_t i )
{
 const std::uint64_t id = f_id.load( std::memory_order_relaxed );
 Slab * S = slabs();
 for( int j = 0 ; j < 4 ; ++j )
  if( ( S[ j ].id == id ) && ( S[ j ].next == i ) ) {
   ++S[ j ].next;
   return;
   }
 }

/*------------------------------- THE GROWTH -------------------------------*/

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::stripe( void ) const -> Stripe &
{
 thread_local const unsigned t = detail::flatThreadIds.fetch_add(
                                  1 , std::memory_order_relaxed );
 return( f_S[ t % kStripes ] );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::enter( Stripe & s ) -> Table *
{
 // the protocol of Dekker: the counter raised, then the flag read, both
 // sequentially consistent, against the flag raised, then the counters
 // read, by the thread that grows the table
 for( ; ; ) {
  s.w.fetch_add( 1 , std::memory_order_seq_cst );
  if( ! f_grow.load( std::memory_order_seq_cst ) )
   return( f_table.load( std::memory_order_acquire ) );
  s.w.fetch_sub( 1 , std::memory_order_release );
  help();
  }
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::copy( Table & T , Table & N )
{
 const std::size_t m = N.places() - 1;
 for( ; ; ) {
  const std::size_t b = T.claimed.fetch_add( 1 , std::memory_order_relaxed );
  if( b >= T.blocks() )
   return;
  const std::size_t e = std::min( T.places() , ( b + 1 ) * kBlock );
  for( std::size_t i = b * kBlock ; i < e ; ++i ) {
   const std::uint64_t w = T.W[ i ].load( std::memory_order_acquire );
   if( w == 0 )
    continue;
   for( std::size_t j = home( hashOf( w ) , N ) ; ; j = ( j + 1 ) & m ) {
    std::uint64_t x = 0;
    if( N.W[ j ].compare_exchange_strong( x , w ,
                                          std::memory_order_relaxed ) )
     break;
    }
   }
  T.copied.fetch_add( 1 , std::memory_order_release );
  }
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::help( void )
{
 // the copy of the vector in use, once its next one is published; the
 // vectors replaced stay, so that a stale one has all its blocks claimed
 for( int n = 0 ; f_grow.load( std::memory_order_acquire ) ; ) {
  Table * T = f_table.load( std::memory_order_acquire );
  if( Table * N = T->next.load( std::memory_order_acquire ) ;
      N && ( T->claimed.load( std::memory_order_relaxed ) < T->blocks() ) )
   copy( *T , *N );
  else if( ++n < 64 )
   detail::spinPause();
  else
   std::this_thread::yield();
  }
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::grow( Table * T , int lg )
{
 bool e = false;
 if( ! f_grow.compare_exchange_strong( e , true ,
                                       std::memory_order_seq_cst ) ) {
  help();                                 // another thread grows it
  return;
  }
 if( ( f_table.load( std::memory_order_relaxed ) != T ) || ( T->lg >= lg ) ) {
  f_grow.store( false , std::memory_order_release );
  return;                                 // grown meanwhile
  }
 for( int i = 0 ; i < kStripes ; ++i )    // the critical sections out
  for( int n = 0 ;
       f_S[ i ].w.load( std::memory_order_seq_cst ) & 0xFFFF ; )
   if( ++n < 64 )
    detail::spinPause();
   else
    std::this_thread::yield();
 std::unique_ptr< Table > N;
 try {
  N = std::make_unique< Table >( lg );
  f_old.reserve( f_old.size() + 1 );
  }
 catch( ... ) {                           // the table as it was
  f_grow.store( false , std::memory_order_release );
  throw;
  }
 T->next.store( N.get() , std::memory_order_release );
 copy( *T , *N );
 for( int n = 0 ;
      T->copied.load( std::memory_order_acquire ) < T->blocks() ; )
  if( ++n < 64 )
   detail::spinPause();
  else
   std::this_thread::yield();
 f_old.push_back( std::move( f_cur ) );   // a reader may be on it
 f_cur = std::move( N );
 f_table.store( f_cur.get() , std::memory_order_release );
 f_rehashes.fetch_add( 1 , std::memory_order_relaxed );
 f_grow.store( false , std::memory_order_release );
 }

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::period( int lg )
{
 // a sum of the 64 counters misses on up to 64 lines: once per 1024
 // insertions of a counter in a large vector, more often in a small one,
 // whose half it would otherwise pass by much
 return( std::uint64_t( 1 ) << std::clamp( lg - 10 , 0 , 10 ) );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentFlatTable< K , V , H , Eq >::counted( void ) const
{
 std::uint64_t n = 0;
 for( int i = 0 ; i < kStripes ; ++i )
  n += f_S[ i ].w.load( std::memory_order_relaxed ) >> 16;
 return( std::size_t( n ) );
 }

/*------------------------------ THE LOOKUPS -------------------------------*/

template< class K , class V , class H , class Eq >
std::uint64_t ConcurrentFlatTable< K , V , H , Eq >::lookup(
                                     const K & k , std::uint64_t h ) const
{
 ADS_AUDIT( hash( k ) == h );
 if( zeroKey( k ) )
  return( f_zero.load( std::memory_order_acquire ) );
 for( ; ; ) {
  const Table * T = f_table.load( std::memory_order_acquire );
  const std::size_t m = T->places() - 1;
  for( std::size_t i = home( h , *T ) , r = 0 ; r <= m ;
       i = ( i + 1 ) & m , ++r ) {
   const std::uint64_t w = T->W[ i ].load( std::memory_order_acquire );
   if( w == 0 )
    break;
   if( holds( w , k , h ) )
    return( w );
   }
  // absent at the free place read, unless the table grew meanwhile
  if( f_table.load( std::memory_order_acquire ) == T )
   return( 0 );
  }
 }

template< class K , class V , class H , class Eq >
std::optional< V > ConcurrentFlatTable< K , V , H , Eq >::find(
                                     const K & k , std::uint64_t h ) const
{
 const std::uint64_t w = lookup( k , h );
 if( w == 0 )
  return( std::nullopt );
 if constexpr( packed )
  return( std::optional< V >( valueOf( w ) ) );
 else
  return( std::optional< V >( node( ( w & ( ( std::uint64_t( 1 ) << kFp ) -
                                            1 ) ) - 1 )->e.value ) );
 }

template< class K , class V , class H , class Eq >
std::optional< V > ConcurrentFlatTable< K , V , H , Eq >::find(
                                                   const K & k ) const
{
 return( find( k , hash( k ) ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::contains( const K & k ,
                                                      std::uint64_t h ) const
{
 return( lookup( k , h ) != 0 );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::contains( const K & k ) const
{
 return( lookup( k , hash( k ) ) != 0 );
 }

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::entry( const K & k ,
                                                   std::uint64_t h ) const
 -> Entry * requires( ! packed )
{
 const std::uint64_t w = lookup( k , h );
 if( w == 0 )
  return( nullptr );
 return( &node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) - 1 )->e );
 }

/*------------------------------ THE INSERTION -----------------------------*/

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::findOrInsert( const K & k ,
                                                          std::uint64_t h ,
                                                          const V & v )
 -> std::pair< Ref , bool >
{
 ADS_AUDIT( hash( k ) == h );
 Stripe & s = stripe();
 auto ref = [ & ]( std::uint64_t w ) -> Ref {
  if constexpr( packed )
   return( valueOf( w ) );
  else
   return( node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) - 1 )->e );
  };
 if constexpr( packed )
  if( zeroKey( k ) ) {                    // kept apart, with a mark
   const std::uint64_t w = pack( k , v ) | mark();
   std::uint64_t x = 0;
   if( f_zero.compare_exchange_strong( x , w ,
                                       std::memory_order_acq_rel ) ) {
    s.w.fetch_add( std::uint64_t( 1 ) << 16 , std::memory_order_relaxed );
    return( std::pair< Ref , bool >( valueOf( w ) , true ) );
    }
   return( std::pair< Ref , bool >( valueOf( x ) , false ) );
   }
 Node * nd = nullptr;                     // the node made, if any
 std::uint64_t idx = 0;
 for( ; ; ) {
  Table * T = enter( s );
  const std::size_t m = T->places() - 1;
  bool crowded = false;
  for( std::size_t i = home( h , *T ) , r = 0 ; ; i = ( i + 1 ) & m ) {
   std::uint64_t x = T->W[ i ].load( std::memory_order_acquire );
   if( x == 0 ) {
    std::uint64_t w;
    if constexpr( packed )
     w = pack( k , v );
    else {
     if( ! nd ) {
      idx = claim();
      nd = ::new( static_cast< void * >( node( idx ) ) ) Node{ h ,
                                                              { k , v } };
      }
     w = ( print( h ) << kFp ) | ( idx + 1 );
     }
    if( T->W[ i ].compare_exchange_strong( x , w ,
                                           std::memory_order_acq_rel ,
                                           std::memory_order_acquire ) ) {
     if constexpr( ! packed )
      consume( idx );
     ADS_INVARIANT( reached( *T , i ) );
     const std::uint64_t c = s.w.fetch_add( ( std::uint64_t( 1 ) << 16 ) -
                                            1 , std::memory_order_release );
     if( ( ( ( c >> 16 ) + 1 ) & ( period( T->lg ) - 1 ) ) == 0 &&
         ( 2 * counted() > T->places() ) )
      grow( T , T->lg + 1 );
     return( std::pair< Ref , bool >( ref( w ) , true ) );
     }
    }
   if( holds( x , k , h ) ) {             // there, or just inserted
    s.w.fetch_sub( 1 , std::memory_order_release );
    if constexpr( ! packed )
     if( nd )
      nd->~Node();
    return( std::pair< Ref , bool >( ref( x ) , false ) );
    }
   if( ++r == T->places() ) {             // full
    crowded = true;
    break;
    }
   if( ( r == kProbe ) && ( 8 * counted() >= T->places() ) ) {
    crowded = true;
    break;
    }
   }
  s.w.fetch_sub( 1 , std::memory_order_release );
  if( crowded )
   grow( T , T->lg + 1 );
  }
 }

template< class K , class V , class H , class Eq >
auto ConcurrentFlatTable< K , V , H , Eq >::findOrInsert( const K & k ,
                                                          const V & v )
 -> std::pair< Ref , bool >
{
 return( findOrInsert( k , hash( k ) , v ) );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::insert( const K & k ,
                                                    const V & v )
{
 return( findOrInsert( k , hash( k ) , v ).second );
 }

template< class K , class V , class H , class Eq >
template< class F >
bool ConcurrentFlatTable< K , V , H , Eq >::update( const K & k , F && f )
 requires( packed )
{
 const std::uint64_t h = hash( k );
 if( zeroKey( k ) ) {
  for( std::uint64_t x = f_zero.load( std::memory_order_acquire ) ; x ; )
   if( f_zero.compare_exchange_weak( x , pack( k , f( valueOf( x ) ) ) |
                                     mark() ,
                                     std::memory_order_acq_rel ) )
    return( true );
  return( false );
  }
 // a writer, so that no growth copies the word while it changes
 Stripe & s = stripe();
 Table * T = enter( s );
 const std::size_t m = T->places() - 1;
 bool found = false;
 for( std::size_t i = home( h , *T ) , r = 0 ; r <= m ;
      i = ( i + 1 ) & m , ++r ) {
  std::uint64_t x = T->W[ i ].load( std::memory_order_acquire );
  if( x == 0 )
   break;
  if( f_eq( keyOf( x ) , k ) ) {
   while( ! T->W[ i ].compare_exchange_weak( x , pack( k ,
                                             f( valueOf( x ) ) ) ,
                                             std::memory_order_acq_rel ) )
    ;
   found = true;
   break;
   }
  }
 s.w.fetch_sub( 1 , std::memory_order_release );
 return( found );
 }

/*------------------------------ THE WHOLE -------------------------------- */

template< class K , class V , class H , class Eq >
std::size_t ConcurrentFlatTable< K , V , H , Eq >::size( void ) const
{
 return( counted() );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentFlatTable< K , V , H , Eq >::capacity( void ) const
{
 return( f_table.load( std::memory_order_acquire )->places() );
 }

template< class K , class V , class H , class Eq >
std::size_t ConcurrentFlatTable< K , V , H , Eq >::rehashes( void ) const
{
 return( f_rehashes.load( std::memory_order_relaxed ) );
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::nodes( std::size_t n )
{
 // the segments of the first n nodes and of the reservations of 64 threads
 if constexpr( ! packed ) {
  const std::uint64_t m = std::uint64_t( n ) + kSlab * kStripes;
  for( int j = 0 ; ( j < kSegs ) &&
       ( ( ( ( std::uint64_t( 1 ) << j ) - 1 ) << kLgSeg ) < m ) ; ++j )
   segment( j , true );
  }
 else
  (void) n;
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::reserve( std::size_t n )
{
 nodes( n );
 const int lg = int( std::bit_width( std::max< std::size_t >( 2 * n , 1 ) -
                                     1 ) );
 for( Table * T = f_table.load( std::memory_order_acquire ) ; T->lg < lg ;
      T = f_table.load( std::memory_order_acquire ) )
  grow( T , lg );
 }

template< class K , class V , class H , class Eq >
void ConcurrentFlatTable< K , V , H , Eq >::clear( void )
{
 Table & T = *f_cur;
 for( std::size_t i = 0 ; i < T.places() ; ++i )
  if( const std::uint64_t w = T.W[ i ].load( std::memory_order_relaxed ) ) {
   if constexpr( ! packed )
    node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) - 1 )->~Node();
   T.W[ i ].store( 0 , std::memory_order_relaxed );
   }
 f_zero.store( 0 , std::memory_order_relaxed );
 for( int i = 0 ; i < kStripes ; ++i )
  f_S[ i ].w.store( 0 , std::memory_order_relaxed );
 // the nodes again from the first, under a new identity, so that no
 // thread uses a reservation made before
 f_nodes.store( 0 , std::memory_order_relaxed );
 f_id.store( detail::flatTableIds.fetch_add( 1 , std::memory_order_relaxed )
             + 1 , std::memory_order_relaxed );
 f_old.clear();
 }

template< class K , class V , class H , class Eq >
template< class F >
void ConcurrentFlatTable< K , V , H , Eq >::forEach( F && f ) const
{
 if constexpr( packed )
  if( const std::uint64_t x = f_zero.load( std::memory_order_acquire ) )
   f( keyOf( 0 ) , valueOf( x ) );
 const Table & T = *f_table.load( std::memory_order_acquire );
 for( std::size_t i = 0 ; i < T.places() ; ++i )
  if( const std::uint64_t w = T.W[ i ].load( std::memory_order_acquire ) ) {
   if constexpr( packed )
    f( keyOf( w ) , valueOf( w ) );
   else {
    const Node * p = node( ( w & ( ( std::uint64_t( 1 ) << kFp ) - 1 ) ) -
                           1 );
    f( p->e.key , p->e.value );
    }
   }
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::reached( const Table & T ,
                                                     std::size_t i ) const
{
 const std::size_t m = T.places() - 1;
 const std::uint64_t w = T.W[ i ].load( std::memory_order_acquire );
 const std::uint64_t h = hashOf( w );
 if constexpr( ! packed )
  if( ( w >> kFp ) != print( h ) )
   return( false );
 for( std::size_t j = home( h , T ) ; j != i ; j = ( j + 1 ) & m ) {
  const std::uint64_t x = T.W[ j ].load( std::memory_order_acquire );
  if( x == 0 )
   return( false );                       // a free place in between
  bool twice;
  if constexpr( packed )
   twice = f_eq( keyOf( x ) , keyOf( w ) );
  else
   twice = ( x == w );
  if( twice )
   return( false );                       // the key twice
  }
 return( true );
 }

template< class K , class V , class H , class Eq >
bool ConcurrentFlatTable< K , V , H , Eq >::consistent( void ) const
{
 const Table & T = *f_table.load( std::memory_order_acquire );
 if( ( &T != f_cur.get() ) || f_grow.load( std::memory_order_acquire ) )
  return( false );
 std::size_t n = f_zero.load( std::memory_order_acquire ) ? 1 : 0;
 for( std::size_t i = 0 ; i < T.places() ; ++i )
  if( T.W[ i ].load( std::memory_order_acquire ) ) {
   ++n;
   if( ! reached( T , i ) )
    return( false );
   }
 for( int i = 0 ; i < kStripes ; ++i )    // no thread inside
  if( f_S[ i ].w.load( std::memory_order_acquire ) & 0xFFFF )
   return( false );
 return( n == counted() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File ConcurrentFlatTable.tpp --------------------*/
/*--------------------------------------------------------------------------*/
