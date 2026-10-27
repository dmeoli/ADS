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
 if( ! t ) return( true );
 const K & k = nodeKey( t );
 if( ( lower && ! less( *lower , k ) ) || ( upper && ! less( k , *upper ) ) )
  return( false );
 return( keysBetween( nodeLeft( t ) , lower , &k , less ) &&
         keysBetween( nodeRight( t ) , &k , upper , less ) );
 }

/// the black height of t, the leaf nil counted, -1 if the subtree breaks
/// a constraint
template< class N , class Red >
long blackHeight( N t , Red & red )
{
 if( ! t ) return( 1 );
 const bool r = red( t );
 const auto l = nodeLeft( t ) , q = nodeRight( t );
 if( r && ( ( l && red( l ) ) || ( q && red( q ) ) ) ) return( -1 );
 const long hl = blackHeight( l , red ) , hr = blackHeight( q , red );
 if( ( hl < 0 ) || ( hl != hr ) ) return( -1 );
 return( hl + ( r ? 0 : 1 ) );
 }

/// the height of t, -1 if empty
template< class N >
long nodeHeight( N t )
{
 if( ! t ) return( -1 );
 const long l = nodeHeight( nodeLeft( t ) ) , r = nodeHeight( nodeRight( t ) );
 return( 1 + ( l > r ? l : r ) );
 }

/// the range search below t, the nodes examined added to *seen
template< class N , class K , class F , class Compare >
void rangeVisit( N t , const K & lo , const K & hi , F & f , Compare & less ,
                 std::size_t & seen )
{
 if( ! t ) return;
 ++seen;
 const auto & k = nodeKey( t );
 if( less( k , lo ) ) {                     // all the left subtree too
  rangeVisit( nodeRight( t ) , lo , hi , f , less , seen );
  return;
  }
 if( less( hi , k ) ) {                     // all the right subtree too
  rangeVisit( nodeLeft( t ) , lo , hi , f , less , seen );
  return;
  }
 rangeVisit( nodeLeft( t ) , lo , hi , f , less , seen );
 f( t );
 rangeVisit( nodeRight( t ) , lo , hi , f , less , seen );
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
 if( ( ! t ) || ( ! s ) ) return( ( ! t ) && ( ! s ) );
 return( eq( detail::nodeKey( t ) , detail::nodeKey( s ) ) &&
         equalTrees( detail::nodeLeft( t ) , detail::nodeLeft( s ) , eq ) &&
         equalTrees( detail::nodeRight( t ) , detail::nodeRight( s ) , eq ) );
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
  auto * t = new BinaryTree< T >( pre[ i ] );
  BinaryTree< T > * last = nullptr;
  while( ( ! S.empty() ) && ( j < in.size() ) &&
         ( S.back()->read() == in[ j ] ) ) {
   last = S.back();
   S.pop_back();
   ++j;
   }
  if( last ) last->insertRight( t );
  else S.back()->insertLeft( t );
  S.push_back( t );
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
