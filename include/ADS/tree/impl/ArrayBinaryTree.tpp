/*--------------------------------------------------------------------------*/
/*------------------------ File ArrayBinaryTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ArrayBinaryTree [see ArrayBinaryTree.h], with their
 * contracts.
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

template< class T >
ArrayBinaryTree< T >::ArrayBinaryTree( const T & v )
 : f_A( 1 , v )
{
 }

template< class T >
ArrayBinaryTree< T >::ArrayBinaryTree( const T & v ,
                                       const ArrayBinaryTree & l ,
                                       const ArrayBinaryTree & r )
 : ArrayBinaryTree( v )
{
 if( ! l.empty() ) insertLeft( 0 , l );
 if( ! r.empty() ) insertRight( 0 , r );
 }

template< class T >
bool ArrayBinaryTree< T >::empty( void ) const
{
 return( ! has( 0 ) );
 }

template< class T >
long ArrayBinaryTree< T >::root( void ) const
{
 return( has( 0 ) ? 0 : NIL );
 }

template< class T >
long ArrayBinaryTree< T >::parent( long i ) const
{
 ADS_PRE( has( i ) );
 return( i > 0 ? dary_parent( i , 2 ) : NIL );
 }

template< class T >
long ArrayBinaryTree< T >::left( long i ) const
{
 ADS_PRE( has( i ) );
 return( has( 2 * i + 1 ) ? 2 * i + 1 : NIL );
 }

template< class T >
long ArrayBinaryTree< T >::right( long i ) const
{
 ADS_PRE( has( i ) );
 return( has( 2 * i + 2 ) ? 2 * i + 2 : NIL );
 }

template< class T >
bool ArrayBinaryTree< T >::isLeaf( long i ) const
{
 ADS_PRE( has( i ) );
 return( ( left( i ) == NIL ) && ( right( i ) == NIL ) );
 }

template< class T >
const T & ArrayBinaryTree< T >::read( long i ) const
{
 return( *at( i ) );
 }

template< class T >
void ArrayBinaryTree< T >::write( long i , const T & v )
{
 *at( i ) = v;
 }

template< class T >
void ArrayBinaryTree< T >::setRoot( const T & v )
{
 ADS_PRE( empty() );
 place( 0 ) = v;
 ADS_POST( root() == 0 );
 ADS_INVARIANT( rooted() );
 }

template< class T >
void ArrayBinaryTree< T >::insertLeft( long i , const ArrayBinaryTree & t )
{
 graft( i , 2 * i + 1 , t );
 ADS_POST( t.empty() || ( left( i ) == 2 * i + 1 ) );
 }

template< class T >
void ArrayBinaryTree< T >::insertRight( long i , const ArrayBinaryTree & t )
{
 graft( i , 2 * i + 2 , t );
 ADS_POST( t.empty() || ( right( i ) == 2 * i + 2 ) );
 }

template< class T >
void ArrayBinaryTree< T >::remove( long i )
{
 clear( i );
 ADS_POST( ! has( i ) );
 ADS_INVARIANT( rooted() );
 }

template< class T >
void ArrayBinaryTree< T >::clear( long i )
{
 if( ! has( i ) ) return;
 f_A[ i ].reset();
 clear( 2 * i + 1 );
 clear( 2 * i + 2 );
 }

template< class T >
bool ArrayBinaryTree< T >::rooted( void ) const
{
 for( long i = 1 ; i < long( f_A.size() ) ; ++i )
  if( f_A[ i ].has_value() && ! has( dary_parent( i , 2 ) ) )
   return( false );
 return( true );
 }

template< class T >
std::vector< long > ArrayBinaryTree< T >::preorder( void ) const
{
 std::vector< long > P;
 auto rec = [ & ]( auto & self , long i ) -> void {
  if( ! has( i ) ) return;
  P.push_back( i );
  self( self , 2 * i + 1 );
  self( self , 2 * i + 2 );
  };
 rec( rec , 0 );
 return( P );
 }

template< class T >
bool ArrayBinaryTree< T >::has( long i ) const
{
 return( ( i >= 0 ) && ( i < long( f_A.size() ) ) && f_A[ i ].has_value() );
 }

template< class T >
std::optional< T > & ArrayBinaryTree< T >::place( long i )
{
 if( i >= long( f_A.size() ) ) f_A.resize( i + 1 );
 return( f_A[ i ] );
 }

template< class T >
const std::optional< T > & ArrayBinaryTree< T >::at( long i ) const
{
 ADS_PRE( has( i ) );
 return( f_A[ i ] );
 }

template< class T >
std::optional< T > & ArrayBinaryTree< T >::at( long i )
{
 ADS_PRE( has( i ) );
 return( f_A[ i ] );
 }

template< class T >
void ArrayBinaryTree< T >::copy( long k , const ArrayBinaryTree & t , long j )
{
 if( ! t.has( j ) ) return;
 place( k ) = t.f_A[ j ];
 copy( 2 * k + 1 , t , 2 * j + 1 );
 copy( 2 * k + 2 , t , 2 * j + 2 );
 }

template< class T >
void ArrayBinaryTree< T >::graft( long i , long c ,
                                  const ArrayBinaryTree & t )
{
 ADS_PRE( has( i ) && ! has( c ) );
 if( &t == this ) {                     // t would grow under the copy
  const ArrayBinaryTree s( t );
  copy( c , s , 0 );
  }
 else
  copy( c , t , 0 );
 ADS_INVARIANT( rooted() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ArrayBinaryTree.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
