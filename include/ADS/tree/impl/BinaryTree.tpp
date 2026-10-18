/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BinaryTree [see BinaryTree.h], with their contracts.
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
BinaryTree< T >::BinaryTree( const T & v )
 : f_value( v )
{
 }

template< class T >
BinaryTree< T >::~BinaryTree()
{
 std::vector< BinaryTree * > S;   // iterative, as the height may be n
 for( BinaryTree * c : { f_left , f_right } )
  if( c ) S.push_back( c );
 while( ! S.empty() ) {
  BinaryTree * u = S.back();
  S.pop_back();
  for( BinaryTree * c : { u->f_left , u->f_right } )
   if( c ) S.push_back( c );
  u->f_left = u->f_right = nullptr;   // its children are on S
  delete u;
  }
 }

template< class T >
const T & BinaryTree< T >::read( void ) const
{
 return( f_value );
 }

template< class T >
void BinaryTree< T >::write( const T & v )
{
 f_value = v;
 }

template< class T >
auto BinaryTree< T >::parent( void ) const -> BinaryTree *
{
 return( f_parent );
 }

template< class T >
auto BinaryTree< T >::left( void ) const -> BinaryTree *
{
 return( f_left );
 }

template< class T >
auto BinaryTree< T >::right( void ) const -> BinaryTree *
{
 return( f_right );
 }

template< class T >
void BinaryTree< T >::insertLeft( BinaryTree * t )
{
 ADS_PRE( f_left == nullptr );
 insert( f_left , t );
 ADS_POST( f_left == t );
 }

template< class T >
void BinaryTree< T >::insertRight( BinaryTree * t )
{
 ADS_PRE( f_right == nullptr );
 insert( f_right , t );
 ADS_POST( f_right == t );
 }

template< class T >
void BinaryTree< T >::deleteLeft( void )
{
 delete f_left;
 f_left = nullptr;
 }

template< class T >
void BinaryTree< T >::deleteRight( void )
{
 delete f_right;
 f_right = nullptr;
 }

template< class T >
auto BinaryTree< T >::detachLeft( void ) -> BinaryTree *
{
 BinaryTree * t = detach( f_left );
 ADS_POST( ( f_left == nullptr ) && ( ( t == nullptr ) ||
                                      ( t->f_parent == nullptr ) ) );
 return( t );
 }

template< class T >
auto BinaryTree< T >::detachRight( void ) -> BinaryTree *
{
 BinaryTree * t = detach( f_right );
 ADS_POST( ( f_right == nullptr ) && ( ( t == nullptr ) ||
                                       ( t->f_parent == nullptr ) ) );
 return( t );
 }

template< class T >
auto BinaryTree< T >::detach( BinaryTree * & child ) -> BinaryTree *
{
 BinaryTree * t = child;
 child = nullptr;
 if( t ) t->f_parent = nullptr;
 ADS_INVARIANT( linked() );
 return( t );
 }

template< class T >
void BinaryTree< T >::insert( BinaryTree * & child , BinaryTree * t )
{
 ADS_PRE( ( t != nullptr ) && ( t->f_parent == nullptr ) && ( t != this ) );
 ADS_AUDIT( top() != t );
 t->f_parent = this;
 child = t;
 ADS_INVARIANT( linked() );
 }

template< class T >
bool BinaryTree< T >::linked( void ) const
{
 for( const BinaryTree * c : { f_left , f_right } )   // the links that
  if( c && ( c->f_parent != this ) )                  // insert() and
   return( false );                                   // detach() change
 return( true );
 }

template< class T >
auto BinaryTree< T >::top( void ) const -> const BinaryTree *
{
 const BinaryTree * u = this;
 while( u->f_parent )
  u = u->f_parent;
 return( u );
 }

template< class T , class Pre , class In , class Post >
void dfs( BinaryTree< T > * t , Pre && pre , In && in , Post && post )
{
 // iterative, as the height may be n: each node on the stack with the
 // number of the steps done, the children read when they are entered
 std::vector< std::pair< BinaryTree< T > * , int > > S;
 if( t ) S.emplace_back( t , 0 );
 while( ! S.empty() ) {
  BinaryTree< T > * u = S.back().first;
  const int k = S.back().second++;
  if( k == 0 ) {
   pre( u );                             // (1)
   if( u->left() ) S.emplace_back( u->left() , 0 );
   }
  else if( k == 1 ) {
   in( u );                              // (2)
   if( u->right() ) S.emplace_back( u->right() , 0 );
   }
  else {
   S.pop_back();
   post( u );                            // (3)
   }
  }
 }

template< class T , class F >
void pre_visit( BinaryTree< T > * t , F && f )
{
 auto none = []( BinaryTree< T > * ) {};
 dfs( t , f , none , none );
 }

template< class T , class F >
void in_visit( BinaryTree< T > * t , F && f )
{
 auto none = []( BinaryTree< T > * ) {};
 dfs( t , none , f , none );
 }

template< class T , class F >
void post_visit( BinaryTree< T > * t , F && f )
{
 auto none = []( BinaryTree< T > * ) {};
 dfs( t , none , none , f );
 }

template< class T >
BinaryTree< T > * copy( const BinaryTree< T > * t )
{
 if( ! t ) return( nullptr );
 auto b = new BinaryTree< T >( t->read() );
 if( auto l = copy( t->left() ) ) b->insertLeft( l );
 if( auto r = copy( t->right() ) ) b->insertRight( r );
 return( b );
 }

template< class T >
BinaryTree< T > * join( const T & v , const BinaryTree< T > * l ,
                        const BinaryTree< T > * r )
{
 auto b = new BinaryTree< T >( v );
 if( auto x = copy( l ) ) b->insertLeft( x );
 if( auto y = copy( r ) ) b->insertRight( y );
 return( b );
 }

template< class T >
BinaryTree< T > * to_binary( const Tree< T > * t )
{
 ADS_PRE( t != nullptr );
 auto b = new BinaryTree< T >( t->read() );
 if( auto c = t->leftmostChild() )
  b->insertLeft( to_binary( c ) );
 if( auto s = t->rightSibling() )
  b->insertRight( to_binary( s ) );
 return( b );
 }

namespace detail {

/// the value written in the atom a, all of it
template< class T >
T fromAtom( const std::string & a )
{
 std::istringstream in( a );
 T v{};
 in >> v;
 ADS_PRE( ! in.fail() &&                     // e.g., not 8x for an int
          ( in.peek() == std::char_traits< char >::eof() ) );
 return( v );
 }

/// the binary tree of the list from L[ i ] on, i moved past it
template< class T >
BinaryTree< T > * readBinary( const std::vector< std::string > & L ,
                              std::size_t & i )
{
 ADS_PRE( ( i < L.size() ) && ( L[ i ] == "(" ) );
 ++i;
 ADS_PRE( i < L.size() );
 if( L[ i ] == ")" ) {                      // ()
  ++i;
  return( nullptr );
  }
 ADS_PRE( ( L[ i ] != "(" ) );               // the value of the root
 std::unique_ptr< BinaryTree< T > > t(       // freed if a contract throws
  new BinaryTree< T >( fromAtom< T >( L[ i++ ] ) ) );
 if( auto * l = readBinary< T >( L , i ) )
  t->insertLeft( l );
 if( auto * r = readBinary< T >( L , i ) )
  t->insertRight( r );
 ADS_PRE( ( i < L.size() ) && ( L[ i ] == ")" ) );
 ++i;
 return( t.release() );
 }

}  // end( namespace detail )

template< class T >
std::string to_list( const BinaryTree< T > * t )
{
 if( ! t )
  return( "()" );
 std::ostringstream out;
 out << "( " << t->read() << " " << to_list( t->left() ) << " "
     << to_list( t->right() ) << " )";
 return( out.str() );
 }

template< class T >
BinaryTree< T > * from_list( std::string_view s )
{
 const auto L = detail::listTokens( s );
 std::size_t i = 0;
 std::unique_ptr< BinaryTree< T > > t(       // freed if a contract throws
  detail::readBinary< T >( L , i ) );
 ADS_PRE( i == L.size() );
 return( t.release() );
 }

inline long dary_child( long i , long d , long j )
{
 ADS_PRE( ( i >= 0 ) && ( d >= 1 ) && ( j >= 1 ) && ( j <= d ) &&
          ( i <= ( std::numeric_limits< long >::max() - j ) / d ) );
 return( d * i + j );
 }

inline long dary_parent( long i , long d )
{
 ADS_PRE( ( i > 0 ) && ( d >= 1 ) );
 return( ( i - 1 ) / d );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BinaryTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
