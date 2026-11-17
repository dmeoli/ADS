/*--------------------------------------------------------------------------*/
/*--------------------- File LockFreePriorityQueue.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LockFreePriorityQueue [see LockFreePriorityQueue.h],
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
#include <new>
#include <vector>

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*-------------------------------- THE NODES -------------------------------*/

template< class K , class T , class Less >
void *
LockFreePriorityQueue< K , T , Less >::allocate( std::size_t n , int h ,
					       std::atomic< Word > *& next )
{
 constexpr std::size_t a = std::max(
   { alignof( Node ) , alignof( Link ) , alignof( std::atomic< Word > ) } );
 constexpr std::size_t w = sizeof( std::atomic< Word > );
 n = ( n + w - 1 ) / w * w;  // the pointers aligned after it
 void * p = ::operator new( n + std::size_t( h ) * w , std::align_val_t( a ) );
 next =
   reinterpret_cast< std::atomic< Word > * >( static_cast< char * >( p ) + n );
 for( int i = 0 ; i < h ; ++i )
  ::new( static_cast< void * >( next + i ) ) std::atomic< Word >( 0 );
 return( p );
 }

template< class K , class T , class Less >
auto LockFreePriorityQueue< K , T , Less >::makeLink( int h ) -> Link *
{
 std::atomic< Word > * next;
 void * p = allocate( sizeof( Link ) , h , next );
 return( ::new( p ) Link{ next , nullptr , h , false } );
 }

template< class K , class T , class Less >
auto LockFreePriorityQueue< K , T , Less >::make( int h , const K & k ,
						std::uint64_t s , T && x )
  -> Node *
{
 std::atomic< Word > * next;
 void * p = allocate( sizeof( Node ) , h , next );
 // no exception past the allocation: the key is copied, the item moved,
 // both without exceptions
 return( ::new( p )
	    Node{ { next , nullptr , h , true } , k , s , std::move( x ) } );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::destroy( Node * p )
{
 constexpr std::size_t a = std::max(
   { alignof( Node ) , alignof( Link ) , alignof( std::atomic< Word > ) } );
 p->~Node();
 ::operator delete( static_cast< void * >( p ) , std::align_val_t( a ) );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::destroyLink( Link * p )
{
 constexpr std::size_t a = std::max(
   { alignof( Node ) , alignof( Link ) , alignof( std::atomic< Word > ) } );
 p->~Link();
 ::operator delete( static_cast< void * >( p ) , std::align_val_t( a ) );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::destroyAll( Link * p )
{
 while( p ) {
  Link * q = p->later;
  destroy( static_cast< Node * >( p ) );
  p = q;
  }
 }

template< class K , class T , class Less >
bool LockFreePriorityQueue< K , T , Less >::before( const Link * p ,
						    const K & k ,
						  std::uint64_t s ) const
{
 const Node * n = static_cast< const Node * >( p );
 return( f_less( n->key , k ) ||
	  ( ( ! f_less( k , n->key ) ) && ( n->stamp < s ) ) );
 }

/*-------------------------------- THE QUEUE -------------------------------*/

template< class K , class T , class Less >
LockFreePriorityQueue< K , T , Less >::LockFreePriorityQueue( unsigned offset ,
							    std::uint64_t seed ,
							    Less less )
    : f_b( offset ) , f_less( less ) , f_head( nullptr ) , f_tail( nullptr ) ,
      f_seed( seed )
{
 ADS_PRE( offset >= 1 );
 f_tail = makeLink( 1 );
 try {
  f_head = makeLink( kLevels );
  }
 catch( ... ) {
  destroyLink( f_tail );
  throw;
  }
 for( int i = 0 ; i < kLevels ; ++i )
  f_head->next[ i ].store( word( f_tail ) , std::memory_order_relaxed );
 }

template< class K , class T , class Less >
LockFreePriorityQueue< K , T , Less >::~LockFreePriorityQueue()
{
 release();
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::release( void )
{
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ; ) {
  Record * q = r->next;
  for( Link * l : r->limbo )
   destroyAll( l );
  delete r;
  r = q;
  }
 // the nodes from the first of the head on: those before it were cut off,
 // and freed above with their limbo
 Link * p = ptr( f_head->next[ 0 ].load( std::memory_order_acquire ) );
 while( p != f_tail ) {
  Link * q = ptr( p->next[ 0 ].load( std::memory_order_relaxed ) );
  destroy( static_cast< Node * >( p ) );
  p = q;
  }
 destroyLink( f_head );
 destroyLink( f_tail );
 }

template< class K , class T , class Less >
auto LockFreePriorityQueue< K , T , Less >::handle( void ) -> Handle
{
 // the seed of a handle is a counter mixed as by splitmix64 [see
 // MultiQueue::handle()]
 std::uint64_t z =
   f_seed.fetch_add( 0x9E3779B97F4A7C15ULL , std::memory_order_relaxed );
 z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
 z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
 z ^= z >> 31;
 // a record given back by a handle, else a new one pushed on the list
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
      r = r->next )
  if( ( ! r->used.load( std::memory_order_relaxed ) ) &&
      ( ! r->used.exchange( true , std::memory_order_acquire ) ) )
   return( Handle( *this , r , z ) );
 const std::uint64_t id = f_ids.fetch_add( 1 , std::memory_order_relaxed );
 ADS_PRE( id < ( std::uint64_t( 1 ) << 20 ) );
 Record * r = new Record;
 r->id = id;
 r->next = f_records.load( std::memory_order_relaxed );
 while( ! f_records.compare_exchange_weak(
   r->next , r , std::memory_order_release , std::memory_order_relaxed ) ) {
  }
 return( Handle( *this , r , z ) );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::locate( const K & k ,
						    std::uint64_t s ,
						  Link ** preds ,
						  Link ** succs ) const
{
 // the walk of Lindén and Jonsson: at the upper levels a node whose
 // successor is deleted is passed, at level 0 a deleted node as well
 Link * p = f_head;
 for( int i = kLevels - 1 ; i >= 0 ; --i ) {
  Word w = p->next[ i ].load( kWalk );
  Link * c = ptr( w );
  while( ( ( c != f_tail ) && before( c , k , s ) ) || marked( c ) ||
	 ( ( i == 0 ) && ( w & kMark ) ) ) {
   p = c;
   w = p->next[ i ].load( kWalk );
   c = ptr( w );
   }
  preds[ i ] = p;
  succs[ i ] = c;
  }
 }

template< class K , class T , class Less >
bool LockFreePriorityQueue< K , T , Less >::behind( const Link * c ,
						  const Link * n ) const
{
 // c is the successor at an upper level of the node n, already linked at
 // level 0; c is before n only if it is deleted, and then either its
 // successor is deleted as well (the mark), or c is the last deleted node
 // and its successor f the first node not deleted, which is n or a node
 // before n, of a smaller key; a node c after n never looks so but when n
 // is deleted, and then a false answer only stops the tower of n
 if( c == f_tail )
  return( false );
 const Word w = c->next[ 0 ].load( kWalk );
 if( w & kMark )
  return( true );
 const Link * f = ptr( w );
 const Node * m = static_cast< const Node * >( n );
 return( ( f == n ) || ( ( f != f_tail ) && before( f , m->key ,
						    m->stamp ) ) );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::restructure( Counts & c )
{
 // from the top down, each upper pointer of the head whose node has its
 // successor deleted moves to the first node of its level whose successor
 // is not, found from the last node passed at the level above
 Link * p = f_head;
 for( int i = kLevels - 1 ; i > 0 ; ) {
  Word h = f_head->next[ i ].load( kWalk );
  Link * x = ptr( p->next[ i ].load( kWalk ) );
  if( ! marked( ptr( h ) ) ) {
   --i;
   continue;
   }
  while( marked( x ) ) {
   p = x;
   x = ptr( p->next[ i ].load( kWalk ) );
   }
  ++c.rmw;
  if( f_head->next[ i ].compare_exchange_strong( h , word( x ) ) )
   --i;
  else
   ++c.failed;
  }
 }

template< class K , class T , class Less >
std::size_t LockFreePriorityQueue< K , T , Less >::size( void ) const
{
 std::size_t n = 0;
 Word w = f_head->next[ 0 ].load( std::memory_order_acquire );
 for( Link * p = ptr( w ) ; p != f_tail ; ) {
  if( ! ( w & kMark ) )
   ++n;
  w = p->next[ 0 ].load( std::memory_order_acquire );
  p = ptr( w );
  }
 return( n );
 }

template< class K , class T , class Less >
bool LockFreePriorityQueue< K , T , Less >::empty( void ) const
{
 return( size() == 0 );
 }

template< class K , class T , class Less >
std::size_t LockFreePriorityQueue< K , T , Less >::prefix( void ) const
{
 std::size_t n = 0;
 for( Word w = f_head->next[ 0 ].load( std::memory_order_acquire ) ;
      ( w & kMark ) && ( ptr( w ) != f_tail ) ;
      w = ptr( w )->next[ 0 ].load( std::memory_order_acquire ) )
  ++n;
 return( n );
 }

template< class K , class T , class Less >
bool LockFreePriorityQueue< K , T , Less >::consistent( void ) const
{
 // level 0: the marked words first, no cut in progress, no node being
 // inserted, the nodes not deleted increasing by ( key , stamp )
 std::vector< const Link * > order;
 Word w = f_head->next[ 0 ].load( std::memory_order_acquire );
 if( w & kCut )
  return( false );
 bool live = false;
 const Node * last = nullptr;
 for( const Link * p = ptr( w ) ; p != f_tail ; ) {
  if( ( p->height < 1 ) || ( p->height > kLevels ) ||
      p->inserting.load( std::memory_order_acquire ) )
   return( false );
  if( ! ( w & kMark ) ) {
   live = true;
   const Node * n = static_cast< const Node * >( p );
   if( last && ! before( last , n->key , n->stamp ) )
    return( false );
   last = n;
   }
  else if( live )
   return( false );  // a deleted node after a live one
  order.push_back( p );
  w = p->next[ 0 ].load( std::memory_order_acquire );
  if( w & kCut )
   return( false );
  p = ptr( w );
  }
 // each upper level: a subsequence of level 0 from the first node of the
 // head, of nodes high enough, up to the tail
 for( int i = 1 ; i < kLevels ; ++i ) {
  std::size_t j = 0;
  for( const Link * p =
	 ptr( f_head->next[ i ].load( std::memory_order_acquire ) ) ;
       p != f_tail ;
       p = ptr( p->next[ i ].load( std::memory_order_acquire ) ) ) {
   while( ( j < order.size() ) && ( order[ j ] != p ) )
    ++j;
   if( ( j == order.size() ) || ( p->height <= i ) )
    return( false );
   ++j;
   }
  }
 return( true );
 }

/*------------------------------- THE HANDLES ------------------------------*/

template< class K , class T , class Less >
LockFreePriorityQueue< K , T , Less >::Handle::Handle( Handle && o ) noexcept
    : f_q( o.f_q ) , f_r( o.f_r ) , f_x( o.f_x ) , f_c( o.f_c )
{
 o.f_r = nullptr;
 }

template< class K , class T , class Less >
LockFreePriorityQueue< K , T , Less >::Handle::~Handle()
{
 if( f_r )
  f_r->used.store( false , std::memory_order_release );
 }

template< class K , class T , class Less >
std::uint64_t LockFreePriorityQueue< K , T , Less >::Handle::next( void )
{
 std::uint64_t z = ( f_x += 0x9E3779B97F4A7C15ULL );
 z = ( z ^ ( z >> 30 ) ) * 0xBF58476D1CE4E5B9ULL;
 z = ( z ^ ( z >> 27 ) ) * 0x94D049BB133111EBULL;
 return( z ^ ( z >> 31 ) );
 }

template< class K , class T , class Less >
int LockFreePriorityQueue< K , T , Less >::Handle::height( void )
{
 // the trailing zeros of a random word: h with probability 2^-h, and the
 // bit L - 1 set so that h is at most L
 return( 1 + std::countr_zero( next() |
				( std::uint64_t( 1 ) << ( kLevels - 1 ) ) ) );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::Handle::enter( void )
{
 // the announce, by a sequentially consistent exchange, before any read
 // of the list. It takes the place of a relaxed store followed by a full
 // fence, which the sanitizer of threads does not model, with the same
 // guarantee for the reclamation [see Lemma 16 of the theory page]: the
 // reads of the words are sequentially consistent too (kWalk), so that a
 // read that sees a word older than the last compare-and-swap of a cut is
 // coherence-ordered before it and thus precedes it in the total order S
 // of the sequentially consistent operations, and the exchange, sequenced
 // before the read, precedes it in S as well, as the fence did; a scan
 // that follows the cut in S then reads the announce or a later value of
 // the record. On x86-64 the exchange is one instruction where the store
 // and the fence were two, and the reads cost what they did.
 const std::uint64_t e = f_q->f_epoch.load( std::memory_order_seq_cst );
 f_r->local.exchange( 2 * e + 1 , std::memory_order_seq_cst );
 for( int b = 0 ; b < 3 ; ++b )
  if( f_r->limbo[ b ] && ( f_r->epoch[ b ] + 2 <= e ) ) {
   destroyAll( f_r->limbo[ b ] );
   f_r->limbo[ b ] = nullptr;
   }
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::Handle::leave( void )
{
 f_r->local.store( 0 , std::memory_order_release );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::Handle::retire( Node * p )
{
 const std::uint64_t e = f_q->f_epoch.load( std::memory_order_seq_cst );
 const int b = int( e % 3 );
 if( f_r->epoch[ b ] != e ) {
  // the limbo of the same residue holds nodes of the epoch e - 3 or older
  ADS_INVARIANT( ( ! f_r->limbo[ b ] ) || ( f_r->epoch[ b ] + 2 <= e ) );
  destroyAll( f_r->limbo[ b ] );
  f_r->limbo[ b ] = nullptr;
  f_r->epoch[ b ] = e;
  }
 p->later = f_r->limbo[ b ];
 f_r->limbo[ b ] = p;
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::Handle::advance( void )
{
 std::uint64_t e = f_q->f_epoch.load( std::memory_order_seq_cst );
 for( Record * r = f_q->f_records.load( std::memory_order_acquire ) ; r ;
      r = r->next ) {
  const std::uint64_t v = r->local.load( std::memory_order_seq_cst );
  if( ( v & 1 ) && ( v != 2 * e + 1 ) )
   return;  // a thread still in an older epoch
  }
 f_q->f_epoch.compare_exchange_strong( e , e + 1 , std::memory_order_seq_cst );
 }

template< class K , class T , class Less >
void LockFreePriorityQueue< K , T , Less >::Handle::insert( K k , T x )
{
 const int h = height();
 const std::uint64_t s = ( f_r->count++ << 20 ) | f_r->id;
 Node * n = make( h , k , s , std::move( x ) );
 Link * P[ kLevels ];
 Link * S[ kLevels ];
 enter();
 for( ; ; ) {
  f_q->locate( k , s , P , S );
  n->next[ 0 ].store( word( S[ 0 ] ) , std::memory_order_relaxed );
  Word e = word( S[ 0 ] );
  ++f_c.rmw;
  if( P[ 0 ]->next[ 0 ].compare_exchange_strong( e , word( n ) ) )
   break;  // the linearization point
  ++f_c.failed;
  }
 for( int i = 1 ; i < h ; ) {
  if( marked( n ) || f_q->behind( S[ i ] , n ) )
   break;  // n deleted, or S[ i ] before n
  n->next[ i ].store( word( S[ i ] ) , std::memory_order_relaxed );
  Word e = word( S[ i ] );
  ++f_c.rmw;
  if( P[ i ]->next[ i ].compare_exchange_strong( e , word( n ) ) )
   ++i;
  else {
   ++f_c.failed;
   f_q->locate( k , s , P , S );
   if( S[ 0 ] != n )
    break;  // n deleted
   }
  }
 n->inserting.store( false , std::memory_order_release );
 leave();
 }

template< class K , class T , class Less >
auto LockFreePriorityQueue< K , T , Less >::Handle::tryDeleteMin( void )
  -> std::optional< Entry >
{
 enter();
 Link * const head = f_q->f_head;
 const Word obs = head->next[ 0 ].load( kWalk );
 Link * x = head;
 Link * nh = nullptr;
 unsigned o = 0;
 for( ; ; ) {
  ++o;
  Word w = x->next[ 0 ].load( kWalk );
  if( ptr( w ) == f_q->f_tail ) {  // empty, linearized at this read
   f_c.walked += o;
   leave();
   return( std::nullopt );
   }
  if( ( ! nh ) && x->inserting.load( std::memory_order_acquire ) )
   nh = x;  // the head stops before an insertion
  if( ! ( w & kMark ) ) {
   ++f_c.rmw;
   w = x->next[ 0 ].fetch_or( kMark );  // the linearization point
   if( w & kMark )
    ++f_c.failed;
   }
  x = ptr( w );
  if( ! ( w & kMark ) )
   break;
  }
 ADS_INVARIANT( x != f_q->f_tail );
 f_c.walked += o;
 Node * y = static_cast< Node * >( x );
 std::optional< Entry > r( Entry{ y->key , std::move( y->item ) } );
 if( ! nh )
  nh = x;
 // the cut: one at a time, by the bit kCut of the head, and only from the
 // head that this DeleteMin read, already marked
 if( ( o > f_q->f_b ) && ( obs & kMark ) && ( ! ( obs & kCut ) ) &&
     ( ptr( obs ) != nh ) &&
     ( head->next[ 0 ].load( std::memory_order_relaxed ) == obs ) ) {
  Word e = obs;
  ++f_c.rmw;
  if( head->next[ 0 ].compare_exchange_strong( e ,
					       word( nh ) | kMark | kCut ) ) {
   ++f_c.cuts;
   f_q->restructure( f_c );
   ++f_c.rmw;
   head->next[ 0 ].fetch_and( ~kCut , std::memory_order_release );
   for( Link * p = ptr( obs ) ; p != nh ; ) {
    Link * q = ptr( p->next[ 0 ].load( std::memory_order_relaxed ) );
    retire( static_cast< Node * >( p ) );
    p = q;
    }
   advance();
   }
  else
   ++f_c.failed;
  }
 leave();
 return( r );
 }

template< class K , class T , class Less >
auto LockFreePriorityQueue< K , T , Less >::Handle::tryMinKey( void )
  -> std::optional< K >
{
 enter();
 std::optional< K > r;
 for( Link * x = f_q->f_head ; ; ) {
  const Word w = x->next[ 0 ].load( kWalk );
  if( ptr( w ) == f_q->f_tail )
   break;
  if( ! ( w & kMark ) ) {  // the first node not deleted
   r.emplace( static_cast< Node * >( ptr( w ) )->key );
   break;
   }
  x = ptr( w );
  }
 leave();
 return( r );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File LockFreePriorityQueue.tpp -------------------*/
/*--------------------------------------------------------------------------*/
