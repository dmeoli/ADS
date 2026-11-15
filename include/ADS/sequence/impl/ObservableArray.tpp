/*--------------------------------------------------------------------------*/
/*------------------------ File ObservableArray.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ObservableArray [see `ObservableArray.h`], with their
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

namespace ADS
{

template< class T , class P >
void ObservableArray< T , P >::clear( void )
{
 f_a.clear();
 if constexpr( Subject::template listened< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class T , class P >
template< std::input_iterator I >
void ObservableArray< T , P >::assign( I first , I last )
{
 f_a.assign( first , last );
 if constexpr( Subject::template listened< mod::Rebuilt > )
  if( f_sub.template live< mod::Rebuilt >() ) {
   f_sub.emit( mod::Rebuilt{} );
   f_sub.audit( *this );
   }
 }

template< class T , class P >
void ObservableArray< T , P >::setTold( std::size_t i , const T & v )
{
 const Assign e{ i , f_a[ i ] , v };
 f_a[ i ] = v;
 f_sub.emit( e );
 f_sub.audit( *this );
 }

template< class T , class P >
void ObservableArray< T , P >::pushTold( const T & v )
{
 f_a.push_back( v );
 f_sub.emit( Insert{ f_a.size() - 1 , f_a.size() - 1 , v } );
 f_sub.audit( *this );
 }

template< class T , class P >
void ObservableArray< T , P >::popTold( void )
{
 const std::size_t i = f_a.size() - 1;
 f_sub.emit( Erase{ i , i , f_a[ i ] } );  // before the removal
 f_a.pop_back();
 f_sub.audit( *this );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ObservableArray.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
