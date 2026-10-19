/*--------------------------------------------------------------------------*/
/*--------------------------- File RunSorts.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of RunSorts.h, with their contracts.
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

/// the runs and their merges, shared by Timsort and Powersort
template< class T , class C >
class RunMerger {

 public:

 RunMerger( std::vector< T > & A , C & less ) : f_A( A ) , f_less( less ) {}

 /// the end of the run from lo, extended to minrun items, made ascending;
 /// extended() tells whether it had to be extended
 std::size_t nextRun( std::size_t lo , std::size_t minrun ) {
  const std::size_t n = f_A.size();
  T * const a = f_A.data();
  std::size_t hi = lo + 1;
  if( hi < n ) {
   if( f_less( a[ hi ] , a[ lo ] ) ) {      // strictly decreasing
    while( ( hi + 1 < n ) && f_less( a[ hi + 1 ] , a[ hi ] ) ) ++hi;
    std::reverse( a + lo , a + hi + 1 );
    }
   else                                     // non-decreasing
    while( ( hi + 1 < n ) && ! f_less( a[ hi + 1 ] , a[ hi ] ) ) ++hi;
   ++hi;
   }
  const std::size_t end = std::min( n , lo + std::max( minrun ,
                                                       hi - lo ) );
  f_extended = end > hi;
  for( std::size_t i = hi ; i < end ; ++i ) {   // binary insertion
   std::size_t l = lo , r = i;                  // after the equals
   while( l < r ) {
    const std::size_t m = l + ( r - l ) / 2;
    if( f_less( a[ i ] , a[ m ] ) ) r = m; else l = m + 1;
    }
   std::rotate( a + l , a + i , a + i + 1 );
   }
  return( end );
  }

 /// true if the last run of nextRun() was extended: the input is in
 /// disorder there
 bool extended( void ) const { return( f_extended ); }

 /// merges the adjacent runs A[ s1 .. e1 ) and A[ e1 .. e2 ); disorder
 /// tells that both come from runs extended by nextRun(), i.e., that the
 /// order of their items is hard to predict
 void merge( std::size_t s1 , std::size_t e1 , std::size_t e2 ,
             bool disorder = false ) {
  // the left items before A[ e1 ] , the right ones after A[ e1 - 1 ]
  s1 = gallopRight( f_A[ e1 ] , s1 , e1 );
  if( s1 == e1 )
   return;
  e2 = gallopLeft( f_A[ e1 - 1 ] , e1 , e2 );
  if( e2 == e1 )
   return;
  f_B.assign( std::make_move_iterator( f_A.begin() + s1 ) ,
              std::make_move_iterator( f_A.begin() + e1 ) );
  T * const a = f_A.data();
  T * const b = f_B.data();
  std::size_t i = 0 , j = e1 , k = s1;
  const std::size_t n1 = f_B.size();
  if constexpr( std::is_arithmetic_v< T > )
   if( disorder ) {                         // no branch on the order
    while( ( i < n1 ) && ( j < e2 ) ) {
     const bool right = f_less( a[ j ] , b[ i ] );
     a[ k++ ] = right ? a[ j ] : b[ i ];
     j += right;
     i += ! right;
     }
    while( i < n1 ) a[ k++ ] = b[ i++ ];
    return;
    }
  for( bool done = false ; ! done ; ) {
   std::size_t winsL = 0 , winsR = 0;       // one by one, while no run
   do                                       // wins f_gallop times in a row
    if( f_less( a[ j ] , b[ i ] ) ) {       // the right one, strictly
     a[ k++ ] = std::move( a[ j++ ] );
     done = j == e2;
     ++winsR;
     winsL = 0;
     }
    else {
     a[ k++ ] = std::move( b[ i++ ] );
     done = i == n1;
     ++winsL;
     winsR = 0;
     }
   while( ! done && ( ( winsL | winsR ) < f_gallop ) );
   while( ! done ) {                        // by galloping, while it pays
    const std::size_t tL = gallopRightIn( f_B , a[ j ] , i , n1 );
    winsL = tL - i;                         // the left items <= A[ j ]
    while( i < tL ) a[ k++ ] = std::move( b[ i++ ] );
    if( ( done = i == n1 ) )
     break;
    a[ k++ ] = std::move( a[ j++ ] );
    if( ( done = j == e2 ) )
     break;
    const std::size_t tR = gallopLeft( b[ i ] , j , e2 );
    winsR = tR - j;                         // the right items < B[ i ]
    while( j < tR ) a[ k++ ] = std::move( a[ j++ ] );
    if( ( done = j == e2 ) )
     break;
    a[ k++ ] = std::move( b[ i++ ] );
    if( ( done = i == n1 ) )
     break;
    if( ( winsL < 7 ) && ( winsR < 7 ) ) {
     f_gallop += 2;                         // it does not pay: one by one
     break;
     }
    if( f_gallop > 1 )
     --f_gallop;                            // it pays: sooner next time
    }
   }
  while( i < n1 ) a[ k++ ] = std::move( b[ i++ ] );
  }

 private:

 /// the first place in [ lo , hi ) of V whose item is larger than x,
 /// by galloping from lo
 template< class V >
 std::size_t gallopRightIn( const V & v , const T & x , std::size_t lo ,
                            std::size_t hi ) {
  std::size_t a = lo , step = 1 , b = lo;   // v[ a .. b ) <= x
  while( ( b < hi ) && ! f_less( x , v[ b ] ) ) {
   a = b + 1;
   b = lo + step;
   step *= 2;
   }
  b = std::min( b , hi );
  while( a < b ) {
   const std::size_t m = a + ( b - a ) / 2;
   if( f_less( x , v[ m ] ) ) b = m; else a = m + 1;
   }
  return( a );
  }

 std::size_t gallopRight( const T & x , std::size_t lo , std::size_t hi ) {
  return( gallopRightIn( f_A , x , lo , hi ) );
  }

 /// the first place in [ lo , hi ) of A whose item is not smaller than x,
 /// by galloping from hi backwards
 std::size_t gallopLeft( const T & x , std::size_t lo , std::size_t hi ) {
  std::size_t a = lo , b = hi;              // A[ b .. hi ) >= x
  for( std::size_t off = 1 ; off <= hi - lo ; off *= 2 ) {
   const std::size_t t = hi - off;          // probes hi - 1 , hi - 2 ,
   if( f_less( f_A[ t ] , x ) ) {           // hi - 4 , ...
    a = t + 1;
    break;
    }
   b = t;
   }
  while( a < b ) {
   const std::size_t m = a + ( b - a ) / 2;
   if( f_less( f_A[ m ] , x ) ) a = m + 1; else b = m;
   }
  return( a );
  }

 std::vector< T > & f_A;   ///< the vector
 C & f_less;               ///< the order
 std::vector< T > f_B;     ///< the buffer of a left run
 std::size_t f_gallop = 7; ///< the wins in a row that start galloping
 bool f_extended = false;  ///< the last run was extended

 };  // end( class RunMerger )

/// the minrun of Timsort: n / 2^k in [ 32 , 64 ], rounded up if any bit
/// was shifted out
inline std::size_t minrun( std::size_t n )
{
 std::size_t r = 0;
 while( n >= 64 ) {
  r |= n & 1;
  n >>= 1;
  }
 return( n + r );
 }

/// the power of the boundary between A[ s1 .. s2 ) and A[ s2 .. e2 ) in a
/// vector of n items: the first bit where the midpoints, as fractions of n,
/// differ
inline int nodePower( std::size_t s1 , std::size_t s2 , std::size_t e2 ,
                      std::size_t n )
{
 std::size_t l = s1 + s2 , r = s2 + e2;     // twice the midpoints
 int k = 0;
 for( bool a = l >= n , b = r >= n ; a == b ; a = l >= n , b = r >= n ) {
  ++k;
  if( a ) {
   l -= n;
   r -= n;
   }
  l <<= 1;
  r <<= 1;
  }
 return( k + 1 );
 }

/// the merges of Powersort over the runs of the n items from 0 on, where
/// next( lo ) makes the run from lo ascending and gives its end and whether
/// it was extended; the number of runs
template< class T , class C , class N >
std::size_t powerMerges( RunMerger< T , C > & M , std::size_t n , N next )
{
 if( n == 0 )
  return( 0 );
 struct Run { std::size_t start , end; int power; bool disorder; };
 std::vector< Run > S;                      // the powers increase upwards
 const auto r0 = next( std::size_t( 0 ) );
 std::size_t s = 0 , e = r0.first;          // the current run s .. e
 bool d = r0.second;                        // and its disorder
 std::size_t count = 1;
 auto mergeTop = [ & ]( void ) {           // the top merged with s .. e
  d = d && S.back().disorder;
  M.merge( S.back().start , s , e , d );
  s = S.back().start;
  S.pop_back();
  };
 while( e < n ) {
  const auto [ e2 , d2 ] = next( e );        // the run after
  ++count;
  const int p = nodePower( s , e , e2 , n );
  while( ! S.empty() && ( S.back().power > p ) )
   mergeTop();
  S.push_back( { s , e , p , d } );
  s = e;
  e = e2;
  d = d2;
  }
 while( ! S.empty() )
  mergeTop();
 return( count );
 }

}  // end( namespace detail )

template< class T , class C >
void timSort( std::vector< T > & A , C less , std::size_t * runs )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size() , m = detail::minrun( n );
 detail::RunMerger< T , C > M( A , less );
 struct Run { std::size_t start , len; bool disorder; };
 std::vector< Run > S;
 std::size_t count = 0;
 auto mergeAt = [ & ]( std::size_t i ) {    // S[ i ] and S[ i + 1 ]
  S[ i ].disorder = S[ i ].disorder && S[ i + 1 ].disorder;
  M.merge( S[ i ].start , S[ i + 1 ].start ,
           S[ i + 1 ].start + S[ i + 1 ].len , S[ i ].disorder );
  S[ i ].len += S[ i + 1 ].len;
  S.erase( S.begin() + i + 1 );
  };
 for( std::size_t lo = 0 ; lo < n ; ++count ) {
  const std::size_t hi = M.nextRun( lo , m );
  S.push_back( { lo , hi - lo , M.extended() } );
  lo = hi;
  while( S.size() > 1 ) {                   // the invariants of the stack
   std::size_t i = S.size() - 2;
   if( ( ( i > 0 ) && ( S[ i - 1 ].len <= S[ i ].len + S[ i + 1 ].len ) ) ||
       ( ( i > 1 ) && ( S[ i - 2 ].len <= S[ i - 1 ].len + S[ i ].len ) ) ) {
    if( S[ i - 1 ].len < S[ i + 1 ].len ) --i;
    mergeAt( i );
    }
   else if( S[ i ].len <= S[ i + 1 ].len )
    mergeAt( i );
   else
    break;
   }
  }
 while( S.size() > 1 )
  mergeAt( S.size() - 2 );
 if( runs ) *runs = count;
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void powerSort( std::vector< T > & A , C less , std::size_t * runs )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 detail::RunMerger< T , C > M( A , less );
 const std::size_t count = detail::powerMerges( M , n ,
                                                [ & ]( std::size_t lo ) {
  const std::size_t e = M.nextRun( lo , 24 );
  return( std::pair( e , M.extended() ) ); } );
 if( runs ) *runs = count;
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RunSorts.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
