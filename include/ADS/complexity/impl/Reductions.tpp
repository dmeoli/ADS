/*--------------------------------------------------------------------------*/
/*-------------------------- File Reductions.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Reductions [see Reductions.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONTRACTS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// H has the edges between distinct nodes that G does not have
inline bool complementOf( const Adjacency & G , const Adjacency & H )
{
 if( ! square( H ) || ( H.size() != G.size() ) ) return( false );
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = 0 ; v < G.size() ; ++v )
   if( ( u != v ) && ( H[ u ][ v ] == G[ u ][ v ] ) ) return( false );
 return( true );
 }

}  // end( namespace detail )

inline int tileVar( int h , int i , int j , int n )
{
 ADS_PRE( ( n >= 1 ) && ( h >= 0 ) );
 ADS_PRE( ( i >= 0 ) && ( i < n ) && ( j >= 0 ) && ( j < n ) );
 return( ( h * n + i ) * n + j );
 }

inline std::pair< Adjacency , int >
dominoToClique( const std::vector< Tile > & D , int n )
{
 ADS_PRE( ( n >= 1 ) && ! D.empty() );
 const auto result = [ & ]( void ) -> std::pair< Adjacency , int > {
  const int m = int( D.size() ) , N = m * n * n;
  Adjacency G( N , std::vector< bool >( N , false ) );
  auto alive = [ & ]( int h , int i , int j ) {
   return( ( i != 0 ) || ( j != 0 ) || ( h == 0 ) ); };
  for( int h = 0 ; h < m ; ++h )
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j )
     for( int g = 0 ; g < m ; ++g )
      for( int a = 0 ; a < n ; ++a )
       for( int b = 0 ; b < n ; ++b ) {
        if( ! alive( h , i , j ) || ! alive( g , a , b ) ) continue;
        if( ( a == i ) && ( b == j ) ) continue;
        const int di = a - i , dj = b - j;
        bool edge;
        if( std::abs( di ) + std::abs( dj ) != 1 )
         edge = true;                                         // (3)
        else if( di == 1 ) edge = D[ h ].right == D[ g ].left;   // (4)
        else if( di == -1 ) edge = D[ g ].right == D[ h ].left;
        else if( dj == 1 ) edge = D[ h ].up == D[ g ].down;
        else edge = D[ g ].up == D[ h ].down;
        G[ tileVar( h , i , j , n ) ][ tileVar( g , a , b , n ) ] = edge;
        }
  return( std::pair( G , n * n ) );
  }();
 ADS_POST( detail::symmetric( result.first ) );
 ADS_POST( result.first.size() == D.size() * n * n );
 ADS_POST( result.second == n * n );
 return( result );
 }

inline Cnf dominoToSat( const std::vector< Tile > & D , int n )
{
 ADS_PRE( ( n >= 1 ) && ! D.empty() );
 const auto result = [ & ]( void ) -> Cnf {
  const int m = int( D.size() );
  auto u = [ & ]( int h , int i , int j ) {
   return( tileVar( h , i , j , n ) + 1 ); };
  Cnf F = { { u( 0 , 0 , 0 ) } };                                 // (1)
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j ) {
    Clause c;                                                     // (2)
    for( int h = 0 ; h < m ; ++h ) c.push_back( u( h , i , j ) );
    F.push_back( c );
    }
  for( int i = 0 ; i < n ; ++i )                                  // (3)
   for( int j = 0 ; j < n ; ++j )
    for( int h = 0 ; h < m ; ++h )
     for( int g = h + 1 ; g < m ; ++g )
      F.push_back( { -u( h , i , j ) , -u( g , i , j ) } );
  for( int i = 0 ; i + 1 < n ; ++i )                              // (4)
   for( int j = 0 ; j < n ; ++j )
    for( int h = 0 ; h < m ; ++h )
     for( int g = 0 ; g < m ; ++g )
      if( D[ h ].right != D[ g ].left )
       F.push_back( { -u( h , i , j ) , -u( g , i + 1 , j ) } );
  for( int i = 0 ; i < n ; ++i )                                  // (5)
   for( int j = 0 ; j + 1 < n ; ++j )
    for( int h = 0 ; h < m ; ++h )
     for( int g = 0 ; g < m ; ++g )
      if( D[ h ].up != D[ g ].down )
       F.push_back( { -u( h , i , j ) , -u( g , i , j + 1 ) } );
  return( F );
  }();
 ADS_POST( detail::literals( result , D.size() * n * n ) );
 return( result );
 }

inline std::pair< std::vector< std::vector< long > > , std::vector< long > >
dominoToLp01( const std::vector< Tile > & D , int n )
{
 ADS_PRE( ( n >= 1 ) && ! D.empty() );
 using Lp = std::pair< std::vector< std::vector< long > > ,
                     std::vector< long > >;
 const auto result = [ & ]( void ) -> Lp {
  const int m = int( D.size() ) , N = m * n * n;
  std::vector< std::vector< long > > A;
  std::vector< long > b;
  auto row = [ & ]( const std::vector< int > & vars , long rhs , bool eq ) {
   std::vector< long > r( N , 0 );
   for( int v : vars ) r[ v ] += 1;
   A.push_back( r );
   b.push_back( rhs );
   if( eq ) {                                      // and - r <= - rhs
    for( auto & x : r ) x = -x;
    A.push_back( r );
    b.push_back( -rhs );
    }
   };
  row( { tileVar( 0 , 0 , 0 , n ) } , 1 , true );                  // (1)
  for( int i = 0 ; i < n ; ++i )                                  // (2)
   for( int j = 0 ; j < n ; ++j ) {
    std::vector< int > v;
    for( int h = 0 ; h < m ; ++h ) v.push_back( tileVar( h , i , j , n ) );
    row( v , 1 , true );
    }
  for( int h = 0 ; h < m ; ++h )                                  // (3), (4)
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j ) {
     std::vector< int > right = { tileVar( h , i , j , n ) } ,
                        up = { tileVar( h , i , j , n ) };
     for( int g = 0 ; g < m ; ++g ) {
      if( i + 1 < n && D[ h ].right != D[ g ].left )
       right.push_back( tileVar( g , i + 1 , j , n ) );
      if( j + 1 < n && D[ h ].up != D[ g ].down )
       up.push_back( tileVar( g , i , j + 1 , n ) );
      }
     if( right.size() > 1 ) row( right , 1 , false );
     if( up.size() > 1 ) row( up , 1 , false );
     }
  return( std::pair( A , b ) );
  }();
 ADS_POST( result.first.size() == result.second.size() );
 ADS_POST( detail::columns( result.first , int( D.size() ) * n * n ) );
 return( result );
 }

inline Adjacency complement( const Adjacency & G )
{
 ADS_PRE( detail::square( G ) );
 const auto result = [ & ]( void ) -> Adjacency {
  Adjacency C = G;
  for( std::size_t u = 0 ; u < G.size() ; ++u )
   for( std::size_t v = 0 ; v < G.size() ; ++v )
    C[ u ][ v ] = ( u != v ) && ! G[ u ][ v ];
  return( C );
  }();
 ADS_POST( detail::complementOf( G , result ) );
 return( result );
 }

inline std::vector< std::vector< long > > hamiltonToTsp( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::vector< std::vector< long > > {
  const std::size_t n = G.size();
  std::vector< std::vector< long > > Dist( n , std::vector< long >( n , 0 ) );
  for( std::size_t i = 0 ; i < n ; ++i )
   for( std::size_t j = 0 ; j < n ; ++j )
    if( i != j ) Dist[ i ][ j ] = G[ i ][ j ] ? 1 : 2;
  return( Dist );
  }();
 ADS_POST( detail::square( result ) && ( result.size() == G.size() ) );
 return( result );
 }

inline long halfSum( const std::vector< long > & A )
{
 ADS_PRE( detail::nonnegative( A ) );
 const auto result = [ & ]( void ) -> long {
  const long s = std::accumulate( A.begin() , A.end() , 0L );
  return( s % 2 ? -1 : s / 2 );
  }();
 ADS_POST( ( result == -1 ) ||
           ( 2 * result == std::accumulate( A.begin() , A.end() , 0L ) ) );
 return( result );
 }

inline Adjacency cycleToPath( const Adjacency & G )
{
 ADS_PRE( detail::square( G ) );
 const auto result = [ & ]( void ) -> Adjacency {
  const std::size_t n = G.size();
  Adjacency H( n + 1 , std::vector< bool >( n + 1 , false ) );
  for( std::size_t u = 0 ; u < n ; ++u )
   for( std::size_t v = 0 ; v < n ; ++v )
    if( G[ u ][ v ] ) {
     if( u == 0 ) H[ n ][ v ] = true;              // out of 0: out of n
     else H[ u ][ v ] = true;                      // into 0 stays into 0
     }
  return( H );
  }();
 ADS_POST( detail::square( result ) && ( result.size() == G.size() + 1 ) );
 return( result );
 }

inline Adjacency cycleToChain( const Adjacency & G )
{
 ADS_PRE( detail::square( G ) );
 const auto result = [ & ]( void ) -> Adjacency {
  const Adjacency P = cycleToPath( G );
  const int N = int( P.size() ) , M = 3 * N + 2;
  Adjacency H( M , std::vector< bool >( M , false ) );
  auto link = [ & ]( int a , int b ) { H[ a ][ b ] = H[ b ][ a ] = true; };
  for( int v = 0 ; v < N ; ++v ) {
   link( 3 * v , 3 * v + 1 );
   link( 3 * v + 1 , 3 * v + 2 );
   }
  for( int u = 0 ; u < N ; ++u )
   for( int v = 0 ; v < N ; ++v )
    if( P[ u ][ v ] ) link( 3 * u + 2 , 3 * v );
  link( 3 * N , 3 * ( N - 1 ) );        // before the start, the node N - 1
  link( 3 * N + 1 , 2 );                // after the end, the node 0
  return( H );
  }();
 ADS_POST( detail::symmetric( result ) );
 ADS_POST( result.size() == 3 * ( G.size() + 1 ) + 2 );
 return( result );
 }

namespace detail {

/// floor( sqrt( LONG_MAX ) ), by bisection: the largest x with x^2 in a
/// long, 3037000499 where long has 64 bits and 46340 where it has 32
constexpr long longRoot( void )
{
 long lo = 0 , hi = std::numeric_limits< long >::max();
 while( lo < hi ) {
  const long m = lo + ( hi - lo ) / 2 + 1;
  if( m <= std::numeric_limits< long >::max() / m ) lo = m; else hi = m - 1;
  }
 return( lo );
 }

inline constexpr long kLongRoot = longRoot();   // computed at compile time

}  // end( namespace detail )

inline std::vector< Point > sortToHull( const std::vector< long > & a )
{
 ADS_PRE( std::all_of( a.begin() , a.end() , []( long x ) {
  return( ( x >= -detail::kLongRoot ) && ( x <= detail::kLongRoot ) ); } ) );
 const auto result = [ & ]( void ) -> std::vector< Point > {
  std::vector< Point > P;
  for( long x : a ) P.push_back( { x , x * x } );
  return( P );
  }();
 ADS_POST( result.size() == a.size() );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Reductions.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
