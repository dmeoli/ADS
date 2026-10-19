/*--------------------------------------------------------------------------*/
/*---------------------------- File Lyndon.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Lyndon [see Lyndon.h], with their contracts.
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

#include <string>
#include <utility>

namespace ADS {

inline bool isLyndon( std::string_view S )
{
 if( S.empty() ) return( false );
 for( std::size_t i = 1 ; i < S.size() ; ++i )
  if( ! ( S < S.substr( i ) ) ) return( false );
 return( true );
 }

namespace detail {

/// the scan of Duval on S from i: the length of w and the end j of the
/// scan, s_2 = S[ i .. j ) = w^h w'
inline std::pair< std::size_t , std::size_t > duvalRun( std::string_view S ,
                                                        std::size_t i )
{
 std::size_t j = i + 1 , k = i;      // S[ k ] is |w| places before S[ j ]
 while( ( j < S.size() ) && ( S[ k ] <= S[ j ] ) ) {
  k = S[ k ] < S[ j ] ? i : k + 1;   // a longer Lyndon word, or w' grows
  ++j;
  }
 return( std::pair( j - k , j ) );
 }

}  // end( namespace detail )

inline std::vector< std::size_t > lyndonFactorization( std::string_view S )
{
 std::vector< std::size_t > F;
 for( std::size_t i = 0 ; i < S.size() ; ) {
  const auto [ w , j ] = detail::duvalRun( S , i );
  while( i + w <= j ) {              // the copies of w are factors
   F.push_back( i );
   i += w;
   }
  }
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t f = 0 ; f < F.size() ; ++f ) {
   const std::size_t e = f + 1 < F.size() ? F[ f + 1 ] : S.size();
   const std::string_view w = S.substr( F[ f ] , e - F[ f ] );
   if( ! isLyndon( w ) ) return( false );
   if( ( f > 0 ) && ( S.substr( F[ f - 1 ] , F[ f ] - F[ f - 1 ] ) < w ) )
    return( false );
   }
  return( F.empty() ? S.empty() : F[ 0 ] == 0 ); }() );
 return( F );
 }

inline std::size_t leastRotation( std::string_view S )
{
 const std::size_t n = S.size();
 if( n == 0 ) return( 0 );
 const std::string D = std::string( S ) + std::string( S );
 std::size_t r = 0;
 for( std::size_t i = 0 ; i < n ; ) {
  r = i;                              // the factor that covers n
  const auto [ w , j ] = detail::duvalRun( D , i );
  while( i + w <= j ) i += w;
  }
 ADS_AUDIT( [ & ]( void ) {
  const std::string_view best = std::string_view( D ).substr( r , n );
  for( std::size_t i = 0 ; i < n ; ++i )
   if( std::string_view( D ).substr( i , n ) < best ) return( false );
  return( true ); }() );
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Lyndon.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
