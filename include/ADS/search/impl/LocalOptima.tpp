/*--------------------------------------------------------------------------*/
/*-------------------------- File LocalOptima.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of LocalOptima.h [see LocalOptima.h],
 * with their contracts.
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

template< class T , class C >
std::size_t findPeak( const std::vector< T > & A , C less )
{
 ADS_PRE( ! A.empty() );
 const std::size_t n = A.size();
 std::size_t lo = 0 , hi = n - 1;           // A[ lo .. hi ] holds a peak
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( less( A[ m ] , A[ m + 1 ] ) )
   lo = m + 1;                              // A[ lo - 1 ] < A[ lo ]
  else
   hi = m;                                  // A[ hi + 1 ] <= A[ hi ]
  }
 ADS_POST( detail::contract( [ & ]( void ) {
  return( ( ( lo == 0 ) || ! less( A[ lo ] , A[ lo - 1 ] ) ) &&
          ( ( lo == n - 1 ) || ! less( A[ lo ] , A[ lo + 1 ] ) ) ); } ) );
 return( lo );
 }

template< class T , class C >
std::size_t localMinimum( const std::vector< T > & A , C less )
{
 return( findPeak( A , [ & ]( const T & a , const T & b ) {
  return( less( b , a ) ); } ) );
 }

template< class T , class C >
BinaryTree< T > * localMinimum( BinaryTree< T > * t , C less )
{
 ADS_PRE( t != nullptr );
 BinaryTree< T > * const root = t;
 for( ; ; ) {
  BinaryTree< T > * next = nullptr;
  for( BinaryTree< T > * u : { t->left() , t->right() } )
   if( u && less( u->read() , t->read() ) ) {
    next = u;                               // the first smaller child
    break;
    }
  if( ! next )
   break;
  t = next;
  }
 ADS_POST( detail::contract( [ & ]( void ) {
  for( const BinaryTree< T > * u : { t->left() , t->right() } )
   if( u && less( u->read() , t->read() ) )
    return( false );
  return( ( t == root ) || less( t->read() , t->parent()->read() ) ); } ) );
 return( t );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LocalOptima.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
