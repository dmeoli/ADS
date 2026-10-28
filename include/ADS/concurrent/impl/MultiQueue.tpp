/*--------------------------------------------------------------------------*/
/*-------------------------- File MultiQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MultiQueue [see MultiQueue.h], with their contracts.
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

#include <new>
#include <thread>

#include "ADS/concurrent/impl/Spin.tpp"

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*------------------------------- THE QUEUES -------------------------------*/

template< class K , class T , class Less >
K MultiQueue< K , T , Less >::none( const Less & less )
{
 using L = std::numeric_limits< K >;
 const K hi = L::has_infinity ? L::infinity() : L::max();
 const K lo = L::has_infinity ? -L::infinity() : L::lowest();
 return( less( lo , hi ) ? hi : lo );
 }

template< class K , class T , class Less >
MultiQueue< K , T , Less >::MultiQueue( std::size_t n , unsigned s ,
                                        std::uint64_t seed , Less less )
 : f_n( n ) , f_s( s ) , f_less{ less } , f_none( none( less ) ) ,
   f_q( nullptr ) , f_seed( seed )
{
 ADS_PRE( ( n >= 1 ) && ( s >= 1 ) );
 f_q = static_cast< Queue * >( ::operator new( n * sizeof( Queue ) ,
                               std::align_val_t( alignof( Queue ) ) ) );
 std::size_t i = 0;
 try {
  for( ; i < n ; ++i )
   ::new( static_cast< void * >( f_q + i ) ) Queue( f_less , f_none );
  }
 catch( ... ) {
  while( i > 0 )
   f_q[ --i ].~Queue();
  ::operator delete( f_q , std::align_val_t( alignof( Queue ) ) );
  throw;
  }
 }

template< class K , class T , class Less >
MultiQueue< K , T , Less >::~MultiQueue()
{
 for( std::size_t i = f_n ; i-- > 0 ; )
  f_q[ i ].~Queue();
 ::operator delete( f_q , std::align_val_t( alignof( Queue ) ) );
 }

template< class K , class T , class Less >
auto MultiQueue< K , T , Less >::handle( void ) -> Handle
{
 // the seeds of the handles apart by the odd constant of splitmix64, so
 // that their sequences are the stretches of one sequence far apart
 return( Handle( *this , f_seed.fetch_add( 0x9E3779B97F4A7C15ULL ,
                                           std::memory_order_relaxed ) ) );
 }

template< class K , class T , class Less >
bool MultiQueue< K , T , Less >::tryLock( Queue & q )
{
 // a plain read first, so that a held lock costs no write to its line
 return( ! q.lock.load( std::memory_order_relaxed ) &&
         ! q.lock.exchange( true , std::memory_order_acquire ) );
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::lock( Queue & q )
{
 for( int n = 0 ; ! tryLock( q ) ; )
  if( ++n < 64 )
   detail::spinPause();
  else
   std::this_thread::yield();
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::unlock( Queue & q )
{
 q.lock.store( false , std::memory_order_release );
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::publish( Queue & q ) const
{
 q.top.store( q.nd ? q.D[ q.at( q.nd - 1 ) ].key : f_none ,
              std::memory_order_relaxed );
 q.count.store( std::size_t( q.nd + q.ni ) + q.heap.size() ,
                std::memory_order_relaxed );
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::put( Queue & q , Entry && e ) const
{
 // D a ring sorted by decreasing keys from lo: the least pair is the last
 // one, and the greatest the first, so that a pop and an eviction move no
 // other pair, and an insertion only the pairs less than the new one
 if( q.nd && f_less( e , q.D[ q.lo ] ) ) {
  if( q.nd == kBuf ) {                    // the greatest of D into I
   q.I[ q.ni++ ] = std::move( q.D[ q.lo ] );
   q.lo = std::uint8_t( q.at( 1 ) );
   --q.nd;
   }
  int k = q.nd;                           // sorted, from the least end
  for( ; ( k > 0 ) && f_less( q.D[ q.at( k - 1 ) ] , e ) ; --k )
   q.D[ q.at( k ) ] = std::move( q.D[ q.at( k - 1 ) ] );
  q.D[ q.at( k ) ] = std::move( e );
  ++q.nd;
  }
 else if( ( q.nd == 0 ) ||
          ( ( q.nd < kBuf ) && ( q.ni == 0 ) && q.heap.empty() ) ) {
  // not less than any pair of D, which holds them all: the new greatest
  q.lo = std::uint8_t( q.at( kBuf - 1 ) );
  q.D[ q.lo ] = std::move( e );
  ++q.nd;
  }
 else
  q.I[ q.ni++ ] = std::move( e );
 if( q.ni == kBuf ) {
  for( int i = 0 ; i < q.ni ; ++i )
   q.heap.push( std::move( q.I[ i ] ) );
  q.ni = 0;
  }
 }

template< class K , class T , class Less >
auto MultiQueue< K , T , Less >::take( Queue & q ) const -> Entry
{
 Entry e = std::move( q.D[ q.at( --q.nd ) ] );
 if( q.nd == 0 ) {                        // D again with the least pairs
  for( int i = 0 ; i < q.ni ; ++i )
   q.heap.push( std::move( q.I[ i ] ) );
  q.ni = 0;
  const int m = int( std::min< std::size_t >( kBuf , q.heap.size() ) );
  for( int i = m ; i-- > 0 ; )
   q.D[ i ] = q.heap.pop();
  q.lo = 0;
  q.nd = std::uint8_t( m );
  }
 return( e );
 }

template< class K , class T , class Less >
bool MultiQueue< K , T , Less >::ordered( const Queue & q ) const
{
 if( ( q.lo >= kBuf ) || ( q.nd > kBuf ) || ( q.ni >= kBuf ) )
  return( false );
 if( ( q.nd == 0 ) && ( ( q.ni > 0 ) || ! q.heap.empty() ) )
  return( false );
 for( int k = 1 ; k < q.nd ; ++k )
  if( f_less( q.D[ q.at( k - 1 ) ] , q.D[ q.at( k ) ] ) )
   return( false );
 if( q.nd == 0 )
  return( true );
 for( int i = 0 ; i < q.ni ; ++i )
  if( f_less( q.I[ i ] , q.D[ q.lo ] ) )
   return( false );
 return( q.heap.empty() || ! f_less( q.heap.top() , q.D[ q.lo ] ) );
 }

template< class K , class T , class Less >
auto MultiQueue< K , T , Less >::scan( std::size_t r )
 -> std::optional< Entry >
{
 // each queue locked in turn, also those whose minimum reads empty: when
 // no push runs, an empty answer means that all of them were empty
 for( std::size_t c = 0 ; c < f_n ; ++c ) {
  Queue & q = f_q[ ( r + c ) % f_n ];
  lock( q );
  if( q.nd ) {
   Entry e = take( q );
   publish( q );
   ADS_INVARIANT( ordered( q ) );
   unlock( q );
   return( std::optional< Entry >( std::move( e ) ) );
   }
  unlock( q );
  }
 return( std::nullopt );
 }

template< class K , class T , class Less >
std::size_t MultiQueue< K , T , Less >::size( void ) const
{
 std::size_t n = 0;
 for( std::size_t i = 0 ; i < f_n ; ++i )
  n += f_q[ i ].count.load( std::memory_order_relaxed );
 return( n );
 }

template< class K , class T , class Less >
bool MultiQueue< K , T , Less >::consistent( void ) const
{
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  Queue & q = f_q[ i ];
  lock( q );
  const bool ok = ordered( q ) &&
   ( q.top.load( std::memory_order_relaxed ) ==
     ( q.nd ? q.D[ q.at( q.nd - 1 ) ].key : f_none ) ) &&
   ( q.count.load( std::memory_order_relaxed ) ==
     std::size_t( q.nd + q.ni ) + q.heap.size() );
  unlock( q );
  if( ! ok )
   return( false );
  }
 return( true );
 }

/*------------------------------- THE HANDLES ------------------------------*/

template< class K , class T , class Less >
std::uint64_t MultiQueue< K , T , Less >::Handle::next( void )
{
 std::uint64_t z = ( f_x += 0x9E3779B97F4A7C15ULL );
 z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
 z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
 return( z ^ ( z >> 31 ) );
 }

template< class K , class T , class Less >
std::size_t MultiQueue< K , T , Less >::Handle::any( void )
{
 // the top 32 bits times n, the multiply-shift of Lemire, which needs no
 // division
 return( std::size_t( ( ( next() >> 32 ) * std::uint64_t( f_m->f_n ) )
                      >> 32 ) );
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::Handle::draw( void )
{
 // two distinct queues, unless there is one: the same queue twice would
 // be a pop from a single random queue, whose rank errors grow without
 // bound
 f_a = any();
 f_b = any();
 while( ( f_b == f_a ) && ( f_m->f_n > 1 ) )
  f_b = any();
 f_used = 0;
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::Handle::use( void )
{
 if( f_used >= f_m->f_s )
  draw();
 ++f_used;
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::Handle::push( const K & k , T x )
{
 MultiQueue & m = *f_m;
 for( ; ; ) {
  use();
  Queue & q = m.f_q[ ( next() & 1 ) ? f_a : f_b ];
  if( tryLock( q ) ) {
   m.put( q , Entry{ k , std::move( x ) } );
   m.publish( q );
   ADS_INVARIANT( m.ordered( q ) );
   unlock( q );
   return;
   }
  draw();
  }
 }

template< class K , class T , class Less >
auto MultiQueue< K , T , Less >::Handle::tryPop( void )
 -> std::optional< Entry >
{
 MultiQueue & m = *f_m;
 for( ; ; ) {
  use();
  // the minima read without locks, possibly stale: the order is relaxed
  // anyway, and the lock decides
  Queue & a = m.f_q[ f_a ];
  Queue & b = m.f_q[ f_b ];
  const K ka = a.top.load( std::memory_order_relaxed );
  const K kb = b.top.load( std::memory_order_relaxed );
  Queue & q = m.f_less.less( kb , ka ) ? b : a;
  const K kq = m.f_less.less( kb , ka ) ? kb : ka;
  if( ! m.f_less.less( kq , m.f_none ) )
   return( m.scan( any() ) );             // both read empty
  if( tryLock( q ) ) {
   if( q.nd == 0 ) {
    unlock( q );
    return( m.scan( any() ) );
    }
   Entry e = m.take( q );
   m.publish( q );
   ADS_INVARIANT( m.ordered( q ) );
   unlock( q );
   return( std::optional< Entry >( std::move( e ) ) );
   }
  draw();
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MultiQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
