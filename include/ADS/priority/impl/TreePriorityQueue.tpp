/*--------------------------------------------------------------------------*/
/*----------------------- File TreePriorityQueue.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of TreePriorityQueue [see TreePriorityQueue.h], with
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

template< class T , std::totally_ordered P >
bool TreePriorityQueue< T , P >::isEmpty( void ) const
{
 return( f_t.isEmpty() );
 }

template< class T , std::totally_ordered P >
int TreePriorityQueue< T , P >::size( void ) const
{
 return( int( f_t.size() ) );
 }

template< class T , std::totally_ordered P >
const T & TreePriorityQueue< T , P >::min( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( RedBlackTree< Key , T >::value( f_t.min() ) );
 }

template< class T , std::totally_ordered P >
T TreePriorityQueue< T , P >::deleteMin( void )
{
 ADS_PRE( ! isEmpty() );
 const int n = size();
 const T v = min();
 f_t.remove( RedBlackTree< Key , T >::key( f_t.min() ) );
 ADS_POST( size() == n - 1 );
 ADS_INVARIANT( consistent() );
 return( v );
 }

template< class T , std::totally_ordered P >
typename TreePriorityQueue< T , P >::Handle
TreePriorityQueue< T , P >::insert( const T & x , const P & p )
{
 const int n = size();
 f_t.insert( { p , long( f_p.size() ) } , x );
 f_p.push_back( p );
 ADS_POST( size() == n + 1 );
 ADS_INVARIANT( consistent() );
 return( long( f_p.size() ) - 1 );
 }

template< class T , std::totally_ordered P >
void TreePriorityQueue< T , P >::decrease( Handle y , const P & p )
{
 ADS_PRE( ( y >= 0 ) && ( y < long( f_p.size() ) ) &&
          f_t.contains( { f_p[ y ] , y } ) && !( f_p[ y ] < p ) );
 const auto v = f_t.lookup( { f_p[ y ] , y } );
 f_t.remove( { f_p[ y ] , y } );
 f_t.insert( { p , y } , *v );
 f_p[ y ] = p;
 ADS_INVARIANT( consistent() );
 }

template< class T , std::totally_ordered P >
bool TreePriorityQueue< T , P >::consistent( void ) const
{
 bool ok = true;
 f_t.forEach( [ & ]( const Key & k , const T & ) {
  ok = ok && ( k.second >= 0 ) && ( k.second < long( f_p.size() ) ) &&
       ( f_p[ k.second ] == k.first ); } );
 return( ok );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File TreePriorityQueue.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
