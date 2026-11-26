/*--------------------------------------------------------------------------*/
/*--------------------- File ParallelBiconnectivity.tpp --------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * FAST-BCC and the 2-edge-connected components of
 * ParallelBiconnectivity.h [see ParallelBiconnectivity.h], with their
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

namespace ADS
{

namespace detail
{

/// the trees of a spanning forest rooted at their least nodes: the number
/// of each name in a preorder of the forest, the largest number in its
/// subtree and its parent, \f$ -1 \f$ at a root
struct RootedForest
{
 std::vector< int > pre;     ///< the number in the preorder
 std::vector< int > last;    ///< the largest number in the subtree
 std::vector< int > parent;  ///< the parent, -1 at a root
 };

/// a relaxed atomic view of an integer shared by the threads
template< class T >
std::atomic_ref< T > relaxed( T & x )
{
 return( std::atomic_ref< T >( x ) );
 }

/// the forest of the edges F on the names [ 0 , n ), rooted at the roots
/// root[ v ] of the trees (each tree with its least node as the root), by
/// the tour of Euler of its arcs ranked by a sparse ruling set
template< class E >
RootedForest rootForest( E & ex , int n ,
			 const std::vector< std::pair< int , int > > & F ,
			 const std::vector< int > & root )
{
 constexpr auto mo = std::memory_order_relaxed;
 const std::size_t k = F.size() , a = 2 * k , N = std::size_t( n );
 // the rows of the arcs of the forest: head[ x ] and the reverse twin[ x ]
 std::vector< int > off( N + 1 , 0 ) , cur( N , 0 );
 parallelFor( ex , 0 , k , [ & ]( std::size_t i ) {
  relaxed( cur[ F[ i ].first ] ).fetch_add( 1 , mo );
  relaxed( cur[ F[ i ].second ] ).fetch_add( 1 , mo );
  } );
 off[ N ] = parallelExclusiveScan( ex , cur.begin() , N , off.begin() , 0 );
 parallelFor(
   ex , 0 , N , [ & ]( std::size_t v ) { cur[ v ] = off[ v ]; } , 4096 );
 std::vector< int > head( a ) , twin( a );
 parallelFor( ex , 0 , k , [ & ]( std::size_t i ) {
  const auto [ u , v ] = F[ i ];
  const int x = relaxed( cur[ u ] ).fetch_add( 1 , mo );
  const int y = relaxed( cur[ v ] ).fetch_add( 1 , mo );
  head[ x ] = v;
  head[ y ] = u;
  twin[ x ] = y;
  twin[ y ] = x;
  } );
 // the tour: the arc after ( u , v ) is the one after ( v , u ) in the row
 // of v, cyclically, but at the end of the list of each tree, the arc into
 // its root r before the first arc out of r
 std::vector< int > next( a );
 parallelFor(
   ex , 0 , a ,
   [ & ]( std::size_t x ) {
    const int v = head[ x ] , t = twin[ x ];
    if( ( root[ v ] == v ) && ( t + 1 == off[ v + 1 ] ) )
     next[ x ] = -1;
    else
     next[ x ] = ( t + 1 == off[ v + 1 ] ) ? off[ v ] : t + 1;
    } ,
   4096 );
 // the rulers: the first arc of each list, and the arcs that a hash picks
 const FastHash< std::uint64_t > hash{ 0x9E3779B97F4A7C15ULL };
 auto first = [ & ]( std::size_t x ) {
  const int u = head[ twin[ x ] ];
  return( ( root[ u ] == u ) && ( off[ u ] == int( x ) ) );
  };
 auto ruler = [ & ]( std::size_t x ) {
  return( first( x ) || ( ( hash.hash64( x ) & 63 ) == 0 ) );
  };
 const auto cut = blocksOf( 0 , a , 4096 );
 const std::vector< int > R =
   gatherBlocks( ex , cut.first , [ & ]( std::size_t b ) {
    std::vector< int > L;
    const std::size_t x1 = std::min( a , ( b + 1 ) * cut.second );
    for( std::size_t x = b * cut.second ; x < x1 ; ++x )
     if( ruler( x ) )
      L.push_back( int( x ) );
    return( L );
    } );
 // the walks from each ruler to the next one: the ruler of each arc and its
 // place from it, then the places of the rulers along each list
 const std::size_t nr = R.size();
 std::vector< int > own( a , -1 ) , rank( a ) , rnext( nr ) , rlen( nr ) ,
   rbase( nr );
 parallelFor(
   ex , 0 , nr , [ & ]( std::size_t j ) { own[ R[ j ] ] = int( j ); } , 4096 );
 parallelFor(
   ex , 0 , nr ,
   [ & ]( std::size_t j ) {
    int x = R[ j ] , c = 1 , y;
    rank[ x ] = 0;
    for( ; ; ) {
     y = next[ x ];
     // the arc after x is reached from x alone: its owner, -1 unless it
     // is a ruler, is read and written by this walk only
     if( ( y < 0 ) || ( own[ y ] >= 0 ) )
      break;
     own[ y ] = int( j );
     rank[ y ] = c++;
     x = y;
     }
    rnext[ j ] = ( y < 0 ) ? -1 : own[ y ];
    rlen[ j ] = c;
    } ,
   64 , 4 * kBlocks );
 // the nodes of each tree, 1 for an isolated root
 std::vector< int > size( N , 0 );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    if( root[ v ] == int( v ) )
     size[ v ] = 1;
    } ,
   4096 );
 parallelFor(
   ex , 0 , nr ,
   [ & ]( std::size_t j0 ) {
    if( ! first( std::size_t( R[ j0 ] ) ) )
     return;
    int s = 0;
    for( int j = int( j0 ) ; j >= 0 ; j = rnext[ j ] ) {
     rbase[ j ] = s;
     s += rlen[ j ];
     }
    size[ head[ twin[ R[ j0 ] ] ] ] = s / 2 + 1;
    } ,
   1 );
 parallelFor(
   ex , 0 , a , [ & ]( std::size_t x ) { rank[ x ] += rbase[ own[ x ] ]; } ,
   4096 );
 // the trees one after the other, by increasing root: the slot 2 base( r )
 // of each root r holds 1, the slot 2 base( r ) + 1 + rank( x ) of each arc
 // x of its tree 1 if x goes down, so that the prefix sums up to the slot
 // of an arc going down to v count the nodes up to v in the preorder
 RootedForest T;
 std::vector< int > & base = T.pre;
 base.assign( N , 0 );
 parallelExclusiveScan( ex , size.begin() , N , base.begin() , 0 );
 std::vector< int > S( 2 * N , 0 );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    if( root[ v ] == int( v ) )
     S[ 2 * std::size_t( base[ v ] ) ] = 1;
    } ,
   4096 );
 auto slot = [ & ]( std::size_t x ) {
  return( 2 * std::size_t( base[ root[ head[ x ] ] ] ) + 1 +
	   std::size_t( rank[ x ] ) );
  };
 parallelFor(
   ex , 0 , a ,
   [ & ]( std::size_t x ) { S[ slot( x ) ] = rank[ x ] < rank[ twin[ x ] ]; } ,
   4096 );
 parallelInclusiveScan( ex , S.begin() , 2 * N , S.begin() , 0 );
 T.last.assign( N , 0 );
 T.parent.assign( N , -1 );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    if( root[ v ] == int( v ) )
     T.last[ v ] = base[ v ] + size[ v ] - 1;
    } ,
   4096 );
 std::vector< int > pre( N );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    if( root[ v ] == int( v ) )
     pre[ v ] = base[ v ];
    } ,
   4096 );
 parallelFor(
   ex , 0 , a ,
   [ & ]( std::size_t x ) {
    const int y = twin[ x ];
    if( rank[ x ] > rank[ y ] )
     return;  // the arc going up
    const int v = head[ x ];
    pre[ v ] = S[ slot( x ) ] - 1;
    T.last[ v ] = pre[ v ] + ( rank[ y ] - rank[ x ] - 1 ) / 2;
    T.parent[ v ] = head[ y ];
    } ,
   4096 );
 T.pre = std::move( pre );
 return( T );
 }

/// the minima and the maxima of the ranges of an array, by blocks of 64
/// items: the minima and maxima of the prefixes and of the suffixes of each
/// block, and sparse tables of the minima and of the maxima of the blocks
class BlockRanges
{
 public:
 static constexpr std::size_t B = 64;  ///< the items of a block

 /// the ranges of \f$ \ell \f$ for the minima and of \f$ h \f$ for the
 /// maxima, of the same size
 template< class E >
 BlockRanges( E & ex , std::vector< int > l , std::vector< int > h )
     : f_l( std::move( l ) ) , f_h( std::move( h ) ) {
  const std::size_t n = f_l.size() , nb = ( n + B - 1 ) / B;
  f_pl.resize( n );
  f_sl.resize( n );
  f_ph.resize( n );
  f_sh.resize( n );
  std::vector< int > ml( nb ) , mh( nb );
  parallelFor(
    ex , 0 , nb ,
    [ & ]( std::size_t b ) {
     const std::size_t i0 = b * B , i1 = std::min( n , i0 + B );
     int lo = f_l[ i0 ] , hi = f_h[ i0 ];
     for( std::size_t i = i0 ; i < i1 ; ++i ) {
      lo = std::min( lo , f_l[ i ] );
      hi = std::max( hi , f_h[ i ] );
      f_pl[ i ] = lo;
      f_ph[ i ] = hi;
      }
     ml[ b ] = lo;
     mh[ b ] = hi;
     lo = f_l[ i1 - 1 ];
     hi = f_h[ i1 - 1 ];
     for( std::size_t i = i1 ; i-- > i0 ; ) {
      lo = std::min( lo , f_l[ i ] );
      hi = std::max( hi , f_h[ i ] );
      f_sl[ i ] = lo;
      f_sh[ i ] = hi;
      }
     } ,
    64 );
  f_tl = SparseTable< int >( std::move( ml ) );
  f_th = SparseTable< int , std::greater< int > >( std::move( mh ) );
  }

 /// the minimum of \f$ \ell \f$ and the maximum of \f$ h \f$ on
 /// \f$ [ i , j ] \f$
 /// \pre \f$ i \le j < n \f$
 [[nodiscard]] std::pair< int , int > query( std::size_t i ,
					    std::size_t j ) const {
  const std::size_t bi = i / B , bj = j / B;
  if( bi == bj ) {
   int lo = f_l[ i ] , hi = f_h[ i ];
   for( std::size_t k = i + 1 ; k <= j ; ++k ) {
    lo = std::min( lo , f_l[ k ] );
    hi = std::max( hi , f_h[ k ] );
    }
   return( std::pair< int , int >( lo , hi ) );
   }
  int lo = std::min( f_sl[ i ] , f_pl[ j ] ) ,
      hi = std::max( f_sh[ i ] , f_ph[ j ] );
  if( bi + 1 < bj ) {
   lo = std::min( lo , f_tl[ f_tl.query( bi + 1 , bj - 1 ) ] );
   hi = std::max( hi , f_th[ f_th.query( bi + 1 , bj - 1 ) ] );
   }
  return( std::pair< int , int >( lo , hi ) );
  }

 private:
 std::vector< int > f_l , f_h;    ///< the items
 std::vector< int > f_pl , f_sl;  ///< the minima of prefixes and suffixes
 std::vector< int > f_ph , f_sh;  ///< the maxima of prefixes and suffixes
 SparseTable< int > f_tl;        ///< the minima of the blocks
 SparseTable< int , std::greater< int > > f_th;  ///< their maxima
 };

/// what the skeleton of FAST-BCC needs of a node: its number in the
/// preorder, the largest in its subtree, its parent and whether the edge to
/// its parent is in the skeleton (not a fence)
struct BccTag
{
 int pre , last , parent;
 bool plain;
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< Executor E , Graph G >
BiconnectedComponents biconnectedComponents( E & ex , const G & g )
{
 ADS_PRE( ! g.directed() );
 constexpr auto mo = std::memory_order_relaxed;
 const int n = g.bound();
 const std::size_t N = std::size_t( n );
 const detail::Rows< G > A( g );
 auto keepAll = []( int , std::size_t ) { return( true ); };
 // the spanning forest, rooted at the least node of each tree
 std::vector< std::pair< int , int > > F;
 std::vector< int > root( N );
 {
  ConcurrentUnionFind uf{ N };
  detail::afforest( ex , n , A , true , uf , keepAll , &F );
  parallelFor(
    ex , 0 , N ,
    [ & ]( std::size_t v ) { root[ v ] = uf.find( std::int32_t( v ) ); } ,
    4096 );
  }
 detail::RootedForest R = detail::rootForest( ex , n , F , root );
 F = {};
 // the tags: the least and the largest number of a neighbor of each node,
 // by the preorder, and their ranges over the subtrees
 std::vector< detail::BccTag > T( N );
 {
  std::vector< int > lo( N ) , hi( N );
  parallelFor(
    ex , 0 , N ,
    [ & ]( std::size_t v ) {
     int l = R.pre[ v ] , h = l;
     for( std::size_t k = A.first( int( v ) ) ; k < A.last( int( v ) ) ;
	  ++k ) {
      const int w = R.pre[ A.head( k ) ];
      l = std::min( l , w );
      h = std::max( h , w );
      }
     lo[ R.pre[ v ] ] = l;
     hi[ R.pre[ v ] ] = h;
     } ,
    1024 , 4 * kBlocks );
  const detail::BlockRanges Q( ex , std::move( lo ) , std::move( hi ) );
  parallelFor(
    ex , 0 , N ,
    [ & ]( std::size_t v ) {
     const int p = R.parent[ v ];
     T[ v ] = { R.pre[ v ] , R.last[ v ] , p , false };
     if( p < 0 )
      return;
     const auto [ l , h ] =
       Q.query( std::size_t( R.pre[ v ] ) , std::size_t( R.last[ v ] ) );
     T[ v ].plain = ( l < R.pre[ p ] ) || ( h > R.last[ p ] );
     } ,
    4096 );
  }
 // the skeleton: the edges of the forest that are not fences, and the edges
 // whose ends are not one an ancestor of the other
 auto skeleton = [ & ]( int v , std::size_t k ) {
  const int w = A.head( k );
  if( w == v )
   return( false );
  const detail::BccTag &x = T[ v ] , &y = T[ w ];
  if( x.pre < y.pre )
   return( ( y.pre > x.last ) || ( ( y.parent == v ) && y.plain ) );
  return( ( x.pre > y.last ) || ( ( x.parent == w ) && x.plain ) );
  };
 std::vector< int > C( N );
 {
  ConcurrentUnionFind uf{ N };
  detail::afforest( ex , n , A , true , uf , skeleton , nullptr );
  parallelFor(
    ex , 0 , N ,
    [ & ]( std::size_t v ) { C[ v ] = uf.find( std::int32_t( v ) ); } , 4096 );
  }
 // the blocks: each component of the skeleton but a root alone, numbered
 // by its least node
 std::vector< int > id( N ) , flag( N );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    flag[ v ] = ( C[ v ] == int( v ) ) && ( T[ v ].parent >= 0 );
    } ,
   4096 );
 const int nb = parallelExclusiveScan( ex , flag.begin() , N , id.begin() ,
				       0 );
 std::vector< int > B( N , -1 ) , head( std::size_t( nb ) , -1 ) ,
   least( std::size_t( nb ) , -1 ) , ref( N , -1 );
 std::vector< std::uint8_t > bridge( std::size_t( nb ) , 0 ) , cut( N , 0 );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    if( T[ v ].parent >= 0 )
     B[ v ] = id[ C[ v ] ];
    if( flag[ v ] )
     least[ id[ v ] ] = int( v );
    } ,
   4096 );
 // the heads, the bridges and the cut nodes, from the first nodes of the
 // components, those under a fence whose parent is in another component
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t i ) {
    const int v = int( i ) , p = T[ v ].parent;
    if( ( p < 0 ) || T[ v ].plain || ( C[ v ] == C[ p ] ) )
     return;
    const int k = B[ v ];
    detail::relaxed( head[ k ] ).store( p , mo );
    int up = 0;
    bool alone = true;
    for( std::size_t a = A.first( v ) ; a < A.last( v ) ; ++a ) {
     up += A.head( a ) == p;
     alone = alone && ! skeleton( v , a );
     }
    if( alone && ( up == 1 ) )  // the block is v and one edge to p
     bridge[ k ] = 1;
    int r = -1;
    if( T[ p ].parent >= 0 ) {  // p is in its own block and heads k
     if( detail::relaxed( cut[ p ] ).load( mo ) == 0 )
      detail::relaxed( cut[ p ] ).store( 1 , mo );
     }
    else if( ! detail::relaxed( ref[ p ] )
		 .compare_exchange_strong( r , k ,
					   mo ) &&
	     ( r != k ) )  // the root p heads two blocks
     detail::relaxed( cut[ p ] ).store( 1 , mo );
    } ,
   1024 , 4 * kBlocks );
 BiconnectedComponents out( std::move( B ) , std::move( R.pre ) ,
			    std::move( head ) , std::move( least ) ,
			    std::move( bridge ) , std::move( cut ) );
 ADS_AUDIT( out.sameBlocks( biconnectedComponents( g ) ) );
 return( out );
 }

template< Executor E , Graph G >
std::vector< int > twoEdgeConnectedComponents( E & ex , const G & g )
{
 ADS_PRE( ! g.directed() );
 const int n = g.bound();
 const std::size_t N = std::size_t( n );
 const BiconnectedComponents b = biconnectedComponents( ex , g );
 const detail::Rows< G > A( g );
 ConcurrentUnionFind uf{ N };
 detail::afforest(
   ex , n , A , true , uf ,
   [ & ]( int v , std::size_t k ) {
    const int w = A.head( k );
    return( ( w != v ) && ! b.isBridge( b.block( v , w ) ) );
    } ,
   nullptr );
 std::vector< int > C( N );
 parallelFor(
   ex , 0 , N ,
   [ & ]( std::size_t v ) {
    C[ v ] = g.hasNode( int( v ) ) ? uf.find( std::int32_t( v ) ) : -1;
    } ,
   4096 );
 ADS_AUDIT( C == twoEdgeConnectedComponents( g ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File ParallelBiconnectivity.tpp ------------------*/
/*--------------------------------------------------------------------------*/
