/*--------------------------------------------------------------------------*/
/*------------------------- File DictionarySet.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DictionarySet [see DictionarySet.h], with their contracts.
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
bool DictionarySet< D >::empty( void ) const
{
 return( f_d.size() == 0 );
 }

template< class D >
bool DictionarySet< D >::contains( const Item & x ) const
{
 if constexpr( KeyedDictionary< D > )
  return( f_d.findKey( x ) != nullptr );
 else
  return( f_d.contains( x ) );
 }

template< class D >
typename D::Value DictionarySet< D >::mark( void )
{
 if constexpr( std::same_as< typename D::Value , bool > )
  return( true );
 else
  return( typename D::Value() );
 }

template< class D >
bool DictionarySet< D >::insert( const Item & x )
{
 bool fresh;
 if constexpr( std::same_as< decltype( f_d.insert( x , mark() ) ) , bool > )
  fresh = f_d.insert( x , mark() );         // the dictionary tells
 else if( ( fresh = ! contains( x ) ) )
  f_d.insert( x , mark() );
 ADS_POST( contains( x ) );
 return( fresh );
 }

template< class D >
bool DictionarySet< D >::remove( const Item & x )
{
 bool was;
 if constexpr( std::same_as< decltype( f_d.remove( x ) ) , bool > )
  was = f_d.remove( x );
 else if( ( was = contains( x ) ) )
  f_d.remove( x );
 ADS_POST( ! contains( x ) );
 return( was );
 }

template< class D >
auto DictionarySet< D >::find( const Item & x ) const -> const Item *
 requires KeyedDictionary< D >
{
 return( f_d.findKey( x ) );
 }

template< class D >
auto DictionarySet< D >::lowerBound( const Item & x ) const -> const Item *
 requires OrderedDictionary< D >
{
 return( f_d.lowerBoundKey( x ) );
 }

template< class D >
template< class F >
void DictionarySet< D >::forEach( F && f ) const
{
 if constexpr( requires { std::as_const( f_d ).forEach(
                           []( const Item & , const auto & ) {} ); } )
  std::as_const( f_d ).forEach( [ & ]( const Item & x , const auto & ) {
   f( x ); } );
 else
  for( const auto & kv : std::as_const( f_d ) )
   f( kv.first );
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
