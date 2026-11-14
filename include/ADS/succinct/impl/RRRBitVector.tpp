/*--------------------------------------------------------------------------*/
/*-------------------------- File RRRBitVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RRRBitVector.h, with their contracts.
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

inline RRRBitVector::RRRBitVector( const BitVector & b ) : f_m( b.size() )
{
 const auto & T = tables();
 const std::size_t nb = ( f_m + kB - 1 ) / kB;
 f_cls.reserve( 6 * nb );
 std::uint64_t ones = 0;
 std::vector< std::uint64_t > rank , ptr;  // packed at their widths below
 for( std::size_t j = 0 ; j < nb ; ++j ) {
  if( j % kSuper == 0 ) {
   rank.push_back( ones );
   ptr.push_back( f_off.size() );
   }
  const int len = int( std::min< std::size_t >( kB , f_m - kB * j ) );
  const std::uint64_t x = b.get( kB * j , len );
  const int c = std::popcount( x );
  f_cls.append( std::uint64_t( c ) , 6 );
  f_off.append( offset( x , c ) , T.len[ c ] );
  ones += std::uint64_t( c );
  }
 rank.push_back( ones );                   // the end closes
 ptr.push_back( f_off.size() );
 f_ones = std::size_t( ones );
 f_wr = std::max( 1 , int( std::bit_width( ones ) ) );
 f_wp = std::max( 1 , int( std::bit_width( ptr.back() ) ) );
 for( std::size_t s = 0 ; s < rank.size() ; ++s ) {
  f_rank.append( rank[ s ] , f_wr );
  f_ptr.append( ptr[ s ] , f_wp );
  }
 ADS_AUDIT( std::equal( b.begin() , b.end() ,
                        SubscriptIterator< RRRBitVector , bool >( this ,
                                                                  0 ) ) );
 }

inline int RRRBitVector::below( std::uint64_t o , int c , int q )
{
 const auto & T = tables();
 int t = c;
 for( int p = kB - 1 ; ( p >= q ) && ( t > 0 ) ; --p ) {   // C( p , 0 ) = 1
  const std::uint64_t v = T.C[ p ][ t ];
  const bool one = o >= v;
  o -= one ? v : 0;
  t -= one;
  }
 return( t );
 }

inline std::uint64_t RRRBitVector::locate( std::size_t j , int & c ,
                                           std::size_t & ones ) const
{
 const auto & T = tables();
 const std::size_t s = j / kSuper;
 ones = std::size_t( f_rank.get( s * std::size_t( f_wr ) , f_wr ) );
 std::size_t ptr = std::size_t( f_ptr.get( s * std::size_t( f_wp ) , f_wp ) );
 // the classes before j, 10 of them a load
 for( std::size_t q = s * kSuper ; q < j ; ) {
  const std::size_t k = std::min< std::size_t >( 10 , j - q );
  std::uint64_t w = f_cls.get( 6 * q , int( 6 * k ) );
  for( std::size_t h = 0 ; h < k ; ++h , w >>= 6 ) {
   const std::size_t cq = std::size_t( w & 63 );
   ones += cq;
   ptr += T.len[ cq ];
   }
  q += k;
  }
 c = cls( j );
 return( f_off.get( ptr , T.len[ c ] ) );
 }

inline bool RRRBitVector::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_m );
 int c;
 std::size_t ones;
 const std::uint64_t o = locate( i / kB , c , ones );
 const int q = int( i % kB );
 return( below( o , c , q ) != below( o , c , q + 1 ) );
 }

inline std::size_t RRRBitVector::rank1( std::size_t i ) const
{
 ADS_PRE( i <= f_m );
 if( i == f_m ) return( f_ones );
 int c;
 std::size_t ones;
 const std::uint64_t o = locate( i / kB , c , ones );
 return( ones + std::size_t( below( o , c , int( i % kB ) ) ) );
 }

inline std::size_t RRRBitVector::select( std::size_t k , bool b ) const
{
 const auto & T = tables();
 constexpr std::size_t kBits = kB * kSuper;   // the bits of a superblock
 auto before = [ & ]( std::size_t s ) {     // the bits b before s
  const std::size_t o = std::size_t( f_rank.get( s * std::size_t( f_wr ) ,
                                                 f_wr ) );
  return( b ? o : std::min( s * kBits , f_m ) - o );
  };
 std::size_t lo = 0 , hi = f_rank.size() / std::size_t( f_wr ) - 1;
 while( lo + 1 < hi ) {                     // before( lo ) < k <= ...
  const std::size_t mid = ( lo + hi ) / 2;
  if( before( mid ) < k ) lo = mid;
  else hi = mid;
  }
 std::size_t r = k - before( lo );          // the r-th of the superblock
 std::size_t ptr = std::size_t( f_ptr.get( lo * std::size_t( f_wp ) ,
                                           f_wp ) );
 // the blocks, their classes 10 at a time from a load
 std::uint64_t w = 0;
 for( std::size_t j = lo * kSuper , h = 0 ; ; ++j , ++h , w >>= 6 ) {
  if( h % 10 == 0 ) {
   const std::size_t nb = ( f_m + kB - 1 ) / kB;
   w = f_cls.get( 6 * j , int( 6 * std::min< std::size_t >( 10 , nb - j ) ) );
   }
  const int c = int( w & 63 );
  const int len = int( std::min< std::size_t >( kB , f_m - kB * j ) );
  const std::size_t in = b ? std::size_t( c ) : std::size_t( len - c );
  if( r <= in ) {
   std::uint64_t o = f_off.get( ptr , T.len[ c ] );
   int t = c;
   if( b ) {
    // from the highest place down, the r-th 1 from the lowest is the
    // ( c - r + 1 )-th met
    for( int p = kB - 1 ; ; --p ) {
     const std::uint64_t v = T.C[ p ][ t ];
     if( o >= v ) {
      if( std::size_t( t ) == r ) return( kB * j + std::size_t( p ) );
      o -= v;
      --t;
      }
     }
    }
   if( 2 * c > len ) {
    // a dense block: from the highest place down, the r-th 0 from the
    // lowest is the ( in - r + 1 )-th met
    std::size_t met = in - r + 1;
    for( int p = kB - 1 ; ; --p ) {
     const std::uint64_t v = T.C[ p ][ t ];
     const bool one = ( t > 0 ) && ( o >= v );
     o -= one ? v : 0;
     t -= one;
     if( ! one && ( p < len ) && ( --met == 0 ) )
      return( kB * j + std::size_t( p ) );
     }
    }
   // the 1s of the block, down to the last of them, then the r-th 0
   std::uint64_t x = 0;
   for( int p = kB - 1 ; t > 0 ; --p ) {
    const std::uint64_t v = T.C[ p ][ t ];
    if( o >= v ) {
     x |= std::uint64_t( 1 ) << p;
     o -= v;
     --t;
     }
    }
   return( kB * j + std::size_t( selectInWord( ~x & detail::lowMask( len ) ,
                                               int( r - 1 ) ) ) );
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
 return( f_cls.size() + f_off.size() + f_rank.size() + f_ptr.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RRRBitVector.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
