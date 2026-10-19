/*--------------------------------------------------------------------------*/
/*------------------------- File RemUnionFind.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RemUnionFind [see RemUnionFind.h], with their contracts.
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

#include <numeric>

namespace ADS {

inline RemUnionFind::RemUnionFind( std::size_t n ) : f_p( n ) , f_count( n )
{
 ADS_PRE( n < ( std::size_t( 1 ) << 32 ) );
 std::iota( f_p.begin() , f_p.end() , std::uint32_t( 0 ) );
 }

inline std::size_t RemUnionFind::n( void ) const
{
 return( f_p.size() );
 }

inline std::size_t RemUnionFind::count( void ) const
{
 return( f_count );
 }

inline std::uint32_t RemUnionFind::parent( std::uint32_t x ) const
{
 ADS_PRE( x < f_p.size() );
 return( f_p[ x ] );
 }

inline std::uint32_t RemUnionFind::find( std::uint32_t x )
{
 ADS_PRE( x < f_p.size() );
 std::uint32_t * p = f_p.data();
 while( p[ x ] != x ) {                     // the halving of the path
  const std::uint32_t g = p[ p[ x ] ];
  p[ x ] = g;
  x = g;
  }
 ADS_POST( f_p[ x ] == x );
 return( x );
 }

inline bool RemUnionFind::merge( std::uint32_t x , std::uint32_t y )
{
 ADS_PRE( ( x < f_p.size() ) && ( y < f_p.size() ) );
 std::uint32_t * p = f_p.data();
 std::uint32_t a = x , b = y;
 bool joined = false;
 while( p[ a ] != p[ b ] ) {
  if( p[ a ] > p[ b ] )                     // a has the smaller parent
   std::swap( a , b );
  if( a == p[ a ] ) {                       // a root: under the other one
   p[ a ] = p[ b ];
   joined = true;
   break;
   }
  const std::uint32_t z = p[ a ];           // the splicing
  p[ a ] = p[ b ];
  a = z;
  }
 f_count -= joined ? 1 : 0;
 ADS_POST( find( x ) == find( y ) );
 ADS_INVARIANT( ( f_p.size() > 4096 ) || forest() );
 return( joined );
 }

inline bool RemUnionFind::same( std::uint32_t x , std::uint32_t y )
{
 ADS_PRE( ( x < f_p.size() ) && ( y < f_p.size() ) );
 return( find( x ) == find( y ) );
 }

inline bool RemUnionFind::forest( void ) const
{
 std::size_t roots = 0;
 for( std::size_t x = 0 ; x < f_p.size() ; ++x ) {
  if( f_p[ x ] < x )
   return( false );
  roots += f_p[ x ] == x ? 1 : 0;
  }
 return( roots == f_count );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RemUnionFind.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
