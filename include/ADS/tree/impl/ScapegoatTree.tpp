/*--------------------------------------------------------------------------*/
/*------------------------- File ScapegoatTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ScapegoatTree [see `ScapegoatTree.h`], with their
 * contracts.
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

template< std::totally_ordered K , class V , bool StoredSizes >
ScapegoatTree< K , V , StoredSizes >::ScapegoatTree( double b , Sizes sizes ,
						   Rebuilds rebuilds ,
						   Scapegoats scapegoats )
    : f_b( b ) , f_sizes( sizes ) , f_rebuild( rebuilds ) ,
      f_goat( scapegoats )
{
 ADS_PRE( ( b > 1 ) && ( b < 2 ) );
 f_pow.push_back( 1 );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
ScapegoatTree< K , V , StoredSizes >::ScapegoatTree(
  ScapegoatTree && o ) noexcept
    : f_root( o.f_root ) , f_n( o.f_n ) , f_q( o.f_q ) , f_b( o.f_b ) ,
      f_sizes( o.f_sizes ) , f_rebuild( o.f_rebuild ) , f_goat( o.f_goat ) ,
      f_pow( std::move( o.f_pow ) ) , f_rebuilds( o.f_rebuilds ) ,
      f_work( o.f_work )
{
 o.f_root = nullptr;
 o.f_n = o.f_q = 0;
 o.f_pow.assign( 1 , 1.0 );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
ScapegoatTree< K , V , StoredSizes >::~ScapegoatTree()
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

template< std::totally_ordered K , class V , bool StoredSizes >
bool ScapegoatTree< K , V , StoredSizes >::tooDeep( std::size_t d )
{
 while( f_pow.size() <= d )
  f_pow.push_back( f_pow.back() * f_b );
 return( f_pow[ d ] > double( f_q ) );  // b^d > q: d > log_b q
 }

template< std::totally_ordered K , class V , bool StoredSizes >
std::size_t ScapegoatTree< K , V , StoredSizes >::sizeOf( Node * u )
{
 if constexpr( StoredSizes )
  return( u ? u->s : 0 );
 else
  return( detail::Rebuild::count( u , f_buf ) );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
void ScapegoatTree< K , V , StoredSizes >::rebuild( Node * u , std::size_t s ,
						  Node * p )
{
 auto set = []( Node * x , std::size_t t ) {
  if constexpr( StoredSizes )
   x->s = t;
  else
   ( void )x , ( void )t;
  };
 Node * r;
 if( f_rebuild == Rebuilds::ARRAY ) {
  f_buf.clear();
  detail::Rebuild::flatten( u , f_buf );
  r = detail::Rebuild::fromArray( f_buf.data() , 0 , f_buf.size() , set );
  }
 else {
  Node * h = detail::Rebuild::toList( u , static_cast< Node * >( nullptr ) );
  r = detail::Rebuild::fromList( s , h , set );
  }
 if( ! p )
  f_root = r;
 else
  ( p->left == u ? p->left : p->right ) = r;
 ++f_rebuilds;
 f_work += s;
 }

template< std::totally_ordered K , class V , bool StoredSizes >
std::optional< V >
ScapegoatTree< K , V , StoredSizes >::lookup( const K & k ) const
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

template< std::totally_ordered K , class V , bool StoredSizes >
bool ScapegoatTree< K , V , StoredSizes >::contains( const K & k ) const
{
 return( lookup( k ).has_value() );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
void ScapegoatTree< K , V , StoredSizes >::insert( const K & k , const V & v )
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
 Node * u = *link = new Node( k , v );
 ++f_n;
 ++f_q;
 if constexpr( StoredSizes )
  for( Node * w : f_path )
   ++w->s;
 const std::size_t d = f_path.size();
 if( tooDeep( d ) ) {  // climb to the scapegoat
  std::size_t su = 1 , i = d;
  for( ; ; ) {
   ADS_INVARIANT( i > 0 );  // a scapegoat exists
   Node * w = f_path[ --i ];
   std::size_t sw;
   if constexpr( StoredSizes )
    sw = w->s;
   else if( f_sizes == Sizes::REUSE )
    sw = su + 1 + sizeOf( w->left == u ? w->right : w->left );
   else
    sw = sizeOf( w );
   if( ( f_goat == Scapegoats::HEIGHT )       // b^H > s( w ), or
	 ? ( f_pow[ d - i ] > double( sw ) )  // s( u ) > s( w ) / b
	 : ( ( f_b * double( su ) > double( sw ) ) || ( i == 0 ) ) ) {
    rebuild( w , sw , i ? f_path[ i - 1 ] : nullptr );
    break;
    }
   u = w;
   su = sw;
   }
  }
 ADS_POST( contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
void ScapegoatTree< K , V , StoredSizes >::remove( const K & k )
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
  if constexpr( StoredSizes )
   for( Node * w : f_path )
    --w->s;
  --f_n;
  if( 2 * f_n < f_q ) {  // too few: all rebuilt
   if( f_root )
    rebuild( f_root , f_n , nullptr );
   f_q = f_n;
   }
  }
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( valid() );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
auto ScapegoatTree< K , V , StoredSizes >::lowerBoundNode( const K & k ) const
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

template< std::totally_ordered K , class V , bool StoredSizes >
auto ScapegoatTree< K , V , StoredSizes >::lowerBound( const K & k ) const
  -> std::optional< std::pair< K , V > >
{
 const Node * b = lowerBoundNode( k );
 return( b ? std::optional< std::pair< K , V > >(
		std::pair< K , V >( b->key , b->value ) )
	    : std::nullopt );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
const K * ScapegoatTree< K , V , StoredSizes >::findKey( const K & k ) const
{
 const Node * b = lowerBoundNode( k );
 return( ( b && ! ( k < b->key ) ) ? &b->key : nullptr );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
const K *
ScapegoatTree< K , V , StoredSizes >::lowerBoundKey( const K & k ) const
{
 const Node * b = lowerBoundNode( k );
 return( b ? &b->key : nullptr );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
long ScapegoatTree< K , V , StoredSizes >::height( void ) const
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

template< std::totally_ordered K , class V , bool StoredSizes >
template< class F >
void ScapegoatTree< K , V , StoredSizes >::forEach( F && f ) const
{
 for( const auto & [ k , v ] : *this )
  f( k , v );
 }

template< std::totally_ordered K , class V , bool StoredSizes >
bool ScapegoatTree< K , V , StoredSizes >::valid( void ) const
{
 if( ( f_q < f_n ) || ( 2 * f_n < f_q ) )
  return( false );
 const long h = height();
 if( ( h >= 0 ) &&  // h <= log_b q
     ( std::pow( f_b , double( h ) ) > double( f_q ) * ( 1 + 1e-9 ) ) )
  return( false );
 std::size_t n = 0;
 const K * last = nullptr;
 for( const auto & [ k , v ] : *this ) {
  if( last && ! ( *last < k ) )
   return( false );
  last = &k;
  ++n;
  }
 if( n != f_n )
  return( false );
 if constexpr( StoredSizes ) {
  std::vector< const Node * > S;
  if( f_root )
   S.push_back( f_root );
  while( ! S.empty() ) {
   const Node * u = S.back();
   S.pop_back();
   if( u->s !=
       1 + ( u->left ? u->left->s : 0 ) + ( u->right ? u->right->s : 0 ) )
    return( false );
   if( u->left )
    S.push_back( u->left );
   if( u->right )
    S.push_back( u->right );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ScapegoatTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
