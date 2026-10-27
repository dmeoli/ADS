/*--------------------------------------------------------------------------*/
/*--------------------- File BinaryTreeAlgorithms.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BinaryTreeAlgorithms.h, with their contracts.
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

/// the depth of u and, in *root, the root of its tree
template< class N >
std::size_t depthAndRoot( N u , N * root )
{
 std::size_t d = 0;
 for( ; nodeParent( u ) ; u = nodeParent( u ) ) ++d;
 *root = u;
 return( d );
 }

/// a is an ancestor of u, u itself included
template< class N >
bool isAncestorOf( N a , N u )
{
 for( ; u ; u = nodeParent( u ) )
  if( u == a ) return( true );
 return( false );
 }

/// the keys of t in ( *lower , *upper ), nullptr for an infinite end
template< class N , class K , class Compare >
bool keysBetween( N t , const K * lower , const K * upper ,
                    Compare & less )
{
 // a previsit with a stack of its own, as the height may be n: each
 // subtree with the interval its ancestors leave to it
 struct Item { N u; const K * lo , * hi; };
 std::vector< Item > S = { Item{ t , lower , upper } };
 while( ! S.empty() ) {
  const Item x = S.back();
  S.pop_back();
  if( ! x.u ) continue;
  const K & k = nodeKey( x.u );
  if( ( x.lo && ! less( *x.lo , k ) ) || ( x.hi && ! less( k , *x.hi ) ) )
   return( false );
  S.push_back( Item{ nodeRight( x.u ) , &k , x.hi } );
  S.push_back( Item{ nodeLeft( x.u ) , x.lo , &k } );
  }
 return( true );
 }

/// the black height of t, the leaf nil counted, -1 if the subtree breaks
/// a constraint
template< class N , class Red >
long blackHeight( N t , Red & red )
{
 // a postvisit with a stack of its own, as the height may be n: each node
 // with its color, the subtrees done and the black height of the left one,
 // h that of the last subtree done; a broken constraint ends the visit, as
 // its -1 would go up to the root
 struct Item { N u; bool r; int k; long hl; };
 std::vector< Item > S;
 long h = 1;
 auto enter = [ & ]( N u ) {                // false if u breaks (V3)
  if( ! u ) {
   h = 1;
   return( true );
   }
  const bool r = red( u );
  const auto l = nodeLeft( u ) , q = nodeRight( u );
  if( r && ( ( l && red( l ) ) || ( q && red( q ) ) ) ) return( false );
  S.push_back( Item{ u , r , 0 , 0 } );
  return( true );
  };
 if( ! enter( t ) ) return( -1 );
 while( ! S.empty() ) {
  Item & x = S.back();
  if( x.k == 0 ) {
   x.k = 1;
   if( ! enter( nodeLeft( x.u ) ) ) return( -1 );
   }
  else if( x.k == 1 ) {
   x.k = 2;
   x.hl = h;
   if( ! enter( nodeRight( x.u ) ) ) return( -1 );
   }
  else {
   if( x.hl != h ) return( -1 );
   h = x.hl + ( x.r ? 0 : 1 );
   S.pop_back();
   }
  }
 return( h );
 }

/// the height of t, -1 if empty
template< class N >
long nodeHeight( N t )
{
 long h = -1;                               // by levels, with no recursion
 for( std::vector< N > L = { t } ; ; ++h ) {
  std::vector< N > M;
  for( N u : L )
   if( u ) {
    M.push_back( nodeLeft( u ) );
    M.push_back( nodeRight( u ) );
    }
  if( M.empty() ) return( h );
  L = std::move( M );
  }
 }

/// the range search below t, the nodes examined added to *seen
template< class N , class K , class F , class Compare >
void rangeVisit( N t , const K & lo , const K & hi , F & f , Compare & less ,
                 std::size_t & seen )
{
 // an invisit with a stack of its own, as the height may be n: a node out
 // of the range is passed by, with its subtree on the far side of the range
 std::vector< N > S;
 for( ; ; ) {
  while( t ) {
   ++seen;
   const auto & k = nodeKey( t );
   if( less( k , lo ) )                     // all the left subtree too
    t = nodeRight( t );
   else if( less( hi , k ) )                // all the right subtree too
    t = nodeLeft( t );
   else {
    S.push_back( t );
    t = nodeLeft( t );
    }
   }
  if( S.empty() ) return;
  t = S.back();
  S.pop_back();
  f( t );
  t = nodeRight( t );
  }
 }

/// the search tree of A[ i .. j - 1 ], its lower middle key at the root
template< class R , class T , class Make >
R minHeightRec( const std::vector< T > & A , std::size_t i , std::size_t j ,
                Make & make )
{
 if( i >= j ) return( nullptr );
 const std::size_t m = i + ( j - i - 1 ) / 2;
 R l = minHeightRec< R >( A , i , m , make );
 R r = minHeightRec< R >( A , m + 1 , j , make );
 return( make( A[ m ] , l , r ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< ParentBinaryNode N >
N lowestCommonAncestor( N u , N v )
{
 ADS_PRE( u && v );
 N ru = nullptr , rv = nullptr;
 std::size_t du = detail::depthAndRoot( u , &ru );
 std::size_t dv = detail::depthAndRoot( v , &rv );
 ADS_PRE( ru == rv );                       // the same tree
 const N u0 = u , v0 = v;
 for( ; du > dv ; --du ) u = detail::nodeParent( u );
 for( ; dv > du ; --dv ) v = detail::nodeParent( v );
 while( u != v ) {
  u = detail::nodeParent( u );
  v = detail::nodeParent( v );
  }
 ADS_POST( detail::isAncestorOf( u , u0 ) && detail::isAncestorOf( u , v0 ) );
 ADS_POST( [ & ]() {                        // no child is a common ancestor
  for( N c : { N( detail::nodeLeft( u ) ) , N( detail::nodeRight( u ) ) } )
   if( c && detail::isAncestorOf( c , u0 ) && detail::isAncestorOf( c , v0 ) )
    return( false );
  return( true );
  }() );
 return( u );
 }

template< KeyedBinaryNode N , KeyedBinaryNode M , class Equal >
bool equalTrees( N t , M s , Equal eq )
{
 // the two previsits with a stack of their own, as the height may be n
 std::vector< std::pair< N , M > > S = { { t , s } };
 while( ! S.empty() ) {
  const auto [ a , b ] = S.back();
  S.pop_back();
  if( ( ! a ) || ( ! b ) ) {
   if( a || b ) return( false );
   continue;
   }
  if( ! eq( detail::nodeKey( a ) , detail::nodeKey( b ) ) ) return( false );
  S.emplace_back( detail::nodeRight( a ) , detail::nodeRight( b ) );
  S.emplace_back( detail::nodeLeft( a ) , detail::nodeLeft( b ) );
  }
 return( true );
 }

template< KeyedBinaryNode N , class Compare >
bool isSearchTree( N t , Compare less )
{
 using K = std::remove_cvref_t< decltype( detail::nodeKey( t ) ) >;
 return( detail::keysBetween( t , static_cast< const K * >( nullptr ) ,
                                static_cast< const K * >( nullptr ) ,
                                less ) );
 }

template< BinaryNode N , class Red >
bool isRedBlackTree( N t , Red red )
{
 if( t && red( t ) ) return( false );
 return( detail::blackHeight( t , red ) > 0 );
 }

template< KeyedBinaryNode N , class K , class F , class Compare >
std::size_t searchTreeRange( N t , const K & lo , const K & hi , F && f ,
                             Compare less )
{
 ADS_PRE( ! less( hi , lo ) );
 std::size_t seen = 0;
 detail::rangeVisit( t , lo , hi , f , less , seen );
 return( seen );
 }

template< class T , class Make , class Compare >
auto minimumHeightSearchTree( const std::vector< T > & A , Make make ,
                              Compare less )
 -> std::invoke_result_t< Make & , const T & , std::nullptr_t ,
                          std::nullptr_t >
{
 using R = std::invoke_result_t< Make & , const T & , std::nullptr_t ,
                                 std::nullptr_t >;
 for( std::size_t i = 1 ; i < A.size() ; ++i )
  ADS_PRE( less( A[ i - 1 ] , A[ i ] ) );
 R t = detail::minHeightRec< R >( A , 0 , A.size() , make );
 ADS_AUDIT( detail::nodeHeight( t ) ==
            long( std::bit_width( A.size() ) ) - 1 );
 return( t );
 }

template< class T , class Compare >
 requires std::predicate< Compare & , const T & , const T & >
BinaryTree< T > * minimumHeightSearchTree( const std::vector< T > & A ,
                                           Compare less )
{
 return( minimumHeightSearchTree( A , []( const T & v , BinaryTree< T > * l ,
                                          BinaryTree< T > * r ) {
  auto * t = new BinaryTree< T >( v );
  if( l ) t->insertLeft( l );
  if( r ) t->insertRight( r );
  return( t );
  } , less ) );
 }

template< class T >
BinaryTree< T > * fromPreorderInorder( const std::vector< T > & pre ,
                                       const std::vector< T > & in )
{
 ADS_PRE( pre.size() == in.size() );
 if( pre.empty() ) return( nullptr );
 std::unique_ptr< BinaryTree< T > > root( new BinaryTree< T >( pre[ 0 ] ) );
 std::vector< BinaryTree< T > * > S( 1 , root.get() );   // right subtree open
 std::size_t j = 0;                                      // the next of in
 for( std::size_t i = 1 ; i < pre.size() ; ++i ) {
  std::unique_ptr< BinaryTree< T > > t(      // freed if a comparison throws
   new BinaryTree< T >( pre[ i ] ) );
  BinaryTree< T > * last = nullptr;
  while( ( ! S.empty() ) && ( j < in.size() ) &&
         ( S.back()->read() == in[ j ] ) ) {
   last = S.back();
   S.pop_back();
   ++j;
   }
  if( last ) last->insertRight( t.get() );
  else S.back()->insertLeft( t.get() );
  S.push_back( t.release() );               // now owned by root
  }
 ADS_PRE( [ & ]() {                         // the invisit is in
  std::size_t k = 0;
  for( auto * u : inorder( root.get() ) )
   if( !( u->read() == in[ k++ ] ) ) return( false );
  return( true );
  }() );
 return( root.release() );
 }

template< class T , class Compare >
BinaryTree< T > * cartesianBinaryTree( const std::vector< T > & A ,
                                       Compare less )
{
 const std::size_t n = A.size();
 if( n == 0 ) return( nullptr );
 const std::vector< std::size_t > P = cartesianTree( A , less );
 std::vector< BinaryTree< T > * > N( n );
 for( std::size_t i = 0 ; i < n ; ++i ) N[ i ] = new BinaryTree< T >( A[ i ] );
 BinaryTree< T > * root = nullptr;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( P[ i ] == n ) root = N[ i ];
  else if( i < P[ i ] ) N[ P[ i ] ]->insertLeft( N[ i ] );
  else N[ P[ i ] ]->insertRight( N[ i ] );
 ADS_AUDIT( [ & ]() {
  std::size_t k = 0;
  for( auto * u : inorder( root ) )
   if( u != N[ k++ ] ) return( false );
  return( k == n );
  }() );
 return( root );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File BinaryTreeAlgorithms.tpp -------------------*/
/*--------------------------------------------------------------------------*/
