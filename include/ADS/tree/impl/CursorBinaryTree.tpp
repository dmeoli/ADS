/*--------------------------------------------------------------------------*/
/*----------------------- File CursorBinaryTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CursorBinaryTree [see CursorBinaryTree.h], with their
 * contracts.
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
CursorBinaryTree< T >::CursorBinaryTree( const T & v )
{
 f_root = make( v , NIL );
 }

template< class T >
bool CursorBinaryTree< T >::isEmpty( void ) const
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
 std::vector< int > P;
 auto rec = [ & ]( auto & self , int n ) -> void {
  if( n == NIL ) return;
  P.push_back( n );
  self( self , f_N[ n ].left );
  self( self , f_N[ n ].right );
  };
 rec( rec , f_root );
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
 int count = 0;
 auto rec = [ & ]( auto & self , int n , int p ) -> bool {
  if( n == NIL ) return( true );
  if( ( n < 0 ) || ( n >= int( f_N.size() ) ) || ! f_N[ n ].used ||
      ( f_N[ n ].parent != p ) || ( ++count > f_size ) )
   return( false );
  return( self( self , f_N[ n ].left , n ) &&
          self( self , f_N[ n ].right , n ) );
  };
 return( rec( rec , f_root , NIL ) && ( count == f_size ) );
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
 if( n == NIL ) return;
 release( f_N[ n ].left );
 release( f_N[ n ].right );
 f_N[ n ].used = false;
 f_N[ n ].left = f_free;                // the free list
 f_free = n;
 --f_size;
 }

template< class T >
int CursorBinaryTree< T >::copy( const CursorBinaryTree & t , int s , int p )
{
 if( s == NIL ) return( NIL );
 const int n = make( t.node( s ).value , p );
 const int l = copy( t , t.f_N[ s ].left , n );
 const int r = copy( t , t.f_N[ s ].right , n );
 f_N[ n ].left = l;
 f_N[ n ].right = r;
 return( n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File CursorBinaryTree.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
