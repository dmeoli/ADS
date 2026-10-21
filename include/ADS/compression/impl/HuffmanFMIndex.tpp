/*--------------------------------------------------------------------------*/
/*------------------------ File HuffmanFMIndex.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of HuffmanFMIndex.h, with their contracts.
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

template< class RS >
HuffmanFMIndex< RS >::HuffmanFMIndex( std::string_view s , std::size_t mu )
 : f_n( s.size() ) , f_mu( mu )
{
 ADS_PRE( mu > 0 );
 ADS_PRE( std::uint64_t( s.size() ) < ( std::uint64_t( 1 ) << 32 ) );
 const auto sa = detail::sentinelSuffixArray( s );
 const std::size_t rows = f_n + 1;
 f_w = std::max( 1 , int( std::bit_width( rows ) ) );
 std::vector< std::uint32_t > L( rows );
 for( std::size_t i = 0 ; i < rows ; ++i )
  L[ i ] = sa[ i ] == 0 ? 0 : std::uint32_t( static_cast< unsigned char >(
                                               s[ sa[ i ] - 1 ] ) ) + 1;
 for( const auto c : L ) ++f_C[ c + 1 ];
 for( std::size_t c = 1 ; c < f_C.size() ; ++c ) f_C[ c ] += f_C[ c - 1 ];
 f_L = HuffmanWaveletTree< RS >( L , false );   // rank alone
 // the samples: the rows of the suffixes at the multiples of mu, marked,
 // their places in the order of the rows, and the row of each of them
 BitVector mark( rows );
 std::size_t marks = 0;
 for( std::size_t i = 0 ; i < rows ; ++i )
  if( sa[ i ] % f_mu == 0 ) {
   mark.set( i );
   ++marks;
   }
 f_place = BitVector( marks * std::size_t( f_w ) );
 f_row = BitVector( ( f_n / f_mu + 1 ) * std::size_t( f_w ) );
 for( std::size_t i = 0 , k = 0 ; i < rows ; ++i )
  if( sa[ i ] % f_mu == 0 ) {
   f_place.put( std::size_t( f_w ) * k++ , sa[ i ] , f_w );
   f_row.put( std::size_t( f_w ) * ( sa[ i ] / f_mu ) , i , f_w );
   }
 f_mark = CompactRankSelect( std::move( mark ) );
 }

template< class RS >
std::pair< std::size_t , std::size_t > HuffmanFMIndex< RS >::range(
                                                  std::string_view P ) const
{
 ADS_PRE( ! P.empty() );
 std::size_t first = 0 , last = f_n + 1;
 for( std::size_t i = P.size() ; i-- > 0 ; ) {
  const std::uint32_t c = std::uint32_t( static_cast< unsigned char >(
                                          P[ i ] ) ) + 1;
  const auto [ a , b ] = f_L.rank( c , first , last );
  first = f_C[ c ] + a;
  last = f_C[ c ] + b;
  if( first >= last ) return( std::pair< std::size_t , std::size_t >() );
  }
 return( std::pair( first , last ) );
 }

template< class RS >
std::size_t HuffmanFMIndex< RS >::count( std::string_view P ) const
{
 const auto [ first , last ] = range( P );
 return( last - first );
 }

template< class RS >
std::size_t HuffmanFMIndex< RS >::place( std::size_t r ) const
{
 std::size_t t = 0;
 while( ! f_mark[ r ] ) {                   // LF, one symbol to the left
  const auto [ c , k ] = f_L.accessRank( r );
  r = f_C[ c ] + k;
  ++t;
  }
 return( std::size_t( f_place.get( std::size_t( f_w ) * f_mark.rank1( r ) ,
                                   f_w ) ) + t );
 }

template< class RS >
std::vector< std::size_t > HuffmanFMIndex< RS >::locate(
                                                  std::string_view P ) const
{
 const auto [ first , last ] = range( P );
 std::vector< std::size_t > R;
 R.reserve( last - first );
 for( std::size_t r = first ; r < last ; ++r ) R.push_back( place( r ) );
 std::sort( R.begin() , R.end() );
 ADS_AUDIT( std::all_of( R.begin() , R.end() , [ & ]( std::size_t p ) {
  return( ( p + P.size() <= f_n ) &&
          ( extract( p , p + P.size() ) == P ) ); } ) );
 return( R );
 }

template< class RS >
std::string HuffmanFMIndex< RS >::extract( std::size_t i ,
                                           std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j <= f_n ) );
 std::string out( j - i , '\0' );
 if( i == j ) return( out );
 std::size_t p = ( j + f_mu - 1 ) / f_mu * f_mu , r;
 if( p >= f_n ) {                          // the suffix $, row 0
  p = f_n;
  r = 0;
  }
 else r = std::size_t( f_row.get( std::size_t( f_w ) * ( p / f_mu ) , f_w ) );
 for( std::size_t t = p ; t > i ; --t ) {  // r is the row of suffix t
  const auto [ c , k ] = f_L.accessRank( r );
  if( t - 1 < j ) out[ t - 1 - i ] = static_cast< char >( c - 1 );
  r = f_C[ c ] + k;
  }
 return( out );
 }

template< class RS >
std::size_t HuffmanFMIndex< RS >::sizeInBits( void ) const
{
 return( f_L.sizeInBits() + f_mark.size() + f_mark.directoryBits() +
         f_place.size() + f_row.size() + 64 * f_C.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File HuffmanFMIndex.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
