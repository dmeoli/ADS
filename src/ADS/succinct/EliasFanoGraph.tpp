/*--------------------------------------------------------------------------*/
/*------------------------- File EliasFanoGraph.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of EliasFanoGraph.h, with their contracts.
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

inline EliasFanoGraph::EliasFanoGraph(
                       const std::vector< std::vector< std::size_t > > & a )
 : f_n( a.size() )
{
 const std::uint64_t n = f_n;
 ADS_PRE( ( n < ( std::uint64_t( 1 ) << 32 ) ) );
 std::vector< std::uint64_t > E , S;
 S.reserve( f_n + 1 );
 for( std::uint64_t u = 0 ; u < n ; ++u ) {
  S.push_back( E.size() );
  ADS_AUDIT( std::is_sorted( a[ u ].begin() , a[ u ].end() ) &&
             ( std::adjacent_find( a[ u ].begin() , a[ u ].end() ) ==
               a[ u ].end() ) );
  for( const std::size_t v : a[ u ] ) {
   ADS_PRE( v < n );
   E.push_back( u * n + v );
   }
  }
 S.push_back( E.size() );
 f_edges = EliasFano( E , n * n );
 f_start = EliasFano( S , E.size() + 1 );
 }

inline std::size_t EliasFanoGraph::order( void ) const
{
 return( f_n );
 }

inline std::size_t EliasFanoGraph::edges( void ) const
{
 return( f_edges.size() );
 }

inline std::size_t EliasFanoGraph::outdegree( std::size_t u ) const
{
 ADS_PRE( u < f_n );
 return( std::size_t( f_start[ u + 1 ] - f_start[ u ] ) );
 }

inline std::size_t EliasFanoGraph::successor( std::size_t u ,
                                              std::size_t i ) const
{
 ADS_PRE( i < outdegree( u ) );
 return( std::size_t( f_edges[ std::size_t( f_start[ u ] ) + i ] -
                      std::uint64_t( u ) * f_n ) );
 }

inline std::vector< std::size_t > EliasFanoGraph::successors(
                                                       std::size_t u ) const
{
 std::vector< std::size_t > s( outdegree( u ) );
 for( std::size_t i = 0 ; i < s.size() ; ++i ) s[ i ] = successor( u , i );
 return( s );
 }

inline bool EliasFanoGraph::hasEdge( std::size_t u , std::size_t v ) const
{
 ADS_PRE( ( u < f_n ) && ( v < f_n ) );
 return( f_edges.contains( std::uint64_t( u ) * f_n + v ) );
 }

inline std::size_t EliasFanoGraph::sizeInBits( void ) const
{
 return( f_edges.sizeInBits() + f_start.sizeInBits() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File EliasFanoGraph.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
