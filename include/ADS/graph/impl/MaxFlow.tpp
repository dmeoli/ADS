/*--------------------------------------------------------------------------*/
/*---------------------------- File MaxFlow.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FlowNetwork and feasibleCirculation() [see MaxFlow.h],
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS FlowNetwork ----------------------------*/
/*--------------------------------------------------------------------------*/

inline FlowNetwork::FlowNetwork( int n ) : f_adj( n < 0 ? 0 : n )
{
 ADS_PRE( n >= 0 );
 }

inline int FlowNetwork::addEdge( int u , int v , long long c )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
          ( c >= 0 ) );
 f_e.push_back( { v , c , 0 } );
 f_adj[ u ].push_back( int( f_e.size() ) - 1 );
 f_e.push_back( { u , 0 , 0 } );                  // the residual twin
 f_adj[ v ].push_back( int( f_e.size() ) - 1 );
 ADS_INVARIANT( skewSymmetric() );
 return( int( f_e.size() ) - 2 );
 }

inline long long FlowNetwork::flow( int e ) const
{
 ADS_PRE( ( e >= 0 ) && ( e < int( f_e.size() ) ) );
 return( f_e[ e ].flow );
 }

inline long long FlowNetwork::capacity( int e ) const
{
 ADS_PRE( ( e >= 0 ) && ( e < int( f_e.size() ) ) );
 return( f_e[ e ].cap );
 }

inline long long FlowNetwork::maxFlow( int s , int t , Augment how ,
                                      long long * paths )
{
 ADS_PRE( ( s >= 0 ) && ( s < n() ) && ( t >= 0 ) && ( t < n() ) &&
          ( s != t ) );
 long long total = 0;
 for( ; ; ) {
  long long d = 0;
  if( how == Augment::DINIC ) {
   if( ! levels( s , t ) )
    break;
   f_next.assign( n() , 0 );
   while( ( d = blocking( s , t , INF ) ) > 0 ) {
    total += d;
    if( paths ) ++*paths;
    }
   continue;
   }
  d = how == Augment::EDMONDS_KARP ? bfsAugment( s , t ) :
                                     dfsAugment( s , t );
  if( d == 0 )
   break;
  total += d;
  if( paths ) ++*paths;
  }
 ADS_AUDIT( ! minCut( s )[ t ] );
 ADS_INVARIANT( skewSymmetric() );
 return( total );
 }

inline std::vector< bool > FlowNetwork::minCut( int s ) const
{
 ADS_PRE( ( s >= 0 ) && ( s < n() ) );
 std::vector< bool > S( n() , false );
 std::vector< int > Q = { s };
 S[ s ] = true;
 for( std::size_t i = 0 ; i < Q.size() ; ++i )
  for( int e : f_adj[ Q[ i ] ] )
   if( ( residual( e ) > 0 ) && ! S[ f_e[ e ].to ] ) {
    S[ f_e[ e ].to ] = true;
    Q.push_back( f_e[ e ].to );
    }
 return( S );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- AUGMENTATION -------------------------------*/
/*--------------------------------------------------------------------------*/

inline long long FlowNetwork::residual( int e ) const
{
 return( f_e[ e ].cap - f_e[ e ].flow );
 }

inline void FlowNetwork::push( int e , long long d )
{
 f_e[ e ].flow += d;
 f_e[ e ^ 1 ].flow -= d;                       // opposite symmetry
 }

inline long long FlowNetwork::bfsAugment( int s , int t )
{
 std::vector< int > via( n() , -1 );
 std::vector< int > Q = { s };
 std::vector< bool > seen( n() , false );
 seen[ s ] = true;
 for( std::size_t i = 0 ; ( i < Q.size() ) && ! seen[ t ] ; ++i )
  for( int e : f_adj[ Q[ i ] ] )
   if( ( residual( e ) > 0 ) && ! seen[ f_e[ e ].to ] ) {
    seen[ f_e[ e ].to ] = true;
    via[ f_e[ e ].to ] = e;
    Q.push_back( f_e[ e ].to );
    }
 if( ! seen[ t ] )
  return( 0 );
 long long d = INF;
 for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
  d = std::min( d , residual( via[ v ] ) );
 for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
  push( via[ v ] , d );
 return( d );
 }

inline long long FlowNetwork::dfsAugment( int s , int t )
{
 std::vector< int > via( n() , -1 );
 std::vector< bool > seen( n() , false );
 std::vector< int > S = { s };
 seen[ s ] = true;
 while( ! S.empty() && ! seen[ t ] ) {
  const int u = S.back();
  S.pop_back();
  for( int e : f_adj[ u ] )
   if( ( residual( e ) > 0 ) && ! seen[ f_e[ e ].to ] ) {
    seen[ f_e[ e ].to ] = true;
    via[ f_e[ e ].to ] = e;
    S.push_back( f_e[ e ].to );
    }
  }
 if( ! seen[ t ] )
  return( 0 );
 long long d = INF;
 for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
  d = std::min( d , residual( via[ v ] ) );
 for( int v = t ; v != s ; v = f_e[ via[ v ] ^ 1 ].to )
  push( via[ v ] , d );
 return( d );
 }

inline bool FlowNetwork::levels( int s , int t )
{
 f_level.assign( n() , -1 );
 std::vector< int > Q = { s };
 f_level[ s ] = 0;
 for( std::size_t i = 0 ; i < Q.size() ; ++i )
  for( int e : f_adj[ Q[ i ] ] )
   if( ( residual( e ) > 0 ) && ( f_level[ f_e[ e ].to ] < 0 ) ) {
    f_level[ f_e[ e ].to ] = f_level[ Q[ i ] ] + 1;
    Q.push_back( f_e[ e ].to );
    }
 return( f_level[ t ] >= 0 );
 }

inline long long FlowNetwork::blocking( int u , int t , long long d )
{
 // iterative, the levels may be as deep as n: the edges of the path go
 // on a stack, and an edge into a dead end is passed by f_next[ tail ]
 std::vector< int > path;
 while( u != t ) {
  if( f_next[ u ] < f_adj[ u ].size() ) {
   const int e = f_adj[ u ][ f_next[ u ] ];
   const int v = f_e[ e ].to;
   if( ( residual( e ) > 0 ) && ( f_level[ v ] == f_level[ u ] + 1 ) ) {
    path.push_back( e );                      // forward along e
    u = v;
    }
   else
    ++f_next[ u ];
   continue;
   }
  if( path.empty() )                          // no path left from u
   return( 0 );
  u = f_e[ path.back() ^ 1 ].to;              // back to the tail
  path.pop_back();
  ++f_next[ u ];
  }
 for( int e : path )
  d = std::min( d , residual( e ) );
 for( int e : path )
  push( e , d );
 return( d );
 }

inline bool FlowNetwork::skewSymmetric( void ) const
{
 for( std::size_t e = 0 ; e < f_e.size() ; ++e )
  if( ( f_e[ e ].flow > f_e[ e ].cap ) ||
      ( f_e[ e ].flow != -f_e[ e ^ 1 ].flow ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CIRCULATIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// the flows are within the bounds of their edges and conserved at every
/// node
inline bool circulates( int n , const std::vector< BoundedEdge > & E ,
                        const std::vector< long long > & f )
{
 if( f.size() != E.size() )
  return( false );
 std::vector< long long > net( n , 0 );
 for( std::size_t i = 0 ; i < E.size() ; ++i ) {
  if( ( f[ i ] < E[ i ].low ) || ( f[ i ] > E[ i ].high ) )
   return( false );
  net[ E[ i ].u ] -= f[ i ];
  net[ E[ i ].v ] += f[ i ];
  }
 return( std::all_of( net.begin() , net.end() ,
                      []( long long x ) { return( x == 0 ); } ) );
 }

}  // end( namespace detail )

inline bool feasibleCirculation( int n , const std::vector< BoundedEdge > & E ,
                                 std::vector< long long > * flows )
{
 ADS_PRE( n >= 0 );
 ADS_PRE( std::all_of( E.begin() , E.end() , [ & ]( const auto & e ) {
  return( ( e.u >= 0 ) && ( e.u < n ) && ( e.v >= 0 ) && ( e.v < n ) ); } ) );
 FlowNetwork N( n + 2 );                     // n the source, n + 1 the sink
 std::vector< long long > excess( n , 0 );
 std::vector< int > id;
 for( const auto & e : E ) {
  if( e.low > e.high )
   return( false );
  id.push_back( N.addEdge( e.u , e.v , e.high - e.low ) );
  excess[ e.v ] += e.low;
  excess[ e.u ] -= e.low;
  }
 long long need = 0;
 for( int v = 0 ; v < n ; ++v )
  if( excess[ v ] > 0 ) {
   N.addEdge( n , v , excess[ v ] );
   need += excess[ v ];
   }
  else if( excess[ v ] < 0 )
   N.addEdge( v , n + 1 , -excess[ v ] );
 if( N.maxFlow( n , n + 1 ) != need )
  return( false );
 if( flows ) {
  flows->clear();
  for( std::size_t i = 0 ; i < E.size() ; ++i )
   flows->push_back( E[ i ].low + N.flow( id[ i ] ) );
  }
 ADS_AUDIT( ! flows || detail::circulates( n , E , *flows ) );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File MaxFlow.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
