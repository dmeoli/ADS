/*--------------------------------------------------------------------------*/
/*----------------------------- File BPTree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of BPTree.h, with their contracts.
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

inline BPTree::BPTree( const std::vector< std::vector< std::size_t > > & c )
{
 const std::size_t n = c.size();
 BitVector B;
 B.reserve( 2 * n );
 std::size_t seen = 0;
 std::vector< std::pair< std::size_t , std::size_t > > S;   // node , child
 if( n > 0 ) {
  S.emplace_back( 0 , 0 );
  B.push_back( true );
  seen = 1;
  }
 while( ! S.empty() ) {                     // the visit in depth
  auto & [ u , k ] = S.back();
  if( k < c[ u ].size() ) {
   const std::size_t w = c[ u ][ k++ ];
   ADS_PRE( ( w > 0 ) && ( w < n ) && ( seen < n ) );
   ++seen;
   B.push_back( true );
   S.emplace_back( w , 0 );
   }
  else {
   B.push_back( false );
   S.pop_back();
   }
  }
 ADS_PRE( seen == n );                      // each node reached once
 f_p = BalancedParentheses( std::move( B ) );
 }

inline BPTree::BPTree( BitVector b ) : f_p( std::move( b ) )
{
 ADS_PRE( ( f_p.size() == 0 ) ||
          ( f_p[ 0 ] && ( f_p.findClose( 0 ) == f_p.size() - 1 ) ) );
 }

inline std::size_t BPTree::size( void ) const
{
 return( f_p.size() / 2 );
 }

inline std::size_t BPTree::root( void ) const
{
 return( size() ? 0 : nil );
 }

inline std::size_t BPTree::open( std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( f_p.bits().select1( v + 1 ) );
 }

inline std::size_t BPTree::parent( std::size_t v ) const
{
 const std::size_t q = f_p.enclose( open( v ) );
 return( q == nil ? nil : f_p.bits().rank1( q ) );
 }

inline std::size_t BPTree::leftmostChild( std::size_t v ) const
{
 return( f_p[ open( v ) + 1 ] ? v + 1 : nil );
 }

inline std::size_t BPTree::rightSibling( std::size_t v ) const
{
 const std::size_t q = f_p.findClose( open( v ) ) + 1;
 return( ( q < f_p.size() ) && f_p[ q ] ? f_p.bits().rank1( q ) : nil );
 }

inline std::size_t BPTree::degree( std::size_t v ) const
{
 std::size_t d = 0;
 for( std::size_t u = leftmostChild( v ) ; u != nil ; u = rightSibling( u ) )
  ++d;
 return( d );
 }

inline std::size_t BPTree::subtreeSize( std::size_t v ) const
{
 const std::size_t p = open( v );
 return( ( f_p.findClose( p ) - p + 1 ) / 2 );
 }

inline std::size_t BPTree::depth( std::size_t v ) const
{
 return( std::size_t( f_p.excess( open( v ) ) - 1 ) );
 }

inline bool BPTree::isAncestor( std::size_t u , std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( ( u <= v ) && ( v < u + subtreeSize( u ) ) );
 }

inline bool BPTree::isLeaf( std::size_t v ) const
{
 return( leftmostChild( v ) == nil );
 }

inline const BalancedParentheses & BPTree::parentheses( void ) const
{
 return( f_p );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File BPTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
