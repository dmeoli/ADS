/*--------------------------------------------------------------------------*/
/*--------------------------- File SplayTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SplayTree [see SplayTree.h], with their contracts.
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

template< std::totally_ordered K , class V >
SplayTree< K , V >::SplayTree( SplayTree && other ) noexcept
 : f_root( std::exchange( other.f_root , nullptr ) ) ,
   f_size( std::exchange( other.f_size , 0 ) )
{
 }

template< std::totally_ordered K , class V >
SplayTree< K , V >::~SplayTree()
{
 destroy( f_root );
 }

template< std::totally_ordered K , class V >
void SplayTree< K , V >::destroy( Node * t )
{
 while( t ) {                               // no recursion: a splay tree
  if( t->left ) {                           // may be a path
   Node * l = t->left;
   t->left = l->right;
   l->right = t;
   t = l;
   }
  else {
   Node * r = t->right;
   delete t;
   t = r;
   }
  }
 }

template< std::totally_ordered K , class V >
auto SplayTree< K , V >::splay( Node * t , const K & k ) -> Node *
{
 if( ! t )
  return( t );
 Node N{ k , V() };                         // the heads of the two parts
 Node * l = &N , * r = &N;
 for( ; ; ) {
  if( k < t->key ) {
   if( ! t->left ) break;
   if( k < t->left->key ) {                 // zig-zig: rotate right
    Node * y = t->left;
    t->left = y->right;
    y->right = t;
    t = y;
    if( ! t->left ) break;
    }
   r->left = t;                             // link right
   r = t;
   t = t->left;
   }
  else if( t->key < k ) {
   if( ! t->right ) break;
   if( t->right->key < k ) {                // zig-zig: rotate left
    Node * y = t->right;
    t->right = y->left;
    y->left = t;
    t = y;
    if( ! t->right ) break;
    }
   l->right = t;                            // link left
   l = t;
   t = t->right;
   }
  else
   break;
  }
 l->right = t->left;                        // assemble
 r->left = t->right;
 t->left = N.right;
 t->right = N.left;
 return( t );
 }

template< std::totally_ordered K , class V >
std::optional< V > SplayTree< K , V >::lookup( const K & k )
{
 f_root = splay( f_root , k );
 if( f_root && !( f_root->key < k ) && !( k < f_root->key ) )
  return( f_root->value );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
void SplayTree< K , V >::insert( const K & k , const V & v )
{
 f_root = splay( f_root , k );
 if( f_root && !( f_root->key < k ) && !( k < f_root->key ) )
  f_root->value = v;
 else {
  Node * n = new Node{ k , v };
  if( f_root ) {
   if( k < f_root->key ) {
    n->left = f_root->left;
    n->right = f_root;
    f_root->left = nullptr;
    }
   else {
    n->right = f_root->right;
    n->left = f_root;
    f_root->right = nullptr;
    }
   }
  f_root = n;
  ++f_size;
  }
 ADS_POST( !( f_root->key < k ) && !( k < f_root->key ) );
 ADS_INVARIANT( ordered( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
void SplayTree< K , V >::remove( const K & k )
{
 f_root = splay( f_root , k );
 if( f_root && !( f_root->key < k ) && !( k < f_root->key ) ) {
  Node * t = f_root;
  if( ! t->left )
   f_root = t->right;
  else {                                    // the maximum of the left part
   f_root = splay( t->left , k );           // comes up, with no right child
   f_root->right = t->right;
   }
  delete t;
  --f_size;
  }
 ADS_POST( ! f_root || ( f_root->key < k ) || ( k < f_root->key ) );
 ADS_INVARIANT( ordered( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
std::size_t SplayTree< K , V >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V >
std::optional< K > SplayTree< K , V >::root( void ) const
{
 return( f_root ? std::optional< K >( f_root->key ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
template< class F >
void SplayTree< K , V >::forEach( F && f ) const
{
 visit( f_root , f );
 }

template< std::totally_ordered K , class V >
template< class F >
void SplayTree< K , V >::visit( const Node * t , F & f )
{
 std::vector< const Node * > S;             // iterative: the tree may be
 for( ; t || ! S.empty() ; ) {              // a path of n nodes
  for( ; t ; t = t->left ) S.push_back( t );
  t = S.back();
  S.pop_back();
  f( t->key , t->value );
  t = t->right;
  }
 }

template< std::totally_ordered K , class V >
bool SplayTree< K , V >::ordered( const Node * t , const K * lo ,
                                  const K * hi )
{
 std::vector< std::pair< const Node * , std::pair< const K * , const K * > > >
  S = { { t , { lo , hi } } };              // iterative, as visit()
 while( ! S.empty() ) {
  auto [ x , b ] = S.back();
  S.pop_back();
  if( ! x ) continue;
  if( ( b.first && !( *b.first < x->key ) ) ||
      ( b.second && !( x->key < *b.second ) ) )
   return( false );
  S.push_back( { x->left , { b.first , &x->key } } );
  S.push_back( { x->right , { &x->key , b.second } } );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File SplayTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
