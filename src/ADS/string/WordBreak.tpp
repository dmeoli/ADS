/*--------------------------------------------------------------------------*/
/*--------------------------- File WordBreak.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of WordBreak [see WordBreak.h], with their contracts.
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

template< class F >
std::optional< std::vector< std::string > > wordBreak(
                         const std::string & s , F && lookup ,
                         std::size_t longest ,
                         long * lookups )
{
 const std::size_t n = s.size();
 std::vector< long > from( n + 1 , -1 );           // the start of the word
 from[ 0 ] = 0;
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( std::size_t j = i ; j-- > 0 ; ) {
   if( i - j > longest )
    break;
   if( from[ j ] < 0 )
    continue;
   if( lookups ) ++*lookups;
   if( lookup( s.substr( j , i - j ) ) ) {
    from[ i ] = long( j );
    break;
    }
   }
 if( from[ n ] < 0 )
  return( std::nullopt );
 std::vector< std::string > words;
 for( std::size_t i = n ; i > 0 ; i = std::size_t( from[ i ] ) )
  words.insert( words.begin() , s.substr( from[ i ] , i - from[ i ] ) );
 ADS_POST( std::accumulate( words.begin() , words.end() , std::string() )
           == s );
 return( words );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File WordBreak.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
