/*--------------------------------------------------------------------------*/
/*-------------------------- File NthElement.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The selection of NthElement.h, with its contracts.
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

#include <algorithm>
#include <bit>
#include <cstdint>
#include <optional>

namespace ADS {

namespace detail {

/// sorts [ first , last ) by insertion
template< class It , class C >
void selectInsertion( It first , It last , C & less )
{
 if( first == last )
  return;
 for( It i = first + 1 ; i != last ; ++i ) {
  auto x = std::move( *i );
  It j = i;
  for( ; ( j != first ) && less( x , *( j - 1 ) ) ; --j )
   *j = std::move( *( j - 1 ) );
  *j = std::move( x );
  }
 }

/// the pivot in *first; the items smaller than it (not larger, if Equal)
/// before it, the others after it, by the scheme of Lomuto with no branch;
/// the place of the pivot
template< bool Equal , class It , class C >
It selectLomuto( It first , It last , C & less )
{
 const auto p = *first;
 auto left = [ & ]( const auto & x ) {      // x belongs on the left
  if constexpr( Equal )
   return( ! less( p , x ) );
  else
   return( less( x , p ) );
  };
 auto step = [ & ]( It i , It & lt ) {
  auto x = std::move( *i );
  const bool c = left( x );
  *i = std::move( *lt );
  *lt = std::move( x );
  lt += c;
  };
 It lt = first + 1;                         // the end of the small ones
 It i = first + 1;
 for( ; last - i >= 4 ; i += 4 ) {          // 4 at a time, fewer jumps
  step( i , lt );
  step( i + 1 , lt );
  step( i + 2 , lt );
  step( i + 3 , lt );
  }
 for( ; i != last ; ++i )
  step( i , lt );
 --lt;
 std::iter_swap( first , lt );
 return( lt );
 }

/// the same, by the scheme of Hoare: two indices from the ends swap the
/// items on the wrong side, the others are read and never written, and on
/// a range in order the branches are predicted
template< bool Equal , class It , class C >
It selectHoare( It first , It last , C & less )
{
 const auto p = *first;
 auto left = [ & ]( const auto & x ) {      // x belongs on the left
  if constexpr( Equal )
   return( ! less( p , x ) );
  else
   return( less( x , p ) );
  };
 It i = first + 1 , j = last - 1;
 for( ; ; ) {
  while( ( i <= j ) && left( *i ) )
   ++i;
  while( ( i <= j ) && ! left( *j ) )
   --j;
  if( i >= j )
   break;
  std::iter_swap( i , j );
  ++i;
  --j;
  }
 --i;
 std::iter_swap( first , i );
 return( i );
 }

/// the partition by the scheme that suits the order of the range
template< bool Equal , class It , class C >
It selectPartition( It first , It last , C & less , bool ordered )
{
 return( ordered ? selectHoare< Equal >( first , last , less )
                 : selectLomuto< Equal >( first , last , less ) );
 }

/// the direction of the 64 items taken at a stride, if all of them rise
/// (1) or all of them fall (-1), else 0
template< class It , class C >
int selectMonotone( It first , std::ptrdiff_t n , C & less )
{
 constexpr std::ptrdiff_t P = 64;
 bool up = true , down = true;
 for( std::ptrdiff_t i = 1 ; i < P ; ++i ) {
  const auto & a = first[ ( i - 1 ) * ( n / P ) ];
  const auto & b = first[ i * ( n / P ) ];
  up = up && ! less( b , a );
  down = down && ! less( a , b );
  }
 return( up ? 1 : down ? -1 : 0 );
 }

/// whether [ first , last ) is sorted, by blocks of 64 pairs with no exit
/// inside a block, which the compiler can vectorize
template< class It , class C >
bool selectSorted( It first , It last , C & less )
{
 const std::ptrdiff_t n = last - first;
 std::ptrdiff_t i = 0;
 for( ; i + 65 <= n ; i += 64 ) {
  bool bad = false;
  for( std::ptrdiff_t j = 0 ; j < 64 ; ++j )
   bad |= less( first[ i + j + 1 ] , first[ i + j ] );
  if( bad )
   return( false );
  }
 for( ; i + 1 < n ; ++i )
  if( less( first[ i + 1 ] , first[ i ] ) )
   return( false );
 return( true );
 }

/// reverses [ first , last ) and tells whether it was in decreasing
/// order, in one pass that swaps the pairs of places from the two ends
/// while it checks the order of their neighbours; a range that was not in
/// order is left partly reversed, which a selection does not mind
template< class It , class C >
bool selectReversed( It first , It last , C & less )
{
 const std::ptrdiff_t n = last - first , h = n / 2;
 bool bad = false;
 for( std::ptrdiff_t i = 0 ; i < h ; i += 64 ) {
  const std::ptrdiff_t e = std::min( h , i + 64 );
  for( std::ptrdiff_t j = i ; j < e ; ++j )   // neighbours not yet swapped
   bad |= less( first[ j ] , first[ j + 1 ] ) |
          less( first[ n - 2 - j ] , first[ n - 1 - j ] );
  if( bad )
   return( false );
  for( std::ptrdiff_t j = i ; j < e ; ++j )
   std::iter_swap( first + j , first + ( n - 1 - j ) );
  }
 return( true );
 }

/// whether the range looks in some order: the turns (a rise followed by a
/// fall, or the opposite) among 64 of its items taken at a stride, about 41
/// on a random order, are fewer than 16
template< class It , class C >
bool selectOrdered( It first , std::ptrdiff_t n , C & less )
{
 constexpr std::ptrdiff_t P = 64;
 int turns = 0 , last = 0;
 for( std::ptrdiff_t i = 1 ; i < P ; ++i ) {
  const auto & a = first[ ( i - 1 ) * ( n / P ) ];
  const auto & b = first[ i * ( n / P ) ];
  const int d = less( a , b ) ? 1 : less( b , a ) ? -1 : 0;
  if( d && last && ( d != last ) )
   ++turns;
  if( d )
   last = d;
  }
 return( turns < 16 );
 }

/// the place of the median of a, b and c
template< class It , class C >
It selectMedian3( It a , It b , It c , C & less )
{
 if( less( *a , *b ) )
  return( less( *b , *c ) ? b : ( less( *a , *c ) ? c : a ) );
 return( less( *a , *c ) ? a : ( less( *b , *c ) ? c : b ) );
 }

template< class It , class C >
void select( It first , It nth , It last , C & less );

/// the place of the median of the medians of the groups of 5, which are
/// moved to the front
template< class It , class C >
It selectMedianOfMedians( It first , It last , C & less )
{
 It w = first;
 for( It g = first ; last - g >= 5 ; g += 5 ) {
  selectInsertion( g , g + 5 , less );
  std::iter_swap( w++ , g + 2 );
  }
 const auto m = w - first;
 select( first , first + m / 2 , w , less );
 return( first + m / 2 );
 }

template< class It , class C >
void select( It first , It nth , It last , C & less )
{
 int budget = 2 * int( std::bit_width( std::size_t( last - first ) ) );
 std::optional< std::iter_value_t< It > > floor;   // see below
 while( last - first > 24 ) {
  const std::ptrdiff_t n = last - first;
  It p;
  bool ordered = false;
  if( budget <= 0 )                         // the worst case stays linear
   p = selectMedianOfMedians( first , last , less );
  else if( n > 600 ) {                      // the sample of Floyd and Rivest
   const std::ptrdiff_t k = nth - first;
   const double z = std::log( double( n ) );
   const double s = 0.5 * std::exp( 2 * z / 3 );
   const double d = 0.5 * std::sqrt( z * s * ( double( n ) - s ) /
                                     double( n ) ) *
                    ( 2 * k < n ? -1 : 1 );
   const auto lo = std::max< std::ptrdiff_t >( 0 , std::min( k ,
                    std::ptrdiff_t( double( k ) - double( k ) * s /
                                    double( n ) + d ) ) );
   const auto hi = std::min< std::ptrdiff_t >( n - 1 , std::max( k ,
                    std::ptrdiff_t( double( k ) + double( n - k ) * s /
                                    double( n ) + d ) ) );
   ordered = selectOrdered( first , n , less );
   if( ordered ) {                          // a sample from all the range
    const std::ptrdiff_t w = hi - lo + 1;
    for( std::ptrdiff_t i = 0 ; i < w ; ++i )
     std::iter_swap( first + lo + i , first + i * ( n / w ) );
    }
   select( first + lo , nth , first + hi + 1 , less );
   p = nth;
   }
  else
   p = selectMedian3( first , first + n / 2 , last - 1 , less );
  std::iter_swap( first , p );
  // the items of the range are not smaller than the last pivot that cut
  // it on the left: a pivot equal to it is the smallest, and its equals go
  // left at once
  const bool smallest = floor && ! less( *floor , *first );
  It m = smallest ? first :
                    selectPartition< false >( first , last , less , ordered );
  if( m == first ) {                        // the smallest: its equals too
   m = selectPartition< true >( first , last , less , ordered );
   if( nth <= m )
    return;
   first = m + 1;
   }
  else if( nth == m )
   return;
  else if( nth < m )
   last = m;
  else {
   floor = *m;
   first = m + 1;
   }
  if( 4 * ( last - first ) > 3 * n )
   --budget;
  }
 selectInsertion( first , last , less );
 }

}  // end( namespace detail )

template< std::random_access_iterator It , class C >
void nthElement( It first , It nth , It last , C less )
{
 ADS_PRE( ( first == last ) || ( ( first <= nth ) && ( nth < last ) ) );
 if( first == last )
  return;
 const std::ptrdiff_t n = last - first;
 if( n > 600 ) {                            // a range in order costs a scan
  const int m = detail::selectMonotone( first , n , less );
  if( ( m > 0 ) && detail::selectSorted( first , last , less ) )
   return;
  if( ( m < 0 ) && detail::selectReversed( first , last , less ) )
   return;
  }
 detail::select( first , nth , last , less );
 ADS_AUDIT( std::none_of( first , nth , [ & ]( const auto & x ) {
                           return( less( *nth , x ) ); } ) &&
            std::none_of( nth + 1 , last , [ & ]( const auto & x ) {
                           return( less( x , *nth ) ); } ) );
 }

template< class T , class C >
T nthElement( std::vector< T > & A , std::size_t k , C less )
{
 ADS_PRE( k < A.size() );
 nthElement( A.begin() , A.begin() + std::ptrdiff_t( k ) , A.end() ,
             less );
 return( A[ k ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File NthElement.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
