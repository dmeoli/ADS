/*--------------------------------------------------------------------------*/
/*--------------------------- File BinaryTrie.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BinaryTrie, SimpleBinaryTrie and the trie of XFastTrie
 * [see BinaryTrie.h], with their contracts.
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

namespace detail {

/*--------------------------------------------------------------------------*/
/*---------------------------- JumpTrie ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
JumpTrie< U , V , W , Hashed >::JumpTrie( void )
{
 f_root = f_pool.make();
 f_dummy = f_pool.make();
 f_dummy->child[ 0 ] = f_dummy->child[ 1 ] = f_dummy;
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
JumpTrie< U , V , W , Hashed >::JumpTrie( JumpTrie && o ) : JumpTrie()
{
 *this = std::move( o );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
JumpTrie< U , V , W , Hashed > & JumpTrie< U , V , W , Hashed >::operator=(
                                                  JumpTrie && o ) noexcept
{
 f_pool.swap( o.f_pool );
 std::swap( f_root , o.f_root );
 std::swap( f_dummy , o.f_dummy );
 std::swap( f_n , o.f_n );
 std::swap( f_nodes , o.f_nodes );
 if constexpr( Hashed )
  for( int i = 0 ; i <= W ; ++i )
   f_t.t[ i ].swap( o.f_t.t[ i ] );
 return( *this );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
JumpTrie< U , V , W , Hashed >::~JumpTrie()
{
 // the pool frees its blocks: only the values that need it are destroyed
 if constexpr( ! std::is_trivially_destructible_v< V > ) {
  destroy( f_root , 0 );
  f_pool.free( f_root );
  f_pool.free( f_dummy );
  }
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
void JumpTrie< U , V , W , Hashed >::destroy( Node * u , int i )
{
 if( i == W )   // a leaf: its children are the links of the list
  return;
 for( Node * c : u->child )
  if( c ) {
   destroy( c , i + 1 );
   f_pool.free( c );
   }
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
void JumpTrie< U , V , W , Hashed >::clear( void )
{
 destroy( f_root , 0 );
 f_root->child[ 0 ] = f_root->child[ 1 ] = f_root->jump = nullptr;
 f_dummy->child[ 0 ] = f_dummy->child[ 1 ] = f_dummy;
 if constexpr( Hashed )
  for( auto & t : f_t.t )
   t.clear();
 f_n = 0;
 f_nodes = 1;
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
auto JumpTrie< U , V , W , Hashed >::node( U p , int i ) const -> Node *
{
 if( i == 0 )
  return( f_root );
 if constexpr( Hashed ) {
  Node * const * u = f_t.t[ i ].find( p );
  return( u ? *u : nullptr );
  }
 else {
  ( void ) p;
  return( nullptr );
  }
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
auto JumpTrie< U , V , W , Hashed >::deepest( U x , int lo , int hi ,
                                              Node * u ) const
 -> std::pair< Node * , int >
{
 if( ! u )
  u = f_root;
//! [JumpTrie-deepest]
 if constexpr( Hashed ) {
  while( hi - lo > 1 ) {   // the prefix of depth lo is there, of hi not
   const int m = ( lo + hi ) / 2;
   if( Node * v = node( triePrefix< U , W >( x , m ) , m ) ) {
    lo = m;
    u = v;
    }
   else
    hi = m;
   }
  return( std::pair< Node * , int >( u , lo ) );
  }
 else {
  ( void ) hi;
  int i = lo;
  for( ; i < W ; ++i ) {
   Node * c = u->child[ trieBit< U , W >( x , i ) ];
   if( ! c )
    break;
   u = c;
   }
  return( std::pair< Node * , int >( u , i ) );
  }
//! [JumpTrie-deepest]
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
auto JumpTrie< U , V , W , Hashed >::successor( U x , Node * u , int i ) const
 -> Node *
{
//! [JumpTrie-successor]
 if( i == W )
  return( u );
 if( f_n == 0 )
  return( f_dummy );
 // the child of the bit of x is missing, the other one is there: its
 // smallest leaf follows x, its largest one precedes it
 return( trieBit< U , W >( x , i ) == 0 ? u->jump : u->jump->child[ 1 ] );
//! [JumpTrie-successor]
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
auto JumpTrie< U , V , W , Hashed >::leftmost( Node * u , int i ) const
 -> Node *
{
 for( ; i < W ; ++i ) {
  if( ! u->child[ 0 ] )
   return( u->jump );   // the smallest leaf below u
  u = u->child[ 0 ];
  }
 return( u );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
std::optional< U > JumpTrie< U , V , W , Hashed >::find( U x ) const
{
 ADS_PRE( ( trieKey< U , W >( x ) ) );
 const auto [ u , i ] = deepest( x );
 const Node * l = successor( x , u , i );
 if( l == f_dummy )
  return( std::nullopt );
 return( l->key );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
auto JumpTrie< U , V , W , Hashed >::lowerBound( U x ) const
 -> const_iterator
{
 ADS_PRE( ( trieKey< U , W >( x ) ) );
 const auto [ u , i ] = deepest( x );
 return( const_iterator( successor( x , u , i ) ) );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
std::optional< V > JumpTrie< U , V , W , Hashed >::lookup( U x ) const
{
 ADS_PRE( ( trieKey< U , W >( x ) ) );
 if constexpr( Hashed ) {
  Node * l = node( x , W );
  if( ! l )
   return( std::nullopt );
  return( l->value );
  }
 else {
  const auto [ u , i ] = deepest( x );
  if( i < W )
   return( std::nullopt );
  return( u->value );
  }
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
bool JumpTrie< U , V , W , Hashed >::contains( U x ) const
{
 ADS_PRE( ( trieKey< U , W >( x ) ) );
 if constexpr( Hashed )
  return( node( x , W ) != nullptr );
 else
  return( deepest( x ).second == W );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
std::optional< U > JumpTrie< U , V , W , Hashed >::min( void ) const
{
 if( f_n == 0 )
  return( std::nullopt );
 return( f_dummy->child[ 1 ]->key );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
std::optional< U > JumpTrie< U , V , W , Hashed >::max( void ) const
{
 if( f_n == 0 )
  return( std::nullopt );
 return( f_dummy->child[ 0 ]->key );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
bool JumpTrie< U , V , W , Hashed >::insert( U x , const V & v )
{
 ADS_PRE( ( trieKey< U , W >( x ) ) );
//! [JumpTrie-insert]
 auto [ u , i ] = deepest( x );
 if( i == W ) {
  u->value = v;
  return( false );
  }

 // the predecessor of x: the leaf before the smallest one below u, or the
 // largest one below u
 Node * pred = f_dummy;
 if( f_n > 0 )
  pred = trieBit< U , W >( x , i ) == 0 ? u->jump->child[ 0 ] : u->jump;

 // the rest of the path of x
 for( ; i < W ; ++i ) {
  Node * c = f_pool.make();
  c->parent = u;
  u->child[ trieBit< U , W >( x , i ) ] = c;
  if constexpr( Hashed )
   f_t.t[ i + 1 ].insert( triePrefix< U , W >( x , i + 1 ) , c );
  ++f_nodes;
  u = c;
  }
 Node * l = u;
 l->key = x;
 l->value = v;
 l->child[ 0 ] = pred;
 l->child[ 1 ] = pred->child[ 1 ];
 pred->child[ 1 ]->child[ 0 ] = l;
 pred->child[ 1 ] = l;
 ++f_n;

 // the jump pointers that x improves: those of the nodes with one child
 // whose smallest leaf, or largest, is now x
 for( Node * a = l->parent ; a ; a = a->parent )
  if( ( ! a->child[ 0 ] && ( ! a->jump || ( x < a->jump->key ) ) ) ||
      ( ! a->child[ 1 ] && ( ! a->jump || ( x > a->jump->key ) ) ) )
   a->jump = l;
//! [JumpTrie-insert]

 ADS_POST( contains( x ) );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
bool JumpTrie< U , V , W , Hashed >::remove( U x )
{
 ADS_PRE( ( trieKey< U , W >( x ) ) );
 Node * l;
 if constexpr( Hashed )
  l = node( x , W );
 else {
  const auto [ u , i ] = deepest( x );
  l = ( i == W ) ? u : nullptr;
  }
 if( ! l )
  return( false );

//! [JumpTrie-remove]
 Node * prev = l->child[ 0 ];
 Node * next = l->child[ 1 ];
 prev->child[ 1 ] = next;
 next->child[ 0 ] = prev;

 // the nodes of the path left with no child, from the leaf up
 Node * u = l;
 for( int i = W ; i > 0 ; --i ) {
  Node * p = u->parent;
  const int c = trieBit< U , W >( x , i - 1 );
  p->child[ c ] = nullptr;
  if constexpr( Hashed )
   f_t.t[ i ].remove( triePrefix< U , W >( x , i ) );
  f_pool.free( u );
  --f_nodes;
  u = p;
  if( p->child[ 1 - c ] )
   break;
  }
 --f_n;

 // u has one child, or none if it is the root of an empty trie
 if( f_n == 0 )
  u->jump = nullptr;
 else
  u->jump = u->child[ 0 ] ? prev : next;
 for( Node * a = u->parent ; a ; a = a->parent )
  if( a->jump == l )
   a->jump = a->child[ 0 ] ? prev : next;
//! [JumpTrie-remove]

 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
bool JumpTrie< U , V , W , Hashed >::subtree( const Node * u , int i , U p ,
                                              const Node * & lo ,
                                              const Node * & hi ,
                                              std::size_t & n ,
                                              std::size_t & m ) const
{
 ++m;
 if constexpr( Hashed )
  if( ( i > 0 ) && ( node( p , i ) != u ) )
   return( false );
 if( i == W ) {
  if( u->key != p )
   return( false );
  lo = hi = u;
  ++n;
  return( true );
  }
 lo = hi = nullptr;
 for( int c = 0 ; c < 2 ; ++c ) {
  const Node * v = u->child[ c ];
  if( ! v )
   continue;
  if( v->parent != u )
   return( false );
  const Node * a , * b;
  if( ! subtree( v , i + 1 , U( ( p << 1 ) | U( c ) ) , a , b , n , m ) )
   return( false );
  if( ! lo )
   lo = a;
  hi = b;
  }
 if( ! lo )   // no child: only the root of an empty trie
  return( ( u == f_root ) && ! u->jump );
 if( ! u->child[ 0 ] && u->jump != lo )
  return( false );
 if( ! u->child[ 1 ] && u->jump != hi )
  return( false );
 return( true );
 }

template< std::unsigned_integral U , class V , int W , bool Hashed >
 requires TrieWidth< U , W >
bool JumpTrie< U , V , W , Hashed >::consistent( U x ) const
{
 if( f_n > 256 ) {   // around the leaf of x only
  const Node * l = node( x , W );
  if constexpr( ! Hashed ) {
   const auto [ u , i ] = deepest( x );
   l = ( i == W ) ? u : nullptr;
   }
  if( ! l )
   return( true );
  return( ( l->child[ 0 ] == f_dummy || l->child[ 0 ]->key < x ) &&
          ( l->child[ 1 ] == f_dummy || l->child[ 1 ]->key > x ) &&
          ( l->child[ 0 ]->child[ 1 ] == l ) &&
          ( l->child[ 1 ]->child[ 0 ] == l ) );
  }
 const Node * lo , * hi;
 std::size_t n = 0 , m = 0;
 if( ! subtree( f_root , 0 , 0 , lo , hi , n , m ) )
  return( false );
 if( ( n != f_n ) || ( m != f_nodes ) )
  return( false );
 if constexpr( Hashed ) {
  std::size_t s = 0;
  for( int i = 1 ; i <= W ; ++i )
   s += f_t.t[ i ].size();
  if( s != f_nodes - 1 )
   return( false );
  }
 std::size_t k = 0;
 for( const Node * l = f_dummy->child[ 1 ] ; l != f_dummy ;
      l = l->child[ 1 ] , ++k )
  if( ( l->child[ 1 ]->child[ 0 ] != l ) ||
      ( ( l->child[ 1 ] != f_dummy ) && ! ( l->key < l->child[ 1 ]->key ) ) )
   return( false );
 return( k == f_n );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------- SimpleBinaryTrie -------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
SimpleBinaryTrie< U , V , W >::SimpleBinaryTrie( SimpleBinaryTrie && o )
 noexcept
 : f_root( o.f_root ) , f_n( o.f_n ) , f_nodes( o.f_nodes )
{
 o.f_root = Node();
 o.f_n = 0;
 o.f_nodes = 1;
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
SimpleBinaryTrie< U , V , W > & SimpleBinaryTrie< U , V , W >::operator=(
                                          SimpleBinaryTrie && o ) noexcept
{
 std::swap( f_root , o.f_root );
 std::swap( f_n , o.f_n );
 std::swap( f_nodes , o.f_nodes );
 return( *this );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
SimpleBinaryTrie< U , V , W >::~SimpleBinaryTrie()
{
 for( Node * c : f_root.child )
  if( c )
   destroy( c , 1 );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
void SimpleBinaryTrie< U , V , W >::destroy( Node * u , int i )
{
 if( i == W ) {
  delete static_cast< Leaf * >( u );
  return;
  }
 for( Node * c : u->child )
  if( c )
   destroy( c , i + 1 );
 delete u;
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
auto SimpleBinaryTrie< U , V , W >::leaf( U x ) const -> const Leaf *
{
 const Node * u = &f_root;
 for( int i = 0 ; i < W ; ++i )
  if( ! ( u = u->child[ detail::trieBit< U , W >( x , i ) ] ) )
   return( nullptr );
 return( static_cast< const Leaf * >( u ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< U > SimpleBinaryTrie< U , V , W >::find( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
//! [SimpleBinaryTrie-find]
 const Node * u = &f_root;
 const Node * a = nullptr;   // the nearest subtree of larger keys
 int d = 0;                  // its depth
 int i = 0;
 for( ; i < W ; ++i ) {
  const int c = detail::trieBit< U , W >( x , i );
  if( ( c == 0 ) && u->child[ 1 ] ) {
   a = u->child[ 1 ];
   d = i + 1;
   }
  if( ! u->child[ c ] )
   break;
  u = u->child[ c ];
  }
 if( i == W )
  return( x );
 if( ! a )
  return( std::nullopt );
 for( ; d < W ; ++d )   // the leftmost leaf below a
  a = a->child[ 0 ] ? a->child[ 0 ] : a->child[ 1 ];
 return( static_cast< const Leaf * >( a )->key );
//! [SimpleBinaryTrie-find]
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< V > SimpleBinaryTrie< U , V , W >::lookup( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 const Leaf * l = leaf( x );
 if( ! l )
  return( std::nullopt );
 return( l->value );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool SimpleBinaryTrie< U , V , W >::contains( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 return( leaf( x ) != nullptr );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool SimpleBinaryTrie< U , V , W >::insert( U x , const V & v )
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
//! [SimpleBinaryTrie-insert]
 Node * u = &f_root;
 bool fresh = false;
 for( int i = 0 ; i < W ; ++i ) {
  Node * & c = u->child[ detail::trieBit< U , W >( x , i ) ];
  if( ! c ) {
   if( i + 1 == W ) {
    Leaf * l = new Leaf;
    l->key = x;
    c = l;
    }
   else
    c = new Node;
   ++f_nodes;
   fresh = true;
   }
  u = c;
  }
 static_cast< Leaf * >( u )->value = v;
 if( fresh )
  ++f_n;
//! [SimpleBinaryTrie-insert]
 ADS_POST( contains( x ) );
 return( fresh );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool SimpleBinaryTrie< U , V , W >::remove( U x )
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
//! [SimpleBinaryTrie-remove]
 std::array< Node * , W + 1 > P;   // the path of x
 P[ 0 ] = &f_root;
 for( int i = 0 ; i < W ; ++i )
  if( ! ( P[ i + 1 ] = P[ i ]->child[ detail::trieBit< U , W >( x , i ) ] ) )
   return( false );
 delete static_cast< Leaf * >( P[ W ] );
 --f_nodes;
 for( int i = W - 1 ; i >= 0 ; --i ) {
  const int c = detail::trieBit< U , W >( x , i );
  P[ i ]->child[ c ] = nullptr;
  if( P[ i ]->child[ 1 - c ] || ( i == 0 ) )
   break;
  delete P[ i ];
  --f_nodes;
  }
 --f_n;
//! [SimpleBinaryTrie-remove]
 ADS_POST( ! contains( x ) );
 return( true );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
template< class F >
void SimpleBinaryTrie< U , V , W >::visit( const Node * u , int i , F & f )
{
 if( i == W ) {
  const Leaf * l = static_cast< const Leaf * >( u );
  f( l->key , l->value );
  return;
  }
 for( const Node * c : u->child )
  if( c )
   visit( c , i + 1 , f );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
template< class F >
void SimpleBinaryTrie< U , V , W >::forEach( F && f ) const
{
 if( f_n > 0 )
  visit( &f_root , 0 , f );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinaryTrie.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
