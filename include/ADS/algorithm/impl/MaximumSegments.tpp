/*--------------------------------------------------------------------------*/
/*------------------------ File MaximumSegments.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of MaximumSegments.h, with their contracts.
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

/// the sum of the segment is that of its items in D
template< class T >
bool segmentSum( const std::vector< T > & D , const Segment< T > & S )
{
 if( S.first + S.length > D.size() ) return( false );
 T s{};
 for( std::size_t k = S.first ; k < S.first + S.length ; ++k ) s += D[ k ];
 return( s == S.sum );
 }

/// P[ i ] = D[ 0 ] + ... + D[ i - 1 ], P[ 0 ] = 0
template< class T >
std::vector< T > prefixSums( const std::vector< T > & D )
{
 std::vector< T > P( D.size() + 1 , T{} );
 for( std::size_t i = 0 ; i < D.size() ; ++i ) P[ i + 1 ] = P[ i ] + D[ i ];
 return( P );
 }

}  // end( namespace detail )

template< class T >
Segment< T > maxSumSegmentCubic( const std::vector< T > & D )
{
 ADS_PRE( ! D.empty() );
 Segment< T > best{ D[ 0 ] , 0 , 1 };
 for( std::size_t b = 0 ; b < D.size() ; ++b )
  for( std::size_t s = b ; s < D.size() ; ++s ) {
   T tmp{};
   for( std::size_t i = b ; i <= s ; ++i ) tmp += D[ i ];
   if( best.sum < tmp ) best = Segment< T >{ tmp , b , s - b + 1 };
   }
 return( best );
 }

template< class T >
Segment< T > maxSumSegmentQuadratic( const std::vector< T > & D )
{
 ADS_PRE( ! D.empty() );
 Segment< T > best{ D[ 0 ] , 0 , 1 };
 for( std::size_t b = 0 ; b < D.size() ; ++b ) {
  T tmp{};
  for( std::size_t s = b ; s < D.size() ; ++s ) {
   tmp += D[ s ];                           // D[ b .. s ] from D[ b .. s )
   if( best.sum < tmp ) best = Segment< T >{ tmp , b , s - b + 1 };
   }
  }
 return( best );
 }

template< class T >
Segment< T > maxSumSegment( const std::vector< T > & D )
{
 ADS_PRE( ! D.empty() );
 Segment< T > best{ D[ 0 ] , 0 , 1 };
 T tmp{};
 for( std::size_t b = 0 , s = 0 ; s < D.size() ; ++s ) {
  tmp += D[ s ];
  if( best.sum < tmp ) best = Segment< T >{ tmp , b , s - b + 1 };
  if( tmp < T{} ) {                         // Property 1: a new start
   tmp = T{};
   b = s + 1;
   }
  }
 ADS_AUDIT( detail::segmentSum( D , best ) );
 ADS_AUDIT( ( best.sum < T{} ) || ( best.sum == maxSubarraySum( D ) ) );
 return( best );
 }

template< class T >
Segment< T > maxSumSegmentPrefix( const std::vector< T > & D )
{
 ADS_PRE( ! D.empty() );
 Segment< T > best{ D[ 0 ] , 0 , 1 };
 T tmp{} , minTmp{};                        // P[ s ] and min_{b < s} P[ b ]
 for( std::size_t btmp = 0 , s = 0 ; s < D.size() ; ++s ) {
  tmp += D[ s ];
  if( best.sum < tmp - minTmp )
   best = Segment< T >{ tmp - minTmp , btmp , s - btmp + 1 };
  if( tmp < minTmp ) {
   minTmp = tmp;
   btmp = s + 1;
   }
  }
 ADS_AUDIT( detail::segmentSum( D , best ) );
 return( best );
 }

template< class T >
Segment< T > maxSumSegmentLength( const std::vector< T > & D ,
                                  std::size_t L , std::size_t U )
{
 ADS_PRE( ( L >= 1 ) && ( L <= U ) && ( L <= D.size() ) );
 const auto P = detail::prefixSums( D );
 std::deque< std::size_t > W;               // starts, increasing P
 Segment< T > best;
 for( std::size_t i = L ; i <= D.size() ; ++i ) {
  const std::size_t x = i - L;              // the new admissible start
  while( ! W.empty() && !( P[ W.back() ] < P[ x ] ) ) W.pop_back();
  W.push_back( x );
  if( i > U )
   while( W.front() < i - U ) W.pop_front();
  const T s = P[ i ] - P[ W.front() ];
  if( ( best.length == 0 ) || ( best.sum < s ) )
   best = Segment< T >{ s , W.front() , i - W.front() };
  }
 ADS_AUDIT( detail::segmentSum( D , best ) );
 return( best );
 }

template< class T >
Segment< T > longestSegmentAtLeast( const std::vector< T > & D , T t )
{
 const std::size_t n = D.size();
 const auto P = detail::prefixSums( D );
 std::vector< std::size_t > LMin( n + 1 , 0 );   // leftmost min of P[ 0 , x )
 for( std::size_t x = 2 ; x <= n ; ++x )
  LMin[ x ] = P[ x - 1 ] < P[ LMin[ x - 1 ] ] ? x - 1 : LMin[ x - 1 ];
 Segment< T > best;
 for( std::size_t i = 1 ; i <= n ; ++i ) {
  if( best.length >= i ) continue;          // L_i = i - length <= 0
  const std::size_t Li = i - best.length;   // the starts x < L_i
  std::size_t c = LMin[ Li ] , found = n + 1;
  for( ; ; ) {                              // the chain C_1 , C_2 , ...
   if( !( P[ i ] - P[ c ] >= t ) ) break;
   found = c;
   if( c == 0 ) break;
   c = LMin[ c ];
   }
  if( found <= n )
   best = Segment< T >{ P[ i ] - P[ found ] , found , i - found };
  }
 ADS_AUDIT( detail::segmentSum( D , best ) );
 ADS_AUDIT( ( best.length == 0 ) || ( best.sum >= t ) );
 return( best );
 }

template< class T >
Segment< T > longestSegmentAverageAtLeast( const std::vector< T > & D ,
                                           T p , T q )
{
 ADS_PRE( q > T{} );
 std::vector< T > E( D.size() );
 for( std::size_t k = 0 ; k < D.size() ; ++k ) E[ k ] = q * D[ k ] - p;
 Segment< T > S = longestSegmentAtLeast( E , T{} );
 S.sum = T{};
 for( std::size_t k = S.first ; k < S.first + S.length ; ++k )
  S.sum += D[ k ];
 return( S );
 }

template< class T >
Segment< T > maxDensitySegment( const std::vector< T > & D ,
                                std::size_t L )
{
 ADS_PRE( ( L >= 1 ) && ( L <= D.size() ) );
 const auto P = detail::prefixSums( D );
 // the turn of o -> a -> b on the points ( x , P[ x ] ): > 0 to the left
 auto cross = [ & ]( std::size_t o , std::size_t a , std::size_t b ) {
  return( ( T( a ) - T( o ) ) * ( P[ b ] - P[ o ] ) -
          ( P[ a ] - P[ o ] ) * ( T( b ) - T( o ) ) ); };
 std::vector< std::size_t > H;              // the lower hull of the starts
 Segment< T > best;
 for( std::size_t i = L ; i <= D.size() ; ++i ) {
  const std::size_t x = i - L;
  while( ( H.size() >= 2 ) &&
         !( cross( H[ H.size() - 2 ] , H.back() , x ) > T{} ) )
   H.pop_back();
  H.push_back( x );
  std::size_t lo = 0 , hi = H.size() - 1;   // the first vertex where the
  while( lo < hi ) {                        // edge turns away from i
   const std::size_t m = ( lo + hi ) / 2;
   if( cross( H[ m ] , H[ m + 1 ] , i ) >= T{} ) lo = m + 1;
   else hi = m;
   }
  const std::size_t b = H[ lo ];
  const T s = P[ i ] - P[ b ];
  if( ( best.length == 0 ) ||               // s / ( i - b ) > best
      ( s * T( best.length ) > best.sum * T( i - b ) ) )
   best = Segment< T >{ s , b , i - b };
  }
 ADS_AUDIT( detail::segmentSum( D , best ) );
 return( best );
 }

inline std::vector< int > gcIndicator( const std::string & dna )
{
 std::vector< int > D( dna.size() );
 for( std::size_t k = 0 ; k < dna.size() ; ++k )
  D[ k ] = ( dna[ k ] == 'C' ) || ( dna[ k ] == 'G' ) ||
           ( dna[ k ] == 'c' ) || ( dna[ k ] == 'g' ) ? 1 : 0;
 return( D );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File MaximumSegments.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
