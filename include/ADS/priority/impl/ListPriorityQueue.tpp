/*--------------------------------------------------------------------------*/
/*----------------------- File ListPriorityQueue.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ListPriorityQueue [see ListPriorityQueue.h], with
 * their contracts.
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

template< class T , std::totally_ordered P , bool Sorted >
bool ListPriorityQueue< T , P , Sorted >::empty( void ) const
{
 return( f_L.empty() );
 }

template< class T , std::totally_ordered P , bool Sorted >
int ListPriorityQueue< T , P , Sorted >::size( void ) const
{
 return( f_size );
 }

template< class T , std::totally_ordered P , bool Sorted >
const T & ListPriorityQueue< T , P , Sorted >::min( void ) const
{
 ADS_PRE( ! empty() );
 return( f_L.read( first() ).second );
 }

template< class T , std::totally_ordered P , bool Sorted >
T ListPriorityQueue< T , P , Sorted >::deleteMin( void )
{
 ADS_PRE( ! empty() );
 const auto p = first();
 const P q = f_L.read( p ).first;
 T v = f_L.read( p ).second;
 f_L.remove( p );
 --f_size;
 ADS_POST( empty() || !( f_L.read( first() ).first < q ) );
 ADS_INVARIANT( consistent() );
 return( v );
 }

template< class T , std::totally_ordered P , bool Sorted >
typename ListPriorityQueue< T , P , Sorted >::Handle
ListPriorityQueue< T , P , Sorted >::insert( const T & x , const P & p )
{
 ++f_size;
 Handle h;
 if constexpr( Sorted )
  h = f_L.insert( after( p ) , { p , x } );
 else
  h = f_L.insert( f_L.head() , { p , x } );
 ADS_INVARIANT( consistent() );
 return( h );
 }

template< class T , std::totally_ordered P , bool Sorted >
void ListPriorityQueue< T , P , Sorted >::decrease( Handle & y , const P & p )
{
 ADS_AUDIT( owns( y ) );
 ADS_PRE( !( f_L.read( y ).first < p ) );
 if constexpr( Sorted ) {
  const T x = f_L.read( y ).second;
  f_L.remove( y );
  y = f_L.insert( after( p ) , { p , x } );
  }
 else
  f_L.write( y , { p , f_L.read( y ).second } );
 ADS_POST( f_L.read( y ).first == p );
 ADS_INVARIANT( consistent() );
 }

template< class T , std::totally_ordered P , bool Sorted >
typename ListPriorityQueue< T , P , Sorted >::Handle
ListPriorityQueue< T , P , Sorted >::first( void ) const
{
 auto m = f_L.head();
 if constexpr( ! Sorted )
  for( auto q = f_L.next( m ) ; ! f_L.finished( q ) ; q = f_L.next( q ) )
   if( f_L.read( q ).first < f_L.read( m ).first )
    m = q;
 return( m );
 }

template< class T , std::totally_ordered P , bool Sorted >
typename ListPriorityQueue< T , P , Sorted >::Handle
ListPriorityQueue< T , P , Sorted >::after( const P & p ) const
{
 auto q = f_L.head();
 while( ! f_L.finished( q ) && ! ( p < f_L.read( q ).first ) )
  q = f_L.next( q );
 return( q );
 }

template< class T , std::totally_ordered P , bool Sorted >
bool ListPriorityQueue< T , P , Sorted >::owns( Handle y ) const
{
 for( auto q = f_L.head() ; ! f_L.finished( q ) ; q = f_L.next( q ) )
  if( q == y )
   return( true );
 return( false );
 }

template< class T , std::totally_ordered P , bool Sorted >
bool ListPriorityQueue< T , P , Sorted >::consistent( void ) const
{
 int n = 0;
 for( auto q = f_L.head() ; ! f_L.finished( q ) ; q = f_L.next( q ) , ++n )
  if constexpr( Sorted ) {
   const auto r = f_L.next( q );
   if( ! f_L.finished( r ) && ( f_L.read( r ).first < f_L.read( q ).first ) )
    return( false );
   }
 return( n == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File ListPriorityQueue.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
