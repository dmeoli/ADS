/*--------------------------------------------------------------------------*/
/*---------------------- File OrderMaintenanceList.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of OrderMaintenanceList [see `OrderMaintenanceList.h`],
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

namespace ADS {

template< class T >
OrderMaintenanceList< T >::~OrderMaintenanceList()
{
 for( Node * u = f_root ; u ; ) {           // by rotations, with no stack
  if( u->left ) {
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

template< class T >
auto OrderMaintenanceList< T >::first( void ) const -> Element
{
 const Node * u = f_root;
 if( u ) while( u->left ) u = u->left;
 return( ( u && u->dead ) ? next( u ) : u );
 }

template< class T >
auto OrderMaintenanceList< T >::next( Element e ) const -> Element
{
 const Node * u = e;
 do {                                       // the next in symmetric order,
  if( u->right ) {                          // living
   u = u->right;
   while( u->left ) u = u->left;
   }
  else {
   while( u->parent && ( u->parent->right == u ) ) u = u->parent;
   u = u->parent;
   }
  } while( u && u->dead );
 return( u );
 }

template< class T >
void OrderMaintenanceList< T >::rebuild( Node * u )
{
 //! [rebuild]
 Node * p = u->parent;
 const bool wasLeft = p && ( p->left == u );
 const std::uint64_t label = u->label;
 f_buf.clear();
 detail::Rebuild::flatten( u , f_buf );
 const std::size_t all = f_buf.size();
 std::size_t live = 0;
 for( Node * x : f_buf )                    // the dead ones freed
  if( x->dead ) delete x;
  else f_buf[ live++ ] = x;
 f_buf.resize( live );
 f_q -= all - live;
 auto set = []( Node * , std::size_t ) {};
 Node * r = detail::Rebuild::fromArray( f_buf.data() , 0 , live , set );
 if( ! p ) f_root = r;
 else ( wasLeft ? p->left : p->right ) = r;
 if( r ) {                                  // parents and labels, top down
  r->parent = p;
  r->label = label;
  std::vector< Node * > & S = f_buf;
  S.assign( 1 , r );
  while( ! S.empty() ) {
   Node * x = S.back();
   S.pop_back();
   const std::uint64_t h = ( x->label & ( 0 - x->label ) ) >> 1;
   for( Node * c : { x->left , x->right } )
    if( c ) {
     ADS_PRE( h != 0 );                     // height above 63: too many
     c->parent = x;
     c->label = ( c == x->left ) ? x->label - h : x->label + h;
     S.push_back( c );
     }
   }
  }
 //! [rebuild]
 ++f_rebuilds;
 f_work += all;
 }

template< class T >
auto OrderMaintenanceList< T >::addAfter( Element e , const T & v ) -> Element
{
 ADS_PRE( ( ! e || ! e->dead ) && ( f_q < 100000000000ULL ) );
 //! [add]
 Node * u = new Node{ v };
 Node * p = const_cast< Node * >( e );
 bool left;
 if( ! p ) {                                // at the front
  p = f_root;
  if( p ) while( p->left ) p = p->left;
  left = true;
  }
 else {
  if( p->right ) {
   p = p->right;
   while( p->left ) p = p->left;
   left = true;
   }
  else left = false;
  }
 u->parent = p;
 if( ! p ) f_root = u;
 else ( left ? p->left : p->right ) = u;
 ++f_q;
 ++f_live;
 const std::size_t d = p ? std::size_t( depth( p ) ) + 1 : 0;
 while( f_pow.size() <= d ) f_pow.push_back( f_pow.back() * 1.5 );
 if( f_pow[ d ] > double( f_q ) ) {         // too deep: the scapegoat
  Node * c = u;
  std::size_t sc = 1 , h = 1;
  for( Node * w = p ; ; w = w->parent , ++h ) {
   const std::size_t sw =
    sc + 1 + detail::Rebuild::count( w->left == c ? w->right : w->left ,
                                     f_buf );
   if( ( f_pow[ h ] > double( sw ) ) || ! w->parent ) {   // b^h > s( w )
    rebuild( w );
    break;
    }
   c = w;
   sc = sw;
   }
  }
 else if( p ) {
  const std::uint64_t h = ( p->label & ( 0 - p->label ) ) >> 1;
  u->label = left ? p->label - h : p->label + h;
  }
 else
  u->label = kRoot;
 //! [add]
 ADS_INVARIANT( valid() );
 return( u );
 }

template< class T >
void OrderMaintenanceList< T >::remove( Element e )
{
 ADS_PRE( e && ! e->dead );
 const_cast< Node * >( e )->dead = true;
 --f_live;
 if( 2 * f_live < f_q ) rebuild( f_root );  // the dead ones out
 ADS_INVARIANT( valid() );
 }

template< class T >
bool OrderMaintenanceList< T >::valid( void ) const
{
 std::size_t q = 0 , live = 0;
 std::vector< const Node * > S;
 if( f_root ) {
  if( f_root->parent || ( f_root->label != kRoot ) ) return( false );
  S.push_back( f_root );
  }
 while( ! S.empty() ) {
  const Node * x = S.back();
  S.pop_back();
  ++q;
  live += ! x->dead;
  const std::uint64_t h = ( x->label & ( 0 - x->label ) ) >> 1;
  if( x->left && ( ( x->left->parent != x ) ||
                   ( x->left->label != x->label - h ) ) )
   return( false );
  if( x->right && ( ( x->right->parent != x ) ||
                    ( x->right->label != x->label + h ) ) )
   return( false );
  if( std::pow( 1.5 , depth( x ) ) > double( f_q ) * ( 1 + 1e-9 ) )
   return( false );
  if( x->left ) S.push_back( x->left );
  if( x->right ) S.push_back( x->right );
  }
 return( ( q == f_q ) && ( live == f_live ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File OrderMaintenanceList.tpp -------------------*/
/*--------------------------------------------------------------------------*/
