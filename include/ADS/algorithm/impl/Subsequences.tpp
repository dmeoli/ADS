/*--------------------------------------------------------------------------*/
/*------------------------- File Subsequences.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Subsequences.h [see Subsequences.h].
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

/// S is strictly increasing and a subsequence of A
template< class T >
bool increasingSubsequence( const std::vector< T > & S ,
                            const std::vector< T > & A )
{
 std::size_t j = 0;
 for( std::size_t i = 0 ; i < S.size() ; ++i ) {
  if( ( i > 0 ) && !( S[ i - 1 ] < S[ i ] ) )
   return( false );
  while( ( j < A.size() ) && !( A[ j ] == S[ i ] ) ) ++j;
  if( j++ == A.size() )
   return( false );
  }
 return( true );
 }

}  // end( namespace detail )

template< class T >
std::vector< T > lis( const std::vector< T > & A )
{
 const std::size_t n = A.size();
 std::vector< std::size_t > L( n , 1 ) , prev( n , n );
 std::size_t best = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  for( std::size_t j = 0 ; j < i ; ++j )
   if( ( A[ j ] < A[ i ] ) && ( L[ j ] + 1 > L[ i ] ) ) {
    L[ i ] = L[ j ] + 1;
    prev[ i ] = j;
    }
  if( L[ i ] > L[ best ] )
   best = i;
  }
 std::vector< T > out;
 for( std::size_t i = n ? best : n ; i < n ; i = prev[ i ] )
  out.push_back( A[ i ] );
 std::reverse( out.begin() , out.end() );
 ADS_AUDIT( detail::increasingSubsequence( out , A ) &&
            ( out.size() == lisLength( A ) ) );
 return( out );
 }

template< class T >
std::size_t lisLength( const std::vector< T > & A )
{
 std::vector< T > tails;                 // tails[ l ]: the least end of l + 1
 for( const T & a : A ) {
  auto it = std::lower_bound( tails.begin() , tails.end() , a );
  if( it == tails.end() )
   tails.push_back( a );
  else
   *it = a;
  }
 return( tails.size() );
 }

template< class T >
T maxSubarraySum( const std::vector< T > & A )
{
 T best = T( 0 ) , t = T( 0 );
 for( const T & a : A ) {
  t = std::max( t + a , T( 0 ) );
  best = std::max( best , t );
  }
 ADS_POST( !( best < T( 0 ) ) );
 return( best );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File Subsequences.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
