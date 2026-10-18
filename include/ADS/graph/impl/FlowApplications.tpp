/*--------------------------------------------------------------------------*/
/*----------------------- File FlowApplications.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The uses of the maximum flow of FlowApplications.h [see
 * FlowApplications.h], with their contracts.
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

/// the multiplicity of each edge of g, ( u , v ) with u < v if unoriented
template< Graph G >
std::map< std::pair< int , int > , int > edgeCounts( const G & g )
{
 std::map< std::pair< int , int > , int > M;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   if( g.directed() )
    ++M[ std::pair( u , v ) ];
   else if( u < v )
    ++M[ std::pair( u , v ) ];
   } );
 return( M );
 }

/// P are simple paths from s to t along the edges of g, at most k, none
/// using an edge more times than the graph has it
template< Graph G >
bool disjointPaths( const G & g , int s , int t , long k ,
                    const std::vector< std::vector< int > > & P )
{
 if( long( P.size() ) > k )
  return( false );
 auto M = edgeCounts( g );
 for( const auto & p : P ) {
  if( ( p.size() < 2 ) || ( p.front() != s ) || ( p.back() != t ) )
   return( false );
  std::vector< int > q( p );
  std::sort( q.begin() , q.end() );
  if( std::adjacent_find( q.begin() , q.end() ) != q.end() )
   return( false );
  for( std::size_t i = 1 ; i < p.size() ; ++i ) {
   auto e = std::pair( p[ i - 1 ] , p[ i ] );
   if( ! g.directed() && ( e.first > e.second ) )
    std::swap( e.first , e.second );
   if( --M[ e ] < 0 )
    return( false );
   }
  }
 return( true );
 }

/// f is a flow from s to t within c, skew symmetric, with no augmenting
/// path left
template< class T >
bool maximumFlow( const std::vector< std::vector< T > > & c ,
                  const std::vector< std::vector< T > > & f , int s , int t )
{
 const int n = int( c.size() );
 for( int x = 0 ; x < n ; ++x ) {
  T net = T( 0 );
  for( int y = 0 ; y < n ; ++y ) {
   if( ( f[ x ][ y ] != - f[ y ][ x ] ) || ( f[ x ][ y ] > c[ x ][ y ] ) )
    return( false );
   net += f[ x ][ y ];
   }
  if( ( x != s ) && ( x != t ) && ( net != T( 0 ) ) )
   return( false );
  }
 std::vector< bool > seen( n , false );       // the residual network from s
 std::vector< int > Q = { s };
 seen[ s ] = true;
 for( std::size_t h = 0 ; h < Q.size() ; ++h )
  for( int y = 0 ; y < n ; ++y )
   if( ! seen[ y ] && ( c[ Q[ h ] ][ y ] - f[ Q[ h ] ][ y ] > T( 0 ) ) ) {
    seen[ y ] = true;
    Q.push_back( y );
    }
 return( ! seen[ t ] );
 }

/// one unit less of flow along a path from x to y of edges of positive
/// flow, found by a visit in depth; false if there is none
template< class T >
bool cancelFlowPath( std::vector< std::vector< T > > & f , int x , int y )
{
 const int n = int( f.size() );
 std::vector< int > p( n , -1 ) , S = { x };
 p[ x ] = x;
 while( ! S.empty() && ( p[ y ] < 0 ) ) {
  const int a = S.back();
  S.pop_back();
  for( int b = 0 ; b < n ; ++b )
   if( ( p[ b ] < 0 ) && ( f[ a ][ b ] > T( 0 ) ) ) {
    p[ b ] = a;
    S.push_back( b );
    }
  }
 if( p[ y ] < 0 )
  return( false );
 for( int b = y ; b != x ; b = p[ b ] ) {
  --f[ p[ b ] ][ b ];
  ++f[ b ][ p[ b ] ];
  }
 return( true );
 }

/// one augmenting path from s to t in the residual network c - f, by a
/// visit in breadth, and its least residual capacity added along it
template< class T >
void augmentOnce( const std::vector< std::vector< T > > & c ,
                  std::vector< std::vector< T > > & f , int s , int t )
{
 const int n = int( c.size() );
 std::vector< int > p( n , -1 ) , Q = { s };
 p[ s ] = s;
 for( std::size_t h = 0 ; ( h < Q.size() ) && ( p[ t ] < 0 ) ; ++h )
  for( int y = 0 ; y < n ; ++y )
   if( ( p[ y ] < 0 ) && ( c[ Q[ h ] ][ y ] - f[ Q[ h ] ][ y ] > T( 0 ) ) ) {
    p[ y ] = Q[ h ];
    Q.push_back( y );
    }
 if( p[ t ] < 0 )
  return;
 T d = c[ p[ t ] ][ t ] - f[ p[ t ] ][ t ];
 for( int y = t ; y != s ; y = p[ y ] )
  d = std::min( d , c[ p[ y ] ][ y ] - f[ p[ y ] ][ y ] );
 for( int y = t ; y != s ; y = p[ y ] ) {
  f[ p[ y ] ][ y ] += d;
  f[ y ][ p[ y ] ] -= d;
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
std::vector< std::vector< int > > edgeDisjointPaths( const G & g , int s ,
                                                     int t , long k )
{
 ADS_PRE( g.hasNode( s ) && g.hasNode( t ) && ( s != t ) && ( k >= 0 ) );
 const int n = g.bound();
 FlowNetwork N( n + 1 );                         // n, the new source
 struct Arc { int u , v , e; };
 std::vector< Arc > A;
 long out = 0;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   A.push_back( Arc{ u , v , N.addEdge( u , v , 1 ) } );
   out += u == s ? 1 : 0;
   } );
 N.addEdge( n , s , std::min( k , out ) );
 const long F = N.maxFlow( n , t );
 std::vector< std::vector< int > > next( n );   // the edges of flow 1
 if( g.directed() ) {
  for( const auto & a : A )
   if( N.flow( a.e ) == 1 )
    next[ a.u ].push_back( a.v );
  }
 else {                                          // opposite flows cancel
  std::map< std::pair< int , int > , long > net;
  for( const auto & a : A )
   if( N.flow( a.e ) == 1 ) {
    if( a.u < a.v )
     ++net[ std::pair( a.u , a.v ) ];
    else
     --net[ std::pair( a.v , a.u ) ];
    }
  for( const auto & [ e , x ] : net )
   for( long i = 0 ; i < std::max( x , -x ) ; ++i ) {
    if( x > 0 )
     next[ e.first ].push_back( e.second );
    else
     next[ e.second ].push_back( e.first );
    }
  }
 std::vector< std::vector< int > > P;
 std::vector< int > at( n , -1 );                // the place in the walk
 for( long i = 0 ; i < F ; ++i ) {
  std::vector< int > p = { s };
  at[ s ] = 0;
  while( p.back() != t ) {
   const int x = p.back() , y = next[ x ].back();
   next[ x ].pop_back();
   if( at[ y ] >= 0 ) {                          // a cycle, cut out
    while( p.back() != y ) {
     at[ p.back() ] = -1;
     p.pop_back();
     }
    }
   else {
    at[ y ] = int( p.size() );
    p.push_back( y );
    }
   }
  for( int x : p )
   at[ x ] = -1;
  P.push_back( p );
  }
 ADS_AUDIT( detail::disjointPaths( g , s , t , k , P ) );
 return( P );
 }

template< class T >
CapacityDecrease decreaseCapacity( std::vector< std::vector< T > > & c ,
                                   std::vector< std::vector< T > > & f ,
                                   int s , int t , int u , int v )
{
 const int n = int( c.size() );
 ADS_PRE( ( int( f.size() ) == n ) &&
          std::all_of( c.begin() , c.end() , [ n ]( const auto & r ) {
           return( int( r.size() ) == n ); } ) &&
          std::all_of( f.begin() , f.end() , [ n ]( const auto & r ) {
           return( int( r.size() ) == n ); } ) );
 ADS_PRE( ( s >= 0 ) && ( s < n ) && ( t >= 0 ) && ( t < n ) && ( s != t ) &&
          ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) && ( u != v ) &&
          ( c[ u ][ v ] > T( 0 ) ) );
 ADS_AUDIT( detail::maximumFlow( c , f , s , t ) );
 const CapacityDecrease result = [ & ]( void ) {
  --c[ u ][ v ];
  if( f[ u ][ v ] <= c[ u ][ v ] )
   return( CapacityDecrease::SLACK );
  --f[ u ][ v ];                 // an excess at u and a deficit at v
  ++f[ v ][ u ];
  if( detail::cancelFlowPath( f , v , u ) )
   return( CapacityDecrease::CYCLE );
  detail::cancelFlowPath( f , s , u );
  detail::cancelFlowPath( f , v , t );
  detail::augmentOnce( c , f , s , t );
  return( CapacityDecrease::PATH );
  }();
 ADS_AUDIT( detail::maximumFlow( c , f , s , t ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File FlowApplications.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
