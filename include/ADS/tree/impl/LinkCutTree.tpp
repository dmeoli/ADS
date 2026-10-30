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
 // the sizes after the check, with no clamp of n: g++ 16 at -O2 with LTO
 // takes an argument max( n , 0 ) as 0 in every call if one call has
 // n = 0
 ADS_PRE( n >= 0 );
 const std::size_t m = std::size_t( n );
 f_t.assign( m , Node{ { NONE , NONE } , NONE , 0 , id , id } );
 f_p.assign( m , NONE );
 f_pv.assign( m , id );
 }

template< class T , class Op >
int LinkCutTree< T , Op >::addNode( const T & x )
{
 f_t.push_back( Node{ { NONE , NONE } , NONE , 0 , x , x } );
 if( ! f_splay ) {
  f_p.push_back( NONE );
  f_pv.push_back( x );
  }
 return( n() - 1 );
 }

template< class T , class Op >
const T & LinkCutTree< T , Op >::value( int v ) const
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 return( f_splay ? f_t[ v ].val : f_pv[ v ] );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::setValue( int v , const T & x )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 if( ! f_splay ) {
  f_pv[ v ] = x;
  return;
  }
 access( v );                               // v the root of its splay tree:
 f_t[ v ].val = x;                          // no aggregate above it
 pull( v );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- SPLAY TREES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Op >
bool LinkCutTree< T , Op >::isRoot( int x ) const
{
 const int p = f_t[ x ].par;
 return( ( p == NONE ) || ( ( f_t[ p ].ch[ 0 ] != x ) &&
                            ( f_t[ p ].ch[ 1 ] != x ) ) );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::pull( int x )
{
 Node & a = f_t[ x ];
 T g = a.val;
 if( a.ch[ 0 ] != NONE )
  g = f_op( f_t[ a.ch[ 0 ] ].agg , g );
 if( a.ch[ 1 ] != NONE )
  g = f_op( g , f_t[ a.ch[ 1 ] ].agg );
 a.agg = std::move( g );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::push( int x )
{
 Node & a = f_t[ x ];
 if( ! a.rev )
  return;
 std::swap( a.ch[ 0 ] , a.ch[ 1 ] );
 if( a.ch[ 0 ] != NONE )
  f_t[ a.ch[ 0 ] ].rev ^= 1;
 if( a.ch[ 1 ] != NONE )
  f_t[ a.ch[ 1 ] ].rev ^= 1;
 a.rev = 0;
 }

template< class T , class Op >
void LinkCutTree< T , Op >::rotate( int x )
{
 Node * const t = f_t.data();
 const int p = t[ x ].par , g = t[ p ].par;
 const int dx = t[ p ].ch[ 1 ] == x;        // the side of x under p
 const int b = t[ x ].ch[ dx ^ 1 ];         // the subtree that moves
 if( ( g != NONE ) && ( ( t[ g ].ch[ 0 ] == p ) || ( t[ g ].ch[ 1 ] == p ) ) )
  t[ g ].ch[ t[ g ].ch[ 1 ] == p ] = x;     // else g is a path-parent
 t[ x ].par = g;
 t[ x ].ch[ dx ^ 1 ] = p;
 t[ p ].par = x;
 t[ p ].ch[ dx ] = b;
 if( b != NONE )
  t[ b ].par = p;
 pull( p );
 pull( x );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::splay( int x )
{
 // the reversals pushed from the root of the splay tree down to x
 f_stack.clear();
 for( int y = x ; ; y = f_t[ y ].par ) {
  f_stack.push_back( y );
  if( isRoot( y ) )
   break;
  }
 for( std::size_t i = f_stack.size() ; i-- > 0 ; )
  push( f_stack[ i ] );
 while( ! isRoot( x ) ) {
  const int p = f_t[ x ].par;
  if( ! isRoot( p ) ) {
   const int g = f_t[ p ].par;
   if( ( f_t[ g ].ch[ 0 ] == p ) == ( f_t[ p ].ch[ 0 ] == x ) )
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
 for( int x = v ; x != NONE ; x = f_t[ x ].par ) {
  splay( x );
  f_t[ x ].ch[ 1 ] = last;
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
  const Node & a = f_t[ x ];
  for( int c : { a.ch[ 0 ] , a.ch[ 1 ] } )
   if( ( c != NONE ) && ( f_t[ c ].par != x ) )
    return( false );
  if constexpr( std::equality_comparable< T > ) {
   T g = a.val;
   if( a.ch[ 0 ] != NONE )
    g = f_op( f_t[ a.ch[ 0 ] ].agg , g );
   if( a.ch[ 1 ] != NONE )
    g = f_op( g , f_t[ a.ch[ 1 ] ].agg );
   if( ! ( g == a.agg ) )
    return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE VECTOR OF PARENTS --------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Op >
int LinkCutTree< T , Op >::rootOf( int v , bool keep )
{
 const int * const p = f_p.data();         // the walk in registers
 long long k = 0;
 if( keep ) {                              // the path kept for an evert
  f_path.clear();                          // of v that may follow
  f_path.push_back( v );
  for( int u = p[ v ] ; u != NONE ; u = p[ u ] , ++k ) {
   f_path.push_back( u );
   v = u;
   }
  f_pathOf = f_path.front();
  f_pathAt = f_version;
  }
 else
  for( int u = p[ v ] ; u != NONE ; u = p[ u ] , ++k )
   v = u;
 f_steps += k;
 return( v );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::reverseUp( int v )
{
 int * const p = f_p.data();
 if( ( v == f_pathOf ) && ( f_pathAt == f_version ) ) {
  // the path of the last walk from v, unchanged since: no load waits for
  // the one before
  const int * const q = f_path.data();
  const std::size_t k = f_path.size();
  p[ q[ 0 ] ] = NONE;
  for( std::size_t i = 1 ; i < k ; ++i )
   p[ q[ i ] ] = q[ i - 1 ];
  f_steps += static_cast< long long >( k ) - 1;
  ++f_version;
  return;
  }
 long long k = 0;
 for( int prev = NONE ; v != NONE ; ++k ) {
  const int up = p[ v ];
  p[ v ] = prev;
  prev = v;
  v = up;
  }
 f_steps += k;
 ++f_version;
 }

template< class T , class Op >
void LinkCutTree< T , Op >::recount( void )
{
 // the mean of the recent operations, by groups of 16, which follows the
 // growth of the trees; the splay trees once the paths are long, each
 // tree of the forest a path-parent pointer per node, its splay trees
 // single nodes
 f_mean = 0.75 * f_mean + ( 0.25 / 16 ) * double( f_steps );
 f_steps = 0;
 if( ( f_ops >= 64 ) && ( f_mean > f_limit ) )
  useSplay();
 }

template< class T , class Op >
void LinkCutTree< T , Op >::useSplay( void )
{
 if( f_splay )
  return;
 for( int v = 0 ; v < n() ; ++v ) {
  Node & a = f_t[ v ];
  a.ch[ 0 ] = a.ch[ 1 ] = NONE;
  a.par = f_p[ v ];
  a.rev = 0;
  a.val = a.agg = f_pv[ v ];
  }
 f_p.clear();
 f_p.shrink_to_fit();
 f_pv.clear();
 f_pv.shrink_to_fit();
 f_splay = true;
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- DYNAMIC TREES -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Op >
void LinkCutTree< T , Op >::evert( int v )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 if( ! f_splay ) {
  reverseUp( v );
  count();
  return;
  }
 access( v );
 f_t[ v ].rev ^= 1;
 }

template< class T , class Op >
int LinkCutTree< T , Op >::findRoot( int v )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 if( ! f_splay ) {
  const int r = rootOf( v );
  count();
  return( r );
  }
 access( v );
 int r = v;
 for( push( r ) ; f_t[ r ].ch[ 0 ] != NONE ; push( r ) )
  r = f_t[ r ].ch[ 0 ];
 splay( r );
 return( r );
 }

template< class T , class Op >
bool LinkCutTree< T , Op >::connected( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) );
 if( u == v )
  return( true );
 if( ! f_splay ) {
  const bool same = rootOf( u , true ) == rootOf( v );
  count();
  return( same );
  }
 // after access( u ), u roots the splay tree of the path from the root of
 // its tree, with no parent; access( v ) gives it one if and only if it
 // climbs into that path, i.e., if v is in the same tree
 access( u );
 access( v );
 return( f_t[ u ].par != NONE );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::link( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          ! connected( u , v ) );
 if( ! f_splay ) {
  reverseUp( u );
  f_p[ u ] = v;
  ++f_version;
  count();
  return;
  }
 evert( u );
 f_t[ u ].par = v;
 ADS_AUDIT( wellFormed() );
 }

template< class T , class Op >
void LinkCutTree< T , Op >::cut( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          ( u != v ) );
 if( ! f_splay ) {                         // v a child of u, the root
  reverseUp( u );
  ADS_PRE( f_p[ v ] == u );
  if( f_p[ v ] == u )
   f_p[ v ] = NONE;
  ++f_version;
  count();
  return;
  }
 evert( u );
 access( v );
 // the path from u to v is u , v alone if the edge is there: u the left
 // child of v, with no child of its own
 push( v );
 const bool edge = ( f_t[ v ].ch[ 0 ] == u ) &&
                   ( f_t[ u ].ch[ 0 ] == NONE ) &&
                   ( f_t[ u ].ch[ 1 ] == NONE );
 ADS_PRE( edge );
 if( ! edge )
  return;
 f_t[ v ].ch[ 0 ] = NONE;
 f_t[ u ].par = NONE;
 pull( v );
 ADS_AUDIT( wellFormed() );
 }

template< class T , class Op >
bool LinkCutTree< T , Op >::cutParent( int v )
{
 ADS_PRE( ( v >= 0 ) && ( v < n() ) );
 if( ! f_splay ) {
  const bool had = f_p[ v ] != NONE;
  f_p[ v ] = NONE;
  ++f_version;
  return( had );
  }
 access( v );
 push( v );
 const int l = f_t[ v ].ch[ 0 ];
 if( l == NONE )
  return( false );
 f_t[ v ].ch[ 0 ] = NONE;
 f_t[ l ].par = NONE;
 pull( v );
 ADS_AUDIT( wellFormed() );
 return( true );
 }

template< class T , class Op >
int LinkCutTree< T , Op >::lca( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          connected( u , v ) );
 if( ! f_splay ) {                         // by the depths
  int du = 0 , dv = 0;
  for( int x = u ; f_p[ x ] != NONE ; x = f_p[ x ] ) ++du;
  for( int x = v ; f_p[ x ] != NONE ; x = f_p[ x ] ) ++dv;
  f_steps += du + dv;
  for( ; du > dv ; --du ) u = f_p[ u ];
  for( ; dv > du ; --dv ) v = f_p[ v ];
  for( ; u != v ; f_steps += 2 ) {
   u = f_p[ u ];
   v = f_p[ v ];
   }
  count();
  return( u );
  }
 access( u );
 return( access( v ) );
 }

template< class T , class Op >
T LinkCutTree< T , Op >::aggregate( int u , int v )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          connected( u , v ) );
 if( ! f_splay ) {                         // up from v to u, the root
  reverseUp( u );
  const int * const p = f_p.data();
  const T * const val = f_pv.data();
  T g = val[ v ];
  long long k = 0;
  for( int x = v ; x != u ; ++k ) {
   x = p[ x ];
   g = f_op( g , val[ x ] );
   }
  f_steps += k;
  count();
  return( g );
  }
 evert( u );
 access( v );
 return( f_t[ v ].agg );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LinkCutTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
