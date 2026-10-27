/*--------------------------------------------------------------------------*/
/*-------------------------- File SearchTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SearchTree [see SearchTree.h], with their contracts.
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

template< std::totally_ordered K , class V , bool RB >
SearchTree< K , V , RB >::SearchTree( void )
 : f_nil( new Link )
{
 clear_nil();
 }

template< std::totally_ordered K , class V , bool RB >
SearchTree< K , V , RB >::SearchTree( const SearchTree & o )
 : SearchTree()
{
 f_root = copy( o , o.f_root , f_nil.get() );
 f_size = o.f_size;
 }

template< std::totally_ordered K , class V , bool RB >
SearchTree< K , V , RB >::SearchTree( SearchTree && o ) noexcept
 : SearchTree()
{
 swap( o );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::operator=( 
                                          SearchTree o ) noexcept
 -> SearchTree &
{
 swap( o );
 return( *this );
 }

template< std::totally_ordered K , class V , bool RB >
SearchTree< K , V , RB >::~SearchTree()
{
 clear();
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::swap( SearchTree & o ) noexcept
{
 std::swap( f_nil , o.f_nil );
 std::swap( f_root , o.f_root );
 std::swap( f_size , o.f_size );
 }

template< std::totally_ordered K , class V , bool RB >
std::optional< V > SearchTree< K , V , RB >::lookup( const K & k ) const
{
 Link * u = lookupNode( f_root , k );
 if( u == nil() )
  return( std::nullopt );
 return( value( u ) );
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::insert( const K & k , const V & v )
{
 f_root = insertNode( f_root , k , v );
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::remove( const K & k )
{
 const K x = k;  // k may be the key of a node, overwritten by its successor
 f_root = removeNode( f_root , x );
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , bool RB >
bool SearchTree< K , V , RB >::contains( const K & k ) const
{
 return( lookupNode( f_root , k ) != nil() );
 }

template< std::totally_ordered K , class V , bool RB >
std::size_t SearchTree< K , V , RB >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V , bool RB >
bool SearchTree< K , V , RB >::empty( void ) const
{
 return( f_size == 0 );
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::clear( void )
{
 std::vector< Link * > S;   // iterative, as the height may be n
 if( f_root != nil() )
  S.push_back( f_root );
 while( ! S.empty() ) {
  Link * u = S.back();
  S.pop_back();
  if( u->left != nil() ) S.push_back( u->left );
  if( u->right != nil() ) S.push_back( u->right );
  delete static_cast< Node * >( u );
  }
 f_root = nil();
 f_size = 0;
 clear_nil();
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::root( void ) const -> Link *
{
 return( f_root );
 }

template< std::totally_ordered K , class V , bool RB >
bool SearchTree< K , V , RB >::isNil( const Link * u ) const
{
 return( u == nil() );
 }

template< std::totally_ordered K , class V , bool RB >
const K & SearchTree< K , V , RB >::key( const Link * u )
{
 ADS_PRE( u != nullptr );
 return( static_cast< const Node * >( u )->key );
 }

template< std::totally_ordered K , class V , bool RB >
const V & SearchTree< K , V , RB >::value( const Link * u )
{
 ADS_PRE( u != nullptr );
 return( static_cast< const Node * >( u )->value );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::lookupNode( const K & x ) const -> Link *
{
 return( lookupNode( f_root , x ) );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::min( void ) const -> Link *
{
 return( f_root == nil() ? nil() : min( f_root ) );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::max( void ) const -> Link *
{
 return( f_root == nil() ? nil() : max( f_root ) );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::successorNode( Link * t ) const -> Link *
{
 ADS_PRE( t != nullptr );
 if( t == nil() )
  return( t );
 if( t->right != nil() )
  return( min( t->right ) );
 Link * p = t->parent;
 while( ( p != nil() ) && ( t == p->right ) ) {
  t = p;
  p = p->parent;
  }
 return( p );
 }

template< std::totally_ordered K , class V , bool RB >
template< class F >
void SearchTree< K , V , RB >::forEach( F && f ) const
{
 for( Link * t = min() ; t != nil() ; t = successorNode( t ) )
  f( key( t ) , value( t ) );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::predecessorNode( Link * t ) const -> Link *
{
 ADS_PRE( t != nullptr );
 if( t == nil() )
  return( t );
 if( t->left != nil() )
  return( max( t->left ) );
 Link * p = t->parent;
 while( ( p != nil() ) && ( t == p->left ) ) {
  t = p;
  p = p->parent;
  }
 return( p );
 }

template< std::totally_ordered K , class V , bool RB >
bool SearchTree< K , V , RB >::valid( void ) const
{
 std::size_t count = 0;
 // the black height of the subtree of u, -1 if it breaks a constraint
 auto rec = [ & ]( auto & self , const Link * u , const Link * p ,
                   const K * lo , const K * hi ) -> long {
  if( u == nil() )
   return( 0 );
  if( ( u->parent != p ) || ( ++count > f_size ) ||
      ( lo && !( *lo < key( u ) ) ) || ( hi && !( key( u ) < *hi ) ) )
   return( -1 );                         // the parent and the order
  const long l = self( self , u->left , u , lo , &key( u ) ) ,
             r = self( self , u->right , u , &key( u ) , hi );
  if( ( l < 0 ) || ( r < 0 ) )
   return( -1 );
  if constexpr( RB ) {
   if( l != r )
    return( -1 );                        // (V4)
   if( ( u->color == Color::RED ) && ( ( u->left->color == Color::RED ) ||
                                       ( u->right->color == Color::RED ) ) )
    return( -1 );                        // (V3)
   return( l + ( u->color == Color::BLACK ) );
   }
  return( 0 );
  };
 if( RB && ( ( f_root->color != Color::BLACK ) ||
             ( nil()->color != Color::BLACK ) ) )
  return( false );                       // (V1) and (V2)
 return( ( rec( rec , f_root , nil() , nullptr , nullptr ) >= 0 ) &&
         ( count == f_size ) );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::nil( void ) const -> Link *
{
 return( f_nil.get() );
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::clear_nil( void )
{
 f_nil->parent = f_nil->left = f_nil->right = f_nil.get();
 f_nil->color = Color::BLACK;
 }

template< std::totally_ordered K , class V , bool RB >
K & SearchTree< K , V , RB >::key_ref( Link * u )
{
 return( static_cast< Node * >( u )->key );
 }

template< std::totally_ordered K , class V , bool RB >
V & SearchTree< K , V , RB >::value_ref( Link * u )
{
 return( static_cast< Node * >( u )->value );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::new_node( const K & x , const V & v ) -> Link *
{
 ++f_size;
 return( new Node{ { nil() , nil() , nil() , Color::BLACK } , x , v } );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::copy( const SearchTree & o , Link * u ,
                                     Link * p ) -> Link *
{
 if( u == o.nil() )
  return( nil() );
 struct Item { Link * from; Link * parent; Link ** slot; };
 Link * r = nil();
 std::vector< Item > S = { { u , p , &r } };
 while( ! S.empty() ) {
  const Item it = S.back();
  S.pop_back();
  Link * c = new Node{ { it.parent , nil() , nil() , it.from->color } ,
                       key( it.from ) , value( it.from ) };
  *it.slot = c;
  if( it.from->left != o.nil() ) S.push_back( { it.from->left , c ,
                                                &c->left } );
  if( it.from->right != o.nil() ) S.push_back( { it.from->right , c ,
                                                 &c->right } );
  }
 return( r );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::lookupNode( Link * T ,
                                           const K & x ) const -> Link *
{
 while( ( T != nil() ) && ( key( T ) != x ) )
  T = x < key( T ) ? T->left : T->right;
 return( T );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::min( Link * T ) const -> Link *
{
 while( T->left != nil() )
  T = T->left;
 return( T );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::max( Link * T ) const -> Link *
{
 while( T->right != nil() )
  T = T->right;
 return( T );
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::link( Link * p , Link * u , const K & x )
{
 u->parent = p;
 if( p != nil() ) {
  if( x < key( p ) ) p->left = u;
  else p->right = u;
  }
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::insertNode( Link * T , const K & x ,
                                           const V & v ) -> Link *
{
 Link * p = nil();                              // parent
 Link * u = T;
 while( ( u != nil() ) && ( key( u ) != x ) ) {  // finds where to insert
  p = u;
  u = x < key( u ) ? u->left : u->right;
  }
 if( u != nil() ) {                             // the key is there
  value_ref( u ) = v;
  return( T );                                  // mended: also if RB
  }
 Link * n = new_node( x , v );
 link( p , n , x );
 if constexpr( RB ) {
  balanceInsert( n );
  while( n->parent != nil() )
   n = n->parent;
  return( n );                                  // the current root
  }
 else
  return( p == nil() ? n : T );                 // the first node
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::removeNode( Link * T , const K & x0 ) -> Link *
{
 Link * u = lookupNode( T , x0 );
 if( u == nil() )
  return( T );
 const K * x = &x0;
 if( ( u->left != nil() ) && ( u->right != nil() ) ) {   // case (3)
  Link * s = u->right;
  while( s->left != nil() )
   s = s->left;
  key_ref( u ) = key( s );
  value_ref( u ) = value( s );
  x = &key( s );
  u = s;
  }
 Link * t;
 if( ( u->left != nil() ) && ( u->right == nil() ) )     // case (2), left
  t = u->left;
 else                                                    // (2) right, (1)
  t = u->right;
 link( u->parent , t , *x );
 if constexpr( RB )
  if( u->color == Color::BLACK )
   balanceDelete( t );
 if( u->parent == nil() )
  T = t;
 delete static_cast< Node * >( u );
 --f_size;
 if constexpr( RB )
  if( T != nil() )
   while( T->parent != nil() )
    T = T->parent;
 if( T == nil() )
  clear_nil();
 return( T );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::rotateLeft( Link * x ) -> Link *
{
 Link * y = x->right;
 Link * p = x->parent;
 x->right = y->left;                            // (1) B right of x
 if( y->left != nil() ) y->left->parent = x;
 y->left = x;                                   // (2) x left of y
 x->parent = y;
 y->parent = p;                                 // (3) y child of p
 if( p != nil() ) {
  if( p->left == x ) p->left = y;
  else p->right = y;
  }
 return( y );
 }

template< std::totally_ordered K , class V , bool RB >
auto SearchTree< K , V , RB >::rotateRight( Link * x ) -> Link *
{
 Link * y = x->left;
 Link * p = x->parent;
 x->left = y->right;
 if( y->right != nil() ) y->right->parent = x;
 y->right = x;
 x->parent = y;
 y->parent = p;
 if( p != nil() ) {
  if( p->right == x ) p->right = y;
  else p->left = y;
  }
 return( y );
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::balanceInsert( Link * t )
{
 t->color = RED;
 while( t != nil() ) {
  Link * p = t->parent;                                  // parent
  Link * n = p != nil() ? p->parent : nil();             // grandparent
  Link * z = n == nil() ? nil() : ( n->left == p ? n->right : n->left );
  if( p == nil() ) {                                     // case (1)
   t->color = BLACK;
   t = nil();
   }
  else if( p->color == BLACK )                           // case (2)
   t = nil();
  else if( z->color == RED ) {                           // case (3)
   p->color = z->color = BLACK;
   n->color = RED;
   t = n;
   }
  else if( ( t == p->right ) && ( p == n->left ) ) {     // case (4a)
   rotateLeft( p );
   t = p;
   }
  else if( ( t == p->left ) && ( p == n->right ) ) {     // case (4b)
   rotateRight( p );
   t = p;
   }
  else {
   if( ( t == p->left ) && ( p == n->left ) )           // case (5a)
    rotateRight( n );
   else                                                 // case (5b)
    rotateLeft( n );
   p->color = BLACK;
   n->color = RED;
   t = nil();
   }
  }
 }

template< std::totally_ordered K , class V , bool RB >
void SearchTree< K , V , RB >::balanceDelete( Link * t )
{
 while( ( t->parent != nil() ) && ( t->color == BLACK ) ) {  // mended
  Link * p = t->parent;                                       // parent
  const bool L = ( t == p->left );
  Link * Link::* near = L ? &Link::left : &Link::right;
  Link * Link::* far = L ? &Link::right : &Link::left;
  auto rot_near = [ & ]( Link * x ) {    // rotateLeft() if t is left
   return( L ? rotateLeft( x ) : rotateRight( x ) ); };
  auto rot_far = [ & ]( Link * x ) {
   return( L ? rotateRight( x ) : rotateLeft( x ) ); };
  Link * f = p->*far;                                         // sibling
  Link * ns = f->*near;                                       // nephews
  Link * nd = f->*far;
  if( f->color == RED ) {                                     // (1)
   p->color = RED;
   f->color = BLACK;
   rot_near( p );         // t unchanged, then (2), (3) or (4)
   }
  else if( ( ns->color == BLACK ) && ( nd->color == BLACK ) ) {  // (2)
   f->color = RED;
   t = p;
   }
  else if( ( ns->color == RED ) && ( nd->color == BLACK ) ) {    // (3)
   ns->color = BLACK;
   f->color = RED;
   rot_far( f );          // t unchanged, then (4)
   }
  else {                                                      // (4)
   f->color = p->color;
   p->color = BLACK;
   nd->color = BLACK;
   rot_near( p );
   t = f;                 // mended: t = T, the current root
   while( t->parent != nil() )
    t = t->parent;
   }
  }
 t->color = BLACK;        // on the sentinel too, which stays black
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SearchTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
