/*--------------------------------------------------------------------------*/
/*------------------------ File IndexedHeapPair.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of IndexedHeapPair [see IndexedHeapPair.h], with their
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

template< class K , class Bd , unsigned D >
inline void IndexedHeapPair< K , Bd , D >::push( int id , const K & k ,
                                                 const Bd & b )
{
 f_key.push( id , k );
 f_bound.push( id , b );
 ADS_POST( contains( id ) && ( size() == f_bound.size() ) );
 }

template< class K , class Bd , unsigned D >
inline void IndexedHeapPair< K , Bd , D >::update( int id , const K & k ,
                                                   const Bd & b )
{
 ADS_PRE( contains( id ) );
 f_key.update( id , k );
 f_bound.update( id , b );
 }

template< class K , class Bd , unsigned D >
inline void IndexedHeapPair< K , Bd , D >::erase( int id )
{
 ADS_PRE( contains( id ) );
 f_key.erase( id );
 f_bound.erase( id );
 ADS_POST( size() == f_bound.size() );
 }

template< class K , class Bd , unsigned D >
inline int IndexedHeapPair< K , Bd , D >::pop( void )
{
 ADS_PRE( ! empty() );
 const int id = f_key.pop().id;
 f_bound.erase( id );
 ADS_POST( size() == f_bound.size() );
 return( id );
 }

template< class K , class Bd , unsigned D >
inline int IndexedHeapPair< K , Bd , D >::popMinBound( void )
{
 ADS_PRE( ! empty() );
 const int id = f_bound.pop().id;
 f_key.erase( id );
 ADS_POST( size() == f_bound.size() );
 return( id );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File IndexedHeapPair.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
