/*--------------------------------------------------------------------------*/
/*----------------------------- File MFSet.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MFSet [see MFSet.h], with their contracts.
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

inline MFSet::MFSet( int n , Union u , bool compress )
 : f_union( u ) , f_compress( compress ) , f_count( n )
{
 ADS_PRE( n >= 0 );
 f_p.resize( n );
 f_rank.assign( n , 0 );
 f_size.assign( n , 1 );
 for( int i = 0 ; i < n ; ++i )
  f_p[ i ] = i;
 }

inline int MFSet::n( void ) const
{
 return( int( f_p.size() ) );
 }

inline int MFSet::count( void ) const
{
 return( f_count );
 }

inline int MFSet::find( int x )
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 int r = x;
 while( f_p[ r ] != r ) {
  r = f_p[ r ];
  ++f_steps;
  }
 if( f_compress )
  while( f_p[ x ] != r ) {
   const int t = f_p[ x ];
   f_p[ x ] = r;
   x = t;
   }
 ADS_POST( f_p[ r ] == r );
 return( r );
 }

inline bool MFSet::merge( int x , int y )
{
 const int rx = find( x ) , ry = find( y );
 if( rx == ry ) {
  ADS_INVARIANT( forest() );
  return( false );
  }
 --f_count;
 if( f_union == Union::NONE )
  f_p[ ry ] = rx;
 else if( f_union == Union::SIZE ) {
  const int big = f_size[ rx ] >= f_size[ ry ] ? rx : ry;
  const int small = big == rx ? ry : rx;
  f_p[ small ] = big;
  f_size[ big ] += f_size[ small ];
  }
 else if( f_rank[ rx ] > f_rank[ ry ] )
  f_p[ ry ] = rx;
 else if( f_rank[ ry ] > f_rank[ rx ] )
  f_p[ rx ] = ry;
 else {
  f_p[ rx ] = ry;
  ++f_rank[ ry ];
  }
 ADS_POST( root( x ) == root( y ) );
 ADS_INVARIANT( forest() );
 return( true );
 }

inline bool MFSet::same( int x , int y )
{
 return( find( x ) == find( y ) );
 }

inline int MFSet::parent( int x ) const
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 return( f_p[ x ] );
 }

inline int MFSet::rank( int x ) const
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 return( f_rank[ x ] );
 }

inline int MFSet::size( int x ) const
{
 ADS_PRE( ( x >= 0 ) && ( x < n() ) );
 return( f_size[ x ] );
 }

inline long MFSet::steps( void ) const
{
 return( f_steps );
 }

inline int MFSet::root( int x ) const
{
 while( f_p[ x ] != x )
  x = f_p[ x ];
 return( x );
 }

inline bool MFSet::forest( void ) const
{
 const int N = n();
 int roots = 0;
 std::vector< char > state( N , 0 );     // 0 new, 1 on the path, 2 done
 for( int x = 0 ; x < N ; ++x ) {
  if( ( f_p[ x ] < 0 ) || ( f_p[ x ] >= N ) )
   return( false );
  if( f_p[ x ] == x ) ++roots;
  else if( ( f_union == Union::RANK ) &&
           !( f_rank[ x ] < f_rank[ f_p[ x ] ] ) )
   return( false );                      // the rank grows upwards
  }
 for( int x = 0 ; x < N ; ++x ) {        // no cycle but the roots
  int u = x;
  while( ( state[ u ] == 0 ) && ( f_p[ u ] != u ) ) {
   state[ u ] = 1;
   u = f_p[ u ];
   }
  if( state[ u ] == 1 )
   return( false );
  for( u = x ; state[ u ] == 1 ; u = f_p[ u ] )
   state[ u ] = 2;
  state[ u ] = 2;
  }
 if( f_union == Union::SIZE ) {          // the sizes count the nodes
  std::vector< int > count( N , 0 );
  for( int x = 0 ; x < N ; ++x )
   ++count[ root( x ) ];
  for( int x = 0 ; x < N ; ++x )
   if( ( f_p[ x ] == x ) && ( count[ x ] != f_size[ x ] ) )
    return( false );
  }
 return( roots == f_count );
 }

template< Graph G >
MFSet components( const G & g )
{
 MFSet M( g.bound() );
 for( int u : g.V() )
  g.forEachAdj( u , [ & ]( int v ) { M.merge( u , v ); } );
 return( M );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File MFSet.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
