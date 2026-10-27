/*--------------------------------------------------------------------------*/
/*------------------------- File ChildListTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ChildListTree [see ChildListTree.h], with their
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

namespace ADS {

template< class T >
ChildListTree< T >::ChildListTree( const T & v )
{
 f_root = make( v , NIL );
 }

template< class T >
ChildListTree< T >::ChildListTree( const ChildListTree & o )
 : f_N( o.f_N ) , f_root( o.f_root ) , f_free( o.f_free ) ,
   f_size( o.f_size )
{
 for( Node & u : f_N )                      // the cells of the new lists
  for( auto q = u.children.head() ; ! u.children.finished( q ) ;
       q = u.children.next( q ) )
   f_N[ u.children.read( q ) ].where = q;
 }

template< class T >
ChildListTree< T > & ChildListTree< T >::operator=( const ChildListTree & o )
{
 if( this != &o )
  *this = ChildListTree( o );
 return( *this );
 }

template< class T >
bool ChildListTree< T >::empty( void ) const
{
 return( f_root == NIL );
 }

template< class T >
int ChildListTree< T >::root( void ) const
{
 return( f_root );
 }

template< class T >
int ChildListTree< T >::parent( int n ) const
{
 return( node( n ).parent );
 }

template< class T >
int ChildListTree< T >::leftmostChild( int n ) const
{
 const auto & C = node( n ).children;
 return( C.empty() ? NIL : C.read( C.head() ) );
 }

template< class T >
int ChildListTree< T >::rightSibling( int n ) const
{
 const Node & u = node( n );
 if( u.parent == NIL )
  return( NIL );
 const auto & C = f_N[ u.parent ].children;
 const auto q = C.next( u.where );
 return( C.finished( q ) ? NIL : C.read( q ) );
 }

template< class T >
bool ChildListTree< T >::isLeaf( int n ) const
{
 return( node( n ).children.empty() );
 }

template< class T >
bool ChildListTree< T >::isLastSibling( int n ) const
{
 return( rightSibling( n ) == NIL );
 }

template< class T >
const List< int > & ChildListTree< T >::children( int n ) const
{
 return( node( n ).children );
 }

template< class T >
const T & ChildListTree< T >::read( int n ) const
{
 return( node( n ).value );
 }

template< class T >
void ChildListTree< T >::write( int n , const T & v )
{
 node( n ).value = v;
 }

template< class T >
int ChildListTree< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
int ChildListTree< T >::insertChild( int n , const T & v )
{
 (void) node( n );                           // n is a node
 const int c = make( v , n );                // may move the nodes
 auto & C = f_N[ n ].children;
 f_N[ c ].where = C.insert( C.head() , c );
 ADS_POST( ( leftmostChild( n ) == c ) && ( parent( c ) == n ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int ChildListTree< T >::insertSibling( int n , const T & v )
{
 const int p = node( n ).parent;
 ADS_PRE( p != NIL );                        // n is not the root
 const int c = make( v , p );
 auto & C = f_N[ p ].children;
 f_N[ c ].where = C.insert( C.next( f_N[ n ].where ) , c );
 ADS_POST( ( rightSibling( n ) == c ) && ( parent( c ) == p ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int ChildListTree< T >::insertChildTree( int n , const ChildListTree & t ,
                                         int s )
{
 (void) node( n );                           // n is a node
 const int c = copy( t , s , n );            // may move the nodes
 auto & C = f_N[ n ].children;
 f_N[ c ].where = C.insert( C.head() , c );
 ADS_POST( ( leftmostChild( n ) == c ) && ( parent( c ) == n ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int ChildListTree< T >::insertSiblingTree( int n , const ChildListTree & t ,
                                           int s )
{
 const int p = node( n ).parent;
 ADS_PRE( p != NIL );                        // n is not the root
 const int c = copy( t , s , p );
 auto & C = f_N[ p ].children;
 f_N[ c ].where = C.insert( C.next( f_N[ n ].where ) , c );
 ADS_POST( ( rightSibling( n ) == c ) && ( parent( c ) == p ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
void ChildListTree< T >::remove( int n )
{
 const int p = node( n ).parent;
 if( p == NIL )
  f_root = NIL;
 else
  f_N[ p ].children.remove( f_N[ n ].where );
 release( n );
 ADS_INVARIANT( linked() );
 }

template< class T >
std::vector< int > ChildListTree< T >::previsit( void ) const
{
 std::vector< int > P;
 if( f_root != NIL ) visit( f_root , 0 , P , 0 );
 return( P );
 }

template< class T >
std::vector< int > ChildListTree< T >::postvisit( void ) const
{
 std::vector< int > P;
 if( f_root != NIL ) visit( f_root , 0 , P , 1 );
 return( P );
 }

template< class T >
std::vector< int > ChildListTree< T >::invisit( int i ) const
{
 std::vector< int > P;
 if( f_root != NIL ) visit( f_root , i , P , 2 );
 return( P );
 }

template< class T >
const typename ChildListTree< T >::Node &
ChildListTree< T >::node( int n ) const
{
 ADS_PRE( ( n >= 0 ) && ( n < int( f_N.size() ) ) && f_N[ n ].used );
 return( f_N[ n ] );
 }

template< class T >
typename ChildListTree< T >::Node & ChildListTree< T >::node( int n )
{
 ADS_PRE( ( n >= 0 ) && ( n < int( f_N.size() ) ) && f_N[ n ].used );
 return( f_N[ n ] );
 }

template< class T >
int ChildListTree< T >::make( const T & v , int p )
{
 int c = f_free;
 if( c == NIL ) {
  c = int( f_N.size() );
  f_N.push_back( Node{ v , p , List< int >() , nullptr , true } );
  }
 else {
  f_free = f_N[ c ].parent;
  f_N[ c ].value = v;
  f_N[ c ].parent = p;
  f_N[ c ].used = true;
  }
 ++f_size;
 return( c );
 }

template< class T >
int ChildListTree< T >::copy( const ChildListTree & t , int s , int p )
{
 const int r = make( t.node( s ).value , p );
 // a previsit with a stack of its own, as the height may be n: each node
 // of t with its copy, which gets the copies of its children in order;
 // places only, as make() may move the nodes, and t may be this tree
 std::vector< std::pair< int , int > > S = { { s , r } };
 std::vector< int > X;
 while( ! S.empty() ) {
  const auto [ u , c ] = S.back();
  S.pop_back();
  X.clear();                                // the children of u, read
  const auto & U = t.f_N[ u ].children;     // before make() moves them
  for( auto q = U.head() ; ! U.finished( q ) ; q = U.next( q ) )
   X.push_back( U.read( q ) );
  for( int x : X ) {
   const int y = make( t.f_N[ x ].value , c );
   auto & D = f_N[ c ].children;
   f_N[ y ].where = D.insert( D.next( D.tail() ) , y );
   S.emplace_back( x , y );
   }
  }
 return( r );
 }

template< class T >
void ChildListTree< T >::release( int n )
{
 // a postvisit with a stack of its own, as the height may be n: the first
 // child of the node on top is taken out of its list and goes on top,
 // and the node is freed once its list is empty
 std::vector< int > S = { n };
 while( ! S.empty() ) {
  const int u = S.back();
  auto & C = f_N[ u ].children;
  if( ! C.empty() ) {
   S.push_back( C.read( C.head() ) );
   C.remove( C.head() );
   continue;
   }
  S.pop_back();
  f_N[ u ].used = false;
  f_N[ u ].parent = f_free;
  f_N[ u ].where = nullptr;
  f_free = u;
  --f_size;
  }
 }

template< class T >
void ChildListTree< T >::visit( int n , int i , std::vector< int > & P ,
                                int kind ) const
{
 // with a stack of its own, as the height may be n: each node with the
 // cell of its next child and the number k of its children entered
 struct Item { int n; typename List< int >::Pos q; int k; };
 if( kind == 0 ) P.push_back( n );
 std::vector< Item > S = { Item{ n , f_N[ n ].children.head() , 0 } };
 while( ! S.empty() ) {
  Item & x = S.back();
  const auto & C = f_N[ x.n ].children;
  if( C.finished( x.q ) ) {
   if( ( kind == 1 ) || ( ( kind == 2 ) && ( x.k <= i ) ) )
    P.push_back( x.n );
   S.pop_back();
   continue;
   }
  if( ( kind == 2 ) && ( x.k == i ) ) P.push_back( x.n );
  const int u = C.read( x.q );
  x.q = C.next( x.q );
  ++x.k;
  if( kind == 0 ) P.push_back( u );
  S.push_back( Item{ u , f_N[ u ].children.head() , 0 } );
  }
 }

template< class T >
bool ChildListTree< T >::linked( void ) const
{
 if( f_root == NIL )
  return( f_size == 0 );
 int count = 0;
 std::vector< int > S = { f_root };
 while( ! S.empty() ) {
  const int n = S.back();
  S.pop_back();
  if( ( n < 0 ) || ( n >= int( f_N.size() ) ) || ! f_N[ n ].used ||
      ( ++count > f_size ) )
   return( false );
  const auto & C = f_N[ n ].children;
  for( auto q = C.head() ; ! C.finished( q ) ; q = C.next( q ) ) {
   const int c = C.read( q );
   if( ( c < 0 ) || ( c >= int( f_N.size() ) ) ||
       ( f_N[ c ].parent != n ) || ( f_N[ c ].where != q ) )
    return( false );
   S.push_back( c );
   }
  }
 return( count == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ChildListTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
