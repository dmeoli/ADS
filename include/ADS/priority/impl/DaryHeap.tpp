/*--------------------------------------------------------------------------*/
/*--------------------------- File DaryHeap.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DaryHeap [see DaryHeap.h], with their contracts.
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

template< class T , class Better , unsigned D >
DaryHeap< T , Better , D >::DaryHeap( Better better )
 : f_better( std::move( better ) )
{
 }

template< class T , class Better , unsigned D >
void DaryHeap< T , Better , D >::push( const T & x )
{
 f_A.push_back( x );
 up( f_A.size() - 1 );
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better , unsigned D >
const T & DaryHeap< T , Better , D >::top( void ) const
{
 ADS_PRE( ! f_A.empty() );
 return( f_A.front() );
 }

template< class T , class Better , unsigned D >
T DaryHeap< T , Better , D >::pop( void )
{
 ADS_PRE( ! f_A.empty() );
 T x = std::move( f_A.front() );
 f_A.front() = std::move( f_A.back() );
 f_A.pop_back();
 if( ! f_A.empty() )
  down( 0 );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better , unsigned D >
bool DaryHeap< T , Better , D >::isEmpty( void ) const
{
 return( f_A.empty() );
 }

template< class T , class Better , unsigned D >
std::size_t DaryHeap< T , Better , D >::size( void ) const
{
 return( f_A.size() );
 }

template< class T , class Better , unsigned D >
void DaryHeap< T , Better , D >::reserve( std::size_t n )
{
 f_A.reserve( n );
 }

template< class T , class Better , unsigned D >
void DaryHeap< T , Better , D >::clear( void )
{
 f_A.clear();
 }

template< class T , class Better , unsigned D >
void DaryHeap< T , Better , D >::up( std::size_t i )
{
 T x = std::move( f_A[ i ] );               // a hole moves up, no swaps
 while( i > 0 ) {
  const std::size_t p = ( i - 1 ) / D;
  if( ! f_better( x , f_A[ p ] ) )
   break;
  f_A[ i ] = std::move( f_A[ p ] );
  i = p;
  }
 f_A[ i ] = std::move( x );
 }

template< class T , class Better , unsigned D >
void DaryHeap< T , Better , D >::down( std::size_t i )
{
 T x = std::move( f_A[ i ] );
 const std::size_t n = f_A.size();
 for( ; ; ) {
  const std::size_t c = D * i + 1;
  if( c >= n )
   break;
  std::size_t b = c;                        // the best child
  const std::size_t last = c + D < n ? c + D : n;
  for( std::size_t k = c + 1 ; k < last ; ++k )
   if( f_better( f_A[ k ] , f_A[ b ] ) )
    b = k;
  if( ! f_better( f_A[ b ] , x ) )
   break;
  f_A[ i ] = std::move( f_A[ b ] );
  i = b;
  }
 f_A[ i ] = std::move( x );
 }

template< class T , class Better , unsigned D >
bool DaryHeap< T , Better , D >::ordered( void ) const
{
 for( std::size_t i = 1 ; i < f_A.size() ; ++i )
  if( f_better( f_A[ i ] , f_A[ ( i - 1 ) / D ] ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File DaryHeap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
