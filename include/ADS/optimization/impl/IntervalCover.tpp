/*--------------------------------------------------------------------------*/
/*------------------------- File IntervalCover.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of IntervalCover [see IntervalCover.h], with their
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
std::optional< std::vector< std::size_t > > minimumIntervalCover(
                   const std::vector< std::pair< T , T > > & I , T lo , T hi )
{
 ADS_PRE( lo < hi );
 ADS_PRE( std::all_of( I.begin() , I.end() ,
                       []( const std::pair< T , T > & x ) {
                        return( ! ( x.second < x.first ) ); } ) );
 const std::size_t n = I.size();
 std::vector< std::size_t > order( n );
 std::iota( order.begin() , order.end() , std::size_t( 0 ) );
 std::stable_sort( order.begin() , order.end() ,
                   [ & ]( std::size_t a , std::size_t b ) {
                    return( I[ a ].first < I[ b ].first ); } );
 std::vector< std::size_t > out;
 T covered = lo;
 std::size_t i = 0;
 while( covered < hi ) {
  std::size_t pick = n;
  for( ; ( i < n ) && ! ( covered < I[ order[ i ] ].first ) ; ++i )
   if( ( pick == n ) || ( I[ pick ].second < I[ order[ i ] ].second ) )
    pick = order[ i ];
  if( ( pick == n ) || ! ( covered < I[ pick ].second ) )
   return( std::nullopt );
  covered = I[ pick ].second;
  out.push_back( pick );
  }
 ADS_AUDIT( [ & ] {
             T reach = lo;
             for( std::size_t k : out ) {
              if( reach < I[ k ].first )
               return( false );
              reach = std::max( reach , I[ k ].second );
              }
             return( ! ( reach < hi ) );
             }() );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File IntervalCover.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
