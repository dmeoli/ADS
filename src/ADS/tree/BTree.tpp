/*--------------------------------------------------------------------------*/
/*----------------------------- File BTree.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BTree [see BTree.h], with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

template< std::totally_ordered K , class V >
BTree< K , V >::BTree( int t ) : f_t( t )
{
 ADS_PRE( t >= 2 );
 }

template< std::totally_ordered K , class V >
std::size_t BTree< K , V >::place( const Node * x , const K & key )
{
 return( std::size_t( std::lower_bound( x->k.begin() , x->k.end() , key ) -
                      x->k.begin() ) );
 }

template< std::totally_ordered K , class V >
std::optional< V > BTree< K , V >::lookup( const K & key ) const
{
 for( const Node * x = f_root.get() ; x ; ) {
  const std::size_t i = place( x , key );
  if( ( i < x->k.size() ) && !( key < x->k[ i ] ) )
   return( x->v[ i ] );
  if( x->leaf() )
   break;
  x = x->c[ i ].get();
  }
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
void BTree< K , V >::split( Node * x , std::size_t i )
{
 Node * y = x->c[ i ].get();                // 2 t - 1 keys
 auto z = std::make_unique< Node >();
 const std::size_t t = std::size_t( f_t );
 z->k.assign( y->k.begin() + t , y->k.end() );
 z->v.assign( y->v.begin() + t , y->v.end() );
 if( ! y->leaf() )
  for( std::size_t j = t ; j < y->c.size() ; ++j )
   z->c.push_back( std::move( y->c[ j ] ) );
 x->k.insert( x->k.begin() + i , y->k[ t - 1 ] );   // the median goes up
 x->v.insert( x->v.begin() + i , y->v[ t - 1 ] );
 x->c.insert( x->c.begin() + i + 1 , std::move( z ) );
 y->k.resize( t - 1 );
 y->v.resize( t - 1 );
 if( ! y->leaf() )
  y->c.resize( t );
 }

template< std::totally_ordered K , class V >
void BTree< K , V >::insert( const K & key , const V & val )
{
 const std::size_t full = std::size_t( 2 * f_t - 1 );
 if( ! f_root )
  f_root = std::make_unique< Node >();
 if( f_root->k.size() == full ) {           // the root splits: one level up
  auto s = std::make_unique< Node >();
  s->c.push_back( std::move( f_root ) );
  f_root = std::move( s );
  split( f_root.get() , 0 );
  }
 Node * x = f_root.get();
 for( ; ; ) {
  std::size_t i = place( x , key );
  if( ( i < x->k.size() ) && !( key < x->k[ i ] ) ) {
   x->v[ i ] = val;                         // the new value replaces it
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
   if( x->k[ i ] < key ) ++i;
   else if( !( key < x->k[ i ] ) ) {
    x->v[ i ] = val;
    break;
    }
   }
  x = x->c[ i ].get();
  }
 ADS_POST( contains( key ) );
 ADS_INVARIANT( [ & ]( void ) { int d = -1;
  return( valid( f_root.get() , nullptr , nullptr , 0 , d , true ) ); }() );
 }

template< std::totally_ordered K , class V >
std::size_t BTree< K , V >::fill( Node * x , std::size_t i )
{
 const std::size_t t = std::size_t( f_t );
 Node * y = x->c[ i ].get();
 if( y->k.size() >= t )
  return( i );
 if( ( i > 0 ) && ( x->c[ i - 1 ]->k.size() >= t ) ) {   // borrow from the
  Node * l = x->c[ i - 1 ].get();                        // left sibling
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
  Node * r = x->c[ i + 1 ].get();           // borrow from the right sibling
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
 merge( x , j );                            // with a sibling of t - 1 keys
 return( j );
 }

template< std::totally_ordered K , class V >
void BTree< K , V >::merge( Node * x , std::size_t j )
{
 Node * a = x->c[ j ].get() , * b = x->c[ j + 1 ].get();
 a->k.push_back( x->k[ j ] );               // the key between them down
 a->v.push_back( x->v[ j ] );
 a->k.insert( a->k.end() , b->k.begin() , b->k.end() );
 a->v.insert( a->v.end() , b->v.begin() , b->v.end() );
 for( auto & c : b->c )
  a->c.push_back( std::move( c ) );
 x->k.erase( x->k.begin() + j );
 x->v.erase( x->v.begin() + j );
 x->c.erase( x->c.begin() + j + 1 );
 }

template< std::totally_ordered K , class V >
bool BTree< K , V >::remove( Node * x , const K & key )
{
 const std::size_t t = std::size_t( f_t );
 for( ; ; ) {
  const std::size_t i = place( x , key );
  const bool here = ( i < x->k.size() ) && !( key < x->k[ i ] );
  if( x->leaf() ) {
   if( ! here )
    return( false );
   x->k.erase( x->k.begin() + i );
   x->v.erase( x->v.begin() + i );
   return( true );
   }
  if( here ) {
   if( x->c[ i ]->k.size() >= t ) {         // the predecessor replaces it
    Node * p = x->c[ i ].get();
    while( ! p->leaf() ) p = p->c.back().get();
    x->k[ i ] = p->k.back();
    x->v[ i ] = p->v.back();
    const K pk = p->k.back();
    x = x->c[ i ].get();
    return( remove( x , pk ) );
    }
   if( x->c[ i + 1 ]->k.size() >= t ) {     // the successor replaces it
    Node * s = x->c[ i + 1 ].get();
    while( ! s->leaf() ) s = s->c.front().get();
    x->k[ i ] = s->k.front();
    x->v[ i ] = s->v.front();
    const K sk = s->k.front();
    x = x->c[ i + 1 ].get();
    return( remove( x , sk ) );
    }
   merge( x , i );                          // both children have t - 1
   x = x->c[ i ].get();                     // keys: merged around it
   continue;
   }
  x = x->c[ fill( x , i ) ].get();          // down to a child of >= t keys
  }
 }

template< std::totally_ordered K , class V >
void BTree< K , V >::remove( const K & key )
{
 if( f_root && remove( f_root.get() , key ) )
  --f_size;
 if( f_root && f_root->k.empty() ) {        // the root empties: a level less
  if( f_root->leaf() )
   f_root.reset();
  else
   f_root = std::move( f_root->c[ 0 ] );
  }
 ADS_POST( ! contains( key ) );
 ADS_INVARIANT( [ & ]( void ) { int d = -1;
  return( valid( f_root.get() , nullptr , nullptr , 0 , d , true ) ); }() );
 }

template< std::totally_ordered K , class V >
bool BTree< K , V >::contains( const K & key ) const
{
 return( lookup( key ).has_value() );
 }

template< std::totally_ordered K , class V >
std::size_t BTree< K , V >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered K , class V >
bool BTree< K , V >::isEmpty( void ) const
{
 return( f_size == 0 );
 }

template< std::totally_ordered K , class V >
int BTree< K , V >::height( void ) const
{
 int d = -1;
 for( const Node * x = f_root.get() ; x ; x = x->leaf() ? nullptr :
                                             x->c[ 0 ].get() )
  ++d;
 return( d );
 }

template< std::totally_ordered K , class V >
template< class F >
void BTree< K , V >::forEach( F && f ) const
{
 visit( f_root.get() , f );
 }

template< std::totally_ordered K , class V >
template< class F >
void BTree< K , V >::visit( const Node * x , F & f )
{
 if( ! x ) return;
 for( std::size_t i = 0 ; i < x->k.size() ; ++i ) {
  if( ! x->leaf() ) visit( x->c[ i ].get() , f );
  f( x->k[ i ] , x->v[ i ] );
  }
 if( ! x->leaf() ) visit( x->c.back().get() , f );
 }

template< std::totally_ordered K , class V >
bool BTree< K , V >::valid( const Node * x , const K * lo , const K * hi ,
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
  if( ( i > 0 && !( x->k[ i - 1 ] < x->k[ i ] ) ) ||
      ( lo && !( *lo < x->k[ i ] ) ) || ( hi && !( x->k[ i ] < *hi ) ) )
   return( false );
 if( x->leaf() ) {
  if( leafDepth < 0 ) leafDepth = d;
  return( leafDepth == d );
  }
 for( std::size_t i = 0 ; i <= n ; ++i )
  if( ! valid( x->c[ i ].get() , i > 0 ? &x->k[ i - 1 ] : lo ,
               i < n ? &x->k[ i ] : hi , d + 1 , leafDepth , false ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
