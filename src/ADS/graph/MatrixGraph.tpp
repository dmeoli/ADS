/*--------------------------------------------------------------------------*/
/*-------------------------- File MatrixGraph.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MatrixGraph [see MatrixGraph.h], with their contracts.
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

template< class W , class T >
MatrixGraph< W , T >::MatrixGraph( int n , bool directed )
 : B( n , directed ) , f_M( n , std::vector< std::optional< W > >( n ) )
{
 ADS_INVARIANT( consistent() );
 }

template< class W , class T >
int MatrixGraph< W , T >::insertNode( const T & t )
{
 for( auto & r : f_M )
  r.emplace_back();
 f_M.emplace_back( f_M.size() + 1 );
 const int u = B::addNode( t );
 ADS_INVARIANT( consistentAt( u ) );
 return( u );
 }

template< class W , class T >
void MatrixGraph< W , T >::insertEdge( int u , int v , const W & w )
{
 B::edge( u , v );
 if( ! f_M[ u ][ v ] )
  ++B::f_m;
 f_M[ u ][ v ] = w;
 if( ! B::f_directed )
  f_M[ v ][ u ] = w;
 ADS_POST( hasEdge( u , v ) );
 ADS_INVARIANT( consistentAt( u ) && consistentAt( v ) );
 }

template< class W , class T >
void MatrixGraph< W , T >::deleteEdge( int u , int v )
{
 B::edge( u , v );
 if( f_M[ u ][ v ] )
  --B::f_m;
 f_M[ u ][ v ].reset();
 if( ! B::f_directed )
  f_M[ v ][ u ].reset();
 ADS_POST( ! hasEdge( u , v ) );
 ADS_INVARIANT( consistentAt( u ) && consistentAt( v ) );
 }

template< class W , class T >
void MatrixGraph< W , T >::deleteNode( int u )
{
 B::check( u );
 for( int v = 0 ; v < B::bound() ; ++v )
  if( B::f_alive[ v ] && ( v != u ) ) {
   if( f_M[ u ][ v ] ) deleteEdge( u , v );
   if( f_M[ v ][ u ] ) deleteEdge( v , u );
   }
 B::f_alive[ u ] = false;
 --B::f_n;
 ADS_POST( ! B::hasNode( u ) );
 ADS_INVARIANT( consistentAt( u ) );
 }

template< class W , class T >
bool MatrixGraph< W , T >::hasEdge( int u , int v ) const
{
 return( B::hasNode( u ) && B::hasNode( v ) && f_M[ u ][ v ] );
 }

template< class W , class T >
std::optional< W > MatrixGraph< W , T >::weight( int u , int v ) const
{
 if( ! B::hasNode( u ) || ! B::hasNode( v ) )
  return( std::nullopt );
 return( f_M[ u ][ v ] );
 }

template< class W , class T >
std::vector< int > MatrixGraph< W , T >::adj( int u ) const
{
 std::vector< int > out;
 forEachAdj( u , [ & ]( int v ) { out.push_back( v ); } );
 return( out );
 }

template< class W , class T >
template< class F >
void MatrixGraph< W , T >::forEachAdj( int u , F && f ) const
{
 const auto & r = f_M[ B::check( u ) ];
 for( int v = 0 ; v < B::bound() ; ++v )
  if( r[ v ] )
   f( v );
 }

template< class W , class T >
template< class F >
void MatrixGraph< W , T >::forEachEdge( int u , F && f ) const
{
 const auto & r = f_M[ B::check( u ) ];
 for( int v = 0 ; v < B::bound() ; ++v )
  if( r[ v ] )
   f( v , *r[ v ] );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W , class T >
bool MatrixGraph< W , T >::consistent( void ) const
{
 const int b = B::bound();
 if( ! B::nodesCounted() || ( int( f_M.size() ) != b ) )
  return( false );
 long count = 0;
 for( int u = 0 ; u < b ; ++u ) {
  if( int( f_M[ u ].size() ) != b )
   return( false );
  for( int v = 0 ; v < b ; ++v ) {
   if( ! f_M[ u ][ v ] )
    continue;
   if( ( u == v ) || ! B::f_alive[ u ] || ! B::f_alive[ v ] )
    return( false );
   if( ! B::f_directed && ! f_M[ v ][ u ] )
    return( false );
   ++count;
   }
  }
 return( count == ( B::f_directed ? 1 : 2 ) * long( B::f_m ) );
 }

template< class W , class T >
bool MatrixGraph< W , T >::consistentAt( int u ) const
{
 const int b = B::bound();
 if( ( int( f_M.size() ) != b ) || ( int( f_M[ u ].size() ) != b ) ||
     f_M[ u ][ u ] )
  return( false );
 for( int v = 0 ; v < b ; ++v ) {
  const bool out = f_M[ u ][ v ].has_value() , in = f_M[ v ][ u ].has_value();
  if( ( out || in ) && ( ! B::f_alive[ u ] || ! B::f_alive[ v ] ) )
   return( false );
  if( ! B::f_directed && ( out != in ) )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File MatrixGraph.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
