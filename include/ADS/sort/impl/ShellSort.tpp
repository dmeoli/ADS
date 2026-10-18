/*--------------------------------------------------------------------------*/
/*--------------------------- File ShellSort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of ShellSort.h [see ShellSort.h].
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

/// h is not empty, positive and increasing, and h[ 0 ] = 1 if one
inline bool validGaps( const std::vector< std::size_t > & h , bool one )
{
 if( h.empty() || ( h[ 0 ] == 0 ) || ( one && ( h[ 0 ] != 1 ) ) )
  return( false );
 for( std::size_t i = 1 ; i < h.size() ; ++i )
  if( h[ i ] <= h[ i - 1 ] )
   return( false );
 return( true );
 }

}  // end( namespace detail )

inline std::vector< std::size_t > knuthGaps( std::size_t n )
{
 const auto checked = []( const std::vector< std::size_t > & h ) {
  ADS_POST( detail::validGaps( h , true ) );
  return( h );
  };
 std::vector< std::size_t > h = { 1 };
 while( 3 * h.back() + 1 < n )
  h.push_back( 3 * h.back() + 1 );
 return( checked( h ) );
 }

inline std::vector< std::size_t > hibbardGaps( std::size_t n )
{
 std::vector< std::size_t > h = { 1 };
 while( 2 * h.back() + 1 < n )
  h.push_back( 2 * h.back() + 1 );
 ADS_POST( detail::validGaps( h , true ) );
 return( h );
 }

inline std::vector< std::size_t > shellGaps( std::size_t n )
{
 const auto checked = []( const std::vector< std::size_t > & h ) {
  ADS_POST( detail::validGaps( h , true ) );
  return( h );
  };
 std::vector< std::size_t > h;
 for( std::size_t g = n / 2 ; g > 0 ; g /= 2 )
  h.insert( h.begin() , g );
 if( h.empty() )
  h.push_back( 1 );
 return( checked( h ) );
 }

inline std::vector< std::size_t > prattGaps( std::size_t n )
{
 const auto checked = []( const std::vector< std::size_t > & h ) {
  ADS_POST( detail::validGaps( h , true ) );
  return( h );
  };
 std::vector< std::size_t > h;
 for( std::size_t p = 1 ; p <= std::max< std::size_t >( n , 1 ) ; p *= 3 )
  for( std::size_t q = p ; q <= std::max< std::size_t >( n , 1 ) ; q *= 2 )
   h.push_back( q );
 std::sort( h.begin() , h.end() );
 return( checked( h ) );
 }

inline std::vector< std::size_t > ciuraGaps( std::size_t n )
{
 const auto checked = []( const std::vector< std::size_t > & h ) {
  ADS_POST( detail::validGaps( h , true ) );
  return( h );
  };
 std::vector< std::size_t > h = { 1 , 4 , 10 , 23 , 57 , 132 , 301 , 701 };
 while( h.back() < n )
  h.push_back( std::size_t( double( h.back() ) * 2.25 ) );
 while( ( h.size() > 1 ) && ( h.back() >= n ) )
  h.pop_back();
 return( checked( h ) );
 }

template< class T , class C >
void shellSort( std::vector< T > & A ,
                const std::vector< std::size_t > & gaps , C less )
{
 ADS_PRE( detail::validGaps( gaps , false ) );
 const auto before = detail::auditCopy( A );
 const std::size_t n = A.size();
 for( std::size_t g = gaps.size() ; g-- > 0 ; ) {
  const std::size_t h = gaps[ g ];
  for( std::size_t i = h ; i < n ; ++i ) {
   T temp = std::move( A[ i ] );
   std::size_t j = i;
   while( ( j >= h ) && less( temp , A[ j - h ] ) ) {
    A[ j ] = std::move( A[ j - h ] );
    j -= h;
    }
   A[ j ] = std::move( temp );
   }
  }
 ADS_AUDIT( ( gaps[ 0 ] == 1 )
            ? detail::sortedPermutation( A , before , less )
            : detail::samePermutation( A , before , less ) );
 }

template< class T , class C >
void shellSort( std::vector< T > & A , C less )
{
 shellSort( A , knuthGaps( A.size() ) , less );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File ShellSort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
