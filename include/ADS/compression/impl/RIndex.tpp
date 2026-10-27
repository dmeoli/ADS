/*--------------------------------------------------------------------------*/
/*----------------------------- File RIndex.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RIndex.h, with their contracts.
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

inline RIndex::RIndex( std::string_view s ) : f_n( s.size() )
{
 ADS_PRE( std::uint64_t( s.size() ) < ( std::uint64_t( 1 ) << 32 ) );
#if ADS_CONTRACTS >= 3
 f_text.assign( s );
#endif
 const auto sa = detail::sentinelSuffixArray( s );
 const std::size_t rows = f_n + 1;
 f_w = std::max( 1 , int( std::bit_width( rows ) ) );
 auto sym = [ & ]( std::size_t i ) {        // L[ i ], the $ as 0
  return( sa[ i ] == 0 ? 0u : std::uint32_t( static_cast< unsigned char >(
                                               s[ sa[ i ] - 1 ] ) ) + 1 );
  };
 // the runs: their starts, heads and ends, and the counts of L
 std::vector< std::uint64_t > start;
 std::vector< std::uint32_t > head;
 std::vector< std::size_t > end;
 for( std::size_t i = 0 ; i < rows ; ++i ) {
  const std::uint32_t c = sym( i );
  ++f_C[ c + 1 ];
  if( ( i == 0 ) || ( c != head.back() ) ) {
   if( i > 0 ) end.push_back( i - 1 );
   start.push_back( i );
   head.push_back( c );
   }
  }
 end.push_back( rows - 1 );
 for( std::size_t c = 1 ; c < f_C.size() ; ++c ) f_C[ c ] += f_C[ c - 1 ];
 const std::size_t r = start.size();
 f_start = EliasFano( start , rows );
 f_head = HuffmanWaveletTree< CompactRankSelect >( head );
 // the starts of the runs in F, those of each symbol after those of the
 // symbols below it, and the number of the runs of each symbol
 for( std::size_t j = 0 ; j < r ; ++j ) ++f_R[ head[ j ] + 1 ];
 for( std::size_t c = 1 ; c < f_R.size() ; ++c ) f_R[ c ] += f_R[ c - 1 ];
 std::vector< std::uint64_t > F( r + 1 );
 {
 std::array< std::size_t , 258 > at = f_R;
 std::array< std::size_t , 257 > filled{};
 for( std::size_t j = 0 ; j < r ; ++j ) {
  const std::uint32_t c = head[ j ];
  F[ at[ c ]++ ] = f_C[ c ] + filled[ c ];
  filled[ c ] += end[ j ] - start[ j ] + 1;
  }
 }
 F[ r ] = rows;
 f_F = EliasFano( F , rows + 1 );
 // the samples at the ends of the runs, and phi at the places of their
 // starts (but the row 0), by place
 f_end = BitVector( r * std::size_t( f_w ) );
 for( std::size_t j = 0 ; j < r ; ++j )
  f_end.put( j * std::size_t( f_w ) , sa[ end[ j ] ] , f_w );
 std::vector< std::pair< std::uint64_t , std::uint64_t > > kv;
 for( std::size_t j = 1 ; j < r ; ++j )
  kv.emplace_back( sa[ start[ j ] ] , j );
 std::sort( kv.begin() , kv.end() );
 std::vector< std::uint64_t > key( kv.size() );
 f_wr = std::max( 1 , int( std::bit_width( r ) ) );
 f_phi = BitVector( kv.size() * std::size_t( f_wr ) );
 for( std::size_t k = 0 ; k < kv.size() ; ++k ) {
  key[ k ] = kv[ k ].first;
  f_phi.put( k * std::size_t( f_wr ) , kv[ k ].second , f_wr );
  }
 f_key = EliasFano( key , rows );
 }

inline std::size_t RIndex::rank( std::uint32_t c , std::size_t i ) const
{
 if( ( i == 0 ) || ( f_R[ c + 1 ] == f_R[ c ] ) ) return( 0 );
 const std::size_t j = f_start.nextGEQ( i ) - 1;   // the run of i - 1
 const auto [ k0 , k1 ] = f_head.rank( c , j , j + 1 );
 std::size_t r = std::size_t( f_F.access( f_R[ c ] + k0 ) ) - f_C[ c ];
 if( k1 > k0 ) r += i - std::size_t( f_start.access( j ) );
 return( r );
 }

inline std::pair< std::size_t , std::size_t > RIndex::range(
                                                  std::string_view P ) const
{
 ADS_PRE( ! P.empty() );
 std::size_t first = 0 , last = f_n + 1;
 for( std::size_t i = P.size() ; i-- > 0 ; ) {
  const std::uint32_t c = std::uint32_t( static_cast< unsigned char >(
                                          P[ i ] ) ) + 1;
  first = f_C[ c ] + rank( c , first );
  last = f_C[ c ] + rank( c , last );
  if( first >= last ) return( std::pair< std::size_t , std::size_t >() );
  }
 return( std::pair( first , last ) );
 }

inline std::size_t RIndex::count( std::string_view P ) const
{
 const auto [ first , last ] = range( P );
 return( last - first );
 }

inline std::size_t RIndex::phi( std::size_t x ) const
{
 const std::size_t k = f_key.nextGEQ( x + 1 ) - 1;   // the last key <= x
 const std::size_t j = get( f_phi , f_wr , k );    // its run
 return( get( f_end , f_w , j - 1 ) +
         ( x - std::size_t( f_key.access( k ) ) ) );
 }

inline std::vector< std::size_t > RIndex::locate( std::string_view P ) const
{
 ADS_PRE( ! P.empty() );
 std::vector< std::size_t > R;
 std::size_t first = 0 , last = f_n + 1;
 // the toehold: the place of the suffix of the last row of the range
 std::size_t v = get( f_end , f_w , runs() - 1 );
 for( std::size_t i = P.size() ; i-- > 0 ; ) {
  const std::uint32_t c = std::uint32_t( static_cast< unsigned char >(
                                          P[ i ] ) ) + 1;
  if( f_R[ c + 1 ] == f_R[ c ] ) return( R );
  const std::size_t j = f_start.nextGEQ( last ) - 1;   // the run of last - 1
  const auto [ k0 , k1 ] = f_head.rank( c , j , j + 1 );
  if( k1 > k0 ) --v;                        // L[ last - 1 ] = c
  else if( k0 > 0 )                         // the end of the last run of c
   v = get( f_end , f_w , f_head.select( c , k0 ) ) - 1;
  first = f_C[ c ] + rank( c , first );
  last = f_C[ c ] + rank( c , last );
  if( first >= last ) return( R );
  }
 R.reserve( last - first );
 R.push_back( v );
 for( std::size_t k = first + 1 ; k < last ; ++k ) R.push_back(
                                                     v = phi( v ) );
 std::sort( R.begin() , R.end() );
#if ADS_CONTRACTS >= 3
 ADS_AUDIT( std::all_of( R.begin() , R.end() , [ & ]( std::size_t p ) {
  return( f_text.compare( p , P.size() , P ) == 0 ); } ) );
#endif
 return( R );
 }

inline std::size_t RIndex::sizeInBits( void ) const
{
 return( f_start.sizeInBits() + f_head.sizeInBits() + f_F.sizeInBits() +
         f_end.size() + f_key.sizeInBits() + f_phi.size() +
         64 * ( f_C.size() + f_R.size() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File RIndex.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
