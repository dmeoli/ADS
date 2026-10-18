/*--------------------------------------------------------------------------*/
/*------------------------- File HashMultiset.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of HashMultiset [see HashMultiset.h], with their contracts.
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
HashMultiset< T , H >::HashMultiset( void )
 : f_count( 7 , true )
{
 }

template< class T , class H >
std::size_t HashMultiset< T , H >::size( void ) const
{
 return( f_size );
 }

template< class T , class H >
std::size_t HashMultiset< T , H >::distinct( void ) const
{
 return( f_count.size() );
 }

template< class T , class H >
bool HashMultiset< T , H >::isEmpty( void ) const
{
 return( f_size == 0 );
 }

template< class T , class H >
bool HashMultiset< T , H >::contains( const T & x ) const
{
 return( f_count.contains( x ) );
 }

template< class T , class H >
std::size_t HashMultiset< T , H >::count( const T & x ) const
{
 return( f_count.lookup( x ).value_or( 0 ) );
 }

template< class T , class H >
void HashMultiset< T , H >::insert( const T & x )
{
 const std::size_t c = count( x );
 f_count.insert( x , c + 1 );
 ++f_size;
 ADS_POST( count( x ) == c + 1 );
 ADS_INVARIANT( counted() );
 }

template< class T , class H >
void HashMultiset< T , H >::remove( const T & x )
{
 const auto c = count( x );
 if( c == 0 )
  return;
 if( c == 1 )
  f_count.remove( x );
 else
  f_count.insert( x , c - 1 );
 --f_size;
 ADS_POST( count( x ) == c - 1 );
 ADS_INVARIANT( counted() );
 }

template< class T , class H >
bool HashMultiset< T , H >::counted( void ) const
{
 std::size_t s = 0;
 bool ok = true;
 f_count.forEach( [ & ]( const T & , std::size_t c ) {
  s += c;
  ok = ok && ( c > 0 );
  } );
 return( ok && ( s == f_size ) );
 }

template< class T , class H >
template< class F >
void HashMultiset< T , H >::forEach( F && f ) const
{
 f_count.forEach( f );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File HashMultiset.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
