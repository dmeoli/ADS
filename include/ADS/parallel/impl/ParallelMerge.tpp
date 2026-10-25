/*--------------------------------------------------------------------------*/
/*------------------------- File ParallelMerge.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The multisequence selection and the parallel merge of ParallelMerge.h.
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

template< std::random_access_iterator I , class C >
std::vector< std::size_t > multisequenceSplit(
                         const std::vector< std::pair< I , I > > & R ,
                         std::size_t r , C less )
{
 const std::size_t k = R.size();
 // the split lies in [ lo_i , hi_i ] on each range: the items before lo_i
 // are among the r least, those from hi_i on are not
 std::vector< std::size_t > lo( k , 0 ) , hi( k ) , c( k );
 std::size_t total = 0;
 for( std::size_t i = 0 ; i < k ; ++i )
  total += hi[ i ] = std::size_t( R[ i ].second - R[ i ].first );
 ADS_PRE( r <= total );
 std::vector< std::size_t > cand;
 for( ;; ) {
  // the pivot: the weighted median of the middles of the windows, each
  // weighted by its width, so that at least a quarter of the total width
  // goes at each step
  cand.clear();
  std::size_t width = 0;
  for( std::size_t i = 0 ; i < k ; ++i )
   if( hi[ i ] > lo[ i ] ) {
    cand.push_back( i );
    width += hi[ i ] - lo[ i ];
    }
  if( cand.empty() )
   break;
  auto mid = [ & ]( std::size_t i ) {
   return( lo[ i ] + ( hi[ i ] - lo[ i ] ) / 2 ); };
  auto at = [ & ]( std::size_t i ) -> const auto & {
   return( R[ i ].first[ std::ptrdiff_t( mid( i ) ) ] ); };
  std::sort( cand.begin() , cand.end() , [ & ]( std::size_t a ,
                                               std::size_t b ) {
   if( less( at( a ) , at( b ) ) ) return( true );
   if( less( at( b ) , at( a ) ) ) return( false );
   return( a < b ); } );
  std::size_t j = cand.back() , acc = 0;
  for( std::size_t i : cand ) {
   acc += hi[ i ] - lo[ i ];
   if( 2 * acc >= width ) {
    j = i;
    break;
    }
   }
  const std::size_t m = mid( j );
  const auto & x = R[ j ].first[ std::ptrdiff_t( m ) ];
  // the items before ( x , j , m ): <= x in the ranges before j, < x in
  // those after it
  std::size_t before = 0;
  for( std::size_t i = 0 ; i < k ; ++i ) {
   const I f = R[ i ].first;
   if( i == j )
    c[ i ] = m;
   else if( i < j )
    c[ i ] = std::size_t( std::upper_bound( f + std::ptrdiff_t( lo[ i ] ) ,
                                            f + std::ptrdiff_t( hi[ i ] ) ,
                                            x , less ) - f );
   else
    c[ i ] = std::size_t( std::lower_bound( f + std::ptrdiff_t( lo[ i ] ) ,
                                            f + std::ptrdiff_t( hi[ i ] ) ,
                                            x , less ) - f );
   before += c[ i ];
   }
  if( before < r ) {                        // x and those before it: left
   lo = c;
   lo[ j ] = m + 1;
   }
  else {                                    // x and those after it: right
   hi = c;
   hi[ j ] = m;
   }
  }
 return( lo );
 }

/*--------------------------------------------------------------------------*/

template< Executor E , std::random_access_iterator I ,
          std::random_access_iterator O , class C >
O multiwayMerge( E & ex , const std::vector< std::pair< I , I > > & R ,
                 O out , C less )
{
 const std::size_t k = R.size() , p = ex.concurrency();
 std::size_t N = 0;
 for( const auto & r : R )
  N += std::size_t( r.second - r.first );
 if( ( p == 1 ) || ( k < 2 ) || ( N < ( std::size_t( 1 ) << 16 ) ) )
  return( multiwayMerge( R , out , less ) );
 const std::size_t B = std::max< std::size_t >( 2 , std::min( 4 * p ,
                                               N >> 14 ) );
 std::vector< std::vector< std::size_t > > S( B + 1 );
 parallelFor( ex , 0 , B + 1 , [ & ]( std::size_t q ) {
  S[ q ] = multisequenceSplit( R , N * q / B , less );
  } , 1 );
 parallelFor( ex , 0 , B , [ & ]( std::size_t q ) {
  std::vector< std::pair< I , I > > P( k );
  for( std::size_t i = 0 ; i < k ; ++i )
   P[ i ] = { R[ i ].first + std::ptrdiff_t( S[ q ][ i ] ) ,
              R[ i ].first + std::ptrdiff_t( S[ q + 1 ][ i ] ) };
  multiwayMerge( P , out + std::ptrdiff_t( N * q / B ) , less );
  } , 1 );
 ADS_AUDIT( std::is_sorted( out , out + std::ptrdiff_t( N ) , less ) );
 return( out + std::ptrdiff_t( N ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParallelMerge.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
