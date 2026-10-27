/*--------------------------------------------------------------------------*/
/*-------------------------- File KnuthSorts.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of KnuthSorts.h, with their contracts.
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

/// a sequence of places in a treap ordered by position, each node with
/// the size of its subtree and its parent: the item at a rank, the rank of
/// a node and an insertion at a rank in expected \f$ O( \log n ) \f$
class RankTree {
 public:
 explicit RankTree( std::size_t n ) {
  f_item.reserve( n ); f_l.reserve( n ); f_r.reserve( n );
  f_p.reserve( n ); f_s.reserve( n );
  }

 [[nodiscard]] std::size_t size( void ) const { return( sz( f_root ) ); }

 /// the item of rank k
 [[nodiscard]] std::size_t at( std::size_t k ) const {
  long v = f_root;
  for( ; ; ) {
   const std::size_t l = sz( f_l[ v ] );
   if( k == l ) return( f_item[ v ] );
   if( k < l ) v = f_l[ v ];
   else {
    k -= l + 1;
    v = f_r[ v ];
    }
   }
  }

 /// the rank of the node z
 [[nodiscard]] std::size_t rank( long z ) const {
  std::size_t k = sz( f_l[ z ] );
  for( ; f_p[ z ] >= 0 ; z = f_p[ z ] )
   if( f_r[ f_p[ z ] ] == z ) k += sz( f_l[ f_p[ z ] ] ) + 1;
  return( k );
  }

 /// x at the rank k, as a leaf rotated up by its priority; its node
 long insert( std::size_t k , std::size_t x ) {
  const long z = long( f_item.size() );
  f_item.push_back( x ); f_l.push_back( -1 ); f_r.push_back( -1 );
  f_p.push_back( -1 ); f_s.push_back( 1 );
  if( f_root < 0 ) {
   f_root = z;
   return( z );
   }
  for( long v = f_root ; ; ) {              // down to a free place
   ++f_s[ v ];
   const std::size_t l = sz( f_l[ v ] );
   long & next = k <= l ? f_l[ v ] : f_r[ v ];
   if( k > l ) k -= l + 1;
   if( next < 0 ) {
    next = z;
    f_p[ z ] = v;
    break;
    }
   v = next;
   }
  while( ( f_p[ z ] >= 0 ) && ( priority( z ) > priority( f_p[ z ] ) ) )
   rotateUp( z );
  return( z );
  }

 /// the items in order
 [[nodiscard]] std::vector< std::size_t > items( void ) const {
  std::vector< std::size_t > out;
  out.reserve( size() );
  std::vector< long > stack;
  for( long v = f_root ; ( v >= 0 ) || ! stack.empty() ; ) {
   for( ; v >= 0 ; v = f_l[ v ] ) stack.push_back( v );
   v = stack.back();
   stack.pop_back();
   out.push_back( f_item[ v ] );
   v = f_r[ v ];
   }
  return( out );
  }

 private:
 [[nodiscard]] std::size_t sz( long v ) const {
  return( v < 0 ? 0 : f_s[ v ] );
  }

 /// a fixed pseudorandom priority, a mix of the node (the finalizer of
 /// MurmurHash3)
 [[nodiscard]] static unsigned long long priority( long v ) {
  unsigned long long h = ( unsigned long long )( v ) + 1;
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdULL;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ULL;
  return( h ^ ( h >> 33 ) );
  }

 /// z above its parent, the sizes of both fixed
 void rotateUp( long z ) {
  const long p = f_p[ z ] , g = f_p[ p ];
  if( f_l[ p ] == z ) {
   f_l[ p ] = f_r[ z ];
   if( f_r[ z ] >= 0 ) f_p[ f_r[ z ] ] = p;
   f_r[ z ] = p;
   }
  else {
   f_r[ p ] = f_l[ z ];
   if( f_l[ z ] >= 0 ) f_p[ f_l[ z ] ] = p;
   f_l[ z ] = p;
   }
  f_p[ p ] = z;
  f_p[ z ] = g;
  if( g < 0 ) f_root = z;
  else ( f_l[ g ] == p ? f_l[ g ] : f_r[ g ] ) = z;
  f_s[ p ] = 1 + sz( f_l[ p ] ) + sz( f_r[ p ] );
  f_s[ z ] = 1 + sz( f_l[ z ] ) + sz( f_r[ z ] );
  }

 std::vector< std::size_t > f_item , f_s;
 std::vector< long > f_l , f_r , f_p;
 long f_root = -1;
 };

/// the places of A in v sorted by merge insertion; the chain in a RankTree,
/// so that the place of l_{i+2} and each insertion cost
/// \f$ O( \log n ) \f$ and a probe of the binary search
/// \f$ O( \log n ) \f$, instead of a search and a shift of the chain
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
 RankTree chain( n );
 chain.insert( 0 , partner[ large[ 0 ] ] ); // s_1 before l_1, for free
 std::vector< long > node( large.size() );  // the node of l_{i+1}
 for( std::size_t i = 0 ; i < large.size() ; ++i )
  node[ i ] = chain.insert( i + 1 , large[ i ] );
 std::vector< std::size_t > pend;           // s_2 , ... and the leftover
 for( std::size_t i = 1 ; i < large.size() ; ++i )
  pend.push_back( partner[ large[ i ] ] );
 if( odd ) pend.push_back( v.back() );
 const std::size_t m = pend.size();         // pend[ i ] is s_{i+2}
 auto insert = [ & ]( std::size_t i ) {     // s_{i+2} before l_{i+2}
  const std::size_t s = pend[ i ];
  std::size_t hi = chain.size();
  if( i + 1 < large.size() )
   hi = chain.rank( node[ i + 1 ] );
  std::size_t lo = 0;
  while( lo < hi ) {
   const std::size_t md = lo + ( hi - lo ) / 2;
   if( less( A[ s ] , A[ chain.at( md ) ] ) ) hi = md; else lo = md + 1;
   }
  chain.insert( lo , s );
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
 return( chain.items() );
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
