/*--------------------------------------------------------------------------*/
/*--------------------------- File Anagrams.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Anagrams [see Anagrams.h], with their contracts.
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

/// whether the groups partition the words and each holds anagrams
template< class S >
bool anagramClasses( const std::vector< std::vector< S > > & groups ,
                     const std::vector< S > & words )
{
 std::size_t total = 0;
 for( const auto & g : groups ) {
  if( g.empty() )
   return( false );
  for( const auto & w : g )
   if( ! std::is_permutation( w.begin() , w.end() , g.front().begin() ,
                              g.front().end() ) )
    return( false );
  total += g.size();
  }
 return( total == words.size() );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class S , class Hash >
std::vector< std::vector< S > > anagramGroups( const std::vector< S > & words )
{
 std::unordered_map< S , std::size_t , Hash > group;   // signature -> group
 std::vector< std::vector< S > > groups;
 for( const auto & w : words ) {
  S key( w.begin() , w.end() );
  std::sort( key.begin() , key.end() );
  const auto [ it , fresh ] = group.try_emplace( std::move( key ) ,
                                                 groups.size() );
  if( fresh )
   groups.emplace_back();
  groups[ it->second ].push_back( w );
  }
 ADS_AUDIT( detail::anagramClasses( groups , words ) );
 return( groups );
 }

/*--------------------------------------------------------------------------*/

inline std::vector< std::vector< std::string > > anagramGroups(
                                    const std::vector< std::string > & words )
{
 return( anagramGroups< std::string >( words ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Anagrams.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
