/*--------------------------------------------------------------------------*/
/*--------------------------- File ListMFSet.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ListMFSet [see ListMFSet.h], with their contracts.
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

inline ListMFSet::ListMFSet( int n )
{
 ADS_PRE( n >= 0 );
 f_head.resize( n );
 f_list.resize( n );
 for( int i = 0 ; i < n ; ++i ) {
  f_head[ i ] = i;
  f_list[ i ] = { i };
  }
 }

inline int ListMFSet::n( void ) const
{
 return( int( f_head.size() ) );
 }

inline int ListMFSet::find( int x ) const
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 return( f_head[ x ] );
 }

inline bool ListMFSet::merge( int x , int y )
{
 int a = find( x ) , b = find( y );
 if( a == b ) {
  ADS_INVARIANT( listed() );
  return( false );
  }
 if( f_list[ a ].size() < f_list[ b ].size() )
  std::swap( a , b );
 for( int z : f_list[ b ] ) {
  f_head[ z ] = a;
  f_list[ a ].push_back( z );
  ++f_relabels;
  }
 f_list[ b ].clear();
 ADS_POST( find( x ) == find( y ) );
 ADS_INVARIANT( listed() );
 return( true );
 }

inline bool ListMFSet::listed( void ) const
{
 std::size_t total = 0;
 for( int h = 0 ; h < n() ; ++h ) {
  total += f_list[ h ].size();
  for( int z : f_list[ h ] )
   if( ( z < 0 ) || ( z >= n() ) || ( f_head[ z ] != h ) )
    return( false );
  }
 return( total == f_head.size() );
 }

inline long ListMFSet::relabels( void ) const
{
 return( f_relabels );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File ListMFSet.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
