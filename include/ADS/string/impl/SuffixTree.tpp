/*--------------------------------------------------------------------------*/
/*-------------------------- File SuffixTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SuffixTree.h, with their contracts.
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

inline SuffixTree::SuffixTree( std::string_view T , Build b )
{
 construct( toSymbols( T ) , 256 , b );
 }

inline SuffixTree::SuffixTree( const Symbols & T , std::size_t K , Build b )
{
 ADS_PRE( detail::symbolsBelow( T , K ) );
 construct( T , K , b );
 }

inline void SuffixTree::construct( const Symbols & T , std::size_t K ,
                                   Build b )
{
 f_T.resize( T.size() + 1 );
 for( std::size_t i = 0 ; i < T.size() ; ++i ) f_T[ i ] = T[ i ] + 1;
 f_T.back() = 0;
 f_N.reserve( 2 * f_T.size() );
 newNode( 0 , 0 , npos );
 switch( b ) {
  case Build::naive: buildNaive(); break;
  case Build::mcCreight: buildMcCreight(); break;
  case Build::suffixArray: buildFromSuffixArray( T , K ); break;
  }
 finish();
 ADS_INVARIANT( valid() );
 }

inline std::size_t SuffixTree::newNode( std::size_t pos , std::size_t depth ,
                                        std::size_t parent )
{
 Node x;
 x.pos = pos;
 x.depth = depth;
 x.parent = parent;
 f_N.push_back( std::move( x ) );
 return( f_N.size() - 1 );
 }

inline std::size_t SuffixTree::attachLeaf( std::size_t u , std::size_t i )
{
 const std::size_t x = newNode( i , f_T.size() - i , u );
 f_N[ x ].suffix = i;
 f_N[ u ].child[ f_T[ i + f_N[ u ].depth ] ] = x;
 return( x );
 }

inline std::size_t SuffixTree::split( std::size_t u , std::size_t c ,
                                      std::size_t d )
{
 const std::size_t p = f_N[ c ].pos;
 const std::size_t w = newNode( p , d , u );
 f_N[ u ].child[ f_T[ p + f_N[ u ].depth ] ] = w;
 f_N[ w ].child[ f_T[ p + d ] ] = c;
 f_N[ c ].parent = w;
 return( w );
 }

inline void SuffixTree::buildNaive( void )
{
 const std::size_t N = f_T.size();
 for( std::size_t i = 0 ; i < N ; ++i ) {
  std::size_t u = 0;
  for( ; ; ) {
   const auto it = f_N[ u ].child.find( f_T[ i + f_N[ u ].depth ] );
   if( it == f_N[ u ].child.end() ) break;
   const std::size_t c = it->second;
   std::size_t k = f_N[ u ].depth + 1;
   while( ( k < f_N[ c ].depth ) &&
          ( f_T[ f_N[ c ].pos + k ] == f_T[ i + k ] ) )
    ++k;
   if( k < f_N[ c ].depth ) {
    u = split( u , c , k );
    break;
    }
   u = c;
   }
  attachLeaf( u , i );
  }
 }

inline void SuffixTree::buildMcCreight( void )
{
 const std::size_t N = f_T.size();
 std::vector< std::size_t > link( 1 , 0 );      // the suffix links
 auto setLink = [ & ]( std::size_t u , std::size_t v ) {
  if( link.size() <= u ) link.resize( f_N.size() , npos );
  link[ u ] = v;
  };
 auto getLink = [ & ]( std::size_t u ) {
  return( u < link.size() ? link[ u ] : npos );
  };
 attachLeaf( 0 , 0 );
 std::size_t h = 0;                  // the locus of head_{i-1}
 for( std::size_t i = 1 ; i < N ; ++i ) {
  std::size_t w = getLink( h );
  if( w == npos ) {
   // rescan head_{i-1} without its first symbol from the link of the
   // parent, comparing only the first symbol of each edge
   const std::size_t f = f_N[ h ].parent , D = f_N[ h ].depth - 1;
   std::size_t u = f == 0 ? 0 : getLink( f );
   while( f_N[ u ].depth < D ) {
    const std::size_t c = f_N[ u ].child.at( f_T[ i + f_N[ u ].depth ] );
    if( f_N[ c ].depth <= D ) u = c;
    else {
     u = split( u , c , D );
     break;
     }
    }
   w = u;
   setLink( h , w );
   }
  // scan the rest of the suffix i from w, comparing every symbol
  std::size_t u = w;
  for( ; ; ) {
   const auto it = f_N[ u ].child.find( f_T[ i + f_N[ u ].depth ] );
   if( it == f_N[ u ].child.end() ) break;
   const std::size_t c = it->second;
   std::size_t k = f_N[ u ].depth + 1;
   while( ( k < f_N[ c ].depth ) &&
          ( f_T[ f_N[ c ].pos + k ] == f_T[ i + k ] ) )
    ++k;
   if( k < f_N[ c ].depth ) {
    u = split( u , c , k );
    break;
    }
   u = c;
   }
  attachLeaf( u , i );
  h = u;
  }
 }

inline void SuffixTree::buildFromSuffixArray( const Symbols & T ,
                                              std::size_t K )
{
 const std::size_t n = T.size();
 const std::vector< std::size_t > SA = suffixArraySais( T , K ) ,
                                  LCP = lcpArray( T , SA );
 std::size_t last = attachLeaf( 0 , n );         // the terminator first
 for( std::size_t r = 0 ; r < n ; ++r ) {
  const std::size_t l = r == 0 ? 0 : LCP[ r - 1 ];
  std::size_t x = last;
  while( f_N[ f_N[ x ].parent ].depth > l ) x = f_N[ x ].parent;
  const std::size_t p = f_N[ x ].parent;
  const std::size_t u = f_N[ p ].depth == l ? p : split( p , x , l );
  last = attachLeaf( u , SA[ r ] );
  }
 }

inline std::vector< std::size_t > SuffixTree::preorder( void ) const
{
 std::vector< std::size_t > out , S( 1 , 0 );
 out.reserve( f_N.size() );
 while( ! S.empty() ) {
  const std::size_t u = S.back();
  S.pop_back();
  out.push_back( u );
  for( auto it = f_N[ u ].child.rbegin() ; it != f_N[ u ].child.rend() ;
       ++it )
   S.push_back( it->second );
  }
 return( out );
 }

inline void SuffixTree::finish( void )
{
 const std::size_t N = f_T.size() , m = f_N.size();
 f_leaf.assign( N , npos );
 const std::vector< std::size_t > pre = preorder();
 for( std::size_t k = m ; k-- > 0 ; ) {        // children before parents
  Node & x = f_N[ pre[ k ] ];
  if( x.suffix != npos ) {
   x.leaves = 1;
   f_leaf[ x.suffix ] = pre[ k ];
   }
  else {
   x.leaves = 0;
   for( const auto & e : x.child ) x.leaves += f_N[ e.second ].leaves;
   }
  }

 // the Euler tour, a node written on arrival and after each child
 std::vector< long > D;
 f_E.clear();
 f_E.reserve( 2 * m - 1 );
 D.reserve( 2 * m - 1 );
 f_first.assign( m , 0 );
 using It = std::map< std::size_t , std::size_t >::const_iterator;
 std::vector< std::pair< std::size_t , It > > S;
 S.emplace_back( 0 , f_N[ 0 ].child.begin() );
 f_E.push_back( 0 );
 D.push_back( 0 );
 while( ! S.empty() ) {
  auto & [ u , it ] = S.back();
  if( it == f_N[ u ].child.end() ) {
   S.pop_back();
   if( ! S.empty() ) {
    f_E.push_back( S.back().first );
    D.push_back( long( S.size() ) - 1 );
    }
   continue;
   }
  const std::size_t c = ( it++ )->second;
  f_first[ c ] = f_E.size();
  f_E.push_back( c );
  D.push_back( long( S.size() ) );
  S.emplace_back( c , f_N[ c ].child.begin() );
  }
 f_tour = PlusMinusOneRangeMinimum( std::move( D ) );
 }

inline bool SuffixTree::valid( void ) const
{
 const std::size_t N = f_T.size();
 std::size_t leaves = 0;
 for( std::size_t u = 0 ; u < f_N.size() ; ++u ) {
  const Node & x = f_N[ u ];
  if( x.suffix != npos ) {
   ++leaves;
   if( ! x.child.empty() || ( x.depth != N - x.suffix ) ||
       ( x.pos != x.suffix ) )
    return( false );
   }
  else if( ( u != 0 ) && ( x.child.size() < 2 ) ) return( false );
  for( const auto & [ key , c ] : x.child ) {
   const Node & y = f_N[ c ];
   if( ( y.parent != u ) || ( y.depth <= x.depth ) ||
       ( y.pos + y.depth > N ) || ( f_T[ y.pos + x.depth ] != key ) )
    return( false );
   }
  }
 return( ( leaves == N ) && ( f_N[ 0 ].leaves == N ) );
 }

inline std::size_t SuffixTree::textSize( void ) const
{
 return( f_T.empty() ? 0 : f_T.size() - 1 );
 }

inline const SuffixTree::Node & SuffixTree::node( std::size_t u ) const
{
 ADS_PRE( u < f_N.size() );
 return( f_N[ u ] );
 }

inline std::size_t SuffixTree::leaf( std::size_t i ) const
{
 ADS_PRE( i < f_leaf.size() );
 return( f_leaf[ i ] );
 }

inline std::size_t SuffixTree::symbol( std::size_t i ) const
{
 ADS_PRE( i < f_T.size() );
 return( f_T[ i ] );
 }

inline std::size_t SuffixTree::locus( const Symbols & P ) const
{
 const std::size_t p = P.size();
 std::size_t u = 0 , k = 0;
 while( k < p ) {
  const auto it = f_N[ u ].child.find( P[ k ] + 1 );
  if( it == f_N[ u ].child.end() ) return( npos );
  const std::size_t c = it->second;
  for( ++k ; ( k < p ) && ( k < f_N[ c ].depth ) ; ++k )
   if( f_T[ f_N[ c ].pos + k ] != P[ k ] + 1 ) return( npos );
  u = c;
  }
 return( u );
 }

inline std::size_t SuffixTree::locus( std::string_view P ) const
{
 return( locus( toSymbols( P ) ) );
 }

inline std::size_t SuffixTree::count( std::string_view P ) const
{
 const std::size_t u = locus( P );
 if( u == npos ) return( 0 );
 return( f_N[ u ].leaves - ( u == 0 ? 1 : 0 ) );    // not the terminator
 }

inline std::vector< std::size_t > SuffixTree::locate(
                                                std::string_view P ) const
{
 std::vector< std::size_t > out;
 const std::size_t u = locus( P );
 if( u == npos ) return( out );
 std::vector< std::size_t > S( 1 , u );
 while( ! S.empty() ) {
  const std::size_t v = S.back();
  S.pop_back();
  if( f_N[ v ].suffix != npos ) {
   if( f_N[ v ].suffix < textSize() ) out.push_back( f_N[ v ].suffix );
   continue;
   }
  for( auto it = f_N[ v ].child.rbegin() ; it != f_N[ v ].child.rend() ;
       ++it )
   S.push_back( it->second );
  }
 return( out );
 }

inline std::size_t SuffixTree::lca( std::size_t u , std::size_t v ) const
{
 ADS_PRE( ( u < f_N.size() ) && ( v < f_N.size() ) );
 std::size_t a = f_first[ u ] , b = f_first[ v ];
 if( a > b ) std::swap( a , b );
 return( f_E[ f_tour.query( a , b ) ] );
 }

inline std::size_t SuffixTree::lce( std::size_t i , std::size_t j ) const
{
 const std::size_t n = textSize();
 ADS_PRE( ( i <= n ) && ( j <= n ) );
 if( ( i == n ) || ( j == n ) ) return( 0 );
 if( i == j ) return( n - i );
 return( f_N[ lca( f_leaf[ i ] , f_leaf[ j ] ) ].depth );
 }

inline std::pair< std::vector< std::size_t > , std::vector< std::size_t > >
                                      SuffixTree::toSuffixArray( void ) const
{
 std::vector< std::size_t > SA , LCP;
 using It = std::map< std::size_t , std::size_t >::const_iterator;
 std::vector< std::pair< std::size_t , It > > S;
 S.emplace_back( 0 , f_N[ 0 ].child.begin() );
 while( ! S.empty() ) {
  auto & [ u , it ] = S.back();
  if( it == f_N[ u ].child.end() ) {
   S.pop_back();
   continue;
   }
  if( it != f_N[ u ].child.begin() ) LCP.push_back( f_N[ u ].depth );
  const std::size_t c = ( it++ )->second;
  if( f_N[ c ].suffix != npos ) SA.push_back( f_N[ c ].suffix );
  else
   S.emplace_back( c , f_N[ c ].child.begin() );
  }
 // the terminator is the first leaf, and its lcp with the next is 0
 SA.erase( SA.begin() );
 if( ! LCP.empty() ) LCP.erase( LCP.begin() );
 return( std::pair( std::move( SA ) , std::move( LCP ) ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SuffixTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
