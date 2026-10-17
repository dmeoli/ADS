/*--------------------------------------------------------------------------*/
/*-------------------------- File ArrayGraph.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ArrayGraph [see ArrayGraph.h], with their contracts.
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
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
ArrayGraph< W >::ArrayGraph( int n ,
                 const std::vector< std::tuple< int , int , W > > & E ,
                 bool directed ) : f_directed( directed )
{
 ADS_PRE( n >= 0 );
 f_off.assign( n + 1 , 0 );
 for( const auto & [ u , v , w ] : E ) {
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) && ( u != v ) );
  ++f_off[ u + 1 ];
  if( ! directed )
   ++f_off[ v + 1 ];
  }
 for( int u = 0 ; u < n ; ++u )
  f_off[ u + 1 ] += f_off[ u ];
 f_to.resize( f_off[ n ] );
 f_w.resize( f_off[ n ] );
 std::vector< int > next( f_off.begin() , f_off.end() - 1 );
 for( const auto & [ u , v , w ] : E ) {
  f_to[ next[ u ] ] = v;
  f_w[ next[ u ]++ ] = w;
  if( ! directed ) {
   f_to[ next[ v ] ] = u;
   f_w[ next[ v ]++ ] = w;
   }
  }
 f_m = int( E.size() );
 ADS_INVARIANT( consistent() );
 }

template< class W >
template< Graph G >
ArrayGraph< W >::ArrayGraph( const G & g )
 : ArrayGraph( g.bound() , edges( g ) , g.directed() ) {}

template< class W >
bool ArrayGraph< W >::hasNode( int u ) const
{
 return( ( u >= 0 ) && ( u < n() ) );
 }

template< class W >
std::vector< int > ArrayGraph< W >::V( void ) const
{
 std::vector< int > out( n() );
 for( int u = 0 ; u < n() ; ++u )
  out[ u ] = u;
 return( out );
 }

template< class W >
std::vector< int > ArrayGraph< W >::adj( int u ) const
{
 ADS_PRE( hasNode( u ) );
 return( std::vector< int >( f_to.begin() + f_off[ u ] ,
                             f_to.begin() + f_off[ u + 1 ] ) );
 }

template< class W >
template< class F >
void ArrayGraph< W >::forEachAdj( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
  f( f_to[ k ] );
 }

template< class W >
template< class F >
void ArrayGraph< W >::forEachEdge( int u , F && f ) const
{
 ADS_PRE( hasNode( u ) );
 for( int k = f_off[ u ] ; k < f_off[ u + 1 ] ; ++k )
  f( f_to[ k ] , f_w[ k ] );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE VECTORS -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
template< class G >
std::vector< std::tuple< int , int , W > >
ArrayGraph< W >::edges( const G & g )
{
 std::vector< std::tuple< int , int , W > > E;
 for( int u : g.V() )
  g.forEachEdge( u , [ & ]( int v , const auto & w ) {
   if( g.directed() || ( u < v ) )
    E.emplace_back( u , v , W( w ) );
   } );
 return( E );
 }

template< class W >
bool ArrayGraph< W >::consistent( void ) const
{
 if( f_off.empty() || ( f_off[ 0 ] != 0 ) ||
     ( std::size_t( f_off.back() ) != f_to.size() ) ||
     ( f_to.size() != f_w.size() ) )
  return( false );
 for( std::size_t u = 0 ; u + 1 < f_off.size() ; ++u )
  if( f_off[ u ] > f_off[ u + 1 ] )
   return( false );
 for( int v : f_to )
  if( ! hasNode( v ) )
   return( false );
 return( f_to.size() == ( f_directed ? 1u : 2u ) * std::size_t( f_m ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ArrayGraph.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
