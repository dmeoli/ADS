/*--------------------------------------------------------------------------*/
/*-------------------------- File LoserTree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of LoserTree and kWayMerge() [see LoserTree.h], with
 * their contracts.
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

template< class T , class C >
LoserTree< T , C >::LoserTree( std::vector< std::optional< T > > keys ,
                               C less )
 : f_key( std::move( keys ) ) , f_less( less )
{
 const std::size_t k = f_key.size();
 ADS_PRE( k >= 1 );
 f_node.assign( k , 0 );
 std::vector< std::size_t > win( k , 0 );  // the winner below each node
 auto player = [ & ]( std::size_t v ) {
  return( v >= k ? v - k : win[ v ] ); };
 for( std::size_t v = k - 1 ; v >= 1 ; --v ) {
  const std::size_t a = player( 2 * v ) , b = player( 2 * v + 1 );
  if( beats( a , b ) ) {
   win[ v ] = a;
   f_node[ v ] = b;
   }
  else {
   win[ v ] = b;
   f_node[ v ] = a;
   }
  }
 f_node[ 0 ] = k > 1 ? win[ 1 ] : 0;
 ADS_INVARIANT( detail::contract( [ & ]( void ) {
  return( winnerIsBest() ); } ) );
 }

template< class T , class C >
bool LoserTree< T , C >::beats( std::size_t i , std::size_t j ) const
{
 if( ! f_key[ i ] ) return( false );
 if( ! f_key[ j ] ) return( true );
 if( i < j )                                // one call of less per match
  return( ! f_less( *f_key[ j ] , *f_key[ i ] ) );
 return( f_less( *f_key[ i ] , *f_key[ j ] ) );
 }

template< class T , class C >
bool LoserTree< T , C >::winnerIsBest( void ) const
{
 for( std::size_t i = 0 ; i < f_key.size() ; ++i )
  if( beats( i , f_node[ 0 ] ) ) return( false );
 return( true );
 }

template< class T , class C >
std::size_t LoserTree< T , C >::players( void ) const
{
 return( f_key.size() );
 }

template< class T , class C >
bool LoserTree< T , C >::isEmpty( void ) const
{
 return( ! f_key[ f_node[ 0 ] ] );
 }

template< class T , class C >
std::size_t LoserTree< T , C >::winner( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( f_node[ 0 ] );
 }

template< class T , class C >
const T & LoserTree< T , C >::top( void ) const
{
 ADS_PRE( ! isEmpty() );
 return( *f_key[ f_node[ 0 ] ] );
 }

template< class T , class C >
void LoserTree< T , C >::replace( std::optional< T > key )
{
 ADS_PRE( ! isEmpty() );
 const std::size_t k = f_key.size();
 std::size_t w = f_node[ 0 ];
 f_key[ w ] = std::move( key );
 for( std::size_t v = ( w + k ) / 2 ; v >= 1 ; v /= 2 )
  if( beats( f_node[ v ] , w ) )            // the loser there wins now
   std::swap( w , f_node[ v ] );
 f_node[ 0 ] = w;
 ADS_INVARIANT( detail::contract( [ & ]( void ) {
  return( winnerIsBest() ); } ) );
 }

template< class T , class C >
std::vector< T > kWayMerge( const std::vector< std::vector< T > > & R ,
                            C less )
{
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  for( const auto & r : R )
   if( ! std::is_sorted( r.begin() , r.end() , less ) ) return( false );
  return( true ); } ) );
 std::vector< T > out;
 if( R.empty() ) return( out );
 std::vector< std::optional< T > > first( R.size() );
 std::vector< std::size_t > next( R.size() , 1 );
 std::size_t n = 0;
 for( std::size_t i = 0 ; i < R.size() ; ++i ) {
  if( ! R[ i ].empty() ) first[ i ] = R[ i ][ 0 ];
  n += R[ i ].size();
  }
 out.reserve( n );
 LoserTree< T , C > L( std::move( first ) , less );
 while( ! L.isEmpty() ) {
  const std::size_t w = L.winner();
  out.push_back( L.top() );
  L.replace( next[ w ] < R[ w ].size() ?
             std::optional< T >( R[ w ][ next[ w ]++ ] ) : std::nullopt );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  std::vector< T > all;
  for( const auto & r : R ) all.insert( all.end() , r.begin() , r.end() );
  return( detail::sortedPermutation( out , std::move( all ) , less ) );
  } ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File LoserTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
