/*--------------------------------------------------------------------------*/
/*-------------------------- File PackedGraph.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PackedGraph [see PackedGraph.h], with their contracts.
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

namespace ADS
{

template< class T >
PackedGraph< T >::PackedGraph( int n , bool directed ) : B( n , directed )
{
 ADS_PRE( std::int64_t( n ) < ( std::int64_t( 1 ) << 31 ) );
 }

template< class T >
int PackedGraph< T >::insertNode( const T & t )
{
 return( B::addNode( t ) );
 }

template< class T >
void PackedGraph< T >::insertEdge( int u , int v )
{
 B::edge( u , v );
 if( f_arcs.insert( key( u , v ) ) ) {
  if( ! B::f_directed )
   f_arcs.insert( key( v , u ) );
  ++B::f_m;
  }
 ADS_POST( hasEdge( u , v ) );
 }

template< class T >
void PackedGraph< T >::deleteEdge( int u , int v )
{
 B::edge( u , v );
 if( f_arcs.erase( key( u , v ) ) ) {
  if( ! B::f_directed )
   f_arcs.erase( key( v , u ) );
  --B::f_m;
  }
 ADS_POST( ! hasEdge( u , v ) );
 }

template< class T >
void PackedGraph< T >::deleteNode( int u )
{
 B::check( u );
 std::vector< std::uint64_t > D;
 forEachAdj( u , [ & ]( int v ) {
  D.push_back( key( u , v ) );
  if( ! B::f_directed )
   D.push_back( key( v , u ) );
  } );
 std::size_t in = 0;
 if( B::f_directed )  // the arcs that enter u, by a visit of all
  f_arcs.forEach( [ & ]( std::uint64_t a ) {
   if( std::uint32_t( a ) == std::uint32_t( u ) ) {
    D.push_back( a );
    ++in;
    }
   } );
 const std::size_t out = B::f_directed ? D.size() - in : D.size() / 2;
 f_arcs.eraseBatch( std::move( D ) );
 B::f_m -= int( B::f_directed ? out + in : out );
 B::f_alive[ u ] = false;
 --B::f_n;
 ADS_POST( ! B::hasNode( u ) );
 ADS_INVARIANT( consistent() );
 }

template< class T >
bool PackedGraph< T >::hasEdge( int u , int v ) const
{
 return( B::hasNode( u ) && B::hasNode( v ) &&
	  f_arcs.contains( key( u , v ) ) );
 }

template< class T >
std::vector< int > PackedGraph< T >::adj( int u ) const
{
 std::vector< int > A;
 forEachAdj( u , [ & ]( int v ) { A.push_back( v ); } );
 return( A );
 }

template< class T >
template< class F >
void PackedGraph< T >::forEachAdj( int u , F && f ) const
{
 B::check( u );
 f_arcs.forEach( key( u , 0 ) , key( u + 1 , 0 ) , [ & ]( std::uint64_t a ) {
  f( int( std::uint32_t( a ) ) );
  } );
 }

template< class T >
std::vector< std::uint64_t > PackedGraph< T >::keys(
  const std::vector< std::pair< int , int > > & A ) const
{
 std::vector< std::uint64_t > K;
 K.reserve( B::f_directed ? A.size() : 2 * A.size() );
 for( const auto & [ u , v ] : A ) {
  B::edge( u , v );
  K.push_back( key( u , v ) );
  if( ! B::f_directed )
   K.push_back( key( v , u ) );
  }
 return( K );
 }

template< class T >
template< Executor E >
std::size_t PackedGraph< T >::insertEdges(
  E & ex , const std::vector< std::pair< int , int > > & A )
{
 const std::size_t a = f_arcs.insertBatch( ex , keys( A ) );
 const std::size_t e = B::f_directed ? a : a / 2;
 B::f_m += int( e );
 ADS_INVARIANT( consistent() );
 return( e );
 }

template< class T >
std::size_t PackedGraph< T >::insertEdges(
  const std::vector< std::pair< int , int > > & A )
{
 Sequential s;
 return( insertEdges( s , A ) );
 }

template< class T >
template< Executor E >
std::size_t PackedGraph< T >::deleteEdges(
  E & ex , const std::vector< std::pair< int , int > > & A )
{
 const std::size_t a = f_arcs.eraseBatch( ex , keys( A ) );
 const std::size_t e = B::f_directed ? a : a / 2;
 B::f_m -= int( e );
 ADS_INVARIANT( consistent() );
 return( e );
 }

template< class T >
std::size_t PackedGraph< T >::deleteEdges(
  const std::vector< std::pair< int , int > > & A )
{
 Sequential s;
 return( deleteEdges( s , A ) );
 }

template< class T >
std::size_t PackedGraph< T >::bytes( void ) const
{
 return( f_arcs.bytes() + B::f_alive.capacity() / 8 +
	  B::f_label.capacity() * sizeof( T ) );
 }

template< class T >
bool PackedGraph< T >::consistent( void ) const
{
 bool ok = true;
 f_arcs.forEach( [ & ]( std::uint64_t a ) {
  const int u = int( a >> 32 ) , v = int( std::uint32_t( a ) );
  ok = ok && ( u != v ) && B::hasNode( u ) && B::hasNode( v ) &&
       ( B::f_directed || f_arcs.contains( key( v , u ) ) );
  } );
 return( ok && ( f_arcs.size() ==
		  std::size_t( B::f_directed ? B::f_m : 2 * B::f_m ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File PackedGraph.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
