/*--------------------------------------------------------------------------*/
/*--------------------------- File Matching.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The matchings of Matching.h [see Matching.h], with their contracts.
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

/// the branch and bound of maxWeightMatching() from the node u
template< class W >
struct MatchingSearch {
 int n;
 const std::vector< std::tuple< int , int , W > > & E;
 std::vector< std::vector< std::size_t > > inc;   ///< the edges to the
                                                  ///< later nodes
 std::vector< W > rest;    ///< the best weights at the nodes >= u, summed
 std::vector< bool > used;
 std::vector< std::size_t > S , best;
 W w , bw;                 ///< the weight of S and of best

 void search( int u ) {
  while( ( u < n ) && used[ u ] )
   ++u;
  if( u == n ) {
   if( bw < w ) {
    bw = w;
    best = S;
    }
   return;
   }
  if( ! ( w + w + rest[ u ] > bw + bw ) )   // no better matching below
   return;
  for( std::size_t e : inc[ u ] ) {
   const auto & [ a , b , c ] = E[ e ];
   const int v = a == u ? b : a;          // the later end
   if( ! used[ v ] ) {
    used[ u ] = used[ v ] = true;
    S.push_back( e );
    w += c;
    search( u + 1 );
    w -= c;
    S.pop_back();
    used[ u ] = used[ v ] = false;
    }
   }
  search( u + 1 );                          // u unmatched
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< std::pair< int , int > > bipartiteBMatching(
                      const std::vector< std::vector< int > > & A , int nr ,
                      const std::vector< long long > & capL ,
                      const std::vector< long long > & capR )
{
 const int nl = int( A.size() );
 ADS_PRE( ( nr >= 0 ) && ( int( capL.size() ) == nl ) &&
          ( int( capR.size() ) == nr ) );
 FlowNetwork N( nl + nr + 2 );
 const int s = nl + nr , t = s + 1;
 std::vector< std::tuple< int , int , int > > id;   // edge , u , v
 for( int u = 0 ; u < nl ; ++u ) {
  ADS_PRE( capL[ u ] >= 0 );
  N.addEdge( s , u , capL[ u ] );
  for( int v : A[ u ] ) {
   ADS_PRE( ( v >= 0 ) && ( v < nr ) );
   id.emplace_back( N.addEdge( u , nl + v , 1 ) , u , v );
   }
  }
 for( int v = 0 ; v < nr ; ++v ) {
  ADS_PRE( capR[ v ] >= 0 );
  N.addEdge( nl + v , t , capR[ v ] );
  }
 N.maxFlow( s , t );
 std::vector< std::pair< int , int > > M;
 for( const auto & [ e , u , v ] : id )
  if( N.flow( e ) == 1 )
   M.emplace_back( u , v );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< long long > dl( nl , 0 );
  std::vector< long long > dr( nr , 0 );
  for( const auto & uv : M )
   if( ( ++dl[ uv.first ] > capL[ uv.first ] ) ||
       ( ++dr[ uv.second ] > capR[ uv.second ] ) )
    return( false );
  return( true );
  }() );
 return( M );
 }

template< class W >
std::pair< W , std::vector< std::size_t > > maxWeightMatching( int n ,
                     const std::vector< std::tuple< int , int , W > > & E )
{
 ADS_PRE( n >= 0 );
 detail::MatchingSearch< W > M{ n , E ,
                             std::vector< std::vector< std::size_t > >( n ) ,
                             std::vector< W >( n + 1 , W( 0 ) ) ,
                             std::vector< bool >( n , false ) , {} , {} ,
                             W( 0 ) , W( 0 ) };
 std::vector< W > top( n , W( 0 ) );        // the best weight at each node
 for( std::size_t e = 0 ; e < E.size() ; ++e ) {
  const auto & [ u , v , w ] = E[ e ];
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) &&
           ( u != v ) );
  if( W( 0 ) < w ) {                     // the others never help
   M.inc[ std::min( u , v ) ].push_back( e );
   top[ u ] = std::max( top[ u ] , w );
   top[ v ] = std::max( top[ v ] , w );
   }
  }
 for( int u = n ; u-- > 0 ; )
  M.rest[ u ] = M.rest[ u + 1 ] + top[ u ];
 M.search( 0 );
 std::sort( M.best.begin() , M.best.end() );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< bool > end( n , false );
  for( std::size_t e : M.best ) {
   const int u = std::get< 0 >( E[ e ] );
   const int v = std::get< 1 >( E[ e ] );
   if( end[ u ] || end[ v ] )
    return( false );
   end[ u ] = end[ v ] = true;
   }
  return( true );
  }() );
 return( std::pair( M.bw , M.best ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Matching.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
