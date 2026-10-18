/*--------------------------------------------------------------------------*/
/*--------------------------- File RankSelect.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RankSelect.h, with their contracts.
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

inline RankSelect::RankSelect( void ) : RankSelect( BitVector() )
{
 }

inline RankSelect::RankSelect( BitVector b ) : f_b( std::move( b ) )
{
 const auto & W = f_b.words();
 const std::size_t nw = W.size();
 f_super.assign( nw / kWords + 1 , 0 );
 f_rel.assign( nw + 1 , 0 );
 std::uint64_t c = 0;
 for( std::size_t w = 0 ; w <= nw ; ++w ) {
  if( w % kWords == 0 ) f_super[ w / kWords ] = c;
  f_rel[ w ] = std::uint16_t( c - f_super[ w / kWords ] );
  if( w < nw ) c += std::uint64_t( sidewaysSum( W[ w ] ) );
  }
 f_ones = std::size_t( c );
 sample( true );
 sample( false );
 ADS_AUDIT( consistent() );
 }

inline const BitVector & RankSelect::bits( void ) const
{
 return( f_b );
 }

inline std::size_t RankSelect::size( void ) const
{
 return( f_b.size() );
 }

inline bool RankSelect::operator[]( std::size_t i ) const
{
 return( f_b[ i ] );
 }

inline std::size_t RankSelect::ones( void ) const
{
 return( f_ones );
 }

inline std::size_t RankSelect::zeros( void ) const
{
 return( f_b.size() - f_ones );
 }

inline std::size_t RankSelect::onesBefore( std::size_t w ) const
{
 return( std::size_t( f_super[ w / kWords ] ) + f_rel[ w ] );
 }

inline std::size_t RankSelect::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_b.size() );
 const std::size_t w = i / 64;
 std::size_t r = onesBefore( w );
 if( i % 64 )
  r += std::size_t( sidewaysSum( f_b.words()[ w ] &
                                 detail::lowMask( int( i % 64 ) ) ) );
 return( r );
 }

inline std::size_t RankSelect::rank0( std::size_t i ) const
{
 return( i - rank1( i ) );
 }

inline void RankSelect::sample( bool b )
{
 auto & S = b ? f_s1 : f_s0;
 S.clear();
 const std::size_t total = b ? f_ones : zeros();
 auto before = [ & ]( std::size_t s ) {   // the bits b before superblock s
  const std::size_t o = std::size_t( f_super[ s ] );
  return( b ? o : 64 * kWords * s - o );
  };
 std::size_t s = 0;
 for( std::size_t k = 1 ; k <= total ; k += kSample ) {
  while( ( s + 1 < f_super.size() ) && ( before( s + 1 ) < k ) ) ++s;
  S.push_back( std::uint32_t( s ) );
  }
 }

inline std::size_t RankSelect::select( std::size_t k , bool b ) const
{
 const auto & S = b ? f_s1 : f_s0;
 auto before = [ & ]( std::size_t s ) {
  const std::size_t o = std::size_t( f_super[ s ] );
  return( b ? o : 64 * kWords * s - o );
  };
 const std::size_t j = ( k - 1 ) / kSample;
 std::size_t lo = S[ j ];
 std::size_t hi = j + 1 < S.size() ? S[ j + 1 ] : f_super.size() - 1;
 while( lo < hi ) {              // the last superblock with fewer than k
  const std::size_t mid = lo + ( hi - lo + 1 ) / 2;
  if( before( mid ) < k ) lo = mid;
  else hi = mid - 1;
  }
 const auto & W = f_b.words();
 std::size_t r = k - before( lo );          // the r-th one of the block
 std::size_t w = lo * kWords;
 const std::size_t last = std::min( w + kWords , W.size() );
 for( ; w + 1 < last ; ++w ) {              // the word, by relative ranks
  const std::size_t in = b ? f_rel[ w + 1 ]
                           : 64 * ( w + 1 - lo * kWords ) - f_rel[ w + 1 ];
  if( in >= r ) break;
  }
 const std::size_t in = b ? f_rel[ w ] : 64 * ( w - lo * kWords ) - f_rel[ w ];
 const Word x = b ? W[ w ] : ~W[ w ];
 return( 64 * w + std::size_t( selectInWord( x , int( r - in - 1 ) ) ) );
 }

inline std::size_t RankSelect::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 const std::size_t p = select( k , true );
 ADS_POST( f_b[ p ] && ( rank1( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t RankSelect::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 const std::size_t p = select( k , false );
 ADS_POST( ! f_b[ p ] && ( rank0( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t RankSelect::directoryBits( void ) const
{
 return( 64 * f_super.size() + 16 * f_rel.size() +
         32 * ( f_s1.size() + f_s0.size() ) );
 }

inline bool RankSelect::consistent( void ) const
{
 std::size_t c = 0;
 for( std::size_t i = 0 ; i <= f_b.size() ; ++i ) {
  if( rank1( i ) != c ) return( false );
  if( i < f_b.size() && f_b[ i ] ) ++c;
  }
 return( c == f_ones );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RankSelect.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
