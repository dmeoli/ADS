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
void ChildListTree< T >::release( int n )
{
 auto & C = f_N[ n ].children;
 while( ! C.empty() ) {
  const int c = C.read( C.head() );
  C.remove( C.head() );
  release( c );
  }
 f_N[ n ].used = false;
 f_N[ n ].parent = f_free;
 f_N[ n ].where = nullptr;
 f_free = n;
 --f_size;
 }

template< class T >
void ChildListTree< T >::visit( int n , int i , std::vector< int > & P ,
                                int kind ) const
{
 const auto & C = f_N[ n ].children;
 if( kind == 0 ) P.push_back( n );
 int k = 0;
 for( auto q = C.head() ; ! C.finished( q ) ; q = C.next( q ) , ++k ) {
  if( ( kind == 2 ) && ( k == i ) ) P.push_back( n );
  visit( C.read( q ) , i , P , kind );
  }
 if( ( kind == 1 ) || ( ( kind == 2 ) && ( k <= i ) ) ) P.push_back( n );
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
