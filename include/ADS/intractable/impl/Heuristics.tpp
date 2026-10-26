/*--------------------------------------------------------------------------*/
/*-------------------------- File Heuristics.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Heuristics [see Heuristics.h], with their contracts.
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
/*---------------------------- THE CONTRACTS -------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

/// side has n sides, each 0 or 1
inline bool sides( const std::vector< int > & side , std::size_t n )
{
 return( ( side.size() == n ) &&
         std::all_of( side.begin() , side.end() , []( int s ) {
          return( ( s == 0 ) || ( s == 1 ) ); } ) );
 }

/// the sum of the side 0 minus that of the side 1
inline long difference( const std::vector< long > & A ,
                        const std::vector< int > & side )
{
 long d = 0;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  d += side[ i ] ? -A[ i ] : A[ i ];
 return( d );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------ GREEDY TSP --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
std::pair< std::vector< std::pair< int , int > > , Nodes >
greedyTsp( const std::vector< std::vector< T > > & D )
{
 ADS_PRE( detail::symmetricMatrix( D ) && ( D.size() >= 3 ) );
 using Result = std::pair< std::vector< std::pair< int , int > > , Nodes >;
 const auto result = [ & ]( void ) -> Result {
  const int n = int( D.size() );
  std::vector< WeightedEdge< T > > E;
  for( int u = 0 ; u < n ; ++u )
   for( int v = u + 1 ; v < n ; ++v ) E.push_back( { u , v , D[ u ][ v ] } );
  std::stable_sort( E.begin() , E.end() , []( const auto & a ,
                    const auto & b ) { return( a.w < b.w ); } );
  std::vector< std::pair< int , int > > S;
  std::vector< int > in( n , 0 );
  std::vector< Nodes > adj( n );
  MFSet M( n );
  for( const auto & e : E ) {
   if( int( S.size() ) == n - 1 ) break;
   if( ( in[ e.u ] < 2 ) && ( in[ e.v ] < 2 ) &&
       ( M.find( e.u ) != M.find( e.v ) ) ) {
    S.emplace_back( e.u , e.v );
    ++in[ e.u ];
    ++in[ e.v ];
    adj[ e.u ].push_back( e.v );
    adj[ e.v ].push_back( e.u );
    M.merge( e.u , e.v );
    }
   }
  int u = 0;
  while( in[ u ] != 1 ) ++u;
  int v = u + 1;
  while( in[ v ] != 1 ) ++v;
  S.emplace_back( u , v );
  adj[ u ].push_back( v );
  adj[ v ].push_back( u );
  Nodes C( 1 , 0 );                       // the circuit, along adj
  for( int prev = -1 ; int( C.size() ) < n ; ) {
   const int x = C.back();
   const int y = ( adj[ x ][ 0 ] != prev ) ? adj[ x ][ 0 ] : adj[ x ][ 1 ];
   prev = x;
   C.push_back( y );
   }
  return( std::pair( S , C ) );
  }();
 ADS_POST( result.first.size() == D.size() );
 ADS_POST( detail::permutation( result.second , D.size() ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- LOCAL SEARCH --------------------------------*/
/*--------------------------------------------------------------------------*/

inline Nodes twoOptMove( Nodes pi , int i , int j )
{
 ADS_PRE( ( 0 <= i ) && ( i < j ) && ( j < int( pi.size() ) ) );
 const auto result = [ & ]( void ) -> Nodes {
  std::reverse( pi.begin() + i + 1 , pi.begin() + j + 1 );
  return( pi );
  }();
 ADS_POST( result.size() == pi.size() );
 return( result );
 }

template< class T >
Nodes twoOpt( const std::vector< std::vector< T > > & D , Nodes pi ,
              std::vector< T > * trace )
{
 ADS_PRE( detail::symmetricMatrix( D ) &&
          detail::permutation( pi , D.size() ) );
 const auto result = [ & ]( void ) -> Nodes {
  const int n = int( pi.size() );
  T c = tourCost( D , pi );
  if( trace ) trace->push_back( c );
  for( ; ; ) {
   T best = c;
   int bi = -1 , bj = -1;
   for( int i = 0 ; i < n ; ++i )
    for( int j = i + 2 ; j < n ; ++j ) {
     if( ( i == 0 ) && ( j == n - 1 ) ) continue;      // consecutive
     const int a = pi[ i ] , b = pi[ i + 1 ] , x = pi[ j ] ,
               y = pi[ ( j + 1 ) % n ];
     const T d = c - D[ a ][ b ] - D[ x ][ y ] + D[ a ][ x ] + D[ b ][ y ];
     if( d < best ) {
      best = d;
      bi = i;
      bj = j;
      }
     }
   if( bi < 0 ) return( pi );            // a local optimum
   pi = twoOptMove( pi , bi , bj );
   c = best;
   if( trace ) trace->push_back( c );
   }
  }();
 ADS_POST( detail::permutation( result , D.size() ) );
 return( result );
 }

inline long kOptNeighbours( int n , int k )
{
 ADS_PRE( ( n >= 3 ) && ( k >= 0 ) && ( k <= n ) );
 Nodes pi( n );
 std::iota( pi.begin() , pi.end() , 0 );
 std::set< std::pair< int , int > > E;
 for( int i = 0 ; i < n ; ++i ) {
  const int a = pi[ i ] , b = pi[ ( i + 1 ) % n ];
  E.insert( { std::min( a , b ) , std::max( a , b ) } );
  }
 long count = 0;
 Nodes s( pi.begin() + 1 , pi.end() );
 do {                                    // each circuit twice, by direction
  int shared = 0;
  for( int i = 0 ; i < n ; ++i ) {
   const int a = i == 0 ? 0 : s[ i - 1 ] , b = i == n - 1 ? 0 : s[ i ];
   shared += int( E.count( { std::min( a , b ) , std::max( a , b ) } ) );
   }
  if( shared == n - k ) ++count;
  } while( std::next_permutation( s.begin() , s.end() ) );
 return( count / 2 );
 }

inline std::vector< int > partitionLocalSearch( const std::vector< long > & A ,
                                                std::vector< int > side )
{
 ADS_PRE( detail::sides( side , A.size() ) );
 [[maybe_unused]] const long d0 = detail::difference( A , side );
 const auto result = [ & ]( void ) -> std::vector< int > {
  const int n = int( A.size() );
  long diff = 0;                          // sum of side 0 - sum of side 1
  for( int i = 0 ; i < n ; ++i ) diff += side[ i ] ? -A[ i ] : A[ i ];
  for( ; ; ) {
   long best = std::labs( diff );
   int bi = -1 , bj = -1;
   for( int i = 0 ; i < n ; ++i ) {
    const long di = side[ i ] ? A[ i ] : -A[ i ];   // the change of moving i
    if( std::labs( diff + 2 * di ) < best ) {
     best = std::labs( diff + 2 * di );
     bi = i;
     bj = -1;
     }
    for( int j = i + 1 ; j < n ; ++j ) {
     if( side[ i ] == side[ j ] ) continue;
     const long dj = side[ j ] ? A[ j ] : -A[ j ];
     if( std::labs( diff + 2 * ( di + dj ) ) < best ) {
      best = std::labs( diff + 2 * ( di + dj ) );
      bi = i;
      bj = j;
      }
     }
    }
   if( bi < 0 ) return( side );
   for( int x : { bi , bj } )
    if( x >= 0 ) {
     diff += 2 * ( side[ x ] ? A[ x ] : -A[ x ] );
     side[ x ] = 1 - side[ x ];
     }
   }
  }();
 ADS_POST( detail::sides( result , A.size() ) );
 ADS_POST( std::labs( detail::difference( A , result ) ) <= std::labs( d0 ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BISECTION -----------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*------------------------------- BUCKETS ----------------------------------*/
/*--------------------------------------------------------------------------*/

inline Buckets::Buckets( int n ) : f_n( n )
{
 ADS_PRE( n >= 0 );
 for( int k = 0 ; k < 2 ; ++k ) {
  f_bucket[ k ].assign( n + 1 , Nodes() );
  f_h[ k ].assign( n , 0 );
  f_place[ k ].assign( n , -1 );
  f_min[ k ] = n + 1;
  f_max[ k ] = -1;
  }
 }

inline void Buckets::insert( int u )
{
 ADS_PRE( ( u >= 0 ) && ( u < f_n ) && ! contains( u ) );
 for( int k = 0 ; k < 2 ; ++k ) {
  f_h[ k ][ u ] = 0;
  put( k , u );
  }
 ADS_INVARIANT( consistent() );
 }

inline void Buckets::remove( int u )
{
 ADS_PRE( ( u >= 0 ) && ( u < f_n ) && contains( u ) );
 for( int k = 0 ; k < 2 ; ++k ) take( k , u );
 ADS_INVARIANT( consistent() );
 }

inline void Buckets::inc( int k , int u )
{
 ADS_PRE( ( ( k == 0 ) || ( k == 1 ) ) && ( u >= 0 ) && ( u < f_n ) );
 ADS_PRE( contains( u ) && ( f_h[ k ][ u ] < f_n ) );
 take( k , u );
 ++f_h[ k ][ u ];
 put( k , u );
 ADS_INVARIANT( consistent() );
 }

inline bool Buckets::contains( int u ) const
{
 ADS_PRE( ( u >= 0 ) && ( u < f_n ) );
 return( f_place[ 0 ][ u ] >= 0 );
 }

inline int Buckets::count( int k , int u ) const
{
 ADS_PRE( ( ( k == 0 ) || ( k == 1 ) ) && ( u >= 0 ) && ( u < f_n ) );
 return( f_h[ k ][ u ] );
 }

inline const Nodes & Buckets::bucket( int k , int h ) const
{
 ADS_PRE( ( ( k == 0 ) || ( k == 1 ) ) && ( h >= 0 ) && ( h <= f_n ) );
 return( f_bucket[ k ][ h ] );
 }

inline int Buckets::dim( int k , int h ) const
{
 ADS_PRE( ( ( k == 0 ) || ( k == 1 ) ) && ( h >= 0 ) && ( h <= f_n ) );
 return( int( f_bucket[ k ][ h ].size() ) );
 }

inline int Buckets::min( int k ) const
{
 ADS_PRE( ( k == 0 ) || ( k == 1 ) );
 return( f_min[ k ] );
 }

inline int Buckets::max( int k ) const
{
 ADS_PRE( ( k == 0 ) || ( k == 1 ) );
 return( f_max[ k ] );
 }

inline int Buckets::choose( int k ) const
{
 ADS_PRE( ( ( k == 0 ) || ( k == 1 ) ) && ( f_min[ 1 - k ] <= f_n ) );
 int u = -1;
 for( int v : f_bucket[ 1 - k ][ f_min[ 1 - k ] ] )
  if( ( u < 0 ) || ( f_h[ k ][ v ] > f_h[ k ][ u ] ) ) u = v;
 ADS_POST( ( u >= 0 ) && contains( u ) );
 return( u );
 }

inline void Buckets::put( int k , int u )
{
 const int h = f_h[ k ][ u ];
 f_place[ k ][ u ] = int( f_bucket[ k ][ h ].size() );
 f_bucket[ k ][ h ].push_back( u );
 f_min[ k ] = std::min( f_min[ k ] , h );
 f_max[ k ] = std::max( f_max[ k ] , h );
 }

inline void Buckets::take( int k , int u )
{
 const int h = f_h[ k ][ u ];
 Nodes & B = f_bucket[ k ][ h ];
 const int p = f_place[ k ][ u ];
 B[ p ] = B.back();
 f_place[ k ][ B[ p ] ] = p;
 B.pop_back();
 f_place[ k ][ u ] = -1;
 if( B.empty() ) {                      // the bounds move, amortized
  if( f_min[ k ] == h )
   while( ( f_min[ k ] <= f_n ) && f_bucket[ k ][ f_min[ k ] ].empty() )
    ++f_min[ k ];
  if( f_max[ k ] == h )
   while( ( f_max[ k ] >= 0 ) && f_bucket[ k ][ f_max[ k ] ].empty() )
    --f_max[ k ];
  }
 }

inline bool Buckets::consistent( void ) const
{
 for( int k = 0 ; k < 2 ; ++k ) {
  int lo = f_n + 1 , hi = -1;
  for( int h = 0 ; h <= f_n ; ++h ) {
   const Nodes & B = f_bucket[ k ][ h ];
   if( ! B.empty() ) {
    lo = std::min( lo , h );
    hi = std::max( hi , h );
    }
   for( int p = 0 ; p < int( B.size() ) ; ++p )
    if( ( f_h[ k ][ B[ p ] ] != h ) || ( f_place[ k ][ B[ p ] ] != p ) )
     return( false );
   }
  if( ( lo != f_min[ k ] ) || ( hi != f_max[ k ] ) ) return( false );
  }
 return( true );
 }

inline std::vector< int > minMaxGreedy( const Adjacency & G , int a , int b )
{
 ADS_PRE( detail::symmetric( G ) && ( G.size() % 2 == 0 ) );
 ADS_PRE( ( a >= 0 ) && ( a < int( G.size() ) ) && ( b >= 0 ) );
 ADS_PRE( ( b < int( G.size() ) ) && ( a != b ) );
 const auto result = [ & ]( void ) -> std::vector< int > {
  const int n = int( G.size() );
  Buckets B( n );
  for( int u = 0 ; u < n ; ++u ) B.insert( u );
  std::vector< int > side( n , -1 );
  auto add = [ & ]( int u , int k ) {
   side[ u ] = k;
   B.remove( u );
   for( int v = 0 ; v < n ; ++v )
    if( G[ u ][ v ] && B.contains( v ) ) B.inc( k , v );
   };
  add( a , 0 );
  add( b , 1 );
  for( int k = 0 , left = n - 2 ; left > 0 ; k = 1 - k , --left )
   add( B.choose( k ) , k );
  return( side );
  }();
 ADS_POST( detail::sides( result , G.size() ) );
 ADS_POST( 2 * std::count( result.begin() , result.end() , 0 ) ==
           long( G.size() ) );
 ADS_POST( ( result[ a ] == 0 ) && ( result[ b ] == 1 ) );
 return( result );
 }

inline int cutSize( const Adjacency & G , const std::vector< int > & side )
{
 ADS_PRE( detail::square( G ) && ( side.size() == G.size() ) );
 int c = 0;
 for( std::size_t u = 0 ; u < G.size() ; ++u )
  for( std::size_t v = u + 1 ; v < G.size() ; ++v )
   c += G[ u ][ v ] && ( side[ u ] != side[ v ] );
 return( c );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- STRINGS ------------------------------------*/
/*--------------------------------------------------------------------------*/

inline int overlap( const std::string & a , const std::string & b )
{
 for( std::size_t l = std::min( a.size() , b.size() ) ; l > 0 ; --l )
  if( a.compare( a.size() - l , l , b , 0 , l ) == 0 )
   return( int( l ) );
 return( 0 );
 }

inline std::string greedySuperstring( const std::vector< std::string > & P )
{
 const auto result = [ & ]( void ) -> std::string {
  std::vector< std::string > Q;
  for( std::size_t i = 0 ; i < P.size() ; ++i ) {
   bool inside = false;
   for( std::size_t j = 0 ; j < P.size() && ! inside ; ++j )
    inside = ( j != i ) && ( P[ j ].find( P[ i ] ) != std::string::npos ) &&
             ( ( P[ j ] != P[ i ] ) || ( j < i ) );
   if( ! inside ) Q.push_back( P[ i ] );
   }
  const int n = int( Q.size() );
  std::vector< WeightedEdge< int > > E;
  for( int i = 0 ; i < n ; ++i )
   for( int j = 0 ; j < n ; ++j )
    if( i != j ) E.push_back( { i , j , - overlap( Q[ i ] , Q[ j ] ) } );
  std::stable_sort( E.begin() , E.end() , []( const auto & a ,
                    const auto & b ) { return( a.w < b.w ); } );
  std::vector< int > in( n , 0 ) , out( n , 0 ) , next( n , -1 );
  MFSet M( std::max( n , 1 ) );
  for( const auto & e : E )
   if( ! out[ e.u ] && ! in[ e.v ] && ( M.find( e.u ) != M.find( e.v ) ) ) {
    out[ e.u ] = in[ e.v ] = 1;
    next[ e.u ] = e.v;
    M.merge( e.u , e.v );
    }
  std::string T;
  for( int s = 0 ; s < n ; ++s )
   if( ! in[ s ] )                       // the start of a path
    for( int u = s , prev = -1 ; u >= 0 ; prev = u , u = next[ u ] )
     T += prev < 0 ? Q[ u ] : Q[ u ].substr( overlap( Q[ prev ] , Q[ u ] ) );
  return( T );
  }();
 ADS_POST( std::all_of( P.begin() , P.end() , [ & ]( const std::string & p ) {
  return( result.find( p ) != std::string::npos ); } ) );
 return( result );
 }

inline std::pair< int , std::vector< std::pair< std::string , std::string > > >
starAlignment( const std::vector< std::string > & P )
{
 ADS_PRE( ! P.empty() );
 using Alignments = std::vector< std::pair< std::string , std::string > >;
 using Result = std::pair< int , Alignments >;
 const auto result = [ & ]( void ) -> Result {
  const int k = int( P.size() );
  int h = 0;
  long best = 0;
  for( int i = 0 ; i < k ; ++i ) {
   long s = 0;
   for( int j = 0 ; j < k ; ++j )
    if( j != i ) s += alignmentScore( P[ i ] , P[ j ] , Alignment::GLOBAL );
   if( ( i == 0 ) || ( s > best ) ) {
    best = s;
    h = i;
    }
   }
  std::vector< std::pair< std::string , std::string > > A;
  for( int j = 0 ; j < k ; ++j )
   A.push_back( globalAlignment( P[ h ] , P[ j ] , '-' ) );
  return( std::pair( h , A ) );
  }();
 ADS_POST( ( result.first >= 0 ) && ( result.first < int( P.size() ) ) );
 ADS_POST( result.second.size() == P.size() );
 return( result );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- COLORING -----------------------------------*/
/*--------------------------------------------------------------------------*/

inline std::vector< int > dsatur( const Adjacency & G )
{
 ADS_PRE( detail::symmetric( G ) );
 const auto result = [ & ]( void ) -> std::vector< int > {
  const int n = int( G.size() );
  std::vector< int > col( n , -1 );
  for( int step = 0 ; step < n ; ++step ) {
   int u = -1 , su = -1 , du = -1;
   for( int v = 0 ; v < n ; ++v ) {
    if( col[ v ] >= 0 ) continue;
    std::vector< bool > seen( n , false );
    int s = 0 , d = 0;
    for( int w = 0 ; w < n ; ++w )
     if( G[ v ][ w ] && ( col[ w ] >= 0 ) ) {
      ++d;
      if( ! seen[ col[ w ] ] ) {
       seen[ col[ w ] ] = true;
       ++s;
       }
      }
    if( ( s > su ) || ( ( s == su ) && ( d > du ) ) ) {
     u = v;
     su = s;
     du = d;
     }
    }
   std::vector< bool > used( n + 1 , false );
   for( int w = 0 ; w < n ; ++w )
    if( G[ u ][ w ] && ( col[ w ] >= 0 ) ) used[ col[ w ] ] = true;
   int c = 0;
   while( used[ c ] ) ++c;
   col[ u ] = c;
   }
  return( col );
  }();
 ADS_POST( detail::proper( G , result , std::max( int( G.size() ) , 1 ) ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Heuristics.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
