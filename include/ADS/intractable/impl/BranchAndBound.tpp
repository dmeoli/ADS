/*--------------------------------------------------------------------------*/
/*------------------------ File BranchAndBound.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BranchAndBound [see BranchAndBound.h], with their
 * contracts.
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ BOUNDS ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
T spanningTreeWeight( const std::vector< std::vector< T > > & D ,
                      const Nodes & V )
{
 ADS_PRE( detail::symmetricMatrix( D ) && detail::places( V , D.size() ) );
 std::vector< WeightedEdge< T > > E;
 for( std::size_t a = 0 ; a < V.size() ; ++a )
  for( std::size_t b = a + 1 ; b < V.size() ; ++b )
   E.push_back( { int( a ) , int( b ) , D[ V[ a ] ][ V[ b ] ] } );
 T w = T( 0 );
 for( const auto & e : kruskal( int( V.size() ) , E ) ) w += e.w;
 return( w );
 }

template< class T >
T oneTree( const std::vector< std::vector< T > > & D , int r )
{
 ADS_PRE( detail::symmetricMatrix( D ) );
 ADS_PRE( ( r >= 0 ) && ( r < int( D.size() ) ) );
 Nodes V;
 std::vector< T > e;
 for( int h = 0 ; h < int( D.size() ) ; ++h )
  if( h != r ) {
   V.push_back( h );
   e.push_back( D[ r ][ h ] );
   }
 std::sort( e.begin() , e.end() );
 T w = spanningTreeWeight( D , V );
 for( std::size_t k = 0 ; k < std::min< std::size_t >( 2 , e.size() ) ; ++k )
  w += e[ k ];
 return( w );
 }

template< class T >
T tspLowerBound( const std::vector< std::vector< T > > & D , const Nodes & S ,
                 TspBound how )
{
 ADS_PRE( detail::square( D ) && ! S.empty() );
 ADS_PRE( detail::places( S , D.size() ) );
 ADS_PRE( ( how == TspBound::BOOK ) || detail::symmetricMatrix( D ) );
 const int n = int( D.size() ) , i = int( S.size() ) - 1;
 T C = T( 0 );
 for( int k = 0 ; k < i ; ++k ) C += D[ S[ k ] ][ S[ k + 1 ] ];
 if( i == n - 1 )
  return( C + D[ S[ i ] ][ S[ 0 ] ] );
 std::vector< bool > inS( n , false );
 for( int s : S ) inS[ s ] = true;
 const T INF = std::numeric_limits< T >::max();
 T A = INF , B = INF;
 Nodes R;
 for( int h = 0 ; h < n ; ++h )
  if( ! inS[ h ] ) {
   R.push_back( h );
   A = std::min( A , D[ h ][ S[ 0 ] ] );   // the return to S[ 0 ]
   B = std::min( B , D[ S[ i ] ][ h ] );
   }
 if( how == TspBound::ONE_TREE )
  return( C + A + B + spanningTreeWeight( D , R ) );
 T sum = A + B;
 for( int h : R ) {                     // D[ h ]: the least p -> h -> q
  T best = INF;
  for( int p = 0 ; p < n ; ++p )
   for( int q = 0 ; q < n ; ++q )
    if( ( p != h ) && ( q != h ) && ( ( p != q ) || ( n == 2 ) ) )
     best = std::min( best , D[ p ][ h ] + D[ h ][ q ] );
  sum += best;
  }
 if constexpr( std::integral< T > )
  return( C + ( sum + 1 ) / 2 );
 else
  return( C + sum / 2 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- BBTSP ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
std::pair< T , Nodes > bbTsp( const std::vector< std::vector< T > > & D ,
                              const Nodes & start , TspBound how ,
                              long * generated )
{
 ADS_PRE( detail::square( D ) );
 ADS_PRE( start.empty() || detail::permutation( start , D.size() ) );
 ADS_PRE( ( how == TspBound::BOOK ) || detail::symmetricMatrix( D ) );
 const auto result = [ & ]( void ) -> std::pair< T , Nodes > {
  const int n = int( D.size() );
  T minCost = start.empty() ? std::numeric_limits< T >::max()
                            : tourCost( D , start );
  Nodes minSol = start;
  if( generated ) *generated = 1;
  if( n <= 1 )
   return( std::pair( T( 0 ) , Nodes( n , 0 ) ) );
  Nodes S( 1 , 0 );
  std::vector< bool > inS( n , false );
  inS[ 0 ] = true;
  auto rec = [ & ]( auto & self , int i ) -> void {
   for( int c = 1 ; c < n ; ++c ) {      // R in increasing order
    if( inS[ c ] ) continue;
    S.push_back( c );
    inS[ c ] = true;
    const T lb = tspLowerBound( D , S , how );
    if( generated ) ++*generated;
    if( lb < minCost ) {
     if( i < n - 1 )
      self( self , i + 1 );
     else {
      minSol = S;
      minCost = lb;
      }
     }
    S.pop_back();
    inS[ c ] = false;
    }
   };
  rec( rec , 1 );
  return( std::pair( minCost , minSol ) );
  }();
 ADS_POST( ( D.size() < 2 ) ||
           detail::permutation( result.second , D.size() ) );
 ADS_POST( ( D.size() < 2 ) ||
           ( tourCost( D , result.second ) == result.first ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ LITTLE ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
std::pair< T , Nodes > littleTsp( const std::vector< std::vector< T > > & D ,
                                  long * generated )
{
 ADS_PRE( detail::square( D ) );
 const auto result = [ & ]( void ) -> std::pair< T , Nodes > {
  const int n = int( D.size() );
  const T INF = std::numeric_limits< T >::max();
  if( generated ) *generated = 1;
  if( n <= 1 )
   return( std::pair( T( 0 ) , Nodes( n , 0 ) ) );

  struct Node {
   std::vector< std::vector< T > > M;     // the reduced costs, INF if out
   std::vector< bool > row , col;         // done
   std::vector< int > succ , pred;        // the edges included
   int edges = 0;
   T lb = T( 0 );
   };
  // the reduction: false if a row or a column has no edge left
  auto reduce = [ & ]( Node & x ) {
   for( int i = 0 ; i < n ; ++i ) {
    if( x.row[ i ] ) continue;
    T m = INF;
    for( int j = 0 ; j < n ; ++j )
     if( ! x.col[ j ] ) m = std::min( m , x.M[ i ][ j ] );
    if( m == INF ) return( false );
    for( int j = 0 ; j < n ; ++j )
     if( ! x.col[ j ] && ( x.M[ i ][ j ] != INF ) ) x.M[ i ][ j ] -= m;
    x.lb += m;
    }
   for( int j = 0 ; j < n ; ++j ) {
    if( x.col[ j ] ) continue;
    T m = INF;
    for( int i = 0 ; i < n ; ++i )
     if( ! x.row[ i ] ) m = std::min( m , x.M[ i ][ j ] );
    if( m == INF ) return( false );
    for( int i = 0 ; i < n ; ++i )
     if( ! x.row[ i ] && ( x.M[ i ][ j ] != INF ) ) x.M[ i ][ j ] -= m;
    x.lb += m;
    }
   return( true );
   };

  std::vector< Node > nodes;
  FibonacciHeapPriorityQueue< int , T > Q;
  {
   Node r;
   r.M = D;
   for( int i = 0 ; i < n ; ++i ) r.M[ i ][ i ] = INF;
   r.row.assign( n , false );
   r.col.assign( n , false );
   r.succ.assign( n , -1 );
   r.pred.assign( n , -1 );
   if( reduce( r ) ) {
    nodes.push_back( std::move( r ) );
    Q.insert( 0 , nodes[ 0 ].lb );
    }
   }
  while( ! Q.empty() ) {
   const int k = Q.deleteMin();
   if( nodes[ k ].edges == n ) {          // a circuit, the best
    Nodes S( 1 , 0 );
    while( int( S.size() ) < n ) S.push_back( nodes[ k ].succ[ S.back() ] );
    return( std::pair( tourCost( D , S ) , S ) );
    }
   // the entry 0 whose exclusion raises the bound the most
   const Node & x = nodes[ k ];
   int bi = -1 , bj = -1;
   T best = T( -1 );
   for( int i = 0 ; i < n ; ++i )
    for( int j = 0 ; j < n ; ++j ) {
     if( x.row[ i ] || x.col[ j ] || ( x.M[ i ][ j ] != T( 0 ) ) ) continue;
     T a = INF , b = INF;
     for( int h = 0 ; h < n ; ++h ) {
      if( ( h != j ) && ! x.col[ h ] ) a = std::min( a , x.M[ i ][ h ] );
      if( ( h != i ) && ! x.row[ h ] ) b = std::min( b , x.M[ h ][ j ] );
      }
     const T p = ( ( a == INF ) || ( b == INF ) ) ? INF : a + b;
     if( ( bi < 0 ) || ( p > best ) ) {
      best = p;
      bi = i;
      bj = j;
      }
     }
   if( bi < 0 ) continue;
   Node in = x , out = x;
   // the child with ( bi , bj )
   in.row[ bi ] = in.col[ bj ] = true;
   in.succ[ bi ] = bj;
   in.pred[ bj ] = bi;
   ++in.edges;
   if( in.edges < n - 1 ) {               // no circuit before the end
    int s = bi , e = bj;
    while( in.pred[ s ] >= 0 ) s = in.pred[ s ];
    while( in.succ[ e ] >= 0 ) e = in.succ[ e ];
    in.M[ e ][ s ] = INF;
    }
   // the child without it
   out.M[ bi ][ bj ] = INF;
   for( Node * c : { & in , & out } ) {
    if( generated ) ++*generated;
    if( ( c->edges < n ) && ! reduce( *c ) ) continue;
    nodes.push_back( std::move( *c ) );
    Q.insert( int( nodes.size() ) - 1 , nodes.back().lb );
    }
   }
  return( std::pair( INF , Nodes() ) );
  }();
 ADS_POST( ( D.size() < 2 ) ||
           detail::permutation( result.second , D.size() ) );
 ADS_POST( ( D.size() < 2 ) ||
           ( tourCost( D , result.second ) == result.first ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- CLIQUE AND KNAPSACK ------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::pair< long , Nodes >
maxWeightClique( const Adjacency & G , const std::vector< long > & w ,
                 long * generated )
{
 ADS_PRE( detail::symmetric( G ) && ( w.size() == G.size() ) );
 ADS_PRE( detail::nonnegative( w ) );
 const auto result = [ & ]( void ) -> std::pair< long , Nodes > {
  const int n = int( G.size() );
  long best = 0;
  Nodes bestSet , S;
  if( generated ) *generated = 1;
  auto rec = [ & ]( auto & self , const Nodes & cand , long weight ) -> void {
   if( weight > best ) {
    best = weight;
    bestSet = S;
    }
   long rest = 0;
   for( int v : cand ) rest += w[ v ];
   for( std::size_t a = 0 ; a < cand.size() ; ++a ) {
    if( weight + rest <= best ) return;   // the bound
    const int v = cand[ a ];
    rest -= w[ v ];
    Nodes next;
    for( std::size_t b = a + 1 ; b < cand.size() ; ++b )
     if( G[ v ][ cand[ b ] ] ) next.push_back( cand[ b ] );
    S.push_back( v );
    if( generated ) ++*generated;
    self( self , next , weight + w[ v ] );
    S.pop_back();
    }
   };
  Nodes all( n );
  std::iota( all.begin() , all.end() , 0 );
  rec( rec , all , 0 );
  std::sort( bestSet.begin() , bestSet.end() );
  return( std::pair( best , bestSet ) );
  }();
 ADS_POST( cliqueCertificate( G , result.second ,
                              int( result.second.size() ) ) );
 ADS_POST( detail::sumOver( w , result.second ) == result.first );
 return( result );
 }

inline std::pair< long , Nodes > knapsackBB( const std::vector< long > & p ,
                                             const std::vector< long > & v ,
                                             long C , long * generated )
{
 ADS_PRE( ( p.size() == v.size() ) && ( C >= 0 ) );
 ADS_PRE( detail::nonnegative( p ) &&
          std::all_of( v.begin() , v.end() , []( long x ) {
  return( x > 0 ); } ) );
 const auto result = [ & ]( void ) -> std::pair< long , Nodes > {
  const int n = int( p.size() );
  Nodes o( n );
  std::iota( o.begin() , o.end() , 0 );
  std::stable_sort( o.begin() , o.end() , [ & ]( int a , int b ) {
   return( p[ a ] * v[ b ] > p[ b ] * v[ a ] ); } );
  long best = 0;
  Nodes bestSet , S;
  if( generated ) *generated = 1;
  // the fractional knapsack of the objects o[ i .. ] within c
  auto bound = [ & ]( int i , long c ) {
   double b = 0;
   for( ; ( i < n ) && ( c > 0 ) ; ++i )
    if( v[ o[ i ] ] <= c ) {
     b += double( p[ o[ i ] ] );
     c -= v[ o[ i ] ];
     }
    else {
     b += double( p[ o[ i ] ] ) * double( c ) / double( v[ o[ i ] ] );
     c = 0;
     }
   return( b );
   };
  auto rec = [ & ]( auto & self , int i , long c , long profit ) -> void {
   if( profit > best ) {
    best = profit;
    bestSet = S;
    }
   if( ( i == n ) || ( double( profit ) + bound( i , c ) <= double( best ) ) )
    return;
   if( v[ o[ i ] ] <= c ) {
    S.push_back( o[ i ] );
    if( generated ) ++*generated;
    self( self , i + 1 , c - v[ o[ i ] ] , profit + p[ o[ i ] ] );
    S.pop_back();
    }
   if( generated ) ++*generated;
   self( self , i + 1 , c , profit );
   };
  rec( rec , 0 , C , 0 );
  std::sort( bestSet.begin() , bestSet.end() );
  return( std::pair( best , bestSet ) );
  }();
 ADS_POST( detail::places( result.second , p.size() ) );
 ADS_POST( detail::sumOver( p , result.second ) == result.first );
 ADS_POST( detail::sumOver( v , result.second ) <= C );
 return( result );
 }

inline std::pair< long , Nodes > knapsackBacktrack(
                                  const std::vector< long > & p ,
                                  const std::vector< long > & v , long C ,
                                  long * generated )
{
 ADS_PRE( ( p.size() == v.size() ) && ( C >= 0 ) );
 ADS_PRE( detail::nonnegative( p ) &&
          std::all_of( v.begin() , v.end() , []( long x ) {
  return( x > 0 ); } ) );
 const int n = int( p.size() );
 std::vector< long > rest( n + 1 , 0 );     // the profits from i on
 for( int i = n - 1 ; i >= 0 ; --i )
  rest[ i ] = rest[ i + 1 ] + p[ i ];
 long best = 0;
 Nodes bestSet , S;
 if( generated ) *generated = 1;
 auto rec = [ & ]( auto & self , int i , long c , long profit ) -> void {
  if( profit > best ) {
   best = profit;
   bestSet = S;
   }
  if( ( i == n ) || ( profit + rest[ i ] <= best ) )
   return;                                  // the simple bound
  if( v[ i ] <= c ) {
   S.push_back( i );
   if( generated ) ++*generated;
   self( self , i + 1 , c - v[ i ] , profit + p[ i ] );
   S.pop_back();
   }
  if( generated ) ++*generated;
  self( self , i + 1 , c , profit );
  };
 rec( rec , 0 , C , 0 );
 ADS_POST( [ & ]( void ) {
  long used = 0;
  for( int i : bestSet ) used += v[ i ];
  return( used <= C );
  }() );
 return( std::pair( best , bestSet ) );
 }

inline std::pair< long , Nodes > knapsackGreedy(
                                  const std::vector< long > & p ,
                                  const std::vector< long > & v , long C )
{
 ADS_PRE( ( p.size() == v.size() ) && ( C >= 0 ) );
 ADS_PRE( detail::nonnegative( p ) &&
          std::all_of( v.begin() , v.end() , []( long x ) {
  return( x > 0 ); } ) );
 const int n = int( p.size() );
 Nodes o( n );
 std::iota( o.begin() , o.end() , 0 );
 std::stable_sort( o.begin() , o.end() , [ & ]( int a , int b ) {
  return( p[ a ] * v[ b ] > p[ b ] * v[ a ] ); } );
 long profit = 0;
 Nodes S;
 for( int i : o )
  if( v[ i ] <= C ) {
   C -= v[ i ];
   profit += p[ i ];
   S.push_back( i );
   }
 std::sort( S.begin() , S.end() );
 return( std::pair( profit , S ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File BranchAndBound.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
