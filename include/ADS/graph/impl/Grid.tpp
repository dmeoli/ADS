/*--------------------------------------------------------------------------*/
/*----------------------------- File Grid.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The visits of a grid of Grid.h [see Grid.h], with their contracts.
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

/// whether the rows of M have all the same length
template< class T >
bool rectangular( const std::vector< std::vector< T > > & M )
{
 for( const auto & row : M )
  if( row.size() != M.front().size() )
   return( false );
 return( true );
 }

/// D holds the distances of a breadth first visit from the sources through
/// the free cells: 0 on the sources, INF on the cells not free, and each
/// other cell at INF or one move farther than a neighbour from which a
/// move reaches it, with no move from a reached cell to a free cell more
/// than one move farther
template< class Free , class Moves >
bool gridBfs( int rows , int cols , const std::vector< GridCell > & sources ,
              Free & free , const Moves & moves ,
              const std::vector< std::vector< int > > & D )
{
 for( const auto & x : sources )
  if( D[ x.first ][ x.second ] != 0 )
   return( false );
 for( int r = 0 ; r < rows ; ++r )
  for( int c = 0 ; c < cols ; ++c ) {
   const int d = D[ r ][ c ];
   if( ( d == INF ) || ( d == 0 ) )
    continue;
   bool parent = false;
   for( const auto & [ dr , dc ] : moves ) {
    const GridCell y( r - dr , c - dc );
    if( insideGrid( rows , cols , y ) && ( D[ y.first ][ y.second ] != INF )
        && ( D[ y.first ][ y.second ] + 1 == d ) )
     parent = true;
    }
   if( ! parent || ! free( GridCell( r , c ) ) )
    return( false );
   }
 for( int r = 0 ; r < rows ; ++r )
  for( int c = 0 ; c < cols ; ++c )
   if( D[ r ][ c ] != INF )
    for( const auto & [ dr , dc ] : moves ) {
     const GridCell y( r + dr , c + dc );
     if( insideGrid( rows , cols , y ) && free( y ) &&
         ( D[ y.first ][ y.second ] > D[ r ][ c ] + 1 ) )
      return( false );
     }
 return( true );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

inline bool insideGrid( int rows , int cols , GridCell x )
{
 return( ( 0 <= x.first ) && ( x.first < rows ) && ( 0 <= x.second ) &&
         ( x.second < cols ) );
 }

template< class Free , class Moves >
std::vector< std::vector< int > > gridDistances( int rows , int cols ,
                                    const std::vector< GridCell > & sources ,
                                    Free && free , const Moves & moves )
{
 ADS_PRE( ( rows >= 0 ) && ( cols >= 0 ) );
 std::vector< std::vector< int > > D( rows , std::vector< int >( cols ,
                                                                 INF ) );
 std::vector< GridCell > Q;                    // the queue, read from h
 for( const auto & x : sources ) {
  ADS_PRE( insideGrid( rows , cols , x ) && free( x ) );
  if( D[ x.first ][ x.second ] != 0 ) {
   D[ x.first ][ x.second ] = 0;
   Q.push_back( x );
   }
  }
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const auto [ r , c ] = Q[ h ];
  for( const auto & [ dr , dc ] : moves ) {
   const GridCell y( r + dr , c + dc );
   if( insideGrid( rows , cols , y ) && ( D[ y.first ][ y.second ] == INF )
       && free( y ) ) {
    D[ y.first ][ y.second ] = D[ r ][ c ] + 1;
    Q.push_back( y );
    }
   }
  }
 ADS_AUDIT( detail::gridBfs( rows , cols , sources , free , moves , D ) );
 return( D );
 }

template< class Free , class Moves >
int gridDistance( int rows , int cols , GridCell s , GridCell t ,
                  Free && free , const Moves & moves )
{
 ADS_PRE( ( rows >= 0 ) && ( cols >= 0 ) && insideGrid( rows , cols , s ) &&
          insideGrid( rows , cols , t ) && free( s ) );
 std::vector< std::vector< int > > D( rows , std::vector< int >( cols ,
                                                                 INF ) );
 std::vector< GridCell > Q = { s };
 D[ s.first ][ s.second ] = 0;
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const auto [ r , c ] = Q[ h ];
  if( Q[ h ] == t )
   return( D[ r ][ c ] );
  for( const auto & [ dr , dc ] : moves ) {
   const GridCell y( r + dr , c + dc );
   if( insideGrid( rows , cols , y ) && ( D[ y.first ][ y.second ] == INF )
       && free( y ) ) {
    D[ y.first ][ y.second ] = D[ r ][ c ] + 1;
    Q.push_back( y );
    }
   }
  }
 return( INF );
 }

template< class T , class Moves >
std::size_t floodFill( std::vector< std::vector< T > > & M , int r , int c ,
                       const std::type_identity_t< T > & color ,
                       const Moves & moves )
{
 ADS_PRE( detail::rectangular( M ) );
 const int rows = int( M.size() );
 const int cols = rows == 0 ? 0 : int( M.front().size() );
 ADS_PRE( insideGrid( rows , cols , GridCell( r , c ) ) );
 const T old = M[ r ][ c ];
 if( old == color )
  return( 0 );
 std::vector< GridCell > Q = { GridCell( r , c ) };
 M[ r ][ c ] = color;                  // painted when it enters the queue
 for( std::size_t h = 0 ; h < Q.size() ; ++h )
  for( const auto & [ dr , dc ] : moves ) {
   const GridCell y( Q[ h ].first + dr , Q[ h ].second + dc );
   if( insideGrid( rows , cols , y ) && ( M[ y.first ][ y.second ] == old ) ) {
    M[ y.first ][ y.second ] = color;
    Q.push_back( y );
    }
   }
 ADS_POST( M[ r ][ c ] == color );
 return( Q.size() );
 }

inline int knightDistance( int n , GridCell s , GridCell t )
{
 ADS_PRE( ( n > 0 ) && insideGrid( n , n , s ) && insideGrid( n , n , t ) );
 return( gridDistance( n , n , s , t , []( GridCell ) { return( true ); } ,
                       KNIGHT_MOVES ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Grid.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
