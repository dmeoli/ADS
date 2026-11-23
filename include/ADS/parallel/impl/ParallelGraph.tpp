/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelGraph.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel visit in breadth and the connected components of
 * ParallelGraph.h.
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

#include <numeric>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace ADS
{

namespace detail
{

/// the concatenation of the vectors that f( b ) gives for the blocks
/// b = 0 , ... , B - 1, in their order
template< class E , class F >
auto gatherBlocks( E & ex , std::size_t B , F && f )
{
 using V = std::invoke_result_t< F & , std::size_t >;
 std::vector< V > L( B );
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) { L[ b ] = f( b ); } , 1 );
 return( flatten( ex , L ) );
 }

inline int atomicLoad( int & x )
{
 return( std::atomic_ref< int >( x ).load( std::memory_order_relaxed ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , class W , ArcLayout AL >
std::vector< int > bfsDistances( E & ex , const StaticGraph< W , AL > & g ,
				 int s ,
				 const StaticGraph< W , AL > * gt )
{
 using G = StaticGraph< W , AL >;
 ADS_PRE( g.hasNode( s ) );
 ADS_PRE( ( gt == nullptr ) || ( gt->n() == g.n() ) );
 const int n = g.n();
 const int * off = g.offsets();
 const auto hd = g.arcHeads();
 const G * in = g.directed() ? gt : &g;
 std::vector< int > dist( std::size_t( n ) , -1 );
 dist[ std::size_t( s ) ] = 0;
 const std::size_t words = ( std::size_t( n ) + 63 ) / 64;
 std::vector< std::uint64_t > cur , nxt;
 std::vector< int > F{ s };
 std::int64_t toCheck = g.arcs() , scout = g.degree( s );
 int level = 0;
 for( ; ; ) {
  if( ( in != nullptr ) && ( scout > toCheck / 15 ) ) {
   // from the bottom up while the frontier is large
   cur.assign( words , 0 );
   parallelFor( ex , 0 , F.size() , [ & ]( std::size_t i ) {
    const int u = F[ i ];
    std::atomic_ref< std::uint64_t >( cur[ std::size_t( u ) >> 6 ] )
      .fetch_or( std::uint64_t( 1 ) << ( u & 63 ) ,
		 std::memory_order_relaxed );
    } );
   const int * ioff = in->offsets();
   const auto ihd = in->arcHeads();
   std::size_t awake = F.size() , old;
   do {
    old = awake;
    nxt.assign( words , 0 );
    awake = parallelReduce(
      ex , 0 , words , std::size_t( 0 ) ,
      [ & ]( std::size_t w0 , std::size_t w1 ) {
       std::size_t c = 0;
       const int v1 = int( std::min( std::size_t( n ) , w1 * 64 ) );
       for( int v = int( w0 * 64 ) ; v < v1 ; ++v ) {
	if( dist[ std::size_t( v ) ] >= 0 )
	 continue;
	for( int k = ioff[ v ] ; k < ioff[ v + 1 ] ; ++k ) {
	 const int u = ihd[ k ];
	 if( ( cur[ std::size_t( u ) >> 6 ] >> ( u & 63 ) ) & 1 ) {
	  dist[ std::size_t( v ) ] = level + 1;
	  nxt[ std::size_t( v ) >> 6 ] |= std::uint64_t( 1 ) << ( v & 63 );
	  ++c;
	  break;
	  }
	 }
        }
       return( c );
       } ,
      std::plus<>() , 64 );
    ++level;
    cur.swap( nxt );
    } while( ( awake > 0 ) &&
	    ( ( awake >= old ) || ( awake > std::size_t( n ) / 18 ) ) );
   if( awake == 0 )
    break;
   // back from the top down: the frontier from the bitmap
   const auto cut = blocksOf( 0 , words , 64 );
   const std::size_t B = cut.first , size = cut.second;
   F = detail::gatherBlocks( ex , B , [ & ]( std::size_t b ) {
    std::vector< int > L;
    const std::size_t w1 = std::min( words , ( b + 1 ) * size );
    for( std::size_t w = b * size ; w < w1 ; ++w )
     for( std::uint64_t x = cur[ w ] ; x ; x &= x - 1 )
      L.push_back( int( w * 64 ) + std::countr_zero( x ) );
    return( L );
    } );
   scout = 1;
   continue;
   }
  // from the top down: each node of the frontier claims its neighbors
  toCheck -= scout;
  const auto cut = blocksOf( 0 , F.size() , 64 );
  const std::size_t B = cut.first , size = cut.second;
  std::vector< std::int64_t > part( B , 0 );
  F = detail::gatherBlocks( ex , B , [ & ]( std::size_t b ) {
   std::vector< int > L;
   std::int64_t sc = 0;
   const std::size_t i1 = std::min( F.size() , ( b + 1 ) * size );
   for( std::size_t i = b * size ; i < i1 ; ++i ) {
    const int u = F[ i ];
    for( int k = off[ u ] ; k < off[ u + 1 ] ; ++k ) {
     const int v = hd[ k ];
     std::atomic_ref< int > d( dist[ std::size_t( v ) ] );
     int e = -1;
     if( ( d.load( std::memory_order_relaxed ) < 0 ) &&
	 d.compare_exchange_strong( e , level + 1 ,
				    std::memory_order_relaxed ) ) {
      L.push_back( v );
      sc += off[ v + 1 ] - off[ v ];
      }
     }
    }
   part[ b ] = sc;
   return( L );
   } );
  scout = std::accumulate( part.begin() , part.end() , std::int64_t( 0 ) );
  ++level;
  if( F.empty() )
   break;
  }
 return( dist );
 }

/*--------------------------------------------------------------------------*/

namespace detail
{

/// the links of Afforest on the union-find uf, over the n nodes of the
/// compressed rows A [see Graph.h]: the arcs that keep( v , k ) keeps, those
/// of the nodes in the largest set of a sample skipped if sample (keep
/// must then keep an arc and its reverse alike), and the arcs ( v , u )
/// whose links joined two sets, a spanning forest of the arcs kept,
/// appended to *forest if it is not null
template< class E , class R , class K >
void afforest( E & ex , int n , const R & A , bool sample ,
	       ConcurrentUnionFind & uf , K && keep ,
	       std::vector< std::pair< int , int > > * forest )
{
 using Arcs = std::vector< std::pair< int , int > >;
 std::vector< Arcs > taken;
 // the calls body( v , t ) on the nodes, in the blocks of a parallel loop
 // of the grain g, each block appending to t the arcs whose links joined
 // two sets, if the forest is asked for
 auto over = [ & ]( std::size_t g , auto && body ) {
  const auto cut = blocksOf( 0 , std::size_t( n ) , g );
  const std::size_t s = cut.second;
  Arcs t = gatherBlocks( ex , cut.first , [ & ]( std::size_t b ) {
   Arcs L;
   const int v1 = int( std::min( std::size_t( n ) , ( b + 1 ) * s ) );
   for( int v = int( b * s ) ; v < v1 ; ++v )
    body( v , L );
   return( L );
   } );
  if( forest != nullptr )
   taken.push_back( std::move( t ) );
  };
 auto link = [ & ]( int v , std::size_t k , Arcs & L ) {
  const int u = A.head( k );
  if( keep( v , k ) && uf.unite( v , u ) && ( forest != nullptr ) )
   L.emplace_back( v , u );
  };
 // each node points to its root
 auto compress = [ & ] {
  parallelFor(
    ex , 0 , std::size_t( n ) ,
    [ & ]( std::size_t v ) { uf.compress( std::int32_t( v ) ); } , 4096 );
  };
 constexpr std::size_t rounds = 2;
 for( std::size_t r = 0 ; r < rounds ; ++r ) {
  over( 4096 , [ & ]( int v , Arcs & L ) {
   if( A.first( v ) + r < A.last( v ) )
    link( v , A.first( v ) + r , L );
   } );
  compress();
  }
 // the largest component, from a sample; skipped only if unoriented
 int c = -1;
 if( ( n > 0 ) && sample ) {
  std::mt19937 rng( 0x5EED );
  std::unordered_map< int , int > count;
  int best = 0;
  for( int k = 0 ; k < 1024 ; ++k ) {
   const int x =
     uf.parent( std::int32_t( uniformIndex( rng , unsigned( n ) ) ) );
   if( ++count[ x ] > best ) {
    best = count[ x ];
    c = x;
    }
   }
  }
 over( 1024 , [ & ]( int v , Arcs & L ) {
  if( uf.parent( v ) == c )
   return;
  for( std::size_t k = A.first( v ) + rounds ; k < A.last( v ) ; ++k )
   link( v , k , L );
  } );
 if( forest == nullptr )
  return;
 std::size_t total = forest->size();
 for( const Arcs & t : taken )
  total += t.size();
 std::size_t at = forest->size();
 forest->resize( total );
 for( const Arcs & t : taken ) {
  parallelFor( ex , 0 , t.size() ,
	       [ & ]( std::size_t i ) { ( *forest )[ at + i ] = t[ i ]; } );
  at += t.size();
  }
 }

}  // end( namespace detail )

template< Executor E , class W , ArcLayout AL >
std::vector< int > connectedComponents( E & ex ,
					const StaticGraph< W , AL > & g )
{
 const int n = g.n();
 const detail::Rows< StaticGraph< W , AL > > A( g );
 ConcurrentUnionFind uf{ std::size_t( n ) };
 detail::afforest(
   ex , n , A , ! g.directed() , uf ,
   []( int , std::size_t ) { return( true ); } ,
   nullptr );
 std::vector< int > comp( static_cast< std::size_t >( n ) );
 parallelFor(
   ex , 0 , std::size_t( n ) ,
   [ & ]( std::size_t v ) { comp[ v ] = uf.find( std::int32_t( v ) ); } ,
   4096 );
 return( comp );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelGraph.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
