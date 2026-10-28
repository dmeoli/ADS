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
 const int e = int( f_to.size() );
 f_to.push_back( v );
 f_cap.push_back( c );
 f_res.push_back( c );
 f_adj[ u ].push_back( { e , v } );
 f_to.push_back( u );                             // the residual twin
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
 if( c < flow( e ) ) {                         // the flow cut to c: the
  const long long d = flow( e ) - c;           // tail keeps the excess,
  push( e , -d );                              // the head the deficit
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
  f -= f_cap[ a.e ] - f_res[ a.e ];              // the twins of those in
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

inline long long FlowNetwork::maxFlow( int s , int t , Augment how ,
                                      long long * paths )
{
 ADS_PRE( ( s >= 0 ) && ( s < n() ) && ( t >= 0 ) && ( t < n() ) &&
          ( s != t ) );
 // the value before the cuts of setCapacity(), whose excess or deficit
 // at t is the flow that they took from it or gave it
 const long long before = inflow( t ) -
                          ( f_unbalanced ? f_imb[ t ] : 0 );
 if( f_unbalanced )                         // the push-relabel takes the
  rebalance( s , t , how != Augment::PUSH_RELABEL );   // excesses itself
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
 f_res[ e ^ 1 ] += d;                          // opposite symmetry
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
    path.push_back( e );                      // forward along e
    u = v;
    }
   else
    ++f_next[ u ];
   continue;
   }
  if( path.empty() )                          // no path left from u
   return( 0 );
  u = f_to[ path.back() ^ 1 ];              // back to the tail
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
      const int a = dir == 0 ? e : e ^ 1;     // the arc that is pushed
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
    ADS_INVARIANT( y >= 0 );                  // one is always there
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
 if( excesses ) {                           // nothing left anywhere
  f_imbNodes.clear();
  f_unbalanced = false;
  }
 }

inline void FlowNetwork::pushRelabel( int s , int t )
{
 // on the edges in place, each with its twin e ^ 1, no copy of them
 const int N = n();
 long long * const R = f_res.data();
 std::vector< long long > ex( N , 0 );
 // the excesses left by setCapacity(), where they are; with none and a
 // flow already, the flow is maximum unless a residual path leads from s
 // to t, which a visit of the side of s finds without touching the rest
 bool any = false;
 if( f_unbalanced ) {
  for( const int x : f_imbNodes ) {
   if( ( x != s ) && ( x != t ) && ( f_imb[ x ] > 0 ) ) {
    ex[ x ] = f_imb[ x ];
    any = true;
    }
   f_imb[ x ] = 0;
   }
  f_imbNodes.clear();
  f_unbalanced = false;
  }
 if( ! any && ( inflow( t ) != 0 ) ) {       // a flow already: warm
  std::vector< char > seen( N , 0 );
  std::vector< int > S = { s };
  seen[ s ] = 1;
  bool reach = false;
  while( ! S.empty() && ! reach ) {
   const int u = S.back();
   S.pop_back();
   for( const auto & [ e , v ] : f_adj[ u ] )
    if( ! seen[ v ] && ( R[ e ] > 0 ) ) {
     seen[ v ] = 1;
     reach = reach || ( v == t );
     S.push_back( v );
     }
   }
  if( ! reach )
   return;
  }
 std::vector< int > d( N , N );
 // the active nodes by label, as stacks, and all the nodes by label, as
 // doubly linked lists for the gaps
 std::vector< int > aFirst( 2 * N + 1 , -1 ) , aNext( N );
 std::vector< char > queued( N , 0 );
 std::vector< int > bFirst( 2 * N + 1 , -1 ) , bNext( N ) , bPrev( N );
 int aMax = -1 , bMax = -1;
 auto aPush = [ & ]( int v ) {
  if( queued[ v ] )
   return;
  queued[ v ] = 1;
  aNext[ v ] = aFirst[ d[ v ] ];
  aFirst[ d[ v ] ] = v;
  if( d[ v ] > aMax )
   aMax = d[ v ];
  };
 auto bAdd = [ & ]( int v ) {
  bPrev[ v ] = -1;
  bNext[ v ] = bFirst[ d[ v ] ];
  if( bNext[ v ] >= 0 )
   bPrev[ bNext[ v ] ] = v;
  bFirst[ d[ v ] ] = v;
  if( d[ v ] > bMax )
   bMax = d[ v ];
  };
 auto bDel = [ & ]( int v ) {
  if( bPrev[ v ] >= 0 )
   bNext[ bPrev[ v ] ] = bNext[ v ];
  else
   bFirst[ d[ v ] ] = bNext[ v ];
  if( bNext[ v ] >= 0 )
   bPrev[ bNext[ v ] ] = bPrev[ v ];
  };

 // the labels: the distances to the target in the residual network, by a
 // visit in breadth backwards over the nodes still in the game (those
 // below the top); the others stay at the top
 int target = t , top = N;
 std::vector< int > Q;
 std::vector< char > inGame( N , 1 );
 auto relabelAll = [ & ]( bool first ) {
  int * const D = d.data();
  const char * const G = inGame.data();
  int want = 0;                             // the active ones to label
  for( int v = 0 ; v < N ; ++v )
   if( G[ v ] ) {
    D[ v ] = top;
    queued[ v ] = 0;
    want += ( ex[ v ] > 0 ) && ( v != s ) && ( v != t );
    }
  for( int k = 0 ; k <= aMax ; ++k )
   aFirst[ k ] = -1;
  for( int k = 0 ; k <= bMax ; ++k )
   bFirst[ k ] = -1;
  aMax = bMax = -1;
  // s and t out of the visit, the target at 0 and the other at the top
  D[ s ] = D[ t ] = -1;
  D[ target ] = 0;
  Q.resize( std::size_t( N ) );
  int * const q = Q.data();
  std::size_t head = 0 , tail = 0;
  q[ tail++ ] = target;
  const int tp = top;
  const bool all = target == t;             // the first phase: no node out
  std::size_t level = tail;                 // the end of the level of now
  int last = 0;                             // its label
  while( head < tail ) {
   if( head == level ) {                    // a level done: all the
    if( want == 0 )                         // active ones labeled, the
     break;                                 // others above (the lazy
    level = tail;                           // update of HIPR)
    ++last;
    }
   const int v = q[ head++ ];
   const int dv = D[ v ] + 1;
   const Incidence * a = f_adj[ v ].data();
   const Incidence * const z = a + f_adj[ v ].size();
   // no branch on the data in the first labeling: the node taken by a
   // conditional write, as its tests are hard to predict
   if( all && first )
    for( ; a != z ; ++a ) {
     const int u = a->to;
     const bool ok = ( D[ u ] == tp ) & ( R[ a->e ^ 1 ] > 0 );
     D[ u ] = ok ? dv : D[ u ];
     q[ tail ] = u;
     tail += ok;
     want -= ok & ( ex[ u ] > 0 );
     }
   else
    for( ; a != z ; ++a ) {
     const int u = a->to;
     if( D[ u ] != tp )                     // labeled, or s or t
      continue;
     const int e = a->e;
     if( G[ u ] && ( R[ e ^ 1 ] > 0 ) ) {
      D[ u ] = dv;
      q[ tail++ ] = u;
      want -= ex[ u ] > 0;
      }
     }
   }
  D[ target == t ? s : t ] = tp;
  if( head < tail ) {                       // stopped early: the nodes not
   const int above = last + 2;              // reached get a lower bound of
   for( int v = 0 ; v < N ; ++v )           // their distance
    if( G[ v ] && ( D[ v ] == tp ) && ( v != s ) && ( v != t ) &&
        ( above < tp ) ) {
     D[ v ] = above;
     q[ tail++ ] = v;
     }
   }
  for( std::size_t i = 1 ; i < tail ; ++i ) {
   const int v = q[ i ];
   bAdd( v );
   if( ex[ v ] > 0 )
    aPush( v );
   }
  };

 const long long nm = 6LL * N + static_cast< long long >( f_to.size() ) / 2;
 long long work = 0;
 // the discharges of the highest active nodes: each scans its edges once,
 // pushing to the lower neighbours and taking the least label of the
 // others, which is its new label if an excess is left; the gaps and the
 // global relabelings as in Cherkassky and Goldberg
 auto run = [ & ]( bool global ) {
  while( aMax >= 0 ) {
   const int u = aFirst[ aMax ];
   if( u < 0 ) {
    --aMax;
    continue;
    }
   aFirst[ aMax ] = aNext[ u ];
   queued[ u ] = 0;
   const int du = d[ u ];
   if( ( du != aMax ) || ( ex[ u ] == 0 ) )
    continue;                               // a stale entry
   long long x = ex[ u ];
   int nd = top;
   const auto & A = f_adj[ u ];
   for( const auto & [ e , v ] : A ) {
    const int dv = d[ v ];
    if( ( dv >= du ) && ( dv >= nd ) )
     continue;                              // no use, read no edge
    const long long r = R[ e ];
    if( r <= 0 )
     continue;
    if( dv < du ) {                         // dv = du - 1, the labels valid
     const long long y = r < x ? r : x;
     R[ e ] -= y;
     R[ e ^ 1 ] += y;
     if( ( ex[ v ] == 0 ) && ( v != s ) && ( v != t ) )
      aPush( v );
     ex[ v ] += y;
     x -= y;
     if( x == 0 )
      break;
     }
    else if( dv < nd )
     nd = dv;
    }
   ex[ u ] = x;
   if( x == 0 )
    continue;
   // an excess left: the relabeling of u, or a gap
   work += 12 + static_cast< long long >( A.size() );
   bDel( u );
   if( bFirst[ du ] < 0 ) {                 // a gap at du: all above it
    for( int k = du + 1 ; k <= bMax ; ++k ) {
     for( int v = bFirst[ k ] ; v >= 0 ; v = bNext[ v ] )
      d[ v ] = top;
     bFirst[ k ] = -1;
     }
    bMax = du - 1;
    d[ u ] = top;
    continue;
    }
   d[ u ] = nd + 1 < top ? nd + 1 : top;
   if( d[ u ] == top )
    continue;
   bAdd( u );
   aPush( u );
   if( global && ( 2 * work > nm ) ) {      // a global relabeling
    work = 0;
    relabelAll( false );
    }
   }
  };

 // the first phase: the edges out of s saturated, the labels up to the
 // last node with an excess, and a maximum preflow; the excesses that
 // cannot reach t end at the top
 for( const auto & [ e , v ] : f_adj[ s ] ) {
  const long long r = R[ e ];
  if( ( r > 0 ) && ( v != s ) ) {
   R[ e ] = 0;
   R[ e ^ 1 ] += r;
   ex[ v ] += r;
   }
  }
 relabelAll( true );
 run( true );

 // the second phase, only if an excess missed t: the excesses back to s,
 // on the labels of the distances to s among the nodes out of the game of
 // the first phase, those at the top
 bool left = false;
 for( int v = 0 ; ( v < N ) && ! left ; ++v )
  left = ( ex[ v ] > 0 ) && ( v != s ) && ( v != t ) && ( d[ v ] == top );
 if( ! left )
  return;
 for( int v = 0 ; v < N ; ++v )
  inGame[ v ] = ( d[ v ] == top ) || ( v == s );
 inGame[ t ] = 0;
 target = s;
 top = 2 * N;
 relabelAll( false );
 run( false );
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
