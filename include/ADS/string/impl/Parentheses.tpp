/*--------------------------------------------------------------------------*/
/*-------------------------- File Parentheses.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Parentheses [see Parentheses.h], with their contracts.
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

/// whether s is a balanced string of the parentheses open and close
inline bool isBalanced( std::string_view s , char open , char close )
{
 std::size_t depth = 0;
 for( char c : s )
  if( c == open )
   ++depth;
  else if( ( c != close ) || ( depth-- == 0 ) )
   return( false );
 return( depth == 0 );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

inline std::size_t longestBalancedSubsequence( std::string_view s ,
                                               char open , char close )
{
 ADS_PRE( open != close );
 std::size_t pairs = 0 , pending = 0;
 for( char c : s )
  if( c == open )
   ++pending;
  else if( ( c == close ) && ( pending > 0 ) ) {
   --pending;
   ++pairs;
   }
 return( 2 * pairs );
 }

/*--------------------------------------------------------------------------*/

inline std::pair< std::size_t , std::size_t > longestBalancedSubstring(
                                std::string_view s , char open , char close )
{
 ADS_PRE( open != close );
 std::size_t bestStart = 0 , bestLength = 0;
 std::size_t base = 0;              // where the current balanced run starts
 std::vector< std::size_t > opens;  // the opens not matched yet
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  if( s[ i ] == open )
   opens.push_back( i );
  else if( ( s[ i ] == close ) && ! opens.empty() ) {
   opens.pop_back();
   const std::size_t start = opens.empty() ? base : opens.back() + 1;
   if( i + 1 - start > bestLength ) {
    bestStart = start;
    bestLength = i + 1 - start;
    }
   }
  else {                            // an unmatched close or another char
   opens.clear();
   base = i + 1;
   }
 ADS_POST( detail::isBalanced( s.substr( bestStart , bestLength ) , open ,
                               close ) );
 return( std::pair( bestStart , bestLength ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Parentheses.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
