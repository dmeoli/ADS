/*--------------------------------------------------------------------------*/
/*--------------------------- File TreapList.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of TreapList [see `TreapList.h`], with their contracts.
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

template< class T >
void TreapList< T >::rotateRight( Ptr & t )
{
 Ptr l = std::move( t->left );
 t->left = std::move( l->right );
 update( t );
 l->right = std::move( t );
 t = std::move( l );
 update( t );
 }

template< class T >
void TreapList< T >::rotateLeft( Ptr & t )
{
 Ptr r = std::move( t->right );
 t->right = std::move( r->left );
 update( t );
 r->left = std::move( t );
 t = std::move( r );
 update( t );
 }

template< class T >
auto TreapList< T >::nodeAt( std::size_t i ) const -> Node *
{
 //! [get]
 Node * t = f_root.get();
 for( std::size_t l ; i != ( l = sz( t->left ) ) ; )
  if( i < l ) t = t->left.get();
  else {
   i -= l + 1;
   t = t->right.get();
   }
 return( t );
 //! [get]
 }

template< class T >
const T & TreapList< T >::get( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( nodeAt( i )->item );
 }

template< class T >
void TreapList< T >::set( std::size_t i , const T & v )
{
 ADS_PRE( i < size() );
 nodeAt( i )->item = v;
 }

template< class T >
void TreapList< T >::add( Ptr & t , std::size_t i , const T & v )
{
 //! [add]
 if( ! t ) {
  t = std::make_unique< Node >( Node{ v , f_g() , 1 , nullptr , nullptr } );
  return;
  }
 if( i <= sz( t->left ) ) {
  add( t->left , i , v );
  update( t );
  if( t->left->priority > t->priority ) rotateRight( t );
  }
 else {
  add( t->right , i - sz( t->left ) - 1 , v );
  update( t );
  if( t->right->priority > t->priority ) rotateLeft( t );
  }
 //! [add]
 }

template< class T >
void TreapList< T >::add( std::size_t i , const T & v )
{
 ADS_PRE( i <= size() );
 add( f_root , i , v );
 ADS_INVARIANT( valid( f_root ) );
 }

template< class T >
T TreapList< T >::remove( Ptr & t , std::size_t i )
{
 //! [remove]
 const std::size_t l = sz( t->left );
 if( ( i == l ) && ( ! t->left || ! t->right ) ) {   // one child at most
  T v = std::move( t->item );
  t = std::move( t->left ? t->left : t->right );
  return( v );
  }
 if( i == l ) {                             // down, the child of larger
  if( t->left->priority > t->right->priority ) {   // priority up
   rotateRight( t );
   T v = remove( t->right , sz( t->right->left ) );
   update( t );
   return( v );
   }
  rotateLeft( t );
  T v = remove( t->left , sz( t->left->left ) );
  update( t );
  return( v );
  }
 T v = ( i < l ) ? remove( t->left , i ) : remove( t->right , i - l - 1 );
 update( t );
 return( v );
 //! [remove]
 }

template< class T >
T TreapList< T >::remove( std::size_t i )
{
 ADS_PRE( i < size() );
 T v = remove( f_root , i );
 ADS_INVARIANT( valid( f_root ) );
 return( v );
 }

template< class T >
auto TreapList< T >::split( Ptr t , std::size_t i ) -> std::pair< Ptr , Ptr >
{
 //! [split]
 if( ! t )
  return( std::pair< Ptr , Ptr >( nullptr , nullptr ) );
 if( i <= sz( t->left ) ) {
  auto [ a , b ] = split( std::move( t->left ) , i );
  t->left = std::move( b );
  update( t );
  return( std::pair< Ptr , Ptr >( std::move( a ) , std::move( t ) ) );
  }
 auto [ a , b ] = split( std::move( t->right ) , i - sz( t->left ) - 1 );
 t->right = std::move( a );
 update( t );
 return( std::pair< Ptr , Ptr >( std::move( t ) , std::move( b ) ) );
 //! [split]
 }

template< class T >
auto TreapList< T >::join( Ptr a , Ptr b ) -> Ptr
{
 //! [join]
 if( ! a ) return( b );
 if( ! b ) return( a );
 if( a->priority > b->priority ) {
  a->right = join( std::move( a->right ) , std::move( b ) );
  update( a );
  return( a );
  }
 b->left = join( std::move( a ) , std::move( b->left ) );
 update( b );
 return( b );
 //! [join]
 }

template< class T >
auto TreapList< T >::truncate( std::size_t i ) -> TreapList
{
 ADS_PRE( i <= size() );
 TreapList r( f_g() );
 auto [ a , b ] = split( std::move( f_root ) , i );
 f_root = std::move( a );
 r.f_root = std::move( b );
 ADS_INVARIANT( valid( f_root ) && valid( r.f_root ) );
 return( r );
 }

template< class T >
void TreapList< T >::absorb( TreapList & s )
{
 if( &s == this )
  return;
 f_root = join( std::move( f_root ) , std::move( s.f_root ) );
 ADS_INVARIANT( valid( f_root ) );
 }

template< class T >
bool TreapList< T >::valid( const Ptr & t )
{
 if( ! t )
  return( true );
 if( ( t->left && ( t->left->priority > t->priority ) ) ||
     ( t->right && ( t->right->priority > t->priority ) ) ||
     ( t->s != 1 + sz( t->left ) + sz( t->right ) ) )
  return( false );
 return( valid( t->left ) && valid( t->right ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File TreapList.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
