/*--------------------------------------------------------------------------*/
/*--------------------------- File MinStack.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MinStack [see MinStack.h], with their contracts.
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

template< class T , class C >
void MinStack< T , C >::push( const T & x )
{
 [[maybe_unused]] const std::size_t n = f_A.size();
 //! [push]
 f_A.push_back( x );
 if( f_M.empty() || ! f_less( f_M.back() , f_A.back() ) )
  f_M.push_back( f_A.back() );
 //! [push]
 ADS_POST( f_A.size() == n + 1 );
 ADS_INVARIANT( ordered() );
 }

template< class T , class C >
T MinStack< T , C >::pop( void )
{
 ADS_PRE( ! f_A.empty() );
 [[maybe_unused]] const std::size_t n = f_A.size();
 //! [pop]
 T x = std::move( f_A.back() );
 f_A.pop_back();
 if( ! f_less( f_M.back() , x ) )          // x is the least: it leaves
  f_M.pop_back();
 //! [pop]
 ADS_POST( f_A.size() == n - 1 );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class C >
bool MinStack< T , C >::ordered( void ) const
{
 for( std::size_t i = 1 ; i < f_M.size() ; ++i )
  if( f_less( f_M[ i - 1 ] , f_M[ i ] ) )
   return( false );
 if( f_A.empty() )
  return( f_M.empty() );
 const auto m = std::min_element( f_A.begin() , f_A.end() , f_less );
 return( ! f_less( *m , f_M.back() ) && ! f_less( f_M.back() , *m ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File MinStack.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
