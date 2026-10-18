/*--------------------------------------------------------------------------*/
/*------------------------- File DictionarySet.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DictionarySet [see DictionarySet.h], with their contracts.
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

template< class D >
template< class... A >
 requires std::constructible_from< D , A... >
DictionarySet< D >::DictionarySet( A &&... a )
 : f_d( std::forward< A >( a )... )
{
 }

template< class D >
std::size_t DictionarySet< D >::size( void ) const
{
 return( f_d.size() );
 }

template< class D >
bool DictionarySet< D >::isEmpty( void ) const
{
 return( f_d.size() == 0 );
 }

template< class D >
bool DictionarySet< D >::contains( const Item & x ) const
{
 return( f_d.contains( x ) );
 }

template< class D >
void DictionarySet< D >::insert( const Item & x )
{
 f_d.insert( x , true );
 ADS_POST( contains( x ) );
 }

template< class D >
void DictionarySet< D >::remove( const Item & x )
{
 f_d.remove( x );
 ADS_POST( ! contains( x ) );
 }

template< class D >
template< class F >
void DictionarySet< D >::forEach( F && f ) const
{
 f_d.forEach( [ & ]( const Item & x , bool ) { f( x ); } );
 }

template< class D >
const D & DictionarySet< D >::dictionary( void ) const
{
 return( f_d );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File DictionarySet.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
