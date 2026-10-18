/*--------------------------------------------------------------------------*/
/*--------------------------- File ListGraph.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ListGraph [see ListGraph.h], with their contracts.
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
ListGraph< W , T >::ListGraph( int n , bool directed )
 : B( n , directed ) , f_adj( n )
{
 ADS_INVARIANT( consistent() );
 }

template< class W , class T >
int ListGraph< W , T >::insertNode( const T & t )
{
 f_adj.emplace_back();
 const int u = B::addNode( t );
 ADS_POST( f_adj[ u ].isEmpty() );
 return( u );
 }

template< class W , class T >
void ListGraph< W , T >::insertEdge( int u , int v , const W & w )
{
 B::edge( u , v );
 if( ! set( u , v , w ) ) {
  f_adj[ u ].insert( f_adj[ u ].next( f_adj[ u ].tail() ) , { v , w } );
  if( ! B::f_directed )
   f_adj[ v ].insert( f_adj[ v ].next( f_adj[ v ].tail() ) , { u , w } );
  ++B::f_m;
  }
 else if( ! B::f_directed )
  set( v , u , w );
 ADS_POST( hasEdge( u , v ) );
 ADS_INVARIANT( consistentAt( u , v ) && consistentAt( v , u ) );
 }

template< class W , class T >
void ListGraph< W , T >::deleteEdge( int u , int v )
{
 B::edge( u , v );
 if( erase( u , v ) ) {
  if( ! B::f_directed )
   erase( v , u );
  --B::f_m;
  }
 ADS_POST( ! hasEdge( u , v ) );
 ADS_INVARIANT( consistentAt( u , v ) && consistentAt( v , u ) );
 }

template< class W , class T >
void ListGraph< W , T >::deleteNode( int u )
{
 B::check( u );
 for( int v = 0 ; v < B::bound() ; ++v )
  if( B::f_alive[ v ] && ( v != u ) && erase( v , u ) && B::f_directed )
   --B::f_m;
 for( auto p = f_adj[ u ].head() ; ! f_adj[ u ].finished( p ) ;
      p = f_adj[ u ].remove( p ) )
  --B::f_m;
 B::f_alive[ u ] = false;
 --B::f_n;
 ADS_POST( ! B::hasNode( u ) );
 ADS_INVARIANT( consistent() );
 }

template< class W , class T >
bool ListGraph< W , T >::hasEdge( int u , int v ) const
{
 return( B::hasNode( u ) && B::hasNode( v ) && find( u , v ) );
 }

template< class W , class T >
std::optional< W > ListGraph< W , T >::weight( int u , int v ) const
{
 if( ! B::hasNode( u ) )
  return( std::nullopt );
 const auto & L = f_adj[ u ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  if( L.read( p ).first == v )
   return( L.read( p ).second );
 return( std::nullopt );
 }

template< class W , class T >
std::vector< int > ListGraph< W , T >::adj( int u ) const
{
 std::vector< int > out;
 forEachAdj( u , [ & ]( int v ) { out.push_back( v ); } );
 return( out );
 }

template< class W , class T >
template< class F >
void ListGraph< W , T >::forEachAdj( int u , F && f ) const
{
 const auto & L = f_adj[ B::check( u ) ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  f( L.read( p ).first );
 }

template< class W , class T >
template< class F >
void ListGraph< W , T >::forEachEdge( int u , F && f ) const
{
 const auto & L = f_adj[ B::check( u ) ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  f( L.read( p ).first , L.read( p ).second );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE LISTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W , class T >
bool ListGraph< W , T >::find( int u , int v ) const
{
 return( weight( u , v ).has_value() );
 }

template< class W , class T >
bool ListGraph< W , T >::set( int u , int v , const W & w )
{
 auto & L = f_adj[ u ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  if( L.read( p ).first == v ) {
   L.write( p , { v , w } );
   return( true );
   }
 return( false );
 }

template< class W , class T >
bool ListGraph< W , T >::erase( int u , int v )
{
 auto & L = f_adj[ u ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  if( L.read( p ).first == v ) {
   L.remove( p );
   return( true );
   }
 return( false );
 }

template< class W , class T >
bool ListGraph< W , T >::consistent( void ) const
{
 if( ! B::nodesCounted() || ( f_adj.size() != B::f_alive.size() ) )
  return( false );
 long count = 0;
 std::vector< int > seen( B::bound() , -1 );
 for( int u = 0 ; u < B::bound() ; ++u ) {
  const auto & L = f_adj[ u ];
  if( ! B::f_alive[ u ] ) {
   if( ! L.isEmpty() ) return( false );
   continue;
   }
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) ) {
   const int v = L.read( p ).first;
   if( ! B::hasNode( v ) || ( v == u ) || ( seen[ v ] == u ) )
    return( false );
   seen[ v ] = u;
   if( ! B::f_directed ) {
    const auto w = weight( v , u );
    if( ! w ) return( false );
    if constexpr( std::equality_comparable< W > )
     if( !( *w == L.read( p ).second ) ) return( false );
    }
   ++count;
   }
  }
 return( count == ( B::f_directed ? 1 : 2 ) * long( B::f_m ) );
 }

template< class W , class T >
bool ListGraph< W , T >::consistentAt( int u , int v ) const
{
 std::vector< int > A;
 for( auto p = f_adj[ u ].head() ; ! f_adj[ u ].finished( p ) ;
      p = f_adj[ u ].next( p ) )
  A.push_back( f_adj[ u ].read( p ).first );
 std::sort( A.begin() , A.end() );
 if( std::adjacent_find( A.begin() , A.end() ) != A.end() )
  return( false );
 for( int x : A )
  if( ( x == u ) || ! B::hasNode( x ) )
   return( false );
 if( B::f_directed )
  return( true );
 const auto a = weight( u , v ) , b = weight( v , u );
 if( a.has_value() != b.has_value() )
  return( false );
 if constexpr( std::equality_comparable< W > )
  if( a && !( *a == *b ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File ListGraph.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
