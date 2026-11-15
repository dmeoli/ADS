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

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS FlowNetwork ----------------------------*/
/*--------------------------------------------------------------------------*/

inline FlowNetwork::FlowNetwork( int n )
{
 ADS_PRE( n >= 0 );
 f_adj.resize( std::size_t( n ) );
 }

inline int FlowNetwork::addEdge( int u , int v , long long c )
{
 ADS_PRE( ( u >= 0 ) && ( u < n() ) && ( v >= 0 ) && ( v < n() ) &&
	  ( c >= 0 ) );
 const int e = int( f_to.size() );
 f_to.push_back( v );
 f_cap.push_back( c );
 f_res.push_back( c );
 f_adj[ u ].push_back( { e , v } );
 f_to.push_back( u );  // the residual twin
 f_cap.push_back( 0 );
 f_res.push_back( 0 );
 f_adj[ v ].push_back( { e + 1 , u } );
 ADS_INVARIANT( skewSymmetric() );
 return( e );
 }

inline int FlowNetwork::addNode( void )
{
 f_adj.emplace_back();
 if( ! f_imb.empty() )
  f_imb.push_back( 0 );
 return( n() - 1 );
 }

inline void FlowNetwork::setCapacity( int e , long long c )
{
 ADS_PRE( ( e >= 0 ) && ( e < int( f_to.size() ) ) && ( e % 2 == 0 ) &&
	  ( c >= 0 ) );
 if( c < flow( e ) ) {                // the flow cut to c: the
  const long long d = flow( e ) - c;  // tail keeps the excess,
  push( e , -d );                      // the head the deficit
  if( f_imb.empty() )
   f_imb.assign( n() , 0 );
  f_imb[ f_to[ e ^ 1 ] ] += d;
  f_imb[ f_to[ e ] ] -= d;
  f_imbNodes.push_back( f_to[ e ^ 1 ] );
  f_imbNodes.push_back( f_to[ e ] );
  f_unbalanced = true;
  }
 f_res[ e ] = c - flow( e );
 f_cap[ e ] = c;
 ADS_INVARIANT( skewSymmetric() );
 }

inline long long FlowNetwork::inflow( int t ) const
{
 long long f = 0;
 for( const Incidence & a : f_adj[ t ] )
  f -= f_cap[ a.e ] - f_res[ a.e ];  // the twins of those in
 return( f );
 }

inline long long FlowNetwork::flow( int e ) const
{
 ADS_PRE( ( e >= 0 ) && ( e < int( f_to.size() ) ) );
 return( f_cap[ e ] - f_res[ e ] );
 }

inline long long FlowNetwork::capacity( int e ) const
{
 ADS_PRE( ( e >= 0 ) && ( e < int( f_to.size() ) ) );
 return( f_cap[ e ] );
 }

inline void FlowNetwork::assignFlow( const std::vector< long long > & F )
{
 ADS_PRE( 2 * F.size() == f_to.size() );
 for( std::size_t k = 0 ; k < F.size() ; ++k ) {
  ADS_PRE( ( F[ k ] >= 0 ) && ( F[ k ] <= f_cap[ 2 * k ] ) );
  f_res[ 2 * k ] = f_cap[ 2 * k ] - F[ k ];
  f_res[ 2 * k + 1 ] = F[ k ];
  }
 std::fill( f_imb.begin() , f_imb.end() , 0 );
 f_imbNodes.clear();
 f_unbalanced = false;
 ADS_INVARIANT( skewSymmetric() );
 }

inline long long FlowNetwork::maxFlow( int s , int t , Augment how ,
				       long long * paths )
{
 ADS_PRE( ( s >= 0 ) && ( s < n() ) && ( t >= 0 ) && ( t < n() ) &&
	  ( s != t ) );
 // the value before the cuts of setCapacity(), whose excess or deficit
 // at t is the flow that they took from it or gave it
 const long long before = inflow( t ) - ( f_unbalanced ? f_imb[ t ] : 0 );
 if( f_unbalanced )  // the push-relabel takes the
  rebalance( s , t , how != Augment::PUSH_RELABEL );  // excesses itself
 if( how == Augment::PUSH_RELABEL )
  pushRelabel( s , t );
 else
  for( ; ; ) {
   long long d = 0;
   if( how == Augment::DINIC ) {
    if( ! levels( s , t ) )
     break;
    f_next.assign( n() , 0 );
    while( ( d = blocking( s , t , INF ) ) > 0 )
     if( paths )
      ++*paths;
    continue;
    }
   d = how == Augment::EDMONDS_KARP ? bfsAugment( s , t ) : dfsAugment( s ,
									t );
   if( d == 0 )
    break;
   if( paths )
    ++*paths;
   }
 ADS_AUDIT( ! minCut( s )[ t ] );
 ADS_INVARIANT( skewSymmetric() );
 return( inflow( t ) - before );
 }

inline std::vector< bool > FlowNetwork::minCut( int s ) const
{
 ADS_PRE( ( s >= 0 ) && ( s < n() ) );
 std::vector< bool > S( n() , false );
 std::vector< int > Q = { s };
 S[ s ] = true;
 for( std::size_t i = 0 ; i < Q.size() ; ++i )
  for( const auto & [ e , v ] : f_adj[ Q[ i ] ] )
   if( ! S[ v ] && ( residual( e ) > 0 ) ) {
    S[ v ] = true;
    Q.push_back( v );
    }
 return( S );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- AUGMENTATION -------------------------------*/
/*--------------------------------------------------------------------------*/

inline long long FlowNetwork::residual( int e ) const
{
 return( f_res[ e ] );
 }

inline void FlowNetwork::push( int e , long long d )
{
 f_res[ e ] -= d;
 f_res[ e ^ 1 ] += d;  // opposite symmetry
 }

inline long long FlowNetwork::bfsAugment( int s , int t )
{
 std::vector< int > via( n() , -1 );
 std::vector< int > Q = { s };
 std::vector< bool > seen( n() , false );
 seen[ s ] = true;
 for( std::size_t i = 0 ; ( i < Q.size() ) && ! seen[ t ] ; ++i )
  for( const auto & [ e , v ] : f_adj[ Q[ i ] ] )
   if( ! seen[ v ] && ( residual( e ) > 0 ) ) {
    seen[ v ] = true;
    via[ v ] = e;
    Q.push_back( v );
    }
 if( ! seen[ t ] )
  return( 0 );
 long long d = INF;
 for( int v = t ; v != s ; v = f_to[ via[ v ] ^ 1 ] )
  d = std::min( d , residual( via[ v ] ) );
 for( int v = t ; v != s ; v = f_to[ via[ v ] ^ 1 ] )
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
  for( const auto & [ e , v ] : f_adj[ u ] )
   if( ! seen[ v ] && ( residual( e ) > 0 ) ) {
    seen[ v ] = true;
    via[ v ] = e;
    S.push_back( v );
    }
  }
 if( ! seen[ t ] )
  return( 0 );
 long long d = INF;
 for( int v = t ; v != s ; v = f_to[ via[ v ] ^ 1 ] )
  d = std::min( d , residual( via[ v ] ) );
 for( int v = t ; v != s ; v = f_to[ via[ v ] ^ 1 ] )
  push( via[ v ] , d );
 return( d );
 }

inline bool FlowNetwork::levels( int s , int t )
{
 f_level.assign( n() , -1 );
 std::vector< int > Q = { s };
 f_level[ s ] = 0;
 for( std::size_t i = 0 ; i < Q.size() ; ++i )
  for( const auto & [ e , v ] : f_adj[ Q[ i ] ] )
   if( ( f_level[ v ] < 0 ) && ( residual( e ) > 0 ) ) {
    f_level[ v ] = f_level[ Q[ i ] ] + 1;
    Q.push_back( v );
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
   const auto [ e , v ] = f_adj[ u ][ f_next[ u ] ];
   if( ( residual( e ) > 0 ) && ( f_level[ v ] == f_level[ u ] + 1 ) ) {
    path.push_back( e );  // forward along e
    u = v;
    }
   else
    ++f_next[ u ];
   continue;
   }
  if( path.empty() )  // no path left from u
   return( 0 );
  u = f_to[ path.back() ^ 1 ];  // back to the tail
  path.pop_back();
  ++f_next[ u ];
  }
 for( int e : path )
  d = std::min( d , residual( e ) );
 for( int e : path )
  push( e , d );
 return( d );
 }

inline void FlowNetwork::rebalance( int s , int t , bool excesses )
{
 // each excess to s, to t or to a deficit, then each deficit from s, t or
 // an excess, along paths of the residual network found by visits in
 // breadth; only the nodes that setCapacity() touched are looked at, and
 // the visits mark the nodes with their number instead of clearing
 const int N = n();
 std::vector< int > via( N );
 std::vector< int > Q;
 std::vector< int > seen( N , -1 );
 int visit = 0;
 for( int dir = excesses ? 0 : 1 ; dir < 2 ; ++dir )
  for( const int x : f_imbNodes )
   while( ( x != s ) && ( x != t ) &&
	  ( dir == 0 ? f_imb[ x ] > 0 : f_imb[ x ] < 0 ) ) {
    // dir 0: forward from x to an end; dir 1: backward from x
    ++visit;
    Q.assign( 1 , x );
    seen[ x ] = visit;
    int y = -1;
    for( std::size_t i = 0 ; ( i < Q.size() ) && ( y < 0 ) ; ++i )
     for( const auto & [ e , v ] : f_adj[ Q[ i ] ] ) {
      const int a = dir == 0 ? e : e ^ 1;  // the arc that is pushed
      if( ( seen[ v ] == visit ) || ( residual( a ) <= 0 ) )
       continue;
      seen[ v ] = visit;
      via[ v ] = a;
      Q.push_back( v );
      if( ( v == s ) || ( v == t ) ||
	  ( dir == 0 ? f_imb[ v ] < 0 : f_imb[ v ] > 0 ) ) {
       y = v;
       break;
       }
      }
    ADS_INVARIANT( y >= 0 );  // one is always there
    if( y < 0 )
     return;
    long long d = dir == 0 ? f_imb[ x ] : -f_imb[ x ];
    if( ( y != s ) && ( y != t ) )
     d = std::min( d , dir == 0 ? -f_imb[ y ] : f_imb[ y ] );
    for( int v = y ; v != x ; ) {
     const int a = via[ v ];
     d = std::min( d , residual( a ) );
     v = dir == 0 ? f_to[ a ^ 1 ] : f_to[ a ];
     }
    for( int v = y ; v != x ; ) {
     const int a = via[ v ];
     push( a , d );
     v = dir == 0 ? f_to[ a ^ 1 ] : f_to[ a ];
     }
    f_imb[ x ] += dir == 0 ? -d : d;
    if( ( y != s ) && ( y != t ) )
     f_imb[ y ] += dir == 0 ? d : -d;
    }
 f_imb[ s ] = f_imb[ t ] = 0;
 if( excesses ) {  // nothing left anywhere
  f_imbNodes.clear();
  f_unbalanced = false;
  }
 }

inline bool FlowNetwork::skewSymmetric( void ) const
{
 for( std::size_t e = 0 ; e < f_to.size() ; ++e )
  if( ( f_res[ e ] < 0 ) ||
      ( f_cap[ e ] - f_res[ e ] != f_res[ e ^ 1 ] - f_cap[ e ^ 1 ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- CIRCULATIONS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

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
  return( ( e.u >= 0 ) && ( e.u < n ) && ( e.v >= 0 ) && ( e.v < n ) );
  } ) );
 FlowNetwork N( n + 2 );  // n the source, n + 1 the sink
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
