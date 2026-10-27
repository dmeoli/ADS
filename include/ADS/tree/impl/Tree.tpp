/*--------------------------------------------------------------------------*/
/*----------------------------- File Tree.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Tree [see Tree.h], with their contracts.
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
Tree< T >::Tree( const T & v )
 : f_value( v )
{
 }

template< class T >
Tree< T >::~Tree()
{
 std::vector< Tree * > S;   // iterative, as the height may be n
 for( Tree * c = f_child ; c ; c = c->f_sibling )
  S.push_back( c );
 while( ! S.empty() ) {
  Tree * u = S.back();
  S.pop_back();
  for( Tree * c = u->f_child ; c ; c = c->f_sibling )
   S.push_back( c );
  u->f_child = nullptr;      // its children are on S
  delete u;
  }
 }

template< class T >
const T & Tree< T >::read( void ) const
{
 return( f_value );
 }

template< class T >
void Tree< T >::write( const T & v )
{
 f_value = v;
 }

template< class T >
auto Tree< T >::parent( void ) const -> Tree *
{
 return( f_parent );
 }

template< class T >
auto Tree< T >::leftmostChild( void ) const -> Tree *
{
 return( f_child );
 }

template< class T >
auto Tree< T >::rightSibling( void ) const -> Tree *
{
 return( f_sibling );
 }

template< class T >
void Tree< T >::insertChild( Tree * t )
{
 ADS_PRE( ( t != nullptr ) && ( t->f_parent == nullptr ) && ( t != this ) );
 ADS_AUDIT( top() != t );
 t->f_parent = this;
 t->f_sibling = f_child;
 f_child = t;
 ADS_POST( ( f_child == t ) && ( t->f_parent == this ) );
 ADS_INVARIANT( linked() );
 }

template< class T >
void Tree< T >::insertSibling( Tree * t )
{
 ADS_PRE( ( t != nullptr ) && ( t->f_parent == nullptr ) && ( t != this ) &&
          ( f_parent != nullptr ) );
 ADS_AUDIT( top() != t );
 t->f_parent = f_parent;
 t->f_sibling = f_sibling;
 f_sibling = t;
 ADS_POST( ( f_sibling == t ) && ( t->f_parent == f_parent ) );
 ADS_INVARIANT( f_parent->linked() );
 }

template< class T >
void Tree< T >::deleteChild( void )
{
 if( Tree * c = f_child ) {
  f_child = c->f_sibling;
  c->f_sibling = nullptr;
  delete c;
  }
 ADS_INVARIANT( linked() );
 }

template< class T >
void Tree< T >::deleteSibling( void )
{
 if( Tree * s = f_sibling ) {
  f_sibling = s->f_sibling;
  s->f_sibling = nullptr;
  delete s;
  }
 ADS_INVARIANT( ( f_parent == nullptr ) || f_parent->linked() );
 }

template< class T >
bool Tree< T >::linked( void ) const
{
 for( const Tree * u = f_child ; u ; u = u->f_sibling )
  if( u->f_parent != this )
   return( false );
 return( true );
 }

template< class T >
auto Tree< T >::top( void ) const -> const Tree *
{
 const Tree * u = this;
 while( u->f_parent )
  u = u->f_parent;
 return( u );
 }

template< class T , class Pre , class Post >
void dfs( Tree< T > * t , Pre && pre , Post && post )
{
 ADS_PRE( t != nullptr );
 // iterative, as the height may be n: each node on the stack with the child
 // being visited, nil before the first one; the children are read after
 // pre( u ), the next sibling after the subtree, as the recursion would
 struct Item { Tree< T > * u; Tree< T > * c; };
 std::vector< Item > S;
 pre( t );                                    // (1)
 S.push_back( Item{ t , nullptr } );
 while( ! S.empty() ) {
  Item & x = S.back();
  x.c = x.c ? x.c->rightSibling() : x.u->leftmostChild();
  if( Tree< T > * c = x.c ) {
   pre( c );                                  // (1)
   S.push_back( Item{ c , nullptr } );
   }
  else {
   Tree< T > * u = x.u;
   S.pop_back();
   post( u );                                 // (2)
   }
  }
 }

template< class T , class F >
void pre_visit( Tree< T > * t , F && f )
{
 dfs( t , f , []( Tree< T > * ) {} );
 }

template< class T , class F >
void post_visit( Tree< T > * t , F && f )
{
 dfs( t , []( Tree< T > * ) {} , f );
 }

template< class T , class F >
void in_visit( Tree< T > * t , int i , F && f )
{
 ADS_PRE( ( t != nullptr ) && ( i >= 0 ) );
 // iterative, as dfs(): k the children of u entered so far, and f( u )
 // once they are i, or they are over
 struct Item { Tree< T > * u; Tree< T > * c; int k; bool started , done; };
 std::vector< Item > S = { Item{ t , nullptr , 0 , false , false } };
 while( ! S.empty() ) {
  Item & x = S.back();
  if( x.started ) {
   x.c = x.c->rightSibling();
   ++x.k;
   }
  else {
   x.c = x.u->leftmostChild();
   x.started = true;
   }
  if( ! x.done && ( ( x.k == i ) || ! x.c ) ) {
   x.done = true;
   f( x.u );
   }
  if( Tree< T > * c = S.back().c )
   S.push_back( Item{ c , nullptr , 0 , false , false } );
  else
   S.pop_back();
  }
 }

template< class T >
Tree< T > * copy( const Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 // a previsit with a stack of its own, as the height may be n: each node
 // with its copy, its next child to copy and the last child copied; a copy
 // is hung under its parent when its subtree is done, and the copies not
 // yet hung are freed if anything throws
 struct Item {
  const Tree< T > * from;
  std::unique_ptr< Tree< T > > to;
  const Tree< T > * next;
  Tree< T > * last;
  };
 std::vector< Item > S;
 std::unique_ptr< Tree< T > > c( new Tree< T >( t->read() ) );
 S.push_back( Item{ t , std::move( c ) , t->leftmostChild() , nullptr } );
 for( ; ; ) {
  Item & x = S.back();
  if( const Tree< T > * u = x.next ) {
   x.next = u->rightSibling();
   std::unique_ptr< Tree< T > > y( new Tree< T >( u->read() ) );
   S.push_back( Item{ u , std::move( y ) , u->leftmostChild() , nullptr } );
   continue;
   }
  c = std::move( x.to );
  S.pop_back();
  if( S.empty() )
   break;
  Item & p = S.back();
  if( p.last ) p.last->insertSibling( c.get() );
  else p.to->insertChild( c.get() );
  p.last = c.release();
  }
 ADS_POST( c->parent() == nullptr );
 return( c.release() );
 }

template< class T >
bool Tree< T >::isLeaf( void ) const
{
 return( f_child == nullptr );
 }

template< class T >
bool Tree< T >::isLastSibling( void ) const
{
 return( f_sibling == nullptr );
 }

template< class T >
int maxWeightLevel( Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 std::vector< Tree< T > * > L = { t };      // the nodes of a level
 int level = 0 , best = 0;
 T bestWeight{};
 for( ; ! L.empty() ; ++level ) {
  T w{};
  std::vector< Tree< T > * > N;
  for( auto * u : L ) {
   w = w + u->read();
   for( auto * c = u->leftmostChild() ; c ; c = c->rightSibling() )
    N.push_back( c );
   }
  if( ( level == 0 ) || ( bestWeight < w ) ) {
   best = level;
   bestWeight = w;
   }
  L = std::move( N );
  }
 return( best );
 }

template< class T >
std::string to_list( const Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 // a previsit with a stack of its own, as the height may be n: the next
 // child to write of each node open
 std::ostringstream out;
 out << "( " << t->read();
 std::vector< const Tree< T > * > S = { t->leftmostChild() };
 while( ! S.empty() ) {
  if( const Tree< T > * c = S.back() ) {
   S.back() = c->rightSibling();
   out << " ( " << c->read();
   S.push_back( c->leftmostChild() );
   }
  else {
   S.pop_back();
   out << " )";
   }
  }
 return( out.str() );
 }

namespace detail {

/// the tree of the list from L[ i ] on, i moved past it
template< class T >
Tree< T > * readTree( const std::vector< std::string > & L , std::size_t & i )
{
 // the descent by the grammar with a stack of its own, as the height may
 // be n: each node open with its last child, hung under its parent at its
 // ")"; the nodes freed if a contract throws
 struct Item { std::unique_ptr< Tree< T > > t; Tree< T > * last; };
 std::vector< Item > S;
 for( ; ; ) {
  if( S.empty() || ( ( i < L.size() ) && ( L[ i ] == "(" ) ) ) {
   ADS_PRE( ( i + 1 < L.size() ) && ( L[ i ] == "(" ) &&
            ( L[ i + 1 ] != "(" ) && ( L[ i + 1 ] != ")" ) );
   std::istringstream in( L[ i + 1 ] );
   T v{};
   in >> v;
   ADS_PRE( ! in.fail() &&                  // all the atom, e.g., not 8x
            ( in.peek() == std::char_traits< char >::eof() ) );
   std::unique_ptr< Tree< T > > t( new Tree< T >( v ) );
   S.push_back( Item{ std::move( t ) , nullptr } );
   i += 2;
   continue;
   }
  ADS_PRE( ( i < L.size() ) && ( L[ i ] == ")" ) );
  ++i;
  std::unique_ptr< Tree< T > > c = std::move( S.back().t );
  S.pop_back();
  if( S.empty() )
   return( c.release() );
  Item & p = S.back();
  if( p.last )
   p.last->insertSibling( c.get() );
  else
   p.t->insertChild( c.get() );
  p.last = c.release();
  }
 }

/// the tokens of a parenthesized list: "(", ")" and the atoms between them
inline std::vector< std::string > listTokens( std::string_view s )
{
 std::vector< std::string > T;
 for( std::size_t i = 0 ; i < s.size() ; )
  if( s[ i ] == ' ' )
   ++i;
  else if( ( s[ i ] == '(' ) || ( s[ i ] == ')' ) )
   T.emplace_back( 1 , s[ i++ ] );
  else {
   std::size_t j = i;
   while( ( j < s.size() ) && ( s[ j ] != ' ' ) && ( s[ j ] != '(' ) &&
          ( s[ j ] != ')' ) )
    ++j;
   T.emplace_back( s.substr( i , j - i ) );
   i = j;
   }
 return( T );
 }

}  // end( namespace detail )

template< class T >
Tree< T > * tree_from_list( std::string_view s )
{
 const auto L = detail::listTokens( s );
 std::size_t i = 0;
 std::unique_ptr< Tree< T > > t( detail::readTree< T >( L , i ) );
 ADS_PRE( i == L.size() );                   // t freed if it throws
 return( t.release() );
 }

template< class T , class F >
void bfs( Tree< T > * t , F && f )
{
 ADS_PRE( t != nullptr );
 List< Tree< T > * > Q;   // enqueue at the tail, dequeue from the head
 Q.insert( Q.next( Q.tail() ) , t );
 while( ! Q.empty() ) {
  Tree< T > * u = Q.read( Q.head() );
  Q.remove( Q.head() );
  f( u );
  for( u = u->leftmostChild() ; u ; u = u->rightSibling() )
   Q.insert( Q.next( Q.tail() ) , u );
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Tree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
