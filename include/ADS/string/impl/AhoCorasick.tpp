/*--------------------------------------------------------------------------*/
/*-------------------------- File AhoCorasick.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of AhoCorasick [see AhoCorasick.h], with their contracts.
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

#include "ADS/string/StringMatching.h"

#include <algorithm>
#include <limits>

namespace ADS {

namespace detail {

/// no transition of the trie yet
inline constexpr std::uint32_t acNone =
                                  std::numeric_limits< std::uint32_t >::max();

}  // end( namespace detail )

inline AhoCorasick::AhoCorasick( void )
{
 f_go.emplace_back();
 f_go.back().fill( detail::acNone );
 f_ends.emplace_back();
 f_built = false;
 build();
 }

inline AhoCorasick::AhoCorasick( const std::vector< std::string > & P )
 : AhoCorasick()
{
 for( const auto & p : P ) (void) add( p );
 build();
 }

inline std::size_t AhoCorasick::add( std::string_view P )
{
 ADS_PRE( ! P.empty() );
 if( f_built ) {                     // the trie again, from the patterns
  const std::vector< std::string > Q = std::move( f_P );
  f_P.clear();
  f_go.assign( 1 , {} );
  f_go[ 0 ].fill( detail::acNone );
  f_ends.assign( 1 , {} );
  f_built = false;
  for( const auto & q : Q ) (void) add( q );
  }
 std::uint32_t u = 0;
 for( unsigned char c : P ) {
  if( f_go[ u ][ c ] == detail::acNone ) {
   f_go[ u ][ c ] = std::uint32_t( f_go.size() );
   f_go.emplace_back();
   f_go.back().fill( detail::acNone );
   f_ends.emplace_back();
   }
  u = f_go[ u ][ c ];
  }
 f_ends[ u ].push_back( std::uint32_t( f_P.size() ) );
 f_P.emplace_back( P );
 return( f_P.size() - 1 );
 }

inline void AhoCorasick::build( void )
{
 if( f_built ) return;
 const std::size_t S = f_go.size();
 f_fail.assign( S , 0 );
 f_out.assign( S , 0 );
 // breadth first: the failure of a state comes before its children
 std::vector< std::uint32_t > queue;
 queue.reserve( S );
 for( unsigned c = 0 ; c < 256 ; ++c )
  if( f_go[ 0 ][ c ] == detail::acNone ) f_go[ 0 ][ c ] = 0;
  else queue.push_back( f_go[ 0 ][ c ] );
 for( std::size_t h = 0 ; h < queue.size() ; ++h ) {
  const std::uint32_t u = queue[ h ] , f = f_fail[ u ];
  f_out[ u ] = f_ends[ f ].empty() ? f_out[ f ] : f;
  for( unsigned c = 0 ; c < 256 ; ++c ) {
   const std::uint32_t v = f_go[ u ][ c ];
   if( v == detail::acNone ) f_go[ u ][ c ] = f_go[ f ][ c ];
   else {
    f_fail[ v ] = f_go[ f ][ c ];
    queue.push_back( v );
    }
   }
  }
 f_built = true;
 }

template< class F >
void AhoCorasick::scan( std::string_view T , F && report ) const
{
 ADS_PRE( f_built );
 std::uint32_t s = 0;
 for( std::size_t j = 0 ; j < T.size() ; ++j ) {
  s = f_go[ s ][ static_cast< unsigned char >( T[ j ] ) ];
  for( std::uint32_t q = f_ends[ s ].empty() ? f_out[ s ] : s ; q ;
       q = f_out[ q ] )
   for( const std::uint32_t id : f_ends[ q ] ) report( j , id );
  }
 }

inline std::vector< std::pair< std::size_t , std::size_t > >
 AhoCorasick::findAll( std::string_view T ) const
{
 std::vector< std::pair< std::size_t , std::size_t > > R;
 scan( T , [ & ]( std::size_t j , std::uint32_t id ) {
  R.emplace_back( j + 1 - f_P[ id ].size() , id ); } );
 ADS_AUDIT( [ & ]( void ) {
  std::vector< std::vector< std::size_t > > by( f_P.size() );
  for( const auto & [ s , id ] : R ) by[ id ].push_back( s );
  for( std::size_t id = 0 ; id < f_P.size() ; ++id ) {
   std::sort( by[ id ].begin() , by[ id ].end() );
   if( by[ id ] != naiveMatching( T , f_P[ id ] ) ) return( false );
   }
  return( true ); }() );
 return( R );
 }

inline std::size_t AhoCorasick::count( std::string_view T ) const
{
 std::size_t c = 0;
 scan( T , [ & ]( std::size_t , std::uint32_t ) { ++c; } );
 return( c );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File AhoCorasick.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
