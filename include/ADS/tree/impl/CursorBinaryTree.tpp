/*--------------------------------------------------------------------------*/
/*----------------------- File CursorBinaryTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CursorBinaryTree [see CursorBinaryTree.h], with their
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
CursorBinaryTree< T >::CursorBinaryTree( const T & v )
{
 f_root = make( v , NIL );
 }

template< class T >
bool CursorBinaryTree< T >::empty( void ) const
{
 return( f_root == NIL );
 }

template< class T >
int CursorBinaryTree< T >::root( void ) const
{
 return( f_root );
 }

template< class T >
int CursorBinaryTree< T >::parent( int n ) const
{
 return( node( n ).parent );
 }

template< class T >
int CursorBinaryTree< T >::left( int n ) const
{
 return( node( n ).left );
 }

template< class T >
int CursorBinaryTree< T >::right( int n ) const
{
 return( node( n ).right );
 }

template< class T >
bool CursorBinaryTree< T >::isLeaf( int n ) const
{
 return( ( left( n ) == NIL ) && ( right( n ) == NIL ) );
 }

template< class T >
const T & CursorBinaryTree< T >::read( int n ) const
{
 return( node( n ).value );
 }

template< class T >
void CursorBinaryTree< T >::write( int n , const T & v )
{
 node( n ).value = v;
 }

template< class T >
int CursorBinaryTree< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
int CursorBinaryTree< T >::setRoot( const T & v )
{
 ADS_PRE( f_root == NIL );
 f_root = make( v , NIL );
 ADS_INVARIANT( linked() );
 return( f_root );
 }

template< class T >
int CursorBinaryTree< T >::insertLeft( int n , const T & v )
{
 ADS_PRE( node( n ).left == NIL );
 const int c = make( v , n );
 f_N[ n ].left = c;
 ADS_POST( ( left( n ) == c ) && ( parent( c ) == n ) && isLeaf( c ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int CursorBinaryTree< T >::insertRight( int n , const T & v )
{
 ADS_PRE( node( n ).right == NIL );
 const int c = make( v , n );
 f_N[ n ].right = c;
 ADS_POST( ( right( n ) == c ) && ( parent( c ) == n ) && isLeaf( c ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int CursorBinaryTree< T >::insertLeft( int n , const CursorBinaryTree & t ,
                                       int s )
{
 ADS_PRE( node( n ).left == NIL );
 const int c = copy( t , s , n );
 f_N[ n ].left = c;
 ADS_POST( ( left( n ) == c ) && ( parent( c ) == n ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int CursorBinaryTree< T >::insertRight( int n , const CursorBinaryTree & t ,
                                        int s )
{
 ADS_PRE( node( n ).right == NIL );
 const int c = copy( t , s , n );
 f_N[ n ].right = c;
 ADS_POST( ( right( n ) == c ) && ( parent( c ) == n ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
void CursorBinaryTree< T >::remove( int n )
{
 const int p = node( n ).parent;
 if( p == NIL ) f_root = NIL;
 else if( f_N[ p ].left == n ) f_N[ p ].left = NIL;
 else f_N[ p ].right = NIL;
 release( n );
 ADS_INVARIANT( linked() );
 }

template< class T >
std::vector< int > CursorBinaryTree< T >::preorder( void ) const
{
 std::vector< int > P , S;                // S a stack of its own, as the
 if( f_root != NIL )                      // height may be n
  S.push_back( f_root );
 while( ! S.empty() ) {
  const int n = S.back();
  S.pop_back();
  P.push_back( n );
  if( f_N[ n ].right != NIL ) S.push_back( f_N[ n ].right );
  if( f_N[ n ].left != NIL ) S.push_back( f_N[ n ].left );
  }
 return( P );
 }

template< class T >
auto CursorBinaryTree< T >::node( int n ) const -> const Node &
{
 ADS_PRE( ( n >= 0 ) && ( n < int( f_N.size() ) ) && f_N[ n ].used );
 return( f_N[ n ] );
 }

template< class T >
auto CursorBinaryTree< T >::node( int n ) -> Node &
{
 return( const_cast< Node & >( std::as_const( *this ).node( n ) ) );
 }

template< class T >
bool CursorBinaryTree< T >::linked( void ) const
{
 // a previsit with a stack of its own, as the height may be n: each node
 // with the parent it must have; more than n nodes met is a cycle
 int count = 0;
 std::vector< std::pair< int , int > > S = { { f_root , NIL } };
 while( ! S.empty() ) {
  const auto [ n , p ] = S.back();
  S.pop_back();
  if( n == NIL ) continue;
  if( ( n < 0 ) || ( n >= int( f_N.size() ) ) || ! f_N[ n ].used ||
      ( f_N[ n ].parent != p ) || ( ++count > f_size ) )
   return( false );
  S.emplace_back( f_N[ n ].right , n );
  S.emplace_back( f_N[ n ].left , n );
  }
 return( count == f_size );
 }

template< class T >
int CursorBinaryTree< T >::make( const T & v , int p )
{
 ++f_size;
 if( f_free != NIL ) {
  const int n = f_free;
  f_free = f_N[ n ].left;
  f_N[ n ] = { v , p , NIL , NIL , true };
  return( n );
  }
 f_N.push_back( { v , p , NIL , NIL , true } );
 return( int( f_N.size() ) - 1 );
 }

template< class T >
void CursorBinaryTree< T >::release( int n )
{
 // a postvisit with a stack of its own, as the height may be n: each node
 // with the number of its subtrees done
 std::vector< std::pair< int , int > > S;
 if( n != NIL ) S.emplace_back( n , 0 );
 while( ! S.empty() ) {
  const int u = S.back().first;
  const int k = S.back().second++;
  if( k < 2 ) {
   const int c = k == 0 ? f_N[ u ].left : f_N[ u ].right;
   if( c != NIL ) S.emplace_back( c , 0 );
   continue;
   }
  S.pop_back();
  f_N[ u ].used = false;
  f_N[ u ].left = f_free;               // the free list
  f_free = u;
  --f_size;
  }
 }

template< class T >
int CursorBinaryTree< T >::copy( const CursorBinaryTree & t , int s , int p )
{
 if( s == NIL ) return( NIL );
 const int n = make( t.node( s ).value , p );
 // a previsit with a stack of its own, as the height may be n: each node
 // of t to copy with the copy of its parent and the side, left (0) or
 // right (1); places only, as make() may move the nodes, and t may be
 // this tree
 struct Item { int from , parent , side; };
 std::vector< Item > S;
 if( t.f_N[ s ].right != NIL ) S.push_back( Item{ t.f_N[ s ].right , n , 1 } );
 if( t.f_N[ s ].left != NIL ) S.push_back( Item{ t.f_N[ s ].left , n , 0 } );
 while( ! S.empty() ) {
  const Item x = S.back();
  S.pop_back();
  const int c = make( t.f_N[ x.from ].value , x.parent );
  ( x.side == 0 ? f_N[ x.parent ].left : f_N[ x.parent ].right ) = c;
  const int l = t.f_N[ x.from ].left , r = t.f_N[ x.from ].right;
  if( r != NIL ) S.push_back( Item{ r , c , 1 } );
  if( l != NIL ) S.push_back( Item{ l , c , 0 } );
  }
 return( n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File CursorBinaryTree.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
