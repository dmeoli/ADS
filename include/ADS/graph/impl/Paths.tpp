/*--------------------------------------------------------------------------*/
/*----------------------------- File Paths.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The problems on the paths of Paths.h [see Paths.h], with their contracts.
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

/// f( v , l ) on each edge ( u , v ) of length l
template< Graph G , class F >
void forEachLength( const G & g , int u , F && f )
{
 using D = PathLength< G >;
 if constexpr( requires { typename G::Weight; } &&
               ( ! std::is_same_v< typename G::Weight , std::monostate > ) )
  g.forEachEdge( u , [ & ]( int v , const auto & w ) { f( v , D( w ) ); } );
 else
  g.forEachAdj( u , [ & ]( int v ) { f( v , D( 1 ) ); } );
 }

/// c are the counts of the shortest paths from s: 1 on s, 0 on the nodes
/// not reached, and on the others the sum of the counts of the u with an
/// edge ( u , v ) and d[ u ] + 1 = d[ v ], modulo 2^64
template< Graph G >
bool pathCounts( const G & g , int s , const std::vector< std::uint64_t > & c )
{
 const auto d = distance( g , s ).first;
 std::vector< std::uint64_t > sum( g.bound() , 0 );
 for( int u : g.V() )
  if( d[ u ] != INF )
   g.forEachAdj( u , [ & ]( int v ) {
    if( d[ v ] == d[ u ] + 1 )
     sum[ v ] += c[ u ];
    } );
 for( int v : g.V() )
  if( c[ v ] != ( v == s ? 1 : d[ v ] == INF ? 0 : sum[ v ] ) )
   return( false );
 return( true );
 }

/// d and p are the distances and the parents of a breadth first forest
/// rooted at the nodes of S
template< Graph G >
bool bfsForest( const G & g , const std::vector< int > & S ,
                const std::vector< int > & d , const std::vector< int > & p )
{
 std::vector< bool > in( g.bound() , false );
 for( int s : S )
  in[ s ] = true;
 for( int v : g.V() ) {
  if( in[ v ] ) {
   if( ( d[ v ] != 0 ) || ( p[ v ] != NIL ) )
    return( false );
   }
  else if( d[ v ] == INF ) {
   if( p[ v ] != NIL )
    return( false );
   }
  else if( ! g.hasNode( p[ v ] ) || ( d[ p[ v ] ] == INF ) ||
           ( d[ v ] != d[ p[ v ] ] + 1 ) )
   return( false );
  }
 bool ok = true;
 for( int u : g.V() )
  if( d[ u ] != INF )
   g.forEachAdj( u , [ & ]( int v ) {
    if( d[ v ] > d[ u ] + 1 )
     ok = false;
    } );
 return( ok );
 }

/// each kept edge ( u , v ) has L[ u ] >= w + L[ v ], and each L[ u ] is 0
/// or equal to w + L[ v ] on some kept edge
template< Graph G , class Keep >
bool longestFrom( const G & g , Keep & keep ,
                  const std::vector< PathLength< G > > & L )
{
 using D = PathLength< G >;
 for( int u : g.V() ) {
  bool ok = true , reached = L[ u ] == D( 0 );
  forEachLength( g , u , [ & ]( int v , D w ) {
   if( keep( u , v ) ) {
    ok = ok && ! ( L[ u ] < D( w + L[ v ] ) );
    reached = reached || ( L[ u ] == D( w + L[ v ] ) );
    }
   } );
  if( ! ok || ! reached )
   return( false );
  }
 return( true );
 }

/// Johnson's state for the cycles through s: the stack, the blocked
/// nodes and the lists B of the nodes waiting on each blocked one
struct CycleSearch {
 std::vector< std::vector< int > > A;   ///< the adjacent nodes
 std::vector< int > stack;
 std::vector< bool > blocked;
 std::vector< std::set< int > > B;
 int s;

 /// u and the nodes waiting on it, recursively, are unblocked
 void unblock( int u ) {
  blocked[ u ] = false;
  while( ! B[ u ].empty() ) {
   const int w = *B[ u ].begin();
   B[ u ].erase( B[ u ].begin() );
   if( blocked[ w ] )
    unblock( w );
   }
  }

 /// the cycles through s that extend the stack by v; true if any
 template< class F >
 bool circuit( int v , F & f ) {
  bool found = false;
  stack.push_back( v );
  blocked[ v ] = true;
  for( int w : A[ v ] )
   if( w == s ) {
    std::vector< int > C( stack );
    C.push_back( s );
    f( std::as_const( C ) );
    found = true;
    }
   else if( ! blocked[ w ] && circuit( w , f ) )
    found = true;
  if( found )
   unblock( v );
  else
   for( int w : A[ v ] )
    B[ w ].insert( v );      // v waits until w is unblocked
  stack.pop_back();
  return( found );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*----------------------------- SHORTEST PATHS -----------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
std::vector< std::uint64_t > countShortestPaths( const G & g , int s )
{
 ADS_PRE( g.hasNode( s ) );
 std::vector< int > d( g.bound() , INF ) , Q = { s };
 std::vector< std::uint64_t > c( g.bound() , 0 );
 d[ s ] = 0;
 c[ s ] = 1;
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  g.forEachAdj( u , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ u ] + 1;
    Q.push_back( v );
    }
   if( d[ v ] == d[ u ] + 1 )
    c[ v ] += c[ u ];
   } );
  }
 ADS_AUDIT( detail::pathCounts( g , s , c ) );
 return( c );
 }

template< Graph G >
std::pair< std::vector< int > , std::vector< int > >
multiSourceDistance( const G & g , const std::vector< int > & S )
{
 std::vector< int > d( g.bound() , INF ) , p( g.bound() , NIL ) , Q;
 for( int s : S ) {
  ADS_PRE( g.hasNode( s ) );
  if( d[ s ] != 0 ) {
   d[ s ] = 0;
   Q.push_back( s );
   }
  }
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  g.forEachAdj( u , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ u ] + 1;
    p[ v ] = u;
    Q.push_back( v );
    }
   } );
  }
 ADS_AUDIT( detail::bfsForest( g , S , d , p ) );
 return( std::pair( d , p ) );
 }

template< Graph G >
int setDistance( const G & g , const std::vector< bool > & A ,
                 const std::vector< bool > & B )
{
 ADS_PRE( ( int( A.size() ) == g.bound() ) &&
          ( int( B.size() ) == g.bound() ) );
 std::vector< int > d( g.bound() , INF ) , Q;
 for( int u : g.V() )
  if( A[ u ] ) {
   if( B[ u ] )
    return( 0 );
   d[ u ] = 0;
   Q.push_back( u );
   }
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  int found = INF;
  g.forEachAdj( u , [ & ]( int v ) {
   if( d[ v ] == INF ) {
    d[ v ] = d[ u ] + 1;
    if( B[ v ] )
     found = d[ v ];
    Q.push_back( v );
    }
   } );
  if( found != INF )                    // the first node of B discovered
   return( found );
  }
 return( INF );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- LONGEST PATHS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
std::vector< int > longestPathLayering( const G & g )
{
 std::vector< int > in( g.bound() , 0 ) , layer( g.bound() , 0 ) , Q;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { ++in[ v ]; } );
 for( int u : g.V() )
  if( in[ u ] == 0 )
   Q.push_back( u );
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  g.forEachAdj( u , [ & ]( int v ) {
   layer[ v ] = std::max( layer[ v ] , layer[ u ] + 1 );
   if( --in[ v ] == 0 )
    Q.push_back( v );
   } );
  }
 ADS_PRE( int( Q.size() ) == g.n() );            // else a cycle
 ADS_AUDIT( [ & ]( void ) {
  std::vector< bool > below( g.bound() , false );
  for( int u : g.V() ) {
   bool ok = true;
   g.forEachAdj( u , [ & ]( int v ) {
    ok = ok && ( layer[ v ] > layer[ u ] );
    if( layer[ v ] == layer[ u ] + 1 )
     below[ v ] = true;
    } );
   if( ! ok )
    return( false );
   }
  for( int u : g.V() )
   if( ( layer[ u ] > 0 ) && ! below[ u ] )
    return( false );
  return( true );
  }() );
 return( layer );
 }

template< Graph G , class Keep >
std::vector< PathLength< G > > dagLongestPaths( const G & g , Keep && keep )
{
 using D = PathLength< G >;
 std::vector< D > L( g.bound() , D( 0 ) );
 std::vector< char > state( g.bound() , 0 );    // 0 new, 1 open, 2 done
 std::vector< std::vector< std::pair< int , D > > > out( g.bound() );
 for( int u : g.V() )
  detail::forEachLength( g , u , [ & ]( int v , D w ) {
   if( keep( u , v ) )
    out[ u ].emplace_back( v , w );
   } );
 std::vector< std::pair< int , std::size_t > > S;   // node and next edge
 for( int r : g.V() ) {
  if( state[ r ] != 0 )
   continue;
  S.emplace_back( r , 0 );
  state[ r ] = 1;
  while( ! S.empty() ) {
   auto & [ u , i ] = S.back();
   if( i < out[ u ].size() ) {
    const int v = out[ u ][ i++ ].first;
    ADS_PRE( state[ v ] != 1 );                 // else a cycle
    if( state[ v ] == 0 ) {
     state[ v ] = 1;
     S.emplace_back( v , 0 );
     }
    }
   else {
    for( const auto & [ v , w ] : out[ u ] )
     L[ u ] = std::max( L[ u ] , D( w + L[ v ] ) );
    state[ u ] = 2;
    S.pop_back();
    }
   }
  }
 ADS_AUDIT( detail::longestFrom( g , keep , L ) );
 return( L );
 }

template< Graph G >
std::vector< PathLength< G > > dagLongestPaths( const G & g )
{
 return( dagLongestPaths( g , []( int , int ) { return( true ); } ) );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- CYCLES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G , class F >
void simpleCycles( const G & g , int s , F && f )
{
 ADS_PRE( g.directed() && g.hasNode( s ) );
 detail::CycleSearch J;
 J.A.resize( g.bound() );
 for( int u : g.V() )
  J.A[ u ] = g.adj( u );
 J.blocked.assign( g.bound() , false );
 J.B.resize( g.bound() );
 J.s = s;
 J.circuit( s , f );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------------- A LINE ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
std::optional< std::vector< std::size_t > > refuelingStops(
                                       const std::vector< T > & D , T L , T R )
{
 ADS_PRE( ( L >= T( 0 ) ) && ( R >= T( 0 ) ) &&
          std::is_sorted( D.begin() , D.end() ) &&
          ( D.empty() || ( ( D.front() >= T( 0 ) ) && ( D.back() <= L ) ) ) );
 std::vector< std::size_t > S;
 T reach = R;                                   // the farthest point
 std::size_t i = 0;
 while( reach < L ) {
  std::size_t best = D.size();
  for( ; ( i < D.size() ) && ( D[ i ] <= reach ) ; ++i )
   best = i;                                    // the last one in reach
  if( best == D.size() )
   return( std::nullopt );
  S.push_back( best );
  reach = D[ best ] > L - R ? L : D[ best ] + R;      // no overflow, R < L
  }
 ADS_AUDIT( [ & ]( void ) {
  T at = T( 0 );
  for( std::size_t k = 0 ; k < S.size() ; ++k ) {
   if( ( ( k > 0 ) && ( S[ k ] <= S[ k - 1 ] ) ) || ( D[ S[ k ] ] - at > R ) )
    return( false );
   at = D[ S[ k ] ];
   }
  return( L - at <= R );
  }() );
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Paths.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
