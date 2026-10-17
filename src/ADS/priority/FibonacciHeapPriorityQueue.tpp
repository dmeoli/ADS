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
FibonacciHeapPriorityQueue< T , P >::~FibonacciHeapPriorityQueue()
{
 std::vector< Node * > S;
 if( f_min )
  S.push_back( f_min );
 while( ! S.empty() ) {                     // each list, then its children
  Node * x = S.back();
  S.pop_back();
  Node * y = x;
  do {
   Node * next = y->right;
   if( y->child )
    S.push_back( y->child );
   delete y;
   y = next;
   } while( y != x );
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
 Node * z = new Node{ p , x };
 z->left = z->right = z;
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
 while( Node * c = z->child ) {             // the children become roots
  remove( c , z->child );
  c->parent = nullptr;
  c->mark = false;
  splice( c , f_min );
  }
 if( z->right == z )
  f_min = nullptr;
 else {
  f_min = z->right;
  unlink( z );
  consolidate();
  }
 --f_n;
 T v = std::move( z->value );
 delete z;
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
void FibonacciHeapPriorityQueue< T , P >::consolidate( void )
{
 std::vector< Node * > roots;
 Node * x = f_min;
 do {
  roots.push_back( x );
  x = x->right;
  } while( x != f_min );
 std::vector< Node * > A;
 for( Node * w : roots ) {
  unlink( w );
  Node * y = w;
  for( int d = y->degree ; ; d = y->degree ) {
   if( int( A.size() ) <= d )
    A.resize( d + 1 , nullptr );
   Node * z = A[ d ];
   if( ! z )
    break;
   if( z->priority < y->priority )
    std::swap( y , z );
   z->parent = y;                           // z under y
   z->mark = false;
   if( y->child )
    splice( z , y->child );
   else
    y->child = z;
   ++y->degree;
   A[ d ] = nullptr;
   }
  A[ y->degree ] = y;
  }
 f_min = nullptr;
 for( Node * y : A )
  if( y )
   add_root( y );
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
