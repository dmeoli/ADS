/*--------------------------------------------------------------------------*/
/*---------------------- File MinimumSpanningTree.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum spanning trees of MinimumSpanningTree.h [see
 * MinimumSpanningTree.h], with their contracts.
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

/// p is a minimum spanning tree of the component of r: the nodes with a
/// parent and r are those reached from r, and the edges to the parents a
/// minimum spanning forest of the edges inside it
template< Graph G >
bool primTree( const G & g , int r , const std::vector< int > & p )
{
 using W = typename G::Weight;
 const auto reach = distance( g , r ).first;
 std::vector< WeightedEdge< W > > A , T;
 for( int v : g.V() ) {
  const bool reached = reach[ v ] != INF;
  if( ( v != r ) && ( reached != ( p[ v ] != NIL ) ) )
   return( false );
  if( ( v == r ) || ! reached )
   continue;
  bool found = false;
  g.forEachEdge( p[ v ] , [ & ]( int x , const W & w ) {
   if( ( x == v ) && ! found ) {
    T.push_back( { p[ v ] , v , w } );
    found = true;
    }
   } );
  if( ! found )
   return( false );
  }
 for( const auto & e : edges( g ) )
  if( reach[ e.u ] != INF )
   A.push_back( e );
 return( isMinimumSpanningForest( g.bound() , A , T ) );
 }

}  // end( namespace detail )

template< Graph G >
std::vector< WeightedEdge< typename G::Weight > > edges( const G & g )
{
 ADS_PRE( ! g.directed() );
 std::vector< WeightedEdge< typename G::Weight > > E;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const auto & w ) {
   if( u < v )
    E.push_back( { u , v , w } );
   } );
 return( E );
 }

template< class W , Mergeable M >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          M & sets , long k ,
                                          MergeCounts * c )
{
 ADS_PRE( n >= 0 );
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( const auto & e ) {
  return( ( e.u >= 0 ) && ( e.u < n ) && ( e.v >= 0 ) && ( e.v < n ) &&
          ( ( k <= 0 ) || ( ( e.w >= W( 0 ) ) && ( e.w < W( k ) ) ) ) ); } ) );
 [[maybe_unused]] const auto all = ADS_CONTRACTS >= 3 ? A :
                                   std::vector< WeightedEdge< W > >();
 if( k > 0 ) {                                  // Counting Sort, stable
  std::vector< std::size_t > cnt( k + 1 , 0 );
  for( const auto & e : A ) ++cnt[ std::size_t( e.w ) + 1 ];
  for( long i = 0 ; i < k ; ++i ) cnt[ i + 1 ] += cnt[ i ];
  std::vector< WeightedEdge< W > > B( A.size() );
  for( const auto & e : A ) B[ cnt[ std::size_t( e.w ) ]++ ] = e;
  A = std::move( B );
  }
 else
  std::stable_sort( A.begin() , A.end() , []( const auto & a ,
                    const auto & b ) { return( a.w < b.w ); } );
 std::vector< WeightedEdge< W > > T;
 MergeCounts t;
 for( std::size_t i = 0 ; ( i < A.size() ) && ( int( T.size() ) < n - 1 ) ;
      ++i ) {
  ++t.merges;
  if( sets.merge( A[ i ].u , A[ i ].v ) ) {     // the ends were apart
   ++t.unions;
   T.push_back( A[ i ] );
   }
  }
 if( c ) *c = t;
 ADS_AUDIT( isMinimumSpanningForest( n , all , T ) );
 return( T );
 }

template< class W >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          long k )
{
 ADS_PRE( n >= 0 );
 MFSet M( std::max( n , 0 ) );
 return( kruskal( n , std::move( A ) , M , k ) );
 }

template< Graph G >
auto kruskal( const G & g , long k )
{
 return( kruskal( g.bound() , edges( g ) , k ) );
 }

template< Graph G , PriorityQueue Q >
std::vector< int > prim( const G & g , int r , Q & q , FrontierCounts * c )
{
 ADS_PRE( ! g.directed() && g.hasNode( r ) && q.empty() );
 using W = typename G::Weight;
 using F = PriorityFrontier< Q >;
 const int n = g.bound();
 std::vector< int > p( n , NIL );
 std::vector< bool > in( n , false );                 // in the tree
 std::vector< W > key( n , infinity< W >() );
 std::vector< typename Q::Handle > pos( n );
 FrontierCounts k;
 for( int u : g.V() ) {
  pos[ u ] = q.insert( u , F::key( u , u == r ? W( 0 ) : infinity< W >() ) );
  ++k.adds;
  }
 key[ r ] = W( 0 );
 while( ! q.empty() ) {
  const int u = q.deleteMin();
  ++k.extracts;
  in[ u ] = true;
  if( key[ u ] == infinity< W >() )                 // another component
   continue;
  g.forEachEdge( u , [ & ]( int v , const W & w ) {
   ++k.edges;
   if( ! in[ v ] && ( w < key[ v ] ) ) {
    key[ v ] = w;
    q.decrease( pos[ v ] , F::key( v , w ) );
    ++k.decreases;
    p[ v ] = u;
    }
   } );
  }
 if( c ) *c = k;
 ADS_AUDIT( detail::primTree( g , r , p ) );
 return( p );
 }

template< Graph G >
std::vector< int > prim( const G & g , int r , Frontier how )
{
 ADS_PRE( ! g.directed() && g.hasNode( r ) );
 ADS_PRE( ( how == Frontier::LIST ) || ( how == Frontier::HEAP ) ||
          ( how == Frontier::FIBONACCI ) );
 using W = typename G::Weight;
 if( how == Frontier::LIST ) {
  ListPriorityQueue< int , W , false > Q;
  return( prim( g , r , Q ) );
  }
 if( how == Frontier::FIBONACCI ) {
  FibonacciHeapPriorityQueue< int , W > Q;
  return( prim( g , r , Q ) );
  }
 HeapPriorityQueue< int , W > Q( std::max( g.bound() , 1 ) );
 return( prim( g , r , Q ) );
 }

template< class W >
bool isMinimumSpanningForest( int n ,
                              const std::vector< WeightedEdge< W > > & A ,
                              const std::vector< WeightedEdge< W > > & T )
{
 auto same = []( const auto & a , const auto & b ) {
  return( ( a.w == b.w ) && ( ( ( a.u == b.u ) && ( a.v == b.v ) ) ||
                              ( ( a.u == b.v ) && ( a.v == b.u ) ) ) );
  };
 MFSet F( std::max( n , 1 ) ) , C( std::max( n , 1 ) );
 int comps = n;
 for( const auto & e : A )
  if( C.merge( e.u , e.v ) )
   --comps;
 std::vector< std::vector< std::pair< int , W > > > adj( n );
 for( const auto & t : T ) {
  if( std::none_of( A.begin() , A.end() ,
                    [ & ]( const auto & e ) { return( same( e , t ) ); } ) ||
      ! F.merge( t.u , t.v ) )
   return( false );                     // not an edge of A, or a cycle
  adj[ t.u ].emplace_back( t.v , t.w );
  adj[ t.v ].emplace_back( t.u , t.w );
  }
 if( int( T.size() ) != n - comps )
  return( false );
 for( const auto & e : A ) {            // the heaviest edge on the path
  if( e.u == e.v )
   continue;
  std::vector< int > from( n , -2 );
  std::vector< W > heaviest( n );
  std::vector< int > Q = { e.u };
  from[ e.u ] = -1;
  for( std::size_t i = 0 ; i < Q.size() ; ++i )
   for( auto [ y , w ] : adj[ Q[ i ] ] )
    if( from[ y ] == -2 ) {
     from[ y ] = Q[ i ];
     heaviest[ y ] = ( Q[ i ] == e.u ) || ( heaviest[ Q[ i ] ] < w )
                     ? w : heaviest[ Q[ i ] ];
     Q.push_back( y );
     }
  if( ( from[ e.v ] != -2 ) && ( e.w < heaviest[ e.v ] ) )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File MinimumSpanningTree.tpp --------------------*/
/*--------------------------------------------------------------------------*/
