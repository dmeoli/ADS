/*--------------------------------------------------------------------------*/
/*-------------------------- File JacobsonTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of JacobsonTree.h, with their contracts.
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

inline JacobsonTree::JacobsonTree( void ) : f_b( BitVector( 1 , false ) )
{
 }

inline JacobsonTree::JacobsonTree(
         const std::vector< std::pair< std::size_t , std::size_t > > & c )
{
 const std::size_t n = c.size();
 BitVector B;
 B.reserve( 2 * n + 1 );
 B.push_back( n > 0 );                      // the root
 std::vector< std::size_t > Q;              // the visit by levels
 Q.reserve( n );
 std::vector< bool > in( n , false );       // the nodes reached
 if( n > 0 ) {
  Q.push_back( 0 );
  in[ 0 ] = true;
  }
 for( std::size_t h = 0 ; h < Q.size() ; ++h )
  for( const std::size_t u : { c[ Q[ h ] ].first , c[ Q[ h ] ].second } ) {
   ADS_PRE( ( u == nil ) || ( ( u > 0 ) && ( u < n ) && ! in[ u ] ) );
   B.push_back( u != nil );
   if( u != nil ) {
    Q.push_back( u );
    in[ u ] = true;
    }
   }
 ADS_PRE( Q.size() == n );                  // each node reached once
 f_b = RankSelect( std::move( B ) );
 }

inline JacobsonTree::JacobsonTree( BitVector b ) : f_b( std::move( b ) )
{
 ADS_AUDIT( valid() );
 }

inline std::size_t JacobsonTree::size( void ) const
{
 return( f_b.ones() );
 }

inline std::size_t JacobsonTree::root( void ) const
{
 return( size() ? 0 : nil );
 }

inline std::size_t JacobsonTree::nodeAt( std::size_t q ) const
{
 return( f_b[ q ] ? f_b.rank1( q ) : nil );
 }

inline std::size_t JacobsonTree::left( std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( nodeAt( 2 * v + 1 ) );            // 2 x in the notes
 }

inline std::size_t JacobsonTree::right( std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( nodeAt( 2 * v + 2 ) );            // 2 x + 1
 }

inline std::size_t JacobsonTree::parent( std::size_t v ) const
{
 ADS_PRE( v < size() );
 if( v == 0 ) return( nil );
 const std::size_t u = ( f_b.select1( v + 1 ) + 1 ) / 2 - 1;   // floor
 ADS_POST( ( left( u ) == v ) || ( right( u ) == v ) );
 return( u );
 }

inline bool JacobsonTree::isLeaf( std::size_t v ) const
{
 return( ( left( v ) == nil ) && ( right( v ) == nil ) );
 }

inline const BitVector & JacobsonTree::bits( void ) const
{
 return( f_b.bits() );
 }

inline bool JacobsonTree::valid( void ) const
{
 if( f_b.size() != 2 * f_b.ones() + 1 ) return( false );
 for( std::size_t k = 0 ; k < f_b.ones() ; ++k )   // before its children
  if( f_b.select1( k + 1 ) > 2 * k ) return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File JacobsonTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
