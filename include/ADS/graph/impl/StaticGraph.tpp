/*--------------------------------------------------------------------------*/
/*-------------------------- File StaticGraph.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StaticGraph [see StaticGraph.h], with their contracts.
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

#include <climits>

namespace ADS
{

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
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) && ( u != v ) );
  ++cnt[ v + 1 ];
  if( ! f_directed )
   ++cnt[ u + 1 ];
  }
 for( int v = 0 ; v < n ; ++v )
  cnt[ v + 1 ] += cnt[ v ];
 // the arcs by head, each with its weight
 struct Item
 {
  int u , v;                   // the tail and the head
  ADS_NO_UNIQUE_ADDRESS W w;  // the weight
  };
 std::vector< Item > byHead( na );
 for( const auto & e : E ) {
  const auto [ u , v , w ] = get( e );
  byHead[ cnt[ v ]++ ] = Item{ u , v , W( w ) };
  if( ! f_directed )
   byHead[ cnt[ u ]++ ] = Item{ v , u , W( w ) };
  }
 f_off.assign( std::size_t( n ) + 1 , 0 );
 for( const Item & a : byHead )
  ++f_off[ a.u + 1 ];
 for( int u = 0 ; u < n ; ++u )
  f_off[ u + 1 ] += f_off[ u ];
 f_head.resize( na );
 if constexpr( weighted )
  f_w.resize( na );
 std::vector< int > next( f_off.begin() , f_off.end() - 1 );
 for( const Item & a : byHead ) {
  const int p = next[ a.u ]++;
  f_head[ p ] = a.v;
  if constexpr( weighted )
   f_w[ p ] = a.w;
  }
 f_m = int( E.size() );
 ADS_INVARIANT( consistent() );
 }

template< class W >
StaticGraph< W >::StaticGraph(
  int n , const std::vector< std::tuple< int , int , W > > & E ,
  bool directed )
    : f_directed( directed )
{
 build( n , E , []( const auto & e ) { return( e ); } );
 }

template< class W >
StaticGraph< W >::StaticGraph( int n ,
			       const std::vector< std::pair< int ,
							     int > > & E ,
			       bool directed )
 requires( ! weighted )
    : f_directed( directed )
{
 build( n , E , []( const auto & e ) {
  return( std::tuple< int , int , W >( e.first , e.second , W() ) );
  } );
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
  out.push_back( f_head[ k ] );
 return( out );
 }

template< class W >
template< class F >
void StaticGraph< W >::forEachAdj( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 // the row read once: f may write anywhere, and the compiler could not
 // keep the end and the arcs in registers across its calls
 const int * a = f_head.data() + f_off[ u ];
 const int * const end = f_head.data() + f_off[ u + 1 ];
 for( ; a != end ; ++a )
  f( *a );
 }

template< class W >
template< class F >
void StaticGraph< W >::forEachEdge( int u , F && f ) const
 requires( weighted )
{
 ADS_PRE( hasNode( u ) );
 const int * a = f_head.data() + f_off[ u ];
 const int * const end = f_head.data() + f_off[ u + 1 ];
 const W * w = f_w.data() + f_off[ u ];
 for( ; a != end ; ++a , ++w )
  f( *a , *w );
 }

template< class W >
int StaticGraph< W >::find( int u , int v ) const
{
 if( ! hasNode( u ) )
  return( -1 );
 const int * const a = f_head.data() + f_off[ u ];
 const int * const end = f_head.data() + f_off[ u + 1 ];
 const int * k = std::lower_bound( a , end , v );
 return( ( k != end ) && ( *k == v ) ? int( k - f_head.data() ) : -1 );
 }

template< class W >
std::optional< W > StaticGraph< W >::weight( int u , int v ) const
 requires( weighted )
{
 const int k = find( u , v );
 if( k < 0 )
  return( std::nullopt );
 return( f_w[ k ] );
 }

template< class W >
bool StaticGraph< W >::setWeight( int u , int v , const W & w )
 requires( weighted )
{
 ADS_PRE( hasNode( u ) && hasNode( v ) );
 const int k = find( u , v );
 if( k < 0 )
  return( false );
 f_w[ k ] = w;
 if( ! f_directed ) {
  const int h = find( v , u );
  if( h >= 0 )
   f_w[ h ] = w;
  }
 return( true );
 }

template< class W >
typename StaticGraph< W >::ArcRange StaticGraph< W >::out( int u ) const
{
 ADS_PRE( hasNode( u ) );
 const ArcCursor a = arcData();
 return( ArcRange( a + f_off[ u ] , a + f_off[ u + 1 ] ) );
 }

template< class W >
StaticGraph< W > StaticGraph< W >::transpose( void ) const
{
 if( ! f_directed )
  return( *this );
 std::vector< std::tuple< int , int , W > > E;
 E.reserve( f_head.size() );
 for( int u = 0 ; u < n() ; ++u )
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k ) {
   if constexpr( weighted )
    E.emplace_back( f_head[ k ] , u , f_w[ k ] );
   else
    E.emplace_back( f_head[ k ] , u , W() );
   }
 return( StaticGraph( n() , E , true ) );
 }

template< class W >
StaticGraph< W >
StaticGraph< W >::relabel( const std::vector< int > & p ) const
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
   const int v = f_head[ k ];
   if( ! f_directed && ( v < u ) )
    continue;  // each unoriented edge once
   if constexpr( weighted )
    E.emplace_back( p[ u ] , p[ v ] , f_w[ k ] );
   else
    E.emplace_back( p[ u ] , p[ v ] , W() );
   }
 return( StaticGraph( n() , E , f_directed ) );
 }

template< class W >
bool StaticGraph< W >::consistent( void ) const
{
 if( f_off.empty() || ( f_off[ 0 ] != 0 ) ||
     ( std::size_t( f_off.back() ) != f_head.size() ) ||
     ( f_head.size() != ( f_directed ? 1u : 2u ) * std::size_t( f_m ) ) ||
     ( f_w.size() != ( weighted ? f_head.size() : 0u ) ) )
  return( false );
 for( int u = 0 ; u < n() ; ++u ) {
  if( f_off[ u ] > f_off[ u + 1 ] )
   return( false );
  for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k ) {
   const int v = f_head[ k ];
   if( ! hasNode( v ) || ( ( k > f_off[ u ] ) && ( f_head[ k - 1 ] > v ) ) )
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
 const int * hd = g.heads();
 for( int k = -1 ; k < n ; ++k ) {
  const int x = k < 0 ? r : k;  // r, then the least not reached
  if( ( n == 0 ) || ( p[ x ] >= 0 ) )
   continue;
  p[ x ] = next++;
  Q[ tail++ ] = x;
  while( head < tail ) {
   const int u = Q[ head++ ];
   for( int a = off[ u ] ; a < off[ u + 1 ] ; ++a ) {
    const int v = hd[ a ];
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
