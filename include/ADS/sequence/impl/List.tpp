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

template< class T , bool Pooled >
List< T , Pooled >::List( void ) : f_sentinel{ nullptr , nullptr , T() }
{
 f_sentinel.pred = &f_sentinel;
 f_sentinel.succ = &f_sentinel;
 }

template< class T , bool Pooled >
List< T , Pooled >::List( std::initializer_list< T > items ) : List()
{
 for( const auto & v : items )
  insert( next( tail() ) , v );
 }

template< class T , bool Pooled >
template< std::input_iterator I , std::sentinel_for< I > S >
List< T , Pooled >::List( I first , S last ) : List()
{
 for( ; first != last ; ++first )
  insert( sentinel() , *first );
 }

template< class T , bool Pooled >
List< T , Pooled >::List( const List & other ) : List()
{
 for( Pos p = other.head() ; ! other.finished( p ) ; p = other.next( p ) )
  insert( next( tail() ) , other.read( p ) );
 }

template< class T , bool Pooled >
List< T , Pooled >::List( List && other ) noexcept : List()
{
 f_pool.swap( other.f_pool );
 adopt( other );
 }

template< class T , bool Pooled >
List< T , Pooled > & List< T , Pooled >::operator=( List other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T , bool Pooled >
List< T , Pooled >::~List()
{
 if constexpr( ! ( Pooled && std::is_trivially_destructible_v< T > ) )
  while( ! isEmpty() )
   remove( head() );
 }

template< class T , bool Pooled >
void List< T , Pooled >::swap( List & other ) noexcept
{
 List t;
 t.adopt( *this );
 adopt( other );
 other.adopt( t );
 f_pool.swap( other.f_pool );
 }

template< class T , bool Pooled >
void List< T , Pooled >::adopt( List & o ) noexcept
{
 if( o.isEmpty() )
  return;
 f_sentinel.succ = o.f_sentinel.succ;
 f_sentinel.pred = o.f_sentinel.pred;
 f_sentinel.succ->pred = &f_sentinel;
 f_sentinel.pred->succ = &f_sentinel;
 o.f_sentinel.succ = &o.f_sentinel;
 o.f_sentinel.pred = &o.f_sentinel;
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Cell * List< T , Pooled >::sentinel( void ) const
{
 return( &f_sentinel );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SEQUENCE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , bool Pooled >
bool List< T , Pooled >::isEmpty( void ) const
{
 return( f_sentinel.succ == &f_sentinel );
 }

template< class T , bool Pooled >
bool List< T , Pooled >::finished( Pos p ) const
{
 return( p == &f_sentinel );
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Pos List< T , Pooled >::head( void ) const
{
 return( f_sentinel.succ );
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Pos List< T , Pooled >::tail( void ) const
{
 return( f_sentinel.pred );
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Pos List< T , Pooled >::next( Pos p ) const
{
 ADS_PRE( p != nullptr );
 return( p->succ );
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Pos List< T , Pooled >::prev( Pos p ) const
{
 ADS_PRE( p != nullptr );
 return( p->pred );
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Pos
List< T , Pooled >::insert( Pos p , const T & v )
{
 ADS_PRE( p != nullptr );
 ADS_AUDIT( owns( p ) );
 Pos t = f_pool.make( p->pred , p , v );
 p->pred->succ = t;
 p->pred = t;
 ADS_POST( ! isEmpty() && ( t->succ == p ) && ( p->pred == t ) &&
           detail::equal( t->value , v ) );
 ADS_INVARIANT( linked() );
 return( t );
 }

template< class T , bool Pooled >
typename List< T , Pooled >::Pos List< T , Pooled >::remove( Pos p )
{
 ADS_PRE( ( p != nullptr ) && ! finished( p ) );
 ADS_AUDIT( owns( p ) );
 Pos q = p->pred;
 q->succ = p->succ;
 p->succ->pred = q;
 Pos t = p->succ;
 f_pool.free( p );
 ADS_POST( q->succ == t );
 ADS_INVARIANT( linked() );
 return( t );
 }

template< class T , bool Pooled >
const T & List< T , Pooled >::read( Pos p ) const
{
 ADS_PRE( ( p != nullptr ) && ! finished( p ) );
 return( p->value );
 }

template< class T , bool Pooled >
void List< T , Pooled >::write( Pos p , const T & v )
{
 ADS_PRE( ( p != nullptr ) && ! finished( p ) );
 ADS_AUDIT( owns( p ) );
 p->value = v;
 ADS_POST( detail::equal( p->value , v ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , bool Pooled >
void List< T , Pooled >::push_back( const T & v )
{
 insert( sentinel() , v );
 }

template< class T , bool Pooled >
void List< T , Pooled >::push_front( const T & v )
{
 insert( head() , v );
 }

template< class T , bool Pooled >
bool List< T , Pooled >::operator==( const List & o ) const
{
 Pos p = head() , q = o.head();
 for( ; ! finished( p ) && ! o.finished( q ) ; p = p->succ , q = q->succ )
  if( !( p->value == q->value ) )
   return( false );
 return( finished( p ) && o.finished( q ) );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the walk stops at the sentinel: with pred a function, two cells with
/// the same successor would be the same cell
template< class T , bool Pooled >
bool List< T , Pooled >::linked( void ) const
{
 Pos c = sentinel();
 do {
  if( ( c->succ == nullptr ) || ( c->succ->pred != c ) )
   return( false );
  c = c->succ;
  } while( c != sentinel() );
 return( true );
 }

template< class T , bool Pooled >
bool List< T , Pooled >::owns( Pos p ) const
{
 Pos c = sentinel();
 do {
  if( c == p )
   return( true );
  c = c->succ;
  } while( c != sentinel() );
 return( false );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File List.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
