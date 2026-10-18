/*--------------------------------------------------------------------------*/
/*----------------------------- File Treap.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Treap [see Treap.h], with their contracts.
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
Treap< K , V >::Treap( std::uint64_t seed ) : f_g( seed )
{
 }

template< std::totally_ordered K , class V >
std::optional< V > Treap< K , V >::lookup( const K & k ) const
{
 for( const Node * t = f_root.get() ; t ; )
  if( k < t->key ) t = t->left.get();
  else if( t->key < k ) t = t->right.get();
  else return( t->value );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
void Treap< K , V >::rotateRight( Ptr & t )
{
 Ptr l = std::move( t->left );
 t->left = std::move( l->right );
 l->right = std::move( t );
 t = std::move( l );
 }

template< std::totally_ordered K , class V >
void Treap< K , V >::rotateLeft( Ptr & t )
{
 Ptr r = std::move( t->right );
 t->right = std::move( r->left );
 r->left = std::move( t );
 t = std::move( r );
 }

template< std::totally_ordered K , class V >
bool Treap< K , V >::insert( Ptr & t , const K & k , const V & v )
{
 if( ! t ) {
  t = std::make_unique< Node >( Node{ k , v , f_g() , nullptr , nullptr } );
  return( true );
  }
 if( k < t->key ) {
  const bool fresh = insert( t->left , k , v );
  if( t->left->priority > t->priority ) rotateRight( t );   // up the heap
  return( fresh );
  }
 if( t->key < k ) {
  const bool fresh = insert( t->right , k , v );
  if( t->right->priority > t->priority ) rotateLeft( t );
  return( fresh );
  }
 t->value = v;
 return( false );
 }

template< std::totally_ordered K , class V >
void Treap< K , V >::insert( const K & k , const V & v )
{
 if( insert( f_root , k , v ) )
  ++f_size;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
bool Treap< K , V >::remove( Ptr & t , const K & k )
{
 if( ! t )
  return( false );
 if( k < t->key ) return( remove( t->left , k ) );
 if( t->key < k ) return( remove( t->right , k ) );
 if( ! t->left ) {                          // a leaf, or one child: away
  t = std::move( t->right );
  return( true );
  }
 if( ! t->right ) {
  t = std::move( t->left );
  return( true );
  }
 if( t->left->priority > t->right->priority ) {   // down, the child of
  rotateRight( t );                               // higher priority up
  return( remove( t->right , k ) );
  }
 rotateLeft( t );
 return( remove( t->left , k ) );
 }

template< std::totally_ordered K , class V >
void Treap< K , V >::remove( const K & k )
{
 if( remove( f_root , k ) )
  --f_size;
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
bool Treap< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V >
std::size_t Treap< K , V >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V >
int Treap< K , V >::height( const Ptr & t )
{
 return( t ? 1 + std::max( height( t->left ) , height( t->right ) ) : -1 );
 }

template< std::totally_ordered K , class V >
int Treap< K , V >::height( void ) const
{
 return( height( f_root ) );
 }

template< std::totally_ordered K , class V >
template< class F >
void Treap< K , V >::forEach( F && f ) const
{
 visit( f_root , f );
 }

template< std::totally_ordered K , class V >
template< class F >
void Treap< K , V >::visit( const Ptr & t , F & f )
{
 if( ! t ) return;
 visit( t->left , f );
 f( t->key , t->value );
 visit( t->right , f );
 }

template< std::totally_ordered K , class V >
bool Treap< K , V >::valid( const Ptr & t , const K * lo , const K * hi )
{
 if( ! t )
  return( true );
 if( ( lo && !( *lo < t->key ) ) || ( hi && !( t->key < *hi ) ) )
  return( false );
 if( ( t->left && ( t->left->priority > t->priority ) ) ||
     ( t->right && ( t->right->priority > t->priority ) ) )
  return( false );
 return( valid( t->left , lo , &t->key ) && valid( t->right , &t->key , hi ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Treap.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
