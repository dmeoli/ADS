/*--------------------------------------------------------------------------*/
/*-------------------------- File SinglyList.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SinglyList [see SinglyList.h], with their contracts.
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
/*------------------------- CONSTRUCTING AND COPYING -----------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
SinglyList< T >::SinglyList( std::initializer_list< T > items )
{
 Pos p = nullptr;
 for( const auto & v : items ) {
  insert( p , v );
  p = next( p );
  }
 }

template< class T >
SinglyList< T >::SinglyList( const SinglyList & other )
{
 Pos p = nullptr;
 for( Cell * c = other.f_first ; c ; c = c->next ) {
  insert( p , c->value );
  p = next( p );
  }
 }

template< class T >
SinglyList< T >::SinglyList( SinglyList && other ) noexcept
 : f_first( std::exchange( other.f_first , nullptr ) )
{
 }

template< class T >
SinglyList< T > & SinglyList< T >::operator=( SinglyList other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
SinglyList< T >::~SinglyList()
{
 while( ! isEmpty() )
  remove( head() );
 }

template< class T >
void SinglyList< T >::swap( SinglyList & other ) noexcept
{
 std::swap( f_first , other.f_first );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SEQUENCE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool SinglyList< T >::isEmpty( void ) const
{
 return( f_first == nullptr );
 }

template< class T >
bool SinglyList< T >::finished( Pos p ) const
{
 if( p == &f_before )
  return( true );
 return( p ? ( p->next == nullptr ) : ( f_first == nullptr ) );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::head( void ) const
{
 return( nullptr );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::tail( void ) const
{
 if( ! f_first )
  return( &f_before );
 Pos p = nullptr;                    // the cell before the last one
 while( ! finished( next( p ) ) )
  p = next( p );
 return( p );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::next( Pos p ) const
{
 if( p == &f_before )
  return( nullptr );
 ADS_PRE( ! finished( p ) );
 return( p ? p->next : f_first );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::prev( Pos p ) const
{
 if( ! p )
  return( &f_before );
 if( p == f_first )
  return( nullptr );
 Pos q = f_first;                    // a scan for the cell before p
 while( q->next != p )
  q = q->next;
 return( q );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::insert( Pos p , const T & v )
{
 ADS_PRE( p != &f_before );
 if( ! p )
  f_first = new Cell{ v , f_first };
 else
  p->next = new Cell{ v , p->next };
 ADS_POST( detail::equal( read( p ) , v ) );
 return( p );
 }

template< class T >
typename SinglyList< T >::Pos SinglyList< T >::remove( Pos p )
{
 ADS_PRE( ! finished( p ) );
 Cell * t = cell( p );
 ( p ? p->next : f_first ) = t->next;
 delete t;
 return( p );
 }

template< class T >
const T & SinglyList< T >::read( Pos p ) const
{
 ADS_PRE( ! finished( p ) );
 return( cell( p )->value );
 }

template< class T >
void SinglyList< T >::write( Pos p , const T & v )
{
 ADS_PRE( ! finished( p ) );
 cell( p )->value = v;
 ADS_POST( detail::equal( read( p ) , v ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool SinglyList< T >::operator==( const SinglyList & o ) const
{
 const Cell * a = f_first , * b = o.f_first;
 for( ; a && b ; a = a->next , b = b->next )
  if( !( a->value == b->value ) )
   return( false );
 return( ! a && ! b );
 }

template< class T >
typename SinglyList< T >::Cell * SinglyList< T >::cell( Pos p ) const
{
 return( p ? p->next : f_first );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SinglyList.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
