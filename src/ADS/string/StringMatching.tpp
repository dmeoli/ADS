/*--------------------------------------------------------------------------*/
/*------------------------ File StringMatching.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of StringMatching [see StringMatching.h], with their
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

inline std::vector< std::size_t > naiveMatching( std::string_view T ,
                                                 std::string_view P )
{
 std::vector< std::size_t > R;
 for( std::size_t s = 0 ; s + P.size() <= T.size() ; ++s )
  if( T.compare( s , P.size() , P ) == 0 ) R.push_back( s );
 return( R );
 }

inline std::vector< long > failure( std::string_view P )
{
 std::vector< long > F( P.size() + 1 );
 F[ 0 ] = -1;
 long k = -1;
 for( std::size_t q = 0 ; q < P.size() ; ++q ) {
  while( ( k >= 0 ) && ( P[ k ] != P[ q ] ) ) k = F[ k ];
  F[ q + 1 ] = ++k;
  ADS_POST( F[ q + 1 ] <= long( q ) );    // a proper border
  }
 return( F );
 }

namespace detail {

/// the search of KMP, G on a mismatch and F[ m ] after an occurrence
inline std::vector< std::size_t > kmpSearch( std::string_view T ,
                                             std::string_view P ,
                                             const std::vector< long > & G ,
                                             const std::vector< long > & F ,
                                             long * comparisons )
{
 std::vector< std::size_t > R;
 const long m = long( P.size() );
 long q = 0;                            // the characters of P matched
 if( m == 0 ) {
  for( std::size_t i = 0 ; i <= T.size() ; ++i ) R.push_back( i );
  return( R );
  }
 for( std::size_t i = 0 ; i < T.size() ; ++i ) {
  for( ; ; ) {
   if( comparisons ) ++*comparisons;
   if( P[ q ] == T[ i ] ) break;
   q = G[ q ];
   if( q < 0 ) break;
   }
  if( ++q == m ) {
   R.push_back( i + 1 - m );
   q = F[ m ];
   }
  }
 ADS_AUDIT( R == naiveMatching( T , P ) );
 return( R );
 }

}  // end( namespace detail )

inline std::vector< std::size_t > kmp( std::string_view T ,
                                       std::string_view P ,
                                       long * comparisons )
{
 const auto F = failure( P );
 return( detail::kmpSearch( T , P , F , F , comparisons ) );
 }

inline std::vector< long > strongFailure( std::string_view P )
{
 const auto F = failure( P );
 const std::size_t m = P.size();
 std::vector< long > G( m + 1 );
 G[ 0 ] = -1;
 for( std::size_t q = 1 ; q <= m ; ++q ) {
  const long k = F[ q ];
  G[ q ] = ( q < m ) && ( k >= 0 ) && ( P[ k ] == P[ q ] ) ? G[ k ] : k;
  ADS_POST( G[ q ] <= F[ q ] );
  }
 return( G );
 }

inline std::vector< std::size_t > kmpStrong( std::string_view T ,
                                             std::string_view P ,
                                             long * comparisons )
{
 return( detail::kmpSearch( T , P , strongFailure( P ) , failure( P ) ,
                            comparisons ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File StringMatching.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
