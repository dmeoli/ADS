/*--------------------------------------------------------------------------*/
/*----------------------------- File Sort.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sorts of Sort.h.
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

/// true if A is already sorted, or strictly decreasing and then reversed:
/// one pass that stops at the first pair out of order, so that it costs
/// next to nothing on an input in no order
template< class T , class C >
bool presorted( std::vector< T > & A , C & less )
{
 const std::size_t n = A.size();
 std::size_t i = 1;
 while( ( i < n ) && ! less( A[ i ] , A[ i - 1 ] ) ) ++i;
 if( i == n )
  return( true );
 if( i > 1 )
  return( false );
 while( ( i < n ) && less( A[ i ] , A[ i - 1 ] ) ) ++i;
 if( i < n )
  return( false );
 std::reverse( A.begin() , A.end() );       // strictly decreasing: stable
 return( true );
 }

}  // end( namespace detail )

template< class T , class C >
void sort( std::vector< T > & A , C less )
{
 if constexpr( detail::naturalOrder< T , C > && RadixKey< T > ) {
  if( A.size() < kRadix )
   pdqSort( A , less );
  else if( ! detail::presorted( A , less ) )
   radixSort( A );
  }
 else if constexpr( detail::naturalOrder< T , C > &&
                    std::is_same_v< T , std::string > ) {
  if( A.size() < kStrings )
   pdqSort( A , less );
  else
   msdRadixSort( A );
  }
 else
  pdqSort( A , less );
 }

template< class T , class C >
void stableSort( std::vector< T > & A , C less )
{
 if constexpr( detail::naturalOrder< T , C > && RadixKey< T > ) {
  if( A.size() < kRadix )
   powerSort( A , less );
  else if( ! detail::presorted( A , less ) ) {
   if constexpr( std::is_floating_point_v< T > )   // -0 and +0 are equal
    radixSortBy( A , []( T x ) {
     return( radixKey( x == T( 0 ) ? T( 0 ) : x ) ); } );
   else
    radixSort( A );
   }
  }
 else if constexpr( detail::naturalOrder< T , C > &&
                    std::is_same_v< T , std::string > ) {
  if( A.size() < kStrings )
   powerSort( A , less );
  else
   msdRadixSort( A );
  }
 else
  powerSort( A , less );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Sort.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
