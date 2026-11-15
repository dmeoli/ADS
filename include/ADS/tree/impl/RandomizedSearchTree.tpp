/*--------------------------------------------------------------------------*/
/*---------------------- File RandomizedSearchTree.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RandomizedSearchTree [see `RandomizedSearchTree.h`],
 * with their contracts.
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

template< std::totally_ordered K , class V >
auto RandomizedSearchTree< K , V >::find( const K & k ) const -> Node *
{
 return( searchTreeFind( f_root.get() , k ) );
 }

template< std::totally_ordered K , class V >
std::optional< V > RandomizedSearchTree< K , V >::lookup( const K & k ) const
{
 const Node * t = find( k );
 return( t ? std::optional< V >( t->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
bool RandomizedSearchTree< K , V >::contains( const K & k ) const
{
 return( find( k ) != nullptr );
 }

template< std::totally_ordered K , class V >
auto RandomizedSearchTree< K , V >::split( Ptr t , const K & k )
  -> std::pair< Ptr , Ptr >
{
 if( ! t )
  return( std::pair< Ptr , Ptr >( nullptr , nullptr ) );
 if( k < t->key ) {
  auto [ a , b ] = split( std::move( t->left ) , k );
  t->left = std::move( b );
  update( t );
  return( std::pair< Ptr , Ptr >( std::move( a ) , std::move( t ) ) );
  }
 auto [ a , b ] = split( std::move( t->right ) , k );
 t->right = std::move( a );
 update( t );
 return( std::pair< Ptr , Ptr >( std::move( t ) , std::move( b ) ) );
 }

template< std::totally_ordered K , class V >
void RandomizedSearchTree< K , V >::insert( Ptr & t , Ptr & u )
{
 if( uniformBelow( f_g , sz( t ) + 1 ) == 0 ) {  // u the root of t
  auto [ a , b ] = split( std::move( t ) , u->key );
  u->left = std::move( a );
  u->right = std::move( b );
  update( u );
  t = std::move( u );
  return;
  }
 insert( u->key < t->key ? t->left : t->right , u );
 ++t->s;
 }

template< std::totally_ordered K , class V >
void RandomizedSearchTree< K , V >::insert( const K & k , const V & v )
{
 if( Node * t = find( k ) )
  t->value = v;
 else {
  Ptr u = std::make_unique< Node >( Node{ k , v , 1 , nullptr , nullptr } );
  insert( f_root , u );
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
auto RandomizedSearchTree< K , V >::join( Ptr a , Ptr b ) -> Ptr
{
 if( ! a )
  return( b );
 if( ! b )
  return( a );
 if( uniformBelow( f_g , a->s + b->s ) < a->s ) {
  a->right = join( std::move( a->right ) , std::move( b ) );
  update( a );
  return( a );
  }
 b->left = join( std::move( a ) , std::move( b->left ) );
 update( b );
 return( b );
 }

template< std::totally_ordered K , class V >
bool RandomizedSearchTree< K , V >::remove( Ptr & t , const K & k )
{
 if( ! t )
  return( false );
 bool gone;
 if( k < t->key )
  gone = remove( t->left , k );
 else if( t->key < k )
  gone = remove( t->right , k );
 else {
  t = join( std::move( t->left ) , std::move( t->right ) );
  return( true );
  }
 if( gone )
  --t->s;
 return( gone );
 }

template< std::totally_ordered K , class V >
void RandomizedSearchTree< K , V >::remove( const K & k )
{
 remove( f_root , k );
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
auto RandomizedSearchTree< K , V >::lowerBound( const K & k ) const
  -> std::optional< std::pair< K , V > >
{
 return( searchTreeLowerBoundEntry( f_root.get() , k ) );
 }

template< std::totally_ordered K , class V >
const K * RandomizedSearchTree< K , V >::findKey( const K & k ) const
{
 return( searchTreeFindKey( f_root.get() , k ) );
 }

template< std::totally_ordered K , class V >
const K * RandomizedSearchTree< K , V >::lowerBoundKey( const K & k ) const
{
 return( searchTreeLowerBoundKey( f_root.get() , k ) );
 }

template< std::totally_ordered K , class V >
std::pair< K , V > RandomizedSearchTree< K , V >::select( std::size_t i ) const
{
 const Node * t = searchTreeSelect( f_root.get() , i );
 return( std::pair< K , V >( t->key , t->value ) );
 }

template< std::totally_ordered K , class V >
std::size_t RandomizedSearchTree< K , V >::rank( const K & k ) const
{
 return( searchTreeRank( f_root.get() , k ) );
 }

template< std::totally_ordered K , class V >
int RandomizedSearchTree< K , V >::height( const Ptr & t )
{
 return( t ? 1 + std::max( height( t->left ) , height( t->right ) ) : -1 );
 }

template< std::totally_ordered K , class V >
int RandomizedSearchTree< K , V >::height( void ) const
{
 return( height( f_root ) );
 }

template< std::totally_ordered K , class V >
template< class F >
void RandomizedSearchTree< K , V >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this )
  f( k , v );
 }

template< std::totally_ordered K , class V >
bool RandomizedSearchTree< K , V >::valid( const Ptr & t , const K * lo ,
					  const K * hi )
{
 if( ! t )
  return( true );
 if( ( lo && ! ( *lo < t->key ) ) || ( hi && ! ( t->key < *hi ) ) ||
     ( t->s != 1 + sz( t->left ) + sz( t->right ) ) )
  return( false );
 return( valid( t->left , lo , &t->key ) && valid( t->right , &t->key , hi ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File RandomizedSearchTree.tpp -------------------*/
/*--------------------------------------------------------------------------*/
