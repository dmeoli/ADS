/*--------------------------------------------------------------------------*/
/*------------------------- File IncidenceGraph.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of IncidenceGraph [see IncidenceGraph.h], with their
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

inline IncidenceGraph::IncidenceGraph( int n ) : f_n( n )
{
 ADS_PRE( n >= 0 );
 }

inline std::vector< int > IncidenceGraph::V( void ) const
{
 std::vector< int > N( static_cast< std::size_t >( f_n ) );
 for( int i = 0 ; i < f_n ; ++i )
  N[ std::size_t( i ) ] = i;
 return( N );
 }

inline int IncidenceGraph::entry( int i , int e ) const
{
 ADS_PRE( hasNode( i ) && ( e >= 0 ) && ( e < f_m ) );
 return( f_A[ std::size_t( e ) * std::size_t( f_n ) + std::size_t( i ) ] );
 }

inline int IncidenceGraph::find( int e , std::int8_t v ) const
{
 const std::int8_t * c = f_A.data() + std::size_t( e ) * std::size_t( f_n );
 for( int j = 0 ; j < f_n ; ++j )
  if( c[ j ] == v )
   return( j );
 return( f_n );
 }

inline bool IncidenceGraph::columnOk( int e ) const
{
 int minus = 0 , plus = 0;
 for( int j = 0 ; j < f_n ; ++j ) {
  const int v = entry( j , e );
  minus += v == -1;
  plus += v == 1;
  if( ( v < -1 ) || ( v > 1 ) )
   return( false );
  }
 return( ( minus == 1 ) && ( plus == 1 ) );
 }

inline int IncidenceGraph::column( int i , int j ) const
{
//! [IncidenceGraph-column]
 const std::size_t n = std::size_t( f_n );
 for( int e = 0 ; e < f_m ; ++e ) {
  const std::int8_t * c = f_A.data() + std::size_t( e ) * n;
  if( ( c[ i ] == -1 ) && ( c[ j ] == 1 ) )
   return( e );
  }
 return( f_m );
//! [IncidenceGraph-column]
 }

inline void IncidenceGraph::insertEdge( int i , int j )
{
 ADS_PRE( hasNode( i ) && hasNode( j ) && ( i != j ) );
 const std::size_t n = std::size_t( f_n );
 f_A.resize( f_A.size() + n , 0 );
 std::int8_t * c = f_A.data() + std::size_t( f_m ) * n;
 c[ i ] = -1;
 c[ j ] = 1;
 ++f_m;
 ADS_POST( hasEdge( i , j ) );
 ADS_INVARIANT( columnOk( f_m - 1 ) );
 }

inline void IncidenceGraph::deleteEdge( int i , int j )
{
 ADS_PRE( hasNode( i ) && hasNode( j ) && ( i != j ) );
 const int e = column( i , j );
 if( e == f_m )
  return;
//! [IncidenceGraph-deleteEdge]
 const std::size_t n = std::size_t( f_n );
 std::copy_n( f_A.data() + std::size_t( f_m - 1 ) * n , n ,
              f_A.data() + std::size_t( e ) * n );
 f_A.resize( f_A.size() - n );
 --f_m;
//! [IncidenceGraph-deleteEdge]
 ADS_INVARIANT( ( e == f_m ) || columnOk( e ) );
 }

inline bool IncidenceGraph::hasEdge( int i , int j ) const
{
 ADS_PRE( hasNode( i ) && hasNode( j ) );
 return( ( i != j ) && ( column( i , j ) < f_m ) );
 }

template< class F >
void IncidenceGraph::forEachAdj( int i , F && f ) const
{
 ADS_PRE( hasNode( i ) );
//! [IncidenceGraph-adj]
 const std::size_t n = std::size_t( f_n );
 for( int e = 0 ; e < f_m ; ++e )
  if( f_A[ std::size_t( e ) * n + std::size_t( i ) ] == -1 )
   f( find( e , 1 ) );
//! [IncidenceGraph-adj]
 }

inline std::vector< int > IncidenceGraph::adj( int i ) const
{
 std::vector< int > out;
 forEachAdj( i , [ & ]( int j ) { out.push_back( j ); } );
 return( out );
 }

inline std::vector< int > IncidenceGraph::inAdj( int i ) const
{
 ADS_PRE( hasNode( i ) );
 const std::size_t n = std::size_t( f_n );
 std::vector< int > in;
 for( int e = 0 ; e < f_m ; ++e )
  if( f_A[ std::size_t( e ) * n + std::size_t( i ) ] == 1 )
   in.push_back( find( e , -1 ) );
 return( in );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File IncidenceGraph.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
