/*--------------------------------------------------------------------------*/
/*----------------------------- File BVGraph.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BVGraph.h, with their contracts.
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

inline BitVector BVGraph::copyList( const List & s , const List & ref )
{
 BitVector c;
 c.reserve( ref.size() );
 for( const std::size_t y : ref )
  c.push_back( std::binary_search( s.begin() , s.end() , y ) );
 return( c );
 }

inline std::pair< bool , std::vector< std::size_t > > BVGraph::copyBlocks(
                                                         const BitVector & c )
{
 std::vector< std::size_t > len;
 if( c.empty() ) return( std::pair( false , len ) );
 std::size_t run = 1;
 for( std::size_t i = 1 ; i < c.size() ; ++i )
  if( c[ i ] == c[ i - 1 ] ) ++run;
  else {
   len.push_back( run - 1 );
   run = 1;
   }
 return( std::pair( c[ 0 ] , len ) );       // the last run is omitted
 }

inline std::pair< std::vector< BVGraph::Interval > , BVGraph::List >
BVGraph::intervals( const List & e , std::size_t lmin )
{
 std::vector< Interval > iv;
 List res;
 for( std::size_t i = 0 , j ; i < e.size() ; i = j ) {
  for( j = i + 1 ; ( j < e.size() ) && ( e[ j ] == e[ j - 1 ] + 1 ) ; ++j ) {}
  if( j - i >= lmin ) iv.emplace_back( e[ i ] , j - i );
  else res.insert( res.end() , e.begin() + std::ptrdiff_t( i ) ,
                   e.begin() + std::ptrdiff_t( j ) );
  }
 return( std::pair( iv , res ) );
 }

inline std::vector< std::uint64_t > BVGraph::residualGaps( const List & res ,
                                                           std::size_t x )
{
 std::vector< std::uint64_t > g( res.size() );
 for( std::size_t i = 0 ; i < res.size() ; ++i )
  g[ i ] = i ? res[ i ] - res[ i - 1 ] - 1
             : signedToNatural( std::int64_t( res[ 0 ] ) -
                                std::int64_t( x ) );
 return( g );
 }

inline BVGraph::BVGraph( const std::vector< List > & a , std::size_t w ,
                         std::size_t r , std::size_t lmin )
 : f_n( a.size() ) , f_lmin( lmin )
{
 ADS_PRE( ( w >= 1 ) && ( r >= 1 ) && ( lmin >= 2 ) );
 BitWriter out;
 std::vector< std::size_t > chain( f_n , 0 );
 std::vector< std::uint64_t > start;
 start.reserve( f_n + 1 );
 for( std::size_t x = 0 ; x < f_n ; ++x ) {
  const List & S = a[ x ];
  ADS_AUDIT( std::adjacent_find( S.begin() , S.end() ,
                                 []( std::size_t p , std::size_t q ) {
                                  return( p >= q ); } ) == S.end() );
  ADS_PRE( S.empty() || ( S.back() < f_n ) );
  start.push_back( out.size() );
  f_m += S.size();
  encodeGamma( out , S.size() + 1 );
  if( S.empty() ) continue;
  std::size_t best = 0 , common = 0;        // the reference, in the window
  for( std::size_t k = 1 ; ( k <= w ) && ( k <= x ) ; ++k ) {
   if( chain[ x - k ] >= r ) continue;
   const List & T = a[ x - k ];
   std::size_t c = 0;
   for( std::size_t i = 0 , j = 0 ; ( i < S.size() ) && ( j < T.size() ) ; )
    if( S[ i ] < T[ j ] ) ++i;
    else if( T[ j ] < S[ i ] ) ++j;
    else { ++c; ++i; ++j; }
   if( c > common ) {
    common = c;
    best = k;
    }
   }
  encodeGamma( out , best + 1 );
  List extra;
  if( best > 0 ) {                          // the copy blocks
   const List & T = a[ x - best ];
   const auto [ first , len ] = copyBlocks( copyList( S , T ) );
   encodeGamma( out , len.size() + 1 );
   out.writeBit( first );
   for( const std::size_t l : len ) encodeGamma( out , l + 1 );
   std::set_difference( S.begin() , S.end() , T.begin() , T.end() ,
                        std::back_inserter( extra ) );
   chain[ x ] = chain[ x - best ] + 1;
   }
  else extra = S;
  const auto [ iv , res ] = intervals( extra , lmin );
  encodeGamma( out , iv.size() + 1 );
  for( std::size_t i = 0 ; i < iv.size() ; ++i ) {
   const auto [ left , len ] = iv[ i ];
   encodeGamma( out , 1 + ( i ? left - ( iv[ i - 1 ].first +
                                         iv[ i - 1 ].second - 1 ) - 2
                              : signedToNatural( std::int64_t( left ) -
                                                 std::int64_t( x ) ) ) );
   encodeGamma( out , len - lmin + 1 );
   }
  for( const std::uint64_t g : residualGaps( res , x ) )
   encodeDelta( out , g + 1 );
  }
 start.push_back( out.size() );
 f_bits = out.release();
 f_start = EliasFano( start );
 ADS_AUDIT( [ & ] {
  for( std::size_t x = 0 ; x < f_n ; ++x )
   if( successors( x ) != a[ x ] ) return( false );
  return( true ); }() );
 }

inline std::size_t BVGraph::order( void ) const
{
 return( f_n );
 }

inline std::size_t BVGraph::edges( void ) const
{
 return( f_m );
 }

inline std::size_t BVGraph::outdegree( std::size_t x ) const
{
 ADS_PRE( x < f_n );
 BitReader rd( f_bits , std::size_t( f_start[ x ] ) );
 return( std::size_t( decodeGamma( rd ) - 1 ) );
 }

inline std::size_t BVGraph::reference( std::size_t x ) const
{
 ADS_PRE( x < f_n );
 BitReader rd( f_bits , std::size_t( f_start[ x ] ) );
 if( decodeGamma( rd ) == 1 ) return( 0 );
 return( std::size_t( decodeGamma( rd ) - 1 ) );
 }

inline BVGraph::List BVGraph::decode( std::size_t x ) const
{
 BitReader rd( f_bits , std::size_t( f_start[ x ] ) );
 const std::size_t d = std::size_t( decodeGamma( rd ) - 1 );
 List s;
 if( d == 0 ) return( s );
 s.reserve( d );
 const std::size_t r = std::size_t( decodeGamma( rd ) - 1 );
 if( r > 0 ) {                              // the items copied
  const List T = decode( x - r );
  const std::size_t nb = std::size_t( decodeGamma( rd ) - 1 );
  bool bit = rd.readBit();
  std::size_t p = 0;
  for( std::size_t b = 0 ; b <= nb ; ++b , bit = ! bit ) {
   const std::size_t len = b < nb ? std::size_t( decodeGamma( rd ) )
                                  : T.size() - p;   // the last run
   for( std::size_t i = 0 ; i < len ; ++i , ++p )
    if( bit ) s.push_back( T[ p ] );
   }
  }
 const std::size_t ni = std::size_t( decodeGamma( rd ) - 1 );
 std::size_t right = 0;
 for( std::size_t i = 0 ; i < ni ; ++i ) {  // the intervals
  const std::uint64_t g = decodeGamma( rd ) - 1;
  const std::size_t left = i ? right + 2 + std::size_t( g )
                             : std::size_t( std::int64_t( x ) +
                                            naturalToSigned( g ) );
  const std::size_t len = std::size_t( decodeGamma( rd ) - 1 ) + f_lmin;
  for( std::size_t k = 0 ; k < len ; ++k ) s.push_back( left + k );
  right = left + len - 1;
  }
 for( std::size_t i = 0 , k = d - s.size() , v = 0 ; i < k ; ++i ) {
  const std::uint64_t g = decodeDelta( rd ) - 1;   // the residuals
  v = i ? v + std::size_t( g ) + 1
        : std::size_t( std::int64_t( x ) + naturalToSigned( g ) );
  s.push_back( v );
  }
 std::sort( s.begin() , s.end() );
 return( s );
 }

inline BVGraph::List BVGraph::successors( std::size_t x ) const
{
 ADS_PRE( x < f_n );
 return( decode( x ) );
 }

inline std::size_t BVGraph::codeBits( void ) const
{
 return( f_bits.size() );
 }

inline std::size_t BVGraph::sizeInBits( void ) const
{
 return( f_bits.size() + f_start.sizeInBits() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BVGraph.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
