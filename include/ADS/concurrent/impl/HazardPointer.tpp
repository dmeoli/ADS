/*--------------------------------------------------------------------------*/
/*-------------------------- File HazardPointer.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of HazardDomain [see HazardPointer.h], with their
 * contracts.
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

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*------------------------------- THE DOMAIN -------------------------------*/

/// the bin of a thread on `HazardDomain::global()`, given back when the
/// thread ends
class HazardDomain::Home
{
 public:
 Bin * bin = nullptr;  ///< the bin, nil until the first retire

 /// the bin given back, after a scan
 ~Home() {
  if( bin )
   HazardDomain::global().releaseBin( bin );
  }
 };

inline HazardDomain::~HazardDomain()
{
 for( Bin * b = f_bins.load( std::memory_order_acquire ) ; b ; ) {
  Bin * q = b->next;
  ADS_PRE( ! b->used.load( std::memory_order_acquire ) );
  detail::freeRetired( b->head );
  delete b;
  b = q;
  }
 for( Slot * s = f_slots.load( std::memory_order_acquire ) ; s ; ) {
  Slot * q = s->next;
  ADS_PRE( ! s->used.load( std::memory_order_acquire ) );
  delete s;
  s = q;
  }
 }

inline std::size_t HazardDomain::bins( void ) const
{
 std::size_t n = 0;
 for( Bin * b = f_bins.load( std::memory_order_acquire ) ; b ; b = b->next )
  ++n;
 return( n );
 }

inline std::size_t HazardDomain::pending( void ) const
{
 std::uint64_t n = 0;
 for( Bin * b = f_bins.load( std::memory_order_acquire ) ; b ; b = b->next )
  n += b->count.load( std::memory_order_relaxed );
 return( std::size_t( n ) );
 }

inline std::uint64_t HazardDomain::freed( void ) const
{
 std::uint64_t n = 0;
 for( Bin * b = f_bins.load( std::memory_order_acquire ) ; b ; b = b->next )
  n += b->freed.load( std::memory_order_relaxed );
 return( n );
 }

inline auto HazardDomain::acquireSlot( void ) -> Slot *
{
 for( Slot * s = f_slots.load( std::memory_order_acquire ) ; s ; s = s->next )
  if( ( ! s->used.load( std::memory_order_relaxed ) ) &&
      ( ! s->used.exchange( true , std::memory_order_acquire ) ) )
   return( s );
 Slot * s = new Slot;
 // counted before it is published, so that H bounds the slots that a scan
 // reads, and pushed by a sequentially consistent compare-and-swap, which
 // a scan that reads the first slot with sequential consistency after it
 // in S sees [see the theory page]
 f_slots_n.fetch_add( 1 , std::memory_order_relaxed );
 s->next = f_slots.load( std::memory_order_relaxed );
 while( ! f_slots.compare_exchange_weak( s->next , s ,
					 std::memory_order_seq_cst ,
					 std::memory_order_relaxed ) ) {
  }
 return( s );
 }

inline void HazardDomain::releaseSlot( Slot * s ) noexcept
{
 s->ptr.store( nullptr , std::memory_order_release );
 s->used.store( false , std::memory_order_release );
 }

inline auto HazardDomain::acquireBin( void ) -> Bin *
{
 for( Bin * b = f_bins.load( std::memory_order_acquire ) ; b ; b = b->next )
  if( ( ! b->used.load( std::memory_order_relaxed ) ) &&
      ( ! b->used.exchange( true , std::memory_order_acquire ) ) )
   return( b );
 Bin * b = new Bin;
 b->next = f_bins.load( std::memory_order_relaxed );
 while( ! f_bins.compare_exchange_weak( b->next , b ,
					std::memory_order_release ,
					std::memory_order_relaxed ) ) {
  }
 return( b );
 }

inline void HazardDomain::releaseBin( Bin * b ) noexcept
{
 if( b->head )
  scan( *b );
 b->used.store( false , std::memory_order_release );
 }

inline void HazardDomain::retire( Bin & b , Retirable * p ) noexcept
{
 ADS_PRE( p->reclaim );
 p->retired_next = b.head;
 b.head = p;
 const std::uint64_t n = b.count.load( std::memory_order_relaxed ) + 1;
 b.count.store( n , std::memory_order_relaxed );
 if( ( n >= threshold() ) && ( ! b.scanning ) )
  scan( b );
 }

inline void HazardDomain::scan( Bin & b ) noexcept
{
 // the slots read after the unlink of every node of the bin, each node
 // having been retired by this thread after its unlink [see the theory
 // page]; a node held by no slot can no longer be protected, since a
 // protect() that comes later reads again a word that no longer reaches it
 b.scanning = true;
 b.seen.clear();
 try {
  for( Slot * s = f_slots.load( std::memory_order_seq_cst ) ; s ; s = s->next )
   if( const void * p = s->ptr.load( std::memory_order_seq_cst ) )
    b.seen.push_back( p );
  }
 catch( ... ) {
  b.scanning = false;  // no room for the vector: the nodes wait
  return;
  }
 std::sort( b.seen.begin() , b.seen.end() );
 // the list taken whole, so that a node retired by a deleter goes to a new
 // list and is scanned later
 Retirable * p = std::exchange( b.head , nullptr );
 Retirable * kept = nullptr;
 Retirable * last = nullptr;
 std::uint64_t k = 0;
 std::uint64_t f = 0;
 while( p ) {
  Retirable * q = p->retired_next;
  const void * a = p->reclaim( p , Retirable::Op::address );
  if( std::binary_search( b.seen.begin() , b.seen.end() , a ) ) {
   p->retired_next = kept;
   if( ! kept )
    last = p;
   kept = p;
   ++k;
   }
  else {
   p->reclaim( p , Retirable::Op::free );
   ++f;
   }
  p = q;
  }
 if( kept ) {
  last->retired_next = b.head;
  b.head = kept;
  }
 const std::uint64_t n = b.count.load( std::memory_order_relaxed );
 b.count.store( n - f , std::memory_order_relaxed );
 detail::ownerAdd( b.freed , f );
 b.scanning = false;
 ADS_INVARIANT( k <= b.seen.size() );
 }

inline auto HazardDomain::home( void ) -> Bin &
{
 // the domain is made before the bin of the thread, and therefore
 // destroyed after it, also on the main thread
 HazardDomain & d = global();
 thread_local Home h;
 if( ! h.bin )
  h.bin = d.acquireBin();
 return( *h.bin );
 }

/*------------------------------ THE RECORDS -------------------------------*/

template< int K >
HazardDomain::Local< K >::Local( HazardDomain & d ) : f_d( &d )
{
 std::size_t i = 0;
 try {
  for( ; i < std::size_t( K ) ; ++i )
   f_s[ i ] = d.acquireSlot();
  f_b = d.acquireBin();
  }
 catch( ... ) {
  while( i > 0 )
   releaseSlot( f_s[ --i ] );
  throw;
  }
 }

template< int K >
HazardDomain::Local< K >::~Local()
{
 if( ! f_b )
  return;
 for( Slot * s : f_s )
  releaseSlot( s );
 f_d->releaseBin( f_b );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HazardPointer.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
