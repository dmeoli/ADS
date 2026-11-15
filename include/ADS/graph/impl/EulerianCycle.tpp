/*--------------------------------------------------------------------------*/
/*------------------------- File EulerianCycle.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Eulerian cycles and trails of EulerianCycle.h [see EulerianCycle.h],
 * with their contracts.
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

namespace ADS
{

namespace detail
{

/// the edges of a graph for the walks of Hierholzer: the incidences of each
/// node u in inc[ off[ u ] .. off[ u + 1 ] ), each an edge and its other
/// end, an arc of an oriented graph from its tail only and an unoriented
/// edge from both ends, the two arcs of the adjacency paired in order; the
/// excess of each node, the edges leaving minus those entering (the degree,
/// if unoriented), and the number m of the edges
struct EulerEdges
{
 std::vector< std::size_t > off;
 std::vector< std::pair< int , int > > inc;  ///< ( edge , other end )
 std::vector< int > excess;
 std::size_t m = 0;
 int last = -1;  ///< the last node with an edge

 template< Graph G >
 explicit EulerEdges( const G & g ) {
  const int n = g.bound();
  const Rows< G > A( g );
  off.assign( std::size_t( n ) + 1 , 0 );
  excess.assign( n , 0 );
  if( g.directed() ) {
   inc.reserve( A.first( n ) );
   for( int u = 0 ; u < n ; ++u ) {
    off[ u ] = inc.size();
    for( std::size_t i = A.first( u ) ; i < A.last( u ) ; ++i ) {
     const int v = A.head( i );
     inc.emplace_back( int( inc.size() ) , v );
     ++excess[ u ];
     --excess[ v ];
     last = u;
     }
    }
   off[ n ] = inc.size();
   m = inc.size();
   return;
   }
  // an unoriented edge is the arc from its smaller end; the others are
  // its twins, met in the same order from the larger end
  std::vector< std::pair< int , int > > E;
  for( int u = 0 ; u < n ; ++u )
   for( std::size_t i = A.first( u ) ; i < A.last( u ) ; ++i ) {
    const int v = A.head( i );
    ADS_PRE( u != v );
    ++excess[ u ];
    if( u < v )
     E.emplace_back( u , v );
    last = std::max( last , std::max( u , v ) );
    }
  m = E.size();
  for( int u = 0 ; u < n ; ++u )
   off[ u + 1 ] = off[ u ] + std::size_t( excess[ u ] );
  inc.resize( off[ n ] );
  std::vector< std::size_t > at( off.begin() , off.end() - 1 );
  for( std::size_t e = 0 ; e < m ; ++e ) {
   inc[ at[ E[ e ].first ]++ ] = { int( e ) , E[ e ].second };
   inc[ at[ E[ e ].second ]++ ] = { int( e ) , E[ e ].first };
   }
  ADS_PRE( inc.size() == 2 * m );  // each arc has its twin
  }

 /// the walk of Hierholzer from s: the trail, backwards
 std::vector< int > walk( int s ) const {
  std::vector< std::size_t > next( off.begin() , off.end() - 1 );
  std::vector< bool > used( m , false );
  std::vector< int > S = { s } , C;
  C.reserve( m + 1 );
  while( ! S.empty() ) {
   const int u = S.back();
   std::size_t & i = next[ u ];
   while( ( i < off[ u + 1 ] ) && used[ inc[ i ].first ] )
    ++i;  // the twin of a used edge
   if( i < off[ u + 1 ] ) {
    used[ inc[ i ].first ] = true;
    S.push_back( inc[ i++ ].second );
    }
   else {
    C.push_back( u );
    S.pop_back();
    }
   }
  std::reverse( C.begin() , C.end() );
  return( C );
  }

 /// the trail from s, or nothing if it misses an edge
 std::optional< std::vector< int > > trail( int s ) const {
  auto C = walk( s );
  if( C.size() != m + 1 )  // the edges are not connected
   return( std::nullopt );
  return( C );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
std::optional< std::vector< int > > eulerianCycle( const G & g )
{
 const detail::EulerEdges E( g );
 if( E.m == 0 )
  return( std::vector< int >() );
 for( int x : E.excess )
  if( g.directed() ? x != 0 : x % 2 != 0 )
   return( std::nullopt );
 auto C = E.trail( E.last );
 ADS_AUDIT( ! C || isEulerianCycle( g , *C ) );
 return( C );
 }

template< Graph G >
std::optional< std::vector< int > > eulerianTrail( const G & g )
{
 const detail::EulerEdges E( g );
 if( E.m == 0 )
  return( std::vector< int >() );
 int s = -1 , odd = 0;
 for( int u = 0 ; u < g.bound() ; ++u ) {
  const int x = E.excess[ u ];
  if( g.directed() ) {
   if( ( x < -1 ) || ( x > 1 ) )
    return( std::nullopt );
   if( x == 1 ) {
    if( s != -1 )
     return( std::nullopt );
    s = u;
    }
   }
  else if( x % 2 != 0 ) {
   if( ++odd > 2 )
    return( std::nullopt );
   if( s == -1 )
    s = u;
   }
  }
 // with the excesses summing to 0, one node of excess 1 leaves one of -1
 // and the others balanced; the odd degrees are even in number
 auto C = E.trail( s == -1 ? E.last : s );
 ADS_AUDIT( ! C || isEulerianTrail( g , *C ) );
 return( C );
 }

template< Graph G >
std::optional< std::vector< int > > eulerianTrail( const G & g , int s )
{
 ADS_PRE( g.hasNode( s ) );
 const detail::EulerEdges E( g );
 if( E.m == 0 )
  return( std::vector< int >() );
 if( E.off[ s ] == E.off[ s + 1 ] )  // s has no edge to start with
  return( std::nullopt );
 int unbalanced = 0;
 for( int u = 0 ; u < g.bound() ; ++u ) {
  const int x = E.excess[ u ];
  if( g.directed() ) {
   if( ( x < -1 ) || ( x > 1 ) || ( ( x == 1 ) && ( u != s ) ) )
    return( std::nullopt );
   unbalanced += x != 0;
   }
  else if( x % 2 != 0 ) {
   if( ++unbalanced > 2 )
    return( std::nullopt );
   }
  }
 if( ( unbalanced > 0 ) && ( E.excess[ s ] % 2 == 0 ) )
  return( std::nullopt );  // s is not an end of the trail
 auto C = E.trail( s );
 ADS_AUDIT( ! C || ( isEulerianTrail( g , *C ) && ( C->front() == s ) ) );
 return( C );
 }

template< Graph G >
bool isEulerianCycle( const G & g , const std::vector< int > & c )
{
 if( ! c.empty() && ( c.front() != c.back() ) )
  return( false );
 return( isEulerianTrail( g , c ) );
 }

template< Graph G >
bool isEulerianTrail( const G & g , const std::vector< int > & c )
{
 const bool d = g.directed();
 auto arc = [ d ]( int u , int v ) {
  return( d ? std::pair( u , v )
	     : std::pair( std::min( u , v ) , std::max( u , v ) ) );
  };
 std::vector< std::pair< int , int > > E , C;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   if( d || ( u < v ) )
    E.push_back( arc( u , v ) );
   } );
 if( E.empty() )
  return( c.empty() );
 if( c.size() != E.size() + 1 )
  return( false );
 for( std::size_t i = 0 ; i + 1 < c.size() ; ++i )
  C.push_back( arc( c[ i ] , c[ i + 1 ] ) );
 std::sort( E.begin() , E.end() );
 std::sort( C.begin() , C.end() );
 return( E == C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File EulerianCycle.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
