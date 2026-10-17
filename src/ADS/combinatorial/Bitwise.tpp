/*--------------------------------------------------------------------------*/
/*---------------------------- File Bitwise.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Bitwise.h, with their contracts.
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

constexpr int sidewaysSum( Word x )
{
 x = x - ( ( x >> 1 ) & 0x5555555555555555ULL );            // 2-bit fields
 x = ( x & 0x3333333333333333ULL ) + ( ( x >> 2 ) & 0x3333333333333333ULL );
 x = ( x + ( x >> 4 ) ) & 0x0F0F0F0F0F0F0F0FULL;            // bytes
 return( int( ( x * 0x0101010101010101ULL ) >> 56 ) );      // their sum
 }

constexpr int ruler( Word x )
{
 ADS_PRE( x != 0 );
 return( sidewaysSum( x ^ ( x - 1 ) ) - 1 );   // the trailing zeros and 1
 }

constexpr int lambda( Word x )
{
 ADS_PRE( x != 0 );
 int r = 0;                                 // by halving the width
 for( int s = 32 ; s > 0 ; s /= 2 )
  if( x >> s ) {
   x >>= s;
   r += s;
   }
 return( r );
 }

constexpr Word rightmostOne( Word x )
{
 return( x & ( ~x + 1 ) );                  // x & -x
 }

constexpr Word smearRight( Word x )
{
 return( x ? x | ( x - 1 ) : 0 );
 }

constexpr Word reverseBits( Word x )
{
 constexpr Word m[ 5 ] = { 0x5555555555555555ULL , 0x3333333333333333ULL ,
                           0x0F0F0F0F0F0F0F0FULL , 0x00FF00FF00FF00FFULL ,
                           0x0000FFFF0000FFFFULL };
 for( int k = 0 , s = 1 ; k < 5 ; ++k , s *= 2 )   // swap the fields of s
  x = ( ( x >> s ) & m[ k ] ) | ( ( x & m[ k ] ) << s );
 return( ( x >> 32 ) | ( x << 32 ) );
 }

constexpr Word gray( Word x )
{
 return( x ^ ( x >> 1 ) );
 }

constexpr Word grayInverse( Word g )
{
 for( int s = 1 ; s < 64 ; s *= 2 )         // the prefix XOR of the bits
  g ^= g >> s;
 return( g );
 }

constexpr Word nextSameWeight( Word x )
{
 ADS_PRE( x != 0 );
 const Word u = rightmostOne( x ) , v = x + u;
 ADS_PRE( v != 0 );                         // no overflow
 return( v + ( ( ( v ^ x ) / u ) >> 2 ) );
 }

namespace detail {

/// the 32 bits of x spread to the even places of a word
constexpr Word spread( Word x )
{
 x = ( x | ( x << 16 ) ) & 0x0000FFFF0000FFFFULL;
 x = ( x | ( x << 8 ) ) & 0x00FF00FF00FF00FFULL;
 x = ( x | ( x << 4 ) ) & 0x0F0F0F0F0F0F0F0FULL;
 x = ( x | ( x << 2 ) ) & 0x3333333333333333ULL;
 return( ( x | ( x << 1 ) ) & 0x5555555555555555ULL );
 }

/// the even places of x gathered into 32 bits
constexpr Word gather( Word x )
{
 x &= 0x5555555555555555ULL;
 x = ( x | ( x >> 1 ) ) & 0x3333333333333333ULL;
 x = ( x | ( x >> 2 ) ) & 0x0F0F0F0F0F0F0F0FULL;
 x = ( x | ( x >> 4 ) ) & 0x00FF00FF00FF00FFULL;
 x = ( x | ( x >> 8 ) ) & 0x0000FFFF0000FFFFULL;
 return( ( x | ( x >> 16 ) ) & 0x00000000FFFFFFFFULL );
 }

}  // end( namespace detail )

constexpr Word interleave( std::uint32_t x , std::uint32_t y )
{
 return( detail::spread( x ) | ( detail::spread( y ) << 1 ) );
 }

constexpr std::pair< std::uint32_t , std::uint32_t > deinterleave( Word z )
{
 return( std::pair( std::uint32_t( detail::gather( z ) ) ,
                    std::uint32_t( detail::gather( z >> 1 ) ) ) );
 }

constexpr bool hasZeroByte( Word x )
{
 return( ( ( x - 0x0101010101010101ULL ) & ~x &
           0x8080808080808080ULL ) != 0 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Bitwise.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
