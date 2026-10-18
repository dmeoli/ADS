/*--------------------------------------------------------------------------*/
/*--------------------------- File EasyCases.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of EasyCases [see EasyCases.h], with their contracts.
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

/// no two pairs of M share an end, the ends in [ 0 , p ) and [ 0 , q )
inline bool matching( int p , int q ,
                      const std::vector< std::pair< int , int > > & M )
{
 std::vector< bool > L( std::max( p , 0 ) , false ) ,
                     R( std::max( q , 0 ) , false );
 for( auto [ i , j ] : M ) {
  if( ( i < 0 ) || ( i >= p ) || ( j < 0 ) || ( j >= q ) || L[ i ] || R[ j ] )
   return( false );
  L[ i ] = R[ j ] = true;
  }
 return( true );
 }

/// each interval [ a , b ) has a <= b
inline bool intervals( const std::vector< std::pair< long , long > > & I )
{
 return( std::all_of( I.begin() , I.end() , []( const auto & x ) {
  return( x.first <= x.second ); } ) );
 }

}  // end( namespace detail )

inline int literalNode( int l )
{
 ADS_PRE( l != 0 );
 return( 2 * ( std::abs( l ) - 1 ) + ( l < 0 ) );
 }

inline std::optional< std::vector< bool > > twoSat( const Cnf & F , int n )
{
 ADS_PRE( ( n >= 0 ) && detail::literals( F , std::size_t( n ) ) );
 ADS_PRE( std::all_of( F.begin() , F.end() , []( const Clause & c ) {
  return( ( c.size() == 1 ) || ( c.size() == 2 ) ); } ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< bool > > {
  ListGraph<> g( 2 * n );
  std::set< std::pair< int , int > > E;       // each implication once
  auto imply = [ & ]( int u , int v ) {
   if( ( u != v ) && E.insert( { u , v } ).second ) g.insertEdge( u , v );
   };
  for( const Clause & c : F ) {
   const int a = c[ 0 ] , b = c.size() > 1 ? c[ 1 ] : c[ 0 ];
   imply( literalNode( -a ) , literalNode( b ) );
   imply( literalNode( -b ) , literalNode( a ) );
   }
  // the components in topological order of the condensation
  const auto S = topsort( g );
  const auto id = scc( g );
  std::vector< int > rank( 2 * n + 1 , 0 );   // of each component
  for( int i = 0 , r = 0 ; i < int( S.size() ) ; ++i )
   if( ! rank[ id[ S[ i ] ] ] ) rank[ id[ S[ i ] ] ] = ++r;
  std::vector< bool > x( n );
  for( int v = 0 ; v < n ; ++v ) {
   const int p = id[ 2 * v ] , q = id[ 2 * v + 1 ];
   if( p == q ) return( std::nullopt );
   x[ v ] = rank[ p ] > rank[ q ];
   }
  return( x );
  }();
 ADS_POST( ! result || satCertificate( F , *result ) );
 return( result );
 }

inline std::optional< std::vector< int > > twoColoring( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< int > > {
  const int n = int( G.size() );
  std::vector< int > col( n , -1 );
  ArrayQueue< int > Q( std::max( n , 1 ) );
  for( int r = 0 ; r < n ; ++r ) {
   if( col[ r ] >= 0 ) continue;
   col[ r ] = 0;
   Q.enqueue( r );
   while( ! Q.isEmpty() ) {
    const int u = Q.dequeue();
    for( int v = 0 ; v < n ; ++v ) {
     if( ! G[ u ][ v ] ) continue;
     if( col[ v ] == col[ u ] ) return( std::nullopt );
     if( col[ v ] < 0 ) {
      col[ v ] = 1 - col[ u ];
      Q.enqueue( v );
      }
     }
    }
   }
  return( col );
  }();
 ADS_POST( ! result || detail::proper( G , *result , 2 ) );
 return( result );
 }

inline std::vector< std::pair< int , int > >
bipartiteMatching( int p , int q ,
                   const std::vector< std::pair< int , int > > & E )
{
 ADS_PRE( ( p >= 0 ) && ( q >= 0 ) );
 ADS_PRE( std::all_of( E.begin() , E.end() , [ & ]( const auto & e ) {
  return( ( e.first >= 0 ) && ( e.first < p ) && ( e.second >= 0 ) &&
          ( e.second < q ) ); } ) );
 const auto result = [ & ]( void ) -> std::vector< std::pair< int , int > > {
  FlowNetwork N( p + q + 2 );
  const int s = p + q , t = p + q + 1;
  for( int i = 0 ; i < p ; ++i ) N.addEdge( s , i , 1 );
  for( int j = 0 ; j < q ; ++j ) N.addEdge( p + j , t , 1 );
  std::vector< int > e;
  for( auto [ i , j ] : E ) e.push_back( N.addEdge( i , p + j , 1 ) );
  N.maxFlow( s , t );
  std::vector< std::pair< int , int > > M;
  for( std::size_t k = 0 ; k < E.size() ; ++k )
   if( N.flow( e[ k ] ) > 0 ) M.push_back( E[ k ] );
  return( M );
  }();
 ADS_POST( detail::matching( p , q , result ) );
 return( result );
 }

inline Adjacency
intervalGraph( const std::vector< std::pair< long , long > > & I )
{
 ADS_PRE( detail::intervals( I ) );
 const auto result = [ & ]( void ) -> Adjacency {
  const std::size_t n = I.size();
  Adjacency G( n , std::vector< bool >( n , false ) );
  for( std::size_t u = 0 ; u < n ; ++u )
   for( std::size_t v = u + 1 ; v < n ; ++v )
    if( ( I[ u ].first < I[ v ].second ) && ( I[ v ].first < I[ u ].second ) )
     G[ u ][ v ] = G[ v ][ u ] = true;
  return( G );
  }();
 ADS_POST( detail::symmetric( result ) && ( result.size() == I.size() ) );
 return( result );
 }

inline int intervalClique( const std::vector< std::pair< long , long > > & I )
{
 ADS_PRE( detail::intervals( I ) );
 const auto result = [ & ]( void ) -> int {
  std::vector< std::pair< long , int > > E;
  for( auto [ a , b ] : I ) {
   E.emplace_back( a , 1 );
   E.emplace_back( b , -1 );
   }
  std::sort( E.begin() , E.end() );
  int c = 0 , best = 0;
  for( auto [ x , d ] : E ) best = std::max( best , c += d );
  return( best );
  }();
 ADS_POST( ( result >= 0 ) && ( result <= int( I.size() ) ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File EasyCases.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
