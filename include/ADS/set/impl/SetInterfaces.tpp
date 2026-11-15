/*--------------------------------------------------------------------------*/
/*------------------------- File SetInterfaces.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SetInterfaces [see SetInterfaces.h], with their contracts.
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

template< class T , class Eq >
const T * ArrayUSet< T , Eq >::find( const T & x ) const
{
 for( const T & y : f_A )
  if( f_eq( y , x ) )
   return( &y );
 return( nullptr );
 }

template< class T , class Eq >
bool ArrayUSet< T , Eq >::insert( const T & x )
{
 if( find( x ) )
  return( false );
 f_A.push_back( x );
 ADS_POST( find( x ) != nullptr );
 return( true );
 }

template< class T , class Eq >
bool ArrayUSet< T , Eq >::remove( const T & x )
{
 const T * p = find( x );
 if( ! p )
  return( false );
 T & y = f_A[ std::size_t( p - f_A.data() ) ];
 if( &y != &f_A.back() )
  y = std::move( f_A.back() );
 f_A.pop_back();
 ADS_POST( find( x ) == nullptr );
 return( true );
 }

template< class T , class C >
const T * ArraySSet< T , C >::lowerBound( const T & x ) const
{
 const auto it = std::lower_bound( f_A.begin() , f_A.end() , x , f_less );
 return( it == f_A.end() ? nullptr : &*it );
 }

template< class T , class C >
const T * ArraySSet< T , C >::find( const T & x ) const
{
 const T * p = lowerBound( x );
 return( ( p && ! f_less( x , *p ) ) ? p : nullptr );
 }

template< class T , class C >
bool ArraySSet< T , C >::insert( const T & x )
{
 const auto it = std::lower_bound( f_A.begin() , f_A.end() , x , f_less );
 if( ( it != f_A.end() ) && ! f_less( x , *it ) )
  return( false );
 f_A.insert( it , x );
 ADS_POST( find( x ) != nullptr );
 ADS_INVARIANT( sorted() );
 return( true );
 }

template< class T , class C >
bool ArraySSet< T , C >::remove( const T & x )
{
 const auto it = std::lower_bound( f_A.begin() , f_A.end() , x , f_less );
 if( ( it == f_A.end() ) || f_less( x , *it ) )
  return( false );
 f_A.erase( it );
 ADS_POST( find( x ) == nullptr );
 ADS_INVARIANT( sorted() );
 return( true );
 }

template< class T , class C >
bool ArraySSet< T , C >::sorted( void ) const
{
 for( std::size_t i = 1 ; i < f_A.size() ; ++i )
  if( ! f_less( f_A[ i - 1 ] , f_A[ i ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SetInterfaces.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
