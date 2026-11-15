/*--------------------------------------------------------------------------*/
/*--------------------------- File BlockStore.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BlockStore [see BlockStore.h], with their contracts.
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
T BlockStore< T >::readBlock( std::size_t i ) const
{
 ADS_PRE( ( i < f_B.size() ) && f_used[ i ] );
 ++f_reads;
 if( f_mem )
  f_mem->countRead();
 return( f_B[ i ] );
 }

template< class T >
void BlockStore< T >::writeBlock( std::size_t i , const T & b )
{
 ADS_PRE( ( i < f_B.size() ) && f_used[ i ] );
 ++f_writes;
 if( f_mem )
  f_mem->countWrite();
 f_B[ i ] = b;
 }

template< class T >
std::size_t BlockStore< T >::placeBlock( const T & b )
{
 std::size_t i;
 if( f_free.empty() ) {
  i = f_B.size();
  f_B.push_back( b );
  f_used.push_back( true );
  }
 else {
  i = f_free.back();
  f_free.pop_back();
  f_B[ i ] = b;
  f_used[ i ] = true;
  }
 ++f_writes;
 if( f_mem )
  f_mem->countWrite();
 return( i );
 }

template< class T >
void BlockStore< T >::freeBlock( std::size_t i )
{
 ADS_PRE( ( i < f_B.size() ) && f_used[ i ] );
 f_used[ i ] = false;
 f_B[ i ] = T();
 f_free.push_back( i );
 }

template< class T >
const T & BlockStore< T >::peek( std::size_t i ) const
{
 ADS_PRE( ( i < f_B.size() ) && f_used[ i ] );
 return( f_B[ i ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BlockStore.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
