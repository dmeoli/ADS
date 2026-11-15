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

namespace ADS
{

namespace detail
{

/// the branch and bound of branchAndBoundMatching() from the node u
template< class W >
struct MatchingSearch
{
 int n;
 const std::vector< std::tuple< int , int , W > > & E;
 std::vector< std::vector< std::size_t > > inc;  ///< the edges to the
						 ///< later nodes
 std::vector< W > rest;  ///< the best weights at the nodes >= u, summed
 std::vector< bool > used;
 std::vector< std::size_t > S , best;
 W w , bw;  ///< the weight of S and of best

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
  if( ! ( w + w + rest[ u ] > bw + bw ) )  // no better matching below
   return;
  for( std::size_t e : inc[ u ] ) {
   const auto & [ a , b , c ] = E[ e ];
   const int v = a == u ? b : a;  // the later end
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
  search( u + 1 );  // u unmatched
  }
 };

/// the searches of Edmonds of maxCardinalityMatching(), one free node at a
/// time, the blossoms as the sets of a union-find whose representatives
/// know their bases
template< Graph G >
struct CardinalityBlossom
{
 enum Label : unsigned char { NONE , EVEN , ODD , BRIDGED };

 const Rows< G > & A;
 std::vector< int > mate;
 std::vector< Label > label;
 std::vector< int > parent;      ///< of an odd node, the even one
 std::vector< int > ba , bb;      ///< of a bridged node, its edge
 std::vector< int > link , base;  ///< the union-find of the blossoms
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
   link[ v ] = link[ link[ v ] ];  // halving
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
  while( ! St.empty() ) {
   auto [ v , w ] = St.back();
   St.pop_back();
   for( ; ; ) {                // v takes w, the rest of P( v )
    const int t = mate[ v ];  // is rematched
    mate[ v ] = w;
    if( ( t == NIL ) || ( mate[ t ] != v ) )
     break;                     // the root, or a part done
    if( label[ v ] == EVEN ) {  // P( v ) = v , t , P( p[ t ] )
     const int u = parent[ t ];
     mate[ t ] = u;
     w = t;
     v = u;
     }
    else {  // P( v ) = v .. a , P( b )
     St.emplace_back( bb[ v ] , ba[ v ] );
     w = bb[ v ];
     v = ba[ v ];
     }
    }
   }
  }

 /// the search from the free node r: true if the matching grew, false if
 /// the nodes of the tree, which no augmenting path will cross, are out
 bool search( int r ) {
  touched.assign( 1 , r );
  Q.assign( 1 , r );
  label[ r ] = EVEN;
  bool grown = false;
  for( std::size_t q = 0 ; ( q < Q.size() ) && ! grown ; ++q ) {
   const int x = Q[ q ];
   for( std::size_t i = A.first( x ) ; i < A.last( x ) ; ++i ) {
    const int y = A.head( i );
    if( dead[ y ] || ( label[ y ] == ODD ) || ( y == x ) )
     continue;
    if( label[ y ] == NONE ) {
     if( mate[ y ] == NIL ) {  // an augmenting path
      augment( x , y );
      grown = true;
      break;
      }
     label[ y ] = ODD;  // the tree grows by two
     parent[ y ] = x;
     const int z = mate[ y ];
     label[ z ] = EVEN;
     touched.push_back( y );
     touched.push_back( z );
     Q.push_back( z );
     }
    else {
     const int bx = B( x ) , by = B( y );
     if( bx != by ) {  // a blossom
      const int c = meet( bx , by , r );
      shrink( x , y , c );
      shrink( y , x , c );
      }
     }
    }
   }
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

/// the primal-dual algorithm of maxWeightMatching() on the blossoms: the
/// edge k joins the ends 2 k and 2 k + 1, endpoint[ p ] is the node of the
/// end p and p ^ 1 is the other end of its edge; the names of the blossoms
/// are n .. 2 n - 1, those of the nodes 0 .. n - 1 the trivial blossoms
template< class W >
struct WeightedBlossom
{
 int n;
 bool maxCardinality;
 std::vector< std::tuple< int , int , W > > E;  ///< the edges kept
 std::vector< std::size_t > id;               ///< their places given
 std::vector< int > endpoint;
 std::vector< std::vector< int > > adj;     ///< of each node, the far ends
 std::vector< int > mate;                   ///< of a node, the far end of its
					    ///< edge in M, or -1
 std::vector< int > label;                  ///< 0 free, 1 S, 2 T
 std::vector< int > labelend;               ///< the end through which a node
					    ///< or a blossom got its label
 std::vector< int > inblossom;              ///< the outermost blossom
 std::vector< int > parent;                 ///< the blossom just around
 std::vector< std::vector< int > > childs;  ///< the cycle, from the base
 std::vector< std::vector< int > > endps;   ///< the ends of the cycle
 std::vector< int > base;
 std::vector< int > bestedge;                  ///< the least slack to S
 std::vector< std::vector< int > > bestedges;  ///< of an S-blossom
 std::vector< bool > hasBest;
 std::vector< int > unused;
 std::vector< W > dual;
 std::vector< bool > allowed;  ///< tight edges known
 std::vector< int > queue , path , ends , stack;
 std::vector< bool > scanned;
 std::vector< int > bestto;
 std::vector< std::pair< int , int > > tasks;

 WeightedBlossom( int nn ,
		  const std::vector< std::tuple< int , int , W > > & F ,
		  bool mc )
     : n( nn ) , maxCardinality( mc ) {
  W top( 0 );
  for( std::size_t k = 0 ; k < F.size() ; ++k ) {
   const auto & [ u , v , w ] = F[ k ];
   ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) && ( u != v ) );
   if( mc || ( W( 0 ) < w ) ) {  // a light edge never helps
    E.push_back( F[ k ] );
    id.push_back( k );
    top = std::max( top , w );
    }
   }
  const std::size_t m = E.size();
  endpoint.resize( 2 * m );
  adj.assign( n , {} );
  for( std::size_t k = 0 ; k < m ; ++k ) {
   const auto & [ u , v , w ] = E[ k ];
   endpoint[ 2 * k ] = u;
   endpoint[ 2 * k + 1 ] = v;
   adj[ u ].push_back( int( 2 * k + 1 ) );
   adj[ v ].push_back( int( 2 * k ) );
   }
  mate.assign( n , -1 );
  label.assign( 2 * n , 0 );
  labelend.assign( 2 * n , -1 );
  inblossom.resize( n );
  for( int v = 0 ; v < n ; ++v )
   inblossom[ v ] = v;
  parent.assign( 2 * n , -1 );
  childs.assign( 2 * n , {} );
  endps.assign( 2 * n , {} );
  base.assign( 2 * n , -1 );
  for( int v = 0 ; v < n ; ++v )
   base[ v ] = v;
  bestedge.assign( 2 * n , -1 );
  bestedges.assign( 2 * n , {} );
  hasBest.assign( 2 * n , false );
  for( int b = 2 * n - 1 ; b >= n ; --b )
   unused.push_back( b );
  dual.assign( 2 * n , W( 0 ) );
  for( int v = 0 ; v < n ; ++v )
   dual[ v ] = top;
  allowed.assign( m , false );
  scanned.assign( 2 * n , false );
  bestto.assign( 2 * n , -1 );
  }

 /// the slack of the edge k, the blossoms around both ends left out
 W slack( int k ) const {
  return( dual[ std::get< 0 >( E[ k ] ) ] + dual[ std::get< 1 >( E[ k ] ) ] -
	   std::get< 2 >( E[ k ] ) - std::get< 2 >( E[ k ] ) );
  }

 /// f( v ) for each node v of the blossom b
 template< class F >
 void leaves( int b , F && f ) {
  if( b < n ) {
   f( b );
   return;
   }
  stack.assign( 1 , b );
  while( ! stack.empty() ) {
   const int t = stack.back();
   stack.pop_back();
   if( t < n )
    f( t );
   else
    for( int c : childs[ t ] )
     stack.push_back( c );
   }
  }

 /// the place j of a cycle of L items, taken modulo L
 static std::size_t at( int j , std::size_t L ) {
  const int l = int( L );
  return( std::size_t( ( ( j % l ) + l ) % l ) );
  }

 /// the node w, through the end p, and its blossom get the label t; the
 /// mate of the base of a T-blossom gets S
 void assignLabel( int w , int t , int p ) {
  for( ; ; ) {
   const int b = inblossom[ w ];
   label[ w ] = label[ b ] = t;
   labelend[ w ] = labelend[ b ] = p;
   bestedge[ w ] = bestedge[ b ] = -1;
   if( t == 1 ) {
    leaves( b , [ & ]( int v ) { queue.push_back( v ); } );
    return;
    }
   const int q = mate[ base[ b ] ];  // T: on to the mate of its base
   w = endpoint[ q ];
   t = 1;
   p = q ^ 1;
   }
  }

 /// the base of the blossom that the edge ( v , w ) between two S-nodes
 /// closes, or -1 if they are in two trees (an augmenting path)
 int scanBlossom( int v , int w ) {
  path.clear();
  int b0 = -1;
  while( ( v != -1 ) || ( w != -1 ) ) {
   int b = inblossom[ v ];
   if( scanned[ b ] ) {
    b0 = base[ b ];
    break;
    }
   path.push_back( b );
   scanned[ b ] = true;
   if( labelend[ b ] == -1 )  // a root
    v = -1;
   else {  // up through a T-blossom
    v = endpoint[ labelend[ b ] ];
    b = inblossom[ v ];
    v = endpoint[ labelend[ b ] ];
    }
   if( w != -1 )
    std::swap( v , w );
   }
  for( int b : path )
   scanned[ b ] = false;
  return( b0 );
  }

 /// the blossom of base b0 closed by the edge k, an S-blossom
 void addBlossom( int b0 , int k ) {
  int v = std::get< 0 >( E[ k ] ) , w = std::get< 1 >( E[ k ] );
  const int bb = inblossom[ b0 ];
  int bv = inblossom[ v ] , bw = inblossom[ w ];
  const int b = unused.back();
  unused.pop_back();
  base[ b ] = b0;
  parent[ b ] = -1;
  parent[ bb ] = b;
  path.clear();
  ends.clear();
  while( bv != bb ) {  // from v down to the base
   parent[ bv ] = b;
   path.push_back( bv );
   ends.push_back( labelend[ bv ] );
   v = endpoint[ labelend[ bv ] ];
   bv = inblossom[ v ];
   }
  path.push_back( bb );
  std::reverse( path.begin() , path.end() );
  std::reverse( ends.begin() , ends.end() );
  ends.push_back( 2 * k );
  while( bw != bb ) {  // and from w back up
   parent[ bw ] = b;
   path.push_back( bw );
   ends.push_back( labelend[ bw ] ^ 1 );
   w = endpoint[ labelend[ bw ] ];
   bw = inblossom[ w ];
   }
  childs[ b ] = path;
  endps[ b ] = ends;
  label[ b ] = 1;
  labelend[ b ] = labelend[ bb ];
  dual[ b ] = W( 0 );
  leaves( b , [ & ]( int x ) {
   if( label[ inblossom[ x ] ] == 2 )  // a T-node becomes S
    queue.push_back( x );
   inblossom[ x ] = b;
   } );
  // the least slack to each other S-blossom, from the lists of the
  // sub-blossoms or from the edges of the nodes of the trivial ones
  std::vector< int > to;
  auto offer = [ & ]( int e ) {
   int j = std::get< 1 >( E[ e ] );
   if( inblossom[ j ] == b )
    j = std::get< 0 >( E[ e ] );
   const int bj = inblossom[ j ];
   if( ( bj != b ) && ( label[ bj ] == 1 ) &&
       ( ( bestto[ bj ] == -1 ) || ( slack( e ) < slack( bestto[ bj ] ) ) ) ) {
    if( bestto[ bj ] == -1 )
     to.push_back( bj );
    bestto[ bj ] = e;
    }
   };
  for( int s : childs[ b ] ) {
   if( hasBest[ s ] )
    for( int e : bestedges[ s ] )
     offer( e );
   else
    leaves( s , [ & ]( int x ) {
     for( int p : adj[ x ] )
      offer( p / 2 );
     } );
   bestedges[ s ].clear();
   hasBest[ s ] = false;
   bestedge[ s ] = -1;
   }
  bestedges[ b ].clear();
  bestedge[ b ] = -1;
  for( int bj : to ) {
   const int e = bestto[ bj ];
   bestto[ bj ] = -1;
   bestedges[ b ].push_back( e );
   if( ( bestedge[ b ] == -1 ) || ( slack( e ) < slack( bestedge[ b ] ) ) )
    bestedge[ b ] = e;
   }
  hasBest[ b ] = true;
  }

 /// the outermost blossom b expanded into its sub-blossoms: at the end of
 /// a stage, with those of dual 0 expanded too; otherwise a T-blossom of
 /// dual 0, whose sub-blossoms on the even side of the path from the edge
 /// that labeled it to its base get the labels T and S in turn
 void expandBlossom( int b0 , bool endstage ) {
  std::vector< int > todo = { b0 };
  while( ! todo.empty() ) {
   const int b = todo.back();
   todo.pop_back();
   for( int s : childs[ b ] ) {
    parent[ s ] = -1;
    if( s < n )
     inblossom[ s ] = s;
    else if( endstage && ( dual[ s ] == W( 0 ) ) )
     todo.push_back( s );
    else
     leaves( s , [ & ]( int x ) { inblossom[ x ] = s; } );
    }
   if( ! endstage && ( label[ b ] == 2 ) )
    relabel( b );
   label[ b ] = labelend[ b ] = -1;
   childs[ b ].clear();
   endps[ b ].clear();
   base[ b ] = -1;
   bestedges[ b ].clear();
   hasBest[ b ] = false;
   bestedge[ b ] = -1;
   unused.push_back( b );
   }
  }

 /// the labels of the sub-blossoms of the T-blossom b just expanded
 void relabel( int b ) {
  const std::vector< int > & C = childs[ b ];
  const std::vector< int > & P = endps[ b ];
  const std::size_t L = C.size();
  const int entry = inblossom[ endpoint[ labelend[ b ] ^ 1 ] ];
  int j = int( std::find( C.begin() , C.end() , entry ) - C.begin() );
  int step , trick;
  if( j & 1 ) {
   j -= int( L );
   step = 1;
   trick = 0;
   }
  else {
   step = -1;
   trick = 1;
   }
  int p = labelend[ b ];
  while( j != 0 ) {  // T and S in turn to the base
   label[ endpoint[ p ^ 1 ] ] = 0;
   label[ endpoint[ P[ at( j - trick , L ) ] ^ trick ^ 1 ] ] = 0;
   assignLabel( endpoint[ p ^ 1 ] , 2 , p );
   allowed[ P[ at( j - trick , L ) ] / 2 ] = true;
   j += step;
   p = P[ at( j - trick , L ) ] ^ trick;
   allowed[ p / 2 ] = true;
   j += step;
   }
  const int bv = C[ at( j , L ) ];  // the base: T, not its mate
  label[ endpoint[ p ^ 1 ] ] = label[ bv ] = 2;
  labelend[ endpoint[ p ^ 1 ] ] = labelend[ bv ] = p;
  bestedge[ bv ] = -1;
  for( j += step ; C[ at( j , L ) ] != entry ; j += step ) {
   const int s = C[ at( j , L ) ];  // the odd side: T if reached
   if( label[ s ] == 1 )
    continue;
   int r = -1;
   leaves( s , [ & ]( int x ) {
    if( ( r == -1 ) && label[ x ] )
     r = x;
    } );
   if( r != -1 ) {
    label[ r ] = 0;
    label[ endpoint[ mate[ base[ s ] ] ] ] = 0;
    assignLabel( r , 2 , labelend[ r ] );
    }
   }
  }

 /// the matching inside the blossom b0 swapped along the even path from v
 /// to its base, which v becomes
 void augmentBlossom( int b0 , int v0 ) {
  tasks.assign( 1 , { b0 , v0 } );
  while( ! tasks.empty() ) {
   const auto [ b , v ] = tasks.back();
   tasks.pop_back();
   int t = v;
   while( parent[ t ] != b )
    t = parent[ t ];
   if( t >= n )
    tasks.emplace_back( t , v );
   std::vector< int > & C = childs[ b ];
   std::vector< int > & P = endps[ b ];
   const std::size_t L = C.size();
   const int i = int( std::find( C.begin() , C.end() , t ) - C.begin() );
   int j = i , step , trick;
   if( i & 1 ) {
    j -= int( L );
    step = 1;
    trick = 0;
    }
   else {
    step = -1;
    trick = 1;
    }
   while( j != 0 ) {
    j += step;
    t = C[ at( j , L ) ];
    const int p = P[ at( j - trick , L ) ] ^ trick;
    if( t >= n )
     tasks.emplace_back( t , endpoint[ p ] );
    j += step;
    t = C[ at( j , L ) ];
    if( t >= n )
     tasks.emplace_back( t , endpoint[ p ^ 1 ] );
    mate[ endpoint[ p ] ] = p ^ 1;
    mate[ endpoint[ p ^ 1 ] ] = p;
    }
   std::rotate( C.begin() , C.begin() + i , C.end() );
   std::rotate( P.begin() , P.begin() + i , P.end() );
   base[ b ] = v;
   }
  }

 /// the matching augmented along the edge k between two S-nodes of two
 /// trees and their paths to the roots
 void augmentMatching( int k ) {
  for( auto [ s , p ] :
       { std::pair( std::get< 0 >( E[ k ] ) , int( 2 * k + 1 ) ) ,
	 std::pair( std::get< 1 >( E[ k ] ) , int( 2 * k ) ) } )
   for( ; ; ) {
    const int bs = inblossom[ s ];
    if( bs >= n )
     augmentBlossom( bs , s );
    mate[ s ] = p;
    if( labelend[ bs ] == -1 )  // the root
     break;
    const int t = endpoint[ labelend[ bs ] ];
    const int bt = inblossom[ t ];
    s = endpoint[ labelend[ bt ] ];
    const int j = endpoint[ labelend[ bt ] ^ 1 ];
    if( bt >= n )
     augmentBlossom( bt , j );
    mate[ j ] = labelend[ bt ];
    p = labelend[ bt ] ^ 1;
    }
  }

 /// the stages, until one does not augment
 void solve( void ) {
  for( int stage = 0 ; stage < n ; ++stage ) {
   std::fill( label.begin() , label.end() , 0 );
   std::fill( bestedge.begin() , bestedge.end() , -1 );
   for( int b = n ; b < 2 * n ; ++b ) {
    bestedges[ b ].clear();
    hasBest[ b ] = false;
    }
   std::fill( allowed.begin() , allowed.end() , false );
   queue.clear();
   for( int v = 0 ; v < n ; ++v )
    if( ( mate[ v ] == -1 ) && ( label[ inblossom[ v ] ] == 0 ) )
     assignLabel( v , 1 , -1 );
   bool augmented = false;
   for( ; ; ) {
    while( ! queue.empty() && ! augmented ) {
     const int v = queue.back();
     queue.pop_back();
     for( int p : adj[ v ] ) {
      const int k = p / 2 , w = endpoint[ p ];
      if( inblossom[ v ] == inblossom[ w ] )
       continue;
      W ks( 0 );
      if( ! allowed[ k ] ) {
       ks = slack( k );
       if( ks <= W( 0 ) )
	allowed[ k ] = true;
       }
      if( allowed[ k ] ) {
       if( label[ inblossom[ w ] ] == 0 )
	assignLabel( w , 2 , p ^ 1 );
       else if( label[ inblossom[ w ] ] == 1 ) {
	const int b0 = scanBlossom( v , w );
	if( b0 >= 0 )
	 addBlossom( b0 , k );
	else {
	 augmentMatching( k );
	 augmented = true;
	 break;
	 }
        }
       else if( label[ w ] == 0 ) {  // a node of a T-blossom, reached
	label[ w ] = 2;
	labelend[ w ] = p ^ 1;
        }
       }
      else if( label[ inblossom[ w ] ] == 1 ) {
       const int b = inblossom[ v ];
       if( ( bestedge[ b ] == -1 ) || ( ks < slack( bestedge[ b ] ) ) )
	bestedge[ b ] = k;
       }
      else if( label[ w ] == 0 ) {
       if( ( bestedge[ w ] == -1 ) || ( ks < slack( bestedge[ w ] ) ) )
	bestedge[ w ] = k;
       }
      }
     }
    if( augmented )
     break;
    // the change of the duals, by the least of the four bounds
    int type = -1 , edge = -1 , blossom = -1;
    W delta( 0 );
    if( ! maxCardinality ) {
     type = 1;
     delta = *std::min_element( dual.begin() , dual.begin() + n );
     }
    for( int v = 0 ; v < n ; ++v )
     if( ( label[ inblossom[ v ] ] == 0 ) && ( bestedge[ v ] != -1 ) ) {
      const W d = slack( bestedge[ v ] );
      if( ( type == -1 ) || ( d < delta ) ) {
       delta = d;
       type = 2;
       edge = bestedge[ v ];
       }
      }
    for( int b = 0 ; b < 2 * n ; ++b )
     if( ( parent[ b ] == -1 ) && ( label[ b ] == 1 ) &&
	 ( bestedge[ b ] != -1 ) ) {
      const W d = slack( bestedge[ b ] ) / 2;
      if( ( type == -1 ) || ( d < delta ) ) {
       delta = d;
       type = 3;
       edge = bestedge[ b ];
       }
      }
    for( int b = n ; b < 2 * n ; ++b )
     if( ( base[ b ] >= 0 ) && ( parent[ b ] == -1 ) && ( label[ b ] == 2 ) &&
	 ( ( type == -1 ) || ( dual[ b ] < delta ) ) ) {
      delta = dual[ b ];
      type = 4;
      blossom = b;
      }
    if( type == -1 ) {  // no bound: the optimum
     type = 1;
     delta =
       std::max( W( 0 ) ,
		 *std::min_element( dual.begin() , dual.begin() + n ) );
     }
    for( int v = 0 ; v < n ; ++v )
     if( label[ inblossom[ v ] ] == 1 )
      dual[ v ] -= delta;
     else if( label[ inblossom[ v ] ] == 2 )
      dual[ v ] += delta;
    for( int b = n ; b < 2 * n ; ++b )
     if( ( base[ b ] >= 0 ) && ( parent[ b ] == -1 ) ) {
      if( label[ b ] == 1 )
       dual[ b ] += delta;
      else if( label[ b ] == 2 )
       dual[ b ] -= delta;
      }
    if( type == 1 )
     break;
    if( type == 4 ) {
     expandBlossom( blossom , false );
     continue;
     }
    allowed[ edge ] = true;  // types 2 and 3: a tight edge
    int i = std::get< 0 >( E[ edge ] );
    if( label[ inblossom[ i ] ] == 0 )
     i = std::get< 1 >( E[ edge ] );
    queue.push_back( i );
    }
   if( ! augmented )
    break;
   for( int b = n ; b < 2 * n ; ++b )  // the S-blossoms of dual 0
    if( ( parent[ b ] == -1 ) && ( base[ b ] >= 0 ) && ( label[ b ] == 1 ) &&
	( dual[ b ] == W( 0 ) ) )
     expandBlossom( b , true );
   }
  }

 /// the conditions of optimality of the duals, for integer weights
 bool optimal( void ) const {
  W offset( 0 );
  if( n == 0 )
   return( true );
  if( maxCardinality )
   offset =
     std::max( W( 0 ) ,
	       -*std::min_element( dual.begin() , dual.begin() + n ) );
  for( int v = 0 ; v < n ; ++v )
   if( ( dual[ v ] + offset < W( 0 ) ) ||
       ( ( mate[ v ] == -1 ) && ( dual[ v ] + offset != W( 0 ) ) ) )
    return( false );
  for( int b = n ; b < 2 * n ; ++b )
   if( ( base[ b ] >= 0 ) && ( dual[ b ] < W( 0 ) ) )
    return( false );
  std::vector< int > I , J;
  for( std::size_t k = 0 ; k < E.size() ; ++k ) {
   const auto & [ u , v , w ] = E[ k ];
   W s = dual[ u ] + dual[ v ] - w - w;
   I.assign( 1 , u );
   J.assign( 1 , v );
   while( parent[ I.back() ] != -1 )
    I.push_back( parent[ I.back() ] );
   while( parent[ J.back() ] != -1 )
    J.push_back( parent[ J.back() ] );
   for( auto a = I.rbegin() , c = J.rbegin() ;
	( a != I.rend() ) && ( c != J.rend() ) && ( *a == *c ) ; ++a , ++c )
    s += dual[ *a ] + dual[ *a ];
   const bool iu = ( mate[ u ] != -1 ) && ( mate[ u ] / 2 == int( k ) );
   const bool iv = ( mate[ v ] != -1 ) && ( mate[ v ] / 2 == int( k ) );
   if( ( s < W( 0 ) ) || ( iu != iv ) || ( iu && ( s != W( 0 ) ) ) )
    return( false );
   }
  for( int b = n ; b < 2 * n ; ++b )  // full where the dual is positive
   if( ( base[ b ] >= 0 ) && ( dual[ b ] > W( 0 ) ) ) {
    if( endps[ b ].size() % 2 == 0 )
     return( false );
    for( std::size_t i = 1 ; i < endps[ b ].size() ; i += 2 ) {
     const int p = endps[ b ][ i ];
     if( ( mate[ endpoint[ p ] ] != ( p ^ 1 ) ) ||
	 ( mate[ endpoint[ p ^ 1 ] ] != p ) )
      return( false );
     }
    }
  return( true );
  }
 };

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- FUNCTIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< std::pair< int , int > >
bipartiteBMatching( const std::vector< std::vector< int > > & A , int nr ,
		    const std::vector< long long > & capL ,
		    const std::vector< long long > & capR )
{
 const int nl = int( A.size() );
 ADS_PRE( ( nr >= 0 ) && ( int( capL.size() ) == nl ) &&
	  ( int( capR.size() ) == nr ) );
 FlowNetwork N( nl + nr + 2 );
 const int s = nl + nr , t = s + 1;
 std::vector< std::tuple< int , int , int > > id;  // edge , u , v
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

#if defined( ADS_MATCHING_LEMON ) && ADS_MATCHING_LEMON

namespace detail
{

/// the mates of maxCardinalityMatching() by the MaxMatching of LEMON, on
/// a copy of the graph whose nodes are the names
template< Graph G >
std::vector< int > lemonCardinalityMatching( const G & g )
{
 const int n = g.bound();
 ::lemon::SmartGraph L;
 L.reserveNode( n );
 for( int u = 0 ; u < n ; ++u )
  L.addNode();
 for( int u = 0 ; u < n ; ++u )
  if( g.hasNode( u ) )
   g.forEachAdj( u , [ & ]( int v ) {
    if( u < v )
     L.addEdge( L.nodeFromId( u ) , L.nodeFromId( v ) );
    } );
 ::lemon::MaxMatching< ::lemon::SmartGraph > M( L );
 M.run();
 std::vector< int > mate( std::size_t( n ) , NIL );
 for( int u = 0 ; u < n ; ++u ) {
  const auto v = M.mate( L.nodeFromId( u ) );
  if( v != ::lemon::INVALID )
   mate[ std::size_t( u ) ] = L.id( v );
  }
 return( mate );
 }

/// the matching of maxWeightMatching() by the MaxWeightedMatching of LEMON,
/// on the edges of positive weight: its weight and its places in E
template< class W >
std::pair< W , std::vector< std::size_t > >
lemonWeightMatching( int n ,
		     const std::vector< std::tuple< int , int , W > > & E )
{
 ::lemon::SmartGraph L;
 L.reserveNode( n );
 for( int u = 0 ; u < n ; ++u )
  L.addNode();
 ::lemon::SmartGraph::EdgeMap< W > w( L );
 std::vector< std::size_t > id;  // the place of each edge of L
 for( std::size_t e = 0 ; e < E.size() ; ++e ) {
  const auto & [ u , v , c ] = E[ e ];
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) && ( u != v ) );
  if( W( 0 ) < c ) {  // the others never help
   w.set( L.addEdge( L.nodeFromId( u ) , L.nodeFromId( v ) ) , c );
   id.push_back( e );
   }
  }
 ::lemon::MaxWeightedMatching< ::lemon::SmartGraph ,
			       ::lemon::SmartGraph::EdgeMap< W > >
   M( L , w );
 M.run();
 std::vector< std::size_t > S;
 W total( 0 );
 for( ::lemon::SmartGraph::EdgeIt e( L ) ; e != ::lemon::INVALID ; ++e )
  if( M.matching( e ) ) {
   S.push_back( id[ std::size_t( L.id( e ) ) ] );
   total += w[ e ];
   }
 std::sort( S.begin() , S.end() );
 return( std::pair( total , S ) );
 }

}  // end( namespace detail )

#endif

template< Graph G >
std::vector< int > maxCardinalityMatching( const G & g )
{
 ADS_PRE( ! g.directed() );
 const int n = g.bound();
#if defined( ADS_MATCHING_LEMON ) && ADS_MATCHING_LEMON
 std::vector< int > mate = detail::lemonCardinalityMatching( g );
#else
 const detail::Rows< G > A( g );
 detail::CardinalityBlossom< G > S( A , n );
 for( int u = 0 ; u < n ; ++u )  // a greedy matching
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
 std::vector< int > mate = std::move( S.mate );
#endif
 ADS_AUDIT( [ & ]( void ) {
  for( int u = 0 ; u < n ; ++u ) {
   const int v = mate[ u ];
   if( v == NIL )
    continue;
   if( ! g.hasNode( u ) || ( v < 0 ) || ( v >= n ) || ( mate[ v ] != u ) )
    return( false );
   bool edge = false;
   g.forEachAdj( u , [ & ]( int x ) { edge = edge || ( x == v ); } );
   if( ! edge )
    return( false );
   }
  return( true );
  }() );
 return( mate );
 }

template< class W >
 requires std::is_arithmetic_v< W > && std::is_signed_v< W >
std::pair< W , std::vector< std::size_t > >
maxWeightMatching( int n ,
		   const std::vector< std::tuple< int , int , W > > & E ,
		   bool maxCardinality )
{
 ADS_PRE( n >= 0 );
 std::vector< std::size_t > M;
 W total( 0 );
#if defined( ADS_MATCHING_LEMON ) && ADS_MATCHING_LEMON
 if( ! maxCardinality )
  std::tie( total , M ) = detail::lemonWeightMatching( n , E );
 else
#endif
 {
  detail::WeightedBlossom< W > B( n , E , maxCardinality );
  B.solve();
  if constexpr( std::is_integral_v< W > )
   ADS_AUDIT( B.optimal() );
  for( int v = 0 ; v < n ; ++v )
   if( ( B.mate[ v ] != -1 ) && ( ( B.mate[ v ] & 1 ) == 1 ) ) {
    const int k = B.mate[ v ] / 2;  // v is the first end of edge k
    M.push_back( B.id[ k ] );
    total += std::get< 2 >( B.E[ k ] );
    }
  std::sort( M.begin() , M.end() );
  }
 ADS_AUDIT( [ & ]( void ) {
  std::vector< bool > end( n , false );
  for( std::size_t e : M ) {
   const int u = std::get< 0 >( E[ e ] );
   const int v = std::get< 1 >( E[ e ] );
   if( end[ u ] || end[ v ] )
    return( false );
   end[ u ] = end[ v ] = true;
   }
  return( true );
  }() );
 return( std::pair( total , M ) );
 }

template< class W >
std::pair< W , std::vector< std::size_t > >
branchAndBoundMatching( int n ,
			const std::vector< std::tuple< int , int , W > > & E )
{
 ADS_PRE( n >= 0 );
 detail::MatchingSearch< W > M{ n ,
				E ,
				std::vector< std::vector< std::size_t > >( n ) ,
				std::vector< W >( n + 1 , W( 0 ) ) ,
				std::vector< bool >( n , false ) ,
				{} ,
				{} ,
				W( 0 ) ,
				W( 0 ) };
 std::vector< W > top( n , W( 0 ) );  // the best weight at each node
 for( std::size_t e = 0 ; e < E.size() ; ++e ) {
  const auto & [ u , v , w ] = E[ e ];
  ADS_PRE( ( u >= 0 ) && ( u < n ) && ( v >= 0 ) && ( v < n ) && ( u != v ) );
  if( W( 0 ) < w ) {  // the others never help
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
