/*--------------------------------------------------------------------------*/
/*------------------------- File LinkedHashSet.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LinkedHashSet [see LinkedHashSet.h], with their contracts.
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

template< class T , class H >
LinkedHashSet< T , H >::LinkedHashSet( void )
 : f_index( 7 , true )
{
 }

template< class T , class H >
LinkedHashSet< T , H >::LinkedHashSet( const LinkedHashSet & o )
 : LinkedHashSet()
{
 o.forEach( [ & ]( const T & x ) { insert( x ); } );
 }

template< class T , class H >
std::size_t LinkedHashSet< T , H >::size( void ) const
{
 return( f_index.size() );
 }

template< class T , class H >
bool LinkedHashSet< T , H >::isEmpty( void ) const
{
 return( f_index.size() == 0 );
 }

template< class T , class H >
bool LinkedHashSet< T , H >::contains( const T & x ) const
{
 return( f_index.contains( x ) );
 }

template< class T , class H >
void LinkedHashSet< T , H >::insert( const T & x )
{
 if( ! f_index.contains( x ) )
  f_index.insert( x , f_order.insert( f_order.next( f_order.tail() ) ,
                                      x ) );
 ADS_POST( contains( x ) );
 ADS_INVARIANT( indexed() );
 }

template< class T , class H >
void LinkedHashSet< T , H >::remove( const T & x )
{
 if( const auto p = f_index.lookup( x ) ) {
  f_order.remove( *p );
  f_index.remove( x );
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( indexed() );
 }

template< class T , class H >
bool LinkedHashSet< T , H >::indexed( void ) const
{
 std::size_t n = 0;
 for( auto p = f_order.head() ; ! f_order.finished( p ) ;
      p = f_order.next( p ) , ++n )
  if( f_index.lookup( f_order.read( p ) ) != p )
   return( false );
 return( n == f_index.size() );
 }

template< class T , class H >
template< class F >
void LinkedHashSet< T , H >::forEach( F && f ) const
{
 for( auto p = f_order.head() ; ! f_order.finished( p ) ;
      p = f_order.next( p ) )
  f( f_order.read( p ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File LinkedHashSet.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
