/*--------------------------------------------------------------------------*/
/*--------------------- File PartialRebuildingTree.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PartialRebuildingTree [see `PartialRebuildingTree.h`],
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

template< std::totally_ordered K , class V , RebuildPolicy P >
PartialRebuildingTree< K , V , P >::PartialRebuildingTree(
  PartialRebuildingTree && o ) noexcept
    : f_root( o.f_root ) , f_g( o.f_g ) , f_rebuilds( o.f_rebuilds ) ,
      f_work( o.f_work )
{
 o.f_root = nullptr;
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
PartialRebuildingTree< K , V , P >::~PartialRebuildingTree()
{
 for( Node * u = f_root ; u ; ) {  // by rotations, with no stack
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

template< std::totally_ordered K , class V , RebuildPolicy P >
std::optional< V >
PartialRebuildingTree< K , V , P >::lookup( const K & k ) const
{
 for( const Node * w = f_root ; w ; )
  if( k < w->key )
   w = w->left;
  else if( w->key < k )
   w = w->right;
  else
   return( w->value );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
bool PartialRebuildingTree< K , V , P >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
void PartialRebuildingTree< K , V , P >::rebuild( std::size_t i )
{
 Node * u = f_path[ i ];
 const std::size_t s = u->s;
 auto set = []( Node * x , std::size_t t ) {
  x->s = t;
  if constexpr( P == RebuildPolicy::COUNTDOWN )
   x->t = long( t / 3 );
  };
 f_buf.clear();
 detail::Rebuild::flatten( u , f_buf );
 Node * r = detail::Rebuild::fromArray( f_buf.data() , 0 , f_buf.size() ,
					set );
 if( i == 0 )
  f_root = r;
 else {
  Node * p = f_path[ i - 1 ];
  ( p->left == u ? p->left : p->right ) = r;
  }
 ++f_rebuilds;
 f_work += s;
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
void PartialRebuildingTree< K , V , P >::rebalance( void )
{
 for( std::size_t i = 0 ; i < f_path.size() ; ++i ) {
  Node * u = f_path[ i ];
  bool selected;
  if constexpr( P == RebuildPolicy::WEIGHT )
   selected = ( 3 * std::max( sz( u->left ) , sz( u->right ) ) > 2 * u->s );
  else if constexpr( P == RebuildPolicy::COUNTDOWN )
   selected = ( --u->t <= 0 );
  else
   selected = ( uniformBelow( f_g , u->s ) == 0 );
  if( selected ) {
   rebuild( i );
   break;
   }
  }
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
void PartialRebuildingTree< K , V , P >::insert( const K & k , const V & v )
{
 f_path.clear();
 Node ** link = &f_root;
 while( Node * w = *link ) {
  if( k < w->key )
   link = &w->left;
  else if( w->key < k )
   link = &w->right;
  else {
   w->value = v;
   ADS_POST( contains( k ) );
   return;
   }
  f_path.push_back( w );
  }
 *link = new Node( k , v );
 for( Node * w : f_path )
  ++w->s;
 rebalance();
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
void PartialRebuildingTree< K , V , P >::remove( const K & k )
{
 f_path.clear();
 Node ** link = &f_root;
 Node * u;
 while( ( u = *link ) && ( ( k < u->key ) || ( u->key < k ) ) ) {
  f_path.push_back( u );
  link = ( k < u->key ) ? &u->left : &u->right;
  }
 if( u ) {
  if( u->left && u->right ) {  // the successor takes its place
   f_path.push_back( u );
   Node ** sl = &u->right;
   while( ( *sl )->left ) {
    f_path.push_back( *sl );
    sl = &( *sl )->left;
    }
   Node * w = *sl;
   u->key = std::move( w->key );
   u->value = std::move( w->value );
   link = sl;
   u = w;
   }
  *link = u->left ? u->left : u->right;  // splice
  delete u;
  for( Node * w : f_path )
   --w->s;
  rebalance();
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
auto PartialRebuildingTree< K , V , P >::lowerBoundNode( const K & k ) const
  -> const Node *
{
 const Node * best = nullptr;
 for( const Node * t = f_root ; t ; )
  if( t->key < k )
   t = t->right;
  else {
   best = t;
   if( ! ( k < t->key ) )
    break;
   t = t->left;
   }
 return( best );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
auto PartialRebuildingTree< K , V , P >::lowerBound( const K & k ) const
  -> std::optional< std::pair< K , V > >
{
 const Node * b = lowerBoundNode( k );
 return( b ? std::optional< std::pair< K , V > >(
		std::pair< K , V >( b->key , b->value ) )
	    : std::nullopt );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
const K * PartialRebuildingTree< K , V , P >::findKey( const K & k ) const
{
 const Node * b = lowerBoundNode( k );
 return( ( b && ! ( k < b->key ) ) ? &b->key : nullptr );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
const K *
PartialRebuildingTree< K , V , P >::lowerBoundKey( const K & k ) const
{
 const Node * b = lowerBoundNode( k );
 return( b ? &b->key : nullptr );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
long PartialRebuildingTree< K , V , P >::height( void ) const
{
 long h = -1;  // by levels, with no recursion
 std::vector< const Node * > L , M;
 if( f_root )
  L.push_back( f_root );
 for( ; ! L.empty() ; ++h , L.swap( M ) ) {
  M.clear();
  for( const Node * u : L ) {
   if( u->left )
    M.push_back( u->left );
   if( u->right )
    M.push_back( u->right );
   }
  }
 return( h );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
template< class F >
void PartialRebuildingTree< K , V , P >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this )
  f( k , v );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
bool PartialRebuildingTree< K , V , P >::valid( void ) const
{
 const K * last = nullptr;
 for( const auto & [ k , v ] : *this ) {
  if( last && ! ( *last < k ) )
   return( false );
  last = &k;
  }
 std::vector< const Node * > S;
 if( f_root )
  S.push_back( f_root );
 while( ! S.empty() ) {
  const Node * u = S.back();
  S.pop_back();
  if( u->s != 1 + sz( u->left ) + sz( u->right ) )
   return( false );
  if constexpr( P == RebuildPolicy::WEIGHT )
   if( 3 * std::max( sz( u->left ) , sz( u->right ) ) > 2 * u->s )
    return( false );
  if( u->left )
   S.push_back( u->left );
  if( u->right )
   S.push_back( u->right );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File PartialRebuildingTree.tpp -------------------*/
/*--------------------------------------------------------------------------*/
