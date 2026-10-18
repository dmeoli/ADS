/*--------------------------------------------------------------------------*/
/*---------------------------- File Dequeue.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Dequeue [see Dequeue.h], with their contracts.
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

template< class T >
bool Dequeue< T >::isEmpty( void ) const
{
 return( f_L.isEmpty() );
 }

template< class T >
void Dequeue< T >::insertHead( const T & v )
{
 f_L.insert( f_L.head() , v );
 ADS_POST( ! f_L.isEmpty() );
 }

template< class T >
void Dequeue< T >::insertTail( const T & v )
{
 f_L.insert( f_L.next( f_L.tail() ) , v );
 ADS_POST( ! f_L.isEmpty() );
 }

template< class T >
T Dequeue< T >::removeHead( void )
{
 ADS_PRE( ! f_L.isEmpty() );
 T v = f_L.read( f_L.head() );
 f_L.remove( f_L.head() );
 return( v );
 }

template< class T >
T Dequeue< T >::removeTail( void )
{
 ADS_PRE( ! f_L.isEmpty() );
 T v = f_L.read( f_L.tail() );
 f_L.remove( f_L.tail() );
 return( v );
 }

template< class T >
const T & Dequeue< T >::head( void ) const
{
 ADS_PRE( ! f_L.isEmpty() );
 return( f_L.read( f_L.head() ) );
 }

template< class T >
const T & Dequeue< T >::tail( void ) const
{
 ADS_PRE( ! f_L.isEmpty() );
 return( f_L.read( f_L.tail() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Dequeue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
