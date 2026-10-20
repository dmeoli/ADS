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

/// for each byte, its bit 0 first: its excess, the least excess of its
/// prefixes of 1 to 8 bits, the greatest excess of its suffixes, and the
/// length of the shortest prefix of excess -k and of the shortest suffix of
/// excess k, for k = 1 , ... , 8 (0 if none)
struct ByteExcess {
 std::array< std::int8_t , 256 > exc , min , max;
 std::array< std::array< std::uint8_t , 8 > , 256 > pre , suf;
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
    if( ( e < 0 ) && ( t.pre[ v ][ -e - 1 ] == 0 ) )
     t.pre[ v ][ -e - 1 ] = std::uint8_t( i + 1 );
    }
   t.exc[ v ] = std::int8_t( e );
   t.min[ v ] = std::int8_t( m );
   int s = 0 , M = -8;
   for( int i = 7 ; i >= 0 ; --i ) {
    s += ( ( v >> i ) & 1 ) ? 1 : -1;
    M = s > M ? s : M;
    if( ( s > 0 ) && ( t.suf[ v ][ s - 1 ] == 0 ) )
     t.suf[ v ][ s - 1 ] = std::uint8_t( 8 - i );
    }
   t.max[ v ] = std::int8_t( M );
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
 const std::size_t m = f_b.size();
 f_nleaf = ( m + kLeaf - 1 ) / kLeaf;
 const std::size_t ng = ( f_nleaf + kGroup - 1 ) / kGroup;
 while( f_groups < ng ) f_groups *= 2;
 f_lexc.assign( f_nleaf , 0 );
 f_lmin.assign( f_nleaf , 0 );
 f_exc.assign( 2 * f_groups , 0 );
 f_min.assign( 2 * f_groups , 0 );
 for( std::size_t l = 0 ; l < f_nleaf ; ++l ) {   // the leaves, by a scan
  std::int64_t e = 0 , mn = 0;             // a byte at a time, by tables
  const auto & T = detail::byteExcess();
  std::size_t i = l * kLeaf;
  for( ; i + 8 <= leafEnd( l ) ; i += 8 ) {
   const auto v = f_b.bits().get( i , 8 );
   mn = std::min( mn , e + T.min[ v ] );
   e += T.exc[ v ];
   }
  for( ; i < leafEnd( l ) ; ++i ) {
   e += f_b[ i ] ? 1 : -1;
   mn = std::min( mn , e );
   }
  f_lexc[ l ] = std::int16_t( e );
  f_lmin[ l ] = std::int16_t( mn );
  const std::size_t g = f_groups + l / kGroup;
  f_min[ g ] = std::min( f_min[ g ] , f_exc[ g ] + mn );
  f_exc[ g ] += e;
  }
 for( std::size_t v = f_groups - 1 ; v >= 1 ; --v ) {
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

inline std::int64_t BalancedParentheses::excess( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( 2 * std::int64_t( f_b.rank1( i + 1 ) ) - std::int64_t( i + 1 ) );
 }

inline std::size_t BalancedParentheses::scanForward( std::size_t a ,
                                                     std::size_t b ,
                                                     std::int64_t & e ,
                                                     std::int64_t d ) const
{
 const auto & T = detail::byteExcess();
 const BitVector & B = f_b.bits();
 std::size_t j = a;
 for( ; j + 8 <= b ; j += 8 ) {             // 8 bits at a time, anywhere
  const auto v = B.get( j , 8 );
  if( e + T.min[ v ] <= d )
   return( j + T.pre[ v ][ e - d - 1 ] - 1 );
  e += T.exc[ v ];
  }
 for( ; j < b ; ++j )
  if( ( e += B[ j ] ? 1 : -1 ) <= d ) return( j );
 return( nil );
 }

inline std::size_t BalancedParentheses::scanBackward( std::size_t a ,
                                                      std::size_t b ,
                                                      std::int64_t & e ,
                                                      std::int64_t d ) const
{
 const auto & T = detail::byteExcess();
 const BitVector & B = f_b.bits();
 std::size_t x = b + 1;                     // the bits b , b - 1 , ... , a
 for( ; x >= a + 8 ; x -= 8 ) {             // 8 bits at a time, anywhere
  const auto v = B.get( x - 8 , 8 );
  if( e - T.max[ v ] <= d )
   return( x - T.suf[ v ][ e - d - 1 ] );
  e -= T.exc[ v ];
  }
 for( ; x > a ; --x )
  if( ( e -= B[ x - 1 ] ? 1 : -1 ) <= d ) return( x - 1 );
 return( nil );
 }

inline std::size_t BalancedParentheses::forwardSearch( std::size_t i ,
                                                       std::int64_t d ) const
{
 std::int64_t e = 0;                        // E( . ) - E( i )
 std::size_t l = i / kLeaf;
 if( const auto j = scanForward( i + 1 , leafEnd( l ) , e , d ) ; j != nil )
  return( j );
 // the next leaves of the group, then the tree of the groups
 const std::size_t last = std::min( f_nleaf , ( l / kGroup + 1 ) * kGroup );
 for( ++l ; l < last ; ++l ) {
  if( e + f_lmin[ l ] <= d )
   return( scanForward( l * kLeaf , leafEnd( l ) , e , d ) );
  e += f_lexc[ l ];
  }
 std::size_t v = f_groups + ( l - 1 ) / kGroup;
 for( ; v > 1 ; v /= 2 )                    // up to a right range with d
  if( v % 2 == 0 ) {
   if( e + f_min[ v + 1 ] <= d ) break;
   e += f_exc[ v + 1 ];
   }
 if( v == 1 ) return( nil );
 for( ++v ; v < f_groups ; )                // down to its first group
  if( e + f_min[ 2 * v ] <= d ) v = 2 * v;
  else {
   e += f_exc[ 2 * v ];
   v = 2 * v + 1;
   }
 for( l = ( v - f_groups ) * kGroup ; e + f_lmin[ l ] > d ; ++l )
  e += f_lexc[ l ];
 return( scanForward( l * kLeaf , leafEnd( l ) , e , d ) );
 }

inline std::size_t BalancedParentheses::backwardSearch( std::size_t i ,
                                                        std::int64_t d ) const
{
 // E( . ) - E( i ), from E( i - 1 ) on, so that it is above d
 std::int64_t e = f_b[ i ] ? -1 : 1;
 if( e <= d ) return( i );
 if( i == 0 ) return( nil );
 std::size_t l = i / kLeaf;
 if( const auto k = scanBackward( l * kLeaf , i - 1 , e , d ) ; k != nil )
  return( k );
 // the leaves before in the group, then the tree of the groups; e is the
 // excess before the range, and a range of excess x and least excess m
 // holds a place with d if e - x + m <= d
 const std::size_t first = ( l / kGroup ) * kGroup;
 for( ; l > first ; --l ) {
  if( e - f_lexc[ l - 1 ] + f_lmin[ l - 1 ] <= d )
   return( scanBackward( ( l - 1 ) * kLeaf , leafEnd( l - 1 ) - 1 , e ,
                         d ) );
  e -= f_lexc[ l - 1 ];
  }
 std::size_t v = f_groups + l / kGroup;
 for( ; v > 1 ; v /= 2 )                    // up to a left range with d
  if( v % 2 == 1 ) {
   if( e - f_exc[ v - 1 ] + f_min[ v - 1 ] <= d ) break;
   e -= f_exc[ v - 1 ];
   }
 if( v == 1 ) return( nil );
 for( --v ; v < f_groups ; )                // down to its last group
  if( e - f_exc[ 2 * v + 1 ] + f_min[ 2 * v + 1 ] <= d ) v = 2 * v + 1;
  else {
   e -= f_exc[ 2 * v + 1 ];
   v = 2 * v;
   }
 l = std::min( f_nleaf , ( v - f_groups + 1 ) * kGroup );
 for( ; e - f_lexc[ l - 1 ] + f_lmin[ l - 1 ] > d ; --l )
  e -= f_lexc[ l - 1 ];
 return( scanBackward( ( l - 1 ) * kLeaf , leafEnd( l - 1 ) - 1 , e , d ) );
 }

inline std::size_t BalancedParentheses::findClose( std::size_t i ) const
{
 ADS_PRE( ( i < size() ) && f_b[ i ] );
 const std::size_t j = forwardSearch( i , -1 );
 ADS_POST( ( j != nil ) && ! f_b[ j ] && ( excess( j ) == excess( i ) - 1 ) );
 return( j );
 }

inline std::size_t BalancedParentheses::findOpen( std::size_t j ) const
{
 ADS_PRE( ( j < size() ) && ! f_b[ j ] );
 const std::size_t i = backwardSearch( j , 0 );
 ADS_POST( ( i != nil ) && ( findClose( i ) == j ) );
 return( i );
 }

inline std::size_t BalancedParentheses::enclose( std::size_t i ) const
{
 ADS_PRE( ( i < size() ) && f_b[ i ] );
 const std::size_t p = backwardSearch( i , -2 );   // nil at the outer level
 ADS_POST( ( p == nil ) ? excess( i ) == 1 :
           f_b[ p ] && ( p < i ) && ( findClose( p ) > i ) );
 return( p );
 }

inline const CompactRankSelect & BalancedParentheses::bits( void ) const
{
 return( f_b );
 }

inline std::size_t BalancedParentheses::treeBits( void ) const
{
 return( 16 * ( f_lexc.size() + f_lmin.size() ) +
         64 * ( f_exc.size() + f_min.size() ) );
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
