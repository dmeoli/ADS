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

 /// the end of the run from lo, extended to minrun items, made ascending
 std::size_t nextRun( std::size_t lo , std::size_t minrun ) {
  const std::size_t n = f_A.size();
  std::size_t hi = lo + 1;
  if( hi < n ) {
   if( f_less( f_A[ hi ] , f_A[ lo ] ) ) {  // strictly decreasing
    while( ( hi + 1 < n ) && f_less( f_A[ hi + 1 ] , f_A[ hi ] ) ) ++hi;
    std::reverse( f_A.begin() + lo , f_A.begin() + hi + 1 );
    }
   else                                     // non-decreasing
    while( ( hi + 1 < n ) && ! f_less( f_A[ hi + 1 ] , f_A[ hi ] ) ) ++hi;
   ++hi;
   }
  const std::size_t end = std::min( n , lo + std::max( minrun ,
                                                       hi - lo ) );
  for( std::size_t i = hi ; i < end ; ++i ) {   // binary insertion
   std::size_t a = lo , b = i;                  // after the equals
   while( a < b ) {
    const std::size_t m = a + ( b - a ) / 2;
    if( f_less( f_A[ i ] , f_A[ m ] ) ) b = m; else a = m + 1;
    }
   std::rotate( f_A.begin() + a , f_A.begin() + i , f_A.begin() + i + 1 );
   }
  return( end );
  }

 /// merges the adjacent runs A[ s1 .. e1 ) and A[ e1 .. e2 )
 void merge( std::size_t s1 , std::size_t e1 , std::size_t e2 ) {
  // the left items before A[ e1 ] , the right ones after A[ e1 - 1 ]
  s1 = gallopRight( f_A[ e1 ] , s1 , e1 );
  if( s1 == e1 )
   return;
  e2 = gallopLeft( f_A[ e1 - 1 ] , e1 , e2 );
  if( e2 == e1 )
   return;
  f_B.assign( std::make_move_iterator( f_A.begin() + s1 ) ,
              std::make_move_iterator( f_A.begin() + e1 ) );
  std::size_t i = 0 , j = e1 , k = s1;
  const std::size_t n1 = f_B.size();
  while( ( i < n1 ) && ( j < e2 ) ) {
   std::size_t winsL = 0 , winsR = 0;
   while( ( i < n1 ) && ( j < e2 ) && ( winsL < 7 ) && ( winsR < 7 ) )
    if( f_less( f_A[ j ] , f_B[ i ] ) ) {   // the right one, strictly
     f_A[ k++ ] = std::move( f_A[ j++ ] );
     ++winsR;
     winsL = 0;
     }
    else {
     f_A[ k++ ] = std::move( f_B[ i++ ] );
     ++winsL;
     winsR = 0;
     }
   if( ( i == n1 ) || ( j == e2 ) )
    break;
   if( winsL >= 7 ) {                       // the left items <= A[ j ]
    const std::size_t t = gallopRightIn( f_B , f_A[ j ] , i , n1 );
    while( i < t ) f_A[ k++ ] = std::move( f_B[ i++ ] );
    }
   else {                                   // the right items < B[ i ]
    const std::size_t t = gallopLeft( f_B[ i ] , j , e2 );
    while( j < t ) f_A[ k++ ] = std::move( f_A[ j++ ] );
    }
   }
  while( i < n1 ) f_A[ k++ ] = std::move( f_B[ i++ ] );
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

}  // end( namespace detail )

template< class T , class C >
void timSort( std::vector< T > & A , C less , std::size_t * runs )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size() , m = detail::minrun( n );
 detail::RunMerger< T , C > M( A , less );
 struct Run { std::size_t start , len; };
 std::vector< Run > S;
 std::size_t count = 0;
 auto mergeAt = [ & ]( std::size_t i ) {    // S[ i ] and S[ i + 1 ]
  M.merge( S[ i ].start , S[ i + 1 ].start ,
           S[ i + 1 ].start + S[ i + 1 ].len );
  S[ i ].len += S[ i + 1 ].len;
  S.erase( S.begin() + i + 1 );
  };
 for( std::size_t lo = 0 ; lo < n ; ++count ) {
  const std::size_t hi = M.nextRun( lo , m );
  S.push_back( { lo , hi - lo } );
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
 struct Run { std::size_t start , end; int power; };
 std::vector< Run > S;                      // the powers increase upwards
 std::size_t count = 0;
 if( n > 0 ) {
  std::size_t s = 0 , e = M.nextRun( 0 , 24 );   // the current run
  count = 1;
  while( e < n ) {
   const std::size_t e2 = M.nextRun( e , 24 );
   ++count;
   const int p = detail::nodePower( s , e , e2 , n );
   while( ! S.empty() && ( S.back().power > p ) ) {
    M.merge( S.back().start , s , e );      // the top merged with s .. e
    s = S.back().start;
    S.pop_back();
    }
   S.push_back( { s , e , p } );
   s = e;
   e = e2;
   }
  while( ! S.empty() ) {
   M.merge( S.back().start , s , e );
   s = S.back().start;
   S.pop_back();
   }
  }
 if( runs ) *runs = count;
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RunSorts.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
