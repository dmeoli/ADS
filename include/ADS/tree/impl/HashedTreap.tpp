/*--------------------------------------------------------------------------*/
/*-------------------------- File HashedTreap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of HashedTreap [see `HashedTreap.h`], with their contracts.
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
std::optional< V > HashedTreap< K , V >::lookup( const K & k ) const
{
 for( const Node * t = f_root.get() ; t ; )
  if( k < t->key ) t = t->left.get();
  else if( t->key < k ) t = t->right.get();
  else return( t->value );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
bool HashedTreap< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V >
auto HashedTreap< K , V >::lowerBound( const K & k ) const
 -> std::optional< std::pair< K , V > >
{
 const Node * best = nullptr;
 for( const Node * t = f_root.get() ; t ; )
  if( t->key < k ) t = t->right.get();
  else {
   best = t;
   if( !( k < t->key ) ) break;
   t = t->left.get();
   }
 return( best ? std::optional< std::pair< K , V > >(
                 std::pair< K , V >( best->key , best->value ) )
              : std::nullopt );
 }

template< std::totally_ordered K , class V >
void HashedTreap< K , V >::rotateRight( Ptr & t )
{
 Ptr l = std::move( t->left );
 t->left = std::move( l->right );
 l->right = std::move( t );
 t = std::move( l );
 }

template< std::totally_ordered K , class V >
void HashedTreap< K , V >::rotateLeft( Ptr & t )
{
 Ptr r = std::move( t->right );
 t->right = std::move( r->left );
 r->left = std::move( t );
 t = std::move( r );
 }

template< std::totally_ordered K , class V >
int HashedTreap< K , V >::insert( Ptr & t , const K & k , const V & v )
{
 //! [add]
 // 0: k was there; 1: a new node that may still go up; 2: settled
 if( ! t ) {
  t = std::make_unique< Node >( Node{ k , v , nullptr , nullptr } );
  return( 1 );
  }
 if( !( k < t->key ) && !( t->key < k ) ) {
  t->value = v;
  return( 0 );
  }
 const bool left = k < t->key;
 Ptr & c = left ? t->left : t->right;
 const int r = insert( c , k , v );
 if( r != 1 )
  return( r );
 if( !( priority( c.get() ) > priority( t.get() ) ) )   // the heap holds:
  return( 2 );                                    // no hash above
 if( left ) rotateRight( t );
 else rotateLeft( t );
 return( 1 );
 //! [add]
 }

template< std::totally_ordered K , class V >
void HashedTreap< K , V >::insert( const K & k , const V & v )
{
 if( insert( f_root , k , v ) )               // 1 or 2: a new key
  ++f_size;
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
bool HashedTreap< K , V >::remove( Ptr & t , const K & k )
{
 //! [remove]
 if( ! t )
  return( false );
 if( k < t->key ) return( remove( t->left , k ) );
 if( t->key < k ) return( remove( t->right , k ) );
 if( ! t->left ) {
  t = std::move( t->right );
  return( true );
  }
 if( ! t->right ) {
  t = std::move( t->left );
  return( true );
  }
 if( priority( t->left.get() ) > priority( t->right.get() ) ) {
  rotateRight( t );
  return( remove( t->right , k ) );
  }
 rotateLeft( t );
 return( remove( t->left , k ) );
 //! [remove]
 }

template< std::totally_ordered K , class V >
void HashedTreap< K , V >::remove( const K & k )
{
 if( remove( f_root , k ) )
  --f_size;
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid( f_root , nullptr , nullptr ) );
 }

template< std::totally_ordered K , class V >
int HashedTreap< K , V >::height( const Ptr & t )
{
 return( t ? 1 + std::max( height( t->left ) , height( t->right ) ) : -1 );
 }

template< std::totally_ordered K , class V >
int HashedTreap< K , V >::height( void ) const
{
 return( height( f_root ) );
 }

template< std::totally_ordered K , class V >
template< class F >
void HashedTreap< K , V >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this ) f( k , v );
 }

template< std::totally_ordered K , class V >
bool HashedTreap< K , V >::valid( const Ptr & t , const K * lo ,
                                const K * hi ) const
{
 if( ! t )
  return( true );
 if( ( lo && !( *lo < t->key ) ) || ( hi && !( t->key < *hi ) ) )
  return( false );
 if( ( t->left && ( priority( t->left.get() ) > priority( t.get() ) ) ) ||
     ( t->right && ( priority( t->right.get() ) > priority( t.get() ) ) ) )
  return( false );
 return( valid( t->left , lo , &t->key ) && valid( t->right , &t->key , hi ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File HashedTreap.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
