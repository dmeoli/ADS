/*--------------------------------------------------------------------------*/
/*----------------------- File RollbackUnionFind.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RollbackUnionFind [see RollbackUnionFind.h] and the
 * offline dynamic connectivity, with their contracts.
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

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS RollbackUnionFind -------------------------*/
/*--------------------------------------------------------------------------*/

inline RollbackUnionFind::RollbackUnionFind( int n )
{
 reset( n );
 }

inline void RollbackUnionFind::reset( int n )
{
 ADS_PRE( n >= 0 );
 const std::size_t m = std::size_t( n );
 f_p.resize( m );
 for( std::size_t x = 0 ; x < m ; ++x )
  f_p[ x ] = int( x );
 f_rank.assign( m , 0 );
 f_size.assign( m , 1 );
 f_stack.clear();
 f_count = int( m );
 }

inline int RollbackUnionFind::find( int x ) const
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 while( f_p[ x ] != x )
  x = f_p[ x ];
 return( x );
 }

inline bool RollbackUnionFind::merge( int x , int y )
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) && ( y >= 0 ) && ( y < n() ) );
 x = find( x );
 y = find( y );
 if( x == y ) {
  f_stack.push_back( { -1 , -1 , false } );
  return( false );
  }
 if( f_rank[ x ] > f_rank[ y ] )
  std::swap( x , y );                     // x of lower rank, under y
 const bool grew = f_rank[ x ] == f_rank[ y ];
 f_p[ x ] = y;
 f_size[ y ] += f_size[ x ];
 if( grew )
  ++f_rank[ y ];
 f_stack.push_back( { x , y , grew } );
 --f_count;
 ADS_INVARIANT( forest() );
 return( true );
 }

inline void RollbackUnionFind::undo( int k )
{
 ADS_PRE( ( k >= 0 ) && ( k <= checkpoint() ) );
 for( ; k > 0 ; --k ) {
  const Record r = f_stack.back();
  f_stack.pop_back();
  if( r.child < 0 )
   continue;
  f_p[ r.child ] = r.child;
  f_size[ r.parent ] -= f_size[ r.child ];
  if( r.grew )
   --f_rank[ r.parent ];
  ++f_count;
  }
 ADS_INVARIANT( forest() );
 }

inline void RollbackUnionFind::rollback( int t )
{
 ADS_PRE( ( t >= 0 ) && ( t <= checkpoint() ) );
 undo( checkpoint() - t );
 }

inline bool RollbackUnionFind::forest( void ) const
{
 const int n = this->n();
 std::vector< int > size( n , 0 );
 int roots = 0;
 for( int x = 0 ; x < n ; ++x ) {
  int r = x , h = 0;
  for( ; f_p[ r ] != r ; ++h ) {
   if( ( f_p[ r ] < 0 ) || ( f_p[ r ] >= n ) || ( h > n ) )
    return( false );
   r = f_p[ r ];
   }
  if( h > f_rank[ r ] )
   return( false );                       // the rank bounds the height
  ++size[ r ];
  }
 for( int x = 0 ; x < n ; ++x )
  if( f_p[ x ] == x ) {
   ++roots;
   if( size[ x ] != f_size[ x ] )
    return( false );
   }
 return( roots == f_count );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- OFFLINE DYNAMIC CONNECTIVITY ---------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< int > offlineConnectivity(
                           int n , const std::vector< ConnectivityOp > & ops )
{
 ADS_PRE( n >= 0 );
 using Op = ConnectivityOp;
 const int k = int( ops.size() );
 // before[ t ]: the questions before the time t
 std::vector< int > before( k + 1 , 0 );
 for( int t = 0 ; t < k ; ++t )
  before[ t + 1 ] = before[ t ] + ( ( ops[ t ].kind == Op::CONNECTED ) ||
                                    ( ops[ t ].kind == Op::COMPONENTS ) );
 const int q = before[ k ];
 std::vector< int > out( q , 0 );

 // the intervals of life of the edges, in the leaves of the questions
 struct Life {
  int u , v , s , e;
  };
 std::vector< Life > lives;
 std::unordered_map< std::uint64_t , std::vector< int > > open;
 auto key = []( int u , int v ) {
  if( v < u ) std::swap( u , v );
  return( ( std::uint64_t( std::uint32_t( u ) ) << 32 ) |
          std::uint32_t( v ) );
  };
 for( int t = 0 ; t < k ; ++t ) {
  const Op & o = ops[ t ];
  ADS_PRE( ( o.u >= 0 ) && ( o.u < n ) && ( o.v >= 0 ) && ( o.v < n ) );
  if( o.kind == Op::INSERT ) {
   open[ key( o.u , o.v ) ].push_back( int( lives.size() ) );
   lives.push_back( { o.u , o.v , before[ t ] , q } );
   }
  else if( o.kind == Op::DELETE ) {
   auto it = open.find( key( o.u , o.v ) );
   ADS_PRE( ( it != open.end() ) && ! it->second.empty() );
   lives[ it->second.back() ].e = before[ t ];
   it->second.pop_back();
   }
  }
 if( q == 0 )
  return( out );

 // the segment tree of the leaves [ 0 , q ), the node x of [ a , b ) with
 // the children 2 x and 2 x + 1
 std::vector< std::vector< int > > L( 4 * std::size_t( q ) );
 auto add = [ & ]( auto && self , int x , int a , int b , int s , int e ,
                   int id ) -> void {
  if( ( e <= a ) || ( b <= s ) )
   return;
  if( ( s <= a ) && ( b <= e ) ) {
   L[ x ].push_back( id );
   return;
   }
  const int c = ( a + b ) / 2;
  self( self , 2 * x , a , c , s , e , id );
  self( self , 2 * x + 1 , c , b , s , e , id );
  };
 for( int id = 0 ; id < int( lives.size() ) ; ++id )
  if( lives[ id ].s < lives[ id ].e )
   add( add , 1 , 0 , q , lives[ id ].s , lives[ id ].e , id );

 // the questions in the order of the leaves
 std::vector< const Op * > ask;
 ask.reserve( q );
 for( const auto & o : ops )
  if( ( o.kind == Op::CONNECTED ) || ( o.kind == Op::COMPONENTS ) )
   ask.push_back( & o );

 RollbackUnionFind U( n );
 auto visit = [ & ]( auto && self , int x , int a , int b ) -> void {
  const int t = U.checkpoint();
  for( int id : L[ x ] )
   U.merge( lives[ id ].u , lives[ id ].v );
  if( b - a == 1 ) {
   const Op & o = *ask[ a ];
   out[ a ] = o.kind == Op::COMPONENTS ? U.count() :
                                         int( U.same( o.u , o.v ) );
   }
  else {
   const int c = ( a + b ) / 2;
   self( self , 2 * x , a , c );
   self( self , 2 * x + 1 , c , b );
   }
  U.rollback( t );
  };
 visit( visit , 1 , 0 , q );
 return( out );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RollbackUnionFind.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
