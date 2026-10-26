/*--------------------------------------------------------------------------*/
/*------------------------- File PointerGraph.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PointerGraph [see PointerGraph.h], with their
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

/*--------------------------------------------------------------------------*/
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class W >
typename PointerGraph< T , W >::Node *
PointerGraph< T , W >::insertNode( const T & t )
{
 f_V.push_back( Node{ t , 0 , false , {} } );
 ADS_INVARIANT( consistent() );
 return( &f_V.back() );
 }

template< class T , class W >
typename PointerGraph< T , W >::Node *
PointerGraph< T , W >::find( const T & t )
{
 for( auto & u : f_V ) if( u.label == t ) return( &u );
 return( nullptr );
 }

template< class T , class W >
bool PointerGraph< T , W >::hasEdge( const Node * u , const Node * v ) const
{
 ADS_PRE( ( u != nullptr ) && ( v != nullptr ) );
 ADS_AUDIT( owns( u ) && owns( v ) );
 for( const auto & e : u->out ) if( e.to == v ) return( true );
 return( false );
 }

template< class T , class W >
void PointerGraph< T , W >::insertEdge( Node * u , Node * v , const W & w )
{
 ADS_PRE( ( u != nullptr ) && ( v != nullptr ) );
 ADS_AUDIT( owns( u ) && owns( v ) );
 if( ! hasEdge( u , v ) ) {
  u->out.push_back( { v , w } );
  ++u->degree;
  ++v->degree;
  }
 ADS_POST( hasEdge( u , v ) );
 ADS_INVARIANT( consistent() );
 }

template< class T , class W >
void PointerGraph< T , W >::deleteEdge( Node * u , Node * v )
{
 ADS_PRE( ( u != nullptr ) && ( v != nullptr ) );
 ADS_AUDIT( owns( u ) && owns( v ) );
 for( auto it = u->out.begin() ; it != u->out.end() ; ++it )
  if( it->to == v ) {
   u->out.erase( it );
   --u->degree;
   --v->degree;
   break;
   }
 ADS_POST( ! hasEdge( u , v ) );
 ADS_INVARIANT( consistent() );
 }

template< class T , class W >
bool PointerGraph< T , W >::deleteNode( Node * u )
{
 ADS_PRE( u != nullptr );
 ADS_AUDIT( owns( u ) );
 if( u->degree > 0 ) return( false );
 f_V.remove_if( [ & ]( const Node & x ) { return( &x == u ); } );
 ADS_INVARIANT( consistent() );
 return( true );
 }

template< class T , class W >
void PointerGraph< T , W >::forceDeleteNode( Node * u )
{
 ADS_PRE( u != nullptr );
 ADS_AUDIT( owns( u ) );
 for( auto & x : f_V ) deleteEdge( &x , u );
 while( ! u->out.empty() ) deleteEdge( u , u->out.front().to );
 deleteNode( u );
 }

template< class T , class W >
std::vector< typename PointerGraph< T , W >::Node * >
PointerGraph< T , W >::adjacent( const Node * u ) const
{
 ADS_PRE( u != nullptr );
 ADS_AUDIT( owns( u ) );
 std::vector< Node * > A;
 for( const auto & e : u->out ) A.push_back( e.to );
 return( A );
 }

template< class T , class W >
template< class F >
void PointerGraph< T , W >::dfs( Node * r , F && f )
{
 ADS_PRE( r != nullptr );
 ADS_AUDIT( owns( r ) );
 clear();
 auto rec = [ & ]( auto & self , Node * u ) -> void {
  u->seen = true;
  f( u );
  for( auto & e : u->out ) if( ! e.to->seen ) self( self , e.to );
  };
 rec( rec , r );
 }

template< class T , class W >
template< class F >
void PointerGraph< T , W >::bfs( Node * r , F && f )
{
 ADS_PRE( r != nullptr );
 ADS_AUDIT( owns( r ) );
 clear();
 std::queue< Node * > Q;
 r->seen = true;
 Q.push( r );
 while( ! Q.empty() ) {
  Node * u = Q.front();
  Q.pop();
  f( u );
  for( auto & e : u->out )
   if( ! e.to->seen ) {
    e.to->seen = true;
    Q.push( e.to );
    }
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE NODES ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class W >
void PointerGraph< T , W >::clear( void )
{
 for( auto & u : f_V ) u.seen = false;
 }

template< class T , class W >
bool PointerGraph< T , W >::owns( const Node * u ) const
{
 for( const auto & x : f_V ) if( &x == u ) return( true );
 return( false );
 }

template< class T , class W >
bool PointerGraph< T , W >::consistent( void ) const
{
 std::vector< const Node * > nodes;
 for( const auto & x : f_V ) nodes.push_back( &x );
 std::sort( nodes.begin() , nodes.end() );
 std::vector< std::pair< const Node * , int > > in;
 for( const auto & x : f_V )
  for( const auto & e : x.out ) {
   if( ! std::binary_search( nodes.begin() , nodes.end() ,
                             static_cast< const Node * >( e.to ) ) )
    return( false );
   in.emplace_back( e.to , 1 );
   }
 std::sort( in.begin() , in.end() );
 for( const auto & x : f_V ) {
  const auto r = std::equal_range( in.begin() , in.end() ,
                                   std::pair< const Node * , int >( &x , 1 ) );
  if( x.degree != int( x.out.size() ) + int( r.second - r.first ) )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PointerGraph.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
