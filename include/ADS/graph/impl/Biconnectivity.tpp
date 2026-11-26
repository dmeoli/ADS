/*--------------------------------------------------------------------------*/
/*------------------------ File Biconnectivity.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The blocks, the 2-edge-connected components and the block-cut tree of
 * Biconnectivity.h [see Biconnectivity.h], with their contracts.
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

#include "ADS/mfset/UnionFind.h"

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*------------------------- BiconnectedComponents --------------------------*/
/*--------------------------------------------------------------------------*/

inline BiconnectedComponents::BiconnectedComponents(
  std::vector< int > B , std::vector< int > r , std::vector< int > h ,
  std::vector< int > least , std::vector< std::uint8_t > bridge ,
  std::vector< std::uint8_t > cut )
    : f_B( std::move( B ) ) , f_rank( std::move( r ) ) ,
      f_head( std::move( h ) ) , f_least( std::move( least ) ) ,
      f_bridge( std::move( bridge ) ) , f_cut( std::move( cut ) )
{
 ADS_PRE( ( f_rank.size() == f_B.size() ) && ( f_cut.size() == f_B.size() ) &&
	  ( f_least.size() == f_head.size() ) &&
	  ( f_bridge.size() == f_head.size() ) );
 ADS_AUDIT( consistent() );
 }

inline int BiconnectedComponents::block( int u , int v ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) && ( v >= 0 ) && ( v < bound() ) );
 if( u == v )
  return( -1 );
 return( f_B[ std::size_t( f_rank[ u ] > f_rank[ v ] ? u : v ) ] );
 }

inline int BiconnectedComponents::blockOf( int u ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 return( f_B[ std::size_t( u ) ] );
 }

inline int BiconnectedComponents::head( int k ) const
{
 ADS_PRE( ( k >= 0 ) && ( k < blocks() ) );
 return( f_head[ std::size_t( k ) ] );
 }

inline int BiconnectedComponents::least( int k ) const
{
 ADS_PRE( ( k >= 0 ) && ( k < blocks() ) );
 return( f_least[ std::size_t( k ) ] );
 }

inline bool BiconnectedComponents::isBridge( int k ) const
{
 ADS_PRE( ( k >= 0 ) && ( k < blocks() ) );
 return( f_bridge[ std::size_t( k ) ] != 0 );
 }

inline bool BiconnectedComponents::isCut( int u ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 return( f_cut[ std::size_t( u ) ] != 0 );
 }

inline int BiconnectedComponents::rank( int u ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < bound() ) );
 return( f_rank[ std::size_t( u ) ] );
 }

inline std::vector< int > BiconnectedComponents::cutNodes( void ) const
{
 std::vector< int > c;
 for( int u = 0 ; u < bound() ; ++u )
  if( f_cut[ std::size_t( u ) ] )
   c.push_back( u );
 return( c );
 }

inline std::vector< std::pair< int , int > >
BiconnectedComponents::bridges( void ) const
{
 std::vector< std::pair< int , int > > e;
 for( int k = 0 ; k < blocks() ; ++k )
  if( f_bridge[ std::size_t( k ) ] )
   e.emplace_back( std::min( f_head[ k ] , f_least[ k ] ) ,
		   std::max( f_head[ k ] , f_least[ k ] ) );
 return( e );
 }

template< Graph G >
std::vector< int > BiconnectedComponents::arcBlocks( const G & g ) const
{
 ADS_PRE( g.bound() == bound() );
 const detail::Rows< G > A( g );
 const int n = bound();
 std::vector< int > out( A.first( n ) );
 for( int u = 0 ; u < n ; ++u )
  for( std::size_t i = A.first( u ) ; i < A.last( u ) ; ++i )
   out[ i ] = block( u , A.head( i ) );
 return( out );
 }

inline bool BiconnectedComponents::consistent( void ) const
{
 const int n = bound() , b = blocks();
 std::vector< int > heads( std::size_t( n ) , 0 );
 for( int k = 0 ; k < b ; ++k ) {
  const int l = f_least[ k ] , h = f_head[ k ];
  if( ( l < 0 ) || ( l >= n ) || ( f_B[ l ] != k ) ||
      ( ( k > 0 ) && ( l <= f_least[ k - 1 ] ) ) )
   return( false );
  if( ( h < 0 ) || ( h >= n ) || ( f_B[ h ] == k ) )
   return( false );
  ++heads[ h ];
  }
 for( int u = 0 ; u < n ; ++u ) {
  const int k = f_B[ u ];
  if( ( k < -1 ) || ( k >= b ) || ( ( k >= 0 ) && ( u < f_least[ k ] ) ) )
   return( false );
  // the blocks of u: the one in which it is not the head, and those it
  // heads
  const bool cut = ( k >= 0 ? 1 : 0 ) + heads[ u ] >= 2;
  if( cut != ( f_cut[ u ] != 0 ) )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE VISIT ---------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail
{

template< Graph G >
LowPoints lowPoints( const G & g , const Rows< G > & A )
{
 const int n = g.bound();
 LowPoints L;
 L.dt.assign( std::size_t( n ) , -1 );
 L.low.assign( std::size_t( n ) , -1 );
 L.p.assign( std::size_t( n ) , -1 );
 L.order.reserve( std::size_t( g.n() ) );
 // the recursion: the node, the parent whose edge is still to skip (-2
 // once skipped), the next arc
 struct Frame
 {
  int u , skip;
  std::size_t next;
  };
 std::vector< Frame > R;
 int t = 0;
 auto discover = [ & ]( int v , int parent ) {
  L.dt[ v ] = L.low[ v ] = t++;
  L.p[ v ] = parent;
  L.order.push_back( v );
  R.push_back( { v , parent , A.first( v ) } );
  };
 for( int r = 0 ; r < n ; ++r ) {
  if( ( L.dt[ r ] >= 0 ) || ! g.hasNode( r ) )
   continue;
  discover( r , -1 );
  while( ! R.empty() ) {
   Frame & f = R.back();
   const int u = f.u;
   if( f.next < A.last( u ) ) {
    const int v = A.head( f.next++ );
    if( v == f.skip )  // the edge to the parent, once
     f.skip = -2;
    else if( L.dt[ v ] < 0 )  // the call
     discover( v , u );
    else if( L.dt[ v ] < L.low[ u ] )
     L.low[ u ] = L.dt[ v ];
    continue;
    }
   R.pop_back();  // the return
   const int p = L.p[ u ];
   if( ( p >= 0 ) && ( L.low[ u ] < L.low[ p ] ) )
    L.low[ p ] = L.low[ u ];
   }
  }
 return( L );
 }

inline BiconnectedComponents
assembleBlocks( std::vector< int > B , std::vector< int > rank ,
		std::vector< int > h , std::vector< std::uint8_t > bridge )
{
 const std::size_t n = B.size() , b = h.size();
 // the blocks by increasing least node
 std::vector< int > id( b , -1 ) , least;
 least.reserve( b );
 for( std::size_t x = 0 ; x < n ; ++x ) {
  const int k = B[ x ];
  if( ( k >= 0 ) && ( id[ k ] < 0 ) ) {
   id[ k ] = int( least.size() );
   least.push_back( int( x ) );
   }
  }
 std::vector< int > head( b );
 std::vector< std::uint8_t > br( b );
 std::vector< int > heads( n , 0 );
 for( std::size_t k = 0 ; k < b ; ++k ) {
  head[ id[ k ] ] = h[ k ];
  br[ id[ k ] ] = bridge[ k ];
  ++heads[ h[ k ] ];
  }
 std::vector< std::uint8_t > cut( n , 0 );
 for( std::size_t x = 0 ; x < n ; ++x ) {
  if( B[ x ] >= 0 )
   B[ x ] = id[ B[ x ] ];
  cut[ x ] = ( B[ x ] >= 0 ? 1 : 0 ) + heads[ x ] >= 2;
  }
 return( BiconnectedComponents( std::move( B ) , std::move( rank ) ,
				 std::move( head ) , std::move( least ) ,
				 std::move( br ) , std::move( cut ) ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
BiconnectedComponents biconnectedComponents( const G & g )
{
 ADS_PRE( ! g.directed() );
 const detail::Rows< G > A( g );
 detail::LowPoints L = detail::lowPoints( g , A );
 const int n = g.bound();
 std::vector< int > B( std::size_t( n ) , -1 ) , h;
 std::vector< std::uint8_t > bridge;
 for( int u : L.order ) {  // the blocks from the top down
  const int p = L.p[ u ];
  if( p < 0 )
   continue;
  if( L.low[ u ] >= L.dt[ p ] ) {  // u opens a block under p
   B[ u ] = int( h.size() );
   h.push_back( p );
   bridge.push_back( L.low[ u ] > L.dt[ p ] );
   }
  else
   B[ u ] = B[ p ];
  }
 return( detail::assembleBlocks( std::move( B ) , std::move( L.dt ) ,
				  std::move( h ) , std::move( bridge ) ) );
 }

template< Graph G >
std::vector< int > twoEdgeConnectedComponents( const G & g )
{
 ADS_PRE( ! g.directed() );
 const detail::Rows< G > A( g );
 const detail::LowPoints L = detail::lowPoints( g , A );
 const int n = g.bound();
 std::vector< int > C( std::size_t( n ) , -1 ) , rep( std::size_t( n ) , -1 );
 for( int u : L.order ) {
  const int p = L.p[ u ];
  C[ u ] = ( ( p < 0 ) || ( L.low[ u ] > L.dt[ p ] ) ) ? u : C[ p ];
  }
 for( int x = 0 ; x < n ; ++x )  // the least node of each class
  if( ( C[ x ] >= 0 ) && ( rep[ C[ x ] ] < 0 ) )
   rep[ C[ x ] ] = x;
 for( int & c : C )
  if( c >= 0 )
   c = rep[ c ];
 return( C );
 }

inline BlockCutTree blockCutTree( const BiconnectedComponents & b )
{
 const int nb = b.blocks();
 BlockCutTree T;
 T.cuts = b.cutNodes();
 std::vector< int > at( std::size_t( b.bound() ) , -1 );
 for( std::size_t i = 0 ; i < T.cuts.size() ; ++i )
  at[ T.cuts[ i ] ] = int( i );
 std::vector< std::pair< int , int > > E;
 for( int k = 0 ; k < nb ; ++k )
  if( b.isCut( b.head( k ) ) )
   E.emplace_back( k , nb + at[ b.head( k ) ] );
 for( std::size_t i = 0 ; i < T.cuts.size() ; ++i )
  if( b.blockOf( T.cuts[ i ] ) >= 0 )
   E.emplace_back( b.blockOf( T.cuts[ i ] ) , nb + int( i ) );
 const int N = nb + int( T.cuts.size() );
 ADS_AUDIT( ( [ & ]( void ) {
  DisjointSets<> D( N );
  for( const auto & [ x , y ] : E )
   if( ! D.merge( x , y ) )
    return( false );
  return( true );
  }() ) );
 T.tree = StaticGraph<>( N , E , false );
 return( T );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File Biconnectivity.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
