/*--------------------------------------------------------------------------*/
/*-------------------------- File LinkCutTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LinkCutTree [see LinkCutTree.h], with their contracts.
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

template< class T , class Op >
LinkCutTree< T , Op >::LinkCutTree( int n , const T & id , Op op )
 : f_id( id ) , f_op( std::move( op ) )
{
 ADS_PRE( n >= 0 );
 const std::size_t k = std::size_t( n < 0 ? 0 : n );
 f_par.assign( k , NONE );
 f_left.assign( k , NONE );
 f_right.assign( k , NONE );
 f_rev.assign( k , 0 );
 f_val.assign( k , id );
 f_agg.assign( k , id );
 }

template< class T , class Op >
int LinkCutTree< T , Op >::addNode( const T & x )
{
 f_par.push_back( NONE );
 f_left.push_back( NONE );
 f_right.push_back( NONE );
 f_rev.push_back( 0 );
 f_val.push_back( x );
 f_agg.push_back( x );
 return( n() - 1 );
 }

template< class T , class Op >
const T & LinkCutTree< T , Op >::value( int v ) const
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 return( f_val[ v ] );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::setValue( int v , const T & x )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 access( v );                               // v the root of its splay tree:
 f_val[ v ] = x;                            // no aggregate above it
 pull( v );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- SPLAY TREES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Op >
bool LinkCutTree< T , Op >::isRoot( int x ) const
{
 const int p = f_par[ x ];
 return( ( p == NONE ) || ( ( f_left[ p ] != x ) && ( f_right[ p ] != x ) ) );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::pull( int x )
{
 T a = f_val[ x ];
 if( f_left[ x ] != NONE )
  a = f_op( f_agg[ f_left[ x ] ] , a );
 if( f_right[ x ] != NONE )
  a = f_op( a , f_agg[ f_right[ x ] ] );
 f_agg[ x ] = std::move( a );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::push( int x )
{
 if( ! f_rev[ x ] )
  return;
 std::swap( f_left[ x ] , f_right[ x ] );
 if( f_left[ x ] != NONE )
  f_rev[ f_left[ x ] ] ^= 1;
 if( f_right[ x ] != NONE )
  f_rev[ f_right[ x ] ] ^= 1;
 f_rev[ x ] = 0;
 }

template< class T , class Op >
void LinkCutTree< T , Op >::rotate( int x )
{
 const int p = f_par[ x ] , g = f_par[ p ];
 const bool pRoot = isRoot( p );
 if( f_left[ p ] == x ) {
  f_left[ p ] = f_right[ x ];
  if( f_left[ p ] != NONE )
   f_par[ f_left[ p ] ] = p;
  f_right[ x ] = p;
  }
 else {
  f_right[ p ] = f_left[ x ];
  if( f_right[ p ] != NONE )
   f_par[ f_right[ p ] ] = p;
  f_left[ x ] = p;
  }
 f_par[ p ] = x;
 f_par[ x ] = g;
 if( ! pRoot ) {                            // else g is a path-parent
  if( f_left[ g ] == p )
   f_left[ g ] = x;
  else
   f_right[ g ] = x;
  }
 pull( p );
 pull( x );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::splay( int x )
{
 // the reversals pushed from the root of the splay tree down to x
 f_stack.clear();
 for( int y = x ; ; y = f_par[ y ] ) {
  f_stack.push_back( y );
  if( isRoot( y ) )
   break;
  }
 for( std::size_t i = f_stack.size() ; i-- > 0 ; )
  push( f_stack[ i ] );
 while( ! isRoot( x ) ) {
  const int p = f_par[ x ];
  if( ! isRoot( p ) ) {
   const int g = f_par[ p ];
   if( ( f_left[ g ] == p ) == ( f_left[ p ] == x ) )
    rotate( p );                            // zig-zig
   else
    rotate( x );                            // zig-zag
   }
  rotate( x );
  }
 }

template< class T , class Op >
int LinkCutTree< T , Op >::access( int v )
{
 int last = NONE;
 for( int x = v ; x != NONE ; x = f_par[ x ] ) {
  splay( x );
  f_right[ x ] = last;
  pull( x );
  last = x;
  }
 splay( v );
 return( last );
 }

template< class T , class Op >
bool LinkCutTree< T , Op >::wellFormed( void ) const
{
 for( int x = 0 ; x < n() ; ++x ) {
  for( int c : { f_left[ x ] , f_right[ x ] } )
   if( ( c != NONE ) && ( f_par[ c ] != x ) )
    return( false );
  if constexpr( std::equality_comparable< T > ) {
   T a = f_val[ x ];
   if( f_left[ x ] != NONE )
    a = f_op( f_agg[ f_left[ x ] ] , a );
   if( f_right[ x ] != NONE )
    a = f_op( a , f_agg[ f_right[ x ] ] );
   if( ! ( a == f_agg[ x ] ) )
    return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- DYNAMIC TREES -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Op >
void LinkCutTree< T , Op >::evert( int v )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 access( v );
 f_rev[ v ] ^= 1;
 }

template< class T , class Op >
int LinkCutTree< T , Op >::findRoot( int v )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 access( v );
 int r = v;
 for( push( r ) ; f_left[ r ] != NONE ; push( r ) )
  r = f_left[ r ];
 splay( r );
 return( r );
 }

template< class T , class Op >
bool LinkCutTree< T , Op >::connected( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) );
 return( ( u == v ) || ( findRoot( u ) == findRoot( v ) ) );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::link( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          ! connected( u , v ) );
 evert( u );
 f_par[ u ] = v;
 ADS_AUDIT( wellFormed() );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::cut( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          ( u != v ) );
 evert( u );
 access( v );
 // the path from u to v is u , v alone if the edge is there: u the left
 // child of v, with no child of its own
 push( v );
 const bool edge = ( f_left[ v ] == u ) && ( f_left[ u ] == NONE ) &&
                   ( f_right[ u ] == NONE );
 ADS_PRE( edge );
 if( ! edge )
  return;
 f_left[ v ] = NONE;
 f_par[ u ] = NONE;
 pull( v );
 ADS_AUDIT( wellFormed() );
 }

template< class T , class Op >
bool LinkCutTree< T , Op >::cutParent( int v )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 access( v );
 push( v );
 const int l = f_left[ v ];
 if( l == NONE )
  return( false );
 f_left[ v ] = NONE;
 f_par[ l ] = NONE;
 pull( v );
 ADS_AUDIT( wellFormed() );
 return( true );
 }

template< class T , class Op >
int LinkCutTree< T , Op >::lca( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          connected( u , v ) );
 access( u );
 return( access( v ) );
 }

template< class T , class Op >
T LinkCutTree< T , Op >::aggregate( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          connected( u , v ) );
 evert( u );
 access( v );
 return( f_agg[ v ] );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkCutTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
