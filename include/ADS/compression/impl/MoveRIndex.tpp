/*--------------------------------------------------------------------------*/
/*--------------------------- File MoveRIndex.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of MoveRIndex.h, with their contracts.
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

inline MoveRIndex::MoveRIndex( std::string_view s ) : f_n( s.size() )
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
 // the runs of L and their images by LF: C[ c ] plus the c before them
 std::array< std::uint64_t , 258 > C{};
 for( std::size_t i = 0 ; i < rows ; ++i ) ++C[ sym( i ) + 1 ];
 for( std::size_t c = 1 ; c < C.size() ; ++c ) C[ c ] += C[ c - 1 ];
 std::vector< std::uint64_t > p , q;
 std::vector< std::uint32_t > head;
 std::array< std::uint64_t , 257 > seen{};
 for( std::size_t i = 0 ; i < rows ; ++i ) {
  const std::uint32_t c = sym( i );
  if( ( i == 0 ) || ( c != head.back() ) ) {
   p.push_back( i );
   q.push_back( C[ c ] + seen[ c ] );
   head.push_back( c );
   }
  ++seen[ c ];
  }
 f_lf = MoveStructure( p , q , head , rows );
 const std::size_t m = f_lf.intervals();
 std::vector< std::uint32_t > tags( m );
 for( std::size_t i = 0 ; i < m ; ++i )
  ++f_cnt[ tags[ i ] = f_lf.tag( i ) ];
 f_tag = HuffmanWaveletTree< RankSelect >( tags );
 // phi, cyclic, on the places of the starts of the runs
 std::vector< std::pair< std::uint64_t , std::uint64_t > > kv;
 for( std::size_t j = 0 ; j < p.size() ; ++j )
  kv.emplace_back( sa[ p[ j ] ] , sa[ p[ j ] == 0 ? rows - 1 : p[ j ] - 1 ] );
 std::sort( kv.begin() , kv.end() );
 std::vector< std::uint64_t > key( kv.size() ) , val( kv.size() );
 for( std::size_t k = 0 ; k < kv.size() ; ++k ) {
  key[ k ] = kv[ k ].first;
  val[ k ] = kv[ k ].second;
  }
 f_phi = MoveStructure( key , val , std::vector< std::uint32_t >(
                                     key.size() , 0 ) , rows );
 // the samples at the ends of the intervals of LF, with the interval of
 // phi of each of them less 1 (the toehold after a run of another symbol)
 f_wi = std::max( 1 , int( std::bit_width( f_phi.intervals() ) ) );
 f_end = BitVector( m * std::size_t( f_w ) );
 f_endPhi = BitVector( m * std::size_t( f_wi ) );
 for( std::size_t i = 0 ; i < m ; ++i ) {
  const std::uint64_t x = sa[ f_lf.start( i + 1 ) - 1 ];
  f_end.put( i * std::size_t( f_w ) , x , f_w );
  f_endPhi.put( i * std::size_t( f_wi ) ,
                x == 0 ? 0 : f_phi.interval( x - 1 ) , f_wi );
  }
 }

template< bool Toehold >
bool MoveRIndex::search( std::string_view P , std::uint64_t & s ,
                         std::size_t & is , std::uint64_t & e ,
                         std::size_t & ie , std::uint64_t & v ,
                         std::size_t & iv ) const
{
 ADS_PRE( ! P.empty() );
 const std::size_t m = f_lf.intervals();
 s = 0;
 is = 0;
 e = f_n;
 ie = m - 1;
 if constexpr( Toehold ) {
  v = f_end.get( ( m - 1 ) * std::size_t( f_w ) , f_w );
  iv = f_phi.interval( v );
  }
 for( std::size_t i = P.size() ; i-- > 0 ; ) {
  const std::uint32_t c = std::uint32_t( static_cast< unsigned char >(
                                          P[ i ] ) ) + 1;
  // s to the first row of c at or after it, e to the last at or before
  if( f_lf.tag( is ) != c ) {
   const std::size_t k = f_tag.rank( c , is );
   if( k == f_cnt[ c ] ) return( false );
   is = f_tag.select( c , k + 1 );
   s = f_lf.start( is );
   }
  if( f_lf.tag( ie ) != c ) {
   const std::size_t k = f_tag.rank( c , ie );
   if( k == 0 ) return( false );
   ie = f_tag.select( c , k );
   e = f_lf.start( ie + 1 ) - 1;
   if constexpr( Toehold ) {
    v = f_end.get( ie * std::size_t( f_w ) , f_w ) - 1;
    iv = std::size_t( f_endPhi.get( ie * std::size_t( f_wi ) , f_wi ) );
    }
   }
  else if constexpr( Toehold ) {
   --v;
   if( v < f_phi.start( iv ) ) --iv;
   }
  if( s > e ) return( false );
  std::tie( s , is ) = f_lf.move( s , is );
  std::tie( e , ie ) = f_lf.move( e , ie );
  }
 return( true );
 }

inline std::size_t MoveRIndex::count( std::string_view P ) const
{
 std::uint64_t s , e , v = 0;
 std::size_t is , ie , iv = 0;
 return( search< false >( P , s , is , e , ie , v , iv )
          ? std::size_t( e - s + 1 ) : 0 );
 }

inline std::vector< std::size_t > MoveRIndex::locate( std::string_view P )
 const
{
 std::uint64_t s , e , v;
 std::size_t is , ie , iv;
 std::vector< std::size_t > R;
 if( ! search< true >( P , s , is , e , ie , v , iv ) ) return( R );
 R.reserve( e - s + 1 );
 R.push_back( std::size_t( v ) );
 for( std::uint64_t k = s ; k < e ; ++k ) {
  std::tie( v , iv ) = f_phi.move( v , iv );
  R.push_back( std::size_t( v ) );
  }
 std::sort( R.begin() , R.end() );
#if ADS_CONTRACTS >= 3
 ADS_AUDIT( std::all_of( R.begin() , R.end() , [ & ]( std::size_t x ) {
  return( f_text.compare( x , P.size() , P ) == 0 ); } ) );
#endif
 return( R );
 }

inline std::size_t MoveRIndex::sizeInBits( void ) const
{
 return( f_lf.sizeInBits() + f_tag.sizeInBits() + f_phi.sizeInBits() +
         f_end.size() + f_endPhi.size() + 64 * f_cnt.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MoveRIndex.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
