/*--------------------------------------------------------------------------*/
/*------------------------- File BinaryInteger.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of BinaryInteger.h [see BinaryInteger.h].
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

/// each item of X is 0 or 1
inline bool bits( const Bits & X )
{
 return( std::all_of( X.begin() , X.end() , []( int b ) {
  return( ( b == 0 ) || ( b == 1 ) ); } ) );
 }

}  // end( namespace detail )

inline Bits toBits( std::uint64_t x , std::size_t n )
{
 const std::uint64_t x0 = x;
 Bits b;
 for( ; x || ( b.size() < n ) ; x >>= 1 )
  b.push_back( int( x & 1 ) );
 ADS_POST( ( b.size() >= n ) &&
           ( ( b.size() > 64 ) || ( fromBits( b ) == x0 ) ) );
 return( b );
 }

inline std::uint64_t fromBits( const Bits & b )
{
 ADS_PRE( b.size() <= 64 );
 ADS_AUDIT( detail::bits( b ) );
 std::uint64_t x = 0;
 for( std::size_t i = b.size() ; i-- > 0 ; )
  x = ( x << 1 ) | std::uint64_t( b[ i ] );
 return( x );
 }

inline Bits carry( const std::vector< long > & c )
{
 Bits b;
 long r = 0;
 for( std::size_t i = 0 ; ( i < c.size() ) || r ; ++i ) {
  long v = r + ( i < c.size() ? c[ i ] : 0 );
  long bit = v % 2;
  if( bit < 0 ) bit += 2;
  b.push_back( int( bit ) );
  r = ( v - bit ) / 2;
  }
 while( ( b.size() > 1 ) && ( b.back() == 0 ) )
  b.pop_back();
 return( b );
 }

inline Bits binaryAdd( const Bits & X , const Bits & Y )
{
 ADS_AUDIT( detail::bits( X ) && detail::bits( Y ) );
 Bits Z;
 int r = 0;
 for( std::size_t i = 0 ; ( i < X.size() ) || ( i < Y.size() ) ; ++i ) {
  const int s = ( i < X.size() ? X[ i ] : 0 ) +
                ( i < Y.size() ? Y[ i ] : 0 ) + r;
  Z.push_back( s & 1 );
  r = s >> 1;
  }
 if( r )
  Z.push_back( r );
 return( Z );
 }

inline std::vector< long > widen( const Bits & X )
{
 return( std::vector< long >( X.begin() , X.end() ) );
 }

inline Bits multiplyBinarySchool( const Bits & X , const Bits & Y ,
                            long * mults )
{
 ADS_AUDIT( detail::bits( X ) && detail::bits( Y ) );
 return( carry( multiplyNaive( widen( X ) , widen( Y ) , mults ) ) );
 }

inline Bits multiplyBinaryFour( const Bits & X , const Bits & Y ,
                          long * mults )
{
 ADS_AUDIT( detail::bits( X ) && detail::bits( Y ) );
 return( carry( multiplyFour( widen( X ) , widen( Y ) , mults ) ) );
 }

inline Bits multiplyBinaryKaratsuba( const Bits & X , const Bits & Y ,
                               long * mults )
{
 ADS_AUDIT( detail::bits( X ) && detail::bits( Y ) );
 return( carry( multiplyKaratsuba( widen( X ) , widen( Y ) , mults ) ) );
 }

inline std::uint64_t ahmes( std::uint64_t a , std::uint64_t b ,
                            long * steps )
{
 const std::uint64_t a0 = a , b0 = b;
 std::uint64_t p = 0;
 for( ; a ; a >>= 1 , b <<= 1 ) {
  if( a & 1 )
   p += b;
  if( steps )
   ++*steps;
  }
 ADS_POST( p == a0 * b0 );
 return( p );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File BinaryInteger.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
