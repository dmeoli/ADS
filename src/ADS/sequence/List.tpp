/*--------------------------------------------------------------------------*/
/*----------------------------- File List.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of List [see List.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------- CONSTRUCTING AND DESTRUCTING -------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
List< T >::List( void ) : f_sentinel( new Cell{ nullptr , nullptr , T() } )
{
 f_sentinel->pred = f_sentinel;
 f_sentinel->succ = f_sentinel;
 }

template< class T >
List< T >::List( std::initializer_list< T > items ) : List()
{
 for( const auto & v : items )
  insert( next( tail() ) , v );
 }

template< class T >
List< T >::List( const List & other ) : List()
{
 for( Pos p = other.head() ; ! other.finished( p ) ; p = other.next( p ) )
  insert( next( tail() ) , other.read( p ) );
 }

template< class T >
List< T >::List( List && other ) noexcept : List()
{
 swap( other );
 }

template< class T >
List< T > & List< T >::operator=( List other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
List< T >::~List()
{
 while( ! isEmpty() )
  remove( head() );
 delete f_sentinel;
 }

template< class T >
void List< T >::swap( List & other ) noexcept
{
 std::swap( f_sentinel , other.f_sentinel );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SEQUENCE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool List< T >::isEmpty( void ) const
{
 return( f_sentinel->succ == f_sentinel );
 }

template< class T >
bool List< T >::finished( Pos p ) const
{
 return( p == f_sentinel );
 }

template< class T >
typename List< T >::Pos List< T >::head( void ) const
{
 return( f_sentinel->succ );
 }

template< class T >
typename List< T >::Pos List< T >::tail( void ) const
{
 return( f_sentinel->pred );
 }

template< class T >
typename List< T >::Pos List< T >::next( Pos p ) const
{
 ADS_PRE( p != nullptr );
 return( p->succ );
 }

template< class T >
typename List< T >::Pos List< T >::prev( Pos p ) const
{
 ADS_PRE( p != nullptr );
 return( p->pred );
 }

template< class T >
typename List< T >::Pos List< T >::insert( Pos p , const T & v )
{
 ADS_PRE( p != nullptr );
 Pos t = new Cell{ p->pred , p , v };
 p->pred->succ = t;
 p->pred = t;
 ADS_POST( ! isEmpty() && ( t->succ == p ) && ( p->pred == t ) );
 ADS_INVARIANT( linked() );
 return( t );
 }

template< class T >
typename List< T >::Pos List< T >::remove( Pos p )
{
 ADS_PRE( ( p != nullptr ) && ! finished( p ) );
 Pos q = p->pred;
 q->succ = p->succ;
 p->succ->pred = q;
 Pos t = p->succ;
 delete p;
 ADS_POST( q->succ == t );
 ADS_INVARIANT( linked() );
 return( t );
 }

template< class T >
const T & List< T >::read( Pos p ) const
{
 ADS_PRE( ( p != nullptr ) && ! finished( p ) );
 return( p->value );
 }

template< class T >
void List< T >::write( Pos p , const T & v )
{
 ADS_PRE( ( p != nullptr ) && ! finished( p ) );
 p->value = v;
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the walk stops at the sentinel: with pred a function, two cells with
/// the same successor would be the same cell
template< class T >
bool List< T >::linked( void ) const
{
 Pos c = f_sentinel;
 do {
  if( ( c->succ == nullptr ) || ( c->succ->pred != c ) )
   return( false );
  c = c->succ;
  } while( c != f_sentinel );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File List.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
