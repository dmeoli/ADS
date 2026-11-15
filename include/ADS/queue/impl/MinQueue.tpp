/*--------------------------------------------------------------------------*/
/*--------------------------- File MinQueue.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MinQueue [see MinQueue.h], with their contracts.
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

template< class T , class C >
void MinQueue< T , C >::enqueue( const T & x )
{
 [[maybe_unused]] const std::size_t n = size();
 f_I.push( x );
 ADS_POST( size() == n + 1 );
 }

template< class T , class C >
void MinQueue< T , C >::transfer( void )
{
 while( ! f_I.empty() )
  f_O.push( f_I.pop() );
 }

template< class T , class C >
T MinQueue< T , C >::dequeue( void )
{
 ADS_PRE( ! empty() );
 [[maybe_unused]] const std::size_t n = size();
 if( f_O.empty() )
  transfer();
 T x = f_O.pop();
 ADS_POST( size() == n - 1 );
 return( x );
 }

template< class T , class C >
const T & MinQueue< T , C >::top( void ) const
{
 ADS_PRE( ! empty() );
 return( f_O.empty() ? f_I.items().front() : f_O.top() );
 }

template< class T , class C >
const T & MinQueue< T , C >::min( void ) const
{
 ADS_PRE( ! empty() );
 if( f_I.empty() )
  return( f_O.min() );
 if( f_O.empty() )
  return( f_I.min() );
 return( f_less( f_O.min() , f_I.min() ) ? f_O.min() : f_I.min() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MinQueue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
