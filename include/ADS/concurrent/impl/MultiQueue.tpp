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
 // the seed of a handle is a counter mixed as by splitmix64, a point at
 // random of its sequence: seeds apart by its increment would give one
 // sequence shifted by a draw, and the handles taken one after the other
 // would choose the same queues one operation apart
 std::uint64_t z = f_seed.fetch_add( 0x9E3779B97F4A7C15ULL ,
                                     std::memory_order_relaxed );
 z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
 z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
 return( Handle( *this , z ^ ( z >> 31 ) ) );
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
 q.top.store( q.nd ? q.D[ q.nd - 1 ].key : f_none ,
              std::memory_order_relaxed );
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::flush( Queue & q ) const
{
 for( ; q.ni ; --q.ni )
  q.heap.push( std::move( q.I[ q.ni - 1 ] ) );
 }

template< class K , class T , class Less >
void MultiQueue< K , T , Less >::put( Queue & q , Entry && e ) const
{
 // D[ 0 .. nd - 1 ] sorted by decreasing keys: the greatest pair first,
 // the least last; a pair not greater than the greatest of D goes into D,
 // found from the least end, and a full D gives its greatest to I
 if( q.nd && ! f_less( q.D[ 0 ] , e ) ) {
  std::size_t j = q.nd - 1;               // the last place not less than e
  while( f_less( q.D[ j ] , e ) )
   --j;
  if( q.nd == kBuf ) {
   if( q.ni == kBuf ) {
    flush( q );
    q.heap.push( std::move( q.D[ 0 ] ) );
    }
   else
    q.I[ q.ni++ ] = std::move( q.D[ 0 ] );
   std::move( q.D + 1 , q.D + j + 1 , q.D );
   q.D[ j ] = std::move( e );
   }
  else {
   std::move_backward( q.D + j + 1 , q.D + q.nd , q.D + q.nd + 1 );
   q.D[ j + 1 ] = std::move( e );
   ++q.nd;
   }
  return;
  }
 if( ( q.nd < kBuf ) && ( q.ni == 0 ) && q.heap.empty() ) {
  // greater than all the pairs, which D holds: the new greatest
  std::move_backward( q.D , q.D + q.nd , q.D + q.nd + 1 );
  q.D[ 0 ] = std::move( e );
  ++q.nd;
  return;
  }
 if( q.ni == kBuf ) {
  flush( q );
  q.heap.push( std::move( e ) );
  }
 else
  q.I[ q.ni++ ] = std::move( e );
 }

template< class K , class T , class Less >
auto MultiQueue< K , T , Less >::take( Queue & q ) const -> Entry
{
 Entry e = std::move( q.D[ --q.nd ] );
 if( q.nd == 0 ) {                        // D again with the least pairs
  flush( q );
  std::size_t m = std::min( kBuf , q.heap.size() );
  q.nd = std::uint8_t( m );
  while( m )
   q.D[ --m ] = q.heap.pop();
  }
 return( e );
 }

template< class K , class T , class Less >
bool MultiQueue< K , T , Less >::ordered( const Queue & q ) const
{
 if( ( q.nd > kBuf ) || ( q.ni > kBuf ) )
  return( false );
 if( ( q.nd == 0 ) && ( ( q.ni > 0 ) || ! q.heap.empty() ) )
  return( false );
 for( std::size_t k = 1 ; k < q.nd ; ++k )
  if( f_less( q.D[ k - 1 ] , q.D[ k ] ) )
   return( false );
 if( q.nd == 0 )
  return( true );
 for( std::size_t i = 0 ; i < q.ni ; ++i )
  if( f_less( q.I[ i ] , q.D[ 0 ] ) )
   return( false );
 return( q.heap.empty() || ! f_less( q.heap.top() , q.D[ 0 ] ) );
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
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  Queue & q = f_q[ i ];
  lock( q );
  n += std::size_t( q.nd ) + q.ni + q.heap.size();
  unlock( q );
  }
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
     ( q.nd ? q.D[ q.nd - 1 ].key : f_none ) );
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
std::size_t MultiQueue< K , T , Less >::Handle::bit( void )
{
 if( f_nbits == 0 ) {
  f_bits = next();
  f_nbits = 64;
  }
 --f_nbits;
 const std::size_t b = std::size_t( f_bits & 1 );
 f_bits >>= 1;
 return( b );
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
 f_k[ 0 ] = any();
 f_k[ 1 ] = any();
 while( ( f_k[ 1 ] == f_k[ 0 ] ) && ( f_m->f_n > 1 ) )
  f_k[ 1 ] = any();
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
void MultiQueue< K , T , Less >::Handle::push( K k , T x )
{
 MultiQueue & m = *f_m;
 for( ; ; ) {
  use();
  // one of the two by a random bit as an index, not as a branch, whose
  // outcome would be mispredicted half of the times
  Queue & q = m.f_q[ f_k[ bit() ] ];
  if( tryLock( q ) ) {
   m.put( q , Entry{ std::move( k ) , std::move( x ) } );
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
  // the better of the two selected by an index, not by a branch, whose
  // outcome is that of two keys at random
  const K kk[ 2 ] = {
   m.f_q[ f_k[ 0 ] ].top.load( std::memory_order_relaxed ) ,
   m.f_q[ f_k[ 1 ] ].top.load( std::memory_order_relaxed ) };
  const std::size_t w = m.f_less.less( kk[ 1 ] , kk[ 0 ] );
  Queue & q = m.f_q[ f_k[ w ] ];
  const K kq = kk[ w ];
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
