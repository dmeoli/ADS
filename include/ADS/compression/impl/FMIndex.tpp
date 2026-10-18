/*--------------------------------------------------------------------------*/
/*---------------------------- File FMIndex.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FMIndex [see FMIndex.h], with their contracts.
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

inline FMIndex::FMIndex( std::string_view s , std::size_t b , std::size_t mu )
 : f_n( s.size() ) , f_b( b ) , f_mu( mu ) , f_dollar( 0 )
{
 ADS_PRE( ( b > 0 ) && ( mu > 0 ) );
 ADS_PRE( std::uint64_t( s.size() ) < ( std::uint64_t( 1 ) << 32 ) );
 const auto sa = detail::sentinelSuffixArray( s );
 const std::size_t rows = f_n + 1;
 f_L.assign( rows , '\0' );
 for( std::size_t i = 0 ; i < rows ; ++i )
  if( sa[ i ] == 0 ) f_dollar = i;
  else f_L[ i ] = s[ sa[ i ] - 1 ];
 std::array< std::size_t , 256 > cnt{};
 for( char c : s ) ++cnt[ static_cast< unsigned char >( c ) ];
 f_C[ 0 ] = 1;                             // the $ comes first
 for( std::size_t c = 0 ; c < 256 ; ++c ) {
  f_C[ c + 1 ] = f_C[ c ] + cnt[ c ];
  f_id[ c ] = cnt[ c ] > 0 ? int( f_sigma++ ) : -1;
  }
 f_occ.assign( ( rows / f_b + 1 ) * f_sigma , 0 );   // the counts
 std::vector< std::uint32_t > run( f_sigma , 0 );
 for( std::size_t i = 0 ; i <= rows ; ++i ) {
  if( i % f_b == 0 )
   std::copy( run.begin() , run.end() ,
              f_occ.begin() + std::ptrdiff_t( i / f_b * f_sigma ) );
  if( ( i < rows ) && ( i != f_dollar ) )
   ++run[ std::size_t( f_id[ static_cast< unsigned char >( f_L[ i ] ) ] ) ];
  }
 BitVector mark( rows );                   // the samples
 f_row.assign( f_n / f_mu + 1 , 0 );
 for( std::size_t i = 0 ; i < rows ; ++i )
  if( sa[ i ] % f_mu == 0 ) {
   mark.set( i );
   f_place.push_back( sa[ i ] );
   f_row[ sa[ i ] / f_mu ] = i;
   }
 f_mark = RankSelect( std::move( mark ) );
 }

inline BWT FMIndex::transform( void ) const
{
 BWT t{ f_L , f_dollar };
 t.last.erase( f_dollar , 1 );
 return( t );
 }

inline std::size_t FMIndex::rank( char c , std::size_t k ) const
{
 ADS_PRE( k <= f_n + 1 );
 const int id = f_id[ static_cast< unsigned char >( c ) ];
 if( id < 0 ) return( 0 );
 const std::size_t blk = k / f_b;
 std::size_t r = f_occ[ blk * f_sigma + std::size_t( id ) ];
 for( std::size_t i = blk * f_b ; i < k ; ++i )
  if( ( f_L[ i ] == c ) && ( i != f_dollar ) ) ++r;
 return( r );
 }

inline std::size_t FMIndex::lf( std::size_t r ) const
{
 ADS_PRE( r != f_dollar );
 const char c = f_L[ r ];
 return( f_C[ static_cast< unsigned char >( c ) ] + rank( c , r ) );
 }

inline std::pair< std::size_t , std::size_t > FMIndex::range(
                                                   std::string_view P ) const
{
 ADS_PRE( ! P.empty() );
 std::size_t first = 0 , last = f_n + 1;
 for( std::size_t i = P.size() ; ( i-- > 0 ) && ( first < last ) ; ) {
  const std::size_t c = static_cast< unsigned char >( P[ i ] );
  first = f_C[ c ] + rank( P[ i ] , first );
  last = f_C[ c ] + rank( P[ i ] , last );
  }
 if( first > last ) last = first;
 return( std::pair( first , last ) );
 }

inline std::size_t FMIndex::count( std::string_view P ) const
{
 const auto [ first , last ] = range( P );
 return( last - first );
 }

inline std::size_t FMIndex::place( std::size_t r ) const
{
 std::size_t t = 0;
 for( ; ! marked( r ) ; ++t ) r = lf( r );  // at most mu - 1 steps
 return( f_place[ f_mark.rank1( r ) ] + t );
 }

inline std::vector< std::size_t > FMIndex::locate( std::string_view P ) const
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

inline std::string FMIndex::extract( std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j <= f_n ) );
 std::string out( j - i , '\0' );
 if( i == j ) return( out );
 std::size_t p = ( j + f_mu - 1 ) / f_mu * f_mu , r;
 if( p >= f_n ) {                          // the suffix $, row 0
  p = f_n;
  r = 0;
  }
 else r = f_row[ p / f_mu ];
 for( std::size_t t = p ; t > i ; --t ) {  // r is the row of suffix t
  if( t - 1 < j ) out[ t - 1 - i ] = f_L[ r ];
  r = lf( r );
  }
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File FMIndex.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
