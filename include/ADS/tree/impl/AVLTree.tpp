/*--------------------------------------------------------------------------*/
/*---------------------------- File AVLTree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of AVLTree [see AVLTree.h], with their contracts.
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
std::optional< V > AVLTree< K , V >::lookup( const K & k ) const
{
 for( const Node * t = f_root.get() ; t ; )
  if( k < t->key ) t = t->left.get();
  else if( t->key < k ) t = t->right.get();
  else return( t->value );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
void AVLTree< K , V >::insert( const K & k , const V & v )
{
 if( insert( f_root , k , v ) )
  ++f_size;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
void AVLTree< K , V >::remove( const K & k )
{
 if( remove( f_root , k ) )
  --f_size;
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
bool AVLTree< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V >
std::size_t AVLTree< K , V >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V >
bool AVLTree< K , V >::empty( void ) const
{
 return( f_size == 0 );
 }

template< std::totally_ordered K , class V >
int AVLTree< K , V >::height( void ) const
{
 return( h( f_root ) );
 }

template< std::totally_ordered K , class V >
template< class F >
void AVLTree< K , V >::forEach( F && f ) const
{
 visit( f_root , f );
 }

template< std::totally_ordered K , class V >
void AVLTree< K , V >::update( Ptr & t )
{
 t->h = 1 + std::max( h( t->left ) , h( t->right ) );
 }

template< std::totally_ordered K , class V >
void AVLTree< K , V >::rotateRight( Ptr & t )
{
 Ptr l = std::move( t->left );
 t->left = std::move( l->right );
 update( t );
 l->right = std::move( t );
 t = std::move( l );
 update( t );
 }

template< std::totally_ordered K , class V >
void AVLTree< K , V >::rotateLeft( Ptr & t )
{
 Ptr r = std::move( t->right );
 t->right = std::move( r->left );
 update( t );
 r->left = std::move( t );
 t = std::move( r );
 update( t );
 }

template< std::totally_ordered K , class V >
void AVLTree< K , V >::balance( Ptr & t )
{
 update( t );
 const int b = h( t->left ) - h( t->right );
 if( b > 1 ) {                              // too high on the left
  if( h( t->left->left ) < h( t->left->right ) )
   rotateLeft( t->left );                   // double rotation
  rotateRight( t );
  }
 else if( b < -1 ) {                        // too high on the right
  if( h( t->right->right ) < h( t->right->left ) )
   rotateRight( t->right );
  rotateLeft( t );
  }
 }

template< std::totally_ordered K , class V >
bool AVLTree< K , V >::insert( Ptr & t , const K & k , const V & v )
{
 if( ! t ) {
  t = std::make_unique< Node >( Node{ k , v , 0 , nullptr , nullptr } );
  return( true );
  }
 bool fresh = false;
 if( k < t->key ) fresh = insert( t->left , k , v );
 else if( t->key < k ) fresh = insert( t->right , k , v );
 else t->value = v;                         // the new value replaces it
 balance( t );
 return( fresh );
 }

template< std::totally_ordered K , class V >
bool AVLTree< K , V >::remove( Ptr & t , const K & k )
{
 if( ! t )
  return( false );
 bool gone = true;
 if( k < t->key ) gone = remove( t->left , k );
 else if( t->key < k ) gone = remove( t->right , k );
 else if( ! t->left ) {
  t = std::move( t->right );
  return( true );
  }
 else if( ! t->right ) {
  t = std::move( t->left );
  return( true );
  }
 else {                                     // the successor takes its place
  Node * s = t->right.get();
  while( s->left ) s = s->left.get();
  t->key = s->key;
  t->value = s->value;
  remove( t->right , s->key );
  }
 balance( t );
 return( gone );
 }

template< std::totally_ordered K , class V >
template< class F >
void AVLTree< K , V >::visit( const Ptr & t , F & f )
{
 if( ! t ) return;
 visit( t->left , f );
 f( t->key , t->value );
 visit( t->right , f );
 }

template< std::totally_ordered K , class V >
bool AVLTree< K , V >::valid( const Ptr & t , const K * lo , const K * hi )
{
 if( ! t )
  return( true );
 if( ( lo && !( *lo < t->key ) ) || ( hi && !( t->key < *hi ) ) )
  return( false );
 if( t->h != 1 + std::max( h( t->left ) , h( t->right ) ) )
  return( false );
 const int b = h( t->left ) - h( t->right );
 return( ( b >= -1 ) && ( b <= 1 ) && valid( t->left , lo , &t->key ) &&
         valid( t->right , &t->key , hi ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File AVLTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
