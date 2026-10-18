/*--------------------------------------------------------------------------*/
/*-------------------- File LinkedHeapPriorityQueue.tpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LinkedHeapPriorityQueue [see
 * LinkedHeapPriorityQueue.h], with their contracts.
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

template< class T , std::totally_ordered P , bool L >
bool LinkedHeapPriorityQueue< T , P , L >::isEmpty( void ) const
{
 return( f_n == 0 );
 }

template< class T , std::totally_ordered P , bool L >
int LinkedHeapPriorityQueue< T , P , L >::size( void ) const
{
 return( f_n );
 }

template< class T , std::totally_ordered P , bool L >
const T & LinkedHeapPriorityQueue< T , P , L >::min( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_root->value );
 }

template< class T , std::totally_ordered P , bool L >
const P & LinkedHeapPriorityQueue< T , P , L >::minPriority( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_root->priority );
 }

template< class T , std::totally_ordered P , bool L >
void LinkedHeapPriorityQueue< T , P , L >::insert( const T & x , const P & p )
{
 ++f_n;
 if( f_n == 1 ) {
  f_root = std::make_unique< Node >( p , x , nullptr );
  f_last = f_root.get();
  }
 else {
  Node * u = L ? nextParent() : at( f_n / 2 );
  auto & slot = u->left ? u->right : u->left;
  slot = std::make_unique< Node >( p , x , u );
  f_last = slot.get();
  for( Node * v = slot.get() ; v->parent &&
       ( v->priority < v->parent->priority ) ; v = v->parent )
   swap_contents( v , v->parent );
  }
 ADS_POST( !( p < f_root->priority ) );
 ADS_INVARIANT( isHeap() );
 }

template< class T , std::totally_ordered P , bool L >
T LinkedHeapPriorityQueue< T , P , L >::deleteMin( void )
{
 ADS_PRE( ! isEmpty() );
 Node * r = f_root.get();
 const P q = r->priority;
 T x = std::move( r->value );
 if( f_n == 1 ) {
  f_root.reset();
  f_last = nullptr;
  f_n = 0;
  return( x );
  }
 Node * last = L ? f_last : at( f_n );
 r->value = std::move( last->value );
 r->priority = last->priority;
 Node * p = last->parent;
 if constexpr( L )
  f_last = previousLeaf();
 ( p->right.get() == last ? p->right : p->left ).reset();
 --f_n;
 for( Node * v = r ; ; ) {
  Node * m = v;
  for( Node * c : { v->left.get() , v->right.get() } )
   if( c && ( c->priority < m->priority ) ) m = c;
  if( m == v ) break;
  swap_contents( v , m );
  v = m;
  }
 ADS_POST( !( f_root->priority < q ) );
 ADS_INVARIANT( isHeap() );
 return( x );
 }

template< class T , std::totally_ordered P , bool L >
typename LinkedHeapPriorityQueue< T , P , L >::Node *
LinkedHeapPriorityQueue< T , P , L >::at( int k ) const
{
 int b = 1;
 while( ( b << 1 ) <= k ) b <<= 1;
 Node * u = f_root.get();
 for( b >>= 1 ; b ; b >>= 1 )
  u = ( k & b ) ? u->right.get() : u->left.get();
 return( u );
 }

template< class T , std::totally_ordered P , bool L >
void LinkedHeapPriorityQueue< T , P , L >::swap_contents( Node * a , Node * b )
{
 std::swap( a->priority , b->priority );
 std::swap( a->value , b->value );
 }

template< class T , std::totally_ordered P , bool L >
bool
LinkedHeapPriorityQueue< T , P , L >::shaped( const Node * u , int k ,
                                          int & n ) const
{
 ++n;
 const Node * c[ 2 ] = { u->left.get() , u->right.get() };
 for( int j = 0 ; j < 2 ; ++j ) {
  const int ck = 2 * k + j;
  if( ( c[ j ] != nullptr ) != ( ck <= f_n ) )
   return( false );
  if( c[ j ] && ( ( c[ j ]->parent != u ) ||
                  ( c[ j ]->priority < u->priority ) ||
                  ! shaped( c[ j ] , ck , n ) ) )
   return( false );
  }
 return( true );
 }

template< class T , std::totally_ordered P , bool L >
auto LinkedHeapPriorityQueue< T , P , L >::nextParent( void ) const -> Node *
{
 Node * u = f_last;
 if( u == f_root.get() )                    // the left child of the root
  return( u );
 if( u == u->parent->left.get() )           // the right sibling of last
  return( u->parent );
 while( ( u != f_root.get() ) && ( u == u->parent->right.get() ) )
  u = u->parent;                            // up while a right child
 if( u != f_root.get() )
  u = u->parent->right.get();               // the subtree on the right
 while( u->left )                           // down to the leftmost leaf
  u = u->left.get();
 return( u );
 }

template< class T , std::totally_ordered P , bool L >
auto LinkedHeapPriorityQueue< T , P , L >::previousLeaf( void ) const
 -> Node *
{
 Node * u = f_last;                         // not the root
 if( u == u->parent->right.get() )          // its left sibling
  return( u->parent->left.get() );
 while( ( u != f_root.get() ) && ( u == u->parent->left.get() ) )
  u = u->parent;                            // up while a left child
 if( u != f_root.get() )
  u = u->parent->left.get();                // the subtree on the left
 while( u->right )                          // down to the rightmost node
  u = u->right.get();
 return( u );
 }

template< class T , std::totally_ordered P , bool L >
bool LinkedHeapPriorityQueue< T , P , L >::isHeap( void ) const
{
 if( L && ( f_last != ( f_n ? at( f_n ) : nullptr ) ) )
  return( false );                          // last is the node n
 if( f_n == 0 )
  return( ! f_root );
 if( f_n > 1024 ) {                         // the root and the last leaf
  const Node * last = at( f_n );
  return( f_root && ! f_root->parent && last && ! last->left &&
          ! last->right && !( last->priority < f_root->priority ) );
  }
 int n = 0;
 return( f_root && ! f_root->parent && shaped( f_root.get() , 1 , n ) &&
         ( n == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------ End File LinkedHeapPriorityQueue.tpp ------------------*/
/*--------------------------------------------------------------------------*/
