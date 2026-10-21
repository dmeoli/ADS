/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelGraph.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel visit in breadth and the connected components of
 * ParallelGraph.h.
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

#include <numeric>
#include <random>
#include <unordered_map>

namespace ADS {

namespace detail {

/// the concatenation of the vectors that f( b ) gives for the blocks
/// b = 0 , ... , B - 1, in their order
template< class E , class F >
std::vector< int > gatherBlocks( E & ex , std::size_t B , F && f )
{
 std::vector< std::vector< int > > L( B );
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) { L[ b ] = f( b ); } , 1 );
 std::vector< std::size_t > at( B + 1 , 0 );
 for( std::size_t b = 0 ; b < B ; ++b )
  at[ b + 1 ] = at[ b ] + L[ b ].size();
 std::vector< int > out( at[ B ] );
 parallelFor( ex , 0 , B , [ & ]( std::size_t b ) {
  std::copy( L[ b ].begin() , L[ b ].end() ,
             out.begin() + std::ptrdiff_t( at[ b ] ) ); } , 1 );
 return( out );
 }

inline int atomicLoad( int & x )
{
 return( std::atomic_ref< int >( x ).load( std::memory_order_relaxed ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , class W >
std::vector< int > bfsDistances( E & ex , const StaticGraph< W > & g , int s ,
                                 const StaticGraph< W > * gt )
{
 using G = StaticGraph< W >;
 ADS_PRE( g.hasNode( s ) );
 ADS_PRE( ( gt == nullptr ) || ( gt->n() == g.n() ) );
 const int n = g.n();
 const int * off = g.offsets();
 const auto * arc = g.arcData();
 const G * in = g.directed() ? gt : &g;
 std::vector< int > dist( std::size_t( n ) , -1 );
 dist[ std::size_t( s ) ] = 0;
 const std::size_t words = ( std::size_t( n ) + 63 ) / 64;
 std::vector< std::uint64_t > cur , nxt;
 std::vector< int > F{ s };
 std::int64_t toCheck = g.arcs() , scout = g.degree( s );
 int level = 0;
 for( ;; ) {
  if( ( in != nullptr ) && ( scout > toCheck / 15 ) ) {
   // from the bottom up while the frontier is large
   cur.assign( words , 0 );
   parallelFor( ex , 0 , F.size() , [ & ]( std::size_t i ) {
    const int u = F[ i ];
    std::atomic_ref< std::uint64_t >( cur[ std::size_t( u ) >> 6 ] )
     .fetch_or( std::uint64_t( 1 ) << ( u & 63 ) ,
                std::memory_order_relaxed ); } );
   const int * ioff = in->offsets();
   const auto * iarc = in->arcData();
   std::size_t awake = F.size() , old;
   do {
    old = awake;
    nxt.assign( words , 0 );
    awake = parallelReduce( ex , 0 , words , std::size_t( 0 ) ,
     [ & ]( std::size_t w0 , std::size_t w1 ) {
      std::size_t c = 0;
      const int v1 = int( std::min( std::size_t( n ) , w1 * 64 ) );
      for( int v = int( w0 * 64 ) ; v < v1 ; ++v ) {
       if( dist[ std::size_t( v ) ] >= 0 )
        continue;
       for( int k = ioff[ v ] ; k < ioff[ v + 1 ] ; ++k ) {
        const int u = G::head( iarc[ k ] );
        if( ( cur[ std::size_t( u ) >> 6 ] >> ( u & 63 ) ) & 1 ) {
         dist[ std::size_t( v ) ] = level + 1;
         nxt[ std::size_t( v ) >> 6 ] |= std::uint64_t( 1 ) << ( v & 63 );
         ++c;
         break;
         }
        }
       }
      return( c ); } , std::plus<>() , 64 );
    ++level;
    cur.swap( nxt );
    } while( ( awake > 0 ) && ( ( awake >= old ) ||
                                ( awake > std::size_t( n ) / 18 ) ) );
   if( awake == 0 )
    break;
   // back from the top down: the frontier from the bitmap
   const auto [ B , size ] = blocksOf( 0 , words , 64 );
   F = detail::gatherBlocks( ex , B , [ & , size = size ]( std::size_t b ) {
    std::vector< int > L;
    const std::size_t w1 = std::min( words , ( b + 1 ) * size );
    for( std::size_t w = b * size ; w < w1 ; ++w )
     for( std::uint64_t x = cur[ w ] ; x ; x &= x - 1 )
      L.push_back( int( w * 64 ) + std::countr_zero( x ) );
    return( L ); } );
   scout = 1;
   continue;
   }
  // from the top down: each node of the frontier claims its neighbours
  toCheck -= scout;
  const auto [ B , size ] = blocksOf( 0 , F.size() , 64 );
  std::vector< std::int64_t > part( B , 0 );
  F = detail::gatherBlocks( ex , B , [ & , size = size ]( std::size_t b ) {
   std::vector< int > L;
   std::int64_t sc = 0;
   const std::size_t i1 = std::min( F.size() , ( b + 1 ) * size );
   for( std::size_t i = b * size ; i < i1 ; ++i ) {
    const int u = F[ i ];
    for( int k = off[ u ] ; k < off[ u + 1 ] ; ++k ) {
     const int v = G::head( arc[ k ] );
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
   return( L ); } );
  scout = std::accumulate( part.begin() , part.end() , std::int64_t( 0 ) );
  ++level;
  if( F.empty() )
   break;
  }
 return( dist );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , class W >
std::vector< int > connectedComponents( E & ex , const StaticGraph< W > & g )
{
 using G = StaticGraph< W >;
 const int n = g.n();
 const int * off = g.offsets();
 const auto * arc = g.arcData();
 std::vector< int > comp( static_cast< std::size_t >( n ) );
 parallelFor( ex , 0 , std::size_t( n ) , [ & ]( std::size_t v ) {
  comp[ v ] = int( v ); } , 4096 );
 auto at = [ & ]( int v ) {
  return( std::atomic_ref< int >( comp[ std::size_t( v ) ] ) ); };
 // hooks the larger of the roots of u and v under the smaller
 auto link = [ & ]( int u , int v ) {
  int p1 = at( u ).load( std::memory_order_relaxed ) ,
      p2 = at( v ).load( std::memory_order_relaxed );
  while( p1 != p2 ) {
   const int high = std::max( p1 , p2 ) , low = std::min( p1 , p2 );
   int ph = at( high ).load( std::memory_order_relaxed );
   if( ph == low )
    break;
   if( ( ph == high ) &&
       at( high ).compare_exchange_strong( ph , low ,
                                           std::memory_order_relaxed ) )
    break;
   p1 = at( at( high ).load( std::memory_order_relaxed ) )
         .load( std::memory_order_relaxed );
   p2 = at( low ).load( std::memory_order_relaxed );
   }
  };
 auto compress = [ & ] {
  parallelFor( ex , 0 , std::size_t( n ) , [ & ]( std::size_t i ) {
   const int v = int( i );
   for( ;; ) {
    const int p = at( v ).load( std::memory_order_relaxed ) ,
              q = at( p ).load( std::memory_order_relaxed );
    if( p == q )
     break;
    at( v ).store( q , std::memory_order_relaxed );
    }
   } , 4096 );
  };
 constexpr int rounds = 2;
 for( int r = 0 ; r < rounds ; ++r ) {
  parallelFor( ex , 0 , std::size_t( n ) , [ & ]( std::size_t i ) {
   const int v = int( i );
   if( off[ v ] + r < off[ v + 1 ] )
    link( v , G::head( arc[ off[ v ] + r ] ) );
   } , 4096 );
  compress();
  }
 // the largest component, from a sample; skipped only if unoriented
 int c = -1;
 if( ( n > 0 ) && ! g.directed() ) {
  std::mt19937 rng( 0x5EED );
  std::unordered_map< int , int > count;
  int best = 0;
  for( int k = 0 ; k < 1024 ; ++k ) {
   const int x = comp[ std::size_t( rng() % unsigned( n ) ) ];
   if( ++count[ x ] > best ) {
    best = count[ x ];
    c = x;
    }
   }
  }
 parallelFor( ex , 0 , std::size_t( n ) , [ & ]( std::size_t i ) {
  const int v = int( i );
  if( at( v ).load( std::memory_order_relaxed ) == c )
   return;
  for( int k = off[ v ] + rounds ; k < off[ v + 1 ] ; ++k )
   link( v , G::head( arc[ k ] ) );
  } , 1024 );
 compress();
 return( comp );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelGraph.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
