/*--------------------------------------------------------------------------*/
/*-------------------------- File Observable.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ObservedGraph [see Observable.h], the coalescing of
 * the events, with their contracts.
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

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------------- THE EVENTS -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
std::pair< int , int > endsOf( const GraphEvent< W > & e )
{
 return( std::visit(
   []( const auto & x ) -> std::pair< int , int > {
    if constexpr( requires { x.v; } )
     return( std::pair< int , int >( x.u , x.v ) );
    else
     return( std::pair< int , int >( -1 , -1 ) );
    } ,
   e ) );
 }

namespace detail
{

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
 return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) | std::uint32_t( v ) );
 }

}  // end( namespace detail )

template< class W >
std::vector< GraphEvent< W > >
coalesce( std::span< const GraphEvent< W > > E )
{
 // the state of an edge before its first event and after its last one
 struct Net
 {
  int u , v;
  std::optional< W > before , after;
  };
 std::vector< Net > nets;
 std::unordered_map< std::uint64_t , std::size_t > at;
 std::vector< int > ins , del;
 std::unordered_set< int > born;
 std::unordered_set< int > gone;
 for( const auto & e : E ) {
  if( const auto * x = std::get_if< NodeInserted >( &e ) ) {
   ins.push_back( x->u );
   born.insert( x->u );
   continue;
   }
  if( const auto * x = std::get_if< NodeDeleted >( &e ) ) {
   if( born.count( x->u ) )
    gone.insert( x->u );  // inserted and deleted: no trace
   else
    del.push_back( x->u );
   continue;
   }
  const auto [ u , v ] = endsOf( e );
  std::optional< W > before , after;
  if( const auto * x = std::get_if< EdgeInserted< W > >( &e ) )
   after = x->w;
  else if( const auto * x = std::get_if< EdgeDeleted< W > >( &e ) )
   before = x->w;
  else {
   const auto & c = std::get< WeightChanged< W > >( e );
   before = c.from;
   after = c.to;
   }
  const auto [ it , fresh ] =
    at.try_emplace( detail::edgeKey( u , v ) , nets.size() );
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
  else if( x.before && x.after && ! detail::sameWeight( *x.before ,
							*x.after ) )
   out.emplace_back( WeightChanged< W >{ x.u , x.v , *x.before , *x.after } );
  }
 for( int u : del )
  out.emplace_back( NodeDeleted{ u } );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS ObservedGraph ---------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G , class Policy >
void ObservedGraph< G , Policy >::insertEdgeTold( int u , int v ,
						 const Weight & w )
{
 const std::optional< Weight > old = f_g.weight( u , v );
 f_g.insertEdge( u , v , w );
 const auto [ a , b ] = key( u , v );
 if( ! old ) {
  if constexpr( Subject::template listened< EdgeInserted< Weight > > )
   if( f_sub.template live< EdgeInserted< Weight > >() )
    tell( EdgeInserted< Weight >{ a , b , w } );
  }
 else if constexpr( Subject::template listened< WeightChanged< Weight > > )
  if( f_sub.template live< WeightChanged< Weight > >() )
   tell( WeightChanged< Weight >{ a , b , *old , w } );
 }

template< Graph G , class Policy >
void ObservedGraph< G , Policy >::deleteEdgeTold( int u , int v )
{
 const std::optional< Weight > old = f_g.weight( u , v );
 f_g.deleteEdge( u , v );
 if( old ) {
  const auto [ a , b ] = key( u , v );
  tell( EdgeDeleted< Weight >{ a , b , *old } );
  }
 }

template< Graph G , class Policy >
void ObservedGraph< G , Policy >::deleteNode( int u )
 requires DynamicGraph< G >
{
 ADS_PRE( f_g.hasNode( u ) );
 if constexpr( Subject::template listened< EdgeDeleted< Weight > > ) {
  if( f_sub.template live< EdgeDeleted< Weight > >() ) {
   // the edges of u, out of it and, if oriented, into it:
   // \f$ O( n + m ) \f$, as the deletion of the node in g
   std::vector< std::pair< int , Weight > > out , in;
   f_g.forEachEdge(
     u , [ & ]( int v , const Weight & w ) { out.emplace_back( v , w ); } );
   if( f_g.directed() )
    for( int x : f_g.V() )
     if( x != u )
      if( const auto w = f_g.weight( x , u ) )
       in.emplace_back( x , *w );
   f_g.deleteNode( u );
   for( const auto & [ v , w ] : out ) {
    const auto [ a , b ] = key( u , v );
    tell( EdgeDeleted< Weight >{ a , b , w } );
    }
   for( const auto & [ x , w ] : in )
    tell( EdgeDeleted< Weight >{ x , u , w } );
   }
  else
   f_g.deleteNode( u );
  }
 else
  f_g.deleteNode( u );
 if constexpr( Subject::template listened< NodeDeleted > )
  if( f_sub.template live< NodeDeleted >() )
   tell( NodeDeleted{ u } );
 }

template< Graph G , class Policy >
bool ObservedGraph< G , Policy >::setWeightTold( int u , int v ,
						const Weight & w )
{
 std::optional< Weight > old = f_g.weight( u , v );
 if constexpr( requires { f_g.setWeight( u , v , w ); } ) {
  if( ! f_g.setWeight( u , v , w ) )
   return( false );
  }
 else {
  if( ! old )
   return( false );
  f_g.insertEdge( u , v , w );  // only the weight changes
  }
 const auto [ a , b ] = key( u , v );
 tell( WeightChanged< Weight >{ a , b , *old , w } );
 return( true );
 }

template< Graph G , class Policy >
bool ObservedGraph< G , Policy >::revertible( std::size_t t ) const
 requires( ! Subject::none )
{
 const auto B = f_sub.collected();
 if( ( t > B.size() ) || ( f_sub.mask() != GraphEvents::ALL ) )
  return( false );
 for( std::size_t i = t ; i < B.size() ; ++i )
  if( ( kindOf( B[ i ] ) & GraphEvents::EDGES ) == 0 )
   return( false );  // a node: no inverse
 return( true );
 }

template< Graph G , class Policy >
void ObservedGraph< G , Policy >::rollback( std::size_t t )
 requires( ! Subject::none )
{
 ADS_PRE( revertible( t ) );
 const auto B = f_sub.collected();
 const std::vector< Event > R( B.begin() + std::ptrdiff_t( t ) , B.end() );
 for( auto it = R.rbegin() ; it != R.rend() ; ++it )
  std::visit(
    [ & ]( const auto & x ) {
     if constexpr( mod::invertible< std::decay_t< decltype( x ) > > )
      apply( inverse( x ) );
     } ,
    *it );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Observable.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
