/*--------------------------------------------------------------------------*/
/*--------------------------- File Knapsack.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Knapsack [see Knapsack.h], with their contracts.
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

/// profits and volumes of the same objects, none negative, and C >= 0
inline bool knapsackInput( const std::vector< long > & p ,
                           const std::vector< long > & v , long C )
{
 auto nonnegative = []( long x ) { return( x >= 0 ); };
 return( ( p.size() == v.size() ) && ( C >= 0 ) &&
         std::all_of( p.begin() , p.end() , nonnegative ) &&
         std::all_of( v.begin() , v.end() , nonnegative ) );
 }

}  // end( namespace detail )

inline std::pair< long , std::vector< std::size_t > > knapsack(
                                            const std::vector< long > & p ,
                                            const std::vector< long > & v ,
                                            long C )
{
 ADS_PRE( detail::knapsackInput( p , v , C ) );
 const std::size_t n = p.size();
 std::vector< std::vector< long > > DP( n + 1 ,
                                       std::vector< long >( C + 1 , 0 ) );
 for( std::size_t i = 1 ; i <= n ; ++i )
  for( long c = 1 ; c <= C ; ++c ) {
   DP[ i ][ c ] = DP[ i - 1 ][ c ];
   if( v[ i - 1 ] <= c )
    DP[ i ][ c ] = std::max( DP[ i ][ c ] ,
                             DP[ i - 1 ][ c - v[ i - 1 ] ] + p[ i - 1 ] );
   }
 std::vector< std::size_t > S;
 for( std::size_t i = n , c = std::size_t( C ) ; i > 0 ; --i )
  if( DP[ i ][ c ] != DP[ i - 1 ][ c ] ) {
   S.push_back( i - 1 );
   c -= std::size_t( v[ i - 1 ] );
   }
 std::reverse( S.begin() , S.end() );
 ADS_POST( ( [ & ] {                      // feasible, of the profit found
  long tp = 0 , tv = 0;
  for( std::size_t i : S ) { tp += p[ i ]; tv += v[ i ]; }
  return( ( tv <= C ) && ( tp == DP[ n ][ C ] ) ); }() ) );
 return( std::pair( DP[ n ][ C ] , S ) );
 }

inline long zaino( const std::vector< long > & p ,
                   const std::vector< long > & v , long C ,
                   long * solved )
{
 ADS_PRE( detail::knapsackInput( p , v , C ) );
 const long NEG = std::numeric_limits< long >::min() / 2;
 ChainedHashTable< long , long > DP( 64 , true );   // key i ( C + 1 ) + c
 auto rec = [ & ]( auto & self , long i , long c ) -> long {
  if( c < 0 )
   return( NEG );
  if( ( i == 0 ) || ( c == 0 ) )
   return( 0 );
  const long key = i * ( C + 1 ) + c;
  if( auto r = DP.lookup( key ) )
   return( *r );
  if( solved ) ++*solved;
  const long r = std::max( self( self , i - 1 , c ) ,
                           self( self , i - 1 , c - v[ i - 1 ] ) +
                           p[ i - 1 ] );
  DP.insert( key , r );
  return( r );
  };
 return( rec( rec , long( p.size() ) , C ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Knapsack.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
