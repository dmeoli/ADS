/*--------------------------------------------------------------------------*/
/*----------------------------- File Tree.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Tree [see Tree.h], with their contracts.
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

template< class T >
Tree< T >::Tree( const T & v )
 : f_value( v )
{
 }

template< class T >
Tree< T >::~Tree()
{
 while( f_child )
  deleteChild();
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

template< class T , class Pre , class Post >
void dfs( Tree< T > * t , Pre && pre , Post && post )
{
 ADS_PRE( t != nullptr );
 pre( t );                                    // (1)
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() )
  dfs( u , pre , post );
 post( t );                                   // (2)
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
 auto u = t->leftmostChild();
 for( int k = 0 ; u && ( k < i ) ; ++k , u = u->rightSibling() )
  in_visit( u , i , f );
 f( t );
 for( ; u ; u = u->rightSibling() )
  in_visit( u , i , f );
 }

template< class T >
Tree< T > * copy( const Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 auto c = new Tree< T >( t->read() );
 Tree< T > * last = nullptr;
 for( auto u = t->leftmostChild() ; u ; u = u->rightSibling() ) {
  Tree< T > * x = copy( u );
  if( last ) last->insertSibling( x );
  else c->insertChild( x );
  last = x;
  }
 ADS_POST( c->parent() == nullptr );
 return( c );
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
 std::ostringstream out;
 out << "( " << t->read();
 for( auto * c = t->leftmostChild() ; c ; c = c->rightSibling() )
  out << " " << to_list( c );
 out << " )";
 return( out.str() );
 }

namespace detail {

/// the tree of the list from L[ i ] on, i moved past it
template< class T >
Tree< T > * readTree( const std::vector< std::string > & L , std::size_t & i )
{
 ADS_PRE( ( i + 1 < L.size() ) && ( L[ i ] == "(" ) &&
          ( L[ i + 1 ] != "(" ) && ( L[ i + 1 ] != ")" ) );
 std::istringstream in( L[ i + 1 ] );
 T v{};
 in >> v;
 ADS_PRE( ! in.fail() );
 std::unique_ptr< Tree< T > > t( new Tree< T >( v ) );   // freed if a
 i += 2;                                                  // contract throws
 Tree< T > * last = nullptr;
 while( ( i < L.size() ) && ( L[ i ] == "(" ) ) {
  auto * c = readTree< T >( L , i );
  if( last )
   last->insertSibling( c );
  else
   t->insertChild( c );
  last = c;
  }
 ADS_PRE( ( i < L.size() ) && ( L[ i ] == ")" ) );
 ++i;
 return( t.release() );
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
 auto * t = detail::readTree< T >( L , i );
 ADS_PRE( i == L.size() );
 return( t );
 }

template< class T , class F >
void bfs( Tree< T > * t , F && f )
{
 ADS_PRE( t != nullptr );
 List< Tree< T > * > Q;   // enqueue at the tail, dequeue from the head
 Q.insert( Q.next( Q.tail() ) , t );
 while( ! Q.isEmpty() ) {
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
