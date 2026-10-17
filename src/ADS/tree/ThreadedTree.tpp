/*--------------------------------------------------------------------------*/
/*------------------------- File ThreadedTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ThreadedTree [see ThreadedTree.h], with their
 * contracts.
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

template< std::totally_ordered K >
ThreadedTree< K >::ThreadedTree( void )
{
 f_head.left = &f_head;                     // the empty tree
 f_head.right = &f_head;
 f_head.lthread = true;
 f_head.rthread = false;
 }

template< std::totally_ordered K >
ThreadedTree< K >::~ThreadedTree()
{
 std::vector< Node * > all;
 for( const Node * x = successor( &f_head ) ; x != &f_head ;
      x = successor( x ) )
  all.push_back( const_cast< Node * >( x ) );
 for( Node * x : all ) delete x;
 }

template< std::totally_ordered K >
auto ThreadedTree< K >::successor( const Node * x ) const -> const Node *
{
 if( x->rthread )
  return( x->right );
 const Node * y = x->right;                 // the leftmost of the right
 while( ! y->lthread ) y = y->left;         // subtree
 return( y );
 }

template< std::totally_ordered K >
auto ThreadedTree< K >::predecessor( const Node * x ) const -> const Node *
{
 if( x->lthread )
  return( x->left );
 const Node * y = x->left;
 while( ! y->rthread ) y = y->right;
 return( y );
 }

template< std::totally_ordered K >
void ThreadedTree< K >::insert( const K & k )
{
 if( f_head.lthread ) {                     // the first key: the root
  Node * r = new Node{ k , &f_head , &f_head , true , true };
  f_head.left = r;
  f_head.lthread = false;
  ++f_size;
  }
 else {
  Node * x = f_head.left;
  for( ; ; ) {
   if( k < x->key ) {
    if( x->lthread ) {                      // a new left child: it takes
     Node * n = new Node{ k , x->left , x , true , true };   // the thread
     x->left = n;
     x->lthread = false;
     ++f_size;
     break;
     }
    x = x->left;
    }
   else if( x->key < k ) {
    if( x->rthread ) {
     Node * n = new Node{ k , x , x->right , true , true };
     x->right = n;
     x->rthread = false;
     ++f_size;
     break;
     }
    x = x->right;
    }
   else
    break;
   }
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( threaded() );
 }

template< std::totally_ordered K >
bool ThreadedTree< K >::contains( const K & k ) const
{
 if( f_head.lthread )
  return( false );
 for( const Node * x = f_head.left ; ; ) {
  if( k < x->key ) {
   if( x->lthread ) return( false );
   x = x->left;
   }
  else if( x->key < k ) {
   if( x->rthread ) return( false );
   x = x->right;
   }
  else
   return( true );
  }
 }

template< std::totally_ordered K >
std::size_t ThreadedTree< K >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K >
template< class F >
void ThreadedTree< K >::forEach( F && f ) const
{
 for( const Node * x = successor( &f_head ) ; x != &f_head ;
      x = successor( x ) )
  f( x->key );
 }

template< std::totally_ordered K >
std::vector< K > ThreadedTree< K >::reversed( void ) const
{
 std::vector< K > out;
 for( const Node * x = predecessor( &f_head ) ; x != &f_head ;
      x = predecessor( x ) )
  out.push_back( x->key );
 return( out );
 }

template< std::totally_ordered K >
bool ThreadedTree< K >::threaded( void ) const
{
 std::size_t n = 0;
 const Node * prev = nullptr;
 for( const Node * x = successor( &f_head ) ; x != &f_head ;
      x = successor( x ) , ++n ) {
  if( ( n > f_size ) || ( prev && !( prev->key < x->key ) ) )
   return( false );
  prev = x;
  }
 return( n == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ThreadedTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
