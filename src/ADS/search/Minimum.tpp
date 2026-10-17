/*--------------------------------------------------------------------------*/
/*---------------------------- File Minimum.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Minimum.h [see Minimum.h], with their
 * contracts.
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
std::size_t argmin( const std::vector< T > & A , std::size_t start ,
                    std::size_t end , C less )
{
 ADS_PRE( ( start <= end ) && ( end < A.size() ) );
 std::size_t pos = start;
 for( std::size_t j = start + 1 ; j <= end ; ++j )
  if( less( A[ j ] , A[ pos ] ) )
   pos = j;
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( std::none_of( A.begin() + start , A.begin() + end + 1 ,
                        [ & ]( const T & a ) {
                         return( less( a , A[ pos ] ) ); } ) ); } ) );
 return( pos );
 }

template< class T , class C >
const T & min( const std::vector< T > & A , C less )
{
 ADS_PRE( ! A.empty() );
 return( A[ argmin( A , 0 , A.size() - 1 , less ) ] );
 }

namespace detail {

/// the minimum and the maximum of A[ i .. j ] by halves
template< class T , class C >
std::pair< T , T > minMaxRec( const std::vector< T > & A , std::size_t i ,
                              std::size_t j , C & less )
{
 if( i == j )
  return( std::pair( A[ i ] , A[ i ] ) );
 if( j == i + 1 )
  return( less( A[ j ] , A[ i ] ) ? std::pair( A[ j ] , A[ i ] )
                                  : std::pair( A[ i ] , A[ j ] ) );
 const std::size_t m = i + ( j - i ) / 2;
 const auto [ a , b ] = minMaxRec( A , i , m , less );
 const auto [ c , d ] = minMaxRec( A , m + 1 , j , less );
 return( std::pair( less( c , a ) ? c : a , less( b , d ) ? d : b ) );
 }

/// the pair is the minimum and the maximum of A
template< class T , class C >
bool isMinMax( const std::vector< T > & A , const std::pair< T , T > & r ,
               C & less )
{
 return( contract( [ & ]( void ) {
  const auto [ lo , hi ] = std::minmax_element( A.begin() , A.end() , less );
  return( ! less( r.first , *lo ) && ! less( *lo , r.first ) &&
          ! less( r.second , *hi ) && ! less( *hi , r.second ) );
  } ) );
 }

}  // end( namespace detail )

template< class T , class C >
std::pair< T , T > minMax( const std::vector< T > & A , C less )
{
 ADS_PRE( ! A.empty() );
 const std::size_t n = A.size();
 T lo = A[ 0 ] , hi = A[ 0 ];
 std::size_t i = 1;
 if( n % 2 == 0 ) {                          // the first pair
  if( less( A[ 1 ] , A[ 0 ] ) ) lo = A[ 1 ]; else hi = A[ 1 ];
  i = 2;
  }
 for( ; i + 1 < n ; i += 2 ) {
  const bool s = less( A[ i + 1 ] , A[ i ] );
  const T & a = s ? A[ i + 1 ] : A[ i ] , & b = s ? A[ i ] : A[ i + 1 ];
  if( less( a , lo ) ) lo = a;
  if( less( hi , b ) ) hi = b;
  }
 const std::pair< T , T > r( lo , hi );
 ADS_AUDIT( detail::isMinMax( A , r , less ) );
 return( r );
 }

template< class T , class C >
std::pair< T , T > minMaxDivide( const std::vector< T > & A , C less )
{
 ADS_PRE( ! A.empty() );
 const auto r = detail::minMaxRec( A , 0 , A.size() - 1 , less );
 ADS_AUDIT( detail::isMinMax( A , r , less ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Minimum.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
