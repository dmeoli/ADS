/*--------------------------------------------------------------------------*/
/*-------------------------- File BinaryTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BinaryTree [see BinaryTree.h], with their contracts.
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

namespace detail {

/// the binary tree of the values of the nodes reached from t, each one
/// with the images of left( u ) and right( u ) as its children, made in
/// previsit; a postvisit with a stack of its own, as the height may be n,
/// each node hung under its parent when its subtrees are done, and freed
/// if anything throws before
template< class T , class U , class Left , class Right >
BinaryTree< T > * binaryImage( const U * t , Left left , Right right )
{
 using B = BinaryTree< T >;
 struct Item { const U * from; std::unique_ptr< B > to; int k; };
 std::vector< Item > S;
 std::unique_ptr< B > b( new B( t->read() ) );
 S.push_back( Item{ t , std::move( b ) , 0 } );
 for( ; ; ) {
  Item & x = S.back();
  if( x.k < 2 ) {                           // the next subtree, if any
   const U * c = x.k == 0 ? left( x.from ) : right( x.from );
   ++x.k;
   if( c ) {
    std::unique_ptr< B > n( new B( c->read() ) );
    S.push_back( Item{ c , std::move( n ) , 0 } );
    }
   continue;
   }
  b = std::move( x.to );                    // done: hung under its parent
  S.pop_back();
  if( S.empty() ) return( b.release() );
  if( S.back().k == 1 ) S.back().to->insertLeft( b.get() );
  else S.back().to->insertRight( b.get() );
  b.release();
  }
 }

}  // end( namespace detail )

template< class T >
BinaryTree< T > * copy( const BinaryTree< T > * t )
{
 if( ! t ) return( nullptr );
 return( detail::binaryImage< T >( t ,
  []( const BinaryTree< T > * u ) { return( u->left() ); } ,
  []( const BinaryTree< T > * u ) { return( u->right() ); } ) );
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
 return( detail::binaryImage< T >( t ,
  []( const Tree< T > * u ) { return( u->leftmostChild() ); } ,
  []( const Tree< T > * u ) { return( u->rightSibling() ); } ) );
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
 // the descent by the grammar with a stack of its own, as the height may
 // be n: each node open with the number of its subtrees begun, hung under
 // its parent at its ")"; the nodes freed if a contract throws
 using B = BinaryTree< T >;
 struct Item { std::unique_ptr< B > t; int k; };
 std::vector< Item > S;
 for( ; ; ) {
  ADS_PRE( ( i < L.size() ) && ( L[ i ] == "(" ) );    // a subtree
  ++i;
  ADS_PRE( i < L.size() );
  if( L[ i ] == ")" ) {                     // ()
   ++i;
   if( S.empty() )
    return( nullptr );
   }
  else {
   ADS_PRE( ( L[ i ] != "(" ) );             // the value of its root
   std::unique_ptr< B > t( new B( fromAtom< T >( L[ i++ ] ) ) );
   S.push_back( Item{ std::move( t ) , 0 } );
   }
  for( ; ; ) {                              // the nodes now complete
   if( S.back().k < 2 ) {                   // its left, then right subtree
    ++S.back().k;
    break;
    }
   ADS_PRE( ( i < L.size() ) && ( L[ i ] == ")" ) );
   ++i;
   std::unique_ptr< B > t = std::move( S.back().t );
   S.pop_back();
   if( S.empty() )
    return( t.release() );
   if( S.back().k == 1 ) S.back().t->insertLeft( t.get() );
   else S.back().t->insertRight( t.get() );
   t.release();
   }
  }
 }

}  // end( namespace detail )

template< class T >
std::string to_list( const BinaryTree< T > * t )
{
 // a previsit with a stack of its own, as the height may be n: each node
 // with the number of its subtrees written
 std::ostringstream out;
 std::vector< std::pair< const BinaryTree< T > * , int > > S;
 auto open = [ & ]( const BinaryTree< T > * u ) {
  if( ! u )
   out << "()";
  else {
   out << "( " << u->read() << " ";
   S.emplace_back( u , 0 );
   }
  };
 open( t );
 while( ! S.empty() ) {
  const auto [ u , k ] = S.back();
  if( k == 2 ) {
   S.pop_back();
   out << " )";
   continue;
   }
  S.back().second = k + 1;
  if( k == 1 ) out << " ";
  open( k == 0 ? u->left() : u->right() );
  }
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
