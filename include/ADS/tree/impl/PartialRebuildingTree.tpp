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
 return( searchTreeLookup( f_root , k ) );
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
 Node ** link = searchTreeLink( &f_root , k , f_path );
 if( Node * w = *link ) {
  w->value = v;
  ADS_POST( contains( k ) );
  return;
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
 if( Node ** link = searchTreeLink( &f_root , k , f_path ); *link ) {
  delete searchTreeSplice( link , f_path );
  for( Node * w : f_path )
   --w->s;
  rebalance();
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
auto PartialRebuildingTree< K , V , P >::lowerBound( const K & k ) const
  -> std::optional< std::pair< K , V > >
{
 return( searchTreeLowerBoundEntry( f_root , k ) );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
const K * PartialRebuildingTree< K , V , P >::findKey( const K & k ) const
{
 return( searchTreeFindKey( f_root , k ) );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
const K *
PartialRebuildingTree< K , V , P >::lowerBoundKey( const K & k ) const
{
 return( searchTreeLowerBoundKey( f_root , k ) );
 }

template< std::totally_ordered K , class V , RebuildPolicy P >
long PartialRebuildingTree< K , V , P >::height( void ) const
{
 return( treeHeight( f_root ) );
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
