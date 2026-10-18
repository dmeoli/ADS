/*--------------------------------------------------------------------------*/
/*--------------------------- File Problems.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Problems [see Problems.h], with their contracts.
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

/// M is a square matrix
template< class M >
bool square( const M & G )
{
 for( const auto & row : G )
  if( row.size() != G.size() ) return( false );
 return( true );
 }

/// G is square and symmetric, i.e., an unoriented graph
inline bool symmetric( const Adjacency & G )
{
 if( ! square( G ) ) return( false );
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = u + 1 ; v < G.size() ; ++v )
   if( G[ u ][ v ] != G[ v ][ u ] ) return( false );
 return( true );
 }

/// S has distinct places of [ 0 , n )
inline bool places( const Nodes & S , std::size_t n )
{
 std::vector< bool > seen( n , false );
 for( int v : S ) {
  if( ( v < 0 ) || ( std::size_t( v ) >= n ) || seen[ v ] ) return( false );
  seen[ v ] = true;
  }
 return( true );
 }

/// P is a permutation of [ 0 , n )
inline bool permutation( const Nodes & P , std::size_t n )
{
 return( ( P.size() == n ) && places( P , n ) );
 }

/// consecutive nodes of P are adjacent in G, and the last to the first if
/// closed
inline bool walk( const Adjacency & G , const Nodes & P , bool closed )
{
 for( std::size_t i = 0 ; i + 1 < P.size() ; ++i )
  if( ! G[ P[ i ] ][ P[ i + 1 ] ] ) return( false );
 return( ! closed || P.empty() || G[ P.back() ][ P.front() ] );
 }

/// P goes through all the nodes of G once, along its edges
inline bool spanning( const Adjacency & G , const Nodes & P , bool closed )
{
 return( permutation( P , G.size() ) && walk( G , P , closed ) );
 }

/// no two nodes of S are adjacent
inline bool independent( const Adjacency & G , const Nodes & S )
{
 for( int u : S )
  for( int v : S )
   if( G[ u ][ v ] ) return( false );
 return( places( S , G.size() ) );
 }

/// each edge of G has an end in S
inline bool covers( const Adjacency & G , const Nodes & S )
{
 if( ! places( S , G.size() ) ) return( false );
 std::vector< bool > in( G.size() , false );
 for( int u : S ) in[ u ] = true;
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = u + 1 ; v < G.size() ; ++v )
   if( G[ u ][ v ] && ! in[ u ] && ! in[ v ] ) return( false );
 return( true );
 }

/// the edges between S and the other nodes
inline int cut( const Adjacency & G , const Nodes & S )
{
 std::vector< bool > in( G.size() , false );
 for( int u : S ) in[ u ] = true;
 int c = 0;
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = u + 1 ; v < G.size() ; ++v )
   c += G[ u ][ v ] && ( in[ u ] != in[ v ] );
 return( c );
 }

/// col gives each node a color of [ 0 , k ), adjacent nodes apart
inline bool proper( const Adjacency & G , const std::vector< int > & col ,
                    int k )
{
 if( col.size() != G.size() ) return( false );
 for( std::size_t u = 0 ; u < G.size() ; ++u ) {
  if( ( col[ u ] < 0 ) || ( col[ u ] >= k ) ) return( false );
  for( std::size_t v = 0 ; v < G.size() ; ++v )
   if( G[ u ][ v ] && ( u != v ) && ( col[ u ] == col[ v ] ) )
    return( false );
  }
 return( true );
 }

/// the literals of F are +-( v + 1 ) with 0 <= v < n
inline bool literals( const Cnf & F , std::size_t n )
{
 for( const Clause & c : F )
  for( int l : c )
   if( ( l == 0 ) || ( std::size_t( std::abs( l ) ) > n ) ) return( false );
 return( true );
 }

/// no number of A is negative
inline bool nonnegative( const std::vector< long > & A )
{
 return( std::none_of( A.begin() , A.end() , []( long a ) {
  return( a < 0 ); } ) );
 }

/// the sum of A over the places S
inline long sumOver( const std::vector< long > & A , const Nodes & S )
{
 long s = 0;
 for( int i : S ) s += A[ i ];
 return( s );
 }

/// T is an acyclic oriented graph, by removing the sources, O( n^2 )
inline bool acyclic( const Adjacency & G )
{
 const std::size_t n = G.size();
 std::vector< int > in( n , 0 );
 for( std::size_t u = 0 ; u < n ; ++u )
  for( std::size_t v = 0 ; v < n ; ++v ) in[ v ] += G[ u ][ v ];
 std::vector< std::size_t > S;
 for( std::size_t v = 0 ; v < n ; ++v ) if( ! in[ v ] ) S.push_back( v );
 std::size_t seen = 0;
 while( ! S.empty() ) {
  const std::size_t u = S.back();
  S.pop_back();
  ++seen;
  for( std::size_t v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] && ( --in[ v ] == 0 ) ) S.push_back( v );
  }
 return( seen == n );
 }

/// E is a tree of G with at most k edges at each node
inline bool boundedTree( const Adjacency & G ,
                         const std::vector< std::pair< int , int > > & E ,
                         int k )
{
 const std::size_t n = G.size();
 std::vector< int > deg( n , 0 ) , root( n );
 for( std::size_t v = 0 ; v < n ; ++v ) root[ v ] = int( v );
 auto find = [ & ]( int v ) {
  while( root[ v ] != v ) v = root[ v ];
  return( v ); };
 for( auto [ u , v ] : E ) {
  if( ( u < 0 ) || ( v < 0 ) || ( std::size_t( u ) >= n ) ||
      ( std::size_t( v ) >= n ) || ! G[ u ][ v ] ||
      ( find( u ) == find( v ) ) )
   return( false );
  root[ find( u ) ] = find( v );
  if( ( ++deg[ u ] > k ) || ( ++deg[ v ] > k ) ) return( false );
  }
 return( true );
 }

/// the 0/1 vector x satisfies A x <= b
inline bool feasible01( const std::vector< std::vector< long > > & A ,
                        const std::vector< long > & b ,
                        const std::vector< int > & x )
{
 for( std::size_t r = 0 ; r < A.size() ; ++r ) {
  long s = 0;
  for( std::size_t j = 0 ; j < x.size() ; ++j ) {
   if( ( x[ j ] != 0 ) && ( x[ j ] != 1 ) ) return( false );
   s += A[ r ][ j ] * x[ j ];
   }
  if( s > b[ r ] ) return( false );
  }
 return( true );
 }

/// a gives each program a processor of [ 0 , m ), each loaded at most d
inline bool withinLoads( const std::vector< long > & t , int m , long d ,
                         const std::vector< int > & a )
{
 if( a.size() != t.size() ) return( false );
 std::vector< long > load( m , 0 );
 for( std::size_t i = 0 ; i < t.size() ; ++i ) {
  if( ( a[ i ] < 0 ) || ( a[ i ] >= m ) ) return( false );
  load[ a[ i ] ] += t[ i ];
  }
 return( std::all_of( load.begin() , load.end() , [ & ]( long l ) {
  return( l <= d ); } ) );
 }

/// the sets of Y in S partition [ 0 , m )
inline bool exact( const std::vector< std::array< int , 3 > > & Y , int m ,
                   const Nodes & S )
{
 if( ! places( S , Y.size() ) ) return( false );
 std::vector< int > c( std::max( m , 0 ) , 0 );
 for( int j : S )
  for( int e : Y[ j ] ) ++c[ e ];
 return( std::all_of( c.begin() , c.end() , []( int x ) {
  return( x == 1 ); } ) );
 }

/// the elements of the triple y are distinct, in [ 0 , m )
inline bool triple( const std::array< int , 3 > & y , int m )
{
 return( places( Nodes( y.begin() , y.end() ) ,
                 std::size_t( std::max( m , 0 ) ) ) );
 }

/// the coordinates of x are in [ 0 , q )
inline bool coordinates( const std::array< int , 3 > & x , int q )
{
 return( std::all_of( x.begin() , x.end() , [ & ]( int c ) {
  return( ( c >= 0 ) && ( c < q ) ); } ) );
 }

/// the matrix D is square and symmetric
template< class T >
bool symmetricMatrix( const std::vector< std::vector< T > > & D )
{
 if( ! square( D ) ) return( false );
 for( std::size_t u = 0 ; u < D.size() ; ++u )
  for( std::size_t v = u + 1 ; v < D.size() ; ++v )
   if( D[ u ][ v ] != D[ v ][ u ] ) return( false );
 return( true );
 }

/// the distances D satisfy D[ i ][ j ] <= D[ i ][ k ] + D[ k ][ j ], up to
/// a relative tolerance for floating point, O( n^3 )
template< class T >
bool triangular( const std::vector< std::vector< T > > & D )
{
 const std::size_t n = D.size();
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j )
   for( std::size_t k = 0 ; k < n ; ++k )
    if( double( D[ i ][ j ] ) > double( D[ i ][ k ] + D[ k ][ j ] ) *
                                ( 1 + 1e-12 ) + 1e-12 )
     return( false );
 return( true );
 }

/// the rows of A have n columns
inline bool columns( const std::vector< std::vector< long > > & A , int n )
{
 return( std::all_of( A.begin() , A.end() , [ & ]( const auto & a ) {
  return( int( a.size() ) == n ); } ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------ GRAPHS ------------------------------------*/
/*--------------------------------------------------------------------------*/

inline bool cliqueCertificate( const Adjacency & G , const Nodes & S , int k )
{
 ADS_PRE( detail::square( G ) );
 if( int( S.size() ) < k ) return( false );
 for( std::size_t a = 0 ; a < S.size() ; ++a )
  for( std::size_t b = a + 1 ; b < S.size() ; ++b )
   if( ( S[ a ] == S[ b ] ) || ! G[ S[ a ] ][ S[ b ] ] ) return( false );
 return( true );
 }

namespace detail {

template< class Ok , class Done >
bool chooseK( int n , int k , Ok && ok , Done && done )
{
 Nodes S;
 auto rec = [ & ]( auto & self , int i ) -> bool {
  if( int( S.size() ) == k ) return( done( S ) );
  if( int( S.size() ) + ( n - i ) < k ) return( false );   // pruned
  for( int v = i ; v < n ; ++v )
   if( ok( S , v ) ) {
    S.push_back( v );
    if( self( self , v + 1 ) ) return( true );
    S.pop_back();
    }
  return( false );
  };
 return( rec( rec , 0 ) );
 }

}  // end( namespace detail )

inline std::optional< Nodes > clique( const Adjacency & G , int k )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  Nodes R;
  if( k <= 0 ) return( Nodes() );
  if( detail::chooseK( int( G.size() ) , k , [ & ]( const Nodes & S , int v ) {
        for( int u : S ) if( ! G[ u ][ v ] ) return( false );
        return( true ); } ,
        [ & ]( const Nodes & S ) { R = S; return( true ); } ) )
   return( R );
  return( std::nullopt );
  }();
 ADS_POST( ! result || cliqueCertificate( G , *result , k ) );
 return( result );
 }

inline std::optional< Nodes > independentSet( const Adjacency & G , int k )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  Nodes R;
  if( k <= 0 ) return( Nodes() );
  if( detail::chooseK( int( G.size() ) , k , [ & ]( const Nodes & S , int v ) {
        for( int u : S ) if( G[ u ][ v ] ) return( false );
        return( true ); } ,
        [ & ]( const Nodes & S ) { R = S; return( true ); } ) )
   return( R );
  return( std::nullopt );
  }();
 ADS_POST( ! result || ( int( result->size() ) == k ) );
 ADS_POST( ! result || detail::independent( G , *result ) );
 return( result );
 }

inline std::optional< Nodes > vertexCover( const Adjacency & G , int k )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  const int n = int( G.size() );
  for( int s = 0 ; s <= std::min( k , n ) ; ++s ) {
   Nodes R;
   if( detail::chooseK( n , s , []( const Nodes & , int ) { return( true ); } ,
        [ & ]( const Nodes & S ) {
         std::vector< bool > in( n , false );
         for( int u : S ) in[ u ] = true;
         for( int u = 0 ; u < n ; ++u )
          for( int v = u + 1 ; v < n ; ++v )
           if( G[ u ][ v ] && ! in[ u ] && ! in[ v ] ) return( false );
         R = S;
         return( true ); } ) )
    return( R );
   }
  return( std::nullopt );
  }();
 ADS_POST( ! result || ( int( result->size() ) <= k ) );
 ADS_POST( ! result || detail::covers( G , *result ) );
 return( result );
 }

inline std::optional< std::vector< int > > coloring( const Adjacency & G ,
                                                     int k )
{
 ADS_PRE( detail::symmetric( G ) );
 ADS_PRE( k >= 0 );
 const auto result = [ & ]( void ) -> std::optional< std::vector< int > > {
  const int n = int( G.size() );
  std::vector< int > c( n , -1 );
  auto rec = [ & ]( auto & self , int u ) -> bool {
   if( u == n ) return( true );
   int used = 0;                                 // the colors seen so far
   for( int v = 0 ; v < u ; ++v ) used = std::max( used , c[ v ] + 1 );
   for( int x = 0 ; x < std::min( k , used + 1 ) ; ++x ) {   // no symmetry
    bool ok = true;
    for( int v = 0 ; v < u && ok ; ++v ) ok = ! ( G[ u ][ v ] && c[ v ] == x );
    if( ok ) {
     c[ u ] = x;
     if( self( self , u + 1 ) ) return( true );
     }
    }
   c[ u ] = -1;
   return( false );
   };
  if( rec( rec , 0 ) ) return( c );
  return( std::nullopt );
  }();
 ADS_POST( ! result || detail::proper( G , *result , k ) );
 return( result );
 }

inline std::optional< Nodes > bisection( const Adjacency & G , int k )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  const int n = int( G.size() );
  if( n % 2 ) return( std::nullopt );
  Nodes R;
  if( detail::chooseK( n , n / 2 , [ & ]( const Nodes & S , int v ) {
        return( ! S.empty() || v == 0 ); } , [ & ]( const Nodes & S ) {
         std::vector< bool > in( n , false );
         for( int u : S ) in[ u ] = true;
         int cut = 0;
         for( int u = 0 ; u < n ; ++u )
          for( int v = u + 1 ; v < n ; ++v )
           cut += G[ u ][ v ] && in[ u ] != in[ v ];
         if( cut > k ) return( false );
         R = S;
         return( true ); } ) )
   return( R );
  return( std::nullopt );
  }();
 ADS_POST( ! result || ( 2 * result->size() == G.size() ) );
 ADS_POST( ! result || detail::places( *result , G.size() ) );
 ADS_POST( ! result || ( detail::cut( G , *result ) <= k ) );
 return( result );
 }

template< class T >
bool tspCertificate( const std::vector< std::vector< T > > & D , T k ,
                     const Nodes & P )
{
 ADS_PRE( detail::square( D ) );
 const std::size_t n = P.size();
 if( n != D.size() ) return( false );
 std::vector< bool > seen( n , false );
 for( int c : P ) {
  if( ( c < 0 ) || ( c >= int( n ) ) || seen[ c ] ) return( false );
  seen[ c ] = true;
  }
 T t = D[ P[ n - 1 ] ][ P[ 0 ] ];
 for( std::size_t i = 1 ; i < n ; ++i ) t += D[ P[ i - 1 ] ][ P[ i ] ];
 return( t <= k );
 }

template< class T >
std::optional< Nodes > tsp( const std::vector< std::vector< T > > & D , T k )
{
 ADS_PRE( detail::square( D ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  const int n = int( D.size() );
  if( n == 0 ) return( Nodes() );
  Nodes S = { 0 };
  std::vector< bool > used( n , false );
  used[ 0 ] = true;
  auto rec = [ & ]( auto & self , T len ) -> bool {
   if( len > k ) return( false );                 // pruned
   if( int( S.size() ) == n ) return( len + D[ S.back() ][ 0 ] <= k );
   for( int c = 1 ; c < n ; ++c )
    if( ! used[ c ] ) {
     used[ c ] = true;
     S.push_back( c );
     if( self( self , len + D[ S[ S.size() - 2 ] ][ c ] ) ) return( true );
     S.pop_back();
     used[ c ] = false;
     }
   return( false );
   };
  if( rec( rec , T( 0 ) ) ) return( S );
  return( std::nullopt );
  }();
 ADS_POST( ! result || tspCertificate( D , k , *result ) );
 return( result );
 }

namespace detail {

inline std::optional< Nodes > hamilton( const Adjacency & G ,
                                        const Nodes & starts , bool cycle ,
                                        int end )
{
 const int n = int( G.size() );
 if( n == 0 ) return( Nodes() );
 Nodes P;
 std::vector< bool > used( n , false );
 auto rec = [ & ]( auto & self ) -> bool {
  if( int( P.size() ) == n )
   return( ( ! cycle || G[ P.back() ][ P[ 0 ] ] ) &&
           ( end < 0 || P.back() == end ) );
  for( int v = 0 ; v < n ; ++v )
   if( ! used[ v ] && G[ P.back() ][ v ] ) {
    used[ v ] = true;
    P.push_back( v );
    if( self( self ) ) return( true );
    P.pop_back();
    used[ v ] = false;
    }
  return( false );
  };
 for( int s : starts ) {
  P = { s };
  used.assign( n , false );
  used[ s ] = true;
  if( n == 1 )
   return( ( ! cycle && ( end < 0 || end == s ) ) ?
           std::optional< Nodes >( P ) : std::nullopt );
  if( rec( rec ) ) return( P );
  }
 return( std::nullopt );
 }

inline Nodes all( int n )
{
 Nodes S( n );
 std::iota( S.begin() , S.end() , 0 );
 return( S );
 }

}  // end( namespace detail )

inline std::optional< Nodes > hamiltonianCircuit( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  if( G.size() < 3 ) return( std::nullopt );
  return( detail::hamilton( G , { 0 } , true ) );
  }();
 ADS_POST( ! result || detail::spanning( G , *result , true ) );
 return( result );
 }

inline std::optional< Nodes > hamiltonianCycle( const Adjacency & G )
{
 ADS_PRE( detail::square( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  if( G.size() < 2 ) return( std::nullopt );
  return( detail::hamilton( G , { 0 } , true ) );
  }();
 ADS_POST( ! result || detail::spanning( G , *result , true ) );
 return( result );
 }

inline std::optional< Nodes > hamiltonianPath( const Adjacency & G , int r ,
                                               int t )
{
 ADS_PRE( detail::square( G ) );
 ADS_PRE( ( r >= -1 ) && ( r < int( G.size() ) ) );
 ADS_PRE( ( t >= -1 ) && ( t < int( G.size() ) ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  return( detail::hamilton( G , r < 0 ? detail::all( int( G.size() ) ) :
                                        Nodes( { r } ) , false , t ) );
  }();
 ADS_POST( ! result || detail::spanning( G , *result , false ) );
 ADS_POST( ! result || ( r < 0 ) || ( result->front() == r ) );
 ADS_POST( ! result || ( t < 0 ) || ( result->back() == t ) );
 return( result );
 }

inline std::optional< Nodes > hamiltonianChain( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  return( detail::hamilton( G , detail::all( int( G.size() ) ) , false ) );
  }();
 ADS_POST( ! result || detail::spanning( G , *result , false ) );
 return( result );
 }

inline std::optional< Nodes > longestPath( const Adjacency & G , int r ,
                                           int t , int k )
{
 ADS_PRE( detail::square( G ) );
 ADS_PRE( ( r >= 0 ) && ( r < int( G.size() ) ) );
 ADS_PRE( ( t >= 0 ) && ( t < int( G.size() ) ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  const int n = int( G.size() );
  Nodes P = { r };
  std::vector< bool > used( n , false );
  used[ r ] = true;
  auto rec = [ & ]( auto & self ) -> bool {
   if( ( P.back() == t ) && ( int( P.size() ) - 1 >= k ) ) return( true );
   if( P.back() == t ) return( false );
   for( int v = 0 ; v < n ; ++v )
    if( ! used[ v ] && G[ P.back() ][ v ] ) {
     used[ v ] = true;
     P.push_back( v );
     if( self( self ) ) return( true );
     P.pop_back();
     used[ v ] = false;
     }
   return( false );
   };
  if( rec( rec ) ) return( P );
  return( std::nullopt );
  }();
 ADS_POST( ! result || ( int( result->size() ) > k ) );
 ADS_POST( ! result || detail::places( *result , G.size() ) );
 ADS_POST( ! result || detail::walk( G , *result , false ) );
 ADS_POST( ! result || ( ( result->front() == r ) &&
                          ( result->back() == t ) ) );
 return( result );
 }

inline int longestPathDag( const Adjacency & G , int r , int t )
{
 ADS_PRE( detail::square( G ) && detail::acyclic( G ) );
 ADS_PRE( ( r >= 0 ) && ( r < int( G.size() ) ) );
 ADS_PRE( ( t >= 0 ) && ( t < int( G.size() ) ) );
 const int n = int( G.size() );
 Nodes order;
 std::vector< int > state( n , 0 );
 auto dfs = [ & ]( auto & self , int u ) -> void {
  state[ u ] = 1;
  for( int v = 0 ; v < n ; ++v )
   if( G[ u ][ v ] && ! state[ v ] ) self( self , v );
  order.push_back( u );
  };
 for( int u = 0 ; u < n ; ++u ) if( ! state[ u ] ) dfs( dfs , u );
 std::reverse( order.begin() , order.end() );
 std::vector< int > L( n , -1 );
 L[ r ] = 0;
 for( int u : order )
  if( L[ u ] >= 0 )
   for( int v = 0 ; v < n ; ++v )
    if( G[ u ][ v ] ) L[ v ] = std::max( L[ v ] , L[ u ] + 1 );
 return( L[ t ] );
 }

inline std::optional< std::vector< std::pair< int , int > > >
boundedSpanningTree( const Adjacency & G , int k )
{
 ADS_PRE( detail::symmetric( G ) );
 using Edges = std::vector< std::pair< int , int > >;
 const auto result = [ & ]( void ) -> std::optional< Edges > {
  const int n = int( G.size() );
  std::vector< std::pair< int , int > > E , T;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v )
    if( G[ u ][ v ] ) E.emplace_back( u , v );
  std::vector< int > deg( n , 0 ) , comp( n );
  auto find = [ & ]( int x ) {
   while( comp[ x ] != x ) x = comp[ x ];
   return( x ); };
  auto rec = [ & ]( auto & self , std::size_t i ) -> bool {
   if( int( T.size() ) == n - 1 ) return( true );
   if( ( i == E.size() ) || ( int( T.size() + E.size() - i ) < n - 1 ) )
    return( false );
   auto [ u , v ] = E[ i ];
   if( deg[ u ] < k && deg[ v ] < k ) {
    std::iota( comp.begin() , comp.end() , 0 );
    for( auto [ a , b ] : T ) comp[ find( a ) ] = find( b );
    if( find( u ) != find( v ) ) {
     T.push_back( E[ i ] ); ++deg[ u ]; ++deg[ v ];
     if( self( self , i + 1 ) ) return( true );
     T.pop_back(); --deg[ u ]; --deg[ v ];
     }
    }
   return( self( self , i + 1 ) );
   };
  if( n == 0 || rec( rec , 0 ) ) return( T );
  return( std::nullopt );
  }();
 ADS_POST( ! result ||
           ( result->size() + 1 == std::max< std::size_t >( G.size() , 1 ) ) );
 ADS_POST( ! result || detail::boundedTree( G , *result , k ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- LOGIC AND LINEAR PROGRAMMING ----------------------*/
/*--------------------------------------------------------------------------*/

inline bool satCertificate( const Cnf & F , const std::vector< bool > & x )
{
 ADS_PRE( detail::literals( F , x.size() ) );
 for( const Clause & c : F ) {
  bool sat = false;
  for( int l : c ) sat = sat || ( l > 0 ? x[ l - 1 ] : ! x[ -l - 1 ] );
  if( ! sat ) return( false );
  }
 return( true );
 }

inline std::optional< std::vector< bool > > satisfiable( const Cnf & F ,
                                                         int n )
{
 ADS_PRE( ( n >= 0 ) && detail::literals( F , std::size_t( n ) ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< bool > > {
  std::vector< int > last( F.size() , 0 );         // its largest variable
  for( std::size_t c = 0 ; c < F.size() ; ++c )
   for( int l : F[ c ] ) last[ c ] = std::max( last[ c ] , std::abs( l ) );
  std::vector< bool > x( n , false );
  auto rec = [ & ]( auto & self , int v ) -> bool {
   for( std::size_t c = 0 ; c < F.size() ; ++c )
    if( last[ c ] == v ) {                         // all its variables set
     bool sat = false;
     for( int l : F[ c ] ) sat = sat || ( l > 0 ? x[ l - 1 ] : ! x[ -l - 1 ] );
     if( ! sat ) return( false );
     }
   if( v == n ) return( true );
   for( bool b : { false , true } ) {
    x[ v ] = b;
    if( self( self , v + 1 ) ) return( true );
    }
   return( false );
   };
  if( rec( rec , 0 ) ) return( x );
  return( std::nullopt );
  }();
 ADS_POST( ! result || satCertificate( F , *result ) );
 return( result );
 }

inline std::optional< std::vector< int > >
lp01( const std::vector< std::vector< long > > & A ,
      const std::vector< long > & b , int n )
{
 ADS_PRE( ( n >= 0 ) && ( A.size() == b.size() ) );
 ADS_PRE( detail::columns( A , n ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< int > > {
  std::vector< int > x( n , 0 );
  auto rec = [ & ]( auto & self , int j ) -> bool {
   if( j == n ) {
    for( std::size_t i = 0 ; i < A.size() ; ++i ) {
     long s = 0;
     for( int k = 0 ; k < n ; ++k ) s += A[ i ][ k ] * x[ k ];
     if( s > b[ i ] ) return( false );
     }
    return( true );
    }
   for( int v : { 0 , 1 } ) {
    x[ j ] = v;
    if( self( self , j + 1 ) ) return( true );
    }
   return( false );
   };
  if( rec( rec , 0 ) ) return( x );
  return( std::nullopt );
  }();
 ADS_POST( ! result || detail::feasible01( A , b , *result ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- NUMBERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::optional< Nodes > subsetSumExists( const std::vector< long > & A ,
                                               long k )
{
 ADS_PRE( detail::nonnegative( A ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  Nodes S;
  auto rec = [ & ]( auto & self , std::size_t i , long s ) -> bool {
   if( s == k ) return( true );
   if( i == A.size() || s > k ) return( false );
   S.push_back( int( i ) );
   if( self( self , i + 1 , s + A[ i ] ) ) return( true );
   S.pop_back();
   return( self( self , i + 1 , s ) );
   };
  if( rec( rec , 0 , 0 ) ) return( S );
  return( std::nullopt );
  }();
 ADS_POST( ! result || detail::places( *result , A.size() ) );
 ADS_POST( ! result || ( detail::sumOver( A , *result ) == k ) );
 return( result );
 }

inline std::optional< Nodes > partition( const std::vector< long > & A )
{
 ADS_PRE( detail::nonnegative( A ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  const long s = std::accumulate( A.begin() , A.end() , 0L );
  if( s % 2 ) return( std::nullopt );
  return( subsetSumExists( A , s / 2 ) );
  }();
 ADS_POST( ! result || ( 2 * detail::sumOver( A , *result ) ==
                          std::accumulate( A.begin() , A.end() , 0L ) ) );
 return( result );
 }

inline std::optional< Nodes > knapsackDecision( const std::vector< long > & p ,
                                                const std::vector< long > & v ,
                                                long C , long k )
{
 ADS_PRE( ( p.size() == v.size() ) && detail::nonnegative( v ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  Nodes S;
  auto rec = [ & ]( auto & self , std::size_t i , long tp , long tv ) -> bool {
   if( tv > C ) return( false );
   if( tp >= k ) return( true );
   if( i == p.size() ) return( false );
   S.push_back( int( i ) );
   if( self( self , i + 1 , tp + p[ i ] , tv + v[ i ] ) ) return( true );
   S.pop_back();
   return( self( self , i + 1 , tp , tv ) );
   };
  if( rec( rec , 0 , 0 , 0 ) ) return( S );
  return( std::nullopt );
  }();
 ADS_POST( ! result || detail::places( *result , p.size() ) );
 ADS_POST( ! result || ( ( detail::sumOver( p , *result ) >= k ) &&
                          ( detail::sumOver( v , *result ) <= C ) ) );
 return( result );
 }

inline std::optional< std::vector< int > >
multiprocessorScheduling( const std::vector< long > & t , int m , long d )
{
 ADS_PRE( ( m >= 1 ) && detail::nonnegative( t ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< int > > {
  std::vector< int > a( t.size() , -1 );
  std::vector< long > load( m , 0 );
  auto rec = [ & ]( auto & self , std::size_t i ) -> bool {
   if( i == t.size() ) return( true );
   for( int q = 0 ; q < m ; ++q ) {
    if( load[ q ] + t[ i ] > d ) continue;
    load[ q ] += t[ i ];
    a[ i ] = q;
    if( self( self , i + 1 ) ) return( true );
    load[ q ] -= t[ i ];
    if( load[ q ] == 0 ) break;            // the empty processors are equal
    }
   return( false );
   };
  if( rec( rec , 0 ) ) return( a );
  return( std::nullopt );
  }();
 ADS_POST( ! result || detail::withinLoads( t , m , d , *result ) );
 return( result );
 }

inline std::optional< Nodes >
exactCover3( const std::vector< std::array< int , 3 > > & Y , int m )
{
 ADS_PRE( std::all_of( Y.begin() , Y.end() , [ & ]( const auto & y ) {
  return( detail::triple( y , m ) ); } ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  std::vector< bool > covered( m , false );
  Nodes S;
  auto rec = [ & ]( auto & self , std::size_t j , int left ) -> bool {
   if( left == 0 ) return( true );
   if( j == Y.size() ) return( false );
   const auto & y = Y[ j ];
   if( ! covered[ y[ 0 ] ] && ! covered[ y[ 1 ] ] && ! covered[ y[ 2 ] ] &&
       y[ 0 ] != y[ 1 ] && y[ 0 ] != y[ 2 ] && y[ 1 ] != y[ 2 ] ) {
    for( int e : y ) covered[ e ] = true;
    S.push_back( int( j ) );
    if( self( self , j + 1 , left - 3 ) ) return( true );
    S.pop_back();
    for( int e : y ) covered[ e ] = false;
    }
   return( self( self , j + 1 , left ) );
   };
  if( rec( rec , 0 , m ) ) return( S );
  return( std::nullopt );
  }();
 ADS_POST( ! result || detail::exact( Y , m , *result ) );
 return( result );
 }

inline std::optional< Nodes >
threeDimensionalMatching( const std::vector< std::array< int , 3 > > & M ,
                          int q )
{
 ADS_PRE( std::all_of( M.begin() , M.end() , [ & ]( const auto & x ) {
  return( detail::coordinates( x , q ) ); } ) );
 const auto result = [ & ]( void ) -> std::optional< Nodes > {
  std::vector< std::array< int , 3 > > Y;
  for( const auto & t : M )
   Y.push_back( { t[ 0 ] , q + t[ 1 ] , 2 * q + t[ 2 ] } );
  return( exactCover3( Y , 3 * q ) );
  }();
 ADS_POST( ! result || ( int( result->size() ) == q ) );
 return( result );
 }

inline std::optional< std::vector< long > >
bribes( std::vector< long > M , const std::vector< bool > & ps )
{
 [[maybe_unused]] const std::vector< long > M0 = M;
 ADS_PRE( M.size() == ps.size() );
 const auto result = [ & ]( void ) -> std::optional< std::vector< long > > {
  std::sort( M.begin() , M.end() );
  do {
   long a = 0 , b = 0;
   for( std::size_t i = 0 ; i < M.size() ; ++i ) ( ps[ i ] ? a : b ) += M[ i ];
   if( a == 2 * b ) return( M );
   } while( std::next_permutation( M.begin() , M.end() ) );
  return( std::nullopt );
  }();
 ADS_POST( ! result || std::is_permutation( result->begin() , result->end() ,
                                            M0.begin() ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Problems.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
