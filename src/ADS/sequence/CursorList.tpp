/*--------------------------------------------------------------------------*/
/*-------------------------- File CursorList.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CursorSpace and CursorList [see CursorList.h], with
 * their contracts.
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
/*------------------------------- THE SPACE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
CursorSpace< T >::CursorSpace( long dim ) : f_free( 1 )
{
 ADS_PRE( dim >= 1 );
 f_A.resize( dim + 1 );
 for( long c = 1 ; c < dim ; ++c )         // the free list 1 , ... , dim
  f_A[ c ].next = c + 1;
 f_A[ dim ].next = 0;
 f_A[ 0 ].next = 0;
 }

template< class T >
typename CursorSpace< T >::Cell & CursorSpace< T >::operator[]( long c )
{
 ADS_PRE( ( c >= 1 ) && ( c <= capacity() ) );
 return( f_A[ c ] );
 }

template< class T >
const typename CursorSpace< T >::Cell &
CursorSpace< T >::operator[]( long c ) const
{
 ADS_PRE( ( c >= 1 ) && ( c <= capacity() ) );
 return( f_A[ c ] );
 }

template< class T >
long CursorSpace< T >::take( void )
{
 if( f_free == 0 ) {                       // no free cell: twice the places
  const long n = capacity();
  f_A.resize( 2 * n + 1 );
  for( long c = n + 1 ; c < 2 * n ; ++c )
   f_A[ c ].next = c + 1;
  f_A[ 2 * n ].next = 0;
  f_free = n + 1;
  }
 const long c = f_free;
 f_free = f_A[ c ].next;
 f_A[ c ].next = 0;
 ++f_used;
 ADS_INVARIANT( partition() );
 return( c );
 }

template< class T >
void CursorSpace< T >::give( long c )
{
 ADS_PRE( ( c >= 1 ) && ( c <= capacity() ) && ( f_used > 0 ) );
 f_A[ c ].value = T();
 f_A[ c ].next = f_free;
 f_free = c;
 --f_used;
 ADS_INVARIANT( partition() );
 }

template< class T >
long CursorSpace< T >::capacity( void ) const
{
 return( long( f_A.size() ) - 1 );
 }

template< class T >
long CursorSpace< T >::used( void ) const
{
 return( f_used );
 }

template< class T >
bool CursorSpace< T >::partition( void ) const
{
 long free = 0;
 for( long c = f_free ; c != 0 ; c = f_A[ c ].next )
  if( ( c < 1 ) || ( c > capacity() ) || ( ++free > capacity() ) )
   return( false );                        // out of the vector, or a cycle
 return( free + f_used == capacity() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- CONSTRUCTING AND COPYING -----------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
CursorList< T >::CursorList( void )
 : f_S( std::make_shared< CursorSpace< T > >() )
{
 }

template< class T >
CursorList< T >::CursorList( std::shared_ptr< CursorSpace< T > > S )
 : f_S( std::move( S ) )
{
 ADS_PRE( f_S != nullptr );
 }

template< class T >
CursorList< T >::CursorList( std::initializer_list< T > items )
 : CursorList()
{
 Pos p = 0;
 for( const auto & v : items ) {
  insert( p , v );
  p = next( p );
  }
 }

template< class T >
CursorList< T >::CursorList( const CursorList & other ) : f_S( other.f_S )
{
 Pos p = 0;
 for( long c = other.f_first ; c ; c = ( *f_S )[ c ].next ) {
  insert( p , ( *f_S )[ c ].value );
  p = next( p );
  }
 }

template< class T >
CursorList< T >::CursorList( CursorList && other ) noexcept
 : f_S( other.f_S ) , f_first( std::exchange( other.f_first , 0 ) )
{
 }

template< class T >
CursorList< T > & CursorList< T >::operator=( CursorList other ) noexcept
{
 swap( other );
 return( *this );
 }

template< class T >
CursorList< T >::~CursorList()
{
 if( f_S )
  while( ! isEmpty() )
   remove( head() );
 }

template< class T >
void CursorList< T >::swap( CursorList & other ) noexcept
{
 std::swap( f_S , other.f_S );
 std::swap( f_first , other.f_first );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE SEQUENCE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool CursorList< T >::isEmpty( void ) const
{
 return( f_first == 0 );
 }

template< class T >
bool CursorList< T >::finished( Pos p ) const
{
 if( p == -1 )
  return( true );
 return( p ? ( ( *f_S )[ p ].next == 0 ) : ( f_first == 0 ) );
 }

template< class T >
typename CursorList< T >::Pos CursorList< T >::head( void ) const
{
 return( 0 );
 }

template< class T >
typename CursorList< T >::Pos CursorList< T >::tail( void ) const
{
 if( ! f_first )
  return( -1 );
 Pos p = 0;
 while( ! finished( next( p ) ) )
  p = next( p );
 return( p );
 }

template< class T >
typename CursorList< T >::Pos CursorList< T >::next( Pos p ) const
{
 if( p == -1 )
  return( 0 );
 ADS_PRE( ! finished( p ) );
 return( cell( p ) );
 }

template< class T >
typename CursorList< T >::Pos CursorList< T >::prev( Pos p ) const
{
 if( p == 0 )
  return( -1 );
 if( p == f_first )
  return( 0 );
 long q = f_first;                         // a scan for the cell before p
 while( ( *f_S )[ q ].next != p )
  q = ( *f_S )[ q ].next;
 return( q );
 }

template< class T >
typename CursorList< T >::Pos CursorList< T >::insert( Pos p , const T & v )
{
 ADS_PRE( p != -1 );
 const T x = v;                            // v may be in the space, which
 const long c = f_S->take();               // take() may move: no reference
 ( *f_S )[ c ].value = x;                  // into it is held across it
 long & l = link( p );
 ( *f_S )[ c ].next = l;
 l = c;
 ADS_POST( detail::equal( read( p ) , x ) );
 return( p );
 }

template< class T >
typename CursorList< T >::Pos CursorList< T >::remove( Pos p )
{
 ADS_PRE( ! finished( p ) );
 long & l = link( p );
 const long c = l;
 l = ( *f_S )[ c ].next;
 f_S->give( c );
 return( p );
 }

template< class T >
const T & CursorList< T >::read( Pos p ) const
{
 ADS_PRE( ! finished( p ) );
 return( ( *f_S )[ cell( p ) ].value );
 }

template< class T >
void CursorList< T >::write( Pos p , const T & v )
{
 ADS_PRE( ! finished( p ) );
 ( *f_S )[ cell( p ) ].value = v;
 ADS_POST( detail::equal( read( p ) , v ) );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- BEYOND THE BOOK -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
bool CursorList< T >::operator==( const CursorList & o ) const
{
 long a = f_first , b = o.f_first;
 for( ; a && b ; a = ( *f_S )[ a ].next , b = ( *o.f_S )[ b ].next )
  if( !( ( *f_S )[ a ].value == ( *o.f_S )[ b ].value ) )
   return( false );
 return( ! a && ! b );
 }

template< class T >
const CursorSpace< T > & CursorList< T >::space( void ) const
{
 return( *f_S );
 }

template< class T >
long CursorList< T >::first( void ) const
{
 return( f_first );
 }

template< class T >
long & CursorList< T >::link( Pos p )
{
 return( p ? ( *f_S )[ p ].next : f_first );
 }

template< class T >
long CursorList< T >::cell( Pos p ) const
{
 return( p ? ( *f_S )[ p ].next : f_first );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File CursorList.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
