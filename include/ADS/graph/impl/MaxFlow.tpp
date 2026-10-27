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

inline int FlowNetwork::addNode( void )
{
 f_adj.emplace_back();
 if( ! f_imb.empty() )
  f_imb.push_back( 0 );
 return( n() - 1 );
 }

inline void FlowNetwork::setCapacity( int e , long long c )
{
 ADS_PRE( ( e >= 0 ) && ( e < int( f_e.size() ) ) && ( e % 2 == 0 ) &&
          ( c >= 0 ) );
 if( c < f_e[ e ].flow ) {                     // the flow cut to c: the
  const long long d = f_e[ e ].flow - c;       // tail keeps the excess,
  push( e , -d );                              // the head the deficit
  if( f_imb.empty() )
   f_imb.assign( n() , 0 );
  f_imb[ f_e[ e ^ 1 ].to ] += d;
  f_imb[ f_e[ e ].to ] -= d;
  f_unbalanced = true;
  }
 f_e[ e ].cap = c;
 ADS_INVARIANT( skewSymmetric() );
 }

inline long long FlowNetwork::inflow( int t ) const
{
 long long f = 0;
 for( int e : f_adj[ t ] )
  f -= f_e[ e ].flow;                          // the twins of those in
 return( f );
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
 // the value before the cuts of setCapacity(), whose excess or deficit
 // at t is the flow that they took from it or gave it
 const long long before = inflow( t ) -
                          ( f_unbalanced ? f_imb[ t ] : 0 );
 if( f_unbalanced )
  rebalance( s , t );
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
     if( paths ) ++*paths;
    continue;
    }
   d = how == Augment::EDMONDS_KARP ? bfsAugment( s , t ) :
                                      dfsAugment( s , t );
   if( d == 0 )
    break;
   if( paths ) ++*paths;
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

inline void FlowNetwork::rebalance( int s , int t )
{
 // each excess to s, to t or to a deficit, then each deficit from s or t,
 // along paths of the residual network found by visits in breadth
 const int N = n();
 std::vector< int > via( N );
 std::vector< int > Q;
 std::vector< char > seen( N );
 for( int dir = 0 ; dir < 2 ; ++dir )
  for( int x = 0 ; x < N ; ++x )
   while( ( x != s ) && ( x != t ) &&
          ( dir == 0 ? f_imb[ x ] > 0 : f_imb[ x ] < 0 ) ) {
    // dir 0: forward from x to an end; dir 1: backward from x
    std::fill( seen.begin() , seen.end() , 0 );
    Q.assign( 1 , x );
    seen[ x ] = 1;
    int y = -1;
    for( std::size_t i = 0 ; ( i < Q.size() ) && ( y < 0 ) ; ++i )
     for( int e : f_adj[ Q[ i ] ] ) {
      const int a = dir == 0 ? e : e ^ 1;     // the arc that is pushed
      const int v = f_e[ e ].to;
      if( seen[ v ] || ( residual( a ) <= 0 ) )
       continue;
      seen[ v ] = 1;
      via[ v ] = a;
      Q.push_back( v );
      if( ( v == s ) || ( v == t ) ||
          ( dir == 0 ? f_imb[ v ] < 0 : f_imb[ v ] > 0 ) ) {
       y = v;
       break;
       }
      }
    ADS_INVARIANT( y >= 0 );                  // one is always there
    if( y < 0 )
     return;
    long long d = dir == 0 ? f_imb[ x ] : -f_imb[ x ];
    if( ( y != s ) && ( y != t ) )
     d = std::min( d , dir == 0 ? -f_imb[ y ] : f_imb[ y ] );
    for( int v = y ; v != x ; ) {
     const int a = via[ v ];
     d = std::min( d , residual( a ) );
     v = dir == 0 ? f_e[ a ^ 1 ].to : f_e[ a ].to;
     }
    for( int v = y ; v != x ; ) {
     const int a = via[ v ];
     push( a , d );
     v = dir == 0 ? f_e[ a ^ 1 ].to : f_e[ a ].to;
     }
    f_imb[ x ] += dir == 0 ? -d : d;
    if( ( y != s ) && ( y != t ) )
     f_imb[ y ] += dir == 0 ? d : -d;
    }
 f_imb[ s ] = f_imb[ t ] = 0;
 f_unbalanced = false;
 }

inline void FlowNetwork::pushRelabel( int s , int t )
{
 // the arcs in rows by tail (the residual capacities, the heads and the
 // places of the twins), so that a discharge reads its arcs in a row
 const int N = n();
 const std::size_t M = f_e.size();
 std::vector< int > first( N + 1 , 0 ) , pos( M );
 for( int u = 0 ; u < N ; ++u )
  first[ u + 1 ] = first[ u ] + int( f_adj[ u ].size() );
 std::vector< int > head( M ) , twin( M );
 std::vector< long long > rc( M );
 for( int u = 0 ; u < N ; ++u )
  for( std::size_t i = 0 ; i < f_adj[ u ].size() ; ++i )
   pos[ f_adj[ u ][ i ] ] = first[ u ] + int( i );
 for( std::size_t e = 0 ; e < M ; ++e ) {
  head[ pos[ e ] ] = f_e[ e ].to;
  twin[ pos[ e ] ] = pos[ e ^ 1 ];
  rc[ pos[ e ] ] = residual( int( e ) );
  }
 std::vector< long long > ex( N , 0 );
 std::vector< int > d( N ) , cur( N );
 // the active nodes by label, as stacks, and all the nodes by label, as
 // doubly linked lists for the gaps
 std::vector< int > aFirst( 2 * N + 1 ) , aNext( N );
 std::vector< char > queued( N , 0 );
 std::vector< int > bFirst( 2 * N + 1 ) , bNext( N ) , bPrev( N );
 int aMax = -1 , bMax = -1;
 auto aPush = [ & ]( int v ) {
  if( queued[ v ] )
   return;
  queued[ v ] = 1;
  aNext[ v ] = aFirst[ d[ v ] ];
  aFirst[ d[ v ] ] = v;
  aMax = std::max( aMax , d[ v ] );
  };
 auto bAdd = [ & ]( int v ) {
  bPrev[ v ] = -1;
  bNext[ v ] = bFirst[ d[ v ] ];
  if( bNext[ v ] >= 0 )
   bPrev[ bNext[ v ] ] = v;
  bFirst[ d[ v ] ] = v;
  bMax = std::max( bMax , d[ v ] );
  };
 auto bDel = [ & ]( int v ) {
  if( bPrev[ v ] >= 0 )
   bNext[ bPrev[ v ] ] = bNext[ v ];
  else
   bFirst[ d[ v ] ] = bNext[ v ];
  if( bNext[ v ] >= 0 )
   bPrev[ bNext[ v ] ] = bPrev[ v ];
  };
 auto pushArc = [ & ]( int a , long long x ) {
  rc[ a ] -= x;
  rc[ twin[ a ] ] += x;
  };

 // the labels: the distances to the target in the residual network, by
 // a visit in breadth backwards; the others at the top, out of the game
 int target = t , top = N;
 std::vector< int > Q;
 auto relabelAll = [ & ]( void ) {
  std::fill( d.begin() , d.end() , top );
  std::fill( aFirst.begin() , aFirst.end() , -1 );
  std::fill( queued.begin() , queued.end() , 0 );
  std::fill( bFirst.begin() , bFirst.end() , -1 );
  aMax = bMax = -1;
  d[ target ] = 0;
  Q.assign( 1 , target );
  for( std::size_t i = 0 ; i < Q.size() ; ++i ) {
   const int v = Q[ i ];
   for( int a = first[ v ] ; a < first[ v + 1 ] ; ++a ) {
    const int u = head[ a ];
    if( ( d[ u ] == top ) && ( rc[ twin[ a ] ] > 0 ) &&
        ( u != s ) && ( u != t ) ) {
     d[ u ] = d[ v ] + 1;
     Q.push_back( u );
     }
    }
   }
  for( int v : Q )
   if( ( v != s ) && ( v != t ) ) {
    cur[ v ] = first[ v ];
    bAdd( v );
    if( ex[ v ] > 0 )
     aPush( v );
    }
  };

 const long long nm = 6LL * N + static_cast< long long >( M ) / 2;
 long long work = 0;
 // the discharges of the highest active nodes, with the gaps and the
 // global relabelings; a node at the top no longer reaches the target
 auto run = [ & ]( void ) {
  relabelAll();
  while( aMax >= 0 ) {
   const int u = aFirst[ aMax ];
   if( u < 0 ) {
    --aMax;
    continue;
    }
   aFirst[ aMax ] = aNext[ u ];
   queued[ u ] = 0;
   if( ( d[ u ] != aMax ) || ( ex[ u ] == 0 ) )
    continue;                               // a stale entry
   for( ; ; ) {                             // the discharge of u
    int a = cur[ u ];
    for( ; a < first[ u + 1 ] ; ++a )
     if( ( rc[ a ] > 0 ) && ( d[ head[ a ] ] == d[ u ] - 1 ) ) {
      const int v = head[ a ];
      const long long x = std::min( ex[ u ] , rc[ a ] );
      pushArc( a , x );
      ex[ u ] -= x;
      if( ( ex[ v ] == 0 ) && ( v != s ) && ( v != t ) )
       aPush( v );
      ex[ v ] += x;
      if( ex[ u ] == 0 )
       break;
      }
    if( ex[ u ] == 0 ) {
     cur[ u ] = a;
     break;
     }
    // no admissible arc left: the relabeling of u, or a gap
    const int old = d[ u ];
    int nd = top;
    work += 12 + ( first[ u + 1 ] - first[ u ] );
    for( int b = first[ u ] ; b < first[ u + 1 ] ; ++b )
     if( rc[ b ] > 0 )
      nd = std::min( nd , d[ head[ b ] ] + 1 );
    bDel( u );
    if( bFirst[ old ] < 0 ) {               // a gap at old: all above it
     for( int k = old + 1 ; k <= bMax ; ++k ) {
      for( int v = bFirst[ k ] ; v >= 0 ; v = bNext[ v ] )
       d[ v ] = top;
      bFirst[ k ] = -1;
      }
     bMax = old - 1;
     d[ u ] = top;
     break;
     }
    d[ u ] = std::min( nd , top );
    if( d[ u ] == top )
     break;
    cur[ u ] = first[ u ];
    bAdd( u );
    if( 2 * work > nm ) {                   // a global relabeling
     work = 0;
     relabelAll();
     break;
     }
    }
   }
  };

 // the first phase: a maximum preflow from the flow there is, the arcs
 // out of s saturated; the second: the excesses left back to s
 for( int a = first[ s ] ; a < first[ s + 1 ] ; ++a )
  if( rc[ a ] > 0 ) {
   ex[ head[ a ] ] += rc[ a ];
   pushArc( a , rc[ a ] );
   }
 run();
 bool left = false;                         // excesses that missed t
 for( int v = 0 ; ( v < N ) && ! left ; ++v )
  left = ( ex[ v ] > 0 ) && ( v != s ) && ( v != t );
 if( left ) {
  target = s;
  top = 2 * N;
  work = 0;
  run();
  }
 for( std::size_t e = 0 ; e < M ; ++e )
  f_e[ e ].flow = f_e[ e ].cap - rc[ pos[ e ] ];
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
