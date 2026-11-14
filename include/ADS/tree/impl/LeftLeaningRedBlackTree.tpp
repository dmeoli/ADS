/*--------------------------------------------------------------------------*/
/*-------------------- File LeftLeaningRedBlackTree.tpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LeftLeaningRedBlackTree [see
 * `LeftLeaningRedBlackTree.h`], with their contracts.
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
LeftLeaningRedBlackTree< K , V >::~LeftLeaningRedBlackTree()
{
 for( Link * u = f_root ; u ; ) {           // by rotations, with no stack
  if( u->left ) {
   Link * l = u->left;
   u->left = l->right;
   l->right = u;
   u = l;
   }
  else {
   Link * r = u->right;
   delete static_cast< Node * >( u );
   u = r;
   }
  }
 }

template< std::totally_ordered K , class V >
auto LeftLeaningRedBlackTree< K , V >::findLast( const K & k ) const -> Link *
{
 //! [find]
 Link * w = f_root , * prev = nullptr;
 while( w ) {
  prev = w;
  if( k < key( w ) ) w = w->left;
  else if( key( w ) < k ) w = w->right;
  else return( w );
  }
 return( prev );
 //! [find]
 }

template< std::totally_ordered K , class V >
std::optional< V > LeftLeaningRedBlackTree< K , V >::lookup(
 const K & k ) const
{
 const Link * u = findLast( k );
 return( u && !( key( u ) < k ) && !( k < key( u ) )
          ? std::optional< V >( static_cast< const Node * >( u )->value )
          : std::nullopt );
 }

template< std::totally_ordered K , class V >
bool LeftLeaningRedBlackTree< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::rotateLeft( Link * u )
{
 Link * w = u->right;
 Link * & at = place( u );
 at = w;
 w->parent = u->parent;
 u->right = w->left;
 if( u->right ) u->right->parent = u;
 w->left = u;
 u->parent = w;
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::rotateRight( Link * u )
{
 Link * w = u->left;
 Link * & at = place( u );
 at = w;
 w->parent = u->parent;
 u->left = w->right;
 if( u->left ) u->left->parent = u;
 w->right = u;
 u->parent = w;
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::addFixup( Link * u )
{
 //! [addFixup]
 while( u->colour == kRed ) {
  if( u == f_root ) {
   u->colour = kBlack;
   return;
   }
  Link * w = u->parent;
  if( colour( w->left ) == kBlack ) {       // left-leaning restored
   flipLeft( w );
   u = w;
   w = u->parent;
   }
  if( w->colour == kBlack )
   return;
  Link * g = w->parent;                     // w red: g black
  if( colour( g->right ) == kBlack ) {
   flipRight( g );
   return;
   }
  pushBlack( g );                           // a 2-4 node split
  u = g;
  }
 //! [addFixup]
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::insert( const K & k , const V & v )
{
 //! [add]
 Link * p = findLast( k );
 if( p && !( key( p ) < k ) && !( k < key( p ) ) )
  static_cast< Node * >( p )->value = v;
 else {
  Link * u = new Node( k , v );
  u->parent = p;
  if( ! p ) f_root = u;
  else ( k < key( p ) ? p->left : p->right ) = u;
  ++f_n;
  addFixup( u );
  }
 //! [add]
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
auto LeftLeaningRedBlackTree< K , V >::removeCase2( Link * u ) -> Link *
{
 Link * w = u->parent;
 Link * v = w->right;
 pullBlack( w );
 flipLeft( w );
 Link * q = w->right;
 if( colour( q ) == kRed ) {
  rotateLeft( w );
  flipRight( v );
  pushBlack( q );
  if( colour( v->right ) == kRed ) flipLeft( v );
  return( q );
  }
 return( v );
 }

template< std::totally_ordered K , class V >
auto LeftLeaningRedBlackTree< K , V >::removeCase3( Link * u ) -> Link *
{
 Link * w = u->parent;
 Link * v = w->left;
 pullBlack( w );
 flipRight( w );
 Link * q = w->left;
 if( colour( q ) == kRed ) {
  rotateRight( w );
  flipLeft( v );
  pushBlack( q );
  return( q );
  }
 if( colour( v->left ) == kRed ) {
  pushBlack( v );
  return( v );
  }
 flipLeft( v );
 return( w );
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::removeFixup( Link * u )
{
 //! [removeFixup]
 while( u->colour > kBlack ) {
  if( u == f_root )
   u->colour = kBlack;
  else if( colour( u->parent->left ) == kRed )
   flipRight( u->parent );                  // case 1, then case 3
  else if( u == u->parent->left )
   u = removeCase2( u );
  else
   u = removeCase3( u );
  }
 if( u != f_root ) {                        // the left-leaning property
  Link * w = u->parent;
  if( ( colour( w->right ) == kRed ) && ( colour( w->left ) == kBlack ) )
   flipLeft( w );
  }
 //! [removeFixup]
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::remove( const K & k )
{
 //! [remove]
 Link * u = findLast( k );
 if( u && !( key( u ) < k ) && !( k < key( u ) ) ) {
  Link * w = u->right;                      // w: the node spliced
  if( ! w ) {
   w = u;
   u = w->left;
   }
  else {
   while( w->left ) w = w->left;
   static_cast< Node * >( u )->key =
    std::move( static_cast< Node * >( w )->key );
   static_cast< Node * >( u )->value =
    std::move( static_cast< Node * >( w )->value );
   u = w->right;
   }
  Link phantom;                             // in place of a nil child
  if( ! u ) {
   phantom.colour = kBlack;
   u = &phantom;
   }
  place( w ) = u;
  u->parent = w->parent;                    // also for the phantom
  u->colour += w->colour;
  delete static_cast< Node * >( w );
  --f_n;
  removeFixup( u );
  if( phantom.parent || ( f_root == &phantom ) )
   place( &phantom ) = nullptr;             // the phantom out
  }
 //! [remove]
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
void LeftLeaningRedBlackTree< K , V >::absorb( LeftLeaningRedBlackTree & t )
{
 if( ( &t == this ) || ! t.f_root )
  return;
 [[maybe_unused]] auto smaller = [ & ]( void ) {   // max( this ) < min( t )
  const Link * a = f_root , * b = t.f_root;
  while( a->right ) a = a->right;
  while( b->left ) b = b->left;
  return( key( a ) < key( b ) );
  };
 ADS_PRE( ! f_root || smaller() );
 //! [absorb]
 if( ! f_root ) {                           // nothing here: all of t
  std::swap( f_root , t.f_root );
  std::swap( f_n , t.f_n );
  ADS_INVARIANT( valid() );
  return;
  }
 Link * m = t.f_root;                       // the smallest of t, out
 while( m->left ) m = m->left;
 Node * x = new Node( key( m ) , static_cast< Node * >( m )->value );
 t.remove( x->key );                       // a copy: m is freed
 if( ! t.f_root ) {                         // t had one key: a leaf at the
  Link * p = f_root;                        // end of the right spine
  while( p->right ) p = p->right;
  p->right = x;
  x->parent = p;
  ++f_n;
  addFixup( x );
  ADS_INVARIANT( valid() );
  return;
  }
 const long h1 = blackHeight() , h2 = t.blackHeight();
 Link * r1 = f_root , * r2 = t.f_root;
 f_n += t.f_n + 1;
 t.f_root = nullptr;
 t.f_n = 0;
 Link * c;
 if( h1 >= h2 ) {                           // down the right spine of r1 to
  c = r1;                                   // the black height of r2
  for( long h = h1 ; ( c->colour == kRed ) || ( h > h2 ) ; c = c->right )
   if( c->colour == kBlack ) --h;
  x->parent = c->parent;
  place( c ) = x;
  x->left = c;
  x->right = r2;
  }
 else {                                     // down the left spine of r2
  c = r2;
  for( long h = h2 ; ( c->colour == kRed ) || ( h > h1 ) ; c = c->left )
   if( c->colour == kBlack ) --h;
  x->parent = c->parent;
  if( c == r2 ) f_root = x;
  else {
   f_root = r2;
   c->parent->left = x;
   }
  x->left = r1;
  x->right = c;
  }
 x->left->parent = x;
 x->right->parent = x;
 x->colour = kRed;
 addFixup( x );
 //! [absorb]
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
auto LeftLeaningRedBlackTree< K , V >::lowerBoundNode( const K & k ) const
 -> const Link *
{
 const Link * best = nullptr;
 for( const Link * t = f_root ; t ; )
  if( key( t ) < k ) t = t->right;
  else {
   best = t;
   if( !( k < key( t ) ) ) break;
   t = t->left;
   }
 return( best );
 }

template< std::totally_ordered K , class V >
auto LeftLeaningRedBlackTree< K , V >::lowerBound( const K & k ) const
 -> std::optional< std::pair< K , V > >
{
 const Link * best = lowerBoundNode( k );
 if( ! best )
  return( std::nullopt );
 const Node * b = static_cast< const Node * >( best );
 return( std::pair< K , V >( b->key , b->value ) );
 }

template< std::totally_ordered K , class V >
const K * LeftLeaningRedBlackTree< K , V >::findKey( const K & k ) const
{
 const Link * b = lowerBoundNode( k );
 return( ( b && !( k < key( b ) ) ) ? &key( b ) : nullptr );
 }

template< std::totally_ordered K , class V >
const K * LeftLeaningRedBlackTree< K , V >::lowerBoundKey(
 const K & k ) const
{
 const Link * b = lowerBoundNode( k );
 return( b ? &key( b ) : nullptr );
 }

template< std::totally_ordered K , class V >
long LeftLeaningRedBlackTree< K , V >::height( void ) const
{
 long h = -1;                               // by levels, with no recursion
 std::vector< const Link * > L , M;
 if( f_root ) L.push_back( f_root );
 for( ; ! L.empty() ; ++h , L.swap( M ) ) {
  M.clear();
  for( const Link * u : L ) {
   if( u->left ) M.push_back( u->left );
   if( u->right ) M.push_back( u->right );
   }
  }
 return( h );
 }

template< std::totally_ordered K , class V >
long LeftLeaningRedBlackTree< K , V >::blackHeight( void ) const
{
 long h = 0;
 for( const Link * u = f_root ; u ; u = u->left )
  h += ( u->colour == kBlack );
 return( h );
 }

template< std::totally_ordered K , class V >
auto LeftLeaningRedBlackTree< K , V >::twoFourDegrees( void ) const
 -> std::array< std::size_t , 3 >
{
 std::array< std::size_t , 3 > D = { 0 , 0 , 0 };
 std::vector< const Link * > S;
 if( f_root ) S.push_back( f_root );
 while( ! S.empty() ) {
  const Link * u = S.back();
  S.pop_back();
  if( u->colour == kBlack )                 // a 2-4 node: u and its red
   ++D[ ( colour( u->left ) == kRed ) +     // children
        ( colour( u->right ) == kRed ) ];
  if( u->left ) S.push_back( u->left );
  if( u->right ) S.push_back( u->right );
  }
 return( D );
 }

template< std::totally_ordered K , class V >
template< class F >
void LeftLeaningRedBlackTree< K , V >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this ) f( k , v );
 }

template< std::totally_ordered K , class V >
bool LeftLeaningRedBlackTree< K , V >::valid( void ) const
{
 if( f_root && ( f_root->parent || ( f_root->colour != kBlack ) ) )
  return( false );
 const long bh = blackHeight();
 std::size_t n = 0;
 struct Item { const Link * u; long b; const K * lo , * hi; };
 std::vector< Item > S;
 if( f_root ) S.push_back( Item{ f_root , 0 , nullptr , nullptr } );
 while( ! S.empty() ) {
  const Item x = S.back();
  S.pop_back();
  const Link * u = x.u;
  ++n;
  if( ( u->colour != kRed ) && ( u->colour != kBlack ) ) return( false );
  if( ( x.lo && !( *x.lo < key( u ) ) ) ||
      ( x.hi && !( key( u ) < *x.hi ) ) )
   return( false );
  const long b = x.b + ( u->colour == kBlack );
  if( ( u->colour == kRed ) &&                       // no red edge
      ( ( colour( u->left ) == kRed ) || ( colour( u->right ) == kRed ) ) )
   return( false );
  if( ( colour( u->left ) == kBlack ) && ( colour( u->right ) == kRed ) )
   return( false );                                  // left-leaning
  for( const Link * c : { u->left , u->right } )
   if( c && ( c->parent != u ) ) return( false );
  if( ( ! u->left || ! u->right ) && ( b != bh ) )   // the black height
   return( false );
  if( u->left ) S.push_back( Item{ u->left , b , x.lo , &key( u ) } );
  if( u->right ) S.push_back( Item{ u->right , b , &key( u ) , x.hi } );
  }
 return( n == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------ End File LeftLeaningRedBlackTree.tpp ------------------*/
/*--------------------------------------------------------------------------*/
