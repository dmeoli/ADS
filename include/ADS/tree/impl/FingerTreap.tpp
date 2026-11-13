/*--------------------------------------------------------------------------*/
/*-------------------------- File FingerTreap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FingerTreap [see `FingerTreap.h`], with their contracts.
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
FingerTreap< K , V >::~FingerTreap()
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

template< std::totally_ordered K , class V >
auto FingerTreap< K , V >::search( const K & k , Node * u ) const -> Node *
{
 //! [fingerFind]
 if( ! u ) u = f_root;
 while( u && u->parent && ( ( k < u->lo->key ) || ( u->hi->key < k ) ) )
  u = u->parent;                            // up until the subtree spans k
 for( Node * w = u ; w ; ) {                // then down as usual
  u = w;
  if( k < w->key ) w = w->left;
  else if( w->key < k ) w = w->right;
  else break;
  }
 return( u );
 //! [fingerFind]
 }

template< std::totally_ordered K , class V >
std::optional< V > FingerTreap< K , V >::lookup( const K & k ) const
{
 const Node * u = search( k , nullptr );
 return( u && !( u->key < k ) && !( k < u->key ) ? std::optional< V >(
          u->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
std::optional< V > FingerTreap< K , V >::fingerLookup( const K & k )
{
 f_finger = search( k , f_finger );
 const Node * u = f_finger;
 return( u && !( u->key < k ) && !( k < u->key ) ? std::optional< V >(
          u->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
std::optional< V > FingerTreap< K , V >::lookupFrom( const K & k ,
                                                       Finger f ) const
{
 const Node * u = search( k , f.f_u );
 return( u && !( u->key < k ) && !( k < u->key ) ? std::optional< V >(
          u->value ) : std::nullopt );
 }

template< std::totally_ordered K , class V >
auto FingerTreap< K , V >::finger( const K & k ) const -> Finger
{
 return( Finger( search( k , nullptr ) ) );
 }

template< std::totally_ordered K , class V >
bool FingerTreap< K , V >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V >
void FingerTreap< K , V >::rotateLeft( Node * u )
{
 Node * w = u->right;
 w->parent = u->parent;
 if( w->parent )
  ( w->parent->left == u ? w->parent->left : w->parent->right ) = w;
 else
  f_root = w;
 u->right = w->left;
 if( u->right ) u->right->parent = u;
 u->parent = w;
 w->left = u;
 update( u );
 update( w );
 }

template< std::totally_ordered K , class V >
void FingerTreap< K , V >::rotateRight( Node * u )
{
 Node * w = u->left;
 w->parent = u->parent;
 if( w->parent )
  ( w->parent->left == u ? w->parent->left : w->parent->right ) = w;
 else
  f_root = w;
 u->left = w->right;
 if( u->left ) u->left->parent = u;
 u->parent = w;
 w->right = u;
 update( u );
 update( w );
 }

template< std::totally_ordered K , class V >
void FingerTreap< K , V >::insert( const K & k , const V & v )
{
 //! [add]
 Node * p = search( k , nullptr );
 if( p && !( p->key < k ) && !( k < p->key ) )
  p->value = v;
 else {
  Node * u = new Node{ k , v , f_g() };
  u->lo = u->hi = u;
  u->parent = p;
  if( ! p ) f_root = u;
  else ( k < p->key ? p->left : p->right ) = u;
  while( u->parent && ( u->parent->priority < u->priority ) )
   if( u->parent->right == u ) rotateLeft( u->parent );   // up the heap
   else rotateRight( u->parent );
  for( Node * w = u->parent ; w ; w = w->parent ) update( w );
  ++f_size;
  }
 //! [add]
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
void FingerTreap< K , V >::remove( const K & k )
{
 //! [remove]
 Node * u = search( k , nullptr );
 if( u && !( u->key < k ) && !( k < u->key ) ) {
  while( u->left && u->right )              // down, the child of larger
   if( u->left->priority > u->right->priority ) rotateRight( u );
   else rotateLeft( u );
  Node * c = u->left ? u->left : u->right;  // splice
  Node * p = u->parent;
  if( c ) c->parent = p;
  if( ! p ) f_root = c;
  else ( p->left == u ? p->left : p->right ) = c;
  delete u;
  for( Node * w = p ; w ; w = w->parent ) update( w );
  --f_size;
  f_finger = nullptr;
  }
 //! [remove]
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V >
auto FingerTreap< K , V >::lowerBound( const K & k ) const
 -> std::optional< std::pair< K , V > >
{
 const Node * best = nullptr;
 for( const Node * t = f_root ; t ; )
  if( t->key < k ) t = t->right;
  else {
   best = t;
   if( !( k < t->key ) ) break;
   t = t->left;
   }
 return( best ? std::optional< std::pair< K , V > >(
                 std::pair< K , V >( best->key , best->value ) )
              : std::nullopt );
 }

template< std::totally_ordered K , class V >
std::pair< K , V > FingerTreap< K , V >::select( std::size_t i ) const
{
 ADS_PRE( i < f_size );
 const Node * t = f_root;
 for( ; ; ) {
  const std::size_t l = t->left ? t->left->size : 0;
  if( i < l ) t = t->left;
  else if( i == l ) break;
  else {
   i -= l + 1;
   t = t->right;
   }
  }
 return( std::pair< K , V >( t->key , t->value ) );
 }

template< std::totally_ordered K , class V >
std::size_t FingerTreap< K , V >::rank( const K & k ) const
{
 std::size_t r = 0;
 for( const Node * t = f_root ; t ; )
  if( t->key < k ) {
   r += ( t->left ? t->left->size : 0 ) + 1;
   t = t->right;
   }
  else
   t = t->left;
 return( r );
 }

template< std::totally_ordered K , class V >
int FingerTreap< K , V >::height( void ) const
{
 int h = -1;                                // by levels, with no recursion
 std::vector< const Node * > L , M;
 if( f_root ) L.push_back( f_root );
 for( ; ! L.empty() ; ++h , L.swap( M ) ) {
  M.clear();
  for( const Node * u : L ) {
   if( u->left ) M.push_back( u->left );
   if( u->right ) M.push_back( u->right );
   }
  }
 return( h );
 }

template< std::totally_ordered K , class V >
template< class F >
void FingerTreap< K , V >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this ) f( k , v );
 }

template< std::totally_ordered K , class V >
bool FingerTreap< K , V >::valid( void ) const
{
 if( f_root && f_root->parent ) return( false );
 if( ( f_root ? f_root->size : 0 ) != f_size ) return( false );
 std::vector< std::pair< const Node * , std::pair< const K * , const K * > > >
  S;
 if( f_root ) S.push_back( { f_root , { nullptr , nullptr } } );
 while( ! S.empty() ) {
  const auto [ u , b ] = S.back();
  S.pop_back();
  if( ( b.first && !( *b.first < u->key ) ) ||
      ( b.second && !( u->key < *b.second ) ) )
   return( false );
  const std::size_t s = 1 + ( u->left ? u->left->size : 0 ) +
                        ( u->right ? u->right->size : 0 );
  if( ( u->size != s ) ||
      ( u->lo != ( u->left ? u->left->lo : u ) ) ||
      ( u->hi != ( u->right ? u->right->hi : u ) ) )
   return( false );
  for( const Node * c : { u->left , u->right } )
   if( c && ( ( c->parent != u ) || ( c->priority > u->priority ) ) )
    return( false );
  if( u->left ) S.push_back( { u->left , { b.first , &u->key } } );
  if( u->right ) S.push_back( { u->right , { &u->key , b.second } } );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File FingerTreap.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
