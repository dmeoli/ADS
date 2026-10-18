/*--------------------------------------------------------------------------*/
/*------------------------ File SequenceQueries.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of SequenceQueries.h [see
 * SequenceQueries.h], with their contracts.
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

/// the length b - a of a <= b, on the unsigned type for the integers
template< class T >
auto gapSpan( const T & a , const T & b )
{
 if constexpr( std::is_integral_v< T > ) {
  using U = std::make_unsigned_t< T >;
  return( U( U( b ) - U( a ) ) );
  }
 else
  return( b - a );
 }

}  // end( namespace detail )

template< class T , class C >
std::vector< std::size_t > smallerToTheRight( const std::vector< T > & A ,
                                              C less )
{
 const std::size_t n = A.size();
 std::vector< std::size_t > c( n , 0 ) , P( n ) , B( n );
 std::iota( P.begin() , P.end() , std::size_t( 0 ) );
 for( std::size_t w = 1 ; w < n ; w *= 2 )  // the runs P[ lo .. lo + w )
  for( std::size_t lo = 0 ; lo + w < n ; lo += 2 * w ) {
   const std::size_t mid = lo + w , hi = std::min( lo + 2 * w , n );
   std::size_t i = lo , j = mid , k = lo;
   while( ( i < mid ) && ( j < hi ) )
    if( less( A[ P[ j ] ] , A[ P[ i ] ] ) )
     B[ k++ ] = P[ j++ ];                   // smaller than P[ i .. mid )
    else {
     c[ P[ i ] ] += j - mid;
     B[ k++ ] = P[ i++ ];
     }
   for( ; i < mid ; ++i ) {
    c[ P[ i ] ] += j - mid;
    B[ k++ ] = P[ i ];
    }
   for( ; j < hi ; ++j )
    B[ k++ ] = P[ j ];
   std::copy( B.begin() + std::ptrdiff_t( lo ) ,
              B.begin() + std::ptrdiff_t( hi ) ,
              P.begin() + std::ptrdiff_t( lo ) );
   }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  std::vector< T > D( A );
  return( std::accumulate( c.begin() , c.end() , std::size_t( 0 ) ) ==
          std::size_t( countInversions( D , less ) ) ); } ) );
 return( c );
 }

template< std::ranges::random_access_range R >
std::pair< std::size_t , std::size_t > longestKDistinctRun( const R & S ,
                                                            std::size_t k )
{
 using V = std::ranges::range_value_t< R >;
 const auto s = std::ranges::begin( S );
 const std::size_t n = std::size_t( std::ranges::size( S ) );
 std::map< V , std::size_t > count;         // the values of the window
 std::size_t start = 0 , best = 0 , bestStart = 0;
 for( std::size_t end = 0 ; end < n ; ++end ) {
  ++count[ s[ end ] ];
  while( count.size() > k ) {               // S[ start .. end ]
   const auto it = count.find( s[ start ] );
   if( --it->second == 0 )
    count.erase( it );
   ++start;
   }
  if( end + 1 - start > best ) {
   best = end + 1 - start;
   bestStart = start;
   }
  }
 ADS_AUDIT( std::set< V >( s + std::ptrdiff_t( bestStart ) ,
                           s + std::ptrdiff_t( bestStart + best ) ).size()
            <= k );
 return( std::pair( bestStart , best ) );
 }

template< class T >
 requires( std::is_arithmetic_v< T > && ! std::is_same_v< T , bool > )
std::optional< std::pair< T , T > > largestGap(
                                        std::vector< std::pair< T , T > > I )
{
 ADS_PRE( std::all_of( I.begin() , I.end() , []( const auto & p ) {
  return( p.first <= p.second ); } ) );
 std::sort( I.begin() , I.end() );
 std::optional< std::pair< T , T > > best;
 if( I.empty() )
  return( best );
 T end = I[ 0 ].second;                     // the farthest end so far
 for( const auto & [ a , b ] : I ) {
  if( ( end < a ) && ( ! best || ( detail::gapSpan( end , a ) >
                                   detail::gapSpan( best->first ,
                                                    best->second ) ) ) )
   best = std::pair( end , a );
  end = std::max( end , b );
  }
 ADS_AUDIT( ! best || std::none_of( I.begin() , I.end() ,
                                    [ & ]( const auto & p ) {
  return( ( p.second > best->first ) && ( p.first < best->second ) ); } ) );
 return( best );
 }

template< class T , class W >
 requires std::is_arithmetic_v< T >
W lineAssignment( std::vector< T > H , std::vector< T > L )
{
 ADS_PRE( H.size() == L.size() );
 std::sort( H.begin() , H.end() );
 std::sort( L.begin() , L.end() );
 W cost = W( 0 );
 for( std::size_t i = 0 ; i < H.size() ; ++i )
  cost += ( H[ i ] < L[ i ] ) ? W( L[ i ] ) - W( H[ i ] )
                              : W( H[ i ] ) - W( L[ i ] );
 return( cost );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SequenceQueries.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
