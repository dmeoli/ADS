/*--------------------------------------------------------------------------*/
/*-------------------------- File CursorTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CursorTree [see CursorTree.h], with their contracts.
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
CursorTree< T >::CursorTree( const T & v )
{
 f_root = make( v , NIL );
 }

template< class T >
bool CursorTree< T >::isEmpty( void ) const
{
 return( f_root == NIL );
 }

template< class T >
int CursorTree< T >::root( void ) const
{
 return( f_root );
 }

template< class T >
int CursorTree< T >::parent( int n ) const
{
 return( node( n ).parent );
 }

template< class T >
int CursorTree< T >::leftmostChild( int n ) const
{
 return( node( n ).child );
 }

template< class T >
int CursorTree< T >::rightSibling( int n ) const
{
 return( node( n ).sibling );
 }

template< class T >
bool CursorTree< T >::isLeaf( int n ) const
{
 return( node( n ).child == NIL );
 }

template< class T >
bool CursorTree< T >::isLastSibling( int n ) const
{
 return( node( n ).sibling == NIL );
 }

template< class T >
const T & CursorTree< T >::read( int n ) const
{
 return( node( n ).value );
 }

template< class T >
void CursorTree< T >::write( int n , const T & v )
{
 node( n ).value = v;
 }

template< class T >
int CursorTree< T >::size( void ) const
{
 return( f_size );
 }

template< class T >
int CursorTree< T >::insertChild( int n , const T & v )
{
 node( n );
 const int c = make( v , n );
 f_N[ c ].sibling = f_N[ n ].child;
 f_N[ n ].child = c;
 ADS_POST( ( leftmostChild( n ) == c ) && ( parent( c ) == n ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int CursorTree< T >::insertSibling( int n , const T & v )
{
 ADS_PRE( node( n ).parent != NIL );
 const int p = f_N[ n ].parent;
 const int s = make( v , p );
 f_N[ s ].sibling = f_N[ n ].sibling;
 f_N[ n ].sibling = s;
 ADS_POST( ( rightSibling( n ) == s ) && ( parent( s ) == p ) );
 ADS_INVARIANT( linked() );
 return( s );
 }

template< class T >
int CursorTree< T >::insertChildTree( int n , const CursorTree & t , int s )
{
 node( n );
 const int c = copy( t , s , n );
 f_N[ c ].sibling = f_N[ n ].child;
 f_N[ n ].child = c;
 ADS_POST( ( leftmostChild( n ) == c ) && ( parent( c ) == n ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
int CursorTree< T >::insertSiblingTree( int n , const CursorTree & t , int s )
{
 ADS_PRE( node( n ).parent != NIL );
 const int p = f_N[ n ].parent;
 const int c = copy( t , s , p );
 f_N[ c ].sibling = f_N[ n ].sibling;
 f_N[ n ].sibling = c;
 ADS_POST( ( rightSibling( n ) == c ) && ( parent( c ) == p ) );
 ADS_INVARIANT( linked() );
 return( c );
 }

template< class T >
void CursorTree< T >::remove( int n )
{
 const int p = node( n ).parent;
 if( p == NIL ) f_root = NIL;
 else if( f_N[ p ].child == n ) f_N[ p ].child = f_N[ n ].sibling;
 else {
  int u = f_N[ p ].child;
  while( f_N[ u ].sibling != n ) u = f_N[ u ].sibling;
  f_N[ u ].sibling = f_N[ n ].sibling;
  }
 release_children( n );
 free_node( n );
 ADS_INVARIANT( linked() );
 }

template< class T >
std::vector< int > CursorTree< T >::previsit( void ) const
{
 std::vector< int > P;
 visit( f_root , -1 , P , 0 );
 return( P );
 }

template< class T >
std::vector< int > CursorTree< T >::postvisit( void ) const
{
 std::vector< int > P;
 visit( f_root , -1 , P , 1 );
 return( P );
 }

template< class T >
std::vector< int > CursorTree< T >::invisit( int i ) const
{
 std::vector< int > P;
 visit( f_root , i , P , 2 );
 return( P );
 }

template< class T >
auto CursorTree< T >::node( int n ) const -> const Node &
{
 ADS_PRE( ( n >= 0 ) && ( n < int( f_N.size() ) ) && f_N[ n ].used );
 return( f_N[ n ] );
 }

template< class T >
auto CursorTree< T >::node( int n ) -> Node &
{
 return( const_cast< Node & >( std::as_const( *this ).node( n ) ) );
 }

template< class T >
bool CursorTree< T >::linked( void ) const
{
 int count = 0;
 auto rec = [ & ]( auto & self , int n , int p ) -> bool {
  for( ; n != NIL ; n = f_N[ n ].sibling ) {
   if( ( n < 0 ) || ( n >= int( f_N.size() ) ) || ! f_N[ n ].used ||
       ( f_N[ n ].parent != p ) || ( ++count > f_size ) ||
       ! self( self , f_N[ n ].child , n ) )
    return( false );
   if( p == NIL ) break;                 // the root has no siblings
   }
  return( true );
  };
 return( rec( rec , f_root , NIL ) && ( count == f_size ) );
 }

template< class T >
int CursorTree< T >::make( const T & v , int p )
{
 ++f_size;
 if( f_free != NIL ) {
  const int n = f_free;
  f_free = f_N[ n ].sibling;
  f_N[ n ] = { v , p , NIL , NIL , true };
  return( n );
  }
 f_N.push_back( { v , p , NIL , NIL , true } );
 return( int( f_N.size() ) - 1 );
 }

template< class T >
void CursorTree< T >::free_node( int n )
{
 f_N[ n ].used = false;
 f_N[ n ].sibling = f_free;
 f_free = n;
 --f_size;
 }

template< class T >
void CursorTree< T >::release_children( int n )
{
 for( int u = f_N[ n ].child ; u != NIL ; ) {
  const int next = f_N[ u ].sibling;
  release_children( u );
  free_node( u );
  u = next;
  }
 }

template< class T >
int CursorTree< T >::copy( const CursorTree & t , int s , int p )
{
 const int n = make( t.node( s ).value , p );
 int last = NIL;
 for( int u = t.f_N[ s ].child ; u != NIL ; u = t.f_N[ u ].sibling ) {
  const int c = copy( t , u , n );
  if( last == NIL ) f_N[ n ].child = c;
  else f_N[ last ].sibling = c;
  last = c;
  }
 return( n );
 }

template< class T >
void CursorTree< T >::visit( int n , int i , std::vector< int > & P ,
                             int kind ) const
{
 if( n == NIL ) return;
 if( kind == 0 ) P.push_back( n );
 int k = 0;
 for( int u = f_N[ n ].child ; u != NIL ; u = f_N[ u ].sibling , ++k ) {
  if( ( kind == 2 ) && ( k == i ) ) P.push_back( n );
  visit( u , i , P , kind );
  }
 if( ( kind == 1 ) || ( ( kind == 2 ) && ( k <= i ) ) ) P.push_back( n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File CursorTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
