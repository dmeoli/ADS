/*--------------------------------------------------------------------------*/
/*----------------------------- File K2Tree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of K2Tree.h, with their contracts.
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

inline K2Tree::K2Tree( std::size_t n , const std::vector< Edge > & e ,
                       std::size_t k )
 : f_n( n ) , f_k( k ) , f_side( k )
{
 ADS_PRE( k >= 2 );
 std::vector< Edge > E( e );
 for( const auto & [ u , v ] : E ) ADS_PRE( ( u < n ) && ( v < n ) );
 std::sort( E.begin() , E.end() );
 E.erase( std::unique( E.begin() , E.end() ) , E.end() );
 f_m = E.size();
 while( f_side < n ) f_side *= k;
 struct Node { std::size_t r , c , a , b; };   // a submatrix, its edges
 std::vector< Node > level;
 if( f_m > 0 ) level.push_back( Node{ 0 , 0 , 0 , f_m } );
 BitVector T;
 const std::size_t k2 = k * k;
 for( std::size_t s = f_side / k ; ! level.empty() ; s /= k ) {
  std::vector< Node > next;
  std::vector< Edge > F;                    // the edges, child by child
  F.reserve( E.size() );
  std::vector< std::size_t > count( k2 + 1 );
  for( const Node & x : level ) {
   auto child = [ & ]( const Edge & d ) {
    return( ( d.first - x.r ) / s * k + ( d.second - x.c ) / s );
    };
   std::fill( count.begin() , count.end() , 0 );
   for( std::size_t i = x.a ; i < x.b ; ++i ) ++count[ child( E[ i ] ) + 1 ];
   for( std::size_t j = 0 ; j < k2 ; ++j ) {
    ( s == 1 ? f_l : T ).push_back( count[ j + 1 ] > 0 );
    count[ j + 1 ] += count[ j ];          // where the child starts
    }
   const std::size_t at = F.size();
   F.resize( at + ( x.b - x.a ) );
   std::vector< std::size_t > put( count.begin() , count.end() - 1 );
   for( std::size_t i = x.a ; i < x.b ; ++i )
    F[ at + put[ child( E[ i ] ) ]++ ] = E[ i ];
   if( s > 1 )
    for( std::size_t j = 0 ; j < k2 ; ++j )
     if( count[ j + 1 ] > count[ j ] )
      next.push_back( Node{ x.r + j / k * s , x.c + j % k * s ,
                            at + count[ j ] , at + count[ j + 1 ] } );
   }
  E.swap( F );
  level.swap( next );
  }
 f_t = RankSelect( std::move( T ) , RankSelect::Selects::NONE );
 ADS_POST( f_l.count() == f_m );
 }

inline std::size_t K2Tree::order( void ) const
{
 return( f_n );
 }

inline std::size_t K2Tree::edges( void ) const
{
 return( f_m );
 }

inline bool K2Tree::mark( std::size_t p ) const
{
 return( p < f_t.size() ? f_t[ p ] : f_l[ p - f_t.size() ] );
 }

inline std::size_t K2Tree::children( std::size_t p ) const
{
 return( f_t.rank1( p + 1 ) * f_k * f_k );
 }

inline void K2Tree::walk( std::size_t s , std::size_t r , std::size_t o ,
                          std::size_t base , bool row ,
                          std::vector< std::size_t > & out ) const
{
 const std::size_t c = s / f_k;             // the side of a child
 for( std::size_t j = 0 ; j < f_k ; ++j ) {
  const std::size_t p = row ? base + r / c * f_k + j : base + j * f_k + r / c;
  if( mark( p ) ) {
   if( c == 1 ) out.push_back( o + j );
   else walk( c , r % c , o + j * c , children( p ) , row , out );
   }
  }
 }

inline std::vector< std::size_t > K2Tree::neighbors( std::size_t u ) const
{
 ADS_PRE( u < f_n );
 std::vector< std::size_t > out;
 if( f_m > 0 ) walk( f_side , u , 0 , 0 , true , out );
 return( out );
 }

inline std::vector< std::size_t > K2Tree::reverseNeighbors(
                                                        std::size_t v ) const
{
 ADS_PRE( v < f_n );
 std::vector< std::size_t > out;
 if( f_m > 0 ) walk( f_side , v , 0 , 0 , false , out );
 return( out );
 }

inline bool K2Tree::hasEdge( std::size_t u , std::size_t v ) const
{
 ADS_PRE( ( u < f_n ) && ( v < f_n ) );
 if( f_m == 0 ) return( false );
 for( std::size_t s = f_side , base = 0 ; ; ) {   // one path down
  const std::size_t c = s / f_k;
  const std::size_t p = base + u / c * f_k + v / c;
  if( ! mark( p ) ) return( false );
  if( c == 1 ) return( true );
  base = children( p );
  u %= c;
  v %= c;
  s = c;
  }
 }

inline const BitVector & K2Tree::levels( void ) const
{
 return( f_t.bits() );
 }

inline const BitVector & K2Tree::leaves( void ) const
{
 return( f_l );
 }

inline std::size_t K2Tree::sizeInBits( void ) const
{
 return( f_t.size() + f_t.directoryBits() + f_l.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File K2Tree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
