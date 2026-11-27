/*--------------------------------------------------------------------------*/
/*------------------------ File PerfectMatching.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The perfect matchings of PerfectMatching.h [see PerfectMatching.h], with
 * their contracts.
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

/// the primal-dual algorithm of minCostPerfectMatching(): the trees, each
/// with its own change of the duals, the queues of the slacks per tree and
/// per pair of trees, the blossoms as the cycles of their children; each
/// end of an edge is at the largest node that holds it and not the other
/// end, and is in the list of that node, so that a node reaches the edges
/// that leave it and none within it
struct PerfectBlossoms
{
 using D = long long;  ///< the costs doubled, the duals, the slacks

 static constexpr signed char PLUS = 1;    ///< outer, in a tree
 static constexpr signed char MINUS = -1;  ///< inner, in a tree
 static constexpr signed char FREE = 0;    ///< in no tree, or in a blossom
 static constexpr D INF = std::numeric_limits< D >::max() / 4;

 /// the node of a queue: an edge, or an inner blossom
 struct QNode
 {
  D priority = 0;
  QNode * child = nullptr;
  QNode * next = nullptr;
  QNode * prev = nullptr;
  };

 /// a queue of edges, dropped at once with its tree, whose edges are then
 /// all placed again
 struct Queue
 {
  IntrusivePairingHeap< QNode > h;

  void drop( void ) { h.clear(); }
  };

 struct TreeEdge;

 /// a tree: its root, its change of the duals and its three queues, the
 /// edges to the free nodes, the edges within it, the inner blossoms
 struct Tree
 {
  int root = NIL;
  D eps = 0;
  Queue q0 , q00;
  IntrusivePairingHeap< QNode > qB;
  TreeEdge * te = nullptr;  ///< the list of its edges to the other trees
  int at = NIL;             ///< the place in the list of the trees alive
  bool hot = false;         ///< whether an edge to another tree may be tight
  };

 /// the edges between two trees: those between their outer nodes, and
 /// those from an outer node of t[ d ] to an inner one of t[ 1 - d ], in
 /// the list of t[ d ] by next[ d ]; t[ d ] is NIL once t[ d ] is gone,
 /// and the edge leaves the list of the other tree when this next reads it
 struct TreeEdge
 {
  int t[ 2 ] = { NIL , NIL };
  TreeEdge * next[ 2 ] = { nullptr , nullptr };
  Queue pp , pm[ 2 ];
  };

 int n , m;
 /// what the operations read of an edge, in one line of the cache: its
 /// node in its queue, whose priority is the slack of the edge relative to
 /// the eps of its ends (an edge leaves its queue before its slack
 /// changes), the queue, and, of the end 2 e + 0 (at eu) and 2 e + 1 (at
 /// ev) of the edge e, the list it is in and its links in that list
 struct alignas( 64 ) EdgeRec
 {
  QNode q;
  Queue * where = nullptr;
  int at[ 2 ] = { NIL , NIL };
  int next[ 2 ] = { NIL , NIL };
  int prev[ 2 ] = { NIL , NIL };
  };

 std::vector< int > eu , ev;  ///< the ends of the edges
 std::vector< D > cost2;     ///< the costs doubled
 std::vector< EdgeRec > ed;
 std::vector< QNode > qb;  ///< the node of each blossom in qB

 // the lists of the ends, each the list of one node: a shrink gives to
 // the blossom the list of its outer child with the most ends
 std::vector< int > lid;    ///< the list of each node
 std::vector< int > lnode;  ///< the node of each list
 std::vector< int > first;  ///< the first end of each list
 std::vector< int > cnt;    ///< the number of the ends of each list

 // the nodes, the n vertices and then the blossoms
 std::vector< int > bparent , base , tpar , fchild , snext , sprev , tree;
 std::vector< signed char > lab;
 std::vector< D > y;
 std::vector< std::vector< int > > kids , kedge , kend;
 std::vector< bool > balive;
 std::vector< int > freeB;
 std::vector< int > mate;  ///< the matched edge of each vertex

 std::vector< Tree > T;  ///< reserved at once, so that its queues stay
 std::vector< int > freeT;
 std::vector< int > alive;   ///< the trees alive
 std::deque< TreeEdge > TE;  ///< whose places stay, as the pointers to them
 std::vector< TreeEdge * > freeTE;

 std::vector< TreeEdge * > markTE;
 std::vector< unsigned > markStamp;
 unsigned stamp = 0;
 int cur = NIL;  ///< the tree whose marks are set

 std::vector< unsigned > seen;
 unsigned seenStamp = 0;
 std::vector< int > memo;  ///< the child of the blossom expanded, by node
 std::vector< unsigned > memoAt;
 unsigned memoStamp = 0;
 std::vector< int > collected , N1 , N2 , Pu , Eu , Pv , Ev;
 std::vector< std::pair< int , int > > work;
 int pendingAug = NIL;
 std::size_t augments = 0;  ///< the augmentations so far
 bool infeasible = false;

 PerfectBlossoms( int n0 , const std::vector< int > & U ,
		  const std::vector< int > & V , const std::vector< D > & C2 )
     : n( n0 ) , m( int( U.size() ) ) , eu( U ) , ev( V ) , cost2( C2 ) ,
       ed( std::size_t( m ) ) {
  for( int e = 0 ; e < m ; ++e )
   ed[ std::size_t( e ) ].q.priority = C2[ std::size_t( e ) ];
  const std::size_t N = std::size_t( n ) + std::size_t( n ) / 2 + 1;
  qb.resize( N );
  lid.resize( N );
  lnode.resize( N );
  for( std::size_t x = 0 ; x < N ; ++x )
   lid[ x ] = lnode[ x ] = int( x );
  first.assign( N , NIL );
  cnt.assign( N , 0 );
  bparent.assign( N , NIL );
  base.assign( N , NIL );
  tpar.assign( N , NIL );
  fchild.assign( N , NIL );
  snext.assign( N , NIL );
  sprev.assign( N , NIL );
  tree.assign( N , NIL );
  lab.assign( N , FREE );
  y.assign( N , 0 );
  kids.resize( N );
  kedge.resize( N );
  kend.resize( N );
  balive.assign( N , false );
  seen.assign( N , 0 );
  memo.assign( N , NIL );
  memoAt.assign( N , 0 );
  for( int b = int( N ) - 1 ; b >= n ; --b )
   freeB.push_back( b );
  mate.assign( std::size_t( n ) , NIL );
  for( int v = 0 ; v < n ; ++v )
   base[ std::size_t( v ) ] = v;
  for( int e = m - 1 ; e >= 0 ; --e )  // a loop is never matched
   if( eu[ e ] != ev[ e ] ) {
    link( 2 * e , eu[ e ] );
    link( 2 * e + 1 , ev[ e ] );
    }
  }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE PIECES -------------------------------*/
/*--------------------------------------------------------------------------*/

 int other( int e , int a ) const { return( eu[ e ] ^ ev[ e ] ^ a ); }

 /// the slack of the edge e, relative to the eps of its ends
 D & s( int e ) { return( ed[ std::size_t( e ) ].q.priority ); }
 D s( int e ) const { return( ed[ std::size_t( e ) ].q.priority ); }

 /// the node of the end k, and the links of its list
 int hd( int k ) const {
  return( lnode[ std::size_t( ed[ std::size_t( k >> 1 ) ].at[ k & 1 ] ) ] );
  }
 int & at( int k ) { return( ed[ std::size_t( k >> 1 ) ].at[ k & 1 ] ); }
 int & anext( int k ) { return( ed[ std::size_t( k >> 1 ) ].next[ k & 1 ] ); }
 int & aprev( int k ) { return( ed[ std::size_t( k >> 1 ) ].prev[ k & 1 ] ); }

 /// the edge of the node x of a queue
 int edgeOf( const QNode * x ) const {
  return( int( reinterpret_cast< const EdgeRec * >( x ) - ed.data() ) );
  }

 /// the vertex of the end k
 int vertexOf( int k ) const {
  return( k & 1 ? ev[ k >> 1 ] : eu[ k >> 1 ] );
  }

 /// the end k into the list of the node x
 void link( int k , int x ) {
  const int L = lid[ std::size_t( x ) ];
  at( k ) = L;
  aprev( k ) = NIL;
  anext( k ) = first[ L ];
  if( first[ L ] != NIL )
   aprev( first[ L ] ) = k;
  first[ L ] = k;
  ++cnt[ L ];
  }

 /// the end k out of the list of its node
 void unlink( int k ) {
  if( aprev( k ) != NIL )
   anext( aprev( k ) ) = anext( k );
  else
   first[ at( k ) ] = anext( k );
  if( anext( k ) != NIL )
   aprev( anext( k ) ) = aprev( k );
  --cnt[ at( k ) ];
  }

 /// f( k ) on each end k in the list of the node x, which f may move
 template< class F >
 void forEnds( int x , F && f ) {
  for( int k = first[ lid[ std::size_t( x ) ] ] ; k != NIL ; ) {
   const int nx = anext( k );
   f( k );
   k = nx;
   }
  }

 D eps( int t ) const { return( T[ std::size_t( t ) ].eps ); }

 /// the contribution of the eps of the node x to the slacks of its edges
 D shift( int x ) const {
  return( lab[ x ] == FREE ? 0 : lab[ x ] * eps( tree[ x ] ) );
  }

 /// the true slack of the edge e
 D slack( int e ) const {
  return( s( e ) - shift( hd( 2 * e ) ) - shift( hd( 2 * e + 1 ) ) );
  }

 /// the node at the other end of the edge e from the node x
 int across( int e , int x ) const {
  return( hd( 2 * e ) == x ? hd( 2 * e + 1 ) : hd( 2 * e ) );
  }

 /// the end of the edge e in the node x
 int endIn( int e , int x ) const {
  return( hd( 2 * e ) == x ? eu[ e ] : ev[ e ] );
  }

 /// the mate node of the surface node x, through the matched edge at its
 /// base
 int mateNode( int x ) const {
  const int e = mate[ base[ x ] ];
  return( e == NIL ? NIL : across( e , x ) );
  }

 void addChild( int p , int c ) {
  sprev[ c ] = NIL;
  snext[ c ] = fchild[ p ];
  if( fchild[ p ] != NIL )
   sprev[ fchild[ p ] ] = c;
  fchild[ p ] = c;
  }

 void removeChild( int p , int c ) {
  if( sprev[ c ] != NIL )
   snext[ sprev[ c ] ] = snext[ c ];
  else
   fchild[ p ] = snext[ c ];
  if( snext[ c ] != NIL )
   sprev[ snext[ c ] ] = sprev[ c ];
  sprev[ c ] = snext[ c ] = NIL;
  }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE TREES --------------------------------*/
/*--------------------------------------------------------------------------*/

 int newTree( int r ) {
  int t;
  if( ! freeT.empty() ) {
   t = freeT.back();
   freeT.pop_back();
   }
  else {
   t = int( T.size() );
   T.emplace_back();
   markTE.push_back( nullptr );
   markStamp.push_back( 0 );
   }
  Tree & R = T[ std::size_t( t ) ];
  R.root = r;
  R.eps = 0;
  R.te = nullptr;
  R.at = int( alive.size() );
  R.hot = true;
  alive.push_back( t );
  return( t );
  }

 /// the tree t out of the list of the trees alive, its queues dropped
 void killTree( int t ) {
  Tree & R = T[ std::size_t( t ) ];
  const int last = alive.back();
  alive[ std::size_t( R.at ) ] = last;
  T[ std::size_t( last ) ].at = R.at;
  alive.pop_back();
  R.at = NIL;
  R.root = NIL;
  R.q0.drop();
  R.q00.drop();
  for( TreeEdge * X = R.te ; X ; ) {
   const int k = X->t[ 0 ] == t ? 0 : 1;
   TreeEdge * const nx = X->next[ k ];
   X->pp.drop();
   X->pm[ 0 ].drop();
   X->pm[ 1 ].drop();
   X->t[ k ] = NIL;
   if( X->t[ 1 - k ] == NIL )  // in no list now
    freeTE.push_back( X );
   X = nx;
   }
  R.te = nullptr;
  freeT.push_back( t );
  }

 /// the edge e out of its queue, so that its slack may change
 void dequeue( int e ) {
  EdgeRec & r = ed[ std::size_t( e ) ];
  if( r.where )
   r.where->h.erase( &r.q );
  r.where = nullptr;
  }

 /// f( X , k ) on each edge X from the tree t to a tree alive, t being
 /// X.t[ k ], until f returns true; the edges to the trees gone leave the
 /// list and are freed
 template< class F >
 void forTreeEdges( int t , F && f ) {
  TreeEdge ** p = &T[ std::size_t( t ) ].te;
  while( TreeEdge * X = *p ) {
   const int k = X->t[ 0 ] == t ? 0 : 1;
   if( X->t[ 1 - k ] == NIL ) {  // of a tree gone
    *p = X->next[ k ];
    freeTE.push_back( X );
    continue;
    }
   if( f( *X , k ) )
    return;
   p = &X->next[ k ];
   }
  }

 /// the marks of the trees adjacent to t, for teOf()
 void setCur( int t ) {
  cur = t;
  ++stamp;
  forTreeEdges( t , [ & ]( TreeEdge & X , int k ) {
   const int o = X.t[ 1 - k ];
   markTE[ std::size_t( o ) ] = &X;
   markStamp[ std::size_t( o ) ] = stamp;
   return( false );
   } );
  }

 /// the tree edge between the trees a and b, one of them cur
 TreeEdge & teOf( int a , int b ) {
  const int o = a == cur ? b : a;
  if( markStamp[ std::size_t( o ) ] == stamp )
   return( *markTE[ std::size_t( o ) ] );
  TreeEdge * x;
  if( ! freeTE.empty() ) {
   x = freeTE.back();
   freeTE.pop_back();
   }
  else
   x = &TE.emplace_back();
  TreeEdge & X = *x;
  X.t[ 0 ] = cur;
  X.t[ 1 ] = o;
  X.next[ 0 ] = T[ std::size_t( cur ) ].te;
  T[ std::size_t( cur ) ].te = x;
  X.next[ 1 ] = T[ std::size_t( o ) ].te;
  T[ std::size_t( o ) ].te = x;
  markTE[ std::size_t( o ) ] = x;
  markStamp[ std::size_t( o ) ] = stamp;
  return( X );
  }

 /// the edge e into the queue of its class, out of the one it was in
 void place( int e ) {
  int u = hd( 2 * e ) , v = hd( 2 * e + 1 );
  if( lab[ u ] != PLUS )
   std::swap( u , v );
  Queue * q = nullptr;
  if( lab[ u ] != PLUS )
   ;  // no outer end: its slack never decreases
  else if( lab[ v ] == PLUS ) {
   if( tree[ u ] == tree[ v ] )
    q = &T[ std::size_t( tree[ u ] ) ].q00;
   else {
    q = &teOf( tree[ u ] , tree[ v ] ).pp;
    if( s( e ) - eps( tree[ u ] ) - eps( tree[ v ] ) == 0 ) {
     pendingAug = e;
     T[ std::size_t( tree[ u ] ) ].hot = T[ std::size_t( tree[ v ] ) ].hot =
       true;
     }
    }
   }
  else if( lab[ v ] == FREE )
   q = &T[ std::size_t( tree[ u ] ) ].q0;
  else if( tree[ u ] != tree[ v ] ) {
   TreeEdge & X = teOf( tree[ u ] , tree[ v ] );
   q = &X.pm[ X.t[ 0 ] == tree[ u ] ? 0 : 1 ];
   }
  EdgeRec & r = ed[ std::size_t( e ) ];
  if( r.where ) {
   if( r.where == q )
    return;  // where it was, its slack unchanged
   r.where->h.erase( &r.q );
   }
  r.where = q;
  if( q ) {
   r.q.child = r.q.next = r.q.prev = nullptr;  // perhaps of a queue dropped
   q->h.insert( &r.q );
   }
  }

 /// the label and the tree of x, and an inner blossom in the queue qB
 void setLabel( int x , signed char l , int t ) {
  if( ( x >= n ) && ( lab[ x ] == MINUS ) )
   T[ std::size_t( tree[ x ] ) ].qB.erase( &qb[ x ] );
  lab[ x ] = l;
  tree[ x ] = t;
  if( ( x >= n ) && ( l == MINUS ) ) {
   qb[ x ].priority = y[ x ];
   T[ std::size_t( t ) ].qB.insert( &qb[ x ] );
   }
  }

 /// the nodes of the tree t, the root first
 void treeNodes( int t , std::vector< int > & L ) {
  L.clear();
  L.push_back( T[ std::size_t( t ) ].root );
  for( std::size_t i = 0 ; i < L.size() ; ++i ) {
   const int x = L[ i ];
   if( lab[ x ] == PLUS ) {
    for( int c = fchild[ x ] ; c != NIL ; c = snext[ c ] )
     L.push_back( c );
    }
   else
    L.push_back( mateNode( x ) );
   }
  }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE BLOSSOMS ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the child of the blossom B that holds the vertex a
 int childOf( int B , int a ) const {
  while( bparent[ a ] != B )
   a = bparent[ a ];
  return( a );
  }

 /// the child of the blossom B that holds the vertex a, kept for each node
 /// on the way up until memoStamp changes, so that the ends of B find their
 /// children in time linear in the nodes of B and in the ends
 int childOfMemo( int B , int a ) {
  int x = a;
  while( ( bparent[ x ] != B ) && ( memoAt[ x ] != memoStamp ) )
   x = bparent[ x ];
  const int c = bparent[ x ] == B ? x : memo[ x ];
  for( int z = a ; ; z = bparent[ z ] ) {
   memo[ z ] = c;
   memoAt[ z ] = memoStamp;
   if( z == x )
    break;
   }
  return( c );
  }

 /// the vertex v made the base of the blossom x, the matching within it
 /// rotated around the cycles of its children, at every level
 void rebase( int x , int v ) {
  if( x < n )
   return;
  work.assign( 1 , { x , v } );
  while( ! work.empty() ) {
   const auto [ B , t ] = work.back();
   work.pop_back();
   const int c = childOf( B , t );
   if( c >= n )
    work.emplace_back( c , t );
   auto & K = kids[ B ];
   auto & KE = kedge[ B ];
   auto & KA = kend[ B ];
   const int k = int( K.size() );
   int i = 0;
   while( K[ i ] != c )
    ++i;
   auto match = [ & ]( int j ) {  // the edge j between K[ j ], K[ j + 1 ]
    const int e = KE[ j ] , a = KA[ j ] , b = other( e , a );
    mate[ a ] = mate[ b ] = e;
    const int ca = K[ j ] , cb = K[ ( j + 1 ) % k ];
    if( ca >= n )
     work.emplace_back( ca , a );
    if( cb >= n )
     work.emplace_back( cb , b );
    };
   if( i % 2 == 1 )
    for( int j = i + 1 ; j < k ; j += 2 )
     match( j );
   else
    for( int j = 0 ; j < i ; j += 2 )
     match( j );
   std::rotate( K.begin() , K.begin() + i , K.end() );
   std::rotate( KE.begin() , KE.begin() + i , KE.end() );
   std::rotate( KA.begin() , KA.begin() + i , KA.end() );
   base[ B ] = t;
   }
  }

 /// the plus node above the plus node x, NIL at the root
 int upPlus( int x ) const {
  const int w = mateNode( x );
  return( w == NIL ? NIL : across( tpar[ w ] , w ) );
  }

 /// the edge e between two outer nodes of the tree cur shrinks the cycle
 /// it closes into a blossom, outer
 void shrink( int e ) {
  const int t = cur;
  const D E = eps( t );
  const int u = hd( 2 * e ) , v = hd( 2 * e + 1 );
  ++seenStamp;  // the first common ancestor
  int r = NIL;
  for( int x = u , z = v ; ; ) {
   if( x != NIL ) {
    if( seen[ x ] == seenStamp ) {
     r = x;
     break;
     }
    seen[ x ] = seenStamp;
    x = upPlus( x );
    }
   if( z != NIL ) {
    if( seen[ z ] == seenStamp ) {
     r = z;
     break;
     }
    seen[ z ] = seenStamp;
    z = upPlus( z );
    }
   }
  // the two paths up to r, with the edges to the next node
  auto path = [ & ]( int x , std::vector< int > & P ,
		     std::vector< int > & Q ) {
   P.clear();
   Q.clear();
   while( x != r ) {
    const int w = mateNode( x );
    P.push_back( x );
    Q.push_back( mate[ base[ x ] ] );
    P.push_back( w );
    Q.push_back( tpar[ w ] );
    x = across( tpar[ w ] , w );
    }
   };
  path( u , Pu , Eu );
  path( v , Pv , Ev );
  const int B = freeB.back();
  freeB.pop_back();
  balive[ B ] = true;
  auto & K = kids[ B ];
  auto & KE = kedge[ B ];
  K.assign( 1 , r );
  KE.clear();
  for( int i = int( Pu.size() ) - 1 ; i >= 0 ; --i ) {
   KE.push_back( Eu[ std::size_t( i ) ] );
   K.push_back( Pu[ std::size_t( i ) ] );
   }
  KE.push_back( e );
  for( std::size_t i = 0 ; i < Pv.size() ; ++i ) {
   K.push_back( Pv[ i ] );
   KE.push_back( Ev[ i ] );
   }
  auto & KA = kend[ B ];
  KA.resize( K.size() );
  for( std::size_t i = 0 ; i < K.size() ; ++i )
   KA[ i ] = endIn( KE[ i ] , K[ i ] );
  // the outer child with the most ends, whose list becomes that of B; its
  // children in the tree pass to B as they are, but for those on the cycle
  int big = NIL;
  for( int c : K )
   if( ( lab[ c ] == PLUS ) &&
       ( ( big == NIL ) || ( cnt[ lid[ std::size_t( c ) ] ] >
			     cnt[ lid[ std::size_t( big ) ] ] ) ) )
    big = c;
  for( int c : K )
   if( ( lab[ c ] == MINUS ) && ( across( tpar[ c ] , c ) == big ) )
    removeChild( big , c );
  for( int c : K )
   bparent[ c ] = B;
  base[ B ] = base[ r ];
  bparent[ B ] = NIL;
  tpar[ B ] = NIL;
  snext[ B ] = sprev[ B ] = NIL;
  fchild[ B ] = fchild[ big ];
  fchild[ big ] = NIL;
  std::swap( lid[ std::size_t( B ) ] , lid[ std::size_t( big ) ] );
  lnode[ std::size_t( lid[ std::size_t( B ) ] ) ] = B;
  lnode[ std::size_t( lid[ std::size_t( big ) ] ) ] = big;
  // the place of r in the tree, and the children of the other outer nodes
  if( T[ std::size_t( t ) ].root == r )
   T[ std::size_t( t ) ].root = B;
  for( int c : K )
   if( ( lab[ c ] == PLUS ) && ( c != big ) )
    for( int d = fchild[ c ] ; d != NIL ; ) {
     const int nx = snext[ d ];
     if( bparent[ d ] != B )
      addChild( B , d );
     d = nx;
     }
  // the edges within B frozen, those that leave it moved to its list,
  // those of its inner children shifted to an outer end; the ends of big,
  // already in the list of B, are reached from the other children, which
  // give back to big those of the edges within B
  collected.clear();
  for( int c : K )
   if( c != big )
    forEnds( c , [ & ]( int k ) {
     const int f = k >> 1 , o = hd( k ^ 1 );
     if( o == B ) {  // to big: within B
      dequeue( f );
      s( f ) -= ( lab[ c ] + PLUS ) * E;
      unlink( k ^ 1 );
      link( k ^ 1 , big );
      return;
      }
     if( bparent[ o ] == B ) {
      if( ( k & 1 ) == 0 ) {
       dequeue( f );
       s( f ) -= ( lab[ c ] + lab[ o ] ) * E;
       }
      return;
      }
     unlink( k );
     link( k , B );
     if( lab[ c ] == MINUS ) {
      dequeue( f );
      s( f ) += 2 * E;
      collected.push_back( f );
      }
     } );
  for( int c : K ) {
   y[ c ] += lab[ c ] * E;
   if( ( c >= n ) && ( lab[ c ] == MINUS ) )
    T[ std::size_t( t ) ].qB.erase( &qb[ c ] );
   lab[ c ] = FREE;
   tree[ c ] = NIL;
   tpar[ c ] = fchild[ c ] = snext[ c ] = sprev[ c ] = NIL;
   }
  lab[ B ] = PLUS;
  tree[ B ] = t;
  y[ B ] = -E;
  for( int f : collected )
   place( f );
  }

 /// the inner blossom B of dual 0 of the tree cur expanded: its children
 /// on the even way from the entry to the base inner and outer in turn,
 /// the others free
 void expand( int B ) {
  const int t = cur;
  const D E = eps( t );
  const int pe = tpar[ B ];
  const int ba = endIn( pe , B );
  const int p = across( pe , B );
  auto & K = kids[ B ];
  auto & KE = kedge[ B ];
  const int k = int( K.size() );
  const int c = childOf( B , ba );
  int j = 0;
  while( K[ j ] != c )
   ++j;
  T[ std::size_t( t ) ].qB.erase( &qb[ B ] );
  removeChild( p , B );
  for( int x : K ) {
   lab[ x ] = FREE;
   tree[ x ] = NIL;
   tpar[ x ] = fchild[ x ] = snext[ x ] = sprev[ x ] = NIL;
   }
  // the even way from K[ j ] to K[ 0 ]
  const int step = j % 2 == 1 ? 1 : -1;
  int prev = NIL;
  for( int i = j , pos = 0 ; ; i = ( i + step + k ) % k , ++pos ) {
   const int x = K[ i ];
   lab[ x ] = pos % 2 == 0 ? MINUS : PLUS;
   tree[ x ] = t;
   if( pos == 0 ) {
    tpar[ x ] = pe;
    addChild( p , x );
    }
   else if( pos % 2 == 0 ) {
    tpar[ x ] = KE[ step == 1 ? ( i - 1 + k ) % k : i ];
    addChild( prev , x );
    }
   prev = x;
   if( i == 0 )
    break;
   }
  for( int x : K )
   y[ x ] -= lab[ x ] * E;
  collected.clear();
  for( int x : K )  // the edges between two children, now apart
   forEnds( x , [ & ]( int h ) {
    const int o = hd( h ^ 1 );
    if( ( ( h & 1 ) == 0 ) && ( bparent[ o ] == B ) ) {
     dequeue( h >> 1 );
     s( h >> 1 ) += ( lab[ x ] + lab[ o ] ) * E;
     collected.push_back( h >> 1 );
     }
    } );
  ++memoStamp;
  forEnds( B , [ & ]( int h ) {  // the edges that left B, to its children
   const int x = childOfMemo( B , vertexOf( h ) );
   unlink( h );
   link( h , x );
   dequeue( h >> 1 );
   s( h >> 1 ) += ( lab[ x ] + 1 ) * E;
   collected.push_back( h >> 1 );
   } );
  for( int x : K ) {
   bparent[ x ] = NIL;
   if( ( x >= n ) && ( lab[ x ] == MINUS ) ) {
    qb[ x ].priority = y[ x ];
    T[ std::size_t( t ) ].qB.insert( &qb[ x ] );
    }
   }
  lab[ B ] = FREE;
  tree[ B ] = NIL;
  tpar[ B ] = NIL;
  balive[ B ] = false;
  K.clear();
  KE.clear();
  kend[ B ].clear();
  freeB.push_back( B );
  for( int f : collected )
   place( f );
  }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE OPERATIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

 /// the tight edge e from an outer node of cur to a free node grows the
 /// tree by the free node, inner, and its mate, outer: their duals and the
 /// slacks of their edges kept, each edge placed in the queue of its new
 /// class in one pass over the ends of each node (an edge between the two
 /// nodes twice, the second time with its slack complete)
 void grow( int e ) {
  int u = hd( 2 * e ) , v = hd( 2 * e + 1 );
  if( lab[ u ] != PLUS )
   std::swap( u , v );
  const int t = tree[ u ];
  const D E = eps( t );
  const int w = mateNode( v );
  tpar[ v ] = e;
  fchild[ v ] = NIL;
  addChild( u , v );
  tpar[ w ] = NIL;
  fchild[ w ] = NIL;
  y[ v ] += E;  // before the queue of an inner blossom reads it
  y[ w ] -= E;
  setLabel( v , MINUS , t );
  setLabel( w , PLUS , t );
  Tree & R = T[ std::size_t( t ) ];
  auto into = [ & ]( EdgeRec & r , Queue * q ) {
   r.where = q;
   r.q.child = r.q.next = r.q.prev = nullptr;  // perhaps of a queue dropped
   q->h.insert( &r.q );
   };
  forEnds( v , [ & ]( int k ) {  // v inner: from the outer nodes of others
   EdgeRec & r = ed[ std::size_t( k >> 1 ) ];
   if( r.where ) {
    r.where->h.erase( &r.q );
    r.where = nullptr;
    }
   r.q.priority -= E;
   const int o = hd( k ^ 1 );
   if( ( lab[ o ] == PLUS ) && ( tree[ o ] != t ) ) {
    TreeEdge & X = teOf( tree[ o ] , t );
    into( r , &X.pm[ X.t[ 0 ] == t ? 1 : 0 ] );
    }
   } );
  forEnds( w , [ & ]( int k ) {  // w outer
   const int f = k >> 1;
   EdgeRec & r = ed[ std::size_t( f ) ];
   if( r.where ) {
    r.where->h.erase( &r.q );
    r.where = nullptr;
    }
   r.q.priority += E;
   const int o = hd( k ^ 1 );
   if( lab[ o ] == FREE )
    into( r , &R.q0 );
   else if( lab[ o ] == PLUS ) {
    if( tree[ o ] == t )
     into( r , &R.q00 );
    else {
     into( r , &teOf( t , tree[ o ] ).pp );
     if( r.q.priority - E - eps( tree[ o ] ) == 0 ) {
      pendingAug = f;
      R.hot = T[ std::size_t( tree[ o ] ) ].hot = true;
      }
     }
    }
   else if( tree[ o ] != t ) {
    TreeEdge & X = teOf( t , tree[ o ] );
    into( r , &X.pm[ X.t[ 0 ] == t ? 0 : 1 ] );
    }
   } );
  }

 /// the tight edge e between the outer nodes of two trees augments the
 /// matching along the paths to their roots, and the two trees dissolve
 void augment( int e ) {
  ++augments;
  const int t1 = tree[ hd( 2 * e ) ] , t2 = tree[ hd( 2 * e + 1 ) ];
  treeNodes( t1 , N1 );
  treeNodes( t2 , N2 );
  for( int side = 0 ; side < 2 ; ++side ) {
   int x = hd( 2 * e + side ) , at = side ? ev[ e ] : eu[ e ] , ne = e;
   while( true ) {
    const int w = mateNode( x );
    rebase( x , at );
    base[ x ] = at;
    mate[ at ] = ne;
    if( w == NIL )
     break;
    const int pe = tpar[ w ] , tm = endIn( pe , w );
    rebase( w , tm );
    base[ w ] = tm;
    mate[ tm ] = pe;
    at = other( pe , tm );
    x = across( pe , w );
    ne = pe;
    }
   }
  // the two trees dissolve: all their nodes free, their queues dropped,
  // and their edges, all of which were in those queues or in none, out
  // of any queue but those to the outer nodes of the other trees
  for( const auto * L : { &N1 , &N2 } )
   for( int x : *L ) {
    const D d = -shift( x );
    y[ x ] -= d;
    forEnds( x , [ & ]( int k ) { s( k >> 1 ) += d; } );
    }
  for( const auto * L : { &N1 , &N2 } )
   for( int x : *L ) {
    setLabel( x , FREE , NIL );
    tpar[ x ] = fchild[ x ] = snext[ x ] = sprev[ x ] = NIL;
    }
  killTree( t1 );
  killTree( t2 );
  cur = NIL;
  ++stamp;
  for( const auto * L : { &N1 , &N2 } )
   for( int x : *L )
    forEnds( x , [ & ]( int k ) {
     ed[ std::size_t( k >> 1 ) ].where = nullptr;
     if( lab[ hd( k ^ 1 ) ] == PLUS )
      place( k >> 1 );
     } );
  ++stamp;
  }

 /// the primal operations on the tree t while one is possible
 void processTree( int t , bool & progress ) {
  Tree & R = T[ std::size_t( t ) ];
  if( ! R.hot &&
      ( R.q0.h.empty() || ( R.q0.h.min()->priority - R.eps != 0 ) ) &&
      ( R.q00.h.empty() || ( R.q00.h.min()->priority - 2 * R.eps != 0 ) ) &&
      ( R.qB.empty() || ( R.qB.min()->priority - R.eps != 0 ) ) )
   return;  // nothing tight
  setCur( t );
  if( R.hot ) {
   R.hot = false;
   int e = NIL;
   forTreeEdges( t , [ & ]( TreeEdge & X , int ) {
    if( least( X.pp , eps( X.t[ 0 ] ) , eps( X.t[ 1 ] ) ) != 0 )
     return( false );
    e = edgeOf( X.pp.h.min() );
    return( true );
    } );
   if( e != NIL ) {
    augment( e );
    progress = true;
    return;
    }
   }
  pendingAug = NIL;
  while( true ) {
   if( pendingAug != NIL ) {
    const int e = pendingAug;
    pendingAug = NIL;
    const int u = hd( 2 * e ) , v = hd( 2 * e + 1 );
    if( ( lab[ u ] == PLUS ) && ( lab[ v ] == PLUS ) &&
	( tree[ u ] != tree[ v ] ) && ( slack( e ) == 0 ) ) {
     augment( e );
     progress = true;
     return;
     }
    }
   if( ! R.q0.h.empty() && ( R.q0.h.min()->priority - R.eps == 0 ) )
    grow( edgeOf( R.q0.h.min() ) );
   else if( ! R.q00.h.empty() && ( R.q00.h.min()->priority - 2 * R.eps == 0 ) )
    shrink( edgeOf( R.q00.h.min() ) );
   else if( ! R.qB.empty() && ( R.qB.min()->priority - R.eps == 0 ) )
    expand( int( R.qB.min() - qb.data() ) );
   else
    break;
   progress = true;
   }
  }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DUAL UPDATE -----------------------------*/
/*--------------------------------------------------------------------------*/

 std::vector< D > delta;            ///< the change of the duals of each tree
 std::vector< signed char > state;  ///< 0 untaken, 1 in the component, 2 done
 std::vector< int > comp;           ///< the trees of the component taken

 /// the least slack of the edges of the queue q, less the changes of the
 /// duals a and b of their ends, INF if none
 static D least( const Queue & q , D a , D b ) {
  return( q.h.empty() ? INF : q.h.min()->priority - a - b );
  }

 /// the change of the duals of each tree, the same in each component of
 /// the trees joined by the tight edges from an outer node of one to an
 /// inner node of another, each component found by one visit of the edges
 /// between its trees, which bounds its change by the changes of the
 /// components already taken (and by none for those after it, which are
 /// bounded by it in turn); false if a change is unbounded
 bool dualUpdate( bool & moved ) {
  const std::size_t NT = T.size();
  delta.resize( NT );
  state.resize( NT );
  for( int t : alive ) {
   Tree & R = T[ std::size_t( t ) ];
   D b = least( R.q0 , R.eps , 0 );
   if( ! R.q00.h.empty() )
    b = std::min( b , ( R.q00.h.min()->priority - 2 * R.eps ) / 2 );
   if( ! R.qB.empty() )
    b = std::min( b , R.qB.min()->priority - R.eps );
   delta[ std::size_t( t ) ] = b;
   state[ std::size_t( t ) ] = 0;
   }
  for( int t0 : alive ) {
   if( state[ std::size_t( t0 ) ] != 0 )
    continue;
   comp.assign( 1 , t0 );
   state[ std::size_t( t0 ) ] = 1;
   D d = delta[ std::size_t( t0 ) ];
   for( std::size_t h = 0 ; h < comp.size() ; ++h ) {
    const int t = comp[ h ];
    const D et = eps( t );
    forTreeEdges( t , [ & ]( TreeEdge & X , int k ) {
     const int o = X.t[ 1 - k ];
     const D eo = eps( o );
     const D pp = least( X.pp , et , eo );
     if( state[ std::size_t( o ) ] == 1 ) {  // within the component
      d = std::min( d , pp / 2 );
      return( false );
      }
     const D out = least( X.pm[ k ] , et , -eo );
     D e2 = 0;  // the change of o, 0 for a component after this one
     if( state[ std::size_t( o ) ] == 2 )
      e2 = delta[ std::size_t( o ) ];
     else if( ( out <= 0 ) || ( least( X.pm[ 1 - k ] , eo , -et ) <= 0 ) ) {
      state[ std::size_t( o ) ] = 1;  // o joins the component
      comp.push_back( o );
      d = std::min( { d , pp , delta[ std::size_t( o ) ] } );
      return( false );
      }
     d = std::min( { d , pp - e2 , e2 + out } );
     return( false );
     } );
    }
   if( d >= INF / 2 )
    return( false );
   if( d < 0 )
    d = 0;
   moved = moved || ( d > 0 );
   for( int t : comp ) {
    delta[ std::size_t( t ) ] = d;
    state[ std::size_t( t ) ] = 2;
    }
   }
  for( int t : alive )
   T[ std::size_t( t ) ].eps += delta[ std::size_t( t ) ];
  for( int t : alive )  // the edges between outer nodes made tight
   if( delta[ std::size_t( t ) ] > 0 )
    forTreeEdges( t , [ & ]( TreeEdge & X , int ) {
     if( least( X.pp , eps( X.t[ 0 ] ) , eps( X.t[ 1 ] ) ) == 0 )
      T[ std::size_t( X.t[ 0 ] ) ].hot = T[ std::size_t( X.t[ 1 ] ) ].hot =
	true;
     return( false );
     } );
  return( true );
  }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE SOLVE --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the duals of the vertices and a matching on the tight edges
 void initialize( bool greedy ) {
  std::fill( y.begin() , y.begin() + n , INF );
  for( int e = 0 ; e < m ; ++e )  // in the order of the edges
   if( eu[ e ] != ev[ e ] ) {
    y[ eu[ e ] ] = std::min( y[ eu[ e ] ] , cost2[ e ] / 2 );
    y[ ev[ e ] ] = std::min( y[ ev[ e ] ] , cost2[ e ] / 2 );
    }
  if( greedy )
   for( int v = 0 ; v < n ; ++v ) {
    if( mate[ v ] != NIL )
     continue;
    D b = INF;
    forEnds( v , [ & ]( int k ) {
     const int e = k >> 1;
     b = std::min( b , cost2[ e ] - y[ v ] - y[ other( e , v ) ] );
     } );
    y[ v ] += b;
    for( int k = first[ lid[ std::size_t( v ) ] ] ; k != NIL ;
	 k = anext( k ) ) {
     const int e = k >> 1 , u = other( e , v );
     if( ( mate[ u ] == NIL ) && ( cost2[ e ] - y[ v ] - y[ u ] == 0 ) ) {
      mate[ u ] = mate[ v ] = e;
      break;
      }
     }
    }
  for( int e = 0 ; e < m ; ++e )
   s( e ) = cost2[ e ] - y[ eu[ e ] ] - y[ ev[ e ] ];
  T.reserve( std::size_t( std::count( mate.begin() , mate.end() , NIL ) ) );
  for( int v = 0 ; v < n ; ++v )
   if( mate[ v ] == NIL ) {
    const int t = newTree( v );
    lab[ v ] = PLUS;
    tree[ v ] = t;
    }
  for( int v = 0 ; v < n ; ++v )
   if( mate[ v ] == NIL ) {
    setCur( tree[ v ] );
    forEnds( v , [ & ]( int k ) {
     const int u = hd( k ^ 1 );
     if( ( lab[ u ] != PLUS ) || ( v < u ) )  // else placed from u
      place( k >> 1 );
     } );
    }
  cur = NIL;
  ++stamp;
  }

 /// a perfect matching of least cost, or infeasible
 void solve( bool greedy ) {
  if( n % 2 == 1 ) {
   infeasible = true;
   return;
   }
  for( int v = 0 ; v < n ; ++v )
   if( cnt[ lid[ std::size_t( v ) ] ] == 0 ) {
    infeasible = true;
    return;
    }
  initialize( greedy );
  std::vector< int > L;
  while( ! alive.empty() ) {
   bool progress = false;
   const std::size_t a0 = augments;
   L = alive;
   for( int t : L )
    if( T[ std::size_t( t ) ].at != NIL )
     processTree( t , progress );
   cur = NIL;
   ++stamp;
   if( alive.empty() )
    break;
   if( augments != a0 )
    continue;  // the trees left may grow on the nodes freed
   bool moved = false;
   if( ! dualUpdate( moved ) ) {
    infeasible = true;
    return;
    }
   if( ! progress && ! moved )
    throw( std::logic_error( "minCostPerfectMatching: no progress" ) );
   }
  }

 /// whether every slack is nonnegative, every dual of a blossom too, the
 /// matched edges tight, and the duals sum to twice the cost of the
 /// matching
 bool certified( void ) {
  for( int b = n ; b < int( balive.size() ) ; ++b )
   if( balive[ b ] && ( y[ b ] < 0 ) )
    return( false );
  std::vector< int > A , Bc;
  auto chain = [ & ]( int a , std::vector< int > & C ) {
   C.clear();
   for( int x = a ; x != NIL ; x = bparent[ x ] )
    C.push_back( x );
   };
  for( int e = 0 ; e < m ; ++e ) {
   if( eu[ e ] == ev[ e ] )
    continue;
   chain( eu[ e ] , A );
   chain( ev[ e ] , Bc );
   while( ! A.empty() && ! Bc.empty() && ( A.back() == Bc.back() ) ) {
    A.pop_back();
    Bc.pop_back();
    }
   D sl = cost2[ e ];
   for( int x : A )
    sl -= y[ x ];
   for( int x : Bc )
    sl -= y[ x ];
   if( ( sl < 0 ) || ( ( mate[ eu[ e ] ] == e ) && ( sl != 0 ) ) )
    return( false );
   }
  D dual = 0 , primal = 0;
  for( int x = 0 ; x < int( balive.size() ) ; ++x )
   if( ( x < n ) || balive[ x ] )
    dual += y[ x ];
  for( int v = 0 ; v < n ; ++v ) {
   if( ( mate[ v ] == NIL ) || ( mate[ other( mate[ v ] ,
					      v ) ] != mate[ v ] ) )
    return( false );
   if( v < other( mate[ v ] , v ) )
    primal += cost2[ mate[ v ] ];
   }
  return( dual == primal );
  }
 };

/// the perfect matching of least cost of the graph of n nodes and of the
/// edges ( U[ e ] , V[ e ] ) of the costs C[ e ]
template< class W >
PerfectMatchingResult< W >
perfectMatching( int n , const std::vector< int > & U ,
		 const std::vector< int > & V , const std::vector< W > & C ,
		 const BlossomVOptions & o )
{
 std::vector< long long > C2( C.size() );
 for( std::size_t e = 0 ; e < C.size() ; ++e )
  C2[ e ] = 2 * ( long long )C[ e ];
 PerfectBlossoms P( n , U , V , C2 );
 P.solve( o.greedy );
 PerfectMatchingResult< W > R;
 if( P.infeasible )
  return( R );
 ADS_AUDIT( P.certified() );
 R.perfect = true;
 for( int v = 0 ; v < n ; ++v ) {
  const int e = P.mate[ std::size_t( v ) ];
  if( v < P.other( e , v ) ) {
   R.edges.push_back( std::size_t( e ) );
   R.cost += C[ std::size_t( e ) ];
   }
  }
 std::sort( R.edges.begin() , R.edges.end() );
 std::vector< int > id( P.balive.size() , NIL );
 int k = n;
 for( int b = n ; b < int( P.balive.size() ) ; ++b )
  if( P.balive[ std::size_t( b ) ] )
   id[ std::size_t( b ) ] = k++;
 R.parent.assign( std::size_t( k ) , NIL );
 R.dual.assign( std::size_t( k ) , W( 0 ) );
 for( int x = 0 ; x < int( P.balive.size() ) ; ++x ) {
  const int i = x < n ? x : id[ std::size_t( x ) ];
  if( i == NIL )
   continue;
  const int p = P.bparent[ std::size_t( x ) ];
  R.parent[ std::size_t( i ) ] = p == NIL ? NIL : id[ std::size_t( p ) ];
  R.dual[ std::size_t( i ) ] = W( P.y[ std::size_t( x ) ] );
  }
 return( R );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class W >
 requires std::is_integral_v< W > && std::is_signed_v< W >
PerfectMatchingResult< W >
minCostPerfectMatching( int n ,
			const std::vector< std::tuple< int , int , W > > & E ,
			const BlossomVOptions & o )
{
 ADS_PRE( n >= 0 );
 std::vector< int > U , V;
 std::vector< W > C;
 U.reserve( E.size() );
 V.reserve( E.size() );
 C.reserve( E.size() );
 for( const auto & [ u , v , c ] : E ) {
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) );
  U.push_back( u );
  V.push_back( v );
  C.push_back( c );
  }
 return( detail::perfectMatching( n , U , V , C , o ) );
 }

template< class W , ArcLayout L >
 requires std::is_integral_v< W > && std::is_signed_v< W >
PerfectMatchingResult< W >
minCostPerfectMatching( const StaticGraph< W , L > & g ,
			const BlossomVOptions & o )
{
 ADS_PRE( ! g.directed() );
 std::vector< int > U , V;
 std::vector< W > C;
 std::vector< std::size_t > place;
 std::size_t k = 0;
 for( int u = 0 ; u < g.n() ; ++u )
  for( const auto & a : g.out( u ) ) {
   if( u < a.to ) {
    U.push_back( u );
    V.push_back( a.to );
    C.push_back( a.w );
    place.push_back( k );
    }
   ++k;
   }
 PerfectMatchingResult< W > R = detail::perfectMatching( g.n() , U , V , C ,
							 o );
 for( auto & e : R.edges )
  e = place[ e ];
 return( R );
 }

template< class W >
 requires std::is_integral_v< W > && std::is_signed_v< W >
std::pair< W , std::vector< std::size_t > >
maxWeightMatching( int n ,
		   const std::vector< std::tuple< int , int , W > > & E ,
		   const BlossomVOptions & o )
{
 ADS_PRE( n >= 0 );
 std::vector< int > U , V;
 std::vector< W > C;
 std::vector< std::size_t > id;  // of the edges of the first copy
 for( std::size_t e = 0 ; e < E.size() ; ++e ) {
  const auto & [ u , v , w ] = E[ e ];
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) );
  if( ( u != v ) && ( W( 0 ) < w ) ) {
   U.push_back( u );
   V.push_back( v );
   C.push_back( -w );
   id.push_back( e );
   }
  }
 const std::size_t h = id.size();
 for( std::size_t i = 0 ; i < h ; ++i ) {  // the second copy
  U.push_back( U[ i ] + n );
  V.push_back( V[ i ] + n );
  C.push_back( C[ i ] );
  }
 for( int v = 0 ; v < n ; ++v ) {  // each node to its copy
  U.push_back( v );
  V.push_back( v + n );
  C.push_back( W( 0 ) );
  }
 const PerfectMatchingResult< W > R =
   detail::perfectMatching( 2 * n , U , V , C , o );
 std::vector< std::size_t > M;
 W total( 0 );
 for( std::size_t e : R.edges )
  if( e < h ) {
   M.push_back( id[ e ] );
   total += std::get< 2 >( E[ id[ e ] ] );
   }
 std::sort( M.begin() , M.end() );
 ADS_POST( 2 * total == -R.cost );
 return( std::pair( total , M ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File PerfectMatching.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
