/*--------------------------------------------------------------------------*/
/*----------------------------- File BTree.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BTree [see BTree.h], with their contracts.
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

template< std::totally_ordered K , class V , class P >
template< class... O >
BTree< K , V , P >::BTree( int t , O &... o ) : f_t( t ) , f_sub( o... )
{
 ADS_PRE( t >= 2 );
 }

template< std::totally_ordered K , class V , class P >
std::size_t BTree< K , V , P >::place( const Node * x , const K & key )
{
 return( std::size_t( std::lower_bound( x->k.begin() , x->k.end() , key ) -
		       x->k.begin() ) );
 }

template< std::totally_ordered K , class V , class P >
std::optional< V > BTree< K , V , P >::lookup( const K & key ) const
{
 for( const Node * x = f_root.get() ; x ; ) {
  const std::size_t i = place( x , key );
  if( ( i < x->k.size() ) && ! ( key < x->k[ i ] ) )
   return( x->v[ i ] );
  if( x->leaf() )
   break;
  x = x->c[ i ].get();
  }
 return( std::nullopt );
 }

template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::split( Node * x , std::size_t i )
{
 Node * y = x->c[ i ].get();  // 2 t - 1 keys
 auto z = std::make_unique< Node >();
 const std::size_t t = std::size_t( f_t );
 z->k.assign( y->k.begin() + t , y->k.end() );
 z->v.assign( y->v.begin() + t , y->v.end() );
 if( ! y->leaf() )
  for( std::size_t j = t ; j < y->c.size() ; ++j )
   z->c.push_back( std::move( y->c[ j ] ) );
 x->k.insert( x->k.begin() + i , y->k[ t - 1 ] );  // the median goes up
 x->v.insert( x->v.begin() + i , y->v[ t - 1 ] );
 x->c.insert( x->c.begin() + i + 1 , std::move( z ) );
 y->k.resize( t - 1 );
 y->v.resize( t - 1 );
 if( ! y->leaf() )
  y->c.resize( t );
 }

template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::insert( const K & key , const V & val )
{
 if constexpr( Subject::template listened< Inserted > ||
	       Subject::template listened< Assigned > )
  if( f_sub.template live< Inserted >() || f_sub.template live< Assigned >() )
    [[unlikely]] {
   if constexpr( Subject::dynamic )
    insertCold( key , val );
   else
    insertTold( key , val );
   return;
   }
 const std::size_t full = std::size_t( 2 * f_t - 1 );
 if( ! f_root )
  f_root = std::make_unique< Node >();
 if( f_root->k.size() == full ) {  // the root splits: one level up
  auto s = std::make_unique< Node >();
  s->c.push_back( std::move( f_root ) );
  f_root = std::move( s );
  split( f_root.get() , 0 );
  }
 Node * x = f_root.get();
 for( ; ; ) {
  std::size_t i = place( x , key );
  if( ( i < x->k.size() ) && ! ( key < x->k[ i ] ) ) {
   x->v[ i ] = val;  // the new value replaces it
   break;
   }
  if( x->leaf() ) {
   x->k.insert( x->k.begin() + i , key );
   x->v.insert( x->v.begin() + i , val );
   ++f_size;
   break;
   }
  if( x->c[ i ]->k.size() == full ) {
   split( x , i );
   if( x->k[ i ] < key )
    ++i;
   else if( ! ( key < x->k[ i ] ) ) {
    x->v[ i ] = val;
    break;
    }
   }
  x = x->c[ i ].get();
  }
 ADS_POST( contains( key ) );
 ADS_INVARIANT( [ & ]( void ) {
  int d = -1;
  return( valid( f_root.get() , nullptr , nullptr , 0 , d , true ) );
  }() );
 }
template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::insertPlain( const K & key , const V & val )
{
 const std::size_t full = std::size_t( 2 * f_t - 1 );
 if( ! f_root )
  f_root = std::make_unique< Node >();
 if( f_root->k.size() == full ) {  // the root splits: one level up
  auto s = std::make_unique< Node >();
  s->c.push_back( std::move( f_root ) );
  f_root = std::move( s );
  split( f_root.get() , 0 );
  }
 Node * x = f_root.get();
 for( ; ; ) {
  std::size_t i = place( x , key );
  if( ( i < x->k.size() ) && ! ( key < x->k[ i ] ) ) {
   x->v[ i ] = val;  // the new value replaces it
   break;
   }
  if( x->leaf() ) {
   x->k.insert( x->k.begin() + i , key );
   x->v.insert( x->v.begin() + i , val );
   ++f_size;
   break;
   }
  if( x->c[ i ]->k.size() == full ) {
   split( x , i );
   if( x->k[ i ] < key )
    ++i;
   else if( ! ( key < x->k[ i ] ) ) {
    x->v[ i ] = val;
    break;
    }
   }
  x = x->c[ i ].get();
  }
 ADS_POST( contains( key ) );
 ADS_INVARIANT( [ & ]( void ) {
  int d = -1;
  return( valid( f_root.get() , nullptr , nullptr , 0 , d , true ) );
  }() );
 }

template< std::totally_ordered K , class V , class P >
std::size_t BTree< K , V , P >::fill( Node * x , std::size_t i )
{
 const std::size_t t = std::size_t( f_t );
 Node * y = x->c[ i ].get();
 if( y->k.size() >= t )
  return( i );
 if( ( i > 0 ) && ( x->c[ i - 1 ]->k.size() >= t ) ) {  // borrow from the
  Node * l = x->c[ i - 1 ].get();                       // left sibling
  y->k.insert( y->k.begin() , x->k[ i - 1 ] );
  y->v.insert( y->v.begin() , x->v[ i - 1 ] );
  x->k[ i - 1 ] = l->k.back();
  x->v[ i - 1 ] = l->v.back();
  l->k.pop_back();
  l->v.pop_back();
  if( ! l->leaf() ) {
   y->c.insert( y->c.begin() , std::move( l->c.back() ) );
   l->c.pop_back();
   }
  return( i );
  }
 if( ( i + 1 < x->c.size() ) && ( x->c[ i + 1 ]->k.size() >= t ) ) {
  Node * r = x->c[ i + 1 ].get();  // borrow from the right sibling
  y->k.push_back( x->k[ i ] );
  y->v.push_back( x->v[ i ] );
  x->k[ i ] = r->k.front();
  x->v[ i ] = r->v.front();
  r->k.erase( r->k.begin() );
  r->v.erase( r->v.begin() );
  if( ! r->leaf() ) {
   y->c.push_back( std::move( r->c.front() ) );
   r->c.erase( r->c.begin() );
   }
  return( i );
  }
 const std::size_t j = i + 1 < x->c.size() ? i : i - 1;
 merge( x , j );  // with a sibling of t - 1 keys
 return( j );
 }

template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::merge( Node * x , std::size_t j )
{
 Node *a = x->c[ j ].get() , *b = x->c[ j + 1 ].get();
 a->k.push_back( x->k[ j ] );  // the key between them down
 a->v.push_back( x->v[ j ] );
 a->k.insert( a->k.end() , b->k.begin() , b->k.end() );
 a->v.insert( a->v.end() , b->v.begin() , b->v.end() );
 for( auto & c : b->c )
  a->c.push_back( std::move( c ) );
 x->k.erase( x->k.begin() + j );
 x->v.erase( x->v.begin() + j );
 x->c.erase( x->c.begin() + j + 1 );
 }

template< std::totally_ordered K , class V , class P >
bool BTree< K , V , P >::remove( Node * x , const K & key )
{
 const std::size_t t = std::size_t( f_t );
 for( ; ; ) {
  const std::size_t i = place( x , key );
  const bool here = ( i < x->k.size() ) && ! ( key < x->k[ i ] );
  if( x->leaf() ) {
   if( ! here )
    return( false );
   x->k.erase( x->k.begin() + i );
   x->v.erase( x->v.begin() + i );
   return( true );
   }
  if( here ) {
   if( x->c[ i ]->k.size() >= t ) {  // the predecessor replaces it
    Node * p = x->c[ i ].get();
    while( ! p->leaf() )
     p = p->c.back().get();
    x->k[ i ] = p->k.back();
    x->v[ i ] = p->v.back();
    const K pk = p->k.back();
    x = x->c[ i ].get();
    return( remove( x , pk ) );
    }
   if( x->c[ i + 1 ]->k.size() >= t ) {  // the successor replaces it
    Node * s = x->c[ i + 1 ].get();
    while( ! s->leaf() )
     s = s->c.front().get();
    x->k[ i ] = s->k.front();
    x->v[ i ] = s->v.front();
    const K sk = s->k.front();
    x = x->c[ i + 1 ].get();
    return( remove( x , sk ) );
    }
   merge( x , i );        // both children have t - 1
   x = x->c[ i ].get();  // keys: merged around it
   continue;
   }
  x = x->c[ fill( x , i ) ].get();  // down to a child of >= t keys
  }
 }

template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::remove( const K & key )
{
 if constexpr( Subject::template listened< Erased > )
  if( f_sub.template live< Erased >() ) [[unlikely]] {
   bool there;  // the erasure told, before the removal below
   if constexpr( Subject::dynamic )
    there = removeCold( key );
   else
    there = removeTold( key );
   if( ! there )
    return;
   }
 if( f_root && remove( f_root.get() , key ) )
  --f_size;
 if( f_root && f_root->k.empty() ) {  // the root empties: a level less
  if( f_root->leaf() )
   f_root.reset();
  else
   f_root = std::move( f_root->c[ 0 ] );
  }
 ADS_POST( ! contains( key ) );
 ADS_INVARIANT( [ & ]( void ) {
  int d = -1;
  return( valid( f_root.get() , nullptr , nullptr , 0 , d , true ) );
  }() );
 if constexpr( Subject::template listened< Erased > )
  f_sub.audit( *this );
 }

template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::clear( void )
{
 f_root.reset();
 f_size = 0;
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< std::totally_ordered K , class V , class P >
auto BTree< K , V , P >::slot( const K & key ) const
  -> std::pair< Node * , std::size_t >
{
 for( Node * x = f_root.get() ; x ; ) {
  const std::size_t i = place( x , key );
  if( ( i < x->k.size() ) && ! ( key < x->k[ i ] ) )
   return( std::pair< Node * , std::size_t >( x , i ) );
  if( x->leaf() )
   break;
  x = x->c[ i ].get();
  }
 return( std::pair< Node * , std::size_t >( nullptr , 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- WHEN SOMEONE LISTENS ---------------------------*/
/*--------------------------------------------------------------------------*/

template< std::totally_ordered K , class V , class P >
void BTree< K , V , P >::insertTold( const K & key , const V & val )
{
 if( const auto [ x , i ] = slot( key ); x ) {  // an assignment, in
  const Assigned e{ key , x->v[ i ] , val };     // place
  f_sub.prepare( e );
  x->v[ i ] = val;
  f_sub.emit( e , [ & , x = x , i = i ]() noexcept { x->v[ i ] = e.from; } );
  }
 else {
  const Inserted e{ key , key , val };
  f_sub.prepare( e );
  insertPlain( key , val );
  f_sub.emit( e );
  }
 f_sub.audit( *this );
 }

template< std::totally_ordered K , class V , class P >
bool BTree< K , V , P >::removeTold( const K & key )
{
 const auto [ x , i ] = slot( key );
 if( x == nullptr )
  return( false );
 const Erased e{ key , key , x->v[ i ] };
 f_sub.prepare( e );
 f_sub.emit( e );  // before the removal
 return( true );
 }

template< std::totally_ordered K , class V , class P >
bool BTree< K , V , P >::contains( const K & key ) const
{
 return( lookup( key ).has_value() );
 }

template< std::totally_ordered K , class V , class P >
std::size_t BTree< K , V , P >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V , class P >
bool BTree< K , V , P >::empty( void ) const
{
 return( f_size == 0 );
 }

template< std::totally_ordered K , class V , class P >
int BTree< K , V , P >::height( void ) const
{
 int d = -1;
 for( const Node * x = f_root.get() ; x ;
      x = x->leaf() ? nullptr : x->c[ 0 ].get() )
  ++d;
 return( d );
 }

template< std::totally_ordered K , class V , class P >
std::vector< std::vector< std::vector< K > > >
BTree< K , V , P >::levels( void ) const
{
 std::vector< std::vector< std::vector< K > > > out;
 std::vector< const Node * > L;
 if( f_root )
  L.push_back( f_root.get() );
 while( ! L.empty() ) {
  out.emplace_back();
  std::vector< const Node * > N;
  for( const Node * u : L ) {
   out.back().push_back( u->k );
   for( const auto & c : u->c )
    N.push_back( c.get() );
   }
  L = std::move( N );
  }
 return( out );
 }

template< std::totally_ordered K , class V , class P >
template< class F >
void BTree< K , V , P >::forEach( F && f ) const
{
 visit( f_root.get() , f );
 }

template< std::totally_ordered K , class V , class P >
template< class F >
void BTree< K , V , P >::visit( const Node * x , F & f )
{
 if( ! x )
  return;
 for( std::size_t i = 0 ; i < x->k.size() ; ++i ) {
  if( ! x->leaf() )
   visit( x->c[ i ].get() , f );
  f( x->k[ i ] , x->v[ i ] );
  }
 if( ! x->leaf() )
  visit( x->c.back().get() , f );
 }

template< std::totally_ordered K , class V , class P >
bool BTree< K , V , P >::valid( const Node * x , const K * lo , const K * hi ,
			      int d , int & leafDepth , bool root ) const
{
 if( ! x )
  return( f_size == 0 );
 const std::size_t n = x->k.size() , t = std::size_t( f_t );
 if( ( n > 2 * t - 1 ) || ( ! root && ( n < t - 1 ) ) || ( root && n < 1 ) )
  return( false );
 if( ( x->v.size() != n ) || ( ! x->leaf() && ( x->c.size() != n + 1 ) ) )
  return( false );
 for( std::size_t i = 0 ; i < n ; ++i )
  if( ( i > 0 && ! ( x->k[ i - 1 ] < x->k[ i ] ) ) ||
      ( lo && ! ( *lo < x->k[ i ] ) ) || ( hi && ! ( x->k[ i ] < *hi ) ) )
   return( false );
 if( x->leaf() ) {
  if( leafDepth < 0 )
   leafDepth = d;
  return( leafDepth == d );
  }
 for( std::size_t i = 0 ; i <= n ; ++i )
  if( ! valid( x->c[ i ].get() , i > 0 ? &x->k[ i - 1 ] : lo ,
	       i < n ? &x->k[ i ] : hi , d + 1 , leafDepth , false ) )
   return( false );
 return( true );
 }

template< std::totally_ordered K , class V , class P >
auto BTree< K , V , P >::lowerBoundPlace( const K & key ) const
  -> std::pair< const Node * , std::size_t >
{
 // a node of many keys, not binary: not searchTreeLowerBound()
 std::pair< const Node * , std::size_t > best( nullptr , 0 );
 for( const Node * x = f_root.get() ; x ; ) {
  const std::size_t i = place( x , key );
  if( i < x->k.size() ) {  // a candidate, smaller than those
   best = { x , i };        // above it
   if( ! ( key < x->k[ i ] ) )
    break;
   }
  if( x->leaf() )
   break;
  x = x->c[ i ].get();
  }
 return( best );
 }

template< std::totally_ordered K , class V , class P >
auto BTree< K , V , P >::lowerBound( const K & k ) const
  -> std::optional< std::pair< K , V > >
{
 const auto [ x , i ] = lowerBoundPlace( k );
 return( x ? std::optional< std::pair< K , V > >(
		std::pair< K , V >( x->k[ i ] , x->v[ i ] ) )
	    : std::nullopt );
 }

template< std::totally_ordered K , class V , class P >
const K * BTree< K , V , P >::findKey( const K & k ) const
{
 const auto [ x , i ] = lowerBoundPlace( k );
 return( ( x && ! ( k < x->k[ i ] ) ) ? &x->k[ i ] : nullptr );
 }

template< std::totally_ordered K , class V , class P >
const K * BTree< K , V , P >::lowerBoundKey( const K & k ) const
{
 const auto [ x , i ] = lowerBoundPlace( k );
 return( x ? &x->k[ i ] : nullptr );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
