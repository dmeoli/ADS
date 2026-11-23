/*--------------------------------------------------------------------------*/
/*---------------------- File ConcurrentBTreeMap.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ConcurrentBTreeMap [see ConcurrentBTreeMap.h], with
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

#include "ADS/concurrent/Backoff.h"

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*-------------------------------- THE NODES -------------------------------*/

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
ConcurrentBTreeMap< K , V , Less , B , R >::Node::Node( bool l ) : leaf( l )
{
 this->reclaim = &ConcurrentBTreeMap::destroy;
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
void *
ConcurrentBTreeMap< K , V , Less , B , R >::destroy( Retirable * p ,
						 Retirable::Op op ) noexcept
{
 Node * n = static_cast< Node * >( p );
 if( op == Retirable::Op::address )
  return( n );
 if( n->leaf )
  delete static_cast< Leaf * >( n );
 else
  delete static_cast< Inner * >( n );
 return( nullptr );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
void ConcurrentBTreeMap< K , V , Less , B ,
			 R >::destroyTree( Node * n ) noexcept
{
 if( ! n->leaf ) {
  Inner * in = static_cast< Inner * >( n );
  for( int i = 0 , c = count( in ) ; i <= c ; ++i )
   destroyTree( in->child[ i ].load( std::memory_order_relaxed ) );
  }
 destroy( n , Retirable::Op::free );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
int ConcurrentBTreeMap< K , V , Less , B , R >::count( const Node * n )
{
 // a count read in a state being changed is still one of the node, and
 // within its capacity; the clamp keeps the reads in the arrays anyway
 return( std::clamp( ld( n->count ) , 0 , n->leaf ? kLeaf : kInner ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
template< bool Strict >
int ConcurrentBTreeMap< K , V , Less , B ,
			R >::place( const std::atomic< K > * a ,
						   int n , const K & k ) const
{
 int first = 0;
 while( n > 0 ) {
  const int half = n >> 1;
  const K x = ld( a[ first + half ] );
  const bool right = Strict ? ! f_less( k , x ) : f_less( x , k );
  first = right ? first + half + 1 : first;
  n = right ? n - half - 1 : half;
  }
 return( first );
 }

/*------------------------------ THE VERSIONS ------------------------------*/

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::stable( const Node * n ,
						     std::uint64_t & v )
{
 Backoff b;
 for( ; ; b.pause() ) {
  v = n->version.load( std::memory_order_acquire );
  if( ! ( v & kLocked ) )
   return( ! ( v & kObsolete ) );
  }
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::check( const Node * n ,
						    std::uint64_t v )
{
 // sequentially consistent, so that a check that passes precedes, in the
 // total order of the sequentially consistent operations, the lock of a
 // writer that unlinks a child read before it [see `EpochReclaim.h`];
 // the fields were read with acquire before it, so that a field written
 // under a lock taken after v makes the check fail
 return( n->version.load( std::memory_order_seq_cst ) == v );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::lock( Node * n ,
						   std::uint64_t v )
{
 return( n->version.compare_exchange_strong(
   v , v + kLocked , std::memory_order_seq_cst , std::memory_order_relaxed ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
void ConcurrentBTreeMap< K , V , Less , B , R >::unlock( Node * n )
{
 // the lock cleared and the count raised by one, in one addition
 n->version.fetch_add( kLocked , std::memory_order_release );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
void ConcurrentBTreeMap< K , V , Less , B , R >::unlockObsolete( Node * n )
{
 n->version.fetch_add( kLocked + kObsolete , std::memory_order_release );
 }

/*--------------------------------- THE MAP --------------------------------*/

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
ConcurrentBTreeMap< K , V , Less , B , R >::ConcurrentBTreeMap( Less less )
    : f_less( std::move( less ) ) , f_root( new Leaf )
{ }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
ConcurrentBTreeMap< K , V , Less , B , R >::~ConcurrentBTreeMap()
{
 destroyTree( f_root.load( std::memory_order_acquire ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
auto ConcurrentBTreeMap< K , V , Less , B , R >::handle( void ) -> Handle
{
 return( Handle( *this ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
template< class F >
void ConcurrentBTreeMap< K , V , Less , B , R >::forEach( F && f ) const
{
 // a walk in depth by a stack, the children pushed from the last one, so
 // that the leaves come out from the left
 std::vector< const Node * > S{ f_root.load( std::memory_order_acquire ) };
 while( ! S.empty() ) {
  const Node * n = S.back();
  S.pop_back();
  const int c = count( n );
  if( n->leaf ) {
   const Leaf * l = static_cast< const Leaf * >( n );
   for( int i = 0 ; i < c ; ++i )
    f( ld( l->key[ i ] ) , ld( l->value[ i ] ) );
   }
  else
   for( int i = c ; i >= 0 ; --i )
    S.push_back( ld( static_cast< const Inner * >( n )->child[ i ] ) );
  }
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
std::size_t ConcurrentBTreeMap< K , V , Less , B , R >::size( void ) const
{
 std::size_t n = 0;
 forEach( [ &n ]( const K & , const V & ) { ++n; } );
 return( n );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::empty( void ) const
{
 return( size() == 0 );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
int ConcurrentBTreeMap< K , V , Less , B , R >::height( void ) const
{
 int h = 1;
 for( const Node * n = f_root.load( std::memory_order_acquire ) ; ! n->leaf ;
      ++h )
  n = ld( static_cast< const Inner * >( n )->child[ 0 ] );
 return( h );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::consistent( void ) const
{
 int depth = -1;
 return( consistent( f_root.load( std::memory_order_acquire ) , nullptr ,
		      nullptr , 1 , depth ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::consistent( const Node * n ,
							 const K * lo ,
							 const K * hi , int d ,
							 int & depth ) const
{
 if( n->version.load( std::memory_order_acquire ) & ( kLocked | kObsolete ) )
  return( false );
 const int c = ld( n->count );
 if( ( c < 0 ) || ( c > ( n->leaf ? kLeaf : kInner ) ) )
  return( false );
 const std::atomic< K > * key = n->leaf
				  ? static_cast< const Leaf * >( n )->key
				  : static_cast< const Inner * >( n )->key;
 for( int i = 0 ; i < c ; ++i ) {
  const K k = ld( key[ i ] );
  if( ( lo && ! f_less( *lo , k ) ) || ( hi && f_less( *hi , k ) ) )
   return( false );  // out of ( lo , hi ]
  if( ( i > 0 ) && ! f_less( ld( key[ i - 1 ] ) , k ) )
   return( false );  // not increasing
  }
 if( n->leaf ) {
  if( depth < 0 )
   depth = d;
  return( depth == d );
  }
 const Inner * in = static_cast< const Inner * >( n );
 std::vector< K > ks( static_cast< std::size_t >( c ) );
 for( int i = 0 ; i < c ; ++i )
  ks[ std::size_t( i ) ] = ld( in->key[ i ] );
 for( int i = 0 ; i <= c ; ++i ) {
  const K * a = ( i == 0 ) ? lo : &ks[ std::size_t( i - 1 ) ];
  const K * b = ( i == c ) ? hi : &ks[ std::size_t( i ) ];
  const Node * ch = ld( in->child[ i ] );
  if( ( ! ch ) || ! consistent( ch , a , b , d + 1 , depth ) )
   return( false );
  }
 return( true );
 }

/*-------------------------------- THE HANDLE ------------------------------*/

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
ConcurrentBTreeMap< K , V , Less , B ,
		    R >::Handle::Handle( Handle && o ) noexcept
    : f_m( o.f_m ) , f_l( std::move( o.f_l ) ) , f_leaf( o.f_leaf ) ,
      f_inner( o.f_inner ) , f_read( std::move( o.f_read ) ) , f_c( o.f_c )
{
 o.f_leaf = nullptr;
 o.f_inner = nullptr;
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
ConcurrentBTreeMap< K , V , Less , B , R >::Handle::~Handle()
{
 delete f_leaf;
 delete f_inner;
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::Handle::insert( K k , V x )
{
 return( put< false >( k , x ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::Handle::assign( K k , V x )
{
 return( put< true >( k , x ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
template< bool Assign >
bool ConcurrentBTreeMap< K , V , Less , B , R >::Handle::put( const K & k ,
							  const V & x )
{
 ConcurrentBTreeMap & m = *f_m;
 typename Local::Guard g( f_l );
 for( ; ; ++f_c.restarts ) {
  Node * n = m.f_root.load( std::memory_order_seq_cst );
  std::uint64_t v = 0;
  if( ( ! stable( n , v ) ) ||
      ( n != m.f_root.load( std::memory_order_seq_cst ) ) )
   continue;
  Inner * p = nullptr;
  std::uint64_t vp = 0;
  int ip = 0;
  bool again = false;
  while( ! n->leaf ) {
   Inner * in = static_cast< Inner * >( n );
   const int c = count( in );
   if( c == kInner ) {  // full: split on the way down
    split( p , vp , ip , in , v );
    again = true;
    break;
    }
   const int i = m.template place< false >( in->key , c , k );
   Node * ch = ld( in->child[ i ] );
   std::uint64_t w = 0;
   if( ( ! check( in , v ) ) || ( ! stable( ch , w ) ) ||
       ( ! check( in , v ) ) ) {
    again = true;
    break;
    }
   p = in;
   vp = v;
   ip = i;
   n = ch;
   v = w;
   }
  if( again )
   continue;
  Leaf * l = static_cast< Leaf * >( n );
  const int c = count( l );
  const int i = m.template place< false >( l->key , c , k );
  const bool there = ( i < c ) && ( ! m.f_less( k , ld( l->key[ i ] ) ) );
  if constexpr( ! Assign )
   if( there ) {
    if( check( l , v ) )
     return( false );
    continue;
    }
  if( ( c == kLeaf ) && ( ! there ) ) {
   split( p , vp , ip , l , v );
   continue;
   }
  if( ! lock( l , v ) )
   continue;
  // the linearization point: c, i and there are those of the state locked
  if( there ) {
   st( l->value[ i ] , x );
   unlock( l );
   return( false );
   }
  for( int j = c ; j > i ; --j ) {
   st( l->key[ j ] , ld( l->key[ j - 1 ] ) );
   st( l->value[ j ] , ld( l->value[ j - 1 ] ) );
   }
  st( l->key[ i ] , k );
  st( l->value[ i ] , x );
  st( l->count , c + 1 );
  unlock( l );
  return( true );
  }
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::Handle::split( Inner * p ,
							    std::uint64_t vp ,
							    int i , Node * n ,
							    std::uint64_t v )
{
 ConcurrentBTreeMap & m = *f_m;
 // the new nodes allocated before any lock is taken, so that an
 // exception of the allocation leaves no node locked
 if( n->leaf ? ( ! f_leaf ) : ( ! f_inner ) ) {
  if( n->leaf )
   f_leaf = new Leaf;
  else
   f_inner = new Inner;
  }
 Inner * root = p ? nullptr : new Inner;
 auto fail = [ root ] {
  delete root;
  return( false );
  };
 if( p && ( ! lock( p , vp ) ) )
  return( fail() );
 if( ! lock( n , v ) ) {
  if( p )
   unlock( p );
  return( fail() );
  }
 if( ( ! p ) && ( n != m.f_root.load( std::memory_order_seq_cst ) ) ) {
  unlock( n );
  return( fail() );
  }
 K s{};
 Node * r = nullptr;
 if( n->leaf ) {
  // the left half keeps the first ceil( c / 2 ) pairs
  Leaf * a = static_cast< Leaf * >( n );
  Leaf * b = f_leaf;
  f_leaf = nullptr;
  const int c = count( a ) , h = ( c + 1 ) / 2;
  for( int j = h ; j < c ; ++j ) {
   st( b->key[ j - h ] , ld( a->key[ j ] ) );
   st( b->value[ j - h ] , ld( a->value[ j ] ) );
   }
  st( b->count , c - h );
  st( a->count , h );
  s = ld( a->key[ h - 1 ] );
  r = b;
  }
 else {
  // the key in the middle goes up, the keys after it go right
  Inner * a = static_cast< Inner * >( n );
  Inner * b = f_inner;
  f_inner = nullptr;
  const int c = count( a ) , h = c / 2;
  s = ld( a->key[ h ] );
  for( int j = h + 1 ; j < c ; ++j )
   st( b->key[ j - h - 1 ] , ld( a->key[ j ] ) );
  for( int j = h + 1 ; j <= c ; ++j )
   st( b->child[ j - h - 1 ] , ld( a->child[ j ] ) );
  st( b->count , c - h - 1 );
  st( a->count , h );
  r = b;
  }
 if( p ) {
  // p has room: it was not full at vp, the version locked
  const int c = count( p );
  ADS_INVARIANT( c < kInner );
  for( int j = c ; j > i ; --j )
   st( p->key[ j ] , ld( p->key[ j - 1 ] ) );
  for( int j = c + 1 ; j > i + 1 ; --j )
   st( p->child[ j ] , ld( p->child[ j - 1 ] ) );
  st( p->key[ i ] , s );
  st( p->child[ i + 1 ] , r );
  st( p->count , c + 1 );
  }
 else {
  st( root->key[ 0 ] , s );
  st( root->child[ 0 ] , n );
  st( root->child[ 1 ] , r );
  st( root->count , 1 );
  m.f_root.store( static_cast< Node * >( root ) , std::memory_order_seq_cst );
  }
 unlock( n );
 if( p )
  unlock( p );
 ++f_c.splits;
 return( true );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
bool ConcurrentBTreeMap< K , V , Less , B , R >::Handle::erase( K k )
{
 ConcurrentBTreeMap & m = *f_m;
 typename Local::Guard g( f_l );
 for( ; ; ++f_c.restarts ) {
  Node * n = m.f_root.load( std::memory_order_seq_cst );
  std::uint64_t v = 0;
  if( ( ! stable( n , v ) ) ||
      ( n != m.f_root.load( std::memory_order_seq_cst ) ) )
   continue;
  if( ( ! n->leaf ) && ( count( n ) == 0 ) ) {
   // a root with one child gives way to it
   Inner * in = static_cast< Inner * >( n );
   if( lock( in , v ) ) {
    if( n == m.f_root.load( std::memory_order_seq_cst ) ) {
     m.f_root.store( ld( in->child[ 0 ] ) , std::memory_order_seq_cst );
     unlockObsolete( in );
     f_l.retire( in );
     }
    else
     unlock( in );
    }
   continue;
   }
  bool again = false;
  while( ! n->leaf ) {
   Inner * in = static_cast< Inner * >( n );
   const int c = count( in );
   const int i = m.template place< false >( in->key , c , k );
   Node * ch = ld( in->child[ i ] );
   std::uint64_t w = 0;
   if( ( ! check( in , v ) ) || ( ! stable( ch , w ) ) ||
       ( ! check( in , v ) ) ) {
    again = true;
    break;
    }
   if( ( c > 0 ) && ( count( ch ) < ( ch->leaf ? kLeaf : kInner ) / 4 ) ) {
    // the child is short: merged with, or evened with, a sibling, the
    // version of the sibling read before any lock is taken
    const int j = ( i < c ) ? i : i - 1;
    Node * a = ld( in->child[ j ] );
    Node * b = ld( in->child[ j + 1 ] );
    std::uint64_t va = w , vb = w , vs = 0;
    again = true;
    if( ( ! check( in , v ) ) || ( ! stable( a == ch ? b : a , vs ) ) ||
	( ! check( in , v ) ) )
     break;
    ( a == ch ? vb : va ) = vs;
    if( ! lock( in , v ) )
     break;
    if( ! lock( a , va ) ) {
     unlock( in );
     break;
     }
    if( ! lock( b , vb ) ) {
     unlock( a );
     unlock( in );
     break;
     }
    rebalance( in , j , a , b );
    break;
    }
   n = ch;
   v = w;
   }
  if( again )
   continue;
  Leaf * l = static_cast< Leaf * >( n );
  const int c = count( l );
  const int i = m.template place< false >( l->key , c , k );
  if( ( i >= c ) || m.f_less( k , ld( l->key[ i ] ) ) ) {
   if( check( l , v ) )
    return( false );
   continue;
   }
  if( ! lock( l , v ) )
   continue;
  // the linearization point: c and i are those of the state locked
  for( int j = i + 1 ; j < c ; ++j ) {
   st( l->key[ j - 1 ] , ld( l->key[ j ] ) );
   st( l->value[ j - 1 ] , ld( l->value[ j ] ) );
   }
  st( l->count , c - 1 );
  unlock( l );
  return( true );
  }
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
void ConcurrentBTreeMap< K , V , Less , B , R >::Handle::rebalance( Inner * n ,
								int j ,
								Node * a ,
								Node * b )
{
 ConcurrentBTreeMap & m = *f_m;
 const int cn = count( n ) , ca = count( a ) , cb = count( b );
 const bool leaf = a->leaf;
 const int cap = leaf ? kLeaf : kInner;
 // the keys of the two, with the separator between them if internal
 const int t = ca + cb + ( leaf ? 0 : 1 );
 if( 4 * t <= 3 * cap ) {
  // b moved into a, and dropped from n with its separator
  if( leaf ) {
   Leaf * x = static_cast< Leaf * >( a );
   Leaf * y = static_cast< Leaf * >( b );
   for( int q = 0 ; q < cb ; ++q ) {
    st( x->key[ ca + q ] , ld( y->key[ q ] ) );
    st( x->value[ ca + q ] , ld( y->value[ q ] ) );
    }
   }
  else {
   Inner * x = static_cast< Inner * >( a );
   Inner * y = static_cast< Inner * >( b );
   st( x->key[ ca ] , ld( n->key[ j ] ) );
   for( int q = 0 ; q < cb ; ++q )
    st( x->key[ ca + 1 + q ] , ld( y->key[ q ] ) );
   for( int q = 0 ; q <= cb ; ++q )
    st( x->child[ ca + 1 + q ] , ld( y->child[ q ] ) );
   }
  st( a->count , t );
  for( int q = j + 1 ; q < cn ; ++q )
   st( n->key[ q - 1 ] , ld( n->key[ q ] ) );
  for( int q = j + 2 ; q <= cn ; ++q )
   st( n->child[ q - 1 ] , ld( n->child[ q ] ) );
  st( n->count , cn - 1 );
  unlock( a );
  unlockObsolete( b );
  unlock( n );
  f_l.retire( b );
  ++f_c.merges;
  return;
  }
 // the keys (and the children) of the two in one sequence, then split
 // evenly: a gets the first h keys, and, if internal, the next one goes up
 K key[ 2 * 255 + 1 ];
 const int h = leaf ? t / 2 : ( t - 1 ) / 2;
 if( leaf ) {
  Leaf * x = static_cast< Leaf * >( a );
  Leaf * y = static_cast< Leaf * >( b );
  V value[ 2 * 255 ];
  for( int q = 0 ; q < ca ; ++q ) {
   key[ q ] = ld( x->key[ q ] );
   value[ q ] = ld( x->value[ q ] );
   }
  for( int q = 0 ; q < cb ; ++q ) {
   key[ ca + q ] = ld( y->key[ q ] );
   value[ ca + q ] = ld( y->value[ q ] );
   }
  for( int q = 0 ; q < t ; ++q )
   if( q < h ) {
    st( x->key[ q ] , key[ q ] );
    st( x->value[ q ] , value[ q ] );
    }
   else {
    st( y->key[ q - h ] , key[ q ] );
    st( y->value[ q - h ] , value[ q ] );
    }
  st( a->count , h );
  st( b->count , t - h );
  st( n->key[ j ] , key[ h - 1 ] );
  }
 else {
  Inner * x = static_cast< Inner * >( a );
  Inner * y = static_cast< Inner * >( b );
  Node * child[ 2 * 255 + 2 ];
  for( int q = 0 ; q < ca ; ++q )
   key[ q ] = ld( x->key[ q ] );
  key[ ca ] = ld( n->key[ j ] );
  for( int q = 0 ; q < cb ; ++q )
   key[ ca + 1 + q ] = ld( y->key[ q ] );
  for( int q = 0 ; q <= ca ; ++q )
   child[ q ] = ld( x->child[ q ] );
  for( int q = 0 ; q <= cb ; ++q )
   child[ ca + 1 + q ] = ld( y->child[ q ] );
  for( int q = 0 ; q < h ; ++q )
   st( x->key[ q ] , key[ q ] );
  for( int q = 0 ; q <= h ; ++q )
   st( x->child[ q ] , child[ q ] );
  for( int q = h + 1 ; q < t ; ++q )
   st( y->key[ q - h - 1 ] , key[ q ] );
  for( int q = h + 1 ; q <= t ; ++q )
   st( y->child[ q - h - 1 ] , child[ q ] );
  st( a->count , h );
  st( b->count , t - h - 1 );
  st( n->key[ j ] , key[ h ] );
  }
 unlock( a );
 unlock( b );
 unlock( n );
 ++f_c.shares;
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
template< bool Strict >
bool ConcurrentBTreeMap< K , V , Less , B , R >::Handle::leafOf(
  const K & x , Leaf *& l , std::uint64_t & w , K & f , bool & bounded )
{
 ConcurrentBTreeMap & m = *f_m;
 Node * n = m.f_root.load( std::memory_order_seq_cst );
 std::uint64_t v = 0;
 if( ( ! stable( n , v ) ) ||
     ( n != m.f_root.load( std::memory_order_seq_cst ) ) )
  return( false );
 bounded = false;
 while( ! n->leaf ) {
  Inner * in = static_cast< Inner * >( n );
  const int c = count( in );
  const int i = m.template place< Strict >( in->key , c , x );
  const K k = ( i < c ) ? ld( in->key[ i ] ) : K{};
  Node * ch = ld( in->child[ i ] );
  std::uint64_t u = 0;
  if( ( ! check( in , v ) ) || ( ! stable( ch , u ) ) || ( ! check( in ,
								    v ) ) )
   return( false );
  if( i < c ) {  // the range of the child ends at k
   f = k;
   bounded = true;
   }
  n = ch;
  v = u;
  }
 l = static_cast< Leaf * >( n );
 w = v;
 return( true );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
auto ConcurrentBTreeMap< K , V , Less , B , R >::Handle::find( K k )
  -> std::optional< V >
{
 ConcurrentBTreeMap & m = *f_m;
 typename Local::Guard g( f_l );
 for( ; ; ++f_c.restarts ) {
  Leaf * l = nullptr;
  std::uint64_t w = 0;
  K f{};
  bool bounded = false;
  if( ! leafOf< false >( k , l , w , f , bounded ) )
   continue;
  const int c = count( l );
  const int i = m.template place< false >( l->key , c , k );
  const bool there = ( i < c ) && ( ! m.f_less( k , ld( l->key[ i ] ) ) );
  const V x = there ? ld( l->value[ i ] ) : V{};
  if( ! check( l , w ) )
   continue;
  if( there )
   return( x );
  return( std::nullopt );
  }
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
template< bool Strict >
auto ConcurrentBTreeMap< K , V , Less , B , R >::Handle::bound( const K & k )
  -> std::optional< std::pair< K , V > >
{
 ConcurrentBTreeMap & m = *f_m;
 typename Local::Guard g( f_l );
 for( ; ; ++f_c.restarts ) {
  f_read.clear();
  K x = k;
  bool strict = Strict , again = false;
  std::optional< std::pair< K , V > > e;
  for( ; ; ) {
   Leaf * l = nullptr;
   std::uint64_t w = 0;
   K f{};
   bool bounded = false;
   if( ! ( strict ? leafOf< true >( x , l , w , f , bounded )
		  : leafOf< false >( x , l , w , f , bounded ) ) ) {
    again = true;
    break;
    }
   const int c = count( l );
   const int i = strict ? m.template place< true >( l->key , c , x )
			: m.template place< false >( l->key , c , x );
   if( i < c )
    e.emplace( ld( l->key[ i ] ) , ld( l->value[ i ] ) );
   if( ! check( l , w ) ) {
    again = true;
    break;
    }
   f_read.emplace_back( l , w );
   if( e || ( ! bounded ) )
    break;
   x = f;  // the next leaf holds the keys greater than f
   strict = true;
   }
  if( again )
   continue;
  // the linearization point: the first of these checks, at which all the
  // leaves read still hold the states read
  if( std::all_of( f_read.begin() , f_read.end() , []( const auto & r ) {
       return( check( r.first , r.second ) );
       } ) )
   return( e );
  }
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
auto ConcurrentBTreeMap< K , V , Less , B , R >::Handle::lowerBound( K k )
  -> std::optional< std::pair< K , V > >
{
 return( bound< false >( k ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
auto ConcurrentBTreeMap< K , V , Less , B , R >::Handle::upperBound( K k )
  -> std::optional< std::pair< K , V > >
{
 return( bound< true >( k ) );
 }

template< class K , class V , class Less , std::size_t B ,
	  reclamation_policy R >
template< class F >
std::size_t ConcurrentBTreeMap< K , V , Less , B , R >::Handle::scan( K from ,
								  F && f )
{
 ConcurrentBTreeMap & m = *f_m;
 std::pair< K , V > buf[ kLeaf ];
 std::size_t calls = 0;
 K x = from;
 bool strict = false;
 for( ; ; ) {
  K fence{};
  bool bounded = false;
  int got = 0;
  {
   // one leaf read as one state, inside an operation; f is called out
   // of it, so that it may use the handle
   typename Local::Guard g( f_l );
   for( ; ; ++f_c.restarts ) {
    Leaf * l = nullptr;
    std::uint64_t w = 0;
    if( ! ( strict ? leafOf< true >( x , l , w , fence , bounded )
		   : leafOf< false >( x , l , w , fence , bounded ) ) )
     continue;
    const int c = count( l );
    const int i = strict ? m.template place< true >( l->key , c , x )
			 : m.template place< false >( l->key , c , x );
    got = 0;
    for( int j = i ; j < c ; ++j )
     buf[ got++ ] = { ld( l->key[ j ] ) , ld( l->value[ j ] ) };
    if( check( l , w ) )
     break;
    }
   }
  for( int q = 0 ; q < got ; ++q ) {
   ++calls;
   if constexpr( std::is_same_v<
		   std::invoke_result_t< F & , const K & , const V & > ,
		   bool > ) {
    if( ! f( buf[ q ].first , buf[ q ].second ) )
     return( calls );
    }
   else
    f( buf[ q ].first , buf[ q ].second );
   }
  if( ! bounded )
   return( calls );
  x = fence;
  strict = true;
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File ConcurrentBTreeMap.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
