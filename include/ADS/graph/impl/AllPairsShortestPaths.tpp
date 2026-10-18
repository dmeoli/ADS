/*--------------------------------------------------------------------------*/
/*--------------------- File AllPairsShortestPaths.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The shortest paths between all the pairs of AllPairsShortestPaths.h
 * [see AllPairsShortestPaths.h], with their contracts.
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

/// no d[ u ][ k ] + d[ k ][ v ] is below d[ u ][ v ], on finite entries
template< class D >
bool triangleClosed( const AllPairs< D > & R )
{
 const D INF = infinity< D >();
 const int n = int( R.d.size() );
 for( int k = 0 ; k < n ; ++k )
  for( int u = 0 ; u < n ; ++u )
   if( R.d[ u ][ k ] != INF )
    for( int v = 0 ; v < n ; ++v )
     if( ( R.d[ k ][ v ] != INF ) &&
         ( R.d[ u ][ k ] + R.d[ k ][ v ] < R.d[ u ][ v ] ) )
      return( false );
 return( true );
 }

/// the distances of floydWarshall() without negative cycles: 0 on the
/// diagonal, at most the cost of each edge, closed under the triangle
/// inequality, and d[ u ][ v ] = d[ u ][ p ] + w( p , v ) with
/// p = p[ u ][ v ]
template< Graph G >
bool allPairsOk( const G & g , const AllPairs< typename G::Weight > & R )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 const int n = g.bound();
 std::vector< std::vector< D > > W( n , std::vector< D >( n , INF ) );
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const D & w ) { W[ u ][ v ] = w; } );
 for( int u : g.V() ) {
  if( R.d[ u ][ u ] != D( 0 ) )
   return( false );
  for( int v : g.V() ) {
   if( ( W[ u ][ v ] != INF ) && ( W[ u ][ v ] < R.d[ u ][ v ] ) )
    return( false );
   if( ( u == v ) || ( R.d[ u ][ v ] == INF ) )
    continue;
   const int p = R.p[ u ][ v ];
   if( ( p < 0 ) || ( p >= n ) || ( W[ p ][ v ] == INF ) ||
       ( R.d[ u ][ p ] == INF ) || ( R.d[ u ][ p ] + W[ p ][ v ] !=
                                     R.d[ u ][ v ] ) )
    return( false );
   }
  }
 return( triangleClosed( R ) );
 }

}  // end( namespace detail )

template< Graph G >
AllPairs< typename G::Weight > floydWarshall( const G & g )
{
 using D = typename G::Weight;
 const D INF = infinity< D >();
 const int n = g.bound();
 AllPairs< D > R;
 R.d.assign( n , std::vector< D >( n , INF ) );
 R.p.assign( n , std::vector< int >( n , NIL ) );
 for( int u : g.V() ) {
  R.d[ u ][ u ] = D( 0 );
  g.forEachEdge( u , [ & ]( int v , const D & w ) {
   R.d[ u ][ v ] = w;
   R.p[ u ][ v ] = u;
   } );
  }
 for( int k = 0 ; k < n ; ++k )
  for( int u = 0 ; u < n ; ++u ) {
   if( R.d[ u ][ k ] == INF )
    continue;
   for( int v = 0 ; v < n ; ++v )
    if( ( R.d[ k ][ v ] != INF ) &&
        ( R.d[ u ][ k ] + R.d[ k ][ v ] < R.d[ u ][ v ] ) ) {
     R.d[ u ][ v ] = R.d[ u ][ k ] + R.d[ k ][ v ];
     R.p[ u ][ v ] = R.p[ k ][ v ];
     }
   }
 for( int u = 0 ; u < n ; ++u )
  if( R.d[ u ][ u ] < D( 0 ) )
   R.negativeCycle = true;
 ADS_AUDIT( R.negativeCycle || detail::allPairsOk( g , R ) );
 return( R );
 }

template< class D >
void addEdge( AllPairs< D > & R , int a , int b , const D & w )
{
 const D INF = infinity< D >();
 const int n = int( R.d.size() );
 ADS_PRE( ( a >= 0 ) && ( a < n ) && ( b >= 0 ) && ( b < n ) && ( a != b ) );
 ADS_PRE( ( R.d[ b ][ a ] == INF ) || !( w + R.d[ b ][ a ] < D( 0 ) ) );
 if( w >= R.d[ a ][ b ] )
  return;
 const auto d = R.d;
 const auto p = R.p;
 for( int u = 0 ; u < n ; ++u ) {
  if( ( u != a ) && ( d[ u ][ a ] == INF ) )
   continue;
  const D du = u == a ? D( 0 ) : d[ u ][ a ];
  for( int v = 0 ; v < n ; ++v ) {
   if( ( v != b ) && ( d[ b ][ v ] == INF ) )
    continue;
   const D dv = v == b ? D( 0 ) : d[ b ][ v ];
   if( du + w + dv < R.d[ u ][ v ] ) {
    R.d[ u ][ v ] = du + w + dv;
    R.p[ u ][ v ] = v == b ? a : p[ b ][ v ];
    }
   }
  }
 ADS_POST( !( w < R.d[ a ][ b ] ) );
 ADS_AUDIT( detail::triangleClosed( R ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File AllPairsShortestPaths.tpp -------------------*/
/*--------------------------------------------------------------------------*/
