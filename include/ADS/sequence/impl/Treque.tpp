/*--------------------------------------------------------------------------*/
/*---------------------------- File Treque.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Treque [see Treque.h], with their contracts.
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

namespace ADS
{

template< class T >
Treque< T >::Treque( std::initializer_list< T > l )
{
 for( const T & x : l )
  push_back( x );
 }

template< class T >
void Treque< T >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= size() );
 [[maybe_unused]] const std::size_t n = size();
 if( i < f_L.size() )
  f_L.insert( i , x );
 else
  f_R.insert( i - f_L.size() , x );
 balance();
 ADS_POST( size() == n + 1 );
 ADS_INVARIANT( balanced() );
 }

template< class T >
void Treque< T >::erase( std::size_t i )
{
 ADS_PRE( i < size() );
 [[maybe_unused]] const std::size_t n = size();
 if( i < f_L.size() )
  f_L.erase( i );
 else
  f_R.erase( i - f_L.size() );
 balance();
 ADS_POST( size() == n - 1 );
 ADS_INVARIANT( balanced() );
 }

template< class T >
void Treque< T >::balance( void )
{
 if( f_L.size() > f_R.size() + 1 ) {
  f_R.push_front( std::move( f_L.back() ) );
  f_L.pop_back();
  }
 else if( f_R.size() > f_L.size() + 1 ) {
  f_L.push_back( std::move( f_R.front() ) );
  f_R.pop_front();
  }
 }

template< class T >
bool Treque< T >::balanced( void ) const
{
 return( ( f_L.size() <= f_R.size() + 1 ) &&
	  ( f_R.size() <= f_L.size() + 1 ) );
 }

template< class T >
bool Treque< T >::operator==( const Treque & o ) const
{
 if( size() != o.size() )
  return( false );
 for( std::size_t i = 0 ; i < size() ; ++i )
  if( ! ( ( *this )[ i ] == o[ i ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Treque.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
