/*--------------------------------------------------------------------------*/
/*----------------------------- File List.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of List [see List.h], with their contracts.
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
  while( ! empty() )
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
 if( o.empty() )
  return;
 f_sentinel.succ = o.f_sentinel.succ;
 f_sentinel.pred = o.f_sentinel.pred;
 f_sentinel.succ->pred = &f_sentinel;
 f_sentinel.pred->succ = &f_sentinel;
 o.f_sentinel.succ = &o.f_sentinel;
 o.f_sentinel.pred = &o.f_sentinel;
 f_size = std::exchange( o.f_size , 0 );
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
bool List< T , Pooled >::empty( void ) const
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
 ++f_size;
 ADS_POST( ! empty() && ( t->succ == p ) && ( p->pred == t ) &&
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
 --f_size;
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

/*--------------------------------------------------------------------------*/
/*-------------------------- THE DLList OF MORIN ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , bool Pooled >
typename List< T , Pooled >::Pos
List< T , Pooled >::position( std::size_t i ) const
{
 ADS_PRE( i <= f_size );
 Pos p;
 if( i < f_size / 2 )                       // from the front
  for( p = f_sentinel.succ ; i > 0 ; --i ) p = p->succ;
 else                                       // from the back
  for( p = sentinel() , i = f_size - i ; i > 0 ; --i ) p = p->pred;
 return( p );
 }

template< class T , bool Pooled >
const T & List< T , Pooled >::get( std::size_t i ) const
{
 ADS_PRE( i < f_size );
 return( position( i )->value );
 }

template< class T , bool Pooled >
T List< T , Pooled >::set( std::size_t i , const T & x )
{
 ADS_PRE( i < f_size );
 Pos p = position( i );
 T y = std::move( p->value );
 p->value = x;
 return( y );
 }

template< class T , bool Pooled >
void List< T , Pooled >::add( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_size );
 insert( position( i ) , x );
 }

template< class T , bool Pooled >
T List< T , Pooled >::erase( std::size_t i )
{
 ADS_PRE( i < f_size );
 Pos p = position( i );
 T x = std::move( p->value );
 remove( p );
 return( x );
 }

template< class T , bool Pooled >
void List< T , Pooled >::checkSize( void ) const
{
 std::size_t c = 0;
 for( Pos p = f_sentinel.succ ; p != sentinel() ; p = p->succ )
  ++c;
 if( c != f_size )
  throw( std::logic_error( "List::checkSize: " + std::to_string( c ) +
                           " cells, " + std::to_string( f_size ) +
                           " counted" ) );
 }

template< class T , bool Pooled >
bool List< T , Pooled >::isPalindrome( void ) const
{
 Pos p = f_sentinel.succ , q = f_sentinel.pred;
 for( std::size_t k = f_size / 2 ; k > 0 ; --k , p = p->succ , q = q->pred )
  if( !( p->value == q->value ) )
   return( false );
 return( true );
 }

template< class T , bool Pooled >
void List< T , Pooled >::relink( Cell * c , Cell * p ) noexcept
{
 c->pred->succ = c->succ;                   // out of its place
 c->succ->pred = c->pred;
 c->pred = p->pred;                         // and before p
 c->succ = p;
 p->pred->succ = c;
 p->pred = c;
 }

template< class T , bool Pooled >
void List< T , Pooled >::rotate( std::size_t r )
{
 if( f_size == 0 )
  return;
 r %= f_size;
 if( r == 0 )
  return;
 relink( sentinel() , position( f_size - r ) );   // the sentinel moves
 ADS_INVARIANT( linked() );
 }

template< class T , bool Pooled >
List< T , Pooled > List< T , Pooled >::truncate( std::size_t i )
{
 ADS_PRE( i <= f_size );
 List L;
 if( i == f_size )
  return( L );
 Pos u = position( i );
 if constexpr( Pooled )                     // a cell keeps its pool
  while( u != sentinel() ) {
   L.push_back( std::move( u->value ) );
   u = remove( u );
   }
 else {
  Pos last = f_sentinel.pred;
  u->pred->succ = sentinel();               // this list ends before u
  f_sentinel.pred = u->pred;
  L.f_sentinel.succ = u;                    // and L is u .. last
  u->pred = L.sentinel();
  L.f_sentinel.pred = last;
  last->succ = L.sentinel();
  L.f_size = f_size - i;
  f_size = i;
  }
 ADS_POST( f_size == i );
 ADS_INVARIANT( linked() && L.linked() );
 return( L );
 }

template< class T , bool Pooled >
void List< T , Pooled >::absorb( List & l2 )
{
 ADS_PRE( &l2 != this );
 if( l2.empty() )
  return;
 if constexpr( Pooled )
  f_pool.adopt( l2.f_pool );                // the cells change pool
 Pos a = l2.f_sentinel.succ , b = l2.f_sentinel.pred;
 a->pred = f_sentinel.pred;
 f_sentinel.pred->succ = a;
 b->succ = sentinel();
 f_sentinel.pred = b;
 l2.f_sentinel.succ = l2.f_sentinel.pred = l2.sentinel();
 f_size += std::exchange( l2.f_size , 0 );
 ADS_INVARIANT( linked() && l2.linked() );
 }

template< class T , bool Pooled >
List< T , Pooled > List< T , Pooled >::deal( void )
{
 [[maybe_unused]] const std::size_t n = f_size;
 List L;
 for( Pos p = f_sentinel.succ ;
      ( p != sentinel() ) && ( p->succ != sentinel() ) ; p = p->succ ) {
  Pos q = p->succ;                          // of odd rank
  if constexpr( Pooled ) {
   L.push_back( std::move( q->value ) );
   remove( q );
   }
  else {
   relink( q , L.sentinel() );
   --f_size;
   ++L.f_size;
   }
  }
 ADS_POST( ( f_size == ( n + 1 ) / 2 ) && ( L.f_size == n / 2 ) );
 ADS_INVARIANT( linked() && L.linked() );
 return( L );
 }

template< class T , bool Pooled >
void List< T , Pooled >::reverse( void )
{
 const auto old = detail::snapshot( *this );
 Pos p = sentinel();
 do {                                       // the two links of each cell
  std::swap( p->pred , p->succ );           // exchanged, the sentinel's too
  p = p->pred;                              // the old successor
  } while( p != sentinel() );
 ADS_AUDIT( ( [ & ] {
  Pos a = f_sentinel.succ , b = old->f_sentinel.pred;
  for( ; a != sentinel() ; a = a->succ , b = b->pred )
   if( ! detail::equal( a->value , b->value ) ) return( false );
  return( true ); }() ) );
 ADS_INVARIANT( linked() );
 }

template< class T , bool Pooled >
void List< T , Pooled >::takeFirst( List & l2 )
{
 ADS_PRE( ( &l2 != this ) && ! l2.empty() );
 if constexpr( Pooled ) {
  push_back( std::move( l2.f_sentinel.succ->value ) );
  l2.remove( l2.f_sentinel.succ );
  }
 else {
  relink( l2.f_sentinel.succ , sentinel() );
  --l2.f_size;
  ++f_size;
  }
 }

template< class T , bool Pooled >
template< class C >
List< T , Pooled > List< T , Pooled >::merge( List & l1 , List & l2 ,
                                              C less )
{
 ADS_PRE( &l1 != &l2 );
 List L;
 if constexpr( Pooled ) {                   // the cells of both, relinked
  L.absorb( l1 );
  L.f_pool.adopt( l2.f_pool );
  Pos a = L.f_sentinel.succ , b = l2.f_sentinel.succ;   // b into L
  while( b != l2.sentinel() ) {
   while( ( a != L.sentinel() ) && ! less( b->value , a->value ) )
    a = a->succ;
   Pos c = b;
   b = b->succ;
   relink( c , a );
   }
  L.f_size += std::exchange( l2.f_size , 0 );
  l2.f_sentinel.succ = l2.f_sentinel.pred = l2.sentinel();
  }
 else {
  while( ! l1.empty() && ! l2.empty() )
   if( less( l2.f_sentinel.succ->value , l1.f_sentinel.succ->value ) )
    L.takeFirst( l2 );
   else
    L.takeFirst( l1 );
  L.absorb( l1 );
  L.absorb( l2 );
  }
 ADS_AUDIT( ( [ & ] {
  for( Pos p = L.f_sentinel.succ ; ( p != L.sentinel() ) &&
                                   ( p->succ != L.sentinel() ) ; p = p->succ )
   if( less( p->succ->value , p->value ) ) return( false );
  return( true ); }() ) );
 ADS_INVARIANT( L.linked() );
 return( L );
 }

template< class T , bool Pooled >
template< class C >
typename List< T , Pooled >::Cell *
List< T , Pooled >::sortChain( Cell * a , std::size_t k , C & less )
{
 if( k <= 1 ) {
  if( a ) a->succ = nullptr;
  return( a );
  }
 Cell * b = a;                              // the second half, by a walk
 for( std::size_t j = k / 2 ; j > 0 ; --j ) b = b->succ;
 b = sortChain( b , k - k / 2 , less );     // b first: a still reaches it
 a = sortChain( a , k / 2 , less );
 Cell head{ nullptr , nullptr , T() };      // the merge, stable
 Cell * t = &head;
 while( a && b )
  if( less( b->value , a->value ) ) { t->succ = b; t = b; b = b->succ; }
  else { t->succ = a; t = a; a = a->succ; }
 t->succ = a ? a : b;
 return( head.succ );
 }

template< class T , bool Pooled >
template< class C >
void List< T , Pooled >::mergeSort( C less )
{
 [[maybe_unused]] const std::size_t n = f_size;
 if( f_size <= 1 )
  return;
 Cell * a = sortChain( f_sentinel.succ , f_size , less );
 Cell * p = sentinel();                     // the predecessors again
 for( ; a ; p = a , a = a->succ ) {
  p->succ = a;
  a->pred = p;
  }
 p->succ = sentinel();
 f_sentinel.pred = p;
 ADS_AUDIT( ( [ & ] {
  std::size_t c = 0;
  for( Pos q = f_sentinel.succ ; q != sentinel() ; q = q->succ ) {
   ++c;
   if( ( q->succ != sentinel() ) && less( q->succ->value , q->value ) )
    return( false );
   }
  return( c == n ); }() ) );
 ADS_INVARIANT( linked() );
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
