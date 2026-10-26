/*--------------------------------------------------------------------------*/
/*------------------------- File DancingLinks.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ExactCover [see DancingLinks.h], with their
 * contracts.
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

// the nodes 1 .. N are the heads of the items, the node 0 the head of the
// list of the primary items; the secondary ones are on lists of their own

inline ExactCover::ExactCover( int primary , int secondary )
 : f_primary( primary ) , f_items( primary + secondary )
{
 ADS_PRE( ( primary >= 0 ) && ( secondary >= 0 ) );
 const int N = f_items;
 f_L.resize( N + 1 );
 f_R.resize( N + 1 );
 for( int i = 0 ; i <= N ; ++i ) {
  f_L[ i ] = i - 1;
  f_R[ i ] = i + 1;
  }
 f_L[ 0 ] = primary;                        // the primary items circular
 if( primary >= 0 ) f_R[ primary ] = 0;
 for( int i = primary + 1 ; i <= N ; ++i ) {   // each secondary alone
  f_L[ i ] = i;
  f_R[ i ] = i;
  }
 f_U.resize( N + 1 );
 f_D.resize( N + 1 );
 f_top.assign( N + 1 , 0 );
 f_len.assign( N + 1 , 0 );
 f_option.assign( N + 1 , -1 );
 for( int i = 0 ; i <= N ; ++i ) f_U[ i ] = f_D[ i ] = i;
 }

inline int ExactCover::addOption( const std::vector< int > & items )
{
 ADS_PRE( ! items.empty() );
 std::vector< int > s = items;
 std::sort( s.begin() , s.end() );
 ADS_PRE( ( s.front() >= 0 ) && ( s.front() < f_primary ) &&
          ( s.back() < f_items ) &&
          ( std::adjacent_find( s.begin() , s.end() ) == s.end() ) );
 const int o = int( f_options.size() );
 f_options.push_back( items );
 for( int it : items ) {                    // a node at the bottom of the
  const int i = it + 1 , x = int( f_U.size() );   // column of each item
  f_U.push_back( f_U[ i ] );
  f_D.push_back( i );
  f_top.push_back( i );
  f_option.push_back( o );
  f_D[ f_U[ i ] ] = x;
  f_U[ i ] = x;
  ++f_len[ i ];
  }
 return( o );
 }

inline void ExactCover::hide( int p )
{
 const int o = f_option[ p ];               // the other nodes of the option
 for( int x = p + 1 ; ( x < int( f_U.size() ) ) && ( f_option[ x ] == o ) ;
      ++x ) {
  f_D[ f_U[ x ] ] = f_D[ x ];
  f_U[ f_D[ x ] ] = f_U[ x ];
  --f_len[ f_top[ x ] ];
  ++f_updates;
  }
 for( int x = p - 1 ; ( x > f_items ) && ( f_option[ x ] == o ) ; --x ) {
  f_D[ f_U[ x ] ] = f_D[ x ];
  f_U[ f_D[ x ] ] = f_U[ x ];
  --f_len[ f_top[ x ] ];
  ++f_updates;
  }
 }

inline void ExactCover::unhide( int p )
{
 const int o = f_option[ p ];               // the reverse order of hide()
 int first = p , last = p;
 while( ( first - 1 > f_items ) && ( f_option[ first - 1 ] == o ) ) --first;
 while( ( last + 1 < int( f_U.size() ) ) && ( f_option[ last + 1 ] == o ) )
  ++last;
 for( int x = first ; x < p ; ++x ) {
  f_D[ f_U[ x ] ] = x;
  f_U[ f_D[ x ] ] = x;
  ++f_len[ f_top[ x ] ];
  }
 for( int x = last ; x > p ; --x ) {
  f_D[ f_U[ x ] ] = x;
  f_U[ f_D[ x ] ] = x;
  ++f_len[ f_top[ x ] ];
  }
 }

inline void ExactCover::cover( int i )
{
 for( int p = f_D[ i ] ; p != i ; p = f_D[ p ] ) hide( p );
 f_R[ f_L[ i ] ] = f_R[ i ];                // the item off its list
 f_L[ f_R[ i ] ] = f_L[ i ];
 }

inline void ExactCover::uncover( int i )
{
 f_R[ f_L[ i ] ] = i;                       // the dance: back in place
 f_L[ f_R[ i ] ] = i;
 for( int p = f_U[ i ] ; p != i ; p = f_U[ p ] ) unhide( p );
 }

inline bool ExactCover::exact( const std::vector< int > & options ) const
{
 std::vector< int > count( f_items , 0 );
 for( int o : options )
  for( int it : f_options[ o ] ) ++count[ it ];
 for( int it = 0 ; it < f_items ; ++it )
  if( ( it < f_primary ) ? ( count[ it ] != 1 ) : ( count[ it ] > 1 ) )
   return( false );
 return( true );
 }

template< class F >
long ExactCover::solve( F && visit )
{
 f_updates = 0;
 const auto U0 = f_U , D0 = f_D , L0 = f_L , R0 = f_R;   // for the audit
 std::vector< int > chosen , nodes;
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self ) -> void {
  if( f_R[ 0 ] == 0 ) {                     // X2: every primary covered
   ++count;
   ADS_AUDIT( exact( chosen ) );
   stop = visit( std::as_const( chosen ) );
   return;
   }
  int i = f_R[ 0 ];                         // X3: the fewest options
  for( int j = f_R[ i ] ; j != 0 ; j = f_R[ j ] )
   if( f_len[ j ] < f_len[ i ] ) i = j;
  if( f_len[ i ] == 0 ) return;
  cover( i );                               // X4
  for( int x = f_D[ i ] ; ( x != i ) && ! stop ; x = f_D[ x ] ) {
   chosen.push_back( f_option[ x ] );       // X5: try the option of x
   const int o = f_option[ x ];
   for( int p = x + 1 ; ( p < int( f_U.size() ) ) && ( f_option[ p ] == o ) ;
        ++p ) cover( f_top[ p ] );
   for( int p = x - 1 ; ( p > f_items ) && ( f_option[ p ] == o ) ; --p )
    cover( f_top[ p ] );
   self( self );
   int first = x , last = x;                // X6: undo in reverse order
   while( ( first - 1 > f_items ) && ( f_option[ first - 1 ] == o ) ) --first;
   while( ( last + 1 < int( f_U.size() ) ) && ( f_option[ last + 1 ] == o ) )
    ++last;
   for( int p = first ; p < x ; ++p ) uncover( f_top[ p ] );
   for( int p = last ; p > x ; --p ) uncover( f_top[ p ] );
   chosen.pop_back();
   }
  uncover( i );                             // X8
  };
 rec( rec );
 ADS_AUDIT( ( f_U == U0 ) && ( f_D == D0 ) && ( f_L == L0 ) && ( f_R == R0 ) );
 return( count );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File DancingLinks.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
