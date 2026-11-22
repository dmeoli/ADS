/*--------------------------------------------------------------------------*/
/*-------------------- File InterleavedRankSelect.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of InterleavedRankSelect.h, with their contracts.
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

namespace ADS
{

inline InterleavedRankSelect::InterleavedRankSelect( const BitVector & b )
{
 build( b.words().data() , b.size() );
 }

inline InterleavedRankSelect::InterleavedRankSelect( std::span< const Word > w ,
						     std::size_t n )
{
 ADS_PRE( w.size() >= ( n + 63 ) / 64 );
 build( w.data() , n );
 }

inline void InterleavedRankSelect::build( const Word * w , std::size_t n )
{
 f_n = n;
 const std::size_t lines = n / kData + 1;  // the one of n too
 f_L.clear();
 f_L.resize( 8 * lines );  // each word written below
 // the bits [ t , t + len ) of w, len <= 64 and t + len <= n
 auto read = [ & ]( std::size_t t , int len ) {
  const std::size_t q = t / 64;
  const unsigned o = unsigned( t % 64 );
  Word x = w[ q ] >> o;
  if( o + unsigned( len ) > 64 )
   x |= w[ q + 1 ] << ( 64 - o );
  return( x & detail::lowMask( len ) );
  };
 // a superblock at a time, its lines in the L1 cache: first the bits of
 // each line from its bit 16, a loop with no dependence between the lines,
 // then the count of each line since its superblock in its bits 0 to 15
 // and the samples, the 1s carried from a line to the next
 std::size_t c = 0 , next[ 2 ] = { 1 , 1 };
 const std::size_t nw = ( n + 63 ) / 64;
 for( std::size_t l0 = 0 ; l0 < lines ; l0 += kLines ) {
  const std::size_t l1 = std::min( lines , l0 + kLines );
  for( std::size_t l = l0 ; l < l1 ; ++l ) {
   Word * W = f_L.data() + 8 * l;
   // the word k of the line is the 64 bits from t + 64 k of w, t the bit
   // 16 before the line, the 16 of the count then cleared
   const std::size_t t = l * kData - 16 , q = t / 64;
   const unsigned o = unsigned( t % 64 );
   if( ( l > 0 ) && ( q + 8 < nw ) ) {
#ifdef ADS_WITH_HIGHWAY  // a vector at a time [see SimdInline.h]
    detail::simd::here::shiftedWords8( w + q , o , W );
#else
    for( std::size_t k = 0 ; k < 8 ; ++k )
     W[ k ] = ( w[ q + k ] >> o ) | ( ( w[ q + k + 1 ] << 1 ) << ( 63 - o ) );
#endif
    W[ 0 ] &= ~Word( 0xFFFF );
    }
   else {
    std::fill( W , W + 8 , Word( 0 ) );
    for( std::size_t k = 0 ; k < 8 ; ++k ) {
     const std::size_t off = l * kData + 64 * k;
     if( off >= n )
      break;
     const int len =
       int( std::min< std::size_t >( k < 7 ? 64 : 48 , n - off ) );
     const Word piece = read( off , len );
     W[ k ] |= piece << 16;
     if( k < 7 )
      W[ k + 1 ] |= piece >> 48;
     }
    }
   }
  f_S.push_back( c );
  for( std::size_t l = l0 ; l < l1 ; ++l ) {
   Word * W = f_L.data() + 8 * l;
   std::size_t in = 0;
   for( std::size_t k = 0 ; k < 8 ; ++k )
    in += std::size_t( std::popcount( W[ k ] ) );
   W[ 0 ] |= Word( c - std::size_t( f_S.back() ) );
   // the samples: the line of every 8192-th bit b, with the bits b before
   // the line (the 0s only those before n)
   const std::size_t b1 = c , e1 = c + in , b0 = l * kData - c ,
		     e0 = std::min( n , ( l + 1 ) * kData ) - e1;
   for( ; next[ 1 ] <= e1 ; next[ 1 ] += kSample ) {
    f_sl[ 1 ].push_back( std::uint32_t( l ) );
    f_sc[ 1 ].push_back( b1 );
    }
   for( ; next[ 0 ] <= e0 ; next[ 0 ] += kSample ) {
    f_sl[ 0 ].push_back( std::uint32_t( l ) );
    f_sc[ 0 ].push_back( b0 );
    }
   c = e1;
   }
  }
 f_ones = c;
 // the last line closes the samples
 const std::size_t last = lines - 1 , ob = before( last );
 for( const bool b : { false , true } ) {
  f_sl[ b ].push_back( std::uint32_t( last ) );
  f_sc[ b ].push_back( b ? ob : last * kData - ob );
  }
 ADS_AUDIT( consistent() );
 }

inline bool InterleavedRankSelect::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 const std::size_t p = i % kData + 16;
 return( ( f_L[ 8 * ( i / kData ) + p / 64 ] >> ( p % 64 ) ) & 1 );
 }

inline std::size_t InterleavedRankSelect::inLine( std::size_t l ,
						  unsigned p ) const
{
 // the bits of the line before p [see detail::lineRank()], less the count
 const Word * W = f_L.data() + 8 * l;
 return( detail::lineRank( W , p ) -
	  std::size_t( std::popcount( W[ 0 ] & 0xFFFF ) ) );
 }

inline std::size_t InterleavedRankSelect::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_n );
 const std::size_t l = i / kData;
 return( before( l ) + inLine( l , unsigned( i % kData + 16 ) ) );
 }

inline std::size_t InterleavedRankSelect::select( std::size_t k ,
						  bool b ) const
{
 auto beforeB = [ & ]( std::size_t l ) {
  const std::size_t o = before( l );
  return( b ? o : l * kData - o );
  };
 const std::size_t j = ( k - 1 ) / kSample;
 const std::size_t lo = f_sl[ b ][ j ] , hi = f_sl[ b ][ j + 1 ];
 const std::size_t clo = std::size_t( f_sc[ b ][ j ] ) ,
		   chi = std::size_t( f_sc[ b ][ j + 1 ] );
 // the guess by interpolation between the samples, then a line at a time
 std::size_t l =
   lo + ( chi > clo ? ( k - 1 - clo ) * ( hi - lo ) / ( chi - clo ) : 0 );
 l = std::min( l , hi );
 while( ( l > lo ) && ( beforeB( l ) >= k ) )
  --l;
 while( ( l < hi ) && ( beforeB( l + 1 ) < k ) )
  ++l;
 // the word of the line, and the bit in it
 std::size_t r = k - beforeB( l );
 const Word * W = f_L.data() + 8 * l;
 for( std::size_t q = 0 ; ; ++q ) {
  Word x = b ? W[ q ] : ~W[ q ];
  if( q == 0 )
   x &= ~Word( 0xFFFF );
  const std::size_t c = std::size_t( std::popcount( x ) );
  if( r <= c )
   return( l * kData + 64 * q +
	    std::size_t( selectInWord( x , int( r - 1 ) ) ) - 16 );
  r -= c;
  }
 }

inline std::size_t InterleavedRankSelect::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 return( detail::selected( *this , k , true , select( k , true ) ) );
 }

inline std::size_t InterleavedRankSelect::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 return( detail::selected( *this , k , false , select( k , false ) ) );
 }

inline std::size_t InterleavedRankSelect::directoryBits( void ) const
{
 return( 64 * f_L.size() - f_n + 64 * f_S.size() +
	  96 * ( f_sl[ 0 ].size() + f_sl[ 1 ].size() ) );
 }

inline bool InterleavedRankSelect::consistent( void ) const
{
 return( detail::rankConsistent( *this ) );
 }

/*--------------------------------------------------------------------------*/

template< class A >
inline void InterleavedRankSelect::persist( A & a )
{
 a( f_n , f_ones , f_L , f_S , f_sl , f_sc );
 const std::size_t lines = f_n / kData + 1;
 a.require( ( f_L.size() == 8 * lines ) && ( f_ones <= f_n ) &&
	      ( f_S.size() == ( lines - 1 ) / kLines + 1 ) &&
	      ( f_sl[ 0 ].size() == f_sc[ 0 ].size() ) &&
	      ( f_sl[ 1 ].size() == f_sc[ 1 ].size() ) ,
	    "the lines of an InterleavedRankSelect" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File InterleavedRankSelect.tpp -------------------*/
/*--------------------------------------------------------------------------*/
