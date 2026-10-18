/*--------------------------------------------------------------------------*/
/*----------------------------- File Visit.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The visits of Visit.h [see Visit.h], with their contracts.
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

namespace detail {

/// a frame of the recursion of a visit in depth, kept on an explicit stack
/// so that a path of 10^5 nodes does not overflow the stack of the calls:
/// the node, its adjacent nodes and the place of the next one
struct DfsFrame {
 int u;
 std::vector< int > A;
 std::size_t i = 0;
 };

/// d and p are the distances and the parents of a breadth first visit from
/// r: d[ r ] = 0, d[ v ] = d[ p[ v ] ] + 1 on the reached nodes but r, and
/// no edge from a reached node goes further than one level
template< Graph G >
bool bfsTree( const G & g , int r , const std::vector< int > & d ,
              const std::vector< int > & p )
{
 if( ( d[ r ] != 0 ) || ( p[ r ] != NIL ) )
  return( false );
 for( int v : g.V() ) {
  if( ( v == r ) || ( d[ v ] == INF ) ) {
   if( ( v != r ) && ( p[ v ] != NIL ) )
    return( false );
   continue;
   }
  if( ! g.hasNode( p[ v ] ) || ( d[ p[ v ] ] == INF ) ||
      ( d[ v ] != d[ p[ v ] ] + 1 ) )
   return( false );
  }
 bool ok = true;
 for( int u : g.V() )
  if( d[ u ] != INF )
   g.forEachAdj( u , [ & ]( int v ) {
    if( ( d[ v ] == INF ) || ( d[ v ] > d[ u ] + 1 ) )
     ok = false;
    } );
 return( ok );
 }

/// the items of S are nodes of g
template< Graph G >
bool allNodes( const G & g , const std::vector< int > & S )
{
 return( std::all_of( S.begin() , S.end() ,
                      [ & ]( int u ) { return( g.hasNode( u ) ); } ) );
 }

/// each edge ( u , v ) of g is ( v , u ) in t
template< Graph G >
bool reversed( const G & g , const ListGraph<> & t )
{
 bool ok = true;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ok = ok && t.hasEdge( v , u ); } );
 return( ok );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------------- BFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G , class FN , class FE >
void bfs( const G & g , int r , FN && node , FE && edge )
{
 ADS_PRE( g.hasNode( r ) );
 std::vector< bool > visited( g.bound() , false );
 ArrayQueue< int > S( std::max( g.n() , 1 ) );
 S.enqueue( r );
 visited[ r ] = true;
 while( ! S.isEmpty() ) {
  const int u = S.dequeue();
  node( u );
  g.forEachAdj( u , [ & ]( int v ) {
   edge( u , v );
   if( ! visited[ v ] ) {
    visited[ v ] = true;
    S.enqueue( v );
    }
   } );
  }
 }

template< Graph G >
std::pair< std::vector< int > , std::vector< int > > distance( const G & g ,
                                                                int r )
{
 ADS_PRE( g.hasNode( r ) );
 std::vector< int > d( g.bound() , INF ) , p( g.bound() , NIL );
 ArrayQueue< int > S( std::max( g.n() , 1 ) );
 S.enqueue( r );
 d[ r ] = 0;
 while( ! S.isEmpty() ) {
  const int u = S.dequeue();
  g.forEachAdj( u , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ u ] + 1;
    p[ v ] = u;
    S.enqueue( v );
    }
   } );
  }
 ADS_AUDIT( detail::bfsTree( g , r , d , p ) );
 return( std::pair( d , p ) );
 }

inline std::vector< int > path( int r , int s , const std::vector< int > & p )
{
 ADS_PRE( ( r >= 0 ) && ( r < int( p.size() ) ) && ( s >= 0 ) &&
          ( s < int( p.size() ) ) );
 std::vector< int > out = { s };                    // from s up to r
 for( int x = s ; x != r ; ) {
  x = p[ x ];
  if( ( x == NIL ) || ( out.size() >= p.size() ) )   // none, or a cycle
   return( std::vector< int >() );
  ADS_PRE( ( x >= 0 ) && ( x < int( p.size() ) ) );
  out.push_back( x );
  }
 std::reverse( out.begin() , out.end() );
 ADS_POST( out.empty() || ( ( out.front() == r ) && ( out.back() == s ) ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- DFS ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G , class Pre , class Edge , class Post >
void dfs( const G & g , int u , std::vector< bool > & visited , Pre && pre ,
          Edge && edge , Post && post )
{
 ADS_PRE( g.hasNode( u ) && ( int( visited.size() ) >= g.bound() ) &&
          ! visited[ u ] );
 std::vector< detail::DfsFrame > S;                  // the recursion
 auto discover = [ & ]( int v ) {
  visited[ v ] = true;
  pre( v );                                           // (1)
  S.push_back( detail::DfsFrame{ v , g.adj( v ) } );
  };
 discover( u );
 while( ! S.empty() ) {
  auto & f = S.back();
  if( f.i == f.A.size() ) {                           // the return
   const int x = f.u;
   S.pop_back();
   post( x );                                         // (2)
   continue;
   }
  const int x = f.u , v = f.A[ f.i++ ];
  edge( x , v );
  if( ! visited[ v ] )                                // the call
   discover( v );
  }
 }

template< Graph G >
std::vector< int > cc( const G & g , std::vector< int > S )
{
 ADS_AUDIT( detail::allNodes( g , S ) );
 std::vector< int > id( g.bound() , 0 );
 int count = 0;
 std::vector< int > T;                    // the nodes of the visit to do
 while( ! S.empty() ) {
  const int u = S.back();
  S.pop_back();
  if( id[ u ] != 0 )
   continue;
  id[ u ] = ++count;
  T.push_back( u );
  while( ! T.empty() ) {
   const int x = T.back();
   T.pop_back();
   g.forEachAdj( x , [ & ]( int v ) {
    if( id[ v ] == 0 ) {
     id[ v ] = count;
     T.push_back( v );
     }
    } );
   }
  }
 return( id );
 }

template< Graph G >
std::vector< int > cc( const G & g )
{
 auto S = g.V();
 std::reverse( S.begin() , S.end() );
 auto id = cc( g , S );
 ADS_AUDIT( std::all_of( S.begin() , S.end() ,
                         [ & ]( int u ) { return( id[ u ] > 0 ); } ) );
 return( id );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DfsSchema ------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
DfsSchema< G >::DfsSchema( const G & g ) : f_g( g ) , f_dt( g.bound() , 0 ) ,
                                           f_ft( g.bound() , 0 ) {}

template< Graph G >
template< class Edge >
void DfsSchema< G >::visit( int u , Edge && edge )
{
 ADS_PRE( f_g.hasNode( u ) && ( f_dt[ u ] == 0 ) );
 std::vector< detail::DfsFrame > S;                  // the recursion
 auto discover = [ & ]( int v ) {
  f_dt[ v ] = ++f_time;
  S.push_back( detail::DfsFrame{ v , f_g.adj( v ) } );
  };
 discover( u );
 while( ! S.empty() ) {
  auto & f = S.back();
  if( f.i == f.A.size() ) {                           // the return
   f_ft[ f.u ] = ++f_time;
   S.pop_back();
   continue;
   }
  const int x = f.u , v = f.A[ f.i++ ];
  if( f_dt[ v ] == 0 ) {
   edge( x , v , EdgeKind::TREE );
   discover( v );                                     // the call
   }
  else if( ( f_dt[ x ] > f_dt[ v ] ) && ( f_ft[ v ] == 0 ) )
   edge( x , v , EdgeKind::BACK );
  else if( ( f_dt[ x ] < f_dt[ v ] ) && ( f_ft[ v ] != 0 ) )
   edge( x , v , EdgeKind::FORWARD );
  else
   edge( x , v , EdgeKind::CROSS );
  }
 ADS_POST( f_dt[ u ] < f_ft[ u ] );
 }

template< Graph G >
template< class Edge >
void DfsSchema< G >::visitAll( Edge && edge )
{
 for( int u : f_g.V() )
  if( f_dt[ u ] == 0 )
   visit( u , edge );
 }

template< Graph G >
int DfsSchema< G >::dt( int u ) const
{
 ADS_PRE( f_g.hasNode( u ) );
 return( f_dt[ u ] );
 }

template< Graph G >
int DfsSchema< G >::ft( int u ) const
{
 ADS_PRE( f_g.hasNode( u ) );
 return( f_ft[ u ] );
 }

/*--------------------------------------------------------------------------*/
/*--------------------- CYCLES, ORDERS AND COMPONENTS ----------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
bool hasCycle( const G & g )
{
 std::vector< int > dt( g.bound() , 0 ) , ft( g.bound() , 0 );
 int time = 0;
 std::vector< detail::DfsFrame > S;                  // the recursion
 auto discover = [ & ]( int v ) {
  dt[ v ] = ++time;
  S.push_back( detail::DfsFrame{ v , g.adj( v ) } );
  };
 for( int r : g.V() ) {
  if( dt[ r ] != 0 )
   continue;
  discover( r );
  while( ! S.empty() ) {
   auto & f = S.back();
   if( f.i == f.A.size() ) {                          // the return
    ft[ f.u ] = ++time;
    S.pop_back();
    continue;
    }
   const int u = f.u , v = f.A[ f.i++ ];
   if( dt[ v ] == 0 )
    discover( v );                                    // the call
   else if( ( dt[ u ] > dt[ v ] ) && ( ft[ v ] == 0 ) )
    return( true );                                   // a back edge
   }
  }
 return( false );
 }

template< Graph G >
bool isTopologicalOrder( const G & g , const std::vector< int > & S )
{
 if( int( S.size() ) != g.n() )
  return( false );
 std::vector< int > at( g.bound() , -1 );
 for( int i = 0 ; i < int( S.size() ) ; ++i ) {
  if( ! g.hasNode( S[ i ] ) || ( at[ S[ i ] ] >= 0 ) )
   return( false );
  at[ S[ i ] ] = i;
  }
 bool ok = true;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ok = ok && ( at[ u ] < at[ v ] ); } );
 return( ok );
 }

template< Graph G >
std::vector< int > topsort( const G & g )
{
 std::vector< bool > visited( g.bound() , false );
 std::vector< int > S;
 auto none = []( auto... ) {};
 for( int u : g.V() )
  if( ! visited[ u ] )
   dfs( g , u , visited , none , none ,
        [ & ]( int v ) { S.push_back( v ); } );
 std::reverse( S.begin() , S.end() );
 ADS_AUDIT( ( int( S.size() ) == g.n() ) &&
            ( hasCycle( g ) || isTopologicalOrder( g , S ) ) );
 return( S );
 }

template< Graph G >
std::vector< int > topsortKahn( const G & g )
{
 std::vector< int > in( g.bound() , 0 ) , out;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ++in[ v ]; } );
 for( int u : g.V() )
  if( in[ u ] == 0 )
   out.push_back( u );
 for( std::size_t i = 0 ; i < out.size() ; ++i )
  g.forEachAdj( out[ i ] , [ & ]( int v ) {
   if( --in[ v ] == 0 )
    out.push_back( v );
   } );
 ADS_AUDIT( ( int( out.size() ) < g.n() ) || isTopologicalOrder( g , out ) );
 return( out );
 }

template< Graph G >
ListGraph<> transpose( const G & g )
{
 ListGraph<> t( g.bound() , g.directed() );
 for( int u = 0 ; u < g.bound() ; ++u )
  if( ! g.hasNode( u ) )
   t.deleteNode( u );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { t.insertEdge( v , u ); } );
 ADS_AUDIT( detail::reversed( g , t ) );
 return( t );
 }

template< Graph G >
std::vector< int > scc( const G & g )
{
 auto S = topsort( g );                           // first visit
 std::reverse( S.begin() , S.end() );             // its top at the back
 auto id = cc( transpose( g ) , S );              // second visit
 ADS_POST( std::all_of( S.begin() , S.end() ,
                        [ & ]( int u ) { return( id[ u ] > 0 ); } ) );
 return( id );
 }

template< Graph G >
std::pair< std::vector< int > , std::vector< std::pair< int , int > > >
genericVisit( const G & g , int r , Choice c )
{
 ADS_PRE( g.hasNode( r ) );
 std::vector< int > R = { r };
 std::vector< std::pair< int , int > > B;
 std::vector< bool > in( g.bound() , false );
 std::vector< std::vector< int > > A( g.bound() );   // the adjacencies
 std::vector< std::size_t > at( g.bound() , 0 );     // their cursors
 std::deque< int > Q = { r };
 in[ r ] = true;
 A[ r ] = g.adj( r );
 while( ! Q.empty() ) {
  const int j = c == Choice::NEWEST ? Q.back() : Q.front();
  while( ( at[ j ] < A[ j ].size() ) && in[ A[ j ][ at[ j ] ] ] )
   ++at[ j ];
  if( at[ j ] < A[ j ].size() ) {          // k adjacent to j, not in R
   const int k = A[ j ][ at[ j ]++ ];
   in[ k ] = true;
   A[ k ] = g.adj( k );
   R.push_back( k );
   Q.push_back( k );
   B.emplace_back( j , k );
   }
  else if( c == Choice::NEWEST )           // j leaves Q
   Q.pop_back();
  else
   Q.pop_front();
  }
 ADS_AUDIT( [ & ]( void ) {
  if( B.size() + 1 != R.size() )
   return( false );
  std::vector< bool > seen( g.bound() , false );
  seen[ r ] = true;
  for( auto [ j , k ] : B ) {
   if( ! seen[ j ] || seen[ k ] )
    return( false );
   seen[ k ] = true;
   }
  return( true );
  }() );
 return( std::pair( R , B ) );
 }

template< Graph G >
bool unilaterallyConnected( const G & g )
{
 if( g.n() <= 1 )
  return( true );
 const auto C = scc( g );
 int k = 0;
 for( int u : g.V() )
  k = std::max( k , C[ u ] );
 std::vector< std::set< int > > E( k + 1 );    // the graph of components
 std::vector< int > in( k + 1 , 0 );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   if( ( C[ u ] != C[ v ] ) && E[ C[ u ] ].insert( C[ v ] ).second )
    ++in[ C[ v ] ];
   } );
 std::vector< int > S;                         // the sources
 for( int c = 1 ; c <= k ; ++c )
  if( in[ c ] == 0 ) S.push_back( c );
 while( ! S.empty() ) {
  if( S.size() > 1 )                           // two components with no
   return( false );                            // path between them
  const int c = S.back();
  S.pop_back();
  for( int d : E[ c ] )
   if( --in[ d ] == 0 ) S.push_back( d );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Visit.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
