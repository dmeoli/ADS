/*--------------------------------------------------------------------------*/
/*------------------ File FibonacciHeapPriorityQueue.tpp -------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FibonacciHeapPriorityQueue [see
 * FibonacciHeapPriorityQueue.h], with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
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

template< class T , std::totally_ordered P >
FibonacciHeapPriorityQueue< T , P >::FibonacciHeapPriorityQueue(
                               FibonacciHeapPriorityQueue && other ) noexcept
 : f_min( std::exchange( other.f_min , nullptr ) ) ,
   f_n( std::exchange( other.f_n , 0 ) ) ,
   f_pool( std::move( other.f_pool ) )
{
 }

template< class T , std::totally_ordered P >
FibonacciHeapPriorityQueue< T , P >::~FibonacciHeapPriorityQueue()
{
 if constexpr( ! std::is_trivially_destructible_v< Node > ) {
  std::vector< Node * > S;
  if( f_min )
   S.push_back( f_min );
  while( ! S.empty() ) {                    // each list, then its children
   Node * x = S.back();
   S.pop_back();
   Node * y = x;
   do {
    Node * next = y->right;
    if( y->child )
     S.push_back( y->child );
    f_pool.free( y );
    y = next;
    } while( y != x );
   }
  }
 }

template< class T , std::totally_ordered P >
bool FibonacciHeapPriorityQueue< T , P >::isEmpty( void ) const
{
 return( f_min == nullptr );
 }

template< class T , std::totally_ordered P >
int FibonacciHeapPriorityQueue< T , P >::size( void ) const
{
 return( f_n );
 }

template< class T , std::totally_ordered P >
const T & FibonacciHeapPriorityQueue< T , P >::min( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_min->value );
 }

template< class T , std::totally_ordered P >
const P & FibonacciHeapPriorityQueue< T , P >::minPriority( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_min->priority );
 }

template< class T , std::totally_ordered P >
typename FibonacciHeapPriorityQueue< T , P >::Handle
FibonacciHeapPriorityQueue< T , P >::insert( const T & x , const P & p )
{
 Node * z = f_pool.make( p , x );
 add_root( z );
 ++f_n;
 ADS_POST( ! isEmpty() && !( p < f_min->priority ) );
 ADS_INVARIANT( isHeap() );
 return( z );
 }

template< class T , std::totally_ordered P >
T FibonacciHeapPriorityQueue< T , P >::deleteMin( void )
{
 ADS_PRE( ! isEmpty() );
 Node * z = f_min;
 const P q = z->priority;
 if( Node * c = z->child ) {                // the children join the roots,
  Node * r = z->right;                      // their parents and marks left
  Node * l = c->left;                       // to consolidate()
  z->right = c;
  c->left = z;
  l->right = r;
  r->left = l;
  }
 if( z->right == z )
  f_min = nullptr;
 else {
  Node * w = z->right;
  unlink( z );
  consolidate( w );
  }
 --f_n;
 T v = std::move( z->value );
 f_pool.free( z );
 ADS_POST( isEmpty() || !( f_min->priority < q ) );
 ADS_INVARIANT( isHeap() );
 return( v );
 }

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::decrease( Handle x , const P & p )
{
 ADS_PRE( ( x != nullptr ) && !( x->priority < p ) );
 ADS_AUDIT( owns( x ) );
 x->priority = p;
 Node * y = x->parent;
 if( y && ( x->priority < y->priority ) ) {
  cut( x , y );
  for( Node * z = y ; z->parent ; ) {       // the cascading cut
   if( ! z->mark ) {
    z->mark = true;
    break;
    }
   Node * w = z->parent;
   cut( z , w );
   z = w;
   }
  }
 if( x->priority < f_min->priority )
  f_min = x;
 ADS_POST( x->priority == p );
 ADS_INVARIANT( isHeap() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE TREES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::unlink( Node * x )
{
 x->left->right = x->right;
 x->right->left = x->left;
 x->left = x->right = x;
 }

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::splice( Node * x , Node * y )
{
 x->right = y->right;
 x->left = y;
 y->right->left = x;
 y->right = x;
 }

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::remove( Node * x , Node * & head )
{
 head = ( x->right == x ) ? nullptr : x->right;
 unlink( x );
 }

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::add_root( Node * z )
{
 if( ! f_min )
  f_min = z;
 else {
  splice( z , f_min );
  if( z->priority < f_min->priority )
   f_min = z;
  }
 }

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::cut( Node * x , Node * y )
{
 if( y->child == x )
  remove( x , y->child );
 else
  unlink( x );
 --y->degree;
 x->parent = nullptr;
 x->mark = false;
 splice( x , f_min );
 }

template< class T , std::totally_ordered P >
void FibonacciHeapPriorityQueue< T , P >::consolidate( Node * w )
{
 Node * A[ 64 ] = {};                       // the trees by degree
 int top = 0;                               // no degree above it in A
 const Node * const first = w;
 do {                                       // the next root is read before
  Node * y = w;                             // w is linked, and the roots
  w = w->right;                             // not yet seen keep their links
  y->parent = nullptr;
  y->mark = false;
  int d = y->degree;
  for( Node * z ; ( z = A[ d ] ) ; ++d ) {
   A[ d ] = nullptr;
   if( z->priority < y->priority )
    std::swap( y , z );
   z->parent = y;                           // z under y
   z->mark = false;
   if( Node * c = y->child )
    splice( z , c );
   else {
    z->left = z->right = z;
    y->child = z;
    }
   y->degree = d + 1;
   }
  A[ d ] = y;
  if( top < d )
   top = d;
  } while( w != first );
 Node * head = nullptr;                     // the roots in a circle again
 Node * last = nullptr;
 Node * m = nullptr;
 for( int d = 0 ; d <= top ; ++d )
  if( Node * y = A[ d ] ) {
   if( ! head )
    head = m = y;
   else {
    last->right = y;
    y->left = last;
    if( y->priority < m->priority )
     m = y;
    }
   last = y;
   }
 last->right = head;
 head->left = last;
 f_min = m;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P >
bool FibonacciHeapPriorityQueue< T , P >::owns( const Node * x ) const
{
 std::vector< const Node * > S;
 if( f_min )
  S.push_back( f_min );
 while( ! S.empty() ) {
  const Node * a = S.back();
  S.pop_back();
  const Node * y = a;
  do {
   if( y == x )
    return( true );
   if( y->child )
    S.push_back( y->child );
   y = y->right;
   } while( y != a );
  }
 return( false );
 }

template< class T , std::totally_ordered P >
bool
FibonacciHeapPriorityQueue< T , P >::tree( const Node * x ,
                                           const Node * parent ,
                                           int & n ) const
{
 const Node * y = x;
 int k = 0;
 do {
  if( ( y->right->left != y ) || ( y->parent != parent ) )
   return( false );
  if( parent && ( y->priority < parent->priority ) )
   return( false );
  if( ! parent && y->mark )
   return( false );
  int children = 0;
  if( y->child ) {
   const Node * c = y->child;
   do {
    ++children;
    c = c->right;
    } while( ( c != y->child ) && ( children <= f_n ) );
   if( ! tree( y->child , y , n ) )
    return( false );
   }
  if( children != y->degree )
   return( false );
  ++n;
  ++k;
  y = y->right;
  } while( ( y != x ) && ( k <= f_n ) );
 return( y == x );
 }

template< class T , std::totally_ordered P >
bool FibonacciHeapPriorityQueue< T , P >::isHeap( void ) const
{
 if( ! f_min )
  return( f_n == 0 );
 if( f_n > 1024 ) {                         // the roots only
  const Node * y = f_min;
  int k = 0;
  do {
   if( ( y->right->left != y ) || y->parent ||
       ( y->priority < f_min->priority ) )
    return( false );
   y = y->right;
   } while( ( y != f_min ) && ( ++k <= f_n ) );
  return( y == f_min );
  }
 int n = 0;
 if( ! tree( f_min , nullptr , n ) )
  return( false );
 const Node * y = f_min;
 do {
  if( y->priority < f_min->priority )
   return( false );
  y = y->right;
  } while( y != f_min );
 return( n == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------- End File FibonacciHeapPriorityQueue.tpp -----------------*/
/*--------------------------------------------------------------------------*/
