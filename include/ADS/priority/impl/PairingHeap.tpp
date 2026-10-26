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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , bool Aux >
PairingHeap< T , P , Aux >::PairingHeap( PairingHeap && other ) noexcept
 : f_root( std::exchange( other.f_root , nullptr ) ) ,
   f_min( std::exchange( other.f_min , nullptr ) ) ,
   f_n( std::exchange( other.f_n , 0 ) ) ,
   f_pool( std::move( other.f_pool ) )
{
 }

template< class T , std::totally_ordered P , bool Aux >
PairingHeap< T , P , Aux >::~PairingHeap()
{
 if constexpr( ! std::is_trivially_destructible_v< Node > ) {
  std::vector< Node * > S;
  if( f_root )
   S.push_back( f_root );
  while( ! S.empty() ) {                    // each node, then its children
   Node * x = S.back();                     // and its right sibling
   S.pop_back();
   if( x->child ) S.push_back( x->child );
   if( x->next ) S.push_back( x->next );
   f_pool.free( x );
   }
  }
 }

template< class T , std::totally_ordered P , bool Aux >
const T & PairingHeap< T , P , Aux >::min( void ) const
{
 ADS_PRE( ! empty() );
 return( f_min->value );
 }

template< class T , std::totally_ordered P , bool Aux >
const P & PairingHeap< T , P , Aux >::minPriority( void ) const
{
 ADS_PRE( ! empty() );
 return( f_min->priority );
 }

template< class T , std::totally_ordered P , bool Aux >
inline typename PairingHeap< T , P , Aux >::Handle
PairingHeap< T , P , Aux >::insert( const T & x , const P & p )
{
 Node * z = f_pool.make( p , x );
 if( ! f_root )
  f_root = f_min = z;
 else if constexpr( Aux ) {                 // after the root, in the list
  z->next = f_root->next;
  if( z->next ) z->next->prev = z;
  z->prev = f_root;
  f_root->next = z;
  if( p < f_min->priority )
   f_min = z;
  }
 else
  f_root = f_min = link( f_root , z );
 ++f_n;
 ADS_POST( ! empty() && !( p < f_min->priority ) );
 ADS_INVARIANT( isHeap() );
 return( z );
 }

template< class T , std::totally_ordered P , bool Aux >
inline T PairingHeap< T , P , Aux >::deleteMin( void )
{
 ADS_PRE( ! empty() );
 Node * z = f_min;
 const P q = z->priority;
 if constexpr( Aux ) {                      // the root, the children of z
  if( z != f_root )                         // and the list joined
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
 --f_n;
 T v = std::move( z->value );
 f_pool.free( z );
 ADS_POST( empty() || !( f_min->priority < q ) );
 ADS_INVARIANT( isHeap() );
 return( v );
 }

template< class T , std::totally_ordered P , bool Aux >
inline void PairingHeap< T , P , Aux >::decrease( Handle x , const P & p )
{
 ADS_PRE( ( x != nullptr ) && !( x->priority < p ) );
 x->priority = p;
 if( x != f_root ) {
  Node * y = x->prev;
  if( ( y->child != x ) || ( p < y->priority ) ) {   // not where it is
   cut( x );
   if constexpr( Aux ) {
    x->next = f_root->next;
    if( x->next ) x->next->prev = x;
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

/*--------------------------------------------------------------------------*/
/*------------------------------ THE TREES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , bool Aux >
inline typename PairingHeap< T , P , Aux >::Node *
PairingHeap< T , P , Aux >::link( Node * a , Node * b )
{
 if( b->priority < a->priority )
  std::swap( a , b );
 b->next = a->child;                        // b the leftmost child of a
 if( b->next ) b->next->prev = b;
 b->prev = a;
 a->child = b;
 return( a );
 }

template< class T , std::totally_ordered P , bool Aux >
inline typename PairingHeap< T , P , Aux >::Node *
PairingHeap< T , P , Aux >::twoPass( Node * x )
{
 if( ! x )
  return( nullptr );
 Node * acc = nullptr;                      // the pairs, the last first
 while( x ) {
  Node * a = x;
  Node * b = a->next;
  if( ! b ) {
   a->next = acc;
   acc = a;
   break;
   }
  x = b->next;
  Node * t = link( a , b );
  t->next = acc;
  acc = t;
  }
 Node * r = acc;                            // from the right
 for( acc = acc->next ; acc ; ) {
  Node * n = acc->next;
  r = link( r , acc );
  acc = n;
  }
 r->next = r->prev = nullptr;
 return( r );
 }

template< class T , std::totally_ordered P , bool Aux >
typename PairingHeap< T , P , Aux >::Node *
PairingHeap< T , P , Aux >::multiPass( Node * x )
{
 Node * tail = x;
 while( tail->next )
  tail = tail->next;
 while( x != tail ) {                       // a queue: two from the front,
  Node * a = x;                             // their link to the back
  Node * b = a->next;
  Node * n = b->next;
  Node * t = link( a , b );
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

template< class T , std::totally_ordered P , bool Aux >
inline void PairingHeap< T , P , Aux >::cut( Node * x )
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
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , bool Aux >
bool PairingHeap< T , P , Aux >::tree( const Node * x , int & n )
{
 std::vector< const Node * > S{ x };
 while( ! S.empty() ) {
  const Node * y = S.back();
  S.pop_back();
  ++n;
  const Node * l = y;                       // the children, left to right
  for( const Node * c = y->child ; c ; l = c , c = c->next ) {
   if( ( c->prev != l ) || ( c->priority < y->priority ) )
    return( false );
   S.push_back( c );
   }
  }
 return( true );
 }

template< class T , std::totally_ordered P , bool Aux >
bool PairingHeap< T , P , Aux >::isHeap( void ) const
{
 if( ! f_root )
  return( ( f_n == 0 ) && ! f_min );
 if( f_n > 1024 )
  return( true );
 if( f_root->prev || ( ! Aux && f_root->next ) )
  return( false );
 int n = 0;
 bool least = false;
 for( const Node * r = f_root , * l = nullptr ; r ; l = r , r = r->next ) {
  if( ( l && ( r->prev != l ) ) || ( r->priority < f_min->priority ) )
   return( false );
  least |= ( r == f_min );
  int k = 0;                                // the tree of the root
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
