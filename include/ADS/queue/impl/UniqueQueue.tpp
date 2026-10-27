/*--------------------------------------------------------------------------*/
/*-------------------------- File UniqueQueue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of UniqueQueue [see UniqueQueue.h], with their contracts.
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

template< std::equality_comparable T , Duplicate D >
bool UniqueQueue< T , D >::empty( void ) const
{
 return( f_L.empty() );
 }

template< std::equality_comparable T , Duplicate D >
void UniqueQueue< T , D >::enqueue( const T & v )
{
 const auto p = find( v );
 if( ! f_L.finished( p ) ) {
  if constexpr( D == Duplicate::IGNORE_NEW ) {
   ADS_INVARIANT( unique() );
   return;
   }
  const T w = v;                            // v may be the copy that
  f_L.remove( p );                          // remove() destroys, as in
  --f_size;                                 // enqueue( top() )
  enqueue( w );
  return;
  }
 f_L.insert( f_L.next( f_L.tail() ) , v );
 ++f_size;
 ADS_POST( contains( v ) &&
           ( D == Duplicate::IGNORE_NEW || f_L.read( f_L.tail() ) == v ) );
 ADS_INVARIANT( unique() );
 }

template< std::equality_comparable T , Duplicate D >
T UniqueQueue< T , D >::dequeue( void )
{
 ADS_PRE( ! empty() );
 T v = f_L.read( f_L.head() );
 f_L.remove( f_L.head() );
 --f_size;
 return( v );
 }

template< std::equality_comparable T , Duplicate D >
const T & UniqueQueue< T , D >::top( void ) const
{
 ADS_PRE( ! empty() );
 return( f_L.read( f_L.head() ) );
 }

template< std::equality_comparable T , Duplicate D >
bool UniqueQueue< T , D >::contains( const T & v ) const
{
 return( ! f_L.finished( find( v ) ) );
 }

template< std::equality_comparable T , Duplicate D >
std::size_t UniqueQueue< T , D >::size( void ) const
{
 return( f_size );
 }

template< std::equality_comparable T , Duplicate D >
typename List< T >::Pos UniqueQueue< T , D >::find( const T & v ) const
{
 auto p = f_L.head();
 while( ! f_L.finished( p ) && !( f_L.read( p ) == v ) )
  p = f_L.next( p );
 return( p );
 }

template< std::equality_comparable T , Duplicate D >
bool UniqueQueue< T , D >::unique( void ) const
{
 for( auto p = f_L.head() ; ! f_L.finished( p ) ; p = f_L.next( p ) )
  for( auto q = f_L.next( p ) ; ! f_L.finished( q ) ; q = f_L.next( q ) )
   if( f_L.read( p ) == f_L.read( q ) )
    return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File UniqueQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
