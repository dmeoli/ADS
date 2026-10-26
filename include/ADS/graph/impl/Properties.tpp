/*--------------------------------------------------------------------------*/
/*-------------------------- File Properties.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The properties of a graph of Properties.h [see Properties.h], with their
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

namespace detail {

/// the nodes in the order of a visit in breadth from r, the distances
/// (INF if not reached) and the node farthest from r, the first met
template< Graph G >
std::pair< std::vector< int > , int > farthest( const G & g , int r )
{
 std::vector< int > d( g.bound() , INF ) , Q = { r };
 d[ r ] = 0;
 for( std::size_t h = 0 ; h < Q.size() ; ++h )
  g.forEachAdj( Q[ h ] , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ Q[ h ] ] + 1;
    Q.push_back( v );
    }
   } );
 return( std::pair( d , Q.back() ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------------- TREES ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
bool isTree( const G & g )
{
 ADS_PRE( ! g.directed() && ( g.n() >= 1 ) );
 long ends = 0;                          // each edge counted at both ends
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int ) { ++ends; } );
 if( ends != 2 * long( g.n() - 1 ) )
  return( false );
 const auto d = detail::farthest( g , g.V().front() ).first;
 for( int u : g.V() )
  if( d[ u ] == INF )
   return( false );
 return( true );
 }

template< Graph G >
int diameter( const G & g )
{
 ADS_PRE( g.n() >= 1 );
 int D = 0;
 for( int u : g.V() ) {
  const auto d = detail::farthest( g , u ).first;
  for( int v : g.V() ) {
   ADS_PRE( d[ v ] != INF );
   D = std::max( D , d[ v ] );
   }
  }
 return( D );
 }

template< Graph G >
int treeDiameter( const G & g )
{
 ADS_PRE( isTree( g ) );
 const int a = detail::farthest( g , g.V().front() ).second;
 const auto [ d , b ] = detail::farthest( g , a );
 return( d[ b ] );
 }

template< class T >
std::pair< int , int > treeDiameter( const BinaryTree< T > * t )
{
 if( ! t )
  return( std::pair( -1 , 0 ) );
 const auto [ hl , dl ] = treeDiameter( t->left() );
 const auto [ hr , dr ] = treeDiameter( t->right() );
 return( std::pair( std::max( hl , hr ) + 1 ,
                    std::max( { dl , dr , hl + hr + 2 } ) ) );
 }

template< Graph G >
std::vector< int > treeVertexCover( const G & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< int > p( g.bound() , NIL ) , Q;
 std::vector< bool > seen( g.bound() , false ) , in( g.bound() , false );
 for( int r : g.V() ) {
  if( seen[ r ] )
   continue;
  const std::size_t first = Q.size();
  Q.push_back( r );
  seen[ r ] = true;
  for( std::size_t h = first ; h < Q.size() ; ++h ) {
   const int u = Q[ h ];
   bool parent = p[ u ] == NIL;         // the edge to the parent, once
   g.forEachAdj( u , [ & ]( int v ) {
    if( ! parent && ( v == p[ u ] ) ) {
     parent = true;
     return;
     }
    ADS_PRE( ! seen[ v ] );                     // else a cycle
    seen[ v ] = true;
    p[ v ] = u;
    Q.push_back( v );
    } );
   }
  }
 for( std::size_t h = Q.size() ; h-- > 0 ; ) {   // the leaves first
  const int u = Q[ h ];
  if( ( p[ u ] != NIL ) && ! in[ u ] && ! in[ p[ u ] ] )
   in[ p[ u ] ] = true;
  }
 std::vector< int > C;
 for( int u : g.V() )
  if( in[ u ] )
   C.push_back( u );
 ADS_AUDIT( [ & ]( void ) {
  bool ok = true;
  for( int u : g.V() )
   g.forEachAdj( u , [ & ]( int v ) { ok = ok && ( in[ u ] || in[ v ] ); } );
  return( ok );
  }() );
 return( C );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- STRUCTURE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
std::int64_t countTriangles( const G & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< int > deg( g.bound() , 0 );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int ) { ++deg[ u ]; } );
 auto before = [ & ]( int a , int b ) {        // the order of the nodes
  return( ( deg[ a ] < deg[ b ] ) || ( ( deg[ a ] == deg[ b ] ) &&
                                       ( a < b ) ) );
  };
 std::vector< std::vector< int > > out( g.bound() );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   if( before( u , v ) )
    out[ u ].push_back( v );
   } );
 std::vector< bool > mark( g.bound() , false );
 std::int64_t t = 0;
 for( int u : g.V() ) {
  for( int v : out[ u ] )
   mark[ v ] = true;
  for( int v : out[ u ] )
   for( int w : out[ v ] )
    t += mark[ w ] ? 1 : 0;
  for( int v : out[ u ] )
   mark[ v ] = false;
  }
 return( t );
 }

template< Graph G >
std::optional< std::vector< int > > twoColoring( const G & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< int > color( g.bound() , -1 ) , Q;
 for( int r : g.V() ) {
  if( color[ r ] >= 0 )
   continue;
  color[ r ] = 0;
  Q.assign( 1 , r );
  for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
   const int u = Q[ h ];
   bool odd = false;
   g.forEachAdj( u , [ & ]( int v ) {
    if( color[ v ] < 0 ) {
     color[ v ] = 1 - color[ u ];
     Q.push_back( v );
     }
    else if( color[ v ] == color[ u ] )
     odd = true;
    } );
   if( odd )
    return( std::nullopt );
   }
  }
 ADS_AUDIT( [ & ]( void ) {
  bool ok = true;
  for( int u : g.V() )
   g.forEachAdj( u , [ & ]( int v ) {
    ok = ok && ( color[ u ] != color[ v ] );
    } );
  return( ok );
  }() );
 return( color );
 }

template< class T >
int universalSink( const std::vector< std::vector< T > > & A )
{
 const std::size_t n = A.size();
 ADS_PRE( std::all_of( A.begin() , A.end() , [ n ]( const auto & row ) {
  return( row.size() == n ); } ) );
 if( n == 0 )
  return( NIL );
 std::size_t c = 0;
 for( std::size_t j = 1 ; j < n ; ++j )
  if( bool( A[ c ][ j ] ) )           // c has an edge out
   c = j;                             // else j misses the edge from c
 for( std::size_t j = 0 ; j < n ; ++j )
  if( ( j != c ) && ( bool( A[ c ][ j ] ) || ! bool( A[ j ][ c ] ) ) )
   return( NIL );
 return( int( c ) );
 }

template< Graph G >
std::optional< std::vector< int > > uniqueTopologicalOrder( const G & g )
{
 ADS_PRE( g.directed() );
 std::vector< int > in( g.bound() , 0 ) , S , order;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ++in[ v ]; } );
 for( int u : g.V() )
  if( in[ u ] == 0 )
   S.push_back( u );
 while( S.size() == 1 ) {
  const int u = S.back();
  S.pop_back();
  order.push_back( u );
  g.forEachAdj( u , [ & ]( int v ) {
   if( --in[ v ] == 0 )
    S.push_back( v );
   } );
  }
 if( int( order.size() ) != g.n() )     // two sources, or a cycle
  return( std::nullopt );
 ADS_AUDIT( isTopologicalOrder( g , order ) );
 return( order );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Properties.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
