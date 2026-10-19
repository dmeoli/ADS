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

/// the runs of A as powerSort() finds them, the maximal non-decreasing
/// or strictly decreasing stretches, counted up to cap + 1: one pass that
/// stops at the run cap + 1, so that an input in no order costs O( cap )
template< class T , class C >
std::size_t runs( const std::vector< T > & A , C & less , std::size_t cap )
{
 const std::size_t n = A.size();
 std::size_t r = 0;
 for( std::size_t i = 0 ; ( i < n ) && ( r <= cap ) ; ++r ) {
  std::size_t j = i + 1;
  if( ( j < n ) && less( A[ j ] , A[ i ] ) )
   while( ( j < n ) && less( A[ j ] , A[ j - 1 ] ) ) ++j;
  else
   while( ( j < n ) && ! less( A[ j ] , A[ j - 1 ] ) ) ++j;
  i = j;
  }
 return( r );
 }

/// true if a sample of 64 equally spaced keys of A, n >= 64, holds at most
/// 16 distinct ones and they differ in at least 4 bytes: few keys, but
/// wide, which pdqSort() sorts in \f$ O( n \log k ) \f$ with fewer passes
/// than radixSort() needs bytes
template< RadixKey T >
bool fewWideKeys( const std::vector< T > & A )
{
 using U = decltype( radixKey( T() ) );
 std::array< U , 64 > S;
 const std::size_t step = A.size() / S.size();
 for( std::size_t i = 0 ; i < S.size() ; ++i )
  S[ i ] = radixKey( A[ i * step ] );
 std::sort( S.begin() , S.end() );
 std::size_t k = 1;
 std::uint64_t diff = 0;                    // the bits that differ
 for( std::size_t i = 1 ; i < S.size() ; ++i ) {
  k += S[ i ] != S[ i - 1 ];
  diff |= std::uint64_t( S[ i ] ^ S[ 0 ] );
  }
 int bytes = 0;
 for( ; diff != 0 ; diff >>= 8 )
  bytes += ( diff & 0xFF ) != 0;
 return( ( k <= 16 ) && ( bytes >= 4 ) );
 }

}  // end( namespace detail )

template< class T , class C >
void sort( std::vector< T > & A , C less )
{
 if constexpr( detail::naturalOrder< T , C > && RadixKey< T > ) {
  if( A.size() < kRadix )
   pdqSort( A , less );
  else if( detail::runs( A , less , kRuns ) <= kRuns )
   powerSort( A , less );
  else if( detail::fewWideKeys( A ) )
   pdqSort( A , less );
  else
   radixSort( A );
  }
 else if constexpr( detail::naturalOrder< T , C > &&
                    std::is_same_v< T , std::string > ) {
  if( A.size() < kStrings )
   pdqSort( A , less );
  else if( detail::runs( A , less , kRuns ) <= kRuns )
   powerSort( A , less );
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
  if( ( A.size() < kRadix ) ||
      ( detail::runs( A , less , kRuns ) <= kRuns ) )
   powerSort( A , less );
  else if constexpr( std::is_floating_point_v< T > )   // -0 and +0 equal
   radixSortBy( A , []( T x ) {
    return( radixKey( x == T( 0 ) ? T( 0 ) : x ) ); } );
  else
   radixSort( A );
  }
 else if constexpr( detail::naturalOrder< T , C > &&
                    std::is_same_v< T , std::string > ) {
  if( ( A.size() < kStrings ) ||
      ( detail::runs( A , less , kRuns ) <= kRuns ) )
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
