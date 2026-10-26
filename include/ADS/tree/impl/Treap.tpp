/*--------------------------------------------------------------------------*/
/*----------------------------- File Treap.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Treap [see Treap.h], with their contracts.
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
 update( t );
 l->right = std::move( t );
 t = std::move( l );
 update( t );
 }

template< std::totally_ordered K , class V >
void Treap< K , V >::rotateLeft( Ptr & t )
{
 Ptr r = std::move( t->right );
 t->right = std::move( r->left );
 update( t );
 r->left = std::move( t );
 t = std::move( r );
 update( t );
 }

template< std::totally_ordered K , class V >
bool Treap< K , V >::insert( Ptr & t , const K & k , const V & v )
{
 if( ! t ) {
  t = std::make_unique< Node >( Node{ k , v , f_g() , 1 , nullptr ,
                                      nullptr } );
  return( true );
  }
 if( k < t->key ) {
  const bool fresh = insert( t->left , k , v );
  update( t );
  if( t->left->priority > t->priority ) rotateRight( t );   // up the heap
  return( fresh );
  }
 if( t->key < k ) {
  const bool fresh = insert( t->right , k , v );
  update( t );
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
 if( k < t->key ) {
  const bool gone = remove( t->left , k );
  update( t );
  return( gone );
  }
 if( t->key < k ) {
  const bool gone = remove( t->right , k );
  update( t );
  return( gone );
  }
 if( ! t->left ) {                          // a leaf, or one child: away
  t = std::move( t->right );
  return( true );
  }
 if( ! t->right ) {
  t = std::move( t->left );
  return( true );
  }
 bool gone;
 if( t->left->priority > t->right->priority ) {   // down, the child of
  rotateRight( t );                               // higher priority up
  gone = remove( t->right , k );
  }
 else {
  rotateLeft( t );
  gone = remove( t->left , k );
  }
 update( t );
 return( gone );
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
     ( t->right && ( t->right->priority > t->priority ) ) ||
     ( t->s != 1 + sz( t->left ) + sz( t->right ) ) )
  return( false );
 return( valid( t->left , lo , &t->key ) && valid( t->right , &t->key , hi ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ TreapNodes --------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V >
struct TreapNodes {
 using T = Treap< K , V >;
 using Node = typename T::Node;
 using Key = K;
 using Value = V;
 using Rank = int;

 static Node * nil( void ) { return( nullptr ); }
 static Node * left( const Node * t ) { return( t->left.get() ); }
 static Node * right( const Node * t ) { return( t->right.get() ); }
 static const K & key( const Node * t ) { return( t->key ); }
 static V & value( Node * t ) { return( t->value ); }
 static std::size_t size( const Node * t ) { return( t ? t->s : 0 ); }
 static std::uint64_t priority( const Node * t ) { return( t->priority ); }
 static void expose( Node * t , Node * & l , Node * & r ) {
  l = t->left.release();
  r = t->right.release();
  }
 static Node * connect( Node * m , Node * l , Node * r ) {
  m->left.reset( l );
  m->right.reset( r );
  m->s = 1 + size( l ) + size( r );
  return( m );
  }
 static void dispose( Node * t ) { delete t; }
 static Rank rank( const Node * ) { return( 0 ); }
 static Rank childRank( const Node * , Rank , const Node * ) { return( 0 ); }
 static std::pair< Node * , Rank > join( Node * l , Rank , Node * m ,
                                         Node * r , Rank ) {
  return( std::pair< Node * , Rank >(
           TreapJoin< TreapNodes >::join( l , m , r ) , 0 ) );
  }

 static Node * take( T & t ) {
  t.f_size = 0;
  return( t.f_root.release() );
  }
 static void put( T & t , Node * r ) {
  t.f_root.reset( r );
  t.f_size = size( r );
  }
 static Node * peek( const T & t ) { return( t.f_root.get() ); }
 static T fresh( T & t ) { return( T( t.f_g() ) ); }
 static bool valid( const T & t ) {
  return( t.valid( t.f_root , nullptr , nullptr ) &&
          ( t.f_size == size( t.f_root.get() ) ) );
  }
 static std::uint64_t salt( T & t ) { return( t.f_g() ); }
 static Node * make( const K & k , const V & v , std::uint64_t s ,
                     std::size_t i ) {
  return( new Node{ k , v , splitmix64( s + 0x9e3779b97f4a7c15ULL *
                                            std::uint64_t( i ) ) ,
                    1 , nullptr , nullptr } );
  }
 };

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Treap.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
