/*--------------------------------------------------------------------------*/
/*---------------------- File MinimumSpanningTree.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The minimum spanning trees of MinimumSpanningTree.h [see
 * MinimumSpanningTree.h], with their contracts.
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

template< class W >
std::vector< WeightedEdge< W > > kruskal( int n ,
                                          std::vector< WeightedEdge< W > > A ,
                                          long k )
{
 ADS_PRE( n >= 0 );
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( const auto & e ) {
  return( ( e.u >= 0 ) && ( e.u < n ) && ( e.v >= 0 ) && ( e.v < n ) &&
          ( ( k <= 0 ) || ( ( e.w >= W( 0 ) ) && ( e.w < W( k ) ) ) ) ); } ) );
 const auto all = A;
 if( k > 0 ) {                                  // Counting Sort, stable
  std::vector< std::size_t > c( k + 1 , 0 );
  for( const auto & e : A ) ++c[ std::size_t( e.w ) + 1 ];
  for( long i = 0 ; i < k ; ++i ) c[ i + 1 ] += c[ i ];
  std::vector< WeightedEdge< W > > B( A.size() );
  for( const auto & e : A ) B[ c[ std::size_t( e.w ) ]++ ] = e;
  A = std::move( B );
  }
 else
  std::stable_sort( A.begin() , A.end() , []( const auto & a ,
                    const auto & b ) { return( a.w < b.w ); } );
 std::vector< WeightedEdge< W > > T;
 MFSet M( n );
 for( std::size_t i = 0 ; ( i < A.size() ) && ( int( T.size() ) < n - 1 ) ;
      ++i )
  if( M.merge( A[ i ].u , A[ i ].v ) )         // the ends were apart
   T.push_back( A[ i ] );
 ADS_AUDIT( isMinimumSpanningForest( n , all , T ) );
 return( T );
 }

template< Graph G >
auto kruskal( const G & g , long k )
{
 return( kruskal( g.bound() , edges( g ) , k ) );
 }

template< Graph G >
std::vector< int > prim( const G & g , int r , Frontier how )
{
 ADS_PRE( ! g.directed() && g.hasNode( r ) );
 ADS_PRE( ( how == Frontier::LIST ) || ( how == Frontier::HEAP ) ||
          ( how == Frontier::FIBONACCI ) );
 using W = typename G::Weight;
 const int n = g.bound();
 std::vector< int > p( n , NIL );
 std::vector< bool > in( n , false );                 // in the tree
 std::vector< W > key( n , infinity< W >() );
 using LQ = ListPriorityQueue< int , W , false >;
 using HQ = HeapPriorityQueue< int , W >;
 using FQ = FibonacciHeapPriorityQueue< int , W >;
 auto run = [ & ]( auto & Q , auto & pos ) {
  for( int u : g.V() )
   pos[ u ] = Q.insert( u , u == r ? W( 0 ) : infinity< W >() );
  key[ r ] = W( 0 );
  while( ! Q.isEmpty() ) {
   const int u = Q.deleteMin();
   in[ u ] = true;
   if( key[ u ] == infinity< W >() )               // another component
    continue;
   g.forEachEdge( u , [ & ]( int v , const W & w ) {
    if( ! in[ v ] && ( w < key[ v ] ) ) {
     key[ v ] = w;
     Q.decrease( pos[ v ] , w );
     p[ v ] = u;
     }
    } );
   }
  };
 if( how == Frontier::LIST ) {
  LQ Q;
  std::vector< typename LQ::Handle > pos( n );
  run( Q , pos );
  }
 else if( how == Frontier::FIBONACCI ) {
  FQ Q;
  std::vector< typename FQ::Handle > pos( n );
  run( Q , pos );
  }
 else {
  HQ Q( n );
  std::vector< typename HQ::Handle > pos( n );
  run( Q , pos );
  }
 ADS_AUDIT( detail::primTree( g , r , p ) );
 return( p );
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
