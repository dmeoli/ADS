/*--------------------------------------------------------------------------*/
/*----------------------- File PreorderArrayTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PreorderArrayTree [see PreorderArrayTree.h], with their
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
PreorderArrayTree< T >::PreorderArrayTree( const T & v )
 : f_A( 1 , { NIL , v } )
{
 }

template< class T >
bool PreorderArrayTree< T >::isEmpty( void ) const
{
 return( f_A.empty() );
 }

template< class T >
int PreorderArrayTree< T >::size( void ) const
{
 return( int( f_A.size() ) );
 }

template< class T >
int PreorderArrayTree< T >::root( void ) const
{
 return( f_A.empty() ? NIL : 0 );
 }

template< class T >
int PreorderArrayTree< T >::parent( int i ) const
{
 return( cell( i ).parent );
 }

template< class T >
const T & PreorderArrayTree< T >::read( int i ) const
{
 return( cell( i ).value );
 }

template< class T >
void PreorderArrayTree< T >::write( int i , const T & v )
{
 cell( i );
 f_A[ i ].value = v;
 }

template< class T >
int PreorderArrayTree< T >::leftmostChild( int i ) const
{
 cell( i );
 return( ( i + 1 < size() ) && ( f_A[ i + 1 ].parent == i ) ? i + 1 : NIL );
 }

template< class T >
int PreorderArrayTree< T >::rightSibling( int i ) const
{
 const int e = end( i );
 return( ( e < size() ) && ( f_A[ e ].parent == f_A[ i ].parent ) &&
         ( i > 0 ) ? e : NIL );
 }

template< class T >
bool PreorderArrayTree< T >::isLeaf( int i ) const
{
 return( leftmostChild( i ) == NIL );
 }

template< class T >
int PreorderArrayTree< T >::end( int i ) const
{
 cell( i );
 int e = i + 1;
 while( ( e < size() ) && ( f_A[ e ].parent >= i ) ) ++e;
 return( e );
 }

template< class T >
void PreorderArrayTree< T >::insertChild( int i , const PreorderArrayTree & t )
{
 cell( i );
 splice( i + 1 , i , t );
 ADS_POST( t.isEmpty() || ( leftmostChild( i ) == i + 1 ) );
 ADS_INVARIANT( preordered() );
 }

template< class T >
void PreorderArrayTree< T >::insertSibling( int i ,
                                            const PreorderArrayTree & t )
{
 ADS_PRE( cell( i ).parent != NIL );
 splice( end( i ) , f_A[ i ].parent , t );
 ADS_INVARIANT( preordered() );
 }

template< class T >
void PreorderArrayTree< T >::remove( int i )
{
 const int e = end( i ) , k = e - i;
 f_A.erase( f_A.begin() + i , f_A.begin() + e );
 for( int j = i ; j < size() ; ++j )
  if( f_A[ j ].parent >= e ) f_A[ j ].parent -= k;
 ADS_INVARIANT( preordered() );
 }

template< class T >
auto PreorderArrayTree< T >::cell( int i ) const -> const Cell &
{
 ADS_PRE( ( i >= 0 ) && ( i < size() ) );
 return( f_A[ i ] );
 }

template< class T >
bool PreorderArrayTree< T >::preordered( void ) const
{
 for( int j = 0 ; j < size() ; ++j )
  if( ( j == 0 ) ? ( f_A[ 0 ].parent != NIL )
                 : ( ( f_A[ j ].parent < 0 ) || ( f_A[ j ].parent >= j ) ) )
   return( false );
 return( true );
 }

template< class T >
void PreorderArrayTree< T >::splice( int at , int p ,
                                     const PreorderArrayTree & t )
{
 const int k = t.size();
 if( k == 0 ) return;
 for( int j = at ; j < size() ; ++j )
  if( f_A[ j ].parent >= at ) f_A[ j ].parent += k;
 std::vector< Cell > C( t.f_A );
 C[ 0 ].parent = p;
 for( int j = 1 ; j < k ; ++j ) C[ j ].parent += at;
 f_A.insert( f_A.begin() + at , C.begin() , C.end() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File PreorderArrayTree.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
