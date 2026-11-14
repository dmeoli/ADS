/*--------------------------------------------------------------------------*/
/*-------------------------- File PackedArray.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PackedArray and TritArray [see PackedArray.h], with
 * their contracts.
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

/*--------------------------------------------------------------------------*/
/*----------------------------- PackedArray --------------------------------*/
/*--------------------------------------------------------------------------*/

template< unsigned B , bool Saturate >
PackedArray< B , Saturate >::PackedArray( std::size_t n , Word v )
 : f_n( n )
{
 ADS_PRE( Saturate || ( v <= MAX ) );
 if( v > MAX ) {
  v = MAX;
  f_clamped = n;
  }
 Word x = 0;                                // the word of PER fields v
 for( std::size_t j = 0 ; j < PER ; ++j )
  x |= v << ( j * B );
 f_w.assign( ( n + PER - 1 ) / PER , x );
 if( n % PER )                              // the bits past the last field
  f_w.back() &= ~Word( 0 ) >> ( 64 - ( n % PER ) * B );
 ADS_INVARIANT( clean() );
 }

template< unsigned B , bool Saturate >
template< Executor E , class F >
PackedArray< B , Saturate >::PackedArray( E & ex , std::size_t n , F && f )
 : f_w( ( n + PER - 1 ) / PER ) , f_n( n )
{
 std::atomic< std::size_t > clamped{ 0 };
 // whole words per block, so that no two threads write one word; the
 // grain keeps a block at 2048 words at least, 16 KB
 parallelForBlocks( ex , 0 , f_w.size() ,
                    [ & ]( std::size_t a , std::size_t b ) {
  std::size_t c = 0;
  auto field = [ & ]( std::size_t i ) {
   Word v = Word( f( i ) );
   if constexpr( Saturate ) {
    c += v > MAX;
    v = v > MAX ? MAX : v;
    }
   else
    ADS_PRE( v <= MAX );
   return( v );
   };
  // the full words by a loop of PER steps, which the compiler unrolls
  const std::size_t full = std::min( b , n / PER );
  for( std::size_t w = a ; w < full ; ++w ) {
   Word x = 0;
   for( std::size_t j = 0 ; j < PER ; ++j )
    x |= field( w * PER + j ) << ( j * B );
   f_w[ w ] = x;
   }
  for( std::size_t w = std::max( a , full ) ; w < b ; ++w ) {   // the last
   Word x = 0;
   for( std::size_t j = 0 ; w * PER + j < n ; ++j )
    x |= field( w * PER + j ) << ( j * B );
   f_w[ w ] = x;
   }
  if( c )
   clamped.fetch_add( c , std::memory_order_relaxed );
  } , 2048 );
 f_clamped = clamped.load( std::memory_order_relaxed );
 ADS_INVARIANT( clean() );
 }

template< unsigned B , bool Saturate >
inline typename PackedArray< B , Saturate >::Word
PackedArray< B , Saturate >::get( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 return( ( f_w[ i / PER ] >> ( ( i % PER ) * B ) ) & MAX );
 }

template< unsigned B , bool Saturate >
inline bool PackedArray< B , Saturate >::set( std::size_t i , Word v )
{
 ADS_PRE( ( i < f_n ) && ( Saturate || ( v <= MAX ) ) );
 const bool c = v > MAX;
 if( c ) {
  v = MAX;
  ++f_clamped;
  }
 const unsigned s = unsigned( ( i % PER ) * B );
 Word & w = f_w[ i / PER ];
 w = ( w & ~( MAX << s ) ) | ( v << s );
 ADS_POST( get( i ) == v );
 return( c );
 }

template< unsigned B , bool Saturate >
void PackedArray< B , Saturate >::push_back( Word v )
{
 ADS_PRE( Saturate || ( v <= MAX ) );
 if( f_n % PER == 0 )
  f_w.push_back( 0 );
 ++f_n;
 set( f_n - 1 , v );
 ADS_INVARIANT( clean() );
 }

template< unsigned B , bool Saturate >
bool PackedArray< B , Saturate >::clean( void ) const
{
 if( f_w.size() != ( f_n + PER - 1 ) / PER )
  return( false );
 if( f_n % PER == 0 )
  return( true );
 return( ( f_w.back() >> ( ( f_n % PER ) * B ) ) == 0 );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ TritArray ---------------------------------*/
/*--------------------------------------------------------------------------*/

inline TritArray::TritArray( std::size_t n , unsigned v ) : f_n( n )
{
 ADS_PRE( v <= 2 );
 const std::uint8_t x = std::uint8_t( v * ( 1 + 3 + 9 + 27 + 81 ) );
 f_y.assign( ( n + 4 ) / 5 , x );
 if( n % 5 ) {                              // the digits past the last value
  std::uint8_t y = 0;
  for( std::size_t j = 0 ; j < n % 5 ; ++j )
   y = std::uint8_t( y + v * POW3[ j ] );
  f_y.back() = y;
  }
 ADS_INVARIANT( clean() );
 }

template< Executor E , class F >
TritArray::TritArray( E & ex , std::size_t n , F && f )
 : f_y( ( n + 4 ) / 5 ) , f_n( n )
{
 // whole bytes per block, so that no two threads write one byte
 parallelForBlocks( ex , 0 , f_y.size() ,
                    [ & ]( std::size_t a , std::size_t b ) {
  for( std::size_t y = a ; y < b ; ++y ) {
   const std::size_t i = 5 * y;
   unsigned x = 0;
   if( i + 5 <= n )                         // five values: unrolled
    for( std::size_t j = 5 ; j-- > 0 ; ) {
     const unsigned v = unsigned( f( i + j ) );
     ADS_PRE( v <= 2 );
     x = 3 * x + v;
     }
   else
    for( std::size_t j = n - i ; j-- > 0 ; ) {
     const unsigned v = unsigned( f( i + j ) );
     ADS_PRE( v <= 2 );
     x = 3 * x + v;
     }
   f_y[ y ] = std::uint8_t( x );
   }
  } , 16384 );
 ADS_INVARIANT( clean() );
 }

inline unsigned TritArray::get( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 return( DIGITS[ f_y[ i / 5 ] ][ i % 5 ] );
 }

inline void TritArray::set( std::size_t i , unsigned v )
{
 ADS_PRE( ( i < f_n ) && ( v <= 2 ) );
 std::uint8_t & y = f_y[ i / 5 ];
 const unsigned j = unsigned( i % 5 );
 y = std::uint8_t( int( y ) +
                   ( int( v ) - int( DIGITS[ y ][ j ] ) ) * POW3[ j ] );
 ADS_POST( get( i ) == v );
 }

inline void TritArray::push_back( unsigned v )
{
 ADS_PRE( v <= 2 );
 if( f_n % 5 == 0 )
  f_y.push_back( 0 );
 ++f_n;
 set( f_n - 1 , v );
 ADS_INVARIANT( clean() );
 }

inline bool TritArray::clean( void ) const
{
 if( f_y.size() != ( f_n + 4 ) / 5 )
  return( false );
 for( std::size_t k = f_y.size() > 1024 ? f_y.size() - 1 : 0 ;
      k < f_y.size() ; ++k )                // all, or the last one alone
  if( f_y[ k ] >= 243 )
   return( false );
 if( f_n % 5 == 0 )
  return( true );
 return( f_y.back() / POW3[ f_n % 5 ] == 0 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File PackedArray.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
