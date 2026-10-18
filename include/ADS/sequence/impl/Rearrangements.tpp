/*--------------------------------------------------------------------------*/
/*------------------------ File Rearrangements.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Rearrangements.h [see
 * Rearrangements.h], with their contracts.
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

/// A[ 0 ] <= A[ 1 ] >= A[ 2 ] <= ... by less
template< class T , class C >
bool wiggles( const std::vector< T > & A , C & less )
{
 return( contract( [ & ]( void ) {
  for( std::size_t i = 0 ; i + 1 < A.size() ; ++i )
   if( ( i % 2 == 0 ) ? less( A[ i + 1 ] , A[ i ] )
                      : less( A[ i ] , A[ i + 1 ] ) )
    return( false );
  return( true ); } ) );
 }

/// each item of A at most k places away from its place in the stable
/// sorted order, the one that moves the items the least
template< class T , class C >
bool isKSorted( const std::vector< T > & A , std::size_t k , C & less )
{
 return( contract( [ & ]( void ) {
  std::vector< std::size_t > pi( A.size() );
  std::iota( pi.begin() , pi.end() , std::size_t( 0 ) );
  std::stable_sort( pi.begin() , pi.end() ,
                    [ & ]( std::size_t a , std::size_t b ) {
                     return( less( A[ a ] , A[ b ] ) ); } );
  for( std::size_t r = 0 ; r < pi.size() ; ++r )
   if( ( pi[ r ] > r ? pi[ r ] - r : r - pi[ r ] ) > k )
    return( false );
  return( true ); } ) );
 }

}  // end( namespace detail )

template< class T , class F >
std::pair< std::size_t , std::size_t > dutchNationalFlag(
                  std::vector< T > & A , F && colour , std::size_t * swaps )
{
 std::size_t moved = 0 , i = 0 , j = 0 , k = A.size();
 while( j < k ) {                           // A[ j .. k ) not examined
  const int c = int( colour( A[ j ] ) );
  ADS_PRE( ( c >= 0 ) && ( c <= 2 ) );
  if( c == 0 ) {
   std::swap( A[ i++ ] , A[ j++ ] );
   ++moved;
   }
  else if( c == 1 )
   ++j;
  else {
   std::swap( A[ j ] , A[ --k ] );
   ++moved;
   }
  }
 if( swaps )
  *swaps = moved;
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t h = 0 ; h < A.size() ; ++h )
   if( int( colour( A[ h ] ) ) != ( h < i ? 0 : ( h < j ? 1 : 2 ) ) )
    return( false );
  return( true );
  }() );
 return( std::pair( i , j ) );
 }

template< class T , class C >
void wiggleSort( std::vector< T > & A , C less )
{
 const auto before = detail::auditCopy( A );
 for( std::size_t i = 0 ; i + 1 < A.size() ; ++i )
  if( ( i % 2 == 0 ) ? less( A[ i + 1 ] , A[ i ] )
                     : less( A[ i ] , A[ i + 1 ] ) )
   std::swap( A[ i ] , A[ i + 1 ] );
 ADS_POST( detail::wiggles( A , less ) );
 ADS_AUDIT( detail::samePermutation( A , before , less ) );
 }

template< class T , class C >
void sortKSorted( std::vector< T > & A , std::size_t k , C less )
{
 ADS_AUDIT( detail::isKSorted( A , k , less ) );
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 const auto greater = [ & ]( const T & a , const T & b ) {
  return( less( b , a ) ); };
 if( n > 0 ) {
  const std::size_t w = std::min( k , n - 1 ) + 1;   // the window
  std::vector< T > H( A.begin() , A.begin() + std::ptrdiff_t( w ) );
  std::make_heap( H.begin() , H.end() , greater );   // A[ i .. i + k ]
  for( std::size_t i = 0 ; i < n ; ++i ) {
   std::pop_heap( H.begin() , H.end() , greater );
   A[ i ] = std::move( H.back() );
   H.pop_back();
   if( i + w < n ) {
    H.push_back( A[ i + w ] );
    std::push_heap( H.begin() , H.end() , greater );
    }
   }
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

template< class T , class W >
 requires std::is_arithmetic_v< T >
std::pair< std::size_t , W > minimumSwapSort( const std::vector< T > & A )
{
 const std::size_t n = A.size();
 std::vector< std::size_t > place( n );     // place[ r ]: the rank r item
 std::iota( place.begin() , place.end() , std::size_t( 0 ) );
 std::sort( place.begin() , place.end() ,
            [ & ]( std::size_t a , std::size_t b ) {
             return( A[ a ] < A[ b ] ); } );
 ADS_AUDIT( std::adjacent_find( place.begin() , place.end() ,
                                [ & ]( std::size_t a , std::size_t b ) {
                                 return( A[ a ] == A[ b ] ); } ) ==
            place.end() );
 std::vector< std::size_t > target( n );    // the rank of A[ i ]
 for( std::size_t r = 0 ; r < n ; ++r )
  target[ place[ r ] ] = r;
 std::vector< bool > seen( n , false );
 std::size_t swaps = 0;
 W cost = W( 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  if( seen[ i ] )
   continue;
  std::size_t k = 0;                        // the cycle of i
  W s = W( 0 ) , m = W( A[ i ] );
  for( std::size_t j = i ; ! seen[ j ] ; j = target[ j ] ) {
   seen[ j ] = true;
   ++k;
   s += W( A[ j ] );
   m = std::min( m , W( A[ j ] ) );
   }
  if( k > 1 ) {
   const W M = W( A[ place[ 0 ] ] );
   swaps += k - 1;
   cost += std::min( s + W( k - 2 ) * m , s + m + W( k + 1 ) * M );
   }
  }
 ADS_POST( swaps < std::max( n , std::size_t( 1 ) ) );
 return( std::pair( swaps , cost ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File Rearrangements.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
