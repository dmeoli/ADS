/*--------------------------------------------------------------------------*/
/*------------------------- File EulerianCycle.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The Eulerian cycles of EulerianCycle.h [see EulerianCycle.h], with their
 * contracts.
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

template< Graph G >
std::optional< std::vector< int > > eulerianCycle( const G & g )
{
 ADS_PRE( g.directed() );
 const int n = g.bound();
 std::vector< std::vector< int > > out( n );
 std::vector< int > in( n , 0 );
 MFSet M( std::max( n , 1 ) );
 int m = 0 , start = -1;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   out[ u ].push_back( v );
   ++in[ v ];
   M.merge( u , v );
   ++m;
   start = u;
   } );
 if( m == 0 )
  return( std::vector< int >() );
 for( int u = 0 ; u < n ; ++u ) {
  if( int( out[ u ].size() ) != in[ u ] )
   return( std::nullopt );
  if( ! out[ u ].empty() && ( M.find( u ) != M.find( start ) ) )
   return( std::nullopt );
  }
 std::vector< std::size_t > next( n , 0 );
 std::vector< int > S = { start } , cycle;
 while( ! S.empty() ) {
  const int u = S.back();
  if( next[ u ] < out[ u ].size() )
   S.push_back( out[ u ][ next[ u ]++ ] );
  else {
   cycle.push_back( u );
   S.pop_back();
   }
  }
 std::reverse( cycle.begin() , cycle.end() );
 ADS_AUDIT( isEulerianCycle( g , cycle ) );
 return( cycle );
 }

template< Graph G >
bool isEulerianCycle( const G & g , const std::vector< int > & c )
{
 std::vector< std::pair< int , int > > E , C;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { E.emplace_back( u , v ); } );
 if( E.empty() )
  return( c.empty() );
 if( ( c.size() != E.size() + 1 ) || ( c.front() != c.back() ) )
  return( false );
 for( std::size_t i = 0 ; i + 1 < c.size() ; ++i )
  C.emplace_back( c[ i ] , c[ i + 1 ] );
 std::sort( E.begin() , E.end() );
 std::sort( C.begin() , C.end() );
 return( E == C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File EulerianCycle.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
