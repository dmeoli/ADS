/*--------------------------------------------------------------------------*/
/*------------------------ File FastExactCover.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of FastExactCover.h, with their contracts.
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

inline FastExactCover::FastExactCover( int primary , int secondary )
 : f_primary( primary ) , f_items( primary + secondary )
{
 ADS_PRE( ( primary >= 0 ) && ( secondary >= 0 ) );
 }

inline int FastExactCover::addOption( const std::vector< int > & items ,
                                      const std::vector< int > & colors )
{
 ADS_PRE( ! items.empty() );
 ADS_PRE( colors.empty() || ( colors.size() == items.size() ) );
 std::vector< int > s = items;
 std::sort( s.begin() , s.end() );
 ADS_PRE( ( s.front() >= 0 ) && ( s.front() < f_primary ) &&
          ( s.back() < f_items ) &&
          ( std::adjacent_find( s.begin() , s.end() ) == s.end() ) );
 for( std::size_t k = 0 ; k < colors.size() ; ++k )
  ADS_PRE( ( colors[ k ] >= 0 ) &&
           ( ( colors[ k ] == 0 ) || ( items[ k ] >= f_primary ) ) );
 f_options.push_back( items );
 f_colors.push_back( colors.empty() ? std::vector< int >( items.size() , 0 )
                                    : colors );
 return( int( f_options.size() ) - 1 );
 }

// the node 0 is a spacer of no option; the nodes of the option o follow,
// then its spacer, whose item is minus the size of o and whose loc is o

inline void FastExactCover::build( void )
{
 const int N = f_items;
 f_node.assign( 1 , Node{ -1 , -1 , 0 , 0 } );
 f_item.assign( N , Item{ 0 , 0 , 0 , 0 } );
 for( const auto & o : f_options )
  for( int i : o ) ++f_item[ i ].size;
 int at = 0;
 for( int i = 0 ; i < N ; ++i ) {
  f_item[ i ].start = at;
  at += f_item[ i ].size;
  f_item[ i ].size = 0;
  f_item[ i ].pos = i;
  }
 f_set.assign( at , 0 );
 for( std::size_t o = 0 ; o < f_options.size() ; ++o ) {
  const auto & op = f_options[ o ];
  for( std::size_t k = 0 ; k < op.size() ; ++k ) {
   Item & I = f_item[ op[ k ] ];
   const int x = int( f_node.size() ) , l = I.start + I.size++;
   f_set[ l ] = x;
   f_node.push_back( Node{ op[ k ] , l , f_colors[ o ][ k ] , 0 } );
   }
  f_node.push_back( Node{ -int( op.size() ) , int( o ) , 0 , 0 } );
  }
 f_active.resize( N );
 for( int i = 0 ; i < N ; ++i ) f_active[ i ] = i;
 f_nActive = f_oActive = N;
 f_force.clear();
 }

inline bool FastExactCover::hide( int c , int color , bool check )
{
 Node * const nd = f_node.data();
 Item * const it = f_item.data();
 int * const set = f_set.data();
 const int s = it[ c ].start , e = s + it[ c ].size;
 for( int rr = s ; rr < e ; ++rr ) {
  const int tt = set[ rr ];
  if( color && ( nd[ tt ].color == color ) ) continue;
  for( int nn = tt + 1 ; nn != tt ; ) {     // the option off its other sets
   const int uu = nd[ nn ].item;
   if( uu < 0 ) {
    nn += uu;
    continue;
    }
   Item & U = it[ uu ];
   if( U.pos < f_oActive ) {
    const int ss = U.size - 1;
    if( ( ss <= 1 ) && check && ( uu < f_primary ) &&
        ( U.pos < f_nActive ) ) {
     if( ss == 0 ) return( false );         // uncoverable: give up
     f_force.push_back( uu );               // a forced move
     }
    const int vv = nd[ nn ].loc , last = U.start + ss , nnp = set[ last ];
    set[ last ] = nn;
    set[ vv ] = nnp;
    nd[ nn ].loc = last;
    nd[ nnp ].loc = vv;
    U.size = ss;
    ++f_updates;
    }
   ++nn;
   }
  }
 return( true );
 }

inline int FastExactCover::optionOf( int x ) const
{
 while( f_node[ x ].item >= 0 ) ++x;
 return( f_node[ x ].loc );
 }

inline bool FastExactCover::solution( const std::vector< int > & options )
                                                                       const
{
 std::vector< int > count( f_items , 0 ) , color( f_items , 0 );
 for( int o : options )
  for( std::size_t k = 0 ; k < f_options[ o ].size() ; ++k ) {
   const int it = f_options[ o ][ k ] , c = f_colors[ o ][ k ];
   if( ( c > 0 ) && ( color[ it ] == c ) && ( count[ it ] > 0 ) ) continue;
   if( ( c > 0 ) && ( count[ it ] == 0 ) ) color[ it ] = c;
   ++count[ it ];
   }
 for( int it = 0 ; it < f_items ; ++it )
  if( ( it < f_primary ) ? ( count[ it ] != 1 ) : ( count[ it ] > 1 ) )
   return( false );
 return( true );
 }

template< class F >
long FastExactCover::solve( F && visit )
{
 build();
 f_updates = f_nodes = 0;
 const int P = f_primary;
 std::vector< int > choice( std::size_t( P ) + 1 ) ,
                    saved( std::size_t( P ) + 2 , 0 ) , chosen;
 std::vector< std::pair< int , int > > stack;   // ( item , size )
 Node * const nd = f_node.data();
 Item * const it = f_item.data();
 int * const set = f_set.data();
 int * const act = f_active.data();
 long count = 0;
 int level = 0 , best = 0 , cur = 0;
 auto swapOut = [ & ]( int x ) {            // x off the active items
  const int p = --f_nActive , pp = it[ x ].pos , y = act[ p ];
  act[ p ] = x;
  act[ pp ] = y;
  it[ y ].pos = pp;
  it[ x ].pos = p;
  };
 for( ; ; ) {
 forward:
  ++f_nodes;
  while( ! f_force.empty() ) {              // a forced move still to do
   best = f_force.back();
   f_force.pop_back();
   if( it[ best ].pos < f_nActive ) goto forced;
   }
  {                                         // the fewest options
   int t = -1;
   for( int k = 0 ; k < f_nActive ; ++k ) {
    const int x = act[ k ];
    if( x >= P ) continue;
    const int s = it[ x ].size;
    if( s <= 1 ) {
     if( s == 0 ) {                         // uncoverable from the start
      f_force.clear();
      goto backup;
      }
     f_force.push_back( x );
     }
    else if( ( t < 0 ) || ( s < t ) || ( ( s == t ) && ( x < best ) ) ) {
     best = x;
     t = s;
     }
    }
   if( ! f_force.empty() ) {
    best = f_force.back();
    f_force.pop_back();
    goto forced;
    }
   if( t < 0 ) {                            // every primary item covered
    ++count;
    chosen.clear();
    for( int k = 0 ; k < level ; ++k )
     chosen.push_back( optionOf( choice[ k ] ) );
    ADS_AUDIT( solution( chosen ) );
    if constexpr( std::is_void_v< std::invoke_result_t<
                   F & , const std::vector< int > & > > )
     visit( std::as_const( chosen ) );
    else if( visit( std::as_const( chosen ) ) )
     return( count );
    goto backup;
    }
   }
  swapOut( best );                          // branch on best
  f_oActive = f_nActive;
  hide( best , 0 , false );
  cur = it[ best ].start;
  {                                         // the sizes of the level
   const int base = saved[ level ];
   if( std::size_t( base + f_nActive ) > stack.size() )
    stack.resize( std::size_t( base + f_nActive ) );
   for( int k = 0 ; k < f_nActive ; ++k )
    stack[ base + k ] = std::pair( act[ k ] , it[ act[ k ] ].size );
   saved[ level + 1 ] = base + f_nActive;
   }
  goto advance;
 forced:                                    // one option: nothing saved
  swapOut( best );
  f_oActive = f_nActive;
  hide( best , 0 , false );
  cur = it[ best ].start;
  saved[ level + 1 ] = saved[ level ];
 advance:
  {
   const int x = choice[ level ] = set[ cur ];
   int p = f_oActive = f_nActive;           // its other items out
   for( int q = x + 1 ; q != x ; ) {
    const int c = nd[ q ].item;
    if( c < 0 ) {
     q += c;
     continue;
     }
    const int pp = it[ c ].pos;
    if( pp < p ) {
     const int y = act[ --p ];
     act[ p ] = c;
     act[ pp ] = y;
     it[ y ].pos = pp;
     it[ c ].pos = p;
     }
    ++q;
    }
   f_nActive = p;
   for( int q = x + 1 ; q != x ; ) {        // and their options hidden
    const int c = nd[ q ].item;
    if( c < 0 ) {
     q += c;
     continue;
     }
    if( ( c < P ) ? ! hide( c , 0 , true )
                  : ( ( it[ c ].pos < f_oActive ) &&
                      ! hide( c , nd[ q ].color , true ) ) ) {
     f_force.clear();
     goto abort;
     }
    ++q;
    }
   ++level;
   goto forward;
   }
 backup:
  if( level == 0 ) return( count );
  --level;
  best = nd[ choice[ level ] ].item;
  cur = nd[ choice[ level ] ].loc;
 abort:
  if( cur + 1 >= it[ best ].start + it[ best ].size ) goto backup;
  {                                         // the sizes of the level back
   const int base = saved[ level ] , top = saved[ level + 1 ];
   f_nActive = top - base;
   for( int k = base ; k < top ; ++k )
    it[ stack[ k ].first ].size = stack[ k ].second;
   }
  ++cur;
  goto advance;
  }
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File FastExactCover.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
