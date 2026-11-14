/*--------------------------------------------------------------------------*/
/*----------------------- File QuicksortAdversary.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of QuicksortAdversary [see QuicksortAdversary.h], with
 * their contracts.
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

inline QuicksortAdversary::QuicksortAdversary( std::size_t n )
 : f_val( n , n ) , f_candidate( n )
{
 }

inline bool QuicksortAdversary::Less::operator()( std::size_t x ,
                                                   std::size_t y ) const
{
 auto & a = *f_a;
 const std::size_t gas = a.f_val.size();
 ADS_PRE( ( x < gas ) && ( y < gas ) );
 ++a.f_count;
 if( ( a.f_val[ x ] == gas ) && ( a.f_val[ y ] == gas ) )
  a.f_val[ x == a.f_candidate ? x : y ] = a.f_solid++;
 if( a.f_val[ x ] == gas )
  a.f_candidate = x;
 else if( a.f_val[ y ] == gas )
  a.f_candidate = y;
 return( a.f_val[ x ] < a.f_val[ y ] );
 }

inline std::vector< std::size_t > QuicksortAdversary::items( void ) const
{
 std::vector< std::size_t > I( f_val.size() );
 for( std::size_t i = 0 ; i < I.size() ; ++i )
  I[ i ] = i;
 return( I );
 }

inline std::vector< std::size_t > QuicksortAdversary::input( void ) const
{
 std::vector< std::size_t > V = f_val;
 std::size_t next = f_solid;
 for( auto & v : V )
  if( v == V.size() )
   v = next++;
 return( V );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File QuicksortAdversary.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
