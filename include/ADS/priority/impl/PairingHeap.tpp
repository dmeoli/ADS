/*--------------------------------------------------------------------------*/
/*-------------------------- File PairingHeap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PairingHeap [see PairingHeap.h], with their contracts.
 * The hot ones are inline, so that the compiler expands them in the loop
 * of the caller.
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

/*--------------------------------------------------------------------------*/
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , bool Aux , class A >
PairingHeap< T , P , Aux , A >::PairingHeap( PairingHeap && other ) noexcept
    : f_root( std::exchange( other.f_root , nullptr ) ) ,
      f_min( std::exchange( other.f_min , nullptr ) ) ,
      f_n( std::exchange( other.f_n , 0 ) ) ,
      f_pool( std::move( other.f_pool ) )
{ }

template< class T , std::totally_ordered P , bool Aux , class A >
PairingHeap< T , P , Aux , A >::~PairingHeap()
{
 if constexpr( ! std::is_trivially_destructible_v< Node > ) {
  std::vector< Node * > S;
  if( f_root )
   S.push_back( f_root );
  while( ! S.empty() ) {  // each node, then its children
   Node * x = S.back();   // and its right sibling
   S.pop_back();
   if( x->child )
    S.push_back( x->child );
   if( x->next )
    S.push_back( x->next );
   f_pool.free( x );
   }
  }
 }

template< class T , std::totally_ordered P , bool Aux , class A >
const T & PairingHeap< T , P , Aux , A >::min( void ) const
{
 ADS_PRE( ! empty() );
 return( f_min->value );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
const P & PairingHeap< T , P , Aux , A >::minPriority( void ) const
{
 ADS_PRE( ! empty() );
 return( f_min->priority );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
inline typename PairingHeap< T , P , Aux , A >::Handle
PairingHeap< T , P , Aux , A >::insert( const T & x , const P & p )
{
 Node * z = f_pool.make( p , x );
 attach( z );
 ++f_n;
 ADS_POST( ! empty() && ! ( p < f_min->priority ) );
 ADS_INVARIANT( isHeap() );
 return( z );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
inline T PairingHeap< T , P , Aux , A >::deleteMin( void )
{
 ADS_PRE( ! empty() );
 Node * z = f_min;
 const P q = z->priority;
 unlinkMin();
 --f_n;
 T v = std::move( z->value );
 f_pool.free( z );
 ADS_POST( empty() || ! ( f_min->priority < q ) );
 ADS_INVARIANT( isHeap() );
 return( v );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
inline void PairingHeap< T , P , Aux , A >::decrease( Handle x , const P & p )
{
 ADS_PRE( ( x != nullptr ) && ! ( x->priority < p ) );
 x->priority = p;
 if( x != f_root ) {
  Node * y = x->prev;
  if( ( y->child != x ) || ( p < y->priority ) ) {  // not where it is
   cut( x );
   if constexpr( Aux ) {
    x->next = f_root->next;
    if( x->next )
     x->next->prev = x;
    x->prev = f_root;
    f_root->next = x;
    }
   else
    f_root = link( f_root , x );
   }
  }
 if constexpr( Aux ) {
  if( p < f_min->priority )
   f_min = x;
  }
 else
  f_min = f_root;
 ADS_POST( x->priority == p );
 ADS_INVARIANT( isHeap() );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
T PairingHeap< T , P , Aux , A >::erase( Handle x )
{
 ADS_PRE( ( x != nullptr ) && ! empty() );
 detach( x );
 --f_n;
 T v = std::move( x->value );
 f_pool.free( x );
 ADS_INVARIANT( isHeap() );
 return( v );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
void PairingHeap< T , P , Aux , A >::increase( Handle x , const P & p )
{
 ADS_PRE( ( x != nullptr ) && ! ( p < x->priority ) );
 detach( x );
 x->priority = p;
 attach( x );
 ADS_POST( x->priority == p );
 ADS_INVARIANT( isHeap() );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
void PairingHeap< T , P , Aux , A >::update( Handle x , const P & p )
{
 ADS_PRE( x != nullptr );
 if( p < x->priority )
  decrease( x , p );
 else
  increase( x , p );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE STRUCTURE -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , bool Aux , class A >
inline void PairingHeap< T , P , Aux , A >::attach( Node * z )
{
 if( ! f_root )
  f_root = f_min = z;
 else if constexpr( Aux ) {  // after the root, in the list
  z->next = f_root->next;
  if( z->next )
   z->next->prev = z;
  z->prev = f_root;
  f_root->next = z;
  if( z->priority < f_min->priority )
   f_min = z;
  }
 else
  f_root = f_min = link( f_root , z );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
inline void PairingHeap< T , P , Aux , A >::unlinkMin( void )
{
 Node * z = f_min;
 if constexpr( Aux ) {  // the root, the children of z
  if( z != f_root )     // and the list joined
   cut( z );
  Node * l = f_root->next;
  if( l ) {
   f_root->next = nullptr;
   l->prev = nullptr;
   l = multiPass( l );
   }
  Node * r = z != f_root ? f_root : nullptr;
  for( Node * t : { twoPass( z->child ) , l } )
   if( t )
    r = r ? link( r , t ) : t;
  if( r )
   r->prev = r->next = nullptr;
  f_root = f_min = r;
  }
 else
  f_root = f_min = twoPass( z->child );
 z->child = z->next = z->prev = nullptr;
 }

template< class T , std::totally_ordered P , bool Aux , class A >
void PairingHeap< T , P , Aux , A >::detach( Node * x )
{
 if( x == f_min ) {
  unlinkMin();
  return;
  }
 Node * t = twoPass( x->child );  // x is not the least: neither
 x->child = nullptr;              // is t, nor the least changes
 if constexpr( Aux ) {
  if( x == f_root ) {   // the list goes on from t, or
   Node * l = x->next;  // from its first tree, which
   if( t ) {            // exists since the least is
    t->next = l;        // in it
    if( l )
     l->prev = t;
    f_root = t;
    }
   else {
    l->prev = nullptr;
    f_root = l;
    }
   }
  else {
   cut( x );
   if( t ) {  // after the root, in the list
    t->next = f_root->next;
    if( t->next )
     t->next->prev = t;
    t->prev = f_root;
    f_root->next = t;
    }
   }
  }
 else {
  cut( x );  // x is not the root
  if( t )
   f_root = f_min = link( f_root , t );
  }
 x->next = x->prev = nullptr;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE TREES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class N >
inline N * detail::PairingTrees::link( N * a , N * b )
{
 if( b->priority < a->priority )
  std::swap( a , b );
 b->next = a->child;  // b the leftmost child of a
 if( b->next )
  b->next->prev = b;
 b->prev = a;
 a->child = b;
 return( a );
 }

template< class N >
inline N * detail::PairingTrees::twoPass( N * x )
{
 if( ! x )
  return( nullptr );
 N * acc = nullptr;  // the pairs, the last first
 while( x ) {
  N * a = x;
  N * b = a->next;
  if( ! b ) {
   a->next = acc;
   acc = a;
   break;
   }
  x = b->next;
  N * t = link( a , b );
  t->next = acc;
  acc = t;
  }
 N * r = acc;  // from the right
 for( acc = acc->next ; acc ; ) {
  N * n = acc->next;
  r = link( r , acc );
  acc = n;
  }
 r->next = r->prev = nullptr;
 return( r );
 }

template< class N >
N * detail::PairingTrees::multiPass( N * x )
{
 N * tail = x;
 while( tail->next )
  tail = tail->next;
 while( x != tail ) {  // a queue: two from the front,
  N * a = x;           // their link to the back
  N * b = a->next;
  N * n = b->next;
  N * t = link( a , b );
  t->next = nullptr;
  if( b == tail )
   x = tail = t;
  else {
   x = n;
   tail->next = t;
   tail = t;
   }
  }
 x->next = x->prev = nullptr;
 return( x );
 }

template< class N >
inline void detail::PairingTrees::cut( N * x )
{
 if( x->prev->child == x )
  x->prev->child = x->next;
 else
  x->prev->next = x->next;
 if( x->next )
  x->next->prev = x->prev;
 x->next = x->prev = nullptr;
 }

/*--------------------------------------------------------------------------*/
/*------------------------ THE INTRUSIVE PAIRING HEAP ----------------------*/
/*--------------------------------------------------------------------------*/

template< class N >
inline N * IntrusivePairingHeap< N >::min( void ) const
{
 ADS_PRE( ! empty() );
 return( f_root );
 }

template< class N >
inline void IntrusivePairingHeap< N >::insert( N * x )
{
 ADS_PRE( ( x != nullptr ) && ! x->child && ! x->next && ! x->prev );
 f_root = f_root ? detail::PairingTrees::link( f_root , x ) : x;
 }

template< class N >
inline void IntrusivePairingHeap< N >::erase( N * x )
{
 ADS_PRE( ( x != nullptr ) && ! empty() );
 N * t = detail::PairingTrees::twoPass( x->child );
 x->child = nullptr;
 if( x == f_root )
  f_root = t;
 else {
  detail::PairingTrees::cut( x );
  if( t )
   f_root = detail::PairingTrees::link( f_root , t );
  }
 x->next = x->prev = nullptr;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , bool Aux , class A >
bool PairingHeap< T , P , Aux , A >::tree( const Node * x , int & n )
{
 std::vector< const Node * > S{ x };
 while( ! S.empty() ) {
  const Node * y = S.back();
  S.pop_back();
  ++n;
  const Node * l = y;  // the children, left to right
  for( const Node * c = y->child ; c ; l = c , c = c->next ) {
   if( ( c->prev != l ) || ( c->priority < y->priority ) )
    return( false );
   S.push_back( c );
   }
  }
 return( true );
 }

template< class T , std::totally_ordered P , bool Aux , class A >
bool PairingHeap< T , P , Aux , A >::isHeap( void ) const
{
 if( ! f_root )
  return( ( f_n == 0 ) && ! f_min );
 if( f_n > 1024 )
  return( true );
 if( f_root->prev || ( ! Aux && f_root->next ) )
  return( false );
 int n = 0;
 bool least = false;
 for( const Node *r = f_root , *l = nullptr ; r ; l = r , r = r->next ) {
  if( ( l && ( r->prev != l ) ) || ( r->priority < f_min->priority ) )
   return( false );
  least |= ( r == f_min );
  int k = 0;  // the tree of the root
  if( ! tree( r , k ) )
   return( false );
  n += k;
  }
 return( least && ( n == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PairingHeap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
