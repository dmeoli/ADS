/*--------------------------------------------------------------------------*/
/*---------------------------- File BoolSet.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BoolSet [see BoolSet.h], with their contracts.
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

inline BoolSet::BoolSet( long N )
{
 ADS_PRE( N >= 0 );
 f_N = std::size_t( N );
 f_W.assign( ( f_N + 63 ) / 64 , 0 );
 }

inline std::size_t BoolSet::size( void ) const
{
 return( f_size );
 }

inline bool BoolSet::empty( void ) const
{
 return( f_size == 0 );
 }

inline long BoolSet::dim( void ) const
{
 return( long( f_N ) );
 }

inline bool BoolSet::contains( long x ) const
{
 return( ( x >= 0 ) && ( x < dim() ) &&
         ( ( f_W[ std::size_t( x ) / 64 ] >> ( x % 64 ) ) & 1 ) );
 }

inline void BoolSet::insert( long x )
{
 ADS_PRE( ( x >= 0 ) && ( x < dim() ) );
 std::uint64_t & w = f_W[ std::size_t( x ) / 64 ];
 const std::uint64_t b = std::uint64_t( 1 ) << ( x % 64 );
 f_size += ( w & b ) ? 0 : 1;
 w |= b;
 ADS_POST( contains( x ) );
 ADS_INVARIANT( counted() );
 }

inline void BoolSet::remove( long x )
{
 if( contains( x ) ) {
  --f_size;
  f_W[ std::size_t( x ) / 64 ] &= ~( std::uint64_t( 1 ) << ( x % 64 ) );
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( counted() );
 }

inline bool BoolSet::counted( void ) const
{
 std::size_t c = 0;
 for( const std::uint64_t w : f_W )
  c += std::size_t( std::popcount( w ) );
 return( c == f_size );
 }

namespace detail {

/// C is A op B on every integer of the larger dimension, op one of the
/// characters | & - ^
inline bool boolSetIs( const BoolSet & C , const BoolSet & A ,
                       const BoolSet & B , char op )
{
 const long N = std::max( { A.dim() , B.dim() , C.dim() } );
 for( long i = 0 ; i < N ; ++i ) {
  const bool a = A.contains( i ) , b = B.contains( i );
  const bool c = op == '|' ? ( a || b ) : op == '&' ? ( a && b ) :
                 op == '-' ? ( a && ! b ) : ( a != b );
  if( C.contains( i ) != c )
   return( false );
  }
 return( true );
 }

}  // end( namespace detail )

inline std::optional< long > BoolSet::min( void ) const
{
 for( std::size_t k = 0 ; k < f_W.size() ; ++k )
  if( f_W[ k ] )
   return( long( k * 64 + std::size_t( std::countr_zero( f_W[ k ] ) ) ) );
 return( std::nullopt );
 }

inline std::optional< long > BoolSet::max( void ) const
{
 for( std::size_t k = f_W.size() ; k-- > 0 ; )
  if( f_W[ k ] )
   return( long( k * 64 + 63 - std::size_t( std::countl_zero( f_W[ k ] ) ) ) );
 return( std::nullopt );
 }

template< class F >
void BoolSet::forEach( F && f ) const
{
 for( std::size_t k = 0 ; k < f_W.size() ; ++k )
  for( std::uint64_t w = f_W[ k ] ; w ; w &= w - 1 )
   f( long( k * 64 + std::size_t( std::countr_zero( w ) ) ) );
 }

template< class Op >
inline void BoolSet::combine( BoolSet & C , const BoolSet & A ,
                              const BoolSet & B , Op op )
{
 const std::size_t na = A.f_W.size() , nb = B.f_W.size();
 std::size_t c = 0;
 for( std::size_t k = 0 ; k < C.f_W.size() ; ++k ) {
  const std::uint64_t a = k < na ? A.f_W[ k ] : 0;
  const std::uint64_t b = k < nb ? B.f_W[ k ] : 0;
  std::uint64_t w = op( a , b );
  if( ( k + 1 ) * 64 > C.f_N )              // the bits past the dimension
   w &= ( std::uint64_t( 1 ) << ( C.f_N % 64 ) ) - 1;
  C.f_W[ k ] = w;
  c += std::size_t( std::popcount( w ) );
  }
 C.f_size = c;
 }

inline BoolSet setUnion( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) );
 BoolSet::combine( C , A , B , []( std::uint64_t a , std::uint64_t b ) {
                    return( a | b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '|' ) );
 return( C );
 }

inline BoolSet setIntersection( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::min( A.dim() , B.dim() ) );
 BoolSet::combine( C , A , B , []( std::uint64_t a , std::uint64_t b ) {
                    return( a & b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '&' ) );
 return( C );
 }

inline BoolSet setDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( A.dim() );
 BoolSet::combine( C , A , B , []( std::uint64_t a , std::uint64_t b ) {
                    return( a & ~b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '-' ) );
 return( C );
 }

inline BoolSet setSymmetricDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) );
 BoolSet::combine( C , A , B , []( std::uint64_t a , std::uint64_t b ) {
                    return( a ^ b ); } );
 ADS_POST( detail::boolSetIs( C , A , B , '^' ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BoolSet.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
