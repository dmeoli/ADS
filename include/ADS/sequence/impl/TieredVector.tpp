/*--------------------------------------------------------------------------*/
/*-------------------------- File TieredVector.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of TieredVector [see TieredVector.h], with their
 * contracts.
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

template< class T >
void TieredVector< T >::rebuild( std::size_t n )
{
 std::size_t r = 1;                         // the least power of 2 whose
 while( r * r < n ) r *= 2;                 // square is not below n
 const std::size_t b = r < 64 ? 64 : r;
 if( b == f_b )
  return;
 std::vector< Block > B( ( f_n + b - 1 ) / b );
 for( Block & k : B )
  k.a = std::make_unique< T[] >( b );
 for( std::size_t i = 0 ; i < f_n ; ++i )
  B[ i / b ].a[ i % b ] = std::move( ( *this )[ i ] );
 f_blocks.swap( B );
 f_g = 0;
 f_b = b;
 f_lg = unsigned( std::countr_zero( b ) );
 }

template< class T >
void TieredVector< T >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_n );
 T y = x;                                   // x may be an item here
 if( f_n + 1 > 4 * f_b * f_b )
  rebuild( f_n + 1 );
 if( i < f_n / 2 ) {                        // the shorter side, the front
  insertFront( i , std::move( y ) );
  ADS_INVARIANT( full() );
  return;
  }
 const std::size_t m = f_b - 1;
 if( f_g + f_n == f_blocks.size() * f_b ) {   // a new block at the end
  f_blocks.emplace_back();
  f_blocks.back().a = std::make_unique< T[] >( f_b );
  }
 const std::size_t v = f_g + i;             // the virtual place of i
 const std::size_t k = v >> f_lg , L = ( f_g + f_n ) >> f_lg;
 for( std::size_t j = L ; j > k ; --j ) {   // the last item of each block
  Block & p = f_blocks[ j - 1 ];            // to the front of the next
  Block & q = f_blocks[ j ];
  q.off = ( q.off - 1 ) & m;
  q.a[ q.off ] = std::move( p.a[ ( p.off + m ) & m ] );
  }
 Block & c = f_blocks[ k ];                 // and the items of the block
 const std::size_t end = k < L ? m : f_g + f_n - ( k << f_lg );  // after
 for( std::size_t q = end ; q > ( v & m ) ; --q )           // i one place on
  c.a[ ( c.off + q ) & m ] = std::move( c.a[ ( c.off + q - 1 ) & m ] );
 c.a[ ( c.off + v ) & m ] = std::move( y );
 ++f_n;
 ADS_INVARIANT( full() );
 }

template< class T >
void TieredVector< T >::insertFront( std::size_t i , T && x )
{
 const std::size_t m = f_b - 1;
 if( f_g == 0 ) {                           // a new block at the front
  f_blocks.emplace( f_blocks.begin() );
  f_blocks.front().a = std::make_unique< T[] >( f_b );
  f_g = f_b;
  }
 const std::size_t w = f_g + i - 1;         // the virtual place of x
 const std::size_t k = w >> f_lg;
 for( std::size_t j = 0 ; j < k ; ++j ) {   // the first item of each block
  Block & p = f_blocks[ j ];                // to the back of the one before
  Block & q = f_blocks[ j + 1 ];
  p.off = ( p.off + 1 ) & m;
  p.a[ ( p.off + m ) & m ] = std::move( q.a[ q.off ] );
  }
 Block & c = f_blocks[ k ];                 // and the items of the block
 const std::size_t s = k == 0 ? f_g : 1;    // before x one place back
 for( std::size_t q = s ; q <= ( w & m ) ; ++q )
  c.a[ ( c.off + q - 1 ) & m ] = std::move( c.a[ ( c.off + q ) & m ] );
 c.a[ ( c.off + w ) & m ] = std::move( x );
 --f_g;
 ++f_n;
 }

template< class T >
void TieredVector< T >::eraseFront( std::size_t i )
{
 const std::size_t m = f_b - 1;
 const std::size_t v = f_g + i , k = v >> f_lg;
 Block & c = f_blocks[ k ];                 // the items of the block before
 const std::size_t s = k == 0 ? f_g : 0;    // i one place on
 for( std::size_t q = v & m ; q > s ; --q )
  c.a[ ( c.off + q ) & m ] = std::move( c.a[ ( c.off + q - 1 ) & m ] );
 for( std::size_t j = k ; j > 0 ; --j ) {   // the last item of each block
  Block & p = f_blocks[ j - 1 ];            // before to the front of the
  Block & q = f_blocks[ j ];                // next one
  q.a[ q.off ] = std::move( p.a[ ( p.off + m ) & m ] );
  p.off = ( p.off - 1 ) & m;
  }
 ++f_g;
 --f_n;
 if( f_g == f_b ) {                         // the first block empty
  f_blocks.erase( f_blocks.begin() );
  f_g = 0;
  }
 }

template< class T >
void TieredVector< T >::erase( std::size_t i )
{
 ADS_PRE( i < f_n );
 if( i < f_n / 2 )                          // the shorter side, the front
  eraseFront( i );
 else
  eraseBack( i );
 if( f_n == 0 ) {
  f_blocks.clear();
  f_g = 0;
  }
 if( ( f_b > 64 ) && ( 4 * f_n < f_b * f_b ) )
  rebuild( f_n );
 ADS_INVARIANT( full() );
 }

template< class T >
void TieredVector< T >::eraseBack( std::size_t i )
{
 const std::size_t m = f_b - 1;
 const std::size_t v = f_g + i;
 const std::size_t k = v >> f_lg , L = ( f_g + f_n - 1 ) >> f_lg;
 Block & c = f_blocks[ k ];                 // the items of the block after
 const std::size_t end = k < L ? f_b : f_g + f_n - ( k << f_lg );   // i
 for( std::size_t q = ( v & m ) + 1 ; q < end ; ++q )   // one place back
  c.a[ ( c.off + q - 1 ) & m ] = std::move( c.a[ ( c.off + q ) & m ] );
 for( std::size_t j = k + 1 ; j <= L ; ++j ) {   // the first item of each
  Block & p = f_blocks[ j - 1 ];                 // block to the back of
  Block & q = f_blocks[ j ];                     // the one before
  p.a[ ( p.off + m ) & m ] = std::move( q.a[ q.off ] );
  q.off = ( q.off + 1 ) & m;
  }
 --f_n;
 if( f_g + f_n <= ( f_blocks.size() - 1 ) * f_b )   // the last block empty
  f_blocks.pop_back();
 }

template< class T >
void TieredVector< T >::clear( void )
{
 f_blocks.clear();
 f_n = 0;
 f_g = 0;
 f_b = 64;
 f_lg = 6;
 }

template< class T >
bool TieredVector< T >::full( void ) const
{
 return( ( f_blocks.size() == ( f_g + f_n + f_b - 1 ) / f_b ) &&
         ( f_g < f_b ) && ( ( f_n > 0 ) || ( f_g == 0 ) ) &&
         ( f_b == ( std::size_t( 1 ) << f_lg ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File TieredVector.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
