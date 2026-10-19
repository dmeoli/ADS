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
 f_V.assign( N , false );
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
 return( long( f_V.size() ) );
 }

inline bool BoolSet::contains( long x ) const
{
 return( ( x >= 0 ) && ( x < dim() ) && f_V[ x ] );
 }

inline void BoolSet::insert( long x )
{
 ADS_PRE( ( x >= 0 ) && ( x < dim() ) );
 if( ! f_V[ x ] )
  ++f_size;
 f_V[ x ] = true;
 ADS_POST( contains( x ) );
 ADS_INVARIANT( counted() );
 }

inline void BoolSet::remove( long x )
{
 if( contains( x ) ) {
  --f_size;
  f_V[ x ] = false;
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( counted() );
 }

inline bool BoolSet::counted( void ) const
{
 return( std::size_t( std::count( f_V.begin() , f_V.end() , true ) ) ==
         f_size );
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
 for( long i = 0 ; i < dim() ; ++i )
  if( f_V[ i ] )
   return( i );
 return( std::nullopt );
 }

inline std::optional< long > BoolSet::max( void ) const
{
 for( long i = dim() - 1 ; i >= 0 ; --i )
  if( f_V[ i ] )
   return( i );
 return( std::nullopt );
 }

template< class F >
void BoolSet::forEach( F && f ) const
{
 for( long i = 0 ; i < dim() ; ++i )
  if( f_V[ i ] )
   f( i );
 }

inline BoolSet setUnion( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) );
 for( long i = 0 ; i < A.dim() ; ++i )
  if( A.contains( i ) ) C.insert( i );
 for( long i = 0 ; i < B.dim() ; ++i )
  if( B.contains( i ) ) C.insert( i );
 ADS_POST( detail::boolSetIs( C , A , B , '|' ) );
 return( C );
 }

inline BoolSet setIntersection( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::min( A.dim() , B.dim() ) );
 for( long i = 0 ; i < C.dim() ; ++i )
  if( A.contains( i ) && B.contains( i ) ) C.insert( i );
 ADS_POST( detail::boolSetIs( C , A , B , '&' ) );
 return( C );
 }

inline BoolSet setDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( A.dim() );
 for( long i = 0 ; i < A.dim() ; ++i )
  if( A.contains( i ) && ! B.contains( i ) ) C.insert( i );
 ADS_POST( detail::boolSetIs( C , A , B , '-' ) );
 return( C );
 }

inline BoolSet setSymmetricDifference( const BoolSet & A , const BoolSet & B )
{
 BoolSet C( std::max( A.dim() , B.dim() ) );
 for( long i = 0 ; i < C.dim() ; ++i )
  if( A.contains( i ) != B.contains( i ) ) C.insert( i );
 ADS_POST( detail::boolSetIs( C , A , B , '^' ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BoolSet.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
