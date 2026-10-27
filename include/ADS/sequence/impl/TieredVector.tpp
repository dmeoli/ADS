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
 const std::size_t m = f_b - 1;
 if( f_n == f_blocks.size() * f_b ) {       // a new block at the end
  f_blocks.emplace_back();
  f_blocks.back().a = std::make_unique< T[] >( f_b );
  }
 const std::size_t k = i >> f_lg , L = f_n >> f_lg;
 for( std::size_t j = L ; j > k ; --j ) {   // the last item of each block
  Block & p = f_blocks[ j - 1 ];            // to the front of the next
  Block & q = f_blocks[ j ];
  q.off = ( q.off - 1 ) & m;
  q.a[ q.off ] = std::move( p.a[ ( p.off + m ) & m ] );
  }
 Block & c = f_blocks[ k ];                 // and the items of the block
 const std::size_t end = k < L ? m : f_n - ( k << f_lg );   // after i one
 for( std::size_t q = end ; q > ( i & m ) ; --q )            // place on
  c.a[ ( c.off + q ) & m ] = std::move( c.a[ ( c.off + q - 1 ) & m ] );
 c.a[ ( c.off + i ) & m ] = std::move( y );
 ++f_n;
 ADS_INVARIANT( full() );
 }

template< class T >
void TieredVector< T >::erase( std::size_t i )
{
 ADS_PRE( i < f_n );
 const std::size_t m = f_b - 1;
 const std::size_t k = i >> f_lg , L = ( f_n - 1 ) >> f_lg;
 Block & c = f_blocks[ k ];                 // the items of the block after
 const std::size_t end = k < L ? f_b : f_n - ( k << f_lg );   // i one
 for( std::size_t q = ( i & m ) + 1 ; q < end ; ++q )          // place back
  c.a[ ( c.off + q - 1 ) & m ] = std::move( c.a[ ( c.off + q ) & m ] );
 for( std::size_t j = k + 1 ; j <= L ; ++j ) {   // the first item of each
  Block & p = f_blocks[ j - 1 ];                 // block to the back of
  Block & q = f_blocks[ j ];                     // the one before
  p.a[ ( p.off + m ) & m ] = std::move( q.a[ q.off ] );
  q.off = ( q.off + 1 ) & m;
  }
 --f_n;
 if( f_n <= ( f_blocks.size() - 1 ) * f_b )  // the last block empty
  f_blocks.pop_back();
 if( ( f_b > 64 ) && ( 4 * f_n < f_b * f_b ) )
  rebuild( f_n );
 ADS_INVARIANT( full() );
 }

template< class T >
void TieredVector< T >::clear( void )
{
 f_blocks.clear();
 f_n = 0;
 f_b = 64;
 f_lg = 6;
 }

template< class T >
bool TieredVector< T >::full( void ) const
{
 return( ( f_blocks.size() == ( f_n + f_b - 1 ) / f_b ) &&
         ( f_b == ( std::size_t( 1 ) << f_lg ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File TieredVector.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
