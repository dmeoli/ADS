/*--------------------------------------------------------------------------*/
/*-------------------------- File KnuthSorts.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of KnuthSorts.h, with their contracts.
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

template< class T , class C >
void binaryInsertionSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 for( std::size_t i = 1 ; i < A.size() ; ++i ) {
  std::size_t lo = 0 , hi = i;              // after the equals: stable
  while( lo < hi ) {
   const std::size_t m = lo + ( hi - lo ) / 2;
   if( less( A[ i ] , A[ m ] ) ) hi = m; else lo = m + 1;
   }
  std::rotate( A.begin() + lo , A.begin() + i , A.begin() + i + 1 );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class C >
void listInsertionSort( std::vector< T > & A , C less , std::size_t * moves )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 std::vector< std::size_t > L( n + 1 , 0 ); // L[ 0 ] the head, 0 the end;
 L[ 0 ] = n;                                // the item i is the node i + 1
 if( n > 0 ) {
  L[ n ] = 0;                               // the last item alone
  for( std::size_t j = n - 1 ; j-- > 0 ; ) { // L2: from the right, so that
   std::size_t p = 0 , q = L[ 0 ];          // equal keys keep their order
   while( ( q != 0 ) && less( A[ q - 1 ] , A[ j ] ) ) {
    p = q;
    q = L[ q ];
    }
   L[ p ] = j + 1;                          // L4: j between p and q
   L[ j + 1 ] = q;
   }
  }
 std::vector< T > B;                        // follow the list
 B.reserve( n );
 for( std::size_t q = L[ 0 ] ; q != 0 ; q = L[ q ] )
  B.push_back( std::move( A[ q - 1 ] ) );
 A = std::move( B );
 if( moves ) *moves = n;
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T >
void addressCalculationSort( std::vector< T > & A , std::size_t M )
{
 static_assert( std::is_arithmetic_v< T > , "numbers only" );
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 if( n > 1 ) {
  if( M == 0 ) M = n / 4 + 1;
  const auto [ mn , mx ] = std::minmax_element( A.begin() , A.end() );
  const double lo = double( *mn ) , w = double( *mx ) - lo;
  std::vector< std::vector< T > > lists( M );
  for( const T & x : A ) {                  // the list of the range of x
   // an infinite range (an infinity, or a span beyond a double) would
   // make the place NaN: then all the items go to one list
   std::size_t i = ( w > 0 ) && std::isfinite( w ) ?
                   std::size_t( ( double( x ) - lo ) / w * double( M ) ) : 0;
   if( i >= M ) i = M - 1;
   auto & l = lists[ i ];
   l.push_back( x );                        // inserted in place, stable
   for( std::size_t j = l.size() - 1 ; ( j > 0 ) && ( x < l[ j - 1 ] ) ;
        --j )
    std::swap( l[ j ] , l[ j - 1 ] );
   }
  std::size_t k = 0;
  for( auto & l : lists )
   for( const T & x : l ) A[ k++ ] = x;
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , std::less<>() ) );
 }

namespace detail {

/// the radix exchange of A[ lo .. hi ) on the bit b and the lower ones
template< RadixKey T >
void radixExchange( std::vector< T > & A , std::size_t lo , std::size_t hi ,
                    int b )
{
 while( ( hi - lo > 1 ) && ( b >= 0 ) ) {
  auto bit = [ b ]( T x ) { return( ( radixKey( x ) >> b ) & 1 ); };
  std::size_t i = lo , j = hi;              // 0 in [ lo , i ) , 1 after j
  for( ; ; ) {
   while( ( i < j ) && ! bit( A[ i ] ) ) ++i;
   while( ( i < j ) && bit( A[ j - 1 ] ) ) --j;
   if( i >= j ) break;
   std::swap( A[ i++ ] , A[ --j ] );
   }
  radixExchange( A , lo , i , b - 1 );      // the keys with the bit 0
  lo = i;                                   // those with 1, iterated
  --b;
  }
 }

}  // end( namespace detail )

template< RadixKey T >
void radixExchangeSort( std::vector< T > & A )
{
 const auto before = detail::auditCopy( A );
 detail::radixExchange( A , 0 , A.size() , int( 8 * sizeof( T ) ) - 1 );
 ADS_AUDIT( detail::sortedPermutation( A , before , []( T a , T b ) {
  return( radixKey( a ) < radixKey( b ) ); } ) );
 }

template< class T , class C >
void tournamentSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 if( n > 1 ) {
  std::size_t L = 1;                        // the leaves, a power of 2
  while( L < n ) L *= 2;
  std::vector< std::size_t > W( 2 * L , n );   // the winner of each match,
  auto play = [ & ]( std::size_t a , std::size_t b ) {   // n if nobody
   if( a == n ) return( b );
   if( b == n ) return( a );
   return( less( A[ b ] , A[ a ] ) ? b : a );   // ties to the left: stable
   };
  for( std::size_t i = 0 ; i < n ; ++i ) W[ L + i ] = i;
  for( std::size_t v = L - 1 ; v > 0 ; --v )
   W[ v ] = play( W[ 2 * v ] , W[ 2 * v + 1 ] );
  std::vector< T > B;
  B.reserve( n );
  for( std::size_t k = 0 ; k < n ; ++k ) {
   const std::size_t w = W[ 1 ];
   B.push_back( A[ w ] );
   std::size_t v = L + w;                   // the winner leaves: its path
   W[ v ] = n;                              // is played again
   for( v /= 2 ; v > 0 ; v /= 2 )
    W[ v ] = play( W[ 2 * v ] , W[ 2 * v + 1 ] );
   }
  A = std::move( B );
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

inline long fordJohnsonBound( std::size_t n )
{
 long s = 0;
 for( std::size_t k = 1 ; k <= n ; ++k ) {  // ceil( log2( 3 k / 4 ) )
  long c = 0;
  while( ( 4UL << c ) < 3 * k ) ++c;        // the least c with 2^c >= 3k/4
  s += c;
  }
 return( s );
 }

namespace detail {

/// the places of A in v sorted by merge insertion
template< class T , class C >
std::vector< std::size_t > fordJohnson( const std::vector< T > & A ,
                                        std::vector< std::size_t > v ,
                                        C & less )
{
 const std::size_t n = v.size();
 if( n <= 1 )
  return( v );
 std::vector< std::size_t > large , partner( A.size() );
 for( std::size_t i = 0 ; i + 1 < n ; i += 2 ) {   // pairs, one comparison
  std::size_t a = v[ i ] , b = v[ i + 1 ];
  if( less( A[ b ] , A[ a ] ) ) std::swap( a , b );
  large.push_back( b );
  partner[ b ] = a;
  }
 const bool odd = n % 2;
 large = fordJohnson( A , large , less );   // the larger ones, recursively
 std::vector< std::size_t > chain;
 chain.push_back( partner[ large[ 0 ] ] );  // s_1 before l_1, for free
 chain.insert( chain.end() , large.begin() , large.end() );
 std::vector< std::size_t > pend;           // s_2 , ... and the leftover
 for( std::size_t i = 1 ; i < large.size() ; ++i )
  pend.push_back( partner[ large[ i ] ] );
 if( odd ) pend.push_back( v.back() );
 const std::size_t m = pend.size();         // pend[ i ] is s_{i+2}
 auto insert = [ & ]( std::size_t i ) {     // s_{i+2} before l_{i+2}
  const std::size_t s = pend[ i ];
  std::size_t hi = chain.size();
  if( i + 1 < large.size() )
   hi = std::size_t( std::find( chain.begin() , chain.end() ,
                                large[ i + 1 ] ) - chain.begin() );
  std::size_t lo = 0;
  while( lo < hi ) {
   const std::size_t md = lo + ( hi - lo ) / 2;
   if( less( A[ s ] , A[ chain[ md ] ] ) ) hi = md; else lo = md + 1;
   }
  chain.insert( chain.begin() + lo , s );
  };
 std::size_t prev = 1 , t = 1;              // the Jacobsthal numbers t_k
 for( int k = 1 ; prev < m + 1 ; ++k ) {    // insert s_{t_k} , ... , down
  t = ( ( std::size_t( 1 ) << ( k + 1 ) ) + ( k % 2 ? std::size_t( -1 ) :
                                               std::size_t( 1 ) ) ) / 3;
  if( t <= prev ) continue;
  for( std::size_t j = std::min( t , m + 1 ) ; j > prev ; --j )
   insert( j - 2 );                         // s_j is pend[ j - 2 ]
  prev = t;
  }
 return( chain );
 }

}  // end( namespace detail )

template< class T , class C >
void mergeInsertionSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 std::vector< std::size_t > v( A.size() );
 for( std::size_t i = 0 ; i < v.size() ; ++i ) v[ i ] = i;
 const auto order = detail::fordJohnson( A , v , less );
 std::vector< T > B;
 B.reserve( A.size() );
 for( std::size_t i : order ) B.push_back( std::move( A[ i ] ) );
 A = std::move( B );
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File KnuthSorts.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
