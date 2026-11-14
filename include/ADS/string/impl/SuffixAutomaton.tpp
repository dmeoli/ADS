/*--------------------------------------------------------------------------*/
/*------------------------ File SuffixAutomaton.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuffixAutomaton [see SuffixAutomaton.h], with their
 * contracts.
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

#include <algorithm>

namespace ADS {

inline SuffixAutomaton::SuffixAutomaton( void )
 : f_len( 1 , 0 ) , f_link( 1 , none ) , f_next( 1 ) , f_prefix( 1 , 0 ) {}

inline SuffixAutomaton::SuffixAutomaton( std::string_view S )
 : SuffixAutomaton()
{
 f_len.reserve( 2 * S.size() + 1 );
 for( unsigned char c : S ) extend( c );
 }

inline std::uint32_t SuffixAutomaton::next( std::uint32_t v ,
                                            unsigned char c ) const
{
 for( const auto & [ b , w ] : f_next[ v ] )
  if( b == c ) return( w );
 return( none );
 }

inline void SuffixAutomaton::extend( unsigned char c )
{
 f_count.clear();
 const std::uint32_t cur = std::uint32_t( f_len.size() );
 f_len.push_back( f_len[ f_last ] + 1 );
 f_link.push_back( 0 );
 f_next.emplace_back();
 f_prefix.push_back( 1 );
 std::uint32_t p = f_last;
 for( ; ( p != none ) && ( next( p , c ) == none ) ; p = f_link[ p ] )
  f_next[ p ].emplace_back( c , cur );
 if( p != none ) {
  const std::uint32_t q = next( p , c );
  if( f_len[ q ] == f_len[ p ] + 1 ) f_link[ cur ] = q;
  else {                              // q cloned at the length of p + 1
   const std::uint32_t r = std::uint32_t( f_len.size() );
   f_len.push_back( f_len[ p ] + 1 );
   f_link.push_back( f_link[ q ] );
   f_next.push_back( f_next[ q ] );
   f_prefix.push_back( 0 );
   for( ; ( p != none ) && ( next( p , c ) == q ) ; p = f_link[ p ] )
    for( auto & e : f_next[ p ] )
     if( e.first == c ) e.second = r;
   f_link[ q ] = f_link[ cur ] = r;
   }
  }
 f_last = cur;
 ++f_n;
 ADS_INVARIANT( f_len.size() <= 2 * f_n );
 }

inline bool SuffixAutomaton::contains( std::string_view P ) const
{
 std::uint32_t v = 0;
 for( unsigned char c : P )
  if( ( v = next( v , c ) ) == none ) return( false );
 return( true );
 }

inline void SuffixAutomaton::counts( void ) const
{
 if( ! f_count.empty() ) return;
 const std::size_t S = f_len.size();
 // the states by decreasing length, by a counting sort on the lengths
 std::vector< std::size_t > by( f_n + 2 , 0 ) , order( S );
 for( std::size_t v = 0 ; v < S ; ++v ) ++by[ f_len[ v ] + 1 ];
 for( std::size_t l = 1 ; l < by.size() ; ++l ) by[ l ] += by[ l - 1 ];
 for( std::size_t v = 0 ; v < S ; ++v ) order[ by[ f_len[ v ] ]++ ] = v;
 f_count.assign( S , 0 );
 for( std::size_t v = 0 ; v < S ; ++v ) f_count[ v ] = f_prefix[ v ];
 for( std::size_t k = S ; k-- > 1 ; ) {
  const std::size_t v = order[ k ];
  if( f_link[ v ] != none ) f_count[ f_link[ v ] ] += f_count[ v ];
  }
 }

inline std::size_t SuffixAutomaton::occurrences( std::string_view P ) const
{
 if( P.empty() ) return( f_n + 1 );
 std::uint32_t v = 0;
 for( unsigned char c : P )
  if( ( v = next( v , c ) ) == none ) return( 0 );
 counts();
 return( f_count[ v ] );
 }

inline std::size_t SuffixAutomaton::distinctSubstrings( void ) const
{
 std::size_t d = 0;
 for( std::size_t v = 1 ; v < f_len.size() ; ++v )
  d += f_len[ v ] - f_len[ f_link[ v ] ];
 return( d );
 }

inline std::pair< std::size_t , std::size_t >
 SuffixAutomaton::longestCommonSubstring( std::string_view T ) const
{
 std::uint32_t v = 0;
 std::size_t l = 0 , best = 0 , end = 0;
 for( std::size_t j = 0 ; j < T.size() ; ++j ) {
  const unsigned char c = static_cast< unsigned char >( T[ j ] );
  while( ( v != 0 ) && ( next( v , c ) == none ) ) {
   v = f_link[ v ];                   // a shorter suffix of the match
   l = f_len[ v ];
   }
  const std::uint32_t w = next( v , c );
  if( w != none ) {
   v = w;
   ++l;
   }
  if( l > best ) {
   best = l;
   end = j + 1;
   }
  }
 return( std::pair( best , end ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SuffixAutomaton.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
