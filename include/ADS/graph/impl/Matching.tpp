/*--------------------------------------------------------------------------*/
/*--------------------------- File Matching.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The matchings of Matching.h [see Matching.h], with their contracts.
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

namespace detail {

/// the branch and bound of maxWeightMatching() from the node u
template< class W >
struct MatchingSearch {
 int n;
 const std::vector< std::tuple< int , int , W > > & E;
 std::vector< std::vector< std::size_t > > inc;   ///< the edges to the
                                                  ///< later nodes
 std::vector< W > rest;    ///< the best weights at the nodes >= u, summed
 std::vector< bool > used;
 std::vector< std::size_t > S , best;
 W w , bw;                 ///< the weight of S and of best

 void search( int u ) {
  while( ( u < n ) && used[ u ] )
   ++u;
  if( u == n ) {
   if( bw < w ) {
    bw = w;
    best = S;
    }
   return;
   }
  if( ! ( w + w + rest[ u ] > bw + bw ) )   // no better matching below
   return;
  for( std::size_t e : inc[ u ] ) {
   const auto & [ a , b , c ] = E[ e ];
   const int v = a == u ? b : a;          // the later end
   if( ! used[ v ] ) {
    used[ u ] = used[ v ] = true;
    S.push_back( e );
    w += c;
    search( u + 1 );
    w -= c;
    S.pop_back();
    used[ u ] = used[ v ] = false;
    }
   }
  search( u + 1 );                          // u unmatched
  }
 };

/// the searches of Edmonds of maxCardinalityMatching(), one free node at a
/// time, the blossoms as the sets of a union-find whose representatives
/// know their bases
template< Graph G >
struct CardinalityBlossom {
 enum Label : unsigned char { NONE , EVEN , ODD , BRIDGED };

 const Rows< G > & A;
 std::vector< int > mate;
 std::vector< Label > label;
 std::vector< int > parent;               ///< of an odd node, the even one
 std::vector< int > ba , bb;              ///< of a bridged node, its edge
 std::vector< int > link , base;          ///< the union-find of the blossoms
 std::vector< unsigned char > rank;
 std::vector< bool > mark , dead;
 std::vector< int > touched , Q , walked;
 std::vector< std::pair< int , int > > St;

 CardinalityBlossom( const Rows< G > & a , int n )
  : A( a ) , mate( n , NIL ) , label( n , NONE ) , parent( n , NIL ) ,
    ba( n , NIL ) , bb( n , NIL ) , link( n ) , base( n ) , rank( n , 0 ) ,
    mark( n , false ) , dead( n , false ) {
  for( int v = 0 ; v < n ; ++v )
   link[ v ] = base[ v ] = v;
  }

 int find( int v ) {
  while( link[ v ] != v ) {
   link[ v ] = link[ link[ v ] ];         // halving
   v = link[ v ];
   }
  return( v );
  }

 /// the base of the outermost blossom of v
 int B( int v ) { return( base[ find( v ) ] ); }

 /// the blossoms of u and v merged, of base b
 void unite( int u , int v , int b ) {
  u = find( u );
  v = find( v );
  if( u != v ) {
   if( rank[ u ] < rank[ v ] )
    std::swap( u , v );
   link[ v ] = u;
   if( rank[ u ] == rank[ v ] )
    ++rank[ u ];
   }
  base[ u ] = b;
  }

 /// the first common base of the paths from the bases a and b to the root
 /// r, walked one step on each side in turn
 int meet( int a , int b , int r ) {
  int c = NIL;
  walked.clear();
  for( ; ; std::swap( a , b ) )
   if( a != NIL ) {
    if( mark[ a ] ) {
     c = a;
     break;
     }
    mark[ a ] = true;
    walked.push_back( a );
    a = a == r ? NIL : B( parent[ mate[ a ] ] );
    }
  for( int v : walked )
   mark[ v ] = false;
  return( c );
  }

 /// the odd nodes on the path from B( x ) to the base c become even, with
 /// the edge ( x , y ), and the blossoms on it are merged into that of c
 void shrink( int x , int y , int c ) {
  for( int v = B( x ) ; v != c ; ) {
   const int o = mate[ v ];
   label[ o ] = BRIDGED;
   ba[ o ] = x;
   bb[ o ] = y;
   Q.push_back( o );
   const int next = B( parent[ o ] );
   unite( v , c , c );
   unite( o , c , c );
   v = next;
   }
  }

 /// the matching along y and the path from x to the root, y free
 void augment( int x , int y ) {
  mate[ y ] = x;
  St.assign( 1 , { x , y } );
//! [maxCardinalityMatching-augment]
  while( ! St.empty() ) {
   auto [ v , w ] = St.back();
   St.pop_back();
   for( ; ; ) {                            // v takes w, the rest of P( v )
    const int t = mate[ v ];               // is rematched
    mate[ v ] = w;
    if( ( t == NIL ) || ( mate[ t ] != v ) )
     break;                                // the root, or a part done
    if( label[ v ] == EVEN ) {             // P( v ) = v , t , P( p[ t ] )
     const int u = parent[ t ];
     mate[ t ] = u;
     w = t;
     v = u;
     }
    else {                                 // P( v ) = v .. a , P( b )
     St.emplace_back( bb[ v ] , ba[ v ] );
     w = bb[ v ];
     v = ba[ v ];
     }
    }
   }
//! [maxCardinalityMatching-augment]
  }

 /// the search from the free node r: true if the matching grew, false if
 /// the nodes of the tree, which no augmenting path will cross, are out
 bool search( int r ) {
  touched.assign( 1 , r );
  Q.assign( 1 , r );
  label[ r ] = EVEN;
  bool grown = false;
//! [maxCardinalityMatching-search]
  for( std::size_t q = 0 ; ( q < Q.size() ) && ! grown ; ++q ) {
   const int x = Q[ q ];
   for( std::size_t i = A.first( x ) ; i < A.last( x ) ; ++i ) {
    const int y = A.head( i );
    if( dead[ y ] || ( label[ y ] == ODD ) || ( y == x ) )
     continue;
    if( label[ y ] == NONE ) {
     if( mate[ y ] == NIL ) {               // an augmenting path
      augment( x , y );
      grown = true;
      break;
      }
     label[ y ] = ODD;                      // the tree grows by two
     parent[ y ] = x;
     const int z = mate[ y ];
     label[ z ] = EVEN;
     touched.push_back( y );
     touched.push_back( z );
     Q.push_back( z );
     }
    else {
     const int bx = B( x ) , by = B( y );
     if( bx != by ) {                       // a blossom
      const int c = meet( bx , by , r );
      shrink( x , y , c );
      shrink( y , x , c );
      }
     }
    }
   }
//! [maxCardinalityMatching-search]
  for( int v : touched ) {
   if( ! grown )
    dead[ v ] = true;
   label[ v ] = NONE;
   link[ v ] = base[ v ] = v;
   rank[ v ] = 0;
   }
  return( grown );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< std::pair< int , int > > bipartiteBMatching(
                      const std::vector< std::vector< int > > & A , int nr ,
                      const std::vector< long long > & capL ,
                      const std::vector< long long > & capR )
{
 const int nl = int( A.size() );
 ADS_PRE( ( nr >= 0 ) && ( int( capL.size() ) == nl ) &&
          ( int( capR.size() ) == nr ) );
 FlowNetwork N( nl + nr + 2 );
 const int s = nl + nr , t = s + 1;
 std::vector< std::tuple< int , int , int > > id;   // edge , u , v
 for( int u = 0 ; u < nl ; ++u ) {
  ADS_PRE( capL[ u ] >= 0 );
  N.addEdge( s , u , capL[ u ] );
  for( int v : A[ u ] ) {
   ADS_PRE( ( v >= 0 ) && ( v < nr ) );
   id.emplace_back( N.addEdge( u , nl + v , 1 ) , u , v );
   }
  }
 for( int v = 0 ; v < nr ; ++v ) {
  ADS_PRE( capR[ v ] >= 0 );
  N.addEdge( nl + v , t , capR[ v ] );
  }
 N.maxFlow( s , t );
 std::vector< std::pair< int , int > > M;
 for( const auto & [ e , u , v ] : id )
  if( N.flow( e ) == 1 )
   M.emplace_back( u , v );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< long long > dl( nl , 0 );
  std::vector< long long > dr( nr , 0 );
  for( const auto & uv : M )
   if( ( ++dl[ uv.first ] > capL[ uv.first ] ) ||
       ( ++dr[ uv.second ] > capR[ uv.second ] ) )
    return( false );
  return( true );
  }() );
 return( M );
 }

template< Graph G >
std::vector< int > maxCardinalityMatching( const G & g )
{
 ADS_PRE( ! g.directed() );
 const int n = g.bound();
 const detail::Rows< G > A( g );
 detail::CardinalityBlossom< G > S( A , n );
 for( int u = 0 ; u < n ; ++u )               // a greedy matching
  if( g.hasNode( u ) && ( S.mate[ u ] == NIL ) )
   for( std::size_t i = A.first( u ) ; i < A.last( u ) ; ++i ) {
    const int v = A.head( i );
    if( ( v != u ) && ( S.mate[ v ] == NIL ) ) {
     S.mate[ u ] = v;
     S.mate[ v ] = u;
     break;
     }
    }
 for( int r = 0 ; r < n ; ++r )
  if( g.hasNode( r ) && ( S.mate[ r ] == NIL ) && ! S.dead[ r ] )
   S.search( r );
 ADS_AUDIT( [ & ]( void ) {
  for( int u = 0 ; u < n ; ++u ) {
   const int v = S.mate[ u ];
   if( v == NIL )
    continue;
   if( ! g.hasNode( u ) || ( v < 0 ) || ( v >= n ) || ( S.mate[ v ] != u ) )
    return( false );
   bool edge = false;
   g.forEachAdj( u , [ & ]( int x ) { edge = edge || ( x == v ); } );
   if( ! edge )
    return( false );
   }
  return( true );
  }() );
 return( std::move( S.mate ) );
 }

template< class W >
std::pair< W , std::vector< std::size_t > > maxWeightMatching( int n ,
                     const std::vector< std::tuple< int , int , W > > & E )
{
 ADS_PRE( n >= 0 );
 detail::MatchingSearch< W > M{ n , E ,
                             std::vector< std::vector< std::size_t > >( n ) ,
                             std::vector< W >( n + 1 , W( 0 ) ) ,
                             std::vector< bool >( n , false ) , {} , {} ,
                             W( 0 ) , W( 0 ) };
 std::vector< W > top( n , W( 0 ) );        // the best weight at each node
 for( std::size_t e = 0 ; e < E.size() ; ++e ) {
  const auto & [ u , v , w ] = E[ e ];
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) &&
           ( u != v ) );
  if( W( 0 ) < w ) {                     // the others never help
   M.inc[ std::min( u , v ) ].push_back( e );
   top[ u ] = std::max( top[ u ] , w );
   top[ v ] = std::max( top[ v ] , w );
   }
  }
 for( int u = n ; u-- > 0 ; )
  M.rest[ u ] = M.rest[ u + 1 ] + top[ u ];
 M.search( 0 );
 std::sort( M.best.begin() , M.best.end() );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< bool > end( n , false );
  for( std::size_t e : M.best ) {
   const int u = std::get< 0 >( E[ e ] );
   const int v = std::get< 1 >( E[ e ] );
   if( end[ u ] || end[ v ] )
    return( false );
   end[ u ] = end[ v ] = true;
   }
  return( true );
  }() );
 return( std::pair( M.bw , M.best ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Matching.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
