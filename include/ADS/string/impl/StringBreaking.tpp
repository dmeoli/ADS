/*--------------------------------------------------------------------------*/
/*------------------------ File StringBreaking.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of StringBreaking [see StringBreaking.h], with their
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

namespace detail {

/// the tables of the least costs D and of the largest optimal splits K on
/// the places w_0 = 0 , cuts , w_{n+1} = length, by the speedup of Knuth
template< class I >
struct StringBreakingTables {
 std::vector< I > w;
 std::vector< std::vector< I > > D;
 std::vector< std::vector< std::size_t > > K;

 StringBreakingTables( const std::vector< I > & cuts , I length )
 {
  const std::size_t n = cuts.size();
  for( std::size_t i = 0 ; i < n ; ++i )
   ADS_PRE( ( cuts[ i ] > ( i ? cuts[ i - 1 ] : I( 0 ) ) ) &&
            ( cuts[ i ] < length ) );
  w.assign( n + 2 , I( 0 ) );
  std::copy( cuts.begin() , cuts.end() , w.begin() + 1 );
  w[ n + 1 ] = length;
  D.assign( n + 2 , std::vector< I >( n + 2 , I( 0 ) ) );
  K.assign( n + 2 , std::vector< std::size_t >( n + 2 , 0 ) );
  for( std::size_t i = 0 ; i + 1 < n + 2 ; ++i )
   K[ i ][ i + 1 ] = i + 1;
  for( std::size_t d = 2 ; d < n + 2 ; ++d )
   for( std::size_t i = 0 ; i + d < n + 2 ; ++i ) {
    const std::size_t j = i + d;
    const std::size_t lo = std::max( K[ i ][ j - 1 ] , i + 1 ) ,
                      hi = std::min( K[ i + 1 ][ j ] , j - 1 );
    std::size_t best = lo;
    for( std::size_t k = lo + 1 ; k <= hi ; ++k )
     if( D[ i ][ k ] + D[ k ][ j ] <= D[ i ][ best ] + D[ best ][ j ] )
      best = k;
    D[ i ][ j ] = D[ i ][ best ] + D[ best ][ j ] + ( w[ j ] - w[ i ] );
    K[ i ][ j ] = best;
    }
  }
 };

/// the cost of the cuts in the given order on a string of the length
template< class I >
I breakingCostOf( const std::vector< I > & order , I length )
{
 std::set< I > done = { I( 0 ) , length };
 I c = I( 0 );
 for( const I & p : order ) {
  const auto hi = done.upper_bound( p );
  c += *hi - *std::prev( hi );
  done.insert( p );
  }
 return( c );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

template< class I >
I stringBreakingCost( const std::vector< I > & cuts ,
                      std::type_identity_t< I > length )
{
 const detail::StringBreakingTables< I > T( cuts , length );
 return( T.D[ 0 ][ cuts.size() + 1 ] );
 }

/*--------------------------------------------------------------------------*/

inline std::int64_t stringBreakingCost(
               const std::vector< std::int64_t > & cuts , std::int64_t length )
{
 return( stringBreakingCost< std::int64_t >( cuts , length ) );
 }

/*--------------------------------------------------------------------------*/

template< class I >
std::vector< I > stringBreakingOrder( const std::vector< I > & cuts ,
                                      std::type_identity_t< I > length )
{
 const detail::StringBreakingTables< I > T( cuts , length );
 std::vector< I > order;
 std::vector< std::pair< std::size_t , std::size_t > > todo = {
  std::pair( std::size_t( 0 ) , cuts.size() + 1 ) };
 while( ! todo.empty() ) {
  const auto [ i , j ] = todo.back();
  todo.pop_back();
  if( j <= i + 1 )
   continue;
  const std::size_t k = T.K[ i ][ j ];
  order.push_back( T.w[ k ] );
  todo.push_back( std::pair( k , j ) );
  todo.push_back( std::pair( i , k ) );
  }
 ADS_POST( order.size() == cuts.size() );
 ADS_AUDIT( std::is_permutation( order.begin() , order.end() , cuts.begin() ,
                                 cuts.end() ) &&
            ( detail::breakingCostOf( order , I( length ) ) ==
              T.D[ 0 ][ cuts.size() + 1 ] ) );
 return( order );
 }

/*--------------------------------------------------------------------------*/

inline std::vector< std::int64_t > stringBreakingOrder(
               const std::vector< std::int64_t > & cuts , std::int64_t length )
{
 return( stringBreakingOrder< std::int64_t >( cuts , length ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File StringBreaking.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
