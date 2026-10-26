/*--------------------------------------------------------------------------*/
/*-------------------------- File ParentTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ParentTree [see ParentTree.h], with their contracts.
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

inline ParentTree::ParentTree( int n )
 : f_p( n >= 0 ? n : 0 , -1 )
{
 ADS_PRE( n >= 0 );
 }

inline int ParentTree::parent( int i ) const
{
 ADS_PRE( ( i >= 0 ) && ( i < size() ) );
 return( f_p[ i ] );
 }

inline void ParentTree::setParent( int i , int j )
{
 ADS_PRE( ( i >= 0 ) && ( i < size() ) && ( j >= -1 ) && ( j < size() ) &&
          ( i != j ) );
 ADS_AUDIT( ! below( j , i ) );
 f_p[ i ] = j;
 ADS_POST( parent( i ) == j );
 }

inline int ParentTree::size( void ) const
{
 return( int( f_p.size() ) );
 }

inline bool ParentTree::below( int j , int i ) const
{
 for( ; j != -1 ; j = f_p[ j ] )
  if( j == i )
   return( true );
 return( false );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File ParentTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
