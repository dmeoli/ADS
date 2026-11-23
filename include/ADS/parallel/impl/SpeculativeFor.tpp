/*--------------------------------------------------------------------------*/
/*------------------------ File SpeculativeFor.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The rounds of speculativeFor() [see SpeculativeFor.h].
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

namespace ADS
{

namespace detail
{

/// the iterations of \f$ I[ 0 .. k - 1 ] \f$ whose flag in `keep` is set,
/// in their order, copied into `out`, and their number, by the pack of
/// `ParallelPrimitives.h`
template< class E >
std::size_t packKept( E & ex , const std::vector< std::size_t > & I ,
		      const std::vector< char > & keep , std::size_t k ,
		      std::vector< std::size_t > & out )
{
 return( packInto(
   ex , k , [ & ]( std::size_t i ) { return( keep[ i ] != 0 ); } ,
   [ & ]( std::size_t i ) { return( I[ i ] ); } ,
   [ & ]( std::size_t ) { return( out.begin() ); } ) );
 }

}  // end( namespace detail )

template< Executor E , SpeculativeStep S >
void speculativeFor( E & ex , S & step , std::size_t s , std::size_t e ,
		     std::size_t g , SpeculativeCounts * c )
{
 ADS_PRE( ( s <= e ) && ( g >= 1 ) );
 if( s >= e )
  return;
 const std::size_t rmax = ( e - s ) / std::max< std::size_t >( g , 1 ) + 1;
 std::size_t r = rmax;
 std::vector< std::size_t > I( rmax ) , J( rmax );
 std::vector< char > keep( rmax );
 std::size_t done = s , kept = 0;
 // a round of fewer iterations than this runs on the calling thread
 constexpr std::size_t kGrainRound = 256;
 while( done < e ) {
  const std::size_t size = std::min( r , e - done );
  if( c ) {
   ++c->rounds;
   c->tried += static_cast< long long >( size );
   }
  parallelFor(
    ex , 0 , size ,
    [ & ]( std::size_t i ) {
     if( i >= kept )
      I[ i ] = done + i;
     keep[ i ] = step.reserve( I[ i ] ) ? 1 : 0;
     } ,
    kGrainRound );
  parallelFor(
    ex , 0 , size ,
    [ & ]( std::size_t i ) {
     if( keep[ i ] )
      keep[ i ] = step.commit( I[ i ] ) ? 0 : 1;
     } ,
    kGrainRound );
  kept = detail::packKept( ex , I , keep , size , J );
  ADS_PRE( kept < size );  // the least iteration commits
  if( kept >= size )
   return;
  std::swap( I , J );
  done += size - kept;
  if( double( kept ) > 0.2 * double( size ) )
   r = std::max( { r / 2 , rmax / 64 + 1 , kept } );
  else if( double( kept ) < 0.1 * double( size ) )
   r = std::min( 2 * r , rmax );
  }
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SpeculativeFor.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
