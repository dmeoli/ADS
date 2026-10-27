/*--------------------------------------------------------------------------*/
/*------------------------- File PrefixSearch.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of PrefixSearch.h, with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

inline bool precedes( std::string_view s , std::string_view P , bool upper )
{
 return( ( s < P ) || ( upper && s.starts_with( P ) ) );
 }

inline std::uint64_t prefixKey( std::string_view s )
{
 std::uint64_t k = 0;
 for( std::size_t i = 0 ; i < 8 ; ++i )
  k = ( k << 8 ) | ( i < s.size() ? ( unsigned char )( s[ i ] ) : 0u );
 return( k );
 }

/// the integers of the first 8 characters of the strings of A
inline std::vector< std::uint64_t > prefixKeys(
                                       const std::vector< std::string > & A )
{
 std::vector< std::uint64_t > K( A.size() );
 for( std::size_t i = 0 ; i < A.size() ; ++i ) K[ i ] = prefixKey( A[ i ] );
 return( K );
 }

}  // end( namespace detail )

inline std::size_t commonPrefixLength( std::string_view a ,
                                       std::string_view b )
{
 const std::size_t m = std::min( a.size() , b.size() );
 std::size_t l = 0;
 while( ( l < m ) && ( a[ l ] == b[ l ] ) ) ++l;
 return( l );
 }

inline std::size_t lexicographicPosition(
                                       const std::vector< std::string > & A ,
                                       std::string_view Q )
{
 return( lowerBound( A , std::string( Q ) ) );
 }

inline std::size_t prefixEnd( const std::vector< std::string > & A ,
                              std::string_view P )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 std::size_t lo = 0 , hi = A.size();
 while( lo < hi ) {                         // the binary search of P#
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( detail::precedes( A[ m ] , P , true ) )
   lo = m + 1;
  else
   hi = m;
  }
 ADS_POST( ( ( lo == 0 ) || detail::precedes( A[ lo - 1 ] , P , true ) ) &&
           ( ( lo == A.size() ) ||
             ! detail::precedes( A[ lo ] , P , true ) ) );
 return( lo );
 }

inline std::pair< std::size_t , std::size_t > prefixRange(
                                       const std::vector< std::string > & A ,
                                       std::string_view P )
{
 return( std::pair( lexicographicPosition( A , P ) , prefixEnd( A , P ) ) );
 }

/*--------------------------------------------------------------------------*/

inline BinnedSearch::BinnedSearch( std::vector< std::uint64_t > X )
 : f_X( std::move( X ) )
{
 ADS_AUDIT( std::is_sorted( f_X.begin() , f_X.end() ) );
 const std::size_t m = f_X.size();
 if( m == 0 ) {
  f_start.assign( 1 , 0 );
  return;
  }
 const std::uint64_t x1 = f_X.front() , range = f_X.back() - x1;
 f_b = range / m + 1;                       // ceil( ( range + 1 ) / m )
 const std::size_t nb = std::size_t( range / f_b ) + 1;
 f_start.assign( nb + 1 , m );
 std::size_t i = 0;
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  const std::uint64_t lo = x1 + std::uint64_t( j ) * f_b;   // at most x_m
  while( ( i < m ) && ( f_X[ i ] < lo ) ) ++i;
  f_start[ j ] = i;
  }
 }

inline std::optional< std::size_t > BinnedSearch::binOf(
                                                  std::uint64_t y ) const
{
 if( f_X.empty() || ( y < f_X.front() ) || ( y > f_X.back() ) )
  return( std::nullopt );
 return( std::size_t( ( y - f_X.front() ) / f_b ) );
 }

inline std::size_t BinnedSearch::position( std::uint64_t y ) const
{
 std::size_t r;
 if( f_X.empty() || ( y <= f_X.front() ) )
  r = 0;
 else if( y > f_X.back() )
  r = f_X.size();
 else {
  const std::size_t j = *binOf( y );        // a binary search in its bin
  r = lowerBound( f_X , y , f_start[ j ] , f_start[ j + 1 ] );
  }
 ADS_POST( ( ( r == 0 ) || ( f_X[ r - 1 ] < y ) ) &&
           ( ( r == f_X.size() ) || ( y <= f_X[ r ] ) ) );
 return( r );
 }

inline bool BinnedSearch::contains( std::uint64_t y ) const
{
 const std::size_t r = position( y );
 return( ( r < f_X.size() ) && ( f_X[ r ] == y ) );
 }

inline std::size_t BinnedSearch::bins( void ) const
{
 return( f_start.size() - 1 );
 }

inline std::uint64_t BinnedSearch::width( void ) const
{
 return( f_b );
 }

inline std::pair< std::size_t , std::size_t > BinnedSearch::bin(
                                                       std::size_t j ) const
{
 ADS_PRE( j < bins() );
 return( std::pair( f_start[ j ] , f_start[ j + 1 ] ) );
 }

inline std::size_t BinnedSearch::largestBin( void ) const
{
 std::size_t l = 0;
 for( std::size_t j = 0 ; j < bins() ; ++j )
  l = std::max( l , f_start[ j + 1 ] - f_start[ j ] );
 return( l );
 }

inline std::size_t BinnedSearch::size( void ) const
{
 return( f_X.size() );
 }

inline const std::vector< std::uint64_t > & BinnedSearch::items( void ) const
{
 return( f_X );
 }

/*--------------------------------------------------------------------------*/

inline BinnedStringSearch::BinnedStringSearch( std::vector< std::string > A )
 : f_A( std::move( A ) ) , f_K( detail::prefixKeys( f_A ) )
{
 ADS_AUDIT( std::is_sorted( f_A.begin() , f_A.end() ) );
 }

inline std::size_t BinnedStringSearch::rank( std::string_view P , bool upper ,
                                             std::uint64_t k ) const
{
 const auto & X = f_K.items();
 if( X.empty() || ( k < X.front() ) )       // all the strings follow
  return( 0 );
 if( k > X.back() )                         // all the strings precede
  return( X.size() );
 auto [ lo , hi ] = f_K.bin( *f_K.binOf( k ) );
 while( lo < hi ) {                         // a binary search in the bin
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( detail::precedes( f_A[ m ] , P , upper ) )
   lo = m + 1;
  else
   hi = m;
  }
 return( lo );
 }

inline std::size_t BinnedStringSearch::lexicographicPosition(
                                                  std::string_view Q ) const
{
 const std::size_t r = rank( Q , false , detail::prefixKey( Q ) );
 ADS_POST( ( ( r == 0 ) || ( f_A[ r - 1 ] < Q ) ) &&
           ( ( r == f_A.size() ) || ( Q <= f_A[ r ] ) ) );
 return( r );
 }

inline std::size_t BinnedStringSearch::prefixEnd( std::string_view P ) const
{
 std::uint64_t k = detail::prefixKey( P );  // the largest key prefixed by P
 if( P.empty() )
  k = ~std::uint64_t( 0 );
 else if( P.size() < 8 )
  k |= ( std::uint64_t( 1 ) << ( 8 * ( 8 - P.size() ) ) ) - 1;
 const std::size_t r = rank( P , true , k );
 ADS_POST( ( ( r == 0 ) || detail::precedes( f_A[ r - 1 ] , P , true ) ) &&
           ( ( r == f_A.size() ) ||
             ! detail::precedes( f_A[ r ] , P , true ) ) );
 return( r );
 }

inline std::pair< std::size_t , std::size_t > BinnedStringSearch::prefixRange(
                                                  std::string_view P ) const
{
 return( std::pair( lexicographicPosition( P ) , prefixEnd( P ) ) );
 }

inline const BinnedSearch & BinnedStringSearch::keys( void ) const
{
 return( f_K );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PrefixSearch.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
