/*--------------------------------------------------------------------------*/
/*------------------------- File Approximation.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Approximation [see Approximation.h], with their contracts.
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
/*---------------------------- THE CONTRACTS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// box gives each object a box, none filled beyond k
inline bool boxesWithin( const std::vector< long > & A , long k ,
                         const std::vector< int > & box )
{
 if( box.size() != A.size() ) return( false );
 std::vector< long > load( A.size() , 0 );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) {
  if( ( box[ i ] < 0 ) || ( std::size_t( box[ i ] ) >= A.size() ) )
   return( false );
  if( ( load[ box[ i ] ] += A[ i ] ) > k ) return( false );
  }
 return( true );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*---------------------------- BIN PACKING ---------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< int > firstFit( const std::vector< long > & A , long k ,
                                    bool decreasing )
{
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( long a ) {
  return( ( a >= 0 ) && ( a <= k ) ); } ) );
 const auto result = [ & ]( void ) -> std::vector< int > {
  std::vector< int > order( A.size() );
  std::iota( order.begin() , order.end() , 0 );
  if( decreasing )
   std::stable_sort( order.begin() , order.end() , [ & ]( int a , int b ) {
    return( A[ a ] > A[ b ] ); } );
  std::vector< int > box( A.size() );
  std::vector< long > free;
  for( int i : order ) {
   std::size_t b = 0;
   while( ( b < free.size() ) && ( free[ b ] < A[ i ] ) ) ++b;
   if( b == free.size() ) free.push_back( k );
   free[ b ] -= A[ i ];
   box[ i ] = int( b );
   }
  return( box );
  }();
 ADS_POST( detail::boxesWithin( A , k , result ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- DELTA TSP ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
T tourCost( const std::vector< std::vector< T > > & D , const Nodes & S )
{
 ADS_PRE( detail::square( D ) && detail::places( S , D.size() ) );
 T c = T( 0 );
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  c += D[ S[ i ] ][ S[ ( i + 1 ) % S.size() ] ];
 return( c );
 }

template< class T >
Nodes deltaTsp( const std::vector< std::vector< T > > & D )
{
 ADS_AUDIT( detail::triangular( D ) );
 ADS_PRE( detail::symmetricMatrix( D ) );
 const auto result = [ & ]( void ) -> Nodes {
  const int n = int( D.size() );
  std::vector< WeightedEdge< T > > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v ) E.push_back( { u , v , D[ u ][ v ] } );
  std::vector< Nodes > adj( n );
  for( const auto & e : kruskal( n , E ) ) {
   adj[ e.u ].push_back( e.v );
   adj[ e.v ].push_back( e.u );
   }
  Nodes S;
  std::vector< bool > seen( n , false );
  auto pre = [ & ]( auto & self , int u ) -> void {
   seen[ u ] = true;
   S.push_back( u );
   std::sort( adj[ u ].begin() , adj[ u ].end() );
   for( int v : adj[ u ] ) if( ! seen[ v ] ) self( self , v );
   };
  if( n > 0 ) pre( pre , 0 );
  return( S );
  }();
 ADS_POST( detail::permutation( result , D.size() ) );
 return( result );
 }

inline std::vector< std::vector< long > > tspGap( const Adjacency & G ,
                                                  long s )
{
 ADS_PRE( detail::symmetric( G ) && ( s >= 1 ) );
 const auto result = [ & ]( void ) -> std::vector< std::vector< long > > {
  const int n = int( G.size() );
  std::vector< std::vector< long > > D( n , std::vector< long >( n , 0 ) );
  for( int u = 0 ; u < n ; ++u )
   for( int v = 0 ; v < n ; ++v )
    if( u != v ) D[ u ][ v ] = G[ u ][ v ] ? 1 : s * n;
  return( D );
  }();
 ADS_POST( detail::symmetricMatrix( result ) &&
           ( result.size() == G.size() ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- SUBSET SUM AND ZAINO --------------------------*/
/*--------------------------------------------------------------------------*/

inline std::pair< long , Nodes > apSubsetSum( const std::vector< long > & A ,
                                              long k , int h )
{
 ADS_PRE( ( k >= 0 ) && ( h >= 0 ) && detail::nonnegative( A ) );
 const auto result = [ & ]( void ) -> std::pair< long , Nodes > {
  const int n = int( A.size() );
  Nodes order( n );                       // A[ order[ 0 ] ] >= ...
  std::iota( order.begin() , order.end() , 0 );
  std::stable_sort( order.begin() , order.end() , [ & ]( int a , int b ) {
   return( A[ a ] > A[ b ] ); } );
  long max = 0;
  Nodes maxSol;
  std::vector< bool > in( n , false );
  auto complete = [ & ]( void ) {         // the loop (2)
   long z = 0;
   for( int j = 0 ; j < n ; ++j ) if( in[ j ] ) z += A[ order[ j ] ];
   if( z > k ) return;
   std::vector< bool > S = in;
   for( int j = 0 ; j < n ; ++j )
    if( ! S[ j ] && ( z + A[ order[ j ] ] <= k ) ) {
     S[ j ] = true;
     z += A[ order[ j ] ];
     }
   if( z >= max ) {
    max = z;
    maxSol.clear();
    for( int j = 0 ; j < n ; ++j ) if( S[ j ] ) maxSol.push_back( order[ j ] );
    }
   };
  auto sets = [ & ]( auto & self , int from , int left ) -> void {
   if( left == 0 ) {
    complete();
    return;
    }
   for( int j = from ; j < n ; ++j ) {
    in[ j ] = true;
    self( self , j + 1 , left - 1 );
    in[ j ] = false;
    }
   };
  for( int m = 0 ; m <= std::min( h , n ) ; ++m ) sets( sets , 0 , m );  // (1)
  return( std::pair( max , maxSol ) );
  }();
 ADS_POST( ( result.first <= k ) &&
           detail::places( result.second , A.size() ) );
 ADS_POST( detail::sumOver( A , result.second ) == result.first );
 return( result );
 }

inline std::pair< long , Nodes >
knapsackApprox( const std::vector< long > & p , const std::vector< long > & v ,
                long C , double eps )
{
 ADS_PRE( ( p.size() == v.size() ) && ( eps > 0 ) && ( eps <= 1 ) );
 ADS_PRE( detail::nonnegative( p ) && detail::nonnegative( v ) &&
          ( C >= 0 ) );
 const auto result = [ & ]( void ) -> std::pair< long , Nodes > {
  Nodes I;                                // the objects that fit
  long pmax = 0;
  for( std::size_t i = 0 ; i < p.size() ; ++i )
   if( v[ i ] <= C ) {
    I.push_back( int( i ) );
    pmax = std::max( pmax , p[ i ] );
    }
  if( I.empty() ) return( std::pair( 0L , Nodes() ) );
  const double K = std::max( 1.0 , eps * double( pmax ) / double( I.size() ) );
  std::vector< long > q( I.size() );
  for( std::size_t i = 0 ; i < I.size() ; ++i )
   q[ i ] = long( double( p[ I[ i ] ] ) / K );
  const long Q = std::accumulate( q.begin() , q.end() , 0L );
  const long INF = std::numeric_limits< long >::max() / 2;
  // V[ i ][ s ]: the least volume of the first i objects of rounded
  // profit s
  std::vector< std::vector< long > > V( I.size() + 1 ,
                                       std::vector< long >( Q + 1 , INF ) );
  V[ 0 ][ 0 ] = 0;
  for( std::size_t i = 0 ; i < I.size() ; ++i )
   for( long s = 0 ; s <= Q ; ++s ) {
    V[ i + 1 ][ s ] = V[ i ][ s ];
    if( ( s >= q[ i ] ) && ( V[ i ][ s - q[ i ] ] < INF ) )
     V[ i + 1 ][ s ] = std::min( V[ i + 1 ][ s ] ,
                                 V[ i ][ s - q[ i ] ] + v[ I[ i ] ] );
    }
  long s = Q;
  while( V[ I.size() ][ s ] > C ) --s;
  Nodes S;
  long profit = 0;
  for( std::size_t i = I.size() ; i > 0 ; --i )
   if( V[ i ][ s ] != V[ i - 1 ][ s ] ) {
    S.push_back( I[ i - 1 ] );
    profit += p[ I[ i - 1 ] ];
    s -= q[ i - 1 ];
    }
  std::reverse( S.begin() , S.end() );
  return( std::pair( profit , S ) );
  }();
 ADS_POST( detail::places( result.second , p.size() ) );
 ADS_POST( detail::sumOver( p , result.second ) == result.first );
 ADS_POST( detail::sumOver( v , result.second ) <= C );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- VERTEX COVER --------------------------------*/
/*--------------------------------------------------------------------------*/

inline Nodes vertexCoverApprox( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> Nodes {
  const int n = int( G.size() );
  std::vector< bool > in( n , false );
  Nodes S;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( G[ u ][ v ] && ! in[ u ] && ! in[ v ] ) {
     in[ u ] = in[ v ] = true;
     S.push_back( u );
     S.push_back( v );
     }
  return( S );
  }();
 ADS_POST( detail::covers( G , result ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File Approximation.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
