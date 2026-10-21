/*--------------------------------------------------------------------------*/
/*---------------------- File HuffmanWaveletTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of HuffmanWaveletTree.h, with their contracts.
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

template< class RS >
HuffmanWaveletTree< RS >::HuffmanWaveletTree(
                          std::span< const std::uint32_t > x ,
                          bool withSelect ) : f_n( x.size() )
{
 // a depth of the code of Huffman d needs a frequency of Fibonacci( d + 2 )
 ADS_PRE( std::uint64_t( f_n ) < ( std::uint64_t( 1 ) << 40 ) );
 if( f_n == 0 ) return;
 const std::uint32_t top = *std::max_element( x.begin() , x.end() );
 std::vector< std::uint64_t > f( std::size_t( top ) + 1 , 0 );
 for( const auto c : x ) ++f[ c ];
 f_len.assign( f.size() , 0 );
 f_code.assign( f.size() , 0 );
 // the lengths of the code of Huffman by two queues on the sorted
 // weights: the leaves and the internal nodes, these made in increasing
 // weight, so that the two lightest are at the fronts
 std::vector< std::pair< std::uint64_t , std::uint32_t > > leaf;
 for( std::uint32_t c = 0 ; c <= top ; ++c )
  if( f[ c ] > 0 ) leaf.emplace_back( f[ c ] , c );
 std::sort( leaf.begin() , leaf.end() );
 const std::size_t m = leaf.size();
 if( m == 1 ) f_len[ leaf[ 0 ].second ] = 1;
 else {
  std::vector< std::uint64_t > w( 2 * m - 1 );
  std::vector< std::size_t > up( 2 * m - 1 , 0 );
  for( std::size_t i = 0 ; i < m ; ++i ) w[ i ] = leaf[ i ].first;
  std::size_t i = 0 , j = m;
  auto lightest = [ & ]( std::size_t next ) {
   if( ( i < m ) && ( ( j == next ) || ( w[ i ] <= w[ j ] ) ) )
    return( i++ );
   return( j++ );
   };
  for( std::size_t next = m ; next < 2 * m - 1 ; ++next ) {
   const std::size_t a = lightest( next ) , b = lightest( next );
   w[ next ] = w[ a ] + w[ b ];
   up[ a ] = up[ b ] = next;
   }
  std::vector< std::uint8_t > depth( 2 * m - 1 , 0 );
  for( std::size_t k = 2 * m - 2 ; k-- > 0 ; )
   depth[ k ] = std::uint8_t( depth[ up[ k ] ] + 1 );
  for( std::size_t k = 0 ; k < m ; ++k )
   f_len[ leaf[ k ].second ] = depth[ k ];
  }
 // the canonical codes, by length and then by symbol
 std::vector< std::uint32_t > order;
 for( std::uint32_t c = 0 ; c <= top ; ++c )
  if( f_len[ c ] > 0 ) order.push_back( c );
 std::sort( order.begin() , order.end() ,
            [ & ]( std::uint32_t a , std::uint32_t b ) {
             return( f_len[ a ] != f_len[ b ] ? f_len[ a ] < f_len[ b ]
                                               : a < b ); } );
 std::uint64_t code = 0;
 int prev = f_len[ order[ 0 ] ];
 for( const auto c : order ) {
  code <<= ( f_len[ c ] - prev );
  prev = f_len[ c ];
  f_code[ c ] = code++;
  }
 // the trie of the codes, with the items through each node
 f_node.emplace_back();
 std::vector< std::uint64_t > items( 1 , 0 );
 for( const auto c : order ) {
  std::size_t v = 0;
  for( int d = 0 ; d < f_len[ c ] ; ++d ) {
   items[ v ] += f[ c ];
   const bool b = bit( c , d );
   if( d + 1 == f_len[ c ] ) f_node[ v ].child[ b ] = -1 - std::int32_t( c );
   else {
    if( f_node[ v ].child[ b ] == 0 ) {
     f_node[ v ].child[ b ] = std::int32_t( f_node.size() );
     f_node.emplace_back();
     items.push_back( 0 );
     }
    v = std::size_t( f_node[ v ].child[ b ] );
    }
   }
  }
 // the places of the nodes in B in the order of a visit by levels, and
 // the bits of each item written along its code
 std::vector< std::size_t > queue( 1 , 0 );
 std::uint64_t total = 0;
 for( std::size_t q = 0 ; q < queue.size() ; ++q ) {
  Node & v = f_node[ queue[ q ] ];
  v.off = total;
  total += items[ queue[ q ] ];
  for( const auto ch : v.child )
   if( ch > 0 ) queue.push_back( std::size_t( ch ) );
  }
 BitVector B( total );
 std::vector< std::uint64_t > at( f_node.size() );
 for( std::size_t v = 0 ; v < f_node.size() ; ++v ) at[ v ] = f_node[ v ].off;
 for( const auto c : x ) {
  std::size_t v = 0;
  for( int d = 0 ; d < f_len[ c ] ; ++d ) {
   const bool b = bit( c , d );
   if( b ) B.set( at[ v ] );
   ++at[ v ];
   v = std::size_t( std::max( f_node[ v ].child[ b ] , 0 ) );
   }
  }
 f_B = directory( std::move( B ) , withSelect );
 for( auto & v : f_node ) v.ones = f_B.rank1( v.off );
 ADS_AUDIT( std::equal( x.begin() , x.end() ,
                        SubscriptIterator< HuffmanWaveletTree ,
                                           std::uint32_t >( this , 0 ) ) );
 }

template< class RS >
std::uint32_t HuffmanWaveletTree< RS >::access( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 return( accessRank( i ).first );
 }

template< class RS >
std::pair< std::uint32_t , std::size_t >
 HuffmanWaveletTree< RS >::accessRank( std::size_t i ) const
{
 ADS_PRE( i < f_n );
 for( const Node * v = f_node.data() ; ; ) {
  const std::size_t p = std::size_t( v->off ) + i;
  const bool b = f_B[ p ];
  const std::size_t r = f_B.rank1( p ) - std::size_t( v->ones );
  i = b ? r : i - r;
  const std::int32_t ch = v->child[ b ];
  if( ch < 0 ) return( std::pair( std::uint32_t( -1 - ch ) , i ) );
  v = f_node.data() + ch;
  }
 }

template< class RS >
std::size_t HuffmanWaveletTree< RS >::rank( std::uint32_t c ,
                                            std::size_t i ) const
{
 ADS_PRE( i <= f_n );
 const int L = codeLength( c );
 const Node * v = f_node.data();
 for( int d = 0 ; d < L ; ++d ) {
  const bool b = bit( c , d );
  const std::size_t r = f_B.rank1( std::size_t( v->off ) + i ) -
                        std::size_t( v->ones );
  i = b ? r : i - r;
  v = f_node.data() + std::max( v->child[ b ] , 0 );
  }
 return( L == 0 ? 0 : i );
 }

template< class RS >
std::pair< std::size_t , std::size_t > HuffmanWaveletTree< RS >::rank(
                       std::uint32_t c , std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j <= f_n ) );
 const int L = codeLength( c );
 if( L == 0 ) return( std::pair< std::size_t , std::size_t >() );
 const Node * v = f_node.data();
 for( int d = 0 ; d < L ; ++d ) {
  const bool b = bit( c , d );
  const std::size_t o = std::size_t( v->off ) , ones = std::size_t( v->ones );
  const std::size_t ri = f_B.rank1( o + i ) - ones ,
                    rj = f_B.rank1( o + j ) - ones;
  i = b ? ri : i - ri;
  j = b ? rj : j - rj;
  v = f_node.data() + std::max( v->child[ b ] , 0 );
  }
 return( std::pair( i , j ) );
 }

template< class RS >
std::size_t HuffmanWaveletTree< RS >::select( std::uint32_t c ,
                                              std::size_t k ) const
{
 ADS_PRE( ( k >= 1 ) && ( k <= rank( c , f_n ) ) );
 const int L = codeLength( c );
 const Node * path[ 64 ];                   // the nodes down to the leaf
 const Node * v = f_node.data();
 for( int d = 0 ; d < L ; ++d ) {
  path[ d ] = v;
  v = f_node.data() + std::max( v->child[ bit( c , d ) ] , 0 );
  }
 for( int d = L - 1 ; d >= 0 ; --d ) {      // k-th of the node: its place
  const Node * u = path[ d ];
  const std::size_t o = std::size_t( u->off ) , ones = std::size_t( u->ones );
  const std::size_t p = bit( c , d ) ? f_B.select1( ones + k )
                                     : f_B.select0( o - ones + k );
  k = p - o + 1;
  }
 ADS_POST( access( k - 1 ) == c );
 return( k - 1 );
 }

template< class RS >
std::size_t HuffmanWaveletTree< RS >::sizeInBits( void ) const
{
 return( f_B.size() + f_B.directoryBits() + 8 * sizeof( Node ) *
         f_node.size() + 72 * f_len.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File HuffmanWaveletTree.tpp --------------------*/
/*--------------------------------------------------------------------------*/
