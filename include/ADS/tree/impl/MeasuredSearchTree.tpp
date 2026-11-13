/*--------------------------------------------------------------------------*/
/*----------------------- File MeasuredSearchTree.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MeasuredSearchTree [see `MeasuredSearchTree.h`], with
 * their contracts.
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

template< std::totally_ordered K , class V >
MeasuredSearchTree< K , V >::~MeasuredSearchTree()
{
 for( Node * u = f_root ; u ; ) {           // by rotations to the right,
  if( u->left ) {                           // with no stack
   Node * l = u->left;
   u->left = l->right;
   l->right = u;
   u = l;
   }
  else {
   Node * r = u->right;
   delete u;
   u = r;
   }
  }
 }

template< std::totally_ordered K , class V >
auto MeasuredSearchTree< K , V >::findLast( const K & k ) const -> Node *
{
 //! [find]
 Node * w = f_root , * prev = nullptr;
 while( w ) {
  prev = w;
  if( k < w->key ) w = w->left;
  else if( w->key < k ) w = w->right;
  else return( w );
  }
 return( prev );
 //! [find]
 }

template< std::totally_ordered K , class V >
std::optional< V > MeasuredSearchTree< K , V >::lookup( const K & k ) const
{
 const Node * u = findLast( k );
 return( u && !( u->key < k ) && !( k < u->key ) ? std::optional< V >(
          u->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
bool MeasuredSearchTree< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V >
long MeasuredSearchTree< K , V >::height( void ) const
{
 return( f_root ? f_root->height : -1 );
 }

template< std::totally_ordered K , class V >
auto MeasuredSearchTree< K , V >::measures( const K & k ) const
 -> std::optional< Measures >
{
 const Node * u = findLast( k );
 if( ! u || ( u->key < k ) || ( k < u->key ) )
  return( std::nullopt );
 return( Measures{ u->size , u->depth , u->height } );
 }

template< std::totally_ordered K , class V >
auto MeasuredSearchTree< K , V >::lowerBound( const K & k ) const
 -> std::optional< std::pair< K , V > >
{
 const Node * best = nullptr;
 for( const Node * w = f_root ; w ; )
  if( w->key < k ) w = w->right;
  else {
   best = w;
   if( !( k < w->key ) ) break;
   w = w->left;
   }
 return( best ? std::optional< std::pair< K , V > >(
                 std::pair< K , V >( best->key , best->value ) )
              : std::nullopt );
 }

template< std::totally_ordered K , class V >
auto MeasuredSearchTree< K , V >::getLE( const K & x ) const
 -> std::vector< std::pair< K , V > >
{
 std::vector< std::pair< K , V > > P;
 lessOrEqual( static_cast< const Node * >( f_root ) , x ,
              [ & ]( const Node * u ) {
               P.emplace_back( u->key , u->value ); } );
 return( P );
 }

template< std::totally_ordered K , class V >
void MeasuredSearchTree< K , V >::insert( const K & k , const V & v )
{
 //! [add]
 Node * p = findLast( k );
 if( p && !( p->key < k ) && !( k < p->key ) )
  p->value = v;
 else {
  Node * u = new Node{ k , v };
  u->parent = p;
  if( ! p )
   f_root = u;
  else {
   u->depth = p->depth + 1;
   ( k < p->key ? p->left : p->right ) = u;
   }
  for( Node * w = p ; w ; w = w->parent ) { // the path: sizes and heights
   ++w->size;
   w->height = fresh( w );
   }
  ++f_size;
  }
 //! [add]
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
void MeasuredSearchTree< K , V >::remove( const K & k )
{
 //! [remove]
 Node * u = findLast( k );
 if( u && !( u->key < k ) && !( k < u->key ) ) {
  while( u->left || u->right ) {            // down to a leaf, pairs moved up
   Node * w;
   if( u->right )
    for( w = u->right ; w->left ; w = w->left ) {}
   else
    for( w = u->left ; w->right ; w = w->right ) {}
   u->key = std::move( w->key );
   u->value = std::move( w->value );
   u = w;
   }
  Node * p = u->parent;
  if( ! p ) f_root = nullptr;
  else ( p->left == u ? p->left : p->right ) = nullptr;
  delete u;
  for( Node * w = p ; w ; w = w->parent ) {
   --w->size;
   w->height = fresh( w );
   }
  --f_size;
  }
 //! [remove]
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
template< class F >
void MeasuredSearchTree< K , V >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this ) f( k , v );
 }

template< std::totally_ordered K , class V >
bool MeasuredSearchTree< K , V >::valid( void ) const
{
 if( f_root && ( f_root->parent || f_root->depth ) ) return( false );
 if( ( f_root ? f_root->size : 0 ) != f_size ) return( false );
 if( ! isSearchTree( static_cast< const Node * >( f_root ) ) )
  return( false );
 bool ok = true;
 detail::walkSubtree( static_cast< const Node * >( f_root ) ,
  [ & ]( const Node * u , long d ) {
   const std::size_t s = 1 + ( u->left ? u->left->size : 0 ) +
                         ( u->right ? u->right->size : 0 );
   ok = ok && ( u->size == s ) && ( u->depth == std::size_t( d ) ) &&
        ( u->height == fresh( u ) ) &&
        ( ! u->left || ( u->left->parent == u ) ) &&
        ( ! u->right || ( u->right->parent == u ) );
   } );
 return( ok );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File MeasuredSearchTree.tpp --------------------*/
/*--------------------------------------------------------------------------*/
