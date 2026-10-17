/*--------------------------------------------------------------------------*/
/*----------------------- File HeapPriorityQueue.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of HeapPriorityQueue [see HeapPriorityQueue.h], with
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
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
HeapPriorityQueue< T , P , D >::HeapPriorityQueue( int n ) : f_capacity( n )
{
 ADS_PRE( n >= 0 );
 f_H.reserve( n );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
bool HeapPriorityQueue< T , P , D >::isEmpty( void ) const
{
 return( f_H.empty() );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
int HeapPriorityQueue< T , P , D >::size( void ) const
{
 return( int( f_H.size() ) );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
int HeapPriorityQueue< T , P , D >::capacity( void ) const
{
 return( f_capacity );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
const T & HeapPriorityQueue< T , P , D >::min( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_H[ 0 ]->value );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
const P & HeapPriorityQueue< T , P , D >::minPriority( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_H[ 0 ]->priority );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
T HeapPriorityQueue< T , P , D >::deleteMin( void )
{
 ADS_PRE( ! isEmpty() );
 const int dim = size() - 1;
 const P p = f_H[ 0 ]->priority;
 swap( 0 , dim );
 minHeapRestore( 0 , dim );
 T v = std::move( f_H.back()->value );
 f_H.pop_back();
 ADS_POST( ( size() == dim ) && ( isEmpty() || !( minPriority() < p ) ) );
 ADS_INVARIANT( isHeap() );
 return( v );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
typename HeapPriorityQueue< T , P , D >::Handle
HeapPriorityQueue< T , P , D >::insert( const T & x , const P & p )
{
 ADS_PRE( size() < capacity() );
 const int i = size();
 f_H.push_back( std::make_unique< PriorityItem >(
                                           PriorityItem{ p , x , i } ) );
 Handle h = f_H.back().get();
 up( i );
 ADS_POST( ( size() == i + 1 ) && owns( h ) );
 ADS_INVARIANT( isHeap() );
 return( h );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
void HeapPriorityQueue< T , P , D >::decrease( Handle y , const P & p )
{
 ADS_PRE( owns( y ) && !( y->priority < p ) );
 y->priority = p;
 up( y->pos );
 ADS_POST( owns( y ) && ( y->priority == p ) );
 ADS_INVARIANT( isHeap() );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
std::vector< P > HeapPriorityQueue< T , P , D >::priorities( void ) const
{
 std::vector< P > out;
 for( const auto & h : f_H )
  out.push_back( h->priority );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE HEAP ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
void HeapPriorityQueue< T , P , D >::swap( int i , int j )
{
 std::swap( f_H[ i ] , f_H[ j ] );
 f_H[ i ]->pos = i;
 f_H[ j ]->pos = j;
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
void HeapPriorityQueue< T , P , D >::up( int i )
{
 while( ( i > 0 ) && ( f_H[ i ]->priority < f_H[ parent( i ) ]->priority ) ) {
  swap( i , parent( i ) );
  i = parent( i );
  }
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
void HeapPriorityQueue< T , P , D >::minHeapRestore( int i , int dim )
{
 for( ; ; ) {
  int min = i;
  for( int j = 0 ; j < D ; ++j ) {
   const int c = child( i , j );
   if( ( c < dim ) && ( f_H[ c ]->priority < f_H[ min ]->priority ) )
    min = c;
   }
  if( min == i )
   return;
  swap( i , min );
  i = min;
  }
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
bool HeapPriorityQueue< T , P , D >::owns( Handle y ) const
{
 return( ( y != nullptr ) && ( y->pos >= 0 ) && ( y->pos < size() ) &&
         ( f_H[ y->pos ].get() == y ) );
 }

template< class T , std::totally_ordered P , int D >
 requires( D >= 2 )
bool HeapPriorityQueue< T , P , D >::isHeap( void ) const
{
 if( size() > 1024 ) {                      // the root and the last item
  const int l = size() - 1;
  for( int i : { 0 , l } )
   if( f_H[ i ]->pos != i )
    return( false );
  for( int j = 0 ; ( j < D ) && ( child( 0 , j ) < size() ) ; ++j )
   if( f_H[ child( 0 , j ) ]->priority < f_H[ 0 ]->priority )
    return( false );
  return( !( f_H[ l ]->priority < f_H[ parent( l ) ]->priority ) );
  }
 for( int i = 0 ; i < size() ; ++i ) {
  if( f_H[ i ]->pos != i )
   return( false );
  if( ( i > 0 ) && ( f_H[ i ]->priority < f_H[ parent( i ) ]->priority ) )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File HeapPriorityQueue.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
