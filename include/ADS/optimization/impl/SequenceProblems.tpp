/*--------------------------------------------------------------------------*/
/*----------------------- File SequenceProblems.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SequenceProblems [see SequenceProblems.h], with their
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

template< class T >
T trappingRainWater( const std::vector< T > & h )
{
 T water = T( 0 );
 if( h.size() < 3 )
  return( water );
 std::size_t i = 0 , j = h.size() - 1;
 T left = h[ i ] , right = h[ j ];              // the maxima of each side
 while( i < j )
  if( left < right ) {                          // the level of i is left
   ++i;
   if( left < h[ i ] )
    left = h[ i ];
   else
    water += left - h[ i ];
   }
  else {
   --j;
   if( right < h[ j ] )
    right = h[ j ];
   else
    water += right - h[ j ];
   }
 ADS_POST( ! ( water < T( 0 ) ) );
 return( water );
 }

/*--------------------------------------------------------------------------*/

template< std::integral T >
std::vector< T > largestDivisibleSubset( std::vector< T > A )
{
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( T a ) { return( a > T( 0 ) ); } ) );
 std::sort( A.begin() , A.end() );
 ADS_PRE( std::adjacent_find( A.begin() , A.end() ) == A.end() );
 const std::size_t n = A.size();
 std::vector< std::size_t > L( n , 1 ) , pred( n , n );
 std::size_t best = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  for( std::size_t j = 0 ; j < i ; ++j )
   if( ( A[ i ] % A[ j ] == 0 ) && ( L[ j ] + 1 > L[ i ] ) ) {
    L[ i ] = L[ j ] + 1;
    pred[ i ] = j;
    }
  if( L[ i ] > L[ best ] )
   best = i;
  }
 std::vector< T > out;
 for( std::size_t i = best ; ( n > 0 ) && ( i != n ) ; i = pred[ i ] )
  out.push_back( A[ i ] );
 std::reverse( out.begin() , out.end() );
 ADS_POST( std::adjacent_find( out.begin() , out.end() ,
                               []( T a , T b ) { return( b % a != 0 ); } )
           == out.end() );
 return( out );
 }

/*--------------------------------------------------------------------------*/

template< class T >
T maxSumIncreasingSubsequence( const std::vector< T > & A )
{
 const std::size_t n = A.size();
 std::vector< T > V( A );                       // the ranks of the values
 std::sort( V.begin() , V.end() );
 V.erase( std::unique( V.begin() , V.end() ) , V.end() );
 std::vector< T > F( V.size() + 1 , T( 0 ) );   // Fenwick of the maxima
 T best = T( 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t r = std::size_t( std::lower_bound( V.begin() , V.end() ,
                                                       A[ i ] ) - V.begin() );
  T below = T( 0 );                             // max of the ranks < r
  for( std::size_t x = r ; x > 0 ; x -= x & ( ~x + 1 ) )
   below = std::max( below , F[ x ] );
  const T d = std::max( below + A[ i ] , T( 0 ) );
  best = std::max( best , d );
  for( std::size_t x = r + 1 ; x <= V.size() ; x += x & ( ~x + 1 ) )
   F[ x ] = std::max( F[ x ] , d );
  }
 ADS_POST( ! ( best < T( 0 ) ) );
 return( best );
 }

/*--------------------------------------------------------------------------*/

template< class T , class Less >
std::vector< std::size_t > longestChainSubsequence(
                                    const std::vector< T > & S , Less less )
{
 const std::size_t n = S.size();
 std::vector< std::size_t > L( n , 1 ) , pred( n , n );
 std::size_t best = 0;
 for( std::size_t j = 0 ; j < n ; ++j ) {
  for( std::size_t i = 0 ; i < j ; ++i )
   if( ( L[ i ] + 1 > L[ j ] ) && less( S[ i ] , S[ j ] ) ) {
    L[ j ] = L[ i ] + 1;
    pred[ j ] = i;
    }
  if( L[ j ] > L[ best ] )
   best = j;
  }
 std::vector< std::size_t > chain;
 for( std::size_t j = best ; ( n > 0 ) && ( j != n ) ; j = pred[ j ] )
  chain.push_back( j );
 std::reverse( chain.begin() , chain.end() );
 ADS_POST( std::adjacent_find( chain.begin() , chain.end() ,
            [ & ]( std::size_t a , std::size_t b ) {
             return( ( a >= b ) || ! less( S[ a ] , S[ b ] ) );
             } ) == chain.end() );
 return( chain );
 }

/*--------------------------------------------------------------------------*/

template< class T , class Hash >
std::pair< std::size_t , std::size_t > longestDistinctSegment(
                                                   const std::vector< T > & A )
{
 std::unordered_map< T , std::size_t , Hash > last;
 std::size_t start = 0 , bestStart = 0 , bestLength = 0;
 for( std::size_t end = 0 ; end < A.size() ; ++end ) {
  const auto it = last.find( A[ end ] );
  if( ( it != last.end() ) && ( it->second >= start ) )
   start = it->second + 1;
  last[ A[ end ] ] = end;
  if( end + 1 - start > bestLength ) {
   bestStart = start;
   bestLength = end + 1 - start;
   }
  }
 ADS_AUDIT( ( [ & ] {
              std::unordered_map< T , std::size_t , Hash > seen;
              for( std::size_t i = bestStart ; i < bestStart + bestLength ;
                   ++i )
               if( ! seen.emplace( A[ i ] , i ).second )
                return( false );
              return( true );
              }() ) );
 return( std::pair( bestStart , bestLength ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File SequenceProblems.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
