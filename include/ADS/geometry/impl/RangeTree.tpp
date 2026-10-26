/*--------------------------------------------------------------------------*/
/*-------------------------- File RangeTree.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RangeTree [see RangeTree.h], with their contracts.
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
 requires std::numeric_limits< T >::is_specialized
auto RangeTree< T >::counted( std::vector< Point > & P )
 -> std::vector< std::pair< Point , std::size_t > >
{
 std::sort( P.begin() , P.end() );
 std::vector< std::pair< Point , std::size_t > > B;
 for( const auto & p : P )
  if( ( ! B.empty() ) && ( B.back().first == p ) )
   ++B.back().second;
  else
   B.emplace_back( p , 1 );
 return( B );
 }

template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
RangeTree< T >::RangeTree( std::vector< Point > P )
{
 const auto B = counted( P );
 f_map.multiInsert( B );
 f_n = P.size();
 }

template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
template< Executor E >
RangeTree< T >::RangeTree( E & ex , std::vector< Point > P )
{
 const auto B = counted( P );
 f_map.multiInsert( ex , B );
 f_n = P.size();
 }

template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
void RangeTree< T >::insert( const T & x , const T & y )
{
 const Point p( x , y );
 f_map.insert( p , f_map.lookup( p ).value_or( 0 ) + 1 );
 ++f_n;
 }

template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
void RangeTree< T >::remove( const T & x , const T & y )
{
 const Point p( x , y );
 const auto c = f_map.lookup( p );
 if( ! c )
  return;
 if( *c == 1 ) f_map.remove( p );
 else f_map.insert( p , *c - 1 );
 --f_n;
 }

template< std::totally_ordered T >
 requires std::numeric_limits< T >::is_specialized
std::size_t RangeTree< T >::count( const T & x1 , const T & x2 ,
                                   const T & y1 , const T & y2 ) const
{
 if( ( x2 < x1 ) || ( y2 < y1 ) )
  return( 0 );
 using L = std::numeric_limits< T >;
 std::size_t c = 0;
 f_map.augVisit( Point( x1 , L::lowest() ) , Point( x2 , L::max() ) ,
                 [ & ]( const std::vector< T > & Y ) {
  c += std::size_t( std::upper_bound( Y.begin() , Y.end() , y2 ) -
                    std::lower_bound( Y.begin() , Y.end() , y1 ) );
  } );
 return( c );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File RangeTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
