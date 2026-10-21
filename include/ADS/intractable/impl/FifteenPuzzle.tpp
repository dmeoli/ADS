/*--------------------------------------------------------------------------*/
/*------------------------- File FifteenPuzzle.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FifteenPuzzle [see FifteenPuzzle.h], with their contracts.
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

/// B is a configuration of n x n squares: a permutation of [ 0 , n^2 )
inline bool board( const Board & B , int n )
{
 if( ( n < 1 ) || ( int( B.size() ) != n * n ) ) return( false );
 std::vector< bool > seen( B.size() , false );
 for( int x : B ) {
  if( ( x < 0 ) || ( x >= n * n ) || seen[ x ] ) return( false );
  seen[ x ] = true;
  }
 return( true );
 }

/// each configuration of S is one move away from the previous one
inline bool moves( const std::vector< Board > & S , int n )
{
 for( std::size_t i = 0 ; i + 1 < S.size() ; ++i ) {
  int diff = 0;
  for( int p = 0 ; p < n * n ; ++p ) diff += S[ i ][ p ] != S[ i + 1 ][ p ];
  if( diff != 2 ) return( false );
  }
 return( true );
 }

}  // end( namespace detail )

inline Board puzzleGoal( int n )
{
 ADS_PRE( n >= 1 );
 const auto result = [ & ]( void ) -> Board {
  Board B( n * n );
  for( int i = 0 ; i + 1 < n * n ; ++i ) B[ i ] = i + 1;
  B[ n * n - 1 ] = 0;
  return( B );
  }();
 ADS_POST( detail::board( result , n ) );
 return( result );
 }

inline int manhattan( const Board & B , int n )
{
 ADS_PRE( detail::board( B , n ) );
 int d = 0;
 for( int p = 0 ; p < n * n ; ++p )
  if( B[ p ] ) {
   const int q = B[ p ] - 1;
   d += std::abs( p / n - q / n ) + std::abs( p % n - q % n );
   }
 return( d );
 }

inline std::vector< Board > puzzleMoves( const Board & B , int n )
{
 ADS_PRE( detail::board( B , n ) );
 const auto result = [ & ]( void ) -> std::vector< Board > {
  int h = 0;
  while( B[ h ] ) ++h;
  std::vector< Board > C;
  const int r = h / n , c = h % n;
  for( auto [ dr , dc ] : { std::pair( -1 , 0 ) , std::pair( 1 , 0 ) ,
                            std::pair( 0 , -1 ) , std::pair( 0 , 1 ) } ) {
   const int rr = r + dr , cc = c + dc;
   if( ( rr < 0 ) || ( rr >= n ) || ( cc < 0 ) || ( cc >= n ) ) continue;
   Board X = B;
   std::swap( X[ h ] , X[ rr * n + cc ] );
   C.push_back( X );
   }
  return( C );
  }();
 ADS_POST( ( n < 2 ) || ( ( result.size() >= 2 ) && ( result.size() <= 4 ) ) );
 return( result );
 }

inline bool solvable( const Board & B , int n )
{
 ADS_PRE( detail::board( B , n ) );
 int inv = 0 , hole = 0;
 for( int p = 0 ; p < n * n ; ++p ) {
  if( ! B[ p ] ) {
   hole = p / n;
   continue;
   }
  for( int q = p + 1 ; q < n * n ; ++q ) inv += B[ q ] && ( B[ q ] < B[ p ] );
  }
 if( n % 2 ) return( inv % 2 == 0 );
 return( ( inv + ( n - 1 - hole ) ) % 2 == 0 );
 }

namespace detail {

/// the value of k in the map m of the levels, or nullptr: the find() of
/// the maps of std and Boost, or of the hash tables of ADS
template< class M , class K >
int * levelOf( M & m , const K & k )
{
 if constexpr( std::is_pointer_v< decltype( m.find( k ) ) > )
  return( m.find( k ) );                    // a table of ADS
 else {
  auto it = m.find( k );
  return( it == m.end() ? nullptr : &it->second );
  }
 }

/// the value of k in m set to v
template< class M , class K >
void setLevel( M & m , const K & k , int v )
{
 if constexpr( std::is_pointer_v< decltype( m.find( k ) ) > ) {
  if( int * p = m.find( k ) ) *p = v;
  else m.insert( k , v );
  }
 else
  m.insert_or_assign( k , v );
 }

}  // end( namespace detail )

template< class Map >
std::optional< std::vector< Board > > solvePuzzle( const Board & B , int n ,
                                                   long * visited )
{
 ADS_PRE( detail::board( B , n ) );
 const auto result = [ & ]( void ) -> std::optional< std::vector< Board > > {
  if( visited ) *visited = 0;
  if( ! solvable( B , n ) ) return( std::nullopt );
  const Board goal = puzzleGoal( n );
  std::vector< Board > node( 1 , B );
  std::vector< int > parent( 1 , -1 ) , level( 1 , 0 );
  Map best;                               // the least level of each one
  detail::setLevel( best , B , 0 );
  FibonacciHeapPriorityQueue< int , int > Q;
  Q.insert( 0 , manhattan( B , n ) );
  while( ! Q.empty() ) {
   const int k = Q.deleteMin();
   if( level[ k ] > *detail::levelOf( best , node[ k ] ) )
    continue;                                         // a stale entry
   if( visited ) ++*visited;
   if( node[ k ] == goal ) {
    std::vector< Board > S;
    for( int x = k ; x >= 0 ; x = parent[ x ] )
     S.insert( S.begin() , node[ x ] );
    return( S );
    }
   for( Board & C : puzzleMoves( node[ k ] , n ) ) {
    const int g = level[ k ] + 1;
    const int * at = detail::levelOf( best , C );
    if( at && ( *at <= g ) ) continue;
    detail::setLevel( best , C , g );
    const int lb = manhattan( C , n );
    node.push_back( std::move( C ) );
    parent.push_back( k );
    level.push_back( g );
    Q.insert( int( node.size() ) - 1 , g + lb );
    }
   }
  return( std::nullopt );
  }();
 ADS_POST( ! result ||
           ( ( result->front() == B ) &&
           ( result->back() == puzzleGoal( n ) ) ) );
 ADS_POST( ! result || detail::moves( *result , n ) );
 return( result );
 }

inline Board puzzleSpiralGoal( int n )
{
 ADS_PRE( n >= 1 );
 Board B( n * n , 0 );
 int top = 0 , bottom = n - 1 , left = 0 , right = n - 1 , t = 1;
 while( t < n * n ) {
  for( int c = left ; ( c <= right ) && ( t < n * n ) ; ++c )
   B[ top * n + c ] = t++;
  ++top;
  for( int r = top ; ( r <= bottom ) && ( t < n * n ) ; ++r )
   B[ r * n + right ] = t++;
  --right;
  for( int c = right ; ( c >= left ) && ( t < n * n ) ; --c )
   B[ bottom * n + c ] = t++;
  --bottom;
  for( int r = bottom ; ( r >= top ) && ( t < n * n ) ; --r )
   B[ r * n + left ] = t++;
  ++left;
  }
 ADS_POST( detail::board( B , n ) );
 return( B );
 }

inline std::optional< std::vector< Board > > puzzleBFS(
                        const Board & B , const Board & goal , int n ,
                        long * visited )
{
 ADS_PRE( detail::board( B , n ) && detail::board( goal , n ) );
 if( visited ) *visited = 0;
 std::map< Board , Board > parent;          // the tree of the visit
 parent[ B ] = B;
 std::queue< Board > Q;
 Q.push( B );
 while( ! Q.empty() ) {
  Board u = Q.front();
  Q.pop();
  if( visited ) ++*visited;
  if( u == goal ) {
   std::vector< Board > S = { u };
   while( S.front() != B )
    S.insert( S.begin() , parent[ S.front() ] );
   ADS_POST( ( S.front() == B ) && ( S.back() == goal ) &&
             detail::moves( S , n ) );
   return( S );
   }
  for( auto & v : puzzleMoves( u , n ) )
   if( parent.emplace( v , u ).second )
    Q.push( std::move( v ) );
  }
 return( std::nullopt );
 }

inline std::optional< std::vector< Board > > puzzleDFS(
                        const Board & B , const Board & goal , int n ,
                        int nmax )
{
 ADS_PRE( detail::board( B , n ) && detail::board( goal , n ) &&
          ( nmax >= 0 ) );
 std::vector< Board > S = { B };            // the current path
 std::set< Board > on = { B };
 auto rec = [ & ]( auto & self ) -> bool {
  if( S.back() == goal )
   return( true );
  if( int( S.size() ) > nmax )              // nmax moves made
   return( false );
  for( auto & v : puzzleMoves( S.back() , n ) ) {
   if( on.count( v ) )                      // already on the path
    continue;
   on.insert( v );
   S.push_back( v );
   if( self( self ) )
    return( true );
   S.pop_back();
   on.erase( v );
   }
  return( false );
  };
 if( ! rec( rec ) )
  return( std::nullopt );
 ADS_POST( ( S.back() == goal ) && ( int( S.size() ) <= nmax + 1 ) &&
           detail::moves( S , n ) );
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File FifteenPuzzle.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
