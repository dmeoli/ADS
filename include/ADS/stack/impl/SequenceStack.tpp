/*--------------------------------------------------------------------------*/
/*------------------------- File SequenceStack.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SequenceStack [see SequenceStack.h], with their
 * contracts.
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

template< Sequence S >
bool SequenceStack< S >::empty( void ) const
{
 return( f_s.empty() );
 }

template< Sequence S >
const typename SequenceStack< S >::Item & SequenceStack< S >::top( void ) const
{
 ADS_PRE( ! f_s.empty() );
 return( f_s.read( f_s.head() ) );
 }

template< Sequence S >
typename SequenceStack< S >::Item SequenceStack< S >::pop( void )
{
 ADS_PRE( ! f_s.empty() );
 Item v = f_s.read( f_s.head() );
 f_s.remove( f_s.head() );
 return( v );
 }

template< Sequence S >
void SequenceStack< S >::push( const Item & v )
{
 f_s.insert( f_s.head() , v );
 ADS_POST( ! f_s.empty() && detail::equal( top() , v ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SequenceStack.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
