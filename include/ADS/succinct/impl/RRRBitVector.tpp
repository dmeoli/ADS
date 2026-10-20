/*--------------------------------------------------------------------------*/
/*-------------------------- File RRRBitVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RRRBitVector.h, with their contracts.
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

inline const RRRBitVector::Tables & RRRBitVector::tables( void )
{
 static const Tables T = [] {
  Tables t;
  for( int p = 0 ; p < 64 ; ++p ) {        // Pascal's triangle
   t.C[ p ][ 0 ] = 1;
   for( int k = 1 ; k <= p ; ++k )
    t.C[ p ][ k ] = t.C[ p - 1 ][ k - 1 ] + ( k < p ? t.C[ p - 1 ][ k ] : 0 );
   }
  for( int c = 0 ; c <= kB ; ++c )
   t.len[ c ] = std::uint8_t( std::bit_width( t.C[ kB ][ c ] - 1 ) );
  return( t );
  }();
 return( T );
 }

inline std::uint64_t RRRBitVector::offset( std::uint64_t x , int c )
{
 const auto & T = tables();
 std::uint64_t o = 0;
 for( int t = 1 ; t <= c ; ++t , x &= x - 1 )   // the 1s, from the lowest
  o += T.C[ std::countr_zero( x ) ][ t ];
 return( o );
 }

inline std::uint64_t RRRBitVector::block( std::uint64_t o , int c )
{
 const auto & T = tables();
 std::uint64_t x = 0;
 int p = kB - 1;
 for( int t = c ; t >= 1 ; --t , --p ) {   // the largest p with C <= o
  while( T.C[ p ][ t ] > o ) --p;
  x |= std::uint64_t( 1 ) << p;
  o -= T.C[ p ][ t ];
  }
 return( x );
 }

inline RRRBitVector::RRRBitVector( const BitVector & b ) : f_m( b.size() )
{
 const auto & T = tables();
 const std::size_t nb = ( f_m + kB - 1 ) / kB;
 f_cls.reserve( 6 * nb );
 std::uint64_t ones = 0;
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  if( j % kSuper == 0 ) {
   f_rank.push_back( ones );
   f_ptr.push_back( f_off.size() );
   }
  const int len = int( std::min< std::size_t >( kB , f_m - kB * j ) );
  const std::uint64_t x = b.get( kB * j , len );
  const int c = std::popcount( x );
  f_cls.append( std::uint64_t( c ) , 6 );
  f_off.append( offset( x , c ) , T.len[ c ] );
  ones += std::uint64_t( c );
  }
 f_rank.push_back( ones );                 // the end closes
 f_ptr.push_back( f_off.size() );
 f_ones = std::size_t( ones );
 ADS_AUDIT( std::equal( b.begin() , b.end() ,
                        SubscriptIterator< RRRBitVector , bool >( this ,
                                                                  0 ) ) );
 }

inline std::uint64_t RRRBitVector::decode( std::size_t j ,
                                           std::size_t & ones ) const
{
 const auto & T = tables();
 const std::size_t s = j / kSuper;
 ones = std::size_t( f_rank[ s ] );
 std::size_t ptr = std::size_t( f_ptr[ s ] );
 for( std::size_t q = s * kSuper ; q < j ; ++q ) {
  const int c = cls( q );
  ones += std::size_t( c );
  ptr += T.len[ c ];
  }
 const int c = cls( j );
 return( block( f_off.get( ptr , T.len[ c ] ) , c ) );
 }

inline bool RRRBitVector::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_m );
 std::size_t ones;
 return( ( decode( i / kB , ones ) >> ( i % kB ) ) & 1 );
 }

inline std::size_t RRRBitVector::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_m );
 if( i == f_m ) return( f_ones );
 std::size_t ones;
 const std::uint64_t x = decode( i / kB , ones );
 return( ones + std::size_t( std::popcount( x & detail::lowMask(
                                                 int( i % kB ) ) ) ) );
 }

inline std::size_t RRRBitVector::select( std::size_t k , bool b ) const
{
 const auto & T = tables();
 constexpr std::size_t kBits = kB * kSuper;   // the bits of a superblock
 auto before = [ & ]( std::size_t s ) {     // the bits b before s
  const std::size_t o = std::size_t( f_rank[ s ] );
  return( b ? o : std::min( s * kBits , f_m ) - o );
  };
 std::size_t lo = 0 , hi = f_rank.size() - 1;
 while( lo + 1 < hi ) {                     // before( lo ) < k <= ...
  const std::size_t mid = ( lo + hi ) / 2;
  if( before( mid ) < k ) lo = mid;
  else hi = mid;
  }
 std::size_t r = k - before( lo );          // the r-th of the superblock
 std::size_t ptr = std::size_t( f_ptr[ lo ] );
 for( std::size_t j = lo * kSuper ; ; ++j ) {
  const int c = cls( j );
  const int len = int( std::min< std::size_t >( kB , f_m - kB * j ) );
  const std::size_t in = b ? std::size_t( c ) : std::size_t( len - c );
  if( r <= in ) {
   const std::uint64_t x = block( f_off.get( ptr , T.len[ c ] ) , c );
   const std::uint64_t y = b ? x : ~x & detail::lowMask( len );
   return( kB * j + std::size_t( selectInWord( y , int( r - 1 ) ) ) );
   }
  r -= in;
  ptr += T.len[ c ];
  }
 }

inline std::size_t RRRBitVector::select1( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= f_ones ) );
 const std::size_t p = select( k , true );
 ADS_POST( ( *this )[ p ] && ( rank1( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t RRRBitVector::select0( std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= zeros() ) );
 const std::size_t p = select( k , false );
 ADS_POST( ! ( *this )[ p ] && ( rank0( p ) == k - 1 ) );
 return( p );
 }

inline std::size_t RRRBitVector::sizeInBits( void ) const
{
 return( f_cls.size() + f_off.size() + 64 * ( f_rank.size() +
                                               f_ptr.size() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RRRBitVector.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
