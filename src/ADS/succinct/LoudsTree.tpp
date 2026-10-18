/*--------------------------------------------------------------------------*/
/*--------------------------- File LoudsTree.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of LoudsTree.h, with their contracts. Node v here is node
 * v + 1 of the notes, and a place p here is the place p + 1 there.
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

inline LoudsTree::LoudsTree( void ) : f_b( BitVector( 1 , true ) )
{
 }

inline LoudsTree::LoudsTree(
                       const std::vector< std::vector< std::size_t > > & c )
{
 const std::size_t n = c.size();
 BitVector B;
 B.reserve( 2 * n + 1 );
 if( n > 0 ) B.push_back( false );          // the dummy root, of degree 1
 B.push_back( true );
 std::vector< std::size_t > Q;              // the visit by levels
 Q.reserve( n );
 if( n > 0 ) Q.push_back( 0 );
 for( std::size_t h = 0 ; h < Q.size() ; ++h ) {
  for( const std::size_t u : c[ Q[ h ] ] ) {
   ADS_PRE( ( u > 0 ) && ( u < n ) && ( Q.size() < n ) );
   B.push_back( false );
   Q.push_back( u );
   }
  B.push_back( true );
  }
 ADS_PRE( Q.size() == n );                  // each node reached once
 f_b = RankSelect( std::move( B ) );
 }

inline LoudsTree::LoudsTree( BitVector b ) : f_b( std::move( b ) )
{
 ADS_AUDIT( valid() );
 }

inline std::size_t LoudsTree::size( void ) const
{
 return( f_b.zeros() );
 }

inline std::size_t LoudsTree::root( void ) const
{
 return( size() ? 0 : nil );
 }

inline std::size_t LoudsTree::degree( std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( f_b.select1( v + 2 ) - f_b.select1( v + 1 ) - 1 );
 }

inline std::size_t LoudsTree::parent( std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( v == 0 ? nil : f_b.select0( v + 1 ) - v - 1 );
 }

inline std::size_t LoudsTree::leftmostChild( std::size_t v ) const
{
 ADS_PRE( v < size() );
 const std::size_t p = f_b.select1( v + 1 );   // the 1 before the children
 return( f_b[ p + 1 ] ? nil : p - v );
 }

inline std::size_t LoudsTree::rightSibling( std::size_t v ) const
{
 ADS_PRE( v < size() );
 return( f_b[ f_b.select0( v + 1 ) + 1 ] ? nil : v + 1 );
 }

inline std::size_t LoudsTree::child( std::size_t v , std::size_t i ) const
{
 ADS_PRE( i < degree( v ) );
 const std::size_t u = leftmostChild( v ) + i;
 ADS_POST( parent( u ) == v );
 return( u );
 }

inline bool LoudsTree::isLeaf( std::size_t v ) const
{
 return( leftmostChild( v ) == nil );
 }

inline const BitVector & LoudsTree::bits( void ) const
{
 return( f_b.bits() );
 }

inline bool LoudsTree::valid( void ) const
{
 const std::size_t n = f_b.zeros();
 if( ( f_b.size() != 2 * n + 1 ) || ! f_b[ f_b.size() - 1 ] ) return( false );
 for( std::size_t k = 1 ; k <= n ; ++k ) {  // the parent of the k-th node
  const std::size_t p = f_b.select0( k ) + 1 - k;   // in the notes
  if( k == 1 ? p != 0 : ( p == 0 ) || ( p >= k ) ) return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File LoudsTree.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
