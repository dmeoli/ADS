/*--------------------------------------------------------------------------*/
/*--------------------------- File GridGraph.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of GridGraph [see GridGraph.h], with their contracts.
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

#include <algorithm>
#include <climits>
#include <cmath>
#include <numbers>
#include <string>

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------ CONSTRUCTION ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W , class C , class P >
GridGraph< W , C , P >::GridGraph( int rows , int cols , GridMoves moves ,
				 Corners corners , W straight ,
				 std::optional< W > diagonal , MoveCost model )
    : f_rows( rows ) , f_cols( cols ) , f_moves( moves ) ,
      f_corners( corners ) ,
      f_model( model ) , f_straight( straight ) , f_diagonal( straight )
{
 init( diagonal );
 }

template< class W , class C , class P >
GridGraph< W , C , P >::GridGraph( int rows , int cols ,
				   std::vector< C > cost ,
				 GridMoves moves , Corners corners ,
				 W straight ,
				 std::optional< W > diagonal , MoveCost model )
    : f_rows( rows ) , f_cols( cols ) , f_moves( moves ) ,
      f_corners( corners ) ,
      f_model( model ) , f_straight( straight ) , f_diagonal( straight ) ,
      f_cost( std::move( cost ) )
{
 init( diagonal );
 }

template< class W , class C , class P >
template< class... O >
 requires( sizeof...( O ) > 0 )
GridGraph< W , C , P >::GridGraph( int rows , int cols ,
				   std::vector< C > cost ,
				 GridMoves moves , Corners corners ,
				 W straight ,
				 std::optional< W > diagonal , MoveCost model ,
				 O &... o )
    : f_rows( rows ) , f_cols( cols ) , f_moves( moves ) ,
      f_corners( corners ) ,
      f_model( model ) , f_straight( straight ) , f_diagonal( straight ) ,
      f_cost( std::move( cost ) ) , f_sub( o... )
{
 init( diagonal );
 }

template< class W , class C , class P >
void GridGraph< W , C , P >::init( std::optional< W > diagonal )
{
 ADS_PRE( ( f_rows >= 0 ) && ( f_cols >= 0 ) );
 // the product in 64 bits: rows * cols may not fit an int
 const long long cells = 1LL * f_rows * f_cols;
 ADS_PRE( cells <= INT_MAX );
 ADS_PRE( f_cost.empty() || ( f_cost.size() == std::size_t( cells ) ) );
 ADS_PRE( f_straight > W( 0 ) );
 if( diagonal )
  f_diagonal = *diagonal;
 else if constexpr( std::is_floating_point_v< W > )
  f_diagonal = f_straight * std::numbers::sqrt2_v< W >;
 else
  ADS_PRE( f_moves == GridMoves::FOUR );
 ADS_PRE( f_diagonal > W( 0 ) );
 if( f_cost.empty() ) {
  f_n = f_atmin = int( cells );
  return;
  }
 recount();
 ADS_INVARIANT( counted() );
 }

template< class W , class C , class P >
void GridGraph< W , C , P >::recount( void )
{
 // the graph is oriented if a free cell costs other than the least cost
 f_n = f_atmin = 0;
 f_min = C( 1 );
 for( const C & c : f_cost )
  if( c > C( 0 ) ) {
   if( ( ++f_n == 1 ) || ( c < f_min ) ) {
    f_min = c;
    f_atmin = 1;
    }
   else if( c == f_min )
    ++f_atmin;
   }
 f_directed = ( f_n > 0 ) && ( f_atmin != f_n );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE OPERATIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class W , class C , class P >
int GridGraph< W , C , P >::node( int r , int c ) const
{
 ADS_PRE( insideGrid( f_rows , f_cols , GridCell( r , c ) ) );
 return( r * f_cols + c );
 }

template< class W , class C , class P >
GridCell GridGraph< W , C , P >::cell( int u ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 return( GridCell( u / f_cols , u % f_cols ) );
 }

template< class W , class C , class P >
C GridGraph< W , C , P >::cost( int u ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 return( f_cost.empty() ? C( 1 ) : f_cost[ u ] );
 }

template< class W , class C , class P >
void GridGraph< W , C , P >::setCost( int u , const C & c )
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 if( f_cost.empty() ) {  // all free, of cost 1
  if( c == C( 1 ) )
   return;
  f_cost.assign( std::size_t( bound() ) , C( 1 ) );
  }
 const C a = f_cost[ u ];
 if( a == c )
  return;
 f_cost[ u ] = c;
 if( a > C( 0 ) ) {
  --f_n;
  if( a == f_min )
   --f_atmin;
  }
 if( c > C( 0 ) ) {
  if( ( ++f_n == 1 ) || ( c < f_min ) ) {
   f_min = c;
   f_atmin = 1;
   }
  else if( c == f_min )
   ++f_atmin;
  }
 if( f_n == 0 ) {
  f_min = C( 1 );
  f_atmin = 0;
  }
 else if( f_atmin == 0 )  // the least cost left
  recount();
 f_directed = ( f_n > 0 ) && ( f_atmin != f_n );
 ADS_INVARIANT( counted() );
 if constexpr( Subject::template listened< CellChanged > )
  if( f_sub.template live< CellChanged >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    tellCold( CellChanged{ u , a , c } );
   else {
    f_sub.emit( CellChanged{ u , a , c } );
    f_sub.audit( *this );
    }
   }
 }

template< class W , class C , class P >
template< class F >
void GridGraph< W , C , P >::forEachAffected( int u , const C & from ,
					    const C & to , F && f ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 const bool a = from > C( 0 ) , b = to > C( 0 );
 if( ! a && ! b )  // blocked before and after
  return;
 if( a && b ) {  // free before and after
  if( f_model == MoveCost::ENTERED )
   f( u );
  else
   scan( u , [ & ]( int v , bool ) { f( v ); } );
  return;
  }
 // freed or blocked: u and the free cells at a move from it, in order
 const int k = f_cols;
 const int r = u / k , c = u - r * k;
 const bool eight = f_moves == GridMoves::EIGHT;
 for( int dr = -1 ; dr <= 1 ; ++dr )
  for( int dc = -1 ; dc <= 1 ; ++dc ) {
   if( ! eight && ( dr != 0 ) && ( dc != 0 ) )
    continue;
   const int y = r + dr , x = c + dc;
   if( ( y < 0 ) || ( y >= f_rows ) || ( x < 0 ) || ( x >= k ) )
    continue;
   const int v = u + dr * k + dc;
   if( ( v == u ) || open( v ) )
    f( v );
   }
 }

template< class W , class C , class P >
std::vector< int > GridGraph< W , C , P >::V( void ) const
{
 std::vector< int > out;
 out.reserve( std::size_t( f_n ) );
 for( int u = 0 ; u < bound() ; ++u )
  if( open( u ) )
   out.push_back( u );
 return( out );
 }

template< class W , class C , class P >
std::vector< int > GridGraph< W , C , P >::adj( int u ) const
{
 ADS_PRE( hasNode( u ) );
 std::vector< int > out;
 scan( u , [ & ]( int v , bool ) { out.push_back( v ); } );
 return( out );
 }

template< class W , class C , class P >
template< class F >
void GridGraph< W , C , P >::forEachMove( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 scan( u , f );
 }

template< class W , class C , class P >
template< class G >
void GridGraph< W , C , P >::scan( int u , G && g ) const
{
 // the row and the column of u, so that no name out of the grid is ever
 // computed: u + cols only when u is not in the last row, and so on
 const int k = f_cols;
 const int r = u / k , c = u - r * k;
 const bool up = r > 0 , down = r + 1 < f_rows;
 const bool left = c > 0 , right = c + 1 < k;
 // the four sides, free or not
 const bool n = up && open( u - k ) , s = down && open( u + k );
 const bool w = left && open( u - 1 ) , e = right && open( u + 1 );
 if( f_moves == GridMoves::FOUR ) {
  if( n )
   g( u - k , false );
  if( w )
   g( u - 1 , false );
  if( e )
   g( u + 1 , false );
  if( s )
   g( u + k , false );
  return;
  }
 if( up && left && corner( n , w ) && open( u - k - 1 ) )
  g( u - k - 1 , true );
 if( n )
  g( u - k , false );
 if( up && right && corner( n , e ) && open( u - k + 1 ) )
  g( u - k + 1 , true );
 if( w )
  g( u - 1 , false );
 if( e )
  g( u + 1 , false );
 if( down && left && corner( s , w ) && open( u + k - 1 ) )
  g( u + k - 1 , true );
 if( s )
  g( u + k , false );
 if( down && right && corner( s , e ) && open( u + k + 1 ) )
  g( u + k + 1 , true );
 }

template< class W , class C , class P >
template< class F >
void GridGraph< W , C , P >::forEachAdj( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 scan( u , [ & ]( int v , bool ) { f( v ); } );
 }

template< class W , class C , class P >
template< class F >
void GridGraph< W , C , P >::forEachEdge( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 if( f_cost.empty() || ! f_directed || ( f_model == MoveCost::LEFT ) ) {
  // one cost for all the moves out of u (one for all the free cells, or
  // that of u itself when the cell left pays): two weights only
  const W k = f_cost.empty()
		? W( 1 )
		: W( f_model == MoveCost::LEFT ? f_cost[ u ] : f_min );
  const W ws = k * f_straight , wd = k * f_diagonal;
  scan( u , [ & ]( int v , bool d ) { f( v , d ? wd : ws ); } );
  }
 else
  scan( u , [ & ]( int v , bool d ) {
   f( v , W( f_cost[ v ] ) * ( d ? f_diagonal : f_straight ) );
   } );
 }

template< class W , class C , class P >
bool GridGraph< W , C , P >::counted( void ) const
{
 if( f_cost.empty() )
  return( ( f_n == bound() ) && ( f_atmin == f_n ) );
 long long k = 0 , m = 0;
 for( const C & c : f_cost )
  if( c > C( 0 ) ) {
   ++k;
   m += ( c == f_min );
   if( c < f_min )
    return( false );
   }
 return( ( k == f_n ) && ( m == f_atmin ) && ( ( k > 0 ) == ( m > 0 ) ) &&
	  ( f_directed == ( m != k ) ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

/// D holds the distances of a breadth first visit of g from S: 0 on the
/// sources, INF on the blocked cells, each other cell at INF or one move
/// farther than a neighbor, and no move from a reached cell to a cell
/// more than one move farther
template< class W , class C , class P >
bool gridGraphBfs( const GridGraph< W , C , P > & g ,
		   const std::vector< int > & S ,
		   const std::vector< int > & D )
{
 for( int s : S )
  if( D[ s ] != 0 )
   return( false );
 std::vector< int > best( D.size() , INF );  // the least D of a neighbor
 for( int u = 0 ; u < g.bound() ; ++u ) {
  if( ! g.hasNode( u ) ) {
   if( D[ u ] != INF )
    return( false );
   continue;
   }
  bool ok = true;
  g.forEachAdj( u , [ & ]( int v ) {
   best[ v ] = std::min( best[ v ] , D[ u ] );
   if( ( D[ u ] != INF ) && ( D[ v ] > D[ u ] + 1 ) )
    ok = false;
   } );
  if( ! ok )
   return( false );
  }
 for( int u = 0 ; u < g.bound() ; ++u )
  if( ( D[ u ] != INF ) && ( D[ u ] != 0 ) &&
      ( ( best[ u ] == INF ) || ( best[ u ] + 1 != D[ u ] ) ) )
   return( false );
 return( true );
 }

/// |x - y| of two ints, in 64 bits
inline long long gap( int x , int y )
{
 return( x > y ? 1LL * x - y : 1LL * y - x );
 }

}  // end( namespace detail )

template< class W , class C , class P >
std::vector< int > gridDistances( const GridGraph< W , C , P > & g ,
				  const std::vector< int > & S )
{
 std::vector< int > D( std::size_t( g.bound() ) , INF );
 std::vector< int > Q;  // the queue, read from h
 Q.reserve( std::size_t( g.n() ) );
 for( int s : S ) {
  ADS_PRE( g.hasNode( s ) );
  if( D[ s ] != 0 ) {
   D[ s ] = 0;
   Q.push_back( s );
   }
  }
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  const int u = Q[ h ];
  g.forEachAdj( u , [ & ]( int v ) {
   if( D[ v ] == INF ) {
    D[ v ] = D[ u ] + 1;
    Q.push_back( v );
    }
   } );
  }
 ADS_AUDIT( detail::gridGraphBfs( g , S , D ) );
 return( D );
 }

template< class W >
W manhattan( GridCell a , GridCell b , W straight )
{
 ADS_PRE( straight >= W( 0 ) );
 return( straight * W( detail::gap( a.first , b.first ) +
			detail::gap( a.second , b.second ) ) );
 }

template< class W >
W octile( GridCell a , GridCell b , W straight , W diagonal )
{
 ADS_PRE( ( W( 0 ) <= straight ) && ( straight <= diagonal ) &&
	  ( diagonal <= straight + straight ) );
 const long long x = detail::gap( a.first , b.first );
 const long long y = detail::gap( a.second , b.second );
 const long long M = std::max( x , y ) , m = std::min( x , y );
 return( straight * W( M - m ) + diagonal * W( m ) );
 }

template< class W >
W chebyshev( GridCell a , GridCell b , W straight )
{
 ADS_PRE( straight >= W( 0 ) );
 return( straight * W( std::max( detail::gap( a.first , b.first ) ,
				  detail::gap( a.second , b.second ) ) ) );
 }

template< class W , class C , class P >
W gridHeuristic( const GridGraph< W , C , P > & g , int u , int t )
{
 const GridCell a = g.cell( u ) , b = g.cell( t );
 const W k( g.minCost() );
 if( g.moves() == GridMoves::FOUR )
  return( k * manhattan( a , b , g.straight() ) );
 ADS_PRE( g.diagonal() >= g.straight() );
 return( k *
	  octile( a , b , g.straight() ,
		  std::min( g.diagonal() , g.straight() + g.straight() ) ) );
 }

template< class W , class C >
GridGraph< W , C > readGridMap( std::istream & in , GridMoves moves ,
			       Corners corners )
{
 std::string key , type;
 int h = -1 , w = -1;
 // the header: type, height, width, in this order, and the word map
 in >> key >> type;
 ADS_PRE( in && ( key == "type" ) );
 for( int i = 0 ; i < 2 ; ++i ) {
  int x = -1;
  in >> key >> x;
  ADS_PRE( in && ( ( key == "height" ) || ( key == "width" ) ) );
  ( key == "height" ? h : w ) = x;
  }
 in >> key;
 ADS_PRE( in && ( key == "map" ) && ( h >= 0 ) && ( w >= 0 ) &&
	  ( 1LL * h * w <= INT_MAX ) );
 std::vector< C > cost( std::size_t( h ) * std::size_t( w ) );
 std::string line;
 for( int r = 0 ; r < h ; ++r ) {
  in >> line;
  ADS_PRE( in && ( int( line.size() ) == w ) );
  for( int c = 0 ; c < w ; ++c ) {
   const char x = line[ c ];
   cost[ std::size_t( r ) * w + c ] =
     C( ( x == '.' ) || ( x == 'G' ) || ( x == 'S' ) ? 1 : 0 );
   }
  }
 return( GridGraph< W , C >( h , w , std::move( cost ) , moves , corners ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File GridGraph.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
