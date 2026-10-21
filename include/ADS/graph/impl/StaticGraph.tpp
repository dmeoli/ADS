/*--------------------------------------------------------------------------*/
/*-------------------------- File StaticGraph.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StaticGraph [see StaticGraph.h], with their contracts.
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

#include <climits>

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CONSTRUCTION ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
template< class Edges , class Get >
void StaticGraph< W >::build( int n , const Edges & E , Get && get )
{
 ADS_PRE( n >= 0 );
 const std::size_t na = ( f_directed ? 1 : 2 ) * E.size();
 ADS_PRE( na < std::size_t( INT_MAX ) );
 // the arcs by head, then stably by tail: the rows come out sorted
 std::vector< int > cnt( std::size_t( n ) + 1 , 0 );
 for( const auto & e : E ) {
  const auto [ u , v , w ] = get( e );
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) &&
           ( u != v ) );
  ++cnt[ v + 1 ];
  if( ! f_directed )
   ++cnt[ u + 1 ];
  }
 for( int v = 0 ; v < n ; ++v )
  cnt[ v + 1 ] += cnt[ v ];
 std::vector< std::pair< int , Arc > > byHead( na );
 auto arc = []( int v , const auto & w ) {
  if constexpr( weighted ) return( Arc{ v , W( w ) } );
  else { ( void ) w; return( Arc( v ) ); }
  };
 for( const auto & e : E ) {
  const auto [ u , v , w ] = get( e );
  byHead[ cnt[ v ]++ ] = { u , arc( v , w ) };
  if( ! f_directed )
   byHead[ cnt[ u ]++ ] = { v , arc( u , w ) };
  }
 f_off.assign( std::size_t( n ) + 1 , 0 );
 for( const auto & [ u , a ] : byHead )
  ++f_off[ u + 1 ];
 for( int u = 0 ; u < n ; ++u )
  f_off[ u + 1 ] += f_off[ u ];
 f_arc.resize( na );
 std::vector< int > next( f_off.begin() , f_off.end() - 1 );
 for( const auto & [ u , a ] : byHead )
  f_arc[ next[ u ]++ ] = a;
 f_m = int( E.size() );
 ADS_INVARIANT( consistent() );
 }

template< class W >
StaticGraph< W >::StaticGraph( int n ,
                  const std::vector< std::tuple< int , int , W > > & E ,
                  bool directed ) : f_directed( directed )
{
 build( n , E , []( const auto & e ) { return( e ); } );
 }

template< class W >
StaticGraph< W >::StaticGraph( int n ,
                       const std::vector< std::pair< int , int > > & E ,
                       bool directed ) requires( ! weighted )
 : f_directed( directed )
{
 build( n , E , []( const auto & e ) {
  return( std::tuple< int , int , W >( e.first , e.second , W() ) ); } );
 }

template< class W >
template< Graph G >
StaticGraph< W >::StaticGraph( const G & g ) : f_directed( g.directed() )
{
 std::vector< std::tuple< int , int , W > > E;
 for( int u : g.V() ) {
  if constexpr( weighted )
   g.forEachEdge( u , [ & ]( int v , const auto & w ) {
    if( g.directed() || ( u < v ) )
     E.emplace_back( u , v , W( w ) );
    } );
  else
   g.forEachAdj( u , [ & ]( int v ) {
    if( g.directed() || ( u < v ) )
     E.emplace_back( u , v , W() );
    } );
  }
 build( g.bound() , E , []( const auto & e ) { return( e ); } );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE OPERATIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
int StaticGraph< W >::degree( int u ) const
{
 ADS_PRE( hasNode( u ) );
 return( f_off[ u + 1 ] - f_off[ u ] );
 }

template< class W >
std::vector< int > StaticGraph< W >::V( void ) const
{
 std::vector< int > out( n() );
 for( int u = 0 ; u < n() ; ++u )
  out[ u ] = u;
 return( out );
 }

template< class W >
std::vector< int > StaticGraph< W >::adj( int u ) const
{
 ADS_PRE( hasNode( u ) );
 std::vector< int > out;
 out.reserve( degree( u ) );
 for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
  out.push_back( head( f_arc[ k ] ) );
 return( out );
 }

template< class W >
template< class F >
void StaticGraph< W >::forEachAdj( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
  f( head( f_arc[ k ] ) );
 }

template< class W >
template< class F >
void StaticGraph< W >::forEachEdge( int u , F && f ) const
 requires( weighted )
{
 ADS_PRE( hasNode( u ) );
 for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
  f( f_arc[ k ].to , f_arc[ k ].w );
 }

template< class W >
std::span< const typename StaticGraph< W >::Arc > StaticGraph< W >::out(
                                                             int u ) const
{
 ADS_PRE( hasNode( u ) );
 return( std::span< const Arc >( f_arc.data() + f_off[ u ] ,
                          std::size_t( f_off[ u + 1 ] - f_off[ u ] ) ) );
 }

template< class W >
StaticGraph< W > StaticGraph< W >::transpose( void ) const
{
 if( ! f_directed )
  return( *this );
 std::vector< std::tuple< int , int , W > > E;
 E.reserve( f_arc.size() );
 for( int u = 0 ; u < n() ; ++u )
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k ) {
   if constexpr( weighted )
    E.emplace_back( f_arc[ k ].to , u , f_arc[ k ].w );
   else
    E.emplace_back( f_arc[ k ] , u , W() );
   }
 return( StaticGraph( n() , E , true ) );
 }

template< class W >
StaticGraph< W > StaticGraph< W >::relabel( const std::vector< int > & p )
                                                                      const
{
 ADS_PRE( int( p.size() ) == n() );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< bool > seen( n() , false );
  for( int x : p )
   if( ( x < 0 ) || ( x >= n() ) || seen[ x ] )
    return( false );
   else
    seen[ x ] = true;
  return( true );
  }() );
 std::vector< std::tuple< int , int , W > > E;
 E.reserve( std::size_t( f_m ) );
 for( int u = 0 ; u < n() ; ++u )
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k ) {
   const int v = head( f_arc[ k ] );
   if( ! f_directed && ( v < u ) )
    continue;                        // each unoriented edge once
   if constexpr( weighted )
    E.emplace_back( p[ u ] , p[ v ] , f_arc[ k ].w );
   else
    E.emplace_back( p[ u ] , p[ v ] , W() );
   }
 return( StaticGraph( n() , E , f_directed ) );
 }

template< class W >
bool StaticGraph< W >::consistent( void ) const
{
 if( f_off.empty() || ( f_off[ 0 ] != 0 ) ||
     ( std::size_t( f_off.back() ) != f_arc.size() ) ||
     ( f_arc.size() != ( f_directed ? 1u : 2u ) * std::size_t( f_m ) ) )
  return( false );
 for( int u = 0 ; u < n() ; ++u ) {
  if( f_off[ u ] > f_off[ u + 1 ] )
   return( false );
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k ) {
   const int v = head( f_arc[ k ] );
   if( ! hasNode( v ) ||
       ( ( k > f_off[ u ] ) && ( head( f_arc[ k - 1 ] ) > v ) ) )
    return( false );
   }
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
std::vector< int > bfsOrder( const StaticGraph< W > & g , int r )
{
 const int n = g.n();
 ADS_PRE( ( n == 0 ) || g.hasNode( r ) );
 std::vector< int > p( n , -1 ) , Q( n );
 int head = 0 , tail = 0 , next = 0;
 const int * off = g.offsets();
 const auto * arc = g.arcData();
 for( int k = -1 ; k < n ; ++k ) {
  const int x = k < 0 ? r : k;          // r, then the least not reached
  if( ( n == 0 ) || ( p[ x ] >= 0 ) )
   continue;
  p[ x ] = next++;
  Q[ tail++ ] = x;
  while( head < tail ) {
   const int u = Q[ head++ ];
   for( int a = off[ u ] ; a < off[ u + 1 ] ; ++a ) {
    const int v = StaticGraph< W >::head( arc[ a ] );
    if( p[ v ] < 0 ) {
     p[ v ] = next++;
     Q[ tail++ ] = v;
     }
    }
   }
  }
 return( p );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File StaticGraph.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
