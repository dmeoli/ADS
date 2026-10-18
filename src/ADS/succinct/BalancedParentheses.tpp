/*--------------------------------------------------------------------------*/
/*---------------------- File BalancedParentheses.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BalancedParentheses.h, with their contracts.
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

namespace detail {

/// the excess of each byte, its bit 0 first, and its minimum prefix excess
struct ByteExcess {
 std::array< std::int8_t , 256 > exc;
 std::array< std::int8_t , 256 > min;
 };

inline const ByteExcess & byteExcess( void )
{
 static const ByteExcess T = [] {
  ByteExcess t{};
  for( int v = 0 ; v < 256 ; ++v ) {
   int e = 0 , m = 8;
   for( int i = 0 ; i < 8 ; ++i ) {
    e += ( ( v >> i ) & 1 ) ? 1 : -1;
    m = e < m ? e : m;
    }
   t.exc[ v ] = std::int8_t( e );
   t.min[ v ] = std::int8_t( m );
   }
  return( t );
  }();
 return( T );
 }

}  // end( namespace detail )

inline BalancedParentheses::BalancedParentheses( void )
 : BalancedParentheses( BitVector() )
{
 }

inline BalancedParentheses::BalancedParentheses( BitVector b )
 : f_b( std::move( b ) )
{
 ADS_AUDIT( balanced() );
 constexpr std::int64_t kInf = std::int64_t( 1 ) << 60;   // no place
 const std::size_t m = f_b.size() , nb = ( m + kBlock - 1 ) / kBlock;
 while( f_leaves < nb ) f_leaves *= 2;
 f_exc.assign( 2 * f_leaves , 0 );
 f_min.assign( 2 * f_leaves , kInf );
 for( std::size_t k = 0 ; k < nb ; ++k ) {   // the leaves, by a scan
  std::int64_t e = 0 , mn = kInf;
  for( std::size_t i = k * kBlock ; i < std::min( m , ( k + 1 ) * kBlock ) ;
       ++i ) {
   e += f_b[ i ] ? 1 : -1;
   mn = std::min( mn , e );
   }
  f_exc[ f_leaves + k ] = e;
  f_min[ f_leaves + k ] = mn;
  }
 for( std::size_t v = f_leaves - 1 ; v >= 1 ; --v ) {
  f_exc[ v ] = f_exc[ 2 * v ] + f_exc[ 2 * v + 1 ];
  f_min[ v ] = std::min( f_min[ 2 * v ] ,
                         f_exc[ 2 * v ] + f_min[ 2 * v + 1 ] );
  }
 }

inline std::size_t BalancedParentheses::size( void ) const
{
 return( f_b.size() );
 }

inline bool BalancedParentheses::operator[]( std::size_t i ) const
{
 return( f_b[ i ] );
 }

inline std::int64_t BalancedParentheses::excessBefore( std::size_t p ) const
{
 return( 2 * std::int64_t( f_b.rank1( p ) ) - std::int64_t( p ) );
 }

inline std::int64_t BalancedParentheses::excess( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( excessBefore( i + 1 ) );
 }

inline std::size_t BalancedParentheses::scanForward( std::size_t a ,
                                                     std::size_t b ,
                                                     std::int64_t t ) const
{
 const auto & T = detail::byteExcess();
 std::int64_t e = excessBefore( a );
 for( std::size_t j = a ; j < b ; ) {
  if( ( j % 8 == 0 ) && ( j + 8 <= b ) ) {  // a byte that misses t
   const auto v = f_b.bits().get( j , 8 );
   if( e + T.min[ v ] > t ) {
    e += T.exc[ v ];
    j += 8;
    continue;
    }
   }
  e += f_b[ j ] ? 1 : -1;
  if( e <= t ) return( j );
  ++j;
  }
 return( nil );
 }

inline std::size_t BalancedParentheses::scanBackward( std::size_t a ,
                                                      std::size_t b ,
                                                      std::int64_t t ) const
{
 const auto & T = detail::byteExcess();
 std::int64_t e = excessBefore( b );        // E( k - 1 ) below
 for( std::size_t k = b ; k > a ; ) {
  if( ( k % 8 == 0 ) && ( k >= a + 8 ) ) {  // a byte that misses t
   const auto v = f_b.bits().get( k - 8 , 8 );
   if( e - T.exc[ v ] + T.min[ v ] > t ) {
    e -= T.exc[ v ];
    k -= 8;
    continue;
    }
   }
  if( e <= t ) return( k );
  e -= f_b[ k - 1 ] ? 1 : -1;
  --k;
  }
 return( 0 );
 }

inline std::size_t BalancedParentheses::forwardSearch( std::size_t i ,
                                                       std::int64_t t ) const
{
 const std::size_t m = size() , b = i / kBlock;
 const std::size_t end = std::min( m , ( b + 1 ) * kBlock );
 if( const std::size_t j = scanForward( i + 1 , end , t ) ; j != nil )
  return( j );
 std::int64_t e = excessBefore( end );      // E at the end of the range
 std::size_t v = f_leaves + b;
 for( ; v > 1 ; v /= 2 )                    // up to a right range with t
  if( v % 2 == 0 ) {
   if( e + f_min[ v + 1 ] <= t ) break;
   e += f_exc[ v + 1 ];
   }
 if( v == 1 ) return( nil );
 for( v = v + 1 ; v < f_leaves ; )          // down to its first block
  if( e + f_min[ 2 * v ] <= t ) v = 2 * v;
  else {
   e += f_exc[ 2 * v ];
   v = 2 * v + 1;
   }
 const std::size_t k = v - f_leaves;
 return( scanForward( k * kBlock , std::min( m , ( k + 1 ) * kBlock ) , t ) );
 }

inline std::size_t BalancedParentheses::backwardSearch( std::size_t i ,
                                                        std::int64_t t ) const
{
 const std::size_t m = size() , b = i / kBlock;
 if( const std::size_t q = scanBackward( b * kBlock , i , t ) ; q > 0 )
  return( q );
 std::int64_t e = excessBefore( b * kBlock );   // E before the range
 std::size_t v = f_leaves + b;
 for( ; v > 1 ; v /= 2 )                    // up to a left range with t
  if( v % 2 == 1 ) {
   if( e - f_exc[ v - 1 ] + f_min[ v - 1 ] <= t ) break;
   e -= f_exc[ v - 1 ];
   }
 if( v == 1 ) return( t >= 0 ? 0 : nil );   // E( -1 ) = 0
 v = v - 1;
 e -= f_exc[ v ];                           // E before the range of v
 while( v < f_leaves )                      // down to its last block
  if( e + f_exc[ 2 * v ] + f_min[ 2 * v + 1 ] <= t ) {
   e += f_exc[ 2 * v ];
   v = 2 * v + 1;
   }
  else v = 2 * v;
 const std::size_t k = v - f_leaves;
 return( scanBackward( k * kBlock , std::min( m , ( k + 1 ) * kBlock ) , t ) );
 }

inline std::size_t BalancedParentheses::findClose( std::size_t i ) const
{
 ADS_PRE( ( i < size() ) && f_b[ i ] );
 const std::size_t j = forwardSearch( i , excess( i ) - 1 );
 ADS_POST( ( j != nil ) && ! f_b[ j ] && ( excess( j ) == excess( i ) - 1 ) );
 return( j );
 }

inline std::size_t BalancedParentheses::findOpen( std::size_t j ) const
{
 ADS_PRE( ( j < size() ) && ! f_b[ j ] );
 const std::size_t i = backwardSearch( j , excess( j ) );
 ADS_POST( ( i != nil ) && ( findClose( i ) == j ) );
 return( i );
 }

inline std::size_t BalancedParentheses::enclose( std::size_t i ) const
{
 ADS_PRE( ( i < size() ) && f_b[ i ] );
 const std::int64_t t = excess( i ) - 2;
 if( t < 0 ) return( nil );                 // a pair at the outer level
 const std::size_t p = backwardSearch( i , t );
 ADS_POST( f_b[ p ] && ( p < i ) && ( findClose( p ) > i ) );
 return( p );
 }

inline const RankSelect & BalancedParentheses::bits( void ) const
{
 return( f_b );
 }

inline std::size_t BalancedParentheses::treeBits( void ) const
{
 return( 64 * ( f_exc.size() + f_min.size() ) );
 }

inline bool BalancedParentheses::balanced( void ) const
{
 std::int64_t e = 0;
 for( std::size_t i = 0 ; i < f_b.size() ; ++i ) {
  e += f_b[ i ] ? 1 : -1;
  if( e < 0 ) return( false );
  }
 return( e == 0 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File BalancedParentheses.tpp --------------------*/
/*--------------------------------------------------------------------------*/
