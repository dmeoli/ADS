/*--------------------------------------------------------------------------*/
/*--------------------------- File Intervals.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Intervals [see Intervals.h], with their contracts.
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

namespace detail {

/// each interval \f$ [ a , b ) \f$ has \f$ a < b \f$
inline bool proper( const std::vector< std::pair< long , long > > & I )
{
 return( std::all_of( I.begin() , I.end() , []( const auto & x ) {
  return( x.first < x.second ); } ) );
 }

}  // end( namespace detail )

inline std::vector< std::size_t > independentSet(
                           const std::vector< std::pair< long , long > > & I )
{
 ADS_PRE( detail::proper( I ) );
 std::vector< std::size_t > by( I.size() );
 std::iota( by.begin() , by.end() , 0 );
 std::stable_sort( by.begin() , by.end() ,
                   [ & ]( std::size_t x , std::size_t y ) {
                    return( I[ x ].second < I[ y ].second ); } );
 std::vector< std::size_t > S;
 for( std::size_t i : by )
  if( S.empty() || ( I[ i ].first >= I[ S.back() ].second ) )
   S.push_back( i );
 std::sort( S.begin() , S.end() );
 ADS_AUDIT( ( [ & ] {                     // pairwise disjoint
  for( std::size_t x = 0 ; x < S.size() ; ++x )
   for( std::size_t y = x + 1 ; y < S.size() ; ++y )
    if( ( I[ S[ x ] ].first < I[ S[ y ] ].second ) &&
        ( I[ S[ y ] ].first < I[ S[ x ] ].second ) )
     return( false );
  return( true ); }() ) );
 return( S );
 }

inline std::vector< int > intervalColoring(
                           const std::vector< std::pair< long , long > > & I )
{
 ADS_PRE( detail::proper( I ) );
 std::vector< std::pair< long , long > > E;      // ( end , -i-1 or +i+1 )
 for( std::size_t i = 0 ; i < I.size() ; ++i ) {
  E.emplace_back( I[ i ].first , long( i ) + 1 );
  E.emplace_back( I[ i ].second , -long( i ) - 1 );
  }
 std::sort( E.begin() , E.end() );              // a right end first
 std::vector< int > color( I.size() , -1 ) , free;
 int used = 0;
 for( auto [ x , e ] : E )
  if( e > 0 ) {
   if( free.empty() )
    free.push_back( used++ );
   color[ e - 1 ] = free.back();
   free.pop_back();
   }
  else
   free.push_back( color[ -e - 1 ] );
 ADS_AUDIT( ( [ & ] {                     // intersecting ones apart
  for( std::size_t x = 0 ; x < I.size() ; ++x )
   for( std::size_t y = x + 1 ; y < I.size() ; ++y )
    if( ( I[ x ].first < I[ y ].second ) && ( I[ y ].first < I[ x ].second )
        && ( color[ x ] == color[ y ] ) )
     return( false );
  return( true ); }() ) );
 return( color );
 }

inline std::vector< double > towers( std::vector< double > k , double d )
{
 ADS_PRE( d >= 0 );
 std::sort( k.begin() , k.end() );
 std::vector< double > S;
 for( double x : k )
  if( S.empty() || ( x > S.back() + d ) )
   S.push_back( x + d );
 ADS_AUDIT( std::all_of( k.begin() , k.end() , [ & ]( double x ) {
  return( std::any_of( S.begin() , S.end() , [ & ]( double t ) {
   return( std::abs( x - t ) <= d ); } ) ); } ) );
 return( S );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Intervals.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
