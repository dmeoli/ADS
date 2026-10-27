/*--------------------------------------------------------------------------*/
/*---------------------- File FractionalKnapsack.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FractionalKnapsack [see FractionalKnapsack.h], with their
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

namespace ADS {

inline std::vector< double > fractionalKnapsack(
                                            const std::vector< double > & p ,
                                            const std::vector< double > & v ,
                                            double C )
{
 ADS_PRE( ( p.size() == v.size() ) && ( C >= 0 ) );
 ADS_PRE( std::all_of( v.begin() , v.end() ,
                       []( double x ) { return( x > 0 ); } ) );
 ADS_PRE( std::all_of( p.begin() , p.end() ,
                       []( double x ) { return( x >= 0 ); } ) );
 const double C0 = C;
 const std::size_t n = p.size();
 std::vector< std::size_t > by( n );
 std::iota( by.begin() , by.end() , 0 );
 std::stable_sort( by.begin() , by.end() ,
                   [ & ]( std::size_t a , std::size_t b ) {
                    return( p[ a ] * v[ b ] > p[ b ] * v[ a ] ); } );
 std::vector< double > x( n , 0 );
 for( std::size_t i : by ) {
  if( C <= 0 )                              // full; C may round below 0
   break;
  x[ i ] = std::min( C / v[ i ] , 1.0 );
  C = std::max( C - x[ i ] * v[ i ] , 0.0 );
  }
 ADS_POST( std::all_of( x.begin() , x.end() , []( double y ) {
  return( ( y >= 0 ) && ( y <= 1 ) ); } ) );
 ADS_POST( std::inner_product( x.begin() , x.end() , v.begin() , 0.0 ) <=
           C0 * ( 1 + 1e-9 ) + 1e-9 );
 return( x );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File FractionalKnapsack.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
