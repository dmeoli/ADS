/*--------------------------------------------------------------------------*/
/*-------------------------- File Observable.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Observable [see Observable.h], the coalescing of the
 * events, with their contracts.
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

#include <unordered_map>
#include <unordered_set>

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------- THE EVENTS -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
std::pair< int , int > endsOf( const GraphEvent< W > & e )
{
 return( std::visit( []( const auto & x ) -> std::pair< int , int > {
  if constexpr( requires { x.v; } )
   return( std::pair< int , int >( x.u , x.v ) );
  else
   return( std::pair< int , int >( -1 , -1 ) ); } , e ) );
 }

namespace detail {

/// whether two weights are equal, never if W has no ==
template< class W >
bool sameWeight( const W & a , const W & b )
{
 if constexpr( std::equality_comparable< W > )
  return( a == b );
 else
  return( false );
 }

/// the key of the hash of an edge
inline std::uint64_t edgeKey( int u , int v )
{
 return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
         std::uint32_t( v ) );
 }

}  // end( namespace detail )

template< class W >
std::vector< GraphEvent< W > > coalesce(
                               std::span< const GraphEvent< W > > E )
{
 // the state of an edge before its first event and after its last one
 struct Net {
  int u , v;
  std::optional< W > before , after;
  };
 std::vector< Net > nets;
 std::unordered_map< std::uint64_t , std::size_t > at;
 std::vector< int > ins , del;
 std::unordered_set< int > born;
 std::unordered_set< int > gone;
 for( const auto & e : E ) {
  if( const auto * x = std::get_if< NodeInserted >( & e ) ) {
   ins.push_back( x->u );
   born.insert( x->u );
   continue;
   }
  if( const auto * x = std::get_if< NodeDeleted >( & e ) ) {
   if( born.count( x->u ) )
    gone.insert( x->u );                 // inserted and deleted: no trace
   else
    del.push_back( x->u );
   continue;
   }
  const auto [ u , v ] = endsOf( e );
  std::optional< W > before , after;
  if( const auto * x = std::get_if< EdgeInserted< W > >( & e ) )
   after = x->w;
  else if( const auto * x = std::get_if< EdgeDeleted< W > >( & e ) )
   before = x->w;
  else {
   const auto & c = std::get< WeightChanged< W > >( e );
   before = c.from;
   after = c.to;
   }
  const auto [ it , fresh ] = at.try_emplace( detail::edgeKey( u , v ) ,
                                              nets.size() );
  if( fresh )
   nets.push_back( { u , v , before , after } );
  else
   nets[ it->second ].after = after;
  }

 std::vector< GraphEvent< W > > out;
 out.reserve( ins.size() + nets.size() + del.size() );
 for( int u : ins )
  if( ! gone.count( u ) )
   out.emplace_back( NodeInserted{ u } );
 for( const auto & x : nets ) {
  if( gone.count( x.u ) || gone.count( x.v ) )
   continue;
  if( ! x.before && x.after )
   out.emplace_back( EdgeInserted< W >{ x.u , x.v , *x.after } );
  else if( x.before && ! x.after )
   out.emplace_back( EdgeDeleted< W >{ x.u , x.v , *x.before } );
  else if( x.before && x.after &&
           ! detail::sameWeight( *x.before , *x.after ) )
   out.emplace_back( WeightChanged< W >{ x.u , x.v , *x.before ,
                                         *x.after } );
  }
 for( int u : del )
  out.emplace_back( NodeDeleted{ u } );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS Observable -----------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G , class ... O >
Observable< G , O... >::Observable( G & g , O & ... o )
 : f_g( g ) , f_static( o... )
{
 remask();
 }

template< Graph G , class ... O >
void Observable< G , O... >::remask( void )
{
 f_mask = 0;
 std::apply( [ & ]( auto & ... o ) {
  ( ( f_mask |= unsigned( o.mask() ) ) , ... ); } , f_static );
 for( const auto & s : f_dynamic )
  if( s.o )
   f_mask |= s.mask;
 }

template< Graph G , class ... O >
std::pair< int , int > Observable< G , O... >::key( int u , int v ) const
{
 if( ! f_g.directed() && ( v < u ) )
  return( std::pair< int , int >( v , u ) );
 return( std::pair< int , int >( u , v ) );
 }

/*---------------------------- THE CHANGES ---------------------------------*/

template< Graph G , class ... O >
int Observable< G , O... >::insertNode( void ) requires DynamicGraph< G >
{
 const int u = f_g.insertNode();
 if( listening( GraphEvents::NODE_INSERT ) )
  emit( NodeInserted{ u } );
 return( u );
 }

template< Graph G , class ... O >
void Observable< G , O... >::insertEdge( int u , int v , const Weight & w )
 requires DynamicGraph< G >
{
 if( ! listening( GraphEvents::EDGE_INSERT | GraphEvents::WEIGHT ) ) {
  f_g.insertEdge( u , v , w );
  return;
  }
 const std::optional< Weight > old = f_g.weight( u , v );
 f_g.insertEdge( u , v , w );
 const auto [ a , b ] = key( u , v );
 if( ! old ) {
  if( listening( GraphEvents::EDGE_INSERT ) )
   emit( EdgeInserted< Weight >{ a , b , w } );
  }
 else if( listening( GraphEvents::WEIGHT ) )
  emit( WeightChanged< Weight >{ a , b , *old , w } );
 }

template< Graph G , class ... O >
void Observable< G , O... >::deleteEdge( int u , int v )
 requires DynamicGraph< G >
{
 if( ! listening( GraphEvents::EDGE_DELETE ) ) {
  f_g.deleteEdge( u , v );
  return;
  }
 const std::optional< Weight > old = f_g.weight( u , v );
 f_g.deleteEdge( u , v );
 if( old ) {
  const auto [ a , b ] = key( u , v );
  emit( EdgeDeleted< Weight >{ a , b , *old } );
  }
 }

template< Graph G , class ... O >
void Observable< G , O... >::deleteNode( int u ) requires DynamicGraph< G >
{
 ADS_PRE( f_g.hasNode( u ) );
 if( listening( GraphEvents::EDGE_DELETE ) ) {
  // the edges of u, out of it and, if oriented, into it:
  // \f$ O( n + m ) \f$, as the deletion of the node in g
  std::vector< std::pair< int , Weight > > out , in;
  f_g.forEachEdge( u , [ & ]( int v , const Weight & w ) {
   out.emplace_back( v , w ); } );
  if( f_g.directed() )
   for( int x : f_g.V() )
    if( x != u )
     if( const auto w = f_g.weight( x , u ) )
      in.emplace_back( x , *w );
  f_g.deleteNode( u );
  for( const auto & [ v , w ] : out ) {
   const auto [ a , b ] = key( u , v );
   emit( EdgeDeleted< Weight >{ a , b , w } );
   }
  for( const auto & [ x , w ] : in )
   emit( EdgeDeleted< Weight >{ x , u , w } );
  }
 else
  f_g.deleteNode( u );
 if( listening( GraphEvents::NODE_DELETE ) )
  emit( NodeDeleted{ u } );
 }

template< Graph G , class ... O >
bool Observable< G , O... >::setWeight( int u , int v , const Weight & w )
{
 ADS_PRE( f_g.hasNode( u ) && f_g.hasNode( v ) );
 const bool hear = listening( GraphEvents::WEIGHT );
 std::optional< Weight > old;
 if constexpr( requires { f_g.setWeight( u , v , w ); } ) {
  if( hear )
   old = f_g.weight( u , v );
  if( ! f_g.setWeight( u , v , w ) )
   return( false );
  }
 else {
  old = f_g.weight( u , v );
  if( ! old )
   return( false );
  f_g.insertEdge( u , v , w );          // only the weight changes
  }
 if( hear ) {
  const auto [ a , b ] = key( u , v );
  emit( WeightChanged< Weight >{ a , b , *old , w } );
  }
 return( true );
 }

/*--------------------------- THE OBSERVERS --------------------------------*/

template< Graph G , class ... O >
void Observable< G , O... >::detach( int id )
{
 ADS_PRE( ( f_depth == 0 ) && ( id >= 0 ) &&
          ( id < int( f_dynamic.size() ) ) && f_dynamic[ id ].o );
 f_dynamic[ id ].o = nullptr;
 remask();
 }

template< Graph G , class ... O >
void Observable< G , O... >::emit( Event && e )
{
 ++f_built;
 if( f_depth > 0 ) {
  f_batch.push_back( std::move( e ) );
  return;
  }
 const unsigned k = kindOf( e );
 std::apply( [ & ]( auto & ... o ) {
  ( [ & ]( auto & x ) {
     if( unsigned( x.mask() ) & k )
      x.notify( std::as_const( e ) ); }( o ) , ... ); } , f_static );
 for( const auto & s : f_dynamic )
  if( s.o && ( s.mask & k ) )
   s.one( s.o , e );
 }

template< Graph G , class ... O >
template< class F >
void Observable< G , O... >::deliver( F && send , unsigned m ,
                                      std::span< const Event > E ,
                                      unsigned kinds ,
                                      std::vector< Event > & tmp )
{
 if( ( m & kinds ) == 0 )
  return;
 if( ( kinds & ~m ) == 0 ) {            // it wants all of them
  send( E );
  return;
  }
 tmp.clear();
 for( const auto & e : E )
  if( m & kindOf( e ) )
   tmp.push_back( e );
 if( ! tmp.empty() )
  send( std::span< const Event >( tmp ) );
 }

template< Graph G , class ... O >
void Observable< G , O... >::closeBatch( void )
{
 ADS_PRE( f_depth > 0 );
 if( --f_depth > 0 )
  return;
 if( f_batch.empty() )
  return;
 const std::vector< Event > C = coalesce< Weight >(
                                std::span< const Event >( f_batch ) );
 f_batch.clear();
 if( C.empty() )
  return;
 unsigned kinds = 0;
 for( const auto & e : C )
  kinds |= kindOf( e );
 const std::span< const Event > S( C );
 std::vector< Event > tmp;
 std::apply( [ & ]( auto & ... o ) {
  ( deliver( [ & ]( std::span< const Event > X ) { o.notify( X ); } ,
             unsigned( o.mask() ) , S , kinds , tmp ) , ... ); } ,
  f_static );
 for( const auto & s : f_dynamic )
  if( s.o )
   deliver( [ & ]( std::span< const Event > X ) { s.many( s.o , X ); } ,
            s.mask , S , kinds , tmp );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Observable.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
