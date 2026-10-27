/*--------------------------------------------------------------------------*/
/*------------------------- File PatriciaTrie.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PatriciaTrie [see PatriciaTrie.h], with their
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

inline int PatriciaTrie::at( std::string_view s , std::size_t i )
{
 return( i < s.size() ? int( ( unsigned char )( s[ i ] ) ) : -1 );
 }

inline PatriciaTrie::PatriciaTrie( std::vector< std::string > D )
 : f_D( std::move( D ) )
{
 ADS_AUDIT( std::adjacent_find( f_D.begin() , f_D.end() ,
                                std::greater_equal<>() ) == f_D.end() );
 const std::size_t n = f_D.size();
 f_N.push_back( Node{ 0 , 0 , n , false , {} } );
 std::vector< std::size_t > st( 1 , 0 );   // the rightmost path
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t l = i ? commonPrefixLength( f_D[ i - 1 ] , f_D[ i ] )
                          : 0;
  std::size_t last = kNone;
  while( f_N[ st.back() ].depth > l ) {
   last = st.back();
   st.pop_back();
   }
  if( f_N[ st.back() ].depth < l ) {        // a node of depth l on the edge
   const std::size_t top = st.back() , u = f_N.size();
   f_N.push_back( Node{ l , f_N[ last ].lo , 0 , false , {} } );
   f_N[ u ].child.emplace_back( at( f_D[ f_N[ last ].lo ] , l ) , last );
   f_N[ top ].child.back().second = u;      // last was its last child
   st.push_back( u );
   }
  const std::size_t x = st.back() , y = f_N.size();
  f_N.push_back( Node{ f_D[ i ].size() + 1 , i , i + 1 , true , {} } );
  f_N[ x ].child.emplace_back( at( f_D[ i ] , f_N[ x ].depth ) , y );
  st.push_back( y );
  }
 std::vector< std::size_t > pre , stack( 1 , 0 );   // the ranges, bottom up
 while( ! stack.empty() ) {
  const std::size_t x = stack.back();
  stack.pop_back();
  pre.push_back( x );
  for( const auto & c : f_N[ x ].child ) stack.push_back( c.second );
  }
 for( std::size_t k = pre.size() ; k-- > 0 ; ) {
  Node & x = f_N[ pre[ k ] ];
  if( ! x.child.empty() ) {
   x.lo = f_N[ x.child.front().second ].lo;
   x.hi = f_N[ x.child.back().second ].hi;
   }
  }
 ADS_INVARIANT( wellFormed() );
 }

inline std::size_t PatriciaTrie::childOf( std::size_t x , int c ) const
{
 const auto & C = f_N[ x ].child;
 const auto it = std::lower_bound( C.begin() , C.end() , c ,
                                   []( const auto & e , int v ) {
                                    return( e.first < v ); } );
 return( ( it != C.end() ) && ( it->first == c ) ? it->second : kNone );
 }

inline void PatriciaTrie::descend( std::string_view P , int term ,
                                  std::vector< std::size_t > & path ) const
{
 std::size_t x = 0;                         // by the branching characters
 path.assign( 1 , 0 );
 while( ! f_N[ x ].leaf && ( f_N[ x ].depth <= P.size() ) ) {
  const std::size_t d = f_N[ x ].depth ,
                    y = childOf( x , d < P.size() ? at( P , d ) : term );
  if( y == kNone ) break;
  x = y;
  path.push_back( x );
  }
 }

inline std::size_t PatriciaTrie::blind( std::string_view P , int term ,
                                        std::vector< std::size_t > & path )
 const
{
 const std::size_t L = P.size() + 1;       // P followed by term
 auto pc = [ & ]( std::size_t i ) { return( i < P.size() ? at( P , i )
                                                          : term ); };
 descend( P , term , path );                // first stage
 const std::size_t l = f_N[ path.back() ].lo;   // a leaf below the path
 // second stage: the one string read
 const std::string & s = f_D[ l ];
 const std::size_t S = s.size() + 1;
 std::size_t e = 0;
 while( ( e < L ) && ( e < S ) && ( pc( e ) == at( s , e ) ) ) ++e;
 if( e == L )                               // P is s
  return( l );
 // third stage: the edge ( u , v ) of the path where e falls
 std::size_t j = path.size() - 1;
 while( f_N[ path[ j ] ].depth > e ) --j;
 const Node & u = f_N[ path[ j ] ];
 const int cp = pc( e );
 if( u.depth == e ) {                       // among the children of u
  const auto it = std::upper_bound( u.child.begin() , u.child.end() , cp ,
                                    []( int v , const auto & c ) {
                                     return( v < c.first ); } );
  return( it == u.child.end() ? u.hi : f_N[ it->second ].lo );
  }
 const Node & v = f_N[ path[ j + 1 ] ];     // within the edge into v
 return( cp > at( s , e ) ? v.hi : v.lo );
 }

inline std::size_t PatriciaTrie::lexicographicPosition(
                                                  std::string_view P ) const
{
 std::vector< std::size_t > path;
 const std::size_t r = f_D.empty() ? 0 : blind( P , -1 , path );
 ADS_POST( ( ( r == 0 ) || ( f_D[ r - 1 ] < P ) ) &&
           ( ( r == f_D.size() ) || ( P <= f_D[ r ] ) ) );
 return( r );
 }

inline std::size_t PatriciaTrie::prefixEnd( std::string_view P ) const
{
 std::vector< std::size_t > path;
 const std::size_t r = f_D.empty() ? 0 : blind( P , 256 , path );
 ADS_POST( ( ( r == 0 ) || detail::precedes( f_D[ r - 1 ] , P , true ) ) &&
           ( ( r == f_D.size() ) ||
             ! detail::precedes( f_D[ r ] , P , true ) ) );
 return( r );
 }

inline std::pair< std::size_t , std::size_t > PatriciaTrie::prefixRange(
                                                  std::string_view P ) const
{
 return( std::pair( lexicographicPosition( P ) , prefixEnd( P ) ) );
 }

inline std::size_t PatriciaTrie::blindLeaf( std::string_view P ) const
{
 ADS_PRE( ! f_D.empty() );
 std::vector< std::size_t > path;
 descend( P , -1 , path );
 return( f_N[ path.back() ].lo );
 }

inline bool PatriciaTrie::contains( std::string_view P ) const
{
 const std::size_t r = lexicographicPosition( P );
 return( ( r < f_D.size() ) && ( f_D[ r ] == P ) );
 }

inline std::size_t PatriciaTrie::size( void ) const
{
 return( f_D.size() );
 }

inline std::size_t PatriciaTrie::nodes( void ) const
{
 return( f_N.size() );
 }

inline const std::string & PatriciaTrie::string( std::size_t i ) const
{
 ADS_PRE( i < size() );
 return( f_D[ i ] );
 }

inline std::vector< std::pair< std::size_t , int > > PatriciaTrie::shape(
                                                                void ) const
{
 std::vector< std::pair< std::size_t , int > > S;
 std::vector< std::pair< std::size_t , int > > stack( 1 ,
                                                      std::pair( 0 , -1 ) );
 while( ! stack.empty() ) {                 // preorder, by characters
  const auto [ x , c ] = stack.back();
  stack.pop_back();
  S.emplace_back( f_N[ x ].depth , c );
  const auto & C = f_N[ x ].child;
  for( auto it = C.rbegin() ; it != C.rend() ; ++it )
   stack.emplace_back( it->second , it->first );
  }
 return( S );
 }

inline bool PatriciaTrie::wellFormed( void ) const
{
 for( std::size_t x = 0 ; x < f_N.size() ; ++x ) {
  const Node & u = f_N[ x ];
  if( u.leaf ) {
   if( ! u.child.empty() || ( u.hi != u.lo + 1 ) ||
       ( u.depth != f_D[ u.lo ].size() + 1 ) )
    return( false );
   continue;
   }
  if( ( x != 0 ) && ( u.child.size() < 2 ) )
   return( false );
  if( ( u.hi > u.lo ) &&                    // its strings share its prefix
      ( commonPrefixLength( f_D[ u.lo ] , f_D[ u.hi - 1 ] ) < u.depth ) )
   return( false );
  std::size_t next = u.lo;
  for( std::size_t k = 0 ; k < u.child.size() ; ++k ) {
   const auto [ c , y ] = u.child[ k ];
   if( ( ( k > 0 ) && ( u.child[ k - 1 ].first >= c ) ) ||
       ( f_N[ y ].lo != next ) || ( f_N[ y ].depth <= u.depth ) ||
       ( at( f_D[ f_N[ y ].lo ] , u.depth ) != c ) )
    return( false );
   next = f_N[ y ].hi;
   }
  if( next != u.hi )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File PatriciaTrie.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
