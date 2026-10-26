/*--------------------------------------------------------------------------*/
/*------------------------- File IntervalTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of IntervalTree [see IntervalTree.h], with their
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

template< std::totally_ordered T >
auto IntervalTree< T >::counted( std::vector< Interval > & I )
 -> std::vector< std::pair< Interval , std::size_t > >
{
 for( const auto & i : I )
  ADS_PRE( !( i.second < i.first ) );
 std::sort( I.begin() , I.end() );
 std::vector< std::pair< Interval , std::size_t > > B;
 for( const auto & i : I )
  if( ( ! B.empty() ) && ( B.back().first == i ) )
   ++B.back().second;
  else
   B.emplace_back( i , 1 );
 return( B );
 }

template< std::totally_ordered T >
IntervalTree< T >::IntervalTree( std::vector< Interval > I )
{
 const auto B = counted( I );
 f_map.multiInsert( B );
 f_n = I.size();
 }

template< std::totally_ordered T >
template< Executor E >
IntervalTree< T >::IntervalTree( E & ex , std::vector< Interval > I )
{
 const auto B = counted( I );
 f_map.multiInsert( ex , B );
 f_n = I.size();
 }

template< std::totally_ordered T >
void IntervalTree< T >::insert( const T & a , const T & b )
{
 ADS_PRE( !( b < a ) );
 const Interval i( a , b );
 f_map.insert( i , f_map.lookup( i ).value_or( 0 ) + 1 );
 ++f_n;
 }

template< std::totally_ordered T >
void IntervalTree< T >::remove( const T & a , const T & b )
{
 const Interval i( a , b );
 const auto c = f_map.lookup( i );
 if( ! c )
  return;
 if( *c == 1 ) f_map.remove( i );
 else f_map.insert( i , *c - 1 );
 --f_n;
 }

template< std::totally_ordered T >
bool IntervalTree< T >::stabbed( const T & x ) const
{
 typename MaxEnd::Value m;                 // the prefix with a <= x
 for( auto u = f_map.root() ; u ; )
  if( x < u->key.first )
   u = u->left;
  else {
   m = MaxEnd::combine( m , MaxEnd::combine(
                         u->left ? u->left->aug : std::nullopt ,
                         u->key.second ) );
   u = u->right;
   }
 return( m && !( *m < x ) );
 }

template< std::totally_ordered T >
template< class F >
void IntervalTree< T >::stab( const T & x , F && f ) const
{
 using N = typename Map::Node;
 auto visit = [ & ]( auto & self , const N * u ) -> void {
  if( ( ! u ) || ( *u->aug < x ) )       // no right end reaches x
   return;
  self( self , u->left );
  if( x < u->key.first )                 // and neither to the right
   return;
  if( !( u->key.second < x ) )
   for( std::size_t c = 0 ; c < u->value ; ++c )
    f( u->key.first , u->key.second );
  self( self , u->right );
  };
 visit( visit , f_map.root() );
 }

template< std::totally_ordered T >
std::size_t IntervalTree< T >::count( const T & x ) const
{
 std::size_t c = 0;
 stab( x , [ & ]( const T & , const T & ) { ++c; } );
 return( c );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File IntervalTree.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
