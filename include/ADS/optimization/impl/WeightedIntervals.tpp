/*--------------------------------------------------------------------------*/
/*----------------------- File WeightedIntervals.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of WeightedIntervals [see WeightedIntervals.h], with their
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

template< class W >
std::pair< std::vector< std::size_t > , W > maxinterval(
                              const std::vector< WeightedInterval< W > > & I )
{
 ADS_PRE( std::all_of( I.begin() , I.end() , []( const auto & x ) {
  return( ( x.a < x.b ) && !( x.w < W( 0 ) ) ); } ) );
 const std::size_t n = I.size();
 std::vector< std::size_t > by( n );                // by increasing b
 std::iota( by.begin() , by.end() , 0 );
 std::stable_sort( by.begin() , by.end() , [ & ]( std::size_t x ,
                   std::size_t y ) { return( I[ x ].b < I[ y ].b ); } );
 // the ends sorted, a right end before a left one equal to it
 using LL = long long;
 std::vector< std::pair< LL , LL > > E;            // ( end , +-( i + 1 ) )
 for( std::size_t i = 0 ; i < n ; ++i ) {
  E.emplace_back( I[ by[ i ] ].a , LL( i + 1 ) );
  E.emplace_back( I[ by[ i ] ].b , -LL( i + 1 ) );
  }
 std::sort( E.begin() , E.end() );
 std::vector< std::size_t > pred( n + 1 , 0 );
 std::size_t h = 0;
 for( auto [ x , e ] : E )
  if( e > 0 )
   pred[ e ] = h;
  else
   h = std::max( h , std::size_t( -e ) );
 std::vector< W > DP( n + 1 , W( 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  DP[ i ] = std::max( DP[ i - 1 ] , I[ by[ i - 1 ] ].w + DP[ pred[ i ] ] );
 std::vector< std::size_t > S;
 for( std::size_t i = n ; i > 0 ; )
  if( DP[ i - 1 ] >= I[ by[ i - 1 ] ].w + DP[ pred[ i ] ] )
   --i;
  else {
   S.push_back( by[ i - 1 ] );
   i = pred[ i ];
   }
 std::sort( S.begin() , S.end() );
 ADS_AUDIT( ( [ & ] {                     // disjoint, of the weight found
  W w = W( 0 );
  for( std::size_t x = 0 ; x < S.size() ; ++x ) {
   w += I[ S[ x ] ].w;
   for( std::size_t y = x + 1 ; y < S.size() ; ++y )
    if( ( I[ S[ x ] ].a < I[ S[ y ] ].b ) &&
        ( I[ S[ y ] ].a < I[ S[ x ] ].b ) )
     return( false );
   }
  return( w == DP[ n ] ); }() ) );
 return( std::pair( S , DP[ n ] ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File WeightedIntervals.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
