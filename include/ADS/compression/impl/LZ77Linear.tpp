/*--------------------------------------------------------------------------*/
/*-------------------------- File LZ77Linear.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The function of LZ77Linear.h, with its contracts.
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

inline std::vector< LZ77Phrase > lz77ParseLinear( std::string_view s )
{
 const std::size_t n = s.size();
 ADS_PRE( std::uint64_t( n ) < ( std::uint64_t( 1 ) << 32 ) - 1 );
 std::vector< LZ77Phrase > P;
 if( n == 0 ) return( P );
 constexpr std::uint32_t none = ~std::uint32_t( 0 );
 // PSV and NSV of each place, by a scan of SA with a stack of places
 // increasing from the bottom: a place pops the larger ones, of which it is
 // the next smaller value, and the top is its previous smaller value
 std::vector< std::uint32_t > psv( n , none ) , nsv( n , none ) , stack;
 {
 const auto sa = suffixArraySais( s );
 stack.reserve( 1024 );
 for( const std::size_t x : sa ) {
  while( ! stack.empty() && ( stack.back() > x ) ) {
   nsv[ stack.back() ] = std::uint32_t( x );
   stack.pop_back();
   }
  if( ! stack.empty() ) psv[ x ] = stack.back();
  stack.push_back( std::uint32_t( x ) );
  }
 }
 // the common prefix of the suffixes i and j < i, overlap included
 auto lcp = [ & ]( std::size_t i , std::uint32_t j ) -> std::size_t {
  if( j == none ) return( 0 );
  std::size_t l = 0;
  while( ( i + l < n ) && ( s[ j + l ] == s[ i + l ] ) ) ++l;
  return( l );
  };
 for( std::size_t i = 0 ; i < n ; ) {
  const std::size_t a = lcp( i , psv[ i ] ) , b = lcp( i , nsv[ i ] );
  const std::size_t l = std::max( a , b );
  const std::size_t d = l == 0 ? 0 : i - ( a >= b ? psv[ i ] : nsv[ i ] );
  std::optional< char > next;
  if( i + l < n ) next = s[ i + l ];
  P.push_back( LZ77Phrase{ d , l , next } );
  i += l + 1;
  }
 ADS_AUDIT( lz77Decode( P ) == s );
 return( P );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LZ77Linear.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
