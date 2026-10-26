/*--------------------------------------------------------------------------*/
/*--------------------------- File Anagrams.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Anagrams [see Anagrams.h], with their contracts.
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

template< class W , class P >
std::set< std::string > anagrams( const std::string & s , W && word ,
                                  P && prefix )
{
 std::map< char , int > left;
 for( char c : s ) ++left[ c ];
 std::set< std::string > out;
 std::string sentence , current;
 auto rec = [ & ]( auto & self , std::size_t placed ) -> void {
  if( placed == s.size() ) {
   if( current.empty() )
    out.insert( sentence.substr( 0 , sentence.size() - 1 ) );
   return;
   }
  for( auto & [ c , k ] : left ) {
   if( k == 0 ) continue;
   --k;
   current += c;
   if( prefix( current ) ) {
    self( self , placed + 1 );                   // the word goes on
    if( word( current ) ) {                      // or it ends here
     const std::string w = current , before = sentence;
     sentence += w + ' ';
     current.clear();
     self( self , placed + 1 );
     current = w;
     sentence = before;
     }
    }
   current.pop_back();
   ++k;
   }
  };
 rec( rec , 0 );
 ADS_AUDIT( std::all_of( out.begin() , out.end() , [ & ]( std::string t ) {
  std::erase( t , ' ' );
  std::string u = s;
  std::sort( t.begin() , t.end() );
  std::sort( u.begin() , u.end() );
  return( t == u ); } ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Anagrams.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
