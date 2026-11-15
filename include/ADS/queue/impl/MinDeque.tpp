/*--------------------------------------------------------------------------*/
/*--------------------------- File MinDeque.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MinDeque [see MinDeque.h], with their contracts.
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
void MinDeque< T , C >::deal( MinStack< T , C > & s , MinStack< T , C > & t )
{
 std::vector< T > a;  // s from the bottom
 a.reserve( s.size() );
 while( ! s.empty() )
  a.push_back( s.pop() );
 // a holds s from the top: s_{k-1} , ... , s_0
 const std::size_t k = a.size() , h = ( k + 1 ) / 2;
 for( std::size_t i = 0 ; i < h ; ++i )  // s_{h-1} .. s_0, s_0 on top
  t.push( std::move( a[ k - h + i ] ) );
 for( std::size_t i = k - h ; i > 0 ; --i )  // s_h .. s_{k-1} on s
  s.push( std::move( a[ i - 1 ] ) );
 }

template< class T , class C >
T MinDeque< T , C >::pop_front( void )
{
 ADS_PRE( ! empty() );
 [[maybe_unused]] const std::size_t n = size();
 if( f_F.empty() )
  deal( f_B , f_F );
 T x = f_F.pop();
 ADS_POST( size() == n - 1 );
 return( x );
 }

template< class T , class C >
T MinDeque< T , C >::pop_back( void )
{
 ADS_PRE( ! empty() );
 [[maybe_unused]] const std::size_t n = size();
 if( f_B.empty() )
  deal( f_F , f_B );
 T x = f_B.pop();
 ADS_POST( size() == n - 1 );
 return( x );
 }

template< class T , class C >
const T & MinDeque< T , C >::front( void ) const
{
 ADS_PRE( ! empty() );
 return( f_F.empty() ? f_B.items().front() : f_F.top() );
 }

template< class T , class C >
const T & MinDeque< T , C >::back( void ) const
{
 ADS_PRE( ! empty() );
 return( f_B.empty() ? f_F.items().front() : f_B.top() );
 }

template< class T , class C >
const T & MinDeque< T , C >::min( void ) const
{
 ADS_PRE( ! empty() );
 if( f_F.empty() )
  return( f_B.min() );
 if( f_B.empty() )
  return( f_F.min() );
 return( f_less( f_F.min() , f_B.min() ) ? f_F.min() : f_B.min() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MinDeque.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
