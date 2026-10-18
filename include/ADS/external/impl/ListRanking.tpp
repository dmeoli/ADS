/*--------------------------------------------------------------------------*/
/*------------------------- File ListRanking.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ListRanking.h, with their contracts.
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

namespace detail {

/// the predecessor of each item, n for the head
inline std::vector< std::size_t > predecessors(
                                    const std::vector< std::size_t > & succ )
{
 const std::size_t n = succ.size();
 std::vector< std::size_t > pred( n , n );
 for( std::size_t i = 0 ; i < n ; ++i )
  if( succ[ i ] != i ) pred[ succ[ i ] ] = i;
 return( pred );
 }

/// no item of I is the tail or the successor of another item of I
inline bool independent( const std::vector< std::size_t > & succ ,
                         const std::vector< bool > & I )
{
 for( std::size_t i = 0 ; i < succ.size() ; ++i )
  if( I[ i ] && ( ( succ[ i ] == i ) || I[ succ[ i ] ] ) ) return( false );
 return( true );
 }

/// adjacent items have different colors
inline bool properColors( const std::vector< std::size_t > & succ ,
                          const std::vector< int > & color )
{
 for( std::size_t i = 0 ; i < succ.size() ; ++i )
  if( ( succ[ i ] != i ) && ( color[ i ] == color[ succ[ i ] ] ) )
   return( false );
 return( true );
 }

}  // end( namespace detail )

inline bool isList( const std::vector< std::size_t > & succ )
{
 const std::size_t n = succ.size();
 if( n == 0 ) return( true );
 std::vector< std::size_t > in( n , 0 );
 std::size_t tails = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  if( succ[ i ] >= n ) return( false );
  if( succ[ i ] == i ) ++tails;
  else ++in[ succ[ i ] ];
  }
 if( tails != 1 ) return( false );
 const auto head = std::find( in.begin() , in.end() , std::size_t( 0 ) );
 if( head == in.end() ) return( false );
 std::size_t x = std::size_t( head - in.begin() ) , seen = 1;
 for( ; ( succ[ x ] != x ) && ( seen <= n ) ; x = succ[ x ] ) ++seen;
 return( seen == n );
 }

inline std::vector< std::size_t > listRank(
                                     const std::vector< std::size_t > & succ )
{
 ADS_AUDIT( isList( succ ) );
 const std::size_t n = succ.size();
 std::vector< std::size_t > rank( n , 0 );
 if( n == 0 ) return( rank );
 const auto pred = detail::predecessors( succ );
 std::size_t x = 0;
 while( succ[ x ] != x ) ++x;               // the tail
 for( ; pred[ x ] != n ; x = pred[ x ] )
  rank[ pred[ x ] ] = rank[ x ] + 1;
 return( rank );
 }

inline std::vector< std::size_t > listRankPointerJumping(
                                    const std::vector< std::size_t > & succ ,
                                    std::size_t * rounds )
{
 ADS_AUDIT( isList( succ ) );
 const std::size_t n = succ.size();
 std::vector< std::size_t > s( succ ) , r( n );
 for( std::size_t i = 0 ; i < n ; ++i ) r[ i ] = s[ i ] == i ? 0 : 1;
 std::size_t k = 0;
 for( ; ; ++k ) {
  bool jumped = false;
  std::vector< std::size_t > s2( s ) , r2( r );   // a synchronous round
  for( std::size_t i = 0 ; i < n ; ++i )
   if( s[ i ] != s[ s[ i ] ] ) {
    r2[ i ] = r[ i ] + r[ s[ i ] ];
    s2[ i ] = s[ s[ i ] ];
    jumped = true;
    }
  if( ! jumped ) break;
  s.swap( s2 );
  r.swap( r2 );
  }
 if( rounds ) *rounds = k;
 return( r );
 }

template< class R >
std::vector< bool > randomIndependentSet(
                           const std::vector< std::size_t > & succ , R & g )
{
 ADS_AUDIT( isList( succ ) );
 const std::size_t n = succ.size();
 std::vector< bool > coin( n ) , I( n );
 for( std::size_t i = 0 ; i < n ; ++i ) coin[ i ] = ( g() & 1 ) != 0;
 for( std::size_t i = 0 ; i < n ; ++i )     // heads before tails
  I[ i ] = ( succ[ i ] != i ) && coin[ i ] && ! coin[ succ[ i ] ];
 ADS_AUDIT( detail::independent( succ , I ) );
 return( I );
 }

inline std::vector< int > deterministicCoinTossing(
                                    const std::vector< std::size_t > & succ ,
                                    std::size_t * rounds )
{
 ADS_AUDIT( isList( succ ) );
 const std::size_t n = succ.size();
 std::vector< std::uint64_t > c( n );
 for( std::size_t i = 0 ; i < n ; ++i ) c[ i ] = i;   // distinct colors
 std::size_t k = 0;
 auto most = [ & ]( void ) {
  return( n ? *std::max_element( c.begin() , c.end() ) : 0 ); };
 for( ; most() >= 6 ; ++k ) {
  std::vector< std::uint64_t > c2( n );
  for( std::size_t i = 0 ; i < n ; ++i ) {
   const std::uint64_t d = succ[ i ] == i ? 1 : c[ i ] ^ c[ succ[ i ] ];
   const int p = std::countr_zero( d );    // the lowest differing bit
   c2[ i ] = 2 * std::uint64_t( p ) + ( ( c[ i ] >> p ) & 1 );
   }
  c.swap( c2 );
  }
 if( rounds ) *rounds = k;
 const auto pred = detail::predecessors( succ );
 std::vector< int > color( c.begin() , c.end() );
 for( int v = 5 ; v >= 3 ; --v )            // from six colors to three
  for( std::size_t i = 0 ; i < n ; ++i )
   if( color[ i ] == v ) {
    int x = 0;
    while( ( ( succ[ i ] != i ) && ( color[ succ[ i ] ] == x ) ) ||
           ( ( pred[ i ] != n ) && ( color[ pred[ i ] ] == x ) ) ) ++x;
    color[ i ] = x;
    }
 ADS_AUDIT( detail::properColors( succ , color ) );
 ADS_POST( std::all_of( color.begin() , color.end() , []( int x ) {
  return( ( x >= 0 ) && ( x <= 2 ) ); } ) );
 return( color );
 }

inline std::vector< bool > independentSetOfColors(
                                    const std::vector< std::size_t > & succ ,
                                    const std::vector< int > & color )
{
 ADS_AUDIT( detail::properColors( succ , color ) );
 const std::size_t n = succ.size();
 const auto pred = detail::predecessors( succ );
 std::vector< bool > I( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t s = succ[ i ];
  I[ i ] = ( s != i ) &&                    // the tail is not a neighbour
           ( ( pred[ i ] == n ) || ( color[ i ] < color[ pred[ i ] ] ) ) &&
           ( ( succ[ s ] == s ) || ( color[ i ] < color[ s ] ) );
  }
 ADS_AUDIT( detail::independent( succ , I ) );
 return( I );
 }

template< class R >
std::vector< std::size_t > listRankIndependentSet(
                                    const std::vector< std::size_t > & succ ,
                                    R & g , bool deterministic ,
                                    std::size_t * levels )
{
 ADS_AUDIT( isList( succ ) );
 const std::size_t n = succ.size();
 std::vector< std::size_t > s( succ ) , r( n ) , alive( n ) , pos( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  r[ i ] = s[ i ] == i ? 0 : 1;
  alive[ i ] = i;
  }
 std::vector< std::vector< std::size_t > > removed;   // I, level by level
 while( alive.size() > 1 ) {
  const std::size_t m = alive.size();
  for( std::size_t k = 0 ; k < m ; ++k ) pos[ alive[ k ] ] = k;
  std::vector< std::size_t > ls( m );      // the shorter list, compacted
  for( std::size_t k = 0 ; k < m ; ++k ) ls[ k ] = pos[ s[ alive[ k ] ] ];
  std::vector< bool > in;
  if( deterministic )
   in = independentSetOfColors( ls , deterministicCoinTossing( ls ) );
  else
   do in = randomIndependentSet( ls , g );
   while( std::size_t( 8 * std::count( in.begin() , in.end() , true ) ) <
          std::max< std::size_t >( m , 8 ) );
  std::vector< std::size_t > I , next;
  for( std::size_t k = 0 ; k < m ; ++k ) {
   const std::size_t x = alive[ k ];
   if( in[ k ] )
    I.push_back( x );
   else {
    if( in[ ls[ k ] ] ) {                   // x jumps over its successor
     const std::size_t y = s[ x ];
     r[ x ] += r[ y ];
     s[ x ] = s[ y ];
     }
    next.push_back( x );
    }
   }
  removed.push_back( std::move( I ) );
  alive.swap( next );
  }
 if( levels ) *levels = removed.size();
 for( std::size_t l = removed.size() ; l-- > 0 ; )   // back in, in reverse
  for( const std::size_t x : removed[ l ] ) r[ x ] += r[ s[ x ] ];
 ADS_AUDIT( r == listRank( succ ) );
 return( r );
 }

template< class T , class Op >
void parallelStep( const DiskFile< T > & A ,
                   const DiskFile< std::pair< std::size_t , std::size_t > > &
                                                                         ops ,
                   Op op , DiskFile< T > & out )
{
 using Pair = std::pair< std::size_t , std::size_t >;
 TwoLevelMemory & mem = A.memory();
 ADS_PRE( ( &ops.memory() == &mem ) && ( &out.memory() == &mem ) &&
          out.isEmpty() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  std::vector< bool > seen( A.size() , false );
  for( const Pair & p : ops.contents() ) {
   if( ( p.first >= A.size() ) || ( p.second >= A.size() ) ||
       seen[ p.first ] ) return( false );
   seen[ p.first ] = true;
   }
  return( true ); } ) );
 struct Triple {
  std::size_t a;                            // the destination
  std::size_t b;                            // the source
  T v;                                      // A[ b ]
  };
 DiskFile< Triple > P( mem ) , Q( mem );
 {                                          // 1. scan: the triples
  DiskReader< Pair > rd( ops );
  DiskWriter< Triple > w( P );
  while( rd.hasNext() ) {
   const Pair p = rd.next();
   w.push( Triple{ p.first , p.second , T() } );
   }
  }
 multiwayMergeSort( P , Q , []( const Triple & x , const Triple & y ) {
  return( x.b < y.b ); } );                 // 2. sort by the source
 P.clear();
 {                                          // 3. scan: A[ b ] filled in
  DiskReader< Triple > rq( Q );
  DiskReader< T > ra( A );
  DiskWriter< Triple > w( P );
  while( rq.hasNext() ) {
   Triple t = rq.next();
   t.v = ra.at( t.b );
   w.push( t );
   }
  }
 Q.clear();
 multiwayMergeSort( P , Q , []( const Triple & x , const Triple & y ) {
  return( x.a < y.a ); } );                 // 4. sort by the destination
 P.clear();
 {                                          // 5. scan: op applied
  DiskReader< Triple > rq( Q );
  DiskReader< T > ra( A );
  DiskWriter< T > w( out );
  for( std::size_t i = 0 ; ra.hasNext() ; ++i ) {
   T x = ra.next();
   if( rq.hasNext() && ( rq.peek().a == i ) ) x = op( x , rq.next().v );
   w.push( x );
   }
  }
 }

inline void externalPointerJumping( const DiskFile< std::size_t > & succ ,
                                    DiskFile< std::size_t > & rank ,
                                    std::size_t * rounds )
{
 using Pair = std::pair< std::size_t , std::size_t >;
 struct Node {
  std::size_t succ = 0;
  std::size_t rank = 0;
  };
 TwoLevelMemory & mem = succ.memory();
 ADS_PRE( mem.memorySize() >= 3 * mem.blockSize() );
 ADS_PRE( ( &rank.memory() == &mem ) && rank.isEmpty() );
 ADS_AUDIT( isList( succ.contents() ) );
 DiskFile< Node > N( mem );
 std::size_t tail = 0;
 {
  DiskReader< std::size_t > rd( succ );
  DiskWriter< Node > w( N );
  for( std::size_t i = 0 ; rd.hasNext() ; ++i ) {
   const std::size_t s = rd.next();
   if( s == i ) tail = i;
   w.push( Node{ s , s == i ? std::size_t( 0 ) : std::size_t( 1 ) } );
   }
  }
 std::size_t k = 0;
 for( ; ; ++k ) {
  DiskFile< Pair > ops( mem );              // the items not yet at the tail
  {
   DiskReader< Node > rd( N );
   DiskWriter< Pair > w( ops );
   for( std::size_t i = 0 ; rd.hasNext() ; ++i ) {
    const Node x = rd.next();
    if( x.succ != tail ) w.push( Pair( i , x.succ ) );
    }
   }
  if( ops.isEmpty() ) break;
  DiskFile< Node > N2( mem );
  parallelStep( N , ops , []( const Node & x , const Node & y ) {
   return( Node{ y.succ , x.rank + y.rank } ); } , N2 );
  N = std::move( N2 );
  }
 if( rounds ) *rounds = k;
 {
  DiskReader< Node > rd( N );
  DiskWriter< std::size_t > w( rank );
  while( rd.hasNext() ) w.push( rd.next().rank );
  }
 ADS_AUDIT( rank.contents() == listRank( succ.contents() ) );
 }

namespace detail {

/// an item of a list on the disk, the lists sorted by id
struct ListRecord {
 std::size_t id = 0;
 std::size_t succ = 0;
 std::size_t rank = 0;                      ///< items up to succ
 bool coin = false;
 bool in = false;                           ///< in the independent set
 };

using ListJoin = std::pair< ListRecord , ListRecord >;

/// for each record x of from, by id, the pair of x and the record of
/// x.succ in to: two sorts and two scans
inline DiskFile< ListJoin > pull( const DiskFile< ListRecord > & from ,
                                  const DiskFile< ListRecord > & to )
{
 TwoLevelMemory & mem = from.memory();
 DiskFile< ListJoin > P( mem ) , Q( mem );
 {
  DiskReader< ListRecord > rd( from );
  DiskWriter< ListJoin > w( P );
  while( rd.hasNext() ) w.push( ListJoin( rd.next() , ListRecord() ) );
  }
 multiwayMergeSort( P , Q , []( const ListJoin & a , const ListJoin & b ) {
  return( a.first.succ < b.first.succ ); } );
 P.clear();
 {                                          // a coordinated scan of to
  DiskReader< ListJoin > rq( Q );
  DiskReader< ListRecord > rt( to );
  DiskWriter< ListJoin > w( P );
  while( rq.hasNext() ) {
   ListJoin j = rq.next();
   while( rt.peek().id < j.first.succ ) rt.next();
   j.second = rt.peek();
   w.push( j );
   }
  }
 Q.clear();
 multiwayMergeSort( P , Q , []( const ListJoin & a , const ListJoin & b ) {
  return( a.first.id < b.first.id ); } );
 return( Q );
 }

/// the records of L with their final ranks, by id
template< class R >
DiskFile< ListRecord > rankRecords( const DiskFile< ListRecord > & L ,
                                    R & g , std::size_t level ,
                                    std::size_t * levels )
{
 TwoLevelMemory & mem = L.memory();
 const std::size_t B = mem.blockSize() , n = L.size();
 if( levels ) *levels = std::max( *levels , level );
 DiskFile< ListRecord > out( mem );
 if( n <= ( mem.memorySize() - B ) / 4 ) {  // ranked in memory
  MemoryLease lease( mem , 4 * n );         // records, ids, links, order
  std::vector< ListRecord > A( n );
  for( std::size_t b = 0 , len = 0 ; b < L.blocks() ; ++b )
   len += L.readBlock( b , A.data() + len );
  std::vector< std::size_t > ids( n ) , sp( n ) , order;
  for( std::size_t k = 0 ; k < n ; ++k ) ids[ k ] = A[ k ].id;
  std::vector< bool > isSucc( n , false );
  for( std::size_t k = 0 ; k < n ; ++k ) {
   sp[ k ] = lowerBound( ids , A[ k ].succ );
   if( sp[ k ] != k ) isSucc[ sp[ k ] ] = true;
   }
  std::size_t x = 0;
  while( ( x < n ) && isSucc[ x ] ) ++x;   // the head
  if( n ) {
   for( ; sp[ x ] != x ; x = sp[ x ] ) order.push_back( x );
   for( std::size_t k = order.size() ; k-- > 0 ; )
    A[ order[ k ] ].rank += A[ sp[ order[ k ] ] ].rank;
   }
  DiskWriter< ListRecord > w( out );
  for( const ListRecord & a : A ) w.push( a );
  return( out );
  }
 for( ; ; ) {
  DiskFile< ListRecord > L0( mem ) , L1( mem );
  {                                         // the coins
   DiskReader< ListRecord > rd( L );
   DiskWriter< ListRecord > w( L0 );
   while( rd.hasNext() ) {
    ListRecord x = rd.next();
    x.coin = ( g() & 1 ) != 0;
    w.push( x );
    }
   }
  std::size_t count = 0;
  {                                         // heads before tails
   const DiskFile< ListJoin > J = pull( L0 , L0 );
   DiskReader< ListJoin > rd( J );
   DiskWriter< ListRecord > w( L1 );
   while( rd.hasNext() ) {
    const ListJoin j = rd.next();
    ListRecord x = j.first;
    x.in = x.coin && ! j.second.coin && ( x.id != x.succ );
    count += x.in ? 1 : 0;
    w.push( x );
    }
   }
  if( 8 * count < n ) continue;             // too few: new coins
  L0.clear();
  DiskFile< ListRecord > Ls( mem ) , I( mem );
  {                                         // the jumps over I
   const DiskFile< ListJoin > J = pull( L1 , L1 );
   DiskReader< ListJoin > rd( J );
   DiskWriter< ListRecord > ws( Ls ) , wi( I );
   while( rd.hasNext() ) {
    const ListJoin j = rd.next();
    ListRecord x = j.first;
    if( x.in )
     wi.push( x );
    else {
     if( j.second.in ) {
      x.rank += j.second.rank;
      x.succ = j.second.succ;
      }
     ws.push( x );
     }
    }
   }
  L1.clear();
  const DiskFile< ListRecord > Rs = rankRecords( Ls , g , level + 1 ,
                                                 levels );
  Ls.clear();
  {                                         // I back in, merged by id
   const DiskFile< ListJoin > J = pull( I , Rs );
   DiskReader< ListJoin > rj( J );
   DiskReader< ListRecord > rr( Rs );
   DiskWriter< ListRecord > w( out );
   while( rj.hasNext() || rr.hasNext() )
    if( rj.hasNext() &&
        ( ! rr.hasNext() || ( rj.peek().first.id < rr.peek().id ) ) ) {
     const ListJoin j = rj.next();
     ListRecord x = j.first;
     x.rank += j.second.rank;
     w.push( x );
     }
    else
     w.push( rr.next() );
   }
  return( out );
  }
 }

}  // end( namespace detail )

template< class R >
void externalListRank( const DiskFile< std::size_t > & succ ,
                       DiskFile< std::size_t > & rank , R & g ,
                       std::size_t * levels )
{
 using detail::ListRecord;
 TwoLevelMemory & mem = succ.memory();
 const std::size_t B = mem.blockSize() , M = mem.memorySize();
 ADS_PRE( ( M >= 3 * B ) && ( M >= B + 4 ) );
 ADS_PRE( ( &rank.memory() == &mem ) && rank.isEmpty() );
 ADS_AUDIT( isList( succ.contents() ) );
 DiskFile< ListRecord > L( mem );
 {
  DiskReader< std::size_t > rd( succ );
  DiskWriter< ListRecord > w( L );
  for( std::size_t i = 0 ; rd.hasNext() ; ++i ) {
   const std::size_t s = rd.next();
   w.push( ListRecord{ i , s , s == i ? std::size_t( 0 ) : std::size_t( 1 ) ,
                       false , false } );
   }
  }
 if( levels ) *levels = 0;
 const DiskFile< ListRecord > Rs = detail::rankRecords( L , g , 0 ,
                                                        levels );
 L.clear();
 {
  DiskReader< ListRecord > rd( Rs );
  DiskWriter< std::size_t > w( rank );
  while( rd.hasNext() ) w.push( rd.next().rank );
  }
 ADS_AUDIT( rank.contents() == listRank( succ.contents() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ListRanking.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
