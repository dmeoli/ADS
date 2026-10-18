/*--------------------------------------------------------------------------*/
/*------------------------- File SortedListSet.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SortedListSet [see SortedListSet.h], with their contracts.
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

template< std::totally_ordered T >
SortedListSet< T >::SortedListSet( std::initializer_list< T > items )
{
 for( const auto & x : items )
  insert( x );
 }

template< std::totally_ordered T >
std::size_t SortedListSet< T >::size( void ) const
{
 return( f_size );
 }

template< std::totally_ordered T >
bool SortedListSet< T >::isEmpty( void ) const
{
 return( f_size == 0 );
 }

template< std::totally_ordered T >
bool SortedListSet< T >::contains( const T & x ) const
{
 const auto p = lower( x );
 return( ! f_L.finished( p ) && ( f_L.read( p ) == x ) );
 }

template< std::totally_ordered T >
void SortedListSet< T >::insert( const T & x )
{
 const auto p = lower( x );
 if( f_L.finished( p ) || ! ( f_L.read( p ) == x ) ) {
  f_L.insert( p , x );
  ++f_size;
  }
 ADS_POST( contains( x ) );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered T >
void SortedListSet< T >::insertLast( const T & x )
{
 ADS_PRE( ( f_size == 0 ) || ( f_L.read( f_L.tail() ) < x ) );
 f_L.insert( f_L.next( f_L.tail() ) , x );
 ++f_size;
 ADS_POST( max() == x );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered T >
void SortedListSet< T >::remove( const T & x )
{
 const auto p = lower( x );
 if( ! f_L.finished( p ) && ( f_L.read( p ) == x ) ) {
  f_L.remove( p );
  --f_size;
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered T >
bool SortedListSet< T >::sorted( void ) const
{
 std::size_t n = 0;
 for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) , ++n )
  if( ! f_L.finished( f_L.next( p ) ) &&
      !( f_L.read( p ) < f_L.read( f_L.next( p ) ) ) )
   return( false );
 return( n == f_size );
 }

template< std::totally_ordered T >
std::optional< T > SortedListSet< T >::min( void ) const
{
 if( ! f_size )
  return( std::nullopt );
 return( f_L.read( f_L.head() ) );
 }

template< std::totally_ordered T >
std::optional< T > SortedListSet< T >::max( void ) const
{
 if( ! f_size )
  return( std::nullopt );
 return( f_L.read( f_L.tail() ) );
 }

template< std::totally_ordered T >
template< class F >
void SortedListSet< T >::forEach( F && f ) const
{
 for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) )
  f( f_L.read( p ) );
 }

template< std::totally_ordered T >
const List< T > & SortedListSet< T >::list( void ) const
{
 return( f_L );
 }

template< std::totally_ordered T >
typename List< T >::Pos SortedListSet< T >::lower( const T & x ) const
{
 auto p = f_L.head();
 while( ! f_L.finished( p ) && ( f_L.read( p ) < x ) )
  p = f_L.next( p );
 return( p );
 }

template< class T >
SortedListSet< T > merge( const SortedListSet< T > & A ,
                          const SortedListSet< T > & B ,
                          bool a , bool b , bool both )
{
 SortedListSet< T > C;
 const auto & LA = A.list();
 const auto & LB = B.list();
 auto p = LA.head();
 auto q = LB.head();
 while( ! LA.finished( p ) && ! LB.finished( q ) ) {
  const T & x = LA.read( p );
  const T & y = LB.read( q );
  if( x == y ) {
   if( both ) C.insertLast( x );
   p = LA.next( p );
   q = LB.next( q );
   }
  else if( x < y ) {
   if( a ) C.insertLast( x );
   p = LA.next( p );
   }
  else {
   if( b ) C.insertLast( y );
   q = LB.next( q );
   }
  }
 for( ; a && ! LA.finished( p ) ; p = LA.next( p ) )
  C.insertLast( LA.read( p ) );
 for( ; b && ! LB.finished( q ) ; q = LB.next( q ) )
  C.insertLast( LB.read( q ) );
 return( C );
 }

template< class T >
SortedListSet< T > setUnion( const SortedListSet< T > & A ,
                             const SortedListSet< T > & B )
{
 const auto C = merge( A , B , true , true , true );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '|' ) );
 return( C );
 }

template< class T >
SortedListSet< T > setIntersection( const SortedListSet< T > & A ,
                                    const SortedListSet< T > & B )
{
 const auto C = merge( A , B , false , false , true );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '&' ) );
 return( C );
 }

template< class T >
SortedListSet< T > setDifference( const SortedListSet< T > & A ,
                                  const SortedListSet< T > & B )
{
 const auto C = merge( A , B , true , false , false );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '-' ) );
 return( C );
 }

template< class T >
SortedListSet< T > setSymmetricDifference( const SortedListSet< T > & A ,
                                           const SortedListSet< T > & B )
{
 const auto C = merge( A , B , true , true , false );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '^' ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SortedListSet.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
