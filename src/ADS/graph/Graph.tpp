/*--------------------------------------------------------------------------*/
/*----------------------------- File Graph.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of GraphNodes and incidence() [see Graph.h], with their
 * contracts.
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
/*---------------------------- CLASS GraphNodes ----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool GraphNodes< T >::hasNode( int u ) const
{
 return( ( u >= 0 ) && ( u < bound() ) && f_alive[ u ] );
 }

template< class T >
std::vector< int > GraphNodes< T >::V( void ) const
{
 std::vector< int > out;
 for( int u = 0 ; u < bound() ; ++u )
  if( f_alive[ u ] )
   out.push_back( u );
 return( out );
 }

template< class T >
const T & GraphNodes< T >::label( int u ) const
{
 ADS_PRE( hasNode( u ) );
 return( f_label[ u ] );
 }

template< class T >
void GraphNodes< T >::setLabel( int u , const T & t )
{
 ADS_PRE( hasNode( u ) );
 f_label[ u ] = t;
 }

template< class T >
GraphNodes< T >::GraphNodes( int n , bool directed ) : f_directed( directed )
{
 ADS_PRE( n >= 0 );
 f_alive.assign( n , true );
 f_label.resize( n );
 f_n = n;
 }

template< class T >
int GraphNodes< T >::addNode( const T & t )
{
 f_alive.push_back( true );
 f_label.push_back( t );
 ++f_n;
 ADS_POST( hasNode( bound() - 1 ) );
 ADS_INVARIANT( nodesCounted() );
 return( bound() - 1 );
 }

template< class T >
int GraphNodes< T >::check( int u ) const
{
 ADS_PRE( hasNode( u ) );
 return( u );
 }

template< class T >
void GraphNodes< T >::edge( int u , int v ) const
{
 ADS_PRE( hasNode( u ) && hasNode( v ) && ( u != v ) );
 }

template< class T >
bool GraphNodes< T >::nodesCounted( void ) const
{
 int k = 0;
 for( bool a : f_alive ) k += a;
 return( ( k == f_n ) && ( f_label.size() == f_alive.size() ) &&
         ( f_m >= 0 ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
std::vector< std::vector< int > > incidence( const G & g )
{
 std::vector< std::pair< int , int > > E;
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) {
   if( g.directed() || ( u < v ) )
    E.emplace_back( u , v );
   } );
 std::vector< std::vector< int > > B( g.bound() ,
                                      std::vector< int >( E.size() ) );
 for( std::size_t k = 0 ; k < E.size() ; ++k ) {
  B[ E[ k ].first ][ k ] = g.directed() ? -1 : 1;
  B[ E[ k ].second ][ k ] = 1;
  }
 ADS_POST( B.size() == std::size_t( g.bound() ) );
 return( B );
 }

inline std::vector< int > adjFromIncidence(
                          const std::vector< std::vector< int > > & B ,
                          int u , bool directed )
{
 ADS_PRE( ( u >= 0 ) && ( u < int( B.size() ) ) );
 std::vector< int > A;
 const int from = directed ? -1 : 1 , to = 1;
 for( std::size_t k = 0 ; k < B[ u ].size() ; ++k ) {
  if( B[ u ][ k ] != from )
   continue;
  for( int i = 0 ; i < int( B.size() ) ; ++i )   // the other end
   if( ( i != u ) && ( B[ i ][ k ] == to ) )
    A.push_back( i );
  }
 return( A );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Graph.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
