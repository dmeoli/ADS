/*--------------------------------------------------------------------------*/
/*---------------------------- File ListSet.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ListSet [see ListSet.h], with their contracts.
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

template< std::equality_comparable T >
ListSet< T >::ListSet( std::initializer_list< T > items )
{
 for( const auto & x : items )
  insert( x );
 }

template< std::equality_comparable T >
std::size_t ListSet< T >::size( void ) const
{
 return( f_size );
 }

template< std::equality_comparable T >
bool ListSet< T >::empty( void ) const
{
 return( f_size == 0 );
 }

template< std::equality_comparable T >
bool ListSet< T >::contains( const T & x ) const
{
 return( ! f_L.finished( find( x ) ) );
 }

template< std::equality_comparable T >
void ListSet< T >::insert( const T & x )
{
 if( ! contains( x ) )
  insertNew( x );
 ADS_POST( contains( x ) );
 }

template< std::equality_comparable T >
void ListSet< T >::insertNew( const T & x )
{
 ADS_AUDIT( ! contains( x ) );
 f_L.insert( f_L.head() , x );
 ++f_size;
 ADS_INVARIANT( distinct() );
 }

template< std::equality_comparable T >
void ListSet< T >::remove( const T & x )
{
 const auto p = find( x );
 if( ! f_L.finished( p ) ) {
  f_L.remove( p );
  --f_size;
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( distinct() );
 }

template< std::equality_comparable T >
bool ListSet< T >::distinct( void ) const
{
 std::size_t n = 0;
 for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) , ++n )
  for( auto q = f_L.next( p ) ; ! f_L.finished( q ) ; q = f_L.next( q ) )
   if( f_L.read( p ) == f_L.read( q ) )
    return( false );
 return( n == f_size );
 }

template< std::equality_comparable T >
std::optional< T > ListSet< T >::min( void ) const
 requires std::totally_ordered< T >
{
 std::optional< T > m;
 forEach( [ & ]( const T & x ) { if( ! m || ( x < *m ) ) m = x; } );
 return( m );
 }

template< std::equality_comparable T >
template< class F >
void ListSet< T >::forEach( F && f ) const
{
 for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) )
  f( f_L.read( p ) );
 }

template< std::equality_comparable T >
typename List< T >::Pos ListSet< T >::find( const T & x ) const
{
 auto p = f_L.head();
 while( ! f_L.finished( p ) && ! ( f_L.read( p ) == x ) )
  p = f_L.next( p );
 return( p );
 }

template< class T >
ListSet< T > setUnion( const ListSet< T > & A , const ListSet< T > & B )
{
 ListSet< T > C = B;
 A.forEach( [ & ]( const T & x ) {
  if( ! B.contains( x ) )
   C.insertNew( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '|' ) );
 return( C );
 }

template< class T >
ListSet< T > setIntersection( const ListSet< T > & A ,
                              const ListSet< T > & B )
{
 ListSet< T > C;
 A.forEach( [ & ]( const T & x ) {
  if( B.contains( x ) )
   C.insertNew( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '&' ) );
 return( C );
 }

template< class T >
ListSet< T > setDifference( const ListSet< T > & A , const ListSet< T > & B )
{
 ListSet< T > C;
 A.forEach( [ & ]( const T & x ) {
  if( ! B.contains( x ) )
   C.insertNew( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '-' ) );
 return( C );
 }

template< class T >
ListSet< T > setSymmetricDifference( const ListSet< T > & A ,
                                     const ListSet< T > & B )
{
 ListSet< T > C = setDifference( A , B );
 B.forEach( [ & ]( const T & x ) {
  if( ! A.contains( x ) )
   C.insertNew( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '^' ) );
 return( C );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File ListSet.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
