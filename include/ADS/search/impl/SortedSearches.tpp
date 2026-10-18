/*--------------------------------------------------------------------------*/
/*------------------------ File SortedSearches.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of SortedSearches.h [see
 * SortedSearches.h], with their contracts.
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

/// A is a rotation of a strictly increasing vector: at most one place of
/// the circle where an item is not smaller than the next one, and there
/// the next one is smaller
template< class T , class C >
bool isRotatedIncreasing( const std::vector< T > & A , C & less )
{
 const std::size_t n = A.size();
 if( n <= 1 )
  return( true );
 std::size_t down = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const T & a = A[ i ] , & b = A[ ( i + 1 ) % n ];
  if( ! less( a , b ) ) {
   if( ! less( b , a ) )
    return( false );                        // equal items
   ++down;
   }
  }
 return( down == 1 );
 }

/// the distance b - a of a <= b, on the unsigned type for the integers
template< class T >
auto closestSpan( const T & a , const T & b )
{
 if constexpr( std::is_integral_v< T > ) {
  using U = std::make_unsigned_t< T >;
  return( U( U( b ) - U( a ) ) );
  }
 else
  return( b - a );
 }

/// the distance of a and b
template< class T >
auto closestDistance( const T & a , const T & b )
{
 return( a < b ? closestSpan( a , b ) : closestSpan( b , a ) );
 }

}  // end( namespace detail )

template< class T , class C >
std::size_t rotatedMinimum( const std::vector< T > & A , C less )
{
 ADS_PRE( ! A.empty() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::isRotatedIncreasing( A , less ) ); } ) );
 std::size_t lo = 0 , hi = A.size() - 1;    // the minimum in A[ lo .. hi ]
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( less( A[ hi ] , A[ m ] ) )
   lo = m + 1;                              // the rotation point after m
  else
   hi = m;
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::none_of( A.begin() , A.end() , [ & ]( const T & a ) {
   return( less( a , A[ lo ] ) ); } ) ); } ) );
 return( lo );
 }

template< class T >
 requires( std::is_arithmetic_v< T > && ! std::is_same_v< T , bool > )
std::size_t closestValue( const std::vector< T > & A , const T & x )
{
 ADS_PRE( ! A.empty() );
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 const std::size_t n = A.size() , r = lowerBound( A , x );
 std::size_t p = r;                         // A[ r ] the first not < x
 if( r == n )
  p = lowerBound( A , A[ n - 1 ] );
 else if( ( r > 0 ) && ( detail::closestSpan( A[ r - 1 ] , x ) <=
                         detail::closestSpan( x , A[ r ] ) ) )
  p = lowerBound( A , A[ r - 1 ] );
 ADS_AUDIT( [ & ]( void ) {
  const auto d = detail::closestDistance( A[ p ] , x );
  for( const T & a : A )
   if( ( detail::closestDistance( a , x ) < d ) ||
       ( ( detail::closestDistance( a , x ) == d ) && ( a < A[ p ] ) ) )
    return( false );
  return( ( p == 0 ) || ( A[ p - 1 ] < A[ p ] ) );
  }() );
 return( p );
 }

template< class T , class S >
std::optional< std::pair< std::size_t , std::size_t > >
pairWithSum( const std::vector< T > & A , const S & x )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 if( A.size() < 2 )
  return( std::nullopt );
 std::size_t i = 0 , j = A.size() - 1;
 while( i < j ) {                           // no pair out of A[ i .. j ]
  const S s = S( A[ i ] ) + S( A[ j ] );
  if( s == x ) {
   ADS_POST( ( i < j ) && ( S( A[ i ] ) + S( A[ j ] ) == x ) );
   return( std::pair( i , j ) );
   }
  if( s < x )
   ++i;                                     // A[ i ] too small for all
  else
   --j;                                     // A[ j ] too large for all
  }
 return( std::nullopt );
 }

template< class T , class C >
T kthOfTwoSorted( const std::vector< T > & X , const std::vector< T > & Y ,
                  std::size_t k , C less )
{
 ADS_PRE( k < X.size() + Y.size() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::is_sorted( X.begin() , X.end() , less ) &&
          std::is_sorted( Y.begin() , Y.end() , less ) ); } ) );
 const bool swap = X.size() > Y.size();
 const std::vector< T > & S = swap ? Y : X , & L = swap ? X : Y;
 const std::size_t m = S.size() , n = L.size();
 std::size_t lo = ( k + 1 > n ) ? k + 1 - n : 0 , hi = std::min( k + 1 , m );
 while( lo < hi ) {                         // i items of S, j of L
  const std::size_t i = lo + ( hi - lo ) / 2 , j = k + 1 - i;
  if( less( S[ i ] , L[ j - 1 ] ) )
   lo = i + 1;                              // more items of S
  else
   hi = i;
  }
 const std::size_t i = lo , j = k + 1 - i;
 const T & r = ( i == 0 ) ? L[ j - 1 ] :
               ( j == 0 ) ? S[ i - 1 ] :
               ( less( S[ i - 1 ] , L[ j - 1 ] ) ? L[ j - 1 ] : S[ i - 1 ] );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  std::vector< T > M;
  M.reserve( m + n );
  std::merge( X.begin() , X.end() , Y.begin() , Y.end() ,
              std::back_inserter( M ) , less );
  return( ! less( r , M[ k ] ) && ! less( M[ k ] , r ) ); } ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SortedSearches.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
